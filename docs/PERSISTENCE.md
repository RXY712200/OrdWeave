# Persistence format 3

RC.1 source baseline: `3f99029da73e3b4f0a75a538dd241cf94cda23d7`. This main-branch documentation update is newer than the published tag.

[中文使用](USAGE.zh-CN.md) · [完整API](IMPLEMENTATION_API_REFERENCE.zh-CN.md) · [执行导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) · [内部实现](IMPLEMENTATION_INTERNALS.zh-CN.md) · [验证地图](IMPLEMENTATION_VALIDATION.zh-CN.md) · [技术债](TECHNICAL_DEBT.md)

Windows x64 only. All integer fields are unsigned big-endian; bytes are eight bits. No C structures are dumped. SHA-256 detects accidental damage and chain inconsistency, not malicious rewriting. Binary64 is required by the core static assertion. Compatibility is explicit: unsupported versions are rejected; no migration is provided.

## Directory and coordination

An explicitly supplied absolute UTF-8 drive path names a new directory for create, or an existing database for open. Existing paths are never initialized or overwritten by create. Paths reject UNC/device/extended namespaces, relative components, DOS device names, ADS, short-name tilde aliases, trailing spaces/dots, and reparse components. Length is bounded to 239 UTF-16 code units. Ancestors must exist. Use local ordinary Windows filesystems; network filesystems are outside the validated contract. Hostile concurrent namespace changes are not an authenticated security boundary.

Layout: `manifest.ow`, `writer.lock`, and `wal/00000000000000000001.wal` onwards. Writer coordination uses a live Windows file handle plus LockFileEx; deletion is denied while open. A leftover lock pathname is not evidence of a live owner. Root and WAL directory handles pin directory identity against deletion. File handles reject reparse points and multiple hard links. The OS releases handles/locks on process death. Caller thread serialization remains mandatory.

Creation makes a new directory, locks it, creates/flushed the first segment, then writes/flushed `manifest.pending` and publishes it using MoveFileExW with WRITE_THROUGH and no replacement. Interrupted initialization without a manifest is RECOVERY_REQUIRED, never silently restarted. Failed create may leave this new incomplete directory for operator inspection; it never removes unrelated data.

## Manifest: exactly 96 bytes

| Offset | Width | Meaning |
|---|---|---|
| 0 | 8 | ASCII OWMAN003 |
| 8 | 4 | Version 3 |
| 12 | 4 | Length 96 |
| 16 | 16 | Random Database ID from BCryptGenRandom |
| 32 | 8 | Segment target 67108864 |
| 40 | 8 | Maximum frame 16777216 |
| 48 | 16 | Required zero reserved bytes |
| 64 | 32 | SHA-256 of bytes 0..63 |

The ID never changes on reopen. It is unrelated to runtime object IDs. Reserved fields or unsupported fixed limits are incompatible formats. Wrong size, magic or digest is corruption.

## Segment header: exactly 128 bytes

| Offset | Width | Meaning |
|---|---|---|
| 0 | 8 | ASCII OWWAL003 |
| 8 | 4 | Version 3 |
| 12 | 4 | Length 128 |
| 16 | 16 | Owning Database ID |
| 32 | 8 | Sequence starting at 1 |
| 40 | 8 | Expected first Revision |
| 48 | 32 | Previous committed frame digest; zero for initial segment |
| 80 | 16 | Required zeros |
| 96 | 32 | SHA-256 of bytes 0..95 |

Segment names contain exactly 20 decimal digits and `.wal`. Without a recovery root, discovery requires contiguous sequences starting at one; a validated root instead supplies the first retained sequence and predecessor digest (see [HISTORY.md](HISTORY.md)). Discovery also requires no unknown files/subdirectories, and matching header sequence/identity/revision/digest. Empty intermediate segments are invalid; an empty final segment is allowed.

Before appending a frame that would exceed the 64 MiB target, write the next header to the exact next `.pending` name, flush, close, rename without replacement using WRITE_THROUGH, then open the new final file. A frame never spans segments. After chain validation, recovery discards only a pending file whose sequence is exactly one beyond the final authoritative segment. Conflicting pending names fail. A published empty final segment is a valid interrupted rotation. No manifest rewrite occurs per commit.

## Mutation frame: 136 + payload bytes

| Offset | Width | Meaning |
|---|---|---|
| 0 | 8 | ASCII OWTXN003 |
| 8 | 4 | Complete length N, 144 through 16777216 |
| 12 | 2 | Version 3 |
| 14 | 2 | Opcode 1 through 9 |
| 16 | 8 | Revision, exactly previous + 1 |
| 24 | 4 | Payload length N - 136 |
| 28 | 4 | Required zero |
| 32 | 32 | Previous committed digest (zero initially) |
| 64 | 32 | SHA-256 of bytes 0..63 |
| 96 | N-136 | Deterministic mutation payload |
| N-40 | 32 | SHA-256 of bytes 0 through N-41 |
| N-8 | 8 | ASCII COMMIT03 |

Header integrity and bounded lengths are checked before allocating a frame. Chain digests are frame digests, not header digests. Database ownership is inherited from the validated segment header.

All payloads start with an 8-byte nonzero object ID. Opcodes: 1 Record create, 2 Record delete, 3 field set, 4 field remove, 5 Collection create, 6 Collection delete, 7 Occurrence insert, 8 Occurrence move, 9 Occurrence remove. Opcodes 1,2,5,6,9 contain only that ID.

Fields append a 4-byte name byte length and nonempty UTF-8 name bytes (no NUL). Set additionally appends one type byte, 4-byte value length and complete value. Types: 0 null (length 0), 1 boolean (length 1, byte 0 or 1), 2 signed integer (length 8, two's complement), 3 finite binary64 (length 8, IEEE bits, positive zero only), 4 UTF-8 string, 5 arbitrary binary. Strings are length-delimited and can contain NUL. All UTF-8 is strictly validated. Negative zero on input is normalized to positive zero; negative-zero encodings, infinities and NaNs are rejected during replay. Value payloads and names are bounded to 1 MiB and the existing logical Record budget remains 8 MiB.

Insert appends Collection ID (8), Record ID (8), placement (1), anchor ID (8), for 33 payload bytes total. Move appends placement (1), anchor (8), for 17 total. Placement 0 front, 1 back requires zero anchor; 2 before, 3 after requires a live same-Collection anchor. All fields are mandatory; extra/trailing payload bytes and unknown opcodes are rejected. Replay must create exactly the encoded IDs and change Revision exactly once. No-op frames and logically inconsistent references are corruption. Deleted IDs remain consumed through checkpoint high-water counters and subsequent replay, or the complete creation/deletion sequence during genesis replay.

## Commit state machine and errors

READY -> BUSY -> prepare deterministic frame resources -> apply existing core mutation -> append complete frame -> FlushFileBuffers -> READY/acknowledge. A failed core operation commits nothing. A genuine no-op returns without a frame. Caller outputs remain cleared until acknowledgement. All queries and mutations check session state; BUSY rejects access while an intermediate mutation exists. The caller must still serialize threads; this state guard is not synchronization.

Any append, hash, rotation or flush failure after successful memory application returns OW_COMMIT_UNKNOWN, clears outputs, and poisons the session. All subsequent reads/mutations return OW_RECOVERY_REQUIRED, including Revision and identity; only close/destroy is allowed. Closing frees all resources and reports RECOVERY_REQUIRED. It never tries to roll back an operation that may be durable. Reopen validates the authoritative chain to resolve the outcome. Compare recovered state/Revision with application intent before retrying; automatic retry can duplicate a successfully durable operation.

I/O errors, corruption, unsupported format, busy ownership, commit uncertainty and unavailable initialization/recovery have distinct public statuses. Open failure destroys the unpublished reconstruction and never returns a usable partial database. Ordinary in-memory create performs no disk I/O.

## Recovery

Acquire writer ownership, validate manifest, discover the complete authoritative retained chain (from segment 1, or the validated recovery root), validate each segment and frame, and replay through the existing core/LKS implementation without recursive WAL writes. Publish only after complete validation. Revision, expected IDs, referential integrity and no-op rules are enforced by replay.

A final short frame header is repairable only if its available magic prefix matches. A complete header must pass its own SHA and bounds checks before a missing body is classified as incomplete. Only these final short writes are excluded; truncate to the last complete validated boundary with SetEndOfFile and flush before allowing append. Any complete frame with bad digest/boundary, malformed payload, or broken chain is corruption, even at EOF. An incomplete nonfinal segment is corruption. No frame is skipped, no later segment is selected over an invalid predecessor, and no corrupt database is replaced with an empty database.

FlushFileBuffers and WRITE_THROUGH rely on Windows/filesystem/device durability contracts. Process-kill tests are not power-loss tests and do not prove hardware honors flushes. Checkpoint recovery adds native initialization and ordinary WAL-tail application while independently auditing retained semantics; see [CHECKPOINTS.md](CHECKPOINTS.md).

## Tests and remaining debt

`tests/format_vectors.json` contains fixed independent manifest/segment/first-commit hex vectors for ID bytes 00..0f. `format_oracle.py` independently assembles bytes using Python hashlib, checks those fixed vectors, compares a real emitted frame, and supplies malformed/corrupt/noncanonical recovery cases. It does not call the production encoder. `tests/persistence.c` exercises production-sized rotation, reduced-threshold rotation, seven test-only failure boundaries, cross-process locks, abrupt TerminateProcess, post-write/pre-flush and post-flush/pre-report uncertainty, and deterministic array-model replay. Test fault hooks are compiled only into a separate test library; production contains no enabled hooks. Simulated errors are not physical disk failure or power-loss emulation. Successful tests clean their isolated directories; failed tests may retain build-directory evidence.

Performance debt includes repeated retained-WAL replay, per-call SHA provider setup and occurrence-index reconstruction; see [the register](TECHNICAL_DEBT.md). Explicit anchored prefix cleanup and full backups exist as described in [HISTORY.md](HISTORY.md); ordinary open does not compact. Multi-operation transactions and skipping a corrupt committed retained prefix are unsupported.

## Anchored extension

WAL/manifest bytes remain v3. Without recovery roots, genesis recovery is unchanged. With validated format-1 recovery roots, the first retained segment and predecessor digest come explicitly from the root and mandatory baseline checkpoint; deleted prefixes are unavailable history, not an implicit new genesis. Ordinary open never compacts. Read-only history/verification suppresses tail truncation. Backup completion markers prevent opening incomplete exported copies. See HISTORY.md.

## Phase 6 semantic validation hardening

Every retained WAL operation is independently applied to the core, including operations before the chosen checkpoint; ID gaps, failed operations and no-op frames are corruption even with valid checksums. See [the architecture audit](ARCHITECTURE.md#phase-6-semantic-validation-hardening), [execution walkthrough](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) and resolved TD-101 in [the register](TECHNICAL_DEBT.md).
