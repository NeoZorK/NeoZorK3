# Минимальная версия CMake
cmake_minimum_required(VERSION 3.15)

# Определение проекта
project(NeoZorK3 LANGUAGES CXX)

# Стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Опции Сборки ---
option(NEOZORK3_LINK_STATIC_RUNTIME "Attempt to link C++ runtime statically" OFF)
# Заметка: Результат зависит от компилятора и ОС.
# На Linux с GCC/Clang часто используется флаг -static-libstdc++.
# На Alpine/musl может потребоваться флаг -static для полной статической линковки.
# На macOS статическая линковка libc++ не рекомендуется.
# На Windows (MSVC) это управляется флагами /MT vs /MD.

message(STATUS "Link C++ runtime statically: ${NEOZORK3_LINK_STATIC_RUNTIME}")

# --- Зависимости (FetchContent) ---
# Скачивает и настраивает библиотеки во время выполнения cmake ..
include(FetchContent)

FetchContent_Declare(
  nlohmann_json # Имя для CMake
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3 # Конкретная версия для стабильности
)
FetchContent_MakeAvailable(nlohmann_json) # Скачать и сделать доступной

FetchContent_Declare(
  cpp_httplib # Имя для CMake
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3 # Конкретная версия
)
FetchContent_MakeAvailable(cpp_httplib) # Скачать и сделать доступной

# Опционально: Поиск OpenSSL для HTTPS в httplib
# find_package() пытается найти библиотеку, установленную в системе
# Для FetchContent-библиотек он обычно не нужен, если они сами не требуют
find_package(OpenSSL QUIET) # QUIET - не выводить ошибку, если не найдено

# --- Определение пути вывода бинарников ---
# ${CMAKE_SOURCE_DIR} - это корневая папка проекта (где лежит этот CMakeLists.txt)
set(NEOZORK3_BIN_BASE_DIR ${CMAKE_SOURCE_DIR}/bin)

# Определяем подпапку в bin/ в зависимости от ОС и архитектуры
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        set(PLATFORM_BIN_DIR ${NEOZORK3_BIN_BASE_DIR}/linux)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
        set(PLATFORM_BIN_DIR ${NEOZORK3_BIN_BASE_DIR}/linux_arm)
    else()
        set(PLATFORM_BIN_DIR ${NEOZORK3_BIN_BASE_DIR}/linux_other)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin") # macOS
    set(PLATFORM_BIN_DIR ${NEOZORK3_BIN_BASE_DIR}/macos)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_BIN_DIR ${NEOZORK3_BIN_BASE_DIR}/win)
else()
    set(PLATFORM_BIN_DIR ${NEOZORK3_BIN_BASE_DIR}/other)
endif()

# Устанавливаем папку вывода для исполняемых файлов
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PLATFORM_BIN_DIR})
message(STATUS "Binary output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# --- Определение исполняемого файла ---
add_executable(neozork3_cli
    src/main.cpp
    # Добавляйте сюда другие ваши .cpp файлы из папки src/
    # src/another_source.cpp
)

# --- Указание папок с заголовочными файлами ---
# Где искать #include "..." и #include <...>
target_include_directories(neozork3_cli PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    # ${nlohmann_json_SOURCE_DIR}/include # <-- ЗАКОММЕНТИРУЙТЕ ИЛИ УДАЛИТЕ ЭТУ СТРОКУ
    ${cpp_httplib_SOURCE_DIR}
)

# --- Управление Линковкой C++ Runtime ---
if(NEOZORK3_LINK_STATIC_RUNTIME)
    # Пытаемся добавить флаги для статической линковки C++ runtime
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Флаг для GCC/Clang на Linux
        # Для полной статической сборки на musl может понадобиться "-static"
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
        message(STATUS "Attempting static linkage of libstdc++ (or libc++ if Clang)")
    elseif(MSVC)
        message(WARNING "Static runtime linking requested for MSVC - ensure /MT flag is used (e.g., via CMAKE_MSVC_RUNTIME_LIBRARY or toolchain).")
    else()
        message(WARNING "Static runtime linking requested but no specific flags set for this compiler/platform.")
    endif()
endif()

# --- Линковка Библиотек ---
# Связывание скомпилированного кода с библиотеками
target_link_libraries(neozork3_cli PRIVATE
    nlohmann_json::nlohmann_json # <--- ДОБАВЬТЕ ЭТУ СТРОКУ

    # ... остальная часть секции (проверка OpenSSL и т.д.) ...
    # if(OpenSSL_FOUND AND TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    #   target_link_libraries(neozork3_cli PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    # endif()
)

# --- Опционально: Компиляционные определения ---
# Установка макросов препроцессора (например, -DNAME=VALUE)
# if(OpenSSL_FOUND)
#   # Этот макрос включает код поддержки SSL в cpp-httplib
#   target_compile_definitions(neozork3_cli PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
#   message(STATUS "Enabled OpenSSL support in cpp-httplib")
# endif()