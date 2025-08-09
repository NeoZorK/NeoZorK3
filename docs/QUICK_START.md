# NeoZorK3 - Quick Start

## Build and Run

### 1. Quick Build
```bash
./build.sh
```

### 2. Test the Application
```bash
cd build
./neozork3_cli --help
```

### 3. Initialize Configuration
```bash
./neozork3_cli --config-init
```

### 4. Discover Endpoints for Fantom
```bash
./neozork3_cli --discover-endpoints --blockchain Fantom --source chain
```

### 5. Scan Endpoints
```bash
./neozork3_cli --scan --blockchain Fantom
```

### 6. View Active Endpoints
```bash
./neozork3_cli --active --blockchain Fantom
```

## Main Commands

| Command | Description |
|---------|-------------|
| `--help` | Show help |
| `--config-init` | Initialize configuration |
| `--discover-endpoints -b <blockchain>` | Find endpoints for blockchain |
| `--scan -b <blockchain>` | Scan endpoints |
| `--active -b <blockchain>` | Show active endpoints |
| `--find-dexes -b <blockchain>` | Find DEX contracts |
| `--find-pools -b <blockchain> --dex <dex_id>` | Find liquidity pools |

## Supported Blockchains

- Fantom (ID: 250)
- Ethereum (ID: 1)
- Solana
- Avalanche
- Sonic
- BTC
- And others (identified by Network ID)

## Project Files

- `build/neozork3_cli` - executable file
- `build/NeoZorK-config` - configuration file
- `docs/BUILD_INSTRUCTIONS.md` - detailed build instructions

## Troubleshooting

If you encounter Git issues when downloading dependencies, use local dependencies:

```bash
mkdir -p external
cd external
curl -L https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz -o nlohmann_json.tar.gz
tar -xzf nlohmann_json.tar.gz && mv json-3.11.3 nlohmann_json
curl -L https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.15.3.tar.gz -o cpp_httplib.tar.gz
tar -xzf cpp_httplib.tar.gz && mv cpp-httplib-0.15.3 cpp_httplib
cd ..
```

Then use the modified `CMakeLists.txt` (already included in the project).
