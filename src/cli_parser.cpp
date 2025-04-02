#include "cli_parser.h"
#include "config_manager.h" // for calling initialize_config
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

namespace neozork::cli_parser {

// Show Help
    void print_help() {
        std::cout << "NeoZorK3 Blockchain Arbitrage system\n"
                  << "Usage: neozork3_cli [options]\n\n"
                  << "Options:\n"
                  << "  --help         Show this help message and exit.\n"
                  << "  --config-init  Initialize/reset the configuration file and exit.\n"
                  // TODO: other flag later
                  << std::endl;
    }


// Parse command-line arguments
    void parse_arguments(int argc, char* argv[], neozork::config_manager::struct_config& config) {
        
        // stumb warning 'config'
                 (void)config;
        
         // Argument parsing
         std::vector<std::string> args(argv + 1, argv + argc); // +1 skip program name

        // flag for command execution
         bool command_executed = false;

        // Main Cycle for arguments
         for (size_t i = 0; i < args.size(); ++i) {
             const std::string& arg = args[i];

             if (arg == "--help") {
                 print_help();
                 command_executed = true;
                 break;
             } else if (arg == "--config-init") {
                 try {
                     neozork::config_manager::initialize_config();
                     std::cout << "Configuration file initialized successfully." << std::endl;
                 } catch (const std::exception& e) {
                     std::cerr << "ERROR initializing config: " << e.what() << std::endl;
                     
                     throw;
                 }
                 command_executed = true;
                 
                 break;
             }
             // TODO: other arguments
             // else if (arg == "--scan-endpoints") { ... }
             else {

                 throw std::runtime_error("Unknown argument: " + arg);
             }
         }

        // Check if a command was executed
         if (command_executed) {
              std::exit(0);
         }
    }

} // namespace neozork::cli_parser
