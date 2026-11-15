#include "wal_internal.h"
#include <stdlib.h>
#include <string.h>
OwStatus ow_database_create(OwDatabase **out) {
  return ow_core_database_create(out);
}
static OwStatus open_db(const char *path, int create, OwDatabase **out) {
  OwDatabase *db = NULL;
  OwStore *store = NULL;
  OwStatus s;
  if (!out)
    return OW_INVALID;
  *out = NULL;
  s = ow_core_database_create(&db);
  if (s != OW_OK)
    return s;
  s = ow_store_open(path, create, &store, db);
  if (s != OW_OK) {
    ow_core_database_destroy(db);
    return s;
  }
  ow_core_attach(db, store);
  *out = db;
  return OW_OK;
}
OwStatus ow_database_create_persistent(const char *path, OwDatabase **out) {
  return open_db(path, 1, out);
}
OwStatus ow_database_open(const char *path, OwDatabase **out) {
  return open_db(path, 0, out);
}
OwStatus ow_database_close(OwDatabase *db) {
  OwStore *s = ow_core_store(db);
  OwStatus status = ow_store_guard(s);
  if (status == OW_BUSY)
    return status;
  ow_core_attach(db, NULL);
  ow_core_database_destroy(db);
  ow_store_close(s);
  return status;
}
void ow_database_destroy(OwDatabase *db) { (void)ow_database_close(db); }
OwStatus ow_database_identity(const OwDatabase *db, unsigned char out[16]) {
  return ow_store_identity(ow_core_store(db), out);
}
static OwStatus mutate(OwDatabase *db, OwCommand *cmd) {
  OwStore *store = ow_core_store(db);
  OwStatus s;
  unsigned char *frame = NULL;
  size_t n = 0;
  uint64_t before, after;
  if (!db)
    return OW_INVALID;
  s = ow_store_guard(store);
  if (s != OW_OK)
    return s;
  /* Ordinary in-memory calls preserve their exact Phase 2 allocation behavior.
   */
  if (!store)
    return ow_apply(db, cmd);
  ow_store_enter(store);
  s = ow_encode(cmd, &frame, &n);
  if (s != OW_OK)
    goto done;
  ow_core_revision(db, &before);
  s = ow_apply(db, cmd);
  if (s != OW_OK)
    goto done;
  ow_core_revision(db, &after);
  if (after == before)
    goto done;
  s = ow_store_commit(store, frame, n, cmd, after);
done:
  free(frame);
  ow_store_leave(store);
  return s;
}
OwStatus ow_record_create(OwDatabase *db, OwRecordId *out) {
  OwCommand cmd = {0};
  OwStatus status;
  if (out)
    out->value = 0;
  if (!out)
    return OW_INVALID;
  cmd.op = OP_RECORD_CREATE;
  status = mutate(db, &cmd);
  if (status == OW_OK)
    out->value = cmd.id;
  return status;
}
OwStatus ow_collection_create(OwDatabase *db, OwCollectionId *out) {
  OwCommand cmd = {0};
  OwStatus status;
  if (out)
    out->value = 0;
  if (!out)
    return OW_INVALID;
  cmd.op = OP_COLLECTION_CREATE;
  status = mutate(db, &cmd);
  if (status == OW_OK)
    out->value = cmd.id;
  return status;
}
OwStatus ow_record_delete(OwDatabase *db, OwRecordId id) {
  OwCommand c = {0};
  c.op = OP_RECORD_DELETE;
  c.id = id.value;
  return mutate(db, &c);
}
OwStatus ow_collection_delete(OwDatabase *db, OwCollectionId id) {
  OwCommand c = {0};
  c.op = OP_COLLECTION_DELETE;
  c.id = id.value;
  return mutate(db, &c);
}
OwStatus ow_occurrence_remove(OwDatabase *db, OwOccurrenceId id) {
  OwCommand c = {0};
  c.op = OP_OCCURRENCE_REMOVE;
  c.id = id.value;
  return mutate(db, &c);
}
OwStatus ow_field_set(OwDatabase *db, OwRecordId id, const char *name,
                      const OwValue *v) {
  OwCommand c = {0};
  c.op = OP_FIELD_SET;
  c.id = id.value;
  c.name = name;
  c.value = v;
  return mutate(db, &c);
}
OwStatus ow_field_remove(OwDatabase *db, OwRecordId id, const char *name) {
  OwCommand c = {0};
  c.op = OP_FIELD_REMOVE;
  c.id = id.value;
  c.name = name;
  return mutate(db, &c);
}
OwStatus ow_occurrence_insert(OwDatabase *db, OwCollectionId collection,
                              OwRecordId record, OwPlacement place,
                              OwOccurrenceId anchor, OwOccurrenceId *out) {
  OwCommand c = {0};
  OwStatus s;
  if (out)
    out->value = 0;
  if (!out)
    return OW_INVALID;
  c.op = OP_OCCURRENCE_INSERT;
  c.collection = collection.value;
  c.record = record.value;
  c.place = place;
  c.anchor = anchor.value;
  s = mutate(db, &c);
  if (s == OW_OK)
    out->value = c.id;
  return s;
}
OwStatus ow_occurrence_move(OwDatabase *db, OwOccurrenceId id,
                            OwPlacement place, OwOccurrenceId anchor) {
  OwCommand c = {0};
  c.op = OP_OCCURRENCE_MOVE;
  c.id = id.value;
  c.place = place;
  c.anchor = anchor.value;
  return mutate(db, &c);
}
OwStatus ow_revision(const OwDatabase *db, uint64_t *out) {
  OwStatus s;
  if (out)
    *out = 0;
  s = ow_store_guard(ow_core_store(db));
  if (s != OW_OK)
    return s;
  return ow_core_revision(db, out);
}
OwStatus ow_record_exists(OwDatabase *db, OwRecordId id, int *out) {
  OwStatus s;
  if (out)
    *out = 0;
  s = ow_store_guard(ow_core_store(db));
  if (s != OW_OK)
    return s;
  return ow_core_record_exists(db, id, out);
}
OwStatus ow_field_get(OwDatabase *db, OwRecordId id, const char *name,
                      OwValue *out) {
  OwStatus s;
  if (out)
    memset(out, 0, sizeof(*out));
  s = ow_store_guard(ow_core_store(db));
  if (s != OW_OK)
    return s;
  return ow_core_field_get(db, id, name, out);
}
OwStatus ow_collection_count(OwDatabase *db, OwCollectionId id, size_t *out) {
  OwStatus s;
  if (out)
    *out = 0;
  s = ow_store_guard(ow_core_store(db));
  if (s != OW_OK)
    return s;
  return ow_core_collection_count(db, id, out);
}
OwStatus ow_collection_members(OwDatabase *db, OwCollectionId id,
                               OwOccurrenceId **out, size_t *count) {
  OwStatus s;
  if (out)
    *out = NULL;
  if (count)
    *count = 0;
  s = ow_store_guard(ow_core_store(db));
  if (s != OW_OK)
    return s;
  return ow_core_collection_members(db, id, out, count);
}
OwStatus ow_occurrence_get(OwDatabase *db, OwOccurrenceId id,
                           OwCollectionId *collection, OwRecordId *record) {
  OwStatus s;
  if (collection)
    collection->value = 0;
  if (record)
    record->value = 0;
  s = ow_store_guard(ow_core_store(db));
  if (s != OW_OK)
    return s;
  return ow_core_occurrence_get(db, id, collection, record);
}
OwStatus ow_occurrence_compare(OwDatabase *db, OwOccurrenceId a,
                               OwOccurrenceId b, int *out) {
  OwStatus s;
  if (out)
    *out = 0;
  s = ow_store_guard(ow_core_store(db));
  if (s != OW_OK)
    return s;
  return ow_core_occurrence_compare(db, a, b, out);
}
OwStatus ow_query_equal(OwDatabase *db, const char *name, const OwValue *v,
                        OwRecordId **out, size_t *count) {
  OwStatus s;
  if (out)
    *out = NULL;
  if (count)
    *count = 0;
  s = ow_store_guard(ow_core_store(db));
  if (s != OW_OK)
    return s;
  return ow_core_query_equal(db, name, v, out, count);
}
