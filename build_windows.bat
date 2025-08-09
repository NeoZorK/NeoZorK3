@echo off
REM Windows Build Script for NeoZorK3 Arbitrage Bot
REM This script builds the project on Windows with vcpkg

setlocal enabledelayedexpansion

REM Colors for output
set "RED=[91m"
set "GREEN=[92m"
set "YELLOW=[93m"
set "BLUE=[94m"
set "NC=[0m"

REM Function to print colored output
:print_status
echo %BLUE%[INFO]%NC% %~1
goto :eof

:print_success
echo %GREEN%[SUCCESS]%NC% %~1
goto :eof

:print_warning
echo %YELLOW%[WARNING]%NC% %~1
goto :eof

:print_error
echo %RED%[ERROR]%NC% %~1
goto :eof

REM Check if running from Developer Command Prompt
call :print_status "Checking Windows environment..."

REM Check for Visual Studio
where cl >nul 2>&1
if %errorlevel% neq 0 (
    call :print_error "Visual Studio compiler (cl.exe) not found"
    call :print_status "Please run this script from Visual Studio Developer Command Prompt"
    call :print_status "Or install Visual Studio 2019+ with C++ development tools"
    exit /b 1
)

REM Check for CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    call :print_error "CMake not found"
    call :print_status "Please install CMake from https://cmake.org/download/"
    exit /b 1
)

REM Check for vcpkg
if not defined VCPKG_ROOT (
    if exist "C:\vcpkg\vcpkg.exe" (
        set "VCPKG_ROOT=C:\vcpkg"
    ) else (
        call :print_error "VCPKG_ROOT not set and vcpkg not found in C:\vcpkg"
        call :print_status "Please install vcpkg:"
        call :print_status "  git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg"
        call :print_status "  cd C:\vcpkg"
        call :print_status "  .\bootstrap-vcpkg.bat"
        call :print_status "  .\vcpkg integrate install"
        exit /b 1
    )
)

call :print_success "Environment check passed"

REM Install dependencies if not already installed
call :print_status "Checking vcpkg dependencies..."

if not exist "%VCPKG_ROOT%\installed\x64-windows\include\boost\asio.hpp" (
    call :print_status "Installing Boost libraries..."
    "%VCPKG_ROOT%\vcpkg.exe" install boost-system:x64-windows boost-thread:x64-windows boost-beast:x64-windows
    if %errorlevel% neq 0 (
        call :print_error "Failed to install Boost libraries"
        exit /b 1
    )
)

if not exist "%VCPKG_ROOT%\installed\x64-windows\include\openssl\ssl.h" (
    call :print_status "Installing OpenSSL..."
    "%VCPKG_ROOT%\vcpkg.exe" install openssl:x64-windows
    if %errorlevel% neq 0 (
        call :print_error "Failed to install OpenSSL"
        exit /b 1
    )
)

if not exist "%VCPKG_ROOT%\installed\x64-windows\include\nlohmann\json.hpp" (
    call :print_status "Installing nlohmann/json..."
    "%VCPKG_ROOT%\vcpkg.exe" install nlohmann-json:x64-windows
    if %errorlevel% neq 0 (
        call :print_error "Failed to install nlohmann/json"
        exit /b 1
    )
)

call :print_success "Dependencies check passed"

REM Create build directory
call :print_status "Creating build directory..."
if exist "build" (
    call :print_warning "Build directory already exists. Cleaning..."
    rmdir /s /q build
)
mkdir build
cd build

REM Configure with CMake
call :print_status "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
if %errorlevel% neq 0 (
    call :print_error "CMake configuration failed"
    exit /b 1
)

call :print_success "CMake configuration completed"

REM Build the project
call :print_status "Building the project..."
cmake --build . --config Release --parallel
if %errorlevel% neq 0 (
    call :print_error "Build failed"
    exit /b 1
)

call :print_success "Build completed successfully"

REM Run tests if available
if exist "windows_compatibility_test.exe" (
    call :print_status "Running Windows compatibility tests..."
    windows_compatibility_test.exe
    if %errorlevel% neq 0 (
        call :print_warning "Some tests failed"
    ) else (
        call :print_success "All tests passed"
    )
)

REM Go back to project root
cd ..

REM Create necessary directories
call :print_status "Creating necessary directories..."
if not exist "logs" mkdir logs
if not exist "data" mkdir data
if not exist "docs" mkdir docs

REM Copy configuration example
call :print_status "Setting up configuration..."
if not exist "config.json" (
    if exist "config.json.example" (
        copy config.json.example config.json
        call :print_warning "Created config.json from example. Please edit it with your settings."
    )
)

call :print_success "Configuration setup completed"

REM Show build information
call :print_status "Build Information:"
echo   Platform: Windows
echo   Architecture: x64
echo   Build Type: Release
echo   Compiler: MSVC
echo   CMake: %CMAKE_VERSION%

call :print_success "Build completed successfully!"
call :print_status "Executable location: build\Release\solana_arbitrage_bot.exe"
call :print_status "Configuration file: config.json"
call :print_status "Log directory: logs\"

call :print_status "To run the bot: build\Release\solana_arbitrage_bot.exe --help"

pause
