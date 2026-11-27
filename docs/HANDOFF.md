# OrdWeave — maintainer and AI handoff

This is the public technical entry map for maintainers. Read the local [AGENTS.md](../AGENTS.md) and [development guide](DEVELOPMENT.zh-CN.md), then the task-relevant documents below. All necessary project instructions are public; verify actual source and diffs for a new change.

## Identity and current state

OrdWeave is a Windows x64 C17 embedded static database built on the public APIs of LayerKeySort Full. Records own typed fields; Collections own ordered, independently identified Occurrences; multiple Occurrences may reference the same Record. It offers in-memory and durable single-operation databases, native checkpoints, immutable historical views, explicit anchored retention cleanup, full backups and persisted-data verification.

All **six** implementation phases completed independent review. [v1.0.0-rc.1](https://github.com/RXY712200/OrdWeave/releases/tag/v1.0.0-rc.1) is a published GitHub **Prerelease**. [v1.0.0](https://github.com/RXY712200/OrdWeave/releases/tag/v1.0.0) is published Stable at `32f853defc0a2e9f0ca809fb3535a90c977c7665`. Current branch is unreleased `1.1.0-preview.4` development; see [Preview.1 review](PREVIEW_1.md). The RC.1 annotated tag object is `165b1a72f2c689c78d271d4ab61651e6eefc217e`, peeling to implementation baseline `3f99029da73e3b4f0a75a538dd241cf94cda23d7`. Documentation stabilization advances main after that release; these new guides are not in the immutable RC tag. The preceding documentation package was committed at `96785fd6977754d7511a1954040115f91b4b4851` with [Windows CI 43815101144](https://github.com/RXY712200/OrdWeave/actions/runs/43815101144), 12/12 tests in each MSVC configuration. The subsequent documentation-governance commit is `dc23e791b3dc37f500eed73f5f8e719ea05edcf3`, verified by [Windows CI 43825091924](https://github.com/RXY712200/OrdWeave/actions/runs/43825091924). Both are historical documentation baselines; main may advance again. Inspect current remote HEAD rather than treating either as permanently latest.

```powershell
git ls-remote origin refs/heads/main refs/tags/v1.0.0-rc.1 'refs/tags/v1.0.0-rc.1^{}'
git status --short
git submodule status
gh release view v1.0.0-rc.1 --repo RXY712200/OrdWeave --json url,isPrerelease,isDraft,targetCommitish
gh run list --workflow windows.yml --branch main --limit 5 --json headSha,status,conclusion,url
```

## Read in this order

1. [AGENTS.md](../AGENTS.md) and [DEVELOPMENT.zh-CN.md](DEVELOPMENT.zh-CN.md): project boundaries, build/verification and engineering invariants.
2. [Chinese README](../README.zh-CN.md) / [English README](../README.md), then [USAGE.zh-CN.md](USAGE.zh-CN.md) for complete public examples.
3. [IMPLEMENTATION_WALKTHROUGH.zh-CN.md](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) and [IMPLEMENTATION_API_REFERENCE.zh-CN.md](IMPLEMENTATION_API_REFERENCE.zh-CN.md); [include/ordweave.h](../include/ordweave.h) defines all 44 function signatures.
4. [IMPLEMENTATION_INTERNALS.zh-CN.md](IMPLEMENTATION_INTERNALS.zh-CN.md), the exact formats below and [TECHNICAL_DEBT.md](TECHNICAL_DEBT.md).
5. [IMPLEMENTATION_VALIDATION.zh-CN.md](IMPLEMENTATION_VALIDATION.zh-CN.md), affected source/tests/diffs and actual exact-commit CI.

Exact persistent grammars: [PERSISTENCE.md](PERSISTENCE.md) (manifest/segment/frames), [CHECKPOINTS.md](CHECKPOINTS.md) (checkpoint/selection/native order), [HISTORY.md](HISTORY.md) (policy/root/backup). English [API](API.md), [architecture](ARCHITECTURE.md) and [release evidence](RELEASE_CANDIDATE.md) remain useful summaries.

## Authority and architecture

Current source, public header, CMake and independently observed behavior outrank old plans, phase prompts, handoff prose and chat recollections. A test's presence is not evidence that it ran; a successful historical run does not certify a new commit. Report disagreement rather than silently choosing the convenient account.

`persistence.c` exposes wrappers and sends mutations through pre-encoding, core application and durable confirmation. `ordweave.c` owns records/fields/collections/occurrences and three ID indexes; `lks_adapter.c` uses public LKS ManagedOrder/Order APIs. `wal_format.c` encodes/decodes commands and dispatches core replay. `wal_windows.c` owns paths, OS locks, append/flush/rotation/scans. `sha256.c` wraps BCrypt. There are **six** compiled production `.c` files. `checkpoint_core.inc` is included by ordweave.c; `checkpoint_windows.inc` and `history_windows.inc` by wal_windows.c, not separate object files.

Recovery structurally checks the authoritative retained chain, independently replays its semantics, restores a candidate native checkpoint plus tail, then compares logical results. Valid checksums alone are insufficient. History reconstruction audits retained later commits even for an earlier requested view. Cleanup publishes a mandatory baseline and matching roots before deleting an eligible prefix. Backup copies into a fresh directory marked incomplete, validates independently and publishes completion last.

## Permanent contracts and product boundaries

Preserve public signatures/status values, copied-result ownership, per-type nonreused successful IDs, one-Revision-per-state-changing-operation, no-op semantics, referential constraints, native Handle rebuilding and caller-serialized access. Permanent IDs are not runtime LKS Handles. `COMMIT_UNKNOWN` requires close/reopen investigation; never advise blind mutation retry. Checkpoint/root publication uncertainty may require `RECOVERY_REQUIRED` and quarantine.

Existing formats: manifest/segment/frame v3; checkpoint, selection, policy and recovery-root v1; native `LKS4SNP1`. Preserve byte layouts, canonical encodings, digest chains, high-water values, strict unknown-name/path handling, checkpoint ancestry and current/backup publication-before-deletion rules. A root-authorized deleted prefix is unavailable, not silently reconstructible. Windows x64 filesystem/flush contracts are prerequisites, not proof of power-loss immunity.

Intentionally unsupported: SQL, secondary field indexes, network service, multi-operation transactions, multi-writer/concurrent-handle guarantees, Linux/macOS, Last Days (no commit timestamps), incremental backup and a prebuilt cross-CRT ABI/install SDK. Do not turn every unsupported feature into technical debt or add it without a task.

## Dependencies and provenance

LayerKeySort Full v4.2.0 Stable is pinned under third_party/LayerKeySort at `f751e1ee2afd682874281d6dc495969f9eca49eb`; use its public APIs and do not edit the upstream checkout. Adapted Obscura64 source is fixed at `3db0122560853a8f3db3ba6888067394dc1a3e62`; SHA, write/lock/path and temporary-publication provenance is detailed in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). No ProbSpan implementation is included. MIT notices must be preserved.

The LayerKeySort documentation benchmark was read at documentation commit `13998b0682e56f34b401773f754e6d2c88c51206`; it is a presentation reference, not a dependency pin change.

## Tests, evidence and deferred work

The root CMake registers 12 CTests covering core, OOM/atomicity, WAL/process faults, independent format vectors, native checkpoints, history/roots/deletion/backup, semantic corruption, adversarial crash boundaries and an external C17 public-target consumer. Run Windows MSVC x64 Debug and Release through [windows.yml](../.github/workflows/windows.yml); see [validation guide](IMPLEMENTATION_VALIDATION.zh-CN.md) for exact commands and assertion map. RC baseline evidence: [run 43809986388](https://github.com/RXY712200/OrdWeave/actions/runs/43809986388), 12 tests in each matrix configuration. Recheck CI for the actual new HEAD.

Active debt: global Occurrence index rebuilds, repeated semantic replay, full checkpoint buffers, full-copy backup, per-call SHA provider setup, residual unpublished temporaries. Evidence gaps include physical power-loss validation and broader instrumentation. [TECHNICAL_DEBT.md](TECHNICAL_DEBT.md) separates these from resolved correctness issues (Phase 6 semantic WAL audit and safe backup filename/marker handling). Do not repay performance debt in a documentation task.

## Current maintenance and next-task status

The accepted Preview.1 work covered Steps 1–4: exact Full4.2 integration, isolated old/new measurements, compatibility checks, capability/architecture review and the confirmed early cleanup guard correction. See [Preview.1 decisions](PREVIEW_1.md) for the complete 64-function inventory and proposed later criteria. The existing six implementation phases remain complete; the separate fixed six-stage/twenty-step follow-up plan is not renumbered.

The development and evidence branches are based independently on published Stable `32f853defc0a2e9f0ca809fb3535a90c977c7665`. Evidence is kept on `evidence/v1.1.0-preview.1` under `handoff/v1.1.0-preview.1/`. The draft PR requires independent review; no Preview.2, merge, tag or Release is authorized by this handoff. Existing RC.1 and Stable publication remain unchanged. The original TD/EG records remain except for the explicit measurement addendum and resolved guard entry.

Current authorized work is Preview.4 Steps13–15: private capability prototypes, proposed contracts and version/documentation validation. See [Preview.2 decisions](PREVIEW_2.md). Earlier Preview.1 task prose is historical; no RC.1 or release is authorized.

Current Preview.3 decisions and limits: [persistence/recovery review](PREVIEW_3.md). Earlier task-scope prose is historical. No RC.1 is authorized.

Current Preview.4 is research-complete with no new public API or production data-path change. See [capability decisions and pending contract](PREVIEW_4.md). Earlier stage-scope paragraphs are historical. RC.1 requires a separate task.
