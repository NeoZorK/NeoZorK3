#include <chrono>
#include <iostream>
#include <vector>
#include <string>
#include <exception>

// Our includes
#include "config_manager.h"
#include "cli_parser.h"
#include "endpoint_discovery.h"
#include "main.h"

// MAIN FUNCTION
int main(int argc, char* argv[]) {
    
    // Timer App Start
    auto start_time = std::chrono::high_resolution_clock::now();
    
    
    // Using namespaces for brevity within main
    using namespace neozork::config_manager;
    using namespace neozork::cli_parser;
    using namespace neozork::endpoint_discovery; // NEW

    // 1. Ensure configuration file exists (create default if missing)
    try {
        if (!ensure_config_exists()) {
            // Error should have been printed by ensure_config_exists
            return 1; // Exit with error
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR during config check/creation: " << e.what() << std::endl;
        return 1;
    }

    // 2. Parse command line arguments
    command_parameters params;
    try {
       params = parse_arguments(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "ERROR parsing arguments: " << e.what() << std::endl;
        print_help(); // Show help on parsing error
        return 1;
    }

    // 3. Execute the requested command
    try {
        switch(params.type) {
            case command_type::HELP:
                print_help();
                break; // Exit will happen after switch

            case command_type::CONFIG_INIT:
                initialize_config();
                std::cout << "Configuration file initialized successfully." << std::endl;
                break; // Exit will happen after switch

            case command_type::DISCOVER_ENDPOINTS:
                { // Use a block to scope the config variable
                    std::cout << "Loading configuration for discovery..." << std::endl;
                    struct_config current_config = load_config(); // Load before modifying
                    std::cout << "Discovering endpoints..." << std::endl;
                    discover_endpoints(params.blockchain_name.value(), params.sources, current_config);
                    // Config is saved inside discover_endpoints if changes were made
                }
                break;

             case command_type::NONE:
                 // If no command was specified, just load config and proceed
                 // (or show help)
                 {
                     std::cout << "Loading configuration..." << std::endl;
                     struct_config current_config = load_config();
                     std::cout << "Configuration loaded. No specific command given. Starting main logic (placeholder)..." << std::endl;
                     // TODO: Default main logic (e.g., run tasks)
                 }
                 break;

             // TODO: Add handling for other command_type
             // case command_type::SCAN_ENDPOINTS: ...

             default:
                 std::cerr << "ERROR: Unhandled command type!" << std::endl;
                 return 1;

        }
    } catch (const std::exception& e) {
         std::cerr << "ERROR during command execution: " << e.what() << std::endl;
         return 1;
    } catch (...) {
         std::cerr << "Unknown runtime error during command execution." << std::endl;
         return 1;
    }

    std::cout << "NeoZorK3 finished." << std::endl;
    
    // Timer App End
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // Execution time
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    std::cout << "Total execution time: " << duration << " ms" << std::endl;
    
    // EXIT
    return 0;
}


