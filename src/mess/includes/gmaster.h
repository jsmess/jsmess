#ifndef __GMASTER_H_
#define __GMASTER_H_

#include "sound/custom.h"

/*----------- defined in audio/gmaster.c -----------*/

int gmaster_io_callback(const device_config *device, int ioline, int state);
CUSTOM_START( gmaster_custom_start );

#endif
