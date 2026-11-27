# EG-003 — focused checkpoint OOM investigation

Status: **Open; original failure not reproduced and exact historical return site unknown.** This follow-up investigates only the initial Preview.1 Release checkpoint failure. No production correction is justified by the available evidence. The [technical debt register](TECHNICAL_DEBT.md) retains the anomaly; this document does not change the original stage result or authorize Preview.2.

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

The diagnostic source reports payload line324; the unchanged production site is [checkpoint_core.inc](../src/checkpoint_core.inc), `payload = ow_checkpoint_oom() ? NULL : malloc(n + 8)` at line318. `ow_core_import` preserves OW_NOMEM at its final status normalization. `cp_sweep` skips CORRUPT/UNSUPPORTED orphans, but propagates OW_NOMEM. Diagnostic counters show the sticky allocation hook is zero at failure. No executed corruption return was observed in the refined runs.

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
