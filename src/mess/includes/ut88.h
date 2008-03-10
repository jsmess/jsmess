/*****************************************************************************
 *
 * includes/ut88.h
 *
 ****************************************************************************/

#ifndef UT88_H_
#define UT88_H_


/*----------- defined in machine/ut88.c -----------*/

extern DRIVER_INIT( ut88 );
extern MACHINE_RESET( ut88 );
extern READ8_HANDLER( ut88_keyboard_r );
extern WRITE8_HANDLER( ut88_keyboard_w );
extern WRITE8_HANDLER( ut88_sound_w );


/*----------- defined in video/ut88.c -----------*/

extern const gfx_layout ut88_charlayout;

extern VIDEO_START( ut88 );
extern VIDEO_UPDATE( ut88 );


#endif /* UT88_H_ */
