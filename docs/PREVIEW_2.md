# Preview.2 — core index decisions

Unreleased 1.1.0-preview.2, based exactly on accepted Preview.1 `3c5f371ca53c62912dc3d4d63aa11dc5277af020`, Full4.2 `f751e1ee2afd682874281d6dc495969f9eca49eb`. Only Steps5–8 of the approved six-stage/twenty-step roadmap are implemented. No Preview.3, API, storage-format or recovery-pipeline redesign.

## Selection and proof obligations

| Alternative | Decision |
|---|---|
|A complete ManagedOrder replacement|Retained as measured oracle. Safe publication but global O(N) preparation and temporary memory on every insertion/removal.|
|B in-place ManagedOrder|Rejected by reasoning. Cross-container undo can allocate or exhaust upstream revision; no public reserve/transaction operation proves safe inverse rollback. No unsafe prototype built.|
|C 256-ID partitioned ManagedOrder|Small working prototype. Bounded local rebuild; binary directory lookup, but geometric directory preparation copies O(P) entries and empty-partition removal shifts O(P). At N8192, paired index-operation median11.3013ms versus baseline588.2032ms. Viable, not selected.|
|D compressed nibble-radix|Selected. Two small preparation pages maximum, sixteen digit levels maximum, no directory-wide copy or rebuild. Leaves cover16 IDs; internal nodes branch on compressed common prefixes. LKS remains the Collection ordering engine.|

The initial uncompressed byte radix had excessive sparse allocation; compressed-byte and then compressed-nibble variants were measured before selection. Their raw negative resource results are retained. Prototype D timings near tens of microseconds are timer-sensitive; final public-database paired timings, not microbenchmark ratios, decide the performance target.

`src/occurrence_index.inc` is a private include owned by ordweave.c, not a seventh C translation unit. Index pages own only structural storage and borrow stable Occurrence pointers. A leaf holds up to16 pointers; branching nodes contain at least two children. Prefix shifts strictly decrease in multiples of four, so lookup/iteration-stack/destruction depth is at most16, independent of ID magnitude. Storage is O(occupied leaves + branching prefixes), never O(highest ID). Extreme sparse keys still cost more per live item than dense keys; they do not allocate the intervening ID space.

Preparation checks duplicate/count capacity and allocates a missing leaf plus at most one separating branch. That branch may reference an old subtree but is unpublished. Abort frees only newly owned pages. Existing-page insertion needs no index allocation. After the public LKS Order insertion succeeds, slot/root publication and counts cannot fail; only then refs, ID and database Revision advance. Failed LKS insertion discards prepared pages and object, preserving all old handles, membership, IDs and Revision.

Removal looks up the exact live object, checks database capacity, and calls allocation-free public LKS Order removal first. Success is followed by nonallocating slot removal and branch collapse; failure leaves index and object intact. The private index has no revision counter to exhaust. No-op moves are recognized before capacity checks and do not alter it. Destruction walks borrowed items without LKS index mutation, then releases pages. Record/Collection ManagedOrder indexes are unchanged.

All consumers use ascending digit traversal: checkpoint counts/export, resolver lookup/seen validation, same-state comparison and teardown. Ordinary checkpoint import still creates temporary order residences and then performs public native snapshot restore. This stage does not remove graph validation, semantic replay or alter bytes. WAL/history/backup continue through the same core calls. All44 public signatures/status values remain unchanged.

## Measurements and resource tradeoff

Protocol frozen before measurements: seed17 N128/2048 archived update-heavy traces, generated N8192 update-heavy, separate N128/8192 insertion/removal-heavy traces; 1000 operations each, one warmup, three measured repetitions, separate setup/operations, independent final order/association/field assertions. Same Windows x64 GCC Release host. Proposed target >=2x large insert/remove improvement, no small median regression >10% and >1ms.

| Public database operations | Preview.1 median ms | Preview.2 median ms |
|---|---:|---:|
|N128 update-heavy|1.5911|0.0811|
|N2048 update-heavy|21.8684|0.0953|
|N8192 update-heavy|94.3459|0.1218|
|N128 insert/remove-heavy|4.2554|0.1056|
|N8192 insert/remove-heavy|305.3153|0.1362|

The target is met in these generated workloads. Large ratios reflect removal of full-index rebuilds, not a universal database speed guarantee. Short candidate spans limit precision. This comparison isolates core changes against accepted Preview.1; it is not pooled with the earlier dependency experiment.

Separate requested-allocation diagnostics wrap project/upstream allocation calls, excluding wrapper headers and OS internals; this is not RSS. Prototype common item/schedule arrays are included equally. N8192 operation peak: A985625 bytes, C644433, selected D374817. N128 extremely sparse peak: A38104, C122496, D48944. Selected D has a documented sparse increase versus A, while reducing dense peak; the uncompressed-byte sparse prototype exceeded1.6MB and was rejected. Reporting itself leaves96/97 bytes of instrumentation/stdio accounting residue; those rows are not a zero-leak proof. Upstream allocation injection separately checks zero remaining upstream live blocks.

## Step7 retain decisions

Record/Collection immutable-ID ManagedOrders already provide ordered lookup and require only one container mutation; replacing them adds risk without TD-001's cross-container rebuild cost. Retain them. Field lists avoid additional per-field index ownership for small records; current equality query scans deterministic Record ID order twice to size one exact copied result. No measured requirement justifies another field index or result-growth policy. Keep typed equality, UTF8/embedded NUL, signed-zero, budgets and copied ownership.

Canonical checkpoint field sorting uses qsort by unique field name. Stable equality has no observable benefit when names are unique; switching to LKS Stable Sort would introduce allocation/failure semantics without demonstrated benefit. Retain qsort and exact canonical bytes. Collection relative placement/move/compare stays on supported Full4.2 public Order operations. No private upstream bulk builder or revision fields are used.

## Validation and debt

Existing randomized core model remains enabled. Failure tests add sparse/MAX IDs, branch/page transitions, prepare abort, ascending iteration, branch collapse, every new preparation allocation, safe outputs/refs/handle preservation and simulated public Order capacity rejection. An isolated diagnostics build sweeps actual upstream insertion allocation failures; no diagnostic flag is added to production CMake. Existing format, corruption, fault, retention and backup tests remain active. Exact commands/results and compatibility artifacts are published separately under `handoff/v1.1.0-preview.2/` on the evidence branch.

TD-001's global reconstruction is removed in this development candidate; historical rationale remains. TD-103 guard correction is retained. EG-003 stays Open: no standalone reproduction campaign was repeated; on recurrence capture the first failing site/status/ordinal and fixture before cleanup. Other TD/EG statuses and historic evidence remain. No independent acceptance is claimed.
