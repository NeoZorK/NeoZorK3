#include "cli_parser.h"     // Our header
#include "config_manager.h" // To call initialize_config
#include "version.h"
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept> // For std::runtime_error

namespace neozork::cli_parser {

// Prints the help message (same as before)
void print_help() {
    std::cout << "NeoZorK3 DEX Arbitrage Bot " << neozork::PROGRAM_VERSION << "\n"
    << "===================================\n\n"
    << "Usage: neozork3_cli [command] [options]\n\n" // Changed order slightly
    << "Commands:\n"
    << "  -h, --help             Show this help message and exit.\n"
    << "  -i, --config-init      Initialize/reset the configuration file to default and exit.\n"
    << "  -d, --discover-endpoints\n"
    << "                         Discover RPC endpoints from specified sources.\n"
    << "                         Requires --blockchain. Optionally uses --source (multiple allowed).\n"
    // --- Planned Commands ---
    << "      --scan-endpoints     Scan configured endpoints for a blockchain.\n"
    << "      --scan-single-endpoint\n"
    << "                         Scan a specific endpoint URL.\n"
    << "      --show-active-endpoints\n"
    << "                         List active endpoints for a blockchain.\n"
    << "      --measure-block-speed\n"
    << "                         Measure block speed for a blockchain.\n"
    << "      --find-dexes         Discover DEXes on a blockchain.\n"
    << "      --find-pools         Discover pools for a DEX on a blockchain.\n"
    << "      --get-token-price    Get token price info.\n"
    << "      --find-pools-for-token\n"
    << "                         List pools containing a specific token.\n"
    << "      --find-arbitrage-once\n"
    << "                         Perform a single arbitrage check/trade attempt.\n"
    << "      --run-tasks          Run continuous background arbitrage tasks.\n"
    << "\nCommon Options:\n"
    << "  -b, --blockchain <name|id>\n" // Added alias and id clarification
    << "                         Specify the target blockchain name or network ID.\n"
    << "                         Required by most commands involving a specific chain.\n"
    << "  -s, --source <url|keyword>\n" // Added alias
    << "                         Specify discovery source (URL or keyword like 'chainlist').\n"
    << "                         Can be used multiple times for --discover-endpoints.\n"
    << "                         Defaults to 'chainlist' if omitted for --discover-endpoints.\n" // Changed default
    // --- Planned Options ---
    << "      --password <pass>    Password for encrypted operations (if any).\n"
    << "      --endpoint <url>     Specify a single endpoint URL (for --scan-single-endpoint).\n"
    << "      --dex <dex_id>       Specify DEX ID (for --find-pools, --get-token-price).\n"
    << "      --connection-type <https|wss|ipc>\n"
    << "                         Preferred connection type for actions.\n"
    << "      --mode <show|trade>  Operation mode for arbitrage tasks.\n"
    << "      --strategy <max-profit|stable-profit|min-risk>\n"
    << "                         Profit/risk strategy for arbitrage.\n"
    << "      --arbitrage-types <type1,type2,...|all>\n"
    << "                         Specify arbitrage types to search for.\n"
    << "      --sync-to-block    Attempt to synchronize actions with new blocks.\n"
    << std::endl;
}


// --- REVISED parse_arguments function ---
command_parameters parse_arguments(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc); // Skip program name
    command_parameters params;
    params.type = command_type::NONE; // Default to no command explicitly set yet

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        // --- Flags without arguments ---
        if (arg == "--help" || arg == "-h") {
            params.type = command_type::HELP;
            return params; // Return immediately for help
        } else if (arg == "--config-init" || arg == "-i") {
            params.type = command_type::CONFIG_INIT;
            // Keep parsing in case other flags are present, but the command is set
        } else if (arg == "--discover-endpoints" || arg == "-d") {
            // Only set the command type here. Arguments are handled below.
            if (params.type != command_type::NONE && params.type != command_type::DISCOVER_ENDPOINTS) {
                 throw std::runtime_error("Cannot specify multiple commands (e.g., --config-init and --discover-endpoints)");
            }
            params.type = command_type::DISCOVER_ENDPOINTS;
        }
        // TODO: Add other command flags here (scan-endpoints etc.) in the same way

        // --- Flags WITH arguments ---
        else if (arg == "--blockchain" || arg == "-b") {
            // Check if next argument exists and is not another flag
            if (i + 1 < args.size() && args[i+1].rfind("-", 0) != 0) {
                params.blockchain_name = args[++i]; // Assign value and increment i
            } else {
                throw std::runtime_error("Missing value for argument: " + arg);
            }
        } else if (arg == "--source" || arg == "-s") {
            // Check if next argument exists and is not another flag
            if (i + 1 < args.size() && args[i+1].rfind("-", 0) != 0) {
                params.sources.push_back(args[++i]); // Add value to vector and increment i
            } else {
                throw std::runtime_error("Missing value for argument: " + arg);
            }
        }
        // TODO: Add other flags with arguments here (e.g., --endpoint, --dex, --password)

        // --- Unknown argument ---
        else if (arg.rfind("-", 0) == 0) {
            // Starts with '-' but wasn't recognized above
            throw std::runtime_error("Unknown option: " + arg);
        } else {
            // Positional argument - currently not used explicitly, could be an error
            // or potentially assigned later depending on command context.
            // For now, let's treat unrecognized non-flag arguments as errors.
             throw std::runtime_error("Unexpected positional argument: " + arg);
        }
    }

    // --- Post-parsing Validation ---

    // Check required arguments based on the determined command
    if (params.type == command_type::DISCOVER_ENDPOINTS) {
        if (!params.blockchain_name.has_value()) {
            throw std::runtime_error("--blockchain <name|id> is required for --discover-endpoints");
        }
        // If no sources are provided via --source, use the default
        if (params.sources.empty()) {
            const std::string default_source = "chainlist"; // Changed default to 'chainlist' keyword
            std::cout << "No --source specified, using default source: '" << default_source << "'" << std::endl;
            params.sources.push_back(default_source);
        }
    }
    // TODO: Add validation checks for other commands (e.g., SCAN_ENDPOINTS requires --blockchain)

    // If no command was explicitly set, default action could be help or run-tasks later.
    // For now, if no specific command flag was found, treat it as needing help.
    if (params.type == command_type::NONE) {
         std::cout << "No command specified." << std::endl;
         params.type = command_type::HELP; // Default to showing help if no command given
    }


    return params; // Return collected parameters
}


} // namespace neozork::cli_parser
