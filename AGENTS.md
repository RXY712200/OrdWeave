# OrdWeave coding-agent instructions

## Read before substantive work

Read [docs/WORKFLOW.md](docs/WORKFLOW.md) **in full**, then [docs/HANDOFF.md](docs/HANDOFF.md). Read [DEVELOPMENT](docs/DEVELOPMENT.zh-CN.md), the [public API reference](docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md), and task-relevant implementation/format/validation guides before editing their behavior. WORKFLOW is the complete project policy; this file is its execution summary, not a separate source of permissions. Current explicit user instructions govern authorized scope; report substantive conflicts rather than inventing permissions.

## Scope and authorization

- Check actual remote main, local HEAD, branch, working tree and submodule before work. Preserve unknown user/other-agent changes. Do not assume an old documentation SHA is latest main.
- Implement the complete authorized work unit, including necessary verification. Do not add features, expand architecture, enter another phase or modify another repository without authorization. Research/review/planning alone does not authorize implementation.
- Honor separate permissions for commit, push, PR, merge, tag and Release. Do not infer publishing permission from implementation or push permission. No force-push, historical record overwrite or published-tag movement without explicit authorization.
- Original six phases are complete. Normal repairs remain in the existing task/step; material replanning needs an explained bounded replacement and user approval. Stabilization is not a seventh phase.
- Preserve v1.0.0-rc.1 and its Prerelease. No automatic RC.2 or Stable. Documentation/comments/formatting and modest compatible fixes usually need no RC.2; material reopening of candidate assumptions requires a technical recommendation and user authorization. Stable always requires explicit publication authorization.
- Isolate concurrent work by branch/workspace when appropriate; do not integrate experimental or unrelated product changes silently.

## Source, compatibility and quality

- Inspect actual source, public header, diff, tests and exact-commit CI. Old plans, handoff prose and completion reports are not proof of current behavior.
- Preserve public signatures/status values, result ownership, permanent typed IDs, Revision/no-op rules, caller serialization, WAL v3 and checkpoint/selection/root/policy v1/native LKS4SNP1 contracts unless the task explicitly authorizes a reviewed change.
- Keep LayerKeySort pinned at 70d7f0ef5857b54381b863d292a213909d87459f; use public APIs and do not modify upstream. Runtime LKS Handles are not persistent OrdWeave IDs.
- Keep module responsibilities and names clear, ownership/lifetimes and error cleanup explicit. Avoid unnecessary coupling, duplication, speculative frameworks and cosmetic production rewrites. Comments explain subtle intent, invariants, state transitions and failure branches, not syntax. Judge long functions/files and included implementations on engineering evidence rather than arbitrary size.
- In documentation-only tasks, record and stop for independent review of a credible release-blocking correctness/memory-safety issue; do not bury it as performance debt or start unauthorized refactoring.

## Verification and records

- Select meaningful checks proportional to the change. Distinguish source inspection, existing tests, local execution, CI and independent reproduction. No invented results, coverage, performance or power-loss guarantees.
- For significant code changes run relevant regressions and required build/CTest/Windows configurations. For documentation-only edits check accuracy, links, Markdown, cross-document consistency and scope; do not create redundant functional tests.
- Existing offline documentation check: `python tools/validate_docs.py`. Existing build/test commands and assertion map: [IMPLEMENTATION_VALIDATION](docs/IMPLEMENTATION_VALIDATION.zh-CN.md). This checker does not prove prose or replace checking this file's links.
- Inspect [TECHNICAL_DEBT](docs/TECHNICAL_DEBT.md) for every substantive task. Update only actual new/changed/resolved/reclassified debt or evidence/priority changes. Keep stable TD/EG IDs, evidence and resolved history; unsupported features are not automatically debt.
- Synchronize only affected API, usage, architecture/internals, format, validation, release/changelog, debt, handoff and workflow documentation. Correct current facts without rewriting historical evidence.

## Communication and completion

- Routine user communication and ChatGPT audits use Simplified Chinese; complete Codex tasks and Codex completion/testing reports use English. Keep identifiers and paths exact. Be direct, avoid filler/repetition, disagree with concrete reasons and alternatives; clarify only material unknowns not already answered by authoritative context.
- Deliver a self-contained English report with actual starting/final SHAs, changed files, behavior, checks and outcomes, CI links/headSha, scope/compatibility/dependency/release effects, limitations and working-tree state.
- Every completion report must contain **Technical Debt Impact**, listing affected TD/EG IDs and changes or explicitly **No change.** Do not mechanically edit the registry merely to produce a report.
- Codex does not self-issue independent PASS. ChatGPT independently checks actual evidence and judges PASS/FAIL; the user retains final product, scope and publication authority. FAIL repair stays in the original step. Stop at the authorized completion point.
