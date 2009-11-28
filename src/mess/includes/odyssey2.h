/*****************************************************************************
 *
 * includes/odyssey2.h
 *
 ****************************************************************************/

#ifndef ODYSSEY2_H_
#define ODYSSEY2_H_

#include "streams.h"


#define P1_BANK_LO_BIT            (0x01)
#define P1_BANK_HI_BIT            (0x02)
#define P1_KEYBOARD_SCAN_ENABLE   (0x04)  /* active low */
#define P1_VDC_ENABLE             (0x08)  /* active low */
#define P1_EXT_RAM_ENABLE         (0x10)  /* active low */
#define P1_VDC_COPY_MODE_ENABLE   (0x40)

#define P2_KEYBOARD_SELECT_MASK   (0x07)  /* select row to scan */

#define VDC_CONTROL_REG_STROBE_XY (0x02)

#define I824X_START_ACTIVE_SCAN			6
#define I824X_END_ACTIVE_SCAN			(6 + 160)
#define I824X_START_Y					1
#define I824X_SCREEN_HEIGHT				243
#define I824X_LINE_CLOCKS				228

/*----------- defined in video/odyssey2.c -----------*/

extern const UINT8 odyssey2_colors[];

VIDEO_START( odyssey2 );
VIDEO_UPDATE( odyssey2 );
PALETTE_INIT( odyssey2 );
READ8_HANDLER ( odyssey2_t1_r );
READ8_HANDLER ( odyssey2_video_r );
WRITE8_HANDLER ( odyssey2_video_w );
WRITE8_HANDLER ( odyssey2_lum_w );

STREAM_UPDATE( odyssey2_sh_update );

void odyssey2_ef9341_w( int command, int b, UINT8 data );
UINT8 odyssey2_ef9341_r( int command, int b );

#define SOUND_ODYSSEY2		DEVICE_GET_INFO_NAME( odyssey2_sound )

DEVICE_GET_INFO( odyssey2_sound );

/*----------- defined in machine/odyssey2.c -----------*/

DRIVER_INIT( odyssey2 );
MACHINE_RESET( odyssey2 );

/* i/o ports */
READ8_HANDLER ( odyssey2_bus_r );
WRITE8_HANDLER ( odyssey2_bus_w );

READ8_HANDLER( odyssey2_getp1 );
WRITE8_HANDLER ( odyssey2_putp1 );

READ8_HANDLER( odyssey2_getp2 );
WRITE8_HANDLER ( odyssey2_putp2 );

READ8_HANDLER( odyssey2_getbus );
WRITE8_HANDLER ( odyssey2_putbus );

READ8_HANDLER( odyssey2_t0_r );
void odyssey2_the_voice_lrq_callback( const device_config *device, int state );

READ8_HANDLER ( g7400_bus_r );
WRITE8_HANDLER ( g7400_bus_w );

int odyssey2_cart_verify(const UINT8 *cartdata, size_t size);


#endif /* ODYSSEY2_H_ */
