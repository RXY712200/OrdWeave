#include "ordweave.h"
#include "lks_adapter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "compressed_index.inc"
#define T(x) do{if(!(x)){fprintf(stderr,"prototype:%d %s\n",__LINE__,#x);exit(2);}}while(0)
static double now(void){LARGE_INTEGER t,f;QueryPerformanceCounter(&t);QueryPerformanceFrequency(&f);return(double)t.QuadPart/f.QuadPart;}
typedef struct {uint64_t range;LksManagedOrder *order;} Part;
typedef struct {Part *parts;size_t n,cap;} Partitioned;
static size_t locate(Partitioned *p,uint64_t range){size_t l=0,h=p->n;while(l<h){size_t m=l+(h-l)/2;if(p->parts[m].range<range)l=m+1;else h=m;}return l;}
static void partition_change(Partitioned *p,OwIndexed *add,uint64_t omit){
 uint64_t id=add?add->id:omit,range=id>>8;size_t at=locate(p,range);int exists=at<p->n&&p->parts[at].range==range;
 LksManagedOrder *order=ow_index_create();T(order);const LksOrderHandle *h,*temporary;
 if(exists)for(h=lks_managed_order_first(p->parts[at].order);h;h=lks_order_next(h)){OwIndexed *x=lks_order_item(h);if(x->id!=omit)T(lks_managed_order_insert(order,x,&temporary)==LKS_STATUS_OK);}
 if(add)T(lks_managed_order_insert(order,add,&temporary)==LKS_STATUS_OK);
 Part *prepared=NULL;size_t cap=p->cap;
 if(!exists&&p->n==p->cap){cap=cap?cap*2:4;prepared=malloc(cap*sizeof(*prepared));T(prepared);if(p->n)memcpy(prepared,p->parts,p->n*sizeof(*prepared));}
 /* All potentially failing work precedes publication. Global directory copy
  * only on capacity growth; sorted directory shifts remain O(P), not O(1). */
 if(prepared){free(p->parts);p->parts=prepared;p->cap=cap;}
 if(exists)lks_managed_order_destroy(p->parts[at].order);
 else{memmove(p->parts+at+1,p->parts+at,(p->n-at)*sizeof(*p->parts));p->n++;}
 p->parts[at]=(Part){range,order};
 if(!lks_managed_order_size(order)){lks_managed_order_destroy(order);memmove(p->parts+at,p->parts+at+1,(p->n-at-1)*sizeof(*p->parts));p->n--;}
}
static void baseline_change(LksManagedOrder **index,OwIndexed *add,uint64_t omit){LksManagedOrder *next=ow_index_create();T(next);const LksOrderHandle *h,*out;for(h=lks_managed_order_first(*index);h;h=lks_order_next(h)){OwIndexed *x=lks_order_item(h);if(x->id!=omit)T(lks_managed_order_insert(next,x,&out)==LKS_STATUS_OK);}if(add)T(lks_managed_order_insert(next,add,&out)==LKS_STATUS_OK);lks_managed_order_destroy(*index);*index=next;}
#ifdef RESOURCE
void memory_report(const char *);
#define MEM(x) memory_report(x)
#else
#define MEM(x) ((void)0)
#endif
int main(int argc,char **argv){T(argc==4);char kind=argv[1][0];size_t n=strtoull(argv[2],0,10),steps=1000;int sparse=atoi(argv[3]);T(n&&n<=8192);OwIndexed *items=calloc(n+steps,sizeof(*items));T(items);uint64_t *active=calloc(n,sizeof(*active));T(active);
 LksManagedOrder *a=kind=='A'?ow_index_create():NULL;Partitioned c={0};OwOccIndex *d=kind=='D'?occ_create():NULL;double t=now();
 for(size_t i=0;i<n;i++){items[i].id=sparse?((uint64_t)i<<48)+1:i+1;active[i]=items[i].id;if(kind=='A')baseline_change(&a,items+i,0);else if(kind=='C')partition_change(&c,items+i,0);else{OwOccPlan plan;T(occ_prepare(d,items[i].id,&plan)==OW_OK);occ_commit(d,&plan,items+i);}}
 double setup=now()-t;MEM("setup");t=now();uint32_t seed=17;
 for(size_t i=0;i<steps;i++) {seed=seed*1664525u+1013904223u;size_t pos=seed%n;uint64_t old=active[pos];items[n+i].id=sparse?((uint64_t)(n+i)<<48)+1:n+i+1;active[pos]=items[n+i].id;
  if(kind=='A'){T(ow_index_find(a,old));baseline_change(&a,NULL,old);baseline_change(&a,items+n+i,0);}
  else if(kind=='C'){size_t j=locate(&c,old>>8);T(j<c.n&&ow_index_find(c.parts[j].order,old));partition_change(&c,NULL,old);partition_change(&c,items+n+i,0);}
  else{T(occ_find(d,old));occ_remove(d,old);OwOccPlan plan;T(occ_prepare(d,items[n+i].id,&plan)==OW_OK);occ_commit(d,&plan,items+n+i);}
 }
 double operations=now()-t;MEM("operations");size_t count=0;uint64_t last=0;
 if(kind=='D'){OwOccIterator it;for(OwIndexed *x=occ_first(d,&it);x;x=occ_next(&it)){T(x->id>last);last=x->id;count++;}T(d->count==n);}
 else for(size_t j=0;j<(kind=='C'?c.n:1);j++){const LksOrderHandle *h=lks_managed_order_first(kind=='C'?c.parts[j].order:a);for(;h;h=lks_order_next(h)){OwIndexed *x=lks_order_item(h);T(x->id>last);last=x->id;count++;}}
 T(count==n);for(size_t i=0;i<n;i++)T(kind=='D'?occ_find(d,active[i])!=NULL:kind=='A'?ow_index_find(a,active[i])!=NULL:ow_index_find(c.parts[locate(&c,active[i]>>8)].order,active[i])!=NULL);
 lks_managed_order_destroy(a);for(size_t i=0;i<c.n;i++)lks_managed_order_destroy(c.parts[i].order);free(c.parts);occ_destroy(d);free(items);free(active);MEM("closed");printf("result,%c,%zu,%d,%.9f,%.9f\n",kind,n,sparse,setup,operations);return 0;}
