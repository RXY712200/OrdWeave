/* Owned advisory keys; Record pointers are borrowed only while their owner
 * lives. The core releases cache rows before Record destruction/exchange. */
typedef struct OwFieldRow {
  uint64_t id;
  OwValue value;
  Record *record;
  const LksOrderHandle *handle;
  struct OwFieldRow *next;
} OwFieldRow;
struct OwFieldIndex {
  char *name;
  LksManagedOrder *order;
  size_t count, key_bytes, max_records, max_key_bytes;
};
static int fi_value_compare(const OwValue *a,const OwValue *b) {
  if(a->type!=b->type)return(a->type>b->type)-(a->type<b->type);
  switch(a->type) {
  case OW_NULL:return 0;
  case OW_BOOL:return(a->as.boolean>b->as.boolean)-(a->as.boolean<b->as.boolean);
  case OW_INT:return(a->as.integer>b->as.integer)-(a->as.integer<b->as.integer);
  case OW_FLOAT:return(a->as.real>b->as.real)-(a->as.real<b->as.real);
  default:{size_t an=a->as.bytes.size,bn=b->as.bytes.size,n=an<bn?an:bn;
    int c=n?memcmp(a->as.bytes.data,b->as.bytes.data,n):0;
    return c?c:(an>bn)-(an<bn);}
  }
}
static size_t fi_bytes(const OwValue *v) {
  return v->type==OW_STRING||v->type==OW_BINARY?v->as.bytes.size:0;
}
static int fi_compare(const void *a,const void *b,void *context) {
  const OwFieldRow *x=a,*y=b;(void)context;
  int c=fi_value_compare(&x->value,&y->value);
  return c?c:(x->id>y->id)-(x->id<y->id);
}
static void fi_free_row(OwFieldRow *row) {
  if(row){ow_value_release(&row->value);free(row);}
}
static void fi_dispose(OwFieldIndex *index) {
  if(!index)return;
  OwFieldRow *head=NULL;
  for(const LksOrderHandle *h=lks_managed_order_first(index->order);h;h=lks_order_next(h)) {
    OwFieldRow *row=lks_order_item(h);row->next=head;head=row;
  }
  lks_managed_order_destroy(index->order);
  while(head){OwFieldRow *next=head->next;
    if(head->record->indexed==head)head->record->indexed=NULL;
    fi_free_row(head);head=next;
  }
  free(index->name);free(index);
}
static void fi_clear(OwDatabase *d) {
  OwFieldIndex *old=d->field_index;d->field_index=NULL;fi_dispose(old);
}
static OwStatus fi_row(Record *r,const OwValue *value,OwFieldRow **out) {
  *out=calloc(1,sizeof(**out));if(!*out)return OW_NOMEM;
  (*out)->id=r->key.id;(*out)->record=r;
  OwStatus s=copy_value(value,&(*out)->value);
  if(s!=OW_OK){free(*out);*out=NULL;}return s;
}
OwStatus ow_field_index_enable(OwDatabase *d,const char *name,
                              size_t max_records,size_t max_key_bytes) {
  OwStatus s;
  if(!d||!max_records||!max_key_bytes)return OW_INVALID;
  if((s=name_valid(name))!=OW_OK)return s;
  size_t bytes=strlen(name)+1;
  if(bytes>max_key_bytes)return OW_CAPACITY;
  OwFieldIndex *index=calloc(1,sizeof(*index));if(!index)return OW_NOMEM;
  index->max_records=max_records;index->max_key_bytes=max_key_bytes;index->key_bytes=bytes;
  index->name=malloc(bytes);if(!index->name){fi_dispose(index);return OW_NOMEM;}
  memcpy(index->name,name,bytes);LksComparator cmp={fi_compare,NULL};
  index->order=lks_managed_order_create(&cmp);
  if(!index->order){fi_dispose(index);return OW_NOMEM;}
  for(const LksOrderHandle *h=lks_managed_order_first(d->records);h;h=lks_order_next(h)) {
    Record *r=lks_order_item(h);Field *f=*find_field(r,name);if(!f)continue;
    size_t n=fi_bytes(&f->value);
    if(index->count==max_records||n>max_key_bytes-index->key_bytes){s=OW_CAPACITY;goto fail;}
    OwFieldRow *row=NULL;s=fi_row(r,&f->value,&row);if(s!=OW_OK)goto fail;
    s=ow_lks_status(lks_managed_order_insert(index->order,row,&row->handle));
    if(s!=OW_OK){fi_free_row(row);goto fail;}
    index->count++;index->key_bytes+=n;
  }
  fi_clear(d);d->field_index=index;
  for(const LksOrderHandle *h=lks_managed_order_first(index->order);h;h=lks_order_next(h)) {
    OwFieldRow *row=lks_order_item(h);row->record->indexed=row;
  }
  return OW_OK;
fail:fi_dispose(index);return s;
}
OwStatus ow_field_index_disable(OwDatabase *d) {
  if(!d)return OW_INVALID;fi_clear(d);return OW_OK;
}
/* Called only after a successful relevant core mutation, while all borrowed
 * Records still live. Maintenance never changes the data operation's status. */
static void fi_update(OwDatabase *d,Record *r,int deleting) {
  OwFieldIndex *index=d->field_index;if(!index)return;
  OwFieldRow *old=r->indexed,*row=NULL;
  Field *field=deleting?NULL:*find_field(r,index->name);
  size_t base=index->key_bytes-(old?fi_bytes(&old->value):0);
  if(field) {
    size_t n=fi_bytes(&field->value);
    if((!old&&index->count==index->max_records)||n>index->max_key_bytes-base ||
       fi_row(r,&field->value,&row)!=OW_OK){fi_clear(d);return;}
  }
  if(old) {
    if(lks_managed_order_remove(index->order,old->handle,NULL)!=LKS_STATUS_OK){fi_free_row(row);fi_clear(d);return;}
    r->indexed=NULL;index->count--;index->key_bytes=base;fi_free_row(old);
  }
  if(row) {
    if(lks_managed_order_insert(index->order,row,&row->handle)!=LKS_STATUS_OK){fi_free_row(row);fi_clear(d);return;}
    index->count++;index->key_bytes+=fi_bytes(&row->value);r->indexed=row;
  }
}
static int fi_matches(const OwValue *v,OwType type,const OwValue *lo,int li,
                      const OwValue *hi,int ui) {
  if(v->type!=type)return 0;
  int l=lo?fi_value_compare(v,lo):1,u=hi?fi_value_compare(v,hi):-1;
  return (!lo||l>0||(li&&l==0))&&(!hi||u<0||(ui&&u==0));
}
static OwStatus fi_collect(OwFieldIndex *index,OwType type,const OwValue *lo,int li,
                           const OwValue *hi,int ui,OwRecordId **out,size_t *count) {
  OwFieldRow key={0};key.value.type=type;
  if(lo)key.value=*lo;
  else if(type==OW_INT)key.value.as.integer=INT64_MIN;
  else if(type==OW_FLOAT)key.value.as.real=-DBL_MAX;
  key.id=lo&&!li?UINT64_MAX:0;
  const LksOrderHandle *pred,*eq,*first;
  OwStatus s=ow_lks_status(lks_managed_order_locate(index->order,&key,&pred,&eq,&first));
  if(s!=OW_OK)return s;
  size_t n=0;
  for(const LksOrderHandle *h=first;h;h=lks_order_next(h)) {
    OwFieldRow *row=lks_order_item(h);if(!fi_matches(&row->value,type,lo,li,hi,ui))break;
    if(n==SIZE_MAX/sizeof(OwRecordId))return OW_CAPACITY;n++;
  }
  if(!n)return OW_OK;
  OwRecordId *ids=malloc(n*sizeof(*ids));if(!ids)return OW_NOMEM;
  const LksOrderHandle *h=first;
  for(size_t i=0;i<n;i++,h=lks_order_next(h))ids[i].value=((OwFieldRow*)lks_order_item(h))->id;
  *out=ids;*count=n;return OW_OK;
}
static int fi_record_compare(const void *a,const void *b,void *context) {
  Record *x=(Record*)a,*y=(Record*)b;const char *name=context;
  int c=fi_value_compare(&(*find_field(x,name))->value,&(*find_field(y,name))->value);
  return c?c:(x->key.id>y->key.id)-(x->key.id<y->key.id);
}
OwStatus ow_query_range(OwDatabase *d,const char *name,OwType type,
                        const OwValue *lo,int li,const OwValue *hi,int ui,
                        OwRecordId **out,size_t *count) {
  if(out)*out=NULL;if(count)*count=0;
  if(!d||!out||!count||(unsigned)type>OW_BINARY||(li!=0&&li!=1)||(ui!=0&&ui!=1))return OW_INVALID;
  OwStatus s=name_valid(name);if(s!=OW_OK)return s;
  if(lo){if(lo->type!=type)return OW_INVALID;if((s=value_valid(lo))!=OW_OK)return s;}
  if(hi){if(hi->type!=type)return OW_INVALID;if((s=value_valid(hi))!=OW_OK)return s;}
  if(lo&&hi){int c=fi_value_compare(lo,hi);if(c>0)return OW_INVALID;if(!c&&(!li||!ui))return OW_OK;}
  if(d->field_index&&!strcmp(d->field_index->name,name)) {
    s=fi_collect(d->field_index,type,lo,li,hi,ui,out,count);
    if(s==OW_OK||s==OW_NOMEM||s==OW_CAPACITY)return s;
    fi_clear(d);
  }
  size_t n=0;
  for(const LksOrderHandle *h=lks_managed_order_first(d->records);h;h=lks_order_next(h)) {
    Field *f=*find_field(lks_order_item(h),name);
    if(f&&fi_matches(&f->value,type,lo,li,hi,ui)){if(n==SIZE_MAX/sizeof(void*)||n==SIZE_MAX/sizeof(OwRecordId))return OW_CAPACITY;n++;}
  }
  if(!n)return OW_OK;
  void **items=malloc(n*sizeof(*items));if(!items)return OW_NOMEM;size_t i=0;
  for(const LksOrderHandle *h=lks_managed_order_first(d->records);h;h=lks_order_next(h)) {
    Record *r=lks_order_item(h);Field *f=*find_field(r,name);
    if(f&&fi_matches(&f->value,type,lo,li,hi,ui))items[i++]=r;
  }
  s=ow_lks_status(lks_sort(items,n,fi_record_compare,(void*)name));
  if(s!=OW_OK){free(items);return s;}
  OwRecordId *ids=malloc(n*sizeof(*ids));if(!ids){free(items);return OW_NOMEM;}
  for(i=0;i<n;i++)ids[i].value=((Record*)items[i])->key.id;
  free(items);*out=ids;*count=n;return OW_OK;
}
