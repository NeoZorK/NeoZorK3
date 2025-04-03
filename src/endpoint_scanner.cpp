// src/endpoint_scanner.cpp

#include "endpoint_scanner.h"
#include "config_manager.h"
#include "connection_manager.h" // To make RPC calls
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <chrono>       // For timestamps and latency
#include <iomanip>      // For std::put_time
#include <sstream>      // For string formatting
#include <ctime>        // For std::time_t, std::gmtime
#include <nlohmann/json.hpp> // To parse RPC responses

using json = nlohmann::json;

namespace neozork::endpoint_scanner {

// --- Helper Functions ---

/**
 * @brief Formats a time point into an ISO 8601 string (UTC).
 * Example: "2025-04-03T18:30:00Z"
 * @param tp The time point to format.
 * @return std::string The formatted time string.
 */
std::string format_iso8601_time(const std::chrono::system_clock::time_point& tp) {
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm gm_tm = *std::gmtime(&time); // Use gmtime for UTC ('Z' suffix)
    std::stringstream ss;
    ss << std::put_time(&gm_tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

/**
 * @brief Checks if a JSON RPC response indicates success.
 * Success means the body is valid JSON, has a "result" field, and no "error" field.
 * @param response_body The JSON string response body.
 * @return bool True if the response is a successful JSON RPC response.
 */
bool is_successful_rpc_response(const std::optional<std::string>& response_body) {
    if (!response_body || response_body.value().empty()) {
        return false;
    }
    try {
        json j = json::parse(response_body.value());
        // Check basic JSON RPC structure: must have 'jsonrpc', 'id', and NO 'error' field.
        // We also expect a 'result' field on success.
        if (j.is_object() && j.contains("jsonrpc") && j.contains("id") && j.contains("result") && !j.contains("error")) {
            return true;
        }
    } catch (const json::parse_error& e) {
        std::cerr << "    [Scanner] JSON Parse Error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "    [Scanner] Error checking RPC response: " << e.what() << std::endl;
        return false;
    }
    return false;
}

/**
 * @brief Core logic to scan a specific connection type for a given endpoint entry.
 * This function performs the network request, gathers metrics, and updates the status
 * in the referenced endpoint structure.
 * @param endpoint_ref A reference to the endpoint structure in the config to update.
 * @param connection_type_to_scan The specific type ("https", "wss", etc.) to scan.
 * @param url_to_scan The URL corresponding to the connection type.
 */
void perform_scan_for_type(
                           neozork::config_manager::struct_endpoint& endpoint_ref,
                           const std::string& connection_type_to_scan,
                           const std::string& url_to_scan)
{
    std::cout << "    [Scanner] Testing URL: " << url_to_scan << " (Type: " << connection_type_to_scan << ")" << std::endl;
    
    neozork::config_manager::struct_endpoint_connection_status new_status;
    new_status.last_check = format_iso8601_time(std::chrono::system_clock::now());
    new_status.is_active = false; // Default to inactive
    
    // --- Select Test Method based on Type ---
    // For now, only HTTPS is fully implemented for scanning via JSON RPC
    if (connection_type_to_scan == "https") {
        // Use eth_chainId as a simple, standard, non-state-changing test call
        std::string rpc_method = "eth_chainId";
        json rpc_params = json::array(); // eth_chainId usually takes no parameters
        
        try {
            auto result = neozork::connection_manager::send_json_rpc_request(url_to_scan, rpc_method, rpc_params);
            
            // Populate status fields from the result
            new_status.latency_ms = result.latency_ms; // Will be nullopt if connection failed early
            new_status.traffic_out_bytes = result.request_size_bytes;
            new_status.traffic_in_bytes = result.response_size_bytes;
            new_status.rpc_response_size_bytes = result.response_size_bytes; // Can be the same or just body size
            
            if (result.status_code.has_value() && result.status_code.value() == 200) {
                // Check if the body contains a valid successful JSON RPC response
                if (is_successful_rpc_response(result.body)) {
                    new_status.is_active = true;
                    std::cout << "      -> Result: Active"
                    << (new_status.latency_ms ? ", Latency: " + std::to_string(*new_status.latency_ms) + " ms" : "")
                    << std::endl;
                    
                    // TODO (Experimental): Parse rate limit headers if present and config fields are empty
                    // if (result.response_headers) { parse_and_update_rate_limits(endpoint_ref, *result.response_headers); }
                    
                } else {
                    std::cerr << "      -> Result: Inactive (HTTP 200 but invalid/error RPC response)" << std::endl;
                    // Optionally log result.body if !is_successful_rpc_response(...)
                }
            } else {
                std::cerr << "      -> Result: Inactive (HTTP Status: "
                << (result.status_code.has_value() ? std::to_string(*result.status_code) : "N/A")
                << (result.error_message ? ", Error: " + *result.error_message : "")
                << ")" << std::endl;
            }
            
        } catch (const std::invalid_argument& e) {
            std::cerr << "      -> Result: Inactive (URL Parse Error: " << e.what() << ")" << std::endl;
            new_status.latency_ms = std::nullopt; // Ensure latency is null on this type of error
        } catch (const std::exception& e) {
            std::cerr << "      -> Result: Inactive (Exception during request: " << e.what() << ")" << std::endl;
            new_status.latency_ms = std::nullopt;
        }
        
    } else if (connection_type_to_scan == "wss") {
        // TODO: Implement WebSocket scanning logic
        std::cout << "      -> Result: Scan for 'wss' not implemented yet." << std::endl;
        // Placeholder status: inactive, no latency etc.
        new_status.is_active = false;
        
    } else if (connection_type_to_scan == "http") {
        // TODO: Implement HTTP (non-SSL) scanning logic (similar to https but using httplib::Client)
        std::cout << "      -> Result: Scan for 'http' not implemented yet." << std::endl;
        new_status.is_active = false;
        
    } else if (connection_type_to_scan == "ws") {
        // TODO: Implement non-SSL WebSocket scanning logic
        std::cout << "      -> Result: Scan for 'ws' not implemented yet." << std::endl;
        new_status.is_active = false;
        
    } else if (connection_type_to_scan == "ipc") {
        // TODO: Implement IPC scanning logic (platform-specific)
        std::cout << "      -> Result: Scan for 'ipc' not implemented yet." << std::endl;
        new_status.is_active = false;
        
    } else {
        std::cerr << "      -> Result: Unknown connection type '" << connection_type_to_scan << "'. Cannot scan." << std::endl;
        // Keep status as inactive
    }
    
    // --- Update Config ---
    // Update the status for this specific connection type in the endpoint object
    neozork::config_manager::update_endpoint_status(endpoint_ref, connection_type_to_scan, new_status);
}


// --- Public Orchestrating Functions ---

void run_scan_endpoints(
                        neozork::config_manager::struct_config& config,
                        const std::string& blockchain_name_or_id,
                        const std::optional<std::string>& requested_connection_type)
{
    std::cout << "[Scanner] Starting scan for all endpoints on blockchain: '" << blockchain_name_or_id << "'" << std::endl;
    if (requested_connection_type) {
        std::cout << "[Scanner] Filtering for connection type: '" << *requested_connection_type << "'" << std::endl;
    }
    
    // 1. Find the blockchain
    auto bc_info_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
    if (!bc_info_ref_opt) {
        throw std::runtime_error("Scanner Error: Blockchain '" + blockchain_name_or_id + "' not found in configuration.");
    }
    neozork::config_manager::struct_blockchain_info& bc_info = bc_info_ref_opt.value().get();
    
    std::cout << "[Scanner] Found blockchain '" << bc_info.name << "'. Scanning " << bc_info.endpoints.size() << " configured endpoints..." << std::endl;
    
    // 2. Iterate through endpoints and scan each
    int endpoint_index = 0;
    for (auto& endpoint : bc_info.endpoints) { // Need non-const reference to modify
        endpoint_index++;
        std::cout << "  [Scanner] Processing Endpoint " << endpoint_index << "/" << bc_info.endpoints.size() << "..." << std::endl;
        
        // Determine which types to scan for *this* specific endpoint
        std::vector<std::pair<std::string, std::string>> types_to_scan_for_this_endpoint;
        if (requested_connection_type) {
            // User requested a specific type, check if this endpoint has it
            auto it = endpoint.connection_urls.find(*requested_connection_type);
            if (it != endpoint.connection_urls.end()) {
                types_to_scan_for_this_endpoint.push_back(*it); // Add {type, url} pair
            } else {
                std::cout << "    [Scanner] Skipping endpoint: Does not have requested type '" << *requested_connection_type << "'." << std::endl;
                continue; // Skip to next endpoint
            }
        } else {
            // No specific type requested, scan all types listed for this endpoint
            for (const auto& pair : endpoint.connection_urls) {
                types_to_scan_for_this_endpoint.push_back(pair);
            }
        }
        
        if (types_to_scan_for_this_endpoint.empty()) {
            std::cout << "    [Scanner] No connection types to scan for this endpoint based on filter." << std::endl;
            continue;
        }
        
        // Scan the selected types for the current endpoint
        for (const auto& type_url_pair : types_to_scan_for_this_endpoint) {
            perform_scan_for_type(endpoint, type_url_pair.first, type_url_pair.second);
        }
    } // end loop over endpoints
    
    std::cout << "[Scanner] Finished scanning endpoints for blockchain '" << bc_info.name << "'." << std::endl;
}


void run_scan_single_endpoint(
                              neozork::config_manager::struct_config& config,
                              const std::string& blockchain_name_or_id,
                              const std::string& endpoint_url_to_find,
                              const std::optional<std::string>& requested_connection_type)
{
    std::cout << "[Scanner] Starting scan for single endpoint URL: '" << endpoint_url_to_find << "' on blockchain: '" << blockchain_name_or_id << "'" << std::endl;
    if (requested_connection_type) {
        std::cout << "[Scanner] Filtering for connection type: '" << *requested_connection_type << "'" << std::endl;
    }
    
    // 1. Find the blockchain
    auto bc_info_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
    if (!bc_info_ref_opt) {
        throw std::runtime_error("Scanner Error: Blockchain '" + blockchain_name_or_id + "' not found in configuration.");
    }
    neozork::config_manager::struct_blockchain_info& bc_info = bc_info_ref_opt.value().get();
    
    // 2. Find the specific endpoint by URL
    // Using find_endpoint_by_any_url (assuming this function exists/will be added in config_manager)
    // If find_endpoint_by_any_url is not available, you might need to iterate manually:
    std::optional<std::reference_wrapper<neozork::config_manager::struct_endpoint>> target_endpoint_opt = std::nullopt;
    for (auto& ep : bc_info.endpoints) {
        for (const auto& pair : ep.connection_urls) {
            if (pair.second == endpoint_url_to_find) {
                target_endpoint_opt = std::ref(ep);
                break;
            }
        }
        if (target_endpoint_opt) break;
    }
    
    // Alternatively, assuming find_endpoint_by_any_url exists:
    // auto target_endpoint_opt = neozork::config_manager::find_endpoint_by_any_url(bc_info, endpoint_url_to_find);
    
    if (!target_endpoint_opt) {
        throw std::runtime_error("Scanner Error: Endpoint with URL '" + endpoint_url_to_find + "' not found in configuration for blockchain '" + bc_info.name + "'.");
    }
    neozork::config_manager::struct_endpoint& endpoint = target_endpoint_opt.value().get();
    
    
    // 3. Determine which types to scan and perform scan
    std::cout << "  [Scanner] Found endpoint entry. Scanning..." << std::endl;
    
    std::vector<std::pair<std::string, std::string>> types_to_scan_for_this_endpoint;
    if (requested_connection_type) {
        auto it = endpoint.connection_urls.find(*requested_connection_type);
        if (it != endpoint.connection_urls.end()) {
            // Check if the found URL matches the one provided (or just scan the type if found)
            if (it->second == endpoint_url_to_find || endpoint.connection_urls.size() == 1) { // If only one URL or URL matches target
                types_to_scan_for_this_endpoint.push_back(*it);
            } else {
                // This case is tricky: user specified URL X and type T, but type T maps to URL Y in the config.
                // Scan type T using URL Y, but maybe warn the user?
                std::cerr << "    [Scanner] Warning: Requested type '" << *requested_connection_type
                << "' for URL '" << endpoint_url_to_find << "' corresponds to URL '" << it->second
                << "' in config. Scanning using '" << it->second << "'." << std::endl;
                types_to_scan_for_this_endpoint.push_back(*it);
            }
        } else {
            throw std::runtime_error("Scanner Error: Requested connection type '" + *requested_connection_type + "' not configured for the found endpoint entry.");
        }
    } else {
        // Scan all types for this endpoint
        for (const auto& pair : endpoint.connection_urls) {
            types_to_scan_for_this_endpoint.push_back(pair);
        }
    }
    
    if (types_to_scan_for_this_endpoint.empty()) {
        std::cout << "    [Scanner] No connection types to scan for this endpoint based on filter." << std::endl;
        // This shouldn't happen based on logic above, but check anyway.
    } else {
        // Scan the selected types
        for (const auto& type_url_pair : types_to_scan_for_this_endpoint) {
            perform_scan_for_type(endpoint, type_url_pair.first, type_url_pair.second);
        }
    }
    
    std::cout << "[Scanner] Finished scanning single endpoint URL '" << endpoint_url_to_find << "'." << std::endl;
}


} // namespace neozork::endpoint_scanner
