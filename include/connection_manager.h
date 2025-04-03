// include/connection_manager.h

#ifndef NEOZORK3_CONNECTION_MANAGER_H
#define NEOZORK3_CONNECTION_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <nlohmann/json_fwd.hpp> // Forward declaration for json

namespace neozork::connection_manager {

// Define a type for HTTP headers (using std::map - assumes unique keys or last value wins)
using http_headers = std::map<std::string, std::string>;

// Structure to hold detailed results from an HTTP connection attempt
struct connection_result {
    std::optional<std::string> body;                 // Response body
    std::optional<int> status_code;              // HTTP status code (e.g., 200, 404, 429)
    std::optional<double> latency_ms;            // Request-response latency in milliseconds
    std::optional<long long> request_size_bytes;     // Size of the request body sent
    std::optional<long long> response_size_bytes;    // Size of the response body received
    std::optional<http_headers> response_headers;    // Headers received in the response
    std::optional<std::string> error_message;        // Error details if the request failed at the library level
};


/**
 * @brief Performs an HTTPS GET request.
 * @param host The host (e.g., "api.github.com").
 * @param path The path (e.g., "/repos/owner/repo/contents/path/to/file").
 * @param headers Optional HTTP request headers.
 * @return connection_result Structure containing details about the operation outcome.
 */
connection_result https_get(
                            const std::string& host,
                            const std::string& path,
                            const http_headers& headers = {}
                            );

/**
 * @brief Performs an HTTPS POST request.
 * @param host The host (e.g., "rpc.ftm.tools").
 * @param path The path (e.g., "/").
 * @param request_body The body of the POST request (e.g., JSON RPC payload).
 * @param headers Optional HTTP request headers (Content-Type often needed, e.g., "application/json").
 * @return connection_result Structure containing details about the operation outcome.
 */
connection_result https_post(
                             const std::string& host,
                             const std::string& path,
                             const std::string& request_body,
                             const http_headers& headers = {}
                             );

/**
 * @brief Sends a standard JSON RPC 2.0 request over HTTPS POST.
 * @param url The full HTTPS URL of the JSON RPC endpoint.
 * @param method The JSON RPC method name (e.g., "eth_chainId").
 * @param params The JSON RPC parameters (as nlohmann::json object, can be array or object).
 * @return connection_result Structure containing details about the HTTP operation outcome.
 * @throws std::invalid_argument if the URL is not a valid HTTPS URL.
 */
connection_result send_json_rpc_request(
                                        const std::string& url,
                                        const std::string& method,
                                        const nlohmann::json& params // Use const reference to json object
);


// TODO: Add ws_connect, ipc_connect etc. later

} // namespace neozork::connection_manager

#endif // NEOZORK3_CONNECTION_MANAGER_H
