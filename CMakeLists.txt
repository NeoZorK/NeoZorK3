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
# Now includes at least one source file
add_library(neozork3_utils STATIC
    src/logger.cpp # Provide at least one source
    # src/string_utils.cpp # Keep others commented until implemented
    # ... other utility .cpp files
)
# All modules can see header files from include/
target_include_directories(neozork3_utils PUBLIC include) # Corrected path

# Config Manager Module
add_library(neozork3_config_manager STATIC
    src/config_manager.cpp # Provide at least one source
    # ...
)
target_include_directories(neozork3_config_manager PUBLIC include) # Corrected path
# Dependencies: nlohmann_json for parsing, utils for helper functions
target_link_libraries(neozork3_config_manager PRIVATE
    nlohmann_json::nlohmann_json
    neozork3_utils # Link against the utils static library
)

# Connection Manager Module (example placeholder)
add_library(neozork3_connection_manager STATIC
    src/connection_manager.cpp # Provide at least one source
    # src/http_client.cpp
    # src/ws_client.cpp # Will need a WebSocket library
    # ...
)
target_include_directories(neozork3_connection_manager PUBLIC include) # Corrected path
# Dependencies: cpp-httplib, utils, (later WebSocket lib)
target_link_libraries(neozork3_connection_manager PRIVATE
    # cpp_httplib # Since it's header-only, added via include_directories below
    neozork3_utils # Link against the utils static library
    # ws_library # When you add WebSocket support
)
# Add the include path for cpp-httplib headers
target_include_directories(neozork3_connection_manager PRIVATE ${cpp_httplib_SOURCE_DIR})


# CLI (Command Line Interface) Module
# (Could be named neozork3_cli_logic to avoid confusion with the executable)
add_library(neozork3_cli_logic STATIC
    src/cli_parser.cpp # Provide at least one source
    # src/command_handler.cpp
    # ...
)
target_include_directories(neozork3_cli_logic PUBLIC include) # Corrected path
# Dependencies: config_manager and utils for accessing config and utilities
target_link_libraries(neozork3_cli_logic PRIVATE
    neozork3_config_manager # Link against the config_manager static library
    neozork3_utils          # Link against the utils static library
    # Add other modules called by the CLI
    neozork3_connection_manager # Example, linking against connection_manager static library
)
# If CLI uses httplib directly (unlikely, better through modules)
# target_include_directories(neozork3_cli_logic PRIVATE ${cpp_httplib_SOURCE_DIR})


# --- Executable ---
# Uses src/main.cpp as the entry point

add_executable(neozork3_cli
    src/main.cpp
)

# Specify the include path for the executable
target_include_directories(neozork3_cli PUBLIC include) # Corrected path

# Link the executable with the main CLI logic static library (which pulls in other dependencies)
target_link_libraries(neozork3_cli PRIVATE
    neozork3_cli_logic
)

# If main.cpp directly uses httplib or json (try to avoid, use modules instead)
# target_include_directories(neozork3_cli PRIVATE ${cpp_httplib_SOURCE_DIR}) # Uncomment if needed
# target_link_libraries(neozork3_cli PRIVATE nlohmann_json::nlohmann_json) # Uncomment if needed


# --- Compile/Link Options (examples) ---

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(neozork3_cli PRIVATE -Wall -Wextra -Wpedantic)
    # For Clang, can also add: -Wshadow -Wnon-virtual-dtor
endif()

# Optimization for Release builds
if(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(neozork3_cli PRIVATE -O3)
endif()

# For Debug builds, can add debug flags
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(neozork3_cli PRIVATE -g)
endif()

# Add a define (example)
# target_compile_definitions(neozork3_utils PUBLIC MY_PROJECT_DEFINE)

# --- Installation (if needed) ---
# Now installation is simpler, just install the executable
# install(TARGETS neozork3_cli DESTINATION bin)
# install(FILES include/my_public_header.hpp DESTINATION include) # Example header installation

# --- Tests (if using CTest) ---
# enable_testing()
# add_test(NAME MyTest COMMAND $<TARGET_FILE:neozork3_cli> --test-something) # Use generator expression
# add_subdirectory(tests) # If tests are in a separate folder with their own CMakeLists.txt