#include <gtest/gtest.h>
#include "types.h"

namespace solana_arbitrage {

class BasicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }
    
    void TearDown() override {
        // Cleanup code
    }
};

TEST_F(BasicTest, DecimalCreation) {
    Decimal d1 = Decimal::from_double(1.5);
    Decimal d2 = Decimal::from_double(2.5);
    
    EXPECT_EQ(d1.to_double(), 1.5);
    EXPECT_EQ(d2.to_double(), 2.5);
}

TEST_F(BasicTest, MarketPairComparison) {
    MarketPair pair1{"SOL", "USDC"};
    MarketPair pair2{"SOL", "USDC"};
    MarketPair pair3{"ETH", "USDC"};
    
    EXPECT_EQ(pair1, pair2);
    EXPECT_NE(pair1, pair3);
}

TEST_F(BasicTest, OrderBookOperations) {
    OrderBook book;
    book.bids.push_back({Decimal::from_double(100.0), Decimal::from_double(1.0)});
    book.asks.push_back({Decimal::from_double(101.0), Decimal::from_double(1.0)});
    
    EXPECT_EQ(book.bids.size(), 1);
    EXPECT_EQ(book.asks.size(), 1);
}

TEST_F(BasicTest, ArbitrageOpportunityValidation) {
    ArbitrageOpportunity opp;
    opp.profit_percentage = Decimal::from_double(0.5);
    opp.estimated_profit = Decimal::from_double(10.0);
    
    EXPECT_TRUE(opp.is_profitable());
}

TEST_F(BasicTest, OrderCreation) {
    Order order;
    order.id = "test_order_1";
    order.pair = MarketPair{"SOL", "USDC"};
    order.side = OrderSide::BUY;
    order.type = OrderType::LIMIT;
    order.size = Decimal::from_double(1.0);
    order.price = Decimal::from_double(100.0);
    
    EXPECT_EQ(order.id, "test_order_1");
    EXPECT_EQ(order.side, OrderSide::BUY);
}

TEST_F(BasicTest, ConfigDefaultValues) {
    Config config;
    EXPECT_EQ(config.dry_run, false);
    EXPECT_EQ(config.max_concurrent_orders, 10);
}

TEST_F(BasicTest, RiskLimitsDefaultValues) {
    RiskLimits limits;
    EXPECT_EQ(limits.max_position_size.to_double(), 0.0);
    EXPECT_EQ(limits.max_daily_loss.to_double(), 0.0);
}

} // namespace solana_arbitrage

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
