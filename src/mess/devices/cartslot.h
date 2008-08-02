/***************************************************************************

	Cartrige loading

***************************************************************************/

#ifndef CARTSLOT_H
#define CARTSLOT_H

#include "device.h"
#include "image.h"


#define ROM_CART_LOAD(index,extensions,offset,length,flags)	\
	{ NULL, "#" #index "\0" extensions, offset, length, ROMENTRYTYPE_CARTRIDGE | (flags) },

#define ROM_MIRROR		0x01000000
#define ROM_NOMIRROR	0x00000000
#define ROM_FULLSIZE	0x02000000
#define ROM_FILL_FF		0x04000000
#define ROM_NOCLEAR		0x08000000

void cartslot_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);


#endif /* CARTSLOT_H */
