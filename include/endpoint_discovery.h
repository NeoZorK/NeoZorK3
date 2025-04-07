#ifndef NEOZORK3_ENDPOINT_DISCOVERY_H
#define NEOZORK3_ENDPOINT_DISCOVERY_H

#include <string>
#include <vector>
#include "config_manager.h" // Need access to struct_config

namespace neozork::endpoint_discovery {

/**
 * @brief Discovers chains from sources, filters by name, and syncs them with local config.
 * Adds new chains or new endpoints to existing chains found in the source.
 * @param config The main configuration object (mutable).
 * @param name_filter Substring to filter chain names from the source (case-insensitive). Use "*" or empty for all.
 * @param sources Vector of source keywords (e.g., "chain") or URLs.
 * @return True if the configuration was potentially modified, false otherwise or on critical error.
 */
bool discover_and_sync_chains(
                              neozork::config_manager::struct_config& config,
                              const std::string& name_filter,
                              const std::vector<std::string>& sources
                              );

} // namespace neozork::endpoint_discovery

#endif // NEOZORK3_ENDPOINT_DISCOVERY_H
