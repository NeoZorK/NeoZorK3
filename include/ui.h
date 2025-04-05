// include/ui.h
#ifndef NEOZORK3_UI_H
#define NEOZORK3_UI_H

#include <string>
#include <iostream>

namespace neozork::ui {

// ANSI Color Codes Namespace
namespace colors {
// Basic Colors
const std::string reset = "\033[0m";
const std::string black = "\033[30m";
const std::string red = "\033[31m";
const std::string green = "\033[32m";
const std::string yellow = "\033[33m";
const std::string blue = "\033[34m";
const std::string magenta = "\033[35m";
const std::string cyan = "\033[36m";
const std::string white = "\033[37m";

// Bright Colors
const std::string bright_black = "\033[90m"; // Often gray
const std::string bright_red = "\033[91m";
const std::string bright_green = "\033[92m";
const std::string bright_yellow = "\033[93m";
const std::string bright_blue = "\033[94m";
const std::string bright_magenta = "\033[95m";
const std::string bright_cyan = "\033[96m";
const std::string bright_white = "\033[97m";

// Bold variations (Example)
const std::string bold_red = "\033[1;31m";
const std::string bold_yellow = "\033[1;33m";
const std::string bold_blue = "\033[1;34m";
const std::string bold_cyan = "\033[1;36m";

// Background colors (Example)
// const std::string bg_red = "\033[41m";
}

// --- Functions for colored output ---

// Example: Print blockchain info in blue
inline void print_blockchain_info(const std::string& text) {
    std::cout << colors::bold_blue << text << colors::reset;
}

// Example: Print URL in yellow
inline void print_url(const std::string& text) {
    std::cout << colors::yellow << text << colors::reset;
}

// Example: Print latency/speed in red
inline void print_latency(const std::string& text) {
    std::cout << colors::red << text << colors::reset;
}
inline void print_latency(double value_ms) {
    std::cout << colors::red << std::fixed << std::setprecision(2) << value_ms << " ms" << colors::reset;
}

// Example: Print connection type in cyan
inline void print_connection_type(const std::string& text) {
    std::cout << colors::bold_cyan << "[" << text << "]" << colors::reset;
}

// Example: Print status (Active/Inactive)
inline void print_status(bool is_active) {
    if (is_active) {
        std::cout << colors::bright_green << "Active" << colors::reset;
    } else {
        std::cout << colors::bright_red << "Inactive" << colors::reset;
    }
}

// Example: Print a label/key in white/default
inline void print_label(const std::string& text) {
    std::cout << colors::white << text << colors::reset;
}
// Example: Print a value associated with a label
inline void print_value(const std::string& text) {
    std::cout << colors::bright_white << text << colors::reset;
}
inline void print_value(long long value) {
    std::cout << colors::bright_white << value << colors::reset;
}
inline void print_value(double value, int precision = 2) {
    std::cout << colors::bright_white << std::fixed << std::setprecision(precision) << value << colors::reset;
}

// Function to print endpoint details (will be used by commands)
// Forward declare config structs to avoid full include here? Or include config_manager.h
// Let's include it for now for simplicity.
#include "config_manager.h"
#include <iomanip> // For setprecision

void print_endpoint_details(
                            const neozork::config_manager::struct_blockchain_info& bc_info,
                            const neozork::config_manager::struct_endpoint& endpoint
                            );


} // namespace neozork::ui

#endif // NEOZORK3_UI_H
