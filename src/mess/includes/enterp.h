/*****************************************************************************
 *
 * includes/enterp.h
 *
 ****************************************************************************/

#ifndef ENTERP_H_
#define ENTERP_H_


/*----------- defined in machine/enterp.c -----------*/

DEVICE_LOAD( enterprise_floppy );


/*----------- defined in video/enterp.c -----------*/

VIDEO_START( enterprise );
VIDEO_UPDATE( enterprise );


#endif /* ENTERP_H_ */
