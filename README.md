> **ARCHIVED (2026-09-30): prototype, not functional, no longer maintained.**
> This repository is read-only history. The arbitrage bot never worked: it finds no
> opportunities, signs no transactions and reads no real prices. The only part that is
> reusable is the RPC endpoint discovery/scanner (see below).

# NeoZorK3: Solana DEX arbitrage bot (skeleton)

A C++20 skeleton of a Solana DEX arbitrage bot. It contains the CLI, the logger and an
RPC endpoint discovery/scanner. The trading logic (market data, opportunity search,
transaction building and signing, risk checks, order execution) consists of stubs.

This is not a working trading tool. Do not run it with real funds and do not expect
it to produce profit.

## What works / what is a stub

Verified by reading the code at archive time.

| Area | Status | Files | Notes |
|------|--------|-------|-------|
| CLI parsing | Works | `src/cli_parser.cpp`, `src/command_handlers.cpp` | A few `TODO` flags left |
| Logger (console) | Works | `src/logger.cpp` | File output and rotation are empty stubs (`write_to_file`, `rotate_log_file`) |
| RPC endpoint discovery | Works | `src/endpoint_discovery.cpp` | Loads endpoint lists from chain-list sources; some source types are skipped ("not yet supported") |
| RPC endpoint scanner | Partial | `src/endpoint_scanner.cpp`, `src/connection_manager.cpp` | HTTPS scan implemented; `wss`, `http`, `ws`, `ipc` scans are `TODO` |
| Config loading | Stub | `src/config_manager.cpp:20-44` | `load_config`, `save_config`, `validate_config` return `true`; `main` uses a default `Config`, so `config.json` is not actually read |
| Arbitrage engine | Stub | `src/arbitrage_engine.cpp` | Every `find_opportunities` / `detect_*` returns an empty vector; engine loop only sleeps |
| Market data | Stub | `src/market_data_provider.cpp` | Prices and order books are empty `Decimal()` / `OrderBook()` |
| Blockchain adapters | Stub | `src/blockchain_adapters.cpp` | Signatures are `"placeholder_signature"`; transaction serialization and swap building are placeholder strings |
| Order manager | Stub | `src/order_manager.cpp` | Order executor returns empty `Order()`; queue loop only sleeps |
| Risk manager | Stub | `src/risk_manager.cpp` | All checks return `true`, all metrics return `Decimal()` |
| Tests | Trivial | `tests/` | One small test per module; they pass but only exercise the stubs (`tests/test_basic.cpp` checks basic types) |
| Python demos | Fake data | `tools/demos/` | Prices are generated with `random.uniform`; they demonstrate nothing real |

## Corrections to earlier claims

Earlier versions of this README and of `docs/en/IMPLEMENTATION_STATUS.md` were wrong:

- Triangular, cross-DEX and statistical arbitrage are not implemented (empty stubs).
- Risk management (limits, drawdown, daily loss, slippage) is not implemented.
- There is no "ultra-fast" execution and no benchmark was ever measured. The former
  performance numbers, lock-free queues, memory pools and SIMD claims were not backed by code.
- Raydium, Orca, Jupiter, Serum and OpenBook adapters do not exist as working code.
- Key encryption, TLS-only RPC handling and audit logging are not implemented.
- The statistics output shown in older versions was an illustration, not real output.
- The former claim of full test coverage was false.

## The only reusable part

The RPC endpoint discovery and scanner (`src/endpoint_discovery.cpp`,
`src/endpoint_scanner.cpp`, `src/connection_manager.cpp`, with `include/endpoint_*.h`)
can be lifted into another project. Treat it as partial: only HTTPS scanning is done.

## Building (for reference)

Requirements: C++20 compiler, CMake 3.20+, Boost (system, thread), OpenSSL,
nlohmann_json, Google Test.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make test
```

Command line options are listed by `--help`. See
[docs/en/BUILD_INSTRUCTIONS.md](docs/en/BUILD_INSTRUCTIONS.md) for details.

## Documentation

- [Implementation status (honest matrix)](docs/en/IMPLEMENTATION_STATUS.md)
- [Original project plan](docs/en/PROJECT_PLAN.md): goals only, not what was built
- [Build instructions](docs/en/BUILD_INSTRUCTIONS.md)
- Older self-written reports were moved to [docs/archive/](docs/archive/) and may be inaccurate.
- Other guides in `docs/en/` (trading, testnet, airdrop) describe features that are
  not functional in this code base.

## License

MIT License, see [LICENSE](LICENSE).

## Disclaimer

Educational and research code. Trading cryptocurrencies involves substantial risk of
loss. Use at your own risk; the authors are not responsible for any financial losses.
