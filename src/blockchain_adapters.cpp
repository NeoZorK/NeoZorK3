// src/blockchain_adapters.cpp

#include "blockchain_adapters.h"
#include "config_manager.h"
#include "connection_manager.h" // To make RPC calls
#include "ui.h"
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

// --- Helper function get_latest_block_number (без изменений) ---
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
                    std::string hex_block_num = response_json["result"].get<std::string>();
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
    } catch (const std::exception& e) {
        std::cerr << "      [Adapter] Exception during get_latest_block_number request to " << endpoint_url << ": " << e.what() << std::endl;
    }
    return std::nullopt; // Return empty optional on any failure
}


// --- Implementation for measure_block_speed  ---
std::optional<double> measure_block_speed(
    neozork::config_manager::struct_config& config,
    const std::string& blockchain_name_or_id)
{
    auto overall_start_time = std::chrono::high_resolution_clock::now();
    std::cout << "[Adapter] Measuring block speed for blockchain: '" << blockchain_name_or_id << "'" << std::endl;

    // 1. Find the blockchain
    auto bc_info_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
    if (!bc_info_ref_opt) {
        throw std::runtime_error("Adapter Error: Blockchain '" + blockchain_name_or_id + "' not found in configuration.");
    }
    // Get a reference to the blockchain
    neozork::config_manager::struct_blockchain_info& bc_info = bc_info_ref_opt.value().get();

    // 2. Get the list of potentially active endpoints (const links)
    auto active_endpoints = neozork::config_manager::get_active_endpoints(bc_info, "https");
    if (active_endpoints.empty()) {
         std::cout << "[Adapter] No active HTTPS endpoints found. Checking for other active types..." << std::endl;
         active_endpoints = neozork::config_manager::get_active_endpoints(bc_info, "any");
    }

    if (active_endpoints.empty()) {
         std::cerr << "[Adapter] Error: No active endpoints found for blockchain '" << bc_info.name << "' to measure block speed." << std::endl;
         auto overall_end_time = std::chrono::high_resolution_clock::now();
         auto overall_duration = std::chrono::duration_cast<std::chrono::milliseconds>(overall_end_time - overall_start_time).count();
         std::cout << "[Adapter] Total time spent on measurement attempt: " << overall_duration << " ms." << std::endl;
         return std::nullopt;
    }

    std::cout << "[Adapter] Found " << active_endpoints.size() << " potentially suitable active endpoint(s). Attempting measurement..." << std::endl;

    // 3. Iterate through active endpoints and attempt measurement
    for (size_t idx = 0; idx < active_endpoints.size(); ++idx) {
        // use a const reference to the endpoint
        const auto& endpoint_const_ref = active_endpoints[idx].get();
        std::string endpoint_url_to_use;
        std::string endpoint_type_to_use;

        // --- Find a suitable URL (prioritize https) ---
        auto https_it = endpoint_const_ref.connection_urls.find("https");
        if (https_it != endpoint_const_ref.connection_urls.end()) {
            endpoint_url_to_use = https_it->second;
            endpoint_type_to_use = "https";
        } else {
             std::cerr << "[Adapter] Skipping endpoint " << (idx + 1) << "/" << active_endpoints.size()
                       << " (no HTTPS URL found, other types not supported yet for measurement)." << std::endl;
             continue;
        }

        std::cout << "[Adapter] Attempting measurement using endpoint " << (idx + 1) << "/" << active_endpoints.size()
                  << " (Type: " << endpoint_type_to_use << ", URL: " << endpoint_url_to_use << ")" << std::endl;

        // --- Perform Sampling ---
        const int num_samples = 11;
        const std::chrono::seconds delay_between_samples(1);
        std::vector<std::pair<std::chrono::system_clock::time_point, long long>> block_samples;
        long long last_block = -1;
        bool sample_fetch_failed = false;

        std::cout << "  [Adapter] Gathering " << num_samples << " block number samples..." << std::endl;
        for (int i = 0; i < num_samples; ++i) {

             auto fetch_time = std::chrono::system_clock::now();
             std::optional<long long> current_block_opt = get_latest_block_number(endpoint_url_to_use);

             if (current_block_opt) {
                  long long current_block = *current_block_opt;
                  std::cout << "    Sample " << std::setw(2) << (i + 1) << "/" << num_samples << ": Block " << current_block << std::endl;
                  if (current_block > last_block || last_block == -1) {
                      block_samples.push_back({fetch_time, current_block});
                      last_block = current_block;
                  } else {
                       std::cout << "    (Block number unchanged)" << std::endl;
                  }
             } else {
                 std::cerr << "    [Adapter] Warning: Failed to fetch block number sample " << (i + 1) << " using " << endpoint_url_to_use << ". Stopping sampling for this endpoint." << std::endl;
                 sample_fetch_failed = true;
                 break;
             }

             if (i < num_samples - 1) {
                 std::this_thread::sleep_for(delay_between_samples);
             }
        }

        if (sample_fetch_failed) {
             std::cerr << "[Adapter] Failed to gather sufficient samples using " << endpoint_url_to_use << ". Trying next endpoint if available." << std::endl;
             continue;
        }
        if (block_samples.size() < 2) {
             std::cerr << "[Adapter] Error: Not enough valid block samples gathered (< 2) using " << endpoint_url_to_use << ". Trying next endpoint if available." << std::endl;
             continue;
        }

        // --- Calculate block time ---
        std::vector<double> block_time_intervals_ms;
        long long total_blocks_diff = 0;
        for (size_t i = 1; i < block_samples.size(); ++i) {
            long long block_diff = block_samples[i].second - block_samples[i-1].second;
            if (block_diff <= 0) continue;
            auto time_diff = block_samples[i].first - block_samples[i-1].first;
            double time_diff_ms = std::chrono::duration_cast<std::chrono::microseconds>(time_diff).count() / 1000.0;
            if (time_diff_ms > 0) {
                double ms_per_block = time_diff_ms / static_cast<double>(block_diff);
                block_time_intervals_ms.push_back(ms_per_block);
                total_blocks_diff += block_diff;
            }
        }
        if (block_time_intervals_ms.empty() || total_blocks_diff == 0) {
            std::cerr << "[Adapter] Error: No valid time intervals calculated using " << endpoint_url_to_use << ". Trying next endpoint if available." << std::endl;
            continue;
        }
        double total_time_valid_intervals_ms = 0;
         for (size_t i = 1; i < block_samples.size(); ++i) {
             if (block_samples[i].second - block_samples[i-1].second > 0) {
                  auto time_diff = block_samples[i].first - block_samples[i-1].first;
                  total_time_valid_intervals_ms += std::chrono::duration_cast<std::chrono::microseconds>(time_diff).count() / 1000.0;
             }
         }
        double average_block_time_ms = total_time_valid_intervals_ms / static_cast<double>(total_blocks_diff);


        // --- Successful Measurement ---
        std::cout << "[Adapter] Measurement successful using " << endpoint_url_to_use << std::endl;
        std::cout << "[Adapter] Calculated average block time: " << average_block_time_ms << " ms (" << block_time_intervals_ms.size() << " intervals, " << total_blocks_diff << " blocks total)" << std::endl;

        // --- Update Config ---
        if (neozork::config_manager::update_blockchain_block_speed(bc_info, average_block_time_ms)) {
            std::cout << "[Adapter] Updated blockchain block speed in configuration." << std::endl;
        } else {
             std::cerr << "[Adapter] Warning: Failed to update block speed in configuration object." << std::endl;
        }

        // --- >>> Update Endpoint Config <<< ---
        long long latest_block_measured = block_samples.back().second;

        // Find mutable reference to endpoint
        auto mutable_endpoint_ref_opt = neozork::config_manager::find_endpoint_by_any_url(bc_info, endpoint_url_to_use);

        if (mutable_endpoint_ref_opt) {
            // Update last block number
            if (neozork::config_manager::update_endpoint_block_number(mutable_endpoint_ref_opt.value().get(), latest_block_measured)) {
                std::cout << "[Adapter] Updated last known block (" << latest_block_measured << ") for endpoint " << endpoint_url_to_use << " in configuration." << std::endl;
            } else {
                std::cerr << "[Adapter] Warning: Failed to update last known block for endpoint " << endpoint_url_to_use << "." << std::endl;
            }
        } else {
            std::cerr << "[Adapter] Internal Error: Could not find mutable reference for endpoint " << endpoint_url_to_use << " to update last block number." << std::endl;
        }
        // --- >>>END OF Endpoint Config Update <<< ---


        // --- Successful Measurement ---
        auto overall_end_time = std::chrono::high_resolution_clock::now();
        auto overall_duration = std::chrono::duration_cast<std::chrono::milliseconds>(overall_end_time - overall_start_time).count();
        std::cout << "[Adapter] Total time spent on measurement attempt: " << overall_duration << " ms." << std::endl;

        return average_block_time_ms;

    } // --- END OF MEASUREMENT LOOP ---

    // If we get here, measurement failed
    std::cerr << "[Adapter] Error: Failed to measure block speed using all available active endpoints for blockchain '" << bc_info.name << "'." << std::endl;
    auto overall_end_time = std::chrono::high_resolution_clock::now();
    auto overall_duration = std::chrono::duration_cast<std::chrono::milliseconds>(overall_end_time - overall_start_time).count();
    std::cout << "[Adapter] Total time spent on measurement attempt: " << overall_duration << " ms." << std::endl;
    return std::nullopt;
}

} // namespace neozork::blockchain_adapters
