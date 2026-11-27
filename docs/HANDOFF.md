# OrdWeave v1.1.0 — technical handoff and release metadata

## Identity and exact provenance

Windows x64 C17 embedded static database; caller-serialized, single-writer persistent session. Product technical baseline is v1.1.0 with48 public functions. **Actual current candidate is unreleased `1.1.0-rc.1`**, CMake numeric1.1.0. This does not claim a v1.1.0 Stable tag or Release exists. Only Steps16–18 convergence is authorized; no merge, tag, Release or Stable preparation/publication.

Accepted Preview.4 parent `5ab01df34bcb62a9feb1c61a443a1f08c0a56a06`, [production evidence](https://github.com/RXY712200/OrdWeave/tree/41d49ed610879de00e9464f29b9cf0935a69501f/handoff/v1.1.0-preview.4/step15-implementation), [CI45950372644](https://github.com/RXY712200/OrdWeave/actions/runs/45950372644). Current source branches from that parent at dev/v1.1.0-rc.1; final SHA/CI belong in external evidence to avoid circular self-SHA promises. Full4.2 pin remains `f751e1ee2afd682874281d6dc495969f9eca49eb`.

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

Current Steps16–18 cover correctness/compatibility, fresh matched performance and complete documentation. Codex completion is not independent acceptance. Owner/ChatGPT review the immutable report before further work; no authorized Stable task is fabricated.

## Anticipated narrow candidate-to-Stable transition (not executed)

Technical documentation is already v1.1.0-facing and should be reused if semantics remain unchanged. A separately authorized transition is expected to change only runtime/test literals in `src/ordweave.c`, `tests/smoke.c`, `tests/consumer/main.c`, from1.1.0-rc.1 to1.1.0. CMake is already1.1.0. Minimal release-status facts then update HANDOFF, RELEASE_CANDIDATE, CHANGELOG and AGENTS scope as needed. API/architecture/usage/formats/four detailed guides need no second rewrite.

Version-aware validator permits only those two strings and exact three-file substitution from accepted Preview.4; all other production/build content and historical test content stays protected. Check narrow diff, applicable version smoke/external-consumer tests and docs validator, required final exact-head CI and separately authorized tag/Release identity checks. Reuse accepted technical/compatibility/performance evidence for unchanged semantics. Actual technical/runtime changes warrant renewed affected-scope review; version text alone does not warrant another comprehensive campaign. This checklist grants no transition permission.

历史包装补充：RC coverage map发现history_collection_count、occurrence_get、occurrence_compare、query_equal没有直接测试调用；history.c仅追加已知Revision40的计数6、所属ID、首末比较和counter29等值断言。原测试内容完整保留，validator核对精确插入块；只重跑受影响history，不重复本地全矩阵。
