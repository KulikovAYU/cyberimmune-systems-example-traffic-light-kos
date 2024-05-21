#include <mode.h>


struct Mode* create_mode(void){
    static struct Mode instance;
    KosMutexInit(&instance.mtx);
    return &instance;
}

void set_mode(struct Mode* mode, const unsigned char newval)
{
    KosMutexLock(&mode->mtx);
    mode->value = newval;
    KosMutexUnlock(&mode->mtx);
}


unsigned char  get_mode(struct Mode* mode){
    unsigned char val;
    KosMutexLock(&mode->mtx);
    val = mode->value;
    KosMutexUnlock(&mode->mtx);
    return val;
}