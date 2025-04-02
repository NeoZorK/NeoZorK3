# NeoZorK3 Project

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
                "url": "[https://rpc.ftm.tools/](https://rpc.ftm.tools/)",
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
                     "lastCheck": "2025-04-01T12:00:01Z",
                     // ... traffic/size if applicable
                   },
                   "ipc": null // Example if not configured/tested
                },
                "lastBlockNumber": 70000000 // Optional, updated by scanning or block speed check
              },
              // ... more endpoints
            ]
          },
          // ... more blockchains
        ]
      }
      ```
* **Performance & Efficiency:** Optimized C++ core, Async/Multithreaded, < 1GB RAM target, resource monitoring, OS analysis.
* **Risk Management & Profitability:** Considers chain specifics, strict profit checks, risk modes (`--strategy`), execution modes (`--mode`).
* **Execution & Monitoring:** Block time awareness, optional sync (`--sync-to-block`), secure wallet management, statistics logging, robust error handling.
* **User Interface & Experience:** CLI flags, comprehensive `--help`, colored progress bars, optional password, service/daemon mode.

## 3. Architecture & Modules (Conceptual)

*(Modules remain largely the same, responsibilities adjusted)*
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

*(Same as before: C++17/Clang17, CMake, httplib, nlohmann_json, WebSocket lib, stdlib, cross-platform)*

## 5. Development Guidelines

*(Same as before: Procedural, English comments, Modularity, Memory safety, etc.)*

## 6. Typical Workflow / Usage Pattern

*(Steps 1-3 remain similar: Initial launch/init, Discover, Scan)*
4.  **Verify Active Endpoints:** Use `--show-active-endpoints --blockchain <name>` to see detailed status of usable endpoints (URL, types, latency, etc.).
5.  **(Optional) Measure Block Speed:** Use `--measure-block-speed --blockchain <name>`.
6.  **Discover DEXs/Pools:** Use `--find-dexes --blockchain <name>` and `--find-pools --blockchain <name> --dex <dex_id>` to populate market data. *Crucial for price checks and arbitrage.*
7.  **(Optional) Check Prices/Pools:** Use `--get-token-price` or `--find-pools-for-token` flags to query the gathered data.
8.  **Single Arbitrage Check:** Run `--find-arbitrage-once --blockchain <name> ...`
9.  **Continuous Monitoring:** Run `--run-tasks ...`

## 7. Getting Started / Usage (Preliminary Flags)

*(Config file `NeoZorK-config` is auto-created/used next to the binary)*

**General Options:**
* `--help`: Display detailed usage information.
* `--password <pass>`: Provide password if required for launch.
* `--blockchain <name>`: Specify target blockchain(s). (Required for most operations).
* `--connection-type <ipc|http|https|ws|wss>`: Specify *preferred* default connection protocol for actions (arbitrage, price checks). Default: `https`.

**Configuration Management:**
* `--config-init`: Deletes existing `NeoZorK-config` and creates a new default one.
* `--config-archive`: Backup the current `NeoZorK-config`.
* `--config-restore`: Restore `NeoZorK-config` from backup.

**Endpoint & Chain Info Management:**
* `--discover-endpoints --blockchain <name> --source <defillama|chainlist|aggregator_name|github_list_url>[,...]`: Discover RPC URLs from sources and add basic info to `NeoZorK-config`.
* `--scan-endpoints --blockchain <name>`: Tests *all configured connection types* for endpoints listed in config for the specified blockchain, updates status/latency fields per type.
* `--scan-single-endpoint --blockchain <name> --endpoint <url>`: Scan a specific endpoint URL, testing all its configured types.
* `--show-active-endpoints --blockchain <name>`: Lists detailed information for endpoints marked 'Active' (for any connection type) in the config for the specified blockchain.
* `--measure-block-speed --blockchain <name> [--endpoint <url>]`: Measure block time via an endpoint and save to config.
* `--find-dexes --blockchain <name> [--endpoint <url>]`: Attempt to discover DEX contracts and save to config. (Requires active endpoint).
* `--find-pools --blockchain <name> --dex <dex_id> [--endpoint <url>]`: Attempt to discover liquidity pools for a DEX and save to config. (Requires active endpoint).

**Data Querying (Requires Populated Config):**
* `--get-token-price <token_name>`: Show price across all blockchains/DEXes for the token.
* `--get-token-price --blockchain <name> <token_name>`: Show price for the token on a specific blockchain.
* `--get-token-price --blockchain <name> --dex <dex_id> <token_name>`: Show price for the token on a specific DEX/blockchain.
* `--find-pools-for-token <token_name>`: List all known pools containing the specified token across all blockchains.

**Running Arbitrage Tasks:**
* `--find-arbitrage-once`: Perform a single check/trade cycle based on other parameters.
* `--run-tasks`: Start continuous background monitoring/trading tasks.
* `--mode <show|trade>`: Operational mode for execution. Default: `show`.
* `--strategy <max-profit|stable-profit|min-risk>`: Overarching profit/risk strategy. Default: `stable-profit`.
* `--arbitrage-types <type1>[,<type2>,...|all]`: Specify arbitrage types to search for (e.g., `direct`, `triangular`). Default: `direct,triangular`.
* `--sync-to-block`: (Optional flag) Attempt to synchronize checks with new block events.
* ... (other flags for filtering, budget, wallet selection, etc. TBD)