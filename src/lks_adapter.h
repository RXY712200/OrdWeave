#ifndef OW_LKS_ADAPTER_H
#define OW_LKS_ADAPTER_H
#include "layerkeysort.h"
#include "ordweave.h"
typedef struct OwIndexed {
  uint64_t id;
  const LksOrderHandle *index_handle;
  struct OwIndexed *teardown_next;
} OwIndexed;
LksManagedOrder *ow_index_create(void);
void *ow_index_find(LksManagedOrder *index, uint64_t id);
OwStatus ow_index_insert(LksManagedOrder *index, OwIndexed *item);
OwStatus ow_index_remove(LksManagedOrder *index, OwIndexed *item);
OwStatus ow_lks_status(LksStatus status);
OwStatus ow_order_insert(LksOrder *order, void *item, OwPlacement place,
                         const LksOrderHandle *anchor,
                         const LksOrderHandle **out);
OwStatus ow_order_move(LksOrder *order, const LksOrderHandle *item,
                       OwPlacement place, const LksOrderHandle *anchor);
#endif
