#include "windows_compat.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <wincrypt.h>

// Windows-specific initialization
namespace neozork {
namespace windows {

bool initialize_windows_sockets() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        return false;
    }
    return true;
}

void cleanup_windows_sockets() {
    WSACleanup();
}

std::string get_windows_error_message() {
    DWORD error = GetLastError();
    if (error == 0) {
        return "No error";
    }

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        NULL
    );

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    
    return message;
}

bool is_windows_vista_or_later() {
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
    #pragma warning(disable:4996)
    GetVersionEx(&osvi);
    #pragma warning(default:4996)
    
    return (osvi.dwMajorVersion >= 6);
}

std::string get_windows_version() {
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
    #pragma warning(disable:4996)
    GetVersionEx(&osvi);
    #pragma warning(default:4996)
    
    return std::to_string(osvi.dwMajorVersion) + "." + 
           std::to_string(osvi.dwMinorVersion) + "." + 
           std::to_string(osvi.dwBuildNumber);
}

} // namespace windows
} // namespace neozork

// Global Windows initialization
class WindowsInitializer {
public:
    WindowsInitializer() {
        neozork::windows::initialize_windows_sockets();
    }
    
    ~WindowsInitializer() {
        neozork::windows::cleanup_windows_sockets();
    }
};

// Global instance for automatic initialization/cleanup
static WindowsInitializer g_windows_init;

#endif // _WIN32
