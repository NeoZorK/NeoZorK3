#include <gtest/gtest.h>
#include "risk_manager.h"

namespace solana_arbitrage {

class RiskManagerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RiskManagerTest, BasicTest) {
    EXPECT_TRUE(true);
}

} // namespace solana_arbitrage

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
