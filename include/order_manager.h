#pragma once

#include "types.h"
#include "blockchain_adapters.h"
#include "market_data_provider.h"
#include <memory>
#include <vector>
#include <functional>
#include <future>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <map>

namespace solana_arbitrage {

// Forward declarations
class OrderManager;
class OrderExecutor;
class OrderValidator;

// Callback types
using OrderExecutionCallback = std::function<void(const Order&, bool success, const String& error)>;
using TradeCallback = std::function<void(const Trade&)>;
using BalanceUpdateCallback = std::function<void(const String& token, const Decimal& balance)>;

// Order Executor interface
class IOrderExecutor {
public:
    virtual ~IOrderExecutor() = default;
    
    // Order execution
    virtual std::future<Order> execute_order(const Order& order) = 0;
    virtual std::future<bool> cancel_order(const String& order_id) = 0;
    virtual std::future<Order> get_order_status(const String& order_id) = 0;
    
    // Batch operations
    virtual std::future<std::vector<Order>> execute_orders(const std::vector<Order>& orders) = 0;
    virtual std::future<bool> cancel_all_orders() = 0;
    
    // Connection management
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;
    
    // Performance metrics
    virtual Duration get_average_execution_time() const = 0;
    virtual int get_successful_executions() const = 0;
    virtual int get_failed_executions() const = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
};

// Order Validator interface
class IOrderValidator {
public:
    virtual ~IOrderValidator() = default;
    
    // Validation methods
    virtual bool validate_order(const Order& order) = 0;
    virtual bool validate_order_size(const Order& order, const Decimal& available_balance) = 0;
    virtual bool validate_order_price(const Order& order, const OrderBook& order_book) = 0;
    virtual bool validate_slippage(const Order& order, const Decimal& expected_price, 
                                  const Decimal& actual_price) = 0;
    
    // Configuration
    virtual void set_max_slippage(const Decimal& slippage) = 0;
    virtual void set_min_order_size(const Decimal& size) = 0;
    virtual void set_max_order_size(const Decimal& size) = 0;
    
    // Validation results
    virtual String get_validation_error() const = 0;
    virtual void clear_validation_error() = 0;
};

// Order Manager interface
class IOrderManager {
public:
    virtual ~IOrderManager() = default;
    
    // Manager initialization
    virtual bool initialize(const Config& config, 
                           std::shared_ptr<IMarketDataProvider> market_data,
                           std::shared_ptr<IWalletManager> wallet) = 0;
    virtual void shutdown() = 0;
    
    // Order management
    virtual std::future<Order> place_order(const MarketPair& pair, OrderSide side, 
                                          OrderType type, const Decimal& size, 
                                          const Decimal& price = Decimal()) = 0;
    virtual std::future<bool> cancel_order(const String& order_id) = 0;
    virtual std::future<Order> get_order(const String& order_id) = 0;
    virtual std::future<std::vector<Order>> get_open_orders() = 0;
    virtual std::future<std::vector<Order>> get_order_history(const Timestamp& since = Timestamp()) = 0;
    
    // Trade management
    virtual std::future<std::vector<Trade>> get_trades(const MarketPair& pair, 
                                                       const Timestamp& since = Timestamp()) = 0;
    virtual std::future<Decimal> get_balance(const String& token) = 0;
    
    // Callback registration
    virtual bool subscribe_to_order_updates(OrderExecutionCallback callback) = 0;
    virtual bool subscribe_to_trades(TradeCallback callback) = 0;
    virtual bool subscribe_to_balance_updates(BalanceUpdateCallback callback) = 0;
    
    // Manager control
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool is_running() const = 0;
    
    // Statistics
    virtual int get_total_orders_placed() const = 0;
    virtual int get_total_orders_filled() const = 0;
    virtual int get_total_orders_cancelled() const = 0;
    virtual Decimal get_total_volume_traded() const = 0;
    virtual Decimal get_total_fees_paid() const = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
};

// Concrete implementations

// Order Executor implementation
class OrderExecutor : public IOrderExecutor {
public:
    OrderExecutor(std::shared_ptr<ISolanaRPCClient> rpc_client,
                  std::shared_ptr<IWalletManager> wallet);
    ~OrderExecutor() override;
    
    // IOrderExecutor implementation
    std::future<Order> execute_order(const Order& order) override;
    std::future<bool> cancel_order(const String& order_id) override;
    std::future<Order> get_order_status(const String& order_id) override;
    
    std::future<std::vector<Order>> execute_orders(const std::vector<Order>& orders) override;
    std::future<bool> cancel_all_orders() override;
    
    bool connect() override;
    void disconnect() override;
    bool is_connected() const override;
    
    Duration get_average_execution_time() const override;
    int get_successful_executions() const override;
    int get_failed_executions() const override;
    
    String get_last_error() const override;

private:
    std::shared_ptr<ISolanaRPCClient> rpc_client_;
    std::shared_ptr<IWalletManager> wallet_;
    bool connected_;
    String last_error_;
    
    int successful_executions_;
    int failed_executions_;
    Duration total_execution_time_;
    mutable std::mutex stats_mutex_;
    
    std::future<Order> execute_solana_transaction(const Order& order);
    String build_transaction_for_order(const Order& order);
    bool validate_transaction_response(const String& response);
};

// Order Validator implementation
class OrderValidator : public IOrderValidator {
public:
    OrderValidator();
    ~OrderValidator() override;
    
    // IOrderValidator implementation
    bool validate_order(const Order& order) override;
    bool validate_order_size(const Order& order, const Decimal& available_balance) override;
    bool validate_order_price(const Order& order, const OrderBook& order_book) override;
    bool validate_slippage(const Order& order, const Decimal& expected_price, 
                          const Decimal& actual_price) override;
    
    void set_max_slippage(const Decimal& slippage) override;
    void set_min_order_size(const Decimal& size) override;
    void set_max_order_size(const Decimal& size) override;
    
    String get_validation_error() const override;
    void clear_validation_error() override;

private:
    Decimal max_slippage_;
    Decimal min_order_size_;
    Decimal max_order_size_;
    String validation_error_;
    
    bool validate_order_basic(const Order& order);
    bool validate_market_pair(const MarketPair& pair);
    bool validate_order_type(const OrderType type);
};

// Main Order Manager
class OrderManager : public IOrderManager {
public:
    OrderManager();
    ~OrderManager() override;
    
    // IOrderManager implementation
    bool initialize(const Config& config, 
                    std::shared_ptr<IMarketDataProvider> market_data,
                    std::shared_ptr<IWalletManager> wallet) override;
    void shutdown() override;
    
    std::future<Order> place_order(const MarketPair& pair, OrderSide side, 
                                  OrderType type, const Decimal& size, 
                                  const Decimal& price = Decimal()) override;
    std::future<bool> cancel_order(const String& order_id) override;
    std::future<Order> get_order(const String& order_id) override;
    std::future<std::vector<Order>> get_open_orders() override;
    std::future<std::vector<Order>> get_order_history(const Timestamp& since = Timestamp()) override;
    
    std::future<std::vector<Trade>> get_trades(const MarketPair& pair, 
                                               const Timestamp& since = Timestamp()) override;
    std::future<Decimal> get_balance(const String& token) override;
    
    bool subscribe_to_order_updates(OrderExecutionCallback callback) override;
    bool subscribe_to_trades(TradeCallback callback) override;
    bool subscribe_to_balance_updates(BalanceUpdateCallback callback) override;
    
    void start() override;
    void stop() override;
    bool is_running() const override;
    
    int get_total_orders_placed() const override;
    int get_total_orders_filled() const override;
    int get_total_orders_cancelled() const override;
    Decimal get_total_volume_traded() const override;
    Decimal get_total_fees_paid() const override;
    
    String get_last_error() const override;

private:
    Config config_;
    std::shared_ptr<IMarketDataProvider> market_data_;
    std::shared_ptr<IWalletManager> wallet_;
    std::unique_ptr<IOrderExecutor> order_executor_;
    std::unique_ptr<IOrderValidator> order_validator_;
    
    std::map<String, Order> active_orders_;
    std::vector<Order> order_history_;
    std::vector<Trade> trade_history_;
    std::map<String, Decimal> balances_;
    
    OrderExecutionCallback order_callback_;
    TradeCallback trade_callback_;
    BalanceUpdateCallback balance_callback_;
    
    std::thread manager_thread_;
    std::atomic<bool> running_;
    String last_error_;
    
    int total_orders_placed_;
    int total_orders_filled_;
    int total_orders_cancelled_;
    Decimal total_volume_traded_;
    Decimal total_fees_paid_;
    
    mutable std::mutex manager_mutex_;
    std::condition_variable cv_;
    
    void manager_loop();
    void process_order_queue();
    void update_order_status(const String& order_id, OrderStatus status);
    void add_trade(const Trade& trade);
    void update_balance(const String& token, const Decimal& balance);
    void notify_order_update(const Order& order, bool success, const String& error);
    void notify_trade(const Trade& trade);
    void notify_balance_update(const String& token, const Decimal& balance);
    
    std::queue<Order> order_queue_;
    std::mutex queue_mutex_;
};

// Factory functions
std::unique_ptr<IOrderManager> create_order_manager();
std::unique_ptr<IOrderExecutor> create_order_executor(std::shared_ptr<ISolanaRPCClient> rpc_client,
                                                      std::shared_ptr<IWalletManager> wallet);
std::unique_ptr<IOrderValidator> create_order_validator();

} // namespace solana_arbitrage
