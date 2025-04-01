#ifndef NEOZORK3_CONNECTION_MANAGER_HPP
#define NEOZORK3_CONNECTION_MANAGER_HPP

#include <string>
#include <vector>
#include <optional> // Может быть полезно для возврата данных

// Включаем httplib.h здесь, так как этот модуль будет его использовать
// Убедитесь, что пути к include для него настроены в CMakeLists.txt
// для цели neozork3_connection_manager (уже сделано)
#define CPPHTTPLIB_OPENSSL_SUPPORT // Включить поддержку HTTPS, если OpenSSL доступен
// #define CPPHTTPLIB_ZLIB_SUPPORT // Включить поддержку сжатия, если zlib доступен
#include <httplib.h>

namespace neozork::connection {

    // TODO: Определить класс или функции для управления соединениями
    // class ConnectionManager {
    // public:
    //     std::optional<std::string> http_get(const std::string& host, const std::string& path);
    //     // ... другие методы для POST, WS и т.д.
    // };

} // namespace neozork::connection

#endif // NEOZORK3_CONNECTION_MANAGER_HPP
