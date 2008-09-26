/*
	snescart.h

*/

#ifndef _SNESCART_H
#define _SNESCART_H


MACHINE_START( snes_mess );

void snes_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

#endif /* _SNESCART_H */

