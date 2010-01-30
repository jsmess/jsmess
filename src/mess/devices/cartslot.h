/***************************************************************************

    Cartrige loading

***************************************************************************/

#ifndef __CARTSLOT_H__
#define __CARTSLOT_H__

#include "image.h"
#include "multcart.h"


/***************************************************************************
    MACROS
***************************************************************************/
#define TAG_PCB		"pcb"

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

typedef struct _cartslot_t cartslot_t;
struct _cartslot_t
{
	running_device *pcb_device;
	multicart_t *mc;
};


typedef struct _cartslot_pcb_type cartslot_pcb_type;
struct _cartslot_pcb_type
{
	const char *					name;
	device_type						devtype;
};


typedef struct _cartslot_config cartslot_config;
struct _cartslot_config
{
	const char *					extensions;
	const char *					software_list_name;
	int								must_be_loaded;
	device_start_func				device_start;
	device_image_load_func			device_load;
	device_image_unload_func		device_unload;
	device_image_partialhash_func	device_partialhash;
	cartslot_pcb_type				pcb_types[8];
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO(cartslot);

/* accesses the PCB associated with this cartslot */
running_device *cartslot_get_pcb(running_device *device);

/* accesses a particular socket */
void *cartslot_get_socket(running_device *device, const char *socket_name);

/* accesses a particular socket; gets the length of the associated resource */
int cartslot_get_resource_length(running_device *device, const char *socket_name);


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_CARTSLOT_ADD(_tag) 										\
	MDRV_DEVICE_ADD(_tag, CARTSLOT, 0)									\

#define MDRV_CARTSLOT_MODIFY(_tag)										\
	MDRV_DEVICE_MODIFY(_tag)									\

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

#define MDRV_CARTSLOT_PCBTYPE(_index, _pcb_type_name, _pcb_devtype)			\
	MDRV_DEVICE_CONFIG_DATAPTR_ARRAY_MEMBER(cartslot_config, pcb_types, _index, cartslot_pcb_type, name, _pcb_type_name) \
	MDRV_DEVICE_CONFIG_DATAPTR_ARRAY_MEMBER(cartslot_config, pcb_types, _index, cartslot_pcb_type, devtype, _pcb_devtype)

#define MDRV_CARTSLOT_SOFTWARE_LIST(_listname)							\
	MDRV_DEVICE_CONFIG_DATAPTR(cartslot_config, software_list_name, #_listname )


/***************************************************************************
    FREQUENTLY OCCURING REGION NAMES
***************************************************************************/

#define CARTRIDGE_REGION_ROM	"rom"
#define CARTRIDGE_REGION_CHR	"chr"
#define CARTRIDGE_REGION_PRG	"prg"

#endif /* __CARTSLOT_H__ */
