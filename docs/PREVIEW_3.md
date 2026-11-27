# Preview.3 — persistence and recovery decisions

Unreleased `1.1.0-preview.3`, exact accepted parent `2bbf7640711e6561a5d5979f0de53a7f6ee0a1e8`, Full4.2 `f751e1ee2afd682874281d6dc495969f9eca49eb`. Only Steps9–12 of the fixed six-stage/twenty-step roadmap are authorized. No API, format, platform, concurrency or incremental-backup change; no Preview.4.

## Step9: unpublished occurrence import

`ow_core_import` now allocates Occurrences and prepares/publishes their private ID-index entries directly into the fresh unpublished core. It does not insert temporary explicit-order residences. Existing empty Collection control objects remain and are replaced after public native `lks_snapshot_restore_order`; no private bulk builder is called. The native resolver binds handles only to restored objects.

The parser still checks exact header/digest/length/256MiB bound, ascending typed IDs, high waters, fields, namespaces and associations. A binary lookup into the temporary Collection metadata array accounts for each association and rejects excess membership immediately; final declared/actual/native counts agree. Record refs are incremented exactly once, guarded against overflow. Every imported Occurrence increments the unpublished construction Revision, preserving the old minimum state-changing-operation count check before installing historical Revision. Duplicate, missing, extra and wrong-Collection native associations remain invalid; every object must be seen exactly once. Failed unpublished state is destroyed through the existing core destructor, including objects without live handles and empty/unrestored orders. Missing Record/Collection references still classify CORRUPT; NOMEM remains NOMEM.

Capture/export was reviewed and retained: canonical unique-name qsort, temporary field frame encoding, whole bounded writer buffer, per-Collection native snapshot and serialized blob. LKS has no streaming snapshot serializer. Avoiding one import residence set is useful without broadening this change into a complete buffer/encoding rewrite. Remaining full buffers are TD-003, not claimed solved.

Paired independent import CPU medians (one warmup/three measured runs, same Windows GCC Release host):

| Fixture | Preview.2 ms | Preview.3 ms | Reduction |
|---|---:|---:|---:|
|128 occurrences, one Collection, six integer fields|0.0417|0.0360|13.7%|
|8192, one Collection, six fields|1.5293|1.0573|30.9%|
|8192, eight Collections, six fields|1.3857|0.9797|29.3%|
|2048, eight Collections,64 fields|0.4596|0.2779|39.5%|

A separate deletion-gap fixture retains4096 of8192 IDs across eight Collections: median import0.6164ms →0.4678ms. These are fixed generated states, not field telemetry; short spans limit precise ratios. Exact equality with the source core is asserted after every import. Requested-allocation diagnostics are separate: N8192 one-Collection import attempts25543→17096; peak including live source/export buffer and candidate core2773050→2423455 bytes. Multi-Collection peak2355346→2249239 bytes. Resident final state is equivalent; most saved work is transient residences, not a new database memory bound. Wrapper reporting residue is not a zero-leak or whole-process RSS claim.

## Step10: bounded identical-anchor reuse

Open/history retain structural scanning, authoritative full retained semantic audit, checkpoint-path restoration plus tail and logical equality. Older historical targets still audit latest semantics before reconstructing the earlier target. Root baseline and incomplete-tail rules are unchanged. No session-success or revision-only cache is introduced.

Within one `checkpoint_load`, at most six selection entries can repeat. Successful `(Revision,digest)` checks are reused only within that invocation, under the same protected store/root/chain operation. Different Revision or digest always invokes full `cp_anchor`; only successful checks enter the six-entry stack array. The checked metadata bounds limit both files to three entries. No cache survives the operation; no semantic audit or final comparison is removed. Root/identity/chain remain the same across the local lookup.

Separate diagnostic maintenance runs (open/checkpoint/earlier-history/verify/full-backup, not production timings) show genesis8192 segment calls73→54 and decoded frames598235→442530; semantic segment calls remain15. Retained-root case55→45 calls and275→225 frames; semantic calls remain15. Thus the removed work is duplicate structural ancestry checking, not TD-101's independent semantic proof. Two new independent oracle cases cover same Revision/correct digest in a different generation and same Revision/wrong digest; the latter must still return CORRUPT.

## Step11: SHA, WAL and buffer decisions

Flush ordering, WAL encoding and quarantine are unchanged. A separate128-occurrence persisted-mutation diagnostic measured133 flush calls, about110–117ms flush time, and274 provider opens totaling about0.57–0.61ms. Instrumented timings are diagnostic, not a production speed claim. A10000-hash microexperiment after warmup measured medians4.6852ms for the existing full helper,3.4795ms for provider open/close alone and0.9145ms for hashing with one local reusable provider. Digests match in both hashing modes. This demonstrates a real isolated CPU opportunity, not a proven whole-database win.

Retain current per-call provider ownership for this stage: application flush work dominates the measured persisted path, while propagating reusable contexts throughout frame/checkpoint/history helpers adds lifecycle and independent-database/thread obligations. No unsynchronized global handle, Windows-version-dependent pseudo handle or new dependency is introduced. TD-005 remains Open with measured evidence; it is not declared unimportant universally. Whole checkpoint load/export/native buffers and duplicate field encoding remain TD-003; no unproven streaming rewrite is forced. Existing256MiB limit is a checkpoint-file limit, not database size.

## Step12: backup, retention and integrated evidence

Full-copy backup remains fresh-directory, incomplete-marker, strict-name/path/source identity, per-file flush, manifest-last, independent open/semantic equality/verify and completion-last. The verification after destination open also validates the final copied root/history promises; it is retained. Known unpublished-file cleanup remains conservative: unknown/corrupt or potentially authoritative files and mandatory baselines are not deleted. Root agreement precedes prefix deletion, and the busy/poisoned guard remains unchanged. TD-004 stays Deferred, TD-006 Open.

On identical cloned genesis8192 fixture bytes, median open159.1543→131.1642ms, checkpoint19.0206→17.2065ms, earlier history239.3537→181.9774ms, verify449.7325→333.3210ms, backup1108.5005→841.5415ms. Retained-root and small fixtures show smaller gains; checkpoint medians regress about3–4% there, below the prospective >25% AND >10ms IO threshold. No universal IO gain is claimed. These timings are new paired Preview.2/3 results, never pooled with earlier stages.

Existing12 CTests pass locally in both Release and Debug, including independent byte/format/corruption/process-fault oracles. New white-box tests sweep importer preparation allocations and destroy partial state, and independently reseal an impossible minimum-Revision checkpoint (high waters valid) to require CORRUPT. Existing checkpoint OOM assertions remain strict. EG-003 did not recur and remains Open; original logs/diagnostic patch remain immutable. Stable↔Preview.3 and Preview.2↔Preview.3 representative checkpoint/history/backup round trips pass. A fixed-state deterministic checkpoint export is byte-identical across all three versions.

Raw timings, resource rows, instrumented scan/profile logs, diagnostic source patches, exact source revisions and CI status are published separately on `evidence/v1.1.0-preview.3` under `handoff/v1.1.0-preview.3/`. No diagnostic flags enter production. TD-002 is partially mitigated, still Open; TD-003 is partly reduced, still Open; TD-005/006 Open, TD-004 Deferred. TD-001/103 resolved history and EG-001/002/003 limitations remain. No independent acceptance or later-stage authorization follows from this implementation report.
