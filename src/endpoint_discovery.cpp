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

// --- Utility functions (trim_string, trim_quotes, etc. - unchanged) ---
// ... (keep the existing utility functions here) ...
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

// --- Parsing functions (unchanged from previous version with logging) ---
// ... (keep parse_simple_url_list, parse_ethereum_lists_json, parse_chainlist_rpcs_json) ...
// Parses a simple URL list (one URL per line, ignores # comments)
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

// Parses JSON from sources like ethereum-lists/chains format
std::vector<std::string> parse_ethereum_lists_json(const std::string& content) {
    std::cout << LOG_PREFIX << "Attempting to parse content as ethereum-lists JSON..." << std::endl;
    std::vector<std::string> urls;
    try {
        json j = json::parse(content);
        std::cout << LOG_PREFIX << "JSON parsed successfully." << std::endl;
        // Check for 'rpc' array directly within the main object
        if (j.is_object() && j.contains("rpc") && j.at("rpc").is_array()) {
            std::cout << LOG_PREFIX << "Found 'rpc' array with " << j.at("rpc").size() << " items." << std::endl;
            for (const auto& item : j.at("rpc")) {
                if (item.is_string()) {
                    std::string url = item.get<std::string>();
                    // Ethereum-lists format might contain placeholders like "${INFURA_API_KEY}"
                    // We keep them as is for now. Scanning phase will need to handle/ignore them.
                    if (!url.empty()) {
                         urls.push_back(url);
                    }
                } else {
                    std::cerr << LOG_PREFIX << "WARNING: Non-string item found in 'rpc' array: " << item.dump() << std::endl;
                }
            }
        } else {
            std::cerr << LOG_PREFIX << "WARNING: JSON source is not an object or does not contain a valid 'rpc' array." << std::endl;
        }
    } catch (const json::parse_error& e) {
        std::cerr << LOG_PREFIX << "ERROR: Failed to parse ethereum-lists JSON source: " << e.what() << std::endl;
    } catch (const json::type_error& e) {
         std::cerr << LOG_PREFIX << "ERROR: Type error processing ethereum-lists JSON source: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << LOG_PREFIX << "ERROR: Generic error processing ethereum-lists JSON source: " << e.what() << std::endl;
    }
    std::cout << LOG_PREFIX << "Extracted " << urls.size() << " URLs from ethereum-lists format." << std::endl;
    return urls;
}

// Parses JSON from chainlist.org/rpcs.json format
std::vector<std::string> parse_chainlist_rpcs_json(const std::string& content, int target_chain_id) {
    std::vector<std::string> urls;
    std::cout << LOG_PREFIX << "Attempting to parse content as Chainlist RPCs JSON for target chain ID: " << target_chain_id << "..." << std::endl;
    if (target_chain_id <= 0) {
         std::cerr << LOG_PREFIX << "WARNING: Cannot filter Chainlist by Chain ID because target ID is " << target_chain_id << ". Will attempt to parse all URLs." << std::endl;
         // If ID is unknown, we cannot effectively filter Chainlist.
         // Depending on requirements, we could return all URLs or none.
         // Returning none is safer to avoid adding incorrect endpoints.
         return urls; // Return empty if no valid target ID
    }
    try {
        json j = json::parse(content);
         std::cout << LOG_PREFIX << "Chainlist JSON parsed successfully." << std::endl;

        if (!j.is_array()) {
            std::cerr << LOG_PREFIX << "ERROR: Expected a JSON array from Chainlist source, but got type: " << j.type_name() << std::endl;
            return urls;
        }

        std::cout << LOG_PREFIX << "Found JSON array with " << j.size() << " potential endpoint entries." << std::endl;
        int found_count = 0;
        int processed_count = 0;
        for (const auto& item : j) {
            processed_count++;
            if (!item.is_object()) {
                 std::cerr << LOG_PREFIX << "WARNING: Skipping non-object item in Chainlist array (Index: " << processed_count -1 << ")" << std::endl;
                 continue;
            }
            bool has_chain_id = item.contains("chainId");
            bool has_url = item.contains("url");

            if (!has_chain_id || !has_url) {
                 // std::cerr << LOG_PREFIX << "WARNING: Skipping entry " << processed_count << " missing 'chainId' or 'url'. Entry: " << item.dump(2) << std::endl;
                 continue;
            }

            int current_chain_id = -999;
            std::string current_url = "";
            try {
                 current_chain_id = item.value("chainId", -1);
                 current_url = item.value("url", "");

                if (current_chain_id == target_chain_id && !current_url.empty()) {
                    // Exclude URLs known to be placeholders often found in chainlist that are not real endpoints
                    if (current_url.find("${") == std::string::npos) { // Check if it contains "${"
                         std::cout << LOG_PREFIX << "  MATCH FOUND! Item " << processed_count << ": chainId=" << current_chain_id << " matches target=" << target_chain_id << ". Adding URL: " << current_url << std::endl;
                        urls.push_back(current_url);
                        found_count++;
                    } else {
                        // std::cout << LOG_PREFIX << "  Skipping Item " << processed_count << " (chainId=" << current_chain_id << "): URL contains placeholder - " << current_url << std::endl;
                    }
                } else if (current_url.empty()){
                     // std::cerr << LOG_PREFIX << "  Item " << processed_count << ": URL is empty. Skipping." << std::endl;
                }

            } catch (const json::type_error& te) {
                 std::cerr << LOG_PREFIX << "WARNING: Type error processing Chainlist entry " << processed_count << ": " << te.what() << " - Entry: " << item.dump(2) << std::endl;
            }
        } // end for loop
         std::cout << LOG_PREFIX << "Finished processing " << processed_count << " entries from Chainlist JSON." << std::endl;
         std::cout << LOG_PREFIX << "Found and added " << found_count << " URLs for target chain ID " << target_chain_id << "." << std::endl;

    } catch (const json::parse_error& e) {
         std::cerr << LOG_PREFIX << "ERROR: Failed to parse Chainlist JSON source: " << e.what() << std::endl;
    } catch (const std::exception& e) {
         std::cerr << LOG_PREFIX << "ERROR: Error processing Chainlist JSON source: " << e.what() << std::endl;
    }
    return urls;
}


// --- MODIFIED Main discovery function ---
bool discover_endpoints(
        const std::string& blockchain_name_or_id,
        const std::vector<std::string>& sources,
        neozork::config_manager::struct_config& config)
{
    std::cout << "[Discovery] Starting endpoint discovery for blockchain: '" << blockchain_name_or_id << "'..." << std::endl;

    // 1. Find or create the target blockchain in the config (Logic remains the same)
    // ... (keep the existing find/create logic using blockchain_info_ptr) ...
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
              std::cerr << LOG_PREFIX << "WARNING: Could not determine network ID for name '" << blockchain_name_or_id << "'. Setting network ID to 0. This might prevent Chainlist parsing!" << std::endl;
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
     // Warning moved inside parse_chainlist function

    int total_added_count = 0;

    // 2. Process each source URL/keyword - Updated Logic
    for (const std::string& source : sources) {
        std::cout << "[Discovery] Processing source: '" << source << "'" << std::endl;
        std::vector<std::string> raw_urls;

        std::string url_to_download;
        enum class ParserType { UNKNOWN, CHAINLIST, ETH_LISTS, SIMPLE_LIST, AUTO_DETECT };
        ParserType parser_to_use = ParserType::UNKNOWN;
        std::string source_type_name = "Unknown"; // For logging

        // --- Determine source type and URL ---
        std::string lower_source = source;
        std::transform(lower_source.begin(), lower_source.end(), lower_source.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        if (lower_source == "chainlist") {
            url_to_download = "https://chainlist.org/rpcs.json";
            parser_to_use = ParserType::CHAINLIST;
            source_type_name = "Chainlist Keyword";
            std::cout << LOG_PREFIX << "Source identified as '" << source_type_name << "'. URL: " << url_to_download << std::endl;
        } else if (lower_source == "ethereum-lists") {
            // Using Ethereum Mainnet list as the fixed representative for this keyword
            url_to_download = "https://chainid.network/chains.json"; // More comprehensive list
            // OR use the old one: "https://raw.githubusercontent.com/ethereum-lists/chains/master/_data/chains/eip155-1.json";
            parser_to_use = ParserType::ETH_LISTS; // Needs a dedicated parser for chains.json or use the old one for eip155-1 format
            source_type_name = "Ethereum-Lists Keyword";
             std::cout << LOG_PREFIX << "Source identified as '" << source_type_name << "'. URL: " << url_to_download << " (Note: Needs specific parser adjustment for format)" << std::endl;
             // ***** TODO: Implement or adjust parser for the chosen ethereum-lists URL format *****
             // For now, let's assume parse_ethereum_lists_json handles the eip155-1 format if using that URL.
             // If using chains.json, parse_ethereum_lists_json needs significant changes or a new parser.
             // Let's temporarily disable this option until parser is confirmed/fixed.
             std::cerr << LOG_PREFIX << "WARNING: 'ethereum-lists' keyword support is experimental and depends on the parser matching the URL format (" << url_to_download << "). Using old parser." << std::endl;
             // Revert to old URL and assume old parser for now:
             url_to_download = "https://raw.githubusercontent.com/ethereum-lists/chains/master/_data/chains/eip155-1.json";
             parser_to_use = ParserType::ETH_LISTS;


        } else if (source.rfind("https://", 0) == 0 || source.rfind("http://", 0) == 0) {
            url_to_download = source;
            parser_to_use = ParserType::AUTO_DETECT;
            source_type_name = "Direct URL";
            std::cout << LOG_PREFIX << "Source identified as '" << source_type_name << "'. URL: " << url_to_download << std::endl;
        } else {
            std::cerr << LOG_PREFIX << "WARNING: Source '" << source << "' is not a recognized keyword ('chainlist', 'ethereum-lists') or http(s) URL. Skipping." << std::endl;
            continue; // Skip to next source
        }

        // --- Download content ---
        std::optional<std::string> response_body_opt;
        if (!url_to_download.empty()) {
            // ... (Download logic remains the same as previous version) ...
            std::cout << LOG_PREFIX << "Attempting to download from: " << url_to_download << std::endl;
            size_t host_start = url_to_download.find("://") + 3;
            size_t path_start = url_to_download.find('/', host_start);
             std::string host, path;
             if (path_start == std::string::npos) { // Handle domain-only URLs like "https://cloudflare-eth.com"
                  host = url_to_download.substr(host_start);
                  path = "/"; // Assume root path if none specified
                  // std::cerr << LOG_PREFIX << "WARNING: URL seems to be domain only. Assuming path '/' for: " << url_to_download << std::endl;
                 // continue; // Or decide how to handle domain-only - maybe skip? Let's try path "/"
             } else {
                  host = url_to_download.substr(host_start, path_start - host_start);
                  path = url_to_download.substr(path_start);
             }

             std::cout << LOG_PREFIX << "  Host: " << host << ", Path: " << path << std::endl;
             neozork::connection_manager::http_headers headers = {{"User-Agent", "NeoZorK3_Discovery_Bot/0.1"}};
             response_body_opt = neozork::connection_manager::https_get(host, path, headers);
             // ... (Logging download success/failure) ...
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
             std::cout << LOG_PREFIX << "Parsing content using method: "; // Prepare for parser type log

             switch (parser_to_use) {
                 case ParserType::CHAINLIST:
                     std::cout << "Chainlist Parser (Filter ID: " << target_network_id << ")" << std::endl;
                     raw_urls = parse_chainlist_rpcs_json(response_body, target_network_id);
                     break;
                 case ParserType::ETH_LISTS:
                      std::cout << "Ethereum-Lists Parser" << std::endl;
                      // This parser expects the EIP-155 format (object with 'rpc' array)
                      raw_urls = parse_ethereum_lists_json(response_body);
                      // If using chains.json, this parser needs adjustment.
                     break;
                 case ParserType::AUTO_DETECT:
                     std::cout << "Auto-Detect Parser" << std::endl;
                     std::cout << LOG_PREFIX << "Auto-detecting format for URL: " << source << std::endl;
                     try {
                         // Check if content is valid JSON first
                         if (json::accept(response_body)) {
                             json j_test = json::parse(response_body); // Parse only if accepted
                             if (j_test.is_array()) {
                                 std::cout << LOG_PREFIX << "  Content is JSON array, trying Chainlist parser..." << std::endl;
                                 raw_urls = parse_chainlist_rpcs_json(response_body, target_network_id);
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
                     } catch (const std::exception& e) { // Catch potential errors during auto-detect parsing
                          std::cerr << LOG_PREFIX << "  Error during auto-detect parsing: " << e.what() << ". Falling back to simple list parser..." << std::endl;
                           // Ensure simple list is attempted if JSON parsing threw an exception
                           if (raw_urls.empty()) { // Only if no URLs were found yet
                               raw_urls = parse_simple_url_list(response_body);
                           }
                     }

                     // Final check after auto-detect attempts
                     if (raw_urls.empty()) {
                         std::cerr << LOG_PREFIX << "WARNING: Auto-detect failed to extract URLs using known formats from: " << source << std::endl;
                     } else {
                          std::cout << LOG_PREFIX << "  Auto-detect successful, found " << raw_urls.size() << " URLs." << std::endl;
                     }
                     break;

                 case ParserType::SIMPLE_LIST: // Should not be selected directly anymore, but handle just in case
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

        // 3. Process parsed URLs (Logic remains the same)
        // ... (keep the existing loop for processing raw_urls and adding endpoints) ...
        int added_from_this_source = 0;
        for (const auto& raw_url : raw_urls) {
            std::string cleaned_url = trim_string(trim_quotes(raw_url));
            if (cleaned_url.empty()) continue;

            auto connection_type_opt = get_connection_type_from_url(cleaned_url);
            if (!connection_type_opt) {
                // std::cerr << LOG_PREFIX << "  WARNING: Cannot determine connection type for URL: '" << cleaned_url << "'. Skipping." << std::endl;
                continue;
            }
            std::string connection_type = connection_type_opt.value();
            auto placeholder_opt = extract_placeholder_name(cleaned_url);

            bool url_type_combo_exists = false;
            int existing_endpoint_index = -1;
             // This simple check might not be sufficient if URLs are slightly different but point to same provider.
             // A more advanced check could involve normalizing URLs or comparing hosts.
            for(size_t i = 0; i < blockchain_info.endpoints.size(); ++i) {
                const auto& ep = blockchain_info.endpoints[i];
                auto it = ep.connection_urls.find(connection_type);
                if (it != ep.connection_urls.end() && it->second == cleaned_url) {
                    url_type_combo_exists = true;
                    existing_endpoint_index = i;
                    break;
                }
            }

            if (!url_type_combo_exists) {
                 // std::cout << LOG_PREFIX << "  Adding new endpoint entry for URL: " << cleaned_url << " (Type: " << connection_type << ")" << std::endl;
                neozork::config_manager::struct_endpoint new_endpoint;
                new_endpoint.connection_urls[connection_type] = cleaned_url;
                if (placeholder_opt) {
                    // std::cout << LOG_PREFIX << "    Requires API key placeholder: " << placeholder_opt.value() << std::endl;
                    new_endpoint.required_api_key_placeholder = placeholder_opt;
                }
                 // Use add_endpoint which should handle duplicates internally now
                 if (neozork::config_manager::add_endpoint(blockchain_info, new_endpoint)) {
                     added_from_this_source++;
                 } else {
                     // std::cout << LOG_PREFIX << "  Skipped adding endpoint (already exists based on add_endpoint check): " << cleaned_url << std::endl;
                 }
            } else {
                  // std::cout << LOG_PREFIX << "  Skipped adding URL (already exists in endpoint entry " << existing_endpoint_index << "): " << cleaned_url << std::endl;
            }
        } // end for raw_url
         std::cout << "[Discovery] Added " << added_from_this_source << " new endpoint entries from source '" << source << "'." << std::endl;
         total_added_count += added_from_this_source;


    } // end for source

    std::cout << "[Discovery] Endpoint discovery finished for blockchain '" << blockchain_info.name << "'. Total new endpoint entries added: " << total_added_count << "." << std::endl;

    // 4. Save config if changes were made (Logic remains the same)
    // ... (keep existing save logic) ...
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

    return true; // Discovery process completed
}


// --- Make sure add_endpoint is declared/implemented ---
// Declaration in include/config_manager.h:
// bool add_endpoint(struct_blockchain_info& bc_info_ref, const struct_endpoint& new_endpoint);

// Implementation in src/config_manager.cpp:
/*
bool add_endpoint(struct_blockchain_info& bc_info_ref, const struct_endpoint& new_endpoint) {
    if (new_endpoint.connection_urls.empty()) {
         std::cerr << "Warning: Attempted to add an endpoint with no connection URLs." << std::endl;
         return false;
    }

    // Check if an endpoint with *any* of the same URLs already exists
    for (const auto& [type, url] : new_endpoint.connection_urls) {
        for (const auto& existing_ep : bc_info_ref.endpoints) {
            auto it = existing_ep.connection_urls.find(type);
            if (it != existing_ep.connection_urls.end() && it->second == url) {
                 // Found a duplicate URL/Type combination
                 // std::cout << "Debug: Duplicate endpoint URL found: " << url << " (Type: " << type << ")" << std::endl;
                 return false; // Indicate duplicate, do not add
            }
        }
    }

    // No duplicates found, add the new endpoint
    bc_info_ref.endpoints.push_back(new_endpoint);
    return true; // Indicate success
}
*/


} // namespace neozork::endpoint_discovery
