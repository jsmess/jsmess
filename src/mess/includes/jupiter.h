/*****************************************************************************
 *
 * includes/jupiter.h
 *
 ****************************************************************************/

#ifndef JUPITER_H_
#define JUPITER_H_


/*----------- defined in machine/jupiter.c -----------*/

DIRECT_UPDATE_HANDLER( jupiter_opbaseoverride );
MACHINE_START( jupiter );

DEVICE_IMAGE_LOAD( jupiter_ace );
DEVICE_IMAGE_LOAD( jupiter_tap );
DEVICE_IMAGE_UNLOAD( jupiter_tap );


/*----------- defined in video/jupiter.c -----------*/



#endif /* JUPITER_H_ */
