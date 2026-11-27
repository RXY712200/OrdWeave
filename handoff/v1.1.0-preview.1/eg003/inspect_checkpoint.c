#include "checkpoint_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define T(x) do{if(!(x)){fprintf(stderr,"inspect:%d %s\n",__LINE__,#x);return 2;}}while(0)
#define OK(x) T((x)==OW_OK)
int main(int argc,char **argv){
 T(argc==2);FILE *f=fopen(argv[1],"rb");T(f);T(!fseek(f,0,SEEK_END));long n=ftell(f);T(n>=176&&n<=OW_CHECKPOINT_LIMIT);rewind(f);
 unsigned char *bytes=malloc((size_t)n);T(bytes);T(fread(bytes,1,(size_t)n,f)==(size_t)n);T(!fclose(f));
 OwDatabase *d=NULL;OK(ow_core_database_create(&d));OwStatus s=ow_core_import(d,bytes,(size_t)n,bytes+24);T(s==OW_OK);free(bytes);
 uint64_t rev;OK(ow_core_revision(d,&rev));T(rev==21);OwOccurrenceId *a;size_t count;
 for(uint64_t c=1;c<=3;c++){
  OK(ow_core_collection_members(d,(OwCollectionId){c},&a,&count));T(count==(c==1?2:c==2?0:1));
  if(c==1)T(a[0].value==2&&a[1].value==1);if(c==3)T(a[0].value==3);ow_array_release(a);
 }
 int exists;OK(ow_core_record_exists(d,(OwRecordId){2},&exists));T(!exists);OK(ow_core_record_exists(d,(OwRecordId){3},&exists));T(exists);
 for(unsigned i=OW_NULL;i<=OW_BINARY;i++){
  char name[12];snprintf(name,sizeof(name),"field%u",i);OwValue v;OK(ow_core_field_get(d,(OwRecordId){1},name,&v));T(v.type==(OwType)i);
  if(i==OW_BOOL)T(v.as.boolean==1);if(i==OW_INT)T(v.as.integer==INT64_MIN);if(i==OW_FLOAT)T(v.as.real==-0.5);
  if(i==OW_STRING)T(v.as.bytes.size==5&&!memcmp(v.as.bytes.data,"a\0\xe4\xb8\xad",5));if(i==OW_BINARY)T(v.as.bytes.size==2&&!memcmp(v.as.bytes.data,"\xff\0",2));ow_value_release(&v);
 }
 OwValue v;OK(ow_core_field_get(d,(OwRecordId){1},"cycle",&v));T(v.type==OW_INT&&v.as.integer==4);ow_value_release(&v);
 ow_core_database_destroy(d);puts("import OK; revision21, deleted-ID gap, six typed fields, cycle4 and three collection orders verified");return 0;
}
