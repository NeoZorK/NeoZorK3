#!/bin/bash

# Solana Arbitrage Bot Build Script
# This script builds the project with optimal settings

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
        echo "windows"
    else
        echo "unknown"
    fi
}

# Function to get number of CPU cores
get_cpu_cores() {
    if command_exists nproc; then
        nproc
    elif command_exists sysctl; then
        sysctl -n hw.ncpu
    else
        echo 4  # Default fallback
    fi
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    local missing_deps=()
    
    # Check CMake
    if ! command_exists cmake; then
        missing_deps+=("cmake")
    fi
    
    # Check C++ compiler
    if ! command_exists g++ && ! command_exists clang++; then
        missing_deps+=("C++ compiler (g++ or clang++)")
    fi
    
    # Check make
    if ! command_exists make; then
        missing_deps+=("make")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_status "Please install the missing dependencies and try again."
        exit 1
    fi
    
    print_success "All required dependencies found"
}

# Function to install dependencies (Linux)
install_dependencies_linux() {
    print_status "Installing dependencies for Linux..."
    
    if command_exists apt-get; then
        # Ubuntu/Debian
        sudo apt-get update
        sudo apt-get install -y build-essential cmake libboost-all-dev libssl-dev libgtest-dev
    elif command_exists yum; then
        # CentOS/RHEL
        sudo yum groupinstall -y "Development Tools"
        sudo yum install -y cmake boost-devel openssl-devel gtest-devel
    elif command_exists dnf; then
        # Fedora
        sudo dnf groupinstall -y "Development Tools"
        sudo dnf install -y cmake boost-devel openssl-devel gtest-devel
    else
        print_error "Unsupported Linux distribution. Please install dependencies manually."
        exit 1
    fi
}

# Function to install dependencies (macOS)
install_dependencies_macos() {
    print_status "Installing dependencies for macOS..."
    
    if ! command_exists brew; then
        print_error "Homebrew not found. Please install Homebrew first:"
        print_status "/bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        exit 1
    fi
    
    brew install cmake boost openssl googletest
}

# Function to create build directory
create_build_dir() {
    print_status "Creating build directory..."
    
    if [ -d "build" ]; then
        print_warning "Build directory already exists. Cleaning..."
        rm -rf build
    fi
    
    mkdir -p build
    cd build
}

# Function to configure with CMake
configure_cmake() {
    print_status "Configuring with CMake..."
    
    local build_type=${1:-Release}
    local extra_flags=${2:-}
    
    local cmake_cmd="cmake .. -DCMAKE_BUILD_TYPE=$build_type"
    
    if [ ! -z "$extra_flags" ]; then
        cmake_cmd="$cmake_cmd $extra_flags"
    fi
    
    print_status "Running: $cmake_cmd"
    
    if ! eval "$cmake_cmd"; then
        print_error "CMake configuration failed"
        exit 1
    fi
    
    print_success "CMake configuration completed"
}

# Function to build the project
build_project() {
    print_status "Building the project..."
    
    local jobs=${1:-$(get_cpu_cores)}
    
    print_status "Using $jobs parallel jobs"
    
    if ! make -j"$jobs"; then
        print_error "Build failed"
        exit 1
    fi
    
    print_success "Build completed successfully"
}

# Function to run tests
run_tests() {
    print_status "Running tests..."
    
    if ! make test; then
        print_warning "Some tests failed"
        return 1
    fi
    
    print_success "All tests passed"
    return 0
}

# Function to create directories
create_directories() {
    print_status "Creating necessary directories..."
    
    mkdir -p logs
    mkdir -p data
    mkdir -p docs
    
    print_success "Directories created"
}

# Function to copy configuration example
copy_config_example() {
    print_status "Setting up configuration..."
    
    if [ ! -f "config.json" ] && [ -f "config.json.example" ]; then
        cp config.json.example config.json
        print_warning "Created config.json from example. Please edit it with your settings."
    fi
    
    print_success "Configuration setup completed"
}

# Function to show build information
show_build_info() {
    print_status "Build Information:"
    echo "  OS: $(detect_os)"
    echo "  CPU Cores: $(get_cpu_cores)"
    echo "  Build Type: ${BUILD_TYPE:-Release}"
    echo "  Parallel Jobs: ${JOBS:-$(get_cpu_cores)}"
    
    if command_exists g++; then
        echo "  Compiler: $(g++ --version | head -n1)"
    elif command_exists clang++; then
        echo "  Compiler: $(clang++ --version | head -n1)"
    fi
    
    echo "  CMake: $(cmake --version | head -n1)"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  -d, --debug             Build in debug mode"
    echo "  -r, --release           Build in release mode (default)"
    echo "  -j, --jobs N            Use N parallel jobs (default: auto-detect)"
    echo "  -i, --install-deps      Install dependencies"
    echo "  -t, --test              Run tests after build"
    echo "  -c, --clean             Clean build directory before building"
    echo "  -v, --verbose           Enable verbose output"
    echo ""
    echo "Examples:"
    echo "  $0                      # Build in release mode"
    echo "  $0 -d                   # Build in debug mode"
    echo "  $0 -j 8                 # Build with 8 parallel jobs"
    echo "  $0 -i -t                # Install deps, build, and test"
}

# Main function
main() {
    local build_type="Release"
    local jobs=$(get_cpu_cores)
    local install_deps=false
    local run_tests_flag=false
    local clean_build=false
    local verbose=false
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -d|--debug)
                build_type="Debug"
                shift
                ;;
            -r|--release)
                build_type="Release"
                shift
                ;;
            -j|--jobs)
                jobs="$2"
                shift 2
                ;;
            -i|--install-deps)
                install_deps=true
                shift
                ;;
            -t|--test)
                run_tests_flag=true
                shift
                ;;
            -c|--clean)
                clean_build=true
                shift
                ;;
            -v|--verbose)
                verbose=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    # Set verbose mode
    if [ "$verbose" = true ]; then
        set -x
    fi
    
    print_status "Starting Solana Arbitrage Bot build..."
    
    # Show build information
    show_build_info
    
    # Check dependencies
    check_dependencies
    
    # Install dependencies if requested
    if [ "$install_deps" = true ]; then
        local os=$(detect_os)
        case $os in
            linux)
                install_dependencies_linux
                ;;
            macos)
                install_dependencies_macos
                ;;
            *)
                print_error "Automatic dependency installation not supported for $os"
                print_status "Please install dependencies manually"
                ;;
        esac
    fi
    
    # Clean build directory if requested
    if [ "$clean_build" = true ] && [ -d "build" ]; then
        print_status "Cleaning build directory..."
        rm -rf build
    fi
    
    # Create build directory
    create_build_dir
    
    # Configure with CMake
    configure_cmake "$build_type"
    
    # Build the project
    build_project "$jobs"
    
    # Run tests if requested
    if [ "$run_tests_flag" = true ]; then
        run_tests
    fi
    
    # Go back to project root
    cd ..
    
    # Create necessary directories
    create_directories
    
    # Copy configuration example
    copy_config_example
    
    print_success "Build completed successfully!"
    print_status "Executable location: build/solana_arbitrage_bot"
    print_status "Configuration file: config.json"
    print_status "Log directory: logs/"
    
    if [ "$run_tests_flag" = false ]; then
        print_status "To run tests: $0 -t"
    fi
    
    print_status "To run the bot: ./build/solana_arbitrage_bot --help"
}

# Run main function with all arguments
main "$@"
