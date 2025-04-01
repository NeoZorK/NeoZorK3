#include "config_manager.h" // Включаем наш заголовочный файл .h
#include <fstream>          // Для std::ifstream, std::ofstream
#include <iostream>         // Для вывода ошибок std::cerr
#include <stdexcept>        // Для std::runtime_error
#include <string>
#include <vector>
#include <filesystem>       // Для работы с путями и файлами
#include <optional>
#include <functional>       // Для std::reference_wrapper
#include <algorithm>        // Для std::find_if, std::transform
#include <cctype>           // Для ::tolower

// Подключаем nlohmann/json. Убедитесь, что CMakeLists.txt настроен!
#include <nlohmann/json.hpp>

// Используем псевдоним для удобства
using json = nlohmann::json;

// Начинаем реализацию в нашем пространстве имен
namespace neozork::config_manager {

    /**
     * @brief Вспомогательная функция для получения директории исполняемого файла.
     * @warning Текущая реализация возвращает текущую рабочую директорию.
     * Заменить на более надежный платформо-зависимый способ позже.
     * @return Путь к директории.
     */
    std::filesystem::path get_executable_dir() {
        return std::filesystem::current_path();
        // TODO: Реализовать надежное получение пути к исполняемому файлу
        // для Linux, macOS, Windows.
    }

    /**
     * @brief Получает полный путь к файлу конфигурации NeoZorK-config.
     * @return std::filesystem::path Путь к файлу конфигурации.
     */
    std::filesystem::path get_config_path() {
        const std::string config_filename = "NeoZorK-config";
        return get_executable_dir() / config_filename;
    }

    /**
     * @brief (Приватная) Создает файл конфигурации по умолчанию с примерами.
     * @param path Путь, по которому нужно создать файл.
     * @throw std::runtime_error если не удается открыть или записать файл.
     */
    void create_default_config(const std::filesystem::path& path) {
        std::cout << "Creating default configuration file at: " << path.string() << std::endl;
        struct_config default_config;

        // --- Пример 1: Fantom ---
        struct_blockchain_info fantom;
        fantom.name = "Fantom";
        fantom.network_id = 250;

        struct_endpoint ftm_rpc1;
        ftm_rpc1.url = "https://rpc.ftm.tools/";
        ftm_rpc1.supported_types = {"https"};

        struct_endpoint ftm_rpc2;
        ftm_rpc2.url = "https://fantom-mainnet.public.blastapi.io/";
        ftm_rpc2.supported_types = {"https", "wss"};
        // Добавляем access_token для BlastAPI (замените на ваш реальный токен!)
        // Используем .emplace() или .value() для optional<string>
        ftm_rpc2.access_token.emplace("YOUR_BLASTAPI_TOKEN_HERE"); // <-- ЗАМЕНИТЕ НА ВАШ ТОКЕН

        fantom.endpoints.push_back(ftm_rpc1);
        fantom.endpoints.push_back(ftm_rpc2);

        default_config.blockchains.push_back(fantom);

        // --- Пример 2: Avalanche ---
        struct_blockchain_info avax;
        avax.name = "Avalanche";
        avax.network_id = 43114; // C-Chain

        struct_endpoint avax_rpc1;
        avax_rpc1.url = "https://api.avax.network/ext/bc/C/rpc";
        avax_rpc1.supported_types = {"https"};

        avax.endpoints.push_back(avax_rpc1);
        default_config.blockchains.push_back(avax);

        // --- Сохранение ---
        try {
            json j = default_config; // Сериализация структуры в JSON
            std::ofstream ofs(path);
            if (!ofs.is_open()) {
                throw std::runtime_error("Cannot open config file for writing: " + path.string());
            }
            ofs << j.dump(4); // Вывод с отступами для читаемости
            ofs.close();
            if (!ofs) { // Проверяем ошибки записи/закрытия
                 throw std::runtime_error("Error writing to config file: " + path.string());
            }
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to create default config: " + std::string(e.what()));
        }
    }

    /**
     * @brief Проверяет существование файла конфигурации. Если не существует, создает его по умолчанию.
     * @return true, если файл существует или был успешно создан.
     * @return false, если произошла ошибка при создании файла по умолчанию.
     */
    bool ensure_config_exists() {
        auto path = get_config_path();
        if (!std::filesystem::exists(path)) {
            try {
                create_default_config(path);
                return true;
            } catch (const std::exception& e) {
                // Используем cerr для вывода ошибок
                std::cerr << "ERROR: Failed to create default config: " << e.what() << std::endl;
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Принудительно создает (перезаписывает) файл конфигурации по умолчанию.
     * @throw std::runtime_error если произошла ошибка при создании.
     */
    void initialize_config() {
        auto path = get_config_path();
        try {
            std::cout << "Initializing configuration file (overwriting if exists)..." << std::endl;
            // Опционально: удалить старый файл перед созданием нового
            // std::filesystem::remove(path);
            create_default_config(path);
        } catch (const std::exception& e) {
             throw std::runtime_error("Failed to initialize config: " + std::string(e.what()));
        }
    }

    /**
     * @brief Загружает конфигурацию из файла NeoZorK-config.
     * @return Заполненная структура struct_config.
     * @throw std::runtime_error если файл не найден, не может быть открыт, содержит некорректный JSON,
     * или данные не соответствуют структуре struct_config.
     */
    struct_config load_config() {
        auto path = get_config_path();
        std::ifstream ifs(path);

        if (!ifs.is_open()) {
            throw std::runtime_error("Cannot open config file for reading: " + path.string());
        }

        try {
            json j;
            ifs >> j; // Парсинг JSON из потока
            // Десериализация JSON в структуру struct_config
            return j.get<struct_config>();
        } catch (const json::parse_error& e) {
            throw std::runtime_error("Failed to parse config file JSON (" + path.string() + "): " + std::string(e.what()));
        } catch (const json::type_error& e) {
             throw std::runtime_error("Failed to deserialize config data (" + path.string() + "): " + std::string(e.what()));
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to load config (" + path.string() + "): " + std::string(e.what()));
        }
    }

    /**
     * @brief Сохраняет структуру конфигурации в файл NeoZorK-config.
     * @param config Структура struct_config для сохранения.
     * @throw std::runtime_error если не удается открыть или записать файл.
     */
    void save_config(const struct_config& config) {
        auto path = get_config_path();
        try {
            json j = config; // Сериализация структуры в JSON
            std::ofstream ofs(path);
             if (!ofs.is_open()) {
                throw std::runtime_error("Cannot open config file for writing: " + path.string());
            }
            ofs << j.dump(4); // Запись с отступами
            ofs.close();
             if (!ofs) {
                 throw std::runtime_error("Error writing to config file: " + path.string());
            }
        } catch (const std::exception& e) {
             throw std::runtime_error("Failed to save config (" + path.string() + "): " + std::string(e.what()));
        }
    }

    // --- Реализация Вспомогательных Функций (Частично / Заглушки) ---

    /**
     * @brief Находит блокчейн в конфигурации по имени (без учета регистра) или network_id.
     * @param config_ref Ссылка на объект конфигурации (может быть модифицирован).
     * @param name_or_id_str Имя или ID блокчейна для поиска (как строка).
     * @return std::optional с оберткой над ссылкой на найденный struct_blockchain_info, если найден.
     * std::nullopt, если не найден.
     */
    std::optional<std::reference_wrapper<struct_blockchain_info>> find_blockchain(
        struct_config& config_ref, const std::string& name_or_id_str)
    {
        auto it = std::find_if(config_ref.blockchains.begin(), config_ref.blockchains.end(),
            [&](const struct_blockchain_info& bc) {
                // Сравнение имени без учета регистра
                std::string lower_name = bc.name;
                std::string lower_search = name_or_id_str;
                // Преобразуем обе строки в нижний регистр
                std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                               [](unsigned char c){ return std::tolower(c); });
                std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(),
                               [](unsigned char c){ return std::tolower(c); });

                if (lower_name == lower_search) return true;

                // Проверка по ID
                try {
                    // Используем stoll для большей безопасности с ID
                    long long id = std::stoll(name_or_id_str);
                    if (bc.network_id == id) return true;
                } catch(...) { /* Игнорируем исключения stoi/stoll */ }
                return false;
            });

        if (it != config_ref.blockchains.end()) {
            return std::ref(*it); // Возвращаем обертку над ссылкой
        }
        return std::nullopt;
    }

    /**
     * @brief Константная версия find_blockchain.
     */
    std::optional<std::reference_wrapper<const struct_blockchain_info>> find_blockchain(
        const struct_config& config_ref, const std::string& name_or_id_str)
    {
         auto it = std::find_if(config_ref.blockchains.begin(), config_ref.blockchains.end(),
            [&](const struct_blockchain_info& bc) {
                std::string lower_name = bc.name;
                std::string lower_search = name_or_id_str;
                std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                               [](unsigned char c){ return std::tolower(c); });
                std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(),
                               [](unsigned char c){ return std::tolower(c); });
                if (lower_name == lower_search) return true;
                try {
                    long long id = std::stoll(name_or_id_str);
                    if (bc.network_id == id) return true;
                } catch(...) {}
                return false;
            });

        if (it != config_ref.blockchains.end()) {
            return std::cref(*it); // Константная обертка
        }
        return std::nullopt;
    }

    /**
     * @brief Находит эндпоинт в указанном блокчейне по URL.
     * @param bc_info_ref Ссылка на объект блокчейна.
     * @param url_str URL для поиска.
     * @return std::optional с оберткой над ссылкой на найденный struct_endpoint, если найден.
     */
    std::optional<std::reference_wrapper<struct_endpoint>> find_endpoint(
        struct_blockchain_info& bc_info_ref, const std::string& url_str)
    {
        auto it = std::find_if(bc_info_ref.endpoints.begin(), bc_info_ref.endpoints.end(),
            [&](const struct_endpoint& ep) { return ep.url == url_str; });
        if (it != bc_info_ref.endpoints.end()) {
            return std::ref(*it);
        }
        return std::nullopt;
    }

    /**
     * @brief Константная версия find_endpoint.
     */
    std::optional<std::reference_wrapper<const struct_endpoint>> find_endpoint(
        const struct_blockchain_info& bc_info_ref, const std::string& url_str)
    {
        auto it = std::find_if(bc_info_ref.endpoints.begin(), bc_info_ref.endpoints.end(),
            [&](const struct_endpoint& ep) { return ep.url == url_str; });
        if (it != bc_info_ref.endpoints.end()) {
            return std::cref(*it);
        }
        return std::nullopt;
    }

     /**
      * @brief Находит DEX в указанном блокчейне по ID.
      */
     std::optional<std::reference_wrapper<struct_dex_info>> find_dex(
        struct_blockchain_info& bc_info_ref, const std::string& dex_id_str)
    {
         auto it = std::find_if(bc_info_ref.dexes.begin(), bc_info_ref.dexes.end(),
            [&](const struct_dex_info& dex) { return dex.id == dex_id_str; });
        if (it != bc_info_ref.dexes.end()) {
            return std::ref(*it);
        }
        return std::nullopt;
    }

    /**
     * @brief Константная версия find_dex.
     */
    std::optional<std::reference_wrapper<const struct_dex_info>> find_dex(
        const struct_blockchain_info& bc_info_ref, const std::string& dex_id_str)
    {
         auto it = std::find_if(bc_info_ref.dexes.begin(), bc_info_ref.dexes.end(),
            [&](const struct_dex_info& dex) { return dex.id == dex_id_str; });
        if (it != bc_info_ref.dexes.end()) {
            return std::cref(*it);
        }
        return std::nullopt;
    }

    /**
     * @brief Находит пул в указанном блокчейне по ID пула.
     */
    std::optional<std::reference_wrapper<struct_pool_info>> find_pool(
        struct_blockchain_info& bc_info_ref, const std::string& pool_id_str)
    {
         auto it = std::find_if(bc_info_ref.pools.begin(), bc_info_ref.pools.end(),
            [&](const struct_pool_info& pool) { return pool.pool_id == pool_id_str; });
        if (it != bc_info_ref.pools.end()) {
            return std::ref(*it);
        }
        return std::nullopt;
    }

    /**
     * @brief Константная версия find_pool.
     */
    std::optional<std::reference_wrapper<const struct_pool_info>> find_pool(
        const struct_blockchain_info& bc_info_ref, const std::string& pool_id_str)
    {
         auto it = std::find_if(bc_info_ref.pools.begin(), bc_info_ref.pools.end(),
            [&](const struct_pool_info& pool) { return pool.pool_id == pool_id_str; });
        if (it != bc_info_ref.pools.end()) {
            return std::cref(*it);
        }
        return std::nullopt;
    }

    /**
     * @brief Добавляет новый блокчейн в конфигурацию, если его еще нет.
     * @param config_ref Ссылка на конфигурацию.
     * @param new_blockchain Добавляемый блокчейн.
     * @return true, если блокчейн был добавлен, false если дубликат.
     */
    bool add_blockchain(struct_config& config_ref, const struct_blockchain_info& new_blockchain) {
         // Проверяем по ID и имени
         if (find_blockchain(config_ref, new_blockchain.name) || find_blockchain(config_ref, std::to_string(new_blockchain.network_id))) {
             std::cerr << "Warning: Blockchain '" << new_blockchain.name << "' or ID " << new_blockchain.network_id << " already exists. Skipping." << std::endl;
             return false;
         }
         config_ref.blockchains.push_back(new_blockchain);
         return true;
    }

    /**
     * @brief Добавляет новый эндпоинт для блокчейна, если его еще нет (по URL).
     * @param bc_info_ref Ссылка на блокчейн.
     * @param new_endpoint Добавляемый эндпоинт.
     * @return true, если эндпоинт был добавлен, false если дубликат.
     */
    bool add_endpoint(struct_blockchain_info& bc_info_ref, const struct_endpoint& new_endpoint) {
        if (find_endpoint(bc_info_ref, new_endpoint.url)) {
             // Можно вывести предупреждение, если нужно
             // std::cerr << "Warning: Endpoint '" << new_endpoint.url << "' already exists for blockchain '" << bc_info_ref.name << "'. Skipping." << std::endl;
            return false;
        }
        bc_info_ref.endpoints.push_back(new_endpoint);
        return true;
    }

    /**
     * @brief Добавляет новый DEX для блокчейна, если его еще нет (по ID).
     * @param bc_info_ref Ссылка на блокчейн.
     * @param new_dex Добавляемый DEX.
     * @return true, если DEX был добавлен, false если дубликат.
     */
    bool add_dex(struct_blockchain_info& bc_info_ref, const struct_dex_info& new_dex) {
         if (find_dex(bc_info_ref, new_dex.id)) {
             return false;
         }
        bc_info_ref.dexes.push_back(new_dex);
        return true;
    }

    /**
     * @brief Добавляет новый пул для блокчейна, если его еще нет (по ID пула).
     * @param bc_info_ref Ссылка на блокчейн.
     * @param new_pool Добавляемый пул.
     * @return true, если пул был добавлен, false если дубликат.
     */
    bool add_pool(struct_blockchain_info& bc_info_ref, const struct_pool_info& new_pool) {
         if (find_pool(bc_info_ref, new_pool.pool_id)) {
             return false;
         }
        bc_info_ref.pools.push_back(new_pool);
        return true;
    }

    /**
     * @brief Обновляет статус указанного типа соединения для эндпоинта.
     * @param endpoint_ref Ссылка на эндпоинт.
     * @param connection_type_str Тип соединения ("https", "wss", etc.).
     * @param new_status Новые данные статуса.
     * @return true (в текущей реализации всегда).
     */
    bool update_endpoint_status(
        struct_endpoint& endpoint_ref,
        const std::string& connection_type_str,
        const struct_endpoint_connection_status& new_status)
    {
        endpoint_ref.status[connection_type_str] = new_status;
        // Можно добавить логику проверки, изменился ли статус, если это важно
        return true;
    }

    /**
     * @brief Обновляет номер последнего блока для эндпоинта.
     * @param endpoint_ref Ссылка на эндпоинт.
     * @param block_number Новый номер блока.
     * @return true.
     */
     bool update_endpoint_block_number(
        struct_endpoint& endpoint_ref,
        long long block_number)
    {
        endpoint_ref.last_block_number = block_number;
        return true;
    }

    /**
     * @brief Обновляет измеренную скорость появления блоков для блокчейна.
     * @param bc_info_ref Ссылка на блокчейн.
     * @param speed_ms Скорость в миллисекундах.
     * @return true.
     */
    bool update_blockchain_block_speed(
        struct_blockchain_info& bc_info_ref,
        double speed_ms)
    {
        bc_info_ref.block_speed_ms = speed_ms;
        return true;
    }

    /**
     * @brief Возвращает список активных эндпоинтов для блокчейна.
     * Сначала ищет активные эндпоинты предпочтительного типа,
     * затем любые другие активные, если по предпочтительному типу нет.
     * @param bc_info_ref Константная ссылка на блокчейн.
     * @param preferred_type Строка с предпочтительным типом ("https", "wss", "ipc", etc.) или "any".
     * @return Вектор оберток над константными ссылками на активные эндпоинты.
     */
    std::vector<std::reference_wrapper<const struct_endpoint>> get_active_endpoints(
        const struct_blockchain_info& bc_info_ref,
        const std::string& preferred_type /*= "https"*/) // Значение по умолчанию в .h
    {
        std::vector<std::reference_wrapper<const struct_endpoint>> active_endpoints;
        active_endpoints.reserve(bc_info_ref.endpoints.size()); // Оптимизация

        for(const auto& endpoint : bc_info_ref.endpoints) {
            bool found_active_for_this_endpoint = false;
            // 1. Ищем статус для предпочтительного типа (если это не "any")
            if (preferred_type != "any") {
                auto it = endpoint.status.find(preferred_type);
                if (it != endpoint.status.end() && it->second.is_active) {
                    active_endpoints.push_back(std::cref(endpoint));
                    found_active_for_this_endpoint = true;
                }
            }

            // 2. Если не нашли по предпочтительному ИЛИ ищем "any", ищем любой другой активный
            if (!found_active_for_this_endpoint) {
                 for(const auto& [type, status_data] : endpoint.status) {
                     // Пропускаем предпочтительный тип, если мы уже его проверили (и он неактивен)
                     if (preferred_type != "any" && type == preferred_type) {
                         continue;
                     }
                     if (status_data.is_active) {
                         active_endpoints.push_back(std::cref(endpoint));
                         break; // Нашли первый попавшийся активный, достаточно для этого эндпоинта
                     }
                 }
            }
        }
        // TODO: Возможно, отсортировать по latency_ms активного соединения?
        // Это потребует хранить тип активного соединения или искать его заново.
        return active_endpoints;
    }


} // namespace neozork::config_manager
