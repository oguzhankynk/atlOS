#ifndef AE8E0E44_F56A_41D7_B89F_8616FCB53180
#define AE8E0E44_F56A_41D7_B89F_8616FCB53180

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void* kmalloc(size_t);				
void* krealloc(void *, size_t);		
void* kcalloc(size_t, size_t);		
void  kfree(void *);					


#ifdef __cplusplus
}
#endif


#endif /* AE8E0E44_F56A_41D7_B89F_8616FCB53180 */
