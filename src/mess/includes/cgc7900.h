#pragma once

#ifndef __CGC7900__
#define __CGC7900__

#define M68000_TAG		"uh8"
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

	/* video state */
	UINT16 *z_ram;
	UINT16 *plane_ram;
	UINT16 *clut_ram;
	UINT16 *overlay_ram;
	UINT8 *char_rom;
};

/*----------- defined in video/cgc7900.c -----------*/

MACHINE_DRIVER_EXTERN(cgc7900_video);

#endif
