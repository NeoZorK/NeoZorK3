// src/ui.cpp
#include "ui.h"
#include "config_manager.h"
#include <iostream>
#include <iomanip> // for std::setw, std::left
#include <map>
#include <vector>

namespace neozork::ui {

// Implementation for print_endpoint_details
void print_endpoint_details(
                            const neozork::config_manager::struct_blockchain_info& bc_info,
                            const neozork::config_manager::struct_endpoint& endpoint)
{
    // Print Blockchain context
    print_label("Blockchain: ");
    print_blockchain_info(bc_info.name + " (ID: " + std::to_string(bc_info.network_id) + ")");
    std::cout << std::endl;
    
    // Print URLs and Status for each connection type
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
            std::cout << std::endl;
            
            // Print Status if available
            auto status_it = endpoint.status.find(type);
            if (status_it != endpoint.status.end()) {
                const auto& status = status_it->second;
                std::cout << "      Status: ";
                print_status(status.is_active);
                
                if (status.latency_ms.has_value()) {
                    std::cout << ", Latency: ";
                    print_latency(status.latency_ms.value());
                } else if (status.is_active) {
                    // If active but no latency, print N/A maybe?
                    std::cout << ", Latency: "; print_label("N/A");
                }
                
                print_label("  Last Known Block: ");
                print_value(endpoint.last_block_number); // Вызовет print_value(const std::optional<long long>&)
                std::cout << std::endl;
                
                std::cout << ", Last Check: ";
                print_value(status.last_check.value_or("N/A"));
                std::cout << std::endl;
                // Print traffic/size info if needed
                // std::cout << "        Traffic In/Out: " << status.traffic_in_bytes.value_or(0)
                //           << "/" << status.traffic_out_bytes.value_or(0) << " bytes" << std::endl;
            } else {
                std::cout << "      Status: "; print_label("Not Scanned Yet"); std::cout << std::endl;
            }
        }
    }
    
    // Print other endpoint info if available
    if (endpoint.required_api_key_placeholder) {
        print_label("  Requires Key: "); print_value(*endpoint.required_api_key_placeholder); std::cout << std::endl;
    }
    if (endpoint.access_token) {
        print_label("  Access Token: "); print_value("********"); std::cout << std::endl; // Mask token
    }
    if (endpoint.parallel_query_allowance) {
        print_label("  Parallel Queries Allowed: "); print_value(*endpoint.parallel_query_allowance); std::cout << std::endl;
    }
    if (endpoint.last_block_number) {
        print_label("  Last Known Block: "); print_value(*endpoint.last_block_number); std::cout << std::endl;
    }
    // Print Rate Limits if defined
    if (endpoint.rate_limits) {
        print_label("  Rate Limits (Configured):\n");
        const auto& rl = *endpoint.rate_limits;
        if(rl.per_second) std::cout << "    Per Second: "; print_value(*rl.per_second); std::cout << std::endl;
        if(rl.per_minute) std::cout << "    Per Minute: "; print_value(*rl.per_minute); std::cout << std::endl;
        if(rl.per_hour) std::cout << "    Per Hour:   "; print_value(*rl.per_hour); std::cout << std::endl;
        // Add others if needed
    }
    
    std::cout << "----------------------------------------" << std::endl;
}


// --- Progress Bar Utility (Placeholder) ---
// TODO: Implement later


} // namespace neozork::ui
