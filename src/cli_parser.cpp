#include "cli_parser.h"     // Our header
#include "config_manager.h" // To call initialize_config
#include "version.h"
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept> // For std::runtime_error

namespace neozork::cli_parser {

// --- Updated print_help() function ---
void print_help() {
    std::cout << "NeoZorK3 DEX Arbitrage Bot " << neozork::PROGRAM_VERSION << "\n"
    << "===================================\n\n"
    << "Usage: neozork3_cli [command] [options]\n\n"
    << "Commands:\n"
    << "  -h, --help             Show this help message and exit.\n"
    << "  -i, --config-init      Initialize/reset the configuration file to default and exit.\n"
    << "  -d, --discover-endpoints\n"
    << "                         Discover RPC endpoints from specified sources.\n"
    << "                         Requires -b/--blockchain.\n"
    << "                         Uses -s/--source (multiple allowed, defaults to 'chainlist').\n"
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
    << "  -b, --blockchain <name|id>\n"
    << "                         Specify the target blockchain name or network ID.\n"
    << "                         Required by most commands involving a specific chain.\n"
    << "  -s, --source <keyword|url>\n" // Updated description
    << "                         Specify discovery source. Can be used multiple times.\n"
    << "                         Keywords: 'chainlist', 'ethereum-lists'.\n"
    << "                         Or provide a direct http(s) URL to a JSON or text list.\n"
    << "                         Defaults to 'chainlist' if omitted for --discover-endpoints.\n"
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

// --- parse_arguments function remains the same as the previous corrected version ---
// (No changes needed here, it correctly handles -s and passes the value)
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
            if (params.type != command_type::NONE && params.type != command_type::CONFIG_INIT) {
                 throw std::runtime_error("Cannot specify multiple commands");
            }
            params.type = command_type::CONFIG_INIT;
        } else if (arg == "--discover-endpoints" || arg == "-d") {
            if (params.type != command_type::NONE && params.type != command_type::DISCOVER_ENDPOINTS) {
                 throw std::runtime_error("Cannot specify multiple commands");
            }
            params.type = command_type::DISCOVER_ENDPOINTS;
        }
        // TODO: Add other command flags here

        // --- Flags WITH arguments ---
        else if (arg == "--blockchain" || arg == "-b") {
            if (i + 1 < args.size() && args[i+1].rfind("-", 0) != 0) {
                params.blockchain_name = args[++i];
            } else {
                throw std::runtime_error("Missing value for argument: " + arg);
            }
        } else if (arg == "--source" || arg == "-s") {
            if (i + 1 < args.size() && args[i+1].rfind("-", 0) != 0) {
                params.sources.push_back(args[++i]);
            } else {
                throw std::runtime_error("Missing value for argument: " + arg);
            }
        }
        // TODO: Add other flags with arguments here

        // --- Unknown argument ---
        else if (arg.rfind("-", 0) == 0) {
            throw std::runtime_error("Unknown option: " + arg);
        } else {
             throw std::runtime_error("Unexpected positional argument: '" + arg + "'. Use flags like --blockchain or --source.");
        }
    }

    // --- Post-parsing Validation ---
    if (params.type == command_type::DISCOVER_ENDPOINTS) {
        if (!params.blockchain_name.has_value()) {
            throw std::runtime_error("--blockchain <name|id> is required for --discover-endpoints");
        }
        if (params.sources.empty()) {
            const std::string default_source = "chainlist"; // Default remains chainlist
            std::cout << "No --source specified, using default source: '" << default_source << "'" << std::endl;
            params.sources.push_back(default_source);
        }
    }
    // TODO: Add validation checks for other commands

    if (params.type == command_type::NONE) {
         std::cout << "No command specified." << std::endl;
         params.type = command_type::HELP; // Default to showing help
    }

    return params; // Return collected parameters
}


} // namespace neozork::cli_parser
