# Architecture

## Implemented foundation

OrdWeave implements an opaque in-memory database with typed fields, stable type-specific IDs, collections, and independently ordered occurrences. An internal adapter centralizes ID lookup/uniqueness, insertion/removal, ordered mutations, status translation, and handle expiry. Each of three ID indexes is a public LksManagedOrder. Each collection owns a public LksOrder. Comparator-visible IDs never change. Only the standalone Phase 1 test uses snapshots; the database performs no persistence.

LayerKeySort remains an independent upstream project: https://github.com/RXY712200/LayerKeySort. Its Full v4.1.0 Stable source is pinned as a Git submodule at `70d7f0ef5857b54381b863d292a213909d87459f`. Upstream sources are unmodified, embedded tests/examples/benchmarks/install rules are disabled, and integration uses `LayerKeySort::layerkeysort`.

## Planned ownership and boundaries

Database Core owns records, IDs, data semantics, and transactions. The OrdWeave-owned adapter translates calls to LayerKeySort Full using public APIs. Full handles are runtime-only and must never be treated as persistent database IDs.

The database will use LKS4SNP1 snapshots for order persistence while owning its own records, WAL, and recovery. The Phase 1 test's deterministic application identifiers demonstrate association and resolution only; they are not database ID allocation.

Future persistence code may selectively incorporate modified code from the user's MIT-licensed Obscura64 project, retaining applicable attribution. No such code is included now. ProbSpan is not a required runtime dependency.

The initial stable scope is Windows x64, single-writer operation, and up to three retained checkpoint generations. Linux and macOS support has not been validated. Known future optimization work is technical debt, not permission to compromise correctness; ownership, durability, and recovery invariants must take priority.

## Memory ownership and atomic publication

Records, Collections, and Occurrences are individually allocated at stable addresses. Their first member is an indexed immutable uint64_t ID and its private managed-order handle. Records own linked named fields and copied payloads; a reference count blocks deletion while occurrences exist. An Occurrence borrows its Record and Collection and owns one runtime residence in that collection. Repeated membership is not deduplicated.

Field mutations validate UTF-8, finite binary64 and checked budgets, prepare copies, then publish without further allocations. Record/collection creation prepares storage before the single atomic index insertion. IDs and Revision advance only after publication. The counters reject overflow; no-ops are detected before Revision capacity checks.

Occurrence insertion and removal prepare a replacement ManagedOrder index borrowing the same stable live objects (with the new item or excluding the removed item). Temporary index handles do not overwrite live object handles. After the explicit collection operation succeeds, publication destroys the old index and installs new handles without allocation. On any preparatory or collection-operation failure, the temporary index is destroyed and published state is unchanged. This avoids rollback exhaustion of the old index's private revision counter. Every item is uniqueness-checked before adding a new indexed ID; rebuilding existing entries preserves the established uniqueness invariant.

This deliberately costs O(N log N) work and O(N) temporary index storage for occurrence insertion/removal. It is known optimization debt. Future optimization must preserve correctness, especially multi-structure allocation failures and counter capacity. Moves use LKS's strong atomicity directly. Deletes from record/collection indexes use validated live handles and no allocation; these indexes have no failed-publication churn, so their mutation counts cannot exceed database Revision.

Destruction first destroys every Collection order, expiring its residences. It chains occurrence items through private teardown links, destroys the occurrence index, then frees those objects. It repeats detach-index-before-free for Collections and Records (including fields). This requires no allocation or LKS mutation and remains valid even at an internal revision capacity. No object is freed while an LKS container borrows it, and no expired handle is queried.

## Limits and verification

Central public limits are 1 MiB per field payload and 8 MiB per Record budget. Names are also bounded to 1 MiB. The budget is a logical size accounting rule, not a wire format: sum of 17 bytes metadata plus UTF-8 name bytes plus value payload (null=0, bool=1, int/float=8, string/binary=byte length). This includes replacement subtraction and checked remaining capacity. No WAL frame limit is defined.

Tests cover typed data, copying, constraints, references, queries, no-ops, deterministic array-model operations, allocation failures at OrdWeave-owned boundaries, and counter exhaustion. Fault injection is confined to a separately compiled white-box test translation unit; no global production fault hook exists. Upstream allocation failures are handled via public strong-atomicity contracts but are not exhaustively injected.

No disk persistence, durable transactions, historical Revision queries, SQL, secondary field indexes, cross-collection atomic transfer, or unsupported concurrency promises are implemented. The caller must serialize all database access; objects and returned IDs must not be used with a different database.
