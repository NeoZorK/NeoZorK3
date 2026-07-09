#pragma once

#include "types.h"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>

namespace solana_arbitrage {

// Forward declarations
class ConfigManager;

// Configuration validation callback
using ConfigValidationCallback = std::function<bool(const Config&, String& error)>;

// Configuration Manager interface
class IConfigManager {
public:
    virtual ~IConfigManager() = default;
    
    // Configuration management
    virtual bool load_config(const String& config_file) = 0;
    virtual bool save_config(const String& config_file) = 0;
    virtual bool validate_config(const Config& config) = 0;
    
    // Configuration access
    virtual Config get_config() const = 0;
    virtual void update_config(const Config& config) = 0;
    virtual void set_config_value(const String& key, const String& value) = 0;
    virtual String get_config_value(const String& key) const = 0;
    
    // Configuration validation
    virtual bool add_validation_callback(ConfigValidationCallback callback) = 0;
    virtual void remove_validation_callback() = 0;
    
    // Configuration templates
    virtual Config create_default_config() = 0;
    virtual Config create_test_config() = 0;
    virtual Config create_production_config() = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
    virtual void clear_error() = 0;
};

// Main Configuration Manager
class ConfigManager : public IConfigManager {
public:
    ConfigManager();
    ~ConfigManager() override;
    
    // IConfigManager implementation
    bool load_config(const String& config_file) override;
    bool save_config(const String& config_file) override;
    bool validate_config(const Config& config) override;
    
    Config get_config() const override;
    void update_config(const Config& config) override;
    void set_config_value(const String& key, const String& value) override;
    String get_config_value(const String& key) const override;
    
    bool add_validation_callback(ConfigValidationCallback callback) override;
    void remove_validation_callback() override;
    
    Config create_default_config() override;
    Config create_test_config() override;
    Config create_production_config() override;
    
    String get_last_error() const override;
    void clear_error() override;

private:
    Config config_;
    ConfigValidationCallback validation_callback_;
    String last_error_;
    mutable std::mutex config_mutex_;
    
    bool parse_config_file(const String& content);
    bool serialize_config(const Config& config, String& content);
    bool validate_rpc_endpoint(const String& endpoint);
    bool validate_wallet_private_key(const String& private_key);
    bool validate_decimal_value(const String& value);
    bool validate_integer_value(const String& value);
};

// Factory function
std::unique_ptr<IConfigManager> create_config_manager();

} // namespace solana_arbitrage
