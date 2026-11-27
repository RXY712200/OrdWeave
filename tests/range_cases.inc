/* Public-contract assertions included only by the existing core CTest. */
static void range_ids(OwDatabase *d,OwType type,const OwValue *lo,int li,
                       const OwValue *hi,int ui,const uint64_t *ids,size_t n) {
  OwRecordId *out=(void*)1;size_t count=99;
  OK(ow_query_range(d,"key",type,lo,li,hi,ui,&out,&count));T(count==n);
  T(n?out!=NULL:out==NULL);for(size_t i=0;i<n;i++)T(out[i].value==ids[i]);ow_array_release(out);
}
static void range_contracts(void) {
  OwDatabase *d,*other;OwRecordId r[23];OwValue values[23]={{0}};
  OK(ow_database_create(&d));
  OwRecordId *bad=(void*)1;size_t count=99;
  T(ow_query_range(d,"key",(OwType)-1,NULL,1,NULL,1,&bad,&count)==OW_INVALID&&!bad&&!count);
  for(int type=OW_NULL;type<=OW_BINARY;type++)range_ids(d,(OwType)type,NULL,0,NULL,1,NULL,0);
  T(ow_query_range(d,"key",OW_INT,NULL,2,NULL,1,&bad,&count)==OW_INVALID&&!bad&&!count);
  T(ow_query_range(d,"",OW_INT,NULL,1,NULL,1,&bad,&count)==OW_INVALID);
  OwValue nan={.type=OW_FLOAT,.as.real=NAN};
  T(ow_query_range(d,"key",OW_FLOAT,&nan,1,NULL,1,&bad,&count)==OW_INVALID);
  T(ow_field_index_enable(d,"key",0,4)==OW_INVALID);
  T(ow_field_index_enable(d,"key",1,0)==OW_INVALID);
  OK(ow_field_index_enable(d,"key",SIZE_MAX,SIZE_MAX));OK(ow_field_index_disable(d));OK(ow_field_index_disable(d));
  for(size_t i=0;i<23;i++)OK(ow_record_create(d,r+i));
  values[1].type=values[2].type=OW_NULL;
  for(size_t i=3;i<=5;i++){values[i].type=OW_BOOL;values[i].as.boolean=i!=4;}
  for(size_t i=6;i<=9;i++)values[i].type=OW_INT;
  values[6].as.integer=INT64_MAX;values[7].as.integer=INT64_MIN;
  for(size_t i=10;i<=13;i++)values[i].type=OW_FLOAT;
  values[10].as.real=2;values[11].as.real=-0.0;values[12].as.real=0.0;values[13].as.real=-DBL_MAX;
  const char *str[]={"a\0z","","a","a\0","a\0z"};size_t lens[]={3,0,1,2,3};
  for(size_t i=0;i<5;i++){values[14+i].type=OW_STRING;values[14+i].as.bytes.data=str[i];values[14+i].as.bytes.size=lens[i];}
  const unsigned char data[]={0,255};
  for(size_t i=19;i<23;i++)values[i].type=OW_BINARY;
  values[19].as.bytes.data=data+1;values[19].as.bytes.size=1;
  values[21].as.bytes.data=data;values[21].as.bytes.size=1;
  values[22].as.bytes.data=data;values[22].as.bytes.size=2;
  for(size_t i=1;i<23;i++)OK(ow_field_set(d,r[i],"key",values+i));
  const uint64_t expected[6][5]={{2,3},{5,4,6},{8,9,10,7},{14,12,13,11},{16,17,18,15,19},{21,22,23,20}};
  const size_t sizes[]={2,3,4,4,5,4},starts[]={0,0,1,1,2,1},runs[]={2,1,2,2,1,1};
  OwValue bound[6]={{0}};for(int type=0;type<6;type++)bound[type].type=(OwType)type;
  bound[OW_STRING].as.bytes.data="a\0";bound[OW_STRING].as.bytes.size=2;
  bound[OW_BINARY].as.bytes.data=data;bound[OW_BINARY].as.bytes.size=1;
  for(unsigned indexed=0;indexed<2;indexed++) {
    if(indexed){uint64_t before=rev(d);OK(ow_field_index_enable(d,"key",22,17));T(rev(d)==before);T(ow_field_index_enable(d,"key",21,17)==OW_CAPACITY);T(ow_field_index_enable(d,"key",22,16)==OW_CAPACITY);}
    for(int type=0;type<6;type++) {
      range_ids(d,(OwType)type,NULL,0,NULL,0,expected[type],sizes[type]);
      OwRecordId *eq;size_t n;OK(ow_query_equal(d,"key",bound+type,&eq,&n));T(n==runs[type]);
      for(size_t i=0;i<n;i++)T(eq[i].value==expected[type][starts[type]+i]);ow_array_release(eq);
      for(int li=0;li<2;li++)for(int ui=0;ui<2;ui++) {
        range_ids(d,(OwType)type,bound+type,li,bound+type,ui,expected[type]+starts[type],li&&ui?runs[type]:0);
        size_t first=starts[type]+(li?0:runs[type]);
        range_ids(d,(OwType)type,bound+type,li,NULL,ui,expected[type]+first,sizes[type]-first);
        range_ids(d,(OwType)type,NULL,li,bound+type,ui,expected[type],starts[type]+(ui?runs[type]:0));
      }
    }
  }
  OwValue wrong={.type=OW_INT,.as.integer=1},zero={.type=OW_INT};
  T(ow_query_range(d,"key",OW_BOOL,&wrong,1,NULL,1,&bad,&count)==OW_INVALID&&!bad&&!count);
  T(ow_query_range(d,"key",OW_INT,&wrong,1,&zero,1,&bad,&count)==OW_INVALID&&!bad&&!count);
  T(ow_query_range(d,"key",OW_INT,NULL,1,NULL,-1,&bad,&count)==OW_INVALID);
  OK(ow_field_index_enable(d,"key",64,1024));
  OwValue replacement={.type=OW_STRING,.as.bytes={"owned",5}};
  OK(ow_field_set(d,r[8],"key",&replacement));
  const uint64_t ints[]={8,10,7};range_ids(d,OW_INT,NULL,1,NULL,1,ints,3);
  OK(ow_field_remove(d,r[9],"key"));const uint64_t ints2[]={8,7};range_ids(d,OW_INT,NULL,1,NULL,1,ints2,2);
  OK(ow_record_delete(d,r[7]));const uint64_t ints3[]={7};range_ids(d,OW_INT,NULL,1,NULL,1,ints3,1);
  OK(ow_database_create(&other));OwRecordId independent;OK(ow_record_create(other,&independent));
  OK(ow_field_set(other,independent,"key",&wrong));OK(ow_field_index_enable(other,"key",1,4));const uint64_t one[]={1};range_ids(other,OW_INT,NULL,1,NULL,1,one,1);ow_database_destroy(other);
  OwRecordId *saved;size_t n;OK(ow_query_range(d,"key",OW_INT,NULL,1,NULL,1,&saved,&n));T(n==1);
  OK(ow_field_index_disable(d));ow_database_destroy(d);T(saved[0].value==7);ow_array_release(saved);
}
