#include "ordweave.h"
#include <stdio.h>
int main(void) {
    OwDatabase *db = NULL;
    OwRecordId record = {0};
    OwCollectionId collection = {0};
    OwOccurrenceId first = {0}, second = {0};
    OwOccurrenceId *members = NULL;
    size_t count = 0;
    OwValue value = {.type = OW_INT, .as.integer = 42}, result = {0};
    OwStatus status = ow_database_create(&db);
    if (status != OW_OK) return (int)status;
#define TRY(call) do { status = (call); if (status != OW_OK) goto done; } while (0)
    TRY(ow_record_create(db, &record));
    TRY(ow_field_set(db, record, "answer", &value));
    TRY(ow_collection_create(db, &collection));
    TRY(ow_occurrence_insert(db, collection, record, OW_BACK, (OwOccurrenceId){0}, &first));
    TRY(ow_occurrence_insert(db, collection, record, OW_FRONT, (OwOccurrenceId){0}, &second));
    TRY(ow_collection_members(db, collection, &members, &count));
    if (count != 2 || members[0].value != second.value || members[1].value != first.value) {
        status = OW_INTERNAL; goto done;
    }
    TRY(ow_field_get(db, record, "answer", &result));
    if (result.type != OW_INT || result.as.integer != 42) status = OW_INTERNAL;
    if (status == OW_OK) puts("one Record, two independent ordered Occurrences, answer=42");
done:
    ow_value_release(&result);
    ow_array_release(members);
    ow_database_destroy(db);
    return (int)status;
#undef TRY
}
