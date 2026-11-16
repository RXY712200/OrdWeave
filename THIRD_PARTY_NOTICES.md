# Third-party notices

LayerKeySort Full v4.1.0 Stable, https://github.com/RXY712200/LayerKeySort

Source commit: `70d7f0ef5857b54381b863d292a213909d87459f`.

Copyright (c) 2026 RXY712200. Licensed under the MIT License; the complete upstream notice is preserved in [third_party/LayerKeySort/LICENSE](third_party/LayerKeySort/LICENSE). The submodule sources are unmodified and the real static implementation is linked into OrdWeave and exercised directly by the smoke executable.

Obscura64 source: https://github.com/RXY712200/Obscura64

Exact source commit: `3db0122560853a8f3db3ba6888067394dc1a3e62`.

`src/sha256.c` adapts actual `hash_segment` and `obscura64_sha256_segments` from upstream `src/obscura64_sha256.c`, changing names, statuses and includes. `src/wal_windows.c` adapts the chunked `write_exact` loop from `obscura64_persistence.c`, the CreateFileW/LockFileEx ownership pattern from `obscura64_lock.c`, and DOS-device component checking from `obscura64_v2_file.c`. WAL framing, replay, append and rotation are OrdWeave-specific; the complete-file replacement API was not transplanted. No Profile, obfuscation, DPAPI or legacy machinery is included. The original repository is unmodified.

MIT License, Copyright (c) 2026 RXY712200. The full applicable MIT notice is reproduced in the root LICENSE (same copyright holder and terms as the upstream LICENSE at this source commit).

No ProbSpan source is included.

Checkpoint publication also adapts the verified temporary-file publication sequence in Obscura64 `replace_once` (same source commit above): CREATE_NEW temporary files, exact write, flush, close/reopen verification and write-through rename. Checkpoint metadata backup/selection ordering and generation management are new OrdWeave code. No upstream repository was modified.

## Documentation reference (no dependency change)

The Chinese LayerKeySort walkthrough, API, internals, validation guides and README were read at documentation commit `13998b0682e56f34b401773f754e6d2c88c51206` as a presentation benchmark. OrdWeave explanations are grounded in its own RC.1 source; that reference does not change the runtime submodule pin above. See [the implementation walkthrough](docs/IMPLEMENTATION_WALKTHROUGH.zh-CN.md) and [maintainer handoff](docs/HANDOFF.md).
