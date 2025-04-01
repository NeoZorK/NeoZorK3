#ifndef NEOZORK3_CONFIG_MANAGER_H
#define NEOZORK3_CONFIG_MANAGER_H

#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional> // Для std::reference_wrapper
#include <nlohmann/json.hpp> // Используем <> для внешних библиотек

// Используем snake_case для пространства имен
namespace neozork::config_manager {

    // --- Структуры данных для NeoZorK-config ---

    struct struct_rate_limits {
        std::optional<int> per_second;
        std::optional<int> per_minute;
        std::optional<int> per_hour;
        std::optional<int> per_day;
        std::optional<int> per_month;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(struct_rate_limits, per_second, per_minute, per_hour, per_day, per_month);
    };

    struct struct_endpoint_connection_status {
        bool is_active = false;
        std::optional<double> latency_ms;
        std::optional<std::string> last_check;
        std::optional<long long> traffic_in_bytes;
        std::optional<long long> traffic_out_bytes;
        std::optional<long long> rpc_response_size_bytes;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(struct_endpoint_connection_status, is_active, latency_ms, last_check, traffic_in_bytes, traffic_out_bytes, rpc_response_size_bytes);
    };

    struct struct_endpoint {
        std::string url;
        // Добавляем все запрошенные типы по умолчанию
        std::vector<std::string> supported_types = {"http", "https", "ws", "wss", "ipc"};
        std::map<std::string, struct_endpoint_connection_status> status; // Ключ - тип соединения

        std::optional<struct_rate_limits> rate_limits;
        std::optional<std::string> access_token;
        std::optional<int> parallel_query_allowance;
        std::optional<long long> last_block_number;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(struct_endpoint, url, supported_types, status, rate_limits, access_token, parallel_query_allowance, last_block_number);
    };

    struct struct_token_info {
        std::string symbol;
        std::string address;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(struct_token_info, symbol, address);
    };

    struct struct_pool_info {
        std::string dex_id;
        std::string pool_id;
        struct_token_info token0;
        struct_token_info token1;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(struct_pool_info, dex_id, pool_id, token0, token1);
    };

    struct struct_dex_info {
        std::string id;
        std::string name;
        std::optional<std::string> router_address;
        std::optional<std::string> factory_address;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(struct_dex_info, id, name, router_address, factory_address);
    };

    struct struct_blockchain_info {
        std::string name;
        int network_id = 0;
        std::optional<double> block_speed_ms;
        std::vector<struct_dex_info> dexes;
        std::vector<struct_pool_info> pools;
        std::vector<struct_endpoint> endpoints;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(struct_blockchain_info, name, network_id, block_speed_ms, dexes, pools, endpoints);
    };

    // Корневая структура конфига
    struct struct_config {
        std::vector<struct_blockchain_info> blockchains;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(struct_config, blockchains);
    };


    // --- Объявления Основных Функций Модуля ---

    std::filesystem::path get_config_path();
    struct_config load_config();
    void save_config(const struct_config& config);
    bool ensure_config_exists();
    void initialize_config();

    // --- Объявления Вспомогательных Функций (Хелперов) ---

    // --- Поиск ---
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

    // --- Добавление (с проверкой дубликатов) ---
    // Возвращают true, если элемент был реально добавлен (не дубликат)
    bool add_blockchain(struct_config& config_ref, const struct_blockchain_info& new_blockchain);
    bool add_endpoint(struct_blockchain_info& bc_info_ref, const struct_endpoint& new_endpoint);
    bool add_dex(struct_blockchain_info& bc_info_ref, const struct_dex_info& new_dex);
    bool add_pool(struct_blockchain_info& bc_info_ref, const struct_pool_info& new_pool);

    // --- Обновление данных ---
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

    // --- Получение данных ---
    // Получить список активных эндпоинтов для блокчейна (предпочитая заданный тип)
    std::vector<std::reference_wrapper<const struct_endpoint>> get_active_endpoints(
        const struct_blockchain_info& bc_info_ref,
        const std::string& preferred_type = "https" // например, "wss" или "https"
    );


} // namespace neozork::config_manager

#endif // NEOZORK3_CONFIG_MANAGER_H
