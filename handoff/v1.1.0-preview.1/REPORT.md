# OrdWeave Preview.1 — consolidated execution report

Scope: the entire authorized Steps 1–4 task at [immutable task snapshot](https://github.com/RXY712200/OrdWeave/blob/8a9cf913cbdc407310bd84fbec28a27125340be3/tasks/v1.1.0-preview.1.md). Implementation, local verification, development push, draft PR and separate evidence publication are delivered. This is an implementation self-assessment, not independent acceptance. The unresolved initial OOM-test anomaly is explicitly retained; clean later runs do not resolve its cause.

## Result and exact references

| Item | Value |
|---|---|
|Production baseline / initial local and remote main|`32f853defc0a2e9f0ca809fb3535a90c977c7665`|
|Final development HEAD|`26704b68f2cb61e6edb5746294e924aceb08d5ab`|
|Development branch|`dev/v1.1.0-preview.1`, one commit directly above the fixed baseline|
|Draft PR|[OrdWeave #1](https://github.com/RXY712200/OrdWeave/pull/1), OPEN, draft, base main|
|Evidence branch|`evidence/v1.1.0-preview.1`, independently based directly on the fixed baseline; its commit is identified by the immutable URL used to deliver this report|
|Final remote main|Unchanged fixed baseline|
|Old Full gitlink|`70d7f0ef5857b54381b863d292a213909d87459f`|
|New Full gitlink|`f751e1ee2afd682874281d6dc495969f9eca49eb`|
|Upstream annotated v4.2.0 tag object|`a7f249c54afd3ae131bb0cbe32e8d5eb45a1610b`, authenticated API independently resolves to the new gitlink|
|Upstream publication|[v4.2.0 Release](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.2.0), published 2026-11-27T01:39:14Z, draft=false, prerelease=false|

No named branches existed at initial checks; no unknown work was overwritten. No task-branch merge/cherry-pick, main update, force push, tag, Release or upstream source edit occurred. Existing root build targets and the 12-test registration are unchanged; CMake numeric version alone changes to 1.1.0 and runtime/test expectations to `1.1.0-preview.1`. This is development metadata, not a published release.

## Four internal checkpoints

| Step | Implementation self-assessment | Evidence / qualification |
|---|---|---|
|1 — baseline and targeted triage|PASS|Pinned baseline, research sources, prospective protocol, stable tag/Release and concrete pre-guard write reproduced.|
|2 — exact integration and isolated comparison|PASS|Exact Stable gitlink, 33 existing public call names reviewed, identical-runtime A/B, bilateral persisted compatibility and explicit validator/CI pin integration.|
|3 — capability and architecture review|PASS|64/64 unique public functions, all modules, all required TD/EG records and four TD-001 alternatives reviewed.|
|4 — consolidation and verification|FAIL for an unqualified clean validation result|Required implementation, reporting and publication completed; final GCC and MSVC matrices pass, but initial checkpoint OOM return classification remains unexplained (EG-003 Open). No independent acceptance claimed.|

The task is handed off as a complete bounded execution with a disclosed validation exception, not falsely declared uniformly passing. No confirmed additional critical integrity defect was established. The failure did not reproduce in focused old/new/candidate runs; production recovery/index redesign was not undertaken to guess at a cause.

## Source and research provenance

- `aa95b240b019c518a98bfb900ec374d24a537165`: reviewed archived protocol, generated trace manifest/hashes and native public-database driver. Input is generated application workload, not human telemetry. The driver verifies an independently encoded final order and records, plus historical initial order.
- `1e40955a783f91ec835170bea664bfa8e324b465`: reviewed staging/final-candidate scripts, strict binary and trace hashes, separate diagnostic builds and larger holdout methodology. Candidate experiments are not described as actual Stable4.2 executions.
- `ebfa5c62245f793e90fde590b577003839d3e0b1`: reviewed native evidence README, metadata, resumed-cell procedure, timing columns and attribution summary. Interrupted/resumed accounting and profile rows remain separate from production timing.
- The historical OrdWeave runtime `b12a007c50c2f9efcbee500a14d3cf09dcc74f10` differs from the fixed Stable runtime only in the release-version literal. Upstream exact candidate `fe00536e7bd7f4e5128432ea4a29e54158074903` has **16/16 identical production C files** to final Stable4.2; production-source manifest and CMake are unchanged, installed header differs only in release strings. This supports reuse of qualitative attribution, not pooling old timings with new ones. [Provenance](provenance.json) records equality and source hashes.
- Historical profiles identify global occurrence staging/publication as a major memory-operation cost and WAL/flush as dominant in persistent operations. The new uninstrumented run does not manufacture a new causal percentage for TD-001, IO or provider initialization.

## Isolated A/B protocol and actual results

[Protocol](PROTOCOL.md) was written before new results. A is an archive of exact v1.0.0 production source plus Full4.1; B is the same archive plus Full4.2. All six production C and three included implementation files match byte-for-byte; [hashes](provenance.json) prove that the guard/version changes were excluded. Both use the same small appended driver build target, GCC Release, Ninja and supported public APIs.

Machine: Windows x64, AMD Ryzen9 9955HX, 16 cores/32 logical processors, 16,304,992,256 bytes physical RAM; GCC/MinGW16.2.0. The platform/compiler strings are in [metadata](metadata.json). Build tools observed in this environment: CMake4.4.3, Ninja1.13.2, Python3.13.14. QPC measures wall time in separate setup, operations, checkpoint, reopen and history spans. Peak RSS/allocation bytes were not measured; no memory-saving claim is made.

Two original generated update-heavy traces, seed17, populations128/2048, 1000 operations each; source SHA256 verified against the archive. Memory and persistent modes each have one discarded warmup followed by three fresh measured processes, rotating A/B order. Total32 successful invocations, 24 measured rows, 32,000 successful replay receipts. [Raw CSV](raw.csv), [all compressed receipts](receipts.jsonl.gz), [summary](summary.json), [driver](driver.c), [trace128](traces/db-128-17-update-heavy.trace) and [trace2048](traces/db-2048-17-update-heavy.trace) are archived.

Median milliseconds (A / B):

| Scale / mode | Setup | Operations | Checkpoint | Reopen | History |
|---|---:|---:|---:|---:|---:|
|128 memory|0.4403 / 0.3333|2.3344 / 1.6003|N/A|N/A|N/A|
|2048 memory|118.3817 / 76.6665|33.6725 / 22.0335|N/A|N/A|N/A|
|128 persistent|248.1894 / 243.3727|737.6470 / 724.6910|8.5321 / 8.0657|26.5020 / 25.1941|30.9513 / 29.8827|
|2048 persistent|2026.3645 / 1877.5078|769.4658 / 768.2291|260.4646 / 185.8299|319.8739 / 224.8322|443.2871 / 310.8018|

Prospective investigation thresholds: memory operation median >15% slower **and** >1ms absolute; persistent spans >25% slower **and** >10ms absolute. No threshold was triggered. New memory operation medians improve about31.4%/34.6%; larger checkpoint/reopen/history also improve. Persistent operation latency is essentially flat at the larger scale (negative result for a broad write-throughput improvement claim). Three repetitions on one machine do not establish inferential significance or precise IO gains. Setup and full rebuild still scale substantially. Zero memory-mode disk metrics mean not run, not instantaneous persistence.

Reproduction aids: copy this handoff to a fresh disposable project-local build folder; `python prepare.py <OrdWeave-checkout>` creates A/B archives/builds and checks the two trace hashes; `python run_ab.py` executes the small matrix. `python run_compat.py <OrdWeave-checkout> <final-candidate-build>` executes compatibility fixtures after candidate build. The archive includes scripts with portable root/build parameters rather than the original local hardcoded directory. The run script's incidental import-time preparation was removed for reuse; measured C driver/runtime and protocol are unchanged. Do not run into an existing evidence directory expecting immutable outputs to be preserved; scripts use fresh sandbox/build names. Complete original local outputs remain under ignored `build/preview1-evidence/`.

## Compatibility and cleanup guard evidence

[Compatibility results](compatibility.json) record four successful transitions: old→new, old→new→old, new→old and new→old→new. Existing `ordweave_history fixture` constructs real retained roots and deleted WAL prefixes at Revision40. [The compact public helper](compat.c) checks current field counter29, six exact ascending permanent Occurrence IDs, their Record/Collection associations, Revision40, every guaranteed retained view's Revision, latest historical field value, independently verified database state, freshly written checkpoint, backup open and a second backup returning to the originating build. The helper does not replace corruption/fault suites. It is representative unchanged-format compatibility, not exhaustive file-state enumeration.

[Before-fix log](guard-before.log): after real 9→2 physical WAL deletion and retained range35..41, the busy call returns expected OW_BUSY, but the assertion that deleted `recovery.current.bak` stays absent fails. This confirms pre-guard disk publication, not merely speculative work. Source inspection locates reconstruction and root reconciliation before `ow_verify_database`'s guard. The only functional OrdWeave production correction is four lines: existing guard after output/argument initialization, before reconstruction. [After-fix log](guard-after.log) passes. The committed history regression additionally creates a real flushed-but-unacknowledged fault4 mutation, asserts OW_RECOVERY_REQUIRED with missing replica still absent and zero output, then checks close status, validated reopen and successful normal repair. No new synchronization, status or contract is introduced.

Test development corrections were confined to the fixture: a first poison fixture used checkpoint fault25 and an incorrect successful-close expectation; poisoned close correctly returns OW_RECOVERY_REQUIRED. That checkpoint-publication fixture also made subsequent cleanup unsuitable for this targeted assertion. It was replaced by existing commit fault4 and the correct close assertion. Failed fixture directories remain locally under ignored build storage; they are not represented as production regressions. Final test changes total30 added lines in the existing history CTest, with no removed existing assertions.

## LKS and architectural decisions

The complete grouped [64-function review at candidate HEAD](https://github.com/RXY712200/OrdWeave/blob/26704b68f2cb61e6edb5746294e924aceb08d5ab/docs/PREVIEW_1.md) is the single substantive decision register. [Machine-readable inventory](capability-inventory.json) independently lists all64 exactly once and current production locations; 33 names are currently called. The frozen upstream API verifier also passes64 functions.

The review covers explicit Order, ManagedOrder, stable sort, immutable Group/Batch, snapshots, cursors, strict V3 migration and snapshot key helpers. It distinguishes live Handles, contextual LS1 coordinates and permanent typed database IDs; no public general Path/Tree API is invented. Native public snapshot restore can bulk-build a new unpublished explicit Order; private builders are prohibited, and ManagedOrder has no public transaction/reserve/bulk facility. Group/Batch is not a database transaction or field index.

All requested module boundaries are reviewed: Record/fields, Collection, Occurrence, equality queries, adapter, allocation/ownership, checkpoint import/export, canonical codec/WAL, history/verification, backup and retention. Existing sound ownership/staging/serialization/semantic audit/root agreement/flush/backup completion remain. No mandatory use of unused LKS functions is introduced.

TD-001 alternatives compare full rebuild, in-place maintenance with allocating undo/upstream revision exhaustion, bounded-ID-range ManagedOrder partitions and a specialized ID index. All preserve LKS collection ordering, deterministic iteration and atomicity as requirements; none is implemented. Checkpoint research explicitly identifies temporary collection order and repeated occurrence staging before public native restore, retaining full association/count/seen/high-water validation. Later secondary index, range/ordered query, stable sorting, batch processing and historical differences identify concrete uses and unresolved contracts, not new features in this PR.

Priority: confirmed guard correction first; TD-001 research next; TD-002/checkpoint redundant staging/TD-003 next with independent auditing preserved; TD-005 only after specific profiling; TD-004 Deferred and TD-006 operational cleanup remain bounded. EG-001/002 limitations remain; EG-003 records the new anomaly. Proposed Preview.2 targets include >=2x larger insertion/removal or separately approved checkpoint-import improvement, bounded small-case regression, resource measurements and allocation/capacity/identity proofs. Those are future approval proposals, not achieved claims. The fixed six stages/twenty steps are preserved; only supplied Preview.1 Steps1–4 were executed. The immutable task supplies no complete remaining schedule, so none is fabricated or renumbered.

## Commands and verification outcomes

Actual local Windows x64 GCC/Ninja commands, run by the archived local orchestration scripts:

```powershell
python build/preview1-evidence/prepare.py
python build/preview1-evidence/run_ab.py
python build/preview1-evidence/run_compat.py
cmake -S . -B build/preview1-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc
cmake --build build/preview1-release --parallel 4
ctest --test-dir build/preview1-release --output-on-failure
cmake -S . -B build/preview1-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc
cmake --build build/preview1-debug --parallel 4
ctest --test-dir build/preview1-debug --output-on-failure
python tools/validate_docs.py --examples --generator Ninja --config Release
python tools/validate_docs.py --examples --generator Ninja --config Debug
python tools/validate_docs.py
python third_party/LayerKeySort/tools/verify_v4_api.py
git diff --check
git diff --cached --check
```

The first two scripts each prepared the same isolated archives because of an import-time side effect; both preparations preceded the measured processes and neither incorporated candidate fixes. No historical full matrix was rerun. Targeted guard builds used `cmake --build build/preview1-release --target ordweave_history --parallel 4` and direct `ordweave_history.exe`; targeted checkpoint triage used the existing `-R '^ordweave_checkpoint$'` CTest in A/B/candidate builds. Compatibility compiled `compat.c` with `gcc -std=c17 -O2`, public include, each build's production `libordweave.a`, `liblayerkeysort.a` and `-lbcrypt`.

| Gate | Actual result |
|---|---|
|Initial candidate Release suite|11/12; checkpoint OOM return anomaly, original failure preserved|
|Focused checkpoint A / B / actual candidate|Each1/1 passed|
|Final candidate Release suite|12/12, 11.83s, [log](final-Release-2.log)|
|Final candidate Debug suite|12/12, 11.79s, [log](final-Debug-2.log)|
|Public examples Release / Debug|Each3/3: quickstart, lifecycle and bilingual README; [Release log](final-Release-3.log), [Debug log](final-Debug-3.log)|
|Documentation validation|44/44 exact declarations, 6 production TUs, 3 owned includes, 5 private headers, 21 documents/479 links including anchors; UTF8, RC/Stable provenance, explicit current deltas, pin and unchanged format tables passed|
|Upstream frozen API verifier|64 functions passed|
|Diff checks|Passed, no whitespace errors|
|A/B / bilateral compatibility / focused guard|All final invocations/assertions passed|

The first failure is [recorded verbatim](initial-Release-ctest-failure.log): `oom boundary 126 status 8`, followed by `status == OW_NOMEM` failure. The [preserved fixture subsequently reopened](failed-fixture-reopen.log) at Revision21 with native restore count1 and tail0. Subsequent pass does not establish cause, absence of a defect or upstream blame. No test was disabled, relaxed to accept OW_CORRUPT or removed. No unverified hardware durability, sanitizer, Linux/macOS or all-allocator claim is made.

The validator retains immutable RC tag object/peeled baseline and checks exact RC→Stable full-file version transformations from Git objects. It separately restricts development protected-path changes, exact version-only core/smoke/consumer files, the exact early-guard production change, exact numeric CMake version delta, exact CI pin substitution and actual checked-out dependency. Format table, API, inventory, UTF8, local navigation/anchor and public example checks remain active. Root AGENTS remains included.

## Exact-head independent CI build/test evidence

[Windows PR run45933680850](https://github.com/RXY712200/OrdWeave/actions/runs/45933680850) has `headSha=26704b68f2cb61e6edb5746294e924aceb08d5ab`, event pull_request, completed/success. MSVC x64 Release and Debug jobs each passed12/12 CTests; observed test times9.33s and11.35s. [Job/status JSON](ci.json) and [actual CTest log excerpts](ci-test-summary.log) are archived. GitHub PR CI uses its ordinary PR checkout; the run head identity is the exact final draft head. Push run45933641674 was also triggered at that head but is not needed as a substitute for the inspected PR run. No polling loop or workflow-trigger change was used. Any evidence-branch CI belongs to the baseline-plus-evidence tree and is not candidate validation.

Green CI does not settle EG-003 or provide independent technical acceptance.

## Protected boundaries, Technical Debt Impact and remaining limits

All44 public signatures/status values and the public header remain unchanged. Existing WAL/manifest/segment/frame v3, checkpoint/selection/root/policy v1 and LKS4SNP1 specifications/layouts remain. Core algorithms, index/recovery design, target structure and upstream sources are unchanged except the authorized dependency upgrade, development version metadata and narrow cleanup guard. Existing fault/corruption suites remain enabled. No production OW_TESTING flags were added; the established separate test-storage library remains the only diagnostic build path.

Development files changed: `.github/workflows/windows.yml`, `AGENTS.md`, `CHANGELOG.md`, `CMakeLists.txt`, both READMEs, `docs/DEVELOPMENT.zh-CN.md`, `docs/HANDOFF.md`, `docs/ROADMAP.md`, `docs/TECHNICAL_DEBT.md`, new `docs/PREVIEW_1.md`, `src/history_windows.inc`, `src/ordweave.c`, `tests/consumer/main.c`, `tests/history.c`, `tests/smoke.c`, `third_party/LayerKeySort` gitlink and `tools/validate_docs.py`. No protected technical guide was substantially rewritten; historical technical/format content remains public. No private collaboration content or credentials are included.

**Technical Debt Impact:** TD-103 added Resolved for this development correction, not published/backported. TD-001/002/003/005 gain contextual evidence/priority discussion without status or architecture changes; TD-004 remains Deferred, TD-006 Open, TD-101/102 Resolved, EG-001 Deferred, EG-002 Open. EG-003 is newly Open for unexplained intermittent checkpoint OOM classification. No prior entry/history is deleted or renumbered.

RC.1 remains annotated tag object `165b1a72f2c689c78d271d4ab61651e6eefc217e` peeling to `3f99029da73e3b4f0a75a538dd241cf94cda23d7`, Release446640088 published Prerelease. Stable remains tag object `1a4d62b2f53ed0d4525f71dfd46de2413177dbf8` peeling to the fixed baseline, Release459362640 published Stable. Release body hashes and publication metadata are in provenance.json; no edit/publication call was made.

Remaining review needs: EG-003 cause, single-machine/three-repeat performance generalizability, unmeasured peak resource costs, representative rather than exhaustive bilateral compatibility, existing power-loss/instrumentation gaps and later design proof obligations. No later implementation is started. Independent review must assess actual source/diff, test assertions, first failure and new evidence; the owner controls further authorization.

Development working tree is clean after commit; the upstream checkout is clean at the exact new detached pin. Evidence worktree is independently committed on its own baseline. Ignored build folders contain local originals, binaries, benchmark databases and failed fixtures, intentionally not uploaded. No unknown initial changes were found or discarded. Final delivery verifies both remote branch SHAs and main preservation, and gives immutable report/data URLs.
