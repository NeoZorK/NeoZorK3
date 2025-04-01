#ifndef NEOZORK3_CLI_PARSER_HPP
#define NEOZORK3_CLI_PARSER_HPP

#include <string>
#include <vector>

namespace neozork::cli {

    // Функция для парсинга аргументов командной строки
    // Может вызывать другие функции (command handlers) или возвращать
    // структуру/enum, описывающую запрошенное действие.
    // Бросает исключение при ошибке парсинга или неизвестном аргументе.
    void parse_arguments(int argc, char* argv[]);

    // Можно добавить функцию для вывода справки
    void print_help();

} // namespace neozork::cli

#endif // NEOZORK3_CLI_PARSER_HPP
