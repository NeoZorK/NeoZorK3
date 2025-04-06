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
bool discover_endpoints(
        const std::string& blockchain_name_or_id,
        const std::vector<std::string>& sources,
        neozork::config_manager::struct_config& config)
{
    std::cout << "[Discovery] Starting endpoint discovery for blockchain: '" << blockchain_name_or_id << "'..." << std::endl;

    auto blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
    neozork::config_manager::struct_blockchain_info* blockchain_info_ptr = nullptr;
    int target_network_id = 0; // Определим ID здесь
    bool is_new_blockchain = false;

    if (!blockchain_ref_opt) {
        is_new_blockchain = true;
        std::cout << LOG_PREFIX << "Blockchain '" << blockchain_name_or_id << "' not found in config. Attempting to create..." << std::endl;
        neozork::config_manager::struct_blockchain_info new_bc;
        std::string found_name = blockchain_name_or_id; // Изначально имя = введенная строка

        // Пытаемся распознать ID
        bool input_is_id = false;
        try {
             long long id_ll = std::stoll(blockchain_name_or_id);
             if (id_ll > 0 && id_ll <= std::numeric_limits<int>::max()) {
                 target_network_id = static_cast<int>(id_ll);
                 input_is_id = true;
                 std::cout << LOG_PREFIX << "Input interpreted as network ID: " << target_network_id << std::endl;
             } else {
                 target_network_id = 0; // Невалидный ID
             }
        } catch (...) { target_network_id = 0; /* Не число */ }

        // Если пользователь ввел ID, пытаемся найти имя
        if (input_is_id) {
             // Скачиваем основной список сетей для поиска имени
             std::optional<std::string> chain_list_json = download_master_chain_list();
             if (chain_list_json) {
                 std::optional<std::string> name_from_list = find_chain_name_from_id(*chain_list_json, target_network_id);
                 if (name_from_list) {
                     found_name = *name_from_list; // Используем найденное имя
                     std::cout << LOG_PREFIX << "Found name '" << found_name << "' for ID " << target_network_id << " from master list." << std::endl;
                 } else {
                     std::cerr << LOG_PREFIX << "WARN: Could not find name for ID " << target_network_id << " in master list. Using ID as name." << std::endl;
                     found_name = std::to_string(target_network_id); // Оставляем ID как имя
                 }
             } else {
                 std::cerr << LOG_PREFIX << "WARN: Failed to download master list to find name for ID " << target_network_id << ". Using ID as name." << std::endl;
                  found_name = std::to_string(target_network_id); // Оставляем ID как имя
             }
        } else {
             // Пользователь ввел имя, ID пока неизвестен (0)
             target_network_id = 0;
              std::cerr << LOG_PREFIX << "WARN: Network ID for name '" << blockchain_name_or_id << "' is unknown (0). Chainlist filtering might fail." << std::endl;
        }

        // Устанавливаем окончательные значения для новой записи
        new_bc.name = found_name;
        new_bc.network_id = target_network_id;

        // Добавляем в конфиг
        if (neozork::config_manager::add_blockchain(config, new_bc)) {
             std::cout << LOG_PREFIX << "Successfully added new blockchain '" << new_bc.name << "' (ID: " << new_bc.network_id << ") to config." << std::endl;
             auto re_find = neozork::config_manager::find_blockchain(config, new_bc.name); // Ищем по имени для получения ссылки
             if (re_find) { blockchain_info_ptr = &re_find.value().get(); }
             else { /* Critical error */ std::cerr << "[Discovery] CRITICAL: Cannot re-find added blockchain!" << std::endl; return false; }
        } else {
             // Ошибка добавления (уже существует - такого не должно быть по идее)
             std::cerr << "[Discovery] ERROR: Failed to add blockchain, but it wasn't found initially?" << std::endl;
              // Попробуем найти снова на всякий случай
              auto existing = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
              if(existing) { blockchain_info_ptr = &existing.value().get(); is_new_blockchain = false; } // Нашли существующий
              else { std::cerr << "[Discovery] CRITICAL: Cannot add or find blockchain!" << std::endl; return false; } // Все равно не нашли
        }
    } else {
        // Блокчейн уже существовал
        blockchain_info_ptr = &blockchain_ref_opt.value().get();
        target_network_id = blockchain_info_ptr->network_id; // Используем ID из конфига
         std::cout << LOG_PREFIX << "Found existing blockchain '" << blockchain_info_ptr->name << "' (ID: " << target_network_id << ") in config." << std::endl;
    }

    // Финальная проверка указателя
     if (!blockchain_info_ptr) { std::cerr << "[Discovery] CRITICAL: Blockchain info pointer is null." << std::endl; return false; }
     neozork::config_manager::struct_blockchain_info& blockchain_info = *blockchain_info_ptr;
     // Используем ID из структуры, так как он мог быть найден или установлен в 0
     target_network_id = blockchain_info.network_id;
     std::cout << LOG_PREFIX << "Processing discovery using Network ID: " << target_network_id << std::endl;

    // --- Остальная часть функции ---
    int total_added_count = 0;
    neozork::ui::start_progress("Processing Sources", static_cast<long long>(sources.size()));
    int source_index = 0;

    for (const std::string& source : sources) {
        // ... (логика обработки источников, скачивания, парсинга raw_urls - без изменений) ...
         source_index++;
         std::vector<std::string> raw_urls; std::string url_to_download; enum class PT {U,C,E,S,A} parser_to_use=PT::U; std::string ls=source; std::transform(ls.begin(),ls.end(),ls.begin(),[](unsigned char c){return std::tolower(c);});
         if(ls=="chainlist"){url_to_download="https://chainid.network/chains.json";parser_to_use=PT::C;}else if(ls=="ethereum-lists"){url_to_download="https://...eip155-1.json";parser_to_use=PT::E;}else if(source.rfind("https://",0)==0||source.rfind("http://",0)==0){url_to_download=source;parser_to_use=PT::A;}else{std::cerr<<"\n"<<LOG_PREFIX<<"WARN: Unknown source: "<<source<<". Skip."<<std::endl; neozork::ui::update_progress(source_index); continue;}
         neozork::connection_manager::connection_result download_result; if(!url_to_download.empty()){size_t hs=url_to_download.find("://");if(hs==std::string::npos){neozork::ui::update_progress(source_index); continue;}hs+=3;size_t ps=url_to_download.find('/',hs);std::string h,p;if(ps==std::string::npos){h=url_to_download.substr(hs);p="/";}else{h=url_to_download.substr(hs,ps-hs);p=url_to_download.substr(ps);}if(h.empty()){neozork::ui::update_progress(source_index); continue;} neozork::connection_manager::http_headers hdrs={{"User-Agent","N3D"},{"Accept","*/*"}}; download_result=neozork::connection_manager::https_get(h,p,hdrs); if(!(!download_result.error_message&&download_result.body.has_value())){std::cerr<<"\n"<<LOG_PREFIX<<"ERR download "<<url_to_download<<". Skip."<<std::endl;neozork::ui::update_progress(source_index); continue;}}else continue;
         if(download_result.body){const std::string& rb=download_result.body.value(); switch(parser_to_use){case PT::C:raw_urls=parse_chainlist_rpcs_json(rb,target_network_id);break;case PT::E:raw_urls=parse_ethereum_lists_json(rb);break;case PT::A:try{if(json::accept(rb)){json jt=json::parse(rb);if(jt.is_array())raw_urls=parse_chainlist_rpcs_json(rb,target_network_id);else if(jt.is_object())raw_urls=parse_ethereum_lists_json(rb);if(raw_urls.empty())raw_urls=parse_simple_url_list(rb);}else raw_urls=parse_simple_url_list(rb);}catch(...){raw_urls=parse_simple_url_list(rb);}if(raw_urls.empty())std::cerr<<"\n"<<LOG_PREFIX<<"WARN: Auto-detect fail "<<source<<std::endl;break;case PT::S:raw_urls=parse_simple_url_list(rb);break;default:break;}}

        // Process parsed URLs
        int added_from_this_source = 0;
        for (const auto& raw_url : raw_urls) {
            // ... (логика добавления эндпоинта в blockchain_info через add_endpoint) ...
             std::string cleaned_url=trim_string(trim_quotes(raw_url));if(cleaned_url.empty()||contains_placeholder(cleaned_url))continue; auto ct_opt=get_connection_type_from_url(cleaned_url);if(!ct_opt)continue; std::string connection_type=ct_opt.value(); neozork::config_manager::struct_endpoint new_ep; new_ep.connection_urls[connection_type]=cleaned_url; if(neozork::config_manager::add_endpoint(blockchain_info, new_ep)) {added_from_this_source++;}
        }
        total_added_count += added_from_this_source;
        // Обновляем прогресс после обработки одного источника
        neozork::ui::update_progress(source_index);

    } // end for source loop

    neozork::ui::finish_progress();

    std::cout << "[Discovery] Endpoint discovery finished for blockchain '" << blockchain_info.name << "'. Total new endpoint entries added: " << total_added_count << "." << std::endl;

    // Сохраняем конфиг только если были добавлены *новые* эндпоинты ИЛИ если был добавлен *новый* блокчейн
    if (total_added_count > 0 || is_new_blockchain) {
        try {
            std::cout << "[Discovery] Saving updated configuration..." << std::endl;
            neozork::config_manager::save_config(config);
            std::cout << "[Discovery] Configuration saved successfully." << std::endl;
        } catch (const std::exception& e) { /* ... error handling ... */ return false; }
    } else {
         std::cout << "[Discovery] No new endpoints or blockchains added, configuration file not modified." << std::endl;
    }

    return true;
}
} // namespace neozork::endpoint_discovery
