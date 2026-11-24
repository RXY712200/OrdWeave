/* Inject into upstream Full allocations inside actual OrdWeave operations.
 * This complements OrdWeave's own malloc failpoints without changing sources. */
#include <ordweave.h>
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"db-oom:%d %s\n",__LINE__,#x);exit(2);}}while(0)
int main(void){
    puts("operation,fail_ordinal,result,full_attempts,revision_preserved,full_live_delta");
    for(int operation=0;operation<2;++operation)for(size_t at=1;at<200;++at){
        lks_alloc_test_disable_failure();OwDatabase *db=NULL;CHECK(ow_database_create(&db)==OW_OK);OwRecordId record;OwCollectionId collection;OwOccurrenceId ids[32],out={999};
        CHECK(ow_record_create(db,&record)==OW_OK&&ow_collection_create(db,&collection)==OW_OK);
        for(size_t i=0;i<32;++i)CHECK(ow_occurrence_insert(db,collection,record,OW_BACK,(OwOccurrenceId){0},ids+i)==OW_OK);
        uint64_t revision,after;CHECK(ow_revision(db,&revision)==OW_OK);size_t live=lks_alloc_stats_get().live_bytes;
        lks_alloc_test_fail_on_attempt(at);
        OwStatus status=operation?ow_occurrence_remove(db,ids[16]):ow_occurrence_insert(db,collection,record,OW_BEFORE,ids[16],&out);
        size_t attempts=lks_alloc_test_get_attempt_count();lks_alloc_test_disable_failure();CHECK(ow_revision(db,&after)==OW_OK);
        if(status!=OW_OK){
            CHECK(status==OW_NOMEM&&attempts==at&&after==revision&&lks_alloc_stats_get().live_bytes==live);
            if(!operation)CHECK(out.value==0);
            OwOccurrenceId *members=NULL;size_t n=0;CHECK(ow_collection_members(db,collection,&members,&n)==OW_OK&&n==32);
            for(size_t i=0;i<n;++i){CHECK(members[i].value==ids[i].value);OwCollectionId c;OwRecordId r;CHECK(ow_occurrence_get(db,ids[i],&c,&r)==OW_OK&&r.value==record.value&&c.value==collection.value);}
            ow_array_release(members);
        }else CHECK(after==revision+1);
        printf("%s,%zu,%d,%zu,%d,%lld\n",operation?"remove":"insert",at,status,attempts,after==revision,(long long)lks_alloc_stats_get().live_bytes-(long long)live);
        CHECK(ow_database_close(db)==OW_OK&&lks_alloc_stats_get().live_bytes==0);
        if(status==OW_OK)break;CHECK(at<199);
    }
    return 0;
}
