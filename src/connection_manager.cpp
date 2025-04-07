// src/connection_manager.cpp

#include "connection_manager.h"
#include <iostream> // For error output (std::cerr)
#include <exception>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>    // For timing latency
#include <regex>     // For parsing URL
#include <sstream>   // For string manipulation and hex formatting
#include <iomanip>   // For std::setfill, std::setw
#include <stdexcept> // For std::invalid_argument, std::out_of_range
#include <limits>    // For numeric limits potentially

// Include httplib. Define the macro for HTTPS support BEFORE including.
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h> // The library header itself

// Include nlohmann/json for the helper function
#include <nlohmann/json.hpp>
using json = nlohmann::json;


namespace neozork::connection_manager {

// Helper to convert httplib::Headers to our std::map based headers
// Note: httplib::Headers is std::multimap. This conversion keeps the *last* value for duplicate keys.
http_headers convert_httplib_headers(const httplib::Headers& lib_headers) {
    http_headers our_headers;
    for (const auto& pair : lib_headers) {
        our_headers[pair.first] = pair.second; // Overwrites previous value if key exists
    }
    return our_headers;
}


// --- Implementation of https_get (Updated to return connection_result) ---
connection_result https_get(
                            const std::string& host,
                            const std::string& path,
                            const http_headers& headers)
{
    connection_result result;
    result.request_size_bytes = 0; // No request body for GET
    
    try {
        httplib::SSLClient cli(host);
        cli.set_connection_timeout(10, 0); // 10 seconds connection timeout
        cli.set_read_timeout(30, 0);       // 30 seconds read timeout
        cli.set_follow_location(true);
        
        httplib::Headers http_lib_headers;
        for(const auto& pair : headers) {
            http_lib_headers.emplace(pair.first, pair.second);
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (auto res = cli.Get(path.c_str(), http_lib_headers)) {
            auto end_time = std::chrono::high_resolution_clock::now();
            result.latency_ms = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000.0;
            
            result.status_code = res->status;
            result.body = res->body;
            result.response_size_bytes = res->body.length();
            result.response_headers = convert_httplib_headers(res->headers);
            
            if (res->status < 200 || res->status >= 300) {
                // Error status code, keep body/headers but indicate potential issue via status
                std::cerr << "Connection Manager (GET): HTTP Status " << res->status << " for " << host << path << std::endl;
            }
            // Successful request (status code might still be non-2xx, check outside)
            
        } else {
            // httplib request error (connection failed, DNS error, etc.)
            auto end_time = std::chrono::high_resolution_clock::now();
            result.latency_ms = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000.0; // Measure time even on failure
            auto err = res.error();
            result.error_message = "httplib error: " + httplib::to_string(err);
            std::cerr << "Connection Manager (GET): Request Error: " << *result.error_message
            << " for " << host << path << std::endl;
        }
    } catch (const std::exception& e) {
        result.error_message = "Exception: " + std::string(e.what());
        std::cerr << "Connection Manager (GET): Exception during request: " << *result.error_message
        << " for " << host << path << std::endl;
    } catch (...) {
        result.error_message = "Unknown exception during request.";
        std::cerr << "Connection Manager (GET): Unknown exception during request"
        << " for " << host << path << std::endl;
    }
    return result;
}

// --- Implementation of https_post ---
connection_result https_post(
                             const std::string& host,
                             const std::string& path,
                             const std::string& request_body,
                             const http_headers& headers)
{
    connection_result result;
    result.request_size_bytes = request_body.length();
    
    try {
        httplib::SSLClient cli(host);
        cli.set_connection_timeout(10, 0); // 10 seconds connection timeout
        cli.set_read_timeout(30, 0);       // 30 seconds read timeout
        // No redirects for POST usually
        
        httplib::Headers http_lib_headers;
        bool content_type_set = false;
        for(const auto& pair : headers) {
            http_lib_headers.emplace(pair.first, pair.second);
            if (pair.first == "Content-Type") { // Check if Content-Type was provided
                content_type_set = true;
            }
        }
        // Default Content-Type if not provided by caller
        if (!content_type_set) {
            // http_lib_headers.emplace("Content-Type", "application/json"); // Decided against default here, let caller specify
        }
        
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Determine content type for httplib::Client::Post
        std::string content_type = "application/octet-stream"; // Default if not found
        if (headers.count("Content-Type")) {
            content_type = headers.at("Content-Type");
        } else if (headers.count("content-type")) { // Case-insensitive check
            content_type = headers.at("content-type");
        }
        
        
        if (auto res = cli.Post(path.c_str(), http_lib_headers, request_body, content_type.c_str())) {
            auto end_time = std::chrono::high_resolution_clock::now();
            result.latency_ms = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000.0;
            
            result.status_code = res->status;
            result.body = res->body;
            result.response_size_bytes = res->body.length();
            result.response_headers = convert_httplib_headers(res->headers);
            
            if (res->status < 200 || res->status >= 300) {
                std::cerr << "Connection Manager (POST): HTTP Status " << res->status << " for " << host << path << std::endl;
            }
            
        } else {
            auto end_time = std::chrono::high_resolution_clock::now();
            result.latency_ms = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000.0;
            auto err = res.error();
            result.error_message = "httplib error: " + httplib::to_string(err);
            std::cerr << "Connection Manager (POST): Request Error: " << *result.error_message
            << " for " << host << path << std::endl;
        }
    } catch (const std::exception& e) {
        result.error_message = "Exception: " + std::string(e.what());
        std::cerr << "Connection Manager (POST): Exception during request: " << *result.error_message
        << " for " << host << path << std::endl;
    } catch (...) {
        result.error_message = "Unknown exception during request.";
        std::cerr << "Connection Manager (POST): Unknown exception during request"
        << " for " << host << path << std::endl;
    }
    return result;
}


// --- Implementation of send_json_rpc_request ---
connection_result send_json_rpc_request(
                                        const std::string& url,
                                        const std::string& method,
                                        const json& params)
{
    // 1. Parse URL
    // Simple regex to capture host and optional path from https URL
    // It expects URLs like https://hostname.com or https://hostname.com/path/etc
    std::regex url_regex(R"(^https://([^/]+)(/.*)?$)");
    std::smatch match;
    std::string host;
    std::string path = "/"; // Default path
    
    if (std::regex_match(url, match, url_regex)) {
        if (match.size() >= 2) {
            host = match[1].str();
        }
        if (match.size() >= 3 && match[2].matched) {
            path = match[2].str();
        }
    } else {
        throw std::invalid_argument("Invalid or non-HTTPS URL provided to send_json_rpc_request: " + url);
    }
    
    if (host.empty()) {
        throw std::invalid_argument("Could not extract host from URL: " + url);
    }
    
    // 2. Create JSON RPC payload
    json payload = {
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params},
        {"id", 1} // Use a fixed ID for simplicity in scanning
    };
    std::string request_body_str;
    try {
        request_body_str = payload.dump(); // Serialize JSON to string
    } catch (const json::exception& e) {
        // Should not happen with our simple structure, but good practice
        throw std::runtime_error("JSON serialization failed: " + std::string(e.what()));
    }
    
    
    // 3. Prepare headers
    http_headers rpc_headers = {
        {"Content-Type", "application/json"},
        {"Accept", "application/json"}
        // Add other headers if needed, e.g., User-Agent
        // {"User-Agent", "NeoZorK3/0.1 Scanner"}
    };
    
    // 4. Call https_post
    return https_post(host, path, request_body_str, rpc_headers);
}

// +++ START ADDED ABI HELPERS IMPLEMENTATIONS +++

/**
 * @brief Encodes data for an eth_call for a function with no arguments.
 */
std::string encode_eth_call_data(const std::string& function_sig_hash) {
    // Basic validation: should be "0x" followed by 8 hex chars
    if (function_sig_hash.length() != 10 || function_sig_hash.rfind("0x", 0) != 0) {
        std::cerr << "Connection Manager ABI: Invalid function signature hash format: " << function_sig_hash << std::endl;
        return ""; // Return empty on error
    }
    // For no arguments, data is just the hash
    return function_sig_hash;
}


/**
 * @brief Encodes data for an eth_call for a function with one uint256 argument.
 */
std::string encode_eth_call_data(const std::string& function_sig_hash, unsigned long long argument) {
    // Basic validation for hash
    if (function_sig_hash.length() != 10 || function_sig_hash.rfind("0x", 0) != 0) {
        std::cerr << "Connection Manager ABI: Invalid function signature hash format: " << function_sig_hash << std::endl;
        return "";
    }
    
    // Remove "0x" prefix from hash
    std::string hash_part = function_sig_hash.substr(2);
    
    // Encode the argument as a 32-byte (64 hex chars) padded hex string
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(64) << argument;
    std::string arg_part = ss.str();
    
    // Concatenate "0x" + hash + encoded_arg
    return "0x" + hash_part + arg_part;
}


/**
 * @brief Decodes an address from a standard 32-byte eth_call hex result.
 */
std::string decode_address_from_result(const std::string& result_hex) {
    // Expected format: "0x" followed by 64 hex characters (32 bytes)
    if (result_hex.length() != 66 || result_hex.rfind("0x", 0) != 0) {
        // It might also return "0x" for a zero address/failure, handle gently?
        if (result_hex == "0x") return "0x0000000000000000000000000000000000000000"; // Treat "0x" as zero address
        std::cerr << "Connection Manager ABI: Invalid result format for address decoding: " << result_hex << " (Length: " << result_hex.length() << ")" << std::endl;
        return ""; // Return empty on error
    }
    
    // Address is the last 40 characters (20 bytes)
    std::string address_part = result_hex.substr(66 - 40);
    
    // Prepend "0x"
    return "0x" + address_part;
}


/**
 * @brief Decodes a uint256 value from a standard 32-byte eth_call hex result.
 */
long long decode_uint256_from_result(const std::string& result_hex) {
    // Expected format: "0x" followed by hex characters
    if (result_hex.length() <= 2 || result_hex.rfind("0x", 0) != 0) {
        throw std::invalid_argument("Connection Manager ABI: Invalid hex string format for uint256 decoding: " + result_hex);
    }
    
    // Get the hex part after "0x"
    std::string hex_part = result_hex.substr(2);
    
    // Handle empty hex part (e.g., if input was just "0x")
    if (hex_part.empty()) {
        return 0; // Treat empty hex as zero
    }
    
    try {
        // Use std::stoull for unsigned long long, as uint256 can exceed long long max
        unsigned long long ull_value = std::stoull(hex_part, nullptr, 16);
        
        // Check if it fits in long long before returning
        if (ull_value > static_cast<unsigned long long>(std::numeric_limits<long long>::max())) {
            // Consider how critical exceeding long long is. For pool count, it's unlikely.
            // For token amounts, it's possible. Maybe return unsigned long long instead?
            // For now, throw out_of_range if it exceeds signed long long.
            throw std::out_of_range("Connection Manager ABI: Decoded uint256 value exceeds long long limits: " + result_hex);
        }
        
        // Safely cast and return
        return static_cast<long long>(ull_value);
        
    } catch (const std::invalid_argument& ia) {
        // Error during hex conversion (e.g., non-hex characters)
        throw std::invalid_argument("Connection Manager ABI: Invalid hex characters in uint256 string: " + result_hex + " (" + ia.what() + ")");
    } catch (const std::out_of_range& oor) {
        // Error if value is too large for unsigned long long OR our manual check fails
        throw std::out_of_range("Connection Manager ABI: Uint256 value out of range for (unsigned) long long: " + result_hex + " (" + oor.what() + ")");
    }
}

// +++ END ADDED ABI HELPERS IMPLEMENTATIONS +++

/**
 * @brief Sends a JSON RPC "eth_call" request to an endpoint.
 */
connection_result send_eth_call(
                                const std::string& endpoint_url,
                                const std::string& contract_address,
                                const std::string& encoded_data)
{
    // 1. Parse Endpoint URL into host and path
    // (Using similar regex logic as in send_json_rpc_request, could be refactored into a helper)
    std::regex url_regex(R"(^(https?)://([^/]+)(/.*)?$)"); // Allow http/https
    std::smatch match;
    std::string scheme;
    std::string host;
    std::string path = "/"; // Default path
    
    if (std::regex_match(endpoint_url, match, url_regex) && match.size() >= 3) {
        scheme = match[1].str();
        host = match[2].str();
        if (match.size() >= 4 && match[3].matched) {
            path = match[3].str();
        }
    } else {
        throw std::invalid_argument("Invalid endpoint URL format for send_eth_call: " + endpoint_url);
    }
    
    if (host.empty()) {
        throw std::invalid_argument("Could not extract host from URL: " + endpoint_url);
    }
    // Basic validation for address and data (can be improved)
    if (contract_address.empty() || encoded_data.empty() || encoded_data.rfind("0x", 0) != 0) {
        throw std::invalid_argument("Invalid contract address or encoded data provided to send_eth_call.");
    }
    
    
    // 2. Create the 'params' array for eth_call
    json call_object = {
        {"to", contract_address},
        {"data", encoded_data}
    };
    // Params array: [callObject, blockParameter]
    json params_array = json::array({call_object, "latest"}); // Using "latest" block tag
    
    
    // 3. Create the full JSON RPC payload
    json payload = {
        {"jsonrpc", "2.0"},
        {"method", "eth_call"}, // Specific method
        {"params", params_array},
        {"id", 1} // Use a fixed ID or incrementing if needed later
    };
    
    std::string request_body_str;
    try {
        request_body_str = payload.dump(); // Serialize payload to string
    } catch (const json::exception& e) {
        throw std::runtime_error("JSON serialization failed for eth_call payload: " + std::string(e.what()));
    }
    
    
    // 4. Prepare headers
    http_headers rpc_headers = {
        {"Content-Type", "application/json"},
        {"Accept", "application/json"}
        // Add User-Agent if desired
        // {"User-Agent", "NeoZorK3/PoolFinder"}
    };
    
    
    // 5. Perform the HTTPS POST request using existing function
    // Assuming https_post handles potential exceptions internally and returns a result object
    if (scheme == "https") {
        return https_post(host, path, request_body_str, rpc_headers);
    } else if (scheme == "http") {
        // TODO: Implement http_post if needed, or throw error
        throw std::runtime_error("HTTP (non-SSL) endpoints not currently supported for eth_call.");
        // Placeholder return if http_post existed:
        // return http_post(host, path, request_body_str, rpc_headers);
    } else {
        throw std::invalid_argument("Unsupported URL scheme for eth_call: " + scheme);
    }
    
}

// TODO: Implement ws_connect, ipc_connect etc. later

} // namespace neozork::connection_manager
