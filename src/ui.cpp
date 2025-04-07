// src/ui.cpp
#include "ui.h"
#include "config_manager.h" // Needed for print_endpoint_details implementation
#include <iostream>
#include <iomanip>
#include <string>
#include <vector> // Include vector if needed by print_endpoint_details implementation details
#include <map>    // Include map if needed by print_endpoint_details implementation details
#include <cmath>  // For std::round in progress bar percentage

namespace neozork::ui {


// --- Global variables for progress bar state (within .cpp file) ---
// NOTE: This simple implementation is NOT thread-safe if you introduce threading later.
// For multi-threaded progress, more complex state management would be needed.
namespace { // Anonymous namespace to limit scope to this file
long long progress_total_items = 0;
long long progress_current_item = 0;
std::string progress_label = "";
const int progress_bar_width = 40; // Width of the bar itself in characters
}
// --- End Progress Bar State ---


// Implementation for print_endpoint_details
void print_endpoint_details(
                            const neozork::config_manager::struct_blockchain_info& bc_info,
                            const neozork::config_manager::struct_endpoint& endpoint)
{
    print_label("Blockchain: ");
    print_blockchain_info(bc_info.name + " (ID: " + std::to_string(bc_info.network_id) + ")");
    std::cout << std::endl;
    
    if (endpoint.connection_urls.empty()) {
        print_label("  URLs: ");
        print_value("(None configured)");
        std::cout << std::endl;
    } else {
        print_label("  Configured Connections:\n");
        for (const auto& url_pair : endpoint.connection_urls) {
            const std::string& type = url_pair.first;
            const std::string& url = url_pair.second;
            
            std::cout << "    - ";
            print_connection_type(type);
            std::cout << " URL: ";
            print_url(url);
            
            // Print endpoint status
            auto status_it = endpoint.status.find(type);
            if (status_it != endpoint.status.end()) {
                const auto& status = status_it->second;
                
                std::cout << "\n      Status: ";
                print_status(status.is_active);
                
                if (status.latency_ms.has_value()) {
                    std::cout << ", Latency: ";
                    print_latency(status.latency_ms.value());
                } else if (status.is_active) {
                    std::cout << ", Latency: "; print_label("N/A");
                }
                std::cout << ", Last Check: ";
                print_value(status.last_check.value_or("N/A"));
                
                // Print Traffic Info (if available) on a new indented line
                if (status.traffic_in_bytes.has_value() || status.traffic_out_bytes.has_value()) {
                    // Add newline before printing traffic info
                    std::cout << "\n        Traffic In/Out (bytes): ";
                    
                    print_value(status.traffic_in_bytes.value_or(0));
                    std::cout << " / ";
                    print_value(status.traffic_out_bytes.value_or(0));
                    std::cout << std::endl; // Keep existing newline at the end
                }
                
                
                // Print RPC Response Size (if available) on a new indented line
                if (status.rpc_response_size_bytes.has_value()) {
                    // Add newline before printing RPC size info
                    std::cout << "\n        RPC Resp Size (bytes): ";
                    
                    print_value(status.rpc_response_size_bytes.value());
                    std::cout << std::endl; // Keep existing newline at the end
                }
                
            } else {
                std::cout << "\n      Status: "; print_label("Not Scanned Yet");
            }
            std::cout << std::endl;
        }
    }
    
    // --- Print other endpoint details ---
    if (endpoint.required_api_key_placeholder) {
        print_label("  Requires Key: "); print_value(*endpoint.required_api_key_placeholder); std::cout << std::endl;
    }
    if (endpoint.access_token) {
        print_label("  Access Token: "); print_value("********"); std::cout << std::endl;
    }
    if (endpoint.parallel_query_allowance) {
        print_label("  Parallel Queries Allowed: "); print_value(*endpoint.parallel_query_allowance); std::cout << std::endl;
    }
    // --- Print last block number
    if (endpoint.last_block_number) {
        print_label("  Last Known Block: ");
        print_value(endpoint.last_block_number);
        std::cout << std::endl;
    }
    // Print Rate Limits
    if (endpoint.rate_limits) {
        print_label("  Rate Limits (Configured):\n");
        const auto& rl = *endpoint.rate_limits;
        if(rl.per_second) { std::cout << "    Per Second: "; print_value(*rl.per_second); std::cout << std::endl; }
        if(rl.per_minute) { std::cout << "    Per Minute: "; print_value(*rl.per_minute); std::cout << std::endl; }
        if(rl.per_hour)   { std::cout << "    Per Hour:   "; print_value(*rl.per_hour);   std::cout << std::endl; }
        if(rl.per_day)    { std::cout << "    Per Day:    "; print_value(*rl.per_day);    std::cout << std::endl; }
        if(rl.per_month)  { std::cout << "    Per Month:  "; print_value(*rl.per_month);   std::cout << std::endl; }
        
    }
    
    std::cout << "----------------------------------------" << std::endl;
}

/**
 * @brief Prints detailed information about a DEX.
 * @param bc_info Blockchain info where the DEX resides.
 * @param dex The DEX structure to print.
 */
void print_dex_details(
                       const neozork::config_manager::struct_blockchain_info& bc_info,
                       const neozork::config_manager::struct_dex_info& dex)
{
    // Header for DEX match
    print_label("--- DEX Match Found ---\n");
    
    
    // Blockchain context
    print_label("  Blockchain: ");
    print_blockchain_info(bc_info.name + " (ID: " + std::to_string(bc_info.network_id) + ")");
    std::cout << std::endl;
    
    
    // DEX Name
    print_label("  DEX Name:   ");
    print_value(dex.name);
    std::cout << std::endl;
    
    
    // DEX ID
    print_label("  DEX ID:     ");
    print_value(dex.id);
    std::cout << std::endl;
    
    
    // Factory Address
    print_label("  Factory:    ");
    // Use print_value overload for optional<string> if available, otherwise check manually
    if (dex.factory_address.has_value()) {
        print_value(dex.factory_address.value());
    } else {
        print_value("N/A");
    }
    std::cout << std::endl;
    
    
    // Router Address
    print_label("  Router:     ");
    if (dex.router_address.has_value()) {
        print_value(dex.router_address.value());
    } else {
        print_value("N/A");
    }
    std::cout << std::endl;
    
    
    // Separator line
    std::cout << "----------------------------------------" << std::endl;
    
}

// --- Progress Bar Implementations ---
void start_progress(const std::string& label, long long total_items) {
    // Reset state for a new bar
    progress_label = label;
    progress_total_items = (total_items > 0) ? total_items : 1; // Avoid division by zero, assume 1 if total is 0 or less
    progress_current_item = 0; // Start at 0
    
    // Initial display
    int percentage = 0;
    std::cout << colors::white << progress_label << ": " << colors::reset; // Label
    std::cout << colors::bright_green << "[" << colors::reset; // Opening bracket
    std::cout << std::string(progress_bar_width, ' '); // Empty bar space
    std::cout << colors::bright_green << "] " << colors::reset; // Closing bracket
    std::cout << std::setw(3) << percentage << "%"; // Percentage
    std::cout << " (" << progress_current_item << "/" << progress_total_items << ")"; // Count
    std::cout << std::flush; // Ensure it's displayed immediately
}

void update_progress(long long current_item) {
    progress_current_item = current_item;
    
    // Calculate progress
    double fraction = static_cast<double>(progress_current_item) / static_cast<double>(progress_total_items);
    // Clamp fraction between 0.0 and 1.0
    fraction = std::max(0.0, std::min(1.0, fraction));
    int percentage = static_cast<int>(std::round(fraction * 100.0));
    int filled_width = static_cast<int>(std::round(fraction * progress_bar_width));
    
    // Use carriage return to move cursor to the beginning of the line
    std::cout << "\r";
    
    // Print updated bar
    std::cout << colors::white << progress_label << ": " << colors::reset;
    std::cout << colors::bright_green << "["; // Opening bracket
    std::cout << colors::green << std::string(filled_width, '#'); // Filled part (green)
    std::cout << std::string(progress_bar_width - filled_width, ' '); // Empty part
    std::cout << colors::bright_green << "] " << colors::reset; // Closing bracket
    std::cout << std::setw(3) << percentage << "%"; // Percentage
    // Ensure count doesn't exceed total display due to external logic
    long long display_count = std::min(progress_current_item, progress_total_items);
    std::cout << " (" << display_count << "/" << progress_total_items << ")"; // Count
    
    // Add spaces at the end to overwrite potential longer previous output on the same line
    std::cout << "   "; // Some buffer space
    
    std::cout << std::flush; // Ensure update is displayed
}

void finish_progress() {
    // Print a newline to move off the progress bar line
    std::cout << std::endl;
    // Reset state variables (optional, good practice)
    progress_total_items = 0;
    progress_current_item = 0;
    progress_label = "";
}


} // namespace neozork::ui
