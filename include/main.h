// include/main.h

#ifndef MAIN_H
#define MAIN_H

#include <string>
#include <vector> // Добавили для vector в аргументах

// --- Типы Соединений ---
enum class ConnectionType {
    HTTP,
    HTTPS,
    WS,
    WSS,
    IPC,
    UNKNOWN
};

// --- Объявления Функций ---

/**
 * @brief Parses command line arguments from a vector.
 * @param args Vector of command line arguments (argv).
 * @param type Output: Detected connection type.
 * @param endpoint Output: Detected endpoint URL or path.
 * @return True if valid arguments are found, false otherwise.
 */
bool parse_arguments(const std::vector<std::string>& args, ConnectionType& type, std::string& endpoint);

/**
 * @brief Main function to perform the blockchain query based on connection type.
 * @param type The type of connection to use.
 * @param endpoint The URL or path for the connection.
 */
void query_blockchain(ConnectionType type, const std::string& endpoint);

/**
 * @brief Performs the query using HTTP or HTTPS.
 * @param url The full URL (http://... or https://...).
 * @param use_ssl True for HTTPS, false for HTTP.
 */
void query_via_http(const std::string& url, bool use_ssl);

/**
 * @brief Performs the query using WebSocket or WebSocket Secure.
 * @param url The full URL (ws://... or wss://...).
 * @param use_ssl True for WSS, false for WS.
 */
void query_via_websocket(const std::string& url, bool use_ssl);

/**
 * @brief Performs the query using IPC (Unix Domain Socket).
 * @param path Path to the IPC socket file.
 */
void query_via_ipc(const std::string& path);


#endif // MAIN_H
