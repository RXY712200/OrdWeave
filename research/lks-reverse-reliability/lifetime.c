#include "ordweave.h"
#include <stdio.h>
#include <stdlib.h>
#define T(x) do{if(!(x)){fprintf(stderr,"lifetime:%d\n",__LINE__);exit(2);}}while(0)
#define OK(x) T((x)==OW_OK)
int main(void){size_t cycles=0;
 for(unsigned cycle=0;cycle<64;cycle++){
  OwDatabase*d,*b;OwRecordId r;OK(ow_database_create(&d));OK(ow_database_create(&b));OK(ow_record_create(d,&r));OwCollectionId c[32];
  for(unsigned k=0;k<32;k++){OK(ow_collection_create(d,c+k));OwOccurrenceId ids[3];for(int j=0;j<3;j++)OK(ow_occurrence_insert(d,c[k],r,OW_BACK,(OwOccurrenceId){0},ids+j));
   int ab,bc,ac;OK(ow_occurrence_compare(d,ids[0],ids[1],&ab));OK(ow_occurrence_compare(d,ids[1],ids[2],&bc));OK(ow_occurrence_compare(d,ids[0],ids[2],&ac));T(ab<0&&bc<0&&ac<0);
   T(ow_record_delete(d,r)==OW_CONFLICT);T(ow_collection_delete(d,c[k])==OW_CONFLICT);OwOccurrenceId*saved;size_t n;OK(ow_collection_members(d,c[k],&saved,&n));T(n==3);
   for(int j=0;j<3;j++)OK(ow_occurrence_remove(d,ids[j]));for(int j=0;j<3;j++)T(saved[j].value==ids[j].value);ow_array_release(saved);OK(ow_collection_count(d,c[k],&n));T(n==0);OK(ow_collection_delete(d,c[k]));cycles++;
  }OK(ow_record_delete(d,r));uint64_t rev;OK(ow_revision(b,&rev));T(rev==0);ow_database_destroy(b);ow_database_destroy(d);
 }printf("fill-clear-delete cycles=%zu,independent_instances=128,transitivity_samples=%zu\n",cycles,cycles);return 0;}
