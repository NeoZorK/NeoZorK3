// src/command_handlers.cpp
#include "command_handlers.h" // Our header
#include "config_manager.h"   // Need full struct definitions
#include "cli_parser.h"       // Need command_parameters definition
#include "ui.h"               // Need ui::print functions
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>          // For std::runtime_error

namespace neozork::command_handlers {

// --- Handler for SHOW_ENDPOINT_INFO ---
void handle_show_endpoint_info(
                               const neozork::config_manager::struct_config& config,
                               const neozork::cli_parser::command_parameters& params)
{
    // Get the search term (parser should ensure it exists for this command type)
    if (!params.search_term) {
        // This check is defensive; the parser should have caught this.
        throw std::runtime_error("Internal Error: Search term is missing for handle_show_endpoint_info.");
    }
    const std::string& search_term = params.search_term.value();
    
    // Use ui functions for colored output
    neozork::ui::print_label("Searching for endpoints with URL containing: '");
    neozork::ui::print_value(search_term); // Assuming print_value handles strings appropriately
    neozork::ui::print_label("'\n");
    std::cout << "========================================" << std::endl;
    
    int found_count = 0;
    // Iterate through all blockchains in the config
    for (const auto& bc_info : config.blockchains) {
        // Iterate through all endpoints in the current blockchain
        for (const auto& endpoint : bc_info.endpoints) {
            bool match_found_in_endpoint = false;
            // Check if the search term is a substring of any URL defined for this endpoint
            for (const auto& url_pair : endpoint.connection_urls) {
                // Case-sensitive search for now. Could be made case-insensitive if needed.
                if (url_pair.second.find(search_term) != std::string::npos) {
                    match_found_in_endpoint = true;
                    break; // Found a match in this endpoint, no need to check its other URLs
                }
            }
            
            // If a match was found in any URL of this endpoint, print its details
            if (match_found_in_endpoint) {
                found_count++;
                // Call the UI function to print details for this endpoint in its blockchain context
                neozork::ui::print_endpoint_details(bc_info, endpoint);
            }
        } // end loop over endpoints
    } // end loop over blockchains
    
    std::cout << "========================================" << std::endl;
    neozork::ui::print_label("Total endpoints found matching the criteria: ");
    neozork::ui::print_value(found_count); // Use UI function
    std::cout << std::endl;
}


// --- Implementations for future command handlers ---

// void handle_show_block_speeds(...) { ... }
// void handle_show_active_endpoints(...) { ... }

} // namespace neozork::command_handlers
