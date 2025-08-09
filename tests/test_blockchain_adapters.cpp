#include <gtest/gtest.h>
#include "blockchain_adapters.h"

namespace solana_arbitrage {

class BlockchainAdaptersTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(BlockchainAdaptersTest, BasicTest) {
    EXPECT_TRUE(true);
}

} // namespace solana_arbitrage

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
