# Minimum required CMake version
cmake_minimum_required(VERSION 3.16)

# Project name
project(NeoZorK3 VERSION 1.0 LANGUAGES CXX)

# Set C++ standard (C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Dependencies ---
include(FetchContent)

# 1. nlohmann_json
FetchContent_Declare(
nlohmann_json
GIT_REPOSITORY https://github.com/nlohmann/json.git
GIT_TAG        v3.11.3 # Use an up-to-date stable tag/commit
)
FetchContent_MakeAvailable(nlohmann_json)
# nlohmann_json::nlohmann_json target is now available but not linked yet

# 2. httplib
FetchContent_Declare(
cpp_httplib
GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
GIT_TAG        v0.15.3 # Use an up-to-date stable tag/commit
)
FetchContent_MakeAvailable(cpp_httplib)

# 3. OpenSSL (support HTTPS in cpp-httplib)
# Searching installed library in system OpenSSL
# (Example, libssl-dev in Debian/Ubuntu, openssl-devel in Fedora/CentOS, openssl in Homebrew for macOS)
find_package(OpenSSL REQUIRED)

# --- Executable ---
# Build main.cpp into neozork3_cli
add_executable(neozork3_cli
src/main.cpp
src/config_manager.cpp
src/cli_parser.cpp
src/connection_manager.cpp
src/endpoint_discovery.cpp
)

# Specify the include path for the executable
target_include_directories(neozork3_cli PUBLIC include)

# ADD PATH to headers nlohmann_json для neozork3_cli
target_include_directories(neozork3_cli PRIVATE
${nlohmann_json_SOURCE_DIR}/include
${cpp_httplib_SOURCE_DIR}
)

# Link OpenSSL
# nlohmann_json and cpp_httplib - header-only, no need to link
target_link_libraries(neozork3_cli PRIVATE
OpenSSL::SSL
OpenSSL::Crypto
)

# --- Compile/Link Options ---

# Shared folders  (warnings)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
target_compile_options(neozork3_cli PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Configuration options $<CONFIG:...>

# Debug: add only debeg symbols
target_compile_options(neozork3_cli PRIVATE $<$<CONFIG:Debug>:-g>)

# Release: high optimisatio (-O3), disable NDEBUG
target_compile_options(neozork3_cli PRIVATE $<$<CONFIG:Release>:-O3 -DNDEBUG>)
# can add delete symbols (strip) for Release, if need less size
# target_link_options(neozork3_cli PRIVATE $<$<CONFIG:Release>:-s>)

# RelWithDebInfo: optimisation (-O2 better for debugging, then O3) + symbols
target_compile_options(neozork3_cli PRIVATE $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>)

# MinSizeRel: size optimisation (-Os), disable NDEBUG, delete syymbols (strip)
target_compile_options(neozork3_cli PRIVATE $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>)
target_link_options(neozork3_cli PRIVATE $<$<CONFIG:MinSizeRel>:-s>)

# --- Options: Link Time Optimization (LTO) ---
# Less size and better performance, but more link time.
# can be added for Release and MinSizeRel.
# target_compile_options(neozork3_cli PRIVATE $<$<CONFIG:Release>:-flto>)
# target_link_options(neozork3_cli PRIVATE $<$<CONFIG:Release>:-flto>)
# target_compile_options(neozork3_cli PRIVATE $<$<CONFIG:MinSizeRel>:-flto>)
# target_link_options(neozork3_cli PRIVATE $<$<CONFIG:MinSizeRel>:-flto>)

# ReBuild by single terminal command (been in a build folder)
# cd .. && rm -rf build && mkdir build && cd build && cmake .. -G Xcode -DCMAKE_BUILD_TYPE=Release && cmake --build . --config Release”
