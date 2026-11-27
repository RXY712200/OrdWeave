#include "ordweave.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define T(x) do{if(!(x)){fprintf(stderr,"values:%d\n",__LINE__);exit(2);}}while(0)
#define OK(x) T((x)==OW_OK)
int main(void){OwDatabase*d;OK(ow_database_create(&d));OwRecordId ids[8];double f[]={-DBL_MAX,-DBL_TRUE_MIN,-0.0,0.0,DBL_TRUE_MIN,DBL_MAX};
 for(size_t i=0;i<8;i++)OK(ow_record_create(d,ids+i));
 for(size_t i=0;i<6;i++){OwValue v={.type=OW_FLOAT,.as.real=f[i]};OK(ow_field_set(d,ids[i],"f",&v));}
 OwValue null={.type=OW_NULL};OK(ow_field_set(d,ids[7],"f",&null));
 for(int indexed=0;indexed<2;indexed++){
  if(indexed)OK(ow_field_index_enable(d,"f",7,2));OwRecordId*out;size_t n;
  OK(ow_query_range(d,"f",OW_FLOAT,NULL,1,NULL,1,&out,&n));T(n==6);for(size_t i=0;i<n;i++)T(out[i].value==i+1);ow_array_release(out);
  OwValue zero={.type=OW_FLOAT,.as.real=0};OK(ow_query_range(d,"f",OW_FLOAT,&zero,0,NULL,0,&out,&n));T(n==2&&out[0].value==5&&out[1].value==6);ow_array_release(out);
  OK(ow_query_equal(d,"f",&zero,&out,&n));T(n==2&&out[0].value==3&&out[1].value==4);ow_array_release(out);
 }
 uint64_t before,after;OK(ow_revision(d,&before));OwValue bad={.type=OW_BOOL,.as.boolean=2};T(ow_field_set(d,ids[0],"bad",&bad)==OW_INVALID);
 bad.type=OW_FLOAT;bad.as.real=INFINITY;T(ow_field_set(d,ids[0],"bad",&bad)==OW_INVALID);bad.as.real=NAN;T(ow_field_set(d,ids[0],"bad",&bad)==OW_INVALID);
 bad.type=OW_STRING;bad.as.bytes.data="\xc0\x80";bad.as.bytes.size=2;T(ow_field_set(d,ids[0],"bad",&bad)==OW_INVALID);OK(ow_revision(d,&after));T(before==after);
 OwValue text={.type=OW_STRING,.as.bytes={"\xe4\xb8\xad\0x",5}},copy;OK(ow_field_set(d,ids[0],"text",&text));OK(ow_field_get(d,ids[0],"text",&copy));ow_database_destroy(d);T(copy.as.bytes.size==5&&!memcmp(copy.as.bytes.data,text.as.bytes.data,5));ow_value_release(&copy);
 puts("extreme/subnormal/signed-zero,missing/Null,invalid Boolean/nonfinite/UTF8 and copied multibyte/NUL ownership verified");return 0;}
