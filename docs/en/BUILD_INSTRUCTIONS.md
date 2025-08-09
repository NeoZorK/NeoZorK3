# Build Instructions

This document provides detailed instructions for building the Solana Arbitrage Bot on different platforms.

## Prerequisites

### Required Software

- **C++20 compatible compiler**
  - GCC 10+ (Linux/macOS)
  - Clang 12+ (Linux/macOS)
  - MSVC 2019+ (Windows)
- **CMake 3.20+**
- **Git**

### Required Libraries

- **Boost 1.75+** (system, thread, beast)
- **OpenSSL**
- **Google Test** (for testing)

## Platform-Specific Setup

### Ubuntu/Debian

```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install build-essential cmake git

# Install Boost
sudo apt install libboost-all-dev

# Install OpenSSL
sudo apt install libssl-dev

# Install Google Test
sudo apt install libgtest-dev
```

### CentOS/RHEL/Fedora

```bash
# Install build tools
sudo yum groupinstall "Development Tools"
sudo yum install cmake git

# Install Boost
sudo yum install boost-devel

# Install OpenSSL
sudo yum install openssl-devel

# Install Google Test
sudo yum install gtest-devel
```

### macOS

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install build tools
brew install cmake git

# Install Boost
brew install boost

# Install OpenSSL
brew install openssl

# Install Google Test
brew install googletest
```

### Windows

1. **Install Visual Studio 2019 or later** with C++ development tools
2. **Install CMake** from https://cmake.org/download/
3. **Install vcpkg** for package management:
   ```cmd
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```
4. **Install required packages**:
   ```cmd
   .\vcpkg install boost-system boost-thread boost-beast openssl gtest
   ```

## Building the Project

### Step 1: Clone the Repository

```bash
git clone https://github.com/your-username/solana-arbitrage-bot.git
cd solana-arbitrage-bot
```

### Step 2: Create Build Directory

```bash
mkdir build
cd build
```

### Step 3: Configure with CMake

#### Linux/macOS
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

#### Windows
```cmd
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Step 4: Build

#### Linux/macOS
```bash
make -j$(nproc)
```

#### Windows
```cmd
cmake --build . --config Release --parallel
```

### Step 5: Run Tests

```bash
# Run all tests
make test

# Or run tests directly
ctest --verbose
```

## Build Options

### CMake Configuration Options

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build with optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release

# Enable sanitizers (Debug builds)
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON

# Enable coverage reporting
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON

# Disable tests
cmake .. -DBUILD_TESTS=OFF

# Custom install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
```

### Compiler-Specific Options

#### GCC
```bash
cmake .. -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"
```

#### Clang
```bash
cmake .. -DCMAKE_CXX_FLAGS="-O3 -march=native"
```

## Installation

### System-Wide Installation

```bash
# Build and install
make install

# Or with custom prefix
make install DESTDIR=/opt/solana-arbitrage-bot
```

### Local Installation

```bash
# Copy binary to local directory
cp solana_arbitrage_bot ~/bin/
```

## Troubleshooting

### Common Build Issues

#### 1. Boost Not Found

**Error**: `Could not find Boost`

**Solution**:
```bash
# Ubuntu/Debian
sudo apt install libboost-all-dev

# macOS
brew install boost

# Windows (vcpkg)
.\vcpkg install boost-system boost-thread boost-beast
```

#### 2. OpenSSL Not Found

**Error**: `Could not find OpenSSL`

**Solution**:
```bash
# Ubuntu/Debian
sudo apt install libssl-dev

# macOS
brew install openssl

# Windows (vcpkg)
.\vcpkg install openssl
```

#### 3. C++20 Not Supported

**Error**: `C++20 standard not supported`

**Solution**:
- Update your compiler to a newer version
- For GCC: Use version 10 or later
- For Clang: Use version 12 or later
- For MSVC: Use Visual Studio 2019 or later

#### 4. CMake Version Too Old

**Error**: `CMake 3.20 or higher is required`

**Solution**:
```bash
# Ubuntu/Debian
sudo apt install cmake

# macOS
brew install cmake

# Windows
# Download from https://cmake.org/download/
```

### Platform-Specific Issues

#### Linux

**Issue**: Permission denied when running tests
```bash
# Fix permissions
chmod +x tests/*_test
```

**Issue**: Library not found at runtime
```bash
# Add library path to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

#### macOS

**Issue**: OpenSSL not found
```bash
# Set OpenSSL path
export OPENSSL_ROOT_DIR=/usr/local/opt/openssl
cmake .. -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl
```

**Issue**: Boost not found
```bash
# Set Boost path
export BOOST_ROOT=/usr/local/opt/boost
cmake .. -DBOOST_ROOT=/usr/local/opt/boost
```

#### Windows

**Issue**: vcpkg not found
```cmd
# Set vcpkg toolchain file
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

**Issue**: MSVC compiler not found
```cmd
# Use Developer Command Prompt
# Or set environment variables
set VS160COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\
```

## Development Setup

### IDE Configuration

#### Visual Studio Code

1. Install C++ extension
2. Configure `c_cpp_properties.json`:
```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/include",
                "/usr/include",
                "/usr/local/include"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/gcc",
            "cStandard": "c17",
            "cppStandard": "c++20",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
```

#### CLion

1. Open project directory
2. Configure CMake settings
3. Set C++ standard to C++20

### Debugging

#### Enable Debug Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

#### Run with GDB
```bash
gdb ./solana_arbitrage_bot
```

#### Run with Valgrind (Linux)
```bash
valgrind --leak-check=full ./solana_arbitrage_bot
```

## Performance Optimization

### Compiler Optimizations

```bash
# Maximum optimization
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native -DNDEBUG"

# Link-time optimization
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -flto"
```

### Profile-Guided Optimization

```bash
# Generate profile data
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fprofile-generate"
make
./solana_arbitrage_bot --dry-run

# Use profile data
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fprofile-use -fprofile-correction"
make
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        sudo apt update
        sudo apt install build-essential cmake libboost-all-dev libssl-dev libgtest-dev
    
    - name: Build
      run: |
        mkdir build && cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release
        make -j$(nproc)
    
    - name: Test
      run: |
        cd build
        make test
```

## Package Creation

### Debian Package

```bash
# Install packaging tools
sudo apt install dh-make

# Create package
dh_make --createorig
dpkg-buildpackage -us -uc
```

### RPM Package

```bash
# Install packaging tools
sudo yum install rpm-build

# Create package
rpmbuild -ba solana-arbitrage-bot.spec
```

### Docker Image

```dockerfile
FROM ubuntu:20.04

RUN apt update && apt install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libssl-dev \
    libgtest-dev

COPY . /app
WORKDIR /app

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

ENTRYPOINT ["./build/solana_arbitrage_bot"]
```

Build and run:
```bash
docker build -t solana-arbitrage-bot .
docker run solana-arbitrage-bot --dry-run
```
