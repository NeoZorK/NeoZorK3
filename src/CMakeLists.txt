# Blockchain adapters library
add_library(blockchain_adapters
    blockchain_adapters.cpp
)

target_link_libraries(blockchain_adapters
    PRIVATE
    Boost::system
    Boost::thread
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# Market data provider library
add_library(market_data_provider
    market_data_provider.cpp
)

target_link_libraries(market_data_provider
    PRIVATE
    Boost::system
    Boost::thread
    blockchain_adapters
)

# Arbitrage engine library
add_library(arbitrage_engine
    arbitrage_engine.cpp
)

target_link_libraries(arbitrage_engine
    PRIVATE
    market_data_provider
    blockchain_adapters
)

# Order manager library
add_library(order_manager
    order_manager.cpp
)

target_link_libraries(order_manager
    PRIVATE
    blockchain_adapters
    market_data_provider
)

# Risk manager library
add_library(risk_manager
    risk_manager.cpp
)

target_link_libraries(risk_manager
    PRIVATE
    order_manager
    arbitrage_engine
)

# Config manager library
add_library(config_manager
    config_manager.cpp
)

# Logger library
add_library(logger
    logger.cpp
)

target_link_libraries(logger
    PRIVATE
    Threads::Threads
)
