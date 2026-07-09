#include <gtest/gtest.h>
#include "market_data_provider.h"

namespace solana_arbitrage {

class MarketDataProviderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MarketDataProviderTest, BasicTest) {
    EXPECT_TRUE(true);
}

} // namespace solana_arbitrage

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
