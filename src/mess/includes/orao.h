/*****************************************************************************
 *
 * includes/orao.h
 *
 ****************************************************************************/

#ifndef ORAO_H_
#define ORAO_H_


class orao_state : public driver_device
{
public:
	orao_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_memory(*this, "memory"),
		m_video_ram(*this, "video_ram"){ }

	required_shared_ptr<UINT8> m_memory;
	required_shared_ptr<UINT8> m_video_ram;
};


/*----------- defined in machine/orao.c -----------*/

extern DRIVER_INIT( orao );
extern MACHINE_RESET( orao );

extern DRIVER_INIT( orao103 );

extern READ8_HANDLER( orao_io_r );
extern WRITE8_HANDLER( orao_io_w );


/*----------- defined in video/orao.c -----------*/

extern VIDEO_START( orao );
extern SCREEN_UPDATE_IND16( orao );


#endif /* ORAO_H_ */
