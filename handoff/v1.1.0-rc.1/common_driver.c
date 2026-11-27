/* Public OrdWeave application calls, generated input, independent final model.
 * Receipts are emitted after timing; disk DBs stay in a caller-supplied sandbox. */
#include <ordweave.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <windows.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"database:%d %s\n",__LINE__,#x);exit(2);}}while(0)
#define OK(x) CHECK((x)==OW_OK)
typedef struct{char kind;size_t a,b;int after,cmp;}Op;
static double now(void){LARGE_INTEGER t,f;QueryPerformanceCounter(&t);QueryPerformanceFrequency(&f);return(double)t.QuadPart/f.QuadPart;}
static void verify(OwDatabase *db,OwCollectionId c,const OwOccurrenceId *map,const size_t *order,size_t n){
    OwOccurrenceId *got=NULL;size_t count=0;OK(ow_collection_members(db,c,&got,&count));CHECK(count==n);
    for(size_t i=0;i<n;++i){CHECK(got[i].value==map[order[i]].value);OwCollectionId cc;OwRecordId rr;OK(ow_occurrence_get(db,got[i],&cc,&rr));CHECK(cc.value==c.value && rr.value==order[i]%64+1);}
    ow_array_release(got);
}
int main(int argc,char **argv){
    CHECK(argc==3||argc==4);FILE *f=fopen(argv[1],"r");CHECK(f);size_t n,steps,final_n;CHECK(fscanf(f,"%zu%zu%zu",&n,&steps,&final_n)==3);CHECK(n<=2048&&steps<=1000&&final_n<=n+steps);
    size_t cap=n+steps+1;Op *ops=calloc(steps,sizeof(*ops));size_t *expected=calloc(cap,sizeof(*expected));OwOccurrenceId *map=calloc(cap,sizeof(*map));int *statuses=calloc(steps,sizeof(*statuses));CHECK(ops&&expected&&map&&statuses);
    for(size_t i=0;i<steps;++i)CHECK(fscanf(f," %c %zu %zu %d %d",&ops[i].kind,&ops[i].a,&ops[i].b,&ops[i].after,&ops[i].cmp)==5);
    for(size_t i=0;i<final_n;++i)CHECK(fscanf(f,"%zu",expected+i)==1);CHECK(fclose(f)==0);
    int persistent=!strcmp(argv[2],"persistent");CHECK(!persistent||argc==4);OwDatabase *db=NULL;double start=now();
    if(persistent)OK(ow_database_create_persistent(argv[3],&db));else OK(ow_database_create(&db));
    OwRecordId records[64];for(size_t i=0;i<64;++i){OK(ow_record_create(db,records+i));OwValue v={.type=OW_INT,.as.integer=(int64_t)i};OK(ow_field_set(db,records[i],"priority",&v));}
    OwCollectionId c;OK(ow_collection_create(db,&c));for(size_t i=0;i<n;++i)OK(ow_occurrence_insert(db,c,records[i%64],OW_BACK,(OwOccurrenceId){0},map+i));
    double setup=now()-start;uint64_t initial_rev;OK(ow_revision(db,&initial_rev));
    start=now();
    for(size_t i=0;i<steps;++i){Op o=ops[i];CHECK(o.a<cap&&o.b<cap);OwStatus s;
        if(o.kind=='C'){int cmp=8;s=ow_occurrence_compare(db,map[o.a],map[o.b],&cmp);CHECK(cmp==o.cmp);}
        else if(o.kind=='M')s=ow_occurrence_move(db,map[o.a],o.after?OW_AFTER:OW_BEFORE,map[o.b]);
        else if(o.kind=='I')s=ow_occurrence_insert(db,c,records[o.a%64],o.after?OW_AFTER:OW_BEFORE,map[o.b],map+o.a);
        else{CHECK(o.kind=='R');s=ow_occurrence_remove(db,map[o.a]);}
        statuses[i]=(int)s;CHECK(s==OW_OK);
    }
    double operations=now()-start;verify(db,c,map,expected,final_n);
    OwValue v;OK(ow_field_get(db,records[7],"priority",&v));CHECK(v.type==OW_INT&&v.as.integer==7);ow_value_release(&v);
    double checkpoint=0,reopen=0,history=0;
    if(persistent){
        start=now();OK(ow_checkpoint_create(db));checkpoint=now()-start;
        start=now();OwHistory *h=NULL;OK(ow_history_open(db,initial_rev,&h));OwOccurrenceId *old=NULL;size_t count=0;OK(ow_history_collection_members(h,c,&old,&count));CHECK(count==n);for(size_t i=0;i<n;++i)CHECK(old[i].value==i+1);ow_array_release(old);ow_history_close(h);history=now()-start;
        OK(ow_database_close(db));db=NULL;start=now();OK(ow_database_open(argv[3],&db));reopen=now()-start;verify(db,c,map,expected,final_n);
        OwHistoryRange range;OK(ow_verify_database(db,&range));
    }
    OK(ow_database_close(db));
    printf("summary,%.9f,%.9f,%.9f,%.9f,%.9f,%zu,%zu\n",setup,operations,checkpoint,reopen,history,n,steps);
    for(size_t i=0;i<steps;++i)printf("receipt,%zu,%c,%zu,%zu,%d,%d\n",i,ops[i].kind,ops[i].a,ops[i].b,ops[i].after,statuses[i]);
    free(ops);free(expected);free(map);free(statuses);return 0;
}
