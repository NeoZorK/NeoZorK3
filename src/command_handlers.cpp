// src/command_handlers.cpp
#include "command_handlers.h"
#include "config_manager.h"
#include "cli_parser.h"
#include "endpoint_discovery.h"
#include "endpoint_scanner.h"
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

// --- Handler for SHOW_ENDPOINT_INFO (Combines Endpoint search + context AND direct DEX search) ---
void handle_show_endpoint_info(
                               const neozork::config_manager::struct_config& config,
                               const neozork::cli_parser::command_parameters& params)
{
    // Get the search term
    if (!params.search_term) {
        throw std::runtime_error("Internal Error: Search term is missing for handle_show_endpoint_info.");
    }
    const std::string& search_term = params.search_term.value();
    
    // Update header message
    neozork::ui::print_label("Searching for Endpoints (URL) and DEXes (Name/ID) containing: '");
    neozork::ui::print_value(search_term);
    neozork::ui::print_label("'\n");
    std::cout << "========================================" << std::endl;
    
    
    int found_endpoints_count = 0; // Counter for endpoints
    int found_dex_count = 0;       // Counter for DEXes (restored)
    
    
    // Iterate through all blockchains in the config
    for (const auto& bc_info : config.blockchains) {
        
        // --- Search Endpoints (and display with DEX context) ---
        for (const auto& endpoint : bc_info.endpoints) {
            bool match_found_in_endpoint = false;
            // Check if the search term is a substring of any URL
            for (const auto& url_pair : endpoint.connection_urls) {
                if (url_pair.second.find(search_term) != std::string::npos) {
                    match_found_in_endpoint = true;
                    break;
                }
            }
            
            // If an endpoint URL match was found, print its details
            // The function print_endpoint_details also prints associated DEXes
            if (match_found_in_endpoint) {
                found_endpoints_count++;
                // Call function that prints endpoint details AND the DEX list for context
                neozork::ui::print_endpoint_details(bc_info, endpoint);
            }
        } // end loop over endpoints
        
        
        // +++ START RESTORED DEX Search +++
        // --- Search DEXes directly ---
        for (const auto& dex : bc_info.dexes) {
            // Check if search term matches DEX ID (case-sensitive) or Name (case-insensitive)
            bool id_match = (dex.id.find(search_term) != std::string::npos);
            bool name_match = contains_case_insensitive(dex.name, search_term);
            
            if (id_match || name_match) {
                // --- Check if this DEX was already shown as context for an endpoint above ---
                // (Optional optimization: could track printed blockchain/DEX combos to avoid
                // calling print_dex_details if print_endpoint_details already showed it.
                // For simplicity now, we allow potential duplicate display of DEX info
                // if search term matches both endpoint URL and DEX name/ID on same chain).
                
                found_dex_count++;
                // Call the function that prints *only* the matching DEX details
                neozork::ui::print_dex_details(bc_info, dex);
            }
        } // end loop over dexes
        // +++ END RESTORED DEX Search +++
        
        
    } // end loop over blockchains
    
    
    std::cout << "========================================" << std::endl;
    // Print summary counts (restored both)
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
// --- Handler for SHOW_ACTIVE_ENDPOINTS (MODIFIED for multi-chain name search) ---
void handle_show_active_endpoints(
                                  const neozork::config_manager::struct_config& config,          // Takes config by const ref
                                  const neozork::cli_parser::command_parameters& params)
{
    // 1. Get Blockchain Name or ID
    if (!params.blockchain_name) {
        throw std::runtime_error("Internal Error: blockchain_name is required for handle_show_active_endpoints.");
    }
    const std::string& blockchain_name_or_id = params.blockchain_name.value();
    const std::optional<std::string>& requested_connection_type = params.connection_type; // Optional filter
    
    neozork::ui::print_label("\n--- Active Endpoints for Blockchain(s) matching: ");
    neozork::ui::print_value(blockchain_name_or_id);
    if(requested_connection_type) {
        neozork::ui::print_label(" (Filtered by Type: ");
        neozork::ui::print_connection_type(*requested_connection_type);
        neozork::ui::print_label(")");
    }
    std::cout << " ---\n";
    
    
    // 2. Determine if input is ID or Name Search
    bool is_id_search = false;
    long long search_id_ll = -1;
    try {
        search_id_ll = std::stoll(blockchain_name_or_id);
        if (search_id_ll > 0) is_id_search = true;
    } catch(...) { is_id_search = false; }
    
    
    // 3. Find target blockchain(s) - use const versions of find functions
    std::vector<std::reference_wrapper<const neozork::config_manager::struct_blockchain_info>> target_blockchains;
    
    if (is_id_search) {
        // Find exactly one by ID
        auto bc_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
        if (bc_ref_opt) {
            target_blockchains.push_back(bc_ref_opt.value());
        }
    } else {
        // Find all matching by name substring
        target_blockchains = neozork::config_manager::find_all_blockchains_by_name(config, blockchain_name_or_id);
    }
    
    
    // 4. Check if any blockchains were found
    if (target_blockchains.empty()) {
        if (is_id_search) {
            std::cerr << "Show Active Endpoints Error: Blockchain with ID '" << blockchain_name_or_id << "' not found." << std::endl;
        } else {
            std::cout << "No blockchains found with a name containing '" << blockchain_name_or_id << "'." << std::endl;
        }
        return; // Exit handler
    }
    
    
    // 5. Process each found blockchain (LOOP)
    int total_active_endpoints_shown = 0;
    std::cout << "Found " << target_blockchains.size() << " matching blockchain(s). Processing..." << std::endl;
    
    
    for (const auto& bc_ref_wrapper : target_blockchains) {
        // Get const reference to the current blockchain struct
        const neozork::config_manager::struct_blockchain_info& current_bc_info = bc_ref_wrapper.get();
        
        std::cout << "\n--- Processing: " << current_bc_info.name << " (ID: " << current_bc_info.network_id << ") ---" << std::endl;
        
        
        // 3a. Print Blockchain Info (Block Speed) for current chain
        neozork::ui::print_label("Measured Block Speed: ");
        if (current_bc_info.block_speed_ms.has_value()) {
            neozork::ui::print_latency(current_bc_info.block_speed_ms.value());
        } else {
            neozork::ui::print_value("N/A");
        }
        std::cout << std::endl;
        
        
        // 4a. Get Active Endpoints for current chain
        std::string type_for_get_active = requested_connection_type.value_or("https"); // Default type preference
        std::vector<std::reference_wrapper<const neozork::config_manager::struct_endpoint>> active_endpoints =
        neozork::config_manager::get_active_endpoints(current_bc_info, type_for_get_active);
        
        
        // 5a. Filter by requested connection type (if specified) for current chain
        std::vector<std::reference_wrapper<const neozork::config_manager::struct_endpoint>> filtered_endpoints;
        if (requested_connection_type) {
            const std::string& filter_type = *requested_connection_type;
            for(const auto& endpoint_ref : active_endpoints) {
                const auto& endpoint = endpoint_ref.get();
                auto status_it = endpoint.status.find(filter_type);
                // Add endpoint if it has the requested status AND is active
                if (status_it != endpoint.status.end() && status_it->second.is_active) {
                    filtered_endpoints.push_back(endpoint_ref);
                }
            }
            // Don't print filtering message inside loop? Maybe outside once?
            // std::cout << "[Filtering by type '" << filter_type << "']" << std::endl;
        } else {
            // if no type was requested, just use all active endpoints found by get_active_endpoints
            filtered_endpoints = std::move(active_endpoints);
        }
        
        
        if (filtered_endpoints.empty()) {
            neozork::ui::print_value("  No active endpoints found matching the criteria for this blockchain.\n");
            continue; // Skip to the next blockchain in the outer loop
        }
        
        
        // 6a. Sort Endpoints for current chain
        const std::string sort_key_type = requested_connection_type.value_or(type_for_get_active);
        std::sort(filtered_endpoints.begin(), filtered_endpoints.end(),
                  [&sort_key_type](const auto& a_ref, const auto& b_ref) {
            // ... (Sorting logic remains the same as before) ...
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
            if (latency_a.has_value() && latency_b.has_value()) return *latency_a < *latency_b;
            if (latency_a.has_value()) return true;
            if (latency_b.has_value()) return false;
            // Basic fallback if no latency: compare first URL alphabetically
            std::string url_a = a.connection_urls.empty() ? "" : a.connection_urls.begin()->second;
            std::string url_b = b.connection_urls.empty() ? "" : b.connection_urls.begin()->second;
            return url_a < url_b;
        });
        
        
        std::cout << "  ----------------------------------------" << std::endl;
        
        
        // 7a. Print Endpoints for current chain
        int count_this_chain = 0;
        for(const auto& endpoint_ref : filtered_endpoints) {
            count_this_chain++;
            total_active_endpoints_shown++;
            neozork::ui::print_label("  #" + std::to_string(count_this_chain) + ":\n");
            // Pass current_bc_info which is specific to this iteration
            neozork::ui::print_endpoint_details(current_bc_info, endpoint_ref.get());
        }
        
        
    } // --- End loop over target_blockchains ---
    
    
    std::cout << "\n========================================" << std::endl;
    neozork::ui::print_label("Total active endpoints displayed across all processed blockchains: ");
    neozork::ui::print_value(total_active_endpoints_shown);
    std::cout << std::endl;
}


// --- Handler for FIND_DEXES (MODIFIED for multi-chain name search) ---
void handle_find_dexes(
                       neozork::config_manager::struct_config& config, // Takes config by non-const ref
                       const neozork::cli_parser::command_parameters& params)
{
    // 1. Get Blockchain Name or ID
    if (!params.blockchain_name) {
        throw std::runtime_error("Internal Error: blockchain_name is required for handle_find_dexes.");
    }
    const std::string& blockchain_name_or_id = params.blockchain_name.value();
    
    neozork::ui::print_label("\n--- Finding Known DEXes for Blockchain(s) matching: ");
    neozork::ui::print_value(blockchain_name_or_id);
    std::cout << " ---\n";
    
    
    // 2. Determine if input is ID or Name Search
    bool is_id_search = false;
    long long search_id_ll = -1;
    try {
        search_id_ll = std::stoll(blockchain_name_or_id);
        if (search_id_ll > 0) is_id_search = true;
    } catch(...) { is_id_search = false; }
    
    
    // 3. Find target blockchain(s)
    std::vector<std::reference_wrapper<neozork::config_manager::struct_blockchain_info>> target_blockchains;
    
    if (is_id_search) {
        // Find exactly one by ID
        auto bc_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
        if (bc_ref_opt) {
            target_blockchains.push_back(bc_ref_opt.value());
        }
    } else {
        // Find all matching by name substring
        target_blockchains = neozork::config_manager::find_all_blockchains_by_name(config, blockchain_name_or_id);
    }
    
    
    // 4. Check if any blockchains were found
    if (target_blockchains.empty()) {
        if (is_id_search) {
            std::cerr << "Error: Blockchain with ID '" << blockchain_name_or_id << "' not found in configuration." << std::endl;
        } else {
            std::cout << "No blockchains found in configuration with a name containing '" << blockchain_name_or_id << "'." << std::endl;
        }
        return; // Exit handler
    }
    
    
    // 5. Execute discovery for each found blockchain (LOOP)
    bool any_changes_made = false; // Track if saving is needed
    
    
    std::cout << "Found " << target_blockchains.size() << " matching blockchain(s). Processing..." << std::endl;
    
    
    for (auto& bc_ref_wrapper : target_blockchains) {
        neozork::config_manager::struct_blockchain_info& current_bc_info = bc_ref_wrapper.get();
        std::cout << "\n--- Processing: " << current_bc_info.name << " (ID: " << current_bc_info.network_id << ") ---" << std::endl;
        
        
        // Call the core logic function using the specific ID for unambiguous reference
        std::string current_id_str = std::to_string(current_bc_info.network_id);
        try {
            // Assume discover_dexes_for_blockchain takes config & name/id string
            // And returns true if *any action was taken or completed* for that chain.
            // We might need to refine its return value or track changes better later.
            bool result = neozork::blockchain_adapters::discover_dexes_for_blockchain(config, current_id_str);
            
            if (result) {
                // Maybe discover_dexes should return the count of added/skipped?
                // For now, assume success might mean changes occurred.
                any_changes_made = true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error during DEX discovery for " << current_bc_info.name << ": " << e.what() << std::endl;
            // Continue to the next blockchain
        }
    } // --- End loop over target_blockchains ---
    
    
    std::cout << "\nFinished processing all matching blockchains." << std::endl;
    
    
    // 6. Save Config if any changes might have occurred
    if (any_changes_made) {
        std::cout << "Saving configuration..." << std::endl;
        try {
            neozork::config_manager::save_config(config);
            std::cout << "Configuration saved successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "ERROR saving configuration after processing DEX discovery: " << e.what() << std::endl;
        }
    } else {
        std::cout << "No changes detected or all operations completed without modification, configuration not saved." << std::endl;
    }
    
    
} // end handle_find_dexes

// --- Handler for DISCOVER_ENDPOINTS (Uses New Sync Logic) ---
void handle_discover_endpoints(
    neozork::config_manager::struct_config& config,
    const neozork::cli_parser::command_parameters& params)
{
    // 1. Get Blockchain Name Filter & Sources
    // Note: blockchain_name is now treated as a FILTER.
    std::string name_filter = params.blockchain_name.value_or("*"); // Default to "*" maybe? Let's require it for now.
     if (!params.blockchain_name) {
         // Or handle default "*" if -b is omitted. Requires CLI parser change.
         throw std::runtime_error("--blockchain <name_filter> is required for --discover-endpoints (use '*' to attempt sync for all chains found in source).");
     }
     name_filter = params.blockchain_name.value(); // Use the provided filter

    const std::vector<std::string>& sources = params.sources; // sources might be empty

    // Use default source if none provided
    std::vector<std::string> sources_to_use = sources;
    if (sources_to_use.empty()) {
         sources_to_use.push_back("chain"); // Default source
         std::cout << "Info: No --source specified, using default source: 'chain'" << std::endl;
    }

    // Print info about the operation
    neozork::ui::print_label("\n--- Discovering and Synchronizing Chains/Endpoints ---\n");
    neozork::ui::print_label("Name Filter: "); neozork::ui::print_value(name_filter); std::cout << std::endl;
    neozork::ui::print_label("Sources: ");
    for(size_t i=0; i<sources_to_use.size(); ++i) {
         neozork::ui::print_value(sources_to_use[i]);
         if (i < sources_to_use.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;


    // 2. Call the new core logic function discover_and_sync_chains
    bool changes_made = false;
    try {
         // Pass the mutable config reference, the name filter, and sources
         // This is the call to the NEW function:
         changes_made = neozork::endpoint_discovery::discover_and_sync_chains(config, name_filter, sources_to_use);

    } catch (const std::exception& e) {
         std::cerr << "Error during discovery/sync process: " << e.what() << std::endl;
         changes_made = false; // Don't save on error
    }

    // 3. Save config if changes were reported by the sync function
    std::cout << "\nDiscovery/sync process finished." << std::endl;
    if (changes_made) {
         std::cout << "Saving configuration due to detected changes..." << std::endl;
         try {
             neozork::config_manager::save_config(config);
             std::cout << "Configuration saved successfully." << std::endl;
         } catch (const std::exception& e) {
             std::cerr << "ERROR saving configuration after discovery/sync: " << e.what() << std::endl;
         }
    } else {
          std::cout << "No changes detected requiring configuration save." << std::endl;
    }
} // end handle_discover_endpoints


// --- Handler for SCAN_ENDPOINTS (MODIFIED for multi-chain name search) ---
void handle_scan_endpoints(
                           neozork::config_manager::struct_config& config,
                           const neozork::cli_parser::command_parameters& params)
{
    // 1. Get Blockchain Name or ID & optional connection type
    if (!params.blockchain_name) {
        throw std::runtime_error("Internal Error: blockchain_name is required for handle_scan_endpoints.");
    }
    const std::string& blockchain_name_or_id = params.blockchain_name.value();
    const std::optional<std::string>& connection_type = params.connection_type;
    
    neozork::ui::print_label("\n--- Scanning Endpoints for Blockchain(s) matching: ");
    neozork::ui::print_value(blockchain_name_or_id);
    if(connection_type) { neozork::ui::print_label(" (Type: " + *connection_type + ")"); }
    std::cout << " ---\n";
    
    // 2. Determine if ID or Name Search
    bool is_id_search = false;
    try { if (std::stoll(blockchain_name_or_id) > 0) is_id_search = true; } catch(...) { is_id_search = false; }
    
    // 3. Find target blockchain(s)
    std::vector<std::reference_wrapper<neozork::config_manager::struct_blockchain_info>> target_blockchains;
    if (is_id_search) {
        auto bc_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
        if (bc_ref_opt) target_blockchains.push_back(bc_ref_opt.value());
    } else {
        target_blockchains = neozork::config_manager::find_all_blockchains_by_name(config, blockchain_name_or_id);
    }
    
    // 4. Check if found
    if (target_blockchains.empty()) {
        // Report error/not found message
        std::cerr << "Error/Info: No blockchain(s) found matching '" << blockchain_name_or_id << "'." << std::endl;
        return;
    }
    
    // 5. Execute scan for each found blockchain (LOOP)
    bool any_changes_made = false;
    std::cout << "Found " << target_blockchains.size() << " matching blockchain(s). Processing..." << std::endl;
    for (auto& bc_ref_wrapper : target_blockchains) {
        neozork::config_manager::struct_blockchain_info& current_bc_info = bc_ref_wrapper.get();
        std::cout << "\n--- Processing Scan: " << current_bc_info.name << " (ID: " << current_bc_info.network_id << ") ---" << std::endl;
        try {
            // Call core scanner function - assuming it modifies config directly
            neozork::endpoint_scanner::run_scan_endpoints(config, std::to_string(current_bc_info.network_id), connection_type);
            any_changes_made = true; // Scanning always updates timestamps/status
        } catch (const std::exception& e) {
            std::cerr << "Error during endpoint scan for " << current_bc_info.name << ": " << e.what() << std::endl;
        }
    }
    
    // 6. Save Config if changes were made
    std::cout << "\nFinished processing endpoint scan(s)." << std::endl;
    if (any_changes_made) {
        std::cout << "Saving configuration..." << std::endl;
        try {
            neozork::config_manager::save_config(config);
            std::cout << "Configuration saved successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "ERROR saving configuration after scanning: " << e.what() << std::endl;
        }
    } else {
        std::cout << "No changes requiring configuration save detected." << std::endl;
    }
}


// --- Handler for SCAN_SINGLE_ENDPOINT (Operates on FIRST matching blockchain) ---
void handle_scan_single_endpoint(
                                 neozork::config_manager::struct_config& config,
                                 const neozork::cli_parser::command_parameters& params)
{
    // 1. Get required parameters
    if (!params.blockchain_name) throw std::runtime_error("Internal error: blockchain name missing for single scan.");
    if (!params.endpoint_url) throw std::runtime_error("Internal error: endpoint URL missing for single scan.");
    const std::string& blockchain_name_or_id = params.blockchain_name.value();
    const std::string& endpoint_url = params.endpoint_url.value();
    const std::optional<std::string>& connection_type = params.connection_type;
    
    neozork::ui::print_label("\n--- Scanning Single Endpoint '" + endpoint_url + "' on Blockchain matching: ");
    neozork::ui::print_value(blockchain_name_or_id);
    if(connection_type) { neozork::ui::print_label(" (Type: " + *connection_type + ")"); }
    std::cout << " ---\n";
    
    // 2. Determine if ID or Name Search
    bool is_id_search = false;
    try { if (std::stoll(blockchain_name_or_id) > 0) is_id_search = true; } catch(...) { is_id_search = false; }
    
    // 3. Find the FIRST matching target blockchain
    std::optional<std::reference_wrapper<neozork::config_manager::struct_blockchain_info>> target_blockchain_opt;
    if (is_id_search) {
        target_blockchain_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
    } else {
        auto all_matches = neozork::config_manager::find_all_blockchains_by_name(config, blockchain_name_or_id);
        if (!all_matches.empty()) {
            target_blockchain_opt = all_matches.front(); // Take the first match
            if (all_matches.size() > 1) {
                std::cout << "Warning: Multiple blockchains match name '" << blockchain_name_or_id
                << "'. Scanning endpoint only on the first match found: '"
                << target_blockchain_opt.value().get().name << "'." << std::endl;
            }
        }
    }
    
    // 4. Check if blockchain was found
    if (!target_blockchain_opt) {
        std::cerr << "Error: No blockchain found matching '" << blockchain_name_or_id << "'." << std::endl;
        return;
    }
    
    // 5. Execute scan for the single blockchain found
    neozork::config_manager::struct_blockchain_info& current_bc_info = target_blockchain_opt.value().get();
    std::cout << "Processing blockchain: " << current_bc_info.name << " (ID: " << current_bc_info.network_id << ")" << std::endl;
    bool changes_made = false;
    try {
        // Use specific ID for the call
        neozork::endpoint_scanner::run_scan_single_endpoint(config, std::to_string(current_bc_info.network_id), endpoint_url, connection_type);
        changes_made = true; // Scanning always updates
    } catch (const std::exception& e) {
        std::cerr << "Error during single endpoint scan for " << current_bc_info.name << ": " << e.what() << std::endl;
    }
    
    // 6. Save Config if changes were made
    std::cout << "\nFinished processing single endpoint scan." << std::endl;
    if (changes_made) {
        std::cout << "Saving configuration..." << std::endl;
        try {
            neozork::config_manager::save_config(config);
            std::cout << "Configuration saved successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "ERROR saving configuration after single scan: " << e.what() << std::endl;
        }
    }
}


// --- Handler for MEASURE_BLOCK_SPEED (MODIFIED for multi-chain name search) ---
void handle_measure_block_speed(
                                neozork::config_manager::struct_config& config,
                                const neozork::cli_parser::command_parameters& params)
{
    // 1. Get Blockchain Name or ID
    if (!params.blockchain_name) {
        throw std::runtime_error("Internal Error: blockchain_name is required for handle_measure_block_speed.");
    }
    const std::string& blockchain_name_or_id = params.blockchain_name.value();
    
    neozork::ui::print_label("\n--- Measuring Block Speed for Blockchain(s) matching: ");
    neozork::ui::print_value(blockchain_name_or_id);
    std::cout << " ---\n";
    
    // 2. Determine if ID or Name Search
    bool is_id_search = false;
    try { if (std::stoll(blockchain_name_or_id) > 0) is_id_search = true; } catch(...) { is_id_search = false; }
    
    // 3. Find target blockchain(s)
    std::vector<std::reference_wrapper<neozork::config_manager::struct_blockchain_info>> target_blockchains;
    if (is_id_search) {
        auto bc_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
        if (bc_ref_opt) target_blockchains.push_back(bc_ref_opt.value());
    } else {
        target_blockchains = neozork::config_manager::find_all_blockchains_by_name(config, blockchain_name_or_id);
    }
    
    // 4. Check if found
    if (target_blockchains.empty()) {
        std::cerr << "Error/Info: No blockchain(s) found matching '" << blockchain_name_or_id << "'." << std::endl;
        return;
    }
    
    // 5. Execute measurement for each found blockchain (LOOP)
    bool any_changes_made = false;
    std::cout << "Found " << target_blockchains.size() << " matching blockchain(s). Processing..." << std::endl;
    for (auto& bc_ref_wrapper : target_blockchains) {
        neozork::config_manager::struct_blockchain_info& current_bc_info = bc_ref_wrapper.get();
        std::cout << "\n--- Processing Measurement: " << current_bc_info.name << " (ID: " << current_bc_info.network_id << ") ---" << std::endl;
        try {
            // Call core function using specific ID
            std::optional<double> result = neozork::blockchain_adapters::measure_block_speed(config, std::to_string(current_bc_info.network_id));
            if (result.has_value()) {
                any_changes_made = true; // Measurement succeeded and updated config
            }
        } catch (const std::exception& e) {
            std::cerr << "Error during block speed measurement for " << current_bc_info.name << ": " << e.what() << std::endl;
        }
    }
    
    // 6. Save Config if changes were made
    std::cout << "\nFinished processing block speed measurement(s)." << std::endl;
    if (any_changes_made) {
        std::cout << "Saving configuration..." << std::endl;
        try {
            neozork::config_manager::save_config(config);
            std::cout << "Configuration saved successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "ERROR saving configuration after measurement: " << e.what() << std::endl;
        }
    } else {
        std::cout << "No successful measurements completed, configuration not saved." << std::endl;
    }
}




// --- Implementations for future command handlers ---


} // namespace neozork::command_handlers
