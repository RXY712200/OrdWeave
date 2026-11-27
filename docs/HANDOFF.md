# OrdWeave v1.1.0 — technical handoff and release metadata

## Identity and exact provenance

Windows x64 C17 embedded static database;48 public functions, runtime `1.1.0`, CMake1.1.0. This is the v1.1.0 Stable release source. Publication is authorized after exact-main gates; actual published status is verified at the [Stable release](https://github.com/RXY712200/OrdWeave/releases/tag/v1.1.0), not inferred from this source document.

Accepted RC.1 `76484149fb51cac7d7691a6a9cacc568c4b388fa` and [immutable RC evidence](https://github.com/RXY712200/OrdWeave/blob/ac581c7324dc962daa2146e52c68039a09541792/handoff/v1.1.0-rc.1/REPORT.md) are the technical acceptance baseline. Five accepted PRs were integrated by normal merges with an identical RC tree before the narrow release delta. Final main SHA/tag/Release identities belong in external Stable evidence, avoiding circular self-SHA promises. Full4.2 pin remains `f751e1ee2afd682874281d6dc495969f9eca49eb`.

Historical releases unchanged: v1.0.0-rc.1 Prerelease target `3f99029da73e3b4f0a75a538dd241cf94cda23d7`, annotated object `165b1a72f2c689c78d271d4ab61651e6eefc217e`; v1.0.0 Stable target `32f853defc0a2e9f0ca809fb3535a90c977c7665`, object `1a4d62b2f53ed0d4525f71dfd46de2413177dbf8`. Historical release identity is not current technical implementation. Preview.1/2/3 accepted commits: `3c5f371ca53c62912dc3d4d63aa11dc5277af020`, `2bbf7640711e6561a5d5979f0de53a7f6ee0a1e8`, `edb9675b10125cecdf936e030bd9a54a90a84146`.

## Reading order and source architecture

[Usage](USAGE.zh-CN.md) → [48 API contracts](IMPLEMENTATION_API_REFERENCE.zh-CN.md) → [walkthrough](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) → [internals](IMPLEMENTATION_INTERNALS.zh-CN.md) → [validation](IMPLEMENTATION_VALIDATION.zh-CN.md). Maintainers also read [development](DEVELOPMENT.zh-CN.md), [debt](TECHNICAL_DEBT.md), [release evidence](RELEASE_CANDIDATE.md), exact [WAL](PERSISTENCE.md)/[checkpoint](CHECKPOINTS.md)/[history](HISTORY.md) grammars. Public documents are self-contained; no private access needed.

Six production C translation units, five owned includes, five private headers. ordweave.c owns business objects plus occurrence_index.inc (compressed ID radix), field_index.inc (owned advisory keys), checkpoint_core.inc (canonical graph/native export, direct unpublished import). persistence.c dispatches guards/mutate. lks_adapter.c wraps public LKS/statuses. wal_format.c encodes/decodes/applies commands. wal_windows.c owns Windows sessions/WAL and includes checkpoint_windows.inc (selection/publication/call-local anchor reuse) and history_windows.inc (independent audit/views, root/retention/backup). sha256.c owns BCrypt resources.

## Protected technical contracts

Preserve48 exact signatures and old types/status values, permanent typed monotonic IDs, one Revision per state-changing mutation, no-op/ref rules and caller ownership. Handles remain private and rebuilt after restore. Record/Collection use ManagedOrder; Occurrence uses compressed16-way radix plus public LKS explicit Collection ordering. Prepare/Order-success/commit preserves failure atomicity. Optional field cache owns immutable values, explicitly budgets present entries and variable key bytes, invalidates configuration on maintenance failure, never auto-rebuilds, never changes committed data status. Historical range owns an independent core; copied arrays outlive cache/view/source.

WAL/manifest/segment/frame v3; checkpoint/selection/root/policy v1; native LKS4SNP1. Direct import changes construction, not bytes. Retained WAL remains semantically audited; mandatory root baseline, root agreement before deletion, backup.incomplete then independent verification/completion remain. COMMIT_UNKNOWN means stop/close/validated reopen, not blind retry.

## Evidence and limits

Root registers12 CTests, including independent wire/graph/semantic oracles and public consumer. See validation guide for coverage/commands. RC budget: one local Windows Debug/Release matrix, docs/examples both configurations, final exact-head MSVC matrix and compact cumulative A/B/C campaign. Accepted Step15 bilateral Stable/Preview.3 compatibility,386-byte canonical hash, upstream failures and lifecycle evidence remain applicable because RC runtime delta is version-only, with a small additive historical facade test. Exact comparison and current execution results are external evidence, not inferred from old green CI.

Unsupported: SQL, network service, multiwriter/concurrent guarantee, automatic/persistent field cache, cross-collection transaction, Linux/macOS backend, Last Days, incremental backup. Checkpoint256MiB limit is not a total database-size guarantee. Repeated semantic scans, whole buffers/copies and BCrypt setup remain costs. EG-001 physical power-loss, EG-002 instrumentation scope, EG-003 checkpoint OOM observation remain open; no standalone campaign or unproven attribution. All debt IDs/history remain.

## Completed history and current work

Original six phases are complete: foundation; memory core; WAL/transactions/crash recovery; native checkpoints; history/retention/backup; final hardening. Separate fixed six-stage/twenty-step v1.1 plan accepted Preview.1 dependency/guard, Preview.2 compressed index, Preview.3 direct import/anchor reuse, Preview.4 research plus four-function Step15 implementation. PREVIEW_1 through PREVIEW_4 remain historical snapshots, not current authorization.

Steps16–18 RC convergence is accepted. Owner-authorized Steps19–20 integrate and publish Stable after final gates; post-publication independent review remains separate. No further development is authorized.

## Narrow Stable transition and evidence reuse

The authorized transition changes only the three runtime/smoke/consumer literals to1.1.0, necessary release metadata and the permitted equality-index prose clarification. Current technical documents and accepted RC matrices/examples/compatibility/canonical/OOM/performance evidence are reused. Run targeted version smoke/external consumer, documentation validator/diff check and final exact-main MSVC matrix before publication. No new campaign or technical rewrite is needed for identical semantics.

Keep all debt/EG evidence and historical records. After publication stop for owner/ChatGPT independent review; no automatic next cycle. Actual tag object, peeled main, Release status/latest/assets/body hash are recorded externally.
