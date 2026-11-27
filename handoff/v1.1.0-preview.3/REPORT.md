# OrdWeave Preview.3 — complete Steps9–12 handoff

Implementation self-assessment, not independent acceptance. Entire authorized stage completed continuously; no Preview.4, merge, tag or Release.

## Exact references

| Item | Verified value |
|---|---|
|Accepted Preview.2 parent|`2bbf7640711e6561a5d5979f0de53a7f6ee0a1e8`|
|Final development HEAD|`edb9675b10125cecdf936e030bd9a54a90a84146`|
|Development branch|`dev/v1.1.0-preview.3`|
|Stacked draft|[PR #3](https://github.com/RXY712200/OrdWeave/pull/3), base `dev/v1.1.0-preview.2`|
|Evidence branch|`evidence/v1.1.0-preview.3`, independently based directly on accepted Preview.2|
|Main|Unchanged `32f853defc0a2e9f0ca809fb3535a90c977c7665`|
|Preview.1|Unchanged `3c5f371ca53c62912dc3d4d63aa11dc5277af020`|
|Preview.2|Unchanged accepted parent|
|Full4.2|Unchanged `f751e1ee2afd682874281d6dc495969f9eca49eb`|
|Version|Unreleased `1.1.0-preview.3`; numeric CMake1.1.0 unchanged|

Fixed task snapshot: `9e555728030cb2ba9c9e1f70d888a3534129c6f7/tasks/v1.1.0-preview.3.md`. Approved roadmap remains the six-stage/twenty-step plan at `ea1fdfca0ef54b758b4d3f9e52ecd229391c9cf9`. Only Steps9–12 were executed. Neither the task nor evidence branch is merged into development/production.

| Step | Self-assessment | Result |
|---|---|---|
|9 checkpoint|PASS|Unpublished direct Occurrence builder removes all temporary explicit residences; parser/graph/ref/minimum-Revision and native restore contracts retained; >20% large CPU improvement.|
|10 history/audit|PASS|Bounded operation-local identical-anchor reuse; full independent semantic audit retained; diagnostic scan/frame reduction and distinct-digest regression.|
|11 WAL/SHA/memory|PASS|Profiles and alternatives measured; SHA/WAL ownership/flush and full buffers deliberately retained, partial import allocation reduction measured. No forced debt closure.|
|12 backup/retention/integration|PASS|Safe full-copy/independent validation and conservative cleanup retained; local and exact-head MSVC suites, bilateral compatibility, byte equivalence and publication complete.|

## Changed files and selected architecture

Production changes: `src/checkpoint_core.inc` adds the unpublished builder and actual membership counts; `src/checkpoint_windows.inc` adds invocation-local successful anchor tuples; `src/ordweave.c` changes only the version literal. No new production TU/include/header: six C TUs, four owned includes, five private headers remain. Record/Collection ManagedOrders, compressed Occurrence radix and LKS explicit Collection engine remain unchanged.

Tests: `tests/failures.c` adds importer allocation-failure cleanup and impossible minimum-Revision assertions; `tests/checkpoint_oracle.py` adds two independently resealed ancestry cases; smoke/consumer only update expected version. Tooling: validator extends historical accepted Preview.2 proof and explicit current allowlist. Guidance: AGENTS, CHANGELOG, both READMEs, HANDOFF, TECHNICAL_DEBT and new PREVIEW_3 decision record. No other product, global configuration, dependency, workflow/build target or release record is changed.

The [source decision register](https://github.com/RXY712200/OrdWeave/blob/edb9675b10125cecdf936e030bd9a54a90a84146/docs/PREVIEW_3.md) is the durable detailed review; this report provides the actual measured evidence and commands.

### Import correctness and ownership

Only fresh unpublished cores enter this private importer. Records/fields and Collection control objects still use existing validated core construction. The Occurrence section now validates typed references, ascending IDs, declared membership and overflow, allocates each object and prepares its radix entry directly. No temporary explicit Order insertion occurs. A binary search in the sorted temporary Collection metadata associates each object with a separate actual-member counter; excess members fail immediately. Record refs increase once per Occurrence, including repeated Record references.

Prepared pages are published only into the unpublished graph; index preparation failures free the unowned object, and the existing whole-core teardown destroys every published partial object. Collection control objects retain empty Orders until native restore, so failures before/after any restore remain destroyable. Only public `lks_snapshot_restore_order` creates the final residences; native resolver uniqueness, same-Collection association, complete-seen checks, namespace and native/declared/actual counts remain mandatory. Handles are rebound only from restored Order traversal.

Each direct imported Occurrence still increments the unpublished construction Revision. This retains the existing minimum count of actual state-building mutations before installing historical Revision; validating high waters alone is insufficient. The new test sets a checksummed Revision4 with valid high-water sum3 but a state requiring5 mutations and requires CORRUPT. No-op/ID/Revision rules for normal WAL operations are untouched. Missing references/invalid graphs remain CORRUPT, NOMEM is never speculatively remapped. Header/digest, checked bounds, field budget/type rules, high waters, canonical bytes and256MiB checkpoint-file limit remain.

### Anchor reuse proof and replay map

Open/history still perform structural full retained-chain scans, independent semantic replay from genesis or mandatory root, candidate ancestry validation, native checkpoint+tail reconstruction, and logical equality including IDs/high waters/refs/fields/order. Older historical requests still audit latest before constructing the requested target. Incomplete-tail read-only/writable-open distinctions and authoritative root rules remain unchanged.

Both selection metadata files hold at most3 validated entries, so an invocation-local array of6 `(Revision,digest)` tuples suffices. Only successful `cp_anchor` checks enter it. Reuse is restricted to the same protected `checkpoint_load` invocation/store/root/chain and exact tuple; different digest or Revision still rereads/scans. No cache is retained across calls or based merely on session success. The last semantic and checkpoint-path equality checks remain. No public concurrency guarantee is added; external concurrent file tampering is not inferred safe from this local optimization.

New oracle cases use a different generation with the same Revision and either a correct or wrong digest. The wrong digest must return CORRUPT even though another tuple with that Revision already passed. The valid missing generation permits existing full-WAL fallback. Existing TD-101 checksummed-invalid-prefix tests remain enabled.

## Frozen protocol, data and environment

[Prospective protocol](PROTOCOL.md) was written before candidate results. Baseline is exact accepted Preview.2 with the same pinned Full4.2. Environment/compiler/binary/source hashes are in [metadata](metadata.json): Windows x64, GCC/MinGW16.2, Release/Ninja, same host; Python/CMake/Ninja versions are recorded. No prior-stage timings are pooled.

Four fixed, generated import fixtures:128/8192 one Collection with six integer fields;8192 with eight Collections;2048/eight Collections/64 integer fields. One warmup and three measured repetitions, rotated baseline/candidate order, QPC spans separate setup/export/import. Equality against the source core is asserted for every import, including refs/high waters/order. These benchmark fields are typed integers, not a claimed benchmark of every value type; existing functional/oracle suites cover all six types. A separate deletion-gap fixture retains4096 live Occurrences of high-water8192 across eight Collections. It uses a deterministic schedule rather than a random distribution, disclosed in the raw generator.

The import harness was measured before local anchor optimization, but its exact importer code matches the final commit and it does not call disk checkpoint loading. Maintenance runs and diagnostic candidate source include both production changes. No inference is made that every early benchmark binary is byte-identical to the final build; binary hashes and final source hashes are separately recorded.

| Import CPU median | Preview.2 ms | Preview.3 ms | Reduction |
|---|---:|---:|---:|
|128 /1 Collection /6 fields|0.0417|0.0360|13.7%|
|8192 /1 /6|1.5293|1.0573|30.9%|
|8192 /8 /6|1.3857|0.9797|29.3%|
|2048 /8 /64|0.4596|0.2779|39.5%|
|4096 live /8192 ID high water /8 /6|0.6164|0.4678|24.1%|

The selected large CPU goal >=20% is met. Short spans constrain precise ratios; these are generated inputs, not user telemetry or universal guarantees. Setup/export rows and all measured repetitions remain in [import raw](import-raw.csv) and [sparse raw](sparse-raw.csv).

Separate requested-allocation runs ([resource rows](import-resources.json)) include the live source graph, serialized input and restored core, exclude wrapper headers/OS internals, and are not RSS. At8192/one Collection, import allocation attempts25543→17096 (33.1% fewer); peak2773050→2423455 bytes (12.6% lower). At8192/eight Collections peak2355346→2249239 bytes (4.5% lower). Final graph storage is not reduced materially: transient residences are removed, while source/input/native snapshot/output buffers remain. The inherited allocation wrapper has small reporting-accounting residue; rows are not a zero-leak proof. No complete-checkpoint streaming claim is made.

### Maintenance timings and negative results

Baseline constructs one fixed genesis128, one genesis8192 and one retained-root fixture. Fresh per-run clones preserve fixture bytes/identity/state. One warmup plus three measured repetitions per baseline/candidate pair; separate open/checkpoint/earlier-history/verify/full-backup spans. Source setup and cloning are outside those spans; no claim is made about a paired durable-setup throughput gain. Genesis fixtures contain a native checkpoint plus one real tail mutation. Retained fixture uses the existing history test helper, physical prefix removal and guaranteed range35..40. Older historical targets remain within the guarantee.

| Genesis8192 median | Preview.2 ms | Preview.3 ms |
|---|---:|---:|
|Open|159.1543|131.1642|
|Checkpoint|19.0206|17.2065|
|Earlier history|239.3537|181.9774|
|Verify|449.7325|333.3210|
|Independent full backup|1108.5005|841.5415|

Complete small/root results remain in [maintenance raw](maintenance-raw.csv) and [summary](maintenance-summary.json). Negative findings are retained: small checkpoint median10.2296→10.6404ms, retained-root checkpoint11.1443→11.5245ms; these do not exceed the prospective IO investigation threshold (>25% AND >10ms). Small/root history gains are modest. No precise IO causality or universal acceleration claim follows from three repeats on one host.

### Separate replay and mutation diagnostics

Isolated source-instrumented builds emit one row per `replay_segment`, flags identifying structural/semantic/limited passes, successfully decoded frames, status and QPC time. The original replay function is wrapped without removing checks. Actual patches and logs are published; no instrumentation is in production.

For the complete diagnostic maintenance sequence, genesis8192 segment calls73→54, frames598235→442530, observed scan time2.169835→1.6014094s. Semantic segment calls remain15. Retained-root calls55→45, frames275→225; semantic calls remain15. These are diagnostic counts across the full sequence, not claims of a single-open pass count. The reduction is duplicate ancestry work; independent semantic passes are deliberately retained. [Summary](scan-summary.json) and the four scans logs preserve every row and operation timing.

An additional128-occurrence persisted creation diagnostic records133 actual FlushFileBuffers calls, about116.9ms baseline/109.7ms candidate total, and274 provider opens totaling0.6084ms/0.5694ms. Instrumented values include cold/observer effects and are not used as formal candidate timing. WAL operation encoding, hash chaining, flush and COMMIT_UNKNOWN/RECOVERY_REQUIRED boundaries are unchanged.

The separate10000-call SHA microexperiment has one warmup and three measured repeats. Existing full helper median4.6852ms; provider open/close alone3.4795ms; one locally reused provider with independent hash objects0.9145ms. Both hashing modes compare digest bytes on each iteration. This is a real isolated CPU opportunity. Retain production per-call ownership because measured persisted IO dominates and session/operation context propagation would add cleanup, error and independent-database/thread obligations without established whole-operation benefit. No global mutable provider, new dependency or undocumented Windows-minimum/pseudo-handle requirement is introduced. TD-005 stays Open with evidence, not falsely marked a nonissue.

## Backup/retention and remaining memory decisions

Full backup remains independent fresh-directory copying with incomplete marker, strict path/name/source validation, per-file flush, manifest-last, independent open/semantic equality/verify and completion-last. Duplicate-looking destination verification also checks the final history/root promises, so it is retained. No incremental/hardlink/shared-lifetime scheme is introduced. Conservative cleanup of only independently validated unreferenced generations remains; unknown/corrupt or potentially authoritative orphans and mandatory baselines are not guessed away. Root agreement still precedes deletion, and guarded busy/poisoned cleanup remains byte-identical.

Exporter canonical qsort on unique names, field temporary encoding, complete bounded buffer construction, native snapshot objects and whole-file load are retained. Direct reserved-space serialization/streaming and provider context redesign are rejected for this stage as broader failure/ownership changes than the measured import/anchor opportunities require. This is a deliberate retain decision with recorded remaining costs, not a claim of fully resolving memory/recovery/backup debt.

## Tests and exact-head CI

Actual principal commands:

```powershell
python -X utf8 build/preview3/benchmark.py
python -X utf8 build/preview3/maintenance.py
python -X utf8 build/preview3/diagnostics.py
gcc -O3 -std=c17 -Iinclude -Isrc build/preview3/sha_bench.c build/preview3/base/build/libordweave.a build/preview3/base/build/third_party/LayerKeySort/liblayerkeysort.a -lbcrypt -o build/preview3/sha_bench.exe
build/preview3/sha_bench.exe
cmake -S . -B build/preview3-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/preview3-release --parallel 4
ctest --test-dir build/preview3-release --output-on-failure
cmake -S . -B build/preview3-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/preview3-debug --parallel 4
ctest --test-dir build/preview3-debug --output-on-failure
python tools/validate_docs.py --examples --generator Ninja --config Release
python tools/validate_docs.py --examples --generator Ninja --config Debug
git diff --check
git diff --cached --check
```

Local Release12/12 (11.98s), Debug12/12 (11.97s); documentation examples each3/3; all existing assertions remain enabled. Checkpoint oracle now27 corruption/fallback cases. Import white-box allocation injection covers every reached own preparation site and safely destroys partial cores. A newly resealed invalid minimum-Revision state is rejected independently of high-water validation. Existing native wrong/missing/duplicate associations/counts, fallback/root, process/publication faults, checksummed-invalid prefix and older-target/latest audit tests remain. EG-003 did not recur, stays Open, original log and diagnostic patch unchanged. First unexpected importer status would log ordinal/status and preserve checkpoint bytes before cleanup. No further standalone EG campaign, full upstream matrix, broad sanitizer/platform run or hardware power-loss claim.

Representative bilateral Stable↔Preview.3 and Preview.2↔Preview.3 each pass four transitions including returning rewritten checkpoint/backups to the originating build. Existing helper asserts data, six permanent ordered member IDs/associations, Revision40, retained historical views, native checkpoint, independent backup/open. [Stable results](compatibility.json), [Preview.2 results](compatibility-preview2.json). A fixed-state deterministic export matches exactly across all three builds: SHA256 `655f001cf7f3b790c2df7775f2cd04c19be0bce67eb15a48ab246ef42f836958` ([bytes evidence](byte-equivalence.json)); binaries are not uploaded.

Documentation validator preserves RC tag/Stable full-file proof and exact accepted Preview.1/2 history via Git objects, then permits only explicit Preview.3 source/test paths. Current ordinary core/smoke/consumer changes are exact version-only transforms; history guard, radix, pin/workflow, format tables,44 APIs, inventory, UTF8/links/examples remain protected. It passes24 documents/494 links, six TUs/four includes/five private headers. A temporary validator edit initially applied the parent bound to the wrong comparison; corrected before commit, with no gate removed. Final published code uses separate exact two-commit comparisons. Full functional suites were not repeated after prose-only updates.

[Exact-head Windows run45944409281](https://github.com/RXY712200/OrdWeave/actions/runs/45944409281) is completed/success at `edb9675b10125cecdf936e030bd9a54a90a84146`. Actual MSVC x64 Release and Debug each12/12, CTest10.96s and9.20s respectively. [Job JSON](ci.json) and [actual log excerpts](ci-test-summary.log) are archived. This is the exact-head push workflow; the separate PR workflow was also triggered but is not substituted for an uninspected result. Evidence-branch runs are not candidate validation. No polling loop or workflow-trigger change.

## Technical Debt Impact and final limitations

TD-002 partially mitigated by local anchor deduplication, remains Open: independent semantic replay/multiple states still cost CPU/memory. TD-003 partially mitigated by fewer import allocations/transient residences, remains Open: complete buffers/native snapshots and field temporary encoding remain. TD-004 Deferred, TD-005 Open with new microprofile evidence, TD-006 Open. No new debt is claimed fully resolved; therefore no new resolved-debt introducing commit is invented. Both implemented mitigations are introduced by exact candidate `edb9675b10125cecdf936e030bd9a54a90a84146`. TD-001's original resolved implementation commit `d641e3c8048bff12df36a8ba7e1439868bf8722f` is now recorded explicitly; TD-103 and resolved TD-101/102 history remain. EG-001/002/003 remain with their limits.

Remaining limitations: short CPU spans and one-host generated fixtures; representative rather than exhaustive bilateral compatibility; diagnostic overhead and requested allocation rather than RSS; retained full buffers, per-call provider ownership and conservative residue; unresolved EG-003; no hardware power-loss certification. Full detailed-guide reconstruction remains the RC-stage responsibility. No independent technical acceptance is declared.

Final delivery verifies development/evidence remote SHAs, exact parent/pin, prior branches/main, historical tags and Release identities/body hashes. Unknown initial local work was absent; development/evidence trees are clean after commits. Project-local ignored build directories retain complete originals, diagnostic executables and fixture databases; no binaries, credentials, private policy or excessive logs are published.
