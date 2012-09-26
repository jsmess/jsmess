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


void ti99_handset_task(device_t *device);
void ti99_handset_set_acknowledge(device_t *device, UINT8 data);
int ti99_handset_poll_bus(device_t *device);
int ti99_handset_get_clock(device_t *device);
int ti99_handset_get_int(device_t *device);

#define MCFG_HANDSET_ADD(_tag, _sysintf )	\
	MCFG_DEVICE_ADD(_tag, HANDSET, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_handset_config, sysintf, _sysintf)

DECLARE_LEGACY_DEVICE( HANDSET, ti99_handset );
#endif
