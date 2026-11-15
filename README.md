# OrdWeave

OrdWeave is a C17 embedded database project built around independently maintained LayerKeySort Full. Phase 4 provides a usable in-memory database: typed record fields, collections, repeated occurrences, dynamic ordering, ID lookup, equality queries, and a logical Revision counter. The Phase 1 ordering/snapshot integration test remains intact.

The intended first stable release targets Windows x64, single-writer operation, persistent records and ordering, WAL recovery, and up to three retained checkpoint generations. Single-operation durable WAL commits and restart recovery are implemented on Windows x64. Native LKS checkpoints and checkpoint-plus-WAL-tail recovery are implemented; historical queries and backup recovery remain planned. A small internal LKS adapter now supports runtime ordering and ID indexes. Linux and macOS have not been validated.

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

Limitations: no multi-operation transactions, historical Revision queries, SQL, secondary field indexes, complex cross-collection atomic transfers, or concurrency guarantees. Occurrence insertion/removal currently rebuilds the global occurrence index for failure atomicity; see architecture notes.

## Persistent lifecycle (Windows x64)

Use an explicitly new absolute UTF-8 path, for example `D:/data/my-new-ordweave-db` (its parent must exist). Replace `ow_database_create` in the example with `ow_database_create_persistent(path, &db)`. Mutate through the same API, then call `ow_database_close(db)`. Reopen using `ow_database_open(path, &db)`; query and modify normally. Close never supplies missing durability: each acknowledged state-changing operation was already appended and flushed.

On OW_COMMIT_UNKNOWN, stop using that session, close it and reopen to establish the committed result before deciding whether to retry. See [exact persistence format and recovery rules](docs/PERSISTENCE.md). Python 3 is required for the independent format CTest oracle when BUILD_TESTING is enabled.

## Checkpoints

`ow_checkpoint_create(db)` publishes a validated checkpoint without changing Revision. `ow_checkpoint_info` reports its generation and Revision. `ow_checkpoint_retention(db, count)` accepts 1..3; default 2. Existing v3 databases need no migration and may create their first checkpoint directly. Startup still validates the whole WAL and restores native LKS orders before applying the tail. See [checkpoint format, failure handling and example](docs/CHECKPOINTS.md). No historical WAL cleanup or backup feature is implemented.
