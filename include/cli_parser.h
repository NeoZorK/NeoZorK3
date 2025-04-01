#ifndef NEOZORK3_CLI_PARSER_H
#define NEOZORK3_CLI_PARSER_H

#include <string>
#include <vector>
#include "config_manager.h" // Нужен доступ к типам конфига

namespace neozork::cli_parser { // Используем snake_case

    // Функция парсинга аргументов.
    // Принимает argc, argv и ссылку на загруженный конфиг
    // (на случай, если обработчики команд захотят его использовать).
    // Может бросить исключение при ошибке или завершить программу (для --help, --config-init).
    void parse_arguments(int argc, char* argv[], neozork::config_manager::struct_config& config);

    // Выводит базовую справку
    void print_help();

} // namespace neozork::cli_parser

#endif // NEOZORK3_CLI_PARSER_H
