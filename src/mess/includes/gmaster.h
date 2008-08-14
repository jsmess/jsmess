#ifndef __GMASTER_H_
#define __GMASTER_H_

#include "sound/custom.h"

/*----------- defined in audio/gmaster.c -----------*/

int gmaster_io_callback(int ioline, int state);
void* gmaster_custom_start (int clock, const custom_sound_interface *config);
//extern void gmaster_custom_stop (void);
//extern void gmaster_custom_update (void);

#endif
