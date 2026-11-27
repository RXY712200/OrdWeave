#include "ordweave.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define OK(x) do{OwStatus s=(x);if(s!=OW_OK){fprintf(stderr,"%d status%d\n",__LINE__,s);return 2;}}while(0)
static double now(void){LARGE_INTEGER t,f;QueryPerformanceCounter(&t);QueryPerformanceFrequency(&f);return(double)t.QuadPart/f.QuadPart;}
int main(int argc,char **argv){if(argc!=4)return 3;OwDatabase*d;OwRecordId r;OwCollectionId c;OwOccurrenceId o;OwHistory*h;OwHistoryRange range;uint64_t revision;double t,cp,open,history,verify,backup;
if(!strcmp(argv[1],"create")){size_t n=strtoull(argv[3],0,10);OK(ow_database_create_persistent(argv[2],&d));OK(ow_record_create(d,&r));OK(ow_collection_create(d,&c));for(size_t i=0;i<n;i++)OK(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&o));OK(ow_checkpoint_create(d));OwValue v={.type=OW_INT,.as.integer=17};OK(ow_field_set(d,r,"tail",&v));OK(ow_database_close(d));return 0;}
t=now();OK(ow_database_open(argv[2],&d));open=now()-t;t=now();OK(ow_checkpoint_create(d));cp=now()-t;OK(ow_history_range(d,&range));revision=range.current_revision>range.earliest_revision?range.current_revision-1:range.current_revision;t=now();OK(ow_history_open(d,revision,&h));history=now()-t;ow_history_close(h);t=now();OK(ow_verify_database(d,&range));verify=now()-t;t=now();OK(ow_backup_create(d,argv[3]));backup=now()-t;OK(ow_database_close(d));printf("%.9f,%.9f,%.9f,%.9f,%.9f\n",open,cp,history,verify,backup);return 0;}
