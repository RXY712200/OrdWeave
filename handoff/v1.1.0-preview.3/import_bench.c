#include "checkpoint_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#define OK(x) do{if((x)!=OW_OK){fprintf(stderr,"line%d\n",__LINE__);exit(2);}}while(0)
static double now(void){LARGE_INTEGER a,b;QueryPerformanceCounter(&a);QueryPerformanceFrequency(&b);return(double)a.QuadPart/b.QuadPart;}
int main(int argc,char**argv){if(argc!=4)return 3;size_t n=strtoull(argv[1],0,10),nc=strtoull(argv[2],0,10),fields=strtoull(argv[3],0,10);OwDatabase*d;OwRecordId r;OwCollectionId c[8];OwOccurrenceId o;unsigned char id[16]={0},anchor[32]={0},*bytes;size_t size;double t=now();OK(ow_database_create(&d));OK(ow_record_create(d,&r));for(size_t j=0;j<fields;j++){char name[24];snprintf(name,sizeof(name),"field%zu",j);OwValue v={.type=OW_INT,.as.integer=(int64_t)j};OK(ow_field_set(d,r,name,&v));}for(size_t j=0;j<nc;j++)OK(ow_collection_create(d,c+j));for(size_t j=0;j<n;j++)OK(ow_occurrence_insert(d,c[j%nc],r,OW_BACK,(OwOccurrenceId){0},&o));double setup=now()-t;t=now();OK(ow_core_export(d,id,1,anchor,&bytes,&size));double export=now()-t;OwDatabase*copy;OK(ow_core_database_create(&copy));t=now();OK(ow_core_import(copy,bytes,size,id));double import=now()-t;if(!ow_core_same_state(d,copy))return 4;printf("%zu,%zu,%zu,%.9f,%.9f,%.9f,%zu\n",n,nc,fields,setup,export,import,size);free(bytes);ow_database_destroy(d);ow_core_database_destroy(copy);return 0;}
