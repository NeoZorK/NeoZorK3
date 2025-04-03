#ifndef NEOZORK3_CONFIG_MANAGER_H
#define NEOZORK3_CONFIG_MANAGER_H

#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional> // for std::reference_wrapper
#include <nlohmann/json.hpp> // use <> for external library

namespace neozork::config_manager {

// --- Data Structures for NeoZorK-config ---

// Rate limits
struct struct_rate_limits {
    std::optional<int> per_second;
    std::optional<int> per_minute;
    std::optional<int> per_hour;
    std::optional<int> per_day;
    std::optional<int> per_month;
};

// Endpoint status
struct struct_endpoint_connection_status {
    bool is_active = false;
    std::optional<double> latency_ms;
    std::optional<std::string> last_check;
    std::optional<long long> traffic_in_bytes;
    std::optional<long long> traffic_out_bytes;
    std::optional<long long> rpc_response_size_bytes;
};

// Endpoint
struct struct_endpoint {
    // URL
    // std::optional<std::string> provider_name;
    
    // Changed to map for multiple types
    std::map<std::string, std::string> connection_urls; // key: "https", "wss", "ipc"; value: full URL
    
    // Status
    std::map<std::string, struct_endpoint_connection_status> status;
    
    // Placeholder for rate limits
    std::optional<std::string> required_api_key_placeholder;
    
    std::optional<struct_rate_limits> rate_limits;
    std::optional<std::string> access_token;
    std::optional<int> parallel_query_allowance;
    std::optional<long long> last_block_number;
};

// Tokens
struct struct_token_info {
    std::string symbol;
    std::string address;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(struct_token_info, symbol, address);
};

// Pools
struct struct_pool_info {
    std::string dex_id;
    std::string pool_id;
    struct_token_info token0;
    struct_token_info token1;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(struct_pool_info, dex_id, pool_id, token0, token1);
};

// DEX
struct struct_dex_info {
    std::string id;
    std::string name;
    std::optional<std::string> router_address;
    std::optional<std::string> factory_address;
};

// Blockchains
struct struct_blockchain_info {
    std::string name;
    int network_id = 0;
    std::optional<double> block_speed_ms;
    std::vector<struct_dex_info> dexes;
    std::vector<struct_pool_info> pools;
    std::vector<struct_endpoint> endpoints;
};

// Main config
struct struct_config {
    std::vector<struct_blockchain_info> blockchains;
};

// --- Functions for to_json/from_json

void to_json(nlohmann::json& j, const struct_config& p);
void from_json(const nlohmann::json& j, struct_config& p);

void to_json(nlohmann::json& j, const struct_rate_limits& p);
void from_json(const nlohmann::json& j, struct_rate_limits& p);

void to_json(nlohmann::json& j, const struct_endpoint_connection_status& p);
void from_json(const nlohmann::json& j, struct_endpoint_connection_status& p);

void to_json(nlohmann::json& j, const struct_endpoint& p);
void from_json(const nlohmann::json& j, struct_endpoint& p);

void to_json(nlohmann::json& j, const struct_dex_info& p);
void from_json(const nlohmann::json& j, struct_dex_info& p);

void to_json(nlohmann::json& j, const struct_blockchain_info& p);
void from_json(const nlohmann::json& j, struct_blockchain_info& p);

// --- Main functions ---

std::filesystem::path get_config_path();
struct_config load_config();
void save_config(const struct_config& config);
bool ensure_config_exists();
void initialize_config();

// --- Helpers ---

// --- Find Endpoint Functions ---

/**
 * @brief Finds an endpoint within a blockchain config by matching *any* of its configured URLs.
 * Searches through the endpoint's connection_urls map.
 * @param bc_info_ref Reference to the blockchain structure to search within.
 * @param url_str The URL string to search for.
 * @return Optional reference wrapper to the found endpoint struct (mutable).
 */
std::optional<std::reference_wrapper<struct_endpoint>> find_endpoint_by_any_url(
                                                                                struct_blockchain_info& bc_info_ref,
                                                                                const std::string& url_str
                                                                                );

/**
 * @brief Finds an endpoint within a blockchain config by matching *any* of its configured URLs (const version).
 * @param bc_info_ref Const reference to the blockchain structure to search within.
 * @param url_str The URL string to search for.
 * @return Optional const reference wrapper to the found endpoint struct.
 */
std::optional<std::reference_wrapper<const struct_endpoint>> find_endpoint_by_any_url(
                                                                                      const struct_blockchain_info& bc_info_ref,
                                                                                      const std::string& url_str
                                                                                      );


// Find blockchain by name or network_id
std::optional<std::reference_wrapper<struct_endpoint>> find_endpoint_by_urls(
                                                                             struct_blockchain_info& bc_info_ref,
                                                                             const std::map<std::string, std::string>& urls_to_find
                                                                             );

// --- Search ---
std::optional<std::reference_wrapper<struct_blockchain_info>> find_blockchain(
                                                                              struct_config& config_ref, const std::string& name_or_id_str
                                                                              );
std::optional<std::reference_wrapper<const struct_blockchain_info>> find_blockchain(
                                                                                    const struct_config& config_ref, const std::string& name_or_id_str
                                                                                    );

std::optional<std::reference_wrapper<struct_endpoint>> find_endpoint(
                                                                     struct_blockchain_info& bc_info_ref, const std::string& url_str
                                                                     );
std::optional<std::reference_wrapper<const struct_endpoint>> find_endpoint(
                                                                           const struct_blockchain_info& bc_info_ref, const std::string& url_str
                                                                           );

std::optional<std::reference_wrapper<struct_dex_info>> find_dex(
                                                                struct_blockchain_info& bc_info_ref, const std::string& dex_id_str
                                                                );
std::optional<std::reference_wrapper<const struct_dex_info>> find_dex(
                                                                      const struct_blockchain_info& bc_info_ref, const std::string& dex_id_str
                                                                      );

std::optional<std::reference_wrapper<struct_pool_info>> find_pool(
                                                                  struct_blockchain_info& bc_info_ref, const std::string& pool_id_str
                                                                  );
std::optional<std::reference_wrapper<const struct_pool_info>> find_pool(
                                                                        const struct_blockchain_info& bc_info_ref, const std::string& pool_id_str
                                                                        );

// --- Add (with dublicate check) ---
// Returns true, if new element
bool add_blockchain(struct_config& config_ref, const struct_blockchain_info& new_blockchain);
bool add_endpoint(struct_blockchain_info& bc_info_ref, const struct_endpoint& new_endpoint);
bool add_dex(struct_blockchain_info& bc_info_ref, const struct_dex_info& new_dex);
bool add_pool(struct_blockchain_info& bc_info_ref, const struct_pool_info& new_pool);

// --- Update data ---
bool update_endpoint_status(
                            struct_endpoint& endpoint_ref,
                            const std::string& connection_type_str, // "https", "wss", etc.
                            const struct_endpoint_connection_status& new_status
                            );

bool update_endpoint_block_number(
                                  struct_endpoint& endpoint_ref,
                                  long long block_number
                                  );

bool update_blockchain_block_speed(
                                   struct_blockchain_info& bc_info_ref,
                                   double speed_ms
                                   );

// --- Getting data ---
// Getting list of active endpoints for blockchain
std::vector<std::reference_wrapper<const struct_endpoint>> get_active_endpoints(
                                                                                const struct_blockchain_info& bc_info_ref,
                                                                                const std::string& preferred_type = "https" // "wss", "https"
);


} // namespace neozork::config_manager

#endif // NEOZORK3_CONFIG_MANAGER_H
