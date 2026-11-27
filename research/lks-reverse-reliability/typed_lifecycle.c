#include "ordweave.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define T(x) do{if(!(x)){fprintf(stderr,"typed:%d step=%zu\n",__LINE__,step);exit(2);}}while(0)
#define OK(x) do{calls++;T((x)==OW_OK);}while(0)
static size_t step,calls;
typedef struct{uint64_t id;int active,present;OwValue v;unsigned char bytes[8];} Row;
static Row rows[128];
static int valuecmp(const OwValue*a,const OwValue*b){
 if(a->type!=b->type)return a->type<b->type?-1:1;
 switch(a->type){case OW_NULL:return 0;case OW_BOOL:return a->as.boolean-b->as.boolean;case OW_INT:return a->as.integer<b->as.integer?-1:a->as.integer>b->as.integer;case OW_FLOAT:return a->as.real<b->as.real?-1:a->as.real>b->as.real;default:
  for(size_t i=0;i<a->as.bytes.size&&i<b->as.bytes.size;i++){unsigned x=((const unsigned char*)a->as.bytes.data)[i],y=((const unsigned char*)b->as.bytes.data)[i];if(x!=y)return x<y?-1:1;}
  return a->as.bytes.size<b->as.bytes.size?-1:a->as.bytes.size>b->as.bytes.size;
 }
}
static int rowcmp(const void*a,const void*b){Row*x=*(Row*const*)a,*y=*(Row*const*)b;int c=valuecmp(&x->v,&y->v);return c?c:x->id<y->id?-1:x->id>y->id;}
int main(void){OwDatabase*d[2];uint64_t high=128,rv=256;for(int j=0;j<2;j++){OK(ow_database_create(d+j));for(size_t i=0;i<128;i++){OwRecordId id;OK(ow_record_create(d[j],&id));T(id.value==i+1);OwValue v={.type=OW_INT};OK(ow_field_set(d[j],id,"key",&v));rows[i].id=i+1;rows[i].active=rows[i].present=1;rows[i].v=v;}}OK(ow_field_index_enable(d[1],"key",128,8192));
 for(step=0;step<4096;step++){
  Row*r=rows+(step*71%128);OwRecordId id={r->id};unsigned action=step%7;
  if(!r->active){high++;for(int j=0;j<2;j++){OwRecordId out;OK(ow_record_create(d[j],&out));T(out.value==high);}r->id=high;r->active=1;r->present=0;rv++;}
  else if(action==0||action==1){OwValue v={.type=(OwType)((step/7)%6)};unsigned char bytes[8]={(unsigned char)('a'+step%4),0,'z'};
   if(v.type==OW_BOOL)v.as.boolean=step%2;else if(v.type==OW_INT)v.as.integer=(int64_t)step-2048;else if(v.type==OW_FLOAT)v.as.real=(step%2?-1.0:1.0)*(double)(step%97)/8;else if(v.type==OW_STRING||v.type==OW_BINARY){v.as.bytes.data=bytes;v.as.bytes.size=step%4;}
   int changed=!r->present||valuecmp(&r->v,&v)!=0;for(int j=0;j<2;j++)OK(ow_field_set(d[j],id,"key",&v));r->present=1;r->v=v;if(v.type==OW_STRING||v.type==OW_BINARY){memcpy(r->bytes,bytes,8);r->v.as.bytes.data=r->bytes;}rv+=changed;
  }else if(action==2){for(int j=0;j<2;j++)OK(ow_field_remove(d[j],id,"key"));rv+=r->present;r->present=0;}
  else if(action==3){for(int j=0;j<2;j++)OK(ow_record_delete(d[j],id));r->active=r->present=0;rv++;}
  else if(action==4&&r->present){for(int j=0;j<2;j++)OK(ow_field_set(d[j],id,"key",&r->v));}
  else if(action==5){for(int j=0;j<2;j++){calls++;T(ow_field_set(d[j],(OwRecordId){0},"key",&r->v)==OW_INVALID);}}
  if(step%128==0){OK(ow_field_index_disable(d[1]));OK(ow_field_index_enable(d[1],"other",128,8192));OK(ow_field_index_enable(d[1],"key",128,8192));}
  for(int type=0;type<6;type++){
   Row*expected[128];size_t n=0;for(size_t i=0;i<128;i++)if(rows[i].active&&rows[i].present&&rows[i].v.type==(OwType)type)expected[n++]=rows+i;qsort(expected,n,sizeof(*expected),rowcmp);
   for(int j=0;j<2;j++){OwRecordId*out;size_t count;OK(ow_query_range(d[j],"key",(OwType)type,NULL,0,NULL,1,&out,&count));T(count==n);for(size_t k=0;k<n;k++)T(out[k].value==expected[k]->id);ow_array_release(out);uint64_t actual;OK(ow_revision(d[j],&actual));T(actual==rv);}
  }
 }
 ow_database_destroy(d[0]);ow_database_destroy(d[1]);printf("typed lifecycle logical=4096,public=%zu,high=%llu,revision=%llu\n",calls,(unsigned long long)high,(unsigned long long)rv);return 0;}
