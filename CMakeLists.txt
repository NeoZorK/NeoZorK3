cmake_minimum_required(VERSION 3.20)
project(SolanaArbitrageBot VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Find required packages
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

# Set Boost include directories
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/external)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Main executable
add_executable(solana_arbitrage_bot
    src/main.cpp
    src/blockchain_adapters.cpp
    src/market_data_provider.cpp
    src/arbitrage_engine.cpp
    src/order_manager.cpp
    src/risk_manager.cpp
    src/config_manager.cpp
    src/logger.cpp
)

# Link libraries
target_link_libraries(solana_arbitrage_bot
    PRIVATE
    Boost::system
    Boost::thread
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    blockchain_adapters
    market_data_provider
    arbitrage_engine
    order_manager
    risk_manager
    config_manager
    logger
)

# Install
install(TARGETS solana_arbitrage_bot DESTINATION bin)
