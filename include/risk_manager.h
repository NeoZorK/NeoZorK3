#pragma once

#include "types.h"
#include "order_manager.h"
#include "arbitrage_engine.h"
#include <memory>
#include <vector>
#include <functional>
#include <future>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>

namespace solana_arbitrage {

// Forward declarations
class RiskManager;
class PositionTracker;
class RiskCalculator;

// Callback types
using RiskAlertCallback = std::function<void(const String& alert_type, const String& message)>;
using PositionUpdateCallback = std::function<void(const String& token, const Decimal& position)>;

// Risk Calculator interface
class IRiskCalculator {
public:
    virtual ~IRiskCalculator() = default;
    
    // Risk calculations
    virtual Decimal calculate_position_risk(const String& token, const Decimal& position_size) = 0;
    virtual Decimal calculate_portfolio_risk(const std::map<String, Decimal>& positions) = 0;
    virtual Decimal calculate_var(const std::vector<Decimal>& returns, double confidence_level) = 0;
    virtual Decimal calculate_max_drawdown(const std::vector<Decimal>& equity_curve) = 0;
    
    // Risk metrics
    virtual Decimal get_sharpe_ratio(const std::vector<Decimal>& returns) = 0;
    virtual Decimal get_sortino_ratio(const std::vector<Decimal>& returns) = 0;
    virtual Decimal get_calmar_ratio(const std::vector<Decimal>& returns, const Decimal& max_drawdown) = 0;
    
    // Configuration
    virtual void set_risk_free_rate(const Decimal& rate) = 0;
    virtual void set_confidence_level(double level) = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
};

// Position Tracker interface
class IPositionTracker {
public:
    virtual ~IPositionTracker() = default;
    
    // Position management
    virtual void update_position(const String& token, const Decimal& change) = 0;
    virtual Decimal get_position(const String& token) const = 0;
    virtual std::map<String, Decimal> get_all_positions() const = 0;
    virtual void clear_position(const String& token) = 0;
    virtual void clear_all_positions() = 0;
    
    // Position history
    virtual void add_position_snapshot(const Timestamp& timestamp) = 0;
    virtual std::vector<std::pair<Timestamp, std::map<String, Decimal>>> get_position_history() const = 0;
    
    // Position limits
    virtual bool check_position_limit(const String& token, const Decimal& new_position) = 0;
    virtual void set_position_limit(const String& token, const Decimal& limit) = 0;
    virtual Decimal get_position_limit(const String& token) const = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
};

// Risk Manager interface
class IRiskManager {
public:
    virtual ~IRiskManager() = default;
    
    // Manager initialization
    virtual bool initialize(const Config& config, 
                           std::shared_ptr<IOrderManager> order_manager,
                           std::shared_ptr<IArbitrageEngine> arbitrage_engine) = 0;
    virtual void shutdown() = 0;
    
    // Risk monitoring
    virtual bool check_order_risk(const Order& order) = 0;
    virtual bool check_opportunity_risk(const ArbitrageOpportunity& opportunity) = 0;
    virtual bool check_portfolio_risk() = 0;
    virtual bool check_daily_limits() = 0;
    
    // Risk limits
    virtual void set_max_position_size(const Decimal& size) = 0;
    virtual void set_max_daily_loss(const Decimal& loss) = 0;
    virtual void set_max_drawdown(const Decimal& drawdown) = 0;
    virtual void set_max_orders_per_minute(int max_orders) = 0;
    virtual void set_max_concurrent_positions(int max_positions) = 0;
    
    // Risk metrics
    virtual Decimal get_current_portfolio_value() const = 0;
    virtual Decimal get_daily_pnl() const = 0;
    virtual Decimal get_total_pnl() const = 0;
    virtual Decimal get_current_drawdown() const = 0;
    virtual Decimal get_var(double confidence_level) const = 0;
    
    // Risk alerts
    virtual bool subscribe_to_risk_alerts(RiskAlertCallback callback) = 0;
    virtual bool subscribe_to_position_updates(PositionUpdateCallback callback) = 0;
    
    // Manager control
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool is_running() const = 0;
    virtual void pause_trading() = 0;
    virtual void resume_trading() = 0;
    virtual bool is_trading_paused() const = 0;
    
    // Statistics
    virtual int get_risk_checks_performed() const = 0;
    virtual int get_risk_violations() const = 0;
    virtual int get_trading_pauses() const = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
};

// Concrete implementations

// Risk Calculator implementation
class RiskCalculator : public IRiskCalculator {
public:
    RiskCalculator();
    ~RiskCalculator() override;
    
    // IRiskCalculator implementation
    Decimal calculate_position_risk(const String& token, const Decimal& position_size) override;
    Decimal calculate_portfolio_risk(const std::map<String, Decimal>& positions) override;
    Decimal calculate_var(const std::vector<Decimal>& returns, double confidence_level) override;
    Decimal calculate_max_drawdown(const std::vector<Decimal>& equity_curve) override;
    
    Decimal get_sharpe_ratio(const std::vector<Decimal>& returns) override;
    Decimal get_sortino_ratio(const std::vector<Decimal>& returns) override;
    Decimal get_calmar_ratio(const std::vector<Decimal>& returns, const Decimal& max_drawdown) override;
    
    void set_risk_free_rate(const Decimal& rate) override;
    void set_confidence_level(double level) override;
    
    String get_last_error() const override;

private:
    Decimal risk_free_rate_;
    double confidence_level_;
    String last_error_;
    
    Decimal calculate_volatility(const std::vector<Decimal>& returns);
    Decimal calculate_downside_deviation(const std::vector<Decimal>& returns);
    int find_percentile_index(const std::vector<Decimal>& sorted_values, double percentile);
};

// Position Tracker implementation
class PositionTracker : public IPositionTracker {
public:
    PositionTracker();
    ~PositionTracker() override;
    
    // IPositionTracker implementation
    void update_position(const String& token, const Decimal& change) override;
    Decimal get_position(const String& token) const override;
    std::map<String, Decimal> get_all_positions() const override;
    void clear_position(const String& token) override;
    void clear_all_positions() override;
    
    void add_position_snapshot(const Timestamp& timestamp) override;
    std::vector<std::pair<Timestamp, std::map<String, Decimal>>> get_position_history() const override;
    
    bool check_position_limit(const String& token, const Decimal& new_position) override;
    void set_position_limit(const String& token, const Decimal& limit) override;
    Decimal get_position_limit(const String& token) const override;
    
    String get_last_error() const override;

private:
    std::map<String, Decimal> positions_;
    std::map<String, Decimal> position_limits_;
    std::vector<std::pair<Timestamp, std::map<String, Decimal>>> position_history_;
    String last_error_;
    mutable std::mutex positions_mutex_;
};

// Main Risk Manager
class RiskManager : public IRiskManager {
public:
    RiskManager();
    ~RiskManager() override;
    
    // IRiskManager implementation
    bool initialize(const Config& config, 
                    std::shared_ptr<IOrderManager> order_manager,
                    std::shared_ptr<IArbitrageEngine> arbitrage_engine) override;
    void shutdown() override;
    
    bool check_order_risk(const Order& order) override;
    bool check_opportunity_risk(const ArbitrageOpportunity& opportunity) override;
    bool check_portfolio_risk() override;
    bool check_daily_limits() override;
    
    void set_max_position_size(const Decimal& size) override;
    void set_max_daily_loss(const Decimal& loss) override;
    void set_max_drawdown(const Decimal& drawdown) override;
    void set_max_orders_per_minute(int max_orders) override;
    void set_max_concurrent_positions(int max_positions) override;
    
    Decimal get_current_portfolio_value() const override;
    Decimal get_daily_pnl() const override;
    Decimal get_total_pnl() const override;
    Decimal get_current_drawdown() const override;
    Decimal get_var(double confidence_level) const override;
    
    bool subscribe_to_risk_alerts(RiskAlertCallback callback) override;
    bool subscribe_to_position_updates(PositionUpdateCallback callback) override;
    
    void start() override;
    void stop() override;
    bool is_running() const override;
    void pause_trading() override;
    void resume_trading() override;
    bool is_trading_paused() const override;
    
    int get_risk_checks_performed() const override;
    int get_risk_violations() const override;
    int get_trading_pauses() const override;
    
    String get_last_error() const override;

private:
    Config config_;
    std::shared_ptr<IOrderManager> order_manager_;
    std::shared_ptr<IArbitrageEngine> arbitrage_engine_;
    std::unique_ptr<IRiskCalculator> risk_calculator_;
    std::unique_ptr<IPositionTracker> position_tracker_;
    
    RiskLimits risk_limits_;
    Decimal initial_portfolio_value_;
    Decimal current_portfolio_value_;
    Decimal daily_pnl_;
    Decimal total_pnl_;
    Decimal peak_portfolio_value_;
    Decimal current_drawdown_;
    
    std::vector<Decimal> daily_returns_;
    std::vector<Decimal> equity_curve_;
    std::vector<Timestamp> equity_timestamps_;
    
    RiskAlertCallback risk_alert_callback_;
    PositionUpdateCallback position_update_callback_;
    
    std::thread risk_monitor_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> trading_paused_;
    String last_error_;
    
    int risk_checks_performed_;
    int risk_violations_;
    int trading_pauses_;
    
    mutable std::mutex risk_mutex_;
    std::condition_variable cv_;
    
    void risk_monitor_loop();
    void update_portfolio_value();
    void update_drawdown();
    void check_risk_limits();
    void generate_risk_alert(const String& alert_type, const String& message);
    void notify_position_update(const String& token, const Decimal& position);
    bool check_position_limits(const Order& order);
    bool check_daily_loss_limit();
    bool check_drawdown_limit();
    bool check_order_frequency_limit();
    void pause_trading_if_needed();
};

// Factory functions
std::unique_ptr<IRiskManager> create_risk_manager();
std::unique_ptr<IRiskCalculator> create_risk_calculator();
std::unique_ptr<IPositionTracker> create_position_tracker();

} // namespace solana_arbitrage
