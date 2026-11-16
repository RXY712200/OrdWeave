# Retained history, recovery roots, backup and verification

RC.1 source baseline: `3f99029da73e3b4f0a75a538dd241cf94cda23d7`. This main-branch documentation update is newer than the published tag.

[中文使用](USAGE.zh-CN.md) · [完整API](IMPLEMENTATION_API_REFERENCE.zh-CN.md) · [执行导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) · [内部实现](IMPLEMENTATION_INTERNALS.zh-CN.md) · [验证地图](IMPLEMENTATION_VALIDATION.zh-CN.md) · [技术债](TECHNICAL_DEBT.md)

The current database includes an explicit anchored-recovery extension. Manifest/WAL versions remain 3; checkpoint/selection formats remain 1. Legacy databases with no recovery root still require segment 1 and the complete chain. Ordinary open never compacts or deletes historical WAL. SHA-256 checks identity/continuity/integrity; it is not cryptographic authentication against someone able to rewrite all files.

## Historical views and guaranteed range

`OwHistory` is opaque and has only query/release functions. It owns an independently reconstructed private core, never a public writable OwDatabase pointer. Existing Record/field/Collection/Occurrence/order/equality semantics apply. Returned values and arrays are immutable deep copies released through the existing release functions and survive view destruction. A view remains stable after live mutation or source close. No mutation API accepts it. Caller serialization of live database access is still mandatory.

History construction opens independent read handles, validates manifest and authoritative WAL, imports a compatible native checkpoint at or before the target, and applies only frames through that target. It never advances the live WAL file position, changes live counters, or truncates files. Revisions after the latest or before the recovery baseline return HISTORY_UNAVAILABLE. Revision 0 is queryable in unpruned genesis history. No-op mutations have no additional committed Revision. A damaged committed prefix cannot be bypassed by a later checkpoint.

`ow_history_range` reconstructs the oldest promised state and reports current Revision, earliest guaranteed Revision, policy, Last N, and whether a safe complete-segment prefix can be reclaimed. The guaranteed range is inclusive. All Revisions in it can be reconstructed through the same retained chain. After pruning, older Revisions are unavailable even if interrupted deletion has left obsolete prefix files behind; leftover files do not widen the guarantee.

Manual is the default. Manual and Forever do not advance the boundary or automatically delete history. Last N guarantees at least the latest N committed Revisions, including current: desired floor is `current >= N ? current - N + 1 : 0`. N must be positive; other policies require N=0. Policy changes persist but delete nothing. A Last N request requiring already-pruned Revisions is rejected with HISTORY_UNAVAILABLE; changing policy cannot resurrect deleted history. Manual/Forever preserve the range still available. Only explicit `ow_history_cleanup` reclaims files. The actual floor may be earlier because retention uses whole-segment boundaries. Last Days is deferred: WAL v3 has no persisted timestamps, and modification times are never treated as commit times.

Policy format `history.policy` is 96 bytes: magic OWPOL001 at 0 (8); version 1 at 8 (4); length 96 at 12 (4); Database ID at 16 (16); policy at 32 (4, Manual=0/Forever=1/LastN=2); zero at 36 (4); N at 40 (8); zeros at 48 (16); SHA-256 of bytes 0..63 at 64 (32). All integers in this document are unsigned big-endian. Unsupported versions, noncanonical policy/N combinations, reserved bytes and bad checksums fail.

## Recovery root format 1 — exactly 256 bytes

Files are `recovery.current` and `recovery.current.bak`. Root identity and referenced material are checked, never chosen by mtime or majority voting.

| Offset | Width | Meaning |
|---|---|---|
| 0 | 8 | ASCII OWROOT01 |
| 8 | 4 | Version 1 |
| 12 | 4 | Length 256 |
| 16 | 16 | Database ID |
| 32 | 8 | Nonzero monotone publication epoch |
| 40 | 8 | Earliest retained/baseline Revision R |
| 48 | 8 | Baseline checkpoint generation |
| 56 | 32 | Baseline checkpoint digest |
| 88 | 8 | First retained segment sequence S |
| 96 | 8 | Expected first Revision R+1 |
| 104 | 32 | Predecessor WAL digest at R |
| 136 | 4 | Policy at publication |
| 140 | 4 | Required zero |
| 144 | 8 | Last N at publication, otherwise zero |
| 152 | 8 | Confirmed current Revision C at publication |
| 160 | 32 | WAL digest at C |
| 192 | 32 | Reserved zeros |
| 224 | 32 | SHA-256 over bytes 0..223 |

Root N/policy supplies defaults when no separately published policy exists. A valid policy file takes precedence. R must not overflow R+1, C>=R, and all referenced IDs/sequences/generations are nonzero. No segment numbers or v3 bytes are rewritten.

Anchored open requires the named baseline checkpoint with matching identity, generation, Revision, checkpoint digest and WAL anchor. It validates its complete graph and native LKS blobs. Segment S must begin at R+1 and carry exactly the root predecessor digest. The retained suffix must be contiguous and reach at least C with exactly the persisted digest at C. Subsequent valid commits extend the chain normally. Required missing baseline/segments are RECOVERY_REQUIRED; corruption and unsupported formats remain distinct. Missing/corrupt root bytes are not silently treated as genesis. A valid root may be used when the other root is absent; verification reports the missing redundant entry, and explicit cleanup can synchronize it after reconstruction.

When two valid roots disagree, same-epoch disagreement is corruption. A newer epoch must have nondecreasing baseline and confirmed Revisions, and its actual baseline/suffix must independently validate. Open can use that newer valid path while old prefix files remain. Read-only verification reports root disagreement as RECOVERY_REQUIRED. Explicit cleanup first reconstructs and compares the chosen path with the live committed state, then synchronizes both roots before any deletion. It never deletes based only on which epoch is larger.

## Maintenance protocol and crash states

1. Validate the currently authoritative history and persisted current state, including checkpoint/backup metadata.
2. Find the latest segment whose first Revision is at most desired-floor+1. Its predecessor Revision becomes the actual conservative baseline. If no complete obsolete segment exists, return success without advancing history.
3. Reconstruct that exact baseline using existing replay/LKS restoration, export a native checkpoint, write/flush/reopen/import-validate it, and publish its immutable generation.
4. Propose the new root and independently reconstruct current state from that baseline plus the proposed suffix. Compare both live and reconstructed cores without allocating a complete checkpoint buffer, including every object, field, order, reference count, high-water counter and Revision.
5. Reconcile current/backup checkpoint selections to the baseline so no authoritative selection requires discarded ancestry. At this point the old WAL/root still recovers all confirmed commits.
6. Write/flush/verify new current and backup roots using unique CREATE_NEW temporary files and write-through replacement. Uncertain publication quarantines the writable session; close/reopen is required.
7. Reconstruct the new authoritative path again. Require byte-identical verified current/backup roots before deleting any prefix segment.
8. Delete only sequences below S, tolerating already absent prefix files on retry. Reclaim unreferenced checkpoint generations only after root/selection agreement.
9. Run persisted-data verification again and report the actual retained range.

Before root publication, a failure leaves the old authoritative chain intact; an unpublished checkpoint can remain. Between root publications, all old files remain and reopen can validate the newer path. After both roots agree, an interrupted deletion can leave excess files but cannot reduce recoverability. A later explicit cleanup finishes the prefix deletion without pretending those files extend guaranteed history. No ordinary open deletes history. Physical deletion failures return IO. Publication uncertainty returns RECOVERY_REQUIRED and blocks normal session access.

The baseline counts toward the maximum of three internal generations. Anchored sessions reserve one generation slot for it; regular checkpoint publication uses the remaining slots. Cleanup requires at least two configured slots, and new current-state checkpoints on an anchored session with retention 1 are refused with RETENTION_UNSUPPORTED. Selecting one slot never deletes the mandatory baseline. This conservative refusal avoids exceeding the generation limit. External backup copies are independent and do not consume source slots.

## Full backup

`ow_backup_create(db, destination)` requires a new safe Windows drive path and a consistent healthy source. It rejects existing destinations, lexical source descendants and descendants detected through final volume-GUID parent paths (including drive aliases). It never writes into the source. It copies actual WAL, checkpoints and relevant root/selection/policy/manifest bytes with bounded 64 KiB buffers, flushing each file. Unpublished `.pending` WAL/checkpoint files are not copied. All recovery material required for the source's advertised range is included, for legacy and anchored layouts.

A new `backup.incomplete` marker is created and flushed before copying. Ordinary open refuses such a destination with RECOVERY_REQUIRED. The implementation privately opens the fully copied destination for validation, compares reconstructed current state to the source, verifies its retained range, closes validation handles, then publishes completion by write-through renaming the marker to `backup.complete`. A crash before completion leaves an explicitly incomplete destination. A failed backup can leave that new directory for inspection; it never overwrites an existing destination or reports it as successful. Completion markers are publication gates, not replacements for file/root integrity validation.

The resulting directory is an ordinary independently writable database with the same Database ID, Revision and consumed IDs. It does not reference source handles or absolute source paths. No incremental/cloud backup or same-disk disaster-protection promise is made.

## Read-only verification

`ow_verify_database` rereads the actual manifest, root pair, WAL headers/frames, native checkpoints and selection pair; reconstructs current state and the advertised earliest state; and reports a verified inclusive range. It detects unsupported formats, integrity/graph errors, missing required materials, root/selection disagreement and IO/OOM failures. It does not truncate tails, repair metadata, delete files, or silently accept a damaged referenced checkpoint merely because full replay could work. An incomplete final tail returns RECOVERY_REQUIRED during verification, rather than being repaired. Normal writable startup retains its prior conservative tail-reconciliation behavior.

Errors appended to the existing enum are HISTORY_UNAVAILABLE and RETENTION_UNSUPPORTED; earlier numeric values remain unchanged. Missing required recovery material is RECOVERY_REQUIRED; missing redundant metadata may report NOT_FOUND. Verified success is OW_OK. COMMIT_UNKNOWN and quarantined-session semantics for ordinary mutation commits remain unchanged.

## Test evidence and accepted debt

All previous seven tests remain. New C tests exercise views, policies, root publication/deletion faults, post-crash maintenance restart, independently writable legacy/anchored backups, destination rejection, OOM boundaries and deterministic randomized states across history, cleanup and backup. Test output records actual segment removal (9 to 2, retained Revisions 35..41). A process is terminated between root publications and reopened for continued maintenance. A source directory is moved out of its original path before backup restore. Independent Python code validates the root grammar, fixed vector, physical absence of prefix files and 21 malformed/root/policy/material cases.

Performance debt includes repeated full suffix scans, complete historical reconstruction, synchronous copying/maintenance, full checkpoint buffers and rebuilt ManagedOrder indexes. Recovery has no background pruning, timestamp migration, SQL or alternate ordering engine. Last Days remains deferred. Failed-operation temporary/orphan files may remain; no promise of exhaustive unrelated orphan cleanup is made. Process faults do not establish physical power-loss safety; Windows/filesystem/device flush and rename contracts still apply.

## Phase 6 semantic validation hardening

Historical targets are compared with direct target replay after auditing the complete retained chain, including later operations. An explicit root baseline is the only authority replacing deleted genesis history. See [the architecture audit](ARCHITECTURE.md#phase-6-semantic-validation-hardening), [history internals](IMPLEMENTATION_INTERNALS.zh-CN.md) and TD-101 in [the register](TECHNICAL_DEBT.md).
