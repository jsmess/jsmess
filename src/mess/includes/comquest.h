/*****************************************************************************
 *
 * includes/comquest.h
 *
 ****************************************************************************/

#ifndef COMQUEST_H_
#define COMQUEST_H_


class comquest_state : public driver_device
{
public:
	comquest_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_data[128][8];
	void *m_timer;
	int m_line;
	int m_dma_activ;
	int m_state;
	int m_count;
};


/*----------- defined in video/comquest.c -----------*/

VIDEO_START( comquest );
SCREEN_UPDATE( comquest );


#endif /* COMQUEST_H_ */
