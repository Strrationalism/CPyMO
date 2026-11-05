#include <stddef.h>
#include <malloc.h>
extern volatile unsigned engine_state[46];
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void *__real_realloc(void*,size_t);
static void observe(void *p,size_t size){
 unsigned used=(unsigned)mallinfo().uordblks;
 if(used>engine_state[8])engine_state[8]=used;
 if(!p && size)++engine_state[9];
}
extern void nds_scene_release(void);
void *__wrap_malloc(size_t size){void *p=__real_malloc(size);if(!p&&size){nds_scene_release();p=__real_malloc(size);}observe(p,size);return p;}
void *__wrap_calloc(size_t n,size_t size){void *p=__real_calloc(n,size);if(!p&&n&&size){nds_scene_release();p=__real_calloc(n,size);}observe(p,n && size);return p;}
void *__wrap_realloc(void *old,size_t size){void *p=__real_realloc(old,size);if(!p&&size){nds_scene_release();p=__real_realloc(old,size);}observe(p,size);return p;}
