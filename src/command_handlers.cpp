// src/command_handlers.cpp
#include "command_handlers.h"
#include "config_manager.h"
#include "cli_parser.h"
#include "ui.h"
#include "blockchain_adapters.h" // Include for discover_dexes_for_blockchain
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <algorithm> // for std::sort
#include <functional> // for std::reference_wrapper
#include <cctype>    // Need for ::tolower

namespace neozork::command_handlers {

// Helper function for case-insensitive substring search
// (Could be moved to a utils file later)
bool contains_case_insensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true; // Empty needle is always found
    if (haystack.empty()) return false;
    
    // Simple approach: convert both to lower case
    std::string lower_haystack = haystack;
    std::string lower_needle = needle;
    std::transform(lower_haystack.begin(), lower_haystack.end(), lower_haystack.begin(), ::tolower);
    std::transform(lower_needle.begin(), lower_needle.end(), lower_needle.begin(), ::tolower);
    
    return lower_haystack.find(lower_needle) != std::string::npos;
    
    // Alternative using std::search (more complex but potentially more efficient)
    /*
     auto it = std::search(
     haystack.begin(), haystack.end(),
     needle.begin(), needle.end(),
     [](unsigned char ch1, unsigned char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
     );
     return (it != haystack.end());
     */
}

// --- Handler for SHOW_ENDPOINT_INFO (Now includes DEX search) ---
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
    neozork::ui::print_label("Searching for Endpoints (URL) and DEXes (Name/ID) containing: '");
    neozork::ui::print_value(search_term); // Assuming print_value handles strings appropriately
    neozork::ui::print_label("'\n");
    std::cout << "========================================" << std::endl;
    
    
    int found_endpoints_count = 0; // Counter for endpoints
    int found_dex_count = 0;       // Counter for DEXes
    
    
    // Iterate through all blockchains in the config
    for (const auto& bc_info : config.blockchains) {
        
        // --- Search Endpoints (Existing Logic) ---
        for (const auto& endpoint : bc_info.endpoints) {
            bool match_found_in_endpoint = false;
            // Check if the search term is a substring of any URL defined for this endpoint
            for (const auto& url_pair : endpoint.connection_urls) {
                // Case-sensitive search for URLs is usually fine
                if (url_pair.second.find(search_term) != std::string::npos) {
                    match_found_in_endpoint = true;
                    break; // Found a match in this endpoint, no need to check its other URLs
                }
            }
            
            // If a match was found in any URL of this endpoint, print its details
            if (match_found_in_endpoint) {
                found_endpoints_count++;
                // Call the UI function to print details for this endpoint
                neozork::ui::print_endpoint_details(bc_info, endpoint);
            }
        } // end loop over endpoints
        
        
        // +++ START ADDED CODE for DEX Search +++
        // --- Search DEXes ---
        for (const auto& dex : bc_info.dexes) {
            // Check if search term matches DEX ID (case-sensitive) or Name (case-insensitive)
            bool id_match = (dex.id.find(search_term) != std::string::npos);
            bool name_match = contains_case_insensitive(dex.name, search_term);
            
            if (id_match || name_match) {
                found_dex_count++;
                // Call the new UI function to print DEX details
                neozork::ui::print_dex_details(bc_info, dex);
            }
        } // end loop over dexes
        // +++ END ADDED CODE for DEX Search +++
        
        
    } // end loop over blockchains
    
    
    std::cout << "========================================" << std::endl;
    // Print summary counts
    neozork::ui::print_label("Total endpoints found matching the criteria: ");
    neozork::ui::print_value(found_endpoints_count);
    std::cout << std::endl;
    neozork::ui::print_label("Total DEXes found matching the criteria:     ");
    neozork::ui::print_value(found_dex_count);
    std::cout << std::endl;
}


// --- Handler for SHOW_BLOCK_SPEEDS ---
void handle_show_block_speeds(
                              const neozork::config_manager::struct_config& config,
                              const neozork::cli_parser::command_parameters& params)
{
    (void)params;
    
    neozork::ui::print_label("\n--- Measured Blockchain Speeds ---\n");
    
    if (config.blockchains.empty()) {
        neozork::ui::print_value("No blockchains configured.\n");
        return;
    }
    
    // --- Determine column widths ---
    const int name_width = 20;
    const int id_width = 10;
    const int speed_width = 20;
    const int total_width = name_width + id_width + speed_width;
    
    // --- Print Table Header ---
    std::cout << std::left;
    
    std::cout << neozork::ui::colors::white << std::setw(name_width) << "Blockchain Name" << neozork::ui::colors::reset;
    std::cout << neozork::ui::colors::white << std::setw(id_width) << "Network ID" << neozork::ui::colors::reset;
    std::cout << neozork::ui::colors::white << std::setw(speed_width) << "Avg Block Speed" << neozork::ui::colors::reset;
    std::cout << std::endl;
    
    // Devider
    neozork::ui::print_label(std::string(total_width, '-'));
    std::cout << std::endl;
    
    // --- Print blockchain speed info ---
    for (const auto& bc_info : config.blockchains) {
        
        std::cout << neozork::ui::colors::bold_blue << std::left << std::setw(name_width) << bc_info.name << neozork::ui::colors::reset;
        
        // ID
        std::cout << neozork::ui::colors::blue << std::right << std::setw(id_width) << bc_info.network_id << neozork::ui::colors::reset;
        
        // Print measured block speed
        std::cout << std::right << std::setw(speed_width);
        if (bc_info.block_speed_ms.has_value()) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << bc_info.block_speed_ms.value() << " ms";
            std::cout << neozork::ui::colors::red << ss.str() << neozork::ui::colors::reset;
        } else {
            std::cout << neozork::ui::colors::bright_black << "Not Measured" << neozork::ui::colors::reset;
        }
        std::cout << std::endl;
    }
    
    // Devider
    neozork::ui::print_label(std::string(total_width, '-'));
    std::cout << std::endl;
}
// --- Handler for SHOW_ACTIVE_ENDPOINTS ---
void handle_show_active_endpoints(
                                  const neozork::config_manager::struct_config& config,
                                  const neozork::cli_parser::command_parameters& params)
{
    // 1. Blockchain Name
    if (!params.blockchain_name) {
        throw std::runtime_error("Internal Error: blockchain_name is required for handle_show_active_endpoints.");
    }
    const std::string& blockchain_name_or_id = params.blockchain_name.value();
    const std::optional<std::string>& requested_connection_type = params.connection_type;
    
    neozork::ui::print_label("\n--- Active Endpoints for Blockchain: ");
    neozork::ui::print_value(blockchain_name_or_id);
    if(requested_connection_type) {
        neozork::ui::print_label(" (Type: ");
        neozork::ui::print_connection_type(*requested_connection_type);
        neozork::ui::print_label(")");
    }
    std::cout << " ---\n";
    
    // 2. Find the blockchain
    auto bc_info_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
    if (!bc_info_ref_opt) {
        throw std::runtime_error("Show Active Endpoints Error: Blockchain '" + blockchain_name_or_id + "' not found.");
    }
    const neozork::config_manager::struct_blockchain_info& bc_info = bc_info_ref_opt.value().get();
    
    // 3. Print Blockchain Info
    neozork::ui::print_label("Measured Block Speed: ");
    if (bc_info.block_speed_ms.has_value()) {
        neozork::ui::print_latency(bc_info.block_speed_ms.value());
    } else {
        neozork::ui::print_value("N/A");
    }
    std::cout << std::endl;
    
    // 4. Get Active Endpoints
    std::string type_for_get_active = requested_connection_type.value_or("https");
    
    // Get active endpoints
    std::vector<std::reference_wrapper<const neozork::config_manager::struct_endpoint>> active_endpoints =
    neozork::config_manager::get_active_endpoints(bc_info, type_for_get_active);
    
    // 5. Filter by requested connection type
    std::vector<std::reference_wrapper<const neozork::config_manager::struct_endpoint>> filtered_endpoints;
    if (requested_connection_type) {
        const std::string& filter_type = *requested_connection_type;
        for(const auto& endpoint_ref : active_endpoints) {
            const auto& endpoint = endpoint_ref.get();
            auto status_it = endpoint.status.find(filter_type);
            
            // Add endpoint if it has the requested status
            if (status_it != endpoint.status.end() && status_it->second.is_active) {
                filtered_endpoints.push_back(endpoint_ref);
            }
        }
        std::cout << "[Filtering by type '" << filter_type << "']" << std::endl;
    } else {
        // if no type was requested, just use the active endpoints
        filtered_endpoints = std::move(active_endpoints);
    }
    
    if (filtered_endpoints.empty()) {
        neozork::ui::print_value("No active endpoints found matching the criteria.\n");
        return;
    }
    
    // 6. Sort Endpoints
    // Sort by latency
    const std::string sort_key_type = requested_connection_type.value_or(type_for_get_active);
    
    std::sort(filtered_endpoints.begin(), filtered_endpoints.end(),
              [&sort_key_type](const auto& a_ref, const auto& b_ref) {
        const auto& a = a_ref.get();
        const auto& b = b_ref.get();
        
        std::optional<double> latency_a;
        auto it_a = a.status.find(sort_key_type);
        if (it_a != a.status.end() && it_a->second.latency_ms.has_value()) {
            latency_a = it_a->second.latency_ms;
        }
        
        std::optional<double> latency_b;
        auto it_b = b.status.find(sort_key_type);
        if (it_b != b.status.end() && it_b->second.latency_ms.has_value()) {
            latency_b = it_b->second.latency_ms;
        }
        
        // Rule:
        // - if both have latency, sort by latency
        // - if only A has latency, A goes first
        // - if only B has latency, B goes first
        // - if neither has latency, sort by name
        if (latency_a.has_value() && latency_b.has_value()) {
            return *latency_a < *latency_b;
        } else if (latency_a.has_value()) {
            return true; // A with latency goes first
        } else if (latency_b.has_value()) {
            return false; // B with latency goes first
        } else {
            return false; // B without latency goes first
        }
    });
    
    std::cout << "----------------------------------------" << std::endl;
    
    
    // 7. Print Endpoints
    int count = 0;
    for(const auto& endpoint_ref : filtered_endpoints) {
        count++;
        neozork::ui::print_label("#" + std::to_string(count) + ":\n");
        neozork::ui::print_endpoint_details(bc_info, endpoint_ref.get());
    }
    
    std::cout << "========================================" << std::endl;
    neozork::ui::print_label("Total active endpoints displayed: ");
    neozork::ui::print_value(count);
    std::cout << std::endl;
}

/**
 * @brief Handles the '--find-dexes' command.
 * Attempts to find known DEXes on the specified blockchain and add them to the config.
 * @param config The application configuration (mutable).
 * @param params The parsed command line parameters (must include blockchain_name).
 * @throws std::runtime_error if the blockchain is not found or other errors occur.
 */
void handle_find_dexes(
                       neozork::config_manager::struct_config& config, // Takes config by non-const ref
                       const neozork::cli_parser::command_parameters& params)
{
    // 1. Get Blockchain Name (Parser should guarantee it exists)
    if (!params.blockchain_name) {
        throw std::runtime_error("Internal Error: blockchain_name is required for handle_find_dexes.");
    }
    const std::string& blockchain_name_or_id = params.blockchain_name.value();
    
    neozork::ui::print_label("\n--- Finding Known DEXes for Blockchain: ");
    neozork::ui::print_value(blockchain_name_or_id);
    std::cout << " ---\n";
    
    // 2. Call the core logic function from blockchain_adapters
    bool changes_made = false; // Track if config needs saving
    try {
        // Pass the mutable config reference
        changes_made = neozork::blockchain_adapters::discover_dexes_for_blockchain(config, blockchain_name_or_id);
        
        // Note: discover_dexes_for_blockchain prints its own detailed logs.
        // We just need to handle the overall result and saving.
        
        if (!changes_made) {
            // This can happen if the blockchain isn't found OR if no DEXes were added/updated.
            // The function discover_dexes_for_blockchain already logs specifics.
            std::cout << "DEX discovery process completed, but no changes were made to the configuration for this run." << std::endl;
        }
        
    } catch (const std::exception& e) {
        // Catch potential errors from find_blockchain or other issues
        std::cerr << "Error during DEX discovery: " << e.what() << std::endl;
        // Indicate failure, config likely not saved unless changes_made was true before exception
        changes_made = false; // Ensure we don't save potentially inconsistent state
        // Optionally re-throw or handle differently
        return; // Exit handler on error
    }
    
    // 3. Save Config if changes were made by the discovery function
    // Note: Our current discover_dexes_for_blockchain always returns true on completion,
    //       but it internally tracks additions. We might need a better way
    //       to signal actual modification back, or just save unconditionally if no error.
    // Let's save if the process completed without throwing an exception,
    // as even checking existing DEXes is a form of "completion".
    // A more robust approach would have discover_dexes_for_blockchain return how many were added.
    // For now, save if no exception occurred during the call.
    
    std::cout << "DEX discovery finished. Saving configuration..." << std::endl;
    try {
        neozork::config_manager::save_config(config);
        std::cout << "Configuration saved successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR saving configuration after DEX discovery: " << e.what() << std::endl;
    }
    
}

// --- Implementations for future command handlers ---


} // namespace neozork::command_handlers
