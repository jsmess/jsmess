/***************************************************************************

  $Id: pc8801.c,v 1.21 2006/09/15 02:51:43 npwoods Exp $

***************************************************************************/

/* NOTE: The palette and colortable have been fixed after the changes in the 0.123-0.124 cycle,
    but the pen colours may be wrong. This needs to be tested. */

#include "emu.h"
#include "machine/i8255a.h"
#include "includes/pc8801.h"

/* NPW 23-Oct-2001 - Adding this so that it compiles */
#define palette_transparent_pen	0



enum
{
	noblink_underline,
	blink_underline,
	noblink_block,
	blink_block
};

enum
{
	parameter0,
	parameter1,
	parameter2,
	parameter3,
	parameter4,
	cursorx,
	cursory,
	lpenx,
	lpeny,
	other
};

enum
{
	GRAPH_NO,
	GRAPH_200_COL,
	GRAPH_200_BW,
	GRAPH_400_BW
};


#define TX_SEC		0x0001
#define TX_BL		0x0002
#define TX_REV		0x0004
#define TX_CUR		0x0008
#define TX_OL		0x0010
#define TX_UL		0x0020
#define TX_GL		0x1000
#define TX_COL_MASK	0xe000
#define TX_COL_SHIFT    13
#define TX_80		0x0000
#define TX_40L		0x0100
#define TX_40R		0x0200
#define TX_WID_MASK	0x0300
#define TX_WID_SHIFT	8


#define TRAM(x,y) (state->pc88sr_is_highspeed ? \
	state->pc88sr_textRAM[(state->dmac_addr[2]+(x)+(y)*120)&0xfff] : \
	state->mainRAM[(state->dmac_addr[2]+(x)+(y)*120)&0xffff])
#define ATTR_TMP(x,y) (state->attr_tmp[(x)+(y)*80])
#define TEXT_OLD(x,y) (state->text_old[(x)+(y)*80])
#define ATTR_OLD(x,y) (state->attr_old[(x)+(y)*80])
#define GRP_DIRTY(x,y) (state->graph_dirty[(x)+(y)*80])

INLINE void pc8801_plot_pixel( bitmap_t *bitmap, int x, int y, UINT32 color )
{
	*BITMAP_ADDR16(bitmap, y, x) = (UINT16)color;
}

void pc8801_video_init( running_machine *machine, int hireso )
{
	pc88_state *state = machine->driver_data<pc88_state>();
	screen_device *screen = screen_first(*machine);
	int width = screen->width();
	int height = screen->height();

	state->wbm1 = auto_bitmap_alloc(machine, width, height, BITMAP_FORMAT_INDEXED16);
	state->wbm2 = auto_bitmap_alloc(machine, width, height, BITMAP_FORMAT_INDEXED16);
	state->is_24KHz = hireso;
	state->crtc_on = 0;
	state->text_on = 1;

	state->dmac_fl = 0;
	state->dmac_addr[0] = state->dmac_size[0] = 0;
	state->dmac_addr[1] = state->dmac_size[1] = 0;
	state->dmac_addr[2] = state->dmac_size[2] = 0;
	state->dmac_flag = state->dmac_status = 0;

	state->gVRAM = auto_alloc_array(machine, UINT8, 0xc000);
	state->pc88sr_textRAM = auto_alloc_array(machine, UINT8, 0x1000);
	state->attr_tmp = auto_alloc_array(machine, UINT16, 80*25);
	state->attr_old = auto_alloc_array(machine, UINT16, 80*100);
	state->text_old = auto_alloc_array(machine, UINT16, 80*100);
	state->graph_dirty = auto_alloc_array(machine, char, 80*100);

	memset(state->gVRAM, 0, 0xc000);
	memset(state->pc88sr_textRAM, 0, 0x1000);
	state->selected_vram = 0;
	state->alu_on = 0;
	state->analog_palette = 0;
}

static WRITE8_HANDLER( write_gvram )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	int x, y;

	if (state->gVRAM[offset] != data)
	{
		state->gVRAM[offset] = data;
		switch (state->gmode)
		{
		case GRAPH_200_COL:
		case GRAPH_200_BW:
			x = (offset & 0x3fff) % 80;
			y = (offset & 0x3fff) / 80 / 2;
			break;
		case GRAPH_400_BW:
			x = (offset & 0x3fff) % 80;
			y = (offset & 0x3fff) / 80 / 4 + (offset / 0x4000) * 50;
			break;
		case GRAPH_NO:
		default:
			return;
		}
		if (y < 100)
			GRP_DIRTY(x, y) = 1;
	}
}


static WRITE8_HANDLER(write_gvram0_bank5){write_gvram(space, offset + 0x4000 * 0, data);}
static WRITE8_HANDLER(write_gvram0_bank6){write_gvram(space, offset + 0x4000 * 0 + 0x3000, data);}
static WRITE8_HANDLER(write_gvram1_bank5){write_gvram(space, offset + 0x4000 * 1, data);}
static WRITE8_HANDLER(write_gvram1_bank6){write_gvram(space, offset + 0x4000 * 1 + 0x3000, data);}
static WRITE8_HANDLER(write_gvram2_bank5){write_gvram(space, offset + 0x4000 * 2, data);}
static WRITE8_HANDLER(write_gvram2_bank6){write_gvram(space, offset + 0x4000 * 2 + 0x3000, data);}


static UINT8 read_save_alu( address_space *space, int x, UINT32 offset )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	state->alu_save0 = state->gVRAM[offset + 0x0000];
	state->alu_save1 = state->gVRAM[offset + 0x4000];
	state->alu_save2 = state->gVRAM[offset + 0x8000];

	return (x & 1 ? state->gVRAM[offset + 0x0000] : ~state->gVRAM[offset + 0x0000]) &
			(x & 2 ? state->gVRAM[offset + 0x4000] : ~state->gVRAM[offset + 0x4000]) &
			(x & 4 ? state->gVRAM[offset + 0x8000] : ~state->gVRAM[offset + 0x8000]);
}

static READ8_HANDLER(read_gvram_alu0_bank5){return read_save_alu(space, 0, offset);}
static READ8_HANDLER(read_gvram_alu0_bank6){return read_save_alu(space, 0, offset + 0x3000);}
static READ8_HANDLER(read_gvram_alu1_bank5){return read_save_alu(space, 1, offset);}
static READ8_HANDLER(read_gvram_alu1_bank6){return read_save_alu(space, 1, offset + 0x3000);}
static READ8_HANDLER(read_gvram_alu2_bank5){return read_save_alu(space, 2, offset);}
static READ8_HANDLER(read_gvram_alu2_bank6){return read_save_alu(space, 2, offset + 0x3000);}
static READ8_HANDLER(read_gvram_alu3_bank5){return read_save_alu(space, 3, offset);}
static READ8_HANDLER(read_gvram_alu3_bank6){return read_save_alu(space, 3, offset + 0x3000);}
static READ8_HANDLER(read_gvram_alu4_bank5){return read_save_alu(space, 4, offset);}
static READ8_HANDLER(read_gvram_alu4_bank6){return read_save_alu(space, 4, offset + 0x3000);}
static READ8_HANDLER(read_gvram_alu5_bank5){return read_save_alu(space, 5, offset);}
static READ8_HANDLER(read_gvram_alu5_bank6){return read_save_alu(space, 5, offset + 0x3000);}
static READ8_HANDLER(read_gvram_alu6_bank5){return read_save_alu(space, 6, offset);}
static READ8_HANDLER(read_gvram_alu6_bank6){return read_save_alu(space, 6, offset + 0x3000);}
static READ8_HANDLER(read_gvram_alu7_bank5){return read_save_alu(space, 7, offset);}
static READ8_HANDLER(read_gvram_alu7_bank6){return read_save_alu(space, 7, offset + 0x3000);}


static WRITE8_HANDLER(write_gvram_alu0)
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	switch (state->alu_1 & 0x11)
	{
	case 0x00:
		write_gvram(space, offset, state->gVRAM[offset] & ~data);
		break;
	case 0x01:
		write_gvram(space, offset, state->gVRAM[offset] | data);
		break;
	case 0x10:
		write_gvram(space, offset, state->gVRAM[offset] ^ data);
		break;
	case 0x11:
		break;
	}

	switch (state->alu_1 & (0x11 << 1))
	{
	case 0x00 << 1:
		write_gvram(space, offset + 0x4000, state->gVRAM[offset + 0x4000] & ~data);
		break;
	case 0x01 << 1:
		write_gvram(space, offset + 0x4000, state->gVRAM[offset + 0x4000] | data);
		break;
	case 0x10 << 1:
		write_gvram(space, offset + 0x4000, state->gVRAM[offset + 0x4000] ^ data);
		break;
	case 0x11 << 1:
		break;
	}

	switch (state->alu_1 & (0x11 << 2))
	{
	case 0x00 << 2:
		write_gvram(space, offset + 0x8000, state->gVRAM[offset + 0x8000] & ~data);
		break;
	case 0x01 << 2:
		write_gvram(space, offset + 0x8000, state->gVRAM[offset + 0x8000] | data);
		break;
	case 0x10 << 2:
		write_gvram(space, offset + 0x8000, state->gVRAM[offset + 0x8000] ^ data);
		break;
	case 0x11 << 2:
		break;
	}
}

static WRITE8_HANDLER(write_gvram_alu1)
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	write_gvram(space, offset + 0x0000 , state->alu_save0);
	write_gvram(space, offset + 0x4000 , state->alu_save1);
	write_gvram(space, offset + 0x8000 , state->alu_save2);
}

static WRITE8_HANDLER(write_gvram_alu2){
	pc88_state *state = space->machine->driver_data<pc88_state>();write_gvram(space, offset + 0x0000, state->alu_save1);}

static WRITE8_HANDLER(write_gvram_alu3){
	pc88_state *state = space->machine->driver_data<pc88_state>();write_gvram(space, offset + 0x4000, state->alu_save0);}


static WRITE8_HANDLER(write_gvram_alu0_bank5){write_gvram_alu0(space, offset,data);}
static WRITE8_HANDLER(write_gvram_alu0_bank6){write_gvram_alu0(space, offset + 0x3000,data);}
static WRITE8_HANDLER(write_gvram_alu1_bank5){write_gvram_alu1(space, offset,data);}
static WRITE8_HANDLER(write_gvram_alu1_bank6){write_gvram_alu1(space, offset + 0x3000,data);}
static WRITE8_HANDLER(write_gvram_alu2_bank5){write_gvram_alu2(space, offset,data);}
static WRITE8_HANDLER(write_gvram_alu2_bank6){write_gvram_alu2(space, offset + 0x3000,data);}
static WRITE8_HANDLER(write_gvram_alu3_bank5){write_gvram_alu3(space, offset,data);}
static WRITE8_HANDLER(write_gvram_alu3_bank6){write_gvram_alu3(space, offset + 0x3000,data);}


int pc8801_is_vram_select( running_machine *machine )
{
	pc88_state *state = machine->driver_data<pc88_state>();
	if (state->alu_on)
	{
		/* ALU mode */
		if (state->alu_2 & 0x80)
		{
			/* ALU VRAM */
			switch (state->alu_2 & 0x07)
			{
			case 0:
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, read_gvram_alu0_bank5);
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, read_gvram_alu0_bank6);
				break;
			case 1:
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, read_gvram_alu1_bank5);
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, read_gvram_alu1_bank6);
				break;
			case 2:
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, read_gvram_alu2_bank5);
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, read_gvram_alu2_bank6);
				break;
			case 3:
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, read_gvram_alu3_bank5);
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, read_gvram_alu3_bank6);
				break;
			case 4:
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, read_gvram_alu4_bank5);
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, read_gvram_alu4_bank6);
				break;
			case 5:
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, read_gvram_alu5_bank5);
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, read_gvram_alu5_bank6);
				break;
			case 6:
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, read_gvram_alu6_bank5);
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, read_gvram_alu6_bank6);
				break;
			case 7:
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, read_gvram_alu7_bank5);
				memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, read_gvram_alu7_bank6);
				break;
    		}

			switch (state->alu_2 & 0x30)
			{
			case 0x00:
				memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, write_gvram_alu0_bank5);
				memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, write_gvram_alu0_bank6);
				break;
			case 0x10:
				memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, write_gvram_alu1_bank5);
				memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, write_gvram_alu1_bank6);
				break;
			case 0x20:
				memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, write_gvram_alu2_bank5);
				memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, write_gvram_alu2_bank6);
				break;
			case 0x30:
				memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, write_gvram_alu3_bank5);
				memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, write_gvram_alu3_bank6);
				break;
			}
    		return 1;
		}
		else
		{
			/* MAIN RAM */
			return 0;
		}
	}
	else
	{
		/* old mode */
		switch (state->selected_vram)
		{
		case 1:
			memory_set_bankptr(machine, "bank5", state->gVRAM );
			memory_set_bankptr(machine, "bank6", state->gVRAM + 0x3000 );
			memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, "bank5");
			memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, write_gvram0_bank5);
			memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, "bank6");
			memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, write_gvram0_bank6);
			return 1;

		case 2:
			memory_set_bankptr(machine, "bank5", state->gVRAM + 0x4000 );
			memory_set_bankptr(machine, "bank6", state->gVRAM + 0x4000 + 0x3000 );
			memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, "bank5");
			memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, write_gvram1_bank5);
			memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, "bank6");
			memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, write_gvram1_bank6);
			return 1;

		case 3:
			memory_set_bankptr(machine, "bank5", state->gVRAM + 0x8000 );
			memory_set_bankptr(machine, "bank6", state->gVRAM + 0x8000 + 0x3000 );
			memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, "bank5");
			memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xefff, 0, 0, write_gvram2_bank5);
			memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, "bank6");
			memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf000, 0xffff, 0, 0, write_gvram2_bank6);
			return 1;

		default:
			return 0; /* MAIN RAM */
		}
	}

}

WRITE8_HANDLER( pc88sr_disp_32 )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	state->analog_palette = ((data & 0x20) != 0x00);
	state->alu_on = ((data & 0x40) != 0x00);
	pc8801_update_bank(space->machine);
}

WRITE8_HANDLER( pc88sr_alu )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	switch (offset)
	{
	case 0:
		state->alu_1 = data;
		break;
	case 1:
		state->alu_2 = data;
		break;
	}
	pc8801_update_bank(space->machine);
}

WRITE8_HANDLER( pc88_vramsel_w )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	if (offset == 3)
		state->selected_vram = 0;
	else
		state->selected_vram = offset + 1;

	pc8801_update_bank(space->machine);
}

READ8_HANDLER( pc88_vramtest_r )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	switch (state->selected_vram)
	{
	case 1:  return 0xf9;
	case 2:  return 0xfa;
	case 3:  return 0xfc;
	default: return 0xf8;
	}
}

VIDEO_START( pc8801 )
{
	pc88_state *state = machine->driver_data<pc88_state>();
	state->text_width = 40;
	state->text_height = 20;
	state->blink_period = 24;
}

#define BLOCK_YSIZE (state->is_24KHz ? 4 : 2)

VIDEO_UPDATE( pc8801 )
{
	pc88_state *state = screen->machine->driver_data<pc88_state>();
	int x, y, attr_new, text_new, i, a, tx, ty, oy, gx, gy, ct, cg;
	int full_refresh = 1;

	state->blink_count = (state->blink_count + 1) % (state->blink_period * 4);
	/* attribute expand */
	for (y = 0; y < 25; y++)
	{
		x = 0;
		attr_new = 7 << TX_COL_SHIFT;
		for (i = 0; i <= 20; i++)
		{
			a = TRAM(80 + i * 2 + 1, y);
			if (state->text_color)
			{
				/* color mode */
				if (a & 0x08)
				{
					/* color */
					attr_new = (attr_new & 0x00ff) | ((a & 0xf0) << 8);
				}
				else
				{
					/* attr */
					attr_new = (attr_new & 0xff00)|(a & 37);
				}
			}
			else
			{
				/* B/W mode */
				attr_new = (a & 0x37) | ((a & 0x80) << 5) | (7 << TX_COL_SHIFT);
			}
			if (TRAM(118, y) == 0x80)
			{
				while (x < TRAM(80 + i * 2, y) && x < 80)
				{
					ATTR_TMP(x, y) = attr_new;
					x++;
				};
			}
			else
			{
				while (x < TRAM(80 + i * 2 + 2, y) && x < 80)
				{
					ATTR_TMP(x, y) = attr_new;
					x++;
				};
			}
		}
		while (x < 80)
		{
			ATTR_TMP(x, y) = attr_new;
			x++;
		};
	}
	if (state->text_cursor && (state->text_cursorX >= 0 || state->text_cursorX < 80) && (state->text_cursorY >= 0 || state->text_cursorY < 25))
	{
		ATTR_TMP(state->text_cursorX, state->text_cursorY) |= TX_CUR;
	}

	/* display draw */
	for (y = 0; y < 100; y++)
	{
		ty = y / (100 / state->text_height);
		oy = y % (100 / state->text_height);
		for (x = 0; x < 80; x++)
		{
			/* text */
			if (state->text_width == 40)
				tx = x & (~1);
			else
				tx = x;

			attr_new = ATTR_TMP(tx, ty);

			if (oy < 4)
				text_new = TRAM(tx, ty) * 4 + oy;
			else
				text_new = 0;

			if (attr_new & TX_SEC)
			{
				text_new = 0;
				attr_new &= ~TX_SEC;
			}

			if (attr_new & TX_BL)
			{
				attr_new &= ~TX_BL;
				if ((state->blink_count / state->blink_period) & 1)
					attr_new ^= TX_REV;
			}

			if (attr_new & TX_CUR)
			{
				attr_new &= ~TX_CUR;
				switch (state->cursor_mode)
				{
				case noblink_underline:
					attr_new ^= TX_UL;
					break;
				case blink_underline:
					if (state->blink_count / state->blink_period)
						attr_new ^= TX_UL;
					break;
				case noblink_block:
					attr_new ^= TX_REV;
					break;
				case blink_block:
					if (state->blink_count / state->blink_period)
						attr_new ^= TX_REV;
					break;
				}
			}

			if ((attr_new & TX_UL) && oy != 3)
				attr_new &= ~TX_UL;

			if ((attr_new & TX_OL) && oy != 0)
				attr_new &= ~TX_OL;

			if (state->text_invert)
				attr_new ^= TX_REV;

			if (!state->crtc_on || ((state->dmac_flag & 0x04) == 0x00) || !state->text_on)
			{
				text_new = 0;
				attr_new = 7 << TX_COL_SHIFT;
			}
			if (state->text_width == 40)
			{
				if (x & 1)
					attr_new |= TX_40R;
				else
					attr_new |= TX_40L;
			}
			else
			{
				attr_new |= TX_80;
			}

			if (text_new != TEXT_OLD(x, y) || attr_new != ATTR_OLD(x, y) || GRP_DIRTY(x, y) || full_refresh)
			{
				plot_box(state->wbm2, x * 8, y * BLOCK_YSIZE, 8, BLOCK_YSIZE, palette_transparent_pen);
				ct = ((attr_new & TX_COL_MASK) >> TX_COL_SHIFT) + 8;
				TEXT_OLD(x, y) = text_new;
				ATTR_OLD(x, y) = attr_new;
				if (attr_new & TX_GL)
				{
					/* low resolution(N-BASIC mode) graphics */
					text_new >>= (text_new & 3);
					if (attr_new & TX_REV)
						text_new = ~text_new;

					switch (attr_new & TX_WID_MASK)
					{
					case TX_80:
						if (text_new & 0x04)
							plot_box(state->wbm2, x * 8, y * BLOCK_YSIZE, 4, BLOCK_YSIZE, ct);
						if (text_new & 0x40)
							plot_box(state->wbm2, x * 8 + 4,y * BLOCK_YSIZE, 4, BLOCK_YSIZE, ct);
						break;
					case TX_40L:
						if (text_new & 0x04)
							plot_box(state->wbm2, x * 8, y * BLOCK_YSIZE, 8, BLOCK_YSIZE, ct);
						break;
					case TX_40R:
						if (text_new & 0x40)
							plot_box(state->wbm2, x * 8, y * BLOCK_YSIZE, 8, BLOCK_YSIZE, ct);
						break;
					}
				}
				else
				{
					/* normal text */
					drawgfx_transpen(state->wbm2, NULL, screen->machine->gfx[((attr_new & TX_WID_MASK) >> TX_WID_SHIFT) + (state->is_24KHz ? 3 : 0)], text_new,
					((attr_new & TX_REV) ? 8 : 0) + ((attr_new & TX_COL_MASK) >> TX_COL_SHIFT), 0, 0, x * 8, y * BLOCK_YSIZE, (attr_new & TX_REV) ? 1 : 0);
				}
				if (attr_new & TX_UL)
					plot_box(state->wbm2, x * 8, y * BLOCK_YSIZE + (state->is_24KHz ? 3 : 1), 8, 1, (attr_new & TX_REV) ? palette_transparent_pen : ct);

				if (attr_new & TX_OL)
					plot_box(state->wbm2, x * 8, y * BLOCK_YSIZE, 8, 1, (attr_new & TX_REV) ? palette_transparent_pen : ct);

				/* graphics */
				GRP_DIRTY(x, y) = 0;
				if (state->is_24KHz)
				{
					switch (state->gmode)
					{
					case GRAPH_200_COL:
						for (gy = 0; gy < 2; gy++)
						{
							for (gx = 0; gx < 8; gx++)
							{
								cg = (((state->gVRAM[0x0000 + x + y * 2 * 80 + gy * 80] << gx) & 0x80) >> 7) |
									(((state->gVRAM[0x4000 + x + y * 2 * 80 + gy * 80] << gx) & 0x80) >> 6) |
									(((state->gVRAM[0x8000 + x + y * 2 * 80 + gy * 80] << gx) & 0x80) >> 5);
								pc8801_plot_pixel(state->wbm1, x * 8 + gx, y * 4 + gy * 2, cg);
								pc8801_plot_pixel(state->wbm1, x * 8 + gx, y * 4 + gy * 2 + 1, 17);
							}
						}
						break;
					case GRAPH_200_BW:
						for (gy = 0; gy < 2; gy++)
						{
							for (gx = 0; gx < 8; gx++)
							{
								cg = (((state->gVRAM[0x0000 + x + y * 2 * 80 + gy * 80] << gx) & 0x80) && state->disp_plane[0]) ||
									(((state->gVRAM[0x4000 + x + y * 2 * 80 + gy * 80] << gx) & 0x80) && state->disp_plane[1]) ||
									(((state->gVRAM[0x8000 + x + y * 2 * 80 + gy * 80] << gx) & 0x80) && state->disp_plane[2]) ?
									((attr_new & TX_COL_MASK) >> TX_COL_SHIFT) + 8 : 16;
								pc8801_plot_pixel(state->wbm1, x * 8 + gx, y * 4 + gy * 2, cg);
								pc8801_plot_pixel(state->wbm1, x * 8 + gx, y * 4 + gy * 2 + 1, 17);
							}
						}
						break;
					case GRAPH_400_BW:
						for (gy = 0; gy < 4; gy++)
						{
							for (gx = 0; gx < 8; gx++)
							{
								cg = ((state->gVRAM[(y < 50 ? 0x0000 : 0x4000) + x + (y % 50) * 4 * 80 + gy * 80] << gx) & 0x80)
									&& state->disp_plane[y < 200 ? 0 : 1] ? ((attr_new&TX_COL_MASK) >> TX_COL_SHIFT) + 8 : 16;
								pc8801_plot_pixel(state->wbm1, x * 8 + gx, y * 4 + gy, cg);
							}
						}
						break;
					case GRAPH_NO:
					default:
						plot_box(state->wbm1, x * 8, y * 4, 8, 4, 16);
						break;
					}
				}
				else
				{
					switch (state->gmode)
					{
					case GRAPH_200_COL:
						for (gy = 0; gy < 2; gy++)
						{
							for (gx = 0; gx < 8; gx++)
							{
								cg = (((state->gVRAM[0x0000 + x + y * 2 * 80 + gy * 80] << gx) & 0x80) >> 7) |
									(((state->gVRAM[0x4000 + x + y * 2 * 80 + gy * 80] << gx) & 0x80) >> 6) |
									(((state->gVRAM[0x8000 + x + y * 2 * 80 + gy * 80] << gx) & 0x80) >> 5);
								pc8801_plot_pixel(state->wbm1, x * 8 + gx, y * 2 + gy, cg);
							}
						}
						break;
					case GRAPH_200_BW:
						for (gy = 0; gy < 2; gy++)
						{
							for (gx = 0; gx < 8; gx++)
							{
								cg = (((state->gVRAM[0x0000 + x + y * 2 * 80 + gy * 80] << gx) & 0x80) && state->disp_plane[0]) ||
									(((state->gVRAM[0x4000 + x + y * 2 * 80 + gy * 80] << gx) & 0x80) && state->disp_plane[1]) ||
									(((state->gVRAM[0x8000 + x + y * 2 * 80 + gy * 80] << gx) & 0x80) && state->disp_plane[2]) ?
									((attr_new & TX_COL_MASK) >> TX_COL_SHIFT) + 8 : 16;
								pc8801_plot_pixel(state->wbm1, x * 8 + gx, y * 2 + gy, cg);
							}
						}
						break;
					case GRAPH_NO:
					default:
						plot_box(state->wbm1, x * 8, y * 2, 8, 2, 16);
						break;
					}
				}
			}
		}
	}

	copyscrollbitmap_trans(state->wbm1, state->wbm2, 0, 0, 0, 0, NULL, palette_transparent_pen);
	copybitmap(bitmap, state->wbm1, 0, 0, 0, 0, NULL);
	return 0;
}

PALETTE_INIT( pc8801 )
{
	UINT8 i;

	machine->colortable = colortable_alloc(machine, 18);

	for (i = 0; i < 8; i++)		/* for graphics */
		colortable_palette_set_color(machine->colortable, i, RGB_BLACK);


	for (i = 0; i < 8; i++)		/* standard colours */
		colortable_palette_set_color(machine->colortable, i + 8,
			MAKE_RGB((i & 2) ? 0xff : 0, (i & 4) ? 0xff : 0, (i & 1) ? 0xff : 0));

	/* for background and scanline */
	colortable_palette_set_color(machine->colortable, 16, RGB_BLACK);
	colortable_palette_set_color(machine->colortable, 17, RGB_BLACK);


	for (i = 0; i < 8; i++)
	{
		colortable_entry_set_value(machine->colortable, i * 2, 0);
		colortable_entry_set_value(machine->colortable, i * 2 + 1, i + 8);
		colortable_entry_set_value(machine->colortable, i * 2 + 16, i + 8);
		colortable_entry_set_value(machine->colortable, i * 2 + 17, 0);
	}
}

WRITE8_HANDLER( pc88_crtc_w )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	if (offset == 1)
	{
		/* command */
		switch (data & 0xe0)
		{
		case 0x00:
			/* disp stop / initilize */
			state->crtc_on = 0;
			state->crtc_state = parameter0;
			return;
		case 0x20:
			/* disp on */
			state->crtc_on = 1;
			state->text_invert = ((data & 1) != 0x00);
			return;
		case 0x40:
		/* case 0x43: */
			/* interupt mask */
			return;
		case 0x60:
			/* get light pen point */
			state->crtc_state = lpenx;
			return;
		case 0x80:
			/* set cursor */
			state->crtc_state = cursorx;
			state->text_cursor = ((data & 1) != 0x00);
			return;
		default:
			logerror("pc8801: illegal crtc command 0x%.2x.\n", data);
			return;
		}
	}
	else
	{
		/* parameter */
		switch (state->crtc_state)
		{
		case parameter0:
			state->crtc_state = parameter1;
			if (data == 0xce)
				return;
			logerror("pc8801: illegal crtc parameter0 0x%.2x.\n", data);
			return;

		case parameter1:
			state->crtc_state = parameter2;
			switch (data & 0xc0)
			{
			case 0x00:
				state->blink_period = 8;
				break;
			case 0x40:
				state->blink_period = 16;
				break;
			case 0x80:
				state->blink_period = 24;
				break;
			case 0xc0:
				state->blink_period = 32;
				break;
			}
			if ((data & 0x3f) == 0x13)
			{
				state->text_height = 20;
				return;
			}
			else if ((data & 0x3f) == 0x18)
			{
				state->text_height = 25;
				return;
			}
			logerror("pc8801: illegal crtc parameter1 0x%.2x.\n", data);
			return;

		case parameter2:
			state->crtc_state = parameter3;
			switch (data & 0x60)
			{
			case 0x00:
				state->cursor_mode = noblink_underline;
				break;
			case 0x20:
				state->cursor_mode = blink_underline;
				break;
			case 0x40:
				state->cursor_mode = noblink_block;
				break;
			case 0x60:
				state->cursor_mode = blink_block;
				break;
			}
			if (!state->is_24KHz && (data & 0x9f) == 0x09 && state->text_height == 20)
				return;
			else if (!state->is_24KHz && (data & 0x9f) == 0x07 && state->text_height == 25)
				return;
			else if (state->is_24KHz && (data & 0x9f) == 0x13 && state->text_height == 20)
				return;
			else if (state->is_24KHz && (data & 0x9f) == 0x0f && state->text_height == 25)
				return;
			logerror("pc8801: illegal crtc parameter2 0x%.2x.\n", data);
			return;

		case parameter3:
			state->crtc_state = parameter4;
			if (!state->is_24KHz && data == 0xbe && state->text_height == 20)
				return;
			else if (!state->is_24KHz && data == 0xde && state->text_height == 25)
				return;
			else if (state->is_24KHz && data == 0x38 && state->text_height == 20)
				return;
			else if (state->is_24KHz && data == 0x58 && state->text_height == 25)
				return;
			logerror("pc8801: illegal crtc parameter3 0x%.2x.\n", data);
			return;

		case parameter4:
			state->crtc_state = other;
			if (data == 0x13)
			{
				state->text_color = 0;
				return;
			}
			else if (data == 0x53)
			{
				state->text_color = 1;
				return;
			}
			logerror("pc8801: illegal crtc parameter4 0x%.2x.\n", data);
			return;

		case cursorx:
			state->crtc_state = cursory;
			state->text_cursorX = data;
			return;

		case cursory:
			state->crtc_state = other;
			state->text_cursorY = data;
			return;

		default:
			logerror("pc8801: illegal crtc parameter 0x%.2x.\n", data);
			return;
		}
	}
}

READ8_HANDLER( pc88_crtc_r )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	if (offset == 0)
	{
		/* light pen point */
		switch (state->crtc_state)
		{
		case lpenx:
			state->crtc_state = lpeny;
			return 0xff;
		case lpeny:
			state->crtc_state = other;
			return 0xff;
		default:
			logerror("pc8801:illegal light pen read\n");
			return 0xff;
		}
	}
	else
	{
		/* status */
		return state->crtc_on ? 0x10 : 0x00;
	}
}

WRITE8_HANDLER( pc88_dmac_w )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	switch (offset)
	{
	case 8:
		state->dmac_fl = 0;
		state->dmac_flag = data;
		return;
	case 1:
	case 3:
	case 5:
	case 7:
		if (state->dmac_fl == 0)
		{
			state->dmac_fl = 1;
			state->dmac_size[offset / 2] = (state->dmac_size[offset / 2] & 0xff00) | (data & 0x00ff);
			return;
		}
		else
		{
			state->dmac_fl = 0;
			state->dmac_size[offset / 2] = (state->dmac_size[offset / 2] & 0x00ff) | ((data << 8) & 0xff00);
			return;
		}
	case 0:
	case 2:
	case 4:
	case 6:
		if (state->dmac_fl == 0)
		{
			state->dmac_fl = 1;
			state->dmac_addr[offset / 2] = (state->dmac_addr[offset / 2] & 0xff00) | (data & 0x00ff);
			return;
		}
		else
		{
			state->dmac_fl = 0;
			state->dmac_addr[offset / 2] = (state->dmac_addr[offset / 2] & 0x00ff) | ((data << 8) & 0xff00);
			return;
		}
	}
}

READ8_HANDLER( pc88_dmac_r )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	switch (offset)
	{
	case 8:
		return state->dmac_status;
	case 1:
	case 3:
	case 5:
	case 7:
		if (state->dmac_fl == 0)
		{
			state->dmac_fl = 1;
			return (state->dmac_size[offset / 2] & 0x00ff);
		}
		else
		{
			state->dmac_fl = 0;
			return ((state->dmac_size[offset / 2] >> 8) & 0x00ff);
		}
	case 0:
	case 2:
	case 4:
	case 6:
		if (state->dmac_fl == 0)
		{
			state->dmac_fl = 1;
			return (state->dmac_addr[offset / 2] & 0x00ff);
		}
		else
		{
			state->dmac_fl = 0;
			return ((state->dmac_addr[offset / 2] >> 8) & 0x00ff);
		}
	}
	return 0;
}

WRITE8_HANDLER( pc88sr_disp_30 )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	state->text_width = ((data & 0x01) == 0x00) ? 40 : 80;
}

WRITE8_HANDLER( pc88sr_disp_31 )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	int gmode_new = GRAPH_NO;

	switch (data & 0x19)
	{
	case 0x00:
	case 0x01:
	case 0x10:
	case 0x11:
		gmode_new = GRAPH_NO;
		break;
	case 0x08:
		gmode_new = state->is_24KHz ? GRAPH_400_BW : GRAPH_200_BW;
		break;
	case 0x09:
		gmode_new = GRAPH_200_BW;
		break;
	case 0x18:
	case 0x19:
		gmode_new = GRAPH_200_COL;
		break;
	}

	if (state->gmode != gmode_new)
		state->gmode = gmode_new;
}

WRITE8_HANDLER(pc88_palette_w)
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	int palno;
	int i;

	if (offset == 0)
		palno = 16;
	else if (offset == 1)
	{
		state->text_on = (data & 1) == 0x00;

		for (i = 0; i < 3; i++)
		{
			if (state->disp_plane[i] != ((data & (2 << i)) == 0x00))
				state->disp_plane[i] = (data & (2<<i)) == 0x00;
		}
		return;
	}
	else
		palno = offset - 2;

	if (state->analog_palette)
	{
		if ((data & 0x40) == 0x00)
		{
			state->pal[offset].b = ((data << 5) & 0xe0) | ((data << 2) & 0x1c) | ((data >> 1) & 0x03);
			state->pal[offset].r = ((data << 2) & 0xe0) | ((data >> 1) & 0x1c) | ((data >> 4) & 0x03);
		}
		else
		{
			state->pal[offset].g = ((data << 5) & 0xe0) | ((data << 2) & 0x1c) | ((data >> 1) & 0x03);
		}
	}
	else
	{
		state->pal[offset].b = (data & 1) ? 0xff : 0x00;
		state->pal[offset].r = (data & 2) ? 0xff : 0x00;
		state->pal[offset].g = (data & 4) ? 0xff : 0x00;
	}
	colortable_palette_set_color(space->machine->colortable, palno, MAKE_RGB(state->pal[offset].r, state->pal[offset].g, state->pal[offset].b));
}
