/*****************************************************************************
 *
 * includes/galeb.h
 *
 ****************************************************************************/

#ifndef GALEB_H_
#define GALEB_H_


class galeb_state : public driver_device
{
public:
	galeb_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_video_ram;
};

/*----------- defined in video/galeb.c -----------*/

extern const gfx_layout galeb_charlayout;

extern VIDEO_START( galeb );
extern SCREEN_UPDATE( galeb );


#endif /* GALEB_H_ */
