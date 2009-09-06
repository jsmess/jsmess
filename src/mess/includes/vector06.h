/*****************************************************************************
 *
 * includes/vector06.h
 *
 ****************************************************************************/

#ifndef VECTOR06_H_
#define VECTOR06_H_

#include "machine/i8255a.h"

/*----------- defined in machine/vector06.c -----------*/

extern const i8255a_interface vector06_ppi8255_interface;
extern const i8255a_interface vector06_ppi8255_2_interface;

extern DRIVER_INIT( vector06 );
extern MACHINE_START( vector06 );
extern MACHINE_RESET( vector06 );

extern INTERRUPT_GEN( vector06_interrupt );

extern READ8_HANDLER(vector_8255_1_r);
extern WRITE8_HANDLER(vector_8255_1_w);
extern READ8_HANDLER(vector_8255_2_r);
extern WRITE8_HANDLER(vector_8255_2_w);

extern WRITE8_HANDLER(vector06_color_set);
extern WRITE8_HANDLER(vector_disc_w);

extern UINT8 vector06_keyboard_mask;
extern UINT8 vector_color_index;
extern UINT8 vector_video_mode;

/*----------- defined in video/vector06.c -----------*/

extern PALETTE_INIT( vector06 );
extern VIDEO_START( vector06 );
extern VIDEO_UPDATE( vector06 );

#endif /* VECTOR06_H_ */
