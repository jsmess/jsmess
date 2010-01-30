#ifndef __GMASTER_H__
#define __GMASTER_H__


/*----------- defined in audio/gmaster.c -----------*/

#define SOUND_GMASTER	DEVICE_GET_INFO_NAME( gmaster_sound )

int gmaster_io_callback(running_device *device, int ioline, int state);
DEVICE_GET_INFO( gmaster_sound );

#endif /* __GMASTER_H__ */
