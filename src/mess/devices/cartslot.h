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

typedef struct cartslot_interface_t	cartslot_interface;
struct cartslot_interface_t
{
	const char *extensions;
	int must_be_loaded;
	device_start_func	     	  device_start;
	device_image_load_func	 	  device_load;
	device_image_unload_func 	  device_unload;	
	device_image_partialhash_func device_partialhash;
};

extern const cartslot_interface default_cartslot;

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/
DEVICE_GET_INFO(cartslot);

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_CARTSLOT_ADD(_tag, _config) 	\
	MDRV_DEVICE_ADD(_tag, CARTSLOT, 0)			\
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_CARTSLOT_REMOVE(_tag)			\
	MDRV_DEVICE_REMOVE(_tag, CARTSLOT)

#define MDRV_CARTSLOT_MODIFY(_tag, _config)	\
	MDRV_DEVICE_MODIFY(_tag, CARTSLOT)		\
	MDRV_DEVICE_CONFIG(_config)

#endif /* CARTSLOT_H */
