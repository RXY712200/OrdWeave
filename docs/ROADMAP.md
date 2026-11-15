# Fixed six-phase roadmap

Each phase is one complete implementation task. Bug fixes stay within their original phase. Phases 1¨C4 are implemented; Phases 5¨C6 remain planned. Phases 1¨C3 passed independent review; Phase 4 awaits independent review.

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
