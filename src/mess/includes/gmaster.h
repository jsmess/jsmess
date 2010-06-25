#ifndef __GMASTER_H__
#define __GMASTER_H__


/*----------- defined in audio/gmaster.c -----------*/

DECLARE_LEGACY_SOUND_DEVICE(GMASTER, gmaster_sound);

int gmaster_io_callback(running_device *device, int ioline, int state);

#endif /* __GMASTER_H__ */
