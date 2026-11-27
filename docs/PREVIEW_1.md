# Preview.1 integration and architecture decisions

Development only: `1.1.0-preview.1`, based on Stable `32f853defc0a2e9f0ca809fb3535a90c977c7665`. This is not a published Preview. LayerKeySort Full4.2 Stable is pinned at `f751e1ee2afd682874281d6dc495969f9eca49eb`. Existing public API and formats are unchanged. Historical RC guides remain source-bound to RC.1; this document describes the explicit development delta.

## Complete pinned public capability inventory

The pinned installed header declares exactly **64** functions; the table contains each once. Current-use columns were checked against all six production C files and three owned includes. Retain means reviewed and still appropriate, not a new implementation. Future dispositions are proposals only. Upstream status values outside the existing translation cases still map to OW_INTERNAL; expected call paths satisfy callbacks/lifetime/serialization and do not introduce such errors as public database statuses.

| Family | Public function | Current production use | Disposition / relevant capability and reason | Candidate work |
|---|---|---|---|---|
| Diagnostics | `lks_status_string` | None | **Not applicable** — Status translation preserves OrdWeave status values; textual upstream diagnostics are optional, not a new public status. | No planned runtime change |
| Stable sort | `lks_sort` | None | **Research** — Could replace existing field-pointer qsort or order query results if allocation/failure and tie semantics justify it. | Later sorting design |
| Explicit Order | `lks_order_create` | `src/ordweave.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_destroy` | `src/checkpoint_core.inc`, `src/ordweave.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_size` | `src/checkpoint_core.inc`, `src/ordweave.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_insert_front` | `src/lks_adapter.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_insert_back` | `src/lks_adapter.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_insert_before` | `src/lks_adapter.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_insert_after` | `src/lks_adapter.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_remove` | `src/ordweave.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_first` | `src/checkpoint_core.inc`, `src/ordweave.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_last` | `src/ordweave.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_next` | `src/checkpoint_core.inc`, `src/ordweave.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_previous` | `src/ordweave.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_item` | `src/checkpoint_core.inc`, `src/lks_adapter.c`, `src/ordweave.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_compare` | `src/ordweave.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_move_front` | `src/lks_adapter.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_move_back` | `src/lks_adapter.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_move_before` | `src/lks_adapter.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| Explicit Order | `lks_order_move_after` | `src/lks_adapter.c` | **Retain** — Owned collection order borrows Occurrences; anchors remain same-order/live, no-op is identified before revision capacity. | Current integration |
| ManagedOrder | `lks_managed_order_create` | `src/lks_adapter.c` | **Retain** — Three typed permanent-ID indexes; immutable ID comparator, distinct locate outputs and stable borrowed residence lifetimes. | Current integration |
| ManagedOrder | `lks_managed_order_destroy` | `src/ordweave.c` | **Retain** — Three typed permanent-ID indexes; immutable ID comparator, distinct locate outputs and stable borrowed residence lifetimes. | Current integration |
| ManagedOrder | `lks_managed_order_size` | `src/checkpoint_core.inc` | **Retain** — Three typed permanent-ID indexes; immutable ID comparator, distinct locate outputs and stable borrowed residence lifetimes. | Current integration |
| ManagedOrder | `lks_managed_order_insert` | `src/lks_adapter.c`, `src/ordweave.c` | **Retain** — Three typed permanent-ID indexes; immutable ID comparator, distinct locate outputs and stable borrowed residence lifetimes. | Current integration |
| ManagedOrder | `lks_managed_order_locate` | `src/lks_adapter.c` | **Retain** — Three typed permanent-ID indexes; immutable ID comparator, distinct locate outputs and stable borrowed residence lifetimes. | Current integration |
| ManagedOrder | `lks_managed_order_remove` | `src/lks_adapter.c` | **Retain** — Three typed permanent-ID indexes; immutable ID comparator, distinct locate outputs and stable borrowed residence lifetimes. | Current integration |
| ManagedOrder | `lks_managed_order_first` | `src/checkpoint_core.inc`, `src/ordweave.c` | **Retain** — Three typed permanent-ID indexes; immutable ID comparator, distinct locate outputs and stable borrowed residence lifetimes. | Current integration |
| ManagedOrder | `lks_managed_order_last` | None | **Research** — Existing ID comparator is immutable and total; unused traversal/compare variants add no needed capability. No public transaction/reserve/bulk API. | Preview.2 ID-index research |
| ManagedOrder | `lks_managed_order_compare` | None | **Research** — Existing ID comparator is immutable and total; unused traversal/compare variants add no needed capability. No public transaction/reserve/bulk API. | Preview.2 ID-index research |
| Cursor | `lks_order_cursor_create` | None | **Research** — Internal traversal already serializes lifetime; future streaming must expose invalidation and owner lifetime safely. | Later query design |
| Cursor | `lks_managed_order_cursor_create` | None | **Research** — Internal traversal already serializes lifetime; future streaming must expose invalidation and owner lifetime safely. | Later query design |
| Cursor | `lks_order_cursor_next` | None | **Research** — Internal traversal already serializes lifetime; future streaming must expose invalidation and owner lifetime safely. | Later query design |
| Cursor | `lks_order_cursor_destroy` | None | **Research** — Internal traversal already serializes lifetime; future streaming must expose invalidation and owner lifetime safely. | Later query design |
| Snapshot | `lks_order_snapshot_capture` | `src/checkpoint_core.inc` | **Retain** — Checkpoint native capture/serialize/strict deserialize/namespace-count validation/restore; fresh handles rebound after restore. | Current integration |
| Snapshot | `lks_managed_order_snapshot_capture` | None | **Research** — Native checkpoint uses owned snapshot bytes and resolver associations; other provenance/getter operations need a concrete export use. | Checkpoint/export research |
| Snapshot | `lks_order_snapshot_is_current` | None | **Research** — Native checkpoint uses owned snapshot bytes and resolver associations; other provenance/getter operations need a concrete export use. | Checkpoint/export research |
| Snapshot | `lks_managed_order_snapshot_is_current` | None | **Research** — Native checkpoint uses owned snapshot bytes and resolver associations; other provenance/getter operations need a concrete export use. | Checkpoint/export research |
| Snapshot | `lks_snapshot_destroy` | `src/checkpoint_core.inc` | **Retain** — Checkpoint native capture/serialize/strict deserialize/namespace-count validation/restore; fresh handles rebound after restore. | Current integration |
| Snapshot | `lks_snapshot_count` | `src/checkpoint_core.inc` | **Retain** — Checkpoint native capture/serialize/strict deserialize/namespace-count validation/restore; fresh handles rebound after restore. | Current integration |
| Snapshot | `lks_snapshot_namespace` | `src/checkpoint_core.inc` | **Retain** — Checkpoint native capture/serialize/strict deserialize/namespace-count validation/restore; fresh handles rebound after restore. | Current integration |
| Snapshot | `lks_snapshot_association` | None | **Research** — Native checkpoint uses owned snapshot bytes and resolver associations; other provenance/getter operations need a concrete export use. | Checkpoint/export research |
| Keys / legacy migration | `lks_snapshot_key_length` | None | **Not applicable** — No V3 database import or public textual key contract. LS1 keys are snapshot coordinates, not permanent IDs. | Only a separately authorized migration/export task |
| Keys / legacy migration | `lks_snapshot_key_format` | None | **Not applicable** — No V3 database import or public textual key contract. LS1 keys are snapshot coordinates, not permanent IDs. | Only a separately authorized migration/export task |
| Keys / legacy migration | `lks_snapshot_key_validate` | None | **Not applicable** — No V3 database import or public textual key contract. LS1 keys are snapshot coordinates, not permanent IDs. | Only a separately authorized migration/export task |
| Keys / legacy migration | `lks_snapshot_key_compare` | None | **Not applicable** — No V3 database import or public textual key contract. LS1 keys are snapshot coordinates, not permanent IDs. | Only a separately authorized migration/export task |
| Snapshot | `lks_snapshot_serialized_size` | `src/checkpoint_core.inc` | **Retain** — Checkpoint native capture/serialize/strict deserialize/namespace-count validation/restore; fresh handles rebound after restore. | Current integration |
| Snapshot | `lks_snapshot_serialize` | `src/checkpoint_core.inc` | **Retain** — Checkpoint native capture/serialize/strict deserialize/namespace-count validation/restore; fresh handles rebound after restore. | Current integration |
| Snapshot | `lks_snapshot_deserialize` | `src/checkpoint_core.inc` | **Retain** — Checkpoint native capture/serialize/strict deserialize/namespace-count validation/restore; fresh handles rebound after restore. | Current integration |
| Snapshot | `lks_snapshot_restore_order` | `src/checkpoint_core.inc` | **Retain** — Checkpoint native capture/serialize/strict deserialize/namespace-count validation/restore; fresh handles rebound after restore. | Current integration |
| Keys / legacy migration | `lks_order_import_v3_lk1` | None | **Not applicable** — No V3 database import or public textual key contract. LS1 keys are snapshot coordinates, not permanent IDs. | Only a separately authorized migration/export task |
| Immutable group / batch | `lks_group_build` | None | **Research** — Borrowed immutable sorted results may serve read analytics; no automatic field index, WAL transaction or mutable database batch. | Later query/batch design |
| Immutable group / batch | `lks_group_destroy` | None | **Research** — Borrowed immutable sorted results may serve read analytics; no automatic field index, WAL transaction or mutable database batch. | Later query/batch design |
| Immutable group / batch | `lks_group_size` | None | **Research** — Borrowed immutable sorted results may serve read analytics; no automatic field index, WAL transaction or mutable database batch. | Later query/batch design |
| Immutable group / batch | `lks_group_item_at` | None | **Research** — Borrowed immutable sorted results may serve read analytics; no automatic field index, WAL transaction or mutable database batch. | Later query/batch design |
| Immutable group / batch | `lks_group_merge` | None | **Research** — Borrowed immutable sorted results may serve read analytics; no automatic field index, WAL transaction or mutable database batch. | Later query/batch design |
| Immutable group / batch | `lks_group_batch_build` | None | **Research** — Borrowed immutable sorted results may serve read analytics; no automatic field index, WAL transaction or mutable database batch. | Later query/batch design |
| Immutable group / batch | `lks_group_batch_destroy` | None | **Research** — Borrowed immutable sorted results may serve read analytics; no automatic field index, WAL transaction or mutable database batch. | Later query/batch design |
| Immutable group / batch | `lks_group_batch_size` | None | **Research** — Borrowed immutable sorted results may serve read analytics; no automatic field index, WAL transaction or mutable database batch. | Later query/batch design |
| Immutable group / batch | `lks_group_batch_group_count` | None | **Research** — Borrowed immutable sorted results may serve read analytics; no automatic field index, WAL transaction or mutable database batch. | Later query/batch design |
| Immutable group / batch | `lks_group_batch_group_size` | None | **Research** — Borrowed immutable sorted results may serve read analytics; no automatic field index, WAL transaction or mutable database batch. | Later query/batch design |
| Immutable group / batch | `lks_group_batch_group_at` | None | **Research** — Borrowed immutable sorted results may serve read analytics; no automatic field index, WAL transaction or mutable database batch. | Later query/batch design |
| Immutable group / batch | `lks_group_batch_merge_all` | None | **Research** — Borrowed immutable sorted results may serve read analytics; no automatic field index, WAL transaction or mutable database batch. | Later query/batch design |
| Snapshot | `lks_group_snapshot_capture` | None | **Research** — Native checkpoint uses owned snapshot bytes and resolver associations; other provenance/getter operations need a concrete export use. | Checkpoint/export research |
| Snapshot | `lks_group_snapshot_is_current` | None | **Research** — Native checkpoint uses owned snapshot bytes and resolver associations; other provenance/getter operations need a concrete export use. | Checkpoint/export research |

The V4 header exposes no general live Path/Tree API. The strict V3 LK1 import and LS1 snapshot key helpers above are the relevant public path/key boundary. Runtime Handles, LS1 coordinates and permanent database IDs are three different identities. Native `lks_snapshot_restore_order` publicly resolves associations and constructs an unpublished order in O(N); its internal bulk builder is private and must not be called by OrdWeave. ManagedOrder has no corresponding public bulk/transaction/reserve operation. Cursor revision checking detects mutation, not concurrent access or owner destruction. Immutable Groups borrow payloads; a snapshot copies association bytes, not database object ownership.

## Existing call-site acceptance

`lks_adapter.c` keeps a total uint64 ID comparison without subtraction overflow, duplicate lookup before stable upper-bound insertion, and exact-handle removal. IDs never change while resident. `ordweave.c` retains explicit front/back/before/after insert and move, endpoint/adjacency no-op detection, deterministic traversal and reference-count ownership. Neither comparator nor checkpoint association callback reenters its source. On failure, staged indexes never overwrite live handles. Destruction detaches borrowed items before destroying owning objects, including revision-capacity cases. `checkpoint_core.inc` preserves exact namespace, association identity, duplicate/collection/count and complete-seen checks; loaded snapshots have no live provenance, so is_current is not a restore-validation substitute. All calls remain frozen supported V4 APIs; no adapter rewrite is warranted.

## Module review and decision register

| Module | Decision and supported capability | Risk / future evidence needed |
|---|---|---|
|Record / fields|Retain owned linked fields and typed ID ManagedOrder; copied query values survive source destruction.|Secondary indexes must preserve typed equality, UTF-8 rules, finite floats, field budget and all mutation/replay/import paths.|
|Collection|Retain one explicit Order per collection; duplicate Record references are distinct Occurrences.|Cross-collection transfer is not an LKS move and would need a separately approved atomicity contract.|
|Occurrence|Retain staged replacement ID index for now (TD-001); LKS remains collection ordering engine.|Full rebuild scales with global membership; changing only Order internals cannot eliminate that work.|
|Queries|Retain two-pass equality scan in deterministic Record ID order.|Ordered/range queries need explicit type/null/tie/collation rules; not implied by comparator-managed residence order.|
|LKS adapter|Retain supported calls and existing status mapping.|No dangling/foreign handles, private symbols or mutable comparator fields.|
|Allocation / ownership|Retain prepare-before-publish, borrowed container items and allocation-free final publication.|In-place rollback must account for both LKS revision exhaustion and possible allocating undo; DB revision alone is insufficient.|
|Checkpoint export/import|Research redundant construction. Import currently creates a temporary empty collection Order, performs ordinary occurrence inserts (rebuilding global index repeatedly), then replaces constructed order with native restore.|A private unpublished graph builder could avoid work using public native restore, but must validate every association once, duplicates, counts, references, high waters, cleanup and resolver failures. No change now.|
|WAL / codec / Windows|Retain canonical single-operation encoding, chained hashes, exact flush/rotation and quarantine.|No group/batch helper replaces durable commit or COMMIT_UNKNOWN. No format or flush reduction.|
|History / verify|Retain independent semantic replay and logical state comparison (TD-002).|Checkpoint acceleration must not hide valid-checksummed illegal prefix commands or invalid later history.|
|Backup|Retain independent full copy, per-file flush, incomplete marker and private reopen/compare/verify (TD-004).|No shared/hardlink lifecycle or incremental protocol is authorized.|
|Retention|Retain mandatory native baseline and agreement of both roots before deletion; add early cleanup guard.|Busy/poisoned state is rejection, not permission to reconstruct or repair disk.|

### TD-001 alternatives (research, not implementation approval)

| Alternative | Benefit | Failure / capacity / ownership cost | Disposition |
|---|---|---|---|
|Existing complete replacement|Simple unpublished preparation; no allocation at publication; deterministic IDs.|O(N) index reconstruction per insert/remove and temporary memory; native import compounds it.|Retain baseline oracle.|
|In-place ManagedOrder|Potential logarithmic lookup/mutation without full copy.|Two structures must commit consistently. Undo insert can allocate; removal/undo consume upstream revision capacity. No public reserve/transaction API proves rollback. Preflight DB Revision is insufficient.|Research only; reject naive remove-and-reinsert rollback.|
|Bounded permanent-ID-range ManagedOrder partitions|Rebuild only affected partition, keep LKS ordering and ascending partition/ID iteration.|Directory growth and atomic publication, sparse deleted IDs, overflow boundaries, partition counts and extra lookup/traversal complexity. No ID reuse or nondeterministic iteration.|Preferred bounded prototype for comparison, not preselected winner.|
|Specialized permanent-ID index|Could decouple ID lookup from order revisions and enable allocation-free publication.|Additional implementation/invariants, high-water gaps, memory, OOM testing and maintenance cost. Must retain LKS collection ordering and deterministic ID iteration.|Compare measured cost and proof burden before approval.|

### Risk and priority

1. Cleanup guard: confirmed correctness defect, immediate narrow repair. Original busy reproducer returned OW_BUSY after writing the missing backup root. Fixed test additionally covers poisoned COMMIT_UNKNOWN, zeroed output and validated reopen. This corrects rejection ordering without a new public contract.
2. TD-001: first performance research target. New A/B memory operations improve, but full rebuild remains and historical separate profiles attribute most memory operation time to stage+publish. No new causal percentage is inferred from this uninstrumented A/B.
3. TD-002 and checkpoint redundant staging/TD-003: next feasibility review. Larger checkpoint/reopen/history cost is visible; independent semantic verification is mandatory. Attribution to IO, hashes, repeated import or scans is not measured separately in the new run.
4. TD-005: potentially tractable setup overhead, but new A/B does not isolate BCrypt initialization. Measure before process-wide provider lifecycle changes.
5. TD-004: Deferred full-copy IO cost; no incremental backup requirement authorized. TD-006: Open operational residue, lower urgency absent capacity evidence; never delete unknown recovery evidence automatically.
6. EG-001: hardware power-loss evidence still Deferred. EG-002: broader instrumentation remains Open; existing failure suites are useful, not a whole-process allocator/sanitizer proof. Historical TD-101/102 remain Resolved and unchanged.

Severity is descriptive: guard is a rejected-session side-effect defect; no demonstrated data loss or arbitrary execution. Performance priorities reflect source, bounded measurements and feasibility, not unmeasured production service-level claims.

## Later capability candidates

Secondary field indexes could reduce repeated selective equality scans, but must maintain all mutation, replay and checkpoint paths atomically and define memory/collation cost. Range/ordered query could use ManagedOrder or immutable sorted results with stable tie-by-permanent-ID semantics; it needs approved public API and result lifetime contracts. Stable sort can process copied pointer arrays with explicit OOM behavior; replacing field export sorting must preserve canonical byte order. Group/Batch can form immutable application results and stable merges, not database multi-operation transactions. Historical differences require matching permanent IDs across independently reconstructed views and distinguishing unavailable pruned history from deletion; LS1 key comparison across unrelated snapshot domains cannot supply that comparison.

## Proposed Preview.2 acceptance criteria (requires later approval)

Preserve the owner's fixed **six stages / twenty steps**: this task is only Preview.1 Steps 1–4. No new stage, step or implementation is started here; proposals below do not renumber the remaining plan. The task snapshot supplies the total and current four steps, not the full remaining twenty-step schedule, so no missing schedule is invented.

- Compare TD-001 alternatives on the same trace bytes at N128/2048 and one approved larger bounded scale; three measured repeats after warmup, raw rows and separate production/profile runs. Proposed target: >=2x improvement in larger insertion/removal work versus this pinned baseline, without >10% small-workload median regression beyond 1ms. Confirm working-set/temporary-allocation cost rather than claim it from wall time.
- Publish exact failure-atomicity arguments and injection tests for every reached allocation, handle rebinding, ID gaps, deterministic iteration, Record refs and DB/LKS revision-capacity boundaries. No-op/failed operations consume no DB ID/Revision. Existing correctness/fault/format suites remain enabled and passing.
- If checkpoint import is approved, demonstrate removal of redundant staging on a separately timed fixture with >=2x import speed target at approved larger scale and no increased peak memory, while independently rejecting missing/duplicate/wrong-collection associations and count/high-water corruption. Targets are prospective proposals, not achieved results.
- Preserve all 44 signatures/statuses, native and database format bytes, both-direction persisted compatibility, semantic audit and flush strength. Obtain exact-head Debug/Release CI and independent review. Any public/format change requires separate authorization.

Evidence is archived separately on `evidence/v1.1.0-preview.1` under `handoff/v1.1.0-preview.1/`; the development draft PR remains unmerged. No release tag or Release is created by this stage.

## Validation limitation

The first local Release suite reported checkpoint OOM boundary 126 as OW_CORRUPT rather than expected OW_NOMEM. Its database subsequently reopened successfully (Revision 21, native checkpoint used). Targeted runs on old baseline, dependency-only B and candidate passed; subsequent complete candidate Release and Debug suites each passed 12/12. The initial failure remains unexplained and is retained as EG-003, not erased by reruns or attributed to Full4.2 without evidence. Independent review should consider that limitation alongside exact-head MSVC CI.
