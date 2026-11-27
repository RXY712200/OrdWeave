/* Diagnostic public LKS reproducer, not primary OrdWeave execution. */
#include "layerkeysort.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define T(x) do{if(!(x)){fprintf(stderr,"native FIRST_FAILURE %d %s\n",__LINE__,#x);exit(2);}}while(0)
#define OK(x) T((x)==LKS_STATUS_OK)
static int items[130];
static LksStatus assoc(void*item,uint64_t n,void*c,const void**out,size_t*size){(void)n;(void)c;*out=item;*size=sizeof(int);return LKS_STATUS_OK;}
static LksStatus resolve(const void*p,size_t n,uint64_t ordinal,void*c,void**out){(void)c;T(n==4&&ordinal<128);int id;memcpy(&id,p,4);T(id>=0&&id<128);*out=items+id;return LKS_STATUS_OK;}
int main(void){for(int i=0;i<130;i++)items[i]=i;
 for(int op=0;op<6;op++){
  size_t hits=0,done=0;
  for(size_t ordinal=1;ordinal<=32;ordinal++){
   LksOrder*o=lks_order_create();T(o);const LksOrderHandle*h[130];size_t count=op==0?64:128;
   for(size_t i=0;i<count;i++)OK(lks_order_insert_back(o,items+i,h+i));
   LksSnapshot*snap=NULL,*loaded=NULL;LksOrder*restored=NULL;LksSnapshotOptions opt={"research-v1",11,assoc,NULL};unsigned char*bytes=NULL;size_t size=0;
   if(op>=3){OK(lks_order_snapshot_capture(o,&opt,&snap));size=lks_snapshot_serialized_size(snap);bytes=malloc(size);T(bytes);OK(lks_snapshot_serialize(snap,bytes,size));}
   lks_alloc_test_fail_on_attempt(ordinal);LksStatus status;
   if(op==0)status=lks_order_insert_before(o,h[32],items+128,h+128);
   else if(op==1)status=lks_order_move_before(o,h[127],h[0]);
   else if(op==2)status=lks_order_snapshot_capture(o,&opt,&snap);
   else if(op==3)status=lks_snapshot_serialize(snap,bytes,size);
   else if(op==4)status=lks_snapshot_deserialize(bytes,size,&loaded);
   else status=lks_snapshot_restore_order(snap,resolve,NULL,&restored);
   int fired=lks_alloc_test_failure_triggered();size_t attempts=lks_alloc_test_get_attempt_count();lks_alloc_test_disable_failure();
   if(fired){hits++;T(status==LKS_STATUS_OUT_OF_MEMORY);T(lks_order_size(o)==count);size_t i=0;for(const LksOrderHandle*q=lks_order_first(o);q;q=lks_order_next(q),i++)T(lks_order_item(q)==items+i);T(i==count);}
   else {OK(status);done=1;if(op==5){T(lks_order_size(restored)==128);size_t i=0;for(const LksOrderHandle*q=lks_order_first(restored);q;q=lks_order_next(q),i++)T(lks_order_item(q)==items+i);}}
   printf("ordinal,op=%d,ordinal=%zu,triggered=%d,attempts=%zu,status=%d\n",op,ordinal,fired,attempts,status);
   lks_order_destroy(restored);lks_snapshot_destroy(loaded);lks_snapshot_destroy(snap);free(bytes);lks_order_destroy(o);T(lks_alloc_stats_get().live_blocks==0);
   if(done)break;
  }printf("summary,op=%d,faults=%zu,completed=%zu\n",op,hits,done);
  /* Capture may have >32 copy allocations; bounded coverage remains partial. */
 }
 return 0;
}
