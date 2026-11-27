#include "ordweave.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#define T(x) do{if(!(x)){fprintf(stderr,"bench:%d %s\n",__LINE__,#x);exit(2);}}while(0)
#define OK(x) T((x)==OW_OK)
typedef struct {OwRecordId id;int64_t value;int present;} Model;
static double now(void){LARGE_INTEGER a,b;QueryPerformanceCounter(&a);QueryPerformanceFrequency(&b);return(double)a.QuadPart/b.QuadPart;}
static double change(OwDatabase*d,Model*m,size_t n,size_t buckets,size_t j){size_t i=(j*7919+17)%n;double t=now();if(j%2){OK(ow_record_delete(d,m[i].id));}else{OwValue v={.type=OW_INT,.as.integer=(int64_t)((j+31)%buckets)-128};OK(ow_field_set(d,m[i].id,"key",&v));}double elapsed=now()-t;if(j%2)m[i].present=0;else m[i].value=(int64_t)((j+31)%buckets)-128;return elapsed;}
#ifdef RESOURCE
void memory_report(const char*);
#define MEM(x) memory_report(x)
#else
#define MEM(x) ((void)0)
#endif
int main(int argc,char**argv){T(argc==6);size_t n=strtoull(argv[1],0,10),b=strtoull(argv[2],0,10),changes=strtoull(argv[3],0,10);int interleave=atoi(argv[4]),mode=atoi(argv[5]);T(n>=128&&changes<=100);Model*m=calloc(n,sizeof(*m));T(m);OwDatabase*d;double t=now();OK(ow_database_create(&d));for(size_t i=0;i<n;i++){OK(ow_record_create(d,&m[i].id));m[i].present=1;m[i].value=(int64_t)((i*17+17)%b)-128;OwValue v={.type=OW_INT,.as.integer=m[i].value};OK(ow_field_set(d,m[i].id,"key",&v));}double setup=now()-t,build=0,updates=0,reads=0;MEM("setup");
#ifndef BASELINE
if(mode==2||mode==4){t=now();OK(ow_field_index_enable(d,"key",n,4));build=now()-t;}
#else
T(mode==0);
#endif
MEM("enabled");if(!interleave)for(size_t j=0;j<changes;j++)updates+=change(d,m,n,b,j);
size_t total=0;for(size_t j=0;j<512;j++){if(interleave&&j%8==0&&j/8<changes)updates+=change(d,m,n,b,j/8);OwValue v={.type=OW_INT,.as.integer=(int64_t)((j*17)%b)-128};OwRecordId*out=NULL;size_t count=0;t=now();
#ifndef BASELINE
if(mode>=3)OK(ow_query_range(d,"key",OW_INT,&v,1,&v,1,&out,&count));else
#endif
OK(ow_query_equal(d,"key",&v,&out,&count));reads+=now()-t;size_t k=0;for(size_t i=0;i<n;i++)if(m[i].present&&m[i].value==v.as.integer){T(k<count&&out[k].value==m[i].id.value);k++;}T(k==count);total+=count;ow_array_release(out);}
MEM("read");free(m);ow_database_destroy(d);MEM("closed");printf("result,%zu,%zu,%zu,%d,%d,%.9f,%.9f,%.9f,%.9f,%zu\n",n,b,changes,interleave,mode,setup,build,updates,reads,total);return 0;}
