// src/main.cpp

#include <chrono>
#include <iostream>
#include <vector>
#include <string>
#include <exception>

// Our includes
#include "blockchain_adapters.h"
#include "config_manager.h"
#include "cli_parser.h"
#include "endpoint_discovery.h"
#include "endpoint_scanner.h"
#include "main.h"
#include "version.h"
#include "ui.h"
#include "command_handlers.h"


// MAIN FUNCTION
int main(int argc, char* argv[]) {
    
    // Timer App Start
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Print program version
    std::cout << "NeoZorK3 " << neozork::PROGRAM_VERSION << std::endl;
    
    
    // Using namespaces for brevity within main
    using namespace neozork::config_manager;
    using namespace neozork::cli_parser;
    using namespace neozork::endpoint_discovery;
    using namespace neozork::endpoint_scanner;
    using namespace neozork::blockchain_adapters;
    
    // 1. Ensure configuration file exists (create default if missing)
    try {
        if (!ensure_config_exists()) {
            return 1;
        }
        std::cout << "Configuration file exists or was created." << std::endl;
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
        
        // Print help if parsing failed
        print_help();
        return 1;
    }
    
    // 3. Execute the requested command
    try {
        switch(params.type) {
                
                // --- Handle HELP ---
            case command_type::HELP:
                print_help();
                break;
                
                // --- Handle CONFIG_INIT ---
            case command_type::CONFIG_INIT:
                std::cout << "Initializing configuration..." << std::endl;
                initialize_config();
                std::cout << "Configuration file initialized successfully." << std::endl;
                break;
                
                // --- Handle DISCOVER_ENDPOINTS ---
            case command_type::DISCOVER_ENDPOINTS:
            {
                std::cout << "Loading configuration for discovery..." << std::endl;
                struct_config current_config = load_config();
                
                // Ensure blockchain name is present (already checked by parser, but double check)
                if (!params.blockchain_name) { throw std::runtime_error("Internal error: blockchain name missing for discovery."); }
                
                // Start endpoint discovery
                discover_endpoints(params.blockchain_name.value(), params.sources, current_config);
                
                // Config is saved inside discover_endpoints if changes were made
                std::cout << "Endpoint discovery process finished." << std::endl;
            }
                break;
                
                // --- Handle SCAN_ENDPOINTS ---
            case command_type::SCAN_ENDPOINTS:
            {
                std::cout << "Loading configuration for scanning..." << std::endl;
                struct_config current_config = load_config();
                
                // Ensure blockchain name is present
                if (!params.blockchain_name) { throw std::runtime_error("Internal error: blockchain name missing for scan."); }
                
                std::cout << "Starting endpoint scan..." << std::endl;
                run_scan_endpoints(current_config, params.blockchain_name.value(), params.connection_type);
                
                std::cout << "Scan finished. Saving configuration..." << std::endl;
                save_config(current_config);
                std::cout << "Configuration saved." << std::endl;
            }
                break;
                
                // --- Handle SCAN_SINGLE_ENDPOINT ---
            case command_type::SCAN_SINGLE_ENDPOINT:
            {
                std::cout << "Loading configuration for single endpoint scan..." << std::endl;
                struct_config current_config = load_config();
                
                // Ensure blockchain name and endpoint URL are present
                if (!params.blockchain_name) { throw std::runtime_error("Internal error: blockchain name missing for single scan."); }
                if (!params.endpoint_url) { throw std::runtime_error("Internal error: endpoint URL missing for single scan."); }
                
                std::cout << "Starting single endpoint scan..." << std::endl;
                run_scan_single_endpoint(current_config, params.blockchain_name.value(), params.endpoint_url.value(), params.connection_type);
                
                std::cout << "Scan finished. Saving configuration..." << std::endl;
                save_config(current_config); // Save updated status
                std::cout << "Configuration saved." << std::endl;
            }
                break;
                
                
            case command_type::MEASURE_BLOCK_SPEED:
            {
                std::cout << "Loading configuration for block speed measurement..." << std::endl;
                struct_config current_config = load_config();
                // Ensure blockchain name is present
                if (!params.blockchain_name) { throw std::runtime_error("Internal error: blockchain name missing for measure speed."); }
                
                std::cout << "Starting block speed measurement..." << std::endl;
                std::optional<double> measured_speed = measure_block_speed(current_config, params.blockchain_name.value());
                
                if(measured_speed) {
                    std::cout << "Measurement finished successfully. Average block time: " << *measured_speed << " ms. Saving configuration..." << std::endl;
                    save_config(current_config); // Save updated block speed
                    std::cout << "Configuration saved." << std::endl;
                } else {
                    std::cerr << "Block speed measurement failed. Configuration not saved." << std::endl;
                    // Optionally return different exit code?
                }
            }
                break;
                
            case command_type::SHOW_ENDPOINT_INFO:
            {
                std::cout << "Loading configuration..." << std::endl;
                struct_config current_config = load_config();
                // --- Указать полное имя функции ---
                neozork::command_handlers::handle_show_endpoint_info(current_config, params);
            }
                break;
                
                // --- Handle NONE ---
            case command_type::NONE:
                // If no command was specified, parser defaults to HELP now.
                // If we change default action later, handle it here.
                std::cout << "No specific command executed." << std::endl;
                break;
                
                // TODO: Add handling for other command_type as they are implemented
                // case command_type::SHOW_ACTIVE_ENDPOINTS: ...
                
            default:
                std::cerr << "ERROR: Unhandled command type in main!" << std::endl;
                print_help();
                return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR during command execution: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown runtime error during command execution." << std::endl;
        return 1;
    }
    
    // Timer App End
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    std::cout << "\nNeoZorK3 finished." << std::endl;
    std::cout << "Total execution time: " << duration << " ms" << std::endl;
    
    // EXIT
    return 0;
}
