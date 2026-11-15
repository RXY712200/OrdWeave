#ifndef OW_CHECKPOINT_INTERNAL_H
#define OW_CHECKPOINT_INTERNAL_H
#include "wal_internal.h"
#define OW_CHECKPOINT_LIMIT ((size_t)268435456)
#define OW_CHECKPOINT_HEADER 144u
#define OW_CHECKPOINT_META 416u
OwStatus ow_core_export(OwDatabase *db, const unsigned char id[16],
                        uint64_t generation, const unsigned char anchor[32],
                        unsigned char **out, size_t *size);
OwStatus ow_core_import(OwDatabase *db, const unsigned char *data, size_t size,
                        const unsigned char id[16]);
void ow_core_exchange(OwDatabase *a, OwDatabase *b);
#ifdef OW_TESTING
extern long ow_test_checkpoint_alloc;
extern uint64_t ow_test_checkpoint_restores, ow_test_checkpoint_tail;
#endif
int ow_checkpoint_oom(void);
#endif
