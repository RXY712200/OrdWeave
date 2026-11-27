#include "ordweave.h"
#include "layerkeysort.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define T(x) do{if(!(x)){fprintf(stderr,"prototype:%d %s\n",__LINE__,#x);exit(2);}}while(0)
#define OK(x) T((x)==OW_OK)
#define LK(x) T((x)==LKS_STATUS_OK)
typedef struct {OwRecordId id;OwValue value;int present;const LksOrderHandle *handle;} Row;
#ifdef FAULT
void arm_failure(void);int fault_seen(void);
#endif
static double now(void){LARGE_INTEGER a,b;QueryPerformanceCounter(&a);QueryPerformanceFrequency(&b);return(double)a.QuadPart/b.QuadPart;}
static int signcmp(uint64_t a,uint64_t b){return(a>b)-(a<b);}
static int valuecmp(const Row*a,const Row*b){
 if(a->present!=b->present)return(a->present>b->present)-(a->present<b->present);if(!a->present)return 0;
 if(a->value.type!=b->value.type)return(a->value.type>b->value.type)-(a->value.type<b->value.type);
 switch(a->value.type){case OW_NULL:return 0;case OW_BOOL:return(a->value.as.boolean>b->value.as.boolean)-(a->value.as.boolean<b->value.as.boolean);case OW_INT:return(a->value.as.integer>b->value.as.integer)-(a->value.as.integer<b->value.as.integer);case OW_FLOAT:return(a->value.as.real>b->value.as.real)-(a->value.as.real<b->value.as.real);default:{size_t an=a->value.as.bytes.size,bn=b->value.as.bytes.size,n=an<bn?an:bn;int c=n?memcmp(a->value.as.bytes.data,b->value.as.bytes.data,n):0;return c?c:(an>bn)-(an<bn);}}
}
static int compare(const void*a,const void*b,void*ctx){(void)ctx;const Row*x=a,*y=b;int c=valuecmp(x,y);return c?c:signcmp(x->id.value,y->id.value);}
static int byvalue(const void*a,const void*b,void*ctx){(void)ctx;return valuecmp(a,b);}
static void comparator_contracts(void){Row a={0},b={0};a.present=0;b.present=1;b.value.type=OW_NULL;T(compare(&a,&b,NULL)<0);a=b;a.value.type=OW_FLOAT;b.value.type=OW_FLOAT;a.value.as.real=-0.0;b.value.as.real=0.0;T(valuecmp(&a,&b)==0);a.value.type=OW_INT;a.value.as.integer=0;T(valuecmp(&a,&b)!=0);a.value.type=b.value.type=OW_STRING;a.value.as.bytes.data="a\0b";b.value.as.bytes.data="a\0c";a.value.as.bytes.size=b.value.as.bytes.size=3;T(valuecmp(&a,&b)<0);b.value.as.bytes.data="a\0b";b.value.as.bytes.size=2;T(valuecmp(&a,&b)>0);a.value.type=OW_BINARY;T(valuecmp(&a,&b)>0);T(!isfinite(NAN));}
static size_t indexed_query(LksManagedOrder *index,int64_t key,OwRecordId **out){Row query={0};query.present=1;query.value.type=OW_INT;query.value.as.integer=key;const LksOrderHandle *p,*e,*s;LK(lks_managed_order_locate(index,&query,&p,&e,&s));T(!e);size_t n=0;for(const LksOrderHandle*h=s;h&&valuecmp(lks_order_item(h),&query)==0;h=lks_order_next(h))n++;*out=n?malloc(n*sizeof(**out)):NULL;T(!n||*out);size_t i=0;for(const LksOrderHandle*h=s;i<n;h=lks_order_next(h))(*out)[i++]=((Row*)lks_order_item(h))->id;return n;}
#ifdef RESOURCE
void memory_report(const char*);
#define MEM(x) memory_report(x)
#else
#define MEM(x) ((void)0)
#endif
int main(int argc,char**argv){T(argc==5);size_t n=strtoull(argv[1],0,10),buckets=strtoull(argv[2],0,10),updates=strtoull(argv[3],0,10);int indexed=atoi(argv[4]);T(n&&buckets&&updates<=100);comparator_contracts();OwDatabase*d;OK(ow_database_create(&d));Row *rows=calloc(n,sizeof(*rows));T(rows);double t=now();
 for(size_t i=0;i<n;i++){OK(ow_record_create(d,&rows[i].id));rows[i].present=1;rows[i].value.type=OW_INT;rows[i].value.as.integer=(int64_t)((i*17+17)%buckets);OK(ow_field_set(d,rows[i].id,"key",&rows[i].value));}
 double setup=now()-t;t=now();LksManagedOrder*index=NULL;Row *indexrows=NULL;if(indexed){LksComparator c={compare,NULL};index=lks_managed_order_create(&c);T(index);indexrows=calloc(n,sizeof(*indexrows));T(indexrows);for(size_t i=0;i<n;i++){indexrows[i]=rows[i];LK(lks_managed_order_insert(index,indexrows+i,&indexrows[i].handle));}}double build=now()-t;MEM("built");t=now();
 for(size_t j=0;j<updates;j++){size_t i=(j*7919+17)%n;if(j%2){OK(ow_record_delete(d,rows[i].id));if(index&&lks_managed_order_remove(index,indexrows[i].handle,NULL)!=LKS_STATUS_OK){lks_managed_order_destroy(index);index=NULL;free(indexrows);indexrows=NULL;}rows[i].present=0;continue;}OwValue next={.type=OW_INT,.as.integer=(int64_t)((j+31)%buckets)};OK(ow_field_set(d,rows[i].id,"key",&next));if(index){LksStatus status=lks_managed_order_remove(index,indexrows[i].handle,NULL);if(status!=LKS_STATUS_OK){lks_managed_order_destroy(index);index=NULL;free(indexrows);indexrows=NULL;}}rows[i].value=next;if(index){
#ifdef FAULT
arm_failure();
#endif
indexrows[i].value=next;LksStatus status=lks_managed_order_insert(index,indexrows+i,&indexrows[i].handle);if(status!=LKS_STATUS_OK){lks_managed_order_destroy(index);index=NULL;free(indexrows);indexrows=NULL;}}}
 double update=now()-t;t=now();size_t total=0;
 for(size_t j=0;j<512;j++){int64_t key=(int64_t)((j*17)%buckets);OwRecordId *got=NULL;size_t count=0;if(index)count=indexed_query(index,key,&got);else{OwValue v={.type=OW_INT,.as.integer=key};OK(ow_query_equal(d,"key",&v,&got,&count));}
 size_t k=0;for(size_t i=0;i<n;i++)if(rows[i].present&&rows[i].value.as.integer==key){T(k<count&&got[k].value==rows[i].id.value);k++;}T(k==count);total+=count;ow_array_release(got);}
 double reads=now()-t;MEM("read");
 if(index){Row lower={0};lower.present=1;lower.value.type=OW_INT;lower.value.as.integer=17;const LksOrderHandle *p,*e,*h;LK(lks_managed_order_locate(index,&lower,&p,&e,&h));T(!e);for(int64_t key=17;key<=31;key++)for(size_t i=0;i<n;i++)if(rows[i].present&&rows[i].value.as.integer==key){T(h);Row *got=lks_order_item(h);T(got->id.value==rows[i].id.value);h=lks_order_next(h);}T(!h||((Row*)lks_order_item(h))->value.as.integer>31);}

#ifdef FAULT
T(fault_seen()&&!index);
#endif
lks_managed_order_destroy(index);free(indexrows);free(rows);ow_database_destroy(d);printf("result,%zu,%zu,%zu,%d,%.9f,%.9f,%.9f,%.9f,%zu\n",n,buckets,updates,indexed,setup,build,update,reads,total);return 0;}
