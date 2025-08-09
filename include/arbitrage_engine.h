#pragma once

#include "types.h"
#include "market_data_provider.h"
#include <memory>
#include <vector>
#include <functional>
#include <future>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

namespace solana_arbitrage {

// Forward declarations
class ArbitrageEngine;
class ArbitrageStrategy;
class OpportunityDetector;

// Callback types
using OpportunityCallback = std::function<void(const ArbitrageOpportunity&)>;
using StrategyCallback = std::function<void(const String& strategy_name, const ArbitrageOpportunity&)>;

// Arbitrage Strategy interface
class IArbitrageStrategy {
public:
    virtual ~IArbitrageStrategy() = default;
    
    // Strategy management
    virtual String get_strategy_name() const = 0;
    virtual bool initialize(const Config& config) = 0;
    virtual void shutdown() = 0;
    
    // Opportunity detection
    virtual std::vector<ArbitrageOpportunity> find_opportunities(
        const std::vector<OrderBook>& order_books) = 0;
    virtual bool validate_opportunity(const ArbitrageOpportunity& opportunity) = 0;
    
    // Strategy parameters
    virtual void set_min_profit_percentage(const Decimal& percentage) = 0;
    virtual void set_max_trade_size(const Decimal& size) = 0;
    virtual void set_slippage_tolerance(const Decimal& slippage) = 0;
    
    // Performance metrics
    virtual int get_opportunities_found() const = 0;
    virtual int get_opportunities_executed() const = 0;
    virtual Decimal get_total_profit() const = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
};

// Opportunity Detector interface
class IOpportunityDetector {
public:
    virtual ~IOpportunityDetector() = default;
    
    // Detection methods
    virtual std::vector<ArbitrageOpportunity> detect_triangular_arbitrage(
        const std::vector<OrderBook>& order_books) = 0;
    virtual std::vector<ArbitrageOpportunity> detect_statistical_arbitrage(
        const std::vector<OrderBook>& order_books) = 0;
    virtual std::vector<ArbitrageOpportunity> detect_cross_dex_arbitrage(
        const std::vector<OrderBook>& order_books) = 0;
    
    // Configuration
    virtual void set_detection_threshold(const Decimal& threshold) = 0;
    virtual void set_max_opportunities(int max_count) = 0;
    
    // Performance
    virtual Duration get_last_detection_time() const = 0;
    virtual int get_detections_per_second() const = 0;
};

// Arbitrage Engine interface
class IArbitrageEngine {
public:
    virtual ~IArbitrageEngine() = default;
    
    // Engine management
    virtual bool initialize(const Config& config, std::shared_ptr<IMarketDataProvider> market_data) = 0;
    virtual void shutdown() = 0;
    virtual bool is_running() const = 0;
    
    // Strategy management
    virtual bool add_strategy(std::unique_ptr<IArbitrageStrategy> strategy) = 0;
    virtual bool remove_strategy(const String& strategy_name) = 0;
    virtual std::vector<String> get_active_strategies() const = 0;
    
    // Opportunity subscription
    virtual bool subscribe_to_opportunities(OpportunityCallback callback) = 0;
    virtual bool unsubscribe_from_opportunities() = 0;
    
    // Engine control
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    
    // Statistics
    virtual int get_total_opportunities_found() const = 0;
    virtual int get_total_opportunities_executed() const = 0;
    virtual Decimal get_total_profit() const = 0;
    virtual Duration get_uptime() const = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
};

// Concrete implementations

// Triangular Arbitrage Strategy
class TriangularArbitrageStrategy : public IArbitrageStrategy {
public:
    TriangularArbitrageStrategy();
    ~TriangularArbitrageStrategy() override;
    
    // IArbitrageStrategy implementation
    String get_strategy_name() const override { return "TriangularArbitrage"; }
    bool initialize(const Config& config) override;
    void shutdown() override;
    
    std::vector<ArbitrageOpportunity> find_opportunities(
        const std::vector<OrderBook>& order_books) override;
    bool validate_opportunity(const ArbitrageOpportunity& opportunity) override;
    
    void set_min_profit_percentage(const Decimal& percentage) override;
    void set_max_trade_size(const Decimal& size) override;
    void set_slippage_tolerance(const Decimal& slippage) override;
    
    int get_opportunities_found() const override;
    int get_opportunities_executed() const override;
    Decimal get_total_profit() const override;
    
    String get_last_error() const override;

private:
    Config config_;
    Decimal min_profit_percentage_;
    Decimal max_trade_size_;
    Decimal slippage_tolerance_;
    
    int opportunities_found_;
    int opportunities_executed_;
    Decimal total_profit_;
    String last_error_;
    
    std::vector<ArbitrageOpportunity> find_triangular_paths(
        const std::vector<OrderBook>& order_books);
    bool is_profitable_triangle(const String& token_a, const String& token_b, 
                               const String& token_c, const std::vector<OrderBook>& order_books);
};

// Cross-DEX Arbitrage Strategy
class CrossDEXArbitrageStrategy : public IArbitrageStrategy {
public:
    CrossDEXArbitrageStrategy();
    ~CrossDEXArbitrageStrategy() override;
    
    // IArbitrageStrategy implementation
    String get_strategy_name() const override { return "CrossDEXArbitrage"; }
    bool initialize(const Config& config) override;
    void shutdown() override;
    
    std::vector<ArbitrageOpportunity> find_opportunities(
        const std::vector<OrderBook>& order_books) override;
    bool validate_opportunity(const ArbitrageOpportunity& opportunity) override;
    
    void set_min_profit_percentage(const Decimal& percentage) override;
    void set_max_trade_size(const Decimal& size) override;
    void set_slippage_tolerance(const Decimal& slippage) override;
    
    int get_opportunities_found() const override;
    int get_opportunities_executed() const override;
    Decimal get_total_profit() const override;
    
    String get_last_error() const override;

private:
    Config config_;
    Decimal min_profit_percentage_;
    Decimal max_trade_size_;
    Decimal slippage_tolerance_;
    
    int opportunities_found_;
    int opportunities_executed_;
    Decimal total_profit_;
    String last_error_;
    
    std::vector<ArbitrageOpportunity> find_cross_dex_opportunities(
        const std::vector<OrderBook>& order_books);
    std::map<String, std::vector<OrderBook>> group_by_token_pair(
        const std::vector<OrderBook>& order_books);
};

// Statistical Arbitrage Strategy
class StatisticalArbitrageStrategy : public IArbitrageStrategy {
public:
    StatisticalArbitrageStrategy();
    ~StatisticalArbitrageStrategy() override;
    
    // IArbitrageStrategy implementation
    String get_strategy_name() const override { return "StatisticalArbitrage"; }
    bool initialize(const Config& config) override;
    void shutdown() override;
    
    std::vector<ArbitrageOpportunity> find_opportunities(
        const std::vector<OrderBook>& order_books) override;
    bool validate_opportunity(const ArbitrageOpportunity& opportunity) override;
    
    void set_min_profit_percentage(const Decimal& percentage) override;
    void set_max_trade_size(const Decimal& size) override;
    void set_slippage_tolerance(const Decimal& slippage) override;
    
    int get_opportunities_found() const override;
    int get_opportunities_executed() const override;
    Decimal get_total_profit() const override;
    
    String get_last_error() const override;

private:
    Config config_;
    Decimal min_profit_percentage_;
    Decimal max_trade_size_;
    Decimal slippage_tolerance_;
    
    int opportunities_found_;
    int opportunities_executed_;
    Decimal total_profit_;
    String last_error_;
    
    // Statistical analysis data
    std::map<String, std::vector<Decimal>> price_history_;
    std::map<String, Decimal> mean_prices_;
    std::map<String, Decimal> std_dev_prices_;
    
    void update_price_statistics(const std::vector<OrderBook>& order_books);
    std::vector<ArbitrageOpportunity> find_mean_reversion_opportunities(
        const std::vector<OrderBook>& order_books);
    bool is_price_anomaly(const String& token, const Decimal& current_price);
};

// Opportunity Detector implementation
class OpportunityDetector : public IOpportunityDetector {
public:
    OpportunityDetector();
    ~OpportunityDetector() override;
    
    // IOpportunityDetector implementation
    std::vector<ArbitrageOpportunity> detect_triangular_arbitrage(
        const std::vector<OrderBook>& order_books) override;
    std::vector<ArbitrageOpportunity> detect_statistical_arbitrage(
        const std::vector<OrderBook>& order_books) override;
    std::vector<ArbitrageOpportunity> detect_cross_dex_arbitrage(
        const std::vector<OrderBook>& order_books) override;
    
    void set_detection_threshold(const Decimal& threshold) override;
    void set_max_opportunities(int max_count) override;
    
    Duration get_last_detection_time() const override;
    int get_detections_per_second() const override;

private:
    Decimal detection_threshold_;
    int max_opportunities_;
    Timestamp last_detection_time_;
    int detections_count_;
    Duration total_detection_time_;
    
    std::vector<ArbitrageOpportunity> find_triangular_paths(
        const std::vector<OrderBook>& order_books);
    std::vector<ArbitrageOpportunity> find_cross_dex_opportunities(
        const std::vector<OrderBook>& order_books);
    bool is_profitable_opportunity(const ArbitrageOpportunity& opportunity);
};

// Main Arbitrage Engine
class ArbitrageEngine : public IArbitrageEngine {
public:
    ArbitrageEngine();
    ~ArbitrageEngine() override;
    
    // IArbitrageEngine implementation
    bool initialize(const Config& config, std::shared_ptr<IMarketDataProvider> market_data) override;
    void shutdown() override;
    bool is_running() const override;
    
    bool add_strategy(std::unique_ptr<IArbitrageStrategy> strategy) override;
    bool remove_strategy(const String& strategy_name) override;
    std::vector<String> get_active_strategies() const override;
    
    bool subscribe_to_opportunities(OpportunityCallback callback) override;
    bool unsubscribe_from_opportunities() override;
    
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    
    int get_total_opportunities_found() const override;
    int get_total_opportunities_executed() const override;
    Decimal get_total_profit() const override;
    Duration get_uptime() const override;
    
    String get_last_error() const override;

private:
    Config config_;
    std::shared_ptr<IMarketDataProvider> market_data_;
    std::unique_ptr<IOpportunityDetector> opportunity_detector_;
    
    std::vector<std::unique_ptr<IArbitrageStrategy>> strategies_;
    std::map<String, IArbitrageStrategy*> strategy_map_;
    
    OpportunityCallback opportunity_callback_;
    
    std::thread engine_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    Timestamp start_time_;
    
    int total_opportunities_found_;
    int total_opportunities_executed_;
    Decimal total_profit_;
    String last_error_;
    
    mutable std::mutex engine_mutex_;
    std::condition_variable cv_;
    
    void engine_loop();
    void process_order_books(const std::vector<OrderBook>& order_books);
    void notify_opportunity(const ArbitrageOpportunity& opportunity);
    std::vector<OrderBook> get_all_order_books();
};

// Factory functions
std::unique_ptr<IArbitrageEngine> create_arbitrage_engine();
std::unique_ptr<IArbitrageStrategy> create_triangular_arbitrage_strategy();
std::unique_ptr<IArbitrageStrategy> create_cross_dex_arbitrage_strategy();
std::unique_ptr<IArbitrageStrategy> create_statistical_arbitrage_strategy();
std::unique_ptr<IOpportunityDetector> create_opportunity_detector();

} // namespace solana_arbitrage
