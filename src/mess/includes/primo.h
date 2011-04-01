/*****************************************************************************
 *
 * includes/primo.h
 *
 ****************************************************************************/

#ifndef PRIMO_H_
#define PRIMO_H_

#include "imagedev/snapquik.h"


class primo_state : public driver_device
{
public:
	primo_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16 m_video_memory_base;
	UINT8 m_port_FD;
	int m_nmi;
};


/*----------- defined in machine/primo.c -----------*/

extern READ8_HANDLER ( primo_be_1_r );
extern READ8_HANDLER ( primo_be_2_r );
extern WRITE8_HANDLER ( primo_ki_1_w );
extern WRITE8_HANDLER ( primo_ki_2_w );
extern WRITE8_HANDLER ( primo_FD_w );
extern DRIVER_INIT ( primo32 );
extern DRIVER_INIT ( primo48 );
extern DRIVER_INIT ( primo64 );
extern MACHINE_RESET( primoa );
extern MACHINE_RESET( primob );
extern INTERRUPT_GEN( primo_vblank_interrupt );
extern SNAPSHOT_LOAD( primo );
extern QUICKLOAD_LOAD( primo );


/*----------- defined in video/primo.c -----------*/

extern SCREEN_UPDATE( primo );


#endif /* PRIMO_H_ */
