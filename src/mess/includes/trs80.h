/*****************************************************************************
 *
 * includes/trs80.h
 *
 ****************************************************************************/

#ifndef TRS80_H_
#define TRS80_H_

#include "devices/snapquik.h"


#define TRS80_FONT_W 6
#define TRS80_FONT_H 12


/*----------- defined in machine/trs80.c -----------*/

extern UINT8 trs80_port_ff;

DEVICE_LOAD( trs80_cas );
DEVICE_UNLOAD( trs80_cas );
DEVICE_LOAD( trs80_floppy );
QUICKLOAD_LOAD( trs80_cmd );

MACHINE_START( trs80 );
MACHINE_RESET( trs80 );
DRIVER_INIT( trs80 );

WRITE8_HANDLER ( trs80_port_ff_w );
READ8_HANDLER ( trs80_port_ff_r );
READ8_HANDLER ( trs80_port_xx_r );

INTERRUPT_GEN( trs80_frame_interrupt );
INTERRUPT_GEN( trs80_timer_interrupt );
INTERRUPT_GEN( trs80_fdc_interrupt );

READ8_HANDLER( trs80_irq_status_r );
WRITE8_HANDLER( trs80_irq_mask_w );

READ8_HANDLER( trs80_printer_r );
WRITE8_HANDLER( trs80_printer_w );

WRITE8_HANDLER( trs80_motor_w );

READ8_HANDLER( trs80_keyboard_r );


/*----------- defined in video/trs80.c -----------*/

VIDEO_START( trs80 );
VIDEO_UPDATE( trs80 );

WRITE8_HANDLER( trs80_videoram_w );


#endif	/* TRS80_H_ */
