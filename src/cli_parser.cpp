// src/cli_parser.cpp

#include "cli_parser.h"
#include "config_manager.h"
#include "version.h"
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <map>

namespace neozork::cli_parser {

// --- Updated print_help() function ---
void print_help() {
    std::cout << "NeoZorK3 DEX Arbitrage Bot " << neozork::PROGRAM_VERSION << "\n"
    << "===================================\n\n"
    << "Usage: neozork3_cli [command] [options]\n\n"
    << "Commands:\n"
    << "  -h, --help                 Show this help message and exit.\n"
    << "  -i, --config-init          Initialize/reset the configuration file to default and exit.\n"
    << "  -d, --discover-endpoints   Discover RPC endpoints from specified sources.\n"
    << "                             Requires -b/--blockchain.\n"
    << "                             Uses -s/--source (multiple allowed, defaults to 'chainlist').\n"
    << "      --scan-endpoints       Scan configured endpoints for a blockchain to check status,\n"
    << "                             latency, etc. Requires -b/--blockchain.\n"
    << "                             Optionally use --connection-type to scan only one type.\n"
    << "      --scan-single-endpoint Scan a *specific* configured endpoint URL for a blockchain.\n"
    << "                             Requires -b/--blockchain and --endpoint <url>.\n"
    << "                             Optionally use --connection-type to scan only one type.\n"
    // --- Planned Commands ---
    << "      --show-active-endpoints\n" // Keep planned commands for context
    << "                             List active endpoints for a blockchain.\n"
    << "      --measure-block-speed\n"
    << "                             Measure block speed for a blockchain.\n"
    << "      --find-dexes             Discover DEXes on a blockchain.\n"
    << "      --find-pools             Discover pools for a DEX on a blockchain.\n"
    << "      --get-token-price        Get token price info.\n"
    << "      --find-pools-for-token\n"
    << "                             List pools containing a specific token.\n"
    << "      --find-arbitrage-once\n"
    << "                             Perform a single arbitrage check/trade attempt.\n"
    << "      --run-tasks              Run continuous background arbitrage tasks.\n"
    << "\nCommon Options:\n"
    << "  -b, --blockchain <name|id>   Specify the target blockchain name or network ID.\n"
    << "                               Required by most commands involving a specific chain.\n"
    << "  -s, --source <keyword|url>   Specify discovery source for --discover-endpoints.\n"
    << "                               Can be used multiple times.\n"
    << "                               Keywords: 'chainlist', 'ethereum-lists'.\n"
    << "                               Or provide a direct http(s) URL to a JSON or text list.\n"
    << "                               Defaults to 'chainlist' if omitted.\n"
    << "      --endpoint <url>         Specify a single endpoint URL (required by\n"
    << "                               --scan-single-endpoint).\n"
    << "  -t, --connection-type <type>\n" // Added -t as short flag
    << "                               Optionally specify connection type (https, wss, etc.)\n"
    << "                               for scanning commands. If omitted, all types for the\n"
    << "                               endpoint(s) listed in config are scanned.\n"
    // --- Planned Options ---
    << "      --password <pass>        Password for encrypted operations (if any).\n"
    << "      --dex <dex_id>           Specify DEX ID (for --find-pools, --get-token-price).\n"
    //<< "      --connection-type <https|wss|ipc>\n" // Covered by -t above
    << "      --mode <show|trade>      Operation mode for arbitrage tasks.\n"
    << "      --strategy <max-profit|stable-profit|min-risk>\n"
    << "                               Profit/risk strategy for arbitrage.\n"
    << "      --arbitrage-types <type1,type2,...|all>\n"
    << "                               Specify arbitrage types to search for.\n"
    << "      --sync-to-block        Attempt to synchronize actions with new blocks.\n"
    << std::endl;
}

// --- Updated parse_arguments function ---
command_parameters parse_arguments(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc); // Skip program name
    command_parameters params;
    params.type = command_type::NONE; // Default to no command explicitly set yet

    // Allowed connection types for validation
    const std::map<std::string, bool> valid_connection_types = {
        {"http", true}, {"https", true}, {"ws", true}, {"wss", true}, {"ipc", true}
    };

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        // Helper lambda to check for multiple commands
        auto check_multiple_commands = [&](command_type current_cmd_type) {
            if (params.type != command_type::NONE && params.type != current_cmd_type) {
                 throw std::runtime_error("Cannot specify multiple commands (already have '"
                    + std::to_string(static_cast<int>(params.type)) // Simple representation
                    + "', trying to set '" + arg + "')");
            }
            params.type = current_cmd_type;
        };

        // --- Flags without arguments (Commands) ---
        if (arg == "--help" || arg == "-h") {
            params.type = command_type::HELP;
            return params; // Return immediately for help
        } else if (arg == "--config-init" || arg == "-i") {
            check_multiple_commands(command_type::CONFIG_INIT);
        } else if (arg == "--discover-endpoints" || arg == "-d") {
            check_multiple_commands(command_type::DISCOVER_ENDPOINTS);
        } else if (arg == "--scan-endpoints") {
             check_multiple_commands(command_type::SCAN_ENDPOINTS);
        } else if (arg == "--scan-single-endpoint") {
             check_multiple_commands(command_type::SCAN_SINGLE_ENDPOINT);
        }
        // TODO: Add other command flags here (like --show-active-endpoints etc.)

        // --- Flags WITH arguments (Options) ---
        else if (arg == "--blockchain" || arg == "-b") {
            if (i + 1 < args.size() && args[i+1].rfind("-", 0) != 0) {
                params.blockchain_name = args[++i];
            } else {
                throw std::runtime_error("Missing value for argument: " + arg);
            }
        } else if (arg == "--source" || arg == "-s") {
            // Relevant only for DISCOVER_ENDPOINTS, but parse anyway
            if (i + 1 < args.size() && args[i+1].rfind("-", 0) != 0) {
                params.sources.push_back(args[++i]);
            } else {
                throw std::runtime_error("Missing value for argument: " + arg);
            }
        } else if (arg == "--endpoint") {
            // Relevant only for SCAN_SINGLE_ENDPOINT, but parse anyway
             if (i + 1 < args.size() && args[i+1].rfind("-", 0) != 0) {
                 params.endpoint_url = args[++i];
             } else {
                 throw std::runtime_error("Missing value for argument: " + arg);
             }
        } else if (arg == "--connection-type" || arg == "-t") {
            // Relevant only for SCAN* commands, but parse anyway
             if (i + 1 < args.size() && args[i+1].rfind("-", 0) != 0) {
                 std::string conn_type = args[++i];
                 // Validate the connection type
                 if (valid_connection_types.find(conn_type) == valid_connection_types.end()) {
                     throw std::runtime_error("Invalid value for " + arg + ": '" + conn_type +
                                               "'. Allowed types: http, https, ws, wss, ipc.");
                 }
                 params.connection_type = conn_type;
             } else {
                 throw std::runtime_error("Missing value for argument: " + arg);
             }
        }
        // TODO: Add other flags with arguments here (password, dex_id, etc.)

        // --- Unknown argument ---
        else if (arg.rfind("-", 0) == 0) {
            throw std::runtime_error("Unknown option: " + arg);
        } else {
             // Allow positional arguments only if we expect them for a specific command (none currently)
             throw std::runtime_error("Unexpected positional argument: '" + arg + "'. Use flags like --blockchain or --endpoint.");
        }
    }

    // --- Post-parsing Validation ---

    // Validation for DISCOVER_ENDPOINTS
    if (params.type == command_type::DISCOVER_ENDPOINTS) {
        if (!params.blockchain_name.has_value()) {
            throw std::runtime_error("--blockchain <name|id> is required for --discover-endpoints");
        }
        if (params.sources.empty()) {
            const std::string default_source = "chainlist";
            std::cout << "Info: No --source specified for discovery, using default: '" << default_source << "'" << std::endl;
            params.sources.push_back(default_source);
        }
        // Check for irrelevant flags
        if (params.endpoint_url.has_value()) {
             std::cerr << "Warning: --endpoint argument ignored for --discover-endpoints command." << std::endl;
        }
         if (params.connection_type.has_value()) {
             std::cerr << "Warning: --connection-type argument ignored for --discover-endpoints command." << std::endl;
         }
    }
    // Validation for SCAN_ENDPOINTS
    else if (params.type == command_type::SCAN_ENDPOINTS) {
        if (!params.blockchain_name.has_value()) {
            throw std::runtime_error("--blockchain <name|id> is required for --scan-endpoints");
        }
        // Check for irrelevant flags
        if (params.endpoint_url.has_value()) {
             std::cerr << "Warning: --endpoint argument ignored for --scan-endpoints command (use --scan-single-endpoint instead)." << std::endl;
        }
        if (!params.sources.empty()) {
             std::cerr << "Warning: --source argument ignored for --scan-endpoints command." << std::endl;
        }
    }
    // Validation for SCAN_SINGLE_ENDPOINT
    else if (params.type == command_type::SCAN_SINGLE_ENDPOINT) {
        if (!params.blockchain_name.has_value()) {
            throw std::runtime_error("--blockchain <name|id> is required for --scan-single-endpoint");
        }
        if (!params.endpoint_url.has_value()) {
            throw std::runtime_error("--endpoint <url> is required for --scan-single-endpoint");
        }
         if (!params.sources.empty()) {
             std::cerr << "Warning: --source argument ignored for --scan-single-endpoint command." << std::endl;
         }
    }
    // Add validation checks for other commands as they are implemented

    // Default action if no command specified
    if (params.type == command_type::NONE) {
         // If other options were given without a command, it's likely an error
         if (params.blockchain_name.has_value() || params.endpoint_url.has_value() || params.connection_type.has_value() || !params.sources.empty()) {
              std::cerr << "Warning: Options provided without a specific command." << std::endl;
              params.type = command_type::HELP; // Show help in case of confusion
         } else {
              std::cout << "No command specified. Running default action (currently shows help)." << std::endl;
              // TODO: Decide on default action later (e.g., run tasks or show status)
              params.type = command_type::HELP; // Default to showing help for now
         }
    }

    return params; // Return collected parameters
}

} // namespace neozork::cli_parser
