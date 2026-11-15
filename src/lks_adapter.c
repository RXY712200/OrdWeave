#include "lks_adapter.h"
static int compare(const void *a, const void *b, void *ctx) {
  const OwIndexed *x = a, *y = b;
  (void)ctx;
  return (x->id > y->id) - (x->id < y->id);
}
LksManagedOrder *ow_index_create(void) {
  LksComparator c = {compare, NULL};
  return lks_managed_order_create(&c);
}
OwStatus ow_lks_status(LksStatus s) {
  switch (s) {
  case LKS_STATUS_OK:
    return OW_OK;
  case LKS_STATUS_OUT_OF_MEMORY:
    return OW_NOMEM;
  case LKS_STATUS_CAPACITY_LIMIT:
    return OW_CAPACITY;
  case LKS_STATUS_NOT_FOUND:
    return OW_NOT_FOUND;
  case LKS_STATUS_INVALID_ARGUMENT:
    return OW_INVALID;
  case LKS_STATUS_ALREADY_EXISTS:
    return OW_CONFLICT;
  default:
    return OW_INTERNAL;
  }
}
void *ow_index_find(LksManagedOrder *index, uint64_t id) {
  OwIndexed key = {0};
  key.id = id;
  const LksOrderHandle *pred = NULL, *equal = NULL, *succ = NULL;
  if (!id || lks_managed_order_locate(index, &key, &pred, &equal, &succ) !=
                 LKS_STATUS_OK)
    return NULL;
  return equal ? lks_order_item(equal) : NULL;
}
OwStatus ow_index_insert(LksManagedOrder *index, OwIndexed *item) {
  if (ow_index_find(index, item->id))
    return OW_CONFLICT;
  return ow_lks_status(
      lks_managed_order_insert(index, item, &item->index_handle));
}
OwStatus ow_index_remove(LksManagedOrder *index, OwIndexed *item) {
  OwStatus s =
      ow_lks_status(lks_managed_order_remove(index, item->index_handle, NULL));
  if (s == OW_OK)
    item->index_handle = NULL;
  return s;
}
OwStatus ow_order_insert(LksOrder *o, void *item, OwPlacement p,
                         const LksOrderHandle *a, const LksOrderHandle **out) {
  switch (p) {
  case OW_FRONT:
    return ow_lks_status(lks_order_insert_front(o, item, out));
  case OW_BACK:
    return ow_lks_status(lks_order_insert_back(o, item, out));
  case OW_BEFORE:
    return ow_lks_status(lks_order_insert_before(o, a, item, out));
  case OW_AFTER:
    return ow_lks_status(lks_order_insert_after(o, a, item, out));
  default:
    return OW_INVALID;
  }
}
OwStatus ow_order_move(LksOrder *o, const LksOrderHandle *h, OwPlacement p,
                       const LksOrderHandle *a) {
  switch (p) {
  case OW_FRONT:
    return ow_lks_status(lks_order_move_front(o, h));
  case OW_BACK:
    return ow_lks_status(lks_order_move_back(o, h));
  case OW_BEFORE:
    return ow_lks_status(lks_order_move_before(o, h, a));
  case OW_AFTER:
    return ow_lks_status(lks_order_move_after(o, h, a));
  default:
    return OW_INVALID;
  }
}
