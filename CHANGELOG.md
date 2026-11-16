# Changelog

## 1.0.0-rc.1 ¡ª unreleased

- Complete Windows x64 C17 static database: typed Records, Collections, independent repeated Occurrences, dynamic LKS ordering and immutable IDs.
- Durable single-operation WAL commits, restart recovery, native LKS checkpoints, read-only historical views, explicit anchored Last N cleanup, full verified backups and read-only integrity verification.
- Harden recovery with independent semantic replay: valid-checksummed but logically invalid WAL cannot be hidden by newer checkpoints. Compare checkpoint-based results against authoritative replay.
- Reject unexpected backup directory names before formatting paths; preserve incomplete backup markers when completion publication fails.
- Add semantic corruption fixtures, historical checkpoint-interval assertions, five real process-kill cleanup boundaries and an external public-target C17 consumer.
- Preserve all existing disk format versions and public status values. This is a release candidate, not a published Stable release.
