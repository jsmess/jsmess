/*
    TI-99 Mechatronic mouse with adapter
    Michael Zapf, October 2010
*/

#ifndef __MECMOUSE__
#define __MECMOUSE__

void mecmouse_select(running_device *device, int selnow, int stick1, int stick2);
void mecmouse_poll(running_device *device);
int mecmouse_get_values(running_device *device);
int mecmouse_get_values8(running_device *device, int mode);

#define MDRV_MECMOUSE_ADD(_tag )	\
	MDRV_DEVICE_ADD(_tag, MECMOUSE, 0)

DECLARE_LEGACY_DEVICE( MECMOUSE, ti99_mecmouse );
#endif
