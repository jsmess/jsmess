/*****************************************************************************
 *
 * includes/mikro80.h
 *
 ****************************************************************************/

#ifndef MIKRO80_H_
#define MIKRO80_H_


/*----------- defined in machine/mikro80.c -----------*/

extern DRIVER_INIT( mikro80 );
extern MACHINE_RESET( mikro80 );
extern READ8_HANDLER( mikro80_keyboard_r );
extern WRITE8_HANDLER( mikro80_keyboard_w );

/*----------- defined in video/mikro80.c -----------*/

extern const gfx_layout mikro80_charlayout;

extern VIDEO_START( mikro80 );
extern VIDEO_UPDATE( mikro80 );

#endif /* UT88_H_ */
