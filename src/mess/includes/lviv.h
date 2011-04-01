/*****************************************************************************
 *
 * includes/lviv.h
 *
 ****************************************************************************/

#ifndef LVIV_H_
#define LVIV_H_

#include "imagedev/snapquik.h"
#include "machine/i8255a.h"

class lviv_state : public driver_device
{
public:
	lviv_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	unsigned char * m_video_ram;
	unsigned short m_colortable[1][4];
	UINT8 m_ppi_port_outputs[2][3];
	UINT8 m_startup_mem_map;
};


/*----------- defined in machine/lviv.c -----------*/

extern const i8255a_interface lviv_ppi8255_interface_0;
extern const i8255a_interface lviv_ppi8255_interface_1;

 READ8_HANDLER ( lviv_io_r );
WRITE8_HANDLER ( lviv_io_w );
MACHINE_RESET( lviv );
SNAPSHOT_LOAD( lviv );


/*----------- defined in video/lviv.c -----------*/

extern VIDEO_START( lviv );
extern SCREEN_UPDATE( lviv );
extern const unsigned char lviv_palette[8*3];
extern PALETTE_INIT( lviv );
extern void lviv_update_palette(running_machine &, UINT8);


#endif /* LVIV_H_ */
