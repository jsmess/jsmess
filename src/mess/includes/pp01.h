/*****************************************************************************
 *
 * includes/pp01.h
 *
 ****************************************************************************/

#ifndef PP01_H_
#define PP01_H_

/*----------- defined in machine/pp01.c -----------*/

extern DRIVER_INIT( pp01 );
extern MACHINE_START( pp01 );
extern MACHINE_RESET( pp01 );

/*----------- defined in video/pp01.c -----------*/

extern VIDEO_START( pp01 );
extern VIDEO_UPDATE( pp01 );
extern PALETTE_INIT( pp01 );

#endif
