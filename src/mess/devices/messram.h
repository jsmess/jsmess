/*************************************************************************

    MESS RAM device

    Provides a configurable amount of RAM to drivers

**************************************************************************/

#ifndef __MESSRAM_H__
#define __MESSRAM_H__


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define RAM_DEFAULT_VALUE	0xcd

enum
{
	DEVINFO_STR_MESSRAM_SIZE = DEVINFO_STR_DEVICE_SPECIFIC
};


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _ram_config ram_config;
struct _ram_config
{
	const char *default_size;
	const char *extra_options;
	UINT8 default_value;
};


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MESSRAM DEVICE_GET_INFO_NAME(messram)

#define MDRV_RAM_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, MESSRAM, 0) \
	MDRV_DEVICE_CONFIG_DATA32(ram_config, default_value, RAM_DEFAULT_VALUE)

#define MDRV_RAM_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag)

#define MDRV_RAM_MODIFY(_tag) \
	MDRV_DEVICE_MODIFY(_tag)

#define MDRV_RAM_DEFAULT_SIZE(_default_size) \
	MDRV_DEVICE_CONFIG_DATAPTR(ram_config, default_size, _default_size)

#define MDRV_RAM_EXTRA_OPTIONS(_extra_options) \
	MDRV_DEVICE_CONFIG_DATAPTR(ram_config, extra_options, _extra_options)

#define MDRV_RAM_DEFAULT_VALUE(_default_value) \
	MDRV_DEVICE_CONFIG_DATA32(ram_config, default_value, _default_value)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO( messram );
UINT32 messram_get_size(const device_config *device);
UINT8 *messram_get_ptr(const device_config *device);
void messram_dump(const device_config *device, const char *filename);


#endif /* __MESSRAM_H__ */
