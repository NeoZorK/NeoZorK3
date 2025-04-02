#ifndef NEOZORK3_CLI_PARSER_H
#define NEOZORK3_CLI_PARSER_H

#include <string>
#include <vector>
#include <optional>
#include "config_manager.h" // Need access to config types

namespace neozork::cli_parser { // Using snake_case

    // Commands that the parser can recognize
    enum class command_type {
        NONE,            // No command found (just launch)
        HELP,            // Show help
        CONFIG_INIT,     // Initialize config
        DISCOVER_ENDPOINTS // Run endpoint discovery
        // TODO: Add SCAN_ENDPOINTS, FIND_ARBITRAGE_ONCE, RUN_TASKS etc.
    };

    // Struct to hold command parameters
    struct command_parameters {
        command_type type = command_type::NONE;
        std::optional<std::string> blockchain_name; // For DISCOVER, SCAN, etc.
        std::vector<std::string> sources;           // For DISCOVER
        // TODO: Add other parameters as needed
    };

    /**
     * @brief Parses command line arguments.
     * @param argc Argument count.
     * @param argv Argument values array.
     * @return command_parameters struct describing the requested command and its parameters.
     * @throw std::runtime_error on parsing errors or unknown arguments.
     */
    command_parameters parse_arguments(int argc, char* argv[]);

    // Prints basic help message
    void print_help();

} // namespace neozork::cli_parser

#endif // NEOZORK3_CLI_PARSER_H
