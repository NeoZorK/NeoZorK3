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

// +++ START ADDED ABI HELPERS DECLARATIONS +++

/**
 * @brief Encodes data for an eth_call for a function with no arguments.
 * @param function_sig_hash The 4-byte function signature hash (e.g., "0x0dfe1681").
 * @return The hex string for the 'data' field (e.g., "0x0dfe1681"), or empty string if hash is invalid.
 */
std::string encode_eth_call_data(const std::string& function_sig_hash);

/**
 * @brief Encodes data for an eth_call for a function with one uint256 argument.
 * @param function_sig_hash The 4-byte function signature hash (e.g., "0x1e3dd18b").
 * @param argument The uint256 argument (represented as long long, assumes non-negative).
 * @return The hex string for the 'data' field (e.g., "0x1e3dd18b00...00<hex_arg>"), or empty string on error.
 */
std::string encode_eth_call_data(const std::string& function_sig_hash, unsigned long long argument); // Use unsigned long long

/**
 * @brief Decodes an address from a standard 32-byte eth_call hex result.
 * @param result_hex The result field from JSON RPC response (e.g., "0x00...00<address_40_chars>").
 * @return The decoded address ("0x" + 40 hex chars), or empty string if input is invalid.
 */
std::string decode_address_from_result(const std::string& result_hex);

/**
 * @brief Decodes a uint256 value from a standard 32-byte eth_call hex result.
 * Uses long long, may overflow for very large uint256 values.
 * @param result_hex The result field from JSON RPC response (e.g., "0x00...0<hex_number>").
 * @return The decoded value as long long.
 * @throws std::invalid_argument if hex string is invalid or conversion fails.
 * @throws std::out_of_range if the value exceeds long long limits.
 */
long long decode_uint256_from_result(const std::string& result_hex);

// +++ END ADDED ABI HELPERS DECLARATIONS +++ф

/**
 * @brief Sends a JSON RPC "eth_call" request to an endpoint.
 * @param endpoint_url The full HTTPS URL of the JSON RPC endpoint.
 * @param contract_address The address of the contract to call ('to' field).
 * @param encoded_data The ABI-encoded data for the function call ('data' field).
 * @return connection_result Structure containing details about the HTTP operation outcome.
 * @throws std::invalid_argument if the URL is invalid or other input errors occur.
 * @throws std::runtime_error on JSON errors or other execution issues.
 */
connection_result send_eth_call(
                                const std::string& endpoint_url,
                                const std::string& contract_address,
                                const std::string& encoded_data
                                );

// TODO: Add ws_connect, ipc_connect etc. later

} // namespace neozork::connection_manager

#endif // NEOZORK3_CONNECTION_MANAGER_H
