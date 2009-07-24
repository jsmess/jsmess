/***************************************************************************

  video/nes.c

  Routines to control the unique NES video hardware/PPU.

***************************************************************************/

#include "driver.h"
#include "video/ppu2c0x.h"
#include "includes/nes.h"
#include "machine/nes_mmc.h"

int nes_vram_sprite[8]; /* Used only by mmc5 for now */
static int last_frame_flip = 0;
static double nes_scanlines_per_frame;

static void nes_vh_reset(running_machine *machine)
{
	ppu2c0x_set_vidaccess_callback(devtag_get_device(machine,"ppu"), nes_ppu_vidaccess);
	ppu2c0x_set_scanlines_per_frame(devtag_get_device(machine,"ppu"), ceil(nes_scanlines_per_frame));

	if (nes.four_screen_vram)
	{
		set_nt_mirroring(PPU_MIRROR_4SCREEN);
	}
	else
	{
		switch(nes.hard_mirroring)
		{
			case 0:
				set_nt_mirroring(PPU_MIRROR_HORZ);
				break;
			case 1:
				set_nt_mirroring(PPU_MIRROR_VERT);
				break;
			default:
				set_nt_mirroring(PPU_MIRROR_NONE );
				break;
		}
	}
}

static void nes_vh_start(running_machine *machine, double scanlines_per_frame)
{
	last_frame_flip =  0;
	nes_scanlines_per_frame = scanlines_per_frame;

	add_reset_callback(machine, nes_vh_reset);
}



VIDEO_START( nes_ntsc )
{
	nes_vh_start(machine, PPU_NTSC_SCANLINES_PER_FRAME);
}

VIDEO_START( nes_pal )
{
	nes_vh_start(machine, PPU_PAL_SCANLINES_PER_FRAME);
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
	nes_state *state = screen->machine->driver_data;

	/* render the ppu */
	ppu2c0x_render( state->ppu, bitmap, 0, 0, 0, 0 );

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
