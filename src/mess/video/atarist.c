/*

    TODO:

    - double screen width to allow for medium resolution
    - STe pixelofs
    - blitter hog
    - high resolution

*/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/68901mfp.h"
#include "video/atarist.h"
#include "includes/atarist.h"

INLINE pen_t atarist_shift_mode_0(running_machine *machine)
{
	atarist_state *state = (atarist_state *)machine->driver_data;

	int color = (BIT(state->shifter_rr[3], 15) << 3) | (BIT(state->shifter_rr[2], 15) << 2) | (BIT(state->shifter_rr[1], 15) << 1) | BIT(state->shifter_rr[0], 15);

	state->shifter_rr[0] <<= 1;
	state->shifter_rr[1] <<= 1;
	state->shifter_rr[2] <<= 1;
	state->shifter_rr[3] <<= 1;

	return machine->pens[color];
}

INLINE pen_t atarist_shift_mode_1(running_machine *machine)
{
	atarist_state *state = (atarist_state *)machine->driver_data;

	int color = (BIT(state->shifter_rr[1], 15) << 1) | BIT(state->shifter_rr[0], 15);

	state->shifter_rr[0] <<= 1;
	state->shifter_rr[1] <<= 1;
	state->shifter_shift++;

	if (state->shifter_shift == 16)
	{
		state->shifter_rr[0] = state->shifter_rr[2];
		state->shifter_rr[1] = state->shifter_rr[3];
		state->shifter_rr[2] = state->shifter_rr[3] = 0;
		state->shifter_shift = 0;
	}

	return machine->pens[color];
}

INLINE pen_t atarist_shift_mode_2(running_machine *machine)
{
	atarist_state *state = (atarist_state *)machine->driver_data;

	int color = BIT(state->shifter_rr[0], 15);

	state->shifter_rr[0] <<= 1;
	state->shifter_shift++;

	switch (state->shifter_shift)
	{
	case 16:
		state->shifter_rr[0] = state->shifter_rr[1];
		state->shifter_rr[1] = state->shifter_rr[2];
		state->shifter_rr[2] = state->shifter_rr[3];
		state->shifter_rr[3] = 0;
		break;

	case 32:
		state->shifter_rr[0] = state->shifter_rr[1];
		state->shifter_rr[1] = state->shifter_rr[2];
		state->shifter_rr[2] = 0;
		break;

	case 48:
		state->shifter_rr[0] = state->shifter_rr[1];
		state->shifter_rr[1] = 0;
		state->shifter_shift = 0;
		break;
	}

	return machine->pens[color];
}

static TIMER_CALLBACK( atarist_shifter_tick )
{
	atarist_state *state = (atarist_state *)machine->driver_data;

	int y = machine->primary_screen->vpos();
	int x = machine->primary_screen->hpos();

	pen_t pen;

	switch (state->shifter_mode)
	{
	case 0:
		pen = atarist_shift_mode_0(machine);
		break;

	case 1:
		pen = atarist_shift_mode_1(machine);
		break;

	case 2:
		pen = atarist_shift_mode_2(machine);
		break;

	default:
		pen = get_black_pen(machine);
		break;
	}

	*BITMAP_ADDR32(machine->generic.tmpbitmap, y, x) = pen;
}

INLINE void atarist_shifter_load(running_machine *machine)
{
	atarist_state *state = (atarist_state *)machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, M68000_TAG, ADDRESS_SPACE_PROGRAM);
	UINT16 data = memory_read_word_16be(program, state->shifter_ofs);

	state->shifter_ir[state->shifter_bitplane] = data;
	state->shifter_bitplane++;
	state->shifter_ofs += 2;

	if (state->shifter_bitplane == 4)
	{
		state->shifter_bitplane = 0;

		state->shifter_rr[0] = state->shifter_ir[0];
		state->shifter_rr[1] = state->shifter_ir[1];
		state->shifter_rr[2] = state->shifter_ir[2];
		state->shifter_rr[3] = state->shifter_ir[3];
	}
}

static TIMER_CALLBACK( atarist_glue_tick )
{
	atarist_state *state = (atarist_state *)machine->driver_data;

	int y = machine->primary_screen->vpos();
	int x = machine->primary_screen->hpos();

	int v = (y >= state->shifter_y_start) && (y < state->shifter_y_end);
	int h = (x >= state->shifter_x_start) && (x < state->shifter_x_end);

	int de = h && v;

	if (de != state->shifter_de)
	{
		mc68901_tbi_w(state->mc68901, de);
		state->shifter_de = de;
	}

	if (de)
	{
		atarist_shifter_load(machine);
	}

	if ((y == state->shifter_vblank_start) && (x == 0))
	{
		cputag_set_input_line(machine, M68000_TAG, M68K_IRQ_4, HOLD_LINE);
		state->shifter_ofs = state->shifter_base;
	}

	if (x == state->shifter_hblank_start)
	{
		cputag_set_input_line(machine, M68000_TAG, M68K_IRQ_2, HOLD_LINE);
		state->shifter_ofs += (state->shifter_lineofs * 2); // STe
	}
}

static void atarist_set_screen_parameters(running_machine *machine)
{
	atarist_state *state = (atarist_state *)machine->driver_data;

	if (state->shifter_sync & 0x02)
	{
		state->shifter_x_start = ATARIST_HBDEND_PAL;
		state->shifter_x_end = ATARIST_HBDSTART_PAL;
		state->shifter_y_start = ATARIST_VBDEND_PAL;
		state->shifter_y_end = ATARIST_VBDSTART_PAL;
		state->shifter_hblank_start = ATARIST_HBSTART_PAL;
		state->shifter_vblank_start = ATARIST_VBSTART_PAL;
	}
	else
	{
		state->shifter_x_start = ATARIST_HBDEND_NTSC;
		state->shifter_x_end = ATARIST_HBDSTART_NTSC;
		state->shifter_y_start = ATARIST_VBDEND_NTSC;
		state->shifter_y_end = ATARIST_VBDSTART_NTSC;
		state->shifter_hblank_start = ATARIST_HBSTART_NTSC;
		state->shifter_vblank_start = ATARIST_VBSTART_NTSC;
	}
}

/* Atari ST Shifter */

READ16_HANDLER( atarist_shifter_base_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0x00:
		return (state->shifter_base >> 16) & 0x3f;
	case 0x01:
		return (state->shifter_base >> 8) & 0xff;
	}

	return 0;
}

WRITE16_HANDLER( atarist_shifter_base_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0x00:
		state->shifter_base = (state->shifter_base & 0x00ff00) | (data & 0x3f) << 16;
		logerror("SHIFTER Video Base Address %06x\n", state->shifter_base);
		break;
	case 0x01:
		state->shifter_base = (state->shifter_base & 0x3f0000) | (data & 0xff) << 8;
		logerror("SHIFTER Video Base Address %06x\n", state->shifter_base);
		break;
	}
}

READ16_HANDLER( atarist_shifter_counter_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0x00:
		return (state->shifter_ofs >> 16) & 0x3f;
	case 0x01:
		return (state->shifter_ofs >> 8) & 0xff;
	case 0x02:
		return state->shifter_ofs & 0xff;
	}

	return 0;
}

READ8_HANDLER( atarist_shifter_sync_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->shifter_sync;
}

WRITE8_HANDLER( atarist_shifter_sync_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->shifter_sync = data;
	logerror("SHIFTER Sync %x\n", state->shifter_sync);
	atarist_set_screen_parameters(space->machine);
}

READ8_HANDLER( atarist_shifter_mode_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->shifter_mode;
}

WRITE8_HANDLER( atarist_shifter_mode_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->shifter_mode = data;
	logerror("SHIFTER Mode %x\n", state->shifter_mode);
}

READ16_HANDLER( atarist_shifter_palette_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->shifter_palette[offset];
}

WRITE16_HANDLER( atarist_shifter_palette_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->shifter_palette[offset] = data;
	logerror("SHIFTER Palette[%x] = %x\n", offset, data);

	palette_set_color_rgb(space->machine, offset, pal3bit(data >> 8), pal3bit(data >> 4), pal3bit(data));
}

/* Atari STe Shifter */

READ16_HANDLER( atariste_shifter_base_low_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->shifter_base & 0xfe;
}

WRITE16_HANDLER( atariste_shifter_base_low_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->shifter_base = (state->shifter_base & 0x3fff00) | (data & 0xfe);
	logerror("SHIFTER Video Base Address %06x\n", state->shifter_base);
}

READ16_HANDLER( atariste_shifter_counter_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0x00:
		return (state->shifter_ofs >> 16) & 0x3f;
	case 0x01:
		return (state->shifter_ofs >> 8) & 0xff;
	case 0x02:
		return state->shifter_ofs & 0xfe;
	}

	return 0;
}

WRITE16_HANDLER( atariste_shifter_counter_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0x00:
		state->shifter_ofs = (state->shifter_ofs & 0x00fffe) | (data & 0x3f) << 16;
		logerror("SHIFTER Video Address Counter %06x\n", state->shifter_ofs);
		break;
	case 0x01:
		state->shifter_ofs = (state->shifter_ofs & 0x3f00fe) | (data & 0xff) << 8;
		logerror("SHIFTER Video Address Counter %06x\n", state->shifter_ofs);
		break;
	case 0x02:
		state->shifter_ofs = (state->shifter_ofs & 0x3fff00) | (data & 0xfe);
		logerror("SHIFTER Video Address Counter %06x\n", state->shifter_ofs);
		break;
	}
}

WRITE16_HANDLER( atariste_shifter_palette_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	int r = ((data >> 7) & 0x0e) | BIT(data, 11);
	int g = ((data >> 3) & 0x0e) | BIT(data, 7);
	int b = ((data << 1) & 0x0e) | BIT(data, 3);

	state->shifter_palette[offset] = data;
	logerror("SHIFTER palette %x = %x\n", offset, data);

	palette_set_color_rgb(space->machine, offset, r, g, b);
}

READ16_HANDLER( atariste_shifter_lineofs_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->shifter_lineofs;
}

WRITE16_HANDLER( atariste_shifter_lineofs_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->shifter_lineofs = data & 0xff;
	logerror("SHIFTER Line Offset %x\n", state->shifter_lineofs);
}

READ16_HANDLER( atariste_shifter_pixelofs_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->shifter_pixelofs;
}

WRITE16_HANDLER( atariste_shifter_pixelofs_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->shifter_pixelofs = data & 0x0f;
	logerror("SHIFTER Pixel Offset %x\n", state->shifter_pixelofs);
}

/* Atari ST Blitter */

static const int BLITTER_NOPS[16][4] =
{
	{ 1, 1, 1, 1 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 3, 3 },
	{ 1, 1, 2, 2 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 2, 2 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 2, 2 },
	{ 2, 2, 3, 3 },
	{ 1, 1, 2, 2 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 3, 3 },
	{ 1, 1, 1, 1 }
};

static void atarist_blitter_source(running_machine *machine)
{
	atarist_state *state = (atarist_state *)machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, M68000_TAG, ADDRESS_SPACE_PROGRAM);
	UINT16 data = memory_read_word_16be(program, state->blitter_src);

	if (state->blitter_src_inc_x < 0)
	{
		state->blitter_srcbuf = (data << 16) | (state->blitter_srcbuf >> 16);
	}
	else
	{
		state->blitter_srcbuf = (state->blitter_srcbuf << 16) | data;
	}
}

static UINT16 atarist_blitter_hop(running_machine *machine)
{
	atarist_state *state = (atarist_state *)machine->driver_data;

	UINT16 source = state->blitter_srcbuf >> (state->blitter_skew & 0x0f);
	UINT16 halftone = state->blitter_halftone[state->blitter_ctrl & 0x0f];

	if (state->blitter_ctrl & ATARIST_BLITTER_CTRL_SMUDGE)
	{
		halftone = state->blitter_halftone[source & 0x0f];
	}

	switch (state->blitter_hop)
	{
	case 0:
		return 0xffff;
	case 1:
		return halftone;
	case 2:
		return source;
	case 3:
		return source & halftone;
	}

	return 0;
}

static void atarist_blitter_op(running_machine *machine, UINT16 s, UINT32 dstaddr, UINT16 mask)
{
	atarist_state *state = (atarist_state *)machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, M68000_TAG, ADDRESS_SPACE_PROGRAM);

	UINT16 d = memory_read_word_16be(program, dstaddr);
	UINT16 result = 0;

	if (state->blitter_op & 0x08) result = (~s & ~d);
	if (state->blitter_op & 0x04) result |= (~s & d);
	if (state->blitter_op & 0x02) result |= (s & ~d);
	if (state->blitter_op & 0x01) result |= (s & d);

	memory_write_word_16be(program, dstaddr, result);
}

static TIMER_CALLBACK( atarist_blitter_tick )
{
	atarist_state *state = (atarist_state *)machine->driver_data;

	do
	{
		if (state->blitter_skew & ATARIST_BLITTER_SKEW_FXSR)
		{
			atarist_blitter_source(machine);
			state->blitter_src += state->blitter_src_inc_x;
		}

		atarist_blitter_source(machine);
		atarist_blitter_op(machine, atarist_blitter_hop(machine), state->blitter_dst, state->blitter_endmask1);
		state->blitter_xcount--;

		while (state->blitter_xcount > 0)
		{
			state->blitter_src += state->blitter_src_inc_x;
			state->blitter_dst += state->blitter_dst_inc_x;

			if (state->blitter_xcount == 1)
			{
				if (!(state->blitter_skew & ATARIST_BLITTER_SKEW_NFSR))
				{
					atarist_blitter_source(machine);
				}

				atarist_blitter_op(machine, atarist_blitter_hop(machine), state->blitter_dst, state->blitter_endmask3);
			}
			else
			{
				atarist_blitter_source(machine);
				atarist_blitter_op(machine, atarist_blitter_hop(machine), state->blitter_dst, state->blitter_endmask2);
			}

			state->blitter_xcount--;
		}

		state->blitter_src += state->blitter_src_inc_y;
		state->blitter_dst += state->blitter_dst_inc_y;

		if (state->blitter_dst_inc_y < 0)
		{
			state->blitter_ctrl = (state->blitter_ctrl & 0xf0) | (((state->blitter_ctrl & 0x0f) - 1) & 0x0f);
		}
		else
		{
			state->blitter_ctrl = (state->blitter_ctrl & 0xf0) | (((state->blitter_ctrl & 0x0f) + 1) & 0x0f);
		}

		state->blitter_xcount = state->blitter_xcountl;
		state->blitter_ycount--;
	}
	while (state->blitter_ycount > 0);

	state->blitter_ctrl &= 0x7f;
}

READ16_HANDLER( atarist_blitter_halftone_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->blitter_halftone[offset];
}

READ16_HANDLER( atarist_blitter_src_inc_x_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->blitter_src_inc_x;
}

READ16_HANDLER( atarist_blitter_src_inc_y_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->blitter_src_inc_y;
}

READ16_HANDLER( atarist_blitter_src_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0:
		return (state->blitter_src >> 16) & 0xff;
	case 1:
		return state->blitter_src & 0xfffe;
	}

	return 0;
}

READ16_HANDLER( atarist_blitter_end_mask_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0:
		return state->blitter_endmask1;
	case 1:
		return state->blitter_endmask2;
	case 2:
		return state->blitter_endmask3;
	}

	return 0;
}

READ16_HANDLER( atarist_blitter_dst_inc_x_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->blitter_dst_inc_x;
}

READ16_HANDLER( atarist_blitter_dst_inc_y_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->blitter_dst_inc_y;
}

READ16_HANDLER( atarist_blitter_dst_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0:
		return (state->blitter_dst >> 16) & 0xff;
	case 1:
		return state->blitter_dst & 0xfffe;
	}

	return 0;
}

READ16_HANDLER( atarist_blitter_count_x_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->blitter_xcount;
}

READ16_HANDLER( atarist_blitter_count_y_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	return state->blitter_ycount;
}

READ16_HANDLER( atarist_blitter_op_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	if (ACCESSING_BITS_0_7)
	{
		return state->blitter_hop;
	}
	else
	{
		return state->blitter_op;
	}
}

READ16_HANDLER( atarist_blitter_ctrl_r )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	if (ACCESSING_BITS_0_7)
	{
		return state->blitter_ctrl;
	}
	else
	{
		return state->blitter_skew;
	}
}

WRITE16_HANDLER( atarist_blitter_halftone_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->blitter_halftone[offset] = data;
}

WRITE16_HANDLER( atarist_blitter_src_inc_x_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->blitter_src_inc_x = data & 0xfffe;
}

WRITE16_HANDLER( atarist_blitter_src_inc_y_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->blitter_src_inc_y = data & 0xfffe;
}

WRITE16_HANDLER( atarist_blitter_src_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0:
		state->blitter_src = (data & 0xff) | (state->blitter_src & 0xfffe);
	case 1:
		state->blitter_src = (state->blitter_src & 0xff0000) | (data & 0xfffe);
	}
}

WRITE16_HANDLER( atarist_blitter_end_mask_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0:
		state->blitter_endmask1 = data;
	case 1:
		state->blitter_endmask2 = data;
	case 2:
		state->blitter_endmask3 = data;
	}
}

WRITE16_HANDLER( atarist_blitter_dst_inc_x_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->blitter_dst_inc_x = data & 0xfffe;
}

WRITE16_HANDLER( atarist_blitter_dst_inc_y_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->blitter_dst_inc_y = data & 0xfffe;
}

WRITE16_HANDLER( atarist_blitter_dst_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0:
		state->blitter_dst = (data & 0xff) | (state->blitter_dst & 0xfffe);
	case 1:
		state->blitter_dst = (state->blitter_dst & 0xff0000) | (data & 0xfffe);
	}
}

WRITE16_HANDLER( atarist_blitter_count_x_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->blitter_xcount = data;
}

WRITE16_HANDLER( atarist_blitter_count_y_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	state->blitter_ycount = data;
}

WRITE16_HANDLER( atarist_blitter_op_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	if (ACCESSING_BITS_0_7)
	{
		state->blitter_hop = (data >> 8) & 0x03;
	}
	else
	{
		state->blitter_op = data & 0x0f;
	}
}

WRITE16_HANDLER( atarist_blitter_ctrl_w )
{
	atarist_state *state = (atarist_state *)space->machine->driver_data;

	if (ACCESSING_BITS_0_7)
	{
		state->blitter_ctrl = (data >> 8) & 0xef;

		if (!(state->blitter_ctrl & ATARIST_BLITTER_CTRL_BUSY))
		{
			if ((data >> 8) & ATARIST_BLITTER_CTRL_BUSY)
			{
				int nops = BLITTER_NOPS[state->blitter_op][state->blitter_hop]; // each NOP takes 4 cycles
				timer_set(space->machine, ATTOTIME_IN_HZ((Y2/4)/(4*nops)), NULL, 0, atarist_blitter_tick);
			}
		}
	}
	else
	{
		state->blitter_skew = data & 0xcf;
	}
}

/* Video */

VIDEO_START( atarist )
{
	atarist_state *state = (atarist_state *)machine->driver_data;

	state->shifter_timer = timer_alloc(machine, atarist_shifter_tick, NULL);
	state->glue_timer = timer_alloc(machine, atarist_glue_tick, NULL);

	timer_adjust_periodic(state->shifter_timer, machine->primary_screen->time_until_pos(0,0), 0, ATTOTIME_IN_HZ(Y2/4)); // 125 ns
	timer_adjust_periodic(state->glue_timer, machine->primary_screen->time_until_pos(0,0), 0, ATTOTIME_IN_HZ(Y2/16)); // 500 ns

	/* register for state saving */
	state_save_register_global(machine, state->shifter_base);
	state_save_register_global(machine, state->shifter_ofs);
	state_save_register_global(machine, state->shifter_sync);
	state_save_register_global(machine, state->shifter_mode);
	state_save_register_global_array(machine, state->shifter_palette);
	state_save_register_global(machine, state->shifter_lineofs);
	state_save_register_global(machine, state->shifter_pixelofs);
	state_save_register_global_array(machine, state->shifter_rr);
	state_save_register_global_array(machine, state->shifter_ir);
	state_save_register_global(machine, state->shifter_bitplane);
	state_save_register_global(machine, state->shifter_shift);
	state_save_register_global(machine, state->shifter_h);
	state_save_register_global(machine, state->shifter_v);
	state_save_register_global(machine, state->shifter_de);

	state_save_register_global_array(machine, state->blitter_halftone);
	state_save_register_global(machine, state->blitter_src_inc_x);
	state_save_register_global(machine, state->blitter_src_inc_y);
	state_save_register_global(machine, state->blitter_dst_inc_x);
	state_save_register_global(machine, state->blitter_dst_inc_y);
	state_save_register_global(machine, state->blitter_src);
	state_save_register_global(machine, state->blitter_dst);
	state_save_register_global(machine, state->blitter_endmask1);
	state_save_register_global(machine, state->blitter_endmask2);
	state_save_register_global(machine, state->blitter_endmask3);
	state_save_register_global(machine, state->blitter_xcount);
	state_save_register_global(machine, state->blitter_ycount);
	state_save_register_global(machine, state->blitter_xcountl);
	state_save_register_global(machine, state->blitter_hop);
	state_save_register_global(machine, state->blitter_op);
	state_save_register_global(machine, state->blitter_ctrl);
	state_save_register_global(machine, state->blitter_skew);

	atarist_set_screen_parameters(machine);

	VIDEO_START_CALL(generic_bitmapped);
}
