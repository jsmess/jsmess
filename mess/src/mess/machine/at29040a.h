/*
    ATMEL 29040a

    Rewritten as device
    Michael Zapf, September 2010
*/

/* device interface */
#ifndef __AT29040__
#define __AT29040__

typedef struct _at29c040a_config
{
	UINT8					*(*get_memory)(device_t *device);
	offs_t					offset;

} at29c040a_config;

READ8_DEVICE_HANDLER( at29c040a_r );
WRITE8_DEVICE_HANDLER( at29c040a_w );

int at29c040a_is_dirty(device_t *device);

DECLARE_LEGACY_DEVICE( AT29C040A, at29c040a );

#define MCFG_AT29C040_ADD_P(_tag, _memptr, _offset)	\
	MCFG_DEVICE_ADD(_tag, AT29C040A, 0)	\
	MCFG_DEVICE_CONFIG_DATAPTR(at29c040a_config, get_memory, _memptr) \
	MCFG_DEVICE_CONFIG_DATA32(at29c040a_config, offset, _offset)

#endif

