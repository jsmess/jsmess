/*****************************************************************************
 *
 * includes/poly88.h
 *
 ****************************************************************************/

#ifndef POLY88_H_
#define POLY88_H_

/*----------- defined in machine/poly88.c -----------*/

MACHINE_RESET(poly88);
INTERRUPT_GEN( poly88_interrupt );
READ8_HANDLER(polly_keyboard_r);
WRITE8_HANDLER(polly_intr_w);

/*----------- defined in video/poly88.c -----------*/
extern UINT8 *poly_video_ram;

extern VIDEO_START( poly88 );
extern VIDEO_UPDATE( poly88 );

#endif /* POLY88_H_ */
