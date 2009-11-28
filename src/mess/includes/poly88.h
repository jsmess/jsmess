/*****************************************************************************
 *
 * includes/poly88.h
 *
 ****************************************************************************/

#ifndef POLY88_H_
#define POLY88_H_
#include "machine/msm8251.h"
#include "devices/snapquik.h"

/*----------- defined in machine/poly88.c -----------*/

DRIVER_INIT(poly88);
MACHINE_RESET(poly88);
INTERRUPT_GEN( poly88_interrupt );
READ8_HANDLER(poly88_keyboard_r);
WRITE8_HANDLER(poly88_intr_w);
WRITE8_HANDLER(poly88_baud_rate_w);

extern const msm8251_interface poly88_usart_interface;

extern SNAPSHOT_LOAD( poly88 );


/*----------- defined in video/poly88.c -----------*/

extern UINT8 *poly88_video_ram;

extern VIDEO_START( poly88 );
extern VIDEO_UPDATE( poly88 );

#endif /* POLY88_H_ */

