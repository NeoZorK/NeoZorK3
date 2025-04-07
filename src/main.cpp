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
                
                // --- Handle DISCOVER_ENDPOINTS (Mutable) ---
            case command_type::DISCOVER_ENDPOINTS:
            {
                if (!config_opt) {
                    throw std::runtime_error("Internal error: Config not loaded for DISCOVER_ENDPOINTS.");
                }
                struct_config& mutable_config = config_opt.value();
                
                if (!params.blockchain_name) {
                    throw std::runtime_error("Internal error: blockchain name missing for discovery.");
                    
                }
                // Start endpoint discovery - saving is handled inside this function now
                // Ensure arguments match the function declaration order: string, vector, config&
                discover_endpoints(params.blockchain_name.value(), params.sources, mutable_config);
                std::cout << "Endpoint discovery process finished." << std::endl;
            }
                break;
                
                // --- Handle SCAN_ENDPOINTS (Mutable) ---
            case command_type::SCAN_ENDPOINTS:
            {
                if (!config_opt) { throw std::runtime_error("Internal error: Config not loaded for SCAN_ENDPOINTS."); }
                struct_config& mutable_config = config_opt.value();
                
                if (!params.blockchain_name) { throw std::runtime_error("Internal error: blockchain name missing for scan."); }
                std::cout << "Starting endpoint scan..." << std::endl;
                run_scan_endpoints(mutable_config, params.blockchain_name.value(), params.connection_type);
                
                // Save config after scanning
                std::cout << "Scan finished. Saving configuration..." << std::endl;
                save_config(mutable_config);
                std::cout << "Configuration saved." << std::endl;
            }
                break;
                
                // --- Handle SCAN_SINGLE_ENDPOINT (Mutable) ---
            case command_type::SCAN_SINGLE_ENDPOINT:
            {
                if (!config_opt) { throw std::runtime_error("Internal error: Config not loaded for SCAN_SINGLE_ENDPOINT."); }
                struct_config& mutable_config = config_opt.value();
                
                if (!params.blockchain_name) { throw std::runtime_error("Internal error: blockchain name missing for single scan."); }
                if (!params.endpoint_url) { throw std::runtime_error("Internal error: endpoint URL missing for single scan."); }
                std::cout << "Starting single endpoint scan..." << std::endl;
                run_scan_single_endpoint(mutable_config, params.blockchain_name.value(), params.endpoint_url.value(), params.connection_type);
                
                // Save config after scanning
                std::cout << "Scan finished. Saving configuration..." << std::endl;
                save_config(mutable_config);
                std::cout << "Configuration saved." << std::endl;
            }
                break;
                
                // --- Handle MEASURE_BLOCK_SPEED (Mutable) ---
            case command_type::MEASURE_BLOCK_SPEED:
            {
                if (!config_opt) { throw std::runtime_error("Internal error: Config not loaded for MEASURE_BLOCK_SPEED."); }
                struct_config& mutable_config = config_opt.value();
                
                if (!params.blockchain_name) { throw std::runtime_error("Internal error: blockchain name missing for measure speed."); }
                std::cout << "Starting block speed measurement..." << std::endl;
                std::optional<double> measured_speed = measure_block_speed(mutable_config, params.blockchain_name.value());
                
                // Save config only if measurement was successful
                if(measured_speed) {
                    std::cout << "Measurement finished successfully. Average block time: " << *measured_speed << " ms. Saving configuration..." << std::endl;
                    save_config(mutable_config);
                    std::cout << "Configuration saved." << std::endl;
                } else {
                    std::cerr << "Block speed measurement failed. Configuration not saved." << std::endl;
                }
            }
                break;
                
                // --- Handle FIND_DEXES (Mutable) ---
            case command_type::FIND_DEXES:
            {
                if (!config_opt) {
                    throw std::runtime_error("Internal error: Config not loaded for FIND_DEXES.");
                }
                struct_config& mutable_config = config_opt.value();
                
                // Call the function to find DEXes
                handle_find_dexes(mutable_config, params);
            }
                break;
                
                // --- Read-only commands ---
            case command_type::SHOW_ENDPOINT_INFO:
            case command_type::SHOW_BLOCK_SPEEDS:
            case command_type::SHOW_ACTIVE_ENDPOINTS:
            {
                if (!config_opt) { throw std::runtime_error("Internal error: Config not loaded for read-only command."); }
                
                // Use const reference to avoid copying
                const struct_config& readonly_config = config_opt.value();
                
                if (params.type == command_type::SHOW_ENDPOINT_INFO) {
                    handle_show_endpoint_info(readonly_config, params);
                } else if (params.type == command_type::SHOW_BLOCK_SPEEDS) {
                    handle_show_block_speeds(readonly_config, params);
                } else if (params.type == command_type::SHOW_ACTIVE_ENDPOINTS) {
                    handle_show_active_endpoints(readonly_config, params);
                }
            }
                break; // End of block for read-only commands
                
                
                // --- Handle NONE ---
            case command_type::NONE:
                // If no command was specified, parser defaults to HELP now.
                std::cout << "No specific command executed." << std::endl;
                break;
                
                // --- Handle unknown command ---
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
