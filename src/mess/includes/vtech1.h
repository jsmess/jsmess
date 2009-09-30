/*****************************************************************************
 *
 * includes/vtech1.h
 *
 ****************************************************************************/

#ifndef VTECH1_H_
#define VTECH1_H_

#include "devices/snapquik.h"


#define VTECH1_CLK        3579500
#define VZ300_XTAL1_CLK   XTAL_17_73447MHz

#define VZ_BASIC 0xf0
#define VZ_MCODE 0xf1


/*----------- defined in machine/vtech1.c -----------*/

extern int vtech1_latch;

MACHINE_START( laser110 );
MACHINE_START( laser210 );
MACHINE_START( laser310 );

DEVICE_IMAGE_LOAD( vtech1_floppy );
SNAPSHOT_LOAD( vtech1 );

READ8_HANDLER ( vtech1_printer_r );
WRITE8_HANDLER( vtech1_strobe_w );
READ8_HANDLER ( vtech1_fdc_r );
WRITE8_HANDLER( vtech1_fdc_w );
READ8_HANDLER ( vtech1_joystick_r );
READ8_HANDLER ( vtech1_lightpen_r );
READ8_HANDLER ( vtech1_keyboard_r );
WRITE8_HANDLER( vtech1_latch_w );
READ8_HANDLER ( vtech1_serial_r );
WRITE8_HANDLER( vtech1_serial_w );
WRITE8_HANDLER( vtech1_memory_bank_w );

READ8_DEVICE_HANDLER(vtech1_mc6847_videoram_r);
VIDEO_UPDATE(vtech1);


#endif /* VTECH1_H_ */
