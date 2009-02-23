/*************************************************************************

    Memotech MTX 500, MTX 512 and RS 128

*************************************************************************/

#ifndef __MTX_H__
#define __MTX_H__

#include "devices/snapquik.h"


#define MTX_SYSTEM_CLOCK   XTAL_4MHz


/*----------- defined in drivers/mtx.c -----------*/

extern UINT8 *mtx_ram;


/*----------- defined in machine/mtx.c -----------*/

DRIVER_INIT( mtx512 );
DRIVER_INIT( rs128 );
INTERRUPT_GEN( mtx_interrupt );
SNAPSHOT_LOAD( mtx );

WRITE8_HANDLER( mtx_bankswitch_w );

/* Cassette */
READ8_HANDLER( mtx_cst_r );
WRITE8_HANDLER( mtx_cst_w );

/* Printer */
READ8_DEVICE_HANDLER( mtx_strobe_r );
READ8_DEVICE_HANDLER( mtx_prt_r );

/* Keyboard */
WRITE8_HANDLER( mtx_sense_w );
READ8_HANDLER( mtx_key_lo_r );
READ8_HANDLER( mtx_key_hi_r );


#endif /* __MTX_H__ */
