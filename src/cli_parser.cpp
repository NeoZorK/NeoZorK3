#include "cli_parser.h"     // Our header
#include "config_manager.h" // To call initialize_config
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept> // For std::runtime_error

namespace neozork::cli_parser {

void print_help() {
    std::cout << "NeoZorK3 DEX Arbitrage Bot\n"
    << "Usage: neozork3_cli [command] [options]\n\n"
    << "Commands:\n"
    << "  --help                 Show this help message and exit.\n"
    << "  --config-init          Initialize/reset the configuration file and exit.\n"
    << "  --discover-endpoints   Discover RPC endpoints from sources.\n"
    << "                         Requires --blockchain and --source flags.\n"
    // TODO: Add description for other commands and options later
    << "\nOptions for --discover-endpoints:\n"
    << "  --blockchain <name>    Specify the blockchain name or network ID.\n"
    << "  --source <url/name>    Specify source (URL or keyword like defillama). Can be used multiple times.\n"
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
