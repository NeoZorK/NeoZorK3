#ifndef NEOZORK3_ENDPOINT_DISCOVERY_H
#define NEOZORK3_ENDPOINT_DISCOVERY_H

#include <string>
#include <vector>
#include "config_manager.h" // Need access to struct_config

namespace neozork::endpoint_discovery {

/**
 * @brief Discovers RPC endpoints from specified sources and adds them to the config.
 * @param blockchain_name The name or ID of the blockchain to add endpoints for.
 * @param sources A list of sources (e.g., URLs to GitHub raw files, "defillama", "chainlist").
 * @param config A reference to the loaded configuration object to modify.
 * @return True if discovery was attempted (does not guarantee endpoints were found/added), false on critical error.
 * @throw std::runtime_error on configuration errors (e.g., blockchain not found).
 */
bool discover_endpoints(
                        const std::string& blockchain_name,
                        const std::vector<std::string>& sources,
                        neozork::config_manager::struct_config& config // Pass config by ref to modify
);

} // namespace neozork::endpoint_discovery

#endif // NEOZORK3_ENDPOINT_DISCOVERY_H
