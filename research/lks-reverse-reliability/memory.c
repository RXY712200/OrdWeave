#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
typedef union { max_align_t align;size_t size; } Header;
void *__real_malloc(size_t);void *__real_calloc(size_t,size_t);void *__real_realloc(void *,size_t);void __real_free(void *);
static size_t live,peak,attempts;
static void add(size_t n){live+=n;if(live>peak)peak=live;attempts++;}
void *__wrap_malloc(size_t n){if(n>SIZE_MAX-sizeof(Header))return NULL;Header *h=__real_malloc(sizeof(*h)+n);if(!h)return NULL;h->size=n;add(n);return h+1;}
void *__wrap_calloc(size_t n,size_t s){if(s&&n>SIZE_MAX/s)return NULL;size_t size=n*s;void *p=__wrap_malloc(size);if(p)memset(p,0,size);return p;}
void __wrap_free(void *p){if(p){Header *h=(Header*)p-1;live-=h->size;__real_free(h);}}
void *__wrap_realloc(void *p,size_t n){if(!p)return __wrap_malloc(n);if(!n){__wrap_free(p);return NULL;}if(n>SIZE_MAX-sizeof(Header))return NULL;Header *h=(Header*)p-1;size_t old=h->size;h=__real_realloc(h,sizeof(*h)+n);if(!h)return NULL;h->size=n;live-=old;add(n);return h+1;}
void memory_report(const char *phase){printf("memory,%s,%zu,%zu,%zu\n",phase,live,peak,attempts);peak=live;attempts=0;}
