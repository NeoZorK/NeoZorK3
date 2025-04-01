// src/main.cpp

// Standard library includes
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>
#include <thread> // Для sleep_for в WS
#include <memory> // Для std::shared_ptr (нужен для WebSocketSession)

// Platform specific includes for IPC
#ifndef _WIN32 // Для Unix-подобных систем (macOS, Linux)
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h> // Для errno
#include <cstring> // Для strerror
#endif

// Include third-party libraries
// Определяем поддержку SSL ДО подключения httplib.h
// Вы можете управлять этим макросом через опцию CMake ENABLE_HTTPS_WSS
#if defined(ENABLE_HTTPS_WSS) && ENABLE_HTTPS_WSS
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#include <httplib.h> // Должен быть подключен ПОСЛЕ определения макроса (если он нужен)
#include <nlohmann/json.hpp>

// Include project's own header files
#include "main.h"

// Use aliases
using json = nlohmann::json;

// --- Project Version ---
const std::string NEOZORK3_VERSION = "0.1.5"; // Версия

// --- Helper Function for URL Parsing ---
bool parse_url(const std::string& url, std::string& scheme, std::string& host, int& port, std::string& path) {
    try {
        size_t scheme_end = url.find("://");
        if (scheme_end == std::string::npos) return false;
        scheme = url.substr(0, scheme_end);

        std::string location = url.substr(scheme_end + 3);
        size_t path_start = location.find('/');
        std::string host_port;

        if (path_start == std::string::npos) {
            host_port = location;
            path = "/";
        } else {
            host_port = location.substr(0, path_start);
            path = location.substr(path_start);
        }

        size_t port_start = host_port.find(':');
        if (port_start == std::string::npos) {
            host = host_port;
            if (scheme == "http" || scheme == "ws") port = 80;
            else if (scheme == "https" || scheme == "wss") port = 443;
            else return false;
        } else {
            host = host_port.substr(0, port_start);
            if (port_start + 1 >= host_port.length()) return false; // Port is empty after ':'
            port = std::stoi(host_port.substr(port_start + 1));
        }
        return !host.empty();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to parse URL '" << url << "': " << e.what() << std::endl;
        return false;
    }
}


// --- Main Application Entry Point ---
int main(int argc, char* argv[]) {
    std::cout << "--- NeoZorK3 CLI ---" << std::endl;
    std::cout << "Version: " << NEOZORK3_VERSION << std::endl;
    std::cout << "--------------------" << std::endl;

    std::vector<std::string> args(argv, argv + argc); // Используем vector для аргументов

    ConnectionType connection_type = ConnectionType::UNKNOWN;
    std::string endpoint;

    if (!parse_arguments(args, connection_type, endpoint)) {
        std::cerr << "Usage: " << (args.empty() ? "neozork3_cli" : args[0]) << " <flag> <endpoint>" << std::endl;
        std::cerr << "Flags:" << std::endl;
        std::cerr << "  --http   <url>     Use HTTP connection (e.g., http://fantom.publicnode.com)" << std::endl;
        std::cerr << "  --https  <url>     Use HTTPS connection (e.g., https://host:port)" << std::endl;
        std::cerr << "  --ws     <url>     Use WebSocket connection (e.g., ws://host:port) [Not fully supported]" << std::endl;
        std::cerr << "  --wss    <url>     Use Secure WebSocket connection (e.g., wss://host:port)" << std::endl;
        std::cerr << "  --ipc    <path>    Use IPC connection (e.g., /path/to/geth.ipc)" << std::endl;
        return 1;
    }

    std::cout << "[INFO] Arguments parsed. Type: " << static_cast<int>(connection_type)
              << ", Endpoint: " << endpoint << std::endl;

    query_blockchain(connection_type, endpoint);

    std::cout << "[INFO] NeoZorK3 finished execution." << std::endl;
    return 0;
}

// --- Function Definitions ---

// Реализация парсинга аргументов
bool parse_arguments(const std::vector<std::string>& args, ConnectionType& type, std::string& endpoint) {
     if (args.size() != 3) { return false; }

     const std::string& flag = args[1];
     endpoint = args[2];

     if (flag == "--http") { type = ConnectionType::HTTP; }
     else if (flag == "--https") { type = ConnectionType::HTTPS; }
     else if (flag == "--ws") { type = ConnectionType::WS; }
     else if (flag == "--wss") { type = ConnectionType::WSS; }
     else if (flag == "--ipc") { type = ConnectionType::IPC; }
     else { type = ConnectionType::UNKNOWN; std::cerr << "[ERROR] Unknown flag: " << flag << std::endl; return false; }

     if (endpoint.empty()) { std::cerr << "[ERROR] Endpoint cannot be empty." << std::endl; return false; }

     if (type != ConnectionType::IPC) {
         std::string prefix_http = "http://"; std::string prefix_https = "https://";
         std::string prefix_ws = "ws://"; std::string prefix_wss = "wss://";
         bool scheme_match = false;
         if ((type == ConnectionType::HTTP && endpoint.rfind(prefix_http, 0) == 0) ||
             (type == ConnectionType::HTTPS && endpoint.rfind(prefix_https, 0) == 0) ||
             (type == ConnectionType::WS && endpoint.rfind(prefix_ws, 0) == 0) ||
             (type == ConnectionType::WSS && endpoint.rfind(prefix_wss, 0) == 0)) {
             scheme_match = true;
         }
         if (!scheme_match) {
             std::cerr << "[ERROR] URL scheme does not match the provided flag (" << flag << ")." << std::endl;
             return false;
         }
     }
     return true;
}

// Основная функция-диспетчер
void query_blockchain(ConnectionType type, const std::string& endpoint) {
    switch (type) {
        case ConnectionType::HTTP:
            query_via_http(endpoint, false);
            break;
        case ConnectionType::HTTPS:
            #if defined(CPPHTTPLIB_OPENSSL_SUPPORT)
                query_via_http(endpoint, true);
            #else
                std::cerr << "[ERROR] HTTPS requested, but SSL support is not compiled in. Re-run CMake with -DENABLE_HTTPS_WSS=ON and ensure OpenSSL is installed." << std::endl;
            #endif
            break;
        case ConnectionType::WS:
            query_via_websocket(endpoint, false);
            break;
        case ConnectionType::WSS:
             #if defined(CPPHTTPLIB_OPENSSL_SUPPORT)
                query_via_websocket(endpoint, true);
             #else
                std::cerr << "[ERROR] WSS requested, but SSL support is not compiled in. Re-run CMake with -DENABLE_HTTPS_WSS=ON and ensure OpenSSL is installed." << std::endl;
             #endif
            break;
        case ConnectionType::IPC:
            query_via_ipc(endpoint);
            break;
        default:
            std::cerr << "[ERROR] Unknown connection type specified." << std::endl;
            break;
    }
}


// Реализация запроса через HTTP/HTTPS (Исправлено использование httplib::Result)
void query_via_http(const std::string& url, bool use_ssl) {
    std::cout << "[INFO] Querying via " << (use_ssl ? "HTTPS" : "HTTP") << " to " << url << std::endl;
    std::string scheme, host, path; int port;
    if (!parse_url(url, scheme, host, port, path)) { return; }

    json rpc_request;
    rpc_request["jsonrpc"] = "2.0";
    rpc_request["method"] = "eth_blockNumber";
    rpc_request["params"] = json::array();
    rpc_request["id"] = 1;
    std::string request_body = rpc_request.dump();
    std::cout << "[DEBUG] Request Body: " << request_body << std::endl;

    httplib::Result result; // <-- ИСПРАВЛЕНО: Объявляем без инициализации nullptr
    long long latency_ms = -1;

    try {
        auto start_time = std::chrono::high_resolution_clock::now();

        if (use_ssl) {
            #if defined(CPPHTTPLIB_OPENSSL_SUPPORT)
                httplib::SSLClient cli(host, port);
                cli.set_connection_timeout(5, 0);
                cli.set_read_timeout(10, 0);
                result = cli.Post(path.c_str(), request_body, "application/json"); // Присваиваем результат Post
            #else
                 std::cerr << "[ERROR] Internal: SSL requested but not compiled." << std::endl; return;
            #endif
        } else {
            httplib::Client cli(host, port);
            cli.set_connection_timeout(5, 0);
            cli.set_read_timeout(10, 0);
            result = cli.Post(path.c_str(), request_body, "application/json"); // Присваиваем результат Post
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        std::cout << "[INFO] HTTP(S) Latency: " << latency_ms << " ms" << std::endl;

        // Обработка ответа с использованием httplib::Result
        if (result) { // Проверяем успешность запроса через operator bool()
            std::cout << "[DEBUG] Response Status: " << result->status << std::endl; // Доступ к Response через ->
            std::cout << "[DEBUG] Response Body: " << result->body << std::endl;   // Доступ к Response через ->
            if (result->status == 200) {
                 try {
                     json response_json = json::parse(result->body);
                     if (response_json.contains("error") && !response_json["error"].is_null()) {
                          std::cerr << "[ERROR] RPC Error: " << response_json["error"].dump(2) << std::endl;
                     } else if (response_json.contains("result") && !response_json["result"].is_null()) {
                         std::string latest_block_hex = response_json["result"];
                         try {
                              unsigned long long latest_block_dec = std::stoull(latest_block_hex, nullptr, 16);
                              std::cout << "[INFO] Fantom Latest Block (HTTP/S): " << latest_block_dec << " (Hex: " << latest_block_hex << ")" << std::endl;
                         } catch (const std::exception& e) {
                              std::cerr << "[ERROR] Failed to convert hex block number '" << latest_block_hex << "': " << e.what() << std::endl;
                         }
                     } else { std::cerr << "[ERROR] Invalid JSON-RPC response: 'result' missing or null." << std::endl; }
                 } catch (json::parse_error& e) { std::cerr << "[ERROR] Failed to parse JSON response: " << e.what() << std::endl; }
            } else { std::cerr << "[ERROR] HTTP request completed but with status: " << result->status << std::endl; }
        } else {
             // Запрос не удался, получаем ошибку из Result
             httplib::Error req_error = result.error();
             std::cerr << "[ERROR] HTTP request failed: " << httplib::to_string(req_error) << std::endl;
             #if defined(CPPHTTPLIB_OPENSSL_SUPPORT)
                 if (use_ssl && (req_error == httplib::Error::SSLConnection || req_error == httplib::Error::SSLLoadingCerts)) {
                     std::cerr << "[HINT] Check OpenSSL installation and certificate configuration." << std::endl;
                 }
             #endif
        }

    } catch (const std::exception& e) {
        // Ловим исключения, которые могли возникнуть при создании клиента и т.д.
        std::cerr << "[ERROR] Exception caught during HTTP(S) operation: " << e.what() << std::endl;
    }
}


// Реализация запроса через WebSocket/WSS (Исправлена ошибка send_custom_request_ws)
void query_via_websocket(const std::string& url, bool use_ssl) {
    std::cout << "[INFO] Querying via " << (use_ssl ? "WSS" : "WS") << " to " << url << std::endl;
    std::string scheme, host, path; int port;
    if (!parse_url(url, scheme, host, port, path)) { return; }

    json rpc_request;
    rpc_request["jsonrpc"] = "2.0";
    rpc_request["method"] = "eth_blockNumber";
    rpc_request["params"] = json::array();
    rpc_request["id"] = 2; // Different ID for WS
    std::string request_body = rpc_request.dump();
    std::cout << "[DEBUG] Request Body: " << request_body << std::endl;

    try {
        std::string response_data;
        bool received = false;
        long long latency_ms = -1;

        // Callback functions
        auto on_message = [&](const std::string& data) {
            std::cout << "[DEBUG] WS Message Received: " << data << std::endl;
            if(data.find("\"id\":2") != std::string::npos || data.find("\"id\": 2") != std::string::npos) {
                 response_data = data;
                 received = true;
            } else { std::cout << "[DEBUG] WS Received non-matching message, ignoring." << std::endl; }
        };
        auto on_close = [&]() { std::cout << "[DEBUG] WS Connection closed by peer." << std::endl; if (!received) received = true; };
        auto on_error = [&](const httplib::Error& error_code) { std::cerr << "[ERROR] WS Error: " << httplib::to_string(error_code) << std::endl; received = true; };

        if (use_ssl) {
            #if defined(CPPHTTPLIB_OPENSSL_SUPPORT)
                httplib::SSLClient cli(host, port);
                cli.set_connection_timeout(5);

                std::cout << "[INFO] Connecting WSS..." << std::endl;
                auto start_time = std::chrono::high_resolution_clock::now();
                bool connected = false;

                // Используем connect_ws. Он возвращает shared_ptr на сессию.
                // shared_ptr здесь используется библиотекой для управления ресурсами сессии.
                std::shared_ptr<httplib::WebSocketSession> ws_session = cli.connect_ws(
                     path.c_str(), {}, // path, headers
                     [&](const char *message, size_t length) { on_message(std::string(message, length)); }, // on_message
                     [&]() { on_close(); }, // on_close
                     [&](const char *reason, size_t length) { std::cerr << "[ERROR] WS Disconnect: " << std::string(reason, length) << std::endl; received = true; }, // on_disconnect
                     [&](const httplib::Error& error_code) { on_error(error_code); } // on_error
                );

                if (ws_session) {
                    connected = true;
                    std::cout << "[DEBUG] WSS Connected. Sending request..." << std::endl;
                    if(!ws_session->send(request_body)) { // Отправляем запрос
                         std::cerr << "[ERROR] Failed to send WSS request." << std::endl;
                         connected = false; received = true;
                    }
                } else {
                    std::cerr << "[ERROR] Failed to establish WSS connection." << std::endl;
                    received = true;
                }

                if(connected) {
                    // Ожидание ответа
                    int wait_ms = 0; const int max_wait_ms = 10000;
                    while (!received && wait_ms < max_wait_ms) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); wait_ms += 10; }
                    auto end_time = std::chrono::high_resolution_clock::now();
                    if (received && !response_data.empty()) { latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count(); }
                    else if (!received) { std::cerr << "[ERROR] WSS response timeout." << std::endl; }
                    if (ws_session->is_valid()) { // Проверяем валидность перед закрытием
                        ws_session->close();
                    }
                }
            #else
                 std::cerr << "[ERROR] WSS requested, but SSL support is not compiled in." << std::endl; return;
            #endif
        } else {
            // **Исправление:** httplib::Client не имеет удобных методов для WS.
            std::cerr << "[ERROR] Non-SSL WebSocket (ws://) is not directly supported in this example using httplib::Client." << std::endl;
            std::cerr << "[HINT] Please use WSS (--wss flag) or implement non-SSL WS using a different library or method." << std::endl;
            return; // Прерываем попытку WS
        }

        // Обработка ответа
        if (latency_ms != -1 && !response_data.empty()) {
            std::cout << "[INFO] WS(S) Latency: " << latency_ms << " ms" << std::endl;
            try {
                json response_json = json::parse(response_data);
                if (response_json.contains("error") && !response_json["error"].is_null()) {
                    std::cerr << "[ERROR] RPC Error: " << response_json["error"].dump(2) << std::endl;
                } else if (response_json.contains("result") && !response_json["result"].is_null()) {
                    std::string latest_block_hex = response_json["result"];
                    try {
                        unsigned long long latest_block_dec = std::stoull(latest_block_hex, nullptr, 16);
                        std::cout << "[INFO] Fantom Latest Block (WS/S): " << latest_block_dec << " (Hex: " << latest_block_hex << ")" << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "[ERROR] Failed to convert hex block number '" << latest_block_hex << "': " << e.what() << std::endl;
                    }
                } else { std::cerr << "[ERROR] Invalid JSON-RPC response: 'result' missing or null." << std::endl; }
            } catch (json::parse_error& e) { std::cerr << "[ERROR] Failed to parse JSON response: " << e.what() << std::endl; }
        }

    } catch (const std::exception& e) {
         std::cerr << "[ERROR] Exception caught during WS(S) operation: " << e.what() << std::endl;
    }
    std::cout << "[INFO] WS(S) query finished." << std::endl;
}


// Реализация запроса через IPC (Unix)
void query_via_ipc(const std::string& path) {
    std::cout << "[INFO] Querying via IPC to " << path << std::endl;
    #ifndef _WIN32 // Код только для не-Windows систем
    json rpc_request;
    rpc_request["jsonrpc"] = "2.0";
    rpc_request["method"] = "eth_blockNumber";
    rpc_request["params"] = json::array();
    rpc_request["id"] = 3;
    std::string request_body = rpc_request.dump();
    std::cout << "[DEBUG] Request Body: " << request_body << std::endl;

    int sock = -1; // Используем int для дескриптора сокета
    long long latency_ms = -1;
    std::string response_data;

    try {
        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) { throw std::runtime_error(std::string("Failed to create socket: ") + strerror(errno)); }

        struct sockaddr_un server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sun_family = AF_UNIX;
        strncpy(server_addr.sun_path, path.c_str(), sizeof(server_addr.sun_path) - 1);

        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            throw std::runtime_error(std::string("Failed to connect to IPC socket '") + path + "': " + strerror(errno));
        }
        std::cout << "[INFO] Connected to IPC socket." << std::endl;

        auto start_time = std::chrono::high_resolution_clock::now();

        ssize_t bytes_sent = send(sock, request_body.c_str(), request_body.length(), 0);
        if (bytes_sent < 0 || static_cast<size_t>(bytes_sent) != request_body.length()) {
            // Убрали strerror(errno), т.к. он может быть не потокобезопасен в некоторых контекстах; используем простое сообщение
            throw std::runtime_error("Failed to send request via IPC.");
        }
        std::cout << "[DEBUG] IPC Request sent (" << bytes_sent << " bytes)." << std::endl;

        // Используем std::vector<char> как буфер
        std::vector<char> buffer(4096);
        ssize_t bytes_received_total = 0;
        ssize_t bytes_received_now = 0;
        std::cout << "[DEBUG] Waiting for IPC response..." << std::endl;

        struct timeval tv;
        tv.tv_sec = 10; // 10 секунд таймаут
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        do {
             // Используем buffer.data() - указатель на начало данных вектора
             bytes_received_now = recv(sock, buffer.data() + bytes_received_total, buffer.size() - bytes_received_total - 1, 0);
             if (bytes_received_now < 0) {
                 if (errno == EAGAIN || errno == EWOULDBLOCK) { std::cerr << "[ERROR] IPC receive timeout." << std::endl; break; }
                 throw std::runtime_error(std::string("Failed to receive response via IPC: ") + strerror(errno));
             }
             if (bytes_received_now == 0) { std::cout << "[DEBUG] IPC connection closed by peer." << std::endl; break; }
             bytes_received_total += bytes_received_now;
             if (buffer.size() - bytes_received_total < 1024) { buffer.resize(buffer.size() * 2); }
             // Простая проверка на конец JSON
             if (bytes_received_total > 0 && buffer[bytes_received_total-1] == '}') {
                  buffer[bytes_received_total] = '\0';
                  response_data.assign(buffer.data(), bytes_received_total);
                  break;
             }
        } while (bytes_received_now > 0);

        auto end_time = std::chrono::high_resolution_clock::now();
        if (!response_data.empty()) { latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count(); }
        close(sock); sock = -1; std::cout << "[DEBUG] IPC Socket closed." << std::endl;

        // Обработка ответа (логика парсинга JSON остается прежней)
        if (!response_data.empty()) {
             std::cout << "[INFO] IPC Latency: " << latency_ms << " ms" << std::endl;
             std::cout << "[DEBUG] Response Body: " << response_data << std::endl;
             try {
                 json response_json = json::parse(response_data);
                 if (response_json.contains("error") && !response_json["error"].is_null()) { std::cerr << "[ERROR] RPC Error: " << response_json["error"].dump(2) << std::endl; }
                 else if (response_json.contains("result") && !response_json["result"].is_null()) {
                     std::string latest_block_hex = response_json["result"];
                     try {
                          unsigned long long latest_block_dec = std::stoull(latest_block_hex, nullptr, 16);
                          std::cout << "[INFO] Fantom Latest Block (IPC): " << latest_block_dec << " (Hex: " << latest_block_hex << ")" << std::endl;
                     } catch (const std::exception& e) { std::cerr << "[ERROR] Failed to convert hex block number '" << latest_block_hex << "': " << e.what() << std::endl; }
                 } else { std::cerr << "[ERROR] Invalid JSON-RPC response: 'result' missing or null." << std::endl; }
             } catch (json::parse_error& e) { std::cerr << "[ERROR] Failed to parse JSON response: " << e.what() << std::endl; }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
             std::cerr << "[ERROR] No data received via IPC." << std::endl;
        }

    } catch (const std::exception& e) { std::cerr << "[ERROR] Exception caught during IPC operation: " << e.what() << std::endl; if (sock >= 0) { close(sock); } }
    #else // Если _WIN32 определен
    std::cerr << "[ERROR] IPC connection is not supported on Windows in this example." << std::endl;
    #endif
    std::cout << "[INFO] IPC query finished." << std::endl;
}
