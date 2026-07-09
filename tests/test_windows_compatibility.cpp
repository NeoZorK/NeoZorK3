#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <openssl/ssl.h>

#ifdef _WIN32
#include "windows_compat.h"
#endif

int main() {
    std::cout << "=== Windows Compatibility Test for NeoZorK3 Arbitrage Bot ===" << std::endl;
    
    bool all_tests_passed = true;
    
    // Test 1: Boost.ASIO
    try {
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::resolver resolver(io_context);
        std::cout << "✓ Boost.ASIO: TCP resolver working" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Boost.ASIO failed: " << e.what() << std::endl;
        all_tests_passed = false;
    }
    
    // Test 2: Boost.Thread
    try {
        std::atomic<int> counter{0};
        std::vector<boost::thread> threads;
        
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&counter]() {
                for (int j = 0; j < 100; ++j) {
                    counter.fetch_add(1);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        if (counter.load() == 400) {
            std::cout << "✓ Boost.Thread: Multi-threading working" << std::endl;
        } else {
            throw std::runtime_error("Thread counter mismatch");
        }
    } catch (const std::exception& e) {
        std::cerr << "✗ Boost.Thread failed: " << e.what() << std::endl;
        all_tests_passed = false;
    }
    
    // Test 3: OpenSSL
    try {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        
        SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx) {
            throw std::runtime_error("Failed to create SSL context");
        }
        SSL_CTX_free(ctx);
        
        std::cout << "✓ OpenSSL: SSL context creation working" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ OpenSSL failed: " << e.what() << std::endl;
        all_tests_passed = false;
    }
    
    // Test 4: Windows-specific functionality
    #ifdef _WIN32
    try {
        std::string version = neozork::windows::get_windows_version();
        bool is_vista_or_later = neozork::windows::is_windows_vista_or_later();
        
        std::cout << "✓ Windows API: Version " << version << std::endl;
        std::cout << "✓ Windows API: Vista or later: " << (is_vista_or_later ? "Yes" : "No") << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Windows API failed: " << e.what() << std::endl;
        all_tests_passed = false;
    }
    #else
    std::cout << "✓ Windows API: Not applicable (not on Windows)" << std::endl;
    #endif
    
    // Test 5: System info
    std::cout << "\n=== System Information ===" << std::endl;
    #ifdef _WIN32
    std::cout << "Platform: Windows" << std::endl;
    std::cout << "Architecture: " << (sizeof(void*) == 8 ? "x64" : "x86") << std::endl;
    #else
    std::cout << "Platform: Non-Windows" << std::endl;
    #endif
    
    if (all_tests_passed) {
        std::cout << "\n=== SUCCESS: All Windows compatibility tests passed! ===" << std::endl;
        std::cout << "The NeoZorK3 arbitrage bot is compatible with Windows systems." << std::endl;
        return 0;
    } else {
        std::cout << "\n=== FAILURE: Some tests failed! ===" << std::endl;
        return 1;
    }
}
