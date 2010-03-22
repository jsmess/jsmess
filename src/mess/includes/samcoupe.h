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
#define SAM_TOTAL_WIDTH		768
#define SAM_TOTAL_HEIGHT	312
#define SAM_SCREEN_WIDTH	512
#define SAM_SCREEN_HEIGHT	192
#define SAM_BORDER_LEFT		32
#define SAM_BORDER_RIGHT	32
#define SAM_BORDER_TOP		32
#define SAM_BORDER_BOTTOM	32

/* interrupt sources */
#define SAM_LINE_INT	 0x01
#define SAM_MOUSE_INT    0x02
#define SAM_MIDIIN_INT   0x04
#define SAM_FRAME_INT	 0x08
#define SAM_MIDIOUT_INT  0x10


class coupe_asic
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, coupe_asic(machine)); }

	coupe_asic(running_machine &machine) { }

	UINT8 lmpr, hmpr, vmpr; /* memory pages */
	UINT8 lext, hext;       /* extended memory page */
	UINT8 border;           /* border */
	UINT8 clut[16];         /* color lookup table, 16 entries */
	UINT8 line_int;         /* line interrupt */
	UINT8 status;           /* status register */

	/* attribute */
	UINT8 attribute;

	/* mouse */
	int mouse_index;
	emu_timer *mouse_reset;
	UINT8 mouse_data[9];
	int mouse_x, mouse_y;
};


/*----------- defined in drivers/samcoupe.c -----------*/

void samcoupe_irq(running_device *device, UINT8 src);


/*----------- defined in machine/samcoupe.c -----------*/

void samcoupe_update_memory(const address_space *space);
UINT8 samcoupe_mouse_r(running_machine *machine);

WRITE8_HANDLER( samcoupe_ext_mem_w );
MACHINE_START( samcoupe );
MACHINE_RESET( samcoupe );


/*----------- defined in video/samcoupe.c -----------*/

VIDEO_UPDATE( samcoupe );


#endif /* SAMCOUPE_H_ */
