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
    
    // Log prefix for clarity in output
    std::vector<std::string> urls;
    
    // Add newline for separation from potential progress bar line
    std::cout << std::endl;
    
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
            return urls;
        }
        int found_rpcs_count = 0;
        bool target_chain_found = false;
        
        // Iterate through chain objects in the array
        for (const auto& chain_obj : j) {
            if (!chain_obj.is_object())
                continue;
            // Check if chainId exists, is a number, and matches the target_chain_id
            if (chain_obj.contains("chainId") && chain_obj.at("chainId").is_number_integer())
            {
                int current_id = chain_obj.at("chainId").get<int>();
                if (current_id == target_chain_id)
                {
                    target_chain_found = true;
                    std::string name = chain_obj.value("name", "?"); // Get name for logging
                    // Log that the target chain was found
                    // std::cout << LOG_PREFIX << "Found target chain: '" << name << "'." << std::endl; // Less verbose log
                    // Check if it has an array of RPCs ("rpc" key)
                    // --- Try accessing RPC URLs ---
                    // Define potential key names
                    const std::string rpc_key = "rpc";
                    const std::string rpcs_key = "rpcs"; // Potential alternative key name
                    
                    // Reference to hold the found array (if any)
                    const json* rpc_array_ptr = nullptr;
                    
                    // Try the primary key "rpc"
                    if (chain_obj.contains(rpc_key) && chain_obj.at(rpc_key).is_array()) {
                        rpc_array_ptr = &chain_obj.at(rpc_key);
                    }
                    // If not found, try the alternative key "rpcs"
                    else if (chain_obj.contains(rpcs_key) && chain_obj.at(rpcs_key).is_array()) {
                        rpc_array_ptr = &chain_obj.at(rpcs_key);
                        // Optional: Log that we used the alternative key
                        // std::cout << LOG_PREFIX << "Note: Used alternative key '" << rpcs_key << "' for RPCs for chain " << current_id << "." << std::endl;
                    }
                    
                    // Check if we found a valid RPC array using either key
                    if (rpc_array_ptr) {
                        const auto& rpc_array = *rpc_array_ptr; // Dereference the pointer
                        
                        // Iterate through RPC entries (can be strings or objects)
                        for (const auto& rpc_item : rpc_array) {
                            std::string url;
                            
                            // Extract URL string regardless of whether item is string or object
                            if (rpc_item.is_string()) {
                                url = rpc_item.get<std::string>();
                            } else if (rpc_item.is_object() && rpc_item.contains("url")) {
                                // Use value() for safety in case "url" is missing inside the object
                                url = rpc_item.value("url", "");
                            }
                            
                            // Add URL if valid, not a placeholder, and has a supported scheme
                            if (!url.empty() && !contains_placeholder(url) && get_connection_type_from_url(url).has_value()) {
                                urls.push_back(url);
                                found_rpcs_count++;
                            }
                        }
                    } else {
                        // Log if the target chain is found but doesn't have a valid 'rpc' or 'rpcs' array
                        std::cout << LOG_PREFIX << "WARNING: Target chain '" << name << "' (ID: " << current_id << ") found, but has no valid '" << rpc_key << "' or '" << rpcs_key << "' array field." << std::endl;
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
    
    // Add newline for separation
    std::cout << std::endl;
    
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
    
    // Add newline for separation
    std::cout << std::endl;
    
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


/**
 * @brief Parses chain list JSON content and finds the network ID for a given chain name.
 * Performs case-insensitive comparison for the name.
 * @param json_content The JSON string content.
 * @param target_name The chain name to search for.
 * @return std::optional<int> The found network ID, or nullopt if not found or error.
 */
std::optional<int> find_chain_id_from_name(const std::string& json_content, const std::string& target_name) {
    if (target_name.empty()) return std::nullopt;
    
    // Convert target name to lowercase for case-insensitive comparison
    std::string lower_target_name = target_name;
    std::transform(lower_target_name.begin(), lower_target_name.end(), lower_target_name.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    try {
        json j = json::parse(json_content);
        if (!j.is_array()) {
            std::cerr << LOG_PREFIX << "WARN: Chain list JSON for ID lookup is not an array." << std::endl;
            return std::nullopt;
        }
        
        // Iterate through the array of chain objects
        for (const auto& chain_obj : j) {
            // Check if it's an object and has name and chainId
            if (chain_obj.is_object() && chain_obj.contains("name") && chain_obj.at("name").is_string() && chain_obj.contains("chainId")) {
                // Get the name from the JSON object
                std::string current_name = chain_obj.at("name").get<std::string>();
                // Convert current name to lowercase
                std::string lower_current_name = current_name;
                std::transform(lower_current_name.begin(), lower_current_name.end(), lower_current_name.begin(),
                               [](unsigned char c){ return std::tolower(c); });
                
                // Compare lowercase names
                if (lower_current_name == lower_target_name) {
                    // Found matching name, return the chainId (check if it's a valid number)
                    if (chain_obj.at("chainId").is_number_integer()) {
                        int found_id = chain_obj.at("chainId").get<int>();
                        if (found_id > 0) { // Ensure ID is positive
                            return found_id;
                        } else {
                            std::cerr << LOG_PREFIX << "WARN: Found name '" << target_name << "' but associated chainId is invalid (" << found_id << ")." << std::endl;
                            return std::nullopt;
                        }
                    } else {
                        std::cerr << LOG_PREFIX << "WARN: Found name '" << target_name << "' but associated chainId is not an integer." << std::endl;
                        return std::nullopt;
                    }
                }
            }
        }
    } catch (const json::parse_error& e) {
        std::cerr << LOG_PREFIX << "WARN: Failed to parse chain list JSON for ID lookup: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << LOG_PREFIX << "WARN: Error during chain ID lookup: " << e.what() << std::endl;
    }
    
    // Return empty optional if the name was not found or an error occurred
    return std::nullopt;
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
 * @brief Finds an existing blockchain entry in the config or creates a new one.
 * Handles name/ID lookup using master chain list if necessary.
 * @param config The main configuration object (mutable).
 * @param blockchain_name_or_id User input specifying the blockchain.
 * @return A pair containing:
 * - A pointer to the found or newly created blockchain_info struct within config, or nullptr on critical error.
 * - A boolean indicating if the entry was newly created (true) or already existed (false).
 */
std::pair<neozork::config_manager::struct_blockchain_info*, bool>
find_or_create_blockchain_entry(
                                neozork::config_manager::struct_config& config,
                                const std::string& blockchain_name_or_id)
{
    // Attempt to find the blockchain in the config first
    auto blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
    
    // If found, return pointer to existing entry and false (not new)
    if (blockchain_ref_opt) {
        neozork::config_manager::struct_blockchain_info* existing_ptr = &blockchain_ref_opt.value().get();
        std::cout << LOG_PREFIX << "Found existing blockchain '" << existing_ptr->name << "' (ID: " << existing_ptr->network_id << ")." << std::endl;
        return { existing_ptr, false };
    }
    
    // --- Blockchain not found, attempt to create ---
    std::cout << LOG_PREFIX << "'" << blockchain_name_or_id << "' not found. Trying to create..." << std::endl;
    neozork::config_manager::struct_blockchain_info new_bc; // Structure for the new entry
    int parsed_id = 0;
    std::string name_to_use = blockchain_name_or_id; // Default name is user input
    int final_network_id = 0; // Final ID for the new entry
    bool input_is_id = false; // Flag if input was recognized as an ID
    
    // Try to interpret input as an ID
    try {
        long long id_ll = std::stoll(blockchain_name_or_id);
        if (id_ll > 0 && id_ll <= std::numeric_limits<int>::max()) {
            parsed_id = static_cast<int>(id_ll);
            input_is_id = true;
        }
    } catch (...) { }
    
    // Download master list once for potential lookup
    std::optional<std::string> chain_list_json = download_master_chain_list();
    
    // If input was an ID, look up name
    if (input_is_id) {
        std::cout << LOG_PREFIX << "Input is ID: " << parsed_id << ". Looking up name..." << std::endl;
        if (chain_list_json) {
            std::optional<std::string> name_from_list = find_chain_name_from_id(*chain_list_json, parsed_id);
            if (name_from_list) {
                name_to_use = *name_from_list;
                std::cout << LOG_PREFIX << "Found Name '" << name_to_use << "'." << std::endl;
            } else {
                std::cerr << LOG_PREFIX << "WARN: Name for ID " << parsed_id << " not found. Using ID as name." << std::endl;
                name_to_use = std::to_string(parsed_id);
            }
        } else {
            std::cerr << LOG_PREFIX << "WARN: Failed list download. Using ID as name." << std::endl;
            name_to_use = std::to_string(parsed_id);
        }
        final_network_id = parsed_id; // ID is known
    }
    // If input was a name, look up ID
    else {
        std::cout << LOG_PREFIX << "Input is Name: '" << name_to_use << "'. Looking up ID..." << std::endl;
        if (chain_list_json) {
            std::optional<int> id_from_list = find_chain_id_from_name(*chain_list_json, name_to_use);
            if (id_from_list) {
                final_network_id = *id_from_list; // Use found ID
                std::cout << LOG_PREFIX << "Found Network ID " << final_network_id << "." << std::endl;
            } else {
                final_network_id = 0; // Mark ID as unknown
                std::cerr << LOG_PREFIX << "WARN: Network ID for name '" << name_to_use << "' not found. Setting ID to 0." << std::endl;
            }
        } else {
            final_network_id = 0; // Mark ID as unknown
            std::cerr << LOG_PREFIX << "WARN: Failed list download for ID lookup. Setting ID to 0." << std::endl;
        }
    }
    
    // Assign final values to the new blockchain struct
    new_bc.name = name_to_use;
    new_bc.network_id = final_network_id;
    
    // Attempt to add the new blockchain entry to the config
    if (neozork::config_manager::add_blockchain(config, new_bc)) {
        std::cout << LOG_PREFIX << "Added new blockchain '" << new_bc.name << "' (ID: " << new_bc.network_id << ")." << std::endl;
        
        // Re-find the newly added entry to get a stable pointer/reference
        auto re_find = neozork::config_manager::find_blockchain(config, new_bc.name);
        
        if (re_find) {
            // Return pointer and true (newly created)
            return { &re_find.value().get(), true };
        } else {
            std::cerr << "[Discovery] CRITICAL: Cannot re-find just added blockchain!" << std::endl;
            return { nullptr, false }; // Indicate critical error
        }
    } else {
        // Failed to add (should only happen if it existed, which we checked) - indicates potential issue
        std::cerr << "[Discovery] ERROR: Failed to add blockchain (already exists?)." << std::endl;
        // Try to find existing one again just in case
        auto existing = neozork::config_manager::find_blockchain(config, name_to_use);
        if (existing) {
            std::cout << LOG_PREFIX << "Proceeding with existing blockchain found: '" << existing.value().get().name << "'." << std::endl;
            return { &existing.value().get(), false }; // Return pointer and false (not new)
        } else {
            std::cerr << "[Discovery] CRITICAL: Cannot add or find blockchain!" << std::endl;
            return { nullptr, false }; // Indicate critical error
        }
    }
}


// ---  Helper Function: Process a Single Discovery Source ---

/**
 * @brief Downloads, parses, and processes endpoints from a single source URL or keyword.
 * @param source The source string (e.g., "chain", "eth","defi", "https://...").
 * @param blockchain_info Reference to the blockchain structure in config to add endpoints to.
 * @param current_processing_id The definitive network ID of the target blockchain.
 * @return int The number of new endpoints successfully added from this source.
 */
int process_single_source(
                          const std::string& source,
                          neozork::config_manager::struct_blockchain_info& blockchain_info,
                          int current_processing_id)
{
    // Vector to store URLs parsed from the current source
    std::vector<std::string> raw_urls;
    
    // URL to download content from for this source
    std::string url_to_download;
    
    // Enum to determine which parsing function to use
    // Note: DEFILLAMA uses the same parser as CHAINLIST for now
    enum class ParserType { UNKNOWN, CHAINLIST, ETH_LISTS, SIMPLE_LIST, AUTO_DETECT };
    
    // Default parser type
    ParserType parser_to_use = ParserType::UNKNOWN;
    
    // Convert source keyword to lowercase for easier matching
    std::string lower_source = source;
    std::transform(lower_source.begin(), lower_source.end(), lower_source.begin(), [](unsigned char c){
        return std::tolower(c);
    });
    
    // --- Determine parser type and download URL ---
    if (lower_source == "chain") {
        url_to_download = "https://chainid.network/chains.json";
        parser_to_use = ParserType::CHAINLIST;
    }
    // Accept both keywords (defi and defillama)
    else if (lower_source == "defi" || lower_source == "defillama") {
        url_to_download = "https://api.llama.fi/chains";
        
        // Reuse chainlist parser assuming similar structure
        parser_to_use = ParserType::CHAINLIST;
        std::cout << "/n" << LOG_PREFIX << "Identified source as DefiLlama (using Chainlist-compatible parser)." << std::endl;
    }
    else if (lower_source == "eth") {
        url_to_download = "https://raw.githubusercontent.com/ethereum-lists/chains/master/_data/chains/eip155-1.json"; parser_to_use = ParserType::ETH_LISTS;
    }
    else if (source.rfind("https://", 0) == 0 || source.rfind("http://", 0) == 0) {
        url_to_download = source;
        parser_to_use = ParserType::AUTO_DETECT;
    }
    else { std::cerr << "\n" << LOG_PREFIX << "WARN: Unknown source type: '" << source << "'. Skipping." << std::endl;
        return 0;
    }
    
    // --- Check applicability of 'eth' source ---
    if (parser_to_use == ParserType::ETH_LISTS && current_processing_id != 1) {
        std::cerr << "\n" << LOG_PREFIX << "WARN: Source 'eth' only valid for Ethereum (ID 1), but target is '" << blockchain_info.name << "' (ID: " << current_processing_id << "). Skipping." << std::endl;
        return 0;
    }
    
    // --- Download Content ---
    neozork::connection_manager::connection_result download_result;
    
    // Check if URL to download is empty
    if (url_to_download.empty())
        return 0;
    
    // Parse the URL to extract host and path
    size_t host_start = url_to_download.find("://");
    
    // Check if scheme is present
    if (host_start == std::string::npos) {
        std::cerr << "\n"<<LOG_PREFIX<<"ERR: Invalid URL scheme: "<<url_to_download<<std::endl;
        return 0;
    }
    
    // Move past the scheme (e.g., "https://")
    host_start += 3;
    
    // Find the position of the first '/' after the scheme
    size_t path_start = url_to_download.find('/', host_start);
    
    // Extract host and path from the URL
    std::string host, path;
    
    // If path is not found, use the rest of the string as host and set path to "/"
    if (path_start == std::string::npos) {
        host = url_to_download.substr(host_start); path = "/";
    }
    
    // If path is found, extract host and path separately
    else {
        host = url_to_download.substr(host_start, path_start - host_start); path = url_to_download.substr(path_start);
    }
    
    // Check if host is empty
    if (host.empty()) {
        std::cerr << "\n"<<LOG_PREFIX<<"ERR: Cannot extract host from: "<<url_to_download<<std::endl;
        return 0;
    }
    
    // Prepare headers for the download request
    neozork::connection_manager::http_headers download_headers = {{"User-Agent", "N3D"},{"Accept", "*/*"}};
    
    // Perform the download using the connection manager
    download_result = neozork::connection_manager::https_get(host, path, download_headers);
    
    // Check if the download was successful (no client error and body received)
    if (!download_result.body.has_value() || download_result.error_message) {
        std::cerr << "\n" << LOG_PREFIX << "ERROR downloading " << url_to_download << ". Skipping source." << std::endl;
        return 0;
    }
    
    // --- Parse Downloaded Content ---
    const std::string& response_body = download_result.body.value();
    switch (parser_to_use) {
            
            // Handles "chain" and "defi"
        case ParserType::CHAINLIST:
            // Assuming DefiLlama response structure is compatible with parse_chainlist_rpcs_json
            raw_urls = parse_chainlist_rpcs_json(response_body, current_processing_id);
            break;
            
            // Handles "eth" and "ethereum-lists"
        case ParserType::ETH_LISTS: raw_urls = parse_ethereum_lists_json(response_body);
            break;
            
        case ParserType::AUTO_DETECT:
            // Try parsing as JSON array (Chainlist/DefiLlama format) first
            try {
                json j_try = json::parse(response_body);
                if (j_try.is_array()) {
                    std::cout << LOG_PREFIX << "Auto-detect: Attempting Chainlist/DefiLlama JSON array format..." << std::endl;
                    raw_urls = parse_chainlist_rpcs_json(response_body, current_processing_id);
                } else if (j_try.is_object()) {
                    std::cout << LOG_PREFIX << "Auto-detect: Attempting Ethereum Lists JSON object format..." << std::endl;
                    raw_urls = parse_ethereum_lists_json(response_body);
                }
            } catch (const json::parse_error&) {
                std::cout << LOG_PREFIX << "Auto-detect: Not valid JSON." << std::endl;
                // Fall through to simple list parsing if JSON fails or doesn't match known structures
            }
            
            // If JSON parsing didn't yield URLs, or if it wasn't JSON, try simple list
            if (raw_urls.empty()) {
                std::cout << LOG_PREFIX << "Auto-detect: Attempting simple URL list format..." << std::endl;
                raw_urls = parse_simple_url_list(response_body);
            }
            if (raw_urls.empty()) {
                std::cerr << "\n" << LOG_PREFIX << "WARN: Auto-detect failed to extract URLs from " << source << std::endl;
            }
            break;
        case ParserType::SIMPLE_LIST:
            raw_urls = parse_simple_url_list(response_body);
            break;
        case ParserType::UNKNOWN:
        default:
            std::cerr << "\n" << LOG_PREFIX << "Internal Error: Reached unknown parser type for source '" << source << "'." << std::endl;
            break;
    }
    
    // --- Process Parsed URLs ---
    int added_count = 0;
    for (const auto& raw_url : raw_urls) {
        std::string cleaned_url = trim_string(trim_quotes(raw_url));
        if (cleaned_url.empty() || contains_placeholder(cleaned_url))
            continue;
        
        auto connection_type_opt = get_connection_type_from_url(cleaned_url);
        if (!connection_type_opt)
            continue;
        
        std::string connection_type = connection_type_opt.value();
        neozork::config_manager::struct_endpoint new_ep;
        new_ep.connection_urls[connection_type] = cleaned_url;
        
        // Add endpoint (handles duplicates internally)
        if (neozork::config_manager::add_endpoint(blockchain_info, new_ep)) {
            added_count++;
            // Optional: More verbose logging
            // std::cout << LOG_PREFIX << "Added new endpoint entry: [" << connection_type << "] " << cleaned_url << std::endl;
        } else {
            // Optional: Log if it was skipped due to being a duplicate
            // std::cout << LOG_PREFIX << "Skipped duplicate endpoint entry: [" << connection_type << "] " << cleaned_url << std::endl;
        }
    }
    
    // Return the number of endpoints added *from this specific source*
    std::cout << LOG_PREFIX << "Finished processing source '" << source << "'. Added " << added_count << " new endpoint entries." << std::endl;
    return added_count;
}


// --- Main Discovery Function (Refactored Orchestrator) ---
bool discover_endpoints(
                        const std::string& blockchain_name_or_id,
                        const std::vector<std::string>& sources,
                        neozork::config_manager::struct_config& config)
{
    // Log start
    std::cout << "[Discovery] Starting discovery for: '" << blockchain_name_or_id << "'..." << std::endl;
    
    // --- 1. Find or Create Blockchain Entry ---
    // Call the helper function to get the pointer and status
    auto [blockchain_info_ptr, is_new_blockchain] = find_or_create_blockchain_entry(config, blockchain_name_or_id);
    
    // Check for critical error during find/create
    if (!blockchain_info_ptr) {
        std::cerr << "[Discovery] CRITICAL ERROR: Failed to find or create blockchain entry." << std::endl;
        return false; // Cannot proceed
    }
    // Use a reference for convenience
    neozork::config_manager::struct_blockchain_info& blockchain_info = *blockchain_info_ptr;
    // Get the definitive network ID for processing
    int current_processing_id = blockchain_info.network_id;
    
    // --- 2. Process Discovery Sources ---
    // Counter for total new endpoints added in this run
    int total_added_count = 0;
    
    // Index for progress bar update
    int source_index = 0;
    
    // Add a newline before starting the progress bar for separation
    std::cout << std::endl;
    
    // Start the progress bar
    neozork::ui::start_progress("Processing Sources", static_cast<long long>(sources.size()));
    
    // Loop through each source provided by the user
    for (const std::string& source : sources) {
        source_index++; // Increment index (1-based)
        // Process this source and get the count of added endpoints
        int added_from_this_source = process_single_source(source, blockchain_info, current_processing_id);
        // Add to the total count
        total_added_count += added_from_this_source;
        // Update the progress bar display
        neozork::ui::update_progress(source_index);
    }
    // Finish the progress bar display
    neozork::ui::finish_progress();
    
    // Log the summary for this blockchain
    std::cout << "[Discovery] Discovery finished for '" << blockchain_info.name << "'. Added " << total_added_count << " new endpoint entries." << std::endl;
    
    // --- 3. Save Config ---
    if (total_added_count > 0) {
        try {
            std::cout << "[Discovery] Saving updated configuration (new endpoints added)..." << std::endl;
            neozork::config_manager::save_config(config);
            std::cout << "[Discovery] Configuration saved successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Discovery] ERROR saving config: " << e.what() << std::endl;
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
    
    // Return true to indicate the discovery process completed
    return true;
}


} // namespace neozork::endpoint_discovery
