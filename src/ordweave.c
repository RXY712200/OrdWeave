#include "ordweave.h"
#include "lks_adapter.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(double) == 8 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024,
               "binary64 required");
typedef struct Field {
  char *name;
  OwValue value;
  struct Field *next;
} Field;
typedef struct {
  OwIndexed key;
  Field *fields;
  size_t budget, refs;
} Record;
typedef struct {
  OwIndexed key;
  LksOrder *order;
} Collection;
typedef struct {
  OwIndexed key;
  Record *record;
  Collection *collection;
  const LksOrderHandle *handle;
} Occurrence;
struct OwDatabase {
  LksManagedOrder *records, *collections, *occurrences;
  uint64_t rid, cid, oid, revision;
};
const char *ordweave_version(void) { return "0.1.0-dev"; }
static int utf8(const unsigned char *s, size_t n) {
  size_t i = 0;
  while (i < n) {
    unsigned c = s[i++], cp, min;
    size_t k;
    if (c < 128)
      continue;
    if (c >= 0xc2 && c <= 0xdf) {
      cp = c & 31;
      k = 1;
      min = 128;
    } else if (c >= 0xe0 && c <= 0xef) {
      cp = c & 15;
      k = 2;
      min = 2048;
    } else if (c >= 0xf0 && c <= 0xf4) {
      cp = c & 7;
      k = 3;
      min = 65536;
    } else
      return 0;
    if (k > n - i)
      return 0;
    while (k--) {
      c = s[i++];
      if ((c & 0xc0) != 0x80)
        return 0;
      cp = (cp << 6) | (c & 63);
    }
    if (cp < min || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
      return 0;
  }
  return 1;
}
static OwStatus name_valid(const char *name) {
  size_t n;
  if (!name || !*name)
    return OW_INVALID;
  n = strlen(name);
  if (n > OW_FIELD_LIMIT)
    return OW_CAPACITY;
  return utf8((const unsigned char *)name, n) ? OW_OK : OW_INVALID;
}
static OwStatus value_valid(const OwValue *v) {
  if (!v || v->type < OW_NULL || v->type > OW_BINARY)
    return OW_INVALID;
  if (v->type == OW_BOOL && v->as.boolean != 0 && v->as.boolean != 1)
    return OW_INVALID;
  if (v->type == OW_FLOAT && !isfinite(v->as.real))
    return OW_INVALID;
  if (v->type == OW_STRING || v->type == OW_BINARY) {
    if (v->as.bytes.size > OW_FIELD_LIMIT)
      return OW_CAPACITY;
    if (v->as.bytes.size && !v->as.bytes.data)
      return OW_INVALID;
    if (v->type == OW_STRING && !utf8(v->as.bytes.data, v->as.bytes.size))
      return OW_INVALID;
  }
  return OW_OK;
}
static size_t payload(const OwValue *v) {
  switch (v->type) {
  case OW_NULL:
    return 0;
  case OW_BOOL:
    return 1;
  case OW_INT:
  case OW_FLOAT:
    return 8;
  default:
    return v->as.bytes.size;
  }
}
static int equal(const OwValue *a, const OwValue *b) {
  if (a->type != b->type)
    return 0;
  switch (a->type) {
  case OW_NULL:
    return 1;
  case OW_BOOL:
    return a->as.boolean == b->as.boolean;
  case OW_INT:
    return a->as.integer == b->as.integer;
  case OW_FLOAT:
    return a->as.real == b->as.real;
  default:
    return a->as.bytes.size == b->as.bytes.size &&
           (!a->as.bytes.size ||
            !memcmp(a->as.bytes.data, b->as.bytes.data, a->as.bytes.size));
  }
}
static OwStatus copy_value(const OwValue *v, OwValue *out) {
  *out = *v;
  if (v->type == OW_STRING || v->type == OW_BINARY) {
    void *p = NULL;
    if (v->as.bytes.size) {
      p = malloc(v->as.bytes.size);
      if (!p) {
        memset(out, 0, sizeof(*out));
        return OW_NOMEM;
      }
      memcpy(p, v->as.bytes.data, v->as.bytes.size);
    }
    out->as.bytes.data = p;
  }
  return OW_OK;
}
void ow_value_release(OwValue *v) {
  if (!v)
    return;
  if (v->type == OW_STRING || v->type == OW_BINARY)
    free((void *)v->as.bytes.data);
  memset(v, 0, sizeof(*v));
}
void ow_array_release(void *p) { free(p); }
static void free_fields(Field *f) {
  while (f) {
    Field *next = f->next;
    free(f->name);
    ow_value_release(&f->value);
    free(f);
    f = next;
  }
}
static Field **find_field(Record *r, const char *name) {
  Field **p = &r->fields;
  while (*p && strcmp((*p)->name, name))
    p = &(*p)->next;
  return p;
}
static OwStatus capacity(OwDatabase *d, uint64_t counter) {
  return d->revision == UINT64_MAX || counter == UINT64_MAX ? OW_CAPACITY
                                                            : OW_OK;
}
OwStatus ow_database_create(OwDatabase **out) {
  OwDatabase *d;
  if (!out)
    return OW_INVALID;
  *out = NULL;
  d = calloc(1, sizeof(*d));
  if (!d)
    return OW_NOMEM;
  d->records = ow_index_create();
  d->collections = ow_index_create();
  d->occurrences = ow_index_create();
  if (!d->records || !d->collections || !d->occurrences) {
    ow_database_destroy(d);
    return OW_NOMEM;
  }
  *out = d;
  return OW_OK;
}
/* Detach borrowed items without mutations (also valid at LKS revision
 * capacity). The temporary chain uses an object-owned teardown link, never LKS
 * handles. */
static OwIndexed *detach_index(LksManagedOrder *index) {
  const LksOrderHandle *h;
  OwIndexed *head = NULL;
  for (h = lks_managed_order_first(index); h; h = lks_order_next(h)) {
    OwIndexed *item = lks_order_item(h);
    item->teardown_next = head;
    head = item;
  }
  lks_managed_order_destroy(index);
  return head;
}
void ow_database_destroy(OwDatabase *d) {
  const LksOrderHandle *h;
  OwIndexed *item, *next;
  if (!d)
    return;
  /* Destroy all explicit residences before detaching occurrence index items. */
  for (h = lks_managed_order_first(d->collections); h; h = lks_order_next(h)) {
    Collection *c = lks_order_item(h);
    lks_order_destroy(c->order);
    c->order = NULL;
  }
  item = detach_index(d->occurrences);
  while (item) {
    next = item->teardown_next;
    free(item);
    item = next;
  }
  item = detach_index(d->collections);
  while (item) {
    next = item->teardown_next;
    free(item);
    item = next;
  }
  item = detach_index(d->records);
  while (item) {
    next = item->teardown_next;
    free_fields(((Record *)item)->fields);
    free(item);
    item = next;
  }
  free(d);
}
OwStatus ow_revision(const OwDatabase *d, uint64_t *out) {
  if (out)
    *out = 0;
  if (!d || !out)
    return OW_INVALID;
  *out = d->revision;
  return OW_OK;
}
OwStatus ow_record_create(OwDatabase *d, OwRecordId *out) {
  Record *r;
  OwStatus s;
  if (out)
    out->value = 0;
  if (!d || !out)
    return OW_INVALID;
  if ((s = capacity(d, d->rid)) != OW_OK)
    return s;
  r = calloc(1, sizeof(*r));
  if (!r)
    return OW_NOMEM;
  r->key.id = d->rid + 1;
  s = ow_index_insert(d->records, &r->key);
  if (s != OW_OK) {
    free(r);
    return s;
  }
  out->value = ++d->rid;
  ++d->revision;
  return OW_OK;
}
OwStatus ow_record_exists(OwDatabase *d, OwRecordId id, int *out) {
  if (out)
    *out = 0;
  if (!d || !out || !id.value)
    return OW_INVALID;
  *out = ow_index_find(d->records, id.value) != NULL;
  return OW_OK;
}
OwStatus ow_record_delete(OwDatabase *d, OwRecordId id) {
  Record *r;
  OwStatus s;
  if (!d || !id.value)
    return OW_INVALID;
  r = ow_index_find(d->records, id.value);
  if (!r)
    return OW_NOT_FOUND;
  if (r->refs)
    return OW_CONFLICT;
  if ((s = capacity(d, 0)) != OW_OK)
    return s;
  if ((s = ow_index_remove(d->records, &r->key)) != OW_OK)
    return s;
  free_fields(r->fields);
  free(r);
  ++d->revision;
  return OW_OK;
}
OwStatus ow_field_set(OwDatabase *d, OwRecordId id, const char *name,
                      const OwValue *v) {
  Record *r;
  Field **slot, *f;
  size_t cost, old = 0;
  OwValue copy;
  OwStatus s;
  if (!d || !id.value)
    return OW_INVALID;
  if ((s = name_valid(name)) != OW_OK || (s = value_valid(v)) != OW_OK)
    return s;
  r = ow_index_find(d->records, id.value);
  if (!r)
    return OW_NOT_FOUND;
  slot = find_field(r, name);
  f = *slot;
  if (f && equal(&f->value, v))
    return OW_OK;
  cost = 17 + strlen(name) + payload(v);
  if (f)
    old = 17 + strlen(name) + payload(&f->value);
  if (cost > OW_RECORD_LIMIT - (r->budget - old))
    return OW_CAPACITY;
  if ((s = capacity(d, 0)) != OW_OK)
    return s;
  if ((s = copy_value(v, &copy)) != OW_OK)
    return s;
  if (!f) {
    f = calloc(1, sizeof(*f));
    if (!f) {
      ow_value_release(&copy);
      return OW_NOMEM;
    }
    f->name = malloc(strlen(name) + 1);
    if (!f->name) {
      free(f);
      ow_value_release(&copy);
      return OW_NOMEM;
    }
    strcpy(f->name, name);
    *slot = f;
  } else
    ow_value_release(&f->value);
  f->value = copy;
  r->budget = r->budget - old + cost;
  ++d->revision;
  return OW_OK;
}
OwStatus ow_field_get(OwDatabase *d, OwRecordId id, const char *name,
                      OwValue *out) {
  Record *r;
  Field *f;
  OwStatus s;
  if (out)
    memset(out, 0, sizeof(*out));
  if (!d || !out || !id.value)
    return OW_INVALID;
  if ((s = name_valid(name)) != OW_OK)
    return s;
  r = ow_index_find(d->records, id.value);
  if (!r)
    return OW_NOT_FOUND;
  f = *find_field(r, name);
  return f ? copy_value(&f->value, out) : OW_NOT_FOUND;
}
OwStatus ow_field_remove(OwDatabase *d, OwRecordId id, const char *name) {
  Record *r;
  Field **slot, *f;
  OwStatus s;
  if (!d || !id.value)
    return OW_INVALID;
  if ((s = name_valid(name)) != OW_OK)
    return s;
  r = ow_index_find(d->records, id.value);
  if (!r)
    return OW_NOT_FOUND;
  slot = find_field(r, name);
  f = *slot;
  if (!f)
    return OW_OK;
  if ((s = capacity(d, 0)) != OW_OK)
    return s;
  *slot = f->next;
  r->budget -= 17 + strlen(name) + payload(&f->value);
  f->next = NULL;
  free_fields(f);
  ++d->revision;
  return OW_OK;
}
OwStatus ow_collection_create(OwDatabase *d, OwCollectionId *out) {
  Collection *c;
  OwStatus s;
  if (out)
    out->value = 0;
  if (!d || !out)
    return OW_INVALID;
  if ((s = capacity(d, d->cid)) != OW_OK)
    return s;
  c = calloc(1, sizeof(*c));
  if (!c)
    return OW_NOMEM;
  c->key.id = d->cid + 1;
  c->order = lks_order_create();
  if (!c->order) {
    free(c);
    return OW_NOMEM;
  }
  s = ow_index_insert(d->collections, &c->key);
  if (s != OW_OK) {
    lks_order_destroy(c->order);
    free(c);
    return s;
  }
  out->value = ++d->cid;
  ++d->revision;
  return OW_OK;
}
OwStatus ow_collection_delete(OwDatabase *d, OwCollectionId id) {
  Collection *c;
  OwStatus s;
  if (!d || !id.value)
    return OW_INVALID;
  c = ow_index_find(d->collections, id.value);
  if (!c)
    return OW_NOT_FOUND;
  if (lks_order_size(c->order))
    return OW_CONFLICT;
  if ((s = capacity(d, 0)) != OW_OK)
    return s;
  if ((s = ow_index_remove(d->collections, &c->key)) != OW_OK)
    return s;
  lks_order_destroy(c->order);
  free(c);
  ++d->revision;
  return OW_OK;
}
OwStatus ow_collection_count(OwDatabase *d, OwCollectionId id, size_t *out) {
  Collection *c;
  if (out)
    *out = 0;
  if (!d || !out || !id.value)
    return OW_INVALID;
  c = ow_index_find(d->collections, id.value);
  if (!c)
    return OW_NOT_FOUND;
  *out = lks_order_size(c->order);
  return OW_OK;
}
OwStatus ow_collection_members(OwDatabase *d, OwCollectionId id,
                               OwOccurrenceId **out, size_t *count) {
  Collection *c;
  OwOccurrenceId *a;
  const LksOrderHandle *h;
  size_t n, i = 0;
  if (out)
    *out = NULL;
  if (count)
    *count = 0;
  if (!d || !out || !count || !id.value)
    return OW_INVALID;
  c = ow_index_find(d->collections, id.value);
  if (!c)
    return OW_NOT_FOUND;
  n = lks_order_size(c->order);
  if (!n)
    return OW_OK;
  if (n > SIZE_MAX / sizeof(*a))
    return OW_CAPACITY;
  a = malloc(n * sizeof(*a));
  if (!a)
    return OW_NOMEM;
  for (h = lks_order_first(c->order); h; h = lks_order_next(h))
    a[i++].value = ((Occurrence *)lks_order_item(h))->key.id;
  *out = a;
  *count = n;
  return OW_OK;
}
static OwStatus anchor_get(OwDatabase *d, Collection *c, OwPlacement p,
                           OwOccurrenceId id, Occurrence **out) {
  *out = NULL;
  if (p < OW_FRONT || p > OW_AFTER)
    return OW_INVALID;
  if (p == OW_FRONT || p == OW_BACK)
    return id.value ? OW_INVALID : OW_OK;
  if (!id.value)
    return OW_INVALID;
  *out = ow_index_find(d->occurrences, id.value);
  if (!*out)
    return OW_NOT_FOUND;
  return (*out)->collection == c ? OW_OK : OW_INVALID;
}
/* Build an unpublished index borrowing stable objects. Do not replace their
 * current index handles until the complete operation commits. */
static OwStatus stage_occurrences(OwDatabase *d, Occurrence *add,
                                  Occurrence *omit, LksManagedOrder **out) {
  const LksOrderHandle *h, *temporary;
  LksManagedOrder *index = ow_index_create();
  OwStatus s;
  *out = NULL;
  if (!index)
    return OW_NOMEM;
  for (h = lks_managed_order_first(d->occurrences); h; h = lks_order_next(h)) {
    Occurrence *o = lks_order_item(h);
    if (o == omit)
      continue;
    s = ow_lks_status(lks_managed_order_insert(index, o, &temporary));
    if (s != OW_OK) {
      lks_managed_order_destroy(index);
      return s;
    }
  }
  if (add) {
    if (ow_index_find(index, add->key.id)) {
      lks_managed_order_destroy(index);
      return OW_CONFLICT;
    }
    s = ow_lks_status(lks_managed_order_insert(index, add, &temporary));
    if (s != OW_OK) {
      lks_managed_order_destroy(index);
      return s;
    }
  }
  *out = index;
  return OW_OK;
}
static void publish_occurrences(OwDatabase *d, LksManagedOrder *index) {
  const LksOrderHandle *h;
  lks_managed_order_destroy(d->occurrences);
  d->occurrences = index;
  for (h = lks_managed_order_first(index); h; h = lks_order_next(h))
    ((Occurrence *)lks_order_item(h))->key.index_handle = h;
}
OwStatus ow_occurrence_insert(OwDatabase *d, OwCollectionId cid, OwRecordId rid,
                              OwPlacement p, OwOccurrenceId aid,
                              OwOccurrenceId *out) {
  Collection *c;
  Record *r;
  Occurrence *a, *o;
  OwStatus s;
  LksManagedOrder *staged;
  if (out)
    out->value = 0;
  if (!d || !out || !cid.value || !rid.value)
    return OW_INVALID;
  c = ow_index_find(d->collections, cid.value);
  r = ow_index_find(d->records, rid.value);
  if (!c || !r)
    return OW_NOT_FOUND;
  if ((s = anchor_get(d, c, p, aid, &a)) != OW_OK)
    return s;
  if ((s = capacity(d, d->oid)) != OW_OK || r->refs == SIZE_MAX)
    return OW_CAPACITY;
  o = calloc(1, sizeof(*o));
  if (!o)
    return OW_NOMEM;
  o->key.id = d->oid + 1;
  o->collection = c;
  o->record = r;
  s = stage_occurrences(d, o, NULL, &staged);
  if (s != OW_OK) {
    free(o);
    return s;
  }
  s = ow_order_insert(c->order, o, p, a ? a->handle : NULL, &o->handle);
  if (s != OW_OK) {
    lks_managed_order_destroy(staged);
    free(o);
    return s;
  }
  publish_occurrences(d, staged);
  ++r->refs;
  out->value = ++d->oid;
  ++d->revision;
  return OW_OK;
}
OwStatus ow_occurrence_move(OwDatabase *d, OwOccurrenceId id, OwPlacement p,
                            OwOccurrenceId aid) {
  Occurrence *o, *a;
  OwStatus s;
  int noop;
  if (!d || !id.value)
    return OW_INVALID;
  o = ow_index_find(d->occurrences, id.value);
  if (!o)
    return OW_NOT_FOUND;
  if ((s = anchor_get(d, o->collection, p, aid, &a)) != OW_OK)
    return s;
  noop =
      a == o ||
      (p == OW_FRONT && lks_order_first(o->collection->order) == o->handle) ||
      (p == OW_BACK && lks_order_last(o->collection->order) == o->handle) ||
      (p == OW_BEFORE && lks_order_next(o->handle) == a->handle) ||
      (p == OW_AFTER && lks_order_previous(o->handle) == a->handle);
  if (noop)
    return OW_OK;
  if ((s = capacity(d, 0)) != OW_OK)
    return s;
  s = ow_order_move(o->collection->order, o->handle, p, a ? a->handle : NULL);
  if (s == OW_OK)
    ++d->revision;
  return s;
}
OwStatus ow_occurrence_remove(OwDatabase *d, OwOccurrenceId id) {
  Occurrence *o;
  OwStatus s;
  LksManagedOrder *staged;
  if (!d || !id.value)
    return OW_INVALID;
  o = ow_index_find(d->occurrences, id.value);
  if (!o)
    return OW_NOT_FOUND;
  if ((s = capacity(d, 0)) != OW_OK)
    return s;
  if ((s = stage_occurrences(d, NULL, o, &staged)) != OW_OK)
    return s;
  s = ow_lks_status(lks_order_remove(o->collection->order, o->handle, NULL));
  if (s != OW_OK) {
    lks_managed_order_destroy(staged);
    return s;
  }
  o->handle = NULL;
  publish_occurrences(d, staged);
  --o->record->refs;
  free(o);
  ++d->revision;
  return OW_OK;
}
OwStatus ow_occurrence_get(OwDatabase *d, OwOccurrenceId id,
                           OwCollectionId *cid, OwRecordId *rid) {
  Occurrence *o;
  if (cid)
    cid->value = 0;
  if (rid)
    rid->value = 0;
  if (!d || !cid || !rid || !id.value)
    return OW_INVALID;
  o = ow_index_find(d->occurrences, id.value);
  if (!o)
    return OW_NOT_FOUND;
  cid->value = o->collection->key.id;
  rid->value = o->record->key.id;
  return OW_OK;
}
OwStatus ow_occurrence_compare(OwDatabase *d, OwOccurrenceId left,
                               OwOccurrenceId right, int *out) {
  Occurrence *a, *b;
  if (out)
    *out = 0;
  if (!d || !out || !left.value || !right.value)
    return OW_INVALID;
  a = ow_index_find(d->occurrences, left.value);
  b = ow_index_find(d->occurrences, right.value);
  if (!a || !b)
    return OW_NOT_FOUND;
  if (a->collection != b->collection)
    return OW_INVALID;
  return ow_lks_status(
      lks_order_compare(a->collection->order, a->handle, b->handle, out));
}
OwStatus ow_query_equal(OwDatabase *d, const char *name, const OwValue *v,
                        OwRecordId **out, size_t *count) {
  const LksOrderHandle *h;
  OwRecordId *a;
  size_t n = 0, i = 0;
  OwStatus s;
  if (out)
    *out = NULL;
  if (count)
    *count = 0;
  if (!d || !out || !count)
    return OW_INVALID;
  if ((s = name_valid(name)) != OW_OK || (s = value_valid(v)) != OW_OK)
    return s;
  for (h = lks_managed_order_first(d->records); h; h = lks_order_next(h)) {
    Field *f = *find_field(lks_order_item(h), name);
    if (f && equal(&f->value, v))
      ++n;
  }
  if (!n)
    return OW_OK;
  if (n > SIZE_MAX / sizeof(*a))
    return OW_CAPACITY;
  a = malloc(n * sizeof(*a));
  if (!a)
    return OW_NOMEM;
  for (h = lks_managed_order_first(d->records); h; h = lks_order_next(h)) {
    Record *r = lks_order_item(h);
    Field *f = *find_field(r, name);
    if (f && equal(&f->value, v))
      a[i++].value = r->key.id;
  }
  *out = a;
  *count = n;
  return OW_OK;
}
