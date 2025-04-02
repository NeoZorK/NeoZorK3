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
                    if (!url.empty()) { urls.push_back(url); }
                } else { std::cerr << LOG_PREFIX << "WARNING: Non-string item found in 'rpc' array: " << item.dump() << std::endl; }
            }
        } else { std::cerr << LOG_PREFIX << "WARNING: JSON source is not an object or does not contain a valid 'rpc' array (expected ethereum-lists format)." << std::endl; }
    } catch (const std::exception& e) {
        std::cerr << LOG_PREFIX << "ERROR processing ethereum-lists JSON: " << e.what() << std::endl;
    }
    std::cout << LOG_PREFIX << "Extracted " << urls.size() << " URLs from ethereum-lists format." << std::endl;
    return urls;
}


// --- REWRITTEN parse_chainlist_rpcs_json based on correct structure ---
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
                             if (!rpc_item.is_object() || !rpc_item.contains("url")) { continue; } // Skip invalid rpc entries

                             std::string current_url = rpc_item.value("url", "");
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


// --- Main discovery function (Handles source selection and parsing) ---
// (No changes needed in this function)
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
        new_bc.name = blockchain_name_or_id;
        try {
             long long id_ll = std::stoll(blockchain_name_or_id);
             if (id_ll > 0 && id_ll <= std::numeric_limits<int>::max()) {
                 new_bc.network_id = static_cast<int>(id_ll);
                 std::cout << LOG_PREFIX << "Interpreted '" << blockchain_name_or_id << "' as network ID: " << new_bc.network_id << std::endl;
             } else {
                 std::cerr << LOG_PREFIX << "WARNING: Input '" << blockchain_name_or_id << "' interpreted as numeric ID, but it's out of valid range or non-positive. Setting network ID to 0." << std::endl;
                 new_bc.network_id = 0;
             }
        } catch (const std::invalid_argument&) {
             std::cout << LOG_PREFIX << "Input '" << blockchain_name_or_id << "' is not a number. Will treat as name only." << std::endl;
              std::cerr << LOG_PREFIX << "WARNING: Could not determine network ID for name '" << blockchain_name_or_id << "'. Setting network ID to 0. This might prevent Chainlist filtering!" << std::endl;
              new_bc.network_id = 0;
        } catch (const std::out_of_range&) {
             std::cerr << LOG_PREFIX << "WARNING: Input '" << blockchain_name_or_id << "' is numeric but too large for network ID. Setting network ID to 0." << std::endl;
             new_bc.network_id = 0;
        }

        if (neozork::config_manager::add_blockchain(config, new_bc)) {
             std::cout << LOG_PREFIX << "Successfully added new blockchain '" << new_bc.name << "' (ID: " << new_bc.network_id << ") to config." << std::endl;
             auto newly_added_blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
             if (newly_added_blockchain_ref_opt) {
                  blockchain_info_ptr = &newly_added_blockchain_ref_opt.value().get();
             } else {
                  std::cerr << "[Discovery] CRITICAL ERROR: Failed to find newly added blockchain '" << blockchain_name_or_id << "' immediately after adding it!" << std::endl;
                   return false;
             }
        } else {
             std::cerr << "[Discovery] ERROR: Failed to add new blockchain '" << blockchain_name_or_id << "' to config (already exists or other error)." << std::endl;
             return false;
        }
    } else {
        blockchain_info_ptr = &blockchain_ref_opt.value().get();
         std::cout << LOG_PREFIX << "Found existing blockchain '" << blockchain_info_ptr->name << "' (ID: " << blockchain_info_ptr->network_id << ") in config." << std::endl;
    }

     if (!blockchain_info_ptr) {
         std::cerr << "[Discovery] CRITICAL ERROR: Blockchain info pointer is null after find/create attempt. Cannot proceed." << std::endl;
         return false;
     }
     neozork::config_manager::struct_blockchain_info& blockchain_info = *blockchain_info_ptr;
     int target_network_id = blockchain_info.network_id;
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

        if (lower_source == "chainlist") {
            url_to_download = "https://chainlist.org/rpcs.json";
            parser_to_use = ParserType::CHAINLIST;
            source_type_name = "Chainlist Keyword";
            std::cout << LOG_PREFIX << "Source identified as '" << source_type_name << "'. URL: " << url_to_download << std::endl;
        } else if (lower_source == "ethereum-lists") {
            url_to_download = "https://raw.githubusercontent.com/ethereum-lists/chains/master/_data/chains/eip155-1.json";
            parser_to_use = ParserType::ETH_LISTS;
            source_type_name = "Ethereum-Lists Keyword";
            std::cout << LOG_PREFIX << "Source identified as '" << source_type_name << "'. Using fixed URL: " << url_to_download << std::endl;
        } else if (source.rfind("https://", 0) == 0 || source.rfind("http://", 0) == 0) {
            url_to_download = source;
            parser_to_use = ParserType::AUTO_DETECT;
            source_type_name = "Direct URL";
            std::cout << LOG_PREFIX << "Source identified as '" << source_type_name << "'. URL: " << url_to_download << std::endl;
        } else {
            std::cerr << LOG_PREFIX << "WARNING: Source '" << source << "' is not a recognized keyword ('chainlist', 'ethereum-lists') or http(s) URL. Skipping." << std::endl;
            continue;
        }

        // --- Download content ---
        std::optional<std::string> response_body_opt;
        if (!url_to_download.empty()) {
             std::cout << LOG_PREFIX << "Attempting to download from: " << url_to_download << std::endl;
            size_t host_start = url_to_download.find("://") + 3;
            size_t path_start = url_to_download.find('/', host_start);
             std::string host, path;

             if (path_start == std::string::npos) {
                  host = url_to_download.substr(host_start);
                  path = "/";
                  std::cout << LOG_PREFIX << "  URL has no path, using '/'." << std::endl;
             } else {
                  host = url_to_download.substr(host_start, path_start - host_start);
                  path = url_to_download.substr(path_start);
             }

             if (host.empty()) {
                  std::cerr << LOG_PREFIX << "ERROR: Could not extract host from URL: " << url_to_download << ". Skipping." << std::endl;
                  continue;
             }

             std::cout << LOG_PREFIX << "  Host: " << host << ", Path: " << path << std::endl;
             neozork::connection_manager::http_headers headers = {{"User-Agent", "NeoZorK3_Discovery_Bot/0.1"}};
             response_body_opt = neozork::connection_manager::https_get(host, path, headers);

              if (response_body_opt) {
                   std::cout << LOG_PREFIX << "Download successful. Response size: " << response_body_opt.value().length() << " bytes." << std::endl;
              } else {
                   std::cerr << LOG_PREFIX << "ERROR: Failed to download content from " << url_to_download << ". Skipping this source." << std::endl;
                   continue;
              }
        } else {
             std::cerr << LOG_PREFIX << "ERROR: url_to_download is empty for source '" << source << "'. Skipping." << std::endl;
             continue;
        }

        // --- Parse the downloaded content ---
        if (response_body_opt) {
             const std::string& response_body = response_body_opt.value();
             std::cout << LOG_PREFIX << "Parsing content using method: ";

             switch (parser_to_use) {
                 case ParserType::CHAINLIST:
                     std::cout << "Chainlist Parser (Filter ID: " << target_network_id << ")" << std::endl;
                     raw_urls = parse_chainlist_rpcs_json(response_body, target_network_id); // Use the rewritten parser
                     break;
                 case ParserType::ETH_LISTS:
                      std::cout << "Ethereum-Lists Parser" << std::endl;
                      raw_urls = parse_ethereum_lists_json(response_body);
                     break;
                 case ParserType::AUTO_DETECT:
                     // Keep the auto-detect logic, it will now try the REWRITTEN Chainlist parser first if content is an array
                     std::cout << "Auto-Detect Parser" << std::endl;
                     std::cout << LOG_PREFIX << "Auto-detecting format for URL: " << source << std::endl;
                     try {
                         if (json::accept(response_body)) {
                             json j_test = json::parse(response_body);
                             if (j_test.is_array()) {
                                 std::cout << LOG_PREFIX << "  Content is JSON array, trying REWRITTEN Chainlist parser..." << std::endl;
                                 raw_urls = parse_chainlist_rpcs_json(response_body, target_network_id); // Tries the correct structure now
                             } else if (j_test.is_object()) {
                                  std::cout << LOG_PREFIX << "  Content is JSON object, trying Ethereum-lists parser..." << std::endl;
                                 raw_urls = parse_ethereum_lists_json(response_body);
                             } else {
                                  std::cout << LOG_PREFIX << "  Content is valid JSON but neither array nor object? Trying simple list parser..." << std::endl;
                                 raw_urls = parse_simple_url_list(response_body);
                             }
                         } else {
                              std::cout << LOG_PREFIX << "  Content is not valid JSON. Trying simple list parser..." << std::endl;
                             raw_urls = parse_simple_url_list(response_body);
                         }
                     } catch (const std::exception& e) {
                          std::cerr << LOG_PREFIX << "  Error during auto-detect parsing: " << e.what() << ". Falling back to simple list parser..." << std::endl;
                           if (raw_urls.empty()) {
                               raw_urls = parse_simple_url_list(response_body);
                           }
                     }
                     if (raw_urls.empty()) {
                         std::cerr << LOG_PREFIX << "WARNING: Auto-detect failed to extract URLs using known formats from: " << source << std::endl;
                     } else {
                          std::cout << LOG_PREFIX << "  Auto-detect successful using one of the parsers." << std::endl;
                     }
                     break;

                 case ParserType::SIMPLE_LIST:
                       std::cout << "Simple List Parser" << std::endl;
                      raw_urls = parse_simple_url_list(response_body);
                      break;

                 case ParserType::UNKNOWN:
                 default:
                     std::cerr << "Internal Error: Unknown parser type selected." << std::endl;
                     break;
             }
              std::cout << LOG_PREFIX << "Parsing finished for source '" << source << "'. Found " << raw_urls.size() << " potential URLs." << std::endl;
        }
        // else: download failed, already logged

        // 3. Process parsed URLs: clean, determine type/placeholder, add/update config
        int added_from_this_source = 0;
        for (const auto& raw_url : raw_urls) {
            std::string cleaned_url = trim_string(trim_quotes(raw_url));
            if (cleaned_url.empty()) continue;

            auto connection_type_opt = get_connection_type_from_url(cleaned_url);
            if (!connection_type_opt) {
                continue;
            }
            std::string connection_type = connection_type_opt.value();
            auto placeholder_opt = extract_placeholder_name(cleaned_url);

            neozork::config_manager::struct_endpoint new_endpoint;
            new_endpoint.connection_urls[connection_type] = cleaned_url;
            if (placeholder_opt) {
                 new_endpoint.required_api_key_placeholder = placeholder_opt;
            }

            if (neozork::config_manager::add_endpoint(blockchain_info, new_endpoint)) {
                 added_from_this_source++;
            }
        } // end for raw_url
         std::cout << "[Discovery] Added " << added_from_this_source << " new endpoint entries from source '" << source << "'." << std::endl;
         total_added_count += added_from_this_source;


    } // end for source loop

    std::cout << "[Discovery] Endpoint discovery finished for blockchain '" << blockchain_info.name << "'. Total new endpoint entries added: " << total_added_count << "." << std::endl;

    // 4. Save config if changes were made
    if (total_added_count > 0) {
        try {
            std::cout << "[Discovery] Saving updated configuration..." << std::endl;
            neozork::config_manager::save_config(config);
            std::cout << "[Discovery] Configuration saved successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Discovery] ERROR saving config after discovery: " << e.what() << std::endl;
            return false;
        }
    } else {
         std::cout << "[Discovery] No new endpoints added, configuration file not modified." << std::endl;
    }

    return true;
}

} // namespace neozork::endpoint_discovery
