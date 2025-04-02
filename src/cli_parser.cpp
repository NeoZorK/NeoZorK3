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
        
        if (arg == "--help") {
            params.type = command_type::HELP;
            return params; // Return immediately
        } else if (arg == "--config-init") {
            params.type = command_type::CONFIG_INIT;
            return params; // Return immediately
        } else if (arg == "--discover-endpoints") {
            params.type = command_type::DISCOVER_ENDPOINTS;
            // Continue parsing options for this command
        } else if (arg == "--blockchain") {
            if (i + 1 < args.size()) { // Check if there's a next argument
                params.blockchain_name = args[++i]; // Take next arg as value and increment i
            } else {
                throw std::runtime_error("Missing value for argument: " + arg);
            }
        } else if (arg == "--source") {
            if (i + 1 < args.size()) {
                params.sources.push_back(args[++i]); // Add source to vector
            } else {
                throw std::runtime_error("Missing value for argument: " + arg);
            }
        }
        // TODO: Add handling for other arguments (--scan-endpoints, etc.)
        else {
            throw std::runtime_error("Unknown argument or misplaced value: " + arg);
        }
    }
    
    // Check required arguments for commands
    if (params.type == command_type::DISCOVER_ENDPOINTS) {
        if (!params.blockchain_name.has_value()) {
            throw std::runtime_error("--blockchain is required for --discover-endpoints");
        }
        // Add default source if none provided (NEW)
        if (params.sources.empty()) {
            const std::string default_source = "https://raw.githubusercontent.com/ethereum-lists/chains/master/_data/chains/eip155-1.json";
            std::cout << "No --source specified, using default: " << default_source << std::endl;
            params.sources.push_back(default_source);
        }
    }
    // TODO: Add checks for other commands
    
    return params; // Return collected parameters
}

} // namespace neozork::cli_parser
