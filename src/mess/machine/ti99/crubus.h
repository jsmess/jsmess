#ifndef __CRUBUS__
#define __CRUBUS__
/*
    TI-99 CRU Bus
    header file for crubus
*/

typedef void (*cru_read_function)(device_t *device, offs_t offset, UINT8 *value);
typedef void (*cru_write_function)(device_t *device, offs_t offset, UINT8 value);

typedef struct _cruconf_device
{
	const char				*name;
	UINT16					address_mask;
	UINT16					select_pattern;
	cru_read_function		read;
	cru_write_function		write;
} cruconf_device;

READ8_DEVICE_HANDLER( ti99_crubus_r );
WRITE8_DEVICE_HANDLER( ti99_crubus_w );

/* device interface */
DECLARE_LEGACY_DEVICE( CRUBUS, crubus );

#define MCFG_CRUBUS_ADD(_tag, _devices)			\
	MCFG_DEVICE_ADD(_tag, CRUBUS, 0) \
	MCFG_DEVICE_CONFIG( _devices )

#endif
