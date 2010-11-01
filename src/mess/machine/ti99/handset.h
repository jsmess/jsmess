/*
    TI-99/4 handset
    Michael Zapf, October 2010
*/

#ifndef __HANDSET__
#define __HANDSET__

typedef struct _ti99_handset_config
{
	const char				*sysintf;
} ti99_handset_config;


void ti99_handset_task(running_device *device);
void ti99_handset_set_acknowledge(running_device *device, UINT8 data);
int ti99_handset_poll_bus(running_device *device);
int ti99_handset_get_clock(running_device *device);
int ti99_handset_get_int(running_device *device);

#define MDRV_HANDSET_ADD(_tag, _sysintf )	\
	MDRV_DEVICE_ADD(_tag, HANDSET, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(ti99_handset_config, sysintf, _sysintf)

DECLARE_LEGACY_DEVICE( HANDSET, ti99_handset );
#endif
