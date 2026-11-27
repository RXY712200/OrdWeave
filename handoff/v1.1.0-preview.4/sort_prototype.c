#include "layerkeysort.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define T(x) do{if(!(x)){fprintf(stderr,"sort:%d %s\n",__LINE__,#x);exit(2);}}while(0)
#define LK(x) T((x)==LKS_STATUS_OK)
typedef struct {unsigned long long id;unsigned key;} Row;
static int cmp(const void*a,const void*b,void*ctx){(void)ctx;const Row*x=a,*y=b;return(x->key>y->key)-(x->key<y->key);}
static double now(void){LARGE_INTEGER a,b;QueryPerformanceCounter(&a);QueryPerformanceFrequency(&b);return(double)a.QuadPart/b.QuadPart;}
#ifdef RESOURCE
void memory_report(const char*);
#define MEM(x) memory_report(x)
#else
#define MEM(x) ((void)0)
#endif
int main(int argc,char**argv){T(argc==3);size_t n=strtoull(argv[1],0,10);unsigned mode=(unsigned)atoi(argv[2]);Row *rows=calloc(n,sizeof(*rows));void **items=malloc(n*sizeof(*items)),**expected=malloc(n*sizeof(*expected));T(rows&&items&&expected);for(size_t i=0;i<n;i++){rows[i]=(Row){i+1,(unsigned)((i*17+17)%31)};items[i]=expected[i]=rows+i;}
/* Independent stable insertion model; outside timed work. */
for(size_t i=1;i<n;i++){void *v=expected[i];size_t j=i;while(j&&cmp(expected[j-1],v,NULL)>0){expected[j]=expected[j-1];j--;}expected[j]=v;}
LksComparator c={cmp,NULL};double t=now();LksGroup *group=NULL,*left=NULL,*right=NULL;LksGroupBatch *batch=NULL;void **copy=NULL;
if(mode==0){copy=malloc(n*sizeof(*copy));T(copy);memcpy(copy,items,n*sizeof(*copy));LK(lks_sort(copy,n,cmp,NULL));}
else if(mode==1)LK(lks_group_build(items,n,&c,&group));
else if(mode==2){LK(lks_group_batch_build(items,n,128,&c,&batch));LK(lks_group_batch_merge_all(batch,&c,&group));}
else {LK(lks_group_build(items,n/2,&c,&left));LK(lks_group_build(items+n/2,n-n/2,&c,&right));LK(lks_group_merge(left,right,&c,&group));}
double build=now()-t;
for(size_t i=0;i<n;i++){Row *actual=mode==0?copy[i]:lks_group_item_at(group,i);T(actual==expected[i]);}
MEM("built");double recurring=0;if(mode==3){t=now();for(unsigned j=0;j<64;j++){LksGroup *merged;LK(lks_group_merge(left,right,&c,&merged));T(lks_group_size(merged)==n);lks_group_destroy(merged);}recurring=now()-t;}
MEM("merged");free(copy);lks_group_destroy(group);lks_group_destroy(left);lks_group_destroy(right);lks_group_batch_destroy(batch);free(expected);free(items);free(rows);printf("result,%zu,%u,%.9f,%.9f\n",n,mode,build,recurring);return 0;}
