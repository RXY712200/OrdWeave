# OrdWeave Preview.2 — Steps5–8 execution report

Implementation self-assessment only; no independent acceptance. The entire authorized stage is delivered as a stacked draft PR, with a separate evidence branch. No transition to Preview.3.

## Exact identities and boundaries

- Accepted parent: `3c5f371ca53c62912dc3d4d63aa11dc5277af020`.
- Final development: `2bbf7640711e6561a5d5979f0de53a7f6ee0a1e8`, branch `dev/v1.1.0-preview.2`.
- [Draft PR #2](https://github.com/RXY712200/OrdWeave/pull/2), base `dev/v1.1.0-preview.1`, not main.
- Evidence branch `evidence/v1.1.0-preview.2` independently starts at the accepted parent. The immutable delivery URL identifies its final evidence commit; no candidate/task commits are merged into it.
- Full4.2 gitlink unchanged: `f751e1ee2afd682874281d6dc495969f9eca49eb`.
- Main remains `32f853defc0a2e9f0ca809fb3535a90c977c7665`; Preview.1 remains the accepted parent. Existing RC/Stable tag objects, Releases and upstream source are not edited. No merge, force push, tag, Release, platform or public feature added.
- Task: immutable `6d52526b50b3e6b10c4d5277d56eace5f5757a0b/tasks/v1.1.0-preview.2.md`; approved roadmap read at `ea1fdfca0ef54b758b4d3f9e52ecd229391c9cf9/planning/v1.1.0/MASTER_ROADMAP.zh-CN.md`. Fixed six stages/twenty steps retained.

| Step | Self-assessment | Evidence |
|---|---|---|
|5 research/selection|PASS|Prospective protocol, A/C/D comparable prototypes, B rejected with concrete atomicity/capacity reasoning; resource-driven D refinements retained.|
|6 Occurrence core|PASS|Compressed nibble-radix, bounded prepare/non-failing publication, all consumers adapted, targeted own/upstream failure assertions and full suites.|
|7 Record/Collection/Query|PASS|Retain decisions based on actual call paths/cost; no unjustified field/index/sort refactor.|
|8 integrated verification|PASS for local implementation gates; exact-head CI status recorded separately|Local Debug/Release, bilateral compatibility, exact checkpoint byte comparison, docs/examples, raw measurements and independent build gate snapshot. Owner/ChatGPT acceptance remains separate.|

## Actual change and correctness argument

The sole new private module is `src/occurrence_index.inc`, included by ordweave.c; production still has six C translation units, now four owned includes and five private headers. It implements a compressed nibble-radix permanent-ID lookup index, not a replacement order-maintenance engine. LKS remains the explicit Collection order engine. Record/Collection immutable-ID ManagedOrders remain unchanged.

Eight-byte IDs are traversed as up to16 four-bit digits. Each 144-byte page has16 child/item slots plus prefix/shift/occupancy. Only occupied leaf ranges and actual branching prefixes exist. Internal nodes have at least two children; parent shifts strictly exceed child shifts. Thus depth/stack is bounded16 and storage grows with occupied prefixes, never the numeric high-water ID. Branch/page/MAX-ID tests exercise root splitting and collapse. Sparse rows expose extra per-live-ID structural overhead rather than assuming density.

Insertion allocates the new Occurrence, then zero to two unpublished pages. An existing-page insertion needs no page. A separating branch may borrow an old subtree, but abort frees only its newly owned branch/leaf. No live slot or Handle changes before the public LKS Order insertion succeeds. After success, slot/link/count publication is allocation-free and non-failing, followed by ref, permanent ID and one database Revision publication. Failure releases the prepared structure/object, preserving old root, data, refs, membership, handles and counters.

Removal first checks exact membership and DB Revision capacity, then invokes public LKS removal. Success is followed by allocation-free slot removal, empty-leaf deletion and unary-parent collapse; failure leaves the index untouched. The private ID index has no mutation Revision to exhaust. Destruction also does not mutate an upstream index. Existing no-op move behavior and counter checks remain. No upstream internal revision field, reserve/transaction assumption or private bulk builder is used.

Consumers adapted coherently: anchor/get/move/remove/compare lookup; ascending checkpoint export/count; native resolver lookup/complete-seen validation; same-state comparison; teardown. Ordinary native checkpoint import still constructs its temporary explicit order and restores snapshots through the public API. This stage does not redesign that pipeline or remove independent semantic replay. Public header/status values, WAL v3, checkpoint/selection/root/policy v1 and LKS4SNP1 are unchanged; the guarded cleanup fix stays byte-identical.

## Selection, protocol and complete alternatives

[PROTOCOL.md](PROTOCOL.md) was written before results. A is accepted Preview.1's exact full-rebuild index. B in-place ManagedOrder was rejected without an unsafe prototype: a second-container failure can require allocating undo and consume upstream mutation capacity, and no public reserve/transaction facility proves failure-proof inverse operations. C stages only a 256-ID-range ManagedOrder plus necessary directory preparation. Its directory binary-searches ranges and grows geometrically: growth copies O(P) entries and sorted insertion/empty-partition removal can shift O(P); this is not advertised as constant-time global maintenance. C works and is much faster than A at larger populations, but retains local rebuilding/upstream handles and global directory movement.

D was refined within the research step after measured resource costs: uncompressed 256-way paths use over1.6MB for128 extremely sparse keys; compressed-byte pages reduce this but still waste space. Compressed-nibble pages bound new preparation at two small pages and avoid directory copies, giving the selected simple fixed-depth design. All three rounds' raw summaries/resources are retained in `uncompressed/`, `byte-compressed/` and the root prototype files. Thresholds/traces were not relaxed after viewing results. No four complete databases were built.

Final prototype matrix: same seed17 schedule, populations128/2048/8192, 1000 paired remove/new-ID inserts plus finds, one warmup and three measured repeats with rotated order. Membership and ascending traversal are asserted. At N8192 operations: A588.2032ms, C11.3013ms, selected D0.0175ms. D's short prototype spans are timer-sensitive, so precise microbenchmark speedup ratios are not claimed. Final public-database timings below provide the stage gate.

Separate resource processes wrap requested malloc/calloc/realloc/free bytes for own and upstream static-library calls; wrapper headers and OS allocations are excluded. Common item/schedule arrays are included equally. No RSS or all-process allocation claim. At N8192 operation peak: A985625, C644433, D374817 requested bytes. N128 extreme sparse peak: A38104, C122496, D48944 bytes. D's sparse increase versus A is a real tradeoff, about10.8KB in this bounded case, while dense peak decreases. Current directory/index allocation is independent of numeric ID high water. The reporting machinery leaves96/97 bytes in the wrapper's accounting, so these rows are not asserted to be a zero-leak proof. The separate upstream injection asserts zero actual upstream live blocks after destruction.

## Final paired public-database measurements

Same Windows x64 GCC16.2 Release/Ninja host as earlier stage; exact compiler string and trace SHA256 values are in [metadata](core-metadata.json). A is exact accepted Preview.1 archive with Full4.2; candidate runtime files match final d641e3c. Setup and operations are separate QPC spans. One warmup and three fresh measured repetitions per pair; order rotates. Total40 successful process invocations/30 measured rows, with1000 successful API receipts each. The public driver verifies independently encoded final order, permanent occurrence associations, count and typed Record value. Existing core randomized model/failure tests additionally verify mutation semantics. No unlike machine/dependency timings are pooled.

| Trace | Preview.1 operation median ms | Preview.2 operation median ms |
|---|---:|---:|
|N128 update-heavy, archived seed17|1.5911|0.0811|
|N2048 update-heavy, archived seed17|21.8684|0.0953|
|N8192 update-heavy, generated seed17|94.3459|0.1218|
|N128 insert/remove-heavy, generated seed17|4.2554|0.1056|
|N8192 insert/remove-heavy, generated seed17|305.3153|0.1362|

The prospective >=2x larger insertion/removal target is met, with no small median regression >10% AND >1ms. Candidate sub-millisecond spans limit ratio precision; results are descriptive for generated workloads, not real-user telemetry or service guarantees. Setup/raw rows remain in [core-raw.csv](core-raw.csv), summaries in [core-summary.json](core-summary.json), complete receipts compressed separately. Original archived N128/2048 traces are reused unchanged; larger/new scenario generation uses explicit Python Random(17) and independent sequence model, disclosed as a new generator rather than falsely claiming identical PCG streams. The old isolated dependency evidence is context only. No extra persistent performance campaign was needed; actual persistence behavior is checked below.

## Step7 defensible retain decisions

Record and Collection ManagedOrders already mutate one container and preserve deterministic ID traversal without TD-001's whole-index duplication. Field linked lists keep simple ownership and small-record allocation; no measured bottleneck warrants a new per-field structure. Equality query scans IDs twice to allocate one exact copied result and preserves typed equality, +/-0, names/UTF8/NUL, budgets and copied ownership. No secondary indexes or altered result ordering are introduced.

Canonical checkpoint field sorting is qsort by unique names. Comparator-equal stable ordering has no value when names are unique; replacing it with allocating LKS Stable Sort adds failure/ownership semantics without evidence of benefit. Retain qsort and exact byte output. Collection placement/move/compare remain public Full4.2 operations. No feature work is started to force unused LKS APIs into use.

## Actual validation commands and results

```powershell
python -X utf8 build/preview2/run_prototypes.py
python -X utf8 build/preview2/final_benchmark.py
cmake -S . -B build/preview2-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/preview2-release --parallel 4
ctest --test-dir build/preview2-release --output-on-failure
cmake -S . -B build/preview2-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/preview2-debug --parallel 4
ctest --test-dir build/preview2-debug --output-on-failure
cmake -S . -B build/preview2-upstream -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_C_FLAGS=-DLKS_ENABLE_ALLOC_DIAGNOSTICS -DOW_UPSTREAM_ALLOC_TEST"
cmake --build build/preview2-upstream --target ordweave_failures --parallel 4
build/preview2-upstream/ordweave_failures.exe
python tools/validate_docs.py --examples --generator Ninja --config Release
python tools/validate_docs.py --examples --generator Ninja --config Debug
python tools/validate_docs.py
git diff --check
git diff --cached --check
```

Local complete Release12/12 (11.61s), Debug12/12 (11.76s); examples each3/3. Full format/corruption/fault tests remain enabled. No EG-003 recurrence and no standalone EG-003 campaign. Initial targeted core/failure/checkpoint checks also passed; they preceded the final suite. An upstream-diagnostics-only test refinement subsequently extends the actual allocation sweep to a full64-member Order, covering both allocation failures, while leaving the default preprocessed test path unchanged. Production compile definitions and CMake target structure do not gain diagnostic flags.

`tests/failures.c` adds reverse insertion of sparse/MAX IDs, ascending traversal, exact lookup, duplicate/abort, branch collapse, every reached own preparation allocation at16/256/high-bit/MAX boundaries, stable refs/handles/root/counters on failure, allocation-free removal, and simulated public LKS capacity rejection. The isolated upstream diagnostics build uses existing test-only allocator controls without upstream edits; it sweeps two failing ordinals and asserts unchanged live state and zero upstream live blocks after teardown. No private upstream revision fields are accessed.

Representative bilateral v1.0/new and Preview.1/new each pass four transitions including round trips, using the existing compact fixture/helper: data, six ordered permanent IDs and associations, Revision40, every retained historical view's Revision, latest historical value, fresh checkpoint, independent backup/open and return to the originating build. [Stable results](compatibility.json), [Preview.1 results](compatibility-preview1.json). A deterministic fixed-identity core export produces identical bytes across Stable/Preview.1/Preview.2; all three SHA256 values equal `655f001cf7f3b790c2df7775f2cd04c19be0bce67eb15a48ab246ef42f836958` ([evidence](byte-equivalence.json)). This is representative compatibility, not exhaustive state enumeration.

Validator retains immutable RC tag and exact RC→Stable version-only proof, then checks accepted Preview.1's historical delta using its exact Git objects. Current changes are restricted to explicit reviewed Preview.2 core/include/failure/version paths; version-only smoke/consumer expectations, numeric CMake version, dependency/CI pin, public signatures, owned includes, format tables, UTF8/navigation/anchors and examples remain checked. Result:44 APIs, six C TUs, four includes, five headers,23 documents/489 links. No checks were disabled to accommodate the new core.

Initial tooling corrections were limited to prototype include-path setup and compatibility-script archive-directory selection; no failed measurement or correctness row was discarded. The preserved earlier radix resource results are negative design evidence, not current candidate results. No broad platform/sanitizer/full upstream matrix was run.

## Technical Debt Impact and handoff

TD-001 now records Resolved in this development implementation, with old Open history/rationale retained and independent acceptance pending. Global reconstruction is absent from ordinary mutations; sparse leaf overhead remains a disclosed tradeoff and reconsideration trigger. TD-103 remains resolved and unchanged; EG-003 remains Open for continued development as explicitly authorized. TD-002/003/004/005/006, resolved TD-101/102 and EG-001/002 history are preserved. Checkpoint pipeline/semantic audit/backup/flush debt is not silently claimed repaid.

Remaining risk: new private radix invariants, sparse per-live-item overhead, short-span timing precision, generated-workload generalizability, existing EG-003 cause and hardware/instrumentation limits. Existing source/test/CI evidence is not independent acceptance. Only the owner can authorize the next stage. No Preview.3, merge or release is authorized by this report.

Final remote SHAs, exact-head MSVC job status and immutable evidence URLs are verified at delivery. Development and evidence working trees are clean after their commits; ignored project-local build artifacts retain originals, executables and fixture databases, which are not uploaded. No unknown initial local changes were discarded.

## Runtime-commit MSVC gate

[PR run45938400269](https://github.com/RXY712200/OrdWeave/actions/runs/45938400269) completed success at head d641e3c8048bff12df36a8ba7e1439868bf8722f. Actual MSVC x64 Release and Debug each12/12, observed CTest times10.95s and10.72s. Job JSON and actual log excerpts are archived. Evidence-branch CI is not candidate CI. No polling loop or trigger change. Green CI does not resolve EG-003 or grant independent acceptance.

[Full source decision register](https://github.com/RXY712200/OrdWeave/blob/d641e3c8048bff12df36a8ba7e1439868bf8722f/docs/PREVIEW_2.md) and the published diff provide the implementation review surface.

## Final documentation-head verification

Final PR head is 2bbf7640711e6561a5d5979f0de53a7f6ee0a1e8. Its only delta from tested runtime d641e3c is two entry-guidance sentences in AGENTS.md and docs/HANDOFF.md. Documentation validation and diff checks passed after that correction; full local suites were not redundantly repeated. Final-head [PR CI45938745296](https://github.com/RXY712200/OrdWeave/actions/runs/45938745296) snapshot: in_progress/. Actual per-job states are in ci.json; runtime-commit successful CI is separately ci-runtime.json. Pending final-head jobs are not claimed successful. No polling loop.
