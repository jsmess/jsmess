/*****************************************************************************
 *
 * includes/ondra.h
 *
 ****************************************************************************/

#ifndef ONDRA_H_
#define ONDRA_H_

class ondra_state : public driver_device
{
public:
	ondra_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_video_enable;
	UINT8 m_bank1_status;
	UINT8 m_bank2_status;
};


/*----------- defined in machine/ondra.c -----------*/

extern MACHINE_START( ondra );
extern MACHINE_RESET( ondra );

extern WRITE8_HANDLER( ondra_port_03_w );
extern WRITE8_HANDLER( ondra_port_09_w );
extern WRITE8_HANDLER( ondra_port_0a_w );
/*----------- defined in video/ondra.c -----------*/

extern VIDEO_START( ondra );
extern SCREEN_UPDATE( ondra );

#endif
