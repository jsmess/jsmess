/***************************************************************************
	zx.c

    video hardware
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	The ZX has a very unorthodox video RAM system.  To start a scanline,
	the CPU must jump to video RAM at 0xC000, which is a mirror of the
	RAM at 0x4000.  The video chip (ULA?) pulls a switcharoo and changes
	the video bytes as they are fetched by the CPU.

	The video chip draws the scanline until a HALT instruction (0x76) is
	reached, which indicates no further video RAM for this scanline.  Any
	other video byte is used to generate a tile and at the same time, 
	appears to the CPU as a NOP (0x00) instruction.

****************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "cpu/z80/z80.h"
#include "includes/zx.h"
#include "sound/dac.h"

mame_timer *ula_nmi = NULL;
mame_timer *ula_irq = NULL;
int ula_nmi_active, ula_irq_active;
int ula_frame_vsync = 0;
int ula_scancode_count = 0;
int ula_scanline_count = 0;
int old_x = 0;
int old_y = 0;
int old_c = 0;

/*
 * Toggle the video output between black and white.
 * This happens whenever the ULA scanline IRQs are enabled/disabled.
 * Normally this is done during the synchronized zx_ula_r() function,
 * which outputs 8 pixels per code, but if the video sync is off
 * (during tape IO or sound output) zx_ula_bkgnd() is used to
 * simulate the display of a ZX80/ZX81.
 */
void zx_ula_bkgnd(int color)
{
	if (ula_frame_vsync == 0 && color != old_c)
	{
		int y, new_x, new_y;
		rectangle r;
		mame_bitmap *bitmap = tmpbitmap;

		new_y = video_screen_get_vpos(0);
		new_x = video_screen_get_hpos(0);
/*		logerror("zx_ula_bkgnd: %3d,%3d - %3d,%3d\n", old_x, old_y, new_x, new_y);*/
		y = old_y;
		for (;;)
		{
			if (y == new_y)
			{
				r.min_x = old_x;
				r.max_x = new_x;
				r.min_y = r.max_y = y;
				fillbitmap(bitmap, Machine->pens[color], &r);
				break;
			}
			else
			{
				r.min_x = old_x;
				r.max_x = Machine->screen[0].visarea.max_x;
				r.min_y = r.max_y = y;
				fillbitmap(bitmap, Machine->pens[color], &r);
				old_x = 0;
			}
			if (++y == Machine->screen[0].height)
				y = 0;
		}
		old_x = (new_x + 1) % Machine->screen[0].width;
		old_y = new_y;
		old_c = color;
		DAC_data_w(0, color ? 255 : 0);
	}
}

/*
 * PAL:  310 total lines,
 *			  0.. 55 vblank
 *			 56..247 192 visible lines
 *			248..303 vblank
 *			304...	 vsync
 * NTSC: 262 total lines
 *			  0.. 31 vblank
 *			 32..223 192 visible lines
 *			224..233 vblank
 */
static void zx_ula_nmi(int param)
{
	/*
	 * An NMI is issued on the ZX81 every 64us for the blanked
	 * scanlines at the top and bottom of the display.
	 */
	rectangle r = Machine->screen[0].visarea;
	mame_bitmap *bitmap = tmpbitmap;

	r.min_y = r.max_y = video_screen_get_vpos(0);
	fillbitmap(bitmap, Machine->pens[1], &r);
	logerror("ULA %3d[%d] NMI, R:$%02X, $%04x\n", video_screen_get_vpos(0), ula_scancode_count, (unsigned) cpunum_get_reg(0, Z80_R), (unsigned) cpunum_get_reg(0, Z80_PC));
	cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	if (++ula_scanline_count == Machine->screen[0].height)
		ula_scanline_count = 0;
}

static void zx_ula_irq(int param)
{
	/*
	 * An IRQ is issued on the ZX80/81 whenever the R registers
	 * bit 6 goes low. In MESS this IRQ timed from the first read
	 * from the copy of the DFILE in the upper 32K in zx_ula_r().
	 */
	if (ula_irq_active)
	{
		logerror("ULA %3d[%d] IRQ, R:$%02X, $%04x\n", video_screen_get_vpos(0), ula_scancode_count, (unsigned) cpunum_get_reg(0, Z80_R), (unsigned) cpunum_get_reg(0, Z80_PC));

		ula_irq_active = 0;
		if (++ula_scancode_count == 8)
			ula_scancode_count = 0;
		cpunum_set_input_line(0, 0, HOLD_LINE);
		if (++ula_scanline_count == Machine->screen[0].height)
			ula_scanline_count = 0;
	}
}

int zx_ula_r(int offs, int region)
{
	mame_bitmap *bitmap = tmpbitmap;
	int x, y, chr, data, ireg, rreg, cycles, offs0 = offs, halted = 0;
	UINT8 *chrgen, *rom = memory_region(REGION_CPU1);
	UINT16 *scanline;

	if (!ula_irq_active)
	{
		ula_frame_vsync = 3;

		chrgen = memory_region(region);
		ireg = cpunum_get_reg(0, Z80_I) << 8;
		rreg = cpunum_get_reg(0, Z80_R);
		y = video_screen_get_vpos(0);

		cycles = 4 * (64 - (rreg & 63));
		timer_set(TIME_IN_CYCLES(cycles, 0), 0, zx_ula_irq);
		ula_irq_active = 1;
		scanline = BITMAP_ADDR16(bitmap, y, 0);

		for (x = 0; x < 256; x += 8)
		{
			chr = rom[offs & 0x7fff];
/*			if (!halted)
				logerror("ULA %3d[%d] VID, R:$%02X, PC:$%04x, CHR:%02x\n", y, ula_scancode_count, rreg, offs & 0x7fff, chr);*/
			if (chr & 0x40)
			{
				halted = 1;
				rom[offs] = chr;
				data = 0x00;
			}
			else
			{
				data = chrgen[ireg | ((chr & 0x3f) << 3) | ula_scancode_count];
				rom[offs] = 0x00;
				if (chr & 0x80)
					data ^= 0xff;
				offs++;
			}

			scanline[x + 0] = (data >> 7) & 1;
			scanline[x + 1] = (data >> 6) & 1;
			scanline[x + 2] = (data >> 5) & 1;
			scanline[x + 3] = (data >> 4) & 1;
			scanline[x + 4] = (data >> 3) & 1;
			scanline[x + 5] = (data >> 2) & 1;
			scanline[x + 6] = (data >> 1) & 1;
			scanline[x + 7] = (data >> 0) & 1;
		}
	}

	return rom[offs0];
}

VIDEO_START( zx )
{
	ula_nmi = timer_alloc(zx_ula_nmi);
	ula_irq_active = 0;
	video_start_generic_bitmapped(machine);
}

VIDEO_EOF( zx )
{
	/* decrement video synchronization counter */
	if (ula_frame_vsync)
		--ula_frame_vsync;
}
