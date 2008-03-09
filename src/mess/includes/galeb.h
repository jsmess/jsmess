/*****************************************************************************
 *
 * includes/galeb.h
 *
 ****************************************************************************/

#ifndef GALEB_H_
#define GALEB_H_


/*----------- defined in machine/galeb.c -----------*/

extern DRIVER_INIT( galeb );
extern MACHINE_RESET( galeb );
extern READ8_HANDLER( galeb_keyboard_r );
extern WRITE8_HANDLER( galeb_speaker_w );


/*----------- defined in video/galeb.c -----------*/

extern const gfx_layout galeb_charlayout;

extern PALETTE_INIT( galeb );
extern VIDEO_START( galeb );
extern VIDEO_UPDATE( galeb );


#endif /* GALEB_H_ */
