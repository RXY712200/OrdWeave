#include "ordweave.h"
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#define T(x) do{if(!(x)){fprintf(stderr,"FIRST_FAILURE line=%d step=%zu %s\n",__LINE__,step,#x);exit(2);}}while(0)
static size_t step,calls,mutations,noops,rejected,queries,setup;
#define OK(x) do{calls++;T((x)==OW_OK);}while(0)
static uint32_t rng;
static uint32_t random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static uint64_t revision(OwDatabase*d){uint64_t v;OK(ow_revision(d,&v));return v;}
static uint64_t rev(OwDatabase*d){return revision(d);}
static void full(OwDatabase*d,OwCollectionId c,const uint64_t*a,size_t n){
 OwOccurrenceId*out=NULL;size_t z;OK(ow_collection_members(d,c,&out,&z));T(z==n);
 for(size_t i=0;i<n;i++){T(out[i].value==a[i]);OwCollectionId cc;OwRecordId rr;OK(ow_occurrence_get(d,out[i],&cc,&rr));T(cc.value==c.value&&rr.value==1);}ow_array_release(out);
}
static void put(uint64_t*a,size_t*n,size_t at,uint64_t id){memmove(a+at+1,a+at,(*n-at)*8);a[at]=id;(*n)++;}
static uint64_t take(uint64_t*a,size_t*n,size_t at){uint64_t id=a[at];memmove(a+at,a+at+1,(*n-at-1)*8);(*n)--;return id;}
static void selftest(void){uint64_t a[8]={1,2,3};size_t n=3;put(a,&n,1,4);T(n==4&&a[1]==4&&a[2]==2);T(take(a,&n,2)==2);T(n==3&&a[2]==3);uint64_t wrong[]={1,3,4};T(memcmp(a,wrong,24)!=0);puts("oracle hand fixture and deliberately altered expectation rejected");}
static void memory(unsigned seed,size_t steps,const char*mix,const char*trace,int pattern){
 rng=seed;OwDatabase*d;OwRecordId r;OwCollectionId c,other;OwOccurrenceId id;
 OK(ow_database_create(&d));OK(ow_record_create(d,&r));OK(ow_collection_create(d,&c));OK(ow_collection_create(d,&other));
 uint64_t a[4096];size_t n=0;uint64_t high=0,rv=3;
 for(size_t i=0;i<(pattern==3?32:1024);i++){OK(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&id));T(id.value==++high);a[n++]=id.value;rv++;}setup=calls;FILE*f=fopen(trace,"wb");T(f);
 for(step=0;step<steps;step++){
  unsigned x=random32()%100;size_t i=random32()%n,j=random32()%n;unsigned kind;
  if(!strcmp(mix,"read"))kind=x<10?0:x<20?1:x<35?2:x<45?3:4;
  else if(!strcmp(mix,"move"))kind=x<5?0:x<10?1:x<80?2:x<90?3:4;
  else if(!strcmp(mix,"modify"))kind=x<35?0:x<70?1:x<90?2:x<95?3:4;
  else kind=x<25?0:x<50?1:x<75?2:x<85?3:4;
  if(pattern){kind=step%(pattern==3?128:256)<(pattern==3?64:128)?0:1;i=n/2;j=pattern==1?n/2:n-1;}
  if(n<(pattern==3?2:512))kind=0;if(n>(pattern==3?256:1536))kind=1;
  unsigned p=random32()%4;uint64_t row[]={step,kind,i,j,p};T(fwrite(row,sizeof(row),1,f)==1);
  if(kind==0){size_t at=p==0?0:p==1?n:j+(p==3);OwPlacement place=p==0?OW_FRONT:p==1?OW_BACK:p==2?OW_BEFORE:OW_AFTER;uint64_t anchor=p<2?0:a[j];OK(ow_occurrence_insert(d,c,r,place,(OwOccurrenceId){anchor},&id));T(id.value==++high);put(a,&n,at,id.value);rv++;mutations++;}
  else if(kind==1){uint64_t old=a[i];OK(ow_occurrence_remove(d,(OwOccurrenceId){old}));take(a,&n,i);rv++;mutations++;}
  else if(kind==2){size_t target=p==0?0:p==1?n-1:j;int unchanged=p==0?i==0:p==1?i==n-1:i==j||(p==2?i+1==j:j+1==i);uint64_t anchor=p<2?0:a[j],moved=a[i];OK(ow_occurrence_move(d,(OwOccurrenceId){moved},p==0?OW_FRONT:p==1?OW_BACK:p==2?OW_BEFORE:OW_AFTER,(OwOccurrenceId){anchor}));
   if(!unchanged){take(a,&n,i);if(p==0)target=0;else if(p==1)target=n;else{target=j-(i<j)+(p==3);}put(a,&n,target,moved);rv++;mutations++;}else noops++;
  }else if(kind==3){calls++;T(ow_occurrence_insert(d,other,r,OW_BEFORE,(OwOccurrenceId){a[j]},&id)==OW_INVALID&&!id.value);rejected++;}
  else{int order=9;OK(ow_occurrence_compare(d,(OwOccurrenceId){a[i]},(OwOccurrenceId){a[j]},&order));T((order>0)-(order<0)==(i>j)-(i<j));queries++;}
  T(revision(d)==rv);if(step%4096==0||steps<256)full(d,c,a,n);
  if(step%50000==0){printf("progress,%zu,%zu\n",step,n);fflush(stdout);}
 }
 full(d,c,a,n);T(fclose(f)==0);calls++;T(ow_record_delete(d,r)==OW_CONFLICT);rejected++;
 OwOccurrenceId*saved;size_t sn;OK(ow_collection_members(d,c,&saved,&sn));ow_database_destroy(d);for(size_t i=0;i<sn;i++)T(saved[i].value==a[i]);ow_array_release(saved);
 printf("result,memory,%u,%zu,setup=%zu,public=%zu,mutations=%zu,noops=%zu,rejected=%zu,queries=%zu,live=%zu,high=%llu\n",seed,steps,setup,calls,mutations,noops,rejected,queries,n,(unsigned long long)high);
}
static void capacity(size_t nr,size_t no,size_t nc){
 OwDatabase*d;OK(ow_database_create(&d));OwRecordId*r=calloc(nr,sizeof(*r));OwCollectionId*c=calloc(nc,sizeof(*c));T(r&&c);OwOccurrenceId id;
 for(size_t i=0;i<nr;i++){OK(ow_record_create(d,r+i));T(r[i].value==i+1);}
 for(size_t i=0;i<nc;i++){OK(ow_collection_create(d,c+i));T(c[i].value==i+1);}
 for(size_t i=0;i<no;i++){OK(ow_occurrence_insert(d,c[i%nc],r[i%nr],OW_BACK,(OwOccurrenceId){0},&id));T(id.value==i+1);}
 for(size_t k=0;k<nc;k++){OwOccurrenceId*ids;size_t n;OK(ow_collection_members(d,c[k],&ids,&n));T(n==(no/nc)+(k<no%nc));for(size_t i=0;i<n;i++){uint64_t expected=k+i*nc+1;T(ids[i].value==expected);OwCollectionId cc;OwRecordId rr;OK(ow_occurrence_get(d,ids[i],&cc,&rr));T(cc.value==k+1&&rr.value==(expected-1)%nr+1);}ow_array_release(ids);}
 T(revision(d)==nr+nc+no);calls++;T(ow_record_delete(d,r[0])==OW_CONFLICT);
 for(size_t i=0;i<no;i++)OK(ow_occurrence_remove(d,(OwOccurrenceId){i+1}));
 for(size_t i=0;i<nc;i++)OK(ow_collection_delete(d,c[i]));for(size_t i=0;i<nr;i++)OK(ow_record_delete(d,r[i]));ow_database_destroy(d);free(r);free(c);
 printf("result,capacity,records=%zu,occurrences=%zu,collections=%zu,largest=%zu,public=%zu\n",nr,no,nc,(no+nc-1)/nc,calls);
}
/* Existing hand-authored all-six-type expectations are independently specified
 * arrays, not derived from subject sorting. Fresh execution is supplemental. */
#include "../../tests/range_cases.inc"
static int cmp_id_value(const void*a,const void*b){const int64_t*x=a,*y=b;return x[1]!=y[1]?(x[1]>y[1])-(x[1]<y[1]):(x[0]>y[0])-(x[0]<y[0]);}
static void query(void){
 range_contracts();OwDatabase*d[2];int64_t vals[8192],expected[8192][2];OwRecordId r;size_t count[2];OwRecordId*out[2];
 for(int j=0;j<2;j++){OK(ow_database_create(d+j));for(size_t i=0;i<8192;i++){OK(ow_record_create(d[j],&r));T(r.value==i+1);OwValue v={.type=OW_INT,.as.integer=(int64_t)((i*17)%257)-128};OK(ow_field_set(d[j],r,"key",&v));vals[i]=v.as.integer;}}
 OK(ow_field_index_enable(d[1],"key",8192,4));rng=43;
 for(step=0;step<10000;step++){
  size_t i=random32()%8192;OwValue v={.type=OW_INT,.as.integer=(int64_t)(random32()%513)-256};vals[i]=v.as.integer;
  for(int j=0;j<2;j++)OK(ow_field_set(d[j],(OwRecordId){i+1},"key",&v));
  OwValue lo={.type=OW_INT,.as.integer=(int64_t)(random32()%257)-128},hi={.type=OW_INT,.as.integer=lo.as.integer+17};int li=step%2,ui=(step/2)%2;size_t n=0;
  for(size_t k=0;k<8192;k++)if((vals[k]>lo.as.integer||(li&&vals[k]==lo.as.integer))&&(vals[k]<hi.as.integer||(ui&&vals[k]==hi.as.integer))){expected[n][0]=k+1;expected[n++][1]=vals[k];}
  qsort(expected,n,sizeof(expected[0]),cmp_id_value);
  for(int j=0;j<2;j++){OK(ow_query_range(d[j],"key",OW_INT,&lo,li,&hi,ui,out+j,count+j));T(count[j]==n);for(size_t k=0;k<n;k++)T(out[j][k].value==(uint64_t)expected[k][0]);ow_array_release(out[j]);}
  if(step%64==0){for(int j=0;j<2;j++){OK(ow_query_range(d[j],"key",OW_INT,NULL,0,NULL,1,out+j,count+j));T(count[j]==8192);ow_array_release(out[j]);}T(revision(d[0])==revision(d[1]));}
  if(step%100==0){OK(ow_field_index_disable(d[1]));OK(ow_field_index_enable(d[1],"key",8192,4));}
 }ow_database_destroy(d[0]);ow_database_destroy(d[1]);printf("result,queries,steps=10000,public=%zu\n",calls);
}
static void persistent(const char*path,size_t nr,size_t no,size_t nc,size_t cycles){
 OwDatabase*d;OwRecordId r;OwCollectionId c={1};OwOccurrenceId o;uint64_t*a=calloc(no,8);T(a);OK(ow_database_create_persistent(path,&d));
 for(size_t i=0;i<nr;i++){OK(ow_record_create(d,&r));OwValue v={.type=OW_INT,.as.integer=0};OK(ow_field_set(d,r,"counter",&v));}
 for(size_t i=0;i<nc;i++)OK(ow_collection_create(d,&c));c.value=1;
 for(size_t i=0;i<no;i++){OK(ow_occurrence_insert(d,c,(OwRecordId){1},OW_BACK,(OwOccurrenceId){0},&o));T(o.value==i+1);a[i]=o.value;}
 uint64_t rv=nr*2+nc+no;
 for(step=0;step<cycles;step++){
  OK(ow_field_index_enable(d,"counter",nr,8));OwValue v={.type=OW_INT,.as.integer=(int64_t)step+1};OK(ow_field_set(d,(OwRecordId){1},"counter",&v));rv++;
  uint64_t id=take(a,&no,0);put(a,&no,no,id);OK(ow_occurrence_move(d,(OwOccurrenceId){id},OW_BACK,(OwOccurrenceId){0}));rv++;full(d,c,a,no);T(revision(d)==rv);
  OK(ow_checkpoint_create(d));OwHistory*h;OK(ow_history_open(d,rv,&h));OwOccurrenceId*ids;size_t n;OK(ow_history_collection_members(h,c,&ids,&n));T(n==no);for(size_t i=0;i<n;i++)T(ids[i].value==a[i]);ow_array_release(ids);OwValue got;OK(ow_history_field_get(h,(OwRecordId){1},"counter",&got));T(got.as.integer==(int64_t)step+1);ow_value_release(&got);ow_history_close(h);
  OK(ow_database_close(d));OK(ow_database_open(path,&d));full(d,c,a,no);T(revision(d)==rv);OK(ow_field_get(d,(OwRecordId){1},"counter",&got));T(got.as.integer==(int64_t)step+1);ow_value_release(&got);
  if(step%10==0){printf("progress,cycle,%zu\n",step);fflush(stdout);}
 }
 OwHistoryRange range;OK(ow_history_policy(d,OW_RETAIN_MANUAL,0));OK(ow_history_policy(d,OW_RETAIN_FOREVER,0));OK(ow_history_policy(d,OW_RETAIN_LAST_N,3));OK(ow_history_cleanup(d,&range));OK(ow_verify_database(d,&range));
 char backup[512];snprintf(backup,sizeof(backup),"%s-backup",path);OK(ow_backup_create(d,backup));OwDatabase*b;OK(ow_database_open(backup,&b));full(b,c,a,no);OK(ow_record_create(b,&r));T(r.value==nr+1);T(revision(d)==rv);OK(ow_database_close(b));OK(ow_database_close(d));free(a);printf("result,persistence,cycles=%zu,public=%zu,revision=%llu,earliest=%llu\n",cycles,calls,(unsigned long long)rv,(unsigned long long)range.earliest_revision);
}
int main(int argc,char**argv){selftest();T(argc>=2);
 if(!strcmp(argv[1],"memory")){T(argc==7);memory(atoi(argv[2]),strtoull(argv[3],0,10),argv[4],argv[5],atoi(argv[6]));}
 else if(!strcmp(argv[1],"negative")){OwDatabase*d;OwRecordId r;OwCollectionId c;OwOccurrenceId o;OK(ow_database_create(&d));OK(ow_record_create(d,&r));OK(ow_collection_create(d,&c));for(int i=0;i<3;i++)OK(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&o));uint64_t right[]={1,2,3},wrong[]={1,3,2};full(d,c,right,3);full(d,c,wrong,3);}
 else if(!strcmp(argv[1],"capacity")){T(argc==5);capacity(strtoull(argv[2],0,10),strtoull(argv[3],0,10),strtoull(argv[4],0,10));}
 else if(!strcmp(argv[1],"query"))query();
 else if(!strcmp(argv[1],"persistence")){T(argc==7);persistent(argv[2],strtoull(argv[3],0,10),strtoull(argv[4],0,10),strtoull(argv[5],0,10),strtoull(argv[6],0,10));}
 else return 3;
#ifdef RESOURCE
 extern void memory_report(const char*);memory_report("teardown");
#endif
 return 0;
}
