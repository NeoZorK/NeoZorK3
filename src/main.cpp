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

using namespace neozork::ui::colors;

// MAIN FUNCTION
int main(int argc, char* argv[]) {
    
    // Timer App Start
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Print program version
    std::cout << bold_blue << "NeoZorK 3 " << bright_white << "BlockChain Arbitrage System " << bright_magenta << neozork::PROGRAM_VERSION << reset << std::endl;
    
    
    // Using namespaces for brevity within main
    using namespace neozork::config_manager;
    using namespace neozork::cli_parser;
    using namespace neozork::endpoint_discovery;
    using namespace neozork::endpoint_scanner;
    using namespace neozork::blockchain_adapters;
    using namespace neozork::command_handlers;
    
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
        
        // Load config (read-only by default) into optional
        std::optional<struct_config> config_opt;
        
        // Don't load for HELP or CONFIG_INIT
        if (params.type != command_type::HELP && params.type != command_type::CONFIG_INIT) {
            std::cout << "Loading configuration..." << std::endl;
            config_opt.emplace(load_config());
        }
        
        // --- Execute command based on type ---
        switch(params.type) {
                
            case command_type::HELP:
                print_help();
                break;
                
            case command_type::CONFIG_INIT:
                // Handled directly, doesn't need config object
                std::cout << "Initializing configuration..." << std::endl;
                initialize_config();
                std::cout << "Configuration file initialized successfully." << std::endl;
                break;
                
                // --- Mutable Commands ---
            case command_type::DISCOVER_ENDPOINTS:
            case command_type::SCAN_ENDPOINTS:
            case command_type::SCAN_SINGLE_ENDPOINT:
            case command_type::MEASURE_BLOCK_SPEED:
            case command_type::FIND_DEXES:
            case command_type::FIND_POOLS:
            {
                if (!config_opt) { throw std::runtime_error("Internal error: Config not loaded for mutable command."); }
                struct_config& mutable_config = config_opt.value(); // Get mutable ref
                
                // Call the appropriate handler
                if (params.type == command_type::DISCOVER_ENDPOINTS)     handle_discover_endpoints(mutable_config, params);
                else if (params.type == command_type::SCAN_ENDPOINTS)    handle_scan_endpoints(mutable_config, params);
                else if (params.type == command_type::SCAN_SINGLE_ENDPOINT) handle_scan_single_endpoint(mutable_config, params);
                else if (params.type == command_type::MEASURE_BLOCK_SPEED)  handle_measure_block_speed(mutable_config, params);
                else if (params.type == command_type::FIND_DEXES)        handle_find_dexes(mutable_config, params);
                else if (params.type == command_type::FIND_POOLS)        handle_find_pools(mutable_config, params);
                
                // Add future mutable command handlers here
                
                // Saving is now handled INSIDE the handlers for consistency
            }
                break; // End of block for mutable commands
                
                // --- Read-only Commands ---
            case command_type::SHOW_ENDPOINT_INFO:
            case command_type::SHOW_BLOCK_SPEEDS:
            case command_type::SHOW_ACTIVE_ENDPOINTS:
            {
                if (!config_opt) { throw std::runtime_error("Internal error: Config not loaded for read-only command."); }
                const struct_config& readonly_config = config_opt.value(); // Get const ref
                
                // Call the appropriate handler
                if (params.type == command_type::SHOW_ENDPOINT_INFO)     handle_show_endpoint_info(readonly_config, params);
                else if (params.type == command_type::SHOW_BLOCK_SPEEDS) handle_show_block_speeds(readonly_config, params);
                else if (params.type == command_type::SHOW_ACTIVE_ENDPOINTS) handle_show_active_endpoints(readonly_config, params);
                // Add future read-only command handlers here
            }
                break; // End of block for read-only commands
                
                // --- Handle NONE ---
            case command_type::NONE:
                std::cout << "No specific command executed." << std::endl;
                break;
                
                // --- Default (Error) ---
            default:
                std::cerr << "ERROR: Unhandled command type in main!" << std::endl;
                print_help();
                return 1;
        } // --- End switch ---
        
        // --- Save config if it was modified ---
    } catch (const std::exception& e) {
        std::cerr << "ERROR during command execution: " << e.what() << std::endl;
        return 1;
        
        // Catch-all for any other exceptions
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
