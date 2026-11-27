# OrdWeave repository guidance

OrdWeave is a Windows x64 C17 embedded static database. Read [the technical handoff](docs/HANDOFF.md), [development guide](docs/DEVELOPMENT.zh-CN.md), [public API reference](docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md) and task-relevant format/implementation guides. These public documents are sufficient for contributors; no additional private policy or repository is required.

## Source and compatibility

- Inspect actual HEAD, working tree, diff, source, relevant test assertions and exact-commit CI before changing behavior. Preserve unknown local changes.
- Public contracts are in [ordweave.h](include/ordweave.h). Preserve status values/signatures, result ownership, permanent typed IDs, one Revision per state-changing mutation, no-op behavior and caller-serialized access.
- WAL/manifest/segment/frame remain v3; checkpoint/selection/root/policy remain v1; native order snapshots use LKS4SNP1. Exact layouts stay in [PERSISTENCE](docs/PERSISTENCE.md), [CHECKPOINTS](docs/CHECKPOINTS.md) and [HISTORY](docs/HISTORY.md). Do not change public API, storage formats or production behavior outside the requested task.
- LayerKeySort Full is pinned at `f751e1ee2afd682874281d6dc495969f9eca49eb`. Use its public APIs; do not modify upstream or change the pin without a specific reviewed task. Runtime LKS Handles are not permanent OrdWeave IDs and must be rebuilt after restore.
- Keep object ownership, borrowed container items, reference counts and cleanup explicit. Preserve staged mutation publication and failure atomicity. COMMIT_UNKNOWN requires stop/close/validated reopen, not blind retry. Preserve semantic WAL auditing, mandatory retained baselines, root agreement before prefix deletion and independently verified backup completion.
- Production has six C translation units; five implementation includes belong to their owning translation units, not separate objects. See [internals](docs/IMPLEMENTATION_INTERNALS.zh-CN.md). Do not perform unrelated refactors or add features in documentation/test-only work. Record credible data-integrity or memory-safety defects precisely before expanding scope.

## Build and verification

Requires a recursive submodule checkout, CMake 3.21+, Windows x64 C17 compiler and Python 3 for tests. Use separate build directories per generator/configuration. For MSVC:

```powershell
cmake -S . -B build/msvc -A x64
cmake --build build/msvc --config Debug
ctest --test-dir build/msvc -C Debug --output-on-failure
cmake --build build/msvc --config Release
ctest --test-dir build/msvc -C Release --output-on-failure
```

For GCC/MinGW and Ninja, configure a separate directory with `-G Ninja -DCMAKE_BUILD_TYPE=Release` or `Debug`, then build and run CTest there. The root project registers 12 CTests; [the validation guide](docs/IMPLEMENTATION_VALIDATION.zh-CN.md) maps their assertions, independent oracles and platform limits. The unchanged [Windows workflow](.github/workflows/windows.yml) tests MSVC Debug and Release and checks the dependency pin.

Documentation checks:

```powershell
python tools/validate_docs.py
python tools/validate_docs.py --examples --generator Ninja --config Release
python tools/validate_docs.py --examples --generator Ninja --config Debug
git diff --check
```

Select checks appropriate to the changed behavior. Keep source inspection, local execution, exact-commit CI and hardware durability claims distinct. Update affected public documentation and [actual technical debt records](docs/TECHNICAL_DEBT.md) when findings materially change; retain stable IDs and resolved history.

## Release and maintenance safeguards

The six implementation phases are complete. Published `v1.0.0-rc.1` is a Prerelease at `3f99029da73e3b4f0a75a538dd241cf94cda23d7`; preserve its tag and Release. `v1.0.0` is published Stable at `32f853defc0a2e9f0ca809fb3535a90c977c7665`. This branch is unreleased `1.1.0-preview.4` development, limited to the approved Step15 field-index/range implementation and validation; see [Preview.4](docs/PREVIEW_4.md). Further development or releases require a separate task. Documentation maintenance does not authorize production/API/format/dependency changes, upstream edits, new tags or Releases. Actual release evidence and compatibility constraints remain in [RELEASE_CANDIDATE](docs/RELEASE_CANDIDATE.md).


Current Preview.3 decisions and limits: [persistence/recovery review](docs/PREVIEW_3.md). Earlier task-scope prose is historical. No RC.1 is authorized.


## Approved Step15 implementation state

The approved Preview.4 Step15 continuation adds exactly four public functions (48 total), an advisory session-local field index and live/history typed range queries. See [implemented contract](docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md#preview4-step15). Earlier research-only restrictions describe the research snapshot, not this explicitly approved continuation. Six production translation units own five implementation includes. Independent acceptance is pending; no merge, RC, tag or Release is authorized. Preserve all old API/status types and persistent bytes.
