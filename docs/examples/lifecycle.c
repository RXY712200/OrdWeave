#include "ordweave.h"
#include <stdio.h>
#include <string.h>
/* Usage: lifecycle.exe ABSOLUTE_NEW_DATABASE_PATH ABSOLUTE_NEW_BACKUP_PATH.
 * Parents must already exist. No overwrite, automatic retry or recursive delete. */
int main(int argc, char **argv) {
    OwDatabase *db = NULL, *backup = NULL;
    OwHistory *history = NULL;
    OwRecordId record = {0}, *matches = NULL;
    OwCollectionId collection = {0};
    OwOccurrenceId a = {0}, b = {0}, c = {0}, d = {0}, *members = NULL;
    OwHistoryRange range = {0};
    OwValue v = {0}, result = {0};
    OwStatus status = OW_OK, close_status;
    size_t count = 0;
    uint64_t checkpoint_revision = 0, generation = 0, before = 0, after = 0;
    unsigned char original_id[16], backup_id[16];
    const char text[] = {'a', 0, (char)0xe4, (char)0xb8, (char)0xad};
    const unsigned char binary[] = {0, 0xff, 1};
    if (argc != 3) { fputs("supply two absolute new paths\n", stderr); return 1; }
#define TRY(call) do { status = (call); if (status != OW_OK) goto done; } while (0)
#define CHECK(expr) do { if (!(expr)) { status = OW_INTERNAL; goto done; } } while (0)
    TRY(ow_database_create_persistent(argv[1], &db));
    TRY(ow_database_identity(db, original_id));
    TRY(ow_record_create(db, &record));
    v.type = OW_NULL; TRY(ow_field_set(db, record, "nil", &v));
    v.type = OW_BOOL; v.as.boolean = 1; TRY(ow_field_set(db, record, "enabled", &v));
    v.type = OW_INT; v.as.integer = 42; TRY(ow_field_set(db, record, "counter", &v));
    v.type = OW_FLOAT; v.as.real = 1.25; TRY(ow_field_set(db, record, "ratio", &v));
    v.type = OW_STRING; v.as.bytes.data = text; v.as.bytes.size = sizeof(text);
    TRY(ow_field_set(db, record, "text", &v));
    v.type = OW_BINARY; v.as.bytes.data = binary; v.as.bytes.size = sizeof(binary);
    TRY(ow_field_set(db, record, "bytes", &v));
    TRY(ow_field_get(db, record, "text", &result));
    CHECK(result.as.bytes.size == sizeof(text) && !memcmp(result.as.bytes.data, text, sizeof(text)));
    ow_value_release(&result);
    TRY(ow_collection_create(db, &collection));
    TRY(ow_occurrence_insert(db, collection, record, OW_BACK, (OwOccurrenceId){0}, &a));
    TRY(ow_occurrence_insert(db, collection, record, OW_FRONT, (OwOccurrenceId){0}, &b));
    TRY(ow_occurrence_insert(db, collection, record, OW_BEFORE, a, &c));
    TRY(ow_occurrence_insert(db, collection, record, OW_AFTER, a, &d));
    TRY(ow_collection_members(db, collection, &members, &count));
    CHECK(count == 4 && members[0].value == b.value && members[1].value == c.value &&
          members[2].value == a.value && members[3].value == d.value);
    ow_array_release(members); members = NULL;
    TRY(ow_revision(db, &before));
    TRY(ow_occurrence_move(db, a, OW_BEFORE, a));
    TRY(ow_revision(db, &after)); CHECK(before == after);
    TRY(ow_occurrence_move(db, a, OW_FRONT, (OwOccurrenceId){0}));
    TRY(ow_occurrence_move(db, a, OW_BACK, (OwOccurrenceId){0}));
    TRY(ow_occurrence_move(db, a, OW_BEFORE, b));
    TRY(ow_occurrence_move(db, a, OW_AFTER, c));
    status = ow_record_delete(db, record); CHECK(status == OW_CONFLICT); status = OW_OK;
    status = ow_collection_delete(db, collection); CHECK(status == OW_CONFLICT); status = OW_OK;
    v.type = OW_INT; v.as.integer = 42;
    TRY(ow_query_equal(db, "counter", &v, &matches, &count));
    CHECK(count == 1 && matches[0].value == record.value);
    ow_array_release(matches); matches = NULL;
    /* Only present entries and copied variable key bytes are budgeted here;
     * fixed rows and LKS allocations are outside max_key_bytes. */
    TRY(ow_field_index_enable(db, "counter", 16, 1024));
    TRY(ow_query_range(db, "counter", OW_INT, &v, 1, &v, 1, &matches, &count));
    CHECK(count == 1 && matches[0].value == record.value);
    TRY(ow_checkpoint_create(db));
    TRY(ow_checkpoint_info(db, &generation, &checkpoint_revision)); CHECK(generation != 0);
    v.as.integer = 43; TRY(ow_field_set(db, record, "counter", &v));
    CHECK(count == 1 && matches[0].value == record.value); /* Independent copy. */
    ow_array_release(matches); matches = NULL;
    TRY(ow_history_open(db, checkpoint_revision, &history));
    TRY(ow_history_field_get(history, record, "counter", &result)); CHECK(result.as.integer == 42);
    TRY(ow_history_query_range(history, "counter", OW_INT, NULL, 0, NULL, 1,
                               &matches, &count));
    ow_history_close(history); history = NULL;
    CHECK(count == 1 && matches[0].value == record.value);
    ow_array_release(matches); matches = NULL;
    TRY(ow_revision(db, &before));
    TRY(ow_field_index_disable(db));
    TRY(ow_revision(db, &after)); CHECK(before == after);
    CHECK(result.as.integer == 42); ow_value_release(&result);
    TRY(ow_history_policy(db, OW_RETAIN_MANUAL, 0));
    TRY(ow_history_policy(db, OW_RETAIN_FOREVER, 0));
    TRY(ow_history_policy(db, OW_RETAIN_LAST_N, 3));
    TRY(ow_history_cleanup(db, &range)); /* A small database need not have a reclaimable segment. */
    TRY(ow_verify_database(db, &range));
    printf("retained %llu..%llu\n", (unsigned long long)range.earliest_revision,
           (unsigned long long)range.current_revision);
    TRY(ow_backup_create(db, argv[2]));
    close_status = ow_database_close(db); db = NULL;
    if (close_status != OW_OK) { status = close_status; goto done; }
    TRY(ow_database_open(argv[1], &db));
    TRY(ow_field_get(db, record, "counter", &result)); CHECK(result.as.integer == 43);
    ow_value_release(&result);
    TRY(ow_database_open(argv[2], &backup));
    TRY(ow_database_identity(backup, backup_id)); CHECK(!memcmp(original_id, backup_id, 16));
    TRY(ow_field_get(backup, record, "counter", &result)); CHECK(result.as.integer == 43);
    ow_value_release(&result);
    /* Remove each membership before deleting the shared Record or Collection. */
    TRY(ow_occurrence_remove(db, a)); TRY(ow_occurrence_remove(db, b));
    TRY(ow_occurrence_remove(db, c)); TRY(ow_occurrence_remove(db, d));
    TRY(ow_collection_delete(db, collection)); TRY(ow_record_delete(db, record));
    puts("checkpoint, history, backup and independent reopen verified");
done:
    if (status == OW_COMMIT_UNKNOWN || status == OW_RECOVERY_REQUIRED)
        fputs("Stop; close and reopen for validation. Do not blindly retry a mutation.\n", stderr);
    ow_value_release(&result); ow_array_release(matches); ow_array_release(members);
    ow_history_close(history);
    close_status = ow_database_close(backup);
    if (status == OW_OK && close_status != OW_OK) status = close_status;
    close_status = ow_database_close(db);
    if (status == OW_OK && close_status != OW_OK) status = close_status;
    if (status != OW_OK) fprintf(stderr, "OwStatus=%d\n", (int)status);
    return (int)status;
#undef TRY
#undef CHECK
}
