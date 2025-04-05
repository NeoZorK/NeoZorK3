// include/ui.h
#ifndef NEOZORK3_UI_H
#define NEOZORK3_UI_H

#include <string>   // Needed for std::string arguments/members
#include <iostream> // Needed for std::cout
#include <iomanip>  // Needed for std::fixed, std::setprecision
#include <optional> // Potentially needed if print functions handle optional args

// --- Forward Declarations ---
// Preliminary forward declarations
namespace neozork::config_manager {
struct struct_blockchain_info;
struct struct_endpoint;
} // namespace neozork::config_manager

namespace neozork::ui {

// --- ANSI Colors Namespace  ---
namespace colors {
const std::string reset = "\033[0m";
const std::string black = "\033[30m";
const std::string red = "\033[31m";
const std::string green = "\033[32m";
const std::string yellow = "\033[33m";
const std::string blue = "\033[34m";
const std::string magenta = "\033[35m";
const std::string cyan = "\033[36m";
const std::string white = "\033[37m";
const std::string bright_black = "\033[90m";
const std::string bright_red = "\033[91m";
const std::string bright_green = "\033[92m";
const std::string bright_yellow = "\033[93m";
const std::string bright_blue = "\033[94m";
const std::string bright_magenta = "\033[95m";
const std::string bright_cyan = "\033[96m";
const std::string bright_white = "\033[97m";
const std::string bold_red = "\033[1;31m";
const std::string bold_yellow = "\033[1;33m";
const std::string bold_blue = "\033[1;34m";
const std::string bold_cyan = "\033[1;36m";
}

// --- Functions for colored output  ---
// Dependent only from <iostream>, <string>, <iomanip>, <optional>
inline void print_blockchain_info(const std::string& text) {
    std::cout << colors::bold_blue << text << colors::reset;
}
inline void print_url(const std::string& text) {
    std::cout << colors::yellow << text << colors::reset;
}
inline void print_latency(const std::string& text) {
    std::cout << colors::red << text << colors::reset;
}
inline void print_latency(double value_ms) {
    std::cout << colors::red << std::fixed << std::setprecision(2) << value_ms << " ms" << colors::reset;
}
inline void print_connection_type(const std::string& text) {
    std::cout << colors::bold_cyan << "[" << text << "]" << colors::reset;
}
inline void print_status(bool is_active) {
    if (is_active) {
        std::cout << colors::bright_green << "Active" << colors::reset;
    } else {
        std::cout << colors::bright_red << "Inactive" << colors::reset;
    }
}
inline void print_label(const std::string& text) {
    std::cout << colors::white << text << colors::reset;
}
// Overloaded print_value
inline void print_value(const std::string& text) {
    std::cout << colors::bright_white << text << colors::reset;
}
inline void print_value(const char* text) { // Добавим для const char*
    std::cout << colors::bright_white << text << colors::reset;
}
inline void print_value(int value) {
    std::cout << colors::bright_white << value << colors::reset;
}
inline void print_value(long long value) {
    std::cout << colors::bright_white << value << colors::reset;
}
inline void print_value(double value, int precision = 2) {
    std::cout << colors::bright_white << std::fixed << std::setprecision(precision) << value << colors::reset;
}
// Add overload for optional
inline void print_value(const std::optional<std::string>& opt_str) {
    if (opt_str) {
        print_value(*opt_str);
    } else {
        std::cout << colors::bright_black << "N/A" << colors::reset;
    }
}
inline void print_value(const std::optional<long long>& opt_ll) {
    if (opt_ll) {
        print_value(*opt_ll);
    } else {
        std::cout << colors::bright_black << "N/A" << colors::reset;
    }
}
inline void print_value(const std::optional<int>& opt_int) {
    if (opt_int) {
        print_value(*opt_int);
    } else {
        std::cout << colors::bright_black << "N/A" << colors::reset;
    }
}
inline void print_value(const std::optional<double>& opt_double, int precision = 2) {
    if (opt_double) {
        print_value(*opt_double, precision);
    } else {
        std::cout << colors::bright_black << "N/A" << colors::reset;
    }
}


// --- Print Endpoint Details ---
void print_endpoint_details(
                            const neozork::config_manager::struct_blockchain_info& bc_info,
                            const neozork::config_manager::struct_endpoint& endpoint
                            );

// --- Progress Bar Utility (Placeholder) ---
// TODO: Implement later

} // namespace neozork::ui

#endif // NEOZORK3_UI_H
