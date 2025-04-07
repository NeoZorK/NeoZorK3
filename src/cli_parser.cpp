// src/cli_parser.cpp

#include "cli_parser.h"
#include "config_manager.h"
#include "version.h"
#include "ui.h"
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <map>

namespace neozork::cli_parser {

// --- Updated print_help() function ---
void print_help() {
    
    using namespace neozork::ui::colors;
    
    std::cout << "\n"
    << "===================================\n\n"
    << bright_white << "Usage:" << reset <<"neozork3_cli" << cyan << "[command] [options]" << reset <<"\n\n"
    << bright_white << " Commands:" << reset << "\n"
    << blue << "  -h, --help " << reset << "                  Show this help message and exit.\n"
    << blue << "  -i, --config-init " << reset << "           Initialize/reset the configuration file to default and exit.\n"
    << blue << "  -d, --discover-endpoints "<< reset << "    Discover RPC endpoints from specified sources.\n"
    << "                               Requires:" << yellow << " -b/--blockchain." << reset << "\n"
    << "                               Uses:" << yellow << " -s/--source" << reset << "\n"
    << "                               (multiple allowed:" << yellow << "'defi','eth','chain', defaults to 'chain'"<<reset<< ").\n"
    << blue << "      --scan"<< reset << "                   Scan configured endpoints for a blockchain to check status,\n"
    << "                               latency, etc. Requires:" << yellow << " -b/--blockchain" << reset << ".\n"
    << "                               Optionally use" << yellow << " --connection-type " << reset << "to scan only one type.\n"
    << blue << "      --scan-1"<< reset << "                 Scan a *specific* configured endpoint URL for a blockchain.\n"
    << "                               Requires:" << yellow << " -b/--blockchain"<<reset<<"and"<<yellow<<"--endpoint <url>" << reset << ".\n"
    << "                               Optionally use " << yellow << "--connection-type " << reset << "to scan only one type.\n"
    << blue << "      --get-block" << reset << "              Measure average block time for a blockchain using an\n"
    << "                               active endpoint. Requires:" << yellow << " -b/--blockchain" << reset << ".\n"
    << blue << "      --info" << yellow << "                   <term>" << reset << " Show detailed info for endpoints whose URL contains" << yellow << " <term> " << reset <<".\n"
    << "                               Searches across all configured blockchains.\n"
    << blue << "      --block" << reset << "                  Display a table of measured average block speeds\n"
    << "                               for all configured blockchains.\n"
    << blue << "      --active" << reset << "                 Show active endpoints for a blockchain, sorted by latency.\n"
    << "                               Requires:" << yellow << "-b/--blockchain." << reset << "Optionally filter by\n"
    << "                               " << yellow << "-t/--connection-type" << reset << ".\n"
    << blue << "      --find-dexes" << reset << "             Attempt to find known DEX contracts on a blockchain and\n"
    << "                               add them to the config. Requires: " << yellow << "-b/--blockchain" << reset << ".\n"
    << blue << "      --find-pools"<< reset << "             Attempt to discover liquidity pools for a specific DEX\n"
    << "                               on a blockchain. Requires: " << yellow << "-b/--blockchain" << reset << " and " << yellow << "--dex <dex_id>" << reset << ".\n"
    << bright_white << "\nCommon Options:" << reset << "\n"
    << blue << "  -b, --blockchain <name|id>"<<reset<<"   Specify the target blockchain name or network ID.\n"
    << "                               Required by most commands involving a specific chain.\n"
    << blue << "  -s, --source <keyword|url>"<<reset<<"   Specify discovery source for"<<yellow<<"--discover-endpoints"<<reset<<".\n"
    << "                               Can be used multiple times.\n"
    << "                               Keywords: "<<yellow<<"'chain'"<<reset<<" (chainid.network), "<<yellow<<"'defi'"<<reset<<" (DefiLlama), "<<yellow<<"'eth'"<<reset<<" (ethereum-lists).\n"
    << "                               Or provide a direct http(s) URL to a JSON or text list.\n"
    << "                               Defaults to 'chain' if omitted.\n"
    << blue << "      --endpoint <url>"<<reset<<"         Specify a single endpoint URL (required by\n"
    << "                               --scan-single-endpoint).\n"
    << blue << "  -t, --connection-type <type>"<<reset<<"\n"
    << "                               Optionally specify connection type "<<yellow<<"(https, wss, etc.)"<<reset<<"\n"
    << "                               for scanning commands. If omitted, all types for the\n"
    << "                               endpoint(s) listed in config are scanned.\n"
    << blue << "      --dbr <ms>"<< reset << "               Set delay in milliseconds between pool discovery requests\n"
    << "                               (Valid: 1-3000). Used only with " << yellow << "--find-pools" << reset << ".\n"
    // --- Planned Commands ---
    << bright_black << " Planned Commands: " << reset << "\n"
    << "      --get-token-price        Get token price info.\n"
    << "      --find-pools-for-token\n"
    << "                               List pools containing a specific token.\n"
    << "      --find-arbitrage-once\n"
    << "                               Perform a single arbitrage check/trade attempt.\n"
    << "      --run-tasks              Run continuous background arbitrage tasks" << reset << ".\n"
    // --- Planned Options ---
    << bright_black << " Planned Options: " << reset << "\n"
    << "      --password <pass>        Password for encrypted operations (if any).\n"
    << "      --dex <dex_id>           Specify DEX ID (for --find-pools, --get-token-price).\n"
    //<< "      --connection-type <https|wss|ipc>\n" // Covered by -t above
    << "      --mode <show|trade>      Operation mode for arbitrage tasks.\n"
    << "      --strategy <max-profit|stable-profit|min-risk>\n"
    << "                               Profit/risk strategy for arbitrage.\n"
    << "      --arbitrage-types <type1,type2,...|all>\n"
    << "                               Specify arbitrage types to search for.\n"
    << "      --sync-to-block          Attempt to synchronize actions with new blocks" << reset << ".\n"
    << std::endl;
}

// --- Updated parse_arguments function ---
command_parameters parse_arguments(int argc, char* argv[]) {
    
    // Parse command-line arguments
    std::vector<std::string> args(argv + 1, argv + argc);
    
    // Skip program name
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
                                         + std::to_string(static_cast<int>(params.type))
                                         + "', trying to set '" + arg + "')");
            }
            params.type = current_cmd_type;
        };
        
        // --- Flags without arguments (Commands) ---
        if (arg == "--help" || arg == "-h") {
            params.type = command_type::HELP;
            return params;
        } else if (arg == "--config-init" || arg == "-i") {
            check_multiple_commands(command_type::CONFIG_INIT);
        } else if (arg == "--discover-endpoints" || arg == "-d") {
            check_multiple_commands(command_type::DISCOVER_ENDPOINTS);
        } else if (arg == "--scan") {
            check_multiple_commands(command_type::SCAN_ENDPOINTS);
        } else if (arg == "--scan-1") {
            check_multiple_commands(command_type::SCAN_SINGLE_ENDPOINT);
        }else if (arg == "--get-block") {
            check_multiple_commands(command_type::MEASURE_BLOCK_SPEED);
        } else if (arg == "--info") {
            check_multiple_commands(command_type::SHOW_ENDPOINT_INFO);
            if (i + 1 < args.size() && args[i+1].rfind("-", 0) != 0) {
                params.search_term = args[++i];
            } else {
                throw std::runtime_error("Missing value for argument: " + arg);
            }
        }
        else if (arg == "--block") {
            check_multiple_commands(command_type::SHOW_BLOCK_SPEEDS);
        }
        else if (arg == "--active") {
            check_multiple_commands(command_type::SHOW_ACTIVE_ENDPOINTS);
        }
        else if (arg == "--find-dexes") {
            check_multiple_commands(command_type::FIND_DEXES);
        }
        else if (arg == "--find-pools") {
            check_multiple_commands(command_type::FIND_POOLS);
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
        else if (arg == "--dex") {
            // Relevant only for FIND_POOLS, but parse anyway
            if (i + 1 < args.size() && args[i+1].rfind("-", 0) != 0) {
                params.dex_id = args[++i];
            } else {
                throw std::runtime_error("Missing value for argument: " + arg);
            }
        }
        // Option: --dbr (Delay Between Requests)
        else if (arg == "--dbr") {
            
            std::cout << "DEBUG PARSER: Processing --dbr argument..." << std::endl << std::flush;
                       
             if (i + 1 < args.size() && args[i+1].rfind("-", 0) != 0) {
                  std::string delay_str = args[++i];
                  try {
                      int delay_val = std::stoi(delay_str);
                      // Validate range
                      if (delay_val >= 1 && delay_val <= 3000) {
                          params.delay_between_requests_ms = delay_val;
                      } else {
                          throw std::runtime_error("Value for " + arg + " must be between 1 and 3000 ms.");
                      }
                  } catch (const std::invalid_argument& e) {
                      throw std::runtime_error("Invalid integer value for " + arg + ": '" + delay_str + "'");
                  } catch (const std::out_of_range& e) {
                       throw std::runtime_error("Value out of range for integer for " + arg + ": '" + delay_str + "'");
                  }
             } else {
                  throw std::runtime_error("Missing value for argument: " + arg);
             }
        }
        
        
        else if (params.type == command_type::SHOW_ENDPOINT_INFO) {
            if (params.blockchain_name.has_value()) {
                std::cerr << "Warning: --blockchain argument ignored for --info (searches all blockchains)." << std::endl;
            }
            if (params.endpoint_url.has_value()) {
                std::cerr << "Warning: --endpoint argument ignored for --info." << std::endl;
            }
        }
        
        else if (params.type == command_type::SHOW_BLOCK_SPEEDS) {
            if (params.blockchain_name.has_value() || params.search_term.has_value() ) {
                std::cerr << "Warning: Arguments like --blockchain, --search-term, etc. are ignored for --block." << std::endl;
            }
        }
        
        else if (params.type == command_type::FIND_DEXES) {
            if (!params.blockchain_name.has_value()) {
                throw std::runtime_error("--blockchain <name|id> is required for --find-dexes");
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
            const std::string default_source = "chain";
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
            throw std::runtime_error("--blockchain <name|id> is required for --scan");
        }
        // Check for irrelevant flags
        if (params.endpoint_url.has_value()) {
            std::cerr << "Warning: --endpoint argument ignored for --scan command (use --scan-1 instead)." << std::endl;
        }
        if (!params.sources.empty()) {
            std::cerr << "Warning: --source argument ignored for --scan command." << std::endl;
        }
    }
    // Validation for SCAN_SINGLE_ENDPOINT
    else if (params.type == command_type::SCAN_SINGLE_ENDPOINT) {
        if (!params.blockchain_name.has_value()) {
            throw std::runtime_error("--blockchain <name|id> is required for --scan-1");
        }
        if (!params.endpoint_url.has_value()) {
            throw std::runtime_error("--endpoint <url> is required for --scan-1");
        }
        if (!params.sources.empty()) {
            std::cerr << "Warning: --source argument ignored for --scan-1 command." << std::endl;
        }
    }
    
    // Validation for MEASURE_BLOCK_SPEED
    else if (params.type == command_type::MEASURE_BLOCK_SPEED) {
        if (!params.blockchain_name.has_value()) {
            throw std::runtime_error("--blockchain <name|id> is required for --get-block");
        }
        // Check for irrelevant flags (optional, good practice)
        if (params.endpoint_url.has_value()) {
            std::cerr << "Warning: --endpoint argument ignored for --get-block command." << std::endl;
        }
        if (params.connection_type.has_value()) {
            std::cerr << "Warning: --connection-type argument ignored for --get-block command." << std::endl;
        }
        if (!params.sources.empty()) {
            std::cerr << "Warning: --source argument ignored for --get-block command." << std::endl;
        }
    }
    
    // Validation for SHOW_ACTIVE_ENDPOINTS
    else if (params.type == command_type::SHOW_ACTIVE_ENDPOINTS) {
        if (!params.blockchain_name.has_value()) {
            throw std::runtime_error("--blockchain <name|id> is required for --active");
        }
        // Check for irrelevant flags
        if (params.endpoint_url.has_value()) {
            std::cerr << "Warning: --endpoint argument ignored for --active." << std::endl;
        }
        if (!params.sources.empty()) {
            std::cerr << "Warning: --source argument ignored for --active." << std::endl;
        }
        if (params.search_term.has_value()) {
            std::cerr << "Warning: --search-term argument ignored for --active." << std::endl;
        }
    }
    
    // Validation for SHOW_BLOCK_SPEEDS
    else if (params.type == command_type::FIND_DEXES) {
        if (!params.blockchain_name.has_value()) {
            throw std::runtime_error("--blockchain <name|id> is required for --find-dexes");
        }
    }
    
    // Validation for FIND_POOLS (add check for --dbr usage)
    else if (params.type == command_type::FIND_POOLS) {
        if (!params.blockchain_name.has_value()) {
            throw std::runtime_error("--blockchain <name|id> is required for --find-pools");
        }
        // Check for irrelevant flags (Example)
         if (!params.sources.empty()) { std::cerr << "Warning: --source argument ignored for --find-pools command.\n"; }
    }
    // +++ START ADDED CODE +++
    // General check: Warn if --dbr is used with wrong command
    else if (params.delay_between_requests_ms.has_value()) {
         // If dbr is set, but the command is not FIND_POOLS (or future commands needing it)
         if (params.type != command_type::FIND_POOLS) {
               std::cerr << "Warning: --dbr option is only used with --find-pools and will be ignored for command type "
                         << static_cast<int>(params.type) << ".\n";
         }
    }
    
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
