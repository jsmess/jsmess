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

/*----------- defined in video/sapi1.c -----------*/

extern VIDEO_START( sapi1 );
extern VIDEO_UPDATE( sapi1 );
extern PALETTE_INIT( sapi1 );

#endif
