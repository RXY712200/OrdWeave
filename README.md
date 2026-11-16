# OrdWeave

OrdWeave **1.0.0-rc.1** is a C17 embedded database for Windows x64, built around independently maintained LayerKeySort Full. It supports typed Records, Collections with independently ordered duplicate Occurrences, ID lookup, equality queries, durable single-operation WAL commits, native LKS checkpoints, immutable historical views, explicit anchored cleanup, full backups and persisted-data verification.

This is an unreleased candidate for v1.0.0 Stable. Linux/macOS and concurrent writers are unsupported. See [candidate validation and limits](docs/RELEASE_CANDIDATE.md) and [changelog](CHANGELOG.md).

## Build

Requires Git, CMake 3.21+, and a C17 compiler. For Visual Studio on Windows:

```powershell
git clone --recurse-submodules https://github.com/RXY712200/OrdWeave.git
cd OrdWeave
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For an existing clone, run `git submodule update --init --recursive`. With GCC and Ninja, configure using `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` instead.

LayerKeySort Full v4.1.0 Stable is pinned at `70d7f0ef5857b54381b863d292a213909d87459f` under `third_party/LayerKeySort`. Only its public API is used. The smoke test keeps serialized snapshots in memory; it writes no database files.

See [roadmap](docs/ROADMAP.md), [architecture](docs/ARCHITECTURE.md), and [third-party notices](THIRD_PARTY_NOTICES.md). OrdWeave is MIT licensed.

## Public API and ownership

See [API reference](docs/API.md) for exact signatures and error/lifetime rules. All access to a database must be serialized by its caller. Results are independent copies and remain valid after mutation or database destruction.

```c
#include "ordweave.h"
int main(void) {
    OwDatabase *db = NULL;
    OwRecordId record;
    OwCollectionId collection;
    OwOccurrenceId occurrence;
    OwValue value = {.type = OW_INT, .as.integer = 42};
    if (ow_database_create(&db) != OW_OK) return 1;
    if (ow_record_create(db, &record) != OW_OK ||
        ow_field_set(db, record, "answer", &value) != OW_OK ||
        ow_collection_create(db, &collection) != OW_OK ||
        ow_occurrence_insert(db, collection, record, OW_BACK,
                             (OwOccurrenceId){0}, &occurrence) != OW_OK) {
        ow_database_destroy(db);
        return 1;
    }
    ow_database_destroy(db);
    return 0;
}
```

Limitations: no multi-operation transactions, SQL, secondary field indexes, complex cross-collection atomic transfers, or concurrency guarantees. Occurrence insertion/removal currently rebuilds the global occurrence index for failure atomicity; see architecture notes.

## Persistent lifecycle (Windows x64)

Use an explicitly new absolute UTF-8 path, for example `D:/data/my-new-ordweave-db` (its parent must exist). Replace `ow_database_create` in the example with `ow_database_create_persistent(path, &db)`. Mutate through the same API, then call `ow_database_close(db)`. Reopen using `ow_database_open(path, &db)`; query and modify normally. Close never supplies missing durability: each acknowledged state-changing operation was already appended and flushed.

On OW_COMMIT_UNKNOWN, stop using that session, close it and reopen to establish the committed result before deciding whether to retry. See [exact persistence format and recovery rules](docs/PERSISTENCE.md). Python 3 is required for the independent format CTest oracle when BUILD_TESTING is enabled.

## Checkpoints

`ow_checkpoint_create(db)` publishes a validated checkpoint without changing Revision. `ow_checkpoint_info` reports its generation and Revision. `ow_checkpoint_retention(db, count)` accepts 1..3; default 2. Existing v3 databases need no migration and may create their first checkpoint directly. Startup still validates the whole WAL and restores native LKS orders before applying the tail. See [checkpoint format, failure handling and example](docs/CHECKPOINTS.md). Historical WAL cleanup and full backups are implemented separately in Phase 5.

## Historical views, cleanup, backup and verification

Phase 5 adds opaque read-only `ow_history_*` views, Manual/Forever/Last N policies, explicit safe WAL-prefix cleanup, full `ow_backup_create`, and persisted-data `ow_verify_database`. Manual is the default; setting a policy does not delete files. Cleanup publishes a verified native-checkpoint recovery root before deleting eligible complete segments. Last Days is deferred because v3 commits have no timestamps. See [history/root format and failure rules](docs/HISTORY.md).

## Link from a C17 application

Set BUILD_TESTING=OFF when embedding, use `add_subdirectory(path/to/OrdWeave ordweave)`, and link `OrdWeave::ordweave`. Include only `ordweave.h`. Use a matching Windows x64 compiler/CRT configuration; no cross-toolchain binary ABI is promised. The public example below and tests/consumer demonstrate both database lifecycles. No Stable tag or Release has been published.
