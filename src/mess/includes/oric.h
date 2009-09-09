/*****************************************************************************
 *
 * includes/oric.h
 *
 ****************************************************************************/

#ifndef ORIC_H_
#define ORIC_H_

#include "machine/6522via.h"
#include "machine/wd17xx.h"

/*----------- defined in machine/oric.c -----------*/

extern const via6522_interface oric_6522_interface;
extern const via6522_interface telestrat_via2_interface;
extern const wd17xx_interface oric_wd17xx_interface;

MACHINE_START( oric );
MACHINE_RESET( oric );
READ8_HANDLER( oric_IO_r );
WRITE8_HANDLER( oric_IO_w );
READ8_HANDLER( oric_microdisc_r );
WRITE8_HANDLER( oric_microdisc_w );
extern UINT8 *oric_ram;

WRITE8_HANDLER(oric_psg_porta_write);

/* Telestrat specific */
MACHINE_START( telestrat );


/*----------- defined in video/oric.c -----------*/

VIDEO_START( oric );
VIDEO_UPDATE( oric );


#endif /* ORIC_H_ */
