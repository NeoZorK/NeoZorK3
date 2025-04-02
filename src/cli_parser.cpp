#include "cli_parser.h"     // Our header
#include "config_manager.h" // To call initialize_config
#include "version.h"
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept> // For std::runtime_error

namespace neozork::cli_parser {

// Prints the help message
void print_help() {
    
    // Print the help message
    std::cout << "NeoZorK3 DEX Arbitrage Bot " <<neozork::PROGRAM_VERSION << "\n"
    << "===================================\n\n"
    << "Usage: neozork3_cli <command> [options]\n\n"
    << "Commands:\n"
    << "  -h, --help             Show this help message and exit.\n"
    << "      --config-init      Initialize/reset the configuration file to default and exit.\n"
    << "  -d, --discover-endpoints\n"
    << "                         Discover RPC endpoints from specified sources (or default).\n"
    << "                         Requires --blockchain. Optionally uses --source.\n"
    // --- Planned Commands (add descriptions later) ---
    << "      --scan-endpoints     Scan configured endpoints for a blockchain.\n"
    // << "                      Requires --blockchain.\n"
    << "      --scan-single-endpoint\n"
    // << "                         Scan a specific endpoint URL.\n"
    // << "                         Requires --blockchain and --endpoint.\n"
    << "      --show-active-endpoints\n"
    // << "                         List active endpoints for a blockchain.\n"
    // << "                         Requires --blockchain.\n"
    << "      --measure-block-speed\n"
    // << "                         Measure block speed for a blockchain.\n"
    // << "                         Requires --blockchain.\n"
    << "      --find-dexes         Discover DEXes on a blockchain.\n"
    // << "                         Requires --blockchain.\n"
    << "      --find-pools         Discover pools for a DEX on a blockchain.\n"
    // << "                         Requires --blockchain and --dex.\n"
    << "      --get-token-price    Get token price info.\n"
    // << "                         Requires <token_name> and optionally --blockchain, --dex.\n"
    << "      --find-pools-for-token\n"
    // << "                         List pools containing a specific token.\n"
    // << "                         Requires <token_name>.\n"
    << "      --find-arbitrage-once\n"
    // << "                         Perform a single arbitrage check/trade attempt.\n"
    // << "                         Requires options like --blockchain, --mode, etc.\n"
    << "      --run-tasks          Run continuous background arbitrage tasks.\n"
    << "\nCommon Options:\n"
    << "      --blockchain <name>  Specify the target blockchain name or network ID.\n"
    << "                         Required by most commands.\n"
    << "      --source <url/name>  Specify data source for discovery (URL or keyword).\n"
    << "                         Can be used multiple times. Defaults to EIP155-1 list if omitted.\n"
    // --- Planned Options (add descriptions later) ---
    << "      --password <pass>    Password for encrypted operations (if any).\n"
    << "      --endpoint <url>     Specify a single endpoint URL (for --scan-single-endpoint).\n"
    << "      --dex <dex_id>       Specify DEX ID (for --find-pools, --get-token-price).\n"
    << "      --connection-type <https|wss|ipc>\n"
    // << "                         Preferred connection type for actions.\n"
    << "      --mode <show|trade>  Operation mode for arbitrage tasks.\n"
    << "      --strategy <max-profit|stable-profit|min-risk>\n"
    // << "                         Profit/risk strategy for arbitrage.\n"
    << "      --arbitrage-types <type1,type2,...|all>\n"
    // << "                         Specify arbitrage types to search for.\n"
    << "      --sync-to-block    Attempt to synchronize actions with new blocks.\n"
    
    << std::endl;
}

command_parameters parse_arguments(int argc, char* argv[]) {
    // Convert C-style args to a more convenient std::vector<std::string>
    std::vector<std::string> args(argv + 1, argv + argc); // +1 to skip program name
    command_parameters params;
    
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        
        if (arg == "--help" || arg == "-h") {
            params.type = command_type::HELP;
            return params; // Return immediately
        } else if (arg == "--config-init" || arg == "-i") {
            params.type = command_type::CONFIG_INIT;
            return params; // Return immediately
        } else if (arg == "--discover-endpoints" || arg == "-d") {
            params.type = command_type::DISCOVER_ENDPOINTS;
            
            // if --blockchain is found, check if it has a value
            if (i + 1 < args.size() && args[i+1].rfind("--", 0) != 0) {
                params.blockchain_name = args[++i];
                // if --source is found, check if it has a value
                if (i + 1 < args.size() && args[i+1].rfind("--", 0) != 0) {
                    params.sources.push_back(args[++i]);
                }
            }
            // Flags are processed in the next iteration
        } else if (arg == "--blockchain") {
            
        } else if (arg == "--source") {
            
        }
        
    }
    
    // Check required arguments for commands
    if (params.type == command_type::DISCOVER_ENDPOINTS) {
        if (!params.blockchain_name.has_value()) {
            throw std::runtime_error("--blockchain or positional blockchain name is required for -d/--discover-endpoints");
        }
        // If no sources are provided, use the default
        if (params.sources.empty()) {
            const std::string default_source = "https://raw.githubusercontent.com/ethereum-lists/chains/master/_data/chains/eip155-1.json";
            // TODO: Add more default sources for other blockchains
            std::cout << "No --source specified or positional URL, using default for EIP155-1: " << default_source << std::endl;
            params.sources.push_back(default_source);
        }
    }
    // TODO: Add checks for other commands
    
    return params; // Return collected parameters
}

} // namespace neozork::cli_parser
