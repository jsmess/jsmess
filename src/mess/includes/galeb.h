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


/*----------- defined in video/galeb.c -----------*/

extern const gfx_layout galeb_charlayout;
extern UINT8 *galeb_video_ram;

extern VIDEO_START( galeb );
extern VIDEO_UPDATE( galeb );


#endif /* GALEB_H_ */
