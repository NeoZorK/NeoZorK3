// include/cli_parser.h

#ifndef NEOZORK3_CLI_PARSER_H
#define NEOZORK3_CLI_PARSER_H

#include <string>
#include <vector>
#include <optional>
#include "config_manager.h" // Need access to config types

namespace neozork::cli_parser { // Using snake_case

// Commands that the parser can recognize
enum class command_type {
    NONE,               // No command found (just launch)
    HELP,               // Show help
    CONFIG_INIT,        // Initialize config
    DISCOVER_ENDPOINTS, // Run endpoint discovery
    SCAN_ENDPOINTS,     // Run scanning for all endpoints of a blockchain
    SCAN_SINGLE_ENDPOINT, // Run scanning for a specific endpoint URL
    MEASURE_BLOCK_SPEED,  // Run block speed measurement
    SHOW_ENDPOINT_INFO,    // Show endpoint info
    SHOW_BLOCK_SPEEDS,     // Show block speeds
    SHOW_ACTIVE_ENDPOINTS  // Show active endpoints
    // TODO: Add FIND_ARBITRAGE_ONCE, RUN_TASKS etc. later
};

// Struct to hold command parameters
struct command_parameters {
    command_type type = command_type::NONE;
    std::optional<std::string> blockchain_name;       // For DISCOVER, SCAN*, etc.
    std::vector<std::string> sources;                 // For DISCOVER
    std::optional<std::string> endpoint_url;          // For SCAN_SINGLE_ENDPOINT
    std::optional<std::string> connection_type;       // Optional: For SCAN* commands to filter connection type
    std::optional<std::string> search_term;           // Filter by search term
    // TODO: Add other parameters as needed (password, dex_id, etc.)
};

/**
 * @brief Parses command line arguments.
 * @param argc Argument count.
 * @param argv Argument values array.
 * @return command_parameters struct describing the requested command and its parameters.
 * @throw std::runtime_error on parsing errors or unknown arguments.
 */
command_parameters parse_arguments(int argc, char* argv[]);

/**
 * @brief Prints the help message describing commands and options.
 */
void print_help();

} // namespace neozork::cli_parser

#endif // NEOZORK3_CLI_PARSER_H
