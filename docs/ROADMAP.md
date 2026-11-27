# Fixed six-phase roadmap

All six implementation phases below are completed and passed independent review. [RC.1](https://github.com/RXY712200/OrdWeave/releases/tag/v1.0.0-rc.1) is published as a GitHub Prerelease. The authorized first Stable release target is [v1.0.0](https://github.com/RXY712200/OrdWeave/releases/tag/v1.0.0); its Release records publication status. Post-release documentation maintenance does not add a seventh implementation phase or modify the RC tag. See [the technical handoff](HANDOFF.md), [development guide](DEVELOPMENT.zh-CN.md) and [actual debt register](TECHNICAL_DEBT.md).

1. **Project foundation and LayerKeySort integration.**

   Acceptance: Independent C17 static build, version query, exact pinned submodule, real ordering and snapshot/restore smoke test, passing available build/tests, Windows CI, documentation, and pushed source.

2. **In-memory database core, data model, ordering, indexing, and basic public C API.**

   Acceptance: Implement and verify in-memory semantics, ownership, IDs, ordering and indexing through the basic public API.

3. **Persistence engine, WAL, transaction commits, and crash recovery.**

   Acceptance: Verify durable commits and recovery of committed state after interrupted writes and crashes.

4. **LKS snapshots, database checkpoints, and checkpoint recovery.**

   Acceptance: Verify LKS4SNP1 ordering snapshots and consistent database checkpoint restoration with owned records and recovery.

5. **Revision history, retention, cleanup, backup, and verification.**

   Acceptance: Verify revision semantics, bounded checkpoint retention (up to three generations), safe cleanup, and restorable verified backups.

6. **Comprehensive testing, compatibility review, documentation, and stable release preparation.**

   Acceptance: Complete correctness and failure testing, Windows x64 compatibility review, accurate documentation, and stable release readiness review.

## v1.1.0 follow-up roadmap

The approved follow-up plan retains six stages/twenty steps. Accepted Preview.1 Steps1–4 integrates Full4.2/guard, Preview.2 Steps5–8 compressed ID core, Preview.3 Steps9–12 recovery construction, Preview.4 Steps13–15 capability convergence including approved index/range. Exact sources are in [HANDOFF](HANDOFF.md); historical decisions: [Preview.1](PREVIEW_1.md), [Preview.2](PREVIEW_2.md), [Preview.3](PREVIEW_3.md), [Preview.4](PREVIEW_4.md).

Current RC.1 Steps16–18 validates correctness/compatibility, matched cumulative performance and final v1.1.0 technical documents. Steps19–20 Stable preparation/publication are not authorized or automatic. Execution completion is not independent acceptance. Original six phases above remain historical completion, not renumbered follow-up steps.
