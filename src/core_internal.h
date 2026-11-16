#ifndef OW_CORE_INTERNAL_H
#define OW_CORE_INTERNAL_H
#include "ordweave.h"
typedef struct OwStore OwStore;
OwStatus ow_core_database_create(OwDatabase **out);
void ow_core_database_destroy(OwDatabase *db);
OwStatus ow_core_revision(const OwDatabase *db, uint64_t *out);
OwStatus ow_core_record_create(OwDatabase *db, OwRecordId *out);
OwStatus ow_core_record_delete(OwDatabase *db, OwRecordId id);
OwStatus ow_core_record_exists(OwDatabase *db, OwRecordId id, int *out);
OwStatus ow_core_field_set(OwDatabase *db, OwRecordId id, const char *name,
                           const OwValue *value);
OwStatus ow_core_field_get(OwDatabase *db, OwRecordId id, const char *name,
                           OwValue *out);
OwStatus ow_core_field_remove(OwDatabase *db, OwRecordId id, const char *name);
OwStatus ow_core_collection_create(OwDatabase *db, OwCollectionId *out);
OwStatus ow_core_collection_delete(OwDatabase *db, OwCollectionId id);
OwStatus ow_core_collection_count(OwDatabase *db, OwCollectionId id,
                                  size_t *out);
OwStatus ow_core_collection_members(OwDatabase *db, OwCollectionId id,
                                    OwOccurrenceId **out, size_t *count);
OwStatus ow_core_occurrence_insert(OwDatabase *db, OwCollectionId collection,
                                   OwRecordId record, OwPlacement placement,
                                   OwOccurrenceId anchor, OwOccurrenceId *out);
OwStatus ow_core_occurrence_move(OwDatabase *db, OwOccurrenceId id,
                                 OwPlacement placement, OwOccurrenceId anchor);
OwStatus ow_core_occurrence_remove(OwDatabase *db, OwOccurrenceId id);
OwStatus ow_core_occurrence_get(OwDatabase *db, OwOccurrenceId id,
                                OwCollectionId *collection, OwRecordId *record);
OwStatus ow_core_occurrence_compare(OwDatabase *db, OwOccurrenceId left,
                                    OwOccurrenceId right, int *out);
OwStatus ow_core_query_equal(OwDatabase *db, const char *name,
                             const OwValue *value, OwRecordId **out,
                             size_t *count);
int ow_core_same_state(const OwDatabase *a,const OwDatabase *b);
OwStore *ow_core_store(const OwDatabase *db);
void ow_core_attach(OwDatabase *db, OwStore *store);
OwStatus ow_core_validate(const char *name, const OwValue *value);
#endif
