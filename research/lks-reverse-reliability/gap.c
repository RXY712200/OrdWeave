#include "ordweave.h"
#include <stdio.h>
#include <stdlib.h>
#define T(x) do{if(!(x)){fprintf(stderr,"gap:%d\n",__LINE__);exit(2);}}while(0)
#define OK(x) T((x)==OW_OK)
int main(void){for(int mode=0;mode<2;mode++){OwDatabase*d;OwRecordId r;OwCollectionId c;OwOccurrenceId id,anchor;OK(ow_database_create(&d));OK(ow_record_create(d,&r));OK(ow_collection_create(d,&c));OK(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&anchor));T(anchor.value==1);
 for(size_t i=2;i<=8193;i++){OK(ow_occurrence_insert(d,c,r,mode?OW_AFTER:OW_BEFORE,anchor,&id));T(id.value==i);if(mode)anchor=id;
  if(i%128==0||i==8193){OwOccurrenceId*out;size_t n;OK(ow_collection_members(d,c,&out,&n));T(n==i);for(size_t k=0;k<n;k++)T(out[k].value==(mode?k+1:k==n-1?1:k+2));ow_array_release(out);}
 }ow_database_destroy(d);}puts("public hot-gap:8192 fixed-before-anchor insertions and8192 changing-anchor insertions;full checks every128");return 0;}
