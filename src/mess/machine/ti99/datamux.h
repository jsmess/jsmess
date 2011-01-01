#ifndef __DMUX__
#define __DMUX__

typedef void (*databus_read_function)(device_t *device, offs_t offset, UINT8 *value);
typedef void (*databus_write_function)(device_t *device, offs_t offset, UINT8 value);

/* device interface */
DECLARE_LEGACY_DEVICE( DMUX, datamux );

READ16_DEVICE_HANDLER(  ti99_dmux_r );
WRITE16_DEVICE_HANDLER( ti99_dmux_w );

typedef struct _bus_device
{
	const char				*name;
	UINT16					address_mask;
	UINT16					select_pattern;
	UINT16					write_select;
	databus_read_function	read;
	databus_write_function	write;
	const char				*setting;
	UINT8					set;
	UINT8					unset;
} bus_device;

#define MCFG_DMUX_ADD(_tag, _devices)			\
	MCFG_DEVICE_ADD(_tag, DMUX, 0) \
	MCFG_DEVICE_CONFIG( _devices )
#endif
