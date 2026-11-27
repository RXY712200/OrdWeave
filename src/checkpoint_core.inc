/* Included in the core to keep object layouts private. No ordering
 * implementation. */
#include "checkpoint_internal.h"
#ifdef OW_TESTING
long ow_test_checkpoint_alloc = -1;
uint64_t ow_test_checkpoint_restores, ow_test_checkpoint_tail;
#endif
int ow_checkpoint_oom(void) {
#ifdef OW_TESTING
  if (ow_test_checkpoint_alloc == 0)
    return 1;
  if (ow_test_checkpoint_alloc > 0)
    --ow_test_checkpoint_alloc;
#endif
  return 0;
}
typedef struct {
  unsigned char *p;
  size_t n, cap;
  OwStatus status;
} CpWriter;
static void cp_write(CpWriter *w, const void *p, size_t n) {
  if (w->status != OW_OK)
    return;
  if (n > OW_CHECKPOINT_LIMIT - w->n) {
    w->status = OW_CAPACITY;
    return;
  }
  if (w->n + n > w->cap) {
    size_t cap = w->cap ? w->cap : 4096;
    unsigned char *q;
    while (cap < w->n + n) {
      if (cap > OW_CHECKPOINT_LIMIT / 2) {
        cap = OW_CHECKPOINT_LIMIT;
        break;
      }
      cap *= 2;
    }
    q = ow_checkpoint_oom() ? NULL : realloc(w->p, cap);
    if (!q) {
      w->status = OW_NOMEM;
      return;
    }
    w->p = q;
    w->cap = cap;
  }
  if (n)
    memcpy(w->p + w->n, p, n);
  w->n += n;
}
static void cp_number(CpWriter *w, uint64_t v, size_t n) {
  unsigned char b[8];
  ow_put(b, v, n);
  cp_write(w, b, n);
}
static int cp_field_compare(const void *a, const void *b) {
  return strcmp((*(Field *const *)a)->name, (*(Field *const *)b)->name);
}
static LksStatus cp_associate(void *item, uint64_t ordinal, void *ctx,
                              const void **data, size_t *size) {
  (void)ordinal;
  ow_put(ctx, ((Occurrence *)item)->key.id, 8);
  *data = ctx;
  *size = 8;
  return LKS_STATUS_OK;
}
static void cp_namespace(unsigned char out[40], const unsigned char id[16],
                         uint64_t cid, uint64_t rev, uint64_t gen) {
  memcpy(out, id, 16);
  ow_put(out + 16, cid, 8);
  ow_put(out + 24, rev, 8);
  ow_put(out + 32, gen, 8);
}
OwStatus ow_core_export(OwDatabase *d, const unsigned char id[16], uint64_t gen,
                        const unsigned char anchor[32], unsigned char **out,
                        size_t *size) {
  CpWriter w = {0};
  unsigned char header[144] = {0}, hash[32];
  const LksOrderHandle *h;
  OwStatus status;
  *out = NULL;
  *size = 0;
  memcpy(header, "OWCHECK1", 8);
  ow_put(header + 8, 1, 4);
  ow_put(header + 12, 144, 4);
  memcpy(header + 24, id, 16);
  ow_put(header + 40, gen, 8);
  ow_put(header + 48, d->revision, 8);
  ow_put(header + 56, d->rid, 8);
  ow_put(header + 64, d->cid, 8);
  ow_put(header + 72, d->oid, 8);
  memcpy(header + 80, anchor, 32);
  ow_put(header + 112, lks_managed_order_size(d->records), 8);
  ow_put(header + 120, lks_managed_order_size(d->collections), 8);
  ow_put(header + 128, d->occurrences->count, 8);
  cp_write(&w, header, 144);
  for (h = lks_managed_order_first(d->records); h; h = lks_order_next(h)) {
    Record *r = lks_order_item(h);
    Field *f, **fields = NULL;
    size_t n = 0, i;
    cp_number(&w, r->key.id, 8);
    for (f = r->fields; f; f = f->next)
      ++n;
    cp_number(&w, n, 8);
    if (n) {
      fields = ow_checkpoint_oom() ? NULL : malloc(n * sizeof(*fields));
      if (!fields) {
        w.status = OW_NOMEM;
        break;
      }
      for (f = r->fields, i = 0; f; f = f->next)
        fields[i++] = f;
      qsort(fields, n, sizeof(*fields), cp_field_compare);
    }
    for (i = 0; i < n; i++) {
      unsigned char *frame = NULL;
      size_t bytes = 0;
      OwCommand cmd = {0};
      cmd.op = OP_FIELD_SET;
      cmd.id = r->key.id;
      cmd.name = fields[i]->name;
      cmd.value = &fields[i]->value;
      if (ow_checkpoint_oom())
        status = OW_NOMEM;
      else
        status = ow_encode(&cmd, &frame, &bytes);
      if (status != OW_OK) {
        w.status = status;
        break;
      }
      cp_number(&w, bytes - OW_FRAME_OVERHEAD - 8, 4);
      cp_write(&w, frame + OW_FRAME_HEADER + 8, bytes - OW_FRAME_OVERHEAD - 8);
      free(frame);
    }
    free(fields);
    if (w.status != OW_OK)
      break;
  }
  for (h = lks_managed_order_first(d->collections); w.status == OW_OK && h;
       h = lks_order_next(h)) {
    Collection *c = lks_order_item(h);
    LksSnapshot *snap = NULL;
    unsigned char ns[40], association[8], *blob = NULL;
    size_t n;
    cp_namespace(ns, id, c->key.id, d->revision, gen);
    LksSnapshotOptions opts = {ns, 40, cp_associate, association};
    status =
        ow_checkpoint_oom()
            ? OW_NOMEM
            : ow_lks_status(lks_order_snapshot_capture(c->order, &opts, &snap));
    if (status != OW_OK) {
      w.status = status;
      break;
    }
    n = lks_snapshot_serialized_size(snap);
    if (n > OW_CHECKPOINT_LIMIT) {
      w.status = OW_CAPACITY;
      lks_snapshot_destroy(snap);
      break;
    }
    blob = ow_checkpoint_oom() ? NULL : malloc(n);
    if (!blob) {
      w.status = OW_NOMEM;
      lks_snapshot_destroy(snap);
      break;
    }
    status = ow_lks_status(lks_snapshot_serialize(snap, blob, n));
    lks_snapshot_destroy(snap);
    if (status == OW_OK) {
      cp_number(&w, c->key.id, 8);
      cp_number(&w, lks_order_size(c->order), 8);
      cp_number(&w, n, 8);
      cp_write(&w, blob, n);
    } else
      w.status = status;
    free(blob);
  }
  OwOccIterator occurrence_it;OwIndexed *entry;
  for (entry=occ_first(d->occurrences,&occurrence_it);w.status==OW_OK && entry;
       entry=occ_next(&occurrence_it)) {
    Occurrence *o = (Occurrence *)entry;
    cp_number(&w, o->key.id, 8);
    cp_number(&w, o->collection->key.id, 8);
    cp_number(&w, o->record->key.id, 8);
  }
  if (w.status == OW_OK) {
    if (w.n > OW_CHECKPOINT_LIMIT - 32)
      w.status = OW_CAPACITY;
    else {
      ow_put(w.p + 16, w.n + 32, 8);
      w.status = ow_sha256(w.p, w.n, NULL, 0, hash);
      if (w.status == OW_OK)
        cp_write(&w, hash, 32);
    }
  }
  if (w.status != OW_OK) {
    free(w.p);
    return w.status;
  }
  *out = w.p;
  *size = w.n;
  return OW_OK;
}
typedef struct {
  const unsigned char *p;
  size_t n;
  OwStatus status;
} CpReader;
static const unsigned char *cp_read(CpReader *r, size_t n) {
  const unsigned char *p = r->p;
  if (r->status != OW_OK || n > r->n) {
    r->status = OW_CORRUPT;
    return NULL;
  }
  r->p += n;
  r->n -= n;
  return p;
}
static uint64_t cp_get(CpReader *r, size_t n) {
  const unsigned char *p = cp_read(r, n);
  return p ? ow_get(p, n) : 0;
}
typedef struct {
  Collection *collection;
  const unsigned char *blob;
  size_t size, count;
} CpCollection;
typedef struct {
  OwDatabase *db;
  Collection *collection;
} CpResolver;
static LksStatus cp_resolve(const void *data, size_t size, uint64_t ordinal,
                            void *context, void **out) {
  CpResolver *r = context;
  Occurrence *o;
  (void)ordinal;
  *out = NULL;
  if (size != 8)
    return LKS_STATUS_INVALID_ARGUMENT;
  o = occ_find(r->db->occurrences, ow_get(data, 8));
  if (!o || o->collection != r->collection || o->checkpoint_seen)
    return LKS_STATUS_INVALID_ARGUMENT;
  o->checkpoint_seen = 1;
  *out = o;
  return LKS_STATUS_OK;
}
OwStatus ow_core_import(OwDatabase *d, const unsigned char *bytes, size_t size,
                        const unsigned char id[16]) {
  CpReader rd;
  unsigned char hash[32];
  uint64_t nr, nc, no, i, prev = 0, rev, rid, cid, oid, gen;
  CpCollection *collections = NULL;
  OwStatus status = OW_CORRUPT;
  if (size < 176 || size > OW_CHECKPOINT_LIMIT || memcmp(bytes, "OWCHECK1", 8))
    return OW_CORRUPT;
  if (ow_get(bytes + 8, 4) != 1)
    return OW_UNSUPPORTED;
  if (ow_get(bytes + 12, 4) != 144 || ow_get(bytes + 16, 8) != size ||
      memcmp(bytes + 24, id, 16) || ow_get(bytes + 136, 8))
    return OW_CORRUPT;
  status = ow_sha256(bytes, size - 32, NULL, 0, hash);
  if (status != OW_OK)
    return status;
  if (memcmp(hash, bytes + size - 32, 32))
    return OW_CORRUPT;
  gen = ow_get(bytes + 40, 8);
  rev = ow_get(bytes + 48, 8);
  rid = ow_get(bytes + 56, 8);
  cid = ow_get(bytes + 64, 8);
  oid = ow_get(bytes + 72, 8);
  nr = ow_get(bytes + 112, 8);
  nc = ow_get(bytes + 120, 8);
  no = ow_get(bytes + 128, 8);
  if (!gen || rid > rev || cid > rev - rid || oid > rev - rid - cid || nr > (size - 176) / 16 ||
      nc > (size - 176) / 88 || no > (size - 176) / 24 || nr > rid ||
      nc > cid || no > oid)
    return OW_CORRUPT;
  if (nc) {
    collections =
        ow_checkpoint_oom() ? NULL : calloc((size_t)nc, sizeof(*collections));
    if (!collections)
      return OW_NOMEM;
  }
  rd.p = bytes + 144;
  rd.n = size - 176;
  rd.status = OW_OK;
  status = OW_CORRUPT;
  for (i = 0; i < nr; i++) {
    uint64_t record = cp_get(&rd, 8), fields = cp_get(&rd, 8), j;
    OwRecordId out;
    char *last = NULL;
    if (rd.status != OW_OK || record <= prev || record > rid ||
        fields > rd.n / 9) {
      status = OW_CORRUPT;
      goto done;
    }
    prev = record;
    d->rid = record - 1;
    if (ow_checkpoint_oom()) {
      status = OW_NOMEM;
      goto done;
    }
    status = ow_core_record_create(d, &out);
    if (status != OW_OK)
      goto done;
    for (j = 0; j < fields; j++) {
      size_t n = (size_t)cp_get(&rd, 4);
      const unsigned char *p;
      unsigned char *payload;
      OwCommand cmd;
      OwValue value;
      char *name = NULL;
      if (rd.status != OW_OK || n > OW_FRAME_LIMIT - 8 ||
          !(p = cp_read(&rd, n))) {
        free(last);
        status = OW_CORRUPT;
        goto done;
      }
      payload = ow_checkpoint_oom() ? NULL : malloc(n + 8);
      if (!payload) {
        free(last);
        status = OW_NOMEM;
        goto done;
      }
      ow_put(payload, record, 8);
      memcpy(payload + 8, p, n);
      status = ow_decode(payload, n + 8, OP_FIELD_SET, &cmd, &name, &value);
      if (status == OW_OK && last && strcmp(last, name) >= 0)
        status = OW_CORRUPT;
      if (status == OW_OK)
        status = ow_checkpoint_oom() ? OW_NOMEM
                                     : ow_core_field_set(d, out, name, &value);
      free(last);
      last = name;
      free(payload);
      if (status != OW_OK) {
        free(last);
        goto done;
      }
    }
    free(last);
  }
  prev = 0;
  for (i = 0; i < nc; i++) {
    uint64_t collection = cp_get(&rd, 8), count = cp_get(&rd, 8),
             n = cp_get(&rd, 8);
    OwCollectionId out;
    if (rd.status != OW_OK || collection <= prev || collection > cid ||
        n > rd.n || n > OW_CHECKPOINT_LIMIT || count > no) {
      status = OW_CORRUPT;
      goto done;
    }
    prev = collection;
    d->cid = collection - 1;
    status =
        ow_checkpoint_oom() ? OW_NOMEM : ow_core_collection_create(d, &out);
    if (status != OW_OK)
      goto done;
    collections[i].collection = ow_index_find(d->collections, collection);
    collections[i].count = (size_t)count;
    collections[i].size = (size_t)n;
    collections[i].blob = cp_read(&rd, (size_t)n);
  }
  prev = 0;
  for (i = 0; i < no; i++) {
    uint64_t occurrence = cp_get(&rd, 8), collection = cp_get(&rd, 8),
             record = cp_get(&rd, 8);
    OwOccurrenceId out;
    if (rd.status != OW_OK || occurrence <= prev || occurrence > oid) {
      status = OW_CORRUPT;
      goto done;
    }
    prev = occurrence;
    d->oid = occurrence - 1;
    status = ow_checkpoint_oom()
                 ? OW_NOMEM
                 : ow_core_occurrence_insert(d, (OwCollectionId){collection},
                                             (OwRecordId){record}, OW_BACK,
                                             (OwOccurrenceId){0}, &out);
    if (status != OW_OK)
      goto done;
  }
  if (rd.status != OW_OK || rd.n) {
    status = OW_CORRUPT;
    goto done;
  }
  for (i = 0; i < nc; i++) {
    CpCollection *c = &collections[i];
    LksSnapshot *snap = NULL;
    LksOrder *restored = NULL;
    const void *ns;
    size_t nsn;
    unsigned char expected[40];
    const LksOrderHandle *h;
    CpResolver resolver = {d, c->collection};
    status =
        ow_checkpoint_oom()
            ? OW_NOMEM
            : ow_lks_status(lks_snapshot_deserialize(c->blob, c->size, &snap));
    if (status != OW_OK)
      goto done;
    cp_namespace(expected, id, c->collection->key.id, rev, gen);
    if (lks_snapshot_namespace(snap, &ns, &nsn) != LKS_STATUS_OK || nsn != 40 ||
        memcmp(ns, expected, 40) || lks_snapshot_count(snap) != c->count ||
        c->count != lks_order_size(c->collection->order)) {
      lks_snapshot_destroy(snap);
      status = OW_CORRUPT;
      goto done;
    }
    status = ow_checkpoint_oom() ? OW_NOMEM
                                 : ow_lks_status(lks_snapshot_restore_order(
                                       snap, cp_resolve, &resolver, &restored));
    lks_snapshot_destroy(snap);
    if (status != OW_OK)
      goto done;
    lks_order_destroy(c->collection->order);
    c->collection->order = restored;
    for (h = lks_order_first(restored); h; h = lks_order_next(h))
      ((Occurrence *)lks_order_item(h))->handle = h;
  }
  {
    OwOccIterator it;OwIndexed *entry;
    for(entry=occ_first(d->occurrences,&it);entry;entry=occ_next(&it))
      if (!((Occurrence *)entry)->checkpoint_seen) {
        status = OW_CORRUPT;
        goto done;
      }
  }
  if(d->revision>rev) {status=OW_CORRUPT;goto done;}
  d->rid = rid;
  d->cid = cid;
  d->oid = oid;
  d->revision = rev;
  status = OW_OK;
done:
  free(collections);
  if (status != OW_OK && status != OW_NOMEM && status != OW_IO &&
      status != OW_UNSUPPORTED)
    status = OW_CORRUPT;
  return status;
}
