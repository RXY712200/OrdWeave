# EG-003 — focused checkpoint OOM investigation

Status: **Open; original failure not reproduced and exact historical return site unknown.** This follow-up investigates only the initial Preview.1 Release checkpoint failure. No production correction is justified by the available evidence. The [technical debt register](https://github.com/RXY712200/OrdWeave/blob/3c5f371ca53c62912dc3d4d63aa11dc5277af020/docs/TECHNICAL_DEBT.md) retains the anomaly; this document does not change the original stage result or authorize Preview.2.

## Source and preserved fixture

Examined runtime: `26704b68f2cb61e6edb5746294e924aceb08d5ab`, Full4.2 `f751e1ee2afd682874281d6dc495969f9eca49eb`. The original log reports OOM boundary126, status8 (OW_CORRUPT), where the test expected OW_NOMEM. The preserved post-failure fixture was hashed before investigation and copied into ignored build storage; the original was not opened or modified during this follow-up. A previous Preview.1 probe had already opened it, so it is not an untouched snapshot from the instant before the failing call.

The 29-file inventory contains manifest, WAL, lock, two matching checkpoint selections and **24** checkpoint files: generations21–43 and89. Current/backup selection is generation89, Revision21, retention1, and no pending file survives. Independently checked SHA256, identities, fixed lengths, selection reference/digest and checkpoint header generations all pass. Every retained checkpoint also imports into a fresh core and satisfies assertions for Revision21, the deleted Record ID gap, six typed fields including embedded NUL, cycle4 and three collection orders. Unreferenced older checkpoints are residue of interrupted/injected cleanup, not evidence that they are selected or corrupted.

## Bounded diagnostics and observed path

Only the existing checkpoint executable was instrumented in an isolated source archive; no instrumentation entered the production tree. Budget completed: **three** runs of that one checkpoint test, **five** one-call replays on separate post-failure fixture copies at boundaries124–128, and **24** individual checkpoint import inspections. No benchmark, full CTest suite or Preview.1 workload was repeated.

All three checkpoint-test runs pass. Each of the five copied-fixture attempts returns OW_NOMEM and subsequently reopens at unchanged Revision21. Refined runs trace boundary126 to:

```text
ow_checkpoint_create
  -> cp_sweep (after selection publication)
    -> ow_core_import (older unreferenced checkpoint)
      -> field payload allocation: ow_checkpoint_oom() returns true
      -> OW_NOMEM
    -> OW_NOMEM (not swallowed as a corrupt-orphan skip)
  -> OW_NOMEM
```

The diagnostic source reports payload line324; the unchanged production site is [checkpoint_core.inc](https://github.com/RXY712200/OrdWeave/blob/26704b68f2cb61e6edb5746294e924aceb08d5ab/src/checkpoint_core.inc#L318), `payload = ow_checkpoint_oom() ? NULL : malloc(n + 8)` at line318. `ow_core_import` preserves OW_NOMEM at its final status normalization. `cp_sweep` skips CORRUPT/UNSUPPORTED orphans, but propagates OW_NOMEM. Diagnostic counters show the sticky allocation hook is zero at failure. No executed corruption return was observed in the refined runs.

This is the **observed new NOMEM path**, not the proven original CORRUPT path. Injection ordinal126 counts checkpoint allocation-hook calls; it is not a permanent named allocation site. Successive attempts publish generations and may partially sweep old files, so the on-disk state and traversal work affect which allocation is reached. A post-failure fixture is not the missing pre-call state or original process heap.

## What can be inferred about the original failure

The preserved selection and highest surviving native generation are89, whereas new boundary126 traces publish generation90 before an OOM in sweeping. Source inspection shows ordinary reopen does not sweep checkpoint generations, and successful native final publication is not removed by the failed-call temporary cleanup. Under the preserved-state assumption, this points toward an original rejection **before generation90's native publication**, not the newly observed post-publication sweep failure. This is an inference, not a recorded historical stack trace; external file changes were not monitored at the time.

The remaining relevant paths are:

| Path | How OW_CORRUPT can reach the caller | Missing original evidence |
|---|---|---|
|`cp_next_generation`|Rejects invalid name/width/number/extension or directory/reparse entry; create propagates status.|Original directory enumeration and attributes. Present names are valid.|
|Native pending verification: `cp_publish` → `cp_verify_file`|File length/bytes mismatch, undersized load, core import structure/digest/graph rejection, or normalized invalid import status.|The rejected pending file is removed on pre-publication failure; none remains.|
|`cp_load_bytes` → `read_exact`|Unexpected zero-byte read before the requested fixed length is complete returns OW_CORRUPT.|Original file/read events; no such event was reproduced.|
|Core import error normalization|Invalid graph/resolver/capacity/internal statuses become CORRUPT; NOMEM itself is explicitly preserved.|Original pre-normalization status and parser position.|

The current export path does not translate the explicit allocation hook's OW_NOMEM into OW_CORRUPT. Later metadata/sweep corruption paths also exist in source, but the surviving highest generation argues against them for this original event, subject to the assumption above. The original failure log records only the final status, so naming one exact historical return statement would be unsupported. Present valid files do not rule out transient pending-file rejection or a prior in-memory failure.

## Decision, verification and remaining risk

No production fix, test relaxation, status remapping, dependency change or new persistent format was made. EG-003 remains Open because the original failure is credible but unlocalized; no upstream defect, data loss or host interference has been established. The bounded diagnostics provide substantially more evidence than a successful rerun, but do not establish absence of a bug. Preserve the original log and fixture, and retain the diagnostic patch for a future recurrence.

If it recurs, capture the first taken corruption site and pre-normalization status, generation/phase, allocation ordinal, directory entry/attributes and rejected pending bytes **before cleanup**, on a disposable diagnostic fixture. Do not weaken validation or enable diagnostic logging in production to obtain a green result. No broader experiment or architecture work is proposed here.

Findings, source instrumentation patch, fixture hashes, exact boundary excerpts, import assertions and compressed diagnostic logs are published on `evidence/v1.1.0-preview.1` under `handoff/v1.1.0-preview.1/eg003/`. The immutable URLs and documentation commit are supplied in the completion report. Existing full Preview.1 CI evidence remains attached to its actual runtime commit; this follow-up's documentation checks do not re-certify new runtime behavior.

## Published follow-up and actual commands

Documentation commit: `3c5f371ca53c62912dc3d4d63aa11dc5277af020`, parent `26704b68f2cb61e6edb5746294e924aceb08d5ab`; changes only the EG-003 investigation and debt record. Runtime, dependency, tests, formats, CI and build targets are unchanged. Existing Draft PR #1 remains unmerged. Main and existing release tags are preserved.

Local diagnostic commands (Windows x64 GCC/MinGW16.2, Ninja Release):

```powershell
python build/eg003_prepare.py
cmake --build build/eg003/diagnostic-build --target ordweave_checkpoint --parallel 4
./build/eg003/diagnostic-build/ordweave_checkpoint.exe
python build/eg003_refine.py
gcc -std=c17 -O2 -Iinclude -Isrc build/eg003/inspect_checkpoint.c build/preview1-release/libordweave_test_storage.a build/preview1-release/third_party/LayerKeySort/liblayerkeysort.a -lbcrypt -o build/eg003/inspect_checkpoint.exe
python build/eg003/inspect_fixture.py
python tools/validate_docs.py
git diff --check
```

The first diagnostic compile exposed omitted stdio includes in the temporary instrumentation; these were added to the two isolated includes before successful execution. No repository code was changed for that tooling correction. The published prepare script includes this correction and accepts checkout/fixture arguments. The refined script runs only two further checkpoint-test processes and five fixture-copy calls. `diagnostic.patch` records the actual final isolated instrumentation against the examined commit; the first run preceded the refined hook observer and fixture CLI. These probes preserve OOM hook counts but fprintf can perturb heap layout and timing; successful instrumented runs cannot exclude such sensitivity.

Documentation validation passed: 44/44 API signatures, six production C units, three includes, five private headers, immutable historical/current gates, 22 documents and482 relative links/anchors. No full CTest or documentation-example suite was rerun for documentation-only changes. Original fixture content hashes were rechecked after all diagnostics: all29 unchanged. The initial failure log remains immutable in the parent handoff directory.

Files: fixture-inventory.json contains original29 content hashes/sizes; fixture-inspection.json records independent fixed-byte checks and24 real core imports/assertions; focused-results.json records two refined test exits and five copied-fixture exits (the first run is in checkpoint-run-1.log.gz); boundaries-124-128.log provides concise exact excerpts; fixture-124.log through fixture-128.log contain one-call outcomes; three compressed logs preserve complete diagnostics; diagnostic.patch and the C/Python scripts make the probes inspectable. No database, executable, credential or private policy is uploaded.

Reproduction requires an authorized local copy of the preserved original fixture (not uploaded) and a fresh disposable `build/eg003` directory. Use `python prepare_diagnostics.py <checkout> <fixture>`, then `python run_focused_diagnostics.py <checkout>`; the resulting isolated source/build paths match the recorded local layout. Do not treat a new runtime failure or successful instrumentation as a proven replay of the missing original pre-call process. The 24-file inspection helper links the original Preview.1 test-storage library; it is not new production code.

**Technical Debt Impact:** EG-003 remains Open with new bounded evidence and a narrower phase hypothesis. No TD status changes or defect fix are claimed. Diagnostic work is complete; exact historical return localization remains unavailable, and no speculative change was committed. Further reproduction should capture first-failure events rather than repeat a large workload.

Published text uses LF line endings. The diagnostic patch uses zero context; apply only to a disposable archive with git apply --unidiff-zero. Compressed full logs preserve original diagnostic bytes.
