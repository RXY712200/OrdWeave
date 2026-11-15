# OrdWeave

OrdWeave is a C17 embedded database project built around independently maintained LayerKeySort Full. Phase 1 provides only a static library, a version query, and a real ordering and snapshot round-trip integration test. It is not yet a usable database.

The intended first stable release targets Windows x64, single-writer operation, persistent records and ordering, WAL recovery, and up to three retained checkpoint generations. These features are planned, not implemented. There is no CRUD, transaction, persistence, query engine, or OrdWeave LKS adapter in Phase 1. Linux and macOS have not been validated.

## Build

Requires Git, CMake 3.21+, and a C17 compiler. For Visual Studio on Windows:

```powershell
git clone --recurse-submodules https://github.com/RXY712200/OrdWeave.git
cd OrdWeave
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For an existing clone, run `git submodule update --init --recursive`. With GCC and Ninja, configure using `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` instead.

LayerKeySort Full v4.1.0 Stable is pinned at `70d7f0ef5857b54381b863d292a213909d87459f` under `third_party/LayerKeySort`. Only its public API is used. The smoke test keeps serialized snapshots in memory; it writes no database files.

See [roadmap](docs/ROADMAP.md), [architecture](docs/ARCHITECTURE.md), and [third-party notices](THIRD_PARTY_NOTICES.md). OrdWeave is MIT licensed.
