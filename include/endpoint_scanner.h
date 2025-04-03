// include/endpoint_scanner.h

#ifndef NEOZORK3_ENDPOINT_SCANNER_H
#define NEOZORK3_ENDPOINT_SCANNER_H

#include <string>
#include <vector>
#include <optional>
#include "config_manager.h" // Needs access to config structures

namespace neozork::endpoint_scanner {

/**
 * @brief Scans all configured endpoints for a given blockchain.
 * Updates the status (isActive, latency, etc.) for each configured connection type
 * within each endpoint in the config object, unless a specific connection type is requested.
 * @param config The main configuration object (passed by reference, will be modified).
 * @param blockchain_name_or_id The name or network ID of the blockchain to scan.
 * @param requested_connection_type If provided (e.g., "https", "wss"), only this connection type is scanned. Otherwise, all types listed for each endpoint are scanned.
 * @throws std::runtime_error if the blockchain is not found in the config.
 */
void run_scan_endpoints(
                        neozork::config_manager::struct_config& config,
                        const std::string& blockchain_name_or_id,
                        const std::optional<std::string>& requested_connection_type
                        );

/**
 * @brief Scans a single specific endpoint (identified by one of its URLs) for a given blockchain.
 * Updates the status for the relevant connection type(s) within that specific endpoint
 * entry in the config object.
 * @param config The main configuration object (passed by reference, will be modified).
 * @param blockchain_name_or_id The name or network ID of the blockchain where the endpoint resides.
 * @param endpoint_url_to_find A URL associated with the endpoint to scan (used to locate the endpoint entry).
 * @param requested_connection_type If provided, only this connection type is scanned. Otherwise, all types listed for the endpoint are scanned.
 * @throws std::runtime_error if the blockchain or the specific endpoint URL is not found.
 */
void run_scan_single_endpoint(
                              neozork::config_manager::struct_config& config,
                              const std::string& blockchain_name_or_id,
                              const std::string& endpoint_url_to_find,
                              const std::optional<std::string>& requested_connection_type
                              );

} // namespace neozork::endpoint_scanner

#endif // NEOZORK3_ENDPOINT_SCANNER_H
