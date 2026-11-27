# Downstream reverse-reliability research

Subject: unchanged OrdWeave Stable `0350628f1b67353cc15283a7e1a1ae86a7004459`, exact LKS Full4.2 `f751e1ee2afd682874281d6dc495969f9eca49eb`. Only this research directory changes on this branch. No production fix or qualification/release decision is made here.

`protocol.json` is the pre-execution bounded matrix. Numeric thresholds were already in driver.c before execution; `deviations.json` makes the mix labels explicit and records focused supplementary coverage, oracle correction and diagnostic limitations. Seeds17/29/43/71/101 are synthetic distributions, not real-world statistics.

The independent core model is an ordinary array of permanent IDs, with explicit insert/remove/move transformations, independently predicted next IDs and Revision. Each generated operation checks its result and Revision; pilot checks full state each step, larger cases every4096 and finally. Full comparisons enumerate public arrays and validate associations linearly. This is not an LKS/radix/AVL implementation. A deliberate wrong expected sequence is rejected by the same full checker. Invalid anchors have explicitly expected errors. Separate qsort of owned model values is the range oracle, not disabled LKS Stable Sort; enabled and disabled databases are both checked against it.

Primary sustained cases each have200k attempts. Setup, successful mutations, no-ops, rejected logical calls, generated comparison queries and extra public model checks are separate in evidence. Do not add setup, capacity teardown or repeated oracle calls to claim more generated mutations. Public-call totals include the harness status counter plus exactly counted release/destroy calls. Existing test executables are supplemental assertions, not part of the million-operation total.

`traces.py` independently regenerates the canonical little-endian five-uint64 records `(step,kind,index,anchor_index,placement)` without DUT access. All nine primary/pattern trace SHA-256 values were compared with the actual C-written traces. Successful traces need not be uploaded. Query/type/lifetime/capacity fixtures are deterministic source+parameters, with no time/random state or DUT-result-dependent generator. Local fixture paths are output locations, not user data.

`typed_lifecycle_v1.c` is retained **only as a superseded oracle artifact**. It mutated old borrowed model byte storage before comparing a new key. Code inspection found this even though its run had no mismatch; use typed_lifecycle.c, which compares first then copies the new owned bytes. Only that impacted case was rerun. Do not count the superseded run as trusted coverage.

## Reproduce

Use Windows x64, CMake/Ninja, GCC C17 and Python, recursive pinned checkout; no install/global changes. Run commands from repository root with fresh ignored build directories. Scripts intentionally do not delete/reset existing synthetic databases. `python research/lks-reverse-reliability/reproduce.py --build-only` builds unchanged ordinary Stable plus separately flagged allocation-diagnostic variants. Omit --build-only for primary matrix/most supplemental probes; use eg003.py separately on the archived fixture. All subprocesses in the reproduction runner have finite600s caps; primary run.py samples private memory/working set and enforces per-case caps/disk headroom.

Memory instrumentation command (separate reduced configuration):

```powershell
gcc -O2 -std=c17 -DRESOURCE -Iinclude research/lks-reverse-reliability/driver.c research/lks-reverse-reliability/memory.c build/reverse-prod/libordweave.a build/reverse-prod/third_party/LayerKeySort/liblayerkeysort.a -lbcrypt '-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free' -o build/reverse/memory-diag.exe
```

Run memory mode with seed101, modify mix, 00000/20000 steps and equal-length control output paths; distinct files prevent overwriting. This wrapper measures requested allocation bytes, not RSS or a sanitizer. The equal-length controls both leave179 process-requested bytes; native diagnostic teardown separately asserts zero upstream live blocks. No whole-process leak/UB certification is claimed.

Native fault probe is diagnostic, not primary database coverage: actual full-block insert, full-destination cross-block move, snapshot capture/serialize/deserialize/restore with stable owned items/resolver. Restore first32 failures were followed by one window33..512, reaching first non-failing completion at133; passing expanded alone does not cover1..32. Initial move fixture reached no allocation; refined96-member layout exercises one. Serialization has no allocation to fail. Internal work counters in structure_probe observe actual paths; independent arrays remain the expected public sequence. Tag repair/fallback were not observed.

The evidence branch contains the13KB compressed29-file historical synthetic EG003 fixture and file hashes. Extract into a fresh isolated directory and pass it to eg003.py. It is the already-reopened preserved fixture, **not** a snapshot of the original failing instant. This runs one instrumented current-Stable124–128 window; changed import means ordinal mapping is not assumed historical. The uninstrumented current checkpoint test provides the original-style sweep. No recurrence means Open, not resolved.

Final stage matrix, actual receipts/resources/fault triggers, attribution register and upstream-facing limits are published on the separate evidence branch under `handoff/lks-reverse-reliability/`. Ordinary builds and diagnostic variants are explicitly separate. No new CI qualification matrix or automatic future research is requested.
