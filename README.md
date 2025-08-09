# NeoZorK3 Project

A high-performance decentralized exchange (DEX) arbitrage system written in C++17, designed for rapid detection and execution of profitable trading opportunities across multiple blockchains.

## 🚀 Quick Start

```bash
# Build the project
./build.sh

# Test the application
cd build && ./neozork3_cli --help

# Initialize configuration
./neozork3_cli --config-init

# Discover endpoints for Fantom
./neozork3_cli --discover-endpoints --blockchain Fantom --source chain

# Scan endpoints
./neozork3_cli --scan --blockchain Fantom
```

📖 **For detailed instructions, see [docs/BUILD_INSTRUCTIONS.md](docs/BUILD_INSTRUCTIONS.md)**

## 1. Project Goal

The primary objective of NeoZorK3 is the **rapid detection and execution of profitable decentralized exchange (DEX) arbitrage opportunities** across various blockchains. The target environment includes high-performance servers (like AWS EC2 Alpine Linux x86/ARM) with low latency connections to blockchain endpoints.

## 2. Core Features

* **Multi-Blockchain Support:** Designed to operate on Fantom, Solana, Ethereum, Avalanche, Sonic, BTC, and potentially others (identified via Network ID).
* **Advanced Arbitrage Strategies:**
    * Supports multiple arbitrage types: Direct (Pair), Triangular, Cross-Pool, Cross-DEX.
    * Allows selection of specific strategy types via the `--arbitrage-types` flag.
    * *(Future Scope)* Cross-Chain, CEX <-> DEX Arbitrage.
* **Dynamic Endpoint Management:**
    * **Discovery (`--discover-endpoints`):** Finds RPC endpoint URLs from sources like DefiLlama data, Chainlist.org, known aggregators, specified GitHub lists, official project documentation (manual lookup often required), etc. (`--source`). Downloads/parses data (with progress bar) for a chosen blockchain (`--blockchain`) and adds basic endpoint info (URL, possible types) to `NeoZorK-config`. Does not test connectivity. Providers like Alchemy, Infura, QuickNode, Ankr might also be sources but often require manual API key addition.
        * **Supported Keywords for `--source`:** `chain` (default, uses chainid.network), `defi` (uses DefiLlama), `eth` (uses ethereum-lists, Ethereum mainnet only). Can also provide direct HTTP/HTTPS URLs.
    * **Scanning (`--scan-endpoints`, `--scan-single-endpoint`):** Tests endpoints *already present* in `NeoZorK-config`. Requires selecting a blockchain (`--blockchain`). Scans either *all* its configured endpoints or a *single* specified endpoint URL. It attempts to connect using *all* connection types (http, https, ws, wss, ipc) listed for that endpoint in the config. Updates the endpoint's status fields in the config *per connection type*: `isActive` (true/false), `latency` (ms), and optionally `trafficIn`/`trafficOut` (bytes, estimated), `rpcResponseSizeBytes` (estimated).
    * **Block Speed Measurement (`--measure-block-speed`):** Connects to an active endpoint for a specified blockchain, measures the average time for new blocks, and stores it in `NeoZorK-config`.
    * **Connection Flexibility:** Supports multiple connection protocols: Local IPC, HTTP, HTTPS, WS, WSS. The default type for actions (like arbitrage) can be hinted with `--connection-type`.
* **DEX and Pool Discovery:**
    * **DEX Discovery (`--find-dexes`):** *Requires `--blockchain`*. Attempts to identify known DEX factory/router contracts on the specified blockchain via a chosen active endpoint, saving findings to `NeoZorK-config`. (Complex, chain-specific).
    * **Pool Discovery (`--find-pools`):** *Requires `--blockchain`* and likely `--dex`. For a given DEX, attempts to discover its liquidity pools (token pairs) via an active endpoint, saving findings to `NeoZorK-config`. (Complex, requires contract interaction).
* **Price & Pool Information:**
    * **Get Price (`--get-token-price`):** Retrieves the latest price(s) for a specified token using data previously gathered in `NeoZorK-config` (DEXes, Pools, active endpoints). Can be filtered by blockchain and DEX.
    * **Find Pools (`--find-pools-for-token`):** Lists all known pools from `NeoZorK-config` that contain a specific token.
* **Configuration (`NeoZorK-config`):**
    * Centralized JSON-formatted configuration file named `NeoZorK-config`.
    * Automatically created with a default template if not found next to the binary.
    * Stores structured information (see proposed structure).
    * Management commands: `--config-init` (recreate), `--config-archive`, `--config-restore`.
    * **Proposed Structure (Conceptual JSON - Enhanced):**
      ```json
      {
        "blockchains": [
          {
            "name": "Fantom",
            "networkId": 250,
            "blockSpeedMs": 1000, // Updated by --measure-block-speed
            "dexes": [ // Populated by --find-dexes
              { "id": "SpookySwap", "name": "SpookySwap", "routerAddress": "0x...", "factoryAddress": "0x..." }
            ],
            "pools": [ // Populated by --find-pools (structure TBD, might be nested)
              { "dexId": "SpookySwap", "poolId": "0x...", "token0": {"symbol": "FTM", "address": "..."}, "token1": {"symbol": "USDC", "address": "..."} }
            ],
            "endpoints": [
              {
                // --- Fields added by Discovery ---
                "url": "https://rpc.ftm.tools/",
                "supportedTypes": ["https", "wss"], // Potential connection types
                "rateLimits": { /* ... */ }, // Optional
                "accessToken": null, // Optional
                "parallelQueryAllowance": 5, // Optional
                // --- Fields added/updated by Scanning ---
                "status": {
                   "https": {
                     "isActive": true,
                     "latencyMs": 25.5,
                     "lastCheck": "2025-04-01T12:00:00Z",
                     "trafficInBytes": 1024,
                     "trafficOutBytes": 512,
                     "rpcResponseSizeBytes": 800
                   },
                   "wss": {
                     "isActive": false,
                     "latencyMs": null,
                     "lastCheck": "2025-04-01T12:00:01Z"
                   },
                   "ipc": null // Example if not configured/tested
                },
                "lastBlockNumber": 70000000 // Optional, updated by scanning or block speed check
              }
            ]
          }
        ]
      }
      ```
* **Performance & Efficiency:** Optimized C++ core, Async/Multithreaded, < 1GB RAM target, resource monitoring, OS analysis.
* **Risk Management & Profitability:** Considers chain specifics, strict profit checks, risk modes (`--strategy`), execution modes (`--mode`).
* **Execution & Monitoring:** Block time awareness, optional sync (`--sync-to-block`), secure wallet management, statistics logging, robust error handling.
* **User Interface & Experience:** CLI flags, comprehensive `--help`, colored progress bars, optional password, service/daemon mode.

## 3. Architecture & Modules

* `cli`: Handles all flag parsing.
* `config_manager`: Manages `NeoZorK-config` structure and access.
* `endpoint_discovery`: Handles `--discover-endpoints` (various sources).
* `endpoint_scanner`: Handles `--scan-endpoints`, `--scan-single-endpoint`, `--measure-block-speed`. Tests all configured types per endpoint.
* `connection_manager`: Provides connection capabilities (HTTP, WS, IPC).
* `blockchain_adapters`: Chain-specific interaction (block speed, DEX/pool discovery, contract calls for price fetching).
* `dex_connectors`: DEX-specific logic/ABIs (used by `blockchain_adapters`).
* `arbitrage_engine`: Core arbitrage logic, handles `--find-arbitrage-once`.
* `execution_engine`: Transaction building/signing/sending.
* `wallet_manager`: Secure key handling.
* `task_scheduler`: Handles `--run-tasks`.
* `resource_monitor`: System resource tracking.
* `statistics`: Logging trades/errors.
* `logging`: General application logging.
* `ui`: CLI output, progress bars, handles `--show-active-endpoints`, `--get-token-price`, `--find-pools-for-token` output formatting.
* `utils`: Common helpers.
* `daemonizer`: Service mode logic.

## 4. Technology Stack

* **Language:** C++17
* **Build System:** CMake 3.16+
* **Dependencies:** 
  * nlohmann/json (v3.11.3) - JSON parsing
  * cpp-httplib (v0.15.3) - HTTP client/server
  * OpenSSL - HTTPS support
* **Platform:** Cross-platform (Linux, macOS, Windows)

## 5. Development Guidelines

* **Code Style:** Procedural C++ with English comments
* **Modularity:** Logical separation of concerns
* **Memory Safety:** RAII, smart pointers where appropriate
* **Error Handling:** Comprehensive exception handling
* **Documentation:** English documentation in `docs/` folder

## 6. Typical Workflow

1. **Initial Setup:** Run `./build.sh` to build the project
2. **Configuration:** Initialize with `--config-init`
3. **Endpoint Discovery:** Use `--discover-endpoints --blockchain <name>`
4. **Endpoint Scanning:** Use `--scan --blockchain <name>` to test connectivity
5. **Verify Active Endpoints:** Use `--active --blockchain <name>`
6. **Measure Block Speed:** Use `--measure-block-speed --blockchain <name>`
7. **Discover DEXs/Pools:** Use `--find-dexes` and `--find-pools`
8. **Arbitrage Operations:** Use `--find-arbitrage-once` or `--run-tasks`

## 7. Documentation

* **[Quick Start Guide](docs/QUICK_START.md)** - Get up and running quickly
* **[Build Instructions](docs/BUILD_INSTRUCTIONS.md)** - Detailed build and setup instructions

## 8. License

This project is proprietary software. All rights reserved.

## 9. Contributing

Please refer to the development guidelines in section 5 and ensure all code follows the established patterns and standards.
