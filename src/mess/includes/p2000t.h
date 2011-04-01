/*****************************************************************************
 *
 * includes/p2000t.h
 *
 ****************************************************************************/

#ifndef P2000T_H_
#define P2000T_H_


class p2000t_state : public driver_device
{
public:
	p2000t_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_videoram;
	UINT8 m_port_101f;
	UINT8 m_port_202f;
	UINT8 m_port_303f;
	UINT8 m_port_707f;
	INT8 m_frame_count;
};


/*----------- defined in machine/p2000t.c -----------*/

extern  READ8_HANDLER( p2000t_port_000f_r );
extern  READ8_HANDLER( p2000t_port_202f_r );
extern WRITE8_HANDLER( p2000t_port_101f_w );
extern WRITE8_HANDLER( p2000t_port_303f_w );
extern WRITE8_HANDLER( p2000t_port_505f_w );
extern WRITE8_HANDLER( p2000t_port_707f_w );
extern WRITE8_HANDLER( p2000t_port_888b_w );
extern WRITE8_HANDLER( p2000t_port_8c90_w );
extern WRITE8_HANDLER( p2000t_port_9494_w );


/*----------- defined in video/p2000m.c -----------*/

extern VIDEO_START( p2000m );
extern SCREEN_UPDATE( p2000m );


#endif /* P2000T_H_ */
