#ifndef OW_WAL_INTERNAL_H
#define OW_WAL_INTERNAL_H
#include "core_internal.h"
#include <limits.h>
_Static_assert(CHAR_BIT == 8, "8-bit bytes required");
#define OW_FRAME_LIMIT ((size_t)16777216)
#define OW_SEGMENT_TARGET UINT64_C(67108864)
#define OW_FRAME_HEADER 96u
#define OW_FRAME_OVERHEAD 136u
#define OW_SEGMENT_HEADER 128u
#define OW_MANIFEST_SIZE 96u
typedef enum {
  OP_RECORD_CREATE = 1,
  OP_RECORD_DELETE,
  OP_FIELD_SET,
  OP_FIELD_REMOVE,
  OP_COLLECTION_CREATE,
  OP_COLLECTION_DELETE,
  OP_OCCURRENCE_INSERT,
  OP_OCCURRENCE_MOVE,
  OP_OCCURRENCE_REMOVE
} OwOpcode;
typedef struct {
  OwOpcode op;
  uint64_t id, collection, record, anchor;
  OwPlacement place;
  const char *name;
  const OwValue *value;
} OwCommand;
void ow_put(unsigned char *p, uint64_t v, size_t n);
uint64_t ow_get(const unsigned char *p, size_t n);
OwStatus ow_sha256(const void *a, size_t an, const void *b, size_t bn,
                   unsigned char out[32]);
OwStatus ow_encode(const OwCommand *cmd, unsigned char **frame, size_t *size);
OwStatus ow_decode(const unsigned char *payload, size_t size, OwOpcode op,
                   OwCommand *cmd, char **name, OwValue *value);
OwStatus ow_apply(OwDatabase *db, OwCommand *cmd);
OwStatus ow_store_open(const char *path, int create, OwStore **out,
                       OwDatabase *db);
void ow_store_close(OwStore *store);
OwStatus ow_store_guard(const OwStore *store);
OwStatus ow_store_commit(OwStore *store, unsigned char *frame, size_t size,
                         OwCommand *cmd, uint64_t revision);
OwStatus ow_store_identity(const OwStore *store, unsigned char out[16]);
void ow_store_enter(OwStore *store);
void ow_store_leave(OwStore *store);
#ifdef OW_TESTING
/* Test-only boundaries, per test executable; absent from production. */
extern int ow_test_fault;
extern int ow_test_crash;
extern uint64_t ow_test_segment_target;
#define OW_FAULT(n) (ow_test_fault == (n))
#else
#define OW_FAULT(n) 0
#endif
#endif
