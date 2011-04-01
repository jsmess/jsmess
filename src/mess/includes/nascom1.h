/*****************************************************************************
 *
 * includes/nascom1.h
 *
 ****************************************************************************/

#ifndef NASCOM1_H_
#define NASCOM1_H_

#include "imagedev/snapquik.h"
#include "machine/wd17xx.h"

typedef struct
{
	UINT8	stat_flags;
	UINT8	stat_count;
} nascom1_portstat_t;

typedef struct
{
	UINT8 select;
	UINT8 irq;
	UINT8 drq;
} nascom2_fdc_t;


class nascom1_state : public driver_device
{
public:
	nascom1_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_videoram;
	device_t *m_hd6402;
	int m_tape_size;
	UINT8 *m_tape_image;
	int m_tape_index;
	nascom1_portstat_t m_portstat;
	nascom2_fdc_t m_nascom2_fdc;
};


/*----------- defined in machine/nascom1.c -----------*/

extern const wd17xx_interface nascom2_wd17xx_interface;

DRIVER_INIT( nascom1 );
DEVICE_IMAGE_LOAD( nascom1_cassette );
DEVICE_IMAGE_UNLOAD( nascom1_cassette );
SNAPSHOT_LOAD( nascom1 );

READ8_HANDLER( nascom2_fdc_select_r );
WRITE8_HANDLER( nascom2_fdc_select_w );
READ8_HANDLER( nascom2_fdc_status_r );

READ8_HANDLER( nascom1_port_00_r );
READ8_HANDLER( nascom1_port_01_r );
READ8_HANDLER( nascom1_port_02_r );
WRITE8_HANDLER( nascom1_port_00_w );
WRITE8_HANDLER( nascom1_port_01_w );

READ8_DEVICE_HANDLER( nascom1_hd6402_si );
WRITE8_DEVICE_HANDLER( nascom1_hd6402_so );

MACHINE_RESET( nascom1 );
MACHINE_RESET( nascom2 );

/*----------- defined in video/nascom1.c -----------*/

SCREEN_UPDATE( nascom1 );
SCREEN_UPDATE( nascom2 );


#endif /* NASCOM1_H_ */
