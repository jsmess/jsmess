/***************************************************************************

	Cartrige loading

***************************************************************************/

#ifndef CARTSLOT_H
#define CARTSLOT_H

#include "device.h"
#include "image.h"


#define ROM_CART_LOAD(index,extensions,offset,length,flags)	\
	{ ROMENTRY_CARTRIDGE, offset, length, flags, "#" #index "\0" extensions },

#define ROM_MIRROR		0x00000001
#define ROM_NOMIRROR	0x00000000
#define ROM_FULLSIZE	0x00000002
#define ROM_FILL_FF		0x00000004
#define ROM_NOCLEAR		0x00000008

void cartslot_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);


#endif /* CARTSLOT_H */
