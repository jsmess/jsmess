/*****************************************************************************
 *
 * includes/kramermc.h
 *
 ****************************************************************************/

#ifndef KRAMERMC_H_
#define KRAMERMC_H_


/*----------- defined in machine/kramermc.c -----------*/

extern DRIVER_INIT( kramermc );
extern MACHINE_RESET( kramermc );

/*----------- defined in video/kramermc.c -----------*/

extern const gfx_layout kramermc_charlayout;

extern VIDEO_START( kramermc );
extern VIDEO_UPDATE( kramermc );


#endif /* KRAMERMC_h_ */
