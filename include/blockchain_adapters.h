#pragma once

#include "types.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <functional>
#include <future>

namespace solana_arbitrage {

// Forward declarations
class SolanaRPCClient;
class WalletManager;
class TransactionBuilder;

// Callback types
using OrderCallback = std::function<void(const Order&, bool success, const String& error)>;
using BalanceCallback = std::function<void(const String& token, const Decimal& balance)>;
using TransactionCallback = std::function<void(const String& tx_hash, bool success, const String& error)>;

// Solana RPC Client interface
class ISolanaRPCClient {
public:
    virtual ~ISolanaRPCClient() = default;
    
    // Connection management
    virtual bool connect(const String& endpoint) = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;
    
    // Account information
    virtual std::future<Decimal> get_balance(const String& address) = 0;
    virtual std::future<std::vector<Token>> get_token_accounts(const String& address) = 0;
    virtual std::future<Decimal> get_token_balance(const String& address, const String& token_mint) = 0;
    
    // Transaction management
    virtual std::future<String> send_transaction(const String& transaction) = 0;
    virtual std::future<bool> confirm_transaction(const String& signature, int max_retries = 10) = 0;
    virtual std::future<bool> is_transaction_confirmed(const String& signature) = 0;
    
    // Block information
    virtual std::future<uint64_t> get_latest_blockhash() = 0;
    virtual std::future<uint64_t> get_slot() = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
    virtual void clear_error() = 0;
};

// Wallet Manager interface
class IWalletManager {
public:
    virtual ~IWalletManager() = default;
    
    // Wallet management
    virtual bool load_wallet(const String& private_key) = 0;
    virtual bool create_wallet() = 0;
    virtual String get_public_key() const = 0;
    virtual bool is_wallet_loaded() const = 0;
    
    // Signing
    virtual String sign_transaction(const String& transaction) = 0;
    virtual String sign_message(const String& message) = 0;
    
    // Security
    virtual void clear_private_key() = 0;
    virtual bool export_wallet(String& private_key) const = 0;
};

// Transaction Builder interface
class ITransactionBuilder {
public:
    virtual ~ITransactionBuilder() = default;
    
    // Transaction creation
    virtual void create_transaction() = 0;
    virtual void add_instruction(const String& instruction) = 0;
    virtual void set_recent_blockhash(const String& blockhash) = 0;
    virtual void set_fee_payer(const String& fee_payer) = 0;
    
    // Token transfers
    virtual String build_token_transfer(const String& from, const String& to, 
                                       const String& token_mint, const Decimal& amount) = 0;
    virtual String build_swap_instruction(const MarketPair& pair, OrderSide side, 
                                         const Decimal& amount, const Decimal& min_amount_out) = 0;
    
    // Transaction serialization
    virtual String serialize() const = 0;
    virtual bool deserialize(const String& transaction) = 0;
    
    // Validation
    virtual bool validate() const = 0;
    virtual String get_validation_error() const = 0;
};

// DEX Adapter interface
class IDEXAdapter {
public:
    virtual ~IDEXAdapter() = default;
    
    // Market data
    virtual std::future<OrderBook> get_order_book(const MarketPair& pair) = 0;
    virtual std::future<Decimal> get_token_price(const String& token_address) = 0;
    virtual std::future<std::vector<MarketPair>> get_markets() = 0;
    
    // Trading
    virtual std::future<Order> place_order(const MarketPair& pair, OrderSide side, 
                                          OrderType type, const Decimal& size, 
                                          const Decimal& price = Decimal()) = 0;
    virtual std::future<bool> cancel_order(const String& order_id) = 0;
    virtual std::future<Order> get_order(const String& order_id) = 0;
    virtual std::future<std::vector<Order>> get_open_orders() = 0;
    
    // Account information
    virtual std::future<Decimal> get_balance(const String& token_address) = 0;
    virtual std::future<std::vector<Trade>> get_trades(const MarketPair& pair, 
                                                       const Timestamp& since = Timestamp()) = 0;
    
    // Connection
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
};

// Concrete implementations
class SolanaRPCClient : public ISolanaRPCClient {
public:
    SolanaRPCClient(boost::asio::io_context& io_context);
    ~SolanaRPCClient() override;
    
    // ISolanaRPCClient implementation
    bool connect(const String& endpoint) override;
    void disconnect() override;
    bool is_connected() const override;
    
    std::future<Decimal> get_balance(const String& address) override;
    std::future<std::vector<Token>> get_token_accounts(const String& address) override;
    std::future<Decimal> get_token_balance(const String& address, const String& token_mint) override;
    
    std::future<String> send_transaction(const String& transaction) override;
    std::future<bool> confirm_transaction(const String& signature, int max_retries = 10) override;
    std::future<bool> is_transaction_confirmed(const String& signature) override;
    
    std::future<uint64_t> get_latest_blockhash() override;
    std::future<uint64_t> get_slot() override;
    
    String get_last_error() const override;
    void clear_error() override;

private:
    boost::asio::io_context& io_context_;
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
    String endpoint_;
    String last_error_;
    bool connected_;
    
    String make_rpc_request(const String& method, const String& params);
    String parse_rpc_response(const String& response);
};

class WalletManager : public IWalletManager {
public:
    WalletManager();
    ~WalletManager() override;
    
    // IWalletManager implementation
    bool load_wallet(const String& private_key) override;
    bool create_wallet() override;
    String get_public_key() const override;
    bool is_wallet_loaded() const override;
    
    String sign_transaction(const String& transaction) override;
    String sign_message(const String& message) override;
    
    void clear_private_key() override;
    bool export_wallet(String& private_key) const override;

private:
    String private_key_;
    String public_key_;
    bool wallet_loaded_;
    
    String derive_public_key(const String& private_key);
};

class TransactionBuilder : public ITransactionBuilder {
public:
    TransactionBuilder();
    ~TransactionBuilder() override;
    
    // ITransactionBuilder implementation
    void create_transaction() override;
    void add_instruction(const String& instruction) override;
    void set_recent_blockhash(const String& blockhash) override;
    void set_fee_payer(const String& fee_payer) override;
    
    String build_token_transfer(const String& from, const String& to, 
                               const String& token_mint, const Decimal& amount) override;
    String build_swap_instruction(const MarketPair& pair, OrderSide side, 
                                 const Decimal& amount, const Decimal& min_amount_out) override;
    
    String serialize() const override;
    bool deserialize(const String& transaction) override;
    
    bool validate() const override;
    String get_validation_error() const override;

private:
    struct TransactionData {
        String recent_blockhash;
        String fee_payer;
        std::vector<String> instructions;
        std::vector<String> signatures;
    };
    
    TransactionData transaction_;
    String validation_error_;
    
    String encode_instruction(const String& program_id, const std::vector<String>& accounts, 
                             const String& data);
};

// Factory functions
std::unique_ptr<ISolanaRPCClient> create_solana_rpc_client(boost::asio::io_context& io_context);
std::unique_ptr<IWalletManager> create_wallet_manager();
std::unique_ptr<ITransactionBuilder> create_transaction_builder();

} // namespace solana_arbitrage
