// src/endpoint_discovery.cpp

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
#include <iomanip>   // For std::setw in logging
#include <limits>    // For std::numeric_limits
#include <ui.h>
#include "version.h" // For program version
#include <nlohmann/json.hpp>
#include <regex>

// Include nlohmann/json as we will parse JSON
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Define a helper for consistent logging indentation
#define LOG_PREFIX "    [Discovery] "

namespace neozork::endpoint_discovery {

// --- Utility functions ---
// (Keep the utility functions: trim_string, trim_quotes, contains_placeholder, get_connection_type_from_url, extract_placeholder_name)
std::string trim_string(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first) { return str; }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}
std::string trim_quotes(const std::string& str) {
    if (str.length() >= 2 && ((str.front() == '"' && str.back() == '"') || (str.front() == '\'' && str.back() == '\''))) {
        return str.substr(1, str.length() - 2);
    }
    return str;
}
bool contains_placeholder(const std::string& url) {
    return url.find("${") != std::string::npos && url.find("}") != std::string::npos;
}
std::optional<std::string> get_connection_type_from_url(const std::string& url) {
    if (url.rfind("https://", 0) == 0) return "https";
    if (url.rfind("wss://", 0) == 0) return "wss";
    if (url.rfind("http://", 0) == 0) return "http";
    if (url.rfind("ws://", 0) == 0) return "ws";
    if (url.rfind("ipc://", 0) == 0) return "ipc";
    // Allow simple hostnames too? Maybe later. For now, require scheme.
    // size_t scheme_pos = url.find("://");
    // if (scheme_pos == std::string::npos) return "https"; // Assume https if no scheme? Risky.
    return std::nullopt;
}
std::optional<std::string> extract_placeholder_name(const std::string& url) {
    size_t start_pos = url.find("${");
    if (start_pos != std::string::npos) {
        size_t end_pos = url.find("}", start_pos);
        if (end_pos != std::string::npos) { return url.substr(start_pos + 2, end_pos - (start_pos + 2)); }
    }
    return std::nullopt;
}


// --- Parsing functions ---
std::vector<std::string> parse_chainlist_rpcs_json(const std::string& content, int target_chain_id) {
    std::vector<std::string> urls;
    std::cout << LOG_PREFIX << "Parsing content as Chainlist RPCs JSON for target chain ID: " << target_chain_id << "..." << std::endl;
    if (target_chain_id <= 0) {
        std::cerr << LOG_PREFIX << "WARNING: Cannot filter Chainlist by Chain ID because target ID is " << target_chain_id << ". Returning no URLs." << std::endl;
        return urls;
    }
    try {
        json j = json::parse(content);
        std::cout << LOG_PREFIX << "Chainlist JSON parsed successfully." << std::endl;
        
        if (!j.is_array()) {
            std::cerr << LOG_PREFIX << "ERROR: Expected a JSON array (of chain objects) from Chainlist source, but got type: " << j.type_name() << std::endl;
            return urls;
        }
        
        size_t chain_count = j.size();
        std::cout << LOG_PREFIX << "Found " << chain_count << " chain objects in the top-level array." << std::endl;
        int found_rpcs_count = 0;
        bool target_chain_found = false;
        
        // Iterate through the array of CHAIN OBJECTS
        for (const auto& chain_obj : j) {
            if (!chain_obj.is_object()) { continue; } // Skip non-objects
            
            // Check if this chain object has the target chainId
            if (chain_obj.contains("chainId")) {
                int current_chain_id = chain_obj.value("chainId", -1);
                
                if (current_chain_id == target_chain_id) {
                    target_chain_found = true;
                    std::string chain_name = chain_obj.value("name", "[Unknown Name]");
                    std::cout << LOG_PREFIX << "Found target chain object: '" << chain_name << "' (ID: " << current_chain_id << ")." << std::endl;
                    
                    // Now, check if this chain object has an 'rpc' array
                    if (chain_obj.contains("rpc") && chain_obj.at("rpc").is_array()) {
                        const auto& rpc_array = chain_obj.at("rpc");
                        std::cout << LOG_PREFIX << "  Found 'rpc' array with " << rpc_array.size() << " entries for this chain." << std::endl;
                        
                        // Iterate through the RPC entries within this chain object
                        for (const auto& rpc_item : rpc_array) {
                            // Chainlist RPC items can be strings or objects
                            std::string current_url;
                            if (rpc_item.is_string()) {
                                current_url = rpc_item.get<std::string>();
                            } else if (rpc_item.is_object() && rpc_item.contains("url")) {
                                current_url = rpc_item.value("url", "");
                                // Could potentially extract tracking info here later if needed
                            }
                            
                            if (!current_url.empty()) {
                                // Exclude URLs containing placeholders
                                if (current_url.find("${") == std::string::npos) {
                                    std::cout << LOG_PREFIX << "    Adding valid RPC URL: " << current_url << std::endl;
                                    urls.push_back(current_url);
                                    found_rpcs_count++;
                                } else {
                                    std::cout << LOG_PREFIX << "    Skipping placeholder RPC URL: " << current_url << std::endl;
                                }
                            }
                        } // end inner loop (rpc items)
                    } else {
                        std::cout << LOG_PREFIX << "  WARNING: Target chain object found, but it has no 'rpc' array." << std::endl;
                    }
                    // Since chainId should be unique, we can stop searching after finding the target chain
                    break;
                } // end if current_chain_id == target_chain_id
            } // end if contains("chainId")
        } // end outer loop (chain objects)
        
        if (!target_chain_found) {
            std::cout << LOG_PREFIX << "Target chain ID " << target_chain_id << " not found in the JSON data." << std::endl;
        }
        std::cout << LOG_PREFIX << "Finished processing Chainlist JSON. Found and added " << found_rpcs_count << " valid RPC URLs for target chain ID " << target_chain_id << "." << std::endl;
        
    } catch (const json::parse_error& e) {
        std::cerr << LOG_PREFIX << "ERROR: Failed to parse Chainlist JSON source: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << LOG_PREFIX << "ERROR: Generic error processing Chainlist JSON source: " << e.what() << std::endl;
    }
    return urls;
}

std::vector<std::string> parse_simple_url_list(const std::string& content) {
    std::cout << LOG_PREFIX << "Parsing content as simple URL list..." << std::endl;
    std::vector<std::string> urls;
    std::stringstream ss(content);
    std::string line;
    int line_count = 0;
    while (std::getline(ss, line)) {
        line_count++;
        std::string processed_line = trim_string(line);
        if (!processed_line.empty() && processed_line[0] != '#') {
            urls.push_back(trim_quotes(processed_line));
        }
    }
    std::cout << LOG_PREFIX << "Parsed " << urls.size() << " URLs from " << line_count << " lines." << std::endl;
    return urls;
}

std::vector<std::string> parse_ethereum_lists_json(const std::string& content) {
    std::cout << LOG_PREFIX << "Attempting to parse content as ethereum-lists JSON (eip155-X format)..." << std::endl;
    std::vector<std::string> urls;
    try {
        json j = json::parse(content);
        std::cout << LOG_PREFIX << "JSON parsed successfully." << std::endl;
        if (j.is_object() && j.contains("rpc") && j.at("rpc").is_array()) {
            std::cout << LOG_PREFIX << "Found 'rpc' array with " << j.at("rpc").size() << " items." << std::endl;
            for (const auto& item : j.at("rpc")) {
                if (item.is_string()) {
                    std::string url = item.get<std::string>();
                    // Exclude placeholders
                    if (!url.empty() && url.find("${") == std::string::npos) {
                        urls.push_back(url);
                    } else if (!url.empty()){
                        std::cout << LOG_PREFIX << "Skipping placeholder URL from ethereum-lists: " << url << std::endl;
                    }
                } else { std::cerr << LOG_PREFIX << "WARNING: Non-string item found in 'rpc' array: " << item.dump() << std::endl; }
            }
        } else { std::cerr << LOG_PREFIX << "WARNING: JSON source is not an object or does not contain a valid 'rpc' array (expected ethereum-lists format)." << std::endl; }
    } catch (const json::parse_error& e) {
        std::cerr << LOG_PREFIX << "ERROR parsing ethereum-lists JSON: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << LOG_PREFIX << "ERROR processing ethereum-lists JSON: " << e.what() << std::endl;
    }
    std::cout << LOG_PREFIX << "Extracted " << urls.size() << " URLs from ethereum-lists format." << std::endl;
    return urls;
}

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

// --- Downloading functions ---
std::optional<std::string> download_master_chain_list() {
     std::string list_url = "https://chainid.network/chains.json";
     std::cout << LOG_PREFIX << "Attempting to download master chain list from: " << list_url << " (for name lookup)" << std::endl;

     // Parsing URL
     std::regex url_regex(R"(^(https?)://([^/]+)(/.*)?$)"); // Support http/https
     std::smatch match;
     std::string scheme, host, path = "/";
     if (std::regex_match(list_url, match, url_regex) && match.size() >= 3) {
         scheme = match[1].str();
         host = match[2].str();
         if (match.size() >= 4 && match[3].matched) { path = match[3].str(); }
     } else {
          std::cerr << LOG_PREFIX << "ERROR: Could not parse master chain list URL: " << list_url << std::endl;
          return std::nullopt;
     }

     // Downloading the list
     neozork::connection_manager::http_headers headers = {
         {"User-Agent", "NeoZorK3_Discovery_Bot/" + neozork::PROGRAM_VERSION},
         {"Accept", "application/json, */*"}
     };
     neozork::connection_manager::connection_result result;
     if (scheme == "https") {
        result = neozork::connection_manager::https_get(host, path, headers);
     } else {
         // TODO: Add support for http_get if needed
          std::cerr << LOG_PREFIX << "ERROR: HTTP download not implemented for master chain list." << std::endl;
          return std::nullopt;
     }


     if (!result.error_message && result.body.has_value()) {
          std::cout << LOG_PREFIX << "Master chain list downloaded successfully." << std::endl;
          return result.body;
     } else {
          std::cerr << LOG_PREFIX << "ERROR: Failed to download master chain list: "
                    << (result.error_message ? *result.error_message : "Unknown error") << std::endl;
          return std::nullopt;
     }
}

// --- Main discovery function (Handles source selection and parsing) ---
// (No changes needed in this function structure, only in download/parse part)
bool discover_endpoints(
                        const std::string& blockchain_name_or_id,
                        const std::vector<std::string>& sources,
                        neozork::config_manager::struct_config& config)
{
    std::cout << "[Discovery] Starting endpoint discovery for blockchain: '" << blockchain_name_or_id << "'..." << std::endl;
    
    // 1. Find or create the target blockchain in the config
    auto blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
    neozork::config_manager::struct_blockchain_info* blockchain_info_ptr = nullptr;
    
    if (!blockchain_ref_opt) {
        std::cout << LOG_PREFIX << "Blockchain '" << blockchain_name_or_id << "' not found in config. Attempting to create..." << std::endl;
        neozork::config_manager::struct_blockchain_info new_bc;
        new_bc.name = blockchain_name_or_id; // Assume input is name if not found as ID first
        try {
            // Try interpreting as ID first
            long long id_ll = std::stoll(blockchain_name_or_id);
            if (id_ll > 0 && id_ll <= std::numeric_limits<int>::max()) {
                new_bc.network_id = static_cast<int>(id_ll);
                // If it was an ID, maybe use a default name? Or keep the number as name? Let's keep it.
                std::cout << LOG_PREFIX << "Interpreted '" << blockchain_name_or_id << "' as network ID: " << new_bc.network_id << std::endl;
            } else {
                // Treat as name, ID is unknown
                std::cerr << LOG_PREFIX << "WARNING: Input '" << blockchain_name_or_id << "' looks numeric but is out of valid range for network ID. Treating as name, setting network ID to 0." << std::endl;
                new_bc.network_id = 0;
            }
        } catch (const std::invalid_argument&) {
            std::cout << LOG_PREFIX << "Input '" << blockchain_name_or_id << "' is not a number. Will treat as name." << std::endl;
            // Need to find ID based on name if possible (e.g. query chain?) - complex, do later.
            // For now, set ID to 0 if name is given and not found.
            std::cerr << LOG_PREFIX << "WARNING: Could not determine network ID for name '" << blockchain_name_or_id << "'. Setting network ID to 0. This might prevent Chainlist filtering!" << std::endl;
            new_bc.network_id = 0; // Mark as unknown ID
        } catch (const std::out_of_range&) {
            std::cerr << LOG_PREFIX << "WARNING: Input '" << blockchain_name_or_id << "' is numeric but too large for network ID. Treating as name, setting network ID to 0." << std::endl;
            new_bc.network_id = 0;
        }
        
        // Add the new blockchain structure to the config
        if (neozork::config_manager::add_blockchain(config, new_bc)) {
            std::cout << LOG_PREFIX << "Successfully added new blockchain '" << new_bc.name << "' (ID: " << new_bc.network_id << ") to config." << std::endl;
            // Re-find it to get a reference
            auto newly_added_blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
            if (newly_added_blockchain_ref_opt) {
                blockchain_info_ptr = &newly_added_blockchain_ref_opt.value().get();
            } else {
                // This should not happen if add_blockchain succeeded and find_blockchain is correct
                std::cerr << "[Discovery] CRITICAL ERROR: Failed to find newly added blockchain '" << blockchain_name_or_id << "' immediately after adding it!" << std::endl;
                return false; // Cannot proceed
            }
        } else {
            // add_blockchain returned false, likely means it already existed (e.g., found by ID when input was name)
            std::cerr << "[Discovery] ERROR: Failed to add new blockchain '" << blockchain_name_or_id << "' to config (perhaps already exists?). Trying to find it again..." << std::endl;
            // Try finding again in case it existed under a different identifier (name vs id)
            auto existing_blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
            if (existing_blockchain_ref_opt) {
                blockchain_info_ptr = &existing_blockchain_ref_opt.value().get();
                std::cout << LOG_PREFIX << "Found existing blockchain after add failed: '" << blockchain_info_ptr->name << "' (ID: " << blockchain_info_ptr->network_id << ")." << std::endl;
            } else {
                std::cerr << "[Discovery] CRITICAL ERROR: Failed to add blockchain and cannot find it afterwards. Cannot proceed." << std::endl;
                return false;
            }
        }
    } else {
        // Blockchain already existed
        blockchain_info_ptr = &blockchain_ref_opt.value().get();
        std::cout << LOG_PREFIX << "Found existing blockchain '" << blockchain_info_ptr->name << "' (ID: " << blockchain_info_ptr->network_id << ") in config." << std::endl;
        // If ID was 0 initially, maybe update it now? Could be complex. Leave as is for now.
    }
    
    // Final check if we have a valid pointer
    if (!blockchain_info_ptr) {
        std::cerr << "[Discovery] CRITICAL ERROR: Blockchain info pointer is null after find/create attempt. Cannot proceed." << std::endl;
        return false;
    }
    // Use a reference for convenience
    neozork::config_manager::struct_blockchain_info& blockchain_info = *blockchain_info_ptr;
    int target_network_id = blockchain_info.network_id; // Use the ID from the config struct
    std::cout << LOG_PREFIX << "Using Network ID for Chainlist filtering: " << target_network_id << std::endl;
    
    
    int total_added_count = 0;
    
    // 2. Process each source URL/keyword
    for (const std::string& source : sources) {
        std::cout << "[Discovery] Processing source: '" << source << "'" << std::endl;
        std::vector<std::string> raw_urls;
        
        std::string url_to_download;
        enum class ParserType { UNKNOWN, CHAINLIST, ETH_LISTS, SIMPLE_LIST, AUTO_DETECT };
        ParserType parser_to_use = ParserType::UNKNOWN;
        std::string source_type_name = "Unknown";
        
        std::string lower_source = source;
        std::transform(lower_source.begin(), lower_source.end(), lower_source.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        
        // --- Determine source type and download URL ---
        if (lower_source == "chain") {
            // Chainlist moved its main list, use the aggregate URL or specific chain files if possible
            url_to_download = "https://chainid.network/chains.json"; // This seems to be the replacement source often cited
            // Alternative could be https://github.com/DefiLlama/chainlist/tree/main/constants/extraRpcs
            parser_to_use = ParserType::CHAINLIST; // Keep using our parser, assuming similar structure or adapt parser
            source_type_name = "Chain Keyword (using chainid.network)";
            std::cout << LOG_PREFIX << "Source identified as '" << source_type_name << "'. URL: " << url_to_download << std::endl;
        } else if (lower_source == "eth") {
            // This URL seems stable for chain 1 (Ethereum Mainnet)
            url_to_download = "https://raw.githubusercontent.com/ethereum-lists/chains/master/_data/chains/eip155-1.json";
            parser_to_use = ParserType::ETH_LISTS;
            source_type_name = "Eth Keyword (Mainnet)";
            // We might need a way to specify *which* chain file from ethereum-lists later
            if (target_network_id != 1 && target_network_id != 0) { // Allow ID 0 as unknown
                std::cerr << LOG_PREFIX << "WARNING: Using ethereum-lists keyword but target network ID is " << target_network_id << ", not 1 (Ethereum Mainnet). Results might be incorrect." << std::endl;
            }
            std::cout << LOG_PREFIX << "Source identified as '" << source_type_name << "'. Using fixed URL: " << url_to_download << std::endl;
        } else if (source.rfind("https://", 0) == 0 || source.rfind("http://", 0) == 0) {
            url_to_download = source;
            parser_to_use = ParserType::AUTO_DETECT; // Let parser figure it out
            source_type_name = "Direct URL";
            std::cout << LOG_PREFIX << "Source identified as '" << source_type_name << "'. URL: " << url_to_download << std::endl;
        } else {
            std::cerr << LOG_PREFIX << "WARNING: Source '" << source << "' is not a recognized keyword ('defi', 'chain', 'eth') or http(s) URL. Skipping." << std::endl;
            continue; // Skip this source
        }
        
        // --- Download content ---
        // ** FIX STARTS HERE **
        // Change the type of the variable holding the result
        neozork::connection_manager::connection_result download_result;
        // ** FIX ENDS HERE **
        
        if (!url_to_download.empty()) {
            std::cout << LOG_PREFIX << "Attempting to download from: " << url_to_download << std::endl;
            size_t host_start = url_to_download.find("://");
            if (host_start == std::string::npos) {
                std::cerr << LOG_PREFIX << "ERROR: Could not find :// in URL. Skipping: " << url_to_download << std::endl;
                continue;
            }
            host_start += 3; // Move past "://"
            size_t path_start = url_to_download.find('/', host_start);
            std::string host, path;
            
            if (path_start == std::string::npos) {
                host = url_to_download.substr(host_start);
                path = "/"; // Default path if none exists
                // std::cout << LOG_PREFIX << "  URL has no path, using '/'." << std::endl; // Debug
            } else {
                host = url_to_download.substr(host_start, path_start - host_start);
                path = url_to_download.substr(path_start);
            }
            
            if (host.empty()) {
                std::cerr << LOG_PREFIX << "ERROR: Could not extract host from URL: " << url_to_download << ". Skipping." << std::endl;
                continue;
            }
            
            std::cout << LOG_PREFIX << "  Host: " << host << ", Path: " << path << std::endl;
            // Use standard headers for discovery
            neozork::connection_manager::http_headers headers = {
                {"User-Agent", "NeoZorK3_Discovery_Bot/" + neozork::PROGRAM_VERSION},
                {"Accept", "application/json, text/plain, */*"} // Accept common types
            };
            
            // ** FIX STARTS HERE **
            // Assign the result of https_get to the variable with the correct type
            download_result = neozork::connection_manager::https_get(host, path, headers);
            
            // Check if the download was successful by examining the result object
            // Success means we got a response body (even if status code is non-200)
            // Error means connection failed or exception occurred (error_message will be set)
            if (!download_result.error_message && download_result.body.has_value()) {
                std::cout << LOG_PREFIX << "Download successful. Status: "
                << (download_result.status_code.has_value() ? std::to_string(*download_result.status_code) : "N/A")
                << ". Response size: " << download_result.body.value().length() << " bytes." << std::endl;
                // Proceed to parsing even if status code is not 200, parser might handle it or fail gracefully
            } else {
                std::cerr << LOG_PREFIX << "ERROR: Failed to download content from " << url_to_download
                << (download_result.error_message ? ". Error: " + *download_result.error_message : "")
                << ". Skipping this source." << std::endl;
                continue; // Skip to next source
            }
            // ** FIX ENDS HERE **
            
        } else {
            std::cerr << LOG_PREFIX << "ERROR: url_to_download is empty for source '" << source << "'. Skipping." << std::endl;
            continue;
        }
        
        // --- Parse the downloaded content ---
        // ** FIX STARTS HERE **
        // Check if we have a body to parse
        if (download_result.body) {
            const std::string& response_body = download_result.body.value(); // Use .value() as we checked has_value
            // ** FIX ENDS HERE **
            std::cout << LOG_PREFIX << "Parsing content using method: ";
            
            switch (parser_to_use) {
                case ParserType::CHAINLIST:
                    std::cout << "Chainlist Parser (Filter ID: " << target_network_id << ")" << std::endl;
                    raw_urls = parse_chainlist_rpcs_json(response_body, target_network_id);
                    break;
                case ParserType::ETH_LISTS:
                    std::cout << "Ethereum-Lists Parser" << std::endl;
                    raw_urls = parse_ethereum_lists_json(response_body);
                    break;
                case ParserType::AUTO_DETECT:
                    std::cout << "Auto-Detect Parser" << std::endl;
                    std::cout << LOG_PREFIX << "Auto-detecting format for URL: " << source << std::endl;
                    try {
                        // Attempt JSON parsing first
                        if (json::accept(response_body)) {
                            json j_test = json::parse(response_body);
                            std::cout << LOG_PREFIX << "  Content is valid JSON." << std::endl;
                            if (j_test.is_array()) {
                                // Could be Chainlist format
                                std::cout << LOG_PREFIX << "  JSON is array, trying Chainlist parser..." << std::endl;
                                raw_urls = parse_chainlist_rpcs_json(response_body, target_network_id);
                            } else if (j_test.is_object()) {
                                // Could be Ethereum-Lists format
                                std::cout << LOG_PREFIX << "  JSON is object, trying Ethereum-lists parser..." << std::endl;
                                raw_urls = parse_ethereum_lists_json(response_body);
                            } else {
                                std::cout << LOG_PREFIX << "  JSON is neither array nor object? Trying simple list parser as fallback..." << std::endl;
                                raw_urls = parse_simple_url_list(response_body); // Fallback
                            }
                            // If parsing succeeded, raw_urls will be populated.
                            if (raw_urls.empty() && (j_test.is_array() || j_test.is_object())) {
                                std::cout << LOG_PREFIX << "  JSON specific parser yielded no URLs. Trying simple list parser as final fallback..." << std::endl;
                                raw_urls = parse_simple_url_list(response_body);
                            }
                            
                        } else {
                            // Not JSON, assume simple list
                            std::cout << LOG_PREFIX << "  Content is not valid JSON. Trying simple list parser..." << std::endl;
                            raw_urls = parse_simple_url_list(response_body);
                        }
                    } catch (const json::parse_error& e) {
                        // JSON parsing failed mid-way, might be malformed JSON or text
                        std::cerr << LOG_PREFIX << "  Warning: JSON parsing failed during auto-detect (" << e.what() << "). Trying simple list parser..." << std::endl;
                        raw_urls = parse_simple_url_list(response_body);
                    } catch (const std::exception& e) {
                        // Other errors during parsing
                        std::cerr << LOG_PREFIX << "  Error during auto-detect parsing: " << e.what() << ". Falling back to simple list parser..." << std::endl;
                        raw_urls = parse_simple_url_list(response_body); // Use simple list as fallback
                    }
                    
                    // Final check if auto-detect worked
                    if (raw_urls.empty()) {
                        std::cerr << LOG_PREFIX << "WARNING: Auto-detect failed to extract URLs using known formats from: " << source << std::endl;
                    } else {
                        std::cout << LOG_PREFIX << "  Auto-detect parse successful." << std::endl;
                    }
                    break;
                    
                case ParserType::SIMPLE_LIST: // Explicitly selected simple list
                    std::cout << "Simple List Parser" << std::endl;
                    raw_urls = parse_simple_url_list(response_body);
                    break;
                    
                case ParserType::UNKNOWN: // Should not happen if logic above is correct
                default:
                    std::cerr << "Internal Error: Unknown parser type selected." << std::endl;
                    break;
            }
            std::cout << LOG_PREFIX << "Parsing finished for source '" << source << "'. Found " << raw_urls.size() << " potential URLs." << std::endl;
        }
        // else: download failed or body was empty, already logged
        
        // 3. Process parsed URLs: clean, determine type/placeholder, add/update config
        int added_from_this_source = 0;
        for (const auto& raw_url : raw_urls) {
            std::string cleaned_url = trim_string(trim_quotes(raw_url));
            if (cleaned_url.empty()) continue;
            
            // Check for placeholders again, skip if found during discovery
            if (contains_placeholder(cleaned_url)) {
                std::cout << LOG_PREFIX << "Skipping placeholder URL found after parsing: " << cleaned_url << std::endl;
                continue;
            }
            
            auto connection_type_opt = get_connection_type_from_url(cleaned_url);
            if (!connection_type_opt) {
                std::cerr << LOG_PREFIX << "WARNING: Could not determine connection type from URL (missing scheme?): " << cleaned_url << ". Skipping." << std::endl;
                continue; // Skip if we can't determine type
            }
            std::string connection_type = connection_type_opt.value();
            // Placeholder check done above
            
            // Create a basic endpoint structure for adding
            neozork::config_manager::struct_endpoint new_endpoint_entry;
            new_endpoint_entry.connection_urls[connection_type] = cleaned_url;
            // Note: We don't set placeholder field here anymore, we skip placeholder URLs earlier.
            
            // Try adding this endpoint structure to the config
            // add_endpoint handles checking for duplicates based on URL/type pairs
            if (neozork::config_manager::add_endpoint(blockchain_info, new_endpoint_entry)) {
                std::cout << LOG_PREFIX << "  + Added new endpoint entry: Type=" << connection_type << ", URL=" << cleaned_url << std::endl;
                added_from_this_source++;
            } else {
                // Optional: Log that it was a duplicate if needed for debugging
                // std::cout << LOG_PREFIX << "  = Duplicate endpoint URL/Type skipped: " << cleaned_url << std::endl;
            }
        } // end for raw_url
        std::cout << "[Discovery] Added " << added_from_this_source << " new endpoint entries from source '" << source << "'." << std::endl;
        total_added_count += added_from_this_source;
        
        
    } // end for source loop
    
    std::cout << "[Discovery] Endpoint discovery finished for blockchain '" << blockchain_info.name << "'. Total new endpoint entries added: " << total_added_count << "." << std::endl;
    
    // 4. Save config if changes were made (endpoints added)
    // Note: We only save if new endpoints were actually added to the vector.
    // If add_endpoint only returned false (duplicates), we don't need to save.
    if (total_added_count > 0) {
        try {
            std::cout << "[Discovery] Saving updated configuration..." << std::endl;
            neozork::config_manager::save_config(config);
            std::cout << "[Discovery] Configuration saved successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Discovery] ERROR saving config after discovery: " << e.what() << std::endl;
            // Return false as saving failed, even if discovery technically found things
            return false;
        }
    } else {
        std::cout << "[Discovery] No new endpoints added, configuration file not modified." << std::endl;
    }
    
    return true; // Indicate discovery process completed (even if 0 endpoints were added)
}

} // namespace neozork::endpoint_discovery
