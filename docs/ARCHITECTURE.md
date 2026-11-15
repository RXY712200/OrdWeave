# Architecture

## Implemented foundation

OrdWeave currently exposes only a version query from a C17 static library. The standalone smoke test directly exercises the public LayerKeySort Full API. No adapter or database engine exists yet.

LayerKeySort remains an independent upstream project: https://github.com/RXY712200/LayerKeySort. Its Full v4.1.0 Stable source is pinned as a Git submodule at `70d7f0ef5857b54381b863d292a213909d87459f`. Upstream sources are unmodified, embedded tests/examples/benchmarks/install rules are disabled, and integration uses `LayerKeySort::layerkeysort`.

## Planned ownership and boundaries

Database Core will own records, IDs, data semantics, and transactions. A future OrdWeave-owned adapter will translate calls to LayerKeySort Full using public APIs. Full handles are runtime-only and must never be treated as persistent database IDs.

The database will use LKS4SNP1 snapshots for order persistence while owning its own records, WAL, and recovery. The Phase 1 test's deterministic application identifiers demonstrate association and resolution only; they are not database ID allocation.

Future persistence code may selectively incorporate modified code from the user's MIT-licensed Obscura64 project, retaining applicable attribution. No such code is included now. ProbSpan is not a required runtime dependency.

The initial stable scope is Windows x64, single-writer operation, and up to three retained checkpoint generations. Linux and macOS support has not been validated. Known future optimization work is technical debt, not permission to compromise correctness; ownership, durability, and recovery invariants must take priority.
