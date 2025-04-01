#include <chrono>
#include <iostream>
#include <vector>
#include <string>
#include <exception> // Для std::exception

// Включаем наши модули
#include "config_manager.h"
#include "cli_parser.h"
#include "main.h"

// MAIN FUNCTION
int main(int argc, char* argv[]) {
    
    // Используем using namespace для краткости внутри main
    using namespace neozork::config_manager;
    using namespace neozork::cli_parser;
    
    
    // Timer App Start
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "NeoZorK3 Minimal Build - OK" << std::endl;
    
    // 1. Убедиться, что конфигурационный файл существует
    try {
        if (!ensure_config_exists()) {
            // Ошибка уже должна была быть выведена в ensure_config_exists
            return 1; // Выход с ошибкой
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR during config check/creation: " << e.what() << std::endl;
        return 1;
    }

    // 2. Загрузить конфигурацию
    struct_config current_config; // Переменная для хранения конфига
    try {
        current_config = load_config();
        std::cout << "Configuration loaded successfully." << std::endl;
        // Можно вывести для проверки:
        // std::cout << "Loaded " << current_config.blockchains.size() << " blockchains." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR loading config: " << e.what() << std::endl;
        return 1;
    }

    // 3. Обработать аргументы командной строки
    try {
       parse_arguments(argc, argv, current_config);
       // Если parse_arguments не вызвала exit() (для --help, --config-init),
       // то выполнение продолжится здесь.
       std::cout << "NeoZorK3 setup complete. Starting main logic (placeholder)..." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "ERROR processing arguments: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown runtime error." << std::endl;
        return 1;
    }

    // 4. TODO: Основная логика приложения (если не вышли раньше)
    // Например, запуск сканирования, поиска арбитража и т.д.,
    // используя current_config.

    std::cout << "NeoZorK3 finished." << std::endl;
    

    
    // Timer App End
    auto end_time = std::chrono::high_resolution_clock::now();

    // Выводим время работы
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

      std::cout << "Время работы программы: " << duration << " мс" << std::endl;

    return 0;
}

