/***************************************************************************

  galaxy.c

  Functions to emulate the video hardware of the Galaksija.

  20/05/2008 - Real video implementation by Miodrag Milanovic
  01/03/2008 - Update by Miodrag Milanovic to make Galaksija video work with new SVN code
***************************************************************************/

#include "emu.h"
#include "includes/galaxy.h"
#include "cpu/z80/z80.h"


static TIMER_CALLBACK( gal_video )
{
	galaxy_state *state = machine->driver_data<galaxy_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int y, x;
	if (state->interrupts_enabled == TRUE)
	{
		UINT8 *gfx = memory_region(machine, "gfx1");
		UINT8 dat = (state->latch_value & 0x3c) >> 2;
		if ((state->gal_cnt >= 48 * 2) && (state->gal_cnt < 48 * 210))  // display on screen just state->first 208 lines
		{
			UINT8 mode = (state->latch_value >> 1) & 1; // bit 2 latch represents mode
			UINT16 addr = (cpu_get_reg(machine->device("maincpu"), Z80_I) << 8) | cpu_get_reg(machine->device("maincpu"), Z80_R) | ((state->latch_value & 0x80) ^ 0x80);
			if (mode == 0)
			{
				// Text mode
				if (state->first == 0 && (cpu_get_reg(machine->device("maincpu"), Z80_R) & 0x1f) == 0)
				{
					// Due to a fact that on real processor latch value is set at
					// the end of last cycle we need to skip dusplay of double
					// state->first char in each row
					state->code = 0x00;
					state->first = 1;
				}
				else
				{
					state->code = space->read_byte(addr) & 0xbf;
					state->code += (state->code & 0x80) >> 1;
					state->code = gfx[(state->code & 0x7f) +(dat << 7 )] ^ 0xff;
					state->first = 0;
				}
				y = state->gal_cnt / 48 - 2;
				x = (state->gal_cnt % 48) * 8;

				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 0) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 1) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 2) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 3) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 4) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 5) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 6) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 7) & 1;
			}
			else
			{ // Graphics mode
				if (state->first < 4 && (cpu_get_reg(machine->device("maincpu"), Z80_R) & 0x1f) == 0)
				{
					// Due to a fact that on real processor latch value is set at
					// the end of last cycle we need to skip dusplay of 4 times
					// state->first char in each row
					state->code = 0x00;
					state->first++;
				}
				else
				{
					state->code = space->read_byte(addr) ^ 0xff;
					state->first = 0;
				}
				y = state->gal_cnt / 48 - 2;
				x = (state->gal_cnt % 48) * 8;

				/* hack - until calc of R is fixed in Z80 */
				if (x == 11 * 8 && y == 0)
				{
					state->start_addr = addr;
				}
				if ((x / 8 >= 11) && (x / 8 < 44))
				{
					state->code = space->read_byte(state->start_addr + y * 32 + (state->gal_cnt % 48) - 11) ^ 0xff;
				}
				else
				{
					state->code = 0x00;
				}
				/* end of hack */

				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 0) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 1) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 2) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 3) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 4) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 5) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 6) & 1; x++;
				*BITMAP_ADDR16(machine->generic.tmpbitmap, y, x ) = (state->code >> 7) & 1;
			}
		}
		state->gal_cnt++;
	}
}

void galaxy_set_timer(running_machine *machine)
{
	galaxy_state *state = machine->driver_data<galaxy_state>();
	state->gal_cnt = 0;
	timer_adjust_periodic(state->gal_video_timer, attotime_zero, 0, ATTOTIME_IN_HZ(6144000 / 8));
}

VIDEO_START( galaxy )
{
	galaxy_state *state = machine->driver_data<galaxy_state>();
	state->gal_cnt = 0;

	state->gal_video_timer = timer_alloc(machine, gal_video, NULL);
	timer_adjust_periodic(state->gal_video_timer, attotime_zero, 0, attotime_never);

	VIDEO_START_CALL( generic_bitmapped );
}

VIDEO_UPDATE( galaxy )
{
	galaxy_state *state = screen->machine->driver_data<galaxy_state>();
	timer_adjust_periodic(state->gal_video_timer, attotime_zero, 0, attotime_never);
	if (state->interrupts_enabled == FALSE)
	{
		rectangle black_area = {0, 384 - 1, 0, 208 - 1};
		bitmap_fill(screen->machine->generic.tmpbitmap, &black_area, 0);
	}
	state->interrupts_enabled = FALSE;
	return VIDEO_UPDATE_CALL ( generic_bitmapped );
}

