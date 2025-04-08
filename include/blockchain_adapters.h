// include/blockchain_adapters.h

#ifndef NEOZORK3_BLOCKCHAIN_ADAPTERS_H
#define NEOZORK3_BLOCKCHAIN_ADAPTERS_H

#include <string>
#include <vector>
#include <optional>
#include "config_manager.h" // Needs config structs

namespace neozork::blockchain_adapters {

/**
 * @brief Measures the average block time for a given blockchain using an active endpoint.
 * Finds an active HTTPS endpoint, fetches block numbers periodically, calculates average time,
 * and updates the block_speed_ms field in the config.
 * @param config The main configuration object (passed by reference, may be modified).
 * @param blockchain_name_or_id The name or network ID of the blockchain to measure.
 * @return std::optional<double> The calculated average block speed in milliseconds, or nullopt if measurement failed.
 * @throws std::runtime_error if blockchain not found or no suitable active endpoint is available.
 */
std::optional<double> measure_block_speed(
                                          neozork::config_manager::struct_config& config,
                                          const std::string& blockchain_name_or_id
                                          );

/**
 * @brief Gets the latest block number from a specific RPC endpoint URL.
 * Primarily a helper function for measure_block_speed and potentially others.
 * @param endpoint_url The full HTTPS URL of the RPC endpoint.
 * @return std::optional<long long> The latest block number, or nullopt on error.
 */
std::optional<long long> get_latest_block_number(
                                                 const std::string& endpoint_url
                                                 );

/**
 * @brief Discovers known DEXes for a specific blockchain and adds them to the config.
 * Currently uses a hardcoded list of known DEX factory addresses per chain ID.
 * @param config The main configuration object (mutable).
 * @param blockchain_name_or_id The name or network ID of the blockchain.
 * @return True if the process completed (even if no *new* DEXes were added), false on critical error (e.g., blockchain not found).
 * @throws std::runtime_error on configuration errors.
 */
bool discover_dexes_for_blockchain(
    neozork::config_manager::struct_config& config,
    const std::string& blockchain_name_or_id
);

/**
 * @brief Discovers liquidity pools for a specific DEX on a given blockchain.
 * Interacts with the DEX Factory contract via RPC.
 * @param config The main configuration object (mutable).
 * @param blockchain_id_str The network ID of the blockchain as a string.
 * @param dex_id The ID of the DEX (from config) for which to discover pools.
 * @return True if the configuration was potentially modified (new pools added), false otherwise or on critical error.
 * @throws std::runtime_error on configuration errors (blockchain/DEX not found) or RPC errors.
 */
bool discover_pools_for_dex(
    neozork::config_manager::struct_config& config,
    const std::string& blockchain_id_str,
    const std::string& dex_id,
    int delay_ms // <<< ADDED parameter (0 = no delay)
);

// --- Planned functions (Stubs for now) ---

// std::vector<neozork::config_manager::struct_dex_info> discover_dexes(
//     const neozork::config_manager::struct_blockchain_info& bc_info,
//     const std::string& active_endpoint_url);

// std::vector<neozork::config_manager::struct_pool_info> discover_pools(
//     const neozork::config_manager::struct_dex_info& dex_info,
//      const std::string& active_endpoint_url);

// std::optional<double> get_price_from_pool(...); // Complex

} // namespace neozork::blockchain_adapters

#endif // NEOZORK3_BLOCKCHAIN_ADAPTERS_H
