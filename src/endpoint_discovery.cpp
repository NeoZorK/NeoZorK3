// src/endpoint_discovery.cpp

#include "endpoint_discovery.h"
#include "connection_manager.h" // For HTTP requests
#include "config_manager.h"     // For finding blockchain and adding endpoints
#include "ui.h"                 // For progress bar
#include "version.h"            // For program version
#include <optional>
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

// Define the namespace for endpoint discovery functionalities
namespace neozork::endpoint_discovery {

// Define a prefix for log messages originating from this module
#define LOG_PREFIX "    [Discovery] "

// --- Helper: Intermediate structure for parsed chains from source ---
struct SourceChainInfo {
    std::string name = "Unknown"; // Default name
    int chainId = 0;             // Chain ID
    std::vector<std::string> rpcUrls; // List of RPC URLs found for this chain
    // std::string gecko_id = ""; // Optional: Add other fields if needed later
};

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

// --- Refactored Parser for Chainlist/DefiLlama Format ---
std::vector<SourceChainInfo> parse_chainlist_format_source(const std::string& content) {
    std::vector<SourceChainInfo> all_chains;
    std::cout << std::endl; // Separate from progress bar
    std::cout << LOG_PREFIX << "Parsing source using Chainlist/DefiLlama format..." << std::endl;
    try {
        json j = json::parse(content);
        if (!j.is_array()) {
            std::cerr << LOG_PREFIX << "ERROR: Expected JSON array from source, got " << j.type_name() << std::endl;
            return all_chains;
        }
        for (const auto& chain_obj : j) {
            if (!chain_obj.is_object()) continue;
            SourceChainInfo current_chain;
            current_chain.name = chain_obj.value("name", "Unknown");
            if (chain_obj.contains("chainId") && chain_obj.at("chainId").is_number_integer()) {
                current_chain.chainId = chain_obj.at("chainId").get<int>();
                if (current_chain.chainId <= 0) continue; // Skip invalid ID
            } else { continue; } // Skip missing/invalid ID
            
            const json* rpc_array_ptr = nullptr;
            if (chain_obj.contains("rpc") && chain_obj.at("rpc").is_array()) rpc_array_ptr = &chain_obj.at("rpc");
            else if (chain_obj.contains("rpcs") && chain_obj.at("rpcs").is_array()) rpc_array_ptr = &chain_obj.at("rpcs");
            
            if (rpc_array_ptr) {
                for (const auto& rpc_item : *rpc_array_ptr) {
                    std::string url;
                    if (rpc_item.is_string()) url = rpc_item.get<std::string>();
                    else if (rpc_item.is_object() && rpc_item.contains("url") && rpc_item.at("url").is_string()) url = rpc_item.at("url").get<std::string>();
                    if (!url.empty() && !contains_placeholder(url)) { // Basic checks
                        // Only add URL if scheme is recognized/supported
                        if (get_connection_type_from_url(url).has_value()) {
                            current_chain.rpcUrls.push_back(url);
                        }
                    }
                }
            }
            all_chains.push_back(current_chain);
        }
        std::cout << LOG_PREFIX << "Parsed " << all_chains.size() << " total chains from source." << std::endl;
    } catch (const json::parse_error& e) {
        std::cerr << LOG_PREFIX << "ERROR parsing source JSON: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << LOG_PREFIX << "ERROR processing source JSON: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << LOG_PREFIX << "ERROR processing source JSON (unknown error)." << std::endl;
    }
    return all_chains;
}



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

/**
 * @brief Discovers chains from sources, filters by name, and syncs them with local config.
 * Adds new chains or new endpoints to existing chains found in the source.
 * @param config The main configuration object (mutable).
 * @param name_filter Substring to filter chain names from the source (case-insensitive). Use "*" or empty for all.
 * @param sources Vector of source keywords (e.g., "chain") or URLs.
 * @return True if the configuration was potentially modified, false otherwise or on critical error.
 */
bool discover_and_sync_chains(
                              neozork::config_manager::struct_config& config,
                              const std::string& name_filter,
                              const std::vector<std::string>& sources)
{
    bool changes_made_overall = false;
    
    // Prepare lowercase filter
    std::string lower_name_filter = name_filter;
    if (name_filter != "*") { // Treat "*" as wildcard, don't lowercase it
        std::transform(lower_name_filter.begin(), lower_name_filter.end(), lower_name_filter.begin(), ::tolower);
    }
    
    // Process each source specified by the user
    for (const std::string& source_keyword_or_url : sources) {
        std::cout << "\n" << LOG_PREFIX << "Processing source: '" << source_keyword_or_url << "'" << std::endl;
        
        // --- 1. Determine Download URL and Parser ---
        std::string url_to_download;
        // Use std::function to point to the correct parser (only one refactored for now)
        std::function<std::vector<SourceChainInfo>(const std::string&)> parser_func = nullptr;
        std::string lower_source = source_keyword_or_url;
        std::transform(lower_source.begin(), lower_source.end(), lower_source.begin(), ::tolower);
        
        if (lower_source == "chain") {
            url_to_download = "https://chainid.network/chains.json";
            parser_func = parse_chainlist_format_source;
        } else if (lower_source == "defi" || lower_source == "defillama") {
            url_to_download = "https://api.llama.fi/chains"; // Use the working one
            parser_func = parse_chainlist_format_source;
        } else if (lower_source == "eth") {
            // url_to_download = "..."; // Set URL if needed
            // parser_func = parse_ethereum_lists_json; // Needs refactoring to SourceChainInfo
            std::cerr << LOG_PREFIX << "WARN: Source type 'eth' not yet supported by new sync logic. Skipping." << std::endl;
            continue; // Skip this source
        } else if (lower_source.rfind("https://", 0) == 0 || lower_source.rfind("http://", 0) == 0) {
            url_to_download = source_keyword_or_url;
            // parser_func = parse_simple_url_list; // Cannot return SourceChainInfo easily
            std::cerr << LOG_PREFIX << "WARN: Direct URL source type not yet fully supported by new sync logic (cannot determine chain info). Skipping." << std::endl;
            continue; // Skip this source
        } else {
            std::cerr << LOG_PREFIX << "WARN: Unknown source type: '" << source_keyword_or_url << "'. Skipping." << std::endl;
            continue; // Skip unknown source
        }
        
        
        // --- 2. Fetch Data ---
        std::string source_content = "";
        if (!url_to_download.empty()) {
            std::cout << LOG_PREFIX << "Downloading data from: " << url_to_download << std::endl;
            // Extract host/path
            size_t host_start = url_to_download.find("://");
            if (host_start != std::string::npos) host_start += 3; else { /* Handle error */ std::cerr << " Invalid URL\n"; continue; }
            size_t path_start = url_to_download.find('/', host_start);
            std::string host = (path_start == std::string::npos) ? url_to_download.substr(host_start) : url_to_download.substr(host_start, path_start - host_start);
            std::string path = (path_start == std::string::npos) ? "/" : url_to_download.substr(path_start);
            
            if (!host.empty()) {
                // Use connection manager
                neozork::connection_manager::http_headers headers = {{"User-Agent", "NeoZorK3_Discovery_Bot/" + neozork::PROGRAM_VERSION}};
                auto result = neozork::connection_manager::https_get(host, path, headers);
                if (result.body && !result.error_message) {
                    source_content = result.body.value();
                    std::cout << LOG_PREFIX << "Download successful (" << source_content.length() << " bytes)." << std::endl;
                } else {
                    std::cerr << LOG_PREFIX << "ERROR downloading source: " << (result.error_message ? *result.error_message : "Unknown download error") << std::endl;
                }
            } else {
                std::cerr << LOG_PREFIX << "ERROR: Could not extract host from URL." << std::endl;
            }
        }
        if (source_content.empty()) {
            std::cout << LOG_PREFIX << "No content downloaded or error occurred. Skipping this source." << std::endl;
            continue; // Skip to next source
        }
        
        
        // --- 3. Parse ALL chains from the source data ---
        if (!parser_func) {
            std::cerr << LOG_PREFIX << "Internal error: No parser function assigned for source '" << source_keyword_or_url << "'. Skipping." << std::endl;
            continue;
        }
        std::vector<SourceChainInfo> all_parsed_chains = parser_func(source_content);
        if (all_parsed_chains.empty()) {
            std::cout << LOG_PREFIX << "Parser returned no chain information from the source." << std::endl;
            continue;
        }
        
        
        // --- 4. Filter chains based on the name_filter ---
        std::vector<SourceChainInfo> filtered_chains;
        
        // Handle wildcard filter first
        if (name_filter == "*" || name_filter.empty()) {
            filtered_chains = all_parsed_chains;
            std::cout << LOG_PREFIX << "Processing all " << all_parsed_chains.size() << " chains from source (filter is '" << name_filter << "')." << std::endl;
        } else {
            // Check if the filter is numeric (an ID)
            bool is_id_filter = false;
            long long filter_id_ll = -1;
            try {
                filter_id_ll = std::stoll(name_filter);
                if (filter_id_ll > 0) is_id_filter = true;
            } catch (...) { is_id_filter = false; }
            
            if (is_id_filter) {
                // --- Filter by exact Chain ID ---
                std::cout << LOG_PREFIX << "Filtering source chains by exact chainId: " << filter_id_ll << "..." << std::endl;
                for (const auto& chain : all_parsed_chains) {
                    if (chain.chainId == filter_id_ll) {
                        filtered_chains.push_back(chain);
                        // Assuming chainId is unique, we can stop after finding one
                        break;
                    }
                }
            } else {
                // --- Filter by Name Substring (Case-Insensitive) ---
                std::cout << LOG_PREFIX << "Filtering source chains by name containing: '" << name_filter << "'..." << std::endl;
                std::string lower_name_filter = name_filter;
                std::transform(lower_name_filter.begin(), lower_name_filter.end(), lower_name_filter.begin(), ::tolower);
                
                for (const auto& chain : all_parsed_chains) {
                    std::string lower_chain_name = chain.name;
                    std::transform(lower_chain_name.begin(), lower_chain_name.end(), lower_chain_name.begin(), ::tolower);
                    if (lower_chain_name.find(lower_name_filter) != std::string::npos) {
                        filtered_chains.push_back(chain);
                    }
                }
            }
            // Log count after filtering
            std::cout << LOG_PREFIX << "Found " << filtered_chains.size() << " chains in source matching the filter." << std::endl;
        } // End else (filter is not wildcard)
        
        // Check if filter yielded any results
        if (filtered_chains.empty()) {
            std::cout << LOG_PREFIX << "No chains from this source matched the filter criteria." << std::endl;
            continue; // Skip to next source if filter yields nothing
        }
        
        
        // --- 5. Synchronize each filtered chain with local config ---
        for (const auto& source_chain : filtered_chains) {
            // Skip if chain ID is invalid (already checked in parser, but double-check)
            if (source_chain.chainId <= 0) continue;
            
            std::cout << LOG_PREFIX << "Syncing: " << source_chain.name << " (ID: " << source_chain.chainId << ")" << std::endl;
            bool current_chain_modified = false;
            
            // Find if chain exists locally by ID using config_manager function
            auto local_bc_opt = neozork::config_manager::find_blockchain(config, std::to_string(source_chain.chainId));
            
            if (!local_bc_opt) {
                // --- Add New Blockchain ---
                std::cout << LOG_PREFIX << " -> Chain not found locally. Adding new entry..." << std::endl;
                neozork::config_manager::struct_blockchain_info new_bc;
                new_bc.name = source_chain.name;
                new_bc.network_id = source_chain.chainId;
                // Add endpoints from source
                int added_endpoints_count = 0;
                for(const std::string& url : source_chain.rpcUrls) {
                    auto type_opt = get_connection_type_from_url(url);
                    if (!type_opt) {
                        // std::cout << LOG_PREFIX << "    -> Skipping unsupported endpoint URL: " << url << std::endl;
                        continue; // Skip unsupported schemes
                    }
                    neozork::config_manager::struct_endpoint ep;
                    ep.connection_urls[*type_opt] = url;
                    // Add endpoint to the list for the new blockchain
                    new_bc.endpoints.push_back(ep);
                    added_endpoints_count++;
                }
                
                // Add the new blockchain struct to the main config vector
                // Using the config_manager function which checks for duplicates (though shouldn't happen here)
                if(neozork::config_manager::add_blockchain(config, new_bc)) {
                    std::cout << LOG_PREFIX << " -> Successfully added new blockchain '" << new_bc.name << "' with " << added_endpoints_count << " endpoint(s)." << std::endl;
                    current_chain_modified = true;
                } else {
                    // This case should ideally not be reached if find_blockchain worked correctly
                    std::cerr << LOG_PREFIX << " -> ERROR: Failed to add new blockchain '" << new_bc.name << "' even though it wasn't found initially!" << std::endl;
                }
            } else {
                // --- Update Existing Blockchain (Add missing endpoints) ---
                neozork::config_manager::struct_blockchain_info& local_bc = local_bc_opt.value().get();
                std::cout << LOG_PREFIX << " -> Found locally: '" << local_bc.name << "'. Checking for new endpoints..." << std::endl;
                // Optional: Update name if different? Be careful with this.
                // if (local_bc.name != source_chain.name) { ... update local_bc.name ... current_chain_modified = true; }
                
                int added_endpoints_count = 0;
                for(const std::string& url : source_chain.rpcUrls) {
                    // Check if an endpoint with this URL already exists in the local config for this chain
                    auto existing_ep_opt = neozork::config_manager::find_endpoint_by_any_url(local_bc, url);
                    if (!existing_ep_opt) {
                        // Endpoint doesn't exist locally, add it
                        auto type_opt = get_connection_type_from_url(url);
                        if (!type_opt) continue; // Skip unsupported schemes
                        
                        neozork::config_manager::struct_endpoint new_ep;
                        new_ep.connection_urls[*type_opt] = url;
                        
                        // Add using config_manager function (handles internal vector push_back)
                        if(neozork::config_manager::add_endpoint(local_bc, new_ep)) {
                            // std::cout << LOG_PREFIX << "    -> Added new endpoint: " << url << std::endl; // Verbose log
                            added_endpoints_count++;
                            current_chain_modified = true;
                        } else {
                            // This might happen if add_endpoint implements extra checks, though unlikely now
                            std::cerr << LOG_PREFIX << "    -> WARN: Failed to add new endpoint '" << url << "' via add_endpoint function." << std::endl;
                        }
                    }
                    // else { std::cout << LOG_PREFIX << "    -> Endpoint exists: " << url << std::endl; } // Verbose log
                } // End loop over source RPC URLs
                
                if (added_endpoints_count > 0) {
                    std::cout << LOG_PREFIX << " -> Added " << added_endpoints_count << " new endpoint(s) to existing chain '" << local_bc.name << "'." << std::endl;
                } else {
                    std::cout << LOG_PREFIX << " -> No new endpoints found in source for existing chain '" << local_bc.name << "'." << std::endl;
                }
            } // End if/else (local chain found)
            
            // Track if any modification happened for this chain
            if(current_chain_modified) {
                changes_made_overall = true;
            }
            
        } // --- End loop syncing filtered chains ---
        
    } // --- End loop over sources ---
    
    // Return true if any changes were made across any source/chain
    if (changes_made_overall) {
        std::cout << LOG_PREFIX << "Sync process completed. Changes detected in configuration." << std::endl;
    } else {
        std::cout << LOG_PREFIX << "Sync process completed. No changes detected in configuration." << std::endl;
    }
    return changes_made_overall; // Indicate if saving might be needed
}


} // namespace neozork::endpoint_discovery
