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
	galeb_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT8 *m_video_ram;
};

/*----------- defined in video/galeb.c -----------*/

extern const gfx_layout galeb_charlayout;

extern VIDEO_START( galeb );
extern SCREEN_UPDATE_IND16( galeb );


#endif /* GALEB_H_ */
