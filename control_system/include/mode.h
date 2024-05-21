#ifndef TRAFFIC_LIGHT_MODE_H
#define TRAFFIC_LIGHT_MODE_H

#include <kos/mutex.h>
#define LogPrefix "[ControlSystem]"


struct Mode{
    unsigned char value;
    KosMutex mtx;
};

struct Mode* create_mode(void);
void set_mode(struct Mode* mode, const unsigned char newval);
unsigned char get_mode(struct Mode* mode);

#endif //TRAFFIC_LIGHT_MODE_H
