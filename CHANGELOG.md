# Changelog

## Documentation stabilization on main after RC.1 publication

- Add bilingual project gateways, comprehensive Chinese usage/API/execution/internals/validation/development guides, maintainer/AI handoff and a structured technical debt register.
- Add isolated public C17 examples and documentation validation; consolidate stale phase/publication wording while retaining exact disk specifications.
- No production code, public API, existing tests, dependency pins, disk formats, tags or published Release changes. These documents are not included in the immutable RC.1 tag.

## 1.0.0-rc.1 - 2026-11-16 (Prerelease)

- Complete Windows x64 C17 static database: typed Records, Collections, independent repeated Occurrences, dynamic LKS ordering and immutable IDs.
- Durable single-operation WAL commits, restart recovery, native LKS checkpoints, read-only historical views, explicit anchored Last N cleanup, full verified backups and read-only integrity verification.
- Harden recovery with independent semantic replay: valid-checksummed but logically invalid WAL cannot be hidden by newer checkpoints. Compare checkpoint-based results against authoritative replay.
- Reject unexpected backup directory names before formatting paths; preserve incomplete backup markers when completion publication fails.
- Add semantic corruption fixtures, historical checkpoint-interval assertions, five real process-kill cleanup boundaries and an external public-target C17 consumer.
- All six development phases passed independent review; the release-era plan scheduled comprehensive source-grounded documentation for subsequent RC stabilization before Stable approval (see the post-release entry above).
- Preserve all existing disk format versions and public status values. This is a release candidate, not a published Stable release.
