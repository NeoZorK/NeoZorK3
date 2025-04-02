#include "endpoint_discovery.h"
#include "connection_manager.h" // For HTTP requests
#include "config_manager.h"     // For finding blockchain and adding endpoints
#include <iostream>
#include <string>
#include <vector>
#include <sstream> // For parsing strings
#include <stdexcept>
#include <algorithm> // For std::find_if
#include <cctype>    // For isspace

// Connect nlohmann/json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace neozork::endpoint_discovery {

// --- Utility functions ---

  // Deletes leading and trailing whitespace from a string
  std::string trim_string(const std::string& str) {
      size_t first = str.find_first_not_of(" \t\n\r\f\v");
      if (std::string::npos == first) {
          return str; // String is all whitespace
      }
      size_t last = str.find_last_not_of(" \t\n\r\f\v");
      return str.substr(first, (last - first + 1));
  }

  // Deletes leading and trailing quotes from a string
  std::string trim_quotes(const std::string& str) {
      if (str.length() >= 2 &&
          ((str.front() == '"' && str.back() == '"') ||
           (str.front() == '\'' && str.back() == '\''))) {
          return str.substr(1, str.length() - 2);
      }
      return str;
  }

  // Check if a string contains a placeholder
  bool contains_placeholder(const std::string& url) {
      return url.find("${") != std::string::npos && url.find("}") != std::string::npos;
  }

// --- Endpoint discovery functions --

// Helper function to parse a simple URL list (one URL per line)
std::vector<std::string> parse_simple_url_list(const std::string& content) {
       std::vector<std::string> urls;
       std::stringstream ss(content);
       std::string line;
       while (std::getline(ss, line)) {
           std::string cleaned_url = trim_quotes(trim_string(line));
           // Skip empty lines and comments
           if (!cleaned_url.empty() && cleaned_url[0] != '#') {
               urls.push_back(cleaned_url);
           }
       }
       return urls;
   }


// Helper function to parse a JSON object with Ethereum lists
    std::vector<std::string> parse_ethereum_lists_json(const std::string& content) {
         std::vector<std::string> urls;
         try {
             json j = json::parse(content);
             // Search for 'rpc' array
             if (j.contains("rpc") && j.at("rpc").is_array()) {
                 for (const auto& item : j.at("rpc")) {
                     if (item.is_string()) {
                         // Add URL to the list
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


// Main function for endpoint discovery
bool discover_endpoints(
       const std::string& blockchain_name,
       const std::vector<std::string>& sources,
                        neozork::config_manager::struct_config& config)
   {
       std::cout << "Starting endpoint discovery for blockchain: " << blockchain_name << "..." << std::endl;

       auto blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name);
       if (!blockchain_ref_opt) {
            throw std::runtime_error("Blockchain '" + blockchain_name + "' not found in configuration. Cannot add endpoints.");
       }
       neozork::config_manager::struct_blockchain_info& blockchain_info = blockchain_ref_opt.value().get();

       int added_count = 0;

       for (const std::string& source : sources) {
           std::cout << "  Processing source: " << source << std::endl;
           std::vector<std::string> raw_urls;
           bool is_json_source = (source.length() >= 5 && source.substr(source.length() - 5) == ".json");

           if (source.rfind("https://", 0) == 0 || source.rfind("http://", 0) == 0) {
                size_t host_start = source.find("://") + 3;
                size_t path_start = source.find('/', host_start);
                if (path_start == std::string::npos) {
                     std::cerr << "    ERROR: Invalid URL format: " << source << std::endl;
                     continue;
                }
                std::string host = source.substr(host_start, path_start - host_start);
                std::string path = source.substr(path_start);

                // Добавляем User-Agent, т.к. некоторые серверы (особенно GitHub) могут его требовать
                neozork::connection_manager::http_headers headers = {{"User-Agent", "NeoZorK3_Discovery_Bot/0.1"}};
                auto response_body_opt = neozork::connection_manager::https_get(host, path, headers);

                if (response_body_opt) {
                    std::cout << "    Downloaded content from " << source << std::endl;
                    // Выбираем парсер в зависимости от типа источника
                    if (is_json_source) {
                        raw_urls = parse_ethereum_lists_json(response_body_opt.value());
                    } else {
                        raw_urls = parse_simple_url_list(response_body_opt.value());
                    }
                    std::cout << "    Parsed " << raw_urls.size() << " potential URLs." << std::endl;
                } else {
                     std::cerr << "    ERROR: Failed to download content from " << source << std::endl;
                     continue;
                }

           } else {
                std::cerr << "    WARNING: Source type '" << source << "' not yet supported (assuming local file path - not implemented)." << std::endl;
                continue;
           }

           // 3. Очистить URL, проверить плейсхолдеры и добавить уникальные эндпоинты
           for (const auto& raw_url : raw_urls) {
                // Применяем очистку
                std::string cleaned_url = trim_quotes(trim_string(raw_url));

                if (cleaned_url.empty()) {
                    continue; // Пропускаем пустые
                }

                // Проверяем на плейсхолдеры
                if (contains_placeholder(cleaned_url)) {
                     std::cout << "    WARNING: Skipping URL with placeholder: " << cleaned_url << std::endl;
                     continue; // Пропускаем URL с ${...}
                }

                // Создаем новый эндпоинт
                neozork::config_manager::struct_endpoint new_endpoint;
                new_endpoint.url = cleaned_url; // Сохраняем очищенный URL

                // TODO: Позже, при рефакторинге структуры, здесь нужно будет
                // определять тип (https/wss) из URL и добавлять в map, а не просто url.

                if (neozork::config_manager::add_endpoint(blockchain_info, new_endpoint)) {
                    added_count++;
                }
           }
       }

       std::cout << "Endpoint discovery finished. Added " << added_count << " new valid endpoints for " << blockchain_name << "." << std::endl;

       if (added_count > 0) {
            try {
                neozork::config_manager::save_config(config);
                std::cout << "Configuration saved successfully." << std::endl;
            } catch (const std::exception& e) {
                 std::cerr << "ERROR saving config after discovery: " << e.what() << std::endl;
                 return false;
            }
       }
       return true;
}

} // namespace neozork::endpoint_discovery
