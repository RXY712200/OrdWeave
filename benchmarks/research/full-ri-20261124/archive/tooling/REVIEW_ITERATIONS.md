# V0.5 implementation and evidence review

## Implementation review before the formal captures

- Verified actual Full development HEAD, released pin, OrdWeave HEAD/RC and the submodule gitlink. Current Full changes after release are documentation-only. No product files changed.
- Corrected a research CMake target-name collision before execution.
- Preserved the failed initial locality fixture: append10000 produces a48-entry last block. Use10016 to reach a true64-entry destination; QPC/monotonic clocks replace millisecond clock().
- Fresh local Full strict GCC suite39/39 and research/OrdWeave suite16/16 passed. Additional upstream-allocation injection through DB insertion/removal reached37+34 failing ordinals, then success, preserving revision/order/IDs/live bytes.
- The research validation wrapper first supplied an unsupported --config option, then assumed VS17 on a VS18 runner. Failed logs preserved; wrapper now reads the configured generator. These are harness failures, not product defects.
- The archived generator properly rejects new DB sizes/lengths. Preserve it unchanged and implement a separately versioned, purpose-stream PCG DB generator with an independent serialized-operation verifier. Freeze all45 byte-identical original inputs plus27 distinctly labeled generated DB traces. Cached bytes avoid repeating expensive generation; executable diagnostic replays remain fresh.

## Adversarial design review

- Initial locality case C1 varied source occupancy as well as destination. C2 keeps source0 constant and adds near/far destinations with equal initial occupancy32. Preserve any C1 receipts separately; do not pool them. Structural fingerprints cover occupancy and every occurrence's block membership. Residual differences in destination/index path remain part of the intervention, not pure distance causality.
- OM wrapper identities are separately allocated and checked for surviving original occurrences; replacement priorities and permanent sentinel/backend capacity count toward representation storage. Production builds omit the global diagnostic allocator. Different languages/compilers and clocks remain explicitly reported.
- Repeated within-state interventions are not independent states. Workload seeds, formal processes, diagnostics and warmups stay separate. No generated database request stream is labeled human activity.
- OrdWeave's Windows-only backend is not a Linux success. Its global staged occurrence index and WAL/recovery work belong to the database. Full's operation-level failure fixtures do not certify an atomic multi-call business transaction.
- Input and evidence directories disable automatic Git text conversion to preserve exact byte hashes across Windows/Linux checkouts. ZIP input bytes remain identical; source archive retains upstream MIT license.

The initial duplicate evidence workflow was canceled to avoid concurrent duplicate campaigns. Source/protocol corrections retain their Git history and preflight logs. Final result, omissions, artifact and manuscript checks will be appended after evidence collection; this record is not independent acceptance.

## Final evidence and artifact review

Matched every captured component timing to raw stdout, all324000 database receipts to success, canonical input hashes and cross-host locks. Checked71 allocation failures and four source archives. Distinguished native database files from process receipts. Corrected newline-sensitive audit comparison without changing raw bytes. Added evolving-locality paired summaries, upstream crate development-status disclosure, and retained historical treap sensitivity scope. Source/export QA and visual review of all final11pages completed; fixed figure tick crowding and last-page layout, regenerated and rechecked. No unresolved actionable within-scope correctness issue identified; scientific limitations remain explicit. Production checkouts and historical evidence are unchanged.
