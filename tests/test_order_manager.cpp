#include <gtest/gtest.h>
#include "order_manager.h"

namespace solana_arbitrage {

class OrderManagerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(OrderManagerTest, BasicTest) {
    EXPECT_TRUE(true);
}

} // namespace solana_arbitrage

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
