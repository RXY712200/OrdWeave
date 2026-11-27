#include "ordweave.h"
#include <stdio.h>
#include <stdlib.h>
#define T(x) do{if(!(x)){fprintf(stderr,"highwater:%d\n",__LINE__);exit(2);}}while(0)
#define OK(x) T((x)==OW_OK)
int main(int argc,char**argv){T(argc==2);OwDatabase*d;OwRecordId r[3];OwCollectionId c[3];OwOccurrenceId o[3];OK(ow_database_create_persistent(argv[1],&d));
 for(int i=0;i<2;i++){OK(ow_record_create(d,r+i));T(r[i].value==(uint64_t)i+1);OK(ow_collection_create(d,c+i));T(c[i].value==(uint64_t)i+1);OK(ow_occurrence_insert(d,c[i],r[i],OW_BACK,(OwOccurrenceId){0},o+i));T(o[i].value==(uint64_t)i+1);}
 OK(ow_occurrence_remove(d,o[1]));OK(ow_collection_delete(d,c[1]));OK(ow_record_delete(d,r[1]));OwValue v={.type=OW_INT,.as.integer=1};OK(ow_field_set(d,r[0],"v",&v));uint64_t rev;OK(ow_revision(d,&rev));T(rev==10);OK(ow_checkpoint_create(d));v.as.integer=999;OK(ow_field_set(d,r[0],"v",&v));OK(ow_database_close(d));OK(ow_database_open(argv[1],&d));
 OK(ow_record_create(d,r+2));OK(ow_collection_create(d,c+2));OK(ow_occurrence_insert(d,c[2],r[2],OW_BACK,(OwOccurrenceId){0},o+2));T(r[2].value==3&&c[2].value==3&&o[2].value==3);OK(ow_revision(d,&rev));T(rev==14);
 OwHistory*h;OK(ow_history_open(d,10,&h));OwValue old;OK(ow_history_field_get(h,r[0],"v",&old));T(old.as.integer==1);ow_value_release(&old);OwRecordId*ids;size_t n;v.as.integer=1;OK(ow_history_query_range(h,"v",OW_INT,&v,1,&v,1,&ids,&n));T(n==1&&ids[0].value==1);ow_history_close(h);OK(ow_database_close(d));T(ids[0].value==1);ow_array_release(ids);puts("all three sparse highwaters restored,revision14,old range remains1 vs live999,copied IDs survive both owners");return 0;}
