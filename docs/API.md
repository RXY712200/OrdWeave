# Phase 2 public API

Include `ordweave.h`; link `OrdWeave::ordweave`. The header is the authoritative declaration source. The existing `const char *ordweave_version(void)` remains compatible and returns library-owned static storage.

IDs are distinct structs (`OwRecordId`, `OwCollectionId`, `OwOccurrenceId`), each wrapping `uint64_t value`. Zero is invalid. Separate monotone counters allocate IDs starting at one; successful IDs are never reused within one database lifetime. IDs are scoped to that database and cannot validate provenance from another database.

`OwStatus`: `OW_OK`, `OW_INVALID`, `OW_NOT_FOUND`, `OW_CONFLICT`, `OW_CAPACITY`, `OW_NOMEM`, `OW_INTERNAL`. Required outputs are cleared before validation, including on failure. All required output pointer arguments must point to distinct valid storage. Input pointers must reference readable storage for their documented lengths; outputs must not alias inputs. Existing allocated output results must be released before reusing their variables.

## Values

`OwValue` has `type` (`OW_NULL`, `OW_BOOL`, `OW_INT`, `OW_FLOAT`, `OW_STRING`, `OW_BINARY`) and union `as`: `boolean` (0/1), `integer` (int64_t), `real` (finite binary64), or `bytes` (`const void *data`, `size_t size`). Payload bytes may be NULL only at size zero. Strings are length-delimited valid UTF-8 and may contain NUL. Returned strings have no terminator guarantee. Field names are nonempty NUL-terminated valid UTF-8, case-sensitive, byte-exact, without normalization. Names cannot contain embedded NUL. Float equality treats signed zeros as equal. Types are compared strictly; no numeric coercion.

Inputs are borrowed only for the call. Stored values are deep copied. `ow_field_get` returns an immutable independent deep copy, released only with `ow_value_release`; arrays likewise use `ow_array_release`. Results survive all later database mutations and destruction. Release functions accept NULL; releasing a value resets it to null. Do not release a borrowed input value with `ow_value_release`. Never free OrdWeave results through another allocator.

## Operations

```c
OwStatus ow_database_create(OwDatabase **out);
void ow_database_destroy(OwDatabase *db);
OwStatus ow_revision(const OwDatabase *db, uint64_t *out);
OwStatus ow_record_create(OwDatabase *db, OwRecordId *out);
OwStatus ow_record_delete(OwDatabase *db, OwRecordId id);
OwStatus ow_record_exists(OwDatabase *db, OwRecordId id, int *out);
OwStatus ow_field_set(OwDatabase *db, OwRecordId id, const char *name, const OwValue *value);
OwStatus ow_field_get(OwDatabase *db, OwRecordId id, const char *name, OwValue *out);
OwStatus ow_field_remove(OwDatabase *db, OwRecordId id, const char *name);
void ow_value_release(OwValue *value);
OwStatus ow_collection_create(OwDatabase *db, OwCollectionId *out);
OwStatus ow_collection_delete(OwDatabase *db, OwCollectionId id);
OwStatus ow_collection_count(OwDatabase *db, OwCollectionId id, size_t *out);
OwStatus ow_collection_members(OwDatabase *db, OwCollectionId id, OwOccurrenceId **out, size_t *count);
OwStatus ow_occurrence_insert(OwDatabase *db, OwCollectionId collection, OwRecordId record,
    OwPlacement placement, OwOccurrenceId anchor, OwOccurrenceId *out);
OwStatus ow_occurrence_move(OwDatabase *db, OwOccurrenceId id, OwPlacement placement, OwOccurrenceId anchor);
OwStatus ow_occurrence_remove(OwDatabase *db, OwOccurrenceId id);
OwStatus ow_occurrence_get(OwDatabase *db, OwOccurrenceId id, OwCollectionId *collection, OwRecordId *record);
OwStatus ow_occurrence_compare(OwDatabase *db, OwOccurrenceId left, OwOccurrenceId right, int *out);
OwStatus ow_query_equal(OwDatabase *db, const char *name, const OwValue *value, OwRecordId **out, size_t *count);
/* Arrays are immutable independent copies. Only this function may release them. */
void ow_array_release(void *array);
```

Creation returns a caller-owned opaque database; destruction releases everything and accepts NULL. `ow_revision` reads its logical Revision. Successful state-changing mutations increment it once, never on failures, reads, or genuine no-ops. Capacity prevents ID/Revision wrap. There is no transaction or history implied by Revision.

Record existence returns OK with false for a missing nonzero ID. Other operations return NOT_FOUND for missing objects; missing fields on get return NOT_FOUND. Removing an absent field on a live Record is an OK no-op. Equal field overwrite is an OK no-op. Referenced Record deletion or nonempty Collection deletion returns CONFLICT.

Collection count also serves Collection lookup. Members returns occurrence IDs in current order (empty is NULL/zero). Occurrence get returns its immutable Collection and Record IDs. Compare returns negative/zero/positive for same-Collection order; foreign collections are INVALID. Occurrence lookup is NOT_FOUND after removal.

Placements are FRONT, BACK, BEFORE, AFTER. FRONT/BACK require anchor ID zero. BEFORE/AFTER require a live anchor in the target Collection; a missing anchor is NOT_FOUND and foreign Collection is INVALID. An occurrence never changes its associated Record or Collection. Moving relative to itself, to its existing endpoint, or to its already-satisfied adjacency is an OK no-op. Each duplicate Record membership has an independent ID.

Equality queries scan Records and return matching Record IDs in increasing ID order. Missing fields do not equal null. Empty results are NULL/zero. Queries never change Revision.

All failures preserve observable database contents, membership, ordering, IDs, Revision, and index consistency. Caller serialization is mandatory, including reads versus writes/destruction. No internal locking or cross-collection atomic transfer is promised. Limits and logical Record budget are specified in ARCHITECTURE.md.
