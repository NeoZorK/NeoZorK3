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


// TODO: Implement ws_connect, ipc_connect etc. later

} // namespace neozork::connection_manager
