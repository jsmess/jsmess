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
	homelab_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

};

/*----------- defined in video/homelab.c -----------*/

extern VIDEO_START( homelab );
extern SCREEN_UPDATE( homelab );
extern SCREEN_UPDATE( homelab3 );


#endif /* HOMELAB_H_ */
