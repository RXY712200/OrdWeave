/* Private permanent-ID radix index. Containers borrow stable OwIndexed items.
 * Eight byte digits cover all uint64 IDs without high-water-sized allocation.
 * A prepared missing suffix is unpublished until explicit Order succeeds. */
typedef struct OwOccNode OwOccNode;
struct OwOccNode {
  union { OwOccNode *child; OwIndexed *item; } slot[256];
  unsigned used;
};
typedef struct { OwOccNode *root; size_t count; } OwOccIndex;
typedef struct {
  OwOccNode **link, *branch, *parent, *leaf;
  OwIndexed **item;
} OwOccPlan;
typedef struct {
  OwOccNode *node[8];
  unsigned pos[8], depth;
} OwOccIterator;
static unsigned occ_digit(uint64_t id,unsigned level) {
  return (unsigned)((id >> (56-8*level)) & 255);
}
static OwOccIndex *occ_create(void) { return calloc(1,sizeof(OwOccIndex)); }
static void occ_free_node(OwOccNode *node,unsigned level) {
  if(!node) return;
  if(level<7) for(unsigned i=0;i<256;i++)
    occ_free_node(node->slot[i].child,level+1);
  free(node);
}
static void occ_destroy(OwOccIndex *index) {
  if(index) { occ_free_node(index->root,0); free(index); }
}
static void *occ_find(const OwOccIndex *index,uint64_t id) {
  OwOccNode *node=index ? index->root : NULL;
  if(!id) return NULL;
  for(unsigned level=0;node && level<7;level++)
    node=node->slot[occ_digit(id,level)].child;
  return node ? node->slot[occ_digit(id,7)].item : NULL;
}
static void occ_abort(OwOccPlan *plan) {
  /* Branch can begin at any digit; its depth is recovered from the leaf path
   * by storing it explicitly below rather than freeing borrowed payloads. */
  OwOccNode *node=plan->branch;
  while(node) {
    OwOccNode *next=NULL;
    if(node!=plan->leaf) for(unsigned i=0;i<256;i++)
      if(node->slot[i].child) { next=node->slot[i].child;break; }
    free(node);node=next;
  }
  memset(plan,0,sizeof(*plan));
}
static OwStatus occ_prepare(OwOccIndex *index,uint64_t id,OwOccPlan *plan) {
  OwOccNode **link=&index->root,*parent=NULL,*node;
  memset(plan,0,sizeof(*plan));
  if(!id) return OW_INVALID;
  if(occ_find(index,id)) return OW_CONFLICT;
  if(index->count==SIZE_MAX) return OW_CAPACITY;
  for(unsigned level=0;level<8;level++) {
    node=*link;
    if(!node) {
      node=calloc(1,sizeof(*node));
      if(!node) { occ_abort(plan);return OW_NOMEM; }
      if(!plan->branch) {plan->link=link;plan->branch=node;plan->parent=parent;}
      else { *link=node;parent->used++; }
      /* A partially prepared suffix contains only child pointers until the
       * leaf is set; this marker also permits allocation-failure teardown. */
      plan->leaf=node;
    }
    if(level==7) {plan->leaf=node;plan->item=&node->slot[occ_digit(id,7)].item;break;}
    parent=node;link=&node->slot[occ_digit(id,level)].child;
  }
  return OW_OK;
}
static void occ_commit(OwOccIndex *index,OwOccPlan *plan,OwIndexed *item) {
  *plan->item=item;plan->leaf->used++;
  if(plan->branch) { *plan->link=plan->branch;if(plan->parent) plan->parent->used++; }
  index->count++;memset(plan,0,sizeof(*plan));
}
/* Called only after exact lookup and successful explicit Order removal.
 * No allocation, comparator, upstream mutation or revision counter is used. */
static void occ_remove(OwOccIndex *index,uint64_t id) {
  OwOccNode *path[8],*node=index->root;
  for(unsigned level=0;level<8;level++) {
    path[level]=node;
    if(level<7) node=node->slot[occ_digit(id,level)].child;
  }
  node->slot[occ_digit(id,7)].item=NULL;node->used--;index->count--;
  for(unsigned level=8;level>0;level--) {
    node=path[level-1];if(node->used) break;free(node);
    if(level==1) index->root=NULL;
    else {path[level-2]->slot[occ_digit(id,level-2)].child=NULL;path[level-2]->used--;}
  }
}
static OwIndexed *occ_next(OwOccIterator *it) {
  while(it->depth) {
    unsigned level=it->depth-1;
    if(it->pos[level]==256) {it->depth--;continue;}
    unsigned pos=it->pos[level]++;
    if(level==7) {OwIndexed *item=it->node[level]->slot[pos].item;if(item)return item;}
    else if(it->node[level]->slot[pos].child) {
      it->node[level+1]=it->node[level]->slot[pos].child;it->pos[level+1]=0;it->depth++;
    }
  }
  return NULL;
}
static OwIndexed *occ_first(const OwOccIndex *index,OwOccIterator *it) {
  memset(it,0,sizeof(*it));if(index && index->root){it->node[0]=index->root;it->depth=1;}
  return occ_next(it);
}
