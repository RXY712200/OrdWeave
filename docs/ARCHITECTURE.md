# Architecture

## Implemented foundation

OrdWeave implements an opaque in-memory database with typed fields, stable type-specific IDs, collections, and independently ordered occurrences. An internal adapter centralizes ID lookup/uniqueness, insertion/removal, ordered mutations, status translation, and handle expiry. Each of three ID indexes is a public LksManagedOrder. Each collection owns a public LksOrder. Comparator-visible IDs never change. Only the standalone Phase 1 test uses LKS snapshots. Phase 3 adds a Windows append-only operation WAL; Phase 4 adds independent native-LKS checkpoints.

LayerKeySort remains an independent upstream project: https://github.com/RXY712200/LayerKeySort. Its Full v4.1.0 Stable source is pinned as a Git submodule at `70d7f0ef5857b54381b863d292a213909d87459f`. Upstream sources are unmodified, embedded tests/examples/benchmarks/install rules are disabled, and integration uses `LayerKeySort::layerkeysort`.

## Planned ownership and boundaries

Database Core owns records, IDs, data semantics, and transactions. The OrdWeave-owned adapter translates calls to LayerKeySort Full using public APIs. Full handles are runtime-only and must never be treated as persistent database IDs.

The database will use LKS4SNP1 snapshots for order persistence while owning its own records, WAL, and recovery. The Phase 1 test's deterministic application identifiers demonstrate association and resolution only; they are not database ID allocation.

Future persistence code may selectively incorporate modified code from the user's MIT-licensed Obscura64 project, retaining applicable attribution. The SHA-256 helper is now selectively adapted from Obscura64; see THIRD_PARTY_NOTICES.md. ProbSpan is not a required runtime dependency.

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

No multi-operation transactions, SQL, secondary field indexes, cross-collection atomic transfer, or unsupported concurrency promises are implemented. The caller must serialize all database access; objects and returned IDs must not be used with a different database.

## Phase 3 persistence boundary

Public API wrappers in persistence.c dispatch to the same core implementation via private names. Ordinary in-memory calls bypass WAL encoding. Persistent calls prepare a deterministic frame, mark the session busy, apply the core operation, append and flush, then acknowledge outputs. An uncertain append quarantines the session until close/reopen; it cannot expose mutated memory through any database query. Recovery calls that same core and checks exact IDs and Revision, preserving one data model and the unchanged LKS adapter.

wal_format.c owns explicit encoding/bounded parsing; wal_windows.c owns path validation, handles, exclusive coordination, segmentation, append/flush and recovery. sha256.c is selectively transplanted from Obscura64. Exact ownership, format, initialization/rotation publication and failure states are specified in PERSISTENCE.md. No files are written by ordinary ow_database_create. Persistent destruction closes OS ownership after freeing core memory and does not create a final shutdown snapshot.

## Phase 4 checkpoint integration

Private core export/import helpers serialize typed Records, ID high-water marks, Collection snapshots and Occurrence relationships. The narrow included checkpoint_core.inc keeps core object layouts private; checkpoint_windows.inc shares the existing Windows session and verified file primitives without a new storage-provider layer. Runtime LKS handles remain private and are rebuilt by traversal of restored native orders.

Recovery structurally validates all WAL history, verifies candidate anchors, restores a fresh unpublished core from a validated checkpoint and applies only subsequent operations. It preserves the existing model and public LKS indexes. Separate generation metadata leaves all v3 files unchanged. Checkpoint retention defaults to two, configurable up to three; deletion follows verified current/backup agreement. See CHECKPOINTS.md for exact grammar, failure states and known extra WAL scanning cost. Checkpoint maintenance alone deletes no historical WAL; Phase 5 introduces separate explicit anchored cleanup.

## Phase 5 history and anchored recovery

The narrow history_windows.inc module shares existing Windows file/session primitives, WAL parsing, native checkpoints and core restoration. Historical views own unpublished reconstructed cores behind an opaque read-only facade. Recovery roots explicitly replace genesis ancestry only after a baseline and suffix independently reproduce current state. Prefix deletion occurs only after matching durable current/backup roots; object high-water counters come from the native checkpoint and unchanged replay. Backup copies are validated standalone directories, not runtime snapshots borrowing source pointers. Verification reconstructs actual persisted material and never repairs it. See HISTORY.md for exact formats and crash decisions.

## Phase 6 semantic validation hardening

Checksum/syntax validation alone is insufficient. Before exposing recovered or historical state, the implementation now independently replays every retained mutation from genesis or the validated root baseline, rejecting failed operations, no-op frames and ID/revision inconsistencies even before a later checkpoint. The native-checkpoint-plus-tail result is compared against allocation-free logical comparison with direct replay, including fields, high-water counters and order. Historical targets are likewise checked against direct target replay after validating the complete retained chain. This deliberately adds reconstruction work; no overall startup-speed improvement is promised. Disk versions and native LKS serialization are unchanged. See RELEASE_CANDIDATE.md for the reproduced regression and accepted performance cost.
