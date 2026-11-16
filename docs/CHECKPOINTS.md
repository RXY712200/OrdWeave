# Phase 4 checkpoints ¡ª format 1

Manifest, WAL segment and transaction frame formats remain exactly version 3. Checkpointless databases retain the full-WAL recovery path. No migration or WAL rewriting/deletion occurs. Checksums detect accidental damage; they do not authenticate adversarial rewriting.

## Files and limits

The existing root adds `checkpoints/`, immutable `checkpoints/00000000000000000001.cp` generations, `checkpoint.current`, and `checkpoint.current.bak`. Unpublished checkpoint files use the same generation with `.pending`. Metadata temporary names have a random 64-bit suffix and CREATE_NEW semantics; stale unpublished files do not block future operations and are never selected. Allocation of generation numbers scans all checkpoint final/pending filenames plus the current selection high water; it never uses filename recency as recovery authority. Overflow fails with CAPACITY. Suspect filenames and reparse directories are rejected. Known temporary/orphan files can remain after a process interruption; automated historical cleanup is outside this phase.

A complete checkpoint is bounded to 256 MiB, including its checksum. Every field/name and logical Record budget retains the existing core limits. Counts and lengths are bounded against remaining bytes before allocations. Full checkpoint buffers and native snapshot objects can require substantial temporary memory; OOM cleans unpublished reconstruction. The importer is private and only receives fresh unpublished databases.

## Checkpoint header ¡ª 144 bytes

All integers are unsigned big-endian, bytes are octets, and no C structures/pointers/handles are dumped.

| Offset | Width | Meaning |
|---|---|---|
| 0 | 8 | ASCII OWCHECK1 |
| 8 | 4 | Version 1 |
| 12 | 4 | Header size 144 |
| 16 | 8 | Complete file size, 176..268435456 |
| 24 | 16 | Database ID |
| 40 | 8 | Nonzero generation |
| 48 | 8 | Exact committed Revision |
| 56 | 8 | Record ID high-water mark |
| 64 | 8 | Collection ID high-water mark |
| 72 | 8 | Occurrence ID high-water mark |
| 80 | 32 | WAL frame digest at Revision; all zeros for Revision 0 |
| 112 | 8 | Live Record count |
| 120 | 8 | Live Collection count |
| 128 | 8 | Live Occurrence count |
| 136 | 8 | Reserved zero |

Following the header, in exact sequence:

1. Repeat Record count: Record ID (8), field count (8), then each field as field-encoding length (4) and that many bytes. Record IDs strictly increase. Field names strictly increase under strcmp byte ordering. Field encoding is the format-3 field-set payload after its leading Record ID: name length (4), name bytes, type (1), payload length (4), value bytes. It retains canonical finite binary64, positive zero, signed two's complement integers, UTF-8 and embedded-NUL string semantics. The shared operation codec validates it; duplicate names, trailing field bytes and budget violations fail.
2. Repeat Collection count: Collection ID (8), occurrence count (8), native blob length (8), native blob bytes. Collection IDs strictly increase. Empty Collections still have a native snapshot.
3. Repeat Occurrence count: Occurrence ID (8), owning Collection ID (8), Record ID (8). Occurrence IDs strictly increase. Every referenced object must exist.
4. Exactly 32 bytes: SHA-256 of every preceding checkpoint byte, including header and native blobs. No trailing bytes.

Live IDs are nonzero, do not exceed their respective high-water marks, and high-water marks cannot exceed Revision. Deleted IDs remain consumed. The reconstructed core verifies reference conflicts, typed values, indexes and associations through the same creation/field/occurrence operations used elsewhere, then assigns the recorded high-water counters and committed Revision.

## Native LKS4SNP1

Each Collection is captured with lks_order_snapshot_capture, sized with lks_snapshot_serialized_size and serialized by lks_snapshot_serialize. Namespace is exactly 40 bytes: Database ID (16), Collection ID (8 BE), checkpoint Revision (8 BE), generation (8 BE). Each association is exactly the permanent Occurrence ID (8 BE).

The native format is upstream's frozen grammar: magic LKS4SNP1 (8), flags zero (4), namespace length (4), row count (8), namespace, and for every row association length (8) and association bytes. OrdWeave does not implement this encoder or order engine; only the independent test oracle assembles expected bytes.

Import builds complete Records, Collections and Occurrences before invoking lks_snapshot_deserialize and lks_snapshot_restore_order. The resolver maps IDs to stable database-owned Occurrences, checks Collection ownership and rejects duplicate resolution. Namespace, row counts and association lengths must match. Restored orders replace unpublished construction orders; traversal installs new runtime handles. Every live Occurrence must be seen once. Failure destroys all unpublished objects/containers without dereferencing expired handles.

## Selection metadata ¡ª exactly 416 bytes

| Offset | Width | Meaning |
|---|---|---|
| 0 | 8 | ASCII OWSELECT |
| 8 | 4 | Version 1 |
| 12 | 4 | Length 416 |
| 16 | 16 | Database ID |
| 32 | 4 | Entry count, 1..retention |
| 36 | 4 | Retention, 1..3 |
| 40 | 8 | Reserved zero |
| 48 | 336 | Three fixed 112-byte entry slots |
| 384 | 32 | SHA-256 over bytes 0..383 |

Each used slot: generation (8), Revision (8), checkpoint digest (32), WAL anchor (32), reserved zeros (32). Entries are newest first with strictly descending generation and nonincreasing Revision; repeated checkpoints at the same Revision are allowed. Unused slots are entirely zero. Slot zero is the selected active generation, not merely the largest filename.

## Publication, authority and retention

The session must be healthy and serialized. Checkpointing marks it busy and captures its already committed Revision and WAL digest without changing either. Export state and snapshots, create a new temporary generation file, write, FlushFileBuffers, close, reopen, verify identical complete bytes and import its structure, then rename without replacing an existing generation using MOVEFILE_WRITE_THROUGH. A final generation not referenced by valid selection metadata is not authoritative.

Build a new selection containing the new validated generation and up to retention minus one independently revalidated old generations. Default retention is 2; allowed range is 1..3. The configuration setter affects subsequent checkpoints and is persisted in their selection metadata. It does not itself create a checkpoint or change database Revision. A database with no checkpoint reports generation/Revision zero.

Preserve the old selection in the backup through verified temporary-file replacement. Publish and verify new current metadata. Publish and verify the same new metadata as backup. Only then reclaim older generations no longer referenced. Thus current and backup never intentionally reference deleted files. Crashes can leave old backup/current disagreeing; deletion is deferred until both agree. Interrupted cleanup is safe; a later successful checkpoint sweeps independently validated older unreferenced files. Corrupt or unsupported orphan files are preserved, not guessed safe to delete. Checkpoint generation maintenance itself deletes no WAL; Phase 5 anchored history cleanup is a separate explicit operation. The cleanup helper is a narrow extension point for later history policies, not Phase 5 log compaction.

Verified temporary-file publication adapts Obscura64 replace_once at commit 3db0122560853a8f3db3ba6888067394dc1a3e62 (MIT attribution retained). Random CREATE_NEW metadata temp files, write/flush/reopen verification and write-through rename are used; backup ordering and generation selection are OrdWeave-specific.

Before selection publication, failures return their specific error and leave the previous committed recovery path intact. An immutable orphan can remain. If a selection rename succeeded but verification failed, or publication is otherwise uncertain, poison the session and return RECOVERY_REQUIRED. Close/reopen is required. Cleanup failure can return IO even though the new checkpoint is already authoritative; data Revision is unchanged. Query checkpoint_info or reopen to inspect the selection.

## Startup and fallback

Acquire the existing writer lock and validate the v3 manifest. Scan the entire authoritative WAL chain for header/frame integrity, sequencing, canonical payload syntax and digest continuity without applying historical mutations. No checkpoint permits missing/corrupt WAL history. Validate both metadata candidates and verify every retained metadata anchor against the complete WAL chain. Anchor conflicts or valid-checksummed contradictory same-generation identities fail CORRUPT. A higher-generation candidate is accepted only with verified WAL ancestry, never by majority or modification time. Same-generation retention-only differences prefer current if its active entry is identical.

Missing/corrupt metadata can use the other candidate. If neither is valid, replay the whole valid WAL. Unsupported metadata versions and hard I/O/resource errors fail rather than being hidden by fallback. Referenced checkpoint files with missing/corrupt bytes, wrong identity/anchor, or inconsistent object graphs can use another retained checkpoint; if none is usable, full-WAL reconstruction validates logical semantics. Unsupported checkpoint versions and hard I/O/OOM failures remain distinguishable and abort opening. Files are not overwritten because validation failed.

For a usable checkpoint, construct a fresh unpublished database, restore its native orders, then scan the WAL and apply only frames strictly after the checkpoint Revision. Check ID allocation and exact Revision during tail application. Publish only when the complete tail validates; no rollback to an older committed state is allowed. The existing conservative final-incomplete-frame reconciliation remains in force before append. A corrupt complete tail fails even with a valid checkpoint.

This accelerates object reconstruction, not WAL I/O: the implementation still scans all historical bytes and currently rescans per candidate anchor. That is deliberate performance debt while all WAL remains authoritative. SHA is integrity, not protection against an attacker rewriting the entire checkpoint and log consistently.

## Evidence and limitations

The original five tests remain. Checkpoint tests assert one native restoration and exactly one applied tail commit, all types and embedded NUL, empty/multiple Collections, repeated membership, ID high-water preservation, retention changes, allocation boundaries, metadata publication failures and interrupted cleanup. An independent Python oracle compares complete checkpoint/native bytes, supplies checksummed malformed graphs/blobs and validates safe full-WAL fallback versus checkpoint use. A fixed empty checkpoint vector is checked in separately. Test-only fault hooks never run in production.

Tests cover process-level fault assumptions, not hardware power loss. Durability still depends on Windows/filesystem/device flush and rename contracts. Partial named temporary files and invalid orphan generations may require later operator cleanup. The Phase 4 checkpoint API itself adds no SQL, networking or multi-operation transactions. Phase 5 history, anchored cleanup and backups are documented in HISTORY.md.

## Usage

```c
OwDatabase *db = NULL;
OwRecordId r; OwCollectionId c; OwOccurrenceId o;
OwStatus status = ow_database_create_persistent("D:/data/new-checkpoint-db", &db);
if (status != OW_OK) return status;
if ((status = ow_record_create(db, &r)) != OW_OK ||
    (status = ow_collection_create(db, &c)) != OW_OK ||
    (status = ow_occurrence_insert(db, c, r, OW_BACK, (OwOccurrenceId){0}, &o)) != OW_OK ||
    (status = ow_checkpoint_create(db)) != OW_OK) {
    ow_database_destroy(db); return status;
}
OwValue v = {.type = OW_INT, .as.integer = 42};
status = ow_field_set(db, r, "after-checkpoint", &v);
ow_database_close(db);
if (status != OW_OK) return status;
status = ow_database_open("D:/data/new-checkpoint-db", &db);
if (status != OW_OK) return status;
OwValue result;
status = ow_field_get(db, r, "after-checkpoint", &result);
if (status == OW_OK) { /* verify result.as.integer == 42 */ ow_value_release(&result); }
ow_database_close(db);
return status;
```

## Phase 5 retention baseline

A versioned recovery root can retain a mandatory native baseline after WAL-prefix deletion. It counts toward the maximum three internal generations. Current checkpoint selections are reconciled before publication, and generation deletion protects the root baseline. Once anchored, only verifiable checkpoints at or after the retained baseline may initialize state; historical selection also requires a checkpoint no newer than its requested Revision. Loss of the mandatory root baseline cannot fall back to deleted genesis history. See HISTORY.md for slot accounting, explicit cleanup and root authority.

## Phase 6 semantic validation hardening

Checksum/syntax validation alone is insufficient. Before exposing recovered or historical state, the implementation now independently replays every retained mutation from genesis or the validated root baseline, rejecting failed operations, no-op frames and ID/revision inconsistencies even before a later checkpoint. The native-checkpoint-plus-tail result is compared against allocation-free logical comparison with direct replay, including fields, high-water counters and order. Historical targets are likewise checked against direct target replay after validating the complete retained chain. This deliberately adds reconstruction work; no overall startup-speed improvement is promised. Disk versions and native LKS serialization are unchanged. See RELEASE_CANDIDATE.md for the reproduced regression and accepted performance cost.
