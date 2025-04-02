#ifndef NEOZORK3_CONNECTION_MANAGER_H
#define NEOZORK3_CONNECTION_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace neozork::connection_manager {

// Define a type for HTTP headers
using http_headers = std::map<std::string, std::string>;

/**
 * @brief Performs an HTTPS GET request.
 * @param host The host (e.g., "api.github.com").
 * @param path The path (e.g., "/repos/owner/repo/contents/path/to/file").
 * @param headers Optional HTTP headers.
 * @return std::optional<std::string> The response body on success (2xx status code), otherwise std::nullopt.
 */
std::optional<std::string> https_get(
                                     const std::string& host,
                                     const std::string& path,
                                     const http_headers& headers = {}
                                     );

// TODO: Add https_post, ws_connect etc. later

} // namespace neozork::connection_manager

#endif // NEOZORK3_CONNECTION_MANAGER_H
