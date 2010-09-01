#include "emu.h"
#include "includes/newbrain.h"

static VIDEO_START( newbrain )
{
	newbrain_state *state = machine->driver_data<newbrain_state>();

	/* find memory regions */

	state->char_rom = memory_region(machine, "chargen");

	/* register for state saving */

	state_save_register_global(machine, state->tvcnsl);
	state_save_register_global(machine, state->tvctl);
	state_save_register_global(machine, state->tvram);
	state_save_register_global_array(machine, state->segment_data);
}

static void newbrain_update(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	newbrain_state *state = machine->driver_data<newbrain_state>();

	address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	int y, sx;
	int columns = (state->tvctl & NEWBRAIN_VIDEO_80L) ? 80 : 40;
	int excess = (state->tvctl & NEWBRAIN_VIDEO_32_40) ? 24 : 4;
	int ucr = (state->tvctl & NEWBRAIN_VIDEO_UCR) ? 1 : 0;
	int fs = (state->tvctl & NEWBRAIN_VIDEO_FS) ? 1 : 0;
	int rv = (state->tvctl & NEWBRAIN_VIDEO_RV) ? 1 : 0;
	int gr = 0;

	UINT16 videoram_addr = state->tvram;
	UINT8 rc = 0;

	for (y = 0; y < 250; y++)
	{
		int x = 0;

		for (sx = 0; sx < columns; sx++)
		{
			int bit;

			UINT8 videoram_data = program->read_byte(videoram_addr + sx);
			UINT8 charrom_data;

			if (gr)
			{
				/* render video ram data */
				charrom_data = videoram_data;
			}
			else
			{
				/* render character rom data */
				UINT16 charrom_addr = (rc << 8) | ((BIT(videoram_data, 7) & fs) << 7) | (videoram_data & 0x7f);
				charrom_data = state->char_rom[charrom_addr & 0xfff];

				if ((videoram_data & 0x80) && !fs)
				{
					/* invert character */
					charrom_data ^= 0xff;
				}

				if ((videoram_data & 0x60) && !ucr)
				{
					/* strip bit D0 */
					charrom_data &= 0xfe;
				}
			}

			for (bit = 0; bit < 8; bit++)
			{
				int color = BIT(charrom_data, 7) ^ rv;

				*BITMAP_ADDR16(bitmap, y, x++) = color;

				if (columns == 40)
				{
					*BITMAP_ADDR16(bitmap, y, x++) = color;
				}

				charrom_data <<= 1;
			}
		}

		if (gr)
		{
			/* get new data for each line */
			videoram_addr += columns;
			videoram_addr += excess;
		}
		else
		{
			/* increase row counter */
			rc++;

			if (rc == (ucr ? 8 : 10))
			{
				/* reset row counter */
				rc = 0;

				videoram_addr += columns;
				videoram_addr += excess;
			}
		}
	}
}

static VIDEO_UPDATE( newbrain )
{
	newbrain_state *state = screen->machine->driver_data<newbrain_state>();

	if (state->enrg1 & NEWBRAIN_ENRG1_TVP)
	{
		newbrain_update(screen->machine, bitmap, cliprect);
	}
	else
	{
		bitmap_fill(bitmap, cliprect, 0);
	}

	return 0;
}

/* Machine Drivers */

MACHINE_CONFIG_FRAGMENT( newbrain_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_SIZE(640, 250)
	MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 249)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(newbrain)
	MDRV_VIDEO_UPDATE(newbrain)
MACHINE_CONFIG_END
