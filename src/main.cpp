// src/main.cpp

#include "types.h"
#include "config_manager.h"
#include "logger.h"
#include "blockchain_adapters.h"
#include "market_data_provider.h"
#include "arbitrage_engine.h"
#include "order_manager.h"
#include "risk_manager.h"

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>
#include <atomic>

using namespace solana_arbitrage;

// Global variables for graceful shutdown
std::atomic<bool> g_running(true);
std::shared_ptr<IArbitrageEngine> g_arbitrage_engine;
std::shared_ptr<IOrderManager> g_order_manager;
std::shared_ptr<IRiskManager> g_risk_manager;
std::shared_ptr<IMarketDataProvider> g_market_data;
std::shared_ptr<IConfigManager> g_config_manager;

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully..." << std::endl;
    g_running = false;
}

// Print usage information
void print_usage(const char* program_name) {
    std::cout << "Solana Arbitrage Bot - High Performance C++ Implementation\n"
              << "Usage: " << program_name << " [options]\n\n"
              << "Options:\n"
              << "  -c, --config <file>     Configuration file path (default: config.json)\n"
              << "  -l, --log <file>        Log file path (default: logs/bot.log)\n"
              << "  -v, --verbose           Enable verbose logging\n"
              << "  -d, --dry-run           Run in dry-run mode (no real trades)\n"
              << "  -h, --help              Show this help message\n\n"
              << "Examples:\n"
              << "  " << program_name << " -c my_config.json\n"
              << "  " << program_name << " --dry-run --verbose\n"
              << "  " << program_name << " -l logs/debug.log\n";
}

// Parse command line arguments
bool parse_arguments(int argc, char* argv[], Config& config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return false;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                // Will be loaded by config manager
                ++i;
            } else {
                std::cerr << "Error: Missing config file path" << std::endl;
                return false;
            }
        } else if (arg == "-l" || arg == "--log") {
            if (i + 1 < argc) {
                // Will be set by logger
                ++i;
            } else {
                std::cerr << "Error: Missing log file path" << std::endl;
                return false;
            }
        } else if (arg == "-v" || arg == "--verbose") {
            // Will be set by logger
        } else if (arg == "-d" || arg == "--dry-run") {
            config.dry_run = true;
        } else {
            std::cerr << "Error: Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
            return false;
        }
    }
    return true;
}

// Initialize all components
bool initialize_components(const Config& config) {
    LOG_INFO("Initializing Solana Arbitrage Bot components...");
    
    // Initialize global logger
    if (!initialize_global_logger("logs/bot.log", LogLevel::INFO)) {
        std::cerr << "Failed to initialize logger" << std::endl;
        return false;
    }
    
    LOG_INFO("Logger initialized successfully");
    
    // Initialize config manager
    g_config_manager = create_config_manager();
    if (!g_config_manager) {
        LOG_ERROR("Failed to create config manager");
        return false;
    }
    
    // Initialize blockchain adapters
    boost::asio::io_context io_context;
    auto rpc_client = create_solana_rpc_client(io_context);
    auto wallet_manager = create_wallet_manager();
    
    if (!rpc_client || !wallet_manager) {
        LOG_ERROR("Failed to create blockchain adapters");
        return false;
    }
    
    // Initialize market data provider
    auto market_data_unique = create_market_data_provider();
    if (!market_data_unique) {
        LOG_ERROR("Failed to create market data provider");
        return false;
    }
    
    g_market_data = std::shared_ptr<IMarketDataProvider>(std::move(market_data_unique));
    
    if (!g_market_data->initialize(config)) {
        LOG_ERROR("Failed to initialize market data provider");
        return false;
    }
    
    LOG_INFO("Market data provider initialized successfully");
    
    // Initialize order manager
    auto order_manager_unique = create_order_manager();
    if (!order_manager_unique) {
        LOG_ERROR("Failed to create order manager");
        return false;
    }
    
    g_order_manager = std::shared_ptr<IOrderManager>(std::move(order_manager_unique));
    
    auto wallet_manager_shared = std::shared_ptr<IWalletManager>(std::move(wallet_manager));
    if (!g_order_manager->initialize(config, g_market_data, wallet_manager_shared)) {
        LOG_ERROR("Failed to initialize order manager");
        return false;
    }
    
    LOG_INFO("Order manager initialized successfully");
    
    // Initialize arbitrage engine
    auto arbitrage_engine_unique = create_arbitrage_engine();
    if (!arbitrage_engine_unique) {
        LOG_ERROR("Failed to create arbitrage engine");
        return false;
    }
    
    g_arbitrage_engine = std::shared_ptr<IArbitrageEngine>(std::move(arbitrage_engine_unique));
    
    if (!g_arbitrage_engine->initialize(config, g_market_data)) {
        LOG_ERROR("Failed to initialize arbitrage engine");
        return false;
    }
    
    // Add strategies
    auto triangular_strategy = create_triangular_arbitrage_strategy();
    auto cross_dex_strategy = create_cross_dex_arbitrage_strategy();
    auto statistical_strategy = create_statistical_arbitrage_strategy();
    
    if (triangular_strategy) {
        g_arbitrage_engine->add_strategy(std::move(triangular_strategy));
        LOG_INFO("Added triangular arbitrage strategy");
    }
    
    if (cross_dex_strategy) {
        g_arbitrage_engine->add_strategy(std::move(cross_dex_strategy));
        LOG_INFO("Added cross-DEX arbitrage strategy");
    }
    
    if (statistical_strategy) {
        g_arbitrage_engine->add_strategy(std::move(statistical_strategy));
        LOG_INFO("Added statistical arbitrage strategy");
    }
    
    LOG_INFO("Arbitrage engine initialized successfully");
    
    // Initialize risk manager
    g_risk_manager = create_risk_manager();
    if (!g_risk_manager) {
        LOG_ERROR("Failed to create risk manager");
        return false;
    }
    
    if (!g_risk_manager->initialize(config, g_order_manager, g_arbitrage_engine)) {
        LOG_ERROR("Failed to initialize risk manager");
        return false;
    }
    
    LOG_INFO("Risk manager initialized successfully");
    
    return true;
}

// Start all components
bool start_components() {
    LOG_INFO("Starting Solana Arbitrage Bot components...");
    
    // Start market data provider
    if (!g_market_data->is_connected()) {
        LOG_ERROR("Market data provider is not connected");
        return false;
    }
    
    // Start order manager
    g_order_manager->start();
    LOG_INFO("Order manager started");
    
    // Start risk manager
    g_risk_manager->start();
    LOG_INFO("Risk manager started");
    
    // Start arbitrage engine
    g_arbitrage_engine->start();
    LOG_INFO("Arbitrage engine started");
    
    return true;
}

// Stop all components
void stop_components() {
    LOG_INFO("Stopping Solana Arbitrage Bot components...");
    
    if (g_arbitrage_engine) {
        g_arbitrage_engine->stop();
        LOG_INFO("Arbitrage engine stopped");
    }
    
    if (g_risk_manager) {
        g_risk_manager->stop();
        LOG_INFO("Risk manager stopped");
    }
    
    if (g_order_manager) {
        g_order_manager->stop();
        LOG_INFO("Order manager stopped");
    }
    
    if (g_market_data) {
        g_market_data->shutdown();
        LOG_INFO("Market data provider shutdown");
    }
}

// Print statistics
void print_statistics() {
    if (!g_arbitrage_engine || !g_order_manager || !g_risk_manager) {
        return;
    }
    
    std::cout << "\n=== Bot Statistics ===\n";
    std::cout << "Arbitrage Engine:\n";
    std::cout << "  Opportunities Found: " << g_arbitrage_engine->get_total_opportunities_found() << "\n";
    std::cout << "  Opportunities Executed: " << g_arbitrage_engine->get_total_opportunities_executed() << "\n";
    std::cout << "  Total Profit: " << g_arbitrage_engine->get_total_profit().to_double() << " USD\n";
    std::cout << "  Uptime: " << std::chrono::duration_cast<std::chrono::hours>(g_arbitrage_engine->get_uptime()).count() << " hours\n";
    
    std::cout << "\nOrder Manager:\n";
    std::cout << "  Orders Placed: " << g_order_manager->get_total_orders_placed() << "\n";
    std::cout << "  Orders Filled: " << g_order_manager->get_total_orders_filled() << "\n";
    std::cout << "  Orders Cancelled: " << g_order_manager->get_total_orders_cancelled() << "\n";
    std::cout << "  Total Volume: " << g_order_manager->get_total_volume_traded().to_double() << " USD\n";
    std::cout << "  Total Fees: " << g_order_manager->get_total_fees_paid().to_double() << " USD\n";
    
    std::cout << "\nRisk Manager:\n";
    std::cout << "  Current Portfolio Value: " << g_risk_manager->get_current_portfolio_value().to_double() << " USD\n";
    std::cout << "  Daily P&L: " << g_risk_manager->get_daily_pnl().to_double() << " USD\n";
    std::cout << "  Total P&L: " << g_risk_manager->get_total_pnl().to_double() << " USD\n";
    std::cout << "  Current Drawdown: " << g_risk_manager->get_current_drawdown().to_double() << "%\n";
    std::cout << "  Risk Checks: " << g_risk_manager->get_risk_checks_performed() << "\n";
    std::cout << "  Risk Violations: " << g_risk_manager->get_risk_violations() << "\n";
    std::cout << "  Trading Pauses: " << g_risk_manager->get_trading_pauses() << "\n";
    std::cout << "=====================\n\n";
}

// Main function
int main(int argc, char* argv[]) {
    std::cout << "Solana Arbitrage Bot v1.0.0 - High Performance C++ Implementation\n";
    std::cout << "Copyright (c) 2024 Solana Arbitrage Bot Team\n\n";
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Default configuration
    Config config;
    
    // Parse command line arguments
    if (!parse_arguments(argc, argv, config)) {
        return 1;
    }
    
    // Initialize components
    if (!initialize_components(config)) {
        std::cerr << "Failed to initialize components" << std::endl;
        return 1;
    }
    
    // Start components
    if (!start_components()) {
        std::cerr << "Failed to start components" << std::endl;
        stop_components();
        return 1;
    }
    
    std::cout << "Solana Arbitrage Bot started successfully!\n";
    std::cout << "Press Ctrl+C to stop the bot.\n\n";
    
    // Main loop
    auto last_stats_time = std::chrono::steady_clock::now();
    const auto stats_interval = std::chrono::minutes(5);
    
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Print statistics periodically
        auto now = std::chrono::steady_clock::now();
        if (now - last_stats_time >= stats_interval) {
            print_statistics();
            last_stats_time = now;
        }
        
        // Check if components are still running
        if (g_arbitrage_engine && !g_arbitrage_engine->is_running()) {
            LOG_ERROR("Arbitrage engine stopped unexpectedly");
            break;
        }
        
        if (g_risk_manager && g_risk_manager->is_trading_paused()) {
            LOG_WARNING("Trading is paused due to risk limits");
        }
    }
    
    // Stop components
    stop_components();
    
    // Print final statistics
    print_statistics();
    
    // Shutdown global logger
    shutdown_global_logger();
    
    std::cout << "Solana Arbitrage Bot stopped successfully.\n";
    return 0;
}
