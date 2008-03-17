/*****************************************************************************
 *
 * includes/special.h
 *
 ****************************************************************************/

#ifndef SPECIAL_H_
#define SPECIAL_H_


/*----------- defined in machine/special.c -----------*/

extern DRIVER_INIT( special );
extern MACHINE_RESET( special );

extern READ8_HANDLER( specialist_keyboard_r );
extern WRITE8_HANDLER( specialist_keyboard_w );


/*----------- defined in video/special.c -----------*/

extern VIDEO_START( special );
extern VIDEO_UPDATE( special );


#endif /* SPECIAL_H_ */
