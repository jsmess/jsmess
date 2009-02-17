#ifndef __GMASTER_H__
#define __GMASTER_H__

#include "sound/custom.h"

/*----------- defined in audio/gmaster.c -----------*/

int gmaster_io_callback(const device_config *device, int ioline, int state);
CUSTOM_START( gmaster_custom_start );

#endif /* __GMASTER_H__ */
