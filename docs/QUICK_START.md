# Quick Start Guide

This guide will help you get the Solana Arbitrage Bot up and running quickly.

## Prerequisites

Before you begin, ensure you have:

- **C++20 compatible compiler** (GCC 10+, Clang 12+, or MSVC 2019+)
- **CMake 3.20+**
- **Boost 1.75+** (system, thread, beast)
- **OpenSSL**
- **Solana wallet** with some SOL for transaction fees

## Installation

### 1. Clone the Repository

```bash
git clone https://github.com/your-username/solana-arbitrage-bot.git
cd solana-arbitrage-bot
```

### 2. Install Dependencies

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake libboost-all-dev libssl-dev libgtest-dev
```

#### macOS
```bash
brew install cmake boost openssl googletest
```

#### Windows
```cmd
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install boost-system boost-thread boost-beast openssl gtest
```

### 3. Build the Project

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 4. Run Tests

```bash
make test
```

## Configuration

### 1. Create Configuration File

Create a `config.json` file in the project root:

```json
{
  "rpc_endpoint": "https://api.mainnet-beta.solana.com",
  "wallet_private_key": "your_private_key_here",
  "max_trade_size": 100.0,
  "min_profit_percentage": 0.5,
  "max_concurrent_orders": 5,
  "order_timeout_ms": 30000,
  "dry_run": true,
  "risk_limits": {
    "max_position_size": 1000.0,
    "max_daily_loss": 100.0,
    "max_drawdown": 10.0,
    "max_orders_per_minute": 10
  }
}
```

### 2. Set Up Your Wallet

**⚠️ Security Warning**: Never share your private key!

1. **Generate a new wallet** (recommended for testing):
   ```bash
   # Use Solana CLI to generate a new keypair
   solana-keygen new --outfile wallet.json
   ```

2. **Get your private key**:
   ```bash
   # Extract private key (base58 format)
   solana-keygen pubkey wallet.json
   ```

3. **Add SOL for fees**:
   ```bash
   # Transfer some SOL to your wallet for transaction fees
   solana transfer <your_public_key> 0.1
   ```

### 3. Configure RPC Endpoint

Choose a reliable RPC endpoint:

- **Public**: `https://api.mainnet-beta.solana.com` (rate limited)
- **Private**: Use services like QuickNode, Alchemy, or run your own node

## Running the Bot

### 1. Dry Run (Recommended First)

Test the bot without real trades:

```bash
./solana_arbitrage_bot --dry-run --verbose
```

### 2. Monitor Output

The bot will display real-time statistics:

```
=== Bot Statistics ===
Arbitrage Engine:
  Opportunities Found: 1250
  Opportunities Executed: 45
  Total Profit: 234.56 USD
  Uptime: 2 hours

Order Manager:
  Orders Placed: 90
  Orders Filled: 45
  Orders Cancelled: 45
  Total Volume: 4500.00 USD
  Total Fees: 12.34 USD

Risk Manager:
  Current Portfolio Value: 1234.56 USD
  Daily P&L: 45.67 USD
  Total P&L: 234.56 USD
  Current Drawdown: 2.3%
```

### 3. Live Trading

Once you're confident with the dry run:

```bash
# Remove dry-run flag and set your private key
./solana_arbitrage_bot -c config.json
```

## Understanding the Output

### Log Levels

- **INFO**: General information about bot operation
- **WARNING**: Potential issues that don't stop operation
- **ERROR**: Problems that may affect trading
- **CRITICAL**: Serious issues requiring immediate attention

### Key Metrics

- **Opportunities Found**: Number of arbitrage opportunities detected
- **Opportunities Executed**: Number of trades actually placed
- **Total Profit**: Cumulative profit from all trades
- **Current Drawdown**: Maximum loss from peak portfolio value

## Common Scenarios

### 1. No Opportunities Found

**Possible causes**:
- Market conditions are efficient (no arbitrage opportunities)
- Configuration thresholds are too high
- RPC endpoint issues

**Solutions**:
- Lower `min_profit_percentage` in config
- Check RPC endpoint connectivity
- Monitor during high volatility periods

### 2. High Cancellation Rate

**Possible causes**:
- Market moving too fast
- Slippage tolerance too low
- Network congestion

**Solutions**:
- Increase `slippage_tolerance` in config
- Use faster RPC endpoint
- Reduce trade size

### 3. Risk Violations

**Possible causes**:
- Position limits exceeded
- Daily loss limit reached
- Drawdown protection triggered

**Solutions**:
- Review and adjust risk limits
- Check market conditions
- Consider pausing trading

## Safety Features

### Risk Management

The bot includes several safety features:

1. **Position Limits**: Maximum position size per token
2. **Portfolio Limits**: Overall portfolio exposure limits
3. **Daily Loss Limits**: Maximum daily loss threshold
4. **Drawdown Protection**: Automatic pause on significant losses
5. **Slippage Protection**: Prevents trades with excessive slippage

### Emergency Stop

To stop the bot immediately:

```bash
# Press Ctrl+C for graceful shutdown
# Or kill the process if necessary
pkill -f solana_arbitrage_bot
```

## Troubleshooting

### Common Issues

#### 1. "RPC endpoint not responding"

```bash
# Test RPC endpoint
curl -X POST https://api.mainnet-beta.solana.com \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"getHealth"}'
```

#### 2. "Insufficient SOL for transaction fees"

```bash
# Check SOL balance
solana balance <your_public_key>

# Add more SOL if needed
solana transfer <your_public_key> 0.1
```

#### 3. "Invalid private key format"

Ensure your private key is in the correct format (base58 for Solana).

#### 4. "Market data not available"

Check if the DEX APIs are accessible and your RPC endpoint supports the required methods.

### Getting Help

1. **Check logs**: Look in `logs/bot.log` for detailed error messages
2. **Enable verbose mode**: Run with `--verbose` flag
3. **Test components**: Run individual tests to isolate issues
4. **Community support**: Check GitHub issues and discussions

## Next Steps

### Advanced Configuration

1. **Custom Strategies**: Implement your own arbitrage strategies
2. **Multiple DEXes**: Add support for additional DEXes
3. **Performance Tuning**: Optimize for your specific hardware
4. **Monitoring**: Set up external monitoring and alerts

### Production Deployment

1. **Server Setup**: Deploy on high-performance server
2. **Monitoring**: Set up comprehensive monitoring
3. **Backup**: Implement backup and recovery procedures
4. **Security**: Harden the deployment environment

### Development

1. **Add Features**: Extend the bot with new capabilities
2. **Optimize**: Profile and optimize performance
3. **Test**: Add comprehensive tests for new features
4. **Document**: Update documentation for changes

## Support

- **Documentation**: Check the `docs/` folder for detailed guides
- **Issues**: Report bugs on GitHub Issues
- **Discussions**: Ask questions on GitHub Discussions
- **Wiki**: Check the project wiki for additional resources

## Disclaimer

This software is for educational and research purposes. Trading cryptocurrencies involves substantial risk of loss. Use at your own risk. The authors are not responsible for any financial losses.

Always start with small amounts and thoroughly test the bot before committing significant capital.
