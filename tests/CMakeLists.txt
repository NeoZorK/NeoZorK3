# Enable testing
enable_testing()

# Find GTest
find_package(GTest REQUIRED)

# Test executables
add_executable(test_blockchain_adapters
    test_blockchain_adapters.cpp
)

target_link_libraries(test_blockchain_adapters
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    blockchain_adapters
)

add_executable(test_market_data_provider
    test_market_data_provider.cpp
)

target_link_libraries(test_market_data_provider
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    market_data_provider
)

add_executable(test_arbitrage_engine
    test_arbitrage_engine.cpp
)

target_link_libraries(test_arbitrage_engine
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    arbitrage_engine
)

add_executable(test_order_manager
    test_order_manager.cpp
)

target_link_libraries(test_order_manager
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    order_manager
)

add_executable(test_risk_manager
    test_risk_manager.cpp
)

target_link_libraries(test_risk_manager
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    risk_manager
)

# Add tests
add_test(NAME BlockchainAdaptersTest COMMAND test_blockchain_adapters)
add_test(NAME MarketDataProviderTest COMMAND test_market_data_provider)
add_test(NAME ArbitrageEngineTest COMMAND test_arbitrage_engine)
add_test(NAME OrderManagerTest COMMAND test_order_manager)
add_test(NAME RiskManagerTest COMMAND test_risk_manager)
