# RC.1 Steps16–18 completion report

Self-assessment: Step16 PASS; Step17 PASS; Step18 PASS. Independent acceptance pending. No merge, tag, Release, Stable preparation/publication or extra step.

Parent:5ab01df34bcb62a9feb1c61a443a1f08c0a56a06. Final source:76484149fb51cac7d7691a6a9cacc568c4b388fa, dev/v1.1.0-rc.1. Two normal commits, no history rewrite. Draft PR https://github.com/RXY712200/OrdWeave/pull/5 targets accepted Preview.4. Evidence branch starts at the same accepted parent; this report's containing commit is the final evidence identity delivered externally. Full4.2 pin f751e1ee2afd682874281d6dc495969f9eca49eb unchanged. Runtime is unreleased1.1.0-rc.1, numeric CMake1.1.0.

## Step16

Runtime changes only the version literal; smoke/consumer expectations match. All48 declarations/types/statuses unchanged. api-test-map.csv maps every function to actual direct test calls, not coverage percentages. Four historical facade functions had no direct calls: a14-line additive history block now asserts count6, permanent associations, first/last ordering and counter29 equality at Revision40, with copied output after view close. Original test content remains exact. No production defect was confirmed or speculative fix introduced.

One local Windows GCC16.2/Ninja full matrix: Debug12/12 (12.10s), Release12/12 (12.26s). After identifying the facade gap, only affected history was rebuilt/retested:1/1 each configuration. An initial edit helper rejected its nonunique insertion needle before modifying the test; unchanged history was inadvertently executed once before the corrected insertion. This was not counted as coverage of the new assertions. No repeated full matrix. Source comparison and full-file validator distinguish the version delta from additive test block.

Accepted Step15 bilateral Stable/Preview.3 round trips, enabled-source backup/reopen cache absence,386-byte canonical SHA655f001cf7f3b790c2df7775f2cd04c19be0bce67eb15a48ab246ef42f836958 and owned/upstream OOM evidence are reused, not falsely reported newly executed. reused-evidence.json contains immutable links and hashes. Runtime semantics/build/pin are identical; no redundant compatibility/OOM campaign. Existing six-type, bounds, ID ties, no-op, budget, failure invalidation, immutable history, root, semantic-corruption, retention and backup assertions remain strict. EG-003 did not recur in the bounded suites and remains Open.

## Step17

Protocol frozen before results. A=exact Stable32f853d + original Full4.1 70d7f0e; B=identical A source/settings with only Full4.2 pin; C=current runtime+Full4.2, optional cache off. A/B source hashes are asserted identical, archives isolated, no historical branch edits. Reused seed17 update-heavy1000-operation traces at128/2048 in memory and128 persistent, one warmup/three measured runs with rotated order. Final independent array/association and expected comparison checks execute in the driver; receipts are hashed. Trace hashes, toolchain, binary fingerprints, raw rows and summaries are included. Setup stays separate. Ratios below are fresh matched observations, not products of old ratios.

|Workload/metric|A ms|B ms|C ms|
|---|---:|---:|---:|
|128 memory setup|0.436|0.326|0.051|
|128 memory operations|2.472|1.640|0.084|
|2048 memory setup|116.745|74.297|0.350|
|2048 memory operations|33.329|21.612|0.098|
|128 persistent setup|327.120|231.554|238.632|
|128 persistent operations|758.239|773.057|762.717|
|128 persistent checkpoint|9.349|8.839|8.240|
|128 persistent reopen|26.970|25.375|18.116|
|128 persistent history|31.979|30.622|23.397|

The JSON reports A→B dependency, B→C database and A→C cumulative ratios separately. Aggregate C memory batches are near0.1ms; do not derive precise per-operation nanosecond promises. Persistent mutation is flush dominated and does not show the memory-core speedup. No unexpected regression crossed the frozen CPU10%+1ms or IO25%+10ms thresholds, so no extra campaign/repeat was warranted.

Separate actual C equality off/on index experiment:8192/257 read-only512 queries and1024/19 with64 genuinely interleaved updates/deletions. Stride17 coprime to both bucket counts; occupancy.json verifies all buckets nonempty and records histogram. Prior1024/17 collapses to one initial bucket and remains a historical low-selectivity observation, not uniform. Independent expected IDs and frees are outside query timing. Build/update/read medians and per-run total medians:

|N|mode1 disabled / mode2 enabled|build ms|update ms|read ms|total ms|
|---|---|---:|---:|---:|---:|
|8192|1|0.0000|0.0000|46.8948|46.8948|
|8192|2|1.4140|0.0000|0.2736|1.6945|
|1024|1|0.0000|0.0180|4.6046|4.6226|
|1024|2|0.1388|0.0277|0.2329|0.4004|

Unchanged resource evidence reused separately: disabled private Record backlink8bytes,8192 setup+65,545 requested bytes with same allocation count; index enable+758,108 requested bytes. Not RSS or total memory cap: key budget includes copied name+String/Binary bytes, excludes fixed row/LKS allocation. No universal acceleration guarantee; broader types/ranges were correctness-tested, not performance-characterized here. No blanket upstream qualification, fuzz/stress, huge persistent matrix or power-loss simulation.

## Step18 and document map

Both README languages, API, architecture, persistence, checkpoints, history, usage, development and all four detailed Chinese implementation guides now describe v1.1.0 current structure coherently. Old global ManagedOrder rebuild and temporary native-Order construction descriptions were replaced by compressed radix prepare/publication/direct import. Current owned-key/cache fallback, call-local successful anchor reuse,48 contracts, actual test boundaries and resource costs are integrated. Format tables remain exactly historical rows. Historical Preview records remain unchanged. HANDOFF/RELEASE_CANDIDATE/CHANGELOG/AGENTS isolate actual RC/version/authorization facts; historical release package retained. TECHNICAL_DEBT preserves IDs/statuses and labels obsolete TD-001 implementation historical rather than claiming current reconstruction. No debt closure forced.

Lifecycle example now compiles/calls all four new functions, verifies copied range output across mutation/view close and disable without Revision. Docs examples3/3 Debug and Release. Final validator:48/48 signatures,6TUs/5includes/5private headers,25 documents509 links/anchors, UTF-8 and historical/source/format checks. Version-aware rules allow only RC or Stable literals, exact three-file substitution, and exact additive historical test block; old files cannot silently drift. Source/staged diff checks pass.

Anticipated future transition, not executed: only src/ordweave.c, tests/smoke.c and tests/consumer/main.c literals1.1.0-rc.1→1.1.0; minimal HANDOFF/RELEASE_CANDIDATE/CHANGELOG/AGENTS release facts. CMake already1.1.0. Reuse final technical docs/evidence when semantics unchanged; check narrow delta, relevant version smoke/consumer, docs, required final CI and separately authorized release identities. No second full technical rewrite or automatic comprehensive rerun for version text.

## Final gate and commands

Final exact source CI https://github.com/RXY712200/OrdWeave/actions/runs/45953244433 completed success; head76484149fb51cac7d7691a6a9cacc568c4b388fa. MSVC Debug and Release both12/12 with pin check; see ci.json and ci-summary.log. One final-head matrix, no polling loop or manual reruns. Intermediate push naturally triggered workflow too; it is not substituted for final execution.

Commands: cmake -S . -B build/rc1-{debug,release} -G Ninja -DCMAKE_BUILD_TYPE={Debug,Release}; cmake --build corresponding directory --parallel4; ctest --test-dir corresponding directory --output-on-failure. Targeted: build --target ordweave_history, ctest -R ^ordweave_history$. Docs: python tools/validate_docs.py and --examples --generator Ninja --config Debug/Release. Campaign: python build/rc1/run_campaign.py, using archived drivers/traces and fresh fixture paths. Git diff --check and staged checks. gh run view45953244433 --json headSha,status,conclusion,jobs,url and --log. Script originally uses D:/OrdWeave/build paths; copy helpers/traces to build/rc1 and original archived traces to build/preview1-evidence/traces, or adjust paths. No binaries/database fixtures uploaded.

## Preservation and remaining limits

Main/prior previews/tags remain in final-refs.txt. Authenticated release identities and body hashes match prior provenance (release-identities.json). No upstream/global configuration changes. Source worktree clean after normal pushes; evidence clean after append publication, externally verified. Ignored build outputs remain local. Exact source diff is reviewable in PR5; no old evidence rewritten.

Technical Debt Impact: no new ID/status/priority change. Existing TD-001 resolution preserved, TD-002/003 partial improvements do not close open costs; TD-004/005/006 and resolved TD-101/102/103 retain records. EG-001 physical power-loss, EG-002 instrumentation and EG-003 checkpoint OOM remain. Stop for independent review. No independent acceptance or authorization for Steps19–20 is claimed.
