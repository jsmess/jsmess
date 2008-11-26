/***************************************************************************

  video/nes.c

  Routines to control the unique NES video hardware/PPU.

***************************************************************************/

#include <math.h>

#include "driver.h"
#include "deprecat.h"
#include "video/ppu2c0x.h"
#include "includes/nes.h"
#include "machine/nes_mmc.h"

int nes_vram_sprite[8]; /* Used only by mmc5 for now */
static int last_frame_flip = 0;

static void ppu_nmi(int num, int *ppu_regs)
{
	cpu_set_input_line(Machine->cpu[0], INPUT_LINE_NMI, PULSE_LINE);
}

static void nes_vh_reset(running_machine *machine)
{
	ppu2c0x_reset( machine, 0, 1 );
}

static void nes_vh_start(running_machine *machine, ppu_t ppu_type, double scanlines_per_frame)
{
	ppu2c0x_interface ppu_interface;

	last_frame_flip =  0;

	memset(&ppu_interface, 0, sizeof(ppu_interface));
	ppu_interface.type				= ppu_type;
	ppu_interface.num				= 1;
	ppu_interface.vrom_region[0]	= nes.chr_chunks ? "gfx1" : NULL;
	ppu_interface.mirroring[0]		= PPU_MIRROR_NONE;
	ppu_interface.nmi_handler[0]	= ppu_nmi;

	ppu2c0x_init(machine, &ppu_interface);
	ppu2c0x_set_vidaccess_callback(0, nes_ppu_vidaccess);
	ppu2c0x_set_scanlines_per_frame(0, ceil(scanlines_per_frame));

	if (nes.four_screen_vram)
	{
		ppu2c0x_set_mirroring(0, PPU_MIRROR_4SCREEN);
	}
	else
	{
		switch(nes.hard_mirroring)
		{
			case 0:
				ppu2c0x_set_mirroring(0, PPU_MIRROR_HORZ);
				break;
			case 1:
				ppu2c0x_set_mirroring(0, PPU_MIRROR_VERT);
				break;
		}
	}

	add_reset_callback(machine, nes_vh_reset);

	/* Reset the mapper variables. Will also mark the char-gen ram as dirty */
	mapper_reset(nes.mapper);
}



VIDEO_START( nes_ntsc )
{
	nes_vh_start(machine, PPU_2C02, PPU_NTSC_SCANLINES_PER_FRAME);
}

VIDEO_START( nes_pal )
{
	nes_vh_start(machine, PPU_2C07, PPU_PAL_SCANLINES_PER_FRAME);
}

PALETTE_INIT( nes )
{
	ppu2c0x_init_palette(machine, 0);
}


/***************************************************************************

  Display refresh

***************************************************************************/

VIDEO_UPDATE( nes )
{
	/* render the ppu */
	ppu2c0x_render( 0, bitmap, 0, 0, 0, 0 );

	/* if this is a disk system game, check for the flip-disk key */
	if (nes.mapper == 20)
	{
		// latch this input so it doesn't go at warp speed
		if ((input_port_read(screen->machine, "FLIPDISK") & 0x01) && (!last_frame_flip))
		{
			last_frame_flip = 1;
			nes_fds.current_side++;
			if (nes_fds.current_side > nes_fds.sides)
				nes_fds.current_side = 0;

			if (nes_fds.current_side == 0)
			{
				popmessage("No disk inserted.");
			}
			else
			{
				popmessage("Disk set to side %d", nes_fds.current_side);
			}
		}

		if (!(input_port_read(screen->machine, "FLIPDISK") & 0x01))
		{
			last_frame_flip = 0;
		}
	}
	return 0;
}
