#include "logger.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace solana_arbitrage {

// Global logger instance
std::unique_ptr<ILogger> g_logger = nullptr;

// Factory function
std::unique_ptr<ILogger> create_logger() {
    return std::make_unique<Logger>();
}

// Logger implementation
Logger::Logger() : current_level_(LogLevel::INFO), max_file_size_(10), max_files_(5) {
    // Constructor implementation
}

Logger::~Logger() {
    // Destructor implementation
}

void Logger::trace(const String& message) {
    log(LogLevel::TRACE, message);
}

void Logger::debug(const String& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const String& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const String& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const String& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const String& message) {
    log(LogLevel::CRITICAL, message);
}

void Logger::log(LogLevel level, const String& message) {
    if (!should_log(level)) return;
    
    String formatted_message = format_log_message(level, message);
    
    // Console output
    std::cout << formatted_message << std::endl;
    
    // File output
    if (!log_file_path_.empty()) {
        write_to_file(level, message);
    }
    
    // Notify callbacks
    notify_callbacks(level, message);
}

void Logger::set_log_level(LogLevel level) {
    current_level_ = level;
}

LogLevel Logger::get_log_level() const {
    return current_level_;
}

void Logger::set_log_file(const String& file_path) {
    log_file_path_ = file_path;
}

void Logger::set_max_file_size(size_t max_size) {
    max_file_size_ = max_size;
}

void Logger::set_max_files(int max_files) {
    max_files_ = max_files;
}

bool Logger::add_log_callback(LogCallback callback) {
    log_callback_ = callback;
    return true;
}

void Logger::remove_log_callback() {
    log_callback_ = nullptr;
}

String Logger::level_to_string(LogLevel level) const {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

LogLevel Logger::string_to_level(const String& level_str) const {
    if (level_str == "TRACE") return LogLevel::TRACE;
    if (level_str == "DEBUG") return LogLevel::DEBUG;
    if (level_str == "INFO") return LogLevel::INFO;
    if (level_str == "WARNING") return LogLevel::WARNING;
    if (level_str == "ERROR") return LogLevel::ERROR;
    if (level_str == "CRITICAL") return LogLevel::CRITICAL;
    return LogLevel::INFO;
}

bool Logger::should_log(LogLevel level) const {
    return static_cast<int>(level) >= static_cast<int>(current_level_);
}

String Logger::get_last_error() const {
    return last_error_;
}

void Logger::clear_error() {
    last_error_.clear();
}

void Logger::write_to_file(LogLevel level, const String& message) {
    // Placeholder implementation
}

void Logger::rotate_log_file() {
    // Placeholder implementation
}

String Logger::format_log_message(LogLevel level, const String& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    ss << " [" << level_to_string(level) << "] " << message;
    
    return ss.str();
}

String Logger::get_timestamp_string(const Timestamp& timestamp) {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void Logger::notify_callbacks(LogLevel level, const String& message) {
    auto now = std::chrono::system_clock::now();
    if (log_callback_) {
        log_callback_(level, message, now);
    }
}

// Initialize global logger
bool initialize_global_logger(const String& log_file, LogLevel level) {
    g_logger = create_logger();
    g_logger->set_log_level(level);
    if (!log_file.empty()) {
        g_logger->set_log_file(log_file);
    }
    return true;
}

void shutdown_global_logger() {
    g_logger.reset();
}

// Get global logger
ILogger* get_global_logger() {
    return g_logger.get();
}

} // namespace solana_arbitrage
