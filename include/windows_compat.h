#pragma once

#include <string>

#ifdef _WIN32

namespace neozork {
namespace windows {

// Windows-specific functions
bool initialize_windows_sockets();
void cleanup_windows_sockets();
std::string get_windows_error_message();
bool is_windows_vista_or_later();
std::string get_windows_version();

} // namespace windows
} // namespace neozork

#endif // _WIN32
