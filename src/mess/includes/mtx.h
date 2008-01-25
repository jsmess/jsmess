/*************************************************************************

    Memotech MTX 500, MTX 512 and RS 128

*************************************************************************/

#ifndef MTX_H_
#define MTX_H_

#include "devices/snapquik.h"


#define MTX_SYSTEM_CLOCK   XTAL_4MHz


/*----------- defined in drivers/mtx.c -----------*/

extern UINT8 *mtx_ram;


/*----------- defined in machine/mtx.c -----------*/

DRIVER_INIT( mtx512 );
DRIVER_INIT( rs128 );
MACHINE_RESET( rs128 );
INTERRUPT_GEN( mtx_interrupt );
SNAPSHOT_LOAD( mtx );

WRITE8_HANDLER( mtx_bankswitch_w );

/* Cassette */
READ8_HANDLER( mtx_cst_r );
WRITE8_HANDLER( mtx_cst_w );

/* Printer */
READ8_HANDLER( mtx_strobe_r );
READ8_HANDLER( mtx_prt_r );
WRITE8_HANDLER( mtx_prt_w );

/* Keyboard */
WRITE8_HANDLER( mtx_sense_w );
READ8_HANDLER( mtx_key_lo_r );
READ8_HANDLER( mtx_key_hi_r );

/* Z80 CTC */
READ8_HANDLER( mtx_ctc_r );
WRITE8_HANDLER( mtx_ctc_w );

/* Z80 Dart */
READ8_HANDLER( mtx_dart_data_r );
READ8_HANDLER( mtx_dart_control_r );
WRITE8_HANDLER( mtx_dart_data_w );
WRITE8_HANDLER( mtx_dart_control_w );


#endif /*MTX_H_*/
