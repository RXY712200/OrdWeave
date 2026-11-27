#include "ordweave.h"
#include <stdio.h>
#include <stdlib.h>
static int armed;static size_t calls;
void *__real_malloc(size_t);
void *__wrap_malloc(size_t n){if(armed)calls++;return __real_malloc(n);}
#define T(x) do{if(!(x)){fprintf(stderr,"cache probe %d %s\n",__LINE__,#x);exit(2);}}while(0)
static void probe(OwDatabase*d,size_t expected){OwValue v={.type=OW_INT,.as.integer=100};OwRecordId*ids=NULL;size_t n=0;calls=0;armed=1;OwStatus s=ow_query_range(d,"key",OW_INT,&v,1,&v,1,&ids,&n);armed=0;T(s==OW_OK&&n==1&&ids[0].value==1&&calls==expected);ow_array_release(ids);printf("cache allocations=%zu\n",calls);}
int main(int argc,char**argv){T(argc==2);OwDatabase*d;OwRecordId r;T(ow_database_create_persistent(argv[1],&d)==OW_OK);T(ow_record_create(d,&r)==OW_OK);OwValue v={.type=OW_INT,.as.integer=100};T(ow_field_set(d,r,"key",&v)==OW_OK);probe(d,2);T(ow_field_index_enable(d,"key",1,4)==OW_OK);probe(d,1);T(ow_checkpoint_create(d)==OW_OK);T(ow_database_close(d)==OW_OK);T(ow_database_open(argv[1],&d)==OW_OK);probe(d,2);T(ow_database_close(d)==OW_OK);puts("cache active then absent on reopen; diagnostic malloc counts, unchanged library");return 0;}
