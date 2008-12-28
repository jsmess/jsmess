/***************************************************************************

	Cartrige loading

***************************************************************************/

#ifndef CARTSLOT_H
#define CARTSLOT_H

#include "device.h"
#include "image.h"


#define ROM_CART_LOAD(tag,offset,length,flags)	\
	{ NULL, tag, offset, length, ROMENTRYTYPE_CARTRIDGE | (flags) },

#define ROM_MIRROR		0x01000000
#define ROM_NOMIRROR	0x00000000
#define ROM_FULLSIZE	0x02000000
#define ROM_FILL_FF		0x04000000
#define ROM_NOCLEAR		0x08000000

#define CARTSLOT	DEVICE_GET_INFO_NAME(cartslot)


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cartslot_config cartslot_config;
struct _cartslot_config
{
	const char *					extensions;
	int								must_be_loaded;
	device_start_func				device_start;
	device_image_load_func			device_load;
	device_image_unload_func		device_unload;	
	device_image_partialhash_func	device_partialhash;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO(cartslot);


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_CARTSLOT_ADD(_tag) 										\
	MDRV_DEVICE_ADD(_tag, CARTSLOT, 0)									\

#define MDRV_CARTSLOT_REMOVE(_tag)										\
	MDRV_DEVICE_REMOVE(_tag, CARTSLOT)

#define MDRV_CARTSLOT_MODIFY(_tag)										\
	MDRV_DEVICE_MODIFY(_tag, CARTSLOT)									\

#define MDRV_CARTSLOT_EXTENSION_LIST(_extensions)						\
	MDRV_DEVICE_CONFIG_DATAPTR(cartslot_config, extensions, _extensions)

#define MDRV_CARTSLOT_NOT_MANDATORY										\
	MDRV_DEVICE_CONFIG_DATA32(cartslot_config, must_be_loaded, FALSE)

#define MDRV_CARTSLOT_MANDATORY											\
	MDRV_DEVICE_CONFIG_DATA32(cartslot_config, must_be_loaded, TRUE)

#define MDRV_CARTSLOT_START(_start)										\
	MDRV_DEVICE_CONFIG_DATAPTR(cartslot_config, device_start, DEVICE_START_NAME(_start))

#define MDRV_CARTSLOT_LOAD(_load)										\
	MDRV_DEVICE_CONFIG_DATAPTR(cartslot_config, device_load, DEVICE_IMAGE_LOAD_NAME(_load))

#define MDRV_CARTSLOT_UNLOAD(_unload)									\
	MDRV_DEVICE_CONFIG_DATAPTR(cartslot_config, device_unload, DEVICE_IMAGE_UNLOAD_NAME(_unload))

#define MDRV_CARTSLOT_PARTIALHASH(_partialhash)							\
	MDRV_DEVICE_CONFIG_DATAPTR(cartslot_config, device_partialhash, _partialhash)

#endif /* CARTSLOT_H */
