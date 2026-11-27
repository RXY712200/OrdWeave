#include "checkpoint_internal.h"
#include <stdio.h>
#include <stdlib.h>
#define OK(x) do{if((x)!=OW_OK)return 2;}while(0)
int main(int argc,char **argv){OwDatabase*d;OwRecordId r;OwCollectionId c;OwOccurrenceId a,b;unsigned char id[16]={0},anchor[32]={0},*bytes;size_t n; if(argc!=2)return 3;OK(ow_database_create(&d));OK(ow_record_create(d,&r));OK(ow_collection_create(d,&c));OK(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&a));OK(ow_occurrence_insert(d,c,r,OW_FRONT,(OwOccurrenceId){0},&b));OwValue v={.type=OW_INT,.as.integer=17};OK(ow_field_set(d,r,"value",&v));
#ifdef ENABLED
OK(ow_field_index_enable(d,"value",1,6));
#endif
OK(ow_core_export(d,id,1,anchor,&bytes,&n));FILE*f=fopen(argv[1],"wb");if(!f||fwrite(bytes,1,n,f)!=n||fclose(f))return 4;free(bytes);ow_database_destroy(d);return 0;}
