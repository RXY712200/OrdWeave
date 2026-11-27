#include <stddef.h>
void *__real_malloc(size_t);void *__real_calloc(size_t,size_t);void *__real_realloc(void*,size_t);
static int armed,seen;
void arm_failure(void){armed=1;}
int fault_seen(void){return seen;}
static int fail(void){if(!armed)return 0;armed=0;seen=1;return 1;}
void *__wrap_malloc(size_t n){return fail()?NULL:__real_malloc(n);}
void *__wrap_calloc(size_t n,size_t s){return fail()?NULL:__real_calloc(n,s);}
void *__wrap_realloc(void*p,size_t n){return fail()?NULL:__real_realloc(p,n);}
