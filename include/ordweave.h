#ifndef ORDWEAVE_H
#define ORDWEAVE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
const char *ordweave_version(void);
typedef struct OwDatabase OwDatabase;
typedef struct {
  uint64_t value;
} OwRecordId;
typedef struct {
  uint64_t value;
} OwCollectionId;
typedef struct {
  uint64_t value;
} OwOccurrenceId;
typedef enum {
  OW_OK,
  OW_INVALID,
  OW_NOT_FOUND,
  OW_CONFLICT,
  OW_CAPACITY,
  OW_NOMEM,
  OW_INTERNAL,
  OW_IO,
  OW_CORRUPT,
  OW_UNSUPPORTED,
  OW_BUSY,
  OW_COMMIT_UNKNOWN,
  OW_RECOVERY_REQUIRED, OW_HISTORY_UNAVAILABLE, OW_RETENTION_UNSUPPORTED
} OwStatus;
typedef enum {
  OW_NULL,
  OW_BOOL,
  OW_INT,
  OW_FLOAT,
  OW_STRING,
  OW_BINARY
} OwType;
/* Strings are length-delimited UTF-8 (embedded NUL is valid); binary is
 * arbitrary. Returned buffers are immutable copies, owned until
 * ow_value_release. */
typedef struct {
  OwType type;
  union {
    int boolean;
    int64_t integer;
    double real;
    struct {
      const void *data;
      size_t size;
    } bytes;
  } as;
} OwValue;
typedef enum { OW_FRONT, OW_BACK, OW_BEFORE, OW_AFTER } OwPlacement;
#define OW_FIELD_LIMIT ((size_t)1048576)
#define OW_RECORD_LIMIT ((size_t)8388608)
OwStatus ow_database_create(OwDatabase **out);
void ow_database_destroy(OwDatabase *db);
/* Absolute UTF-8 drive paths on Windows; create requires a nonexistent
 * directory. */
OwStatus ow_database_create_persistent(const char *path, OwDatabase **out);
OwStatus ow_database_open(const char *path, OwDatabase **out);
OwStatus ow_database_close(OwDatabase *db);
typedef struct OwHistory OwHistory;
typedef enum { OW_RETAIN_MANUAL, OW_RETAIN_FOREVER, OW_RETAIN_LAST_N } OwRetentionPolicy;
typedef struct { uint64_t current_revision, earliest_revision; OwRetentionPolicy policy; uint64_t last_n; int reclaimable; } OwHistoryRange;
OwStatus ow_history_open(OwDatabase *db, uint64_t revision, OwHistory **out);
void ow_history_close(OwHistory *view);
OwStatus ow_history_revision(const OwHistory *view, uint64_t *out);
OwStatus ow_history_record_exists(const OwHistory *view, OwRecordId id, int *out);
OwStatus ow_history_field_get(const OwHistory *view, OwRecordId id, const char *name, OwValue *out);
OwStatus ow_history_collection_count(const OwHistory *view, OwCollectionId id, size_t *out);
OwStatus ow_history_collection_members(const OwHistory *view, OwCollectionId id, OwOccurrenceId **out, size_t *count);
OwStatus ow_history_occurrence_get(const OwHistory *view, OwOccurrenceId id, OwCollectionId *collection, OwRecordId *record);
OwStatus ow_history_occurrence_compare(const OwHistory *view, OwOccurrenceId a, OwOccurrenceId b, int *out);
OwStatus ow_history_query_equal(const OwHistory *view, const char *name, const OwValue *value, OwRecordId **out, size_t *count);
OwStatus ow_history_policy(OwDatabase *db, OwRetentionPolicy policy, uint64_t last_n);
OwStatus ow_history_range(OwDatabase *db, OwHistoryRange *out);
OwStatus ow_history_cleanup(OwDatabase *db, OwHistoryRange *out);
OwStatus ow_backup_create(OwDatabase *db, const char *destination);
OwStatus ow_verify_database(OwDatabase *db, OwHistoryRange *out);
OwStatus ow_checkpoint_create(OwDatabase *db);
OwStatus ow_checkpoint_info(const OwDatabase *db, uint64_t *generation, uint64_t *revision);
OwStatus ow_checkpoint_retention(OwDatabase *db, unsigned generations);
OwStatus ow_database_identity(const OwDatabase *db, unsigned char out[16]);
OwStatus ow_revision(const OwDatabase *db, uint64_t *out);
OwStatus ow_record_create(OwDatabase *db, OwRecordId *out);
OwStatus ow_record_delete(OwDatabase *db, OwRecordId id);
OwStatus ow_record_exists(OwDatabase *db, OwRecordId id, int *out);
OwStatus ow_field_set(OwDatabase *db, OwRecordId id, const char *name,
                      const OwValue *value);
OwStatus ow_field_get(OwDatabase *db, OwRecordId id, const char *name,
                      OwValue *out);
OwStatus ow_field_remove(OwDatabase *db, OwRecordId id, const char *name);
void ow_value_release(OwValue *value);
OwStatus ow_collection_create(OwDatabase *db, OwCollectionId *out);
OwStatus ow_collection_delete(OwDatabase *db, OwCollectionId id);
OwStatus ow_collection_count(OwDatabase *db, OwCollectionId id, size_t *out);
OwStatus ow_collection_members(OwDatabase *db, OwCollectionId id,
                               OwOccurrenceId **out, size_t *count);
OwStatus ow_occurrence_insert(OwDatabase *db, OwCollectionId collection,
                              OwRecordId record, OwPlacement placement,
                              OwOccurrenceId anchor, OwOccurrenceId *out);
OwStatus ow_occurrence_move(OwDatabase *db, OwOccurrenceId id,
                            OwPlacement placement, OwOccurrenceId anchor);
OwStatus ow_occurrence_remove(OwDatabase *db, OwOccurrenceId id);
OwStatus ow_occurrence_get(OwDatabase *db, OwOccurrenceId id,
                           OwCollectionId *collection, OwRecordId *record);
OwStatus ow_occurrence_compare(OwDatabase *db, OwOccurrenceId left,
                               OwOccurrenceId right, int *out);
OwStatus ow_query_equal(OwDatabase *db, const char *name, const OwValue *value,
                        OwRecordId **out, size_t *count);
/* Arrays are immutable independent copies. Only this function may release them.
 */
void ow_array_release(void *array);
/* Session-local advisory index. Positive limits bound present-field entries
 * and copied field-name (including NUL) + String/Binary payload bytes only.
 * Fixed metadata/LKS storage is outside the byte budget. Failed enable keeps
 * the prior index. Maintenance failure discards index AND configuration;
 * no automatic rebuild. No data Revision, IDs, or persisted bytes change.
 * Range returns independent IDs sorted by value then ID; use ow_array_release.
 * Bounds must match type; NULL is unbounded (not an OW_NULL value). Inclusivity
 * flags must always be 0/1. Missing excluded; typed Null included, no coercion,
 * finite numeric/unsigned byte-span order. Invalid/inverted bounds rejected.
 * Historical range scans/sorts its own immutable core, never the live cache. */
OwStatus ow_field_index_enable(OwDatabase *db, const char *field,
                              size_t max_records, size_t max_key_bytes);
OwStatus ow_field_index_disable(OwDatabase *db);
OwStatus ow_query_range(OwDatabase *db, const char *field, OwType type,
                       const OwValue *lower, int lower_inclusive,
                       const OwValue *upper, int upper_inclusive,
                       OwRecordId **out, size_t *count);
OwStatus ow_history_query_range(const OwHistory *view, const char *field,
                       OwType type, const OwValue *lower, int lower_inclusive,
                       const OwValue *upper, int upper_inclusive,
                       OwRecordId **out, size_t *count);
#ifdef __cplusplus
}
#endif
#endif
