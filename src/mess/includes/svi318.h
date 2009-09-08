/*****************************************************************************
 *
 * includes/svi318.h
 *
 * Spectravideo SVI-318 and SVI-328
 *
 ****************************************************************************/

#ifndef SVI318_H_
#define SVI318_H_

#include "machine/i8255a.h"
#include "machine/ins8250.h"
#include "machine/wd17xx.h"

/*----------- defined in machine/svi318.c -----------*/

extern const i8255a_interface svi318_ppi8255_interface;
extern const ins8250_interface svi318_ins8250_interface[2];
extern const wd17xx_interface svi_wd17xx_interface;

DRIVER_INIT( svi318 );
MACHINE_START( svi318_pal );
MACHINE_START( svi318_ntsc );
MACHINE_RESET( svi318 );

DEVICE_START( svi318_cart );
DEVICE_IMAGE_LOAD( svi318_cart );
DEVICE_IMAGE_UNLOAD( svi318_cart );

INTERRUPT_GEN( svi318_interrupt );
void svi318_vdp_interrupt(running_machine *machine, int i);

WRITE8_HANDLER( svi318_writemem1 );
WRITE8_HANDLER( svi318_writemem2 );
WRITE8_HANDLER( svi318_writemem3 );
WRITE8_HANDLER( svi318_writemem4 );

READ8_HANDLER( svi318_io_ext_r );
WRITE8_HANDLER( svi318_io_ext_w );

READ8_DEVICE_HANDLER( svi318_ppi_r );
WRITE8_DEVICE_HANDLER( svi318_ppi_w );

WRITE8_HANDLER( svi318_psg_port_b_w );
READ8_HANDLER( svi318_psg_port_a_r );

int svi318_cassette_present(running_machine *machine, int id);

MC6845_UPDATE_ROW( svi806_crtc6845_update_row );
VIDEO_START( svi328_806 );
VIDEO_UPDATE( svi328_806 );
MACHINE_RESET( svi328_806 );

#endif /* SVI318_H_ */
