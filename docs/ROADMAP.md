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

## Subsequent development

Published v1.0.0 remains the immutable production baseline. Current unreleased Preview.1 integrates Full4.2 and a narrow cleanup guard correction; [the review](PREVIEW_1.md) records decisions and proposed later criteria. It does not reopen the six completed phases above or renumber the separately fixed six-stage/twenty-step follow-up plan. No later step is implemented or authorized by these recommendations.
