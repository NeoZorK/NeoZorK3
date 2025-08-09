# NeoZorK3 Tools

This directory contains all the additional tools, scripts, and demos for NeoZorK3 that were developed after v0.1.9. These tools provide enhanced functionality for Solana arbitrage trading, airdrop solutions, and testing.

## 🚀 Quick Start

```bash
# List all available tools
python3 tools/run_tool.py --list

# Start interactive mode
python3 tools/run_tool.py --interactive

# Run a specific script
python3 tools/run_tool.py --script start_testnet_trading.sh

# Run a specific demo
python3 tools/run_tool.py --demo demo_arbitrage.py
```

## 📁 Directory Structure

```
tools/
├── README.md              # This file
├── run_tool.py           # Main tool runner
├── scripts/              # Shell scripts
│   ├── start_real_trading.sh
│   ├── start_testnet_trading.sh
│   └── quick_airdrop.sh
├── demos/                # Python demo scripts
│   ├── demo_arbitrage.py
│   ├── demo_airdrop_solutions.py
│   ├── final_demo.py
│   ├── real_demo.py
│   ├── simple_demo.py
│   ├── smart_airdrop.py
│   ├── testnet_demo.py
│   └── testnet_faucet.py
└── configs/              # Configuration files
    ├── config.json.example
    ├── config.production.json
    ├── config.testnet.json
    └── env.example
```

## 📜 Scripts

### Trading Scripts

#### `start_real_trading.sh`
- **Purpose**: Launch real trading on Solana mainnet
- **Features**: Production configuration, real wallet setup, risk management
- **Usage**: `python3 tools/run_tool.py --script start_real_trading.sh`

#### `start_testnet_trading.sh`
- **Purpose**: Launch trading on Solana testnet for testing
- **Features**: Testnet configuration, faucet integration, safe testing environment
- **Usage**: `python3 tools/run_tool.py --script start_testnet_trading.sh`

#### `quick_airdrop.sh`
- **Purpose**: Quick airdrop claiming and management
- **Features**: Automated airdrop claiming, wallet management
- **Usage**: `python3 tools/run_tool.py --script quick_airdrop.sh`

## 🎮 Demos

### Arbitrage Demos

#### `demo_arbitrage.py`
- **Purpose**: Demonstrate arbitrage bot functionality
- **Features**: Market data simulation, arbitrage opportunity detection
- **Usage**: `python3 tools/run_tool.py --demo demo_arbitrage.py`

#### `simple_demo.py`
- **Purpose**: Simple arbitrage demonstration
- **Features**: Basic arbitrage simulation, price monitoring
- **Usage**: `python3 tools/run_tool.py --demo simple_demo.py`

#### `final_demo.py`
- **Purpose**: Comprehensive arbitrage demonstration
- **Features**: Full arbitrage workflow, multiple strategies
- **Usage**: `python3 tools/run_tool.py --demo final_demo.py`

### Airdrop Demos

#### `demo_airdrop_solutions.py`
- **Purpose**: Demonstrate airdrop claiming solutions
- **Features**: Multiple airdrop strategies, automated claiming
- **Usage**: `python3 tools/run_tool.py --demo demo_airdrop_solutions.py`

#### `smart_airdrop.py`
- **Purpose**: Smart airdrop management and optimization
- **Features**: Intelligent airdrop selection, profit optimization
- **Usage**: `python3 tools/run_tool.py --demo smart_airdrop.py`

### Testnet Demos

#### `testnet_demo.py`
- **Purpose**: Testnet trading demonstration
- **Features**: Testnet environment setup, safe trading simulation
- **Usage**: `python3 tools/run_tool.py --demo testnet_demo.py`

#### `testnet_faucet.py`
- **Purpose**: Testnet faucet integration
- **Features**: Automated SOL claiming, wallet funding
- **Usage**: `python3 tools/run_tool.py --demo testnet_faucet.py`

### Real Trading Demos

#### `real_demo.py`
- **Purpose**: Real trading demonstration
- **Features**: Live market data, actual trading simulation
- **Usage**: `python3 tools/run_tool.py --demo real_demo.py`

## ⚙️ Configurations

### `config.json.example`
- **Purpose**: Example configuration template
- **Usage**: Copy and modify for your setup

### `config.production.json`
- **Purpose**: Production trading configuration
- **Features**: Real trading parameters, risk settings

### `config.testnet.json`
- **Purpose**: Testnet trading configuration
- **Features**: Testnet parameters, safe testing settings

### `env.example`
- **Purpose**: Environment variables template
- **Usage**: Copy to `.env` and fill in your values

## 🔧 Main Tool Runner

The `run_tool.py` script provides a unified interface for all tools:

### Features
- **List Tools**: `--list` or `-l`
- **Run Script**: `--script <script_name>` or `-s <script_name>`
- **Run Demo**: `--demo <demo_name>` or `-d <demo_name>`
- **Show Config**: `--config <config_name>` or `-c <config_name>`
- **Interactive Mode**: `--interactive` or `-i`

### Examples

```bash
# List all available tools
python3 tools/run_tool.py --list

# Run testnet trading script
python3 tools/run_tool.py --script start_testnet_trading.sh

# Run arbitrage demo
python3 tools/run_tool.py --demo demo_arbitrage.py

# Show production config
python3 tools/run_tool.py --config config.production.json

# Start interactive mode
python3 tools/run_tool.py --interactive
```

## 🛠️ Prerequisites

### For Scripts
- Solana CLI installed
- Compiled NeoZorK3 binary (`./build.sh`)
- Proper configuration files

### For Demos
- Python 3.7+
- Required Python packages (install as needed)
- Solana CLI (for some demos)

### For Configurations
- Valid Solana wallet
- API keys (for production)
- Proper network access

## 🔒 Security Notes

- **Testnet First**: Always test on testnet before mainnet
- **Secure Keys**: Keep private keys secure and never commit them
- **Risk Management**: Use proper risk settings in production
- **Monitoring**: Monitor all trading activities

## 📚 Related Documentation

- [Main README](../README.md) - Project overview
- [Build Instructions](../docs/BUILD_INSTRUCTIONS.md) - How to build the project
- [Quick Start Guide](../docs/QUICK_START.md) - Getting started

## 🆘 Troubleshooting

### Common Issues

1. **Script Permission Denied**
   ```bash
   chmod +x tools/scripts/*.sh
   ```

2. **Python Module Not Found**
   ```bash
   pip3 install -r requirements.txt  # if available
   ```

3. **Solana CLI Not Found**
   ```bash
   # Install Solana CLI
   sh -c "$(curl -sSfL https://release.solana.com/stable/install)"
   ```

4. **Configuration Errors**
   - Check config files in `tools/configs/`
   - Ensure proper API keys and wallet setup
   - Verify network connectivity

### Getting Help

- Check the main project documentation
- Review configuration examples
- Test on testnet first
- Monitor logs for error messages
