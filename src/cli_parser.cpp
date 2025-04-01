#include "cli_parser.h"     // Наш заголовок
#include "config_manager.h" // Для вызова initialize_config
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept> // Для std::runtime_error

namespace neozork::cli_parser {

    void print_help() {
        std::cout << "NeoZorK3 Blockchain Arbitrage system\n"
                  << "Usage: neozork3_cli [options]\n\n"
                  << "Options:\n"
                  << "  --help         Show this help message and exit.\n"
                  << "  --config-init  Initialize/reset the configuration file and exit.\n"
                  // TODO: Добавить описание других флагов позже
                  << std::endl;
    }

    void parse_arguments(int argc, char* argv[], neozork::config_manager::struct_config& config) {
        
        // Подавляем предупреждение о неиспользуемом параметре 'config'
                 (void)config;
        
         // Преобразуем C-style аргументы в более удобный std::vector<std::string>
         std::vector<std::string> args(argv + 1, argv + argc); // +1 чтобы пропустить имя программы

         bool command_executed = false; // Флаг, что команда типа --help выполнена

         for (size_t i = 0; i < args.size(); ++i) {
             const std::string& arg = args[i];

             if (arg == "--help") {
                 print_help();
                 command_executed = true;
                 // Обычно после --help работа программы завершается
                 // Можно использовать std::exit(0) или просто дать main завершиться
                 // после выхода из этой функции, если command_executed = true.
                 break; // Выходим из цикла
             } else if (arg == "--config-init") {
                 try {
                     neozork::config_manager::initialize_config();
                     std::cout << "Configuration file initialized successfully." << std::endl;
                 } catch (const std::exception& e) {
                     std::cerr << "ERROR initializing config: " << e.what() << std::endl;
                     // Бросаем исключение дальше или выходим
                     throw; // Перебрасываем, чтобы main мог поймать
                 }
                 command_executed = true;
                 // После инициализации конфига тоже обычно завершаем работу
                 break; // Выходим из цикла
             }
             // TODO: Добавить обработку других аргументов
             // else if (arg == "--scan-endpoints") { ... }
             else {
                 // Обработка неизвестного аргумента
                 throw std::runtime_error("Unknown argument: " + arg);
             }
         }

         // Если была выполнена команда, которая должна завершать работу (help, init),
         // можно здесь либо выйти (std::exit(0)), либо вернуть флаг в main.
         // Пока просто выходим из функции. Main завершится, если не будет дальнейшей логики.
         if (command_executed) {
              // Если нужно гарантированно выйти после --help или --config-init:
              std::exit(0);
         }
    }

} // namespace neozork::cli_parser
