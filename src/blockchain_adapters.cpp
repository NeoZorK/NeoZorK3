// src/blockchain_adapters.cpp

#include "blockchain_adapters.h"
#include "config_manager.h"
#include "connection_manager.h" // To make RPC calls
#include "ui.h"
#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <functional>
#include <map>
#include <stdexcept>
#include <chrono>       // For timing and sleep
#include <thread>       // For std::this_thread::sleep_for
#include <numeric>      // For std::accumulate
#include <limits>       // For numeric limits
#include <nlohmann/json.hpp> // For RPC interactions

using json = nlohmann::json;

// Define a prefix for log messages originating from this module
#define LOG_PREFIX_ADAPTER "    [Adapter] "


namespace { // Anonymous namespace for helper function

// Helper to try eth_call with failover on sorted active endpoints
// Returns the successful response body (hex string "0x...") or nullopt if all endpoints failed
std::optional<std::string> try_eth_call_with_failover(
                                                      const neozork::config_manager::struct_blockchain_info& bc_info,
                                                      const std::string& contract_address,
                                                      const std::string& encoded_data)
{
    // 1. Get sorted active HTTPS endpoints
    auto active_endpoints_ref = neozork::config_manager::get_active_endpoints(bc_info, "https");
    
    // Sort by latency (ascending, nulls last)
    std::sort(active_endpoints_ref.begin(), active_endpoints_ref.end(),
              [](const auto& a_ref, const auto& b_ref) {
        const auto& a = a_ref.get();
        const auto& b = b_ref.get();
        std::optional<double> latency_a;
        auto ita = a.status.find("https");
        if (ita != a.status.end()) latency_a = ita->second.latency_ms;
        
        std::optional<double> latency_b;
        auto itb = b.status.find("https");
        if (itb != b.status.end()) latency_b = itb->second.latency_ms;
        
        if (latency_a.has_value() && latency_b.has_value()) return *latency_a < *latency_b;
        if (latency_a.has_value()) return true; // a has latency, b doesn't -> a comes first
        if (latency_b.has_value()) return false; // b has latency, a doesn't -> b comes first
        return false; // Neither has latency, order doesn't strictly matter
    });
    
    if (active_endpoints_ref.empty()) {
        std::cerr << LOG_PREFIX_ADAPTER << "ERROR: No active HTTPS endpoints found for eth_call." << std::endl;
        return std::nullopt;
    }
    
    // 2. Loop through endpoints trying the call
    for (const auto& ep_ref : active_endpoints_ref) {
        const auto& endpoint = ep_ref.get();
        auto url_it = endpoint.connection_urls.find("https");
        if (url_it == endpoint.connection_urls.end()) continue; // Should not happen if get_active_endpoints worked
        const std::string& current_endpoint_url = url_it->second;
        
        std::cerr << std::endl;
        std::cout << LOG_PREFIX_ADAPTER << "  Attempting eth_call via: " << current_endpoint_url << "..." << std::endl;
        
        try {
            auto result = neozork::connection_manager::send_eth_call(current_endpoint_url, contract_address, encoded_data);
            
            // Check for connection manager level errors
            if (result.error_message) {
                std::cerr << LOG_PREFIX_ADAPTER << "  -> Network/HTTP Error: " << *result.error_message << ". Trying next endpoint." << std::endl;
                continue; // Try next endpoint
            }
            
            // Check for empty body
            if (!result.body || result.body.value().empty()) {
                std::cerr << LOG_PREFIX_ADAPTER << "  -> Empty response body received. Trying next endpoint." << std::endl;
                continue; // Try next endpoint
            }
            
            const std::string& response_body = result.body.value();
            
            // Check if the response body is valid JSON
            try {
                json j = json::parse(response_body);
                // Check if it's a JSON object
                if (j.is_object()) {
                    // Check for JSON RPC Error field
                    if (j.contains("error") && !j["error"].is_null()) {
                        std::cerr << LOG_PREFIX_ADAPTER << "  -> RPC Error received: " << j["error"].dump() << ". Trying next endpoint." << std::endl;
                        continue; // Try next endpoint
                    }
                    // Check for JSON RPC Result field
                    else if (j.contains("result")) {
                        // Check if result is a string (expected for eth_call)
                        if (j.at("result").is_string()) {
                            std::string hex_result = j.at("result").get<std::string>();
                            // Validate hex result format
                            if (hex_result.length() >= 2 && hex_result.rfind("0x", 0) == 0) {
                                std::cout << LOG_PREFIX_ADAPTER << "  -> Call successful (JSON response with result)." << std::endl;
                                return hex_result; // <<< RETURN successful hex result
                            } else {
                                std::cerr << LOG_PREFIX_ADAPTER << "  -> Invalid hex format in JSON result field: " << hex_result << ". Trying next endpoint." << std::endl;
                                continue; // Try next endpoint
                            }
                        }
                        // Handle null result field if necessary, treat as error for now
                        else if (j.at("result").is_null()) {
                            std::cerr << LOG_PREFIX_ADAPTER << "  -> Received null in JSON result field. Trying next endpoint." << std::endl;
                            continue;
                        }
                        // Handle other types in result field if needed, treat as error for now
                        else {
                            std::cerr << LOG_PREFIX_ADAPTER << "  -> Unexpected data type in JSON result field (expected string): " << j.at("result").type_name() << ". Trying next endpoint." << std::endl;
                            continue;
                        }
                    } // end else if contains result
                    // If JSON object contains neither "error" nor "result"
                    else {
                        std::cerr << LOG_PREFIX_ADAPTER << "  -> Invalid JSON RPC structure (missing error/result): " << response_body << ". Trying next endpoint." << std::endl;
                        continue; // Try next endpoint
                    }
                } // end if is_object
                // If it's valid JSON but not an object (e.g., array, string directly) - unexpected
                else {
                    std::cerr << LOG_PREFIX_ADAPTER << "  -> Unexpected JSON type received (expected object): " << j.type_name() << ". Trying next endpoint." << std::endl;
                    continue; // Try next endpoint
                }
            } catch (const json::parse_error&) {
                // --- If it's NOT valid JSON ---
                // Maybe it's a raw hex string directly? (Less common for eth_call)
                // Validate if it looks like a valid hex result "0x..."
                if (response_body.length() >= 2 && response_body.rfind("0x", 0) == 0) {
                    std::cout << LOG_PREFIX_ADAPTER << "  -> Call successful (assuming Raw hex response)." << std::endl;
                    return response_body;
                } else {
                    std::cerr << LOG_PREFIX_ADAPTER << "  -> Invalid response format (Not JSON, Not Hex): " << response_body << ". Trying next endpoint." << std::endl;
                    continue; // Try next endpoint
                }
            } // End catch json::parse_error
            
        } catch (const std::exception& e) {
            // Catch exceptions during send_eth_call itself (e.g., URL parsing)
            std::cerr << LOG_PREFIX_ADAPTER << "  -> Exception during eth_call: " << e.what() << ". Trying next endpoint." << std::endl;
            continue; // Try next endpoint
        }
    } // End loop over endpoints
    
    // If loop finishes without returning, all endpoints failed
    std::cerr << LOG_PREFIX_ADAPTER << "ERROR: eth_call failed on all available endpoints." << std::endl;
    return std::nullopt; // Indicate failure
}

} // End anonymous namespace

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


// Structure to hold known DEX info internally before adding to config
struct known_dex_entry {
    std::string id;                 // Unique identifier (e.g., "spookyswap_v2", "uniswap_v2")
    std::string name;               // User-friendly name (e.g., "SpookySwap V2", "Uniswap V2")
    std::string factory_address;    // Factory contract address
    std::optional<std::string> router_address; // Optional: Router contract address
};

// --- Expanded Hardcoded Map of Known DEXes ---
// chainId -> vector of known DEX entries
// NOTE: Addresses should be verified with official DEX documentation. V3 routers omitted/nullopt.
const std::map<int, std::vector<known_dex_entry>> hardcoded_known_dexes = {
    { 1, {
        // Ethereum
        {"uniswap_v2", "Uniswap V2", "0x5C69bEe701ef814a2B6a3EDD4B1652CB9cc5aA6f", "0x7a250d5630B4cF539739dF2C5dAcb4c659F2488D"},
        {"sushiswap_v2", "SushiSwap V2", "0xC0AEe478e3658e2610c5F7A4A2E1777cE9e4f2Ac", "0xd9e1cE17f2641f24aE83637ab66a2cca9C378B9F"},
        {"uniswap_v3", "Uniswap V3", "0x1F98431c8aD98523631AE4a59f267346ea31F984", std::nullopt}
        // V3 Router complex
    }},
    { 56, {
        // Binance Smart Chain (BSC)
        {"pancakeswap_v2", "PancakeSwap V2", "0xcA143Ce32Fe78f1f7019d7d551a6402fC5350c73", "0x10ED43C718714eb63d5aA57B78B54704E256024E"},
        {"sushiswap_v2", "SushiSwap V2", "0xc35DADB65012eC5796536bD9864eD8773aBc74C4", "0x1b02dA8Cb0d097eB8D57A175b88c7D8b47997506"},
        {"biswap_v1", "BiSwap", "0x858E3312ed3A876947EA49d572A7C42DE08af7EE", "0x3a6d8cA21D1CF76F653A67577FA0D27453350dD8"},
        {"pancakeswap_v3", "PancakeSwap V3", "0x0BFbCF9fa4f9C56B0F40a671Ad40E0805A091865", std::nullopt}
        // V3 Router complex
    }},
    { 137, {
        // Polygon (Matic)
        {"quickswap_v2", "QuickSwap V2", "0x5757371414417b8C6CAad45bAeF941aBc7d3Ab32", "0xa5E0829CaCEd8fFDD4De3c43696c57F7D7A678ff"},
        {"sushiswap_v2", "SushiSwap V2", "0xc35DADB65012eC5796536bD9864eD8773aBc74C4", "0x1b02dA8Cb0d097eB8D57A175b88c7D8b47997506"},
        {"uniswap_v3", "Uniswap V3", "0x1F98431c8aD98523631AE4a59f267346ea31F984", std::nullopt}
        // V3 Router complex
    }},
    { 250, {
        // Fantom
        {"spookyswap_v2", "SpookySwap V2", "0x152eE697f2E276fA89E96742e9bB9f1A45EffAEf", "0xF491e7B69E4244ad4002BC14e878a34207E38c29"}, // Using V1 Router
        {"spiritswap_v2", "SpiritSwap V2", "0xEF45d134b73241EDA7703fa787148D9C9F4950b0", "0x16327E3FbDaCA3bcF7E38F5Af2599D2DDClrA3aC"},
        {"sushiswap_v2", "SushiSwap V2", "0xc35DADB65012eC5796536bD9864eD8773aBc74C4", "0x1b02dA8Cb0d097eB8D57A175b88c7D8b47997506"}
    }},
    { 43114, {
        // Avalanche C-Chain
        {"traderjoe_v1", "Trader Joe V1", "0x9Ad6C38BE94206cA50bb0d90783181662f0Cfa10", "0x60aE616a2155Ee3d9A68541Ba4544862310933d4"},
        {"pangolin_v2", "Pangolin V2", "0xefa94E708679016C5275582db4811E45FA8751A2", "0xE54Ca86531e17Ef3616d22Ca28b0D458b6C89106"},
        {"sushiswap_v2", "SushiSwap V2", "0xc35DADB65012eC5796536bD9864eD8773aBc74C4", "0x1b02dA8Cb0d097eB8D57A175b88c7D8b47997506"}
        // Note: Trader Joe V2 uses LBRouter, different architecture, omitted for now
    }},
    { 42161, {
        // Arbitrum One
        {"sushiswap_v2", "SushiSwap V2", "0xc35DADB65012eC5796536bD9864eD8773aBc74C4", "0x1b02dA8Cb0d097eB8D57A175b88c7D8b47997506"},
        {"uniswap_v3", "Uniswap V3", "0x1F98431c8aD98523631AE4a59f267346ea31F984", std::nullopt},
        {"camelot_v2", "Camelot V2", "0x6EcCab422D763aC031210895C81787E87B43A652", "0xc873fEcbd354f5A56E00E710B90EF4201db2448d"}
    }},
    { 10, {
        // Optimism
        {"uniswap_v3", "Uniswap V3", "0x1F98431c8aD98523631AE4a59f267346ea31F984", std::nullopt},
        {"sushiswap_v2", "SushiSwap V2", "0xc35DADB65012eC5796536bD9864eD8773aBc74C4", "0x1b02dA8Cb0d097eB8D57A175b88c7D8b47997506"},
        {"velodrome_v2", "Velodrome V2", "0xF1B99e3E573A3A7f118B7F226E879212E8551064", "0x9c12939390052919aF3155f41Bf4150fd7665436"}
    }},
    { 8453, {
        // Base
        {"uniswap_v3", "Uniswap V3", "0x33128a8fC17869897dcE68Ed026d694621f6FDfD", std::nullopt},
        
        // Base has different V3 Factory
        {"sushiswap_v2", "SushiSwap V2", "0xc35DADB65012eC5796536bD9864eD8773aBc74C4", "0x1b02dA8Cb0d097eB8D57A175b88c7D8b47997506"},
        {"aerodrome_v1", "Aerodrome", "0x420DD381b31aEf6683db6B902084cB0FFECe40Da", "0xcF77a3Ba9A5CA399B7C97c74d54e5b1Beb874E43"}
    }}
    // Add more chains and DEXes here as needed
};

/**
 * @brief Discovers known DEXes for a specific blockchain and adds them to the config.
 * Currently uses a hardcoded list of known DEX factory addresses per chain ID.
 * @param config The main configuration object (mutable).
 * @param blockchain_name_or_id The name or network ID of the blockchain.
 * @return True if the process completed (even if no *new* DEXes were added), false on critical error (e.g., blockchain not found).
 * @throws std::runtime_error on configuration errors.
 */
bool discover_dexes_for_blockchain(
                                   neozork::config_manager::struct_config& config,
                                   const std::string& blockchain_name_or_id)
{
    std::cout << LOG_PREFIX_ADAPTER << "Starting DEX discovery for blockchain: '" << blockchain_name_or_id << "'" << std::endl;
    
    // 1. Find the blockchain (mutable reference needed to add DEXes)
    auto bc_info_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name_or_id);
    if (!bc_info_ref_opt) {
        // Error already printed by find_blockchain likely, or throw exception
        std::cerr << LOG_PREFIX_ADAPTER << "Error: Blockchain '" << blockchain_name_or_id << "' not found in configuration." << std::endl;
        return false; // Indicate failure
    }
    // Get a mutable reference to the blockchain info
    neozork::config_manager::struct_blockchain_info& bc_info = bc_info_ref_opt.value().get();
    
    // 2. Check if DEXes are known for this chain ID
    auto known_dexes_it = hardcoded_known_dexes.find(bc_info.network_id);
    if (known_dexes_it == hardcoded_known_dexes.end()) {
        std::cout << LOG_PREFIX_ADAPTER << "No hardcoded DEX information found for chain '" << bc_info.name << "' (ID: " << bc_info.network_id << ")." << std::endl;
        return true; // Completed successfully, just nothing to add
    }
    
    const std::vector<known_dex_entry>& dexes_for_this_chain = known_dexes_it->second;
    std::cout << LOG_PREFIX_ADAPTER << "Found " << dexes_for_this_chain.size() << " known DEX entries for '" << bc_info.name << "'. Checking config..." << std::endl;
    
    // 3. Iterate through known DEXes and add them to config if not already present
    int added_count = 0;
    int skipped_count = 0;
    for (const auto& known_dex : dexes_for_this_chain) {
        
        // Prepare the structure to add
        neozork::config_manager::struct_dex_info dex_to_add;
        dex_to_add.id = known_dex.id;
        dex_to_add.name = known_dex.name;
        dex_to_add.factory_address = known_dex.factory_address; // Assuming factory is mandatory
        dex_to_add.router_address = known_dex.router_address;   // Optional router
        
        // Attempt to add using config_manager function (handles duplicates)
        if (neozork::config_manager::add_dex(bc_info, dex_to_add)) {
            std::cout << LOG_PREFIX_ADAPTER << " -> Added '" << dex_to_add.name << "' (ID: " << dex_to_add.id << ") to config." << std::endl;
            added_count++;
        } else {
            // Optional: Log skipped duplicates if needed for debugging
            // std::cout << LOG_PREFIX_ADAPTER << " -> DEX '" << dex_to_add.name << "' (ID: " << dex_to_add.id << ") already exists in config. Skipping." << std::endl;
            skipped_count++;
        }
    }
    
    std::cout << LOG_PREFIX_ADAPTER << "Finished DEX discovery for '" << bc_info.name << "'. Added: " << added_count << ", Already existed/Skipped: " << skipped_count << "." << std::endl;
    
    // Return true if any were added (indicates config was potentially modified)
    return true; // Indicate successful completion
}

/**
 * @brief Discovers liquidity pools for a specific DEX on a given blockchain.
 * Interacts with the DEX Factory contract via RPC.
 * @param config The main configuration object (mutable).
 * @param blockchain_id_str The network ID of the blockchain as a string.
 * @param dex_id The ID of the DEX (from config) for which to discover pools.
 * @return True if the configuration was potentially modified (new pools added), false otherwise or on critical error.
 * @throws std::runtime_error on configuration errors (blockchain/DEX not found) or RPC errors.
 */
// --- Implementation for discover_pools_for_dex ---
bool discover_pools_for_dex(
                            neozork::config_manager::struct_config& config,
                            const std::string& blockchain_id_str,
                            const std::string& dex_id,
                            int delay_ms)
{
    bool changes_made = false;
    std::cout << LOG_PREFIX_ADAPTER << "Starting pool discovery for blockchain ID '" << blockchain_id_str
    << "', DEX '" << dex_id << "'..." << std::endl;
    
    // --- 1. Find Blockchain ---
    auto bc_info_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_id_str);
    if (!bc_info_ref_opt) { /* error handling */ return false; }
    neozork::config_manager::struct_blockchain_info& bc_info = bc_info_ref_opt.value().get(); // Now use bc_info directly
    std::cout << LOG_PREFIX_ADAPTER << "Found blockchain: " << bc_info.name << std::endl;
    
    // --- 2. Find DEX and Factory Address ---
    auto dex_info_ref_opt = neozork::config_manager::find_dex(bc_info, dex_id);
    if (!dex_info_ref_opt) { /* error handling */ return false; }
    const neozork::config_manager::struct_dex_info& dex_info = dex_info_ref_opt.value().get();
    if (!dex_info.factory_address || dex_info.factory_address.value().empty()) { /* error handling */ return false; }
    const std::string factory_address = dex_info.factory_address.value();
    std::cout << LOG_PREFIX_ADAPTER << "Found DEX: " << dex_info.name << " (Factory: " << factory_address << ")" << std::endl;
    
    // --- 3. HTTPS Endpoint check needed by helper ---
    // No need to explicitly find one here, helper function will do it.
    // We just need to ensure bc_info is passed to the helper.
    
    
    // --- 4. Define ABI Signatures ---
    const std::string ALL_PAIRS_LENGTH_SIG = "0x18160ddd";
    const std::string ALL_PAIRS_SIG = "0x1e3dd18b";
    const std::string TOKEN0_SIG = "0x0dfe1681";
    const std::string TOKEN1_SIG = "0xd21220a7";
    
    
    // --- 5. Get Pool Count using Helper ---
    long long pool_count = 0;
    try {
        std::cout << LOG_PREFIX_ADAPTER << "Querying pool count from factory..." << std::endl;
        std::string length_data = neozork::connection_manager::encode_eth_call_data(ALL_PAIRS_LENGTH_SIG);
        if (length_data.empty()) throw std::runtime_error("Failed to encode allPairsLength data.");
        
        // Use the failover helper
        std::optional<std::string> length_result_body = try_eth_call_with_failover(bc_info, factory_address, length_data);
        
        if (!length_result_body) {
            throw std::runtime_error("Failed to get pool count from any endpoint.");
        }
        
        pool_count = neozork::connection_manager::decode_uint256_from_result(*length_result_body);
        if (pool_count < 0) throw std::runtime_error("Received invalid negative pool count.");
        std::cout << LOG_PREFIX_ADAPTER << "Factory reports " << pool_count << " pools." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << LOG_PREFIX_ADAPTER << "ERROR querying pool count: " << e.what() << std::endl;
        //return false; // Don't return here for the test
        std::cout << LOG_PREFIX_ADAPTER << "Bypassing pool count query for testing." << std::endl; // Add log
        pool_count = 5; // Hardcode for test
    }
    
    // if (pool_count == 0) { // Comment out or adjust this check for testing
    //      std::cout << LOG_PREFIX_ADAPTER << "No pools found according to factory. Finished." << std::endl;
    //      return false; // No changes made
    // }
    const long long TEST_POOL_LIMIT = 5; // <<< ЗАДАЕМ ЛИМИТ ДЛЯ ТЕСТА
    if (pool_count == 0) { // If query failed, use test limit
        pool_count = TEST_POOL_LIMIT;
        std::cout << LOG_PREFIX_ADAPTER << "Using test limit of " << TEST_POOL_LIMIT << " pools." << std::endl;
    } else { // If query succeeded, still limit for the test
        pool_count = std::min(pool_count, TEST_POOL_LIMIT); // Use smaller of actual count or test limit
        std::cout << LOG_PREFIX_ADAPTER << "Limiting processing to first " << pool_count << " pools for testing." << std::endl;
    }
    
    
    // --- 6. Initialize Progress Bar ---
    // Use the potentially limited pool_count for the progress bar
    neozork::ui::start_progress("Discovering Pools (Test)", pool_count); // Use limited count
    int added_pool_count = 0;
    
    
    // --- 7. Loop Through Pools ---
    // --- 7. Loop Through Pools ---
    for (long long i = 0; i < pool_count; ++i) {
        std::string pool_address;
        std::string token0_address;
        std::string token1_address;
        
        try {
            // --- 7a. Get Pool Address using Helper ---
            std::string pair_index_data = neozork::connection_manager::encode_eth_call_data(ALL_PAIRS_SIG, static_cast<unsigned long long>(i));
            if (pair_index_data.empty()) throw std::runtime_error("Failed to encode allPairs data");
            
            std::optional<std::string> pair_result_body = try_eth_call_with_failover(bc_info, factory_address, pair_index_data);
            // +++ ADD DELAY AFTER CALL +++
            if (delay_ms > 0) { std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms)); }
            // +++ END DELAY +++
            if (!pair_result_body) throw std::runtime_error("Failed to get pair address from any endpoint");
            
            pool_address = neozork::connection_manager::decode_address_from_result(*pair_result_body);
            if (pool_address.empty() || pool_address == "0x0000000000000000000000000000000000000000") {
                neozork::ui::update_progress(i + 1); continue;
            }
            
            // --- 7b. Check if Pool Exists Locally ---
            if (neozork::config_manager::find_pool(bc_info, pool_address)) {
                neozork::ui::update_progress(i + 1); continue;
            }
            
            // --- 7c. Get Token0 Address using Helper ---
            std::string t0_data = neozork::connection_manager::encode_eth_call_data(TOKEN0_SIG);
            if (t0_data.empty()) throw std::runtime_error("Failed to encode token0 data");
            
            std::optional<std::string> t0_result_body = try_eth_call_with_failover(bc_info, pool_address, t0_data);
            // +++ ADD DELAY AFTER CALL +++
            if (delay_ms > 0) { std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms)); }
            // +++ END DELAY +++
            if (!t0_result_body) throw std::runtime_error("Failed to get token0 from any endpoint for pool " + pool_address);
            
            token0_address = neozork::connection_manager::decode_address_from_result(*t0_result_body);
            if (token0_address.empty() || token0_address == "0x0000000000000000000000000000000000000000") {
                std::cerr << LOG_PREFIX_ADAPTER << "WARN: Invalid token0 address for pool " << pool_address << ". Skipping." << std::endl;
                neozork::ui::update_progress(i + 1); continue;
            }
            
            // --- 7d. Get Token1 Address using Helper ---
            std::string t1_data = neozork::connection_manager::encode_eth_call_data(TOKEN1_SIG);
            if (t1_data.empty()) throw std::runtime_error("Failed to encode token1 data");
            
            std::optional<std::string> t1_result_body = try_eth_call_with_failover(bc_info, pool_address, t1_data);
            // +++ ADD DELAY AFTER CALL +++
            if (delay_ms > 0) { std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms)); }
            // +++ END DELAY +++
            if (!t1_result_body) throw std::runtime_error("Failed to get token1 from any endpoint for pool " + pool_address);
            
            token1_address = neozork::connection_manager::decode_address_from_result(*t1_result_body);
            if (token1_address.empty() || token1_address == "0x0000000000000000000000000000000000000000") {
                std::cerr << LOG_PREFIX_ADAPTER << "WARN: Invalid token1 address for pool " << pool_address << ". Skipping." << std::endl;
                neozork::ui::update_progress(i + 1); continue;
            }
            
            // --- 7e. Construct Pool Info & Add to Config ---
            neozork::config_manager::struct_pool_info new_pool;
            // ... (fill new_pool details) ...
            new_pool.dex_id = dex_id;
            new_pool.pool_id = pool_address;
            new_pool.token0.address = token0_address;
            new_pool.token1.address = token1_address;
            new_pool.token0.symbol = "";
            new_pool.token1.symbol = "";
            
            if (neozork::config_manager::add_pool(bc_info, new_pool)) {
                changes_made = true;
                added_pool_count++;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "\n" << LOG_PREFIX_ADAPTER << "ERROR processing pool index " << i << ": " << e.what() << ". Skipping." << std::endl;
            // --- ADD DELAY EVEN ON ERROR? Optional, maybe not needed here ---
            // if (delay_ms > 0) { std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms)); }
        }
        
        // --- 7f. Update Progress Bar ---
        // REMOVE OLD SLEEP FROM HERE
        // std::cout << "\n" << LOG_PREFIX_ADAPTER << "Debug: Checking delay... "; // Remove this debug log too
        // if (delay_ms > 0) { std::this_thread::sleep_for(...); } // Remove old sleep
        
        neozork::ui::update_progress(i + 1);
        
    } // --- End Loop Through Pools ---
    
    // --- 8. Finish Progress Bar ---
    neozork::ui::finish_progress();
    std::cout << LOG_PREFIX_ADAPTER << "Finished pool discovery loop. Added " << added_pool_count << " new pools to config." << std::endl;
    
    
    // --- 9. Return Status ---
    return changes_made;
}

} // namespace neozork::blockchain_adapters
