/*****************************************************************************
 *
 * includes/coupe.h
 *
 * SAM Coupe
 *
 * Driver by Lee Hammerton
 *
 ****************************************************************************/

#ifndef SAMCOUPE_H_
#define SAMCOUPE_H_

/* screen dimensions */
#define SAM_BLOCK			8

#define SAM_TOTAL_WIDTH		SAM_BLOCK*96
#define SAM_TOTAL_HEIGHT	312
#define SAM_SCREEN_WIDTH	SAM_BLOCK*64
#define SAM_SCREEN_HEIGHT	192
#define SAM_BORDER_LEFT		SAM_BLOCK*4
#define SAM_BORDER_RIGHT	SAM_BLOCK*4
#define SAM_BORDER_TOP		37
#define SAM_BORDER_BOTTOM	46

/* interrupt sources */
#define SAM_LINE_INT	 0x01
#define SAM_MOUSE_INT    0x02
#define SAM_MIDIIN_INT   0x04
#define SAM_FRAME_INT	 0x08
#define SAM_MIDIOUT_INT  0x10


class samcoupe_state :  public driver_device
{
public:
	samcoupe_state(running_machine &machine, const driver_device_config_base &config)
			: driver_device(machine, config) { }

	emu_timer *m_video_update_timer;

	UINT8 m_lmpr, m_hmpr, m_vmpr; /* memory pages */
	UINT8 m_lext, m_hext;       /* extended memory page */
	UINT8 m_border;           /* border */
	UINT8 m_clut[16];         /* color lookup table, 16 entries */
	UINT8 m_line_int;         /* line interrupt */
	UINT8 m_status;           /* status register */

	/* attribute */
	UINT8 m_attribute;

	/* mouse */
	int m_mouse_index;
	emu_timer *m_mouse_reset;
	UINT8 m_mouse_data[9];
	int m_mouse_x, m_mouse_y;
	UINT8 *m_videoram;
};


/*----------- defined in drivers/samcoupe.c -----------*/

void samcoupe_irq(device_t *device, UINT8 src);


/*----------- defined in machine/samcoupe.c -----------*/

void samcoupe_update_memory(address_space *space);
UINT8 samcoupe_mouse_r(running_machine &machine);

WRITE8_HANDLER( samcoupe_ext_mem_w );
MACHINE_START( samcoupe );
MACHINE_RESET( samcoupe );


/*----------- defined in video/samcoupe.c -----------*/

TIMER_CALLBACK( sam_video_update_callback );


#endif /* SAMCOUPE_H_ */
