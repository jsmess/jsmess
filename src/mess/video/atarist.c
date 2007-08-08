/*

	TODO:

	- lineofs
	- pixelofs
	- monochrome mode
	- change screen modes on the fly
	- blitter

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
	UINT32 counter;
	UINT8 sync;
	UINT8 mode;
	UINT16 palette[16];
	UINT8 lineofs;  // STe
	UINT8 pixelofs; // STe
	UINT16 rr[4];
	UINT16 ir[4];
	int vcount;
	int hcount;
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
//		logerror("y %u x %u color %u mode %u\n", y, x, color, shifter.mode);
			
		*BITMAP_ADDR16(atarist_bitmap, y, x) = Machine->pens[color];

		shifter.rr[0] <<= 1;
		shifter.rr[1] <<= 1;
		shifter.rr[2] <<= 1;
		shifter.rr[3] <<= 1;
		break;

	case 1: // 640 x 200, 2 Plane
		color = (BIT(shifter.rr[1], 15) << 1) | BIT(shifter.rr[0], 15);
//		logerror("y %u x %u color %u mode %u\n", y, x, color, shifter.mode);

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
//		logerror("y %u x %u color %u mode %u\n", y, x, color, shifter.mode);
		// unimplemented
		break;
	}
}

static TIMER_CALLBACK(atarist_glue_tick)
{
	int de;
	UINT8 *RAM = memory_region(REGION_CPU1) + shifter.base + shifter.counter;

	shifter.vcount = video_screen_get_vpos(0);

	if (shifter.sync & 0x02)
	{
		// 50 Hz

		switch (shifter.hcount)
		{
		case ATARIST_HBDEND_PAL:
			shifter.h = ASSERT_LINE;
			break;
		case ATARIST_HBDSTART_PAL:
			shifter.h = CLEAR_LINE;
			break;
		case ATARIST_HBSTART_PAL/4:
			cpunum_set_input_line(0, MC68000_IRQ_2, ASSERT_LINE);
			shifter.hcount = 0;
			break;
		}

		switch (shifter.vcount)
		{
		case ATARIST_VBSTART_PAL:
			cpunum_set_input_line(0, MC68000_IRQ_4, ASSERT_LINE);
			shifter.counter = 0;
			break;
		case ATARIST_VBDEND_PAL:
			shifter.v = ASSERT_LINE;
			break;
		case ATARIST_VBDSTART_PAL:
			shifter.v = CLEAR_LINE;
			break;
		}
	}
	else
	{
		// 60 Hz
		switch (shifter.hcount)
		{
		case ATARIST_HBDEND_NTSC:
			shifter.h = ASSERT_LINE;
			break;
		case ATARIST_HBDSTART_NTSC:
			shifter.h = CLEAR_LINE;
			break;
		case ATARIST_HBSTART_NTSC/4:
			cpunum_set_input_line(0, MC68000_IRQ_2, ASSERT_LINE);
			shifter.hcount = 0;
			break;
		}

		switch (shifter.vcount)
		{
		case ATARIST_VBSTART_NTSC:
			shifter.counter = 0;
			cpunum_set_input_line(0, MC68000_IRQ_4, ASSERT_LINE);
			break;
		case ATARIST_VBDEND_NTSC:
			shifter.v = ASSERT_LINE;
			break;
		case ATARIST_VBDSTART_NTSC:
			shifter.v = CLEAR_LINE;
			break;
		}
	}

	de = shifter.h && shifter.v;

	mfp68901_tbi_w(0, de);

//	logerror("shifter base %x\n", shifter.base);
//	logerror("shifter counter %x\n", shifter.counter);
//	logerror("shifter bitplane %x\n", shifter.bitplane);
	shifter.ir[shifter.bitplane] = (RAM[0] << 8) | RAM[1];
	shifter.counter += 2;
	shifter.bitplane++;
	
	if (shifter.bitplane == 4)
	{
		if (de)
		{
			shifter.rr[0] = shifter.ir[0];
			shifter.rr[1] = shifter.ir[1];
			shifter.rr[2] = shifter.ir[2];
			shifter.rr[3] = shifter.ir[3];

			shifter.ir[0] = shifter.ir[1] = shifter.ir[2] = shifter.ir[3] = 0;
		}

		shifter.bitplane = 0;
	}

	shifter.hcount++;
}

READ16_HANDLER( atarist_shifter_r )
{
	switch (offset)
	{
	case 0x00:
		return (shifter.base >> 16) & 0xff;
	case 0x02:
		return (shifter.base >> 8) & 0xff;

	case 0x04:
		return (shifter.counter >> 16) & 0xff;
	case 0x06:
		return (shifter.counter >> 8) & 0xff;
	case 0x08:
		return shifter.counter & 0xff;

	case 0x0a:
		return shifter.sync;
	case 0x60:
		return shifter.mode << 8;

	default:
		if ((offset & 0xe0) == 0x40)
		{
			return shifter.palette[offset & 0x1f];
		}
	}

	return 0;
}

WRITE16_HANDLER( atarist_shifter_w )
{
	switch (offset)
	{
	case 0x00:
		shifter.base = (shifter.base & 0x00ff00) | (data & 0x3f) << 16;
		logerror("shifter base %x\n", shifter.base);
		break;
	case 0x02:
		shifter.base = (shifter.base & 0xff0000) | (data & 0xff) << 8;
		logerror("shifter base %x\n", shifter.base);
		break;

	case 0x0a:
		shifter.sync = data >> 8;
		logerror("shifter sync %x\n", shifter.sync);
		break;

	case 0x60:
		shifter.mode = data >> 8;
		logerror("shifter mode %x\n", shifter.mode);
		break;

	default:
		if ((offset & 0xe0) == 0x40)
		{
			int i = offset & 0x1f;
			shifter.palette[i] = data;
			logerror("shifter base %x\n", shifter.palette[i]);
			palette_set_color_rgb(Machine, offset, pal3bit(data >> 8), pal3bit(data >> 4), pal2bit(data >> 0));
		}
	}
}

READ16_HANDLER( atariste_shifter_r )
{
	switch (offset)
	{
	case 0x00:
		return (shifter.base >> 16) & 0x3f;
	case 0x02:
		return (shifter.base >> 8) & 0xff;
	case 0x0c:
		return shifter.base & 0xfe;

	case 0x04:
		return (shifter.counter >> 16) & 0x3f;
	case 0x06:
		return (shifter.counter >> 8) & 0xff;
	case 0x08:
		return shifter.counter & 0xfe;

	case 0x0a:
		return shifter.sync << 8;
	case 0x60:
		return shifter.mode << 8;

	case 0x0e:
		return shifter.lineofs << 8;
	case 0x64:
		return shifter.pixelofs << 8;

	default:
		if ((offset & 0xe0) == 0x40)
		{
			return shifter.palette[offset & 0x1f];
		}
	}

	return 0;
}

WRITE16_HANDLER( atariste_shifter_w )
{
	switch (offset)
	{
	case 0x00:
		shifter.base = (shifter.base & 0x00ff00) | (data & 0x3f) << 16;
		break;
	case 0x02:
		shifter.base = (shifter.base & 0xff0000) | (data & 0xff) << 8;
		break;
	case 0x0c:
		shifter.base = (shifter.base & 0xffff00) | (data & 0xfe) << 8;
		break;

	case 0x04:
		shifter.counter = (shifter.counter & 0x00ffff) | (data & 0x3f) << 16;
		break;
	case 0x06:
		shifter.counter = (shifter.counter & 0xff00ff) | (data & 0xff) << 8;
		break;
	case 0x08:
		shifter.counter = (shifter.counter & 0xffff00) | (data & 0xfe);
		break;

	case 0x0a:
		shifter.sync = data >> 8;
		break;
	case 0x60:
		shifter.mode = data >> 8;
		break;

	case 0x0e:
		shifter.lineofs = data & 0xff;
		break;
	case 0x64:
		shifter.pixelofs = data & 0x0f;
		break;

	default:
		if ((offset & 0xe0) == 0x40)
		{
			int i = offset & 0x1f;
			int r = ((data >> 7) & 0x0e) | ((data >> 11) & 0x01);
			int g = ((data >> 3) & 0x0e) | ((data >> 4) & 0x01);
			int b = ((data << 1) & 0x0e) | ((data >> 3) & 0x01);
			shifter.palette[i] = data;
			palette_set_color_rgb(Machine, i, r, g, b);
		}
	}
}

VIDEO_START( atarist )
{
	atarist_glue_timer = mame_timer_alloc(atarist_glue_tick);
	atarist_shifter_timer = mame_timer_alloc(atarist_shifter_tick);

	mame_timer_adjust(atarist_glue_timer, time_zero, 0, MAME_TIME_IN_NSEC(500));
	mame_timer_adjust(atarist_shifter_timer, time_zero, 0, MAME_TIME_IN_NSEC(125));

	atarist_bitmap = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, Machine->screen[0].format);

	memset(&shifter, 0, sizeof(shifter));
}

VIDEO_UPDATE( atarist )
{
	copybitmap(bitmap, atarist_bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);

	return 0;
}
