#include "endpoint_discovery.h"
#include "connection_manager.h" // For HTTP requests
#include "config_manager.h"     // For finding blockchain and adding endpoints
#include <iostream>
#include <string>
#include <vector>
#include <sstream> // For parsing strings
#include <stdexcept>

namespace neozork::endpoint_discovery {

// Helper function to parse a simple URL list (one URL per line)
std::vector<std::string> parse_simple_url_list(const std::string& content) {
    std::vector<std::string> urls;
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        // TODO: Add basic URL validation and ignore empty lines/comments
        if (!line.empty() && line.find("://") != std::string::npos) {
            urls.push_back(line);
        }
    }
    return urls;
}

bool discover_endpoints(
                        const std::string& blockchain_name,
                        const std::vector<std::string>& sources,
                        neozork::config_manager::struct_config& config)
{
    std::cout << "Starting endpoint discovery for blockchain: " << blockchain_name << "..." << std::endl;
    
    // 1. Find the target blockchain in the config
    auto blockchain_ref_opt = neozork::config_manager::find_blockchain(config, blockchain_name);
    if (!blockchain_ref_opt) {
        // If blockchain not found, we can either create it or throw an error
        // For now, throw an error
        throw std::runtime_error("Blockchain '" + blockchain_name + "' not found in configuration. Cannot add endpoints.");
        // TODO: Or create a new one?
        // neozork::config_manager::struct_blockchain_info new_bc;
        // new_bc.name = blockchain_name; // Would need network ID somehow
        // config.blockchains.push_back(new_bc);
        // blockchain_ref_opt = std::ref(config.blockchains.back());
    }
    // Get a reference to the blockchain object to add endpoints
    neozork::config_manager::struct_blockchain_info& blockchain_info = blockchain_ref_opt.value().get();
    
    int added_count = 0;
    
    // 2. Process each source
    for (const std::string& source : sources) {
        std::cout << "  Processing source: " << source << std::endl;
        std::vector<std::string> discovered_urls;
        
        // TODO: Expand logic for different source types (defillama, chainlist)
        if (source.rfind("https://", 0) == 0 || source.rfind("http://", 0) == 0) {
            // Assume it's a URL to a simple text list (e.g., GitHub raw)
            // Extract host and path for https_get
            // Example: "https://raw.githubusercontent.com/user/repo/main/list.txt"
            // host = "raw.githubusercontent.com"
            // path = "/user/repo/main/list.txt"
            size_t host_start = source.find("://") + 3;
            size_t path_start = source.find('/', host_start);
            if (path_start == std::string::npos) {
                std::cerr << "    ERROR: Invalid URL format: " << source << std::endl;
                continue;
            }
            std::string host = source.substr(host_start, path_start - host_start);
            std::string path = source.substr(path_start);
            
            // Use connection_manager
            // TODO: Add User-Agent and other headers if necessary
            auto response_body_opt = neozork::connection_manager::https_get(host, path);
            
            if (response_body_opt) {
                std::cout << "    Downloaded list from " << source << std::endl;
                discovered_urls = parse_simple_url_list(response_body_opt.value());
                std::cout << "    Parsed " << discovered_urls.size() << " potential URLs." << std::endl;
            } else {
                std::cerr << "    ERROR: Failed to download list from " << source << std::endl;
                continue; // Move to the next source
            }
            
        } else {
            std::cerr << "    WARNING: Source type '" << source << "' not yet supported." << std::endl;
            continue;
        }
        
        // 3. Add unique endpoints to the config
        for (const auto& url : discovered_urls) {
            neozork::config_manager::struct_endpoint new_endpoint;
            new_endpoint.url = url;
            // Default types are already set in the struct definition
            
            if (neozork::config_manager::add_endpoint(blockchain_info, new_endpoint)) {
                added_count++;
            }
        }
    }
    
    std::cout << "Endpoint discovery finished. Added " << added_count << " new endpoints for " << blockchain_name << "." << std::endl;
    
    // 4. Save the updated config (IMPORTANT!)
    // Save only if something was added (or always, for simplicity?)
    if (added_count > 0) {
        try {
            neozork::config_manager::save_config(config);
            std::cout << "Configuration saved successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "ERROR saving config after discovery: " << e.what() << std::endl;
            return false; // Save error
        }
    }
    
    return true; // Discovery was attempted (maybe without adding new endpoints)
}

} // namespace neozork::endpoint_discovery
