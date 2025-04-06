// src/endpoint_discovery.cpp

#include "endpoint_discovery.h"
#include "connection_manager.h" // For HTTP requests
#include "config_manager.h"     // For finding blockchain and adding endpoints
#include "ui.h"                 // For progress bar
#include "version.h"            // For program version

#include <iostream>
#include <string>
#include <vector>
#include <sstream>              // For parsing strings
#include <stdexcept>
#include <algorithm>            // For std::find_if, std::transform
#include <cctype>               // For ::tolower
#include <iomanip>              // For std::setw (if used in detailed logs)
#include <limits>               // For std::numeric_limits
#include <nlohmann/json.hpp>    // For JSON parsing
#include <regex>                // For URL parsing in helpers

// Use nlohmann::json with the shorter 'json' alias
using json = nlohmann::json;

// Define a prefix for log messages originating from this module
#define LOG_PREFIX "    [Discovery] "

// Define the namespace for endpoint discovery functionalities
namespace neozork::endpoint_discovery {

// --- Utility functions ---

// Trim leading and trailing whitespace from a string
std::string trim_string(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

// Remove leading and trailing double or single quotes from a string
std::string trim_quotes(const std::string& str) {
    if (str.length() >= 2 && ((str.front() == '"' && str.back() == '"') || (str.front() == '\'' && str.back() == '\''))) {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

// Check if a URL string contains a placeholder like ${API_KEY}
bool contains_placeholder(const std::string& url) {
    return url.find("${") != std::string::npos && url.find("}") != std::string::npos;
}

// Determine the connection protocol type (scheme) from a URL string
std::optional<std::string> get_connection_type_from_url(const std::string& url) {
    if (url.rfind("https://", 0) == 0) return "https";
    if (url.rfind("wss://", 0) == 0) return "wss";
    if (url.rfind("http://", 0) == 0) return "http";
    if (url.rfind("ws://", 0) == 0) return "ws";
    if (url.rfind("ipc://", 0) == 0) return "ipc";
    // Return nullopt if scheme is missing or not recognized
    return std::nullopt;
}

// Extract the name of a placeholder (e.g., API_KEY) from a URL like wss://.../${API_KEY}
std::optional<std::string> extract_placeholder_name(const std::string& url) {
    size_t start_pos = url.find("${");
    if (start_pos != std::string::npos) {
        size_t end_pos = url.find("}", start_pos);
        // Ensure closing brace is found after opening one
        if (end_pos != std::string::npos) {
            // Extract the content between ${ and }
            return url.substr(start_pos + 2, end_pos - (start_pos + 2));
        }
    }
    // Return nullopt if no valid placeholder found
    return std::nullopt;
}


// --- Parsing functions ---

// Parse content assuming Chainlist/ChainID JSON format (array of objects with chainId, name, rpc fields)
std::vector<std::string> parse_chainlist_rpcs_json(const std::string& content, int target_chain_id) {
    std::vector<std::string> urls;
    // Log the start of parsing for the specific chain ID
    std::cout << LOG_PREFIX << "Parsing as Chainlist/ChainID JSON for target ID: " << target_chain_id << "..." << std::endl;
    // Ensure target ID is valid for filtering
    if (target_chain_id <= 0) {
        std::cerr << LOG_PREFIX << "WARNING: Invalid target Chain ID " << target_chain_id << ". Cannot filter." << std::endl;
        return urls; // Return empty vector
    }
    try {
        // Parse the input string content into a JSON object
        json j = json::parse(content);
        // Expect a top-level array of chain objects
        if (!j.is_array()) {
            std::cerr << LOG_PREFIX << "ERROR: Expected JSON array from source, got " << j.type_name() << std::endl;
            return urls; // Return empty vector on format mismatch
        }
        int found_rpcs_count = 0;
        bool target_chain_found = false;
        // Iterate through chain objects in the array
        for (const auto& chain_obj : j) {
            if (!chain_obj.is_object()) continue; // Skip non-objects
            // Check if this object contains the target chain ID
            if (chain_obj.contains("chainId")) {
                // Get the ID, defaulting to -1 if not convertible
                int current_id = chain_obj.value("chainId", -1);
                // Compare with the target ID
                if (current_id == target_chain_id) {
                    target_chain_found = true;
                    std::string name = chain_obj.value("name", "?"); // Get name for logging
                    // Log that the target chain was found
                    // std::cout << LOG_PREFIX << "Found target chain: '" << name << "'." << std::endl; // Less verbose log
                    // Check if it has an array of RPCs ("rpc" key)
                    if (chain_obj.contains("rpc") && chain_obj.at("rpc").is_array()) {
                        const auto& rpc_array = chain_obj.at("rpc");
                        // Iterate through RPC entries (can be strings or objects)
                        for (const auto& rpc_item : rpc_array) {
                            std::string url;
                            // Extract URL string regardless of whether item is string or object
                            if (rpc_item.is_string()) {
                                url = rpc_item.get<std::string>();
                            } else if (rpc_item.is_object() && rpc_item.contains("url")) {
                                url = rpc_item.value("url", "");
                            }
                            // Add URL if valid and not a placeholder
                            if (!url.empty() && !contains_placeholder(url)) {
                                urls.push_back(url);
                                found_rpcs_count++;
                            }
                        }
                    } else {
                        std::cout << LOG_PREFIX << "WARNING: Target chain '" << name << "' has no 'rpc' array." << std::endl;
                    }
                    break; // Found the target chain, no need to search further
                }
            }
        }
        if (!target_chain_found) std::cout << LOG_PREFIX << "Target chain ID " << target_chain_id << " not found in source." << std::endl;
        std::cout << LOG_PREFIX << "Finished parsing source. Added " << found_rpcs_count << " RPC URLs." << std::endl;
    } catch (const json::parse_error& e) {
        std::cerr << LOG_PREFIX << "ERROR parsing source JSON: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        // Catch other potential errors during JSON processing
        std::cerr << LOG_PREFIX << "ERROR processing source JSON: " << e.what() << std::endl;
    }
    return urls;
}

// Parse content assuming each non-empty, non-comment line is a URL
std::vector<std::string> parse_simple_url_list(const std::string& content) {
    std::cout << LOG_PREFIX << "Parsing content as simple URL list..." << std::endl;
    std::vector<std::string> urls;
    std::stringstream ss(content);
    std::string line;
    int line_count = 0;
    // Read line by line
    while (std::getline(ss, line)) {
        line_count++;
        std::string processed_line = trim_string(line);
        // Add if not empty and not a comment line starting with '#'
        if (!processed_line.empty() && processed_line[0] != '#') {
            // Remove potential quotes around the URL
            urls.push_back(trim_quotes(processed_line));
        }
    }
    std::cout << LOG_PREFIX << "Parsed " << urls.size() << " URLs from " << line_count << " lines." << std::endl;
    return urls;
}

// Parse content assuming ethereum-lists JSON format (object with "rpc" array of strings)
std::vector<std::string> parse_ethereum_lists_json(const std::string& content) {
    std::cout << LOG_PREFIX << "Attempting to parse as ethereum-lists JSON..." << std::endl;
    std::vector<std::string> urls;
    try {
        json j = json::parse(content);
        // Check for expected structure: an object containing an "rpc" array
        if (j.is_object() && j.contains("rpc") && j.at("rpc").is_array()) {
            // Iterate through the "rpc" array
            for (const auto& item : j.at("rpc")) {
                // Expect strings in the array
                if (item.is_string()) {
                    std::string url = item.get<std::string>();
                    // Add the URL if it's not empty and not a placeholder
                    if (!url.empty() && !contains_placeholder(url)) {
                        urls.push_back(url);
                    } else if (!url.empty()) {
                        // Log skipped placeholder URLs
                        // std::cout << LOG_PREFIX << "Skipping placeholder URL from ethereum-lists: " << url << std::endl; // Less verbose
                    }
                } else {
                    std::cerr << LOG_PREFIX << "WARNING: Non-string item found in 'rpc' array." << std::endl;
                }
            }
        } else {
            std::cerr << LOG_PREFIX << "WARNING: Invalid ethereum-lists format (expected object with 'rpc' array)." << std::endl;
        }
    } catch (const json::parse_error& e) {
        std::cerr << LOG_PREFIX << "ERROR parsing ethereum-lists JSON: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << LOG_PREFIX << "ERROR processing ethereum-lists JSON: " << e.what() << std::endl;
    }
    std::cout << LOG_PREFIX << "Extracted " << urls.size() << " URLs from ethereum-lists." << std::endl;
    return urls;
}


// --- Helper: Find chain name from ID ---

/**
 * @brief Parses chain list JSON content and finds the name for a given chain ID.
 * Expects JSON structure similar to chainid.network/chains.json (array of objects).
 * @param json_content The JSON string content.
 * @param target_id The network ID to search for.
 * @return std::optional<std::string> The found name, or nullopt if not found or error.
 */
std::optional<std::string> find_chain_name_from_id(const std::string& json_content, int target_id) {
    // ID must be positive
    if (target_id <= 0) return std::nullopt;
    
    try {
        json j = json::parse(json_content);
        // Ensure the top level is an array
        if (!j.is_array()) {
            std::cerr << LOG_PREFIX << "WARN: Chain list JSON for name lookup is not an array." << std::endl;
            return std::nullopt;
        }
        
        // Iterate through the array of chain objects
        for (const auto& chain_obj : j) {
            // Check if it's an object and contains the "chainId" key
            if (chain_obj.is_object() && chain_obj.contains("chainId")) {
                // Get the ID value, default to -1 if not convertible
                int current_id = chain_obj.value("chainId", -1);
                // If the ID matches the target ID
                if (current_id == target_id) {
                    // --- CORRECTED SECTION ---
                    // Check if the "name" key exists and its value is a string
                    if (chain_obj.contains("name") && chain_obj.at("name").is_string()) {
                        // Return the name string wrapped in std::optional
                        return chain_obj.at("name").get<std::string>();
                    } else {
                        // Name key is missing or is not a string type
                        std::cerr << LOG_PREFIX << "WARN: Found matching ID " << target_id << " but 'name' key is missing or not a string." << std::endl;
                        return std::nullopt; // Return empty optional as name is invalid/missing
                    }
                    // --- END CORRECTED SECTION ---
                }
            }
        }
    } catch (const json::parse_error& e) {
        // Log JSON parsing errors
        std::cerr << LOG_PREFIX << "WARN: Failed to parse chain list JSON for name lookup: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        // Log other potential exceptions during processing
        std::cerr << LOG_PREFIX << "WARN: Error during chain name lookup: " << e.what() << std::endl;
    }
    
    // Return empty optional if the ID was not found in the list or an error occurred
    return std::nullopt;
}

// --- Helper: Download master chain list ---

/**
 * @brief Downloads the master list of chains (e.g., from chainid.network).
 * Used primarily to look up chain names based on IDs.
 * @return std::optional<std::string> The JSON content as a string if successful, otherwise nullopt.
 */
std::optional<std::string> download_master_chain_list() {
    // URL for the master list
    std::string list_url = "https://chainid.network/chains.json";
    std::cout << LOG_PREFIX << "Attempting to download master chain list from: " << list_url << " (for name lookup)" << std::endl;
    
    // Regex to parse scheme, host, and optional path from the URL
    // Supports http and https
    std::regex url_regex(R"(^(https?)://([^/]+)(/.*)?$)");
    std::smatch match;
    std::string scheme, host, path = "/"; // Default path is root
    
    // Match the URL and extract components
    if (std::regex_match(list_url, match, url_regex) && match.size() >= 3) {
        scheme = match[1].str(); // http or https
        host = match[2].str();   // e.g., chainid.network
        // Check if path component exists
        if (match.size() >= 4 && match[3].matched) {
            path = match[3].str();   // e.g., /chains.json
        }
    } else {
        // Log error if URL cannot be parsed
        std::cerr << LOG_PREFIX << "ERROR: Could not parse master chain list URL: " << list_url << std::endl;
        return std::nullopt;
    }
    
    // Prepare standard HTTP headers for the request
    neozork::connection_manager::http_headers headers = {
        {"User-Agent", "NeoZorK3_Discovery_Bot/" + neozork::PROGRAM_VERSION},
        {"Accept", "application/json, */*"} // Accept JSON primarily
    };
    
    // Perform the download using the connection manager
    neozork::connection_manager::connection_result result;
    if (scheme == "https") {
        result = neozork::connection_manager::https_get(host, path, headers);
    } else {
        // Currently only HTTPS is supported for this specific download
        std::cerr << LOG_PREFIX << "ERROR: HTTP download not implemented for master chain list." << std::endl;
        return std::nullopt;
    }
    
    // Check if the download was successful (no client error and body received)
    if (!result.error_message && result.body.has_value()) {
        std::cout << LOG_PREFIX << "Master chain list downloaded successfully." << std::endl;
        // Return the downloaded JSON content
        return result.body;
    } else {
        // Log download failure details
        std::cerr << LOG_PREFIX << "ERROR: Failed to download master chain list: "
        << (result.error_message ? *result.error_message : "Unknown error") << std::endl;
        return std::nullopt;
    }
}


// --- Main Discovery Function ---

/**
 * @brief Discovers RPC endpoints for a specified blockchain from given sources.
 * Handles finding/creating the blockchain entry in the config, potentially looking up names for IDs.
 * Downloads data from sources, parses URLs, and adds unique endpoints to the config.
 * @param blockchain_name_or_id The name or network ID provided by the user.
 * @param sources Vector of source strings (keywords like 'chain', 'eth' or direct URLs).
 * @param config The main configuration object (mutable).
 * @return True if the discovery process completed (doesn't guarantee endpoints were added), false on critical error.
 */
bool discover_endpoints(
                        const std::string& blockchain_name_or_id,
                        const std::vector<std::string>& sources,
                        neozork::config_manager::struct_config& config)
{
    std::cout << "[Discovery] Starting discovery for: '" << blockchain_name_or_id << "'..." << std::endl;
    
    // --- 1. Find or Create Blockchain Entry ---
    // Attempt to find the blockchain in the config by the provided name or ID
    auto blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
    // Pointer to the blockchain info struct in the config (will be set below)
    neozork::config_manager::struct_blockchain_info* blockchain_info_ptr = nullptr;
    // Final network ID determined for this operation
    int target_network_id = 0;
    // Flag to indicate if we added a new blockchain entry to the config
    bool is_new_blockchain = false;
    
    // If the blockchain was not found in the config
    if (!blockchain_ref_opt) {
        is_new_blockchain = true; // Mark that we are creating a new entry
        std::cout << LOG_PREFIX << "'" << blockchain_name_or_id << "' not found. Trying to create..." << std::endl;
        // Create a new structure to hold info
        neozork::config_manager::struct_blockchain_info new_bc;
        // Default name is the user input
        std::string found_name = blockchain_name_or_id;
        // Flag to check if input was successfully interpreted as an ID
        bool input_is_id = false;
        
        // Try to interpret the input as a number (potential network ID)
        try {
            long long id_ll = std::stoll(blockchain_name_or_id);
            // Check if the parsed ID is within a valid integer range
            if (id_ll > 0 && id_ll <= std::numeric_limits<int>::max()) {
                target_network_id = static_cast<int>(id_ll);
                input_is_id = true;
                std::cout << LOG_PREFIX << "Input interpreted as network ID: " << target_network_id << std::endl;
            } else {
                // Numeric input, but outside valid range
                target_network_id = 0;
                std::cerr << LOG_PREFIX << "WARN: Input '" << blockchain_name_or_id << "' invalid as network ID. Treating as name, ID unknown (0)." << std::endl;
            }
        } catch (...) {
            // Input was not a number, treat as a name
            target_network_id = 0;
            input_is_id = false;
            // std::cout << LOG_PREFIX << "Input is not a number. Treating as name." << std::endl; // Less verbose
        }
        
        // If the input was identified as an ID, try to find the corresponding name
        if (input_is_id) {
            // Download the master chain list to perform the lookup
            std::optional<std::string> chain_list_json = download_master_chain_list();
            // If download succeeded
            if (chain_list_json) {
                // Try to find the name in the downloaded JSON
                std::optional<std::string> name_from_list = find_chain_name_from_id(*chain_list_json, target_network_id);
                // If a name was found
                if (name_from_list) {
                    found_name = *name_from_list; // Use the name found from the list
                    std::cout << LOG_PREFIX << "Found Name '" << found_name << "' for ID " << target_network_id << "." << std::endl;
                } else {
                    // Name not found for this ID in the list
                    std::cerr << LOG_PREFIX << "WARN: Name for ID " << target_network_id << " not found. Using ID as name." << std::endl;
                    found_name = std::to_string(target_network_id); // Use the ID number as the name string
                }
            } else {
                // Failed to download the list for lookup
                std::cerr << LOG_PREFIX << "WARN: Failed list download for name lookup. Using ID as name." << std::endl;
                found_name = std::to_string(target_network_id); // Fallback to ID as name
            }
        } else {
            // Input was a name, ID remains unknown (0)
            target_network_id = 0;
            std::cerr << LOG_PREFIX << "WARN: Input is name. Network ID unknown (0). Filtering might fail." << std::endl;
        }
        
        // Set the final determined name and ID for the new entry
        new_bc.name = found_name;
        new_bc.network_id = target_network_id;
        
        // Attempt to add this new blockchain entry to the main config
        if (neozork::config_manager::add_blockchain(config, new_bc)) {
            std::cout << LOG_PREFIX << "Added new blockchain '" << new_bc.name << "' (ID: " << new_bc.network_id << ")." << std::endl;
            // We need a pointer to the newly added struct, so re-find it (by name should work now)
            auto re_find = neozork::config_manager::find_blockchain(config, new_bc.name);
            if (re_find) {
                blockchain_info_ptr = &re_find.value().get();
            } else {
                // This indicates a serious internal inconsistency
                std::cerr << "[Discovery] CRITICAL: Cannot re-find just added blockchain!" << std::endl;
                return false;
            }
        } else {
            // add_blockchain returned false, usually means it already existed (name or ID conflict)
            std::cerr << "[Discovery] ERROR: Failed to add blockchain (already exists?). Trying to find existing..." << std::endl;
            // Try to find the existing one again to proceed
            auto existing = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
            if (existing) {
                blockchain_info_ptr = &existing.value().get();
                is_new_blockchain = false; // Mark that we are using an existing one after all
                std::cout << LOG_PREFIX << "Proceeding with existing blockchain found: '" << blockchain_info_ptr->name << "'." << std::endl;
            } else {
                // Failed to add and cannot find it either
                std::cerr << "[Discovery] CRITICAL: Cannot add or find blockchain!" << std::endl;
                return false;
            }
        }
    } else {
        // Blockchain was found initially
        blockchain_info_ptr = &blockchain_ref_opt.value().get();
        target_network_id = blockchain_info_ptr->network_id; // Get ID from the existing entry
        std::cout << LOG_PREFIX << "Found existing blockchain '" << blockchain_info_ptr->name << "' (ID: " << target_network_id << ")." << std::endl;
    }
    
    // Ensure we have a valid pointer to the blockchain info struct
    if (!blockchain_info_ptr) {
        std::cerr << "[Discovery] CRITICAL: No blockchain info pointer." << std::endl;
        return false;
    }
    // Use a reference for easier access
    neozork::config_manager::struct_blockchain_info& blockchain_info = *blockchain_info_ptr;
    // Ensure target_network_id reflects the final ID in the struct (important if it was 0)
    target_network_id = blockchain_info.network_id;
    std::cout << LOG_PREFIX << "Processing discovery for '" << blockchain_info.name << "' using Network ID: " << target_network_id << std::endl;
    
    // --- 2. Process Discovery Sources ---
    // Counter for newly added endpoint entries for this run
    int total_added_count = 0;
    // Start the progress bar for processing sources
    neozork::ui::start_progress("Processing Sources", static_cast<long long>(sources.size()));
    // Index for updating the progress bar (1-based)
    int source_index = 0;
    
    // Loop through each provided source (keyword or URL)
    for (const std::string& source : sources) {
        source_index++; // Increment index
        
        // Vector to store URLs parsed from the current source
        std::vector<std::string> raw_urls;
        // URL to download content from for this source
        std::string url_to_download;
        // Enum to determine which parsing function to use
        enum class ParserType { UNKNOWN, CHAINLIST, ETH_LISTS, SIMPLE_LIST, AUTO_DETECT };
        ParserType parser_to_use = ParserType::UNKNOWN;
        // Convert source keyword to lowercase for easier matching
        std::string lower_source = source;
        std::transform(lower_source.begin(), lower_source.end(), lower_source.begin(), [](unsigned char c){ return std::tolower(c); });
        
        // --- Determine parser type and download URL based on source keyword/URL ---
        // Use short keywords 'chain' and 'eth'
        if (lower_source == "chain") { // Changed from "chainlist"
            url_to_download = "https://chainid.network/chains.json"; // Main list URL
            parser_to_use = ParserType::CHAINLIST;
        } else if (lower_source == "eth") { // Changed from "ethereum-lists"
            url_to_download = "https://raw.githubusercontent.com/ethereum-lists/chains/master/_data/chains/eip155-1.json"; // Fixed URL for mainnet
            parser_to_use = ParserType::ETH_LISTS;
            // Warn if target network ID is not Ethereum mainnet
            if (target_network_id != 1 && target_network_id != 0) { // Allow ID 0 as unknown
                std::cerr << LOG_PREFIX << "WARNING: Using 'eth' source but target ID is " << target_network_id << ", not 1. Results might be incorrect." << std::endl;
            }
        } else if (source.rfind("https://", 0) == 0 || source.rfind("http://", 0) == 0) { // Check if source is a URL
            url_to_download = source;
            parser_to_use = ParserType::AUTO_DETECT; // Try to guess format based on content
        } else {
            // Unknown source keyword
            std::cerr << "\n" << LOG_PREFIX << "WARN: Unknown source type: '" << source << "'. Valid keywords: 'chain', 'eth' or a URL. Skipping." << std::endl;
            neozork::ui::update_progress(source_index); // Update progress even if skipping
            continue; // Skip to the next source
        }
        
        // --- Download Content ---
        neozork::connection_manager::connection_result download_result;
        if (!url_to_download.empty()) {
            // Parse host and path from the download URL
            size_t host_start = url_to_download.find("://");
            if (host_start == std::string::npos) {
                std::cerr << "\n" << LOG_PREFIX << "ERROR: Invalid URL scheme: " << url_to_download << std::endl;
                neozork::ui::update_progress(source_index); continue;
            }
            host_start += 3;
            size_t path_start = url_to_download.find('/', host_start);
            std::string host, path;
            if (path_start == std::string::npos) { host = url_to_download.substr(host_start); path = "/"; }
            else { host = url_to_download.substr(host_start, path_start - host_start); path = url_to_download.substr(path_start); }
            if (host.empty()) {
                std::cerr << "\n" << LOG_PREFIX << "ERROR: Could not extract host from: " << url_to_download << std::endl;
                neozork::ui::update_progress(source_index); continue;
            }
            
            // Perform HTTPS GET request
            // std::cout << LOG_PREFIX << "Downloading from " << host << path << "..." << std::endl; // Less verbose
            neozork::connection_manager::http_headers download_headers = {{"User-Agent", "N3D"},{"Accept", "*/*"}};
            download_result = neozork::connection_manager::https_get(host, path, download_headers);
            
            // Check if download failed at connection level or returned empty body
            if (!download_result.body.has_value() || download_result.error_message) {
                std::cerr << "\n" << LOG_PREFIX << "ERROR downloading " << url_to_download
                << (download_result.error_message ? " ("+ *download_result.error_message + ")" : "")
                << ". Skipping source." << std::endl;
                neozork::ui::update_progress(source_index); // Update progress
                continue; // Skip to next source
            }
        } else { continue; } // Skip if URL was somehow empty
        
        // --- Parse Downloaded Content ---
        if (download_result.body) {
            const std::string& response_body = download_result.body.value();
            // std::cout << LOG_PREFIX << "Parsing content..." << std::endl; // Less verbose
            
            // Select appropriate parser based on source type
            switch (parser_to_use) {
                case ParserType::CHAINLIST:
                    raw_urls = parse_chainlist_rpcs_json(response_body, target_network_id);
                    break;
                case ParserType::ETH_LISTS:
                    raw_urls = parse_ethereum_lists_json(response_body);
                    break;
                case ParserType::AUTO_DETECT:
                    // Try parsing as JSON (array or object), fallback to simple list
                    try {
                        if (json::accept(response_body)) {
                            json jt = json::parse(response_body);
                            if (jt.is_array()) raw_urls = parse_chainlist_rpcs_json(response_body, target_network_id);
                            else if (jt.is_object()) raw_urls = parse_ethereum_lists_json(response_body);
                            // If JSON parse didn't yield URLs, try simple list as fallback
                            if (raw_urls.empty()) raw_urls = parse_simple_url_list(response_body);
                        } else {
                            raw_urls = parse_simple_url_list(response_body); // Not JSON, assume simple list
                        }
                    } catch (...) { // Catch any error during parsing/detection
                        raw_urls = parse_simple_url_list(response_body); // Fallback
                    }
                    if (raw_urls.empty()) std::cerr << "\n" << LOG_PREFIX << "WARN: Auto-detect failed for " << source << std::endl;
                    break;
                case ParserType::SIMPLE_LIST:
                    raw_urls = parse_simple_url_list(response_body);
                    break;
                default: /* Should not happen */ break;
            }
        } // End content parsing
        
        // --- 3. Process Parsed URLs ---
        int added_from_this_source = 0;
        // Loop through URLs obtained from the current source
        for (const auto& raw_url : raw_urls) {
            // Clean up the URL string
            std::string cleaned_url = trim_string(trim_quotes(raw_url));
            // Skip if empty or contains placeholders
            if (cleaned_url.empty() || contains_placeholder(cleaned_url)) continue;
            // Determine connection type from URL scheme
            auto connection_type_opt = get_connection_type_from_url(cleaned_url);
            if (!connection_type_opt) continue; // Skip if type unknown
            std::string connection_type = connection_type_opt.value();
            
            // Create a minimal endpoint structure with just this URL/type
            neozork::config_manager::struct_endpoint new_endpoint_entry;
            new_endpoint_entry.connection_urls[connection_type] = cleaned_url;
            
            // Attempt to add this endpoint to the blockchain's list; handles duplicates
            if (neozork::config_manager::add_endpoint(blockchain_info, new_endpoint_entry)) {
                added_from_this_source++; // Increment count if successfully added (not duplicate)
            }
        }
        total_added_count += added_from_this_source; // Add count from this source to total
        // Update progress bar after processing this source
        neozork::ui::update_progress(source_index);
        
    } // End loop over sources
    
    // Finish the progress bar display
    neozork::ui::finish_progress();
    
    std::cout << "[Discovery] Discovery finished for '" << blockchain_info.name << "'. Added " << total_added_count << " new endpoint entries." << std::endl;
    
    // --- 4. Save Config ---
    if (total_added_count > 0) {
        try {
            std::cout << "[Discovery] Saving updated configuration (new endpoints added)..." << std::endl;
            neozork::config_manager::save_config(config);
            std::cout << "[Discovery] Configuration saved successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Discovery] ERROR saving config after discovery: " << e.what() << std::endl;
            return false;
        }
    } else {
        // No new endpoints added
        if (is_new_blockchain) {
            std::cout << "[Discovery] No endpoints found for the newly specified blockchain '" << blockchain_info.name << "'. Configuration not saved." << std::endl;
        } else {
            std::cout << "[Discovery] No *new* endpoints added for existing blockchain '" << blockchain_info.name << "'. Configuration file not modified." << std::endl;
        }
    }
    
    // Return true indicating the process completed
    return true;
}

} // namespace neozork::endpoint_discovery
