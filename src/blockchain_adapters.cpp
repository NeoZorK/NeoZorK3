// src/blockchain_adapters.cpp

#include "blockchain_adapters.h"
#include "config_manager.h"
#include "connection_manager.h" // To make RPC calls
#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <chrono>       // For timing and sleep
#include <thread>       // For std::this_thread::sleep_for
#include <numeric>      // For std::accumulate
#include <limits>       // For numeric limits
#include <nlohmann/json.hpp> // For RPC interactions

using json = nlohmann::json;

namespace neozork::blockchain_adapters {

// --- Helper function to get latest block number ---
std::optional<long long> get_latest_block_number(const std::string& endpoint_url) {
    std::cout << "      [Adapter] Fetching latest block number from: " << endpoint_url << std::endl;
    std::string method = "eth_blockNumber";
    json params = json::array(); // No params for eth_blockNumber
    
    try {
        auto result = neozork::connection_manager::send_json_rpc_request(endpoint_url, method, params);
        
        if (result.status_code.has_value() && result.status_code.value() == 200 && result.body) {
            try {
                json response_json = json::parse(*result.body);
                if (response_json.contains("result") && !response_json.contains("error")) {
                    // Result is typically a hex string like "0x..."
                    std::string hex_block_num = response_json["result"].get<std::string>();
                    // Convert hex string (with "0x" prefix) to long long
                    return std::stoll(hex_block_num, nullptr, 16);
                } else if (response_json.contains("error")) {
                    std::cerr << "      [Adapter] RPC Error from " << endpoint_url << ": " << response_json["error"].dump() << std::endl;
                } else {
                    std::cerr << "      [Adapter] Invalid RPC response structure from " << endpoint_url << ": " << *result.body << std::endl;
                }
            } catch (const json::parse_error& e) {
                std::cerr << "      [Adapter] Failed to parse JSON RPC response from " << endpoint_url << ": " << e.what() << std::endl;
            } catch (const json::type_error& e) {
                std::cerr << "      [Adapter] JSON type error in response from " << endpoint_url << ": " << e.what() << std::endl;
            } catch (const std::invalid_argument& e) {
                std::cerr << "      [Adapter] Failed to convert block number hex to int from " << endpoint_url << ": " << e.what() << std::endl;
            } catch (const std::out_of_range& e) {
                std::cerr << "      [Adapter] Block number out of range from " << endpoint_url << ": " << e.what() << std::endl;
            }
        } else {
            std::cerr << "      [Adapter] Failed to fetch block number from " << endpoint_url
            << " (HTTP Status: " << (result.status_code.has_value() ? std::to_string(*result.status_code) : "N/A")
            << (result.error_message ? ", Error: " + *result.error_message : "") << ")" << std::endl;
        }
    } catch (const std::exception& e) { // Catch potential errors from send_json_rpc_request itself (e.g., URL parsing)
        std::cerr << "      [Adapter] Exception during get_latest_block_number request to " << endpoint_url << ": " << e.what() << std::endl;
    }
    return std::nullopt; // Return empty optional on any failure
}


// --- Implementation for measure_block_speed ---
std::optional<double> measure_block_speed(
                                          neozork::config_manager::struct_config& config,
                                          const std::string& blockchain_name_or_id)
{
    std::cout << "[Adapter] Measuring block speed for blockchain: '" << blockchain_name_or_id << "'" << std::endl;
    
    // 1. Find the blockchain
    auto bc_info_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
    if (!bc_info_ref_opt) {
        throw std::runtime_error("Adapter Error: Blockchain '" + blockchain_name_or_id + "' not found in configuration.");
    }
    neozork::config_manager::struct_blockchain_info& bc_info = bc_info_ref_opt.value().get();
    
    // 2. Find a suitable active endpoint (prefer https)
    std::string active_endpoint_url;
    // Use get_active_endpoints to find suitable ones, then pick the first https if possible
    auto active_endpoints = neozork::config_manager::get_active_endpoints(bc_info, "https"); // Prioritize https
    
    if (active_endpoints.empty()) {
        // Try finding active endpoints of *any* type if https failed
        active_endpoints = neozork::config_manager::get_active_endpoints(bc_info, "any"); // Fallback
    }
    
    if (active_endpoints.empty()) {
        throw std::runtime_error("Adapter Error: No active endpoints found for blockchain '" + bc_info.name + "' to measure block speed.");
    }
    
    // Select the first active endpoint found. Check if it has an https URL first.
    const auto& chosen_endpoint_ref = active_endpoints[0].get();
    auto https_it = chosen_endpoint_ref.connection_urls.find("https");
    if (https_it != chosen_endpoint_ref.connection_urls.end()) {
        active_endpoint_url = https_it->second;
        std::cout << "[Adapter] Using active HTTPS endpoint: " << active_endpoint_url << std::endl;
    } else {
        // Fallback: use the first URL available for the active endpoint (might not be https!)
        // This might fail if get_latest_block_number only supports https currently.
        if (chosen_endpoint_ref.connection_urls.empty()) {
            throw std::runtime_error("Adapter Error: Chosen active endpoint has no URLs listed.");
        }
        active_endpoint_url = chosen_endpoint_ref.connection_urls.begin()->second;
        std::string chosen_type = chosen_endpoint_ref.connection_urls.begin()->first;
        std::cerr << "[Adapter] Warning: No active HTTPS endpoint found. Using first available active endpoint type '"
        << chosen_type << "': " << active_endpoint_url
        << ". Measurement might fail if type is not HTTPS." << std::endl;
        // Need to enhance get_latest_block_number to handle other types later.
        if (chosen_type != "https") {
            std::cerr << "[Adapter] Error: Current implementation only supports HTTPS for block number fetching." << std::endl;
            return std::nullopt; // Cannot proceed yet
        }
    }
    
    
    // 3. Fetch block numbers multiple times
    const int num_samples = 6; // Get N samples to calculate N-1 intervals
    const std::chrono::seconds delay_between_samples(1); // Check every second
    std::vector<std::pair<std::chrono::system_clock::time_point, long long>> block_samples;
    
    std::cout << "[Adapter] Gathering block number samples..." << std::endl;
    long long last_block = -1;
    for (int i = 0; i < num_samples; ++i) {
        auto fetch_time = std::chrono::system_clock::now();
        std::optional<long long> current_block_opt = get_latest_block_number(active_endpoint_url);
        
        if (current_block_opt) {
            long long current_block = *current_block_opt;
            std::cout << "  Sample " << (i + 1) << "/" << num_samples << ": Block " << current_block << std::endl;
            // Only add sample if block number actually changed or it's the first sample
            if (current_block > last_block || last_block == -1) {
                block_samples.push_back({fetch_time, current_block});
                last_block = current_block;
            } else {
                std::cout << "  (Block number unchanged, skipping sample for interval calculation)" << std::endl;
            }
        } else {
            std::cerr << "[Adapter] Warning: Failed to fetch block number sample " << (i + 1) << ". Skipping." << std::endl;
            // Continue trying for next samples
        }
        
        // Wait before next sample, unless it's the last one
        if (i < num_samples - 1) {
            std::this_thread::sleep_for(delay_between_samples);
        }
    }
    
    // 4. Calculate average block time from valid intervals
    if (block_samples.size() < 2) {
        std::cerr << "[Adapter] Error: Not enough valid block samples gathered (< 2) to calculate block time." << std::endl;
        return std::nullopt;
    }
    
    std::vector<double> block_time_intervals_ms;
    long long total_blocks_diff = 0;
    for (size_t i = 1; i < block_samples.size(); ++i) {
        long long block_diff = block_samples[i].second - block_samples[i-1].second;
        // Ensure block difference is positive (sanity check)
        if (block_diff <= 0) {
            std::cerr << "[Adapter] Warning: Non-positive block difference observed (" << block_diff << "). Skipping interval." << std::endl;
            continue;
        }
        
        auto time_diff = block_samples[i].first - block_samples[i-1].first;
        // Convert time difference to milliseconds
        double time_diff_ms = std::chrono::duration_cast<std::chrono::microseconds>(time_diff).count() / 1000.0;
        
        if (time_diff_ms > 0) {
            // Calculate time per block for this interval
            double ms_per_block = time_diff_ms / static_cast<double>(block_diff);
            block_time_intervals_ms.push_back(ms_per_block);
            total_blocks_diff += block_diff;
            std::cout << "  Interval " << i << ": " << block_diff << " blocks in " << time_diff_ms << " ms (" << ms_per_block << " ms/block)" << std::endl;
        } else {
            std::cerr << "[Adapter] Warning: Zero or negative time difference observed. Skipping interval." << std::endl;
        }
    }
    
    if (block_time_intervals_ms.empty()) {
        std::cerr << "[Adapter] Error: No valid time intervals calculated." << std::endl;
        return std::nullopt;
    }
    
    // Calculate the average time per block across all valid intervals
    double total_time_sum_ms = std::accumulate(block_time_intervals_ms.begin(), block_time_intervals_ms.end(), 0.0);
    // Simple average:
    // double average_block_time_ms = total_time_sum_ms / block_time_intervals_ms.size();
    
    // Weighted average (more accurate if intervals cover different numbers of blocks):
    // Calculate total time covered by valid intervals
    double total_time_valid_intervals_ms = 0;
    for (size_t i = 1; i < block_samples.size(); ++i) {
        if (block_samples[i].second - block_samples[i-1].second > 0) {
            auto time_diff = block_samples[i].first - block_samples[i-1].first;
            total_time_valid_intervals_ms += std::chrono::duration_cast<std::chrono::microseconds>(time_diff).count() / 1000.0;
        }
    }
    double average_block_time_ms = total_time_valid_intervals_ms / static_cast<double>(total_blocks_diff);
    
    
    std::cout << "[Adapter] Calculated average block time: " << average_block_time_ms << " ms" << std::endl;
    
    // 5. Update config
    if (neozork::config_manager::update_blockchain_block_speed(bc_info, average_block_time_ms)) {
        std::cout << "[Adapter] Updated block speed in configuration." << std::endl;
    } else {
        // This shouldn't fail based on current implementation, but good to check
        std::cerr << "[Adapter] Warning: Failed to update block speed in configuration object." << std::endl;
    }
    
    return average_block_time_ms; // Return the calculated value
}


} // namespace neozork::blockchain_adapters
