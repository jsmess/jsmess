/*****************************************************************************
 *
 * includes/homelab.h
 *
 ****************************************************************************/

#ifndef HOMELAB_H_
#define HOMELAB_H_


class homelab_state : public driver_device
{
public:
	homelab_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


/*----------- defined in machine/homelab.c -----------*/

extern DRIVER_INIT( homelab );


/*----------- defined in video/homelab.c -----------*/

extern VIDEO_START( homelab );
extern VIDEO_UPDATE( homelab );
extern VIDEO_UPDATE( homelab3 );


#endif /* HOMELAB_H_ */
