#include <gtest/gtest.h>
#include "arbitrage_engine.h"

namespace solana_arbitrage {

class ArbitrageEngineTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ArbitrageEngineTest, BasicTest) {
    EXPECT_TRUE(true);
}

} // namespace solana_arbitrage

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
