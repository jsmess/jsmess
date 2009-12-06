/*****************************************************************************
 *
 * includes/sapi1.h
 *
 ****************************************************************************/

#ifndef SAPI_1_H_
#define SAPI_1_H_

/*----------- defined in machine/sapi1.c -----------*/

extern DRIVER_INIT( sapi1 );
extern MACHINE_START( sapi1 );
extern MACHINE_RESET( sapi1 );

extern READ8_HANDLER (sapi1_keyboard_r );
extern WRITE8_HANDLER(sapi1_keyboard_w );

/*----------- defined in video/sapi1.c -----------*/
extern UINT8* sapi_video_ram;

extern VIDEO_START( sapi1 );
extern VIDEO_UPDATE( sapi1 );
extern VIDEO_START( sapizps3 );
extern VIDEO_UPDATE( sapizps3 );

#endif
