/* Diagnostic only: work counters are observations, never oracle ground truth. */
#include "layerkeysort.h"
#include "lks_order_internal.h"
#include "lks_alloc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define T(x) do{if(!(x)){fprintf(stderr,"structure:%d\n",__LINE__);exit(2);}}while(0)
static int items[10000],seq[512];static const LksOrderHandle*h[10000];
int main(void){LksOrder*o=lks_order_create();T(o);size_t n=32,next=32;
 for(int i=0;i<10000;i++)items[i]=i;
 for(size_t i=0;i<n;i++){seq[i]=i;T(lks_order_insert_back(o,items+i,h+i)==0);}
 size_t splits=0,merges=0,red=0,rot=0,cross=0;
 for(size_t step=0;step<8192;step++){
  if(step<4096){size_t pos=n/2;
   if(step%256<128){int id=next++;T(lks_order_insert_before(o,h[seq[pos]],items+id,h+id)==0);memmove(seq+pos+1,seq+pos,(n-pos)*sizeof(int));seq[pos]=id;n++;}
   else{T(lks_order_remove(o,h[seq[pos]],NULL)==0);memmove(seq+pos,seq+pos+1,(n-pos-1)*sizeof(int));n--;}
  }else{if(step==4096){while(n<256){seq[n]=next;T(lks_order_insert_back(o,items+next,h+next)==0);next++;n++;}}int id=seq[0];T(lks_order_move_back(o,h[id])==0);memmove(seq,seq+1,(n-1)*sizeof(int));seq[n-1]=id;}
  splits+=o->work.splits;merges+=o->work.merges;red+=o->work.redistributions;rot+=o->work.rotations;cross+=o->work.cross_block_moves;
  T(lks_order_internal_valid(o));size_t i=0;
  for(const LksOrderHandle*q=lks_order_first(o);q;q=lks_order_next(q),i++)T(i<n&&lks_order_item(q)==items+seq[i]);T(i==n);
 }
 printf("actual-paths,splits=%zu,merges=%zu,redistributions=%zu,rotations=%zu,cross=%zu,tag_repairs=%zu,tag_fallbacks=%zu\n",splits,merges,red,rot,cross,o->tag_repairs,o->tag_fallbacks);lks_order_destroy(o);T(lks_alloc_stats_get().live_blocks==0);
 o=lks_order_create();T(o);T(lks_order_insert_back(o,items,h)==0);
 for(size_t i=1;i<8192;i++)T(lks_order_insert_before(o,h[0],items+i,h+i)==0);
 size_t at=1;for(const LksOrderHandle*q=lks_order_first(o);q;q=lks_order_next(q)){T(lks_order_item(q)==items+(at==8192?0:at));at++;}T(at==8193);
 printf("fixed-anchor-hot,live=8192,tag_repairs=%zu,tag_fallbacks=%zu\n",o->tag_repairs,o->tag_fallbacks);lks_order_destroy(o);T(lks_alloc_stats_get().live_blocks==0);return 0;
}
