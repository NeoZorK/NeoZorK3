#!/bin/bash

# NeoZorK3 Tools Quick Access
# Simple wrapper for the tools runner

set -e

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${GREEN}🔧 NeoZorK3 Tools${NC}"
echo -e "${BLUE}Quick access to all tools and demos${NC}"
echo

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    echo "❌ Python3 is required but not installed"
    exit 1
fi

# Check if tools directory exists
if [ ! -d "tools" ]; then
    echo "❌ Tools directory not found. Make sure you're in the project root."
    exit 1
fi

# Check if run_tool.py exists
if [ ! -f "tools/run_tool.py" ]; then
    echo "❌ Tools runner not found: tools/run_tool.py"
    exit 1
fi

# Pass all arguments to the Python script
python3 tools/run_tool.py "$@"
