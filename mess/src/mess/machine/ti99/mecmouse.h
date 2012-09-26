/*
    TI-99 Mechatronic mouse with adapter
    Michael Zapf, October 2010
*/

#ifndef __MECMOUSE__
#define __MECMOUSE__

void mecmouse_select(device_t *device, int selnow, int stick1, int stick2);
void mecmouse_poll(device_t *device);
int mecmouse_get_values(device_t *device);
int mecmouse_get_values8(device_t *device, int mode);

#define MCFG_MECMOUSE_ADD(_tag )	\
	MCFG_DEVICE_ADD(_tag, MECMOUSE, 0)

DECLARE_LEGACY_DEVICE( MECMOUSE, ti99_mecmouse );
#endif
