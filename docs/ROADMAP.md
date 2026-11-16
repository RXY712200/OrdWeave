# Fixed six-phase roadmap

All six implementation phases below are completed and passed independent review. [RC.1](https://github.com/RXY712200/OrdWeave/releases/tag/v1.0.0-rc.1) is published as a GitHub Prerelease; Stable publication requires separate explicit authorization after RC stabilization. The new source-grounded documentation is a post-release update to main, not a seventh implementation phase or a change to the RC tag. See [the authoritative workflow](WORKFLOW.md), then [handoff](HANDOFF.md) and [debt register](TECHNICAL_DEBT.md). Workflow documentation is maintenance, not a new phase; candidate and Stable publication remain separately authorized.

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
