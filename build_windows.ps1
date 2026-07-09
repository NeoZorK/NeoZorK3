# Windows Build Script for NeoZorK3 Arbitrage Bot
# This script builds the project on Windows with vcpkg

param(
    [switch]$Debug,
    [switch]$Clean,
    [switch]$Test,
    [switch]$Help
)

# Colors for output
$Red = "Red"
$Green = "Green"
$Yellow = "Yellow"
$Blue = "Blue"
$White = "White"

# Function to print colored output
function Write-Status {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor $Blue
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] $Message" -ForegroundColor $Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host "[WARNING] $Message" -ForegroundColor $Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor $Red
}

# Show help
if ($Help) {
    Write-Host "Windows Build Script for NeoZorK3 Arbitrage Bot" -ForegroundColor $White
    Write-Host ""
    Write-Host "Usage: .\build_windows.ps1 [options]" -ForegroundColor $White
    Write-Host ""
    Write-Host "Options:" -ForegroundColor $White
    Write-Host "  -Debug     Build in debug mode" -ForegroundColor $White
    Write-Host "  -Clean     Clean build directory before building" -ForegroundColor $White
    Write-Host "  -Test      Run tests after build" -ForegroundColor $White
    Write-Host "  -Help      Show this help message" -ForegroundColor $White
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor $White
    Write-Host "  .\build_windows.ps1                    # Build in release mode" -ForegroundColor $White
    Write-Host "  .\build_windows.ps1 -Debug             # Build in debug mode" -ForegroundColor $White
    Write-Host "  .\build_windows.ps1 -Clean -Test       # Clean build and run tests" -ForegroundColor $White
    exit 0
}

Write-Status "Starting NeoZorK3 Arbitrage Bot Windows build..."

# Check if running from Developer Command Prompt
Write-Status "Checking Windows environment..."

# Check for Visual Studio
try {
    $null = Get-Command cl -ErrorAction Stop
    Write-Success "Visual Studio compiler found"
} catch {
    Write-Error "Visual Studio compiler (cl.exe) not found"
    Write-Status "Please run this script from Visual Studio Developer Command Prompt"
    Write-Status "Or install Visual Studio 2019+ with C++ development tools"
    exit 1
}

# Check for CMake
try {
    $null = Get-Command cmake -ErrorAction Stop
    Write-Success "CMake found"
} catch {
    Write-Error "CMake not found"
    Write-Status "Please install CMake from https://cmake.org/download/"
    exit 1
}

# Check for vcpkg
if (-not $env:VCPKG_ROOT) {
    if (Test-Path "C:\vcpkg\vcpkg.exe") {
        $env:VCPKG_ROOT = "C:\vcpkg"
        Write-Success "Found vcpkg in C:\vcpkg"
    } else {
        Write-Error "VCPKG_ROOT not set and vcpkg not found in C:\vcpkg"
        Write-Status "Please install vcpkg:"
        Write-Status "  git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg"
        Write-Status "  cd C:\vcpkg"
        Write-Status "  .\bootstrap-vcpkg.bat"
        Write-Status "  .\vcpkg integrate install"
        exit 1
    }
}

Write-Success "Environment check passed"

# Install dependencies if not already installed
Write-Status "Checking vcpkg dependencies..."

$boostPath = "$env:VCPKG_ROOT\installed\x64-windows\include\boost\asio.hpp"
$opensslPath = "$env:VCPKG_ROOT\installed\x64-windows\include\openssl\ssl.h"
$jsonPath = "$env:VCPKG_ROOT\installed\x64-windows\include\nlohmann\json.hpp"

if (-not (Test-Path $boostPath)) {
    Write-Status "Installing Boost libraries..."
    & "$env:VCPKG_ROOT\vcpkg.exe" install boost-system:x64-windows boost-thread:x64-windows boost-beast:x64-windows
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to install Boost libraries"
        exit 1
    }
}

if (-not (Test-Path $opensslPath)) {
    Write-Status "Installing OpenSSL..."
    & "$env:VCPKG_ROOT\vcpkg.exe" install openssl:x64-windows
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to install OpenSSL"
        exit 1
    }
}

if (-not (Test-Path $jsonPath)) {
    Write-Status "Installing nlohmann/json..."
    & "$env:VCPKG_ROOT\vcpkg.exe" install nlohmann-json:x64-windows
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to install nlohmann/json"
        exit 1
    }
}

Write-Success "Dependencies check passed"

# Set build type
$BuildType = if ($Debug) { "Debug" } else { "Release" }

# Create build directory
Write-Status "Creating build directory..."
if (Test-Path "build") {
    if ($Clean) {
        Write-Warning "Cleaning build directory..."
        Remove-Item -Recurse -Force "build"
    } else {
        Write-Warning "Build directory already exists"
    }
}

if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Push-Location "build"

# Configure with CMake
Write-Status "Configuring with CMake..."
$cmakeArgs = @(
    "..",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
)

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed"
    Pop-Location
    exit 1
}

Write-Success "CMake configuration completed"

# Build the project
Write-Status "Building the project..."
$buildArgs = @(
    "--build", ".",
    "--config", $BuildType,
    "--parallel"
)

& cmake @buildArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed"
    Pop-Location
    exit 1
}

Write-Success "Build completed successfully"

# Run tests if requested
if ($Test) {
    if (Test-Path "windows_compatibility_test.exe") {
        Write-Status "Running Windows compatibility tests..."
        & .\windows_compatibility_test.exe
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Some tests failed"
        } else {
            Write-Success "All tests passed"
        }
    } else {
        Write-Warning "Test executable not found"
    }
}

Pop-Location

# Create necessary directories
Write-Status "Creating necessary directories..."
@("logs", "data", "docs") | ForEach-Object {
    if (-not (Test-Path $_)) {
        New-Item -ItemType Directory -Path $_ | Out-Null
    }
}

# Copy configuration example
Write-Status "Setting up configuration..."
if (-not (Test-Path "config.json")) {
    if (Test-Path "config.json.example") {
        Copy-Item "config.json.example" "config.json"
        Write-Warning "Created config.json from example. Please edit it with your settings."
    }
}

Write-Success "Configuration setup completed"

# Show build information
Write-Status "Build Information:"
Write-Host "  Platform: Windows" -ForegroundColor $White
Write-Host "  Architecture: x64" -ForegroundColor $White
Write-Host "  Build Type: $BuildType" -ForegroundColor $White
Write-Host "  Compiler: MSVC" -ForegroundColor $White

Write-Success "Build completed successfully!"
Write-Status "Executable location: build\$BuildType\solana_arbitrage_bot.exe"
Write-Status "Configuration file: config.json"
Write-Status "Log directory: logs\"

Write-Status "To run the bot: .\build\$BuildType\solana_arbitrage_bot.exe --help"
