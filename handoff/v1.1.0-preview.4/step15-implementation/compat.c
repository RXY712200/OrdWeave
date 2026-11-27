#include "ordweave.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef CANDIDATE
static int measuring;static size_t allocations;
void *__real_malloc(size_t n);
void *__wrap_malloc(size_t n){if(measuring)allocations++;return __real_malloc(n);}
static void cache_state(OwDatabase*d,int enabled){
 OwRecordId *ids;size_t n;allocations=0;measuring=1;
 OwStatus s=ow_query_range(d,"counter",OW_INT,NULL,1,NULL,1,&ids,&n);
 measuring=0;if(s!=OW_OK||n!=1||allocations!=(enabled?1:2)){fprintf(stderr,"cache allocation proof %d %zu %zu\n",enabled,allocations,n);exit(9);}ow_array_release(ids);
}
#endif
#define T(x) do {if(!(x)){fprintf(stderr,"compat:%d %s\n",__LINE__,#x);exit(2);}}while(0)
#define OK(x) T((x)==OW_OK)
static void state(OwDatabase *d) {
  uint64_t rev;OwValue v;OwOccurrenceId *ids;size_t n;
  OK(ow_revision(d,&rev));T(rev==40);
  OK(ow_field_get(d,(OwRecordId){1},"counter",&v));T(v.type==OW_INT&&v.as.integer==29);ow_value_release(&v);
  OK(ow_collection_members(d,(OwCollectionId){1},&ids,&n));T(n==6);
  for(size_t i=0;i<n;i++){OwCollectionId c;OwRecordId r;T(ids[i].value==i+1);OK(ow_occurrence_get(d,ids[i],&c,&r));T(c.value==1&&r.value==1);}ow_array_release(ids);
  OwHistoryRange range;OK(ow_verify_database(d,&range));T(range.current_revision==40&&range.earliest_revision>0&&range.earliest_revision<=36);
  for(uint64_t i=range.earliest_revision;i<=40;i++){
    OwHistory *h;uint64_t got;OK(ow_history_open(d,i,&h));OK(ow_history_revision(h,&got));T(got==i);
    if(i==40){OK(ow_history_field_get(h,(OwRecordId){1},"counter",&v));T(v.type==OW_INT&&v.as.integer==29);ow_value_release(&v);}
    ow_history_close(h);
  }
#ifdef CANDIDATE
  cache_state(d,0);OK(ow_field_index_enable(d,"counter",4,8));cache_state(d,1);
#endif
  OK(ow_checkpoint_create(d));OK(ow_verify_database(d,&range));
}
int main(int argc,char **argv){
 T(argc==4);OwDatabase *d=NULL;OK(ow_database_open(argv[1],&d));state(d);
 OK(ow_backup_create(d,argv[2]));OK(ow_database_close(d));
 OK(ow_database_open(argv[2],&d));state(d);OK(ow_database_close(d));
 /* Leave another independent backup for the originating build to reopen. */
 OK(ow_database_open(argv[2],&d));
#ifdef CANDIDATE
cache_state(d,0);OK(ow_field_index_enable(d,"counter",4,8));cache_state(d,1);
#endif
OK(ow_backup_create(d,argv[3]));OK(ow_database_close(d));
 puts("revision40, six ordered permanent occurrences, fields, retained views, checkpoint and independent backup verified");return 0;
}
