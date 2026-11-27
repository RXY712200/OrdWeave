/* Dedicated white-box test translation unit: no hooks enter production builds.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/lks_adapter.h"
static OwStatus order_insert_failure = OW_OK;
static LksStatus order_remove_failure = LKS_STATUS_OK;
static OwStatus test_order_insert(LksOrder *order, void *item, OwPlacement place,
    const LksOrderHandle *anchor, const LksOrderHandle **out) {
  if (order_insert_failure != OW_OK) { *out=NULL;return order_insert_failure; }
  return ow_order_insert(order,item,place,anchor,out);
}
static LksStatus test_order_remove(LksOrder *order,const LksOrderHandle *handle,
    void **out) {
  if(order_remove_failure!=LKS_STATUS_OK) {if(out)*out=NULL;return order_remove_failure;}
  return lks_order_remove(order,handle,out);
}
static size_t fail_at = SIZE_MAX, calls;
static void *test_malloc(size_t n) {
  return calls++ == fail_at ? NULL : malloc(n);
}
static void *test_calloc(size_t n, size_t s) {
  return calls++ == fail_at ? NULL : calloc(n, s);
}
#define malloc test_malloc
#define calloc test_calloc
#define ow_order_insert test_order_insert
#define lks_order_remove test_order_remove
#include "../src/ordweave.c"
#undef ow_order_insert
#undef lks_order_remove
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
static void radix_contracts(void) {
  const uint64_t edges[]={1,15,16,255,256,UINT64_C(1)<<32,
                         UINT64_C(1)<<63,UINT64_MAX-1,UINT64_MAX};
  OwIndexed items[9]={{0}};OwOccIndex *index=occ_create();T(index);
  for(size_t i=0;i<9;i++) {
    items[i].id=edges[8-i];OwOccPlan plan;
    OK(occ_prepare(index,items[i].id,&plan));occ_abort(&plan);
    T(index->count==i && !occ_find(index,items[i].id));
    OK(occ_prepare(index,items[i].id,&plan));occ_commit(index,&plan,items+i);
    T(occ_find(index,items[i].id)==items+i);
    T(occ_prepare(index,items[i].id,&plan)==OW_CONFLICT);
  }
  OwOccIterator it;OwIndexed *entry;size_t n=0;
  for(entry=occ_first(index,&it);entry;entry=occ_next(&it))T(entry->id==edges[n++]);
  T(n==9);
  /* Deterministic sparse permutation; collapse every branch, including root. */
  for(size_t i=0;i<9;i++){size_t j=(i*5)%9;occ_remove(index,items[j].id);T(!occ_find(index,items[j].id));}
  T(!index->root && !index->count);occ_destroy(index);
  /* Exercise every new core preparation allocation at page/prefix boundaries. */
  const uint64_t ids[]={16,256,UINT64_C(1)<<63,UINT64_MAX};
  for(size_t edge=0;edge<4;edge++)for(size_t ordinal=0;ordinal<4;ordinal++) {
    OwDatabase *d;OwRecordId r;OwCollectionId c;OwOccurrenceId first,out;
    OK(ow_database_create(&d));OK(ow_record_create(d,&r));OK(ow_collection_create(d,&c));
    OK(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&first));
    Occurrence *old=occ_find(d->occurrences,first.value);const LksOrderHandle *h=old->handle;
    OwOccNode *root=d->occurrences->root;uint64_t rev=d->revision;d->oid=ids[edge]-1;
    arm(ordinal);OwStatus status=ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&out);disarm();
    if(ordinal<3){T(status==OW_NOMEM&&!out.value);T(d->oid==ids[edge]-1&&d->revision==rev);T(d->occurrences->root==root&&d->occurrences->count==1);T(old->handle==h&&old->record->refs==1);}
    else{OK(status);T(out.value==ids[edge]&&d->revision==rev+1);arm(0);OK(ow_occurrence_remove(d,out));T(calls==0);disarm();}
    order_insert_failure=OW_CAPACITY;
    if(d->oid!=UINT64_MAX)T(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&out)==OW_CAPACITY);
    order_insert_failure=OW_OK;
    order_remove_failure=LKS_STATUS_CAPACITY_LIMIT;
    T(ow_occurrence_remove(d,first)==OW_CAPACITY);order_remove_failure=LKS_STATUS_OK;
    T(occ_find(d->occurrences,first.value)==old&&old->handle==h&&old->record->refs==1);
    d->revision=UINT64_MAX;OK(ow_occurrence_move(d,first,OW_FRONT,(OwOccurrenceId){0}));
    T(ow_occurrence_remove(d,first)==OW_CAPACITY);ow_database_destroy(d);
  }
}
#ifdef OW_UPSTREAM_ALLOC_TEST
#include "../third_party/LayerKeySort/src/lks_alloc_internal.h"
static void upstream_failures(void) {
  for(size_t ordinal=1;ordinal<16;ordinal++) {
    OwDatabase *d;OwRecordId r;OwCollectionId c;OwOccurrenceId first,out;
    OK(ow_database_create(&d));OK(ow_record_create(d,&r));OK(ow_collection_create(d,&c));
    OK(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&first));
    for(unsigned i=1;i<64;i++)OK(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&out));
    d->oid=255;
    Occurrence *old=occ_find(d->occurrences,1);const LksOrderHandle *h=old->handle;
    uint64_t rev=d->revision;OwOccNode *root=d->occurrences->root;
    lks_alloc_test_fail_on_attempt(ordinal);
    OwStatus status=ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&out);
    int triggered=lks_alloc_test_failure_triggered();lks_alloc_test_disable_failure();
    if(triggered){T(status==OW_NOMEM&&!out.value&&d->oid==255&&d->revision==rev);T(d->occurrences->root==root&&d->occurrences->count==64&&old->handle==h&&old->record->refs==64);}
    else{OK(status);T(out.value==256);}
    ow_database_destroy(d);T(lks_alloc_stats_get().live_blocks==0);
    if(!triggered){printf("upstream insert allocation sweep: %zu failing ordinals\n",ordinal-1);return;}
  }
  T(0);
}
#endif
static void import_preparation_failures(void) {
  OwDatabase *source,*copy;OwRecordId r;OwCollectionId c;OwOccurrenceId o;
  unsigned char id[16]={0},anchor[32]={0},*bytes;size_t size;
  OK(ow_database_create(&source));OK(ow_record_create(source,&r));
  OK(ow_collection_create(source,&c));
  OwValue v={.type=OW_INT,.as.integer=17};
  OK(ow_field_set(source,r,"a",&v));OK(ow_field_set(source,r,"b",&v));
  OK(ow_occurrence_insert(source,c,r,OW_BACK,(OwOccurrenceId){0},&o));
  OK(ow_core_export(source,id,1,anchor,&bytes,&size));
  size_t ordinal;
  for(ordinal=0;ordinal<64;ordinal++) {
    OK(ow_database_create(&copy));arm(ordinal);
    OwStatus status=ow_core_import(copy,bytes,size,id);disarm();
    if(status!=OW_OK && status!=OW_NOMEM) {
      fprintf(stderr,"first import failure: ordinal=%zu status=%d\n",ordinal,status);
      FILE *f=fopen("first-import-failure.cp","wb");
      if(f){fwrite(bytes,1,size,f);fclose(f);}T(0);
    }
    if(status==OW_OK)T(ow_core_same_state(source,copy));
    ow_database_destroy(copy);if(status==OW_OK)break;
  }
  T(ordinal<64);
  /* High waters sum to3, but the actual state requires5 mutations. */
  ow_put(bytes+48,4,8);
  OK(ow_sha256(bytes,size-32,NULL,0,bytes+size-32));
  OK(ow_database_create(&copy));T(ow_core_import(copy,bytes,size,id)==OW_CORRUPT);
  ow_database_destroy(copy);free(bytes);ow_database_destroy(source);
}
int main(void) {
  import_preparation_failures();
  radix_contracts();
#ifdef OW_UPSTREAM_ALLOC_TEST
  upstream_failures();
#endif
  size_t k;
  OwDatabase *d;
  OwRecordId r, out;
  OwCollectionId c;
  OwOccurrenceId o;
  OwValue v = {0}, got;
  uint64_t revision;
  size_t n;
  for (k = 0; k < 3; k++) {
    d = (void *)1;
    arm(k);
    if (k < 2)
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
