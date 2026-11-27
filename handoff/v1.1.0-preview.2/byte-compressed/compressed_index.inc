/* Compressed byte-radix permanent-ID index, private to the core. Borrowed
 * objects never move. Only branching prefixes and occupied 256-ID leaves are
 * allocated; there is no array or path proportional to the ID high water. */
typedef struct OwOccNode OwOccNode;
struct OwOccNode {
  union { OwOccNode *child; OwIndexed *item; } slot[256];
  uint64_t prefix;
  unsigned shift, used;
};
typedef struct { OwOccNode *root; size_t count; } OwOccIndex;
typedef struct {
  OwOccNode **link, *replacement, *branch, *new_leaf, *leaf, *parent;
  OwIndexed **item;
  int grow;
} OwOccPlan;
typedef struct { OwOccNode *node[8];unsigned pos[8],depth; } OwOccIterator;
static uint64_t occ_prefix(uint64_t id,unsigned shift) {
  return shift==56 ? 0 : id>>(shift+8);
}
static unsigned occ_digit(uint64_t id,unsigned shift) {
  return (unsigned)((id>>shift)&255);
}
static OwOccIndex *occ_create(void) { return calloc(1,sizeof(OwOccIndex)); }
static void occ_free_node(OwOccNode *node) {
  if(!node)return;
  if(node->shift)for(unsigned i=0;i<256;i++)occ_free_node(node->slot[i].child);
  free(node);
}
static void occ_destroy(OwOccIndex *index) {
  if(index){occ_free_node(index->root);free(index);}
}
static void *occ_find(const OwOccIndex *index,uint64_t id) {
  OwOccNode *node=index?index->root:NULL;
  if(!id)return NULL;
  while(node && node->prefix==occ_prefix(id,node->shift)) {
    unsigned digit=occ_digit(id,node->shift);
    if(!node->shift)return node->slot[digit].item;
    node=node->slot[digit].child;
  }
  return NULL;
}
static void occ_abort(OwOccPlan *plan) {
  /* Branch may borrow an old live subtree: free only the two owned pages. */
  free(plan->branch);free(plan->new_leaf);memset(plan,0,sizeof(*plan));
}
static OwStatus occ_prepare(OwOccIndex *index,uint64_t id,OwOccPlan *plan) {
  OwOccNode **link=&index->root,*parent=NULL,*node;
  memset(plan,0,sizeof(*plan));
  if(!id)return OW_INVALID;
  if(occ_find(index,id))return OW_CONFLICT;
  if(index->count==SIZE_MAX)return OW_CAPACITY;
  while((node=*link)!=NULL) {
    if(node->prefix!=occ_prefix(id,node->shift))break;
    if(!node->shift){plan->leaf=node;plan->item=&node->slot[id&255].item;return OW_OK;}
    parent=node;link=&node->slot[occ_digit(id,node->shift)].child;
  }
  plan->new_leaf=calloc(1,sizeof(*plan->new_leaf));
  if(!plan->new_leaf)return OW_NOMEM;
  plan->new_leaf->prefix=id>>8;
  plan->leaf=plan->new_leaf;plan->item=&plan->leaf->slot[id&255].item;
  plan->link=link;plan->parent=parent;plan->grow=node==NULL;
  plan->replacement=plan->new_leaf;
  if(node) {
    /* A mismatch is above the old prefix's low byte. The branching digit
     * strictly exceeds both child shifts, so depth never exceeds 8. */
    uint64_t representative=node->prefix<<(node->shift+8);
    unsigned shift=56;
    while(occ_digit(id,shift)==occ_digit(representative,shift))shift-=8;
    plan->branch=calloc(1,sizeof(*plan->branch));
    if(!plan->branch){occ_abort(plan);return OW_NOMEM;}
    plan->branch->shift=shift;plan->branch->prefix=occ_prefix(id,shift);
    plan->branch->used=2;
    plan->branch->slot[occ_digit(id,shift)].child=plan->new_leaf;
    plan->branch->slot[occ_digit(representative,shift)].child=node;
    plan->replacement=plan->branch;
  }
  return OW_OK;
}
static void occ_commit(OwOccIndex *index,OwOccPlan *plan,OwIndexed *item) {
  *plan->item=item;plan->leaf->used++;
  if(plan->replacement){*plan->link=plan->replacement;if(plan->grow&&plan->parent)plan->parent->used++;}
  index->count++;memset(plan,0,sizeof(*plan));
}
/* Exact live ID required, after explicit Order removal has succeeded. No
 * allocation, comparator, upstream mutation or revision counter is involved. */
static void occ_remove(OwOccIndex *index,uint64_t id) {
  OwOccNode **links[8],*path[8],**link=&index->root,*node;unsigned depth=0;
  while((node=*link)!=NULL){links[depth]=link;path[depth++]=node;if(!node->shift)break;link=&node->slot[occ_digit(id,node->shift)].child;}
  node->slot[id&255].item=NULL;node->used--;index->count--;
  if(node->used)return;
  free(node);*links[depth-1]=NULL;
  if(depth>1) {
    node=path[depth-2];node->used--;
    if(node->used==1) {
      OwOccNode *child=NULL;
      for(unsigned i=0;i<256;i++)if(node->slot[i].child){child=node->slot[i].child;break;}
      *links[depth-2]=child;free(node);
    }
  }
}
static OwIndexed *occ_next(OwOccIterator *it) {
  while(it->depth) {
    unsigned level=it->depth-1;
    if(it->pos[level]==256){it->depth--;continue;}
    unsigned pos=it->pos[level]++;OwOccNode *node=it->node[level];
    if(!node->shift){OwIndexed *item=node->slot[pos].item;if(item)return item;}
    else if(node->slot[pos].child){it->node[level+1]=node->slot[pos].child;it->pos[level+1]=0;it->depth++;}
  }
  return NULL;
}
static OwIndexed *occ_first(const OwOccIndex *index,OwOccIterator *it) {
  memset(it,0,sizeof(*it));if(index&&index->root){it->node[0]=index->root;it->depth=1;}
  return occ_next(it);
}
