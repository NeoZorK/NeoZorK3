#include "connection_manager.h"
#include <iostream> // For error output (std::cerr)
#include <exception>
#include <string>
#include <vector>
#include <map>
#include <optional>

// Include httplib. Define the macro for HTTPS support BEFORE including.
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h> // The library header itself

namespace neozork::connection_manager {

// Implementation of https_get
std::optional<std::string> https_get(
                                     const std::string& host,
                                     const std::string& path,
                                     const http_headers& headers)
{
    try {
        // Create an SSL client for HTTPS
        // Use the constructor that takes the host
        httplib::SSLClient cli(host);
        
        // Set timeouts (in seconds and microseconds)
        cli.set_connection_timeout(10, 0); // 10 seconds for connection
        cli.set_read_timeout(30, 0);       // 30 seconds for reading response
        cli.set_follow_location(true);     // Follow redirects (up to 5 times by default)
        
        // Perform the GET request
        // The Get method requires headers of type httplib::Headers, convert our map
        httplib::Headers http_lib_headers;
        for(const auto& pair : headers) {
            http_lib_headers.emplace(pair.first, pair.second);
        }
        
        // Use path.c_str() as httplib might expect C-style strings in some overloads
        if (auto res = cli.Get(path.c_str(), http_lib_headers)) {
            // Check the response status
            if (res->status >= 200 && res->status < 300) {
                // Success (2xx)
                return res->body;
            } else {
                // Server or client error status
                std::cerr << "HTTP Error: Status " << res->status
                << " for " << host << path << std::endl;
                // Optionally print response body if it might contain error details
                // if (!res->body.empty()) {
                //     std::cerr << "Response body: " << res->body << std::endl;
                // }
                return std::nullopt;
            }
        } else {
            // Connection error or other httplib error
            auto err = res.error();
            std::cerr << "HTTP Request Error: " << httplib::to_string(err)
            << " for " << host << path << std::endl;
            return std::nullopt;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception during HTTP request: " << e.what()
        << " for " << host << path << std::endl;
        return std::nullopt;
    } catch (...) {
        std::cerr << "Unknown exception during HTTP request"
        << " for " << host << path << std::endl;
        return std::nullopt;
    }
}

// TODO: Implement https_post and other functions later

} // namespace neozork::connection_manager
