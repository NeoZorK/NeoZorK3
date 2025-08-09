#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <optional>

namespace solana_arbitrage {

// Basic types
using String = std::string;
using Timestamp = std::chrono::system_clock::time_point;
using Duration = std::chrono::milliseconds;

// Decimal representation for precise calculations
struct Decimal {
    int64_t value;  // Value * 10^precision
    int precision;  // Number of decimal places
    
    Decimal(int64_t val = 0, int prec = 8) : value(val), precision(prec) {}
    
    double to_double() const {
        return static_cast<double>(value) / std::pow(10, precision);
    }
    
    static Decimal from_double(double val, int prec = 8) {
        return Decimal(static_cast<int64_t>(val * std::pow(10, prec)), prec);
    }
};

// Token information
struct Token {
    String address;
    String symbol;
    String name;
    int decimals;
    Decimal price_usd;
    
    Token(const String& addr = "", const String& sym = "", const String& nm = "", 
          int dec = 8, Decimal price = Decimal())
        : address(addr), symbol(sym), name(nm), decimals(dec), price_usd(price) {}
};

// Market pair
struct MarketPair {
    String base_token;
    String quote_token;
    String market_address;
    String dex_name;
    
    MarketPair(const String& base = "", const String& quote = "", 
               const String& market = "", const String& dex = "")
        : base_token(base), quote_token(quote), market_address(market), dex_name(dex) {}
    
    bool operator==(const MarketPair& other) const {
        return base_token == other.base_token && quote_token == other.quote_token &&
               market_address == other.market_address && dex_name == other.dex_name;
    }
    
    bool operator<(const MarketPair& other) const {
        if (base_token != other.base_token) return base_token < other.base_token;
        if (quote_token != other.quote_token) return quote_token < other.quote_token;
        if (market_address != other.market_address) return market_address < other.market_address;
        return dex_name < other.dex_name;
    }
};

// Order book entry
struct OrderBookEntry {
    Decimal price;
    Decimal size;
    Timestamp timestamp;
    
    OrderBookEntry(Decimal p = Decimal(), Decimal s = Decimal(), Timestamp ts = std::chrono::system_clock::now())
        : price(p), size(s), timestamp(ts) {}
};

// Order book
struct OrderBook {
    MarketPair pair;
    std::vector<OrderBookEntry> bids;
    std::vector<OrderBookEntry> asks;
    Timestamp timestamp;
    
    OrderBook(const MarketPair& p = MarketPair()) : pair(p), timestamp(std::chrono::system_clock::now()) {}
    
    Decimal get_best_bid() const {
        return bids.empty() ? Decimal() : bids.front().price;
    }
    
    Decimal get_best_ask() const {
        return asks.empty() ? Decimal() : asks.front().price;
    }
    
    Decimal get_spread() const {
        auto bid = get_best_bid();
        auto ask = get_best_ask();
        if (bid.value == 0 || ask.value == 0) return Decimal();
        return Decimal(ask.value - bid.value, ask.precision);
    }
};

// Arbitrage opportunity
struct ArbitrageOpportunity {
    MarketPair buy_market;
    MarketPair sell_market;
    Decimal buy_price;
    Decimal sell_price;
    Decimal profit;
    Decimal profit_percentage;
    Decimal max_trade_size;
    Timestamp timestamp;
    
    ArbitrageOpportunity(const MarketPair& buy = MarketPair(), const MarketPair& sell = MarketPair())
        : buy_market(buy), sell_market(sell), timestamp(std::chrono::system_clock::now()) {}
    
    bool is_profitable() const {
        return profit.value > 0 && profit_percentage.to_double() > 0.1; // 0.1% minimum
    }
};

// Order types
enum class OrderType {
    MARKET,
    LIMIT,
    STOP_LOSS,
    TAKE_PROFIT
};

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderStatus {
    PENDING,
    FILLED,
    CANCELLED,
    REJECTED,
    PARTIALLY_FILLED
};

// Order
struct Order {
    String id;
    MarketPair pair;
    OrderType type;
    OrderSide side;
    Decimal price;
    Decimal size;
    OrderStatus status;
    Timestamp created_at;
    Timestamp updated_at;
    
    Order(const String& order_id = "", const MarketPair& p = MarketPair())
        : id(order_id), pair(p), type(OrderType::MARKET), side(OrderSide::BUY),
          status(OrderStatus::PENDING), created_at(std::chrono::system_clock::now()),
          updated_at(std::chrono::system_clock::now()) {}
};

// Trade
struct Trade {
    String id;
    String order_id;
    MarketPair pair;
    OrderSide side;
    Decimal price;
    Decimal size;
    Decimal fee;
    Timestamp timestamp;
    
    Trade(const String& trade_id = "", const String& ord_id = "", const MarketPair& p = MarketPair())
        : id(trade_id), order_id(ord_id), pair(p), side(OrderSide::BUY),
          timestamp(std::chrono::system_clock::now()) {}
};

// Configuration
struct Config {
    String rpc_endpoint;
    String wallet_private_key;
    Decimal max_trade_size;
    Decimal min_profit_percentage;
    int max_concurrent_orders;
    Duration order_timeout;
    bool dry_run;
    
    Config() : max_trade_size(Decimal::from_double(100.0)), min_profit_percentage(Decimal::from_double(0.5)),
               max_concurrent_orders(5), order_timeout(Duration(30000)), dry_run(true) {}
};

// Risk limits
struct RiskLimits {
    Decimal max_position_size;
    Decimal max_daily_loss;
    Decimal max_drawdown;
    int max_orders_per_minute;
    
    RiskLimits() : max_position_size(Decimal::from_double(1000.0)), 
                   max_daily_loss(Decimal::from_double(100.0)),
                   max_drawdown(Decimal::from_double(10.0)), max_orders_per_minute(10) {}
};

} // namespace solana_arbitrage
