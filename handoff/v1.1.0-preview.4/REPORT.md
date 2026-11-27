# OrdWeave Preview.4 — complete Steps13–15 handoff

**Research complete; no new public capability implemented.** One bounded opt-in index/typed ordered-range-query family is proposed for explicit owner approval. Existing equality scanning, canonical sorting and persistence paths are retained. No RC.1 starts from this handoff. Self-assessments below are implementation/research assessments, not independent acceptance.

## Exact references and scope

| Item | Value |
|---|---|
|Accepted Preview.3 parent|`edb9675b10125cecdf936e030bd9a54a90a84146`|
|Final development HEAD|`cf342dcf881bc9e3518ede042baeaed25f807da7`|
|Initial implementation/version/docs commit|`600773b14711b925a490865e4e459b62fc5e6789`|
|Development branch|`dev/v1.1.0-preview.4`|
|Stacked Draft PR|[PR #4](https://github.com/RXY712200/OrdWeave/pull/4), base `dev/v1.1.0-preview.3`|
|Evidence branch|`evidence/v1.1.0-preview.4`, independently based directly on accepted Preview.3|
|Full4.2 pin|Unchanged `f751e1ee2afd682874281d6dc495969f9eca49eb`|
|Main / Stable|Unchanged `32f853defc0a2e9f0ca809fb3535a90c977c7665`|
|Preview.1|Unchanged `3c5f371ca53c62912dc3d4d63aa11dc5277af020`|
|Preview.2|Unchanged `2bbf7640711e6561a5d5979f0de53a7f6ee0a1e8`|
|Preview.3|Unchanged accepted parent|

Task read from immutable `134d7841026553751f7226a294cd54a4d20620c0/tasks/v1.1.0-preview.4.md`; approved six-stage/twenty-step roadmap remains `ea1fdfca0ef54b758b4d3f9e52ecd229391c9cf9`. Only Steps13–15 were executed. No task/evidence merge, upstream edit, release operation, force push, global configuration change or earlier-branch write.

| Step | Self-assessment | What is complete |
|---|---|---|
|13 query/index|PASS — research|Actual scan/ManagedOrder comparison, selective/nonselective and update/delete workloads, owned keys, resource/break-even evidence, range assertions, maintenance failure fallback and concrete pending contract.|
|14 sort/group/batch|PASS — research|Public LKS Stable Sort/Group/GroupBatch/merge prototypes, independent stable expected IDs and ownership/cost comparisons. No forced production adoption.|
|15 history/selection/handoff|PASS — research and scoped version/docs implementation|Actual immutable-view example, relative-order vs rank semantics, snapshot-domain/cursor/pruning checks, one ranked proposal, retained-production decision, tests and publication.|

A pending public-contract decision is explicit, not an inferred authorization or a blocked research task. If approved later, implementation belongs within Step15 before RC.1; no new roadmap step is invented.

## Actual production diff

Production `src/ordweave.c` changes exactly one string: `1.1.0-preview.3`→`1.1.0-preview.4`. Smoke/consumer expectations make the same exact substitution. No public header, type, status, logical data operation, private radix, LKS Collection operation, checkpoint, WAL, SHA, history, backup, retention, build target, workflow or dependency changes. Six production TUs/four owned includes/five private headers remain. All44 existing signatures and ID/ref/Revision/no-op/serialization/ownership/failure contracts remain.

Development documents changed: AGENTS, CHANGELOG, both READMEs, HANDOFF, TECHNICAL_DEBT and new PREVIEW_4. Validator retains RC/Stable and exact accepted Preview.1–3 historical comparisons, then allows only the three exact current version transformations in protected production/test paths. API, format tables, source inventory, UTF8/navigation/anchor/example checks remain. No prototype or unused API implementation enters production. The final follow-up changes only the research document to include separately owned key materialization and corrected final measurements.

The full compact decision matrix, comparator/lifecycle reasoning and concrete four-signature proposal are included in [CONTRACT_AND_DECISIONS.md](CONTRACT_AND_DECISIONS.md) and [the immutable source document](https://github.com/RXY712200/OrdWeave/blob/cf342dcf881bc9e3518ede042baeaed25f807da7/docs/PREVIEW_4.md). Proposed signatures appear only as labeled documentation, not public header exports.

## Step13 evidence and retained scan decision

Live equality guards the session, validates inputs, scans Record ManagedOrder twice and returns increasing permanent Record IDs. History uses the same core scan over an independently reconstructed immutable core. Field values/names are owned copies; mutations, delete, replay, import, exchange and destruction must all invalidate any derived state coherently. This cross-cutting lifecycle is not implemented speculatively just because an isolated lookup is fast.

Private `query_prototype.c` uses actual OrdWeave create/field-set/delete/equality calls, public LKS ManagedOrder and independent Row expected results. Final index rows are a separately allocated copied integer-key arena, owned until its borrowing container is destroyed. Container ordering is type/value/permanent-ID, so ID0 lower-bound queries locate the first matching value and equal-key output is ID-ascending. Range correctness checks inclusive17..31 against an independent key-then-ID model outside timed reads.

Comparator definitions: missing distinct from present Null; no mixed-type coercion; Int compares safely; finite Float compares numerically with -0/+0 equal; String/Binary compares unsigned byte spans then lengths including embedded NUL; permanent ID breaks equal-value ties. Small assertions cover missing/Null, type separation, zeros, NUL/prefix bytes. Nonfinite values must be rejected before comparator use. The measured dynamic index is integer-only; it is not falsely represented as a complete all-types production implementation. String/Binary key materialization and validation remain mandatory future work.

### Frozen protocol and final metrics

[Protocol](PROTOCOL.md) was frozen before results. Windows x64 GCC Release/Ninja, exact accepted Preview.3 runtime and same Full4.2, fixed17-derived deterministic value/ID schedule, populations128/8192. Selective257-value and nonselective1-value fields,512 reads; read-heavy0 maintenance and mixed50 updates/50 deletions before the reads. One warmup, three measured fresh invocations per pair; alternate scan/index order. Setup, build, update and reads separately timed. Timing includes independent O(N) expected-result verification in **both** read variants, so it is conservative prototype workload cost, not pure lookup latency. Maintenance is phased, not interleaved traffic.

Final medians of each run's build+maintenance+read total:

|8192 records initially|Scan ms|Indexed prototype ms|
|---|---:|---:|
|Selective read-heavy|51.8508|2.8532|
|Selective mixed|47.9283|2.8232|
|Nonselective read-heavy|49.9864|25.2955|
|Nonselective mixed|50.3880|25.2967|

Raw component rows are in [query-raw.csv](query-raw.csv), component medians in [query-summary.json](query-summary.json), actual total medians in [query-total-summary.json](query-total-summary.json). Approximate amortization from component medians:128 selective read-only24 reads, mixed81;8192 selective11/12. These are estimated break-even points, not additional measured one-query experiments or latency promises. Maintenance can include redundant index work for DB no-op updates in the nonselective case; a future integrated implementation should retain the known-valid cache on no-op. It must not consume an extra logical Revision.

Resource runs are separate, requested allocations rather than RSS, and include the common DB/model arrays in both variants.8192 selective read-only built live bytes1,496,867 scan/common model and2,189,387 indexed:692,520 extra bytes for the copied-key arena plus LKS structures. Result-copy peak adds small output buffers; all raw resource phases are preserved in [query-resources.json](query-resources.json). Extra String/Binary payloads are not covered by this integer figure. Wrapper headers/OS internals are excluded; reporting residue is not a whole-process leak proof.

Initial update-only and borrowed-row experiments are retained in separate exploratory directories. A concrete methodology correction added the private index's owned Row arena and materialization to the measured build/memory costs; final metrics above use that correction. Earlier borrowed-row data is **not** used to claim final cost. Timing-floor small cases, common oracle work, deterministic distributions, phased maintenance and one-host/no-telemetry scope limit generalization. No comparison with other databases or cumulative old-stage performance ratio is made.

### Failure/ownership evidence

After a successful DB mutation, remove an old immutable key before replacing its comparator-visible value. If maintenance cannot remove/insert (capacity/OOM), destroy the derived container and its owned arena, then fall back to actual scanning; do not undo the committed database or expose stale keys. A linker-injected actual allocation failure during ManagedOrder reinsertion confirms `fault_seen` and complete invalidation;512 subsequent reads still match the independent model. [Failure result](fault-results.log), [fault allocator](faults.c), full [prototype](query_prototype.c). The index is advisory and externally simulated, not production-integrated. Keys and copied results do not borrow freed DB objects.

Automatic replacement of existing equality is rejected for now despite repeated-read gains: one-shot callers would pay construction/copied-key memory, and every live/history/replay/import/exchange/failure path needs bounded lifecycle integration. The concrete opt-in proposal can expose that cost choice without changing existing equality errors/order silently.

## Step14 evidence and stable ownership

`sort_prototype.c` compares copied pointer-array LKS Stable Sort, Group build, GroupBatch build+merge_all and two Group builds+merge. A separate stable insertion-sort oracle verifies every pointer/ID with equal-key runs outside measured work. It is a correctness oracle, not an alternate production sort library. Source IDs are ascending; stable ties therefore preserve ID order. Arbitrary merged sources need an explicit ID comparator or known chunk precedence; Group base wins equality and Batch respects original chunk order.

Final8192 build/copy medians: Stable Sort0.2482ms, Group0.2541ms, Batch+merge0.2562ms, two-build+merge0.2513ms.64 subsequent owned merges of the reusable groups take1.8057ms. [Raw](sort-raw.csv), [summary](sort-summary.json), [resources](sort-resources.json). Tiny128 results are retained without precise ratio claims. No asserted one-shot superiority of Batch/Group and no forced use of every API.

Groups borrow immutable Row payloads; Batch owns its groups; merge produces a separately owned result; do not destroy a borrowed batch chunk. All containers are destroyed before source rows. Proposed DB results must be independent ID/value copies. Existing canonical checkpoint qsort has unique field names, no meaningful stable tie requirement, and unchanged bytes/error semantics; retain it. No batch mutation, transaction, generic importer, alternate library or persisted format is added.

## Step15 historical/order evidence

`history_prototype.c` creates a real persistent DB, captures two independently reconstructed immutable histories and closes the live source before reading both. Before[1,2,3,4,5], after[5,1,3,4,6]: inserted6, deleted2; canonical retained LCS[1,3,4] explains reordered candidate5; shared Record1's field17→31 is changed. Copied membership arrays remain valid after history closure. Insertion-only[1,2,3]→[9,1,2,3] preserves relative order although every old rank shifts. Thus rank changes are not reported as explicit moves.

The tiny deterministic LCS uses O(N²) DP with bounded8-element buffers and a stated tie rule (skip earlier-sequence element on ties). It does not scale as a proposed implementation and cannot recover a unique explicit action sequence from two views. Duplicate Record references remain distinct via permanent Occurrence IDs. A scalable public diff would need identity-tagged views, retained-ID mapping/LIS or equivalent, ambiguity rules and owned result structures. Current public history does not expose a source-identity getter; this is another reason not to invent a cross-source diff contract now.

Public LKS Snapshot/Cursor/key tests assert: same-domain key order, cross-revision namespace mismatch rejection, copied snapshot-key validity after live Order destruction, actual-mutation cursor INVALIDATED before touching cached handles, and cursor destruction before owner destruction. LS1 text and live Handles are never used as durable IDs. A separate cloned retained-root fixture rejects earliest-1 with HISTORY_UNAVAILABLE; no synthetic missing old state is analyzed. [History results](history-results.log), [pruned results](pruned-results.log).

No public diff, cursor or batch API is ranked. Existing copied history arrays suffice for bounded client-side analysis; the prototype documents where that becomes insufficient rather than promising an unapproved feature.

## One pending contract, not an implementation

Rank1 only: bounded explicit named-field index enable/disable, plus typed ascending range queries for live and immutable history. The exact suggested signatures and all argument/error/ownership/lifetime rules are in [the full contract](CONTRACT_AND_DECISIONS.md). In brief:

- One active live named-field index, positive entry/copied-key-byte limits, process/session-local, no persistence or logical Revision. Enable prepares before replacing; failure preserves old configuration. Disable idempotent.
- Query bounds are optional existing OwValue pointers of the specified type, inclusive bits0/1; invalid/inverted bounds→INVALID, equal-exclusive→empty; missing excluded, Null a distinct present type, finite numeric/byte semantics preserved. Ascending value then permanent ID; independent OwRecordId array released by existing `ow_array_release`; NULL/zero output initialization.
- Cached and uncached results identical. Maintenance resource/capacity failure invalidates/falls back without changing a successful data mutation; no allocating rollback. Guarded quarantine access remains denied. Replay/import/exchange/destruction must invalidate owned derived state, never trust stale Revision/session state. History uses its own immutable core, never the live index; no audit bypass.
- Initial approved query implementation could scan and use public LKS Stable Sort, followed by opt-in ManagedOrder acceleration with coherent lifecycle tests. No Handle, LS1 key or borrowed/public cursor escapes.

**Explicit owner approval is required before any of these four suggested functions is added.** All44 existing signatures/status values remain. A pending approval is an explicit handoff item; no routine confirmation was requested mid-stage, and no automatic RC.1 transition occurs. Any later approved implementation remains within Step15, not a new roadmap step.

## Actual verification and publication

Principal commands executed locally (Windows GCC/MinGW16.2):

```powershell
python -X utf8 build/preview4/run_research.py
gcc -O3 -std=c17 -DFAULT -Iinclude -Ithird_party/LayerKeySort/include build/preview4/query_prototype.c build/preview4/faults.c build/preview3-release/libordweave.a build/preview3-release/third_party/LayerKeySort/liblayerkeysort.a -lbcrypt "-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc" -o build/preview4/query-fault.exe
build/preview4/query-fault.exe 8192 257 100 1
gcc -O3 -std=c17 -Iinclude -Ithird_party/LayerKeySort/include build/preview4/history_prototype.c build/preview3-release/libordweave.a build/preview3-release/third_party/LayerKeySort/liblayerkeysort.a -lbcrypt -o build/preview4/history.exe
build/preview4/history.exe D:/OrdWeave/build/preview4/history-fixture
build/preview4/history.exe pruned D:/OrdWeave/build/preview4/pruned-fixture
cmake -S . -B build/preview4-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/preview4-release --parallel 4
ctest --test-dir build/preview4-release --output-on-failure
python tools/validate_docs.py --examples --generator Ninja --config Release
python tools/validate_docs.py --examples --generator Ninja --config Debug
python tools/validate_docs.py
git diff --check
git diff --cached --check
```

Local Release existing CTest12/12,12.96s; documentation examples each3/3; validator44 declarations, six TUs/four includes/five headers,25 documents/499 relative links including anchors. No assertion was removed/weakened. Local complete Debug CTest was not rerun because only version/docs change; existing MSVC Debug/Release CI is the exact-head independent compiler gate. Documentation examples did build/run in Debug. No unnecessary persisted compatibility/byte campaign was invented for unchanged persistence, no upstream full suite/sanitizer/platform matrix, and no standalone EG-003 campaign. EG-003 did not recur and remains Open.

The published runner accepts `<checkout> <accepted-build-directory>` and bundles the requested-memory wrapper; it is adapted from the actual local path configuration without changing benchmark code. Use a disposable copy so reruns do not overwrite archived evidence. Independent history/OOM programs are separately inspectable and not hidden in production tests. Original local executables/fixtures remain ignored under build; they are not uploaded. Metadata records exact accepted runtime, final document HEAD, compiler/platform, binary and prototype source hashes. Query FAULT-only declarations and pruned-history CLI were added around unchanged default measured paths; binary fingerprints are not falsely asserted identical across those diagnostic builds.

Final exact-head MSVC CI status/job metadata and actual CTest excerpts are archived separately. Evidence-branch runs are not candidate CI; no polling loop or trigger change. Green CI does not constitute independent product acceptance.

Technical Debt Impact: **No status changes.** TD-001/103 resolved history, TD-002/003 partial mitigations, TD-004 Deferred, TD-005/006 Open and resolved TD-101/102 remain; EG-001/002/003 remain. Proposed product capabilities are not automatically unpaid technical debt. No historical evidence is rewritten.

Final delivery verifies parent/head/pin, prior branches/main, historical tags and published Release identities/body hashes. Development/evidence/upstream worktrees are clean after commits; no unknown initial changes were found or discarded. Remaining limitations are integer-only maintained prototype, generated phased workloads, common oracle timing overhead, requested rather than RSS memory, unimplemented all-path cache lifecycle, tiny ambiguous LCS and pending API approval. Research is complete; public capability implementation and independent acceptance are not claimed.

## Final exact-head Windows gate

[PR run45947258732](https://github.com/RXY712200/OrdWeave/actions/runs/45947258732) completed success at cf342dcf881bc9e3518ede042baeaed25f807da7. Actual MSVC x64 Debug12/12 (9.51s) and Release12/12 (7.43s); job JSON and CTest excerpts are archived. The final follow-up changes only research documentation; full local suites were not repeated. No pending job is claimed passed, no polling loop, and evidence-branch CI is not candidate CI.
