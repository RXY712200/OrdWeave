#include "wal_internal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
void ow_put(unsigned char *p, uint64_t v, size_t n) {
  while (n) {
    p[--n] = (unsigned char)v;
    v >>= 8;
  }
}
uint64_t ow_get(const unsigned char *p, size_t n) {
  uint64_t v = 0;
  while (n--)
    v = (v << 8) | *p++;
  return v;
}
OwStatus ow_encode(const OwCommand *c, unsigned char **out, size_t *size) {
  size_t n = 8, nl = 0, vl = 0;
  unsigned char *f, *p;
  uint64_t bits = 0;
  OwStatus s;
  *out = NULL;
  *size = 0;
  if (c->op == OP_FIELD_SET || c->op == OP_FIELD_REMOVE) {
    if (!c->name || (c->op == OP_FIELD_SET && !c->value))
      return OW_INVALID;
    s = ow_core_validate(c->name, c->op == OP_FIELD_SET ? c->value : NULL);
    if (s != OW_OK)
      return s;
    nl = strlen(c->name);
    n += 4 + nl;
    if (c->op == OP_FIELD_SET) {
      switch (c->value->type) {
      case OW_NULL:
        vl = 0;
        break;
      case OW_BOOL:
        vl = 1;
        break;
      case OW_INT:
      case OW_FLOAT:
        vl = 8;
        break;
      default:
        vl = c->value->as.bytes.size;
        break;
      }
      n += 5 + vl;
    }
  } else if (c->op == OP_OCCURRENCE_INSERT)
    n = 33;
  else if (c->op == OP_OCCURRENCE_MOVE)
    n = 17;
  if (n > OW_FRAME_LIMIT - OW_FRAME_OVERHEAD)
    return OW_CAPACITY;
  f = calloc(1, n + OW_FRAME_OVERHEAD);
  if (!f)
    return OW_NOMEM;
  p = f + OW_FRAME_HEADER;
  ow_put(p, c->id, 8);
  p += 8;
  if (c->op == OP_FIELD_SET || c->op == OP_FIELD_REMOVE) {
    ow_put(p, nl, 4);
    p += 4;
    memcpy(p, c->name, nl);
    p += nl;
    if (c->op == OP_FIELD_SET) {
      *p++ = (unsigned char)c->value->type;
      ow_put(p, vl, 4);
      p += 4;
      switch (c->value->type) {
      case OW_NULL:
        break;
      case OW_BOOL:
        *p = (unsigned char)c->value->as.boolean;
        break;
      case OW_INT:
        ow_put(p, (uint64_t)c->value->as.integer, 8);
        break;
      case OW_FLOAT: {
        double v = c->value->as.real;
        if (v == 0)
          v = 0;
        memcpy(&bits, &v, 8);
        ow_put(p, bits, 8);
        break;
      }
      default:
        if (vl)
          memcpy(p, c->value->as.bytes.data, vl);
        break;
      }
    }
  } else if (c->op == OP_OCCURRENCE_INSERT) {
    ow_put(p, c->collection, 8);
    ow_put(p + 8, c->record, 8);
    p[16] = (unsigned char)c->place;
    ow_put(p + 17, c->anchor, 8);
  } else if (c->op == OP_OCCURRENCE_MOVE) {
    *p++ = (unsigned char)c->place;
    ow_put(p, c->anchor, 8);
  }
  *out = f;
  *size = n + OW_FRAME_OVERHEAD;
  return OW_OK;
}
OwStatus ow_decode(const unsigned char *p, size_t n, OwOpcode op, OwCommand *c,
                   char **name, OwValue *v) {
  uint64_t bits;
  size_t nl, vl;
  OwStatus s;
  memset(c, 0, sizeof(*c));
  memset(v, 0, sizeof(*v));
  *name = NULL;
  c->op = op;
  if (op < OP_RECORD_CREATE || op > OP_OCCURRENCE_REMOVE || n < 8)
    return OW_CORRUPT;
  c->id = ow_get(p, 8);
  p += 8;
  n -= 8;
  if (!c->id)
    return OW_CORRUPT;
  if (op == OP_FIELD_SET || op == OP_FIELD_REMOVE) {
    if (n < 4)
      return OW_CORRUPT;
    nl = (size_t)ow_get(p, 4);
    p += 4;
    n -= 4;
    if (!nl || nl > OW_FIELD_LIMIT || nl > n || memchr(p, 0, nl))
      return OW_CORRUPT;
    *name = malloc(nl + 1);
    if (!*name)
      return OW_NOMEM;
    memcpy(*name, p, nl);
    (*name)[nl] = 0;
    c->name = *name;
    p += nl;
    n -= nl;
    if (op == OP_FIELD_SET) {
      if (n < 5)
        return OW_CORRUPT;
      v->type = (OwType)*p++;
      vl = (size_t)ow_get(p, 4);
      p += 4;
      n -= 5;
      if (vl != n || vl > OW_FIELD_LIMIT)
        return OW_CORRUPT;
      switch (v->type) {
      case OW_NULL:
        if (vl)
          return OW_CORRUPT;
        break;
      case OW_BOOL:
        if (vl != 1 || *p > 1)
          return OW_CORRUPT;
        v->as.boolean = *p;
        break;
      case OW_INT:
        if (vl != 8)
          return OW_CORRUPT;
        bits = ow_get(p, 8);
        v->as.integer = bits <= INT64_MAX ? (int64_t)bits
                                          : -1 - (int64_t)(UINT64_MAX - bits);
        break;
      case OW_FLOAT:
        if (vl != 8)
          return OW_CORRUPT;
        bits = ow_get(p, 8);
        if (bits == UINT64_C(0x8000000000000000))
          return OW_CORRUPT;
        memcpy(&v->as.real, &bits, 8);
        break;
      case OW_STRING:
      case OW_BINARY:
        v->as.bytes.data = p;
        v->as.bytes.size = vl;
        break;
      default:
        return OW_CORRUPT;
      }
      c->value = v;
      n = 0;
    }
    s = ow_core_validate(c->name, c->value);
    if (s != OW_OK)
      return OW_CORRUPT;
  } else if (op == OP_OCCURRENCE_INSERT) {
    if (n != 25)
      return OW_CORRUPT;
    c->collection = ow_get(p, 8);
    c->record = ow_get(p + 8, 8);
    c->place = (OwPlacement)p[16];
    c->anchor = ow_get(p + 17, 8);
    n = 0;
    if (!c->collection || !c->record)
      return OW_CORRUPT;
  } else if (op == OP_OCCURRENCE_MOVE) {
    if (n != 9)
      return OW_CORRUPT;
    c->place = (OwPlacement)*p++;
    c->anchor = ow_get(p, 8);
    n = 0;
  }
  if (n)
    return OW_CORRUPT;
  if (op == OP_OCCURRENCE_INSERT || op == OP_OCCURRENCE_MOVE) {
    if (c->place < OW_FRONT || c->place > OW_AFTER ||
        ((c->place < OW_BEFORE) != (c->anchor == 0)))
      return OW_CORRUPT;
  }
  return OW_OK;
}
OwStatus ow_apply(OwDatabase *d, OwCommand *c) {
  OwStatus s;
  OwRecordId r = {0};
  OwCollectionId col = {0};
  OwOccurrenceId o = {0};
  uint64_t id = c->id;
  switch (c->op) {
  case OP_RECORD_CREATE:
    s = ow_core_record_create(d, &r);
    c->id = r.value;
    break;
  case OP_RECORD_DELETE:
    return ow_core_record_delete(d, (OwRecordId){id});
  case OP_FIELD_SET:
    return ow_core_field_set(d, (OwRecordId){id}, c->name, c->value);
  case OP_FIELD_REMOVE:
    return ow_core_field_remove(d, (OwRecordId){id}, c->name);
  case OP_COLLECTION_CREATE:
    s = ow_core_collection_create(d, &col);
    c->id = col.value;
    break;
  case OP_COLLECTION_DELETE:
    return ow_core_collection_delete(d, (OwCollectionId){id});
  case OP_OCCURRENCE_INSERT:
    s = ow_core_occurrence_insert(d, (OwCollectionId){c->collection},
                                  (OwRecordId){c->record}, c->place,
                                  (OwOccurrenceId){c->anchor}, &o);
    c->id = o.value;
    break;
  case OP_OCCURRENCE_MOVE:
    return ow_core_occurrence_move(d, (OwOccurrenceId){id}, c->place,
                                   (OwOccurrenceId){c->anchor});
  case OP_OCCURRENCE_REMOVE:
    return ow_core_occurrence_remove(d, (OwOccurrenceId){id});
  default:
    return OW_CORRUPT;
  }
  if (s == OW_OK && id && id != c->id)
    return OW_CORRUPT;
  return s;
}
