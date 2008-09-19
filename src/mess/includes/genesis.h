/*****************************************************************************
 *
 *	includes/genesis.h
 *
 ****************************************************************************/

#ifndef GENESIS_H_
#define GENESIS_H_


/*----------- defined in machine/genesis.c -----------*/

MACHINE_RESET( md_mappers );
DRIVER_INIT( gencommon );

void genesis_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void pico_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);


#endif /* GENESIS_H_ */
