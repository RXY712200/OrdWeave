#include "ordweave.h"
#include <stdio.h>
#include <stdlib.h>
#define T(x) do{if(!(x)){fprintf(stderr,"large-order:%d\n",__LINE__);exit(2);}}while(0)
#define OK(x) T((x)==OW_OK)
int main(void){OwDatabase*d;OwRecordId r;OwCollectionId c;OwOccurrenceId id;OK(ow_database_create(&d));OK(ow_record_create(d,&r));OK(ow_collection_create(d,&c));
 for(size_t i=0;i<100000;i++){OK(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&id));T(id.value==i+1);}
 for(size_t step=0;step<1024;step++){
  OK(ow_occurrence_move(d,(OwOccurrenceId){step+1},OW_BACK,(OwOccurrenceId){0}));int sign;OK(ow_occurrence_compare(d,(OwOccurrenceId){step+2},(OwOccurrenceId){step+1},&sign));T(sign<0);
  uint64_t before,after;OK(ow_revision(d,&before));OK(ow_occurrence_move(d,(OwOccurrenceId){step+1},OW_BACK,(OwOccurrenceId){0}));OK(ow_revision(d,&after));T(before==after);
  if(step%128==127){OwOccurrenceId*out;size_t n;OK(ow_collection_members(d,c,&out,&n));T(n==100000);for(size_t i=0;i<n;i++)T(out[i].value==(i+step+1)%100000+1);ow_array_release(out);}
 }ow_database_destroy(d);puts("large Order100000,1024long moves,1024endpoint noops,8 independent full rotations");return 0;}
