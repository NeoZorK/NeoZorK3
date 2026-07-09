#pragma once

#include "types.h"
#include <string>
#include <memory>
#include <functional>
#include <mutex>

namespace solana_arbitrage {

// Forward declarations
class Logger;

// Log levels
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    CRITICAL = 5
};

// Log callback
using LogCallback = std::function<void(LogLevel level, const String& message, const Timestamp& timestamp)>;

// Logger interface
class ILogger {
public:
    virtual ~ILogger() = default;
    
    // Logging methods
    virtual void trace(const String& message) = 0;
    virtual void debug(const String& message) = 0;
    virtual void info(const String& message) = 0;
    virtual void warning(const String& message) = 0;
    virtual void error(const String& message) = 0;
    virtual void critical(const String& message) = 0;
    
    // Log with level
    virtual void log(LogLevel level, const String& message) = 0;
    
    // Configuration
    virtual void set_log_level(LogLevel level) = 0;
    virtual LogLevel get_log_level() const = 0;
    virtual void set_log_file(const String& file_path) = 0;
    virtual void set_max_file_size(size_t max_size) = 0;
    virtual void set_max_files(int max_files) = 0;
    
    // Callback registration
    virtual bool add_log_callback(LogCallback callback) = 0;
    virtual void remove_log_callback() = 0;
    
    // Utility methods
    virtual String level_to_string(LogLevel level) const = 0;
    virtual LogLevel string_to_level(const String& level_str) const = 0;
    virtual bool should_log(LogLevel level) const = 0;
    
    // Error handling
    virtual String get_last_error() const = 0;
    virtual void clear_error() = 0;
};

// Main Logger
class Logger : public ILogger {
public:
    Logger();
    ~Logger() override;
    
    // ILogger implementation
    void trace(const String& message) override;
    void debug(const String& message) override;
    void info(const String& message) override;
    void warning(const String& message) override;
    void error(const String& message) override;
    void critical(const String& message) override;
    
    void log(LogLevel level, const String& message) override;
    
    void set_log_level(LogLevel level) override;
    LogLevel get_log_level() const override;
    void set_log_file(const String& file_path) override;
    void set_max_file_size(size_t max_size) override;
    void set_max_files(int max_files) override;
    
    bool add_log_callback(LogCallback callback) override;
    void remove_log_callback() override;
    
    String level_to_string(LogLevel level) const override;
    LogLevel string_to_level(const String& level_str) const override;
    bool should_log(LogLevel level) const override;
    
    String get_last_error() const override;
    void clear_error() override;

private:
    LogLevel current_level_;
    String log_file_path_;
    size_t max_file_size_;
    int max_files_;
    LogCallback log_callback_;
    String last_error_;
    mutable std::mutex logger_mutex_;
    
    void write_to_file(LogLevel level, const String& message);
    void rotate_log_file();
    String format_log_message(LogLevel level, const String& message);
    String get_timestamp_string(const Timestamp& timestamp);
    void notify_callbacks(LogLevel level, const String& message);
};

// Global logger instance
extern std::unique_ptr<ILogger> g_logger;

// Global logging macros
#define LOG_TRACE(msg) if (g_logger && g_logger->should_log(LogLevel::TRACE)) g_logger->trace(msg)
#define LOG_DEBUG(msg) if (g_logger && g_logger->should_log(LogLevel::DEBUG)) g_logger->debug(msg)
#define LOG_INFO(msg) if (g_logger && g_logger->should_log(LogLevel::INFO)) g_logger->info(msg)
#define LOG_WARNING(msg) if (g_logger && g_logger->should_log(LogLevel::WARNING)) g_logger->warning(msg)
#define LOG_ERROR(msg) if (g_logger && g_logger->should_log(LogLevel::ERROR)) g_logger->error(msg)
#define LOG_CRITICAL(msg) if (g_logger && g_logger->should_log(LogLevel::CRITICAL)) g_logger->critical(msg)

// Factory function
std::unique_ptr<ILogger> create_logger();

// Global logger initialization
bool initialize_global_logger(const String& log_file = "", LogLevel level = LogLevel::INFO);
void shutdown_global_logger();

} // namespace solana_arbitrage
