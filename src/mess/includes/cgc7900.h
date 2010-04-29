#pragma once

#ifndef __CGC7900__
#define __CGC7900__

#define M68000_TAG		"uh8"
#define I8035_TAG		"i8035"
#define INS8251_0_TAG	"uc10"
#define INS8251_1_TAG	"uc8"
#define MM58167_TAG		"uc6"
#define AY8910_TAG		"uc4"
#define SCREEN_TAG		"screen"

class cgc7900_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, cgc7900_state(machine)); }

	cgc7900_state(running_machine &machine) { }

	/* interrupt state */
	UINT16 int_mask;

	/* video state */
	UINT16 *plane_ram;
	UINT16 *clut_ram;
	UINT16 *overlay_ram;
	UINT8 *char_rom;
	UINT16 roll_bitmap;
	UINT16 pan_x;
	UINT16 pan_y;
	UINT16 zoom;
	UINT16 blink_select;
	UINT16 plane_select;
	UINT16 plane_switch;
	UINT16 color_status_fg;
	UINT16 color_status_bg;
	UINT16 roll_overlay;
};

/*----------- defined in video/cgc7900.c -----------*/

READ16_HANDLER( cgc7900_z_mode_r );
WRITE16_HANDLER( cgc7900_z_mode_w );
WRITE16_HANDLER( cgc7900_color_status_w );
READ16_HANDLER( cgc7900_sync_r );

MACHINE_DRIVER_EXTERN( cgc7900_video );

#endif
