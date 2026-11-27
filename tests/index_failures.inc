/* White-box ownership/fallback assertions, in the existing failures CTest. */
static void index_failures(void) {
  OwDatabase *d;OwRecordId r,missing;OwValue v={.type=OW_INT,.as.integer=17};
  OK(ow_database_create(&d));OK(ow_record_create(d,&r));OK(ow_record_create(d,&missing));
  OK(ow_field_set(d,r,"key",&v));OK(ow_field_index_enable(d,"key",1,4));
  OwFieldIndex *cache=d->field_index;OwFieldRow *row=((Record*)ow_index_find(d->records,r.value))->indexed;
  uint64_t before=d->revision;
  arm(0);OK(ow_field_set(d,r,"key",&v));T(calls==0&&d->field_index==cache&&d->revision==before);disarm();
  OwValue unrelated={.type=OW_INT,.as.integer=31};
  OK(ow_field_set(d,r,"other",&unrelated));T(d->field_index==cache&&((Record*)ow_index_find(d->records,r.value))->indexed==row);
  T(ow_field_set(d,(OwRecordId){0},"key",&v)==OW_INVALID&&d->field_index==cache);
  T(ow_field_index_enable(d,"key",1,3)==OW_CAPACITY&&d->field_index==cache);
  size_t ordinal;
  for(ordinal=0;ordinal<16;ordinal++) {
    arm(ordinal);OwStatus s=ow_field_index_enable(d,"key",2,SIZE_MAX);disarm();
    if(s==OW_OK)break;T(s==OW_NOMEM&&d->field_index==cache);
    T(((Record*)ow_index_find(d->records,r.value))->indexed==row);
  }
  T(ordinal<16);cache=d->field_index;before=d->revision;
  managed_remove_failure=LKS_STATUS_CAPACITY_LIMIT;
  v.as.integer=18;OK(ow_field_set(d,r,"key",&v));T(!d->field_index);
  managed_remove_failure=LKS_STATUS_OK;OK(ow_field_index_enable(d,"key",2,64));
  managed_insert_failure=LKS_STATUS_CAPACITY_LIMIT;
  v.as.integer=19;OK(ow_field_set(d,r,"key",&v));T(!d->field_index);
  managed_insert_failure=LKS_STATUS_OK;OK(ow_field_index_enable(d,"key",2,64));
  before=d->revision;
  v.as.integer=18;arm(0);OK(ow_field_set(d,r,"key",&v));disarm();
  T(!d->field_index&&d->revision==before+1);
  v.as.integer=19;arm(0);OK(ow_field_set(d,r,"key",&v));T(calls==0&&!d->field_index);disarm();
  OwRecordId *ids=(void*)1;size_t n=99;
  arm(0);T(ow_query_equal(d,"key",&v,&ids,&n)==OW_NOMEM&&!ids&&!n);disarm();
  OK(ow_field_index_enable(d,"key",1,4));
  arm(0);T(ow_query_range(d,"key",OW_INT,NULL,1,NULL,1,&ids,&n)==OW_NOMEM&&!ids&&!n);disarm();T(d->field_index);
  OK(ow_field_set(d,missing,"key",&v));T(!d->field_index); /* entry limit invalidation */
  OK(ow_field_index_enable(d,"key",2,4));
  OK(ow_field_remove(d,missing,"key"));T(d->field_index&&d->field_index->count==1);
  OK(ow_record_delete(d,r));T(d->field_index&&d->field_index->count==0);
  ow_database_destroy(d);
  /* Owned String allocations: core failure preserves old cache, maintenance
   * failures preserve committed data and discard all derived configuration. */
  for(ordinal=0;ordinal<4;ordinal++) {
    OK(ow_database_create(&d));OK(ow_record_create(d,&r));
    OwValue old={.type=OW_STRING,.as.bytes={"old",3}},next={.type=OW_STRING,.as.bytes={"next",4}};
    OK(ow_field_set(d,r,"key",&old));OK(ow_field_index_enable(d,"key",1,64));cache=d->field_index;before=d->revision;
    arm(ordinal);OwStatus s=ow_field_set(d,r,"key",&next);disarm();
    if(!ordinal){T(s==OW_NOMEM&&d->field_index==cache&&d->revision==before);}
    else {OK(s);T(d->revision==before+1);T(ordinal==3?d->field_index!=NULL:d->field_index==NULL);}
    ow_database_destroy(d);
  }
  OK(ow_database_create(&d));OK(ow_record_create(d,&r));
  OwValue byte={.type=OW_BINARY,.as.bytes={"a",1}},two={.type=OW_BINARY,.as.bytes={"ab",2}};
  OK(ow_field_set(d,r,"key",&byte));OK(ow_field_index_enable(d,"key",1,5));
  OK(ow_field_set(d,r,"key",&two));T(!d->field_index);
  OK(ow_field_index_enable(d,"key",1,6));OK(ow_field_set(d,r,"key",&byte));T(d->field_index->key_bytes==5);
  OK(ow_field_index_disable(d));
  for(ordinal=0;ordinal<3;ordinal++){arm(ordinal);OwStatus s=ow_query_range(d,"key",OW_BINARY,NULL,1,NULL,1,&ids,&n);disarm();if(ordinal<2)T(s==OW_NOMEM&&!ids&&!n);else{OK(s);ow_array_release(ids);}}
  ow_database_destroy(d);
  /* Reserved sentinel IDs must not skip or wrap a real UINT64_MAX ID. */
  OK(ow_database_create(&d));d->rid=UINT64_MAX-1;OK(ow_record_create(d,&r));T(r.value==UINT64_MAX);
  v.type=OW_INT;v.as.integer=17;OK(ow_field_set(d,r,"key",&v));OK(ow_field_index_enable(d,"key",1,4));
  OK(ow_query_range(d,"key",OW_INT,&v,1,&v,1,&ids,&n));T(n==1&&ids[0].value==UINT64_MAX);ow_array_release(ids);
  OK(ow_query_range(d,"key",OW_INT,&v,0,NULL,1,&ids,&n));T(!ids&&!n);ow_database_destroy(d);
  /* Exchange cannot silently transfer advisory configuration or row pointers. */
  OwDatabase *b;OK(ow_database_create(&d));OK(ow_database_create(&b));
  OK(ow_record_create(d,&r));OK(ow_field_set(d,r,"key",&v));OK(ow_field_index_enable(d,"key",1,4));
  OK(ow_record_create(b,&r));OK(ow_field_set(b,r,"key",&v));OK(ow_field_index_enable(b,"key",1,4));
  ow_core_exchange(d,b);T(!d->field_index&&!b->field_index);
  T(!((Record*)ow_index_find(d->records,1))->indexed&&!((Record*)ow_index_find(b->records,1))->indexed);
  ow_database_destroy(d);ow_database_destroy(b);
}
#ifdef OW_UPSTREAM_ALLOC_TEST
static void index_upstream_failures(void) {
  for(int maintenance=0;maintenance<2;maintenance++) {
    size_t ordinal;
    for(ordinal=1;ordinal<32;ordinal++) {
      OwDatabase *d;OwRecordId r;OwValue v={.type=OW_INT,.as.integer=17};
      OK(ow_database_create(&d));OK(ow_record_create(d,&r));OK(ow_field_set(d,r,"key",&v));OK(ow_field_index_enable(d,"key",8,64));
      OwFieldIndex *old=d->field_index;uint64_t before=d->revision;
      lks_alloc_test_fail_on_attempt(ordinal);OwStatus s;
      if(maintenance){v.as.integer=31;s=ow_field_set(d,r,"key",&v);}else s=ow_field_index_enable(d,"key",8,64);
      int triggered=lks_alloc_test_failure_triggered();lks_alloc_test_disable_failure();
      if(maintenance){OK(s);T(d->revision==before+1);T(triggered?d->field_index==NULL:d->field_index!=NULL);}
      else if(triggered)T(s==OW_NOMEM&&d->field_index==old&&d->revision==before);else OK(s);
      OwRecordId *ids;size_t n;OK(ow_query_equal(d,"key",&v,&ids,&n));T(n==1&&ids[0].value==r.value);ow_array_release(ids);
      ow_database_destroy(d);T(lks_alloc_stats_get().live_blocks==0);
      if(!triggered)break;
    }
    T(ordinal<32);printf("field index upstream %s: %zu failing ordinals\n",maintenance?"maintenance":"enable",ordinal-1);
  }
}
#endif
