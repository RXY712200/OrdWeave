# 1.0.0-rc.1 review package

This is the final implementation candidate for v1.0.0, not a tag, release or independent acceptance verdict. Final Stable publication requires explicit user authorization after independent review.

## Supported build and use

Windows x64, C17, CMake 3.21+, matching compiler/CRT settings, and the pinned LayerKeySort Full source are required. GCC/MinGW and Windows MSVC are exercised. Clone recursively as shown in README. A source consumer can set BUILD_TESTING=OFF, add_subdirectory(OrdWeave), and link OrdWeave::ordweave. The external consumer test does exactly this with only ordweave.h and tests both in-memory and persistent lifecycles. There is no prebuilt cross-toolchain binary ABI guarantee or installation package. Python 3 is needed only for tests.

Public types include opaque database/history pointers, uint64_t-wrapper IDs, enum statuses/types/placements, OwValue and OwHistoryRange. Existing function signatures, member layouts and status numeric values are unchanged. Compiler options affecting enum sizes, packing or CRT allocation must match. Release strings now report 1.0.0-rc.1; CMake project version is 1.0.0. Ordinary create remains disk-free; persistent calls durably acknowledge each state-changing mutation. Inputs are borrowed for the call and stored as copies; results require the matching OrdWeave release functions. The caller serializes access. See API.md for exact ownership, no-op, uncertainty and maintenance rules.

## Source call-chain audit

| Area | Owning call chain and review |
|---|---|
| IDs, values and limits | public wrappers -> ow_apply -> core mutations; zero/overflow checks, prepare-before-publication, typed validation, UTF-8, binary64 zero/nonfinite rules, bounded Record budget |
| Object/handle lifetime | core occurrence staging -> collection LksOrder mutation -> index publication; borrowed objects outlive all containers; destruction detaches containers before freeing items |
| Indexes | lks_adapter -> public ManagedOrder locate with three distinct outputs, immutable uint64 comparators, uniqueness before insertion |
| Durable mutation | mutate -> core apply -> frame SHA/append -> FlushFileBuffers -> output publication; failures quarantine rather than blindly roll back potentially durable data |
| Rotation and tail | flushed CREATE_NEW segment header -> nonreplacing rename; strict length/header/frame/chain verification; only recognized final short writes may be reconciled on writable open |
| Native checkpoints | export -> public capture/serialize; fresh import -> resolver -> restore -> traversal/handle rebuilding; graph/count/namespace/high-water checks and partial-import cleanup |
| Metadata authority | verified current/backup selection publication, ancestor digest checks, no mtime/majority selection, deletion after agreement |
| History and verification | strict syntax scan -> independent baseline-to-tip semantic replay -> native checkpoint/tail reconstruction -> allocation-free complete state comparison; historical targets also compared against direct target replay |
| Anchored cleanup | reconstruct baseline -> compare proposed suffix with current -> publish both roots -> independent revalidation -> delete only unreferenced prefix -> verify retained range |
| Backup | canonical destination checks -> flushed file copies under incomplete marker -> private independent open/state comparison/verification -> completion rename |
| Resource/error paths | checked lengths and arithmetic, owned buffer cleanup, safe outputs, OOM tests, read-only scans without repair; test-helper path formatting now explicitly checks truncation |

## Demonstrated corrections

1. `replay_segment` previously validated pre-checkpoint bytes/syntax but skipped operation semantics. Independently rebuilt frames deleting a nonexistent Record before a newer valid checkpoint were accepted on the starting source. The new regression failed with return code 0 instead of CORRUPT before the fix. `history_semantic_state` now applies every retained operation from the authoritative baseline through the existing core; checkpoints cannot hide ID gaps, failed operations or no-op frames. Checkpoint-based output must equal independent replay, including counters and native order. The private comparison walks existing core objects without checkpoint export, so it introduces no 256 MiB total-database limit or additional full-buffer allocation.
2. `history_copy_directory` previously formatted unchecked directory names into a fixed buffer and ignored swprintf truncation. A 190-character unexpected checkpoint name returned NOT_FOUND instead of CORRUPT in the regression. The copy path now validates canonical numeric names and expected extensions and checks formatting results before opening any destination path.
3. An inaccessible incomplete-backup marker now produces IO instead of being treated as an absent marker. Completion-rename failure is injected and leaves an unopenable incomplete destination.

No disk-format change is required. Strictly rejecting logically invalid logs is a corruption correction, not a migration. A baseline already accepted by an explicit recovery root remains the authoritative start after older history was physically deleted; this implementation cannot audit deleted bytes.

## Compatibility contracts

Intended 1.0.0 contracts retain manifest/segment/frame v3, checkpoint/selection v1, recovery-root v1, policy v1 and upstream native LKS4SNP1. Exact grammars are in PERSISTENCE.md, CHECKPOINTS.md and HISTORY.md. Existing independent format corpora/fixed vectors remain unchanged and are exercised alongside new semantic fixtures. Valid preexisting Phase 3/4-format fixture databases continue opening without automatic migration. Checksums detect accidental damage, not adversarial rewriting by an attacker with unrestricted file modification.

## Validation evidence

Local commands: cmake --build build; ctest --test-dir build --output-on-failure; cmake --build build/debug; ctest --test-dir build/debug --output-on-failure. The matrix includes the nine original CTests plus independent semantic/filename regressions, historical/crash adversarial tests, and a separately configured public-target consumer. GCC/MinGW is local; MSVC Debug/Release runs through the unchanged Windows workflow. CI links and exact final SHA are supplied in the completion report.

GCC 16.2.0 production-source analysis: compile each of ordweave.c, lks_adapter.c, persistence.c, wal_format.c, wal_windows.c and sha256.c with -std=c17 -Wall -Wextra -Wpedantic -fanalyzer, public include paths, and -c. The analyzer reports a reviewed realloc-path leak diagnostic in cp_write: its trace simultaneously assumes realloc success and a null return. On actual failure the original buffer remains owned by CpWriter and is freed on the export error path; on success q is transferred to w->p and subsequently returned/freed. This is not treated as proof of a leak or as a clean analyzer result. OOM regressions exercise export cleanup. No external static-analysis dependency was added.

## Evidence checklist (not an independent phase verdict)

- PASS: independently checksummed semantic-corruption regressions reject invalid prefix operations.
- PASS: all selected historical Revisions before/at/between/after checkpoints match explicit expected fields, IDs and order.
- PASS: oldest/latest retained states and rejection immediately before the boundary are exercised.
- PASS: real process termination before roots, between roots, before deletion, during partial deletion, and after deletion preserves confirmed commits and supports maintenance retry.
- PASS: later ordinary checkpoints preserve anchored recovery and total generation accounting.
- PASS: legacy/checkpointed/anchored backups reopen independently; completion failures remain incomplete.
- PASS: public C17 consumer configures and links the public library target, with no private LKS header.
- PASS: existing fixed format and compatibility fixture tests remain present.
- PASS: API status/layout and source license/provenance review completed without dependency changes.
- FAIL (authorization gate, not a code defect): independent reviewer acceptance and user Stable-publication approval have not occurred. No tag or GitHub Release is created.

Build/CTest/CI PASS entries refer only to observed runs recorded in the completion report, not a self-issued overall PASS.

## Nonblocking debt and limits

Synchronous maintenance, repeated full semantic history reconstruction, complete checkpoint buffers, rebuilt occurrence indexes and full-copy backups remain deliberate costs. Semantic auditing increases open/history CPU and peak memory; checkpoint restoration still uses native LKS orders but no whole-operation speedup is promised. Linux/macOS, Last Days, SQL, networking, multi-writer access, multi-operation transactions and hardware power-loss certification are unsupported. Filesystem/device durability contracts remain prerequisites. Test fault hooks are absent from production behavior. The inherited checkout action's Node deprecation annotation remains nonblocking; no unrelated action upgrade is made.

Recommendation is READY FOR INDEPENDENT REVIEW when the final local and Windows CI runs succeed. This document does not authorize Stable publication.
