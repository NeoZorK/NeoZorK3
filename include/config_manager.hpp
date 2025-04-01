#ifndef NEOZORK3_CONFIG_MANAGER_HPP
#define NEOZORK3_CONFIG_MANAGER_HPP

#include <filesystem> // Для работы с путями к файлу
#include <string>
#include "nlohmann/json.hpp" // Основная зависимость для работы с JSON

// Рекомендуется помещать ваш код в пространство имен
namespace neozork::config {

    // Получить путь к файлу конфигурации (например, рядом с бинарником)
    std::filesystem::path get_config_path();

    // Загрузить конфигурацию из файла
    // Возвращает объект JSON или бросает исключение при ошибке
    nlohmann::json load_config();

    // Сохранить конфигурацию в файл
    // Бросает исключение при ошибке записи
    void save_config(const nlohmann::json& config);

    // Создать файл конфигурации по умолчанию (с примерами)
    void create_default_config(const std::filesystem::path& path);

    // Убедиться, что конфиг существует (если нет - создать по умолчанию)
    // Возвращает true, если файл существует или был успешно создан, false при ошибке создания
    bool ensure_config_exists();

    // Принудительно инициализировать конфиг (удалить старый, создать по умолчанию)
    // Бросает исключение при ошибке
    void initialize_config();

} // namespace neozork::config

#endif // NEOZORK3_CONFIG_MANAGER_HPP
