# NeoZorK3 - Build and Run Instructions

## Project Description

NeoZorK3 is a decentralized exchange (DEX) arbitrage system written in C++17. The project supports multiple blockchains including Fantom, Solana, Ethereum, Avalanche and others.

## Prerequisites

### macOS
```bash
# Install Homebrew (if not installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake openssl
```

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake libssl-dev git
```

### CentOS/RHEL/Fedora
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake openssl-devel git
# or for Fedora:
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake openssl-devel git
```

## Quick Build

### Automatic Build (Recommended)
```bash
# Run build script
./build.sh
```

### Manual Build

1. **Clone Dependencies** (if not done yet):
```bash
# Create dependencies folder
mkdir -p external
cd external

# Download nlohmann/json
curl -L https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz -o nlohmann_json.tar.gz
tar -xzf nlohmann_json.tar.gz
mv json-3.11.3 nlohmann_json

# Download cpp-httplib
curl -L https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.15.3.tar.gz -o cpp_httplib.tar.gz
tar -xzf cpp_httplib.tar.gz
mv cpp-httplib-0.15.3 cpp_httplib

cd ..
```

2. **Build Project**:
```bash
# Create build folder
mkdir -p build
cd build

# Configure with CMake
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release

# Compile
make -j$(nproc)  # Linux
# or
make -j$(sysctl -n hw.ncpu)  # macOS
```

## Running

After successful build, the executable will be in `build/neozork3_cli`.

### Test the Application
```bash
cd build
./neozork3_cli --help
```

### Main Commands

1. **Initialize Configuration**:
```bash
./neozork3_cli --config-init
```

2. **Discover Endpoints for Blockchain**:
```bash
./neozork3_cli --discover-endpoints --blockchain Fantom
```

3. **Scan Endpoints**:
```bash
./neozork3_cli --scan --blockchain Fantom
```

4. **View Active Endpoints**:
```bash
./neozork3_cli --active --blockchain Fantom
```

5. **Find DEX Contracts**:
```bash
./neozork3_cli --find-dexes --blockchain Fantom
```

## Configuration

The project uses a configuration file `NeoZorK-config` which is automatically created on first run. The file contains:

- Blockchain settings
- RPC endpoints
- DEX contracts
- Liquidity pools
- Performance parameters

## Troubleshooting

### Error "No CMAKE_CXX_COMPILER could be found"
Install C++ compiler:
- macOS: `xcode-select --install`
- Ubuntu: `sudo apt install build-essential`

### Error "OpenSSL not found"
Install OpenSSL:
- macOS: `brew install openssl`
- Ubuntu: `sudo apt install libssl-dev`

### Errors when downloading dependencies
Use local dependencies as described in "Manual Build" section.

## Project Structure

```
NeoZorK3/
├── CMakeLists.txt          # Build configuration
├── build.sh               # Automatic build script
├── include/               # Header files
├── src/                   # Source code
├── external/              # Local dependencies
├── docs/                  # Documentation
├── build/                 # Build folder (created automatically)
└── NeoZorK-config         # Configuration file (created automatically)
```

## Development

### Debug Build
```bash
cd build
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Rebuild
```bash
cd build
make clean
make -j$(nproc)
```

### Full Rebuild
```bash
rm -rf build
./build.sh
```
