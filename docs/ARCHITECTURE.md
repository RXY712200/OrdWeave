# Architecture

Technical baseline: **OrdWeave v1.1.0**, Windows x64 C17, LayerKeySort Full4.2 pinned at `f751e1ee2afd682874281d6dc495969f9eca49eb`. Actual release/version status and exact source provenance are recorded in [HANDOFF](HANDOFF.md).

[中文使用](USAGE.zh-CN.md) · [完整API](IMPLEMENTATION_API_REFERENCE.zh-CN.md) · [执行导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) · [内部实现](IMPLEMENTATION_INTERNALS.zh-CN.md) · [验证地图](IMPLEMENTATION_VALIDATION.zh-CN.md) · [技术债](TECHNICAL_DEBT.md)

## Implemented foundation

OrdWeave implements an opaque in-memory database with typed fields, stable type-specific IDs, collections, and independently ordered occurrences. An internal adapter centralizes ID lookup/uniqueness, insertion/removal, ordered mutations, status translation, and handle expiry. Record and Collection ID indexes are public LksManagedOrders; permanent Occurrence IDs use a private compressed nibble-radix index. Each collection owns a public LksOrder. Comparator-visible IDs never change. The standalone smoke test exercises native LKS snapshots directly; production checkpoints also capture and restore native orders. A Windows append-only operation WAL provides durable commits.

LayerKeySort remains an independent upstream project: https://github.com/RXY712200/LayerKeySort. Its Full v4.2.0 Stable source is pinned as a Git submodule at `f751e1ee2afd682874281d6dc495969f9eca49eb`. Upstream sources are unmodified, embedded tests/examples/benchmarks/install rules are disabled, and integration uses `LayerKeySort::layerkeysort`.

## Ownership and boundaries

Database Core owns records, IDs, data semantics, and transactions. The OrdWeave-owned adapter translates calls to LayerKeySort Full using public APIs. Full handles are runtime-only and must never be treated as persistent database IDs.

The database uses LKS4SNP1 snapshots for order persistence while owning its own records, WAL, and recovery. The Phase 1 test's deterministic application identifiers demonstrate association and resolution only; they are not database ID allocation.

The SHA-256 helper, selected Windows IO/locking/path patterns and verified temporary-file publication adapt MIT-licensed Obscura64 code; see [third-party notices](../THIRD_PARTY_NOTICES.md) for exact provenance. ProbSpan is not a required runtime dependency.

The supported scope is Windows x64, single-writer operation, and up to three retained checkpoint generations. Linux and macOS support has not been validated. Known future optimization work is technical debt, not permission to compromise correctness; ownership, durability, and recovery invariants must take priority.

## Memory ownership and atomic publication

Records, Collections, and Occurrences are individually allocated at stable addresses. Their first member contains an immutable uint64_t ID; Record/Collection keep ManagedOrder index handles, while Occurrences use radix lookup and a Collection Order residence. Records own linked named fields and copied payloads; a reference count blocks deletion while occurrences exist. An Occurrence borrows its Record and Collection and owns one runtime residence in that collection. Repeated membership is not deduplicated.

Field mutations validate UTF-8, finite binary64 and checked budgets, prepare copies, then publish without further allocations. Record/collection creation prepares storage before the single atomic index insertion. IDs and Revision advance only after publication. The counters reject overflow; no-ops are detected before Revision capacity checks.

Occurrence insertion prepares at most a leaf and branching page in the compressed16-way radix, without changing the live tree. The explicit LKS Collection Order insertion must succeed before allocation-free radix publication and refs/ID/Revision updates. Failure frees only prepared pages, preserving old objects and structure. Removal first succeeds in the explicit Order, then clears the radix slot, frees an empty leaf and collapses a single-child branch without allocation. Search uses at most16 nibble levels; memory follows occupied leaves/branches rather than the uint64 high water. Moves remain public LKS operations and preserve IDs/associations.

Record/Collection mutations retain the adapter's immutable-ID comparator and unique-key checks. Field changes prepare authoritative copied values before publication; optional advisory cache maintenance can allocate afterward, but failure drops cache/configuration without failing committed data. No-op and failed authoritative mutations preserve valid cache state.

Destruction first clears the owned field cache, then destroys every Collection order, expiring its residences. It chains occurrence items through private teardown links, destroys the occurrence index, then frees those objects. Radix pages borrow objects and are freed before Occurrence objects. Destruction repeats detach-index-before-free for Collections and Records (including fields). This requires no allocation or LKS mutation and remains valid even at an internal revision capacity. No object is freed while an LKS container borrows it, and no expired handle is queried.

## Limits and verification

Central public limits are 1 MiB per field payload and 8 MiB per Record budget. Names are also bounded to 1 MiB. The budget is a logical size accounting rule, not a wire format: sum of 17 bytes metadata plus UTF-8 name bytes plus value payload (null=0, bool=1, int/float=8, string/binary=byte length). This includes replacement subtraction and checked remaining capacity. The persistent WAL frame limit is 16 MiB; native checkpoint files are limited to 256 MiB. These are separate from the logical Record budget.

Tests cover typed data, copying, constraints, references, queries, no-ops, deterministic array-model operations, allocation failures at OrdWeave-owned boundaries, and counter exhaustion. Fault injection is confined to a separately compiled white-box test translation unit; no global production fault hook exists. Upstream allocation failures are handled via public strong-atomicity contracts but are not exhaustively injected.

No multi-operation transactions, SQL, secondary field indexes, cross-collection atomic transfer, or unsupported concurrency promises are implemented. The caller must serialize all database access; objects and returned IDs must not be used with a different database.

## Persistence boundary

Public API wrappers in persistence.c dispatch to the same core implementation via private names. Ordinary in-memory calls bypass WAL encoding. Persistent calls mark the session busy, prepare a deterministic frame, apply the core operation, append and flush, then acknowledge outputs. An uncertain append quarantines the session until close/reopen; it cannot expose mutated memory through any database query. Recovery calls that same core and checks exact IDs and Revision, preserving one data model and the unchanged LKS adapter.

wal_format.c owns explicit encoding/bounded parsing; wal_windows.c owns path validation, handles, exclusive coordination, segmentation, append/flush and recovery. sha256.c is selectively transplanted from Obscura64. Exact ownership, format, initialization/rotation publication and failure states are specified in PERSISTENCE.md. No files are written by ordinary ow_database_create. Persistent destruction closes OS ownership after freeing core memory and does not create a final shutdown snapshot.

## Checkpoint integration

Private core export/import helpers serialize typed Records, ID high-water marks, Collection snapshots and Occurrence relationships. The narrow included checkpoint_core.inc keeps core object layouts private; checkpoint_windows.inc shares the existing Windows session and verified file primitives without a new storage-provider layer. Runtime LKS handles remain private and are rebuilt by traversal of restored native orders.

Recovery structurally validates all retained WAL, independently audits its semantics, verifies candidate anchors, restores a fresh unpublished core from a validated checkpoint and applies subsequent operations to that result before logical comparison with direct replay. It preserves the existing model and public LKS indexes. Separate generation metadata leaves all v3 files unchanged. Checkpoint retention defaults to two, configurable up to three; deletion follows verified current/backup agreement. See CHECKPOINTS.md for exact grammar, failure states and known extra WAL scanning cost. Checkpoint maintenance alone deletes no historical WAL; explicit anchored cleanup is a separate maintenance API.

## History and anchored recovery

The narrow history_windows.inc module shares existing Windows file/session primitives, WAL parsing, native checkpoints and core restoration. Historical views own unpublished reconstructed cores behind an opaque read-only facade. Recovery roots explicitly replace genesis ancestry only after a baseline and suffix independently reproduce current state. Prefix deletion occurs only after matching durable current/backup roots; object high-water counters come from the native checkpoint and unchanged replay. Backup copies are validated standalone directories, not runtime snapshots borrowing source pointers. Verification reconstructs actual persisted material and never repairs it. See HISTORY.md for exact formats and crash decisions.

## Phase 6 semantic validation hardening

Checksum/syntax validation alone is insufficient. Before exposing recovered or historical state, the implementation now independently replays every retained mutation from genesis or the validated root baseline, rejecting failed operations, no-op frames and ID/revision inconsistencies even before a later checkpoint. The native-checkpoint-plus-tail result is compared against allocation-free logical comparison with direct replay, including fields, high-water counters and order. Historical targets are likewise checked against direct target replay after validating the complete retained chain. This deliberately adds reconstruction work; no overall startup-speed improvement is promised. Disk versions and native LKS serialization are unchanged. See RELEASE_CANDIDATE.md for the reproduced regression and accepted performance cost.

## Session field index and typed ordered queries

field_index.inc is owned by ordweave.c alongside occurrence_index.inc and checkpoint_core.inc. One optional named-field ManagedOrder owns immutable value/ID rows and copied variable payloads, borrows Records, and uses a Record backlink for relevant maintenance. Enable builds privately and atomically replaces; failure preserves the old cache. Budget counts present entries and copied field name including NUL plus String/Binary payload lengths, excluding fixed rows/upstream metadata. Disabled mode has no construction/maintenance allocation but retains the per-Record pointer cost.

Matching equality/range uses locate/traverse; otherwise range scans and uses public LKS Stable Sort. Results own copied IDs, typed value then permanent-ID ordering, finite Float zeros equal, byte-span String/Binary order. Missing is not Null. Maintenance failure discards configuration without automatic rebuild. Exchange/destruction clear before freeing Records; replay/import/reopen/history never inherit cache. Guards quarantine uncertain commits. See [all48 contracts](IMPLEMENTATION_API_REFERENCE.zh-CN.md).

## Import and anchor work reduction

The checkpoint importer constructs Occurrences directly in an unpublished core/radix after validating ID/association/counts; native restore alone creates Order residences. It verifies namespace, unique resolution, all-seen counts and minimum construction Revision before publishing recorded counters. Candidate loading reuses only successful identical (revision,digest) anchor checks within that call, at most six entries. It never caches trust across calls or bypasses semantic replay, hard errors, root validation or logical comparison.
