// src/main.cpp

// Standard library includes
#include <iostream>
#include <vector>
#include <string>
#include <chrono>   // Для измерения времени
#include <stdexcept> // Для std::stoull

// Include third-party libraries
#define CPPHTTPLIB_OPENSSL_SUPPORT // Включаем поддержку SSL (HTTPS) - Убедитесь, что OpenSSL слинкован!
#include <httplib.h>
#include <nlohmann/json.hpp>

// Include project's own header files from 'include/' directory
#include "main.h" // Содержит объявление query_fantom_block() теперь

// Use aliases for convenience
using json = nlohmann::json;

// --- Project Version ---
const std::string NEOZORK3_VERSION = "0.1.1"; // Пример: обновили версию

// --- Main Application Entry Point ---
int main(int argc, char* argv[]) {
    // Output project name and version
    std::cout << "--- NeoZorK3 CLI ---" << std::endl;
    std::cout << "Version: " << NEOZORK3_VERSION << std::endl;
    std::cout << "--------------------" << std::endl;

    // --- Argument Parsing ---
    parse_arguments(argc, argv);
    std::cout << "[INFO] Arguments parsed." << std::endl;

    // --- Configuration Loading ---
    // Placeholder
    std::cout << "[INFO] Configuration loaded." << std::endl;

    // --- Core Logic ---
    query_fantom_block(); // Вызываем новую функцию

    std::cout << "[INFO] NeoZorK3 finished execution." << std::endl;

    return 0;
}

// --- Function Definitions ---

// Placeholder function for parsing command line arguments
void parse_arguments(int argc, char* argv[]) {
    std::cout << "[DEBUG] Received " << argc << " arguments:" << std::endl;
    for (int i = 0; i < argc; ++i) {
        std::cout << "  argv[" << i << "]: " << argv[i] << std::endl;
    }
    // Add actual parsing logic here
}

// Function to query Fantom blockchain for the latest block and measure latency
void query_fantom_block() {
    std::cout << "[INFO] Querying Fantom blockchain..." << std::endl;

    // Define the public Fantom RPC endpoint URL
    // Можно выбрать и другие, например: https://rpc.ftm.tools/
    const std::string fantom_rpc_url = "https://rpc.ankr.com";
    const std::string fantom_rpc_path = "/fantom/";

    // Create the JSON-RPC request body
    json rpc_request;
    rpc_request["jsonrpc"] = "2.0";
    rpc_request["method"] = "eth_blockNumber";
    rpc_request["params"] = json::array(); // Пустой массив параметров
    rpc_request["id"] = 1; // Простой ID запроса

    // Serialize JSON request to string
    std::string request_body;
    try {
        request_body = rpc_request.dump(); // .dump() может выбросить исключение
        std::cout << "[DEBUG] Request Body: " << request_body << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to serialize JSON request: " << e.what() << std::endl;
        return; // Выход из функции при ошибке сериализации
    }


    try {
        // Create HTTPS client (SSL)
        // Убедитесь, что OpenSSL установлен и правильно слинкован в CMakeLists.txt
        // (раскомментируйте строки с OpenSSL в target_link_libraries и target_compile_definitions)
        #ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            httplib::SSLClient cli(fantom_rpc_url); // URL без path
            cli.set_connection_timeout(5, 0); // Таймаут соединения 5 сек
            cli.set_read_timeout(10, 0); // Таймаут чтения 10 сек
            // cli.enable_server_certificate_verification(false); // НЕ ИСПОЛЬЗОВАТЬ В PRODUCTION! Только для тестов без сертификатов.
        #else
            std::cerr << "[ERROR] CPPHTTPLIB_OPENSSL_SUPPORT is not defined. Cannot make HTTPS requests." << std::endl;
            // Возможно, стоит использовать HTTP endpoint, если доступен, или прервать выполнение
            // httplib::Client cli(fantom_rpc_url_http); // если есть http версия
            return;
        #endif

        std::cout << "[INFO] Sending request to " << fantom_rpc_url << fantom_rpc_path << std::endl;

        // Measure latency
        auto start_time = std::chrono::high_resolution_clock::now();

        // Send POST request
        auto res = cli.Post(fantom_rpc_path.c_str(), // Path эндпоинта
                            request_body,          // Тело запроса (JSON строка)
                            "application/json");   // Content-Type

        auto end_time = std::chrono::high_resolution_clock::now();

        // Calculate duration in milliseconds
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        long long latency_ms = duration.count();

        std::cout << "[INFO] Request Latency: " << latency_ms << " ms" << std::endl;

        // Check response
        if (res) {
            std::cout << "[DEBUG] Response Status: " << res->status << std::endl;
            std::cout << "[DEBUG] Response Body: " << res->body << std::endl;

            if (res->status == 200) {
                // Parse JSON response
                try {
                    json response_json = json::parse(res->body);

                    // Check for JSON-RPC error field
                    if (response_json.contains("error") && !response_json["error"].is_null()) {
                         std::cerr << "[ERROR] RPC Error: " << response_json["error"].dump(2) << std::endl;
                    }
                    // Check for result field
                    else if (response_json.contains("result") && !response_json["result"].is_null()) {
                        std::string latest_block_hex = response_json["result"];

                        // Convert hex string to decimal number
                        try {
                             unsigned long long latest_block_dec = std::stoull(latest_block_hex, nullptr, 16);
                             std::cout << "[INFO] Fantom Latest Block: " << latest_block_dec
                                       << " (Hex: " << latest_block_hex << ")" << std::endl;
                        } catch (const std::invalid_argument& ia) {
                             std::cerr << "[ERROR] Invalid hex format received for block number: " << latest_block_hex << std::endl;
                        } catch (const std::out_of_range& oor) {
                             std::cerr << "[ERROR] Block number out of range for unsigned long long: " << latest_block_hex << std::endl;
                        }
                    } else {
                        std::cerr << "[ERROR] Invalid JSON-RPC response: 'result' field missing or null." << std::endl;
                    }
                } catch (json::parse_error& e) {
                    std::cerr << "[ERROR] Failed to parse JSON response: " << e.what() << std::endl;
                }
            } else {
                std::cerr << "[ERROR] HTTP request failed with status: " << res->status << std::endl;
            }
        } else {
            // Handle cpp-httplib specific errors
            auto err = res.error();
            std::cerr << "[ERROR] HTTP request failed: " << httplib::to_string(err) << std::endl;
             if (err == httplib::Error::SSLConnection || err == httplib::Error::SSLLoadingCerts) {
                 std::cerr << "[HINT] Check OpenSSL installation and certificate configuration." << std::endl;
             }
        }

    } catch (const std::exception& e) {
        // Catch potential exceptions from client creation or other operations
        std::cerr << "[ERROR] Exception caught during HTTP operation: " << e.what() << std::endl;
    }

    std::cout << "[INFO] Fantom query finished." << std::endl;
}
