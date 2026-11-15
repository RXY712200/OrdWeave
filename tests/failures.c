/* Dedicated white-box test translation unit: no hooks enter production builds.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static size_t fail_at = SIZE_MAX, calls;
static void *test_malloc(size_t n) {
  return calls++ == fail_at ? NULL : malloc(n);
}
static void *test_calloc(size_t n, size_t s) {
  return calls++ == fail_at ? NULL : calloc(n, s);
}
#define malloc test_malloc
#define calloc test_calloc
#include "../src/ordweave.c"
#undef malloc
#undef calloc
#define T(x)                                                                   \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "%d: %s\n", __LINE__, #x);                               \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)
#define OK(x) T((x) == OW_OK)
static void arm(size_t n) {
  calls = 0;
  fail_at = n;
}
static void disarm(void) { fail_at = SIZE_MAX; }
int main(void) {
  size_t k;
  OwDatabase *d;
  OwRecordId r, out;
  OwCollectionId c;
  OwOccurrenceId o;
  OwValue v = {0}, got;
  uint64_t revision;
  size_t n;
  for (k = 0; k < 2; k++) {
    d = (void *)1;
    arm(k);
    if (k == 0)
      T(ow_database_create(&d) == OW_NOMEM && !d);
    else {
      OK(ow_database_create(&d));
      ow_database_destroy(d);
    }
    disarm();
  }
  OK(ow_database_create(&d));
  OK(ow_record_create(d, &r));
  OK(ow_collection_create(d, &c));
  v.type = OW_STRING;
  v.as.bytes.data = "stored";
  v.as.bytes.size = 6;
  OK(ow_field_set(d, r, "existing", &v));
  revision = d->revision;
  for (k = 0; k < 3; k++) {
    arm(k);
    T(ow_field_set(d, r, "new", &v) == OW_NOMEM);
    disarm();
    T(d->revision == revision);
    T(ow_field_get(d, r, "new", &got) == OW_NOT_FOUND);
    OK(ow_field_get(d, r, "existing", &got));
    T(!memcmp(got.as.bytes.data, "stored", 6));
    ow_value_release(&got);
  }
  arm(0);
  T(ow_field_set(d, r, "existing",
                 &(OwValue){.type = OW_STRING, .as.bytes = {"changed", 7}}) ==
    OW_NOMEM);
  disarm();
  T(d->revision == revision);
  arm(0);
  T(ow_field_get(d, r, "existing", &got) == OW_NOMEM);
  disarm();
  T(got.type == OW_NULL);
  arm(0);
  T(ow_record_create(d, &out) == OW_NOMEM && !out.value);
  disarm();
  T(d->rid == 1 && d->revision == revision);
  arm(0);
  T(ow_collection_create(d, &(OwCollectionId){0}) == OW_NOMEM);
  disarm();
  T(d->cid == 1 && d->revision == revision);
  arm(0);
  T(ow_occurrence_insert(d, c, r, OW_BACK, (OwOccurrenceId){0}, &o) ==
        OW_NOMEM &&
    !o.value);
  disarm();
  T(d->oid == 0 && d->revision == revision);
  OK(ow_collection_count(d, c, &n));
  T(n == 0);
  OK(ow_occurrence_insert(d, c, r, OW_BACK, (OwOccurrenceId){0}, &o));
  {
    OwOccurrenceId *a = (void *)1;
    arm(0);
    T(ow_collection_members(d, c, &a, &n) == OW_NOMEM);
    disarm();
    T(!a && !n);
  }
  {
    OwRecordId *a = (void *)1;
    arm(0);
    T(ow_query_equal(d, "existing", &v, &a, &n) == OW_NOMEM);
    disarm();
    T(!a && !n);
  }
  {
    OwIndexed duplicate = {0};
    duplicate.id = r.value;
    T(ow_index_insert(d->records, &duplicate) == OW_CONFLICT);
    T(lks_managed_order_size(d->records) == 1);
  }
  d->rid = UINT64_MAX;
  T(ow_record_create(d, &out) == OW_CAPACITY && !out.value);
  d->rid = 1;
  d->cid = UINT64_MAX;
  T(ow_collection_create(d, &(OwCollectionId){0}) == OW_CAPACITY);
  d->cid = 1;
  d->oid = UINT64_MAX;
  T(ow_occurrence_insert(d, c, r, OW_BACK, (OwOccurrenceId){0},
                         &(OwOccurrenceId){0}) == OW_CAPACITY);
  d->oid = 1;
  d->revision = UINT64_MAX;
  OK(ow_occurrence_move(d, o, OW_FRONT, (OwOccurrenceId){0}));
  OK(ow_field_set(d, r, "existing", &v));
  OK(ow_field_remove(d, r, "absent"));
  T(ow_occurrence_remove(d, o) == OW_CAPACITY);
  T(ow_record_create(d, &out) == OW_CAPACITY);
  T(ow_field_remove(d, r, "existing") == OW_CAPACITY);
  T(d->revision == UINT64_MAX);
  d->revision = revision + 1;
  ow_database_destroy(d);
  puts("allocation failures, safe outputs and counter exhaustion verified");
  return 0;
}
