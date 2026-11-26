# OrdWeave v1.0.0 Stable — authorized minimal release task

## 1. Objective and authorization

The owner has authorized the first OrdWeave Stable release, v1.0.0, now. Complete this single bounded release task continuously. The next product-development cycle, intended to evaluate/adopt LayerKeySort Full 4.2 for OrdWeave 1.1, is a separate future design task and is NOT authorized here.

Do not start another documentation redevelopment cycle, another RC, or the 1.1 implementation. Detailed technical guides already describe the accepted RC implementation; preserve them as source-bound technical explanations and defer the broad documentation rewrite to the future 1.1 cycle. Correct only actual release metadata and immediately misleading current-release statements as necessary.

The owner specifically wants direct Stable publication in this task rather than an additional RC or another preliminary documentation task. This is a narrow task-specific release authorization, not a new general release policy. The previous source/documentation baseline has received independent review; the final minimal release delta, tests, and publication will be independently inspected by ChatGPT after your completion report. Do not claim that you have performed independent acceptance of your own work or that the final SHA has already received independent review. If new substantive correctness, compatibility, or release-blocking problems arise, stop publication and report evidence instead of treating this authorization as permission for a larger repair.

## 2. Repository and fixed starting evidence

- Repository: https://github.com/RXY712200/OrdWeave
- Target branch: main
- Independently checked main: b12a007c50c2f9efcbee500a14d3cf09dcc74f10
- Previous exact-head Windows CI: https://github.com/RXY712200/OrdWeave/actions/runs/43859122821 (Debug/Release each 12/12)
- Existing RC: v1.0.0-rc.1, annotated tag object 165b1a72f2c689c78d271d4ab61651e6eefc217e, peeled implementation commit 3f99029da73e3b4f0a75a538dd241cf94cda23d7, published Prerelease ID 446640088.
- Existing dependency: LayerKeySort Full v4.1.0 Stable, gitlink 70d7f0ef5857b54381b863d292a213909d87459f.
- Target: new annotated v1.0.0 tag and GitHub Release, draft=false, prerelease=false, appropriate latest Stable selection, at the exact verified final main SHA.
- Release distribution: GitHub-generated source archives and documented recursive Git checkout; no new custom binary/SDK assets. Do not describe generated archives as containing submodule source.

This instruction file lives on a separate task-delivery branch. Do not merge that branch into main, copy the instruction into release files, or include its commit in the release target. Use the actual main workspace or an isolated worktree based on verified main. Read the project guidance and applicable already-authorized collaboration policy.

Before modifying anything, verify repository, local/remote main, working-tree ownership, RC tag and Release, dependency gitlink, existing version strings, all relevant branch/tag/release refs, and actual build/CI requirements. Verify that v1.0.0 does not already exist. Do not overwrite or reset unknown work. If the fixed source baseline has advanced, inspect the difference and proceed only if it is within the same already-reviewed release scope; otherwise report the mismatch and stop.

The currently published upstream LKS release was still v4.1.0 at the last independent check. Verify its actual state if relevant, but do not make the present OrdWeave release depend on whether a future 4.2 release has been published. The owner now expressly authorizes releasing the existing 4.1-based product first.

## 3. One minimal release delta

1. Make the public runtime version report 1.0.0 instead of 1.0.0-rc.1: the known existing implementation is ordweave_version() in src/ordweave.c. Preserve the function signature and every other production behavior. CMakeLists.txt already declares project version 1.0.0; leave it unchanged if correct. Search for version expectations in tests, examples, and build metadata; change only literal release-version expectations if actually required. Do not change functional assertions.
2. Inspect current-facing README.md, README.zh-CN.md, AGENTS.md if present, HANDOFF, DEVELOPMENT, ROADMAP, RELEASE_CANDIDATE and CHANGELOG for statements that would be false after publication. Make the smallest coherent release-status corrections, including the supported stable clone command and a concise dated changelog entry, where necessary. Use truthful prospective wording before publishing, accurate historical RC wording and source-baseline labels. Do not rewrite detailed API, algorithm, format, usage, internal architecture, validation or implementation guides. Preserve historical RC statements in clearly historical material. Do not claim publication occurred before it occurs. Use the new tag as the stable version reference instead of embedding a self-referential commit SHA. Do not invent a 1.1 development plan.
3. The current tools/validate_docs.py deliberately checks protected implementation paths against RC.1. The one authorized version literal change may conflict with that guard. Inspect the actual script and adapt it only if necessary: retain the exact historical RC tag and source checks, explicitly permit ONLY the precise RC.1 -> 1.0.0 version-return delta and any specifically necessary version assertion delta. Compare protected paths to the historical baseline and verify that no other behavior/source/fixture changes are allowed. Do not disable the guard, remove protected paths, accept arbitrary changes in ordweave.c, replace old baseline evidence with a misleading claim, or weaken existing 44-signature/format/link/dependency/example checks. Report the exact exception and independent diff evidence. If the existing checker already supports this exact transition, do not modify it.
4. Confirm actual API and disk contracts, source algorithm bodies (except version literal), dependency, tests, examples, CMake targets, CI, licenses, and technical debt entries are preserved. Do not add or resolve debts merely for a release. Preserve all previous changelog/release history.

Allowed changes are only the narrow version/status/changelog/validator adjustments described above. If a broader technical rewrite appears necessary, stop and explain the actual blocker. No source changes to LKS, no gitlink update, no 4.2 preview/Stable adoption, no new features, no performance optimization, no format migration, no global config or repository settings changes.

## 4. Verify, commit, push, publish

The user authorizes ordinary local edits and checks within the specified scope, one coherent release-preparation commit on main, normal push to main, creation and push of the NEW annotated v1.0.0 tag at the exact tested final commit, and publication of the corresponding non-prerelease GitHub Release. No PR/merge, branch protection bypass, force push, RC tag move/deletion/rename, or existing Release edit is authorized. If protected main requires a different delivery mechanism, report that real restriction rather than bypass it.

- Before commit: inspect complete git diff and git diff --check. Compare to both the accepted main and RC implementation baseline; demonstrate only permitted changes. Confirm the stable runtime version with an actual built consumer or equivalent test.
- Run python tools/validate_docs.py and documentation-example checks in appropriate Windows configurations. Run all existing 12 CTests in Windows x64 Debug and Release, using the project's actual supported toolchain, and report commands/results. Verify existing recovery, format, checkpoint, history and consumer assertions remain intact. Do not silently skip an applicable required check; explain a real blocker.
- Commit the narrow delta, push main normally, capture exact final remote SHA. Confirm exact-head Windows MSVC Debug and Release CI success (12/12 per configuration) from the actual jobs. Any required failed/cancelled/pending gate is not success. Repair ordinary in-scope metadata/check errors within this same task. Do not change production behavior to work around failed tests. No release from stale SHA or an unchecked new main.
- Once the above gates are satisfied, create and push ONLY the new annotated v1.0.0 tag explicitly targeting that verified full SHA. Verify remote tag and peeled SHA before creating the GitHub Release. Preserve v1.0.0-rc.1 and its existing published Prerelease byte-for-byte where observable.
- Publish v1.0.0 as Stable (not draft, not prerelease) with concise accurate release notes describing the accepted product, the 4.1.0 dependency, documentation and actual supported Windows x64 use, supported recovery/history/backup boundaries, known limitations, and recursive source retrieval. Do not claim physical power-loss certification, multi-operation transactions, multiwriter support, 4.2 adoption, new functionality, or universal performance advantages. No new assets beyond the automatic archives. Use normal release publication, not renaming or promoting the historical RC Release object.
- Query actual remote tag, peeled target, main SHA, new Release ID/URL/flags, published status and old RC identity. If there is a partial tag/Release failure, inspect the real external state and complete only missing authorized operations safely; do not delete or move tags or falsify rollback. Verify clean local state or describe pre-existing changes. If final current-facing wording requires a later corrective commit, do not move the published tag; report the issue for review instead. Do not start the next cycle.

## 5. Completion report and stop

Return one self-contained English report with:
- Starting/final local and remote SHAs, exact files/patch scope, version runtime output, and the release-target commit.
- Exact executed commands, environments, test and documentation checks, actual CI run URL and job results, remaining limitations.
- New annotated tag object and peeled commit; new published Release ID, URL, flags and asset inventory; unchanged RC.1 tag/Release evidence and preserved historical baselines.
- API, persistent formats, dependency pin, production behavior, licensing, test and detailed-guide preservation; explain any narrowly required validation/status changes.
- Technical Debt Impact: list any actual change or "No change.".
- Actual publication success or precise blocker/partial state; no self-issued independent PASS.
- Confirm that 1.1 design and LKS Full 4.2 adoption have NOT begun.

Stop after reporting. Independent post-publication review and the next cycle's design belong to ChatGPT and the owner.
