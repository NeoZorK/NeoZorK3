#pragma once

#include "types.h"
#include "blockchain_adapters.h"
#include <memory>
#include <functional>
#include <future>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace solana_arbitrage {

// Forward declarations
class MarketDataProvider;
class DEXAdapter;
class PriceFeed;

// Callback types
using OrderBookCallback = std::function<void(const OrderBook&)>;
using PriceCallback = std::function<void(const String& token, const Decimal& price)>;
using MarketUpdateCallback = std::function<void(const MarketPair& pair)>;

// Market Data Provider interface
class IMarketDataProvider {
public:
    virtual ~IMarketDataProvider() = default;
    
    // Initialization
    virtual bool initialize(const Config& config) = 0;
    virtual void shutdown() = 0;
    
    // Market data subscription
    virtual bool subscribe_to_order_book(const MarketPair& pair, OrderBookCallback callback) = 0;
    virtual bool unsubscribe_from_order_book(const MarketPair& pair) = 0;
    virtual bool subscribe_to_price_feed(const String& token, PriceCallback callback) = 0;
    virtual bool unsubscribe_from_price_feed(const String& token) = 0;
    
    // Data retrieval
    virtual std::future<OrderBook> get_order_book(const MarketPair& pair) = 0;
    virtual std::future<Decimal> get_token_price(const String& token) = 0;
    virtual std::future<std::vector<MarketPair>> get_available_markets() = 0;
    virtual std::future<std::vector<Token>> get_supported_tokens() = 0;
    
    // Real-time data
    virtual OrderBook get_latest_order_book(const MarketPair& pair) const = 0;
    virtual Decimal get_latest_price(const String& token) const = 0;
    
    // Connection management
    virtual bool is_connected() const = 0;
    virtual String get_connection_status() const = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
    virtual void clear_error() = 0;
};

// Use IDEXAdapter from blockchain_adapters.h

// Price Feed interface
class IPriceFeed {
public:
    virtual ~IPriceFeed() = default;
    
    // Initialization
    virtual bool initialize(const String& source_url) = 0;
    virtual void shutdown() = 0;
    
    // Price subscription
    virtual bool subscribe_to_token(const String& token, PriceCallback callback) = 0;
    virtual bool unsubscribe_from_token(const String& token) = 0;
    
    // Data retrieval
    virtual std::future<Decimal> get_price(const String& token) = 0;
    virtual Decimal get_latest_price(const String& token) const = 0;
    
    // Connection
    virtual bool is_connected() const = 0;
    virtual String get_source_name() const = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
};

// Concrete implementations

// Raydium DEX Adapter
class RaydiumAdapter : public IDEXAdapter {
public:
    RaydiumAdapter(std::shared_ptr<ISolanaRPCClient> rpc_client);
    ~RaydiumAdapter() override;
    
    // IDEXAdapter implementation
    bool connect() override;
    void disconnect() override;
    bool is_connected() const override;
    
    std::future<OrderBook> get_order_book(const MarketPair& pair) override;
    std::future<Decimal> get_token_price(const String& token_address) override;
    std::future<std::vector<MarketPair>> get_markets() override;
    
    std::future<Order> place_order(const MarketPair& pair, OrderSide side, 
                                  OrderType type, const Decimal& size, 
                                  const Decimal& price = Decimal()) override;
    std::future<bool> cancel_order(const String& order_id) override;
    std::future<Order> get_order(const String& order_id) override;
    std::future<std::vector<Order>> get_open_orders() override;
    
    std::future<Decimal> get_balance(const String& token_address) override;
    std::future<std::vector<Trade>> get_trades(const MarketPair& pair, 
                                               const Timestamp& since = Timestamp()) override;
    
    String get_dex_name() const { return "Raydium"; }
    String get_dex_version() const { return "1.0"; }
    bool supports_trading() const { return true; }
    
    String get_last_error() const override;

private:
    std::shared_ptr<ISolanaRPCClient> rpc_client_;
    bool connected_;
    String last_error_;
    
    // Raydium-specific constants
    static const String RAYDIUM_PROGRAM_ID;
    static const String SERUM_PROGRAM_ID;
    
    String build_swap_instruction(const MarketPair& pair, OrderSide side, 
                                 const Decimal& amount, const Decimal& min_amount_out);
};

// Orca DEX Adapter
class OrcaAdapter : public IDEXAdapter {
public:
    OrcaAdapter(std::shared_ptr<ISolanaRPCClient> rpc_client);
    ~OrcaAdapter() override;
    
    // IDEXAdapter implementation
    bool connect() override;
    void disconnect() override;
    bool is_connected() const override;
    
    std::future<OrderBook> get_order_book(const MarketPair& pair) override;
    std::future<Decimal> get_token_price(const String& token_address) override;
    std::future<std::vector<MarketPair>> get_markets() override;
    
    std::future<Order> place_order(const MarketPair& pair, OrderSide side, 
                                  OrderType type, const Decimal& size, 
                                  const Decimal& price = Decimal()) override;
    std::future<bool> cancel_order(const String& order_id) override;
    std::future<Order> get_order(const String& order_id) override;
    std::future<std::vector<Order>> get_open_orders() override;
    
    std::future<Decimal> get_balance(const String& token_address) override;
    std::future<std::vector<Trade>> get_trades(const MarketPair& pair, 
                                               const Timestamp& since = Timestamp()) override;
    
    String get_dex_name() const { return "Orca"; }
    String get_dex_version() const { return "1.0"; }
    bool supports_trading() const { return true; }
    
    String get_last_error() const override;

private:
    std::shared_ptr<ISolanaRPCClient> rpc_client_;
    bool connected_;
    String last_error_;
    
    // Orca-specific constants
    static const String ORCA_PROGRAM_ID;
    
    String build_swap_instruction(const MarketPair& pair, OrderSide side, 
                                 const Decimal& amount, const Decimal& min_amount_out);
};

// CoinGecko Price Feed
class CoinGeckoPriceFeed : public IPriceFeed {
public:
    CoinGeckoPriceFeed();
    ~CoinGeckoPriceFeed() override;
    
    // IPriceFeed implementation
    bool initialize(const String& source_url) override;
    void shutdown() override;
    
    bool subscribe_to_token(const String& token, PriceCallback callback) override;
    bool unsubscribe_from_token(const String& token) override;
    
    std::future<Decimal> get_price(const String& token) override;
    Decimal get_latest_price(const String& token) const override;
    
    bool is_connected() const override;
    String get_source_name() const override { return "CoinGecko"; }
    
    String get_last_error() const override;

private:
    String base_url_;
    bool connected_;
    String last_error_;
    std::map<String, Decimal> latest_prices_;
    std::map<String, PriceCallback> callbacks_;
    mutable std::mutex prices_mutex_;
    
    std::thread update_thread_;
    std::atomic<bool> running_;
    std::condition_variable cv_;
    
    void update_prices_loop();
    String make_api_request(const String& endpoint);
};

// Main Market Data Provider
class MarketDataProvider : public IMarketDataProvider {
public:
    MarketDataProvider();
    ~MarketDataProvider() override;
    
    // IMarketDataProvider implementation
    bool initialize(const Config& config) override;
    void shutdown() override;
    
    bool subscribe_to_order_book(const MarketPair& pair, OrderBookCallback callback) override;
    bool unsubscribe_from_order_book(const MarketPair& pair) override;
    bool subscribe_to_price_feed(const String& token, PriceCallback callback) override;
    bool unsubscribe_from_price_feed(const String& token) override;
    
    std::future<OrderBook> get_order_book(const MarketPair& pair) override;
    std::future<Decimal> get_token_price(const String& token) override;
    std::future<std::vector<MarketPair>> get_available_markets() override;
    std::future<std::vector<Token>> get_supported_tokens() override;
    
    OrderBook get_latest_order_book(const MarketPair& pair) const override;
    Decimal get_latest_price(const String& token) const override;
    
    bool is_connected() const override;
    String get_connection_status() const override;
    
    String get_last_error() const override;
    void clear_error() override;

private:
    Config config_;
    std::shared_ptr<ISolanaRPCClient> rpc_client_;
    std::vector<std::unique_ptr<IDEXAdapter>> dex_adapters_;
    std::unique_ptr<IPriceFeed> price_feed_;
    
    std::map<MarketPair, OrderBook> latest_order_books_;
    std::map<String, Decimal> latest_prices_;
    std::map<MarketPair, OrderBookCallback> order_book_callbacks_;
    std::map<String, PriceCallback> price_callbacks_;
    
    mutable std::mutex data_mutex_;
    String last_error_;
    
    std::thread update_thread_;
    std::atomic<bool> running_;
    std::condition_variable cv_;
    
    void initialize_dex_adapters();
    void update_data_loop();
    void notify_order_book_subscribers(const MarketPair& pair, const OrderBook& order_book);
    void notify_price_subscribers(const String& token, const Decimal& price);
};

// Factory functions
std::unique_ptr<IMarketDataProvider> create_market_data_provider();
std::unique_ptr<IDEXAdapter> create_raydium_adapter(std::shared_ptr<ISolanaRPCClient> rpc_client);
std::unique_ptr<IDEXAdapter> create_orca_adapter(std::shared_ptr<ISolanaRPCClient> rpc_client);
std::unique_ptr<IPriceFeed> create_coingecko_price_feed();

} // namespace solana_arbitrage
