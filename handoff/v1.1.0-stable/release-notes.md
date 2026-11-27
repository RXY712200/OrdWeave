OrdWeave v1.1.0 is a Windows x64 C17 embedded static database with48 public functions and LayerKeySort Full4.2 pinned to f751e1ee2afd682874281d6dc495969f9eca49eb.

- Compressed permanent-Occurrence-ID lookup preserves public LKS Collection ordering and failure-atomic publication.
- Direct unpublished checkpoint import and call-local successful anchor reuse reduce redundant construction/verification while retaining independent semantic WAL audit.
- Optional bounded single-field indexing and typed live/history range queries own immutable keys/copied results. Cache maintenance failure discards configuration without failing committed data; explicit enable is required to rebuild.
- Original44 API contracts/types/status values and WAL v3, checkpoint/selection/root/policy v1, LKS4SNP1 formats remain compatible. Accepted bilateral/canonical tests are reused.

Matched seed17 cumulative measurements (1000 memory operations, N2048, Windows GCC16.2): A=1.0+Full4.1 33.329ms, B=identical1.0 runtime+Full4.2 21.612ms, C=1.1+Full4.2 cache off0.098ms. This separates dependency A→B from database B→C; small aggregate C timings do not promise per-operation latency. Persistent N128 mixed operations were758/773/763ms, showing flush-dominated tradeoffs. Optional cache off/on results are a separate experiment, not the common-API cumulative ratio. See the raw index totals and occupancy in the [accepted RC report](https://github.com/RXY712200/OrdWeave/blob/ac581c7324dc962daa2146e52c68039a09541792/handoff/v1.1.0-rc.1/REPORT.md).

Known limits: caller-serialized single-writer access; no SQL/network/backend beyond Windows; full buffers/copies and repeated semantic audit remain costs. Index key-byte budget excludes fixed/LKS metadata; disabled mode retains an8-byte Record backlink. EG-001/002/003 remain registered. Process-crash tests are not physical power-loss certification. No universal speedup or other-database superiority claim.

Build from a recursive source checkout with CMake3.21+, a Windows x64 C17 compiler and Python3 for tests:

```powershell
git clone --recurse-submodules --branch v1.1.0 https://github.com/RXY712200/OrdWeave.git
cd OrdWeave
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

[Usage](https://github.com/RXY712200/OrdWeave/blob/v1.1.0/docs/USAGE.zh-CN.md) · [48 API reference](https://github.com/RXY712200/OrdWeave/blob/v1.1.0/docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md) · [Architecture](https://github.com/RXY712200/OrdWeave/blob/v1.1.0/docs/ARCHITECTURE.md) · [Known debt/evidence gaps](https://github.com/RXY712200/OrdWeave/blob/v1.1.0/docs/TECHNICAL_DEBT.md).

Source release only: GitHub-generated source archives omit the submodule contents. Use recursive Git checkout; no custom binaries, SDK or attached assets are supplied. Independent post-publication acceptance remains with the owner/reviewer.

Measured optional equality index (median build+update+read totals,512 reads):
- N8192: cache off 46.8948ms, on 1.6945ms; 257-bucket selective read-only.
- N1024: cache off 4.6226ms, on 0.4004ms; 19-bucket genuinely interleaved64 changes, verified occupancy.
