/*

	TODO:

	- medium resolution
	- high resolution
	- change screen modes on the fly
	- blitter
	- STe pixelofs

*/

#include "driver.h"
#include "video/generic.h"
#include "cpu/m68000/m68k.h"
#include "cpu/m68000/m68000.h"
#include "machine/68901mfp.h"
#include "video/atarist.h"

static struct SHIFTER
{
	UINT32 base;
	UINT32 ofs;
	UINT8 sync;
	UINT8 mode;
	UINT16 palette[16];
	UINT8 lineofs;  // STe
	UINT8 pixelofs; // STe
	UINT16 rr[4];
	UINT16 ir[4];
	int bitplane;
	int shift;
	int h, v;
} shifter;

static mame_timer *atarist_glue_timer;
static mame_timer *atarist_shifter_timer;
static mame_bitmap *atarist_bitmap;

static TIMER_CALLBACK(atarist_shifter_tick)
{
	int y = video_screen_get_vpos(0);
	int x = video_screen_get_hpos(0);
	int color;

	switch (shifter.mode)
	{
	case 0: // 320 x 200, 4 Plane
		color = (BIT(shifter.rr[3], 15) << 3) | (BIT(shifter.rr[2], 15) << 2) | (BIT(shifter.rr[1], 15) << 1) | BIT(shifter.rr[0], 15);
		
		*BITMAP_ADDR16(atarist_bitmap, y, x) = Machine->pens[color];

		shifter.rr[0] <<= 1;
		shifter.rr[1] <<= 1;
		shifter.rr[2] <<= 1;
		shifter.rr[3] <<= 1;
		break;

	case 1: // 640 x 200, 2 Plane
		color = (BIT(shifter.rr[1], 15) << 1) | BIT(shifter.rr[0], 15);

		*BITMAP_ADDR16(atarist_bitmap, y, x) = Machine->pens[color];

		shifter.rr[0] <<= 1;
		shifter.rr[1] <<= 1;
		shifter.shift++;

		if (shifter.shift == 15)
		{
			shifter.rr[0] = shifter.rr[2];
			shifter.rr[1] = shifter.rr[3];
			shifter.rr[2] = shifter.rr[3] = 0;
			shifter.shift = 0;
		}
		break;

	case 2: // 640 x 400, 1 Plane
		// unimplemented
		break;
	}
}

static TIMER_CALLBACK(atarist_glue_tick)
{
	UINT8 *RAM = memory_region(REGION_CPU1) + shifter.ofs;

	int hcount = video_screen_get_hpos(0) / 4;
	int vcount = video_screen_get_vpos(0);

	int de;

	if (shifter.sync & 0x02)
	{
		// 50 Hz

		switch (hcount)
		{
		case ATARIST_HBDEND_PAL:
			shifter.h = ASSERT_LINE;
			break;
		case ATARIST_HBDSTART_PAL:
			shifter.h = CLEAR_LINE;
			break;
		case ATARIST_HBSTART_PAL/4:
			cpunum_set_input_line(0, MC68000_IRQ_2, ASSERT_LINE);
			shifter.ofs += (shifter.lineofs * 2); // STe
			break;
		}

		switch (vcount)
		{
		case ATARIST_VBDEND_PAL:
			shifter.v = ASSERT_LINE;
			break;
		case ATARIST_VBDSTART_PAL:
			shifter.v = CLEAR_LINE;
			break;
		case ATARIST_VBSTART_PAL:
			cpunum_set_input_line(0, MC68000_IRQ_4, ASSERT_LINE);
			shifter.ofs = shifter.base;
			break;
		}
	}
	else
	{
		// 60 Hz

		switch (hcount)
		{
		case ATARIST_HBDEND_NTSC:
			shifter.h = ASSERT_LINE;
			break;
		case ATARIST_HBDSTART_NTSC:
			shifter.h = CLEAR_LINE;
			break;
		case ATARIST_HBSTART_NTSC/4:
			cpunum_set_input_line(0, MC68000_IRQ_2, ASSERT_LINE);
			shifter.ofs += (shifter.lineofs * 2); // STe
			break;
		}

		switch (vcount)
		{
		case ATARIST_VBDEND_NTSC:
			shifter.v = ASSERT_LINE;
			break;
		case ATARIST_VBDSTART_NTSC:
			shifter.v = CLEAR_LINE;
			break;
		case ATARIST_VBSTART_NTSC:
			cpunum_set_input_line(0, MC68000_IRQ_4, ASSERT_LINE);
			shifter.ofs = shifter.base;
			break;
		}
	}

	de = shifter.h && shifter.v;

	mfp68901_tbi_w(0, de);
	
	if (de)
	{
		shifter.ir[shifter.bitplane] = (RAM[1] << 8) | RAM[0];
		shifter.bitplane++;
		shifter.ofs += 2;
		
		if (shifter.bitplane == 4)
		{
			shifter.bitplane = 0;
			
			shifter.rr[0] = shifter.ir[0];
			shifter.rr[1] = shifter.ir[1];
			shifter.rr[2] = shifter.ir[2];
			shifter.rr[3] = shifter.ir[3];
		}
	}
}

/* Atari ST Shifter */

READ16_HANDLER( atarist_shifter_base_r )
{
	switch (offset)
	{
	case 0x00:
		return (shifter.base >> 16) & 0x3f;
	case 0x01:
		return (shifter.base >> 8) & 0xff;
	}
	
	return 0;
}

WRITE16_HANDLER( atarist_shifter_base_w )
{
	switch (offset)
	{
	case 0x00:
		shifter.base = (shifter.base & 0x00ff00) | (data & 0x3f) << 16;
		logerror("SHIFTER Video Base Address %06x\n", shifter.base);
		break;
	case 0x01:
		shifter.base = (shifter.base & 0x3f0000) | (data & 0xff) << 8;
		logerror("SHIFTER Video Base Address %06x\n", shifter.base);
		break;
	}
}

READ16_HANDLER( atarist_shifter_counter_r )
{
	switch (offset)
	{
	case 0x00:
		return (shifter.ofs >> 16) & 0x3f;
	case 0x01:
		return (shifter.ofs >> 8) & 0xff;
	case 0x02:
		return shifter.ofs & 0xff;
	}

	return 0;
}

READ16_HANDLER( atarist_shifter_sync_r )
{
	return shifter.sync;
}

WRITE16_HANDLER( atarist_shifter_sync_w )
{
	shifter.sync = data >> 8;
	logerror("SHIFTER Sync %x\n", shifter.sync);
}

READ16_HANDLER( atarist_shifter_mode_r )
{
	return shifter.mode << 8;
}

WRITE16_HANDLER( atarist_shifter_mode_w )
{
	shifter.mode = data >> 8;
	logerror("SHIFTER Mode %x\n", shifter.mode);
}

READ16_HANDLER( atarist_shifter_palette_r )
{
	return shifter.palette[offset];
}
		
WRITE16_HANDLER( atarist_shifter_palette_w )
{
	shifter.palette[offset] = data;
	logerror("SHIFTER Palette[%x] = %x\n", offset, data);

	palette_set_color_rgb(Machine, offset, pal3bit(data >> 8), pal3bit(data >> 4), pal3bit(data));
}

/* Atari STe Shifter */

READ16_HANDLER( atariste_shifter_base_low_r )
{
	return shifter.base & 0xfe;
}

WRITE16_HANDLER( atariste_shifter_base_low_w )
{
	shifter.base = (shifter.base & 0x3fff00) | (data & 0xfe);
	logerror("SHIFTER Video Base Address %06x\n", shifter.base);
}

READ16_HANDLER( atariste_shifter_counter_r )
{
	switch (offset)
	{
	case 0x00:
		return (shifter.ofs >> 16) & 0x3f;
	case 0x01:
		return (shifter.ofs >> 8) & 0xff;
	case 0x02:
		return shifter.ofs & 0xfe;
	}

	return 0;
}

WRITE16_HANDLER( atariste_shifter_counter_w )
{
	switch (offset)
	{
	case 0x00:
		shifter.ofs = (shifter.ofs & 0x00fffe) | (data & 0x3f) << 16;
		logerror("SHIFTER Video Address Counter %06x\n", shifter.ofs);
		break;
	case 0x01:
		shifter.ofs = (shifter.ofs & 0x3f00fe) | (data & 0xff) << 8;
		logerror("SHIFTER Video Address Counter %06x\n", shifter.ofs);
		break;
	case 0x02:
		shifter.ofs = (shifter.ofs & 0x3fff00) | (data & 0xfe);
		logerror("SHIFTER Video Address Counter %06x\n", shifter.ofs);
		break;
	}
}

WRITE16_HANDLER( atariste_shifter_palette_w )
{
	int r = ((data >> 7) & 0x0e) | ((data >> 11) & 0x01);
	int g = ((data >> 3) & 0x0e) | ((data >> 4) & 0x01);
	int b = ((data << 1) & 0x0e) | ((data >> 3) & 0x01);

	shifter.palette[offset] = data;
	logerror("SHIFTER palette %x = %x\n", offset, data);

	palette_set_color_rgb(Machine, offset, r, g, b);
}

READ16_HANDLER( atariste_shifter_lineofs_r )
{
	return shifter.lineofs;
}

WRITE16_HANDLER( atariste_shifter_lineofs_w )
{
	shifter.lineofs = data & 0xff;
	logerror("SHIFTER Line Offset %x\n", shifter.lineofs);
}

READ16_HANDLER( atariste_shifter_pixelofs_r )
{
	return shifter.pixelofs;
}

WRITE16_HANDLER( atariste_shifter_pixelofs_w )
{
	shifter.pixelofs = data & 0x0f;
	logerror("SHIFTER Pixel Offset %x\n", shifter.pixelofs);
}

/* Atari ST Blitter */

static struct BLITTER
{
	UINT16 halftone[16];
	UINT16 src_inc_x, src_inc_y, dst_inc_x, dst_inc_y;
	UINT32 src, dst;
	UINT16 endmask_l, endmask_m, endmask_r;
	UINT16 x, y;
	UINT8 hop, lop, ctrl, skew;
} blitter;

READ16_HANDLER( atarist_blitter_halftone_r )
{
	return blitter.halftone[offset];
}

READ16_HANDLER( atarist_blitter_src_inc_x_r )
{
	return blitter.src_inc_x;
}

READ16_HANDLER( atarist_blitter_src_inc_y_r )
{
	return blitter.src_inc_y;
}

READ16_HANDLER( atarist_blitter_src_r )
{
	switch (offset)
	{
	case 0:
		return (blitter.src >> 16) & 0xff;
	case 1:
		return blitter.src & 0xfffe;
	}

	return 0;
}

READ16_HANDLER( atarist_blitter_end_mask_r )
{
	switch (offset)
	{
	case 0:
		return blitter.endmask_l;
	case 1:
		return blitter.endmask_m;
	case 2:
		return blitter.endmask_r;
	}

	return 0;
}

READ16_HANDLER( atarist_blitter_dst_inc_x_r )
{
	return blitter.dst_inc_x;
}

READ16_HANDLER( atarist_blitter_dst_inc_y_r )
{
	return blitter.dst_inc_y;
}

READ16_HANDLER( atarist_blitter_dst_r )
{
	switch (offset)
	{
	case 0:
		return (blitter.dst >> 16) & 0xff;
	case 1:
		return blitter.dst & 0xfffe;
	}

	return 0;
}

READ16_HANDLER( atarist_blitter_count_x_r )
{
	return blitter.x;
}

READ16_HANDLER( atarist_blitter_count_y_r )
{
	return blitter.y;
}

READ16_HANDLER( atarist_blitter_op_r )
{
	if (mem_mask == 0xff00)
	{
		return blitter.hop;
	}
	else
	{
		return blitter.lop;
	}
}

READ16_HANDLER( atarist_blitter_ctrl_r )
{
	if (mem_mask == 0xff00)
	{
		return blitter.ctrl;
	}
	else
	{
		return blitter.skew;
	}
}

WRITE16_HANDLER( atarist_blitter_halftone_w )
{
	blitter.halftone[offset] = data;
}

WRITE16_HANDLER( atarist_blitter_src_inc_x_w )
{
	blitter.src_inc_x = data;
}

WRITE16_HANDLER( atarist_blitter_src_inc_y_w )
{
	blitter.src_inc_y = data;
}

WRITE16_HANDLER( atarist_blitter_src_w )
{
	switch (offset)
	{
	case 0:
		blitter.src = (data & 0xff) | (blitter.src & 0xfffe);
	case 1:
		blitter.src = (blitter.src & 0xff0000) | (data & 0xfffe);
	}
}

WRITE16_HANDLER( atarist_blitter_end_mask_w )
{
	switch (offset)
	{
	case 0:
		blitter.endmask_l = data;
	case 1:
		blitter.endmask_m = data;
	case 2:
		blitter.endmask_r = data;
	}
}

WRITE16_HANDLER( atarist_blitter_dst_inc_x_w )
{
	blitter.dst_inc_x = data;
}

WRITE16_HANDLER( atarist_blitter_dst_inc_y_w )
{
	blitter.dst_inc_y = data;
}

WRITE16_HANDLER( atarist_blitter_dst_w )
{
	switch (offset)
	{
	case 0:
		blitter.dst = (data & 0xff) | (blitter.dst & 0xfffe);
	case 1:
		blitter.dst = (blitter.dst & 0xff0000) | (data & 0xfffe);
	}
}

WRITE16_HANDLER( atarist_blitter_count_x_w )
{
	blitter.x = data;
}

WRITE16_HANDLER( atarist_blitter_count_y_w )
{
	blitter.y = data;
}

WRITE16_HANDLER( atarist_blitter_op_w )
{
	if (mem_mask == 0xff00)
	{
		blitter.hop = data >> 8;
	}
	else
	{
		blitter.lop = data;
	}
}

WRITE16_HANDLER( atarist_blitter_ctrl_w )
{
	if (mem_mask == 0xff00)
	{
		blitter.ctrl = data >> 8;
	}
	else
	{
		blitter.skew = data;
	}
}

/* Video */

VIDEO_START( atarist )
{
	atarist_shifter_timer = mame_timer_alloc(atarist_shifter_tick);
	atarist_glue_timer = mame_timer_alloc(atarist_glue_tick);

	mame_timer_adjust(atarist_glue_timer, video_screen_get_time_until_pos(0,0,4), 0, MAME_TIME_IN_HZ(Y2/16)); // 500 ns
	mame_timer_adjust(atarist_shifter_timer, video_screen_get_time_until_pos(0,0,0), 0, MAME_TIME_IN_HZ(Y2/4)); // 125 ns

	atarist_bitmap = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, Machine->screen[0].format);

	memset(&shifter, 0, sizeof(shifter));

	state_save_register_global(shifter.base);
	state_save_register_global(shifter.ofs);
	state_save_register_global(shifter.sync);
	state_save_register_global(shifter.mode);
	state_save_register_global_array(shifter.palette);
	state_save_register_global(shifter.lineofs);
	state_save_register_global(shifter.pixelofs);
	state_save_register_global_array(shifter.rr);
	state_save_register_global_array(shifter.ir);
	state_save_register_global(shifter.bitplane);
	state_save_register_global(shifter.shift);
	state_save_register_global(shifter.h);
	state_save_register_global(shifter.v);

	state_save_register_global_array(blitter.halftone);
	state_save_register_global(blitter.src_inc_x);
	state_save_register_global(blitter.src_inc_y);
	state_save_register_global(blitter.dst_inc_x);
	state_save_register_global(blitter.dst_inc_y);
	state_save_register_global(blitter.src);
	state_save_register_global(blitter.dst);
	state_save_register_global(blitter.endmask_l);
	state_save_register_global(blitter.endmask_m);
	state_save_register_global(blitter.endmask_r);
	state_save_register_global(blitter.x);
	state_save_register_global(blitter.y);
	state_save_register_global(blitter.hop);
	state_save_register_global(blitter.lop);
	state_save_register_global(blitter.ctrl);
	state_save_register_global(blitter.skew);

	state_save_register_bitmap("video", 0, "atarist_bitmap", atarist_bitmap);
}

VIDEO_UPDATE( atarist )
{
	copybitmap(bitmap, atarist_bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);

	return 0;
}
