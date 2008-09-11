/*****************************************************************************
 *
 * includes/pk8020.h
 *
 ****************************************************************************/

#ifndef PK8020_H_
#define PK8020_H_


/*----------- defined in machine/pk8020.c -----------*/

extern DRIVER_INIT( pk8020 );
extern MACHINE_RESET( pk8020 );

/*----------- defined in video/pk8020.c -----------*/

extern PALETTE_INIT( pk8020 );
extern VIDEO_START( pk8020 );
extern VIDEO_UPDATE( pk8020 );

#endif /* pk8020_H_ */
