// include/command_handlers.h
#ifndef NEOZORK3_COMMAND_HANDLERS_H
#define NEOZORK3_COMMAND_HANDLERS_H

#include <string>
#include <vector>
#include <optional>

// Forward declarations to potentially reduce compile times,
// but including headers is simpler for now.
#include "config_manager.h"
#include "cli_parser.h"

namespace neozork::command_handlers {

// Helper function (if needed, or move to utils)
bool contains_case_insensitive(const std::string& haystack, const std::string& needle);


// --- Handlers for Mutable Operations (take config by non-const ref) ---

/**
 * @brief Handles the '--discover-endpoints' command. Processes one or multiple blockchains.
 */
void handle_discover_endpoints(
                               neozork::config_manager::struct_config& config,
                               const neozork::cli_parser::command_parameters& params
                               );

/**
 * @brief Handles the '--scan-endpoints' command. Processes one or multiple blockchains.
 */
void handle_scan_endpoints(
                           neozork::config_manager::struct_config& config,
                           const neozork::cli_parser::command_parameters& params
                           );

/**
 * @brief Handles the '--scan-single-endpoint' command. Processes a single endpoint on the first matching blockchain.
 */
void handle_scan_single_endpoint(
                                 neozork::config_manager::struct_config& config,
                                 const neozork::cli_parser::command_parameters& params
                                 );

/**
 * @brief Handles the '--measure-block-speed' (or --get-block) command. Processes one or multiple blockchains.
 */
void handle_measure_block_speed(
                                neozork::config_manager::struct_config& config,
                                const neozork::cli_parser::command_parameters& params
                                );

/**
 * @brief Handles the '--show-block-speeds' command.
 * Reads the configuration and displays a formatted table showing the
 * measured block speed for each configured blockchain.
 * @param config The loaded application configuration.
 * @param params The parsed command line parameters (not used by this handler).
 */
void handle_show_block_speeds( // <-- Объявить
                              const neozork::config_manager::struct_config& config,
                              const neozork::cli_parser::command_parameters& params
                              );

/**
 * @brief Handles the '--show-endpoint-info <search_term>' command.
 * Loads the configuration, searches for endpoints where any URL contains the search term
 * across all blockchains, and prints detailed information using the UI module.
 * @param config The loaded application configuration.
 * @param params The parsed command line parameters containing the search term.
 * @throws std::runtime_error if the search term is missing in params.
 */
void handle_show_endpoint_info(
                               const neozork::config_manager::struct_config& config,
                               const neozork::cli_parser::command_parameters& params
                               );
/**
 * @brief Handles the '--show-active-endpoints' command.
 * Finds active endpoints for a specified blockchain (optionally filtered by
 * connection type), sorts them by latency, and displays detailed info.
 * Also displays the stored block speed for the blockchain.
 * @param config The loaded application configuration.
 * @param params The parsed command line parameters (must include blockchain_name).
 * @throws std::runtime_error if the blockchain is not found.
 */
void handle_show_active_endpoints( // <-- Объявить
                                  const neozork::config_manager::struct_config& config,
                                  const neozork::cli_parser::command_parameters& params
                                  );


/**
 * @brief Handles the '--find-dexes' command.
 * Attempts to find known DEXes on the specified blockchain and add them to the config.
 * @param config The application configuration (mutable).
 * @param params The parsed command line parameters (must include blockchain_name).
 * @throws std::runtime_error if the blockchain is not found or other errors occur.
 */
void handle_find_dexes( // Takes config by non-const ref
                       neozork::config_manager::struct_config& config,
                       const neozork::cli_parser::command_parameters& params
                       );
// 

} // namespace neozork::command_handlers

#endif // NEOZORK3_COMMAND_HANDLERS_H
