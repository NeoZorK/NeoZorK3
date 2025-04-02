#ifndef NEOZORK3_CLI_PARSER_H
#define NEOZORK3_CLI_PARSER_H

#include <string>
#include <vector>
#include "config_manager.h" // acces to struct of config

namespace neozork::cli_parser {

// Receive argc, argv and link loaded and parsed config
void parse_arguments(int argc, char* argv[], neozork::config_manager::struct_config& config);

// help
void print_help();

} // namespace neozork::cli_parser

#endif // NEOZORK3_CLI_PARSER_H
