# Preview.4 — capability research and proposed contract

Unreleased `1.1.0-preview.4`, exact accepted parent `edb9675b10125cecdf936e030bd9a54a90a84146`, Full4.2 `f751e1ee2afd682874281d6dc495969f9eca49eb`. Steps13–15 of the approved six-stage/twenty-step plan are research-complete. **No public capability is implemented by this stage.** Public API remains44 functions. No production data-path change is selected; only version/guidance/validator changes enter the development branch. Prototypes live only on the separate evidence branch. No RC.1 is started.

## Decision matrix

| Capability / real use | Appropriate public LKS calls | Evidence / cost | Decision |
|---|---|---|---|
|Repeated named-field equality / filtered sorted UI|ManagedOrder create/insert/locate/remove/next, immutable copied keys with permanent-ID tie|Actual DB scans vs derived index;128/8192 records, selective/nonselective, reads and update/delete maintenance, memory and failure fallback|Worthwhile opt-in query/index contract proposed below; retain existing automatic scan implementation until explicit approval.|
|One-shot sorted query result|`lks_sort`, `lks_group_build`|Independent stable insertion-sort model;8192 pointer results about0.2483/0.2541ms|Use supported LKS sort if an ordered-query API is approved. No replacement of canonical qsort or forced production use now.|
|Recurring immutable sorted batches|GroupBatch build/merge_all; Group build/merge|8192 build+merge0.2562ms; two reusable Groups64 merges1.8057ms; borrowed lifetime explicitly tested|Useful implementation option for future read results; no separate batch API or transaction proposal.|
|Collection changes across revisions|Permanent Occurrence IDs over independent OwHistory results; public Snapshot/Cursor/LS1 only for domain/lifetime checks|Actual persistent views verify inserted6/deleted2/reordered5 and changed Record1; insertion-only rank shift is not movement; pruned view rejected|Do not add a public diff API now. Small quadratic LCS is research only; ambiguous movement and scalable result contract need more justification. Existing copied arrays permit client-side analysis.|
|Canonical checkpoint field sort|Existing unique-name qsort|No duplicate name stability need, unchanged canonical bytes/failure semantics|Retain; no allocating replacement.|

## Step13: semantics and lifecycle

Actual live `ow_query_equal` guards the session, then scans Record ManagedOrder twice and returns increasing permanent Record IDs with one exact result allocation. History delegates to the same core query on an independently reconstructed immutable core. Fields own name/value bytes. Set/remove/delete, WAL apply, native import, core exchange and destruction all affect any derived-cache lifetime; a hidden automatic maintained index would require coherent handling across all of them. Existing no-op, failed mutation, quarantine and result ownership remain unchanged in production.

Prototype keys are immutable copies while resident. Comparator order is presence, type, value, permanent Record ID: missing is distinct from present Null; mixed types never coerce; signed Int comparison avoids subtraction overflow; finite Float uses numeric order with both zeros equal; String/Binary compares unsigned byte spans then lengths, including embedded NUL. Public input validation must reject nonfinite Float before comparison. The measured named field is integer-valued; comparator unit assertions cover missing/Null, type separation, zeros and NUL/prefix byte cases, not a complete production index implementation for every type.

ManagedOrder does not provide transactions/reserve/bulk construction. The private prototype performs the successful DB mutation, then removes the old immutable key before replacing its row key and inserting. Any maintenance removal/capacity or insertion/OOM failure destroys the derived container and falls back to the existing scan. It never attempts allocating inverse rollback of the database, exposes stale keys or turns cache-maintenance failure into a failed committed mutation. All Row payloads remain alive until the borrowed index is destroyed. A linker-injected actual allocation failure during maintenance verifies fallback results against the model. This is a bounded external prototype, not a complete native cache lifecycle integrated into replay/import/exchange.

### Paired protocol and results

Protocol frozen before measuring: exact Preview.3 runtime, seed17-derived deterministic field distributions,128/8192 populations,257-value selective and1-value nonselective cases,512 reads, one warmup and three measured runs. Mixed workload is a separate phase of50 updates/50 deletions followed by512 reads, not interleaved traffic. Setup/build/maintenance/read spans are separate. Initial update-only and borrowed-row measurements are retained as exploratory evidence; the final index separately owns its copied integer-key Row arena, and both allocation and materialization are included in build cost; the mixed update/delete matrix is the reported final set. Baseline/indexed order alternates. Read spans conservatively include independent expected-result scans in both variants; they are not isolated lookup timings.

|8192 workload median total build+maintenance+reads|Scan ms|Indexed prototype ms|
|---|---:|---:|
|Selective, reads only|51.8508|2.8532|
|Selective, mixed maintenance|47.9283|2.8232|
|Nonselective, reads only|49.9864|25.2955|
|Nonselective, mixed maintenance|50.3880|25.2967|

Estimated amortization from component medians is roughly a few dozen reads in the measured selective cases (exact component estimates are in the final report). These estimates are not directly measured single-query break-even guarantees and include the common oracle cost. One-shot queries pay a build cost and copied-key memory; hidden automatic maintenance is therefore not selected solely on repeated-read wins. Resource runs separately measure requested allocation, not RSS:8192 selective read-only built requested bytes about1.50MB scan/common model versus2.19MB indexed, roughly0.69MB extra including a separately allocated copied-key Row arena in this integer fixture. Strings/Binary would require copied payload accounting beyond this measured workload.

### One ranked public addition — explicit approval required

Rank1: **bounded opt-in named-field ordered/range querying**, with an optional live derived index. No other public addition is ranked. Suggested function family below is a concrete review proposal, not declarations added to the public header:

```c
OwStatus ow_field_index_enable(OwDatabase *db, const char *field,
                              size_t max_records, size_t max_key_bytes);
OwStatus ow_field_index_disable(OwDatabase *db);
OwStatus ow_query_range(OwDatabase *db, const char *field, OwType type,
                       const OwValue *lower, int lower_inclusive,
                       const OwValue *upper, int upper_inclusive,
                       OwRecordId **out, size_t *count);
OwStatus ow_history_query_range(const OwHistory *view, const char *field,
                       OwType type, const OwValue *lower, int lower_inclusive,
                       const OwValue *upper, int upper_inclusive,
                       OwRecordId **out, size_t *count);
```

Proposed precise behavior:

- Existing types/statuses/result release only; no persisted metadata. One active named-field index per live DB, process/session-local and explicitly enabled. Positive limits cap indexed entries and owned copied-key bytes, not exact upstream/OS resident bytes. Enable prepares a full replacement before publishing; INVALID/NOMEM/CAPACITY preserves prior enable state. Disable is idempotent. Configuration consumes no logical Revision or permanent ID and is not WAL-logged.
- Range query works whether an index is enabled/valid or not. Missing fields excluded; present Null is a real typed value. Bounds are optional pointers (NULL means unbounded), non-NULL bounds must have the explicit requested type; finite Float, canonical zeros and existing bytes/name validation apply. Inclusivity is0/1; lower>upper returns INVALID; equal bounds with either exclusive returns an empty result. No mixed-type coercion. Result is ascending typed value, then increasing permanent Record ID; this intentionally differs from existing equality's ID-only order and requires approval. Ascending-only first version avoids an unnecessary direction contract.
- Output array is an independently owned copy released by `ow_array_release`, initialized NULL/zero on error; empty succeeds with NULL/zero. Current guard/error validation precedes work; invalid argument→INVALID, resource failure→NOMEM or CAPACITY, busy/quarantine→existing statuses. It does not expose runtime Handles, keys, borrowed values or a public cursor. Query itself does not advance Revision.
- Cache is advisory: failure/budget exhaustion during best-effort maintenance invalidates it without changing a successful data mutation's status, IDs or Revision. Subsequent queries scan/sort or explicitly rebuild under the same serialized operation; they never trust Revision alone after replay/import/exchange. Failed/no-op mutations can retain a known-valid index; invalidation is always safe. Commit-unknown/recovery-required blocks access through existing guards. Mutation/replay/import/exchange/destruction must drop or rebuild owned derived state before releasing old keys/objects. No cache survives close/open or transfers silently between independent DBs.
- Immutable history query uses the view's own reconstructed state and copied temporary results; no live index is shared and no historical audit is bypassed. A view unavailable through pruning must be obtained through existing `ow_history_open` and remains unavailable. No new view-source/cursor lifetime guarantee is implied.
- First implementation may scan and use public LKS Stable Sort, then add the explicit ManagedOrder optimization with bounded failure/lifecycle tests. Equivalent cached/uncached results and fallback are mandatory. Approximate prototype break-even is not an API latency promise.

This family is not approved or implemented. If owner approval follows, integration belongs within Step15 before RC.1, not a new roadmap step. No extra enable/read semantics are silently grafted onto existing `ow_query_equal` now.

## Step14: stable sorted results and immutable groups

The sort prototype compares copied pointer-array LKS Stable Sort, immutable Group build, GroupBatch build/merge_all and two Groups built then merged. An independent stable insertion-sort reference checks every resulting pointer/ID with equal-key runs. Input is ID-ascending, so stable equal-key order also gives permanent-ID ties; general batch merge must instead use an explicit ID tie comparator or preserve known input chunk precedence. Base wins comparator equality on Group merge; arbitrary independent streams do not automatically produce global ID ties.

At8192 rows, median sort0.2482ms, Group0.2541ms, Batch+merge0.2562ms, two-build+merge0.2513ms. Reusing both immutable groups for64 owned merges takes1.8057ms. Build/copy costs are included; no claim that Batch is best for one-shot work. Groups borrow rows, batch owns its groups, merge creates an owned container, and copied DB results must own IDs/values independently. Destroy groups before source Row storage. No alternate sorting/index library, generic importer, durable batch mutation or commit reordering is added.

## Step15: historical/order analysis and rejected extra API

Actual persistent historical views outlive the live database; copied arrays remain valid after view closure. Example before order[1,2,3,4,5], after[5,1,3,4,6] yields inserted6/deleted2 and retained longest common subsequence[1,3,4], hence a canonical reordered candidate5. Shared Record1's integer field changes17→31. Insertion-only[1,2,3]→[9,1,2,3] preserves all relative order despite every old rank shifting.

The prototype's deterministic LCS tie rule skips the earlier sequence on equal alternatives. It is quadratic, bounded to tiny inputs and not proposed as a production large-diff algorithm. Views alone cannot uniquely recover explicit move operations from WAL; a reordered candidate is a minimal relative-order explanation, not an action log. Duplicate Record references are distinguished by permanent Occurrence IDs. A production diff would need source identity embedded with views, a scalable retained-ID/LIS mapping and explicit ambiguous-order/result ownership semantics; existing public history does not expose a source-identity getter. No public diff contract is selected without that justification.

Public LKS cursor invalidation is asserted after an actual move, before accessing cached handles. Cursors are destroyed before their owner. Snapshot association bytes are copied and key text remains valid after live Order destruction; same-domain keys compare, different revision namespaces reject comparison. LS1 coordinates are snapshot-local, never permanent IDs or cross-domain diff keys. A separate real retained-root fixture rejects earliest-1 with HISTORY_UNAVAILABLE; no fabricated old state is returned.

## Validation, debt and scope

All prototype files and raw/resource/failure/history results are on `evidence/v1.1.0-preview.4` under `handoff/v1.1.0-preview.4/`; no unused prototype enters production. Only runtime version and matching smoke/consumer literals change, so no new persistence compatibility campaign is needed. Existing suites/docs and exact-head Windows CI are reported at handoff. Historical RC/Stable and accepted Preview.1–3 validator gates remain; current production bytes must exactly equal accepted Preview.3 apart from version literal. No API or data-path gate is relaxed.

Technical Debt Impact: no existing TD status changes. TD-001's index improvement, TD-002/003 partial mitigations, TD-004 deferred, TD-005/006 open and TD-103 resolved history remain. EG-001/002/003 remain; no EG-003 standalone reproduction campaign. The proposed features are pending product decisions, not automatically technical debt. Research completion is distinct from public capability implementation and independent acceptance.
