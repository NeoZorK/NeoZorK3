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


// --- Main discovery function ---

bool discover_endpoints(
                        const std::string& blockchain_name,
                        const std::vector<std::string>& sources,
                        neozork::config_manager::struct_config& config)
{
    std::cout << "Starting endpoint discovery for blockchain: " << blockchain_name << "..." << std::endl;
    
    // 1. Find or create the target blockchain in the config
    auto blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name);
    if (!blockchain_ref_opt) {
        std::cout << "Blockchain '" << blockchain_name << "' not found. Creating..." << std::endl;
        neozork::config_manager::struct_blockchain_info new_bc;
        new_bc.name = blockchain_name;
        // TODO: Try to determine network_id from source or name later?
        new_bc.network_id = 0; // Defaulting to 0 for now
        if (!neozork::config_manager::add_blockchain(config, new_bc)) {
            // This should not happen if find_blockchain failed, but check anyway
            throw std::runtime_error("Failed to add new blockchain '" + blockchain_name + "' to config.");
        }
        // Find the newly added blockchain again to get the reference
        blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name);
        if (!blockchain_ref_opt) { // Should definitely be found now
            throw std::runtime_error("Internal error: Failed to find newly created blockchain '" + blockchain_name + "'.");
        }
    }
    // Get a reference to modify the blockchain info
    neozork::config_manager::struct_blockchain_info& blockchain_info = blockchain_ref_opt.value().get();
    
    int added_count = 0;
    int updated_count = 0; // Count updated endpoints (e.g., added new connection type)
    
    // 2. Process each source URL/keyword
    for (const std::string& source : sources) {
        std::cout << "  Processing source: " << source << std::endl;
        std::vector<std::string> raw_urls; // Holds URLs parsed from this source
        
        // --- Download content if source is a URL ---
        if (source.rfind("https://", 0) == 0 || source.rfind("http://", 0) == 0) {
            // Extract host and path
            size_t host_start = source.find("://") + 3;
            size_t path_start = source.find('/', host_start);
            if (path_start == std::string::npos) {
                std::cerr << "    ERROR: Invalid URL format: " << source << std::endl;
                continue; // Skip to next source
            }
            std::string host = source.substr(host_start, path_start - host_start);
            std::string path = source.substr(path_start);
            
            // Use connection_manager to get content
            neozork::connection_manager::http_headers headers = {{"User-Agent", "NeoZorK3_Discovery_Bot/0.1"}};
            // --- Make the HTTP GET call ---
            auto response_body_opt = neozork::connection_manager::https_get(host, path, headers);
            
            // --- Check and parse the response ---
            if (response_body_opt) { // Check if download was successful
                std::cout << "    Downloaded content from " << source << std::endl;
                bool is_json_source = (source.length() >= 5 && source.substr(source.length() - 5) == ".json");
                // Choose parser based on source type (simple list or ethereum-lists JSON)
                if (is_json_source) {
                    raw_urls = parse_ethereum_lists_json(response_body_opt.value());
                } else {
                    raw_urls = parse_simple_url_list(response_body_opt.value());
                }
                std::cout << "    Parsed " << raw_urls.size() << " potential URLs." << std::endl;
            } else { // http(s)_get failed
                std::cerr << "    ERROR: Failed to download content from " << source << std::endl;
                continue; // Skip to next source
            }
        } else {
            // TODO: Implement handling for other source types (keywords like "defillama", "chainlist", local files?)
            std::cerr << "    WARNING: Source type '" << source << "' not yet supported (only https/http URLs implemented)." << std::endl;
            continue; // Skip to next source
        }
        
        // 3. Process parsed URLs: clean, determine type/placeholder, add/update config
        for (const auto& raw_url : raw_urls) {
            // Apply cleaning (trim whitespace first, then quotes if needed)
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
            // Try to find if an endpoint already exists that *could* contain this URL
            // (e.g., by comparing hosts or a conceptual grouping - complex, skipping for now)
            // Current simple approach: Check if this exact URL+Type exists in *any* endpoint
            
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
                // For now, we always create a *new* endpoint entry.
                // TODO: Implement merging later if desired (find endpoint by host, add URL to its map).
                neozork::config_manager::struct_endpoint new_endpoint;
                new_endpoint.connection_urls[connection_type] = cleaned_url;
                if (placeholder_opt) {
                    new_endpoint.required_api_key_placeholder = placeholder_opt;
                }
                blockchain_info.endpoints.push_back(new_endpoint);
                added_count++;
            }
            // else: This exact URL/Type combo already exists in some endpoint entry, skip.
        } // end for raw_url
    } // end for source
    
    std::cout << "Endpoint discovery finished. Added " << added_count << " new endpoint entries for " << blockchain_name << "." << std::endl;
    // Note: updated_count is not used currently as we don't merge URLs yet.
    
    // 4. Save config if changes were made
    if (added_count > 0 /*|| updated_count > 0 */) {
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
