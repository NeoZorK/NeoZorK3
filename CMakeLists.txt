# Minimum required CMake version
cmake_minimum_required(VERSION 3.16)

# Project name
project(NeoZorK3 VERSION 1.0 LANGUAGES CXX)

# Set C++ standard (C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF) # Recommended to disable compiler extensions

# --- Dependencies ---
# Using FetchContent to manage dependencies

include(FetchContent)

# 1. cpp-httplib (for HTTP/S requests)
FetchContent_Declare(
  cpp_httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        v0.15.3 # Use an up-to-date stable tag/commit
)
FetchContent_MakeAvailable(cpp_httplib)
# cpp-httplib is a header-only library, but there's no target, include via include_directories

# 2. nlohmann/json (for working with JSON)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3 # Use an up-to-date stable tag/commit
)
FetchContent_MakeAvailable(nlohmann_json)
# nlohmann_json provides the target nlohmann_json::nlohmann_json

# 3. WebSocket library (PLACEHOLDER - needs to be added later)
# Example for uWebSockets (will also require libuv and zlib)
# FetchContent_Declare(...)
# Or for Boost.Beast (will require Boost)
# find_package(Boost REQUIRED COMPONENTS system beast)


# --- Module Definitions (as logical STATIC libraries) ---
# All source files (.cpp) are located in src/
# All header files (.hpp) are located in include/

# Utilities Module (example)
add_library(neozork3_utils STATIC
    src/logger.cpp # Provide at least one source
    # src/string_utils.cpp # Keep others commented until implemented
    # ... other utility .cpp files
)
target_include_directories(neozork3_utils PUBLIC include) # Correct path

# Config Manager Module
add_library(neozork3_config_manager STATIC
    src/config_manager.cpp # Provide at least one source
    # ...
)
target_include_directories(neozork3_config_manager PUBLIC include) # Correct path
# Dependencies: nlohmann_json needed by header, so PUBLIC. utils likely needed only by .cpp, so PRIVATE.
target_link_libraries(neozork3_config_manager
    PUBLIC # Changed to PUBLIC for nlohmann_json
        nlohmann_json::nlohmann_json
    PRIVATE # utils can stay PRIVATE if config_manager.hpp doesn't include utils headers
        neozork3_utils
)

# Connection Manager Module (example placeholder)
add_library(neozork3_connection_manager STATIC
    src/connection_manager.cpp # Provide at least one source
    # src/http_client.cpp
    # src/ws_client.cpp # Will need a WebSocket library
    # ...
)
# Add include path for own headers
target_include_directories(neozork3_connection_manager PUBLIC include) # Correct path
# Add include path for httplib headers PUBLICLY if connection_manager.hpp includes httplib.h
target_include_directories(neozork3_connection_manager PUBLIC ${cpp_httplib_SOURCE_DIR}) # Changed to PUBLIC
# Link dependencies
target_link_libraries(neozork3_connection_manager PRIVATE
    # cpp_httplib is header only, path added above
    neozork3_utils # Link against the utils static library (PRIVATE is likely ok)
    # ws_library # When you add WebSocket support
)


# CLI (Command Line Interface) Module
add_library(neozork3_cli_logic STATIC
    src/cli_parser.cpp # Provide at least one source
    # src/command_handler.cpp
    # ...
)
target_include_directories(neozork3_cli_logic PUBLIC include) # Correct path
# Dependencies: These are likely needed by cli_parser.cpp, not cli_parser.hpp, so PRIVATE is fine.
# CMake will handle transitive PUBLIC dependencies (like nlohmann_json from config_manager) automatically.
target_link_libraries(neozork3_cli_logic PRIVATE
    neozork3_config_manager # Links PRIVATEly, but inherits PUBLIC dependencies like nlohmann_json
    neozork3_utils
    neozork3_connection_manager # Example
)


# --- Executable ---
# Uses src/main.cpp as the entry point

add_executable(neozork3_cli
    src/main.cpp
)

# Specify the include path for the executable
target_include_directories(neozork3_cli PUBLIC include) # Correct path

# Link the executable with the main CLI logic static library
target_link_libraries(neozork3_cli PRIVATE
    neozork3_cli_logic # Inherits PUBLIC include paths transitively
)

# --- Compile/Link Options (examples) ---

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(neozork3_cli PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Optimization for Release builds
if(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(neozork3_cli PRIVATE -O3)
endif()

# For Debug builds, can add debug flags
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(neozork3_cli PRIVATE -g)
endif()

# --- Installation (if needed) ---
# install(TARGETS neozork3_cli DESTINATION bin)

# --- Tests (if using CTest) ---
# enable_testing()
# add_test(NAME MyTest COMMAND $<TARGET_FILE:neozork3_cli> --test-something)