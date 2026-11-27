# Preview.4 Step15 implementation completion report

## Result and authorization

**Codex self-assessment: Step15 PASS. Independent acceptance remains pending.** This is completion of the explicitly approved four-function implementation continuation, not a merge, RC transition, independent review or release. All implementation work remains on the existing dev branch and PR #4. The task is [the immutable approved instruction](https://github.com/RXY712200/OrdWeave/blob/f2eab6a92569f2608ca5aedc387822ede06aaa68/tasks/v1.1.0-preview.4-step15.md).

- Implementation parent / verified initial remote: `cf342dcf881bc9e3518ede042baeaed25f807da7`.
- Final implementation HEAD: **`5ab01df34bcb62a9feb1c61a443a1f08c0a56a06`**, one ordinary commit, `feat: implement approved Step15 bounded field index and range queries`.
- Existing branch: `dev/v1.1.0-preview.4`; existing [PR #4](https://github.com/RXY712200/OrdWeave/pull/4) remains open, targeting `dev/v1.1.0-preview.3`.
- Evidence parent / verified initial remote: `152851ca62d903c0a89f974652839b9ce81234ef`, existing `evidence/v1.1.0-preview.4`. This report and raw artifacts are appended only under `handoff/v1.1.0-preview.4/step15-implementation/`. The evidence commit is the commit containing this report; its exact SHA is supplied in the final delivery to avoid a self-referential commit hash.
- Full4.2 pin remains `f751e1ee2afd682874281d6dc495969f9eca49eb`. Version remains unreleased `1.1.0-preview.4`.

Both remotes were checked before changes and again before the implementation push. Source work was initially clean; no unknown local work was overwritten. Ordinary commits/pushes only. No research campaign was repeated.

## Exact implementation and ownership

[Implementation commit](https://github.com/RXY712200/OrdWeave/commit/5ab01df34bcb62a9feb1c61a443a1f08c0a56a06) and [complete patch](implementation.patch.gz) provide the actual change. Production still has six C translation units, now owning five implementation includes and five private headers. No build target, CMake definition, dependency or CI workflow changed.

- `src/field_index.inc`, owned by `ordweave.c`, implements one per-database advisory ManagedOrder and shared value/ID comparison. Each row owns an immutable OwValue copy (including String/Binary bytes); the field name is copied once. Row Record pointers and LKS container items are borrowed. A private Record backlink permits relevant maintenance without scanning the cache; it costs 8 bytes per Record on this platform even while disabled.
- Enable prepares a complete new name/order/key set privately. Budget, allocation or upstream failure releases that unpublished set and preserves the old cache/backlinks. Only success clears the old cache and publishes new backlinks.
- Relevant successful field set/remove and Record deletion maintain immutable rows by removing the old key and inserting a new owned row. No in-place comparator-key mutation. Maintenance failures clear the whole index **and configuration** without changing the authoritative data mutation's successful status/Revision. No allocating inverse rollback. Following queries scan until an explicit enable. Failed/no-op/unrelated field operations preserve the proven-valid cache; ordinary disabled calls perform no cache construction or maintenance allocation.
- Record deletion clears its cache residence before freeing its payload. Database destruction clears cache before Records; core exchange clears both caches before swapping cores. Private import/replay/history cores start without configuration; index state is neither transferred nor serialized. Store commit uncertainty can follow successful core maintenance, but the existing session guard blocks all later live cache calls until close/validated reopen.
- Live wrappers in `persistence.c` preserve guard/output clearing patterns. `history_windows.inc` adds only an independent-core range wrapper; the original history/recovery body is protected unchanged by the validator. History has no live-cache sharing.
- Range uses public ManagedOrder locate/traverse when matching; otherwise scans, sorts Record pointers with public LKS Stable Sort, and returns copied permanent IDs. The lower sentinel uses ID0 inclusively or UINT64_MAX exclusively, then the strictly greater successor; it never increments a real ID or assumes the exact-match handle is a result. A real UINT64_MAX duplicate-bound case is tested.
- Result allocation failure returns NOMEM with NULL/0, preserving cache validity. Fallback temporary arrays and copied rows/keys are released on each error path. No runtime handle or borrowed payload escapes through public output.

## Public contract and all 48 signatures

[public-api.h](public-api.h) archives all **48 exact signatures**. Validator compares the historical44 against exact research HEAD plus exactly these four approved declarations, and removes those declarations/comments to compare every remaining historical header token (including types/status values). No existing function/type/status was removed or changed.

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

One live index covers present values of a single field across all six types. Missing is excluded; present Null is included. Positive max_records bounds present entries; positive max_key_bytes counts exactly the copied field name including NUL plus copied String/Binary payload lengths. Fixed rows/IDs/value metadata, upstream allocations and OS memory are outside the byte budget. Subtraction-before-addition checks prevent overflow. Enable/disable consume no logical Revision or ID and write no disk configuration. Disable is idempotent.

Range accepts optional bounds of the explicit requested type and validates both 0/1 flags even for absent bounds. Invalid names/types/nonfinite floats/mismatched or inverted bounds fail; equal bounds with an exclusive side succeed empty. Values sort ascending, ties by permanent ID: Null singleton, false before true, signed Int, finite Float with equal signed zeros, unsigned byte-span String/Binary then length, including NUL and empty spans. No coercion/locale ordering. Existing equality still returns increasing IDs and uses a matching cache only optionally. Copied ID arrays use ow_array_release and outlive disable/mutation/source lifetime. Callers still serialize access.

The current README pair, usage entry, header, API reference and PREVIEW_4 describe this exact implemented contract. Historical guides' explicit RC/Stable44 baselines remain historical. Detailed RC guide reconstruction remains a future task, not a completion claim here.

## Tests and actual evidence

Environment: Windows11 x64, AMD Ryzen9 9955HX, MSYS2 GCC16.2.0, C17, Ninja; exact tool versions and binary hashes in [provenance](provenance.json). The compact benchmark was completed before starting final full CTests, avoiding concurrency with the full suite.

|Execution|Observed result|Artifact|
|---|---|---|
|Full local Ninja Debug CTest|12/12, 12.00s|[log](ctest-debug.log)|
|Full local Ninja Release CTest|12/12, 11.93s|[log](ctest-release.log)|
|Isolated upstream diagnostic failures|4 enable ordinals,2 maintenance ordinals; existing2 insertion ordinals; zero upstream live blocks after teardown|[log](upstream-faults.log)|
|Documentation validator|48 exact APIs;6 C units,5 includes,5 private headers; UTF-8,501 links/anchors and historical/source/format protections|[final log](docs-final.log)|
|Public documentation examples, Ninja Debug/Release|3/3 each; first README example, quickstart, lifecycle|[Debug](docs-debug.log), [Release](docs-release.log)|
|External public-target C17 consumer|Included in both full suites, links/calls all four additions and validates persistent/history results|source in implementation commit|
|Diff checks|git diff --check and staged diff check succeeded|commands below|
|Bilateral Stable/candidate and Preview.3/candidate|all four directions plus reverse return reads pass; enabled candidate source backups|[results](compatibility.json)|
|Canonical checkpoint bytes|386 bytes identical for Stable, Preview.3, disabled candidate, enabled candidate|[results](compatibility.json), four export .bin artifacts|
|Exact implementation HEAD Windows MSVC Debug and Release CI|both12/12, successful dependency-pin checks, Debug10.63s/Release11.20s|[CI metadata](ci.json), [complete log](ci-full.log.gz)|

CI: [push run45950372644](https://github.com/RXY712200/OrdWeave/actions/runs/45950372644), exact HEAD `5ab01df34bcb62a9feb1c61a443a1f08c0a56a06`. Debug [job140344562457](https://github.com/RXY712200/OrdWeave/actions/runs/45950372644/job/140344562457) and Release [job140344562163](https://github.com/RXY712200/OrdWeave/actions/runs/45950372644/job/140344562163) are completed/success. The separate PR-event run's status is captured in [ci-pr.json](ci-pr.json). CI success is execution evidence, not independent acceptance.

Focused assertions in tests/range_cases.inc cover six types, missing/Null, signed extremes and zeros, embedded NUL/prefix/empty bytes, duplicate ties, both optional bounds and all flag combinations, invalid bounds/names/types/flags, empty results, exact entry/key budgets and one-over, SIZE_MAX limits, indexed/unindexed equality/range, type changes/remove/delete and independent DBs. tests/index_failures.inc exercises actual owned allocations, upstream OOM, injected ManagedOrder removal/insertion capacity, failed enable preserving old rows, successful replacement, no-op/unrelated/failed updates, post-mutation invalidation/no rebuild, byte-budget shrink/growth, result OOM, UINT64_MAX and exchange teardown. Existing history tests add immutable history/live isolation, copied-result lifetime, enabled-source backup/retention/checkpoint, busy and COMMIT_UNKNOWN quarantine guards. Old tests and fault assertions remain.

Compatibility helpers are reused/extended, not a new DB framework. Stable and Preview.3 source trees are freshly git-archived from their exact commits, and each exact gitlink is independently archived and built. Stable uses its original Full4.1 pin; Preview.3/candidate use Full4.2. Every candidate reopen is checked via diagnostic malloc counting around the public range API: scan path two mallocs versus enabled path one. Thus cache absence is observed, not inferred merely from metadata. Candidate prepares checkpoints/backups with a live enabled index; old readers then validate Revision40, six ordered permanent occurrences, fields, retained historical views and independent backups, and return to the originating implementation.

Canonical SHA-256: `655f001cf7f3b790c2df7775f2cd04c19be0bce67eb15a48ab246ef42f836958`. This is one representative fixed-identity fixture, not proof for all possible states. Existing format/oracle suites provide separate broader coverage. No physical power-loss/hardware durability certification or exhaustive OOM campaign was performed. EG-003 did not recur in these suites; its original checkpoint OOM evidence remains Open, not resolved by this task.

## Compact production performance and requested memory

[Protocol](PROTOCOL.md) was frozen before timing; [actual C API driver](production_bench.c), [driver script](benchmark.py), [raw60 measured rows](production-raw.csv), [component/total medians](production-summary.json) and [separate memory runs](production-resources.json) are included. One untallied warmup plus three measured repetitions per cell, five modes, four fixtures. Baseline is exact research HEAD cf342dcf, not an external index prototype; candidate is the implementation runtime. Deterministic integer model,512 queries; independent expected-result scans and result frees outside timed API spans in every variant. Initial fixture setup is recorded separately; totals below are median per-repetition build+update+read sums. Phased mixed=50 updates/50 deletes; interleaved=32 updates/32 deletes, one every8 queries. Mode ordering rotates each repetition. No Group/history-diff research or broad engine comparison repeated.

|Fixture|Old equality scan ms|Candidate equality disabled ms|Equality enabled ms|Range disabled ms|Range enabled ms|
|---|---:|---:|---:|---:|---:|
|8192,257 values,read-only|46.9603|47.1584|1.6973|73.4728|1.8123|
|8192,257 values,100 phased changes|48.0178|48.9073|1.7796|73.7159|1.8263|
|1024,17 values,64 interleaved changes|4.5935|4.3278|3.3089|26.5601|3.2529|
|128,1 value,read-only|0.5205|0.5340|0.4307|2.6160|0.4293|

Selective equality is27.67x faster than old scan including index construction in this fixture. Build1.4141ms, enabled reads0.2831ms versus disabled reads47.1584ms gives an estimated15.45-query amortization from component medians; not a directly measured break-even guarantee. Interleaved benefit is smaller. Small/nonselective cases can lose for one-shot use; range without index pays sort/allocation costs and can be slower than equality. Performance range bounds are equal inclusive; broad ranges/all-type performance is not characterized by this compact matrix, though correctness is tested independently. No universal speed or latency guarantee.

Disabled-mode total deltas versus old equality: +0.42%,+1.85%,-5.78%,+2.59%, none exceeding the frozen material-regression threshold (>10% and >1ms). Disabled mode adds no index allocation or construction. Setup allocation count is33,037 for baseline and candidate. Requested setup bytes rise1,300,268 ->1,365,813 (+65,545): chiefly8 bytes per Record backlink plus DB pointer, and small surrounding process effects. This real disabled memory cost is disclosed, not described as zero overhead.

Enabling8192 integer keys adds758,108 requested live bytes and16,559 allocations (to2,123,921 requested bytes). The integer field's variable-key budget is only4 bytes for `key` including NUL; row/ID/value metadata and public LKS allocations are outside it. String/Binary payload copies consume additional budget and memory; not represented by this integer memory benchmark. Wrapper accounting is requested bytes, not RSS or allocator overhead. After teardown the wrapper residual is117 bytes for both enabled/disabled candidate,116 baseline: enabled cache adds no residual. Those small process allocations are not claimed to be a whole-process leak certification. Isolated upstream fault teardown separately asserts zero upstream live blocks.

## Reproduction commands

Run from the implementation checkout, Windows x64 with GCC/Ninja/Python and recursive pinned dependency. Bench/compatibility scripts record the actual original project-local paths; to reuse the archived helpers, place these artifact files in build/step15, and place memory.c in build/preview2/memory.c (or change that helper path). Compatibility output destinations must be new; fixture create intentionally rejects existing databases. The historical extraction commands in the drivers use git archive and each exact gitlink, rather than trusting stale local builds. No executables or database fixtures are uploaded; the four small canonical export .bin files are included. The raw unified patch and complete CI log are losslessly gzip-compressed because their natural context/blank lines trigger Git trailing-whitespace checks; decompress to inspect their original contents. Source/staged source diff checks passed.

```powershell
cmake -S . -B build/step15-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/step15-release --parallel 4
ctest --test-dir build/step15-release --output-on-failure
cmake -S . -B build/step15-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/step15-debug --parallel 4
ctest --test-dir build/step15-debug --output-on-failure
cmake -S . -B build/step15-upstream -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_C_FLAGS=-DLKS_ENABLE_ALLOC_DIAGNOSTICS -DOW_UPSTREAM_ALLOC_TEST"
cmake --build build/step15-upstream --target ordweave_failures
build/step15-upstream/ordweave_failures.exe
python tools/validate_docs.py
python tools/validate_docs.py --examples --generator Ninja --config Release
python tools/validate_docs.py --examples --generator Ninja --config Debug
python build/step15/benchmark.py
python build/step15/compatibility.py
git diff --check
git diff --cached --check
gh run view 45950372644 --json headSha,status,conclusion,jobs,url
gh run view 45950372644 --log
```

No production allocator hooks enabled. MSVC execution is GitHub-hosted CI, not claimed as a local MSVC run. Examples validation checks the existing first README examples; all four new function symbols are instead compiled/linked/called through the external consumer and focused tests.

## Preservation, boundaries and remaining work

`remote-heads.txt`, `remote-tags.txt`, `provenance.json` record the unchanged refs and release identities. main stays `32f853defc0a2e9f0ca809fb3535a90c977c7665`; Preview.1 `3c5f371ca53c62912dc3d4d63aa11dc5277af020`; Preview.2 `2bbf7640711e6561a5d5979f0de53a7f6ee0a1e8`; accepted Preview.3 `edb9675b10125cecdf936e030bd9a54a90a84146`. No upstream writes or global configuration changes. Existing RC.1 tag object165b1a72f2c689c78d271d4ab61651e6eefc217e peels to3f99029da73e3b4f0a75a538dd241cf94cda23d7; Release446640088 remains published Prerelease. Stable tag object1a4d62b2f53ed0d4525f71dfd46de2413177dbf8 peels to32f853defc0a2e9f0ca809fb3535a90c977c7665; Release459362640 remains published Stable. Published timestamps/body SHA-256 were authenticated and match the preserved prior provenance.

Actual API code changed only for the authorized addition and its integration; public historical contracts, typed permanent IDs, no-op/Revision/ownership rules remain. No wire version/layout, canonical encoding, dependency pin, upstream code, CMake targets or workflow changes. Existing12 CTests remain; focused assertions were added without disabling old checks. Source worktree is clean after commit/push; ignored build artifacts remain local. Evidence worktree is clean after its append commit/push, confirmed in final delivery. Other old local worktrees were not reset or cleaned.

**Technical Debt Impact: No change to registered IDs, status or priority.** EG-003 remains Open. No claimed closure of prior index/resource/recovery debts merely because this feature exists. The real disabled backlink cost, enabled metadata cost, scan/sort fallback cost and bounded benchmark scope are documented. Full RC-stage documentation rebuilding, cumulative performance and release readiness/acceptance remain separate future authorization. Stop here for owner/ChatGPT independent review; no next-stage work, merge, tag, RC or Release was performed.
