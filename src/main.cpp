#include <chrono>
#include <iostream>
#include <vector>
#include <string>
#include <exception>

// Our includes
#include "config_manager.h"
#include "cli_parser.h"
#include "main.h"

// MAIN FUNCTION
int main(int argc, char* argv[]) {
    
    // using namespace for our modules
    using namespace neozork::config_manager;
    using namespace neozork::cli_parser;
    
    
    // Timer App Start
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "NeoZorK3 Minimal Build - OK" << std::endl;
    
    // 1. Check file exists and create if needed
    try {
        if (!ensure_config_exists()) {
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR during config check/creation: " << e.what() << std::endl;
        return 1;
    }

    // 2. Load config
    struct_config current_config;
    try {
        current_config = load_config();
        std::cout << "Configuration loaded successfully." << std::endl;

        //DEBUG std::cout << "Loaded " << current_config.blockchains.size() << " blockchains." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR loading config: " << e.what() << std::endl;
        return 1;
    }

    // 3. Parse arguments
    try {
       parse_arguments(argc, argv, current_config);

       std::cout << "NeoZorK3 setup complete. Starting main logic (placeholder)..." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "ERROR processing arguments: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown runtime error." << std::endl;
        return 1;
    }

    // 4. TODO: Main logic
    //

    std::cout << "NeoZorK3 finished." << std::endl;
    

    
    // Timer App End
    auto end_time = std::chrono::high_resolution_clock::now();

    // Execution time
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

      std::cout << "Total execution time: " << duration << " ms" << std::endl;

    return 0;
}

