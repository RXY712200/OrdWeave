# OrdWeave

[简体中文](README.zh-CN.md)

OrdWeave is a **Windows x64 C17 embedded static database** built on independently maintained LayerKeySort Full. It stores typed Records and ordered Collections in which the same Record can occur multiple times, with each Occurrence independently identified and movable. This separates shared data from its placement in lists, layers or arrangements.

[**v1.0.0-rc.1**](https://github.com/RXY712200/OrdWeave/releases/tag/v1.0.0-rc.1) is published as a GitHub **Prerelease**. All six implementation phases completed independent review; **Stable is not approved or published**. The RC source baseline is `3f99029da73e3b4f0a75a538dd241cf94cda23d7`. The comprehensive guides on main were added after publication and are not included in the immutable RC tag.

## Model and capabilities

A Record owns named null/bool/int/float/UTF-8/binary fields. A Collection owns order; an Occurrence references one Record and one Collection. Repeating a Record creates distinct Occurrence IDs without copying its fields. Moving an Occurrence changes only its position. Referenced Records and nonempty Collections cannot be deleted.

Use a disk-free in-memory instance or a persistent directory with a lifetime single-writer OS lock. Persistent state-changing operations each receive one Revision and a flushed WAL commit; there are no multi-operation transactions. Native LKS checkpoints, immutable historical views, Manual/Forever/Last N policies, explicit safe WAL-prefix cleanup, full independently reopenable backups and read-only persisted-data verification are implemented. A policy change alone deletes nothing.

## Build

Requires Windows x64, Git with submodules, CMake 3.21+, a C17 compiler, and Python 3 when tests are enabled. For Visual Studio:

```powershell
git clone --recurse-submodules --branch v1.0.0-rc.1 https://github.com/RXY712200/OrdWeave.git
cd OrdWeave
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

With GCC/MinGW and Ninja use `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` in a separate build directory. Existing clones need `git submodule update --init --recursive`. GitHub-generated ZIP/tar archives omit submodule contents and are not self-contained distributions. Clone main instead of the tag when you need the post-release documentation.

LayerKeySort Full v4.1.0 Stable is pinned at `70d7f0ef5857b54381b863d292a213909d87459f`. Only its public API is used; upstream sources are unmodified.

## Use from C17

```c
#include "ordweave.h"
int main(void) {
    OwDatabase *db = NULL;
    OwRecordId record = {0};
    OwCollectionId collection = {0};
    OwOccurrenceId first = {0}, second = {0};
    OwValue value = {.type = OW_INT, .as.integer = 42};
    if (ow_database_create(&db) != OW_OK) return 1;
    int failed = ow_record_create(db, &record) != OW_OK ||
        ow_field_set(db, record, "answer", &value) != OW_OK ||
        ow_collection_create(db, &collection) != OW_OK ||
        ow_occurrence_insert(db, collection, record, OW_BACK,
                             (OwOccurrenceId){0}, &first) != OW_OK ||
        ow_occurrence_insert(db, collection, record, OW_FRONT,
                             (OwOccurrenceId){0}, &second) != OW_OK;
    ow_database_destroy(db);
    return failed;
}
```

This creates two independent Occurrences of one Record. See [quickstart.c](docs/examples/quickstart.c) for checked ordering and returned-value cleanup, and [lifecycle.c](docs/examples/lifecycle.c) for complete persistent/checkpoint/history/backup use.

In a CMake application set `BUILD_TESTING=OFF`, `add_subdirectory(path/to/OrdWeave ordweave)`, then `target_link_libraries(myapp PRIVATE OrdWeave::ordweave)`. Include only `ordweave.h`; use matching compiler/CRT/packing settings. [Usage](docs/USAGE.zh-CN.md) supplies a complete external CMake project. No installed SDK or cross-toolchain binary ABI is promised.

Persistent paths must be new absolute UTF-8 Windows drive paths with existing parents for create; reopen an existing database with `ow_database_open`. `ow_database_close` consumes a healthy handle. Results are independent copies released with `ow_value_release` / `ow_array_release`. Serialize all access in the caller. On `OW_COMMIT_UNKNOWN`, stop, close and reopen to establish the committed result before deciding whether to retry; output ID zero does not prove nothing committed. See [error contracts](docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md).

## Documentation

| Need | Read |
|---|---|
| Start using the database | [Practical usage](docs/USAGE.zh-CN.md), [complete examples](docs/examples/) |
| Find a public contract | [All 44 functions](docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md), [English API](docs/API.md), [public header](include/ordweave.h) |
| Understand execution | [Implementation walkthrough](docs/IMPLEMENTATION_WALKTHROUGH.zh-CN.md), [architecture summary](docs/ARCHITECTURE.md) |
| Modify or debug | [Private internals](docs/IMPLEMENTATION_INTERNALS.zh-CN.md), [development guide](docs/DEVELOPMENT.zh-CN.md) |
| Reproduce evidence | [Validation map](docs/IMPLEMENTATION_VALIDATION.zh-CN.md), [RC review](docs/RELEASE_CANDIDATE.md) |
| Project workflow or AI continuation | Read [WORKFLOW.md](docs/WORKFLOW.md) first, then [HANDOFF.md](docs/HANDOFF.md); coding agents use [AGENTS.md](AGENTS.md) |
| Review costs and deferred work | [Technical debt register](docs/TECHNICAL_DEBT.md), [six completed phases](docs/ROADMAP.md) |
| Inspect exact disk bytes | [WAL](docs/PERSISTENCE.md), [checkpoints](docs/CHECKPOINTS.md), [history/root/backup](docs/HISTORY.md) |

## Limits and licensing

No SQL, field secondary indexes, network service, multi-writer/concurrent-access guarantees, cross-collection atomic transfer, Linux/macOS, Last Days or incremental backup. Checkpoints can reduce the ordinary result's replay tail, but startup still audits the retained WAL semantically; no whole-open speedup is promised. Index rebuilds, repeated scans, full buffers/copies and provider setup are tracked with evidence in the [debt register](docs/TECHNICAL_DEBT.md). Process-crash tests do not certify physical power-loss durability.

OrdWeave is [MIT licensed](LICENSE). LayerKeySort and adapted Obscura64 provenance and notices are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). No ProbSpan code is included.
