/*****************************************************************************
 *
 * includes/homelab.h
 *
 ****************************************************************************/

#ifndef HOMELAB_H_
#define HOMELAB_H_


/*----------- defined in machine/homelab.c -----------*/

extern DRIVER_INIT( homelab );


/*----------- defined in video/homelab.c -----------*/

extern VIDEO_START( homelab );
extern VIDEO_UPDATE( homelab );
extern VIDEO_UPDATE( homelab3 );


#endif /* HOMELAB_H_ */
