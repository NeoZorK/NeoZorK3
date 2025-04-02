#include "endpoint_discovery.h"
#include "connection_manager.h" // For HTTP requests
#include "config_manager.h"     // For finding blockchain and adding endpoints
#include <iostream>
#include <string>
#include <vector>
#include <sstream>   // For parsing strings
#include <stdexcept>
#include <algorithm> // For std::find_if, std::transform
#include <cctype>    // For ::tolower

// Include nlohmann/json as we will parse JSON
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace neozork::endpoint_discovery {

// --- Utility functions ---

// Trims leading and trailing whitespace from a string
std::string trim_string(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first) {
        return str; // String is all whitespace
    }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

// Trims surrounding single or double quotes from a string
std::string trim_quotes(const std::string& str) {
    if (str.length() >= 2 &&
        ((str.front() == '"' && str.back() == '"') ||
         (str.front() == '\'' && str.back() == '\''))) {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

// Checks if a URL contains API key placeholders like ${...}
bool contains_placeholder(const std::string& url) {
    return url.find("${") != std::string::npos && url.find("}") != std::string::npos;
}

// Determines the connection type ("https", "wss", etc.) from the URL scheme
std::optional<std::string> get_connection_type_from_url(const std::string& url) {
    if (url.rfind("https://", 0) == 0) return "https";
    if (url.rfind("wss://", 0) == 0) return "wss";
    if (url.rfind("http://", 0) == 0) return "http";
    if (url.rfind("ws://", 0) == 0) return "ws";
    if (url.rfind("ipc://", 0) == 0) return "ipc"; // For local nodes
    return std::nullopt;
}

// Extracts the placeholder name from a URL (e.g., "INFURA_API_KEY")
std::optional<std::string> extract_placeholder_name(const std::string& url) {
    size_t start_pos = url.find("${");
    if (start_pos != std::string::npos) {
        size_t end_pos = url.find("}", start_pos);
        if (end_pos != std::string::npos) {
            return url.substr(start_pos + 2, end_pos - (start_pos + 2));
        }
    }
    return std::nullopt;
}

// --- Parsing functions ---

// Parses a simple URL list (one URL per line, ignores # comments)
std::vector<std::string> parse_simple_url_list(const std::string& content) {
    std::vector<std::string> urls;
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        std::string processed_line = trim_string(line);
        // Skip empty lines and comments
        if (!processed_line.empty() && processed_line[0] != '#') {
            // Apply quote trimming here if lists might contain quoted URLs
            urls.push_back(trim_quotes(processed_line));
        }
    }
    return urls;
}

// Parses JSON from sources like ethereum-lists/chains format
std::vector<std::string> parse_ethereum_lists_json(const std::string& content) {
    std::vector<std::string> urls;
    try {
        json j = json::parse(content);
        // Find the "rpc" array and check it's an array
        if (j.contains("rpc") && j.at("rpc").is_array()) {
            for (const auto& item : j.at("rpc")) {
                if (item.is_string()) {
                    // Get URL string - no extra trimming/quoting needed if JSON is valid
                    urls.push_back(item.get<std::string>());
                }
            }
        } else {
            std::cerr << "    WARNING: JSON source does not contain a valid 'rpc' array." << std::endl;
        }
    } catch (const json::parse_error& e) {
        std::cerr << "    ERROR: Failed to parse JSON source: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "    ERROR: Error processing JSON source: " << e.what() << std::endl;
    }
    return urls;
}

// NEW parser for chainlist.org/rpcs.json
std::vector<std::string> parse_chainlist_rpcs_json(const std::string& content, int target_chain_id) {
    std::vector<std::string> urls;
    std::cout << "    Parsing Chainlist RPCs JSON for chain ID: " << target_chain_id << "..." << std::endl;
    try {
        json j = json::parse(content);
        if (!j.is_array()) {
            std::cerr << "    ERROR: Expected a JSON array from Chainlist source." << std::endl;
            return urls;
        }

        for (const auto& item : j) { // Iterate through the array of endpoint objects
            if (!item.is_object() || !item.contains("chain_id") || !item.contains("url")) {
                continue; // Skip invalid entries
            }

            try {
                // Use value() for safe extraction, in case type is wrong
                int current_chain_id = item.value("chain_id", -1);
                std::string current_url = item.value("url", "");

                if (current_chain_id == target_chain_id && !current_url.empty()) {
                    // Add URL if chain_id matches
                    urls.push_back(current_url);
                }
            } catch (const json::type_error& te) {
                // Type error extracting chain_id or url
                 std::cerr << "    WARNING: Type error processing Chainlist entry: " << te.what() << " - Entry: " << item.dump() << std::endl;
            }
        }
    } catch (const json::parse_error& e) {
         std::cerr << "    ERROR: Failed to parse Chainlist JSON source: " << e.what() << std::endl;
    } catch (const std::exception& e) {
         std::cerr << "    ERROR: Error processing Chainlist JSON source: " << e.what() << std::endl;
    }
    return urls;
}


// --- Main discovery function ---

bool discover_endpoints(
        const std::string& blockchain_name_or_id, // Renamed for clarity
        const std::vector<std::string>& sources,
        neozork::config_manager::struct_config& config)
{
    std::cout << "Starting endpoint discovery for blockchain: " << blockchain_name_or_id << "..." << std::endl;

    // 1. Find or create the target blockchain in the config
    auto blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
    if (!blockchain_ref_opt) {
        std::cout << "Blockchain '" << blockchain_name_or_id << "' not found. Creating..." << std::endl;
        neozork::config_manager::struct_blockchain_info new_bc;
        new_bc.name = blockchain_name_or_id; // Use the name provided
        try { // Try converting name_or_id to integer for network_id
            new_bc.network_id = std::stoi(blockchain_name_or_id);
        } catch (...) {
            // TODO: Try to determine network_id from source or name later?
            std::cerr << "    WARNING: Could not determine network ID for '" << blockchain_name_or_id << "'. Setting to 0." << std::endl;
            new_bc.network_id = 0; // Defaulting to 0 for now
        }

        if (!neozork::config_manager::add_blockchain(config, new_bc)) {
            throw std::runtime_error("Failed to add new blockchain '" + blockchain_name_or_id + "' to config.");
        }
        // Find the newly added blockchain again to get the reference
        blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
        if (!blockchain_ref_opt) { // Should definitely be found now
             throw std::runtime_error("Internal error: Failed to find newly created blockchain '" + blockchain_name_or_id + "'.");
        }
    }
    // Get a reference to modify the blockchain info
    neozork::config_manager::struct_blockchain_info& blockchain_info = blockchain_ref_opt.value().get();
    // Store the target network ID for use with Chainlist parser
    int target_network_id = blockchain_info.network_id;

    int added_count = 0;
    int updated_count = 0; // Count updated endpoints (e.g., added new connection type)

    // 2. Process each source URL/keyword
    for (const std::string& source : sources) {
        std::cout << "  Processing source: " << source << std::endl;
        std::vector<std::string> raw_urls; // Holds URLs parsed from this source

        // Define the URL to download based on source keyword or direct URL
        std::string url_to_download = source;
        bool is_json_source = false;
        bool is_chainlist_source = false;

        if (source == "chainlist") {
            url_to_download = "https://chainlist.org/rpcs.json";
            is_json_source = true;
            is_chainlist_source = true;
        } else if (source.length() >= 5 && source.substr(source.length() - 5) == ".json") {
            // Assume other .json URLs follow ethereum-lists format for now
            // TODO: Add more specific checks if other JSON formats are expected
            is_json_source = true;
        }

        // --- Download content if source is a URL ---
        if (url_to_download.rfind("https://", 0) == 0 || url_to_download.rfind("http://", 0) == 0) {
            // Extract host and path
            size_t host_start = url_to_download.find("://") + 3;
            size_t path_start = url_to_download.find('/', host_start);
            if (path_start == std::string::npos) {
                std::cerr << "    ERROR: Invalid URL format: " << url_to_download << std::endl;
                continue; // Skip to next source
            }
            std::string host = url_to_download.substr(host_start, path_start - host_start);
            std::string path = url_to_download.substr(path_start);

            // Use connection_manager to get content
            neozork::connection_manager::http_headers headers = {{"User-Agent", "NeoZorK3_Discovery_Bot/0.1"}};
            // --- Make the HTTP GET call ---
            auto response_body_opt = neozork::connection_manager::https_get(host, path, headers);

            // --- Check and parse the response ---
            if (response_body_opt) { // Check if download was successful
                std::cout << "    Downloaded content from " << url_to_download << std::endl;
                // Choose parser based on identified source type
                 if (is_chainlist_source) { // Check Chainlist first
                      raw_urls = parse_chainlist_rpcs_json(response_body_opt.value(), target_network_id);
                 } else if (is_json_source) { // Then check other JSON (ethereum-lists)
                     raw_urls = parse_ethereum_lists_json(response_body_opt.value());
                 } else { // Otherwise, assume simple list
                     raw_urls = parse_simple_url_list(response_body_opt.value());
                 }
                std::cout << "    Parsed " << raw_urls.size() << " potential URLs." << std::endl;
            } else { // http(s)_get failed
                std::cerr << "    ERROR: Failed to download content from " << url_to_download << std::endl;
                continue; // Skip to next source
            }
        } else {
            // TODO: Implement handling for other source types (keywords like "defillama", local files?)
            std::cerr << "    WARNING: Source type '" << source << "' not yet supported (only https/http URLs and 'chainlist' keyword implemented)." << std::endl;
            continue; // Skip to next source
        }

        // 3. Process parsed URLs: clean, determine type/placeholder, add/update config
        for (const auto& raw_url : raw_urls) {
            // Apply cleaning (trim whitespace first)
            // Note: JSON parser usually handles quotes, simple list parser does it too now.
            std::string cleaned_url = trim_string(raw_url);

            if (cleaned_url.empty()) {
                continue; // Skip empty lines
            }

            // Determine connection type (https, wss, etc.)
            auto connection_type_opt = get_connection_type_from_url(cleaned_url);
            if (!connection_type_opt) {
                std::cerr << "    WARNING: Cannot determine connection type for URL: " << cleaned_url << ". Skipping." << std::endl;
                continue;
            }
            std::string connection_type = connection_type_opt.value();

            // Extract API key placeholder, if any
            auto placeholder_opt = extract_placeholder_name(cleaned_url);
            if (placeholder_opt) {
                std::cout << "    INFO: URL requires API key (" << placeholder_opt.value() << "): " << cleaned_url << std::endl;
            }

            // --- Logic to add/update endpoint in config ---
            // Current simple approach: Check if this exact URL+Type exists in *any* endpoint entry.
            // If not, create a *new* endpoint entry just for this URL/Type.
            // Merging URLs (https/wss) for the same provider requires more complex logic (e.g., find by host).

            bool url_type_combo_exists = false;
            for(const auto& ep : blockchain_info.endpoints) {
                auto it = ep.connection_urls.find(connection_type);
                if (it != ep.connection_urls.end() && it->second == cleaned_url) {
                    url_type_combo_exists = true;
                    break;
                }
            }

            if (!url_type_combo_exists) {
                // This specific URL for this specific connection type is new.
                // Add it as a new entry in the endpoints list.
                // TODO: Implement merging later if desired (find endpoint by host, add URL to its map).
                neozork::config_manager::struct_endpoint new_endpoint;
                new_endpoint.connection_urls[connection_type] = cleaned_url; // Add the URL to the map
                if (placeholder_opt) {
                    new_endpoint.required_api_key_placeholder = placeholder_opt; // Store placeholder name
                }
                blockchain_info.endpoints.push_back(new_endpoint);
                added_count++;
            }
            // else: This exact URL/Type combo already exists in some endpoint entry, skip.

        } // end for raw_url
    } // end for source

    std::cout << "Endpoint discovery finished. Added " << added_count << " new endpoint entries for " << blockchain_info.name << "." << std::endl;
    // Note: updated_count is not used currently as we don't merge URLs yet.

    // 4. Save config if changes were made
    if (added_count > 0 /*|| updated_count > 0 */) { // Save if endpoints were added
        try {
            neozork::config_manager::save_config(config);
            std::cout << "Configuration saved successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "ERROR saving config after discovery: " << e.what() << std::endl;
            // Return false even if endpoints were added, as saving failed
            return false;
        }
    }
    return true; // Discovery process completed
}

} // namespace neozork::endpoint_discovery
