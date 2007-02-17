#ifndef __SVISION_H_
#define __SVISION_H_

#include "driver.h"
#include "sound/custom.h"

typedef struct
{
    UINT8 reg[3];
    int pos;
    int size;
} SVISION_CHANNEL;

extern SVISION_CHANNEL svision_channel[2];

void *svision_custom_start(int clock, const struct CustomSound_interface *config);

void svision_soundport_w(SVISION_CHANNEL *channel, int offset, int data);

#endif
