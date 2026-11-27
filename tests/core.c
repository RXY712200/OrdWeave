#include "ordweave.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define T(x)                                                                   \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "%d: %s\n", __LINE__, #x);                               \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)
#define OK(x) T((x) == OW_OK)
static uint64_t rev(OwDatabase *d) {
  uint64_t n;
  OK(ow_revision(d, &n));
  return n;
}
static void members(OwDatabase *d, OwCollectionId c, OwOccurrenceId *model,
                    size_t n) {
  OwOccurrenceId *a = NULL;
  size_t count = 99, i;
  OK(ow_collection_members(d, c, &a, &count));
  T(count == n);
  for (i = 0; i < n; i++) {
    int cmp;
    T(a[i].value == model[i].value);
    OK(ow_occurrence_compare(d, a[i], a[i], &cmp));
    T(cmp == 0);
    if (i) {
      OK(ow_occurrence_compare(d, a[i - 1], a[i], &cmp));
      T(cmp < 0);
    }
  }
  ow_array_release(a);
}
#include "range_cases.inc"
int main(void) {
  range_contracts();
  OwDatabase *d = NULL;
  OwRecordId r, s;
  OwCollectionId c, c2, gotc;
  OwRecordId gotr;
  OwOccurrenceId a, b, e, f, z = {0}, model[256];
  OwValue v = {0}, out = {0};
  OwRecordId *results = NULL;
  size_t n, i;
  uint64_t before;
  int exists;
  char text[] = "hello";
  unsigned char bytes[] = {0, 255, 1};
  OK(ow_database_create(&d));
  T(rev(d) == 0);
  ow_database_destroy(d);
  OK(ow_database_create(&d));
  OK(ow_record_create(d, &r));
  OK(ow_record_create(d, &s));
  T(r.value == 1 && s.value == 2);
  OK(ow_record_exists(d, r, &exists));
  T(exists);
  OK(ow_record_delete(d, s));
  OK(ow_record_exists(d, s, &exists));
  T(!exists);
  OK(ow_record_create(d, &s));
  T(s.value == 3);
  for (i = OW_NULL; i <= OW_BINARY; i++) {
    memset(&v, 0, sizeof(v));
    v.type = (OwType)i;
    if (i == OW_BOOL)
      v.as.boolean = 1;
    if (i == OW_INT)
      v.as.integer = INT64_MIN;
    if (i == OW_FLOAT)
      v.as.real = -0.0;
    if (i == OW_STRING) {
      v.as.bytes.data = text;
      v.as.bytes.size = 5;
    }
    if (i == OW_BINARY) {
      v.as.bytes.data = bytes;
      v.as.bytes.size = 3;
    }
    OK(ow_field_set(d, r, "field", &v));
    OK(ow_field_get(d, r, "field", &out));
    T(out.type == v.type);
    if (i == OW_STRING || i == OW_BINARY) {
      T(out.as.bytes.data != v.as.bytes.data);
      T(!memcmp(out.as.bytes.data, v.as.bytes.data, v.as.bytes.size));
    }
    if (i == OW_INT)
      T(out.as.integer == INT64_MIN);
    if (i == OW_BOOL)
      T(out.as.boolean == 1);
    ow_value_release(&out);
    before = rev(d);
    OK(ow_field_set(d, r, "field", &v));
    T(rev(d) == before);
  }
  v.type = OW_STRING;
  v.as.bytes.data = text;
  v.as.bytes.size = 5;
  OK(ow_field_set(d, r, "copy", &v));
  text[0] = 'X';
  OK(ow_field_get(d, r, "copy", &out));
  T(!memcmp(out.as.bytes.data, "hello", 5));
  OK(ow_field_remove(d, r, "copy"));
  T(!memcmp(out.as.bytes.data, "hello", 5));
  ow_value_release(&out);
  before = rev(d);
  OK(ow_field_remove(d, r, "missing"));
  T(rev(d) == before);
  v.type = OW_FLOAT;
  v.as.real = 0;
  OK(ow_field_set(d, r, "zero", &v));
  before = rev(d);
  v.as.real = -0.0;
  OK(ow_field_set(d, r, "zero", &v));
  T(rev(d) == before);
  v.as.real = INFINITY;
  T(ow_field_set(d, r, "zero", &v) == OW_INVALID);
  v.as.real = NAN;
  T(ow_field_set(d, r, "zero", &v) == OW_INVALID);
  v.type = OW_STRING;
  v.as.bytes.data = "\xc0\xaf";
  v.as.bytes.size = 2;
  T(ow_field_set(d, r, "bad", &v) == OW_INVALID);
  v.as.bytes.data = "x";
  v.as.bytes.size = OW_FIELD_LIMIT + 1;
  T(ow_field_set(d, r, "big", &v) == OW_CAPACITY);
  v.type = OW_NULL;
  T(ow_field_set(d, r, "\xed\xa0\x80", &v) == OW_INVALID);
  T(rev(d) == before);
  OK(ow_field_set(d, r, "match", &v));
  OK(ow_field_set(d, s, "match", &v));
  OK(ow_query_equal(d, "match", &v, &results, &n));
  T(n == 2 && results[0].value == r.value && results[1].value == s.value);
  ow_array_release(results);
  {
    char *buf = malloc(OW_FIELD_LIMIT);
    char name[32];
    T(buf);
    memset(buf, 42, OW_FIELD_LIMIT);
    v.type = OW_BINARY;
    v.as.bytes.data = buf;
    v.as.bytes.size = OW_FIELD_LIMIT;
    for (i = 0; i < 7; i++) {
      snprintf(name, sizeof(name), "large%zu", i);
      OK(ow_field_set(d, s, name, &v));
    }
    before = rev(d);
    T(ow_field_set(d, s, "large7", &v) == OW_CAPACITY);
    T(rev(d) == before);
    free(buf);
  }
  OK(ow_collection_create(d, &c));
  OK(ow_collection_create(d, &c2));
  OK(ow_collection_count(d, c, &n));
  T(n == 0);
  OK(ow_occurrence_insert(d, c, r, OW_BACK, z, &a));
  OK(ow_occurrence_insert(d, c, r, OW_FRONT, z, &b));
  OK(ow_occurrence_insert(d, c, r, OW_BEFORE, a, &e));
  OK(ow_occurrence_insert(d, c, r, OW_AFTER, a, &f));
  model[0] = b;
  model[1] = e;
  model[2] = a;
  model[3] = f;
  members(d, c, model, 4);
  OK(ow_occurrence_get(d, a, &gotc, &gotr));
  T(gotc.value == c.value && gotr.value == r.value);
  before = rev(d);
  OK(ow_occurrence_move(d, a, OW_BEFORE, a));
  OK(ow_occurrence_move(d, a, OW_AFTER, e));
  OK(ow_occurrence_move(d, b, OW_FRONT, z));
  OK(ow_occurrence_move(d, f, OW_BACK, z));
  T(rev(d) == before);
  OK(ow_occurrence_insert(d, c2, r, OW_BACK, z, &e));
  T(ow_record_delete(d, r) == OW_CONFLICT);
  T(ow_collection_delete(d, c) == OW_CONFLICT);
  before = rev(d);
  T(ow_occurrence_move(d, a, OW_BEFORE, e) == OW_INVALID);
  T(ow_occurrence_insert(d, c, r, OW_AFTER, e, &f) == OW_INVALID);
  T(f.value == 0);
  T(rev(d) == before);
  T(ow_occurrence_remove(d, (OwOccurrenceId){9999}) == OW_NOT_FOUND);
  T(ow_collection_count(d, (OwCollectionId){9999}, &n) == OW_NOT_FOUND &&
    n == 0);
  OK(ow_occurrence_remove(d, e));
  OK(ow_collection_delete(d, c2));
  /* Independent array model, fixed seed, all placements and repeated
   * membership. */
  {
    uint32_t seed = 1234567;
    size_t count = 4, step;
    for (step = 4; step < 200; ++step) {
      OK(ow_occurrence_insert(d, c, r, OW_BACK, z, &model[count++]));
    }
    for (step = 0; step < 4000; step++) {
      size_t pos, anchor, at;
      unsigned op, p;
      OwOccurrenceId id, aid = {0};
      OwStatus status;
      seed = seed * 1664525u + 1013904223u;
      op = seed % 3;
      seed = seed * 1664525u + 1013904223u;
      p = seed % 4;
      if (!count)
        op = 0;
      if (count == 256 && op == 0)
        op = 2;
      seed = seed * 1664525u + 1013904223u;
      pos = count ? seed % count : 0;
      seed = seed * 1664525u + 1013904223u;
      anchor = count ? seed % count : 0;
      if (!count && p >= 2)
        p = OW_BACK;
      if (p >= 2)
        aid = model[anchor];
      before = rev(d);
      if (op == 0) {
        status = ow_occurrence_insert(d, c, r, (OwPlacement)p, aid, &id);
        OK(status);
        at = p == OW_FRONT  ? 0
             : p == OW_BACK ? count
                            : anchor + (p == OW_AFTER);
        memmove(model + at + 1, model + at, (count - at) * sizeof(*model));
        model[at] = id;
        ++count;
        T(rev(d) == before + 1);
      } else if (op == 2) {
        OK(ow_occurrence_remove(d, model[pos]));
        memmove(model + pos, model + pos + 1,
                (count - pos - 1) * sizeof(*model));
        --count;
        T(rev(d) == before + 1);
      } else {
        int noop;
        id = model[pos];
        at = p == OW_FRONT  ? 0
             : p == OW_BACK ? count
                            : anchor + (p == OW_AFTER);
        noop = (p >= 2 && pos == anchor) || at == pos || at == pos + 1;
        OK(ow_occurrence_move(d, id, (OwPlacement)p, aid));
        if (!noop) {
          memmove(model + pos, model + pos + 1,
                  (count - pos - 1) * sizeof(*model));
          if (at > pos)
            --at;
          memmove(model + at + 1, model + at,
                  (count - 1 - at) * sizeof(*model));
          model[at] = id;
        }
        T(rev(d) == before + (!noop));
      }
      members(d, c, model, count);
    }
    while (count)
      OK(ow_occurrence_remove(d, model[--count]));
  }
  OK(ow_record_delete(d, r));
  OK(ow_collection_delete(d, c));
  ow_database_destroy(d);
  ow_database_destroy(NULL);
  puts("core types, ownership, failures, queries, revisions and 4000 model "
       "operations verified");
  return 0;
}
