/*****************************************************************************
 *
 * includes/oric.h
 *
 ****************************************************************************/

#ifndef ORIC_H_
#define ORIC_H_

#include "machine/6522via.h"
#include "machine/wd17xx.h"

enum
{
	TELESTRAT_MEM_BLOCK_UNDEFINED,
	TELESTRAT_MEM_BLOCK_RAM,
	TELESTRAT_MEM_BLOCK_ROM
};

typedef struct
{
	int		MemType;
	unsigned char *ptr;
} telestrat_mem_block;

/* current state of the display */
/* some attributes persist until they are turned off.
This structure holds this persistant information */
typedef struct
{
	/* foreground and background colour used for rendering */
	/* if flash attribute is set, these two will both be equal
    to background colour */
	int active_foreground_colour;
	int active_background_colour;
	/* current foreground and background colour */
	int foreground_colour;
	int background_colour;
	int mode;
	/* text attributes */
	int text_attributes;

	unsigned long read_addr;

	/* current addr to fetch data */
	unsigned char *char_data;
	/* base of char data */
	unsigned char *char_base;

	/* if (1<<3), display graphics, if 0, hide graphics */
	int flash_state;
	/* current count */
	int flash_count;
} oric_vh_state;


class oric_state : public driver_device
{
public:
	oric_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_ram;
	int m_is_telestrat;
	unsigned char m_irqs;
	char *m_ram_0x0c000;
	int m_keyboard_line;
	char m_key_sense_bit;
	char m_keyboard_mask;
	unsigned char m_via_port_a_data;
	char m_psg_control;
	unsigned char m_previous_portb_data;
	unsigned char m_port_3fa_w;
	unsigned char m_port_3fb_w;
	unsigned char m_wd179x_int_state;
	unsigned char m_port_314_r;
	unsigned char m_port_318_r;
	unsigned char m_port_314_w;
	unsigned char m_telestrat_bank_selection;
	unsigned char m_telestrat_via2_port_a_data;
	unsigned char m_telestrat_via2_port_b_data;
	telestrat_mem_block m_telestrat_blocks[8];
	oric_vh_state m_vh_state;
};


/*----------- defined in machine/oric.c -----------*/

extern const via6522_interface oric_6522_interface;
extern const via6522_interface telestrat_via2_interface;
extern const wd17xx_interface oric_wd17xx_interface;

MACHINE_START( oric );
MACHINE_RESET( oric );
READ8_HANDLER( oric_IO_r );
WRITE8_HANDLER( oric_IO_w );
READ8_HANDLER( oric_microdisc_r );
WRITE8_HANDLER( oric_microdisc_w );

WRITE8_HANDLER(oric_psg_porta_write);

/* Telestrat specific */
MACHINE_START( telestrat );


/*----------- defined in video/oric.c -----------*/

VIDEO_START( oric );
SCREEN_UPDATE( oric );


#endif /* ORIC_H_ */
