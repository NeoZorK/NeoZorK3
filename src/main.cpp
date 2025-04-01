#ifndef NEOZORK3_MAIN_H
#define NEOZORK3_MAIN_H

// Этот файл может быть пустым или содержать
// какие-то глобальные константы/объявления,
// но лучше избегать глобального состояния.
// Вероятно, его можно будет удалить.

#endif // NEOZORK3_MAIN_H

#include <iostream>
#include <vector>
#include <string>

// Включаем заголовки ТОЛЬКО тех модулей,
// с которыми main будет напрямую взаимодействовать
#include "config_manager.hpp" // Для инициализации/загрузки конфига
#include "cli_parser.hpp"     // Для парсинга аргументов

// УБИРАЕМ: #include <httplib.h>
// УБИРАЕМ: #include <nlohmann/json.hpp> // Если JSON нужен только в config_manager

int main(int argc, char* argv[]) {
    // 1. Убедиться, что конфигурационный файл существует (создать по умолчанию, если нет)
    //    Оборачиваем в try-catch на случай ошибок файловой системы или JSON
    try {
        if (!neozork::config::ensure_config_exists()) {
            std::cerr << "Error: Failed to create or find configuration file at "
                      << neozork::config::get_config_path() << std::endl;
            return 1; // Выход с ошибкой
        }
        // Можно сразу загрузить конфиг, если он нужен в main,
        // но пока пропустим, т.к. cli_parser его сам загрузит/использует при необходимости
        // auto config_data = neozork::config::load_config();
        // std::cout << "Configuration loaded." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Configuration error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown configuration error." << std::endl;
        return 1;
    }

    std::cout << "NeoZorK3 starting..." << std::endl;

    // 2. Обработать аргументы командной строки
    //    Эта функция внутри себя вызовет нужные действия (например, initialize_config)
    try {
       neozork::cli::parse_arguments(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown runtime error." << std::endl;
        return 1;
    }


    // TODO: Здесь будет основная логика приложения, если не вышли раньше
    // (например, запуск постоянных задач, если не было флагов типа --help, --config-init)

    std::cout << "NeoZorK3 finished." << std::endl;

    return 0;
}
