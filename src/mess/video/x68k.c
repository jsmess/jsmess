/*

    Sharp X68000 video functions
    driver by Barry Rodewald

    X68000 video hardware (there are some minor revisions to these custom chips across various X680x0 models):
        Custom sprite controller "Cynthia"
        Custom CRT controller "Vinas / Vicon"
        Custom video controller "VSOP / VIPS"
        Custom video data selector "Cathy"

    In general terms:
        1 "Text" layer - effectively a 4bpp bitmap split into 4 planes at 1bpp each
                         512kB "text" VRAM
                         can write to multiple planes at once
                         can copy one character line to another character line
                         is 1024x1024 in size
        Up to 4 graphic layers - can be 4 layers with a 16 colour palette, 2 layers with a 256 colour palette,
                                 or 1 layer at 16-bit RGB.
                                 512k graphic VRAM
                                 all layers are 512x512, but at 16 colours, the 4 layers can be combined into 1 1024x1024 layer
                                 one or more layers can be cleared at once quickly with a simple hardware function
         2 tilemapped layers - can be 8x8 or 16x16, 16 colours per tile, max 256 colours overall
         1 sprite layer - up to 128 16x16 sprites, 16 colours per sprite, maximum 16 sprites per scanline (not yet implemented).

*/

#include "emu.h"
#include "includes/x68k.h"
#include "machine/68901mfp.h"
#include "devices/messram.h"

UINT16* x68k_gvram;  // Graphic VRAM
UINT16* x68k_tvram;  // Text VRAM
UINT16* x68k_spriteram;  // sprite/background RAM
UINT16* x68k_spritereg;  // sprite/background registers

static bitmap_t* x68k_text_bitmap;  // 1024x1024 4x1bpp planes text
static bitmap_t* x68k_gfx_big_bitmap;  // 16 colour, 1024x1024, 1 page
static bitmap_t* x68k_gfx_0_bitmap_16;  // 16 colour, 512x512, 4 pages
static bitmap_t* x68k_gfx_1_bitmap_16;
static bitmap_t* x68k_gfx_2_bitmap_16;
static bitmap_t* x68k_gfx_3_bitmap_16;
static bitmap_t* x68k_gfx_0_bitmap_256;  // 256 colour, 512x512, 2 pages
static bitmap_t* x68k_gfx_1_bitmap_256;
static bitmap_t* x68k_gfx_0_bitmap_65536;  // 65536 colour, 512x512, 1 page

static tilemap_t* x68k_bg0_8;  // two 64x64 tilemaps, 8x8 characters
static tilemap_t* x68k_bg1_8;
static tilemap_t* x68k_bg0_16;  // two 64x64 tilemaps, 16x16 characters
static tilemap_t* x68k_bg1_16;

static int sprite_shift;

static void x68k_crtc_refresh_mode(running_machine *machine);

INLINE void x68k_plot_pixel(bitmap_t *bitmap, int x, int y, UINT32 color)
{
	*BITMAP_ADDR16(bitmap, y, x) = (UINT16)color;
}

static bitmap_t* x68k_get_gfx_page(int pri,int type)
{
	if(type == GFX16)
	{
		switch(pri)
		{
		case 0:
			return x68k_gfx_0_bitmap_16;
		case 1:
			return x68k_gfx_1_bitmap_16;
		case 2:
			return x68k_gfx_2_bitmap_16;
		case 3:
			return x68k_gfx_3_bitmap_16;
		default:
			return x68k_gfx_0_bitmap_16;  // should never reach here.
		}
	}
	if(type == GFX256)
	{
		switch(pri)
		{
		case 0:
		case 1:
			return x68k_gfx_0_bitmap_256;
		case 2:
		case 3:
			return x68k_gfx_1_bitmap_256;
		default:
			return x68k_gfx_0_bitmap_256;  // should never reach here.
		}
	}
	if(type == GFX65536)
		return x68k_gfx_0_bitmap_65536;

	return NULL;  // should never reach here either.
}

static void x68k_crtc_text_copy(int src, int dest)
{
	// copys one raster in T-VRAM to another raster
	int src_ram = src * 256;  // 128 bytes per scanline
	int dest_ram = dest * 256;
	int line;

	if(dest > 250)
		return;  // for some reason, Salamander causes a SIGSEGV in a debug build in this function.

	for(line=0;line<8;line++)
	{
		// update RAM in each plane
		memcpy(x68k_tvram+dest_ram,x68k_tvram+src_ram,128);
		memcpy(x68k_tvram+dest_ram+0x10000,x68k_tvram+src_ram+0x10000,128);
		memcpy(x68k_tvram+dest_ram+0x20000,x68k_tvram+src_ram+0x20000,128);
		memcpy(x68k_tvram+dest_ram+0x30000,x68k_tvram+src_ram+0x30000,128);

		src_ram+=64;
		dest_ram+=64;
	}

}

static TIMER_CALLBACK(x68k_crtc_operation_end)
{
	int bit = param;
	x68k_sys.crtc.operation &= ~bit;
}

static void x68k_crtc_refresh_mode(running_machine *machine)
{
//  rectangle rect;
//  double scantime;
	rectangle scr,visiblescr;
	int length;

	// Calculate data from register values
	x68k_sys.crtc.vmultiple = 1;
	if((x68k_sys.crtc.reg[20] & 0x10) != 0 && (x68k_sys.crtc.reg[20] & 0x0c) == 0)
		x68k_sys.crtc.vmultiple = 2;  // 31.5kHz + 256 lines = doublescan
	x68k_sys.crtc.htotal = (x68k_sys.crtc.reg[0] + 1) * 8;
	x68k_sys.crtc.vtotal = (x68k_sys.crtc.reg[4] + 1) / x68k_sys.crtc.vmultiple; // default is 567 (568 scanlines)
	x68k_sys.crtc.hbegin = (x68k_sys.crtc.reg[2] * 8) + 1;
	x68k_sys.crtc.hend = (x68k_sys.crtc.reg[3] * 8);
	x68k_sys.crtc.vbegin = (x68k_sys.crtc.reg[6]) / x68k_sys.crtc.vmultiple;
	x68k_sys.crtc.vend = (x68k_sys.crtc.reg[7] - 1) / x68k_sys.crtc.vmultiple;
	x68k_sys.crtc.hsync_end = (x68k_sys.crtc.reg[1]) * 8;
	x68k_sys.crtc.vsync_end = (x68k_sys.crtc.reg[5]) / x68k_sys.crtc.vmultiple;
	x68k_sys.crtc.hsyncadjust = x68k_sys.crtc.reg[8];
	scr.min_x = scr.min_y = 0;
	scr.max_x = x68k_sys.crtc.htotal - 8;
	scr.max_y = x68k_sys.crtc.vtotal;
	if(scr.max_y <= x68k_sys.crtc.vend)
		scr.max_y = x68k_sys.crtc.vend + 2;
	if(scr.max_x <= x68k_sys.crtc.hend)
		scr.max_x = x68k_sys.crtc.hend + 2;
	visiblescr.min_x = x68k_sys.crtc.hbegin;
	visiblescr.max_x = x68k_sys.crtc.hend;
	visiblescr.min_y = x68k_sys.crtc.vbegin;
	visiblescr.max_y = x68k_sys.crtc.vend;

	// expand visible area to the size indicated by CRTC reg 20
	length = x68k_sys.crtc.hend - x68k_sys.crtc.hbegin;
	if (length < x68k_sys.crtc.width)
	{
		visiblescr.min_x = x68k_sys.crtc.hbegin - ((x68k_sys.crtc.width - length)/2);
		visiblescr.max_x = x68k_sys.crtc.hend + ((x68k_sys.crtc.width - length)/2);
	}
	length = x68k_sys.crtc.vend - x68k_sys.crtc.vbegin;
	if (length < x68k_sys.crtc.height)
	{
		visiblescr.min_y = x68k_sys.crtc.vbegin - ((x68k_sys.crtc.height - length)/2);
		visiblescr.max_y = x68k_sys.crtc.vend + ((x68k_sys.crtc.height - length)/2);
	}
	// bounds check
	if(visiblescr.min_x < 0)
		visiblescr.min_x = 0;
	if(visiblescr.min_y < 0)
		visiblescr.min_y = 0;
	if(visiblescr.max_x >= scr.max_x)
		visiblescr.max_x = scr.max_x - 2;
	if(visiblescr.max_y >= scr.max_y - 1)
		visiblescr.max_y = scr.max_y - 2;

//  logerror("CRTC regs - %i %i %i %i  - %i %i %i %i - %i - %i\n",x68k_sys.crtc.reg[0],x68k_sys.crtc.reg[1],x68k_sys.crtc.reg[2],x68k_sys.crtc.reg[3],
//      x68k_sys.crtc.reg[4],x68k_sys.crtc.reg[5],x68k_sys.crtc.reg[6],x68k_sys.crtc.reg[7],x68k_sys.crtc.reg[8],x68k_sys.crtc.reg[9]);
	logerror("video_screen_configure(machine->primary_screen,%i,%i,[%i,%i,%i,%i],55.45)\n",scr.max_x,scr.max_y,visiblescr.min_x,visiblescr.min_y,visiblescr.max_x,visiblescr.max_y);
	machine->primary_screen->configure(scr.max_x,scr.max_y,visiblescr,HZ_TO_ATTOSECONDS(55.45));
}

TIMER_CALLBACK(x68k_hsync)
{
	int state = param;
	attotime hsync_time;

	x68k_sys.crtc.hblank = state;
	if(x68k_sys.crtc.vmultiple == 2) // 256-line (doublescan)
	{
		static int oddscanline;
		if(state == 1)
		{
			if(oddscanline == 1)
			{
				int scan = machine->primary_screen->vpos();
				if(scan > x68k_sys.crtc.vend)
					scan = x68k_sys.crtc.vbegin;
				hsync_time = machine->primary_screen->time_until_pos(scan,(x68k_sys.crtc.htotal + x68k_sys.crtc.hend) / 2);
				timer_adjust_oneshot(x68k_scanline_timer, hsync_time, 0);
				if(scan != 0)
				{
					if((input_port_read(machine, "options") & 0x04))
					{
						machine->primary_screen->update_partial(scan);
					}
				}
			}
			else
			{
				int scan = machine->primary_screen->vpos();
				if(scan > x68k_sys.crtc.vend)
					scan = x68k_sys.crtc.vbegin;
				hsync_time = machine->primary_screen->time_until_pos(scan,x68k_sys.crtc.hend / 2);
				timer_adjust_oneshot(x68k_scanline_timer, hsync_time, 0);
				if(scan != 0)
				{
					if((input_port_read(machine, "options") & 0x04))
					{
						machine->primary_screen->update_partial(scan);
					}
				}
			}
		}
		if(state == 0)
		{
			if(oddscanline == 1)
			{
				int scan = machine->primary_screen->vpos();
				if(scan > x68k_sys.crtc.vend)
					scan = x68k_sys.crtc.vbegin;
				else
					scan++;
				hsync_time = machine->primary_screen->time_until_pos(scan,x68k_sys.crtc.hbegin / 2);
				timer_adjust_oneshot(x68k_scanline_timer, hsync_time, 1);
				oddscanline = 0;
			}
			else
			{
				hsync_time = machine->primary_screen->time_until_pos(machine->primary_screen->vpos(),(x68k_sys.crtc.htotal + x68k_sys.crtc.hbegin) / 2);
				timer_adjust_oneshot(x68k_scanline_timer, hsync_time, 1);
				oddscanline = 1;
			}
		}
	}
	else  // 512-line
	{
		if(state == 1)
		{
			int scan = machine->primary_screen->vpos();
			if(scan > x68k_sys.crtc.vend)
				scan = 0;
			hsync_time = machine->primary_screen->time_until_pos(scan,x68k_sys.crtc.hend);
			timer_adjust_oneshot(x68k_scanline_timer, hsync_time, 0);
			if(scan != 0)
			{
				if((input_port_read(machine, "options") & 0x04))
				{
					machine->primary_screen->update_partial(scan);
				}
			}
		}
		if(state == 0)
		{
			hsync_time = machine->primary_screen->time_until_pos(machine->primary_screen->vpos()+1,x68k_sys.crtc.hbegin);
			timer_adjust_oneshot(x68k_scanline_timer, hsync_time, 1);
	//      if(!(x68k_sys.mfp.gpio & 0x40))  // if GPIP6 is active, clear it
	//          x68k_sys.mfp.gpio |= 0x40;
		}
	}
}

static TIMER_CALLBACK(x68k_crtc_raster_end)
{
	x68k_sys.mfp.gpio |= 0x40;
}

TIMER_CALLBACK(x68k_crtc_raster_irq)
{
	int scan = param;
	attotime irq_time;
	attotime end_time;

	if(scan <= x68k_sys.crtc.vtotal)
	{
		x68k_sys.mfp.gpio &= ~0x40;  // GPIP6
		machine->primary_screen->update_partial(scan);
		irq_time = machine->primary_screen->time_until_pos(scan,2);
		// end of HBlank period clears GPIP6 also?
		end_time = machine->primary_screen->time_until_pos(scan,x68k_sys.crtc.hbegin);
		timer_adjust_oneshot(x68k_raster_irq, irq_time, scan);
		timer_set(machine, end_time,NULL,0,x68k_crtc_raster_end);
		logerror("GPIP6: Raster triggered at line %i (%i)\n",scan,machine->primary_screen->vpos());
	}
}

TIMER_CALLBACK(x68k_crtc_vblank_irq)
{
	running_device *x68k_mfp = devtag_get_device(machine, MC68901_TAG);
	int val = param;
	attotime irq_time;
	int vblank_line;

	if(val == 1)  // VBlank on
	{
		x68k_sys.crtc.vblank = 1;
		vblank_line = x68k_sys.crtc.vbegin;
		irq_time = machine->primary_screen->time_until_pos(vblank_line,2);
		timer_adjust_oneshot(x68k_vblank_irq, irq_time, 0);
		logerror("CRTC: VBlank on\n");
	}
	if(val == 0)  // VBlank off
	{
		x68k_sys.crtc.vblank = 0;
		vblank_line = x68k_sys.crtc.vend;
		if(vblank_line > x68k_sys.crtc.vtotal)
			vblank_line = x68k_sys.crtc.vtotal;
		irq_time = machine->primary_screen->time_until_pos(vblank_line,2);
		timer_adjust_oneshot(x68k_vblank_irq, irq_time, 1);
		logerror("CRTC: VBlank off\n");
	}

	if (x68k_mfp != NULL)
		mc68901_tai_w(x68k_mfp, !x68k_sys.crtc.vblank);
}


// CRTC "VINAS 1+2 / VICON" at 0xe80000
/* 0xe80000 - Registers (all are 16-bit):
 * 0 - Horizontal Total (in characters)
 * 1 - Horizontal Sync End
 * 2 - Horizontal Display Begin
 * 3 - Horizontal Display End
 * 4 - Vertical Total (in scanlines)
 * 5 - Vertical Sync End
 * 6 - Vertical Display Begin
 * 7 - Vertical Display End
 * 8 - Fine Horizontal Sync Adjustment
 * 9 - Raster Line (for Raster IRQ mapped to MFP GPIP6)
 * 10/11 - Text Layer X and Y Scroll
 * 12/13 - Graphic Layer 0 X and Y Scroll
 * 14/15 - Graphic Layer 1 X and Y Scroll
 * 16/17 - Graphic Layer 2 X and Y Scroll
 * 18/19 - Graphic Layer 3 X and Y Scroll
 * 20 - bit 12 - Text VRAM mode : 0 = display, 1 = buffer
 *      bit 11 - Graphic VRAM mode : 0 = display, 1 = buffer
 *      bit 10 - "Real" screen size : 0 = 512x512, 1 = 1024x1024
 *      bits 8,9 - Colour mode :
 *                 00 = 16 colour      01 = 256 colour
 *                 10 = Undefined      11 = 65,536 colour
 *      bit 4 - Horizontal Frequency : 0 = 15.98kHz, 1 = 31.50kHz
 *      bits 2,3 - Vertical dots :
 *                 00 = 256            01 = 512
 *                 10 or 11 = 1024 (interlaced)
 *      bits 0,1 - Horizontal dots :
 *                 00 = 256            01 = 512
 *                 10 = 768            11 = 50MHz clock mode (Compact XVI or later)
 * 21 - bit 9 - Text Screen Access Mask Enable
 *      bit 8 - Text Screen Simultaneous Plane Access Enable
 *      bits 4-7 - Text Screen Simultaneous Plane Access Select
 *      bits 0-3 - Text Screen Line Copy Plane Select
 *                 Graphic Screen High-speed Clear Page Select
 * 22 - Text Screen Line Copy
 *      bits 15-8 - Source Line
 *      bits 7-0 - Destination Line
 * 23 - Text Screen Mask Pattern
 *
 * 0xe80481 - Operation Port (8-bit):
 *      bit 3 - Text Screen Line Copy Begin
 *      bit 1 - Graphic Screen High-speed Clear Begin
 *      bit 0 - Image Taking Begin (?)
 *    Operation Port bits are cleared automatically when the requested
 *    operation is completed.
 */
WRITE16_HANDLER( x68k_crtc_w )
{
	COMBINE_DATA(x68k_sys.crtc.reg+offset);
	switch(offset)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		x68k_crtc_refresh_mode(space->machine);
		break;
	case 9:  // CRTC raster IRQ (GPIP6)
		{
			attotime irq_time;
			irq_time = space->machine->primary_screen->time_until_pos((data) / x68k_sys.crtc.vmultiple,2);

			if(attotime_to_double(irq_time) > 0)
				timer_adjust_oneshot(x68k_raster_irq, irq_time, (data) / x68k_sys.crtc.vmultiple);
		}
		logerror("CRTC: Write to raster IRQ register - %i\n",data);
		break;
	case 20:
		if(ACCESSING_BITS_0_7)
		{
			switch(data & 0x0c)
			{
			case 0x00:
				x68k_sys.crtc.height = 256;
				break;
			case 0x08:
			case 0x0c:  // TODO: 1024 vertical, if horizontal freq = 31kHz
				x68k_sys.crtc.height = 512;
				break;
			case 0x04:
				x68k_sys.crtc.height = 512;
				break;
			}
			switch(data & 0x03)
			{
			case 0x00:
				x68k_sys.crtc.width = 256;
				break;
			case 0x01:
				x68k_sys.crtc.width = 512;
				break;
			case 0x02:
			case 0x03:  // 0x03 = 50MHz clock mode (XVI only)
				x68k_sys.crtc.width = 768;
				break;
			}
		}
		if(ACCESSING_BITS_8_15)
		{
			x68k_sys.crtc.interlace = 0;
			if(data & 0x0400)
				x68k_sys.crtc.interlace = 1;
		}
		x68k_crtc_refresh_mode(space->machine);
		break;
	case 576:  // operation register
		x68k_sys.crtc.operation = data;
		if(data & 0x08)  // text screen raster copy
		{
			x68k_crtc_text_copy((x68k_sys.crtc.reg[22] & 0xff00) >> 8,(x68k_sys.crtc.reg[22] & 0x00ff));
			timer_set(space->machine, ATTOTIME_IN_MSEC(1), NULL, 0x02,x68k_crtc_operation_end);  // time taken to do operation is a complete guess.
		}
		if(data & 0x02)  // high-speed graphic screen clear
		{
			rectangle rect = {0,0,0,0};
			rect.max_x = 512;
			rect.max_y = 512;
			if(x68k_sys.crtc.reg[21] & 0x01)
			{
				bitmap_fill(x68k_gfx_0_bitmap_16,&rect,0);
				bitmap_fill(x68k_gfx_0_bitmap_256,&rect,0);
				bitmap_fill(x68k_gfx_0_bitmap_65536,&rect,0);
				bitmap_fill(x68k_gfx_big_bitmap,&rect,0);
				memset(x68k_gvram,0,0x20000);
			}
			if(x68k_sys.crtc.reg[21] & 0x02)
			{
				bitmap_fill(x68k_gfx_1_bitmap_16,&rect,0);
				bitmap_fill(x68k_gfx_1_bitmap_256,&rect,0);
				memset(x68k_gvram+0x10000,0,0x20000);
			}
			if(x68k_sys.crtc.reg[21] & 0x04)
			{
				bitmap_fill(x68k_gfx_2_bitmap_16,&rect,0);
				memset(x68k_gvram+0x20000,0,0x20000);
			}
			if(x68k_sys.crtc.reg[21] & 0x08)
			{
				bitmap_fill(x68k_gfx_3_bitmap_16,&rect,0);
				memset(x68k_gvram+0x30000,0,0x20000);
			}
			timer_set(space->machine, ATTOTIME_IN_MSEC(10), NULL, 0x02,x68k_crtc_operation_end);  // time taken to do operation is a complete guess.
//          popmessage("CRTC: High-speed gfx screen clear [0x%02x]",x68k_sys.crtc.reg[21] & 0x0f);
		}
		break;
	}
//  logerror("CRTC: [%08x] Wrote %04x to CRTC register %i\n",cpu_get_pc(devtag_get_device(space->machine, "maincpu")),data,offset);
}

READ16_HANDLER( x68k_crtc_r )
{
#if 0
	switch(offset)
	{
	default:
		logerror("CRTC: [%08x] Read from CRTC register %i\n",activecpu_get_pc(),offset);
		return 0xff;
	}
#endif

	if(offset < 24)
	{
//      logerror("CRTC: [%08x] Read %04x from CRTC register %i\n",cpu_get_pc(devtag_get_device(space->machine, "maincpu")),x68k_sys.crtc.reg[offset],offset);
		switch(offset)
		{
		case 9:
			return 0;
		case 10:  // Text X/Y scroll
		case 11:
		case 12:  // Graphic layer 0 scroll
		case 13:
			return x68k_sys.crtc.reg[offset] & 0x3ff;
		case 14:  // Graphic layer 1 scroll
		case 15:
		case 16:  // Graphic layer 2 scroll
		case 17:
		case 18:  // Graphic layer 3 scroll
		case 19:
			return x68k_sys.crtc.reg[offset] & 0x1ff;
		default:
			return x68k_sys.crtc.reg[offset];
		}
	}
	if(offset == 576) // operation port, operation bits are set to 0 when operation is complete
		return x68k_sys.crtc.operation;
//  logerror("CRTC: [%08x] Read from unknown CRTC register %i\n",activecpu_get_pc(),offset);
	return 0xffff;
}

WRITE16_HANDLER( x68k_gvram_w )
{
	int xloc,yloc,pageoffset;
	/*
       G-VRAM usage is determined by colour depth and "real" screen size.

       For screen size of 1024x1024, all G-VRAM space is used, in one big page.
       At 1024x1024 real screen size, colour depth is always 4bpp, and ranges from
       0xc00000-0xdfffff.

       For screen size of 512x512, the colour depth determines the page usage.
       16 colours = 4 pages
       256 colours = 2 pages
       65,536 colours = 1 page
       Page 1 - 0xc00000-0xc7ffff    Page 2 - 0xc80000-0xcfffff
       Page 3 - 0xd00000-0xd7ffff    Page 4 - 0xd80000-0xdfffff
    */

	// handle different G-VRAM page setups
	if(x68k_sys.crtc.reg[20] & 0x08)  // G-VRAM set to buffer
	{
		if(offset < 0x40000)
			COMBINE_DATA(x68k_gvram+offset);
	}
	else
	{
		switch(x68k_sys.crtc.reg[20] & 0x0300)
		{
			case 0x0300:
				if(offset < 0x40000)
					COMBINE_DATA(x68k_gvram+offset);
				break;
			case 0x0100:
				if(offset < 0x40000)
				{
					x68k_gvram[offset] = (x68k_gvram[offset] & 0xff00) | (data & 0x00ff);
				}
				if(offset >= 0x40000 && offset < 0x80000)
				{
					x68k_gvram[offset-0x40000] = (x68k_gvram[offset-0x40000] & 0x00ff) | ((data & 0x00ff) << 8);
				}
				break;
			case 0x0000:
				if(offset < 0x40000)
				{
					x68k_gvram[offset] = (x68k_gvram[offset] & 0xfff0) | (data & 0x000f);
				}
				if(offset >= 0x40000 && offset < 0x80000)
				{
					x68k_gvram[offset-0x40000] = (x68k_gvram[offset-0x40000] & 0xff0f) | ((data & 0x000f) << 4);
				}
				if(offset >= 0x80000 && offset < 0xc0000)
				{
					x68k_gvram[offset-0x80000] = (x68k_gvram[offset-0x80000] & 0xf0ff) | ((data & 0x000f) << 8);
				}
				if(offset >= 0xc0000 && offset < 0x100000)
				{
					x68k_gvram[offset-0xc0000] = (x68k_gvram[offset-0xc0000] & 0x0fff) | ((data & 0x000f) << 12);
				}
				break;
			default:
				logerror("G-VRAM written while layer setup is undefined.\n");
		}
	}

	pageoffset = offset & 0xfffff;
	xloc = pageoffset % 1024;
	yloc = pageoffset / 1024;
	x68k_plot_pixel(x68k_gfx_big_bitmap,xloc,yloc,data & 0x000f);

	pageoffset = offset & 0x3ffff;
	xloc = pageoffset % 512;
	yloc = pageoffset / 512;

	if(offset < 0x40000)  // first page, all colour depths
	{
		x68k_plot_pixel(x68k_gfx_0_bitmap_65536,xloc,yloc,(x68k_gvram[offset] >> 1) + 512);
		x68k_plot_pixel(x68k_gfx_0_bitmap_256,xloc,yloc,data & 0x00ff);
		x68k_plot_pixel(x68k_gfx_0_bitmap_16,xloc,yloc,data & 0x000f);
		if((x68k_sys.crtc.reg[20] & 0x0300) == 0x0300 || (x68k_sys.crtc.reg[20] & 0x0800))
		{  // all 512k is 16-bit wide in 64k colour mode or if GVRAM is set as a buffer
			x68k_plot_pixel(x68k_gfx_1_bitmap_256,xloc,yloc,(data & 0xff00) >> 8);
			x68k_plot_pixel(x68k_gfx_1_bitmap_16,xloc,yloc,(data & 0x00f0) >> 4);
			x68k_plot_pixel(x68k_gfx_2_bitmap_16,xloc,yloc,(data & 0x0f00) >> 8);
			x68k_plot_pixel(x68k_gfx_3_bitmap_16,xloc,yloc,(data & 0xf000) >> 12);
		}
	}
	if(offset >= 0x40000 && offset < 0x80000)  // second page, 16 or 256 colours
	{
		x68k_plot_pixel(x68k_gfx_1_bitmap_256,xloc,yloc,data & 0x00ff);
		x68k_plot_pixel(x68k_gfx_1_bitmap_16,xloc,yloc,data & 0x000f);
	}
	if(offset >= 0x80000 && offset < 0xc0000)  // third page, 16 colours only
	{
		x68k_plot_pixel(x68k_gfx_2_bitmap_16,xloc,yloc,data & 0x000f);
	}
	if(offset >= 0xc0000 && offset < 0x100000)  // fourth page, 16 colours only
	{
		x68k_plot_pixel(x68k_gfx_3_bitmap_16,xloc,yloc,data & 0x000f);
	}
}

WRITE16_HANDLER( x68k_tvram_w )
{
	UINT16 text_mask;

	text_mask = ~(x68k_sys.crtc.reg[23]) & mem_mask;

	if(!(x68k_sys.crtc.reg[21] & 0x0200)) // text access mask enable
		text_mask = 0xffff & mem_mask;

	mem_mask = text_mask;

	if(x68k_sys.crtc.reg[21] & 0x0100)
	{  // simultaneous T-VRAM plane access (I think ;))
		int plane,wr;
		offset = offset & 0x00ffff;
		wr = (x68k_sys.crtc.reg[21] & 0x00f0) >> 4;
		for(plane=0;plane<4;plane++)
		{
			if(wr & (1 << plane))
			{
				COMBINE_DATA(x68k_tvram+offset+(0x10000*plane));
			}
		}
	}
	else
	{
		COMBINE_DATA(x68k_tvram+offset);
	}
}

READ16_HANDLER( x68k_gvram_r )
{
	UINT16 ret = 0;

	if(x68k_sys.crtc.reg[20] & 0x08)  // G-VRAM set to buffer
		return x68k_gvram[offset];

	switch(x68k_sys.crtc.reg[20] & 0x0300)  // colour setup determines G-VRAM use
	{
		case 0x0300: // 65,536 colour (RGB) - 16-bits per word
			if(offset < 0x40000)
				ret = x68k_gvram[offset];
			else
				ret = 0xffff;
			break;
		case 0x0100:  // 256 colour (paletted) - 8 bits per word
			if(offset < 0x40000)
				ret = x68k_gvram[offset] & 0x00ff;
			if(offset >= 0x40000 && offset < 0x80000)
				ret = (x68k_gvram[offset-0x40000] & 0xff00) >> 8;
			if(offset >= 0x80000)
				ret = 0xffff;
			break;
		case 0x0000:  // 16 colour (paletted) - 4 bits per word
			if(offset < 0x40000)
				ret = x68k_gvram[offset] & 0x000f;
			if(offset >= 0x40000 && offset < 0x80000)
				ret = (x68k_gvram[offset-0x40000] & 0x00f0) >> 4;
			if(offset >= 0x80000 && offset < 0xc0000)
				ret = (x68k_gvram[offset-0x80000] & 0x0f00) >> 8;
			if(offset >= 0xc0000 && offset < 0x100000)
				ret = (x68k_gvram[offset-0xc0000] & 0xf000) >> 12;
			break;
		default:
			logerror("G-VRAM read while layer setup is undefined.\n");
			ret = 0xffff;
	}

	return ret;
}

READ16_HANDLER( x68k_tvram_r )
{
	return x68k_tvram[offset];
}

READ32_HANDLER( x68k_tvram32_r )
{
	UINT32 ret = 0;

	if(ACCESSING_BITS_0_15)
		ret |= (x68k_tvram_r(space,(offset*2)+1,0xffff));
	if(ACCESSING_BITS_16_31)
		ret |= x68k_tvram_r(space,offset*2,0xffff) << 16;

	return ret;
}

READ32_HANDLER( x68k_gvram32_r )
{
	UINT32 ret = 0;

	if(ACCESSING_BITS_0_15)
		ret |= x68k_gvram_r(space,offset*2+1,0xffff);
	if(ACCESSING_BITS_16_31)
		ret |= x68k_gvram_r(space,offset*2,0xffff) << 16;

	return ret;
}

WRITE32_HANDLER( x68k_tvram32_w )
{
	if(ACCESSING_BITS_0_7)
		x68k_tvram_w(space,(offset*2)+1,data,0x00ff);
	if(ACCESSING_BITS_8_15)
		x68k_tvram_w(space,(offset*2)+1,data,0xff00);
	if(ACCESSING_BITS_16_23)
		x68k_tvram_w(space,offset*2,data >> 16,0x00ff);
	if(ACCESSING_BITS_24_31)
		x68k_tvram_w(space,offset*2,data >> 16,0xff00);
}

WRITE32_HANDLER( x68k_gvram32_w )
{
	if(ACCESSING_BITS_0_7)
		x68k_gvram_w(space,(offset*2)+1,data,0x00ff);
	if(ACCESSING_BITS_8_15)
		x68k_gvram_w(space,(offset*2)+1,data,0xff00);
	if(ACCESSING_BITS_16_23)
		x68k_gvram_w(space,offset*2,data >> 16,0x00ff);
	if(ACCESSING_BITS_24_31)
		x68k_gvram_w(space,offset*2,data >> 16,0xff00);
}

WRITE16_HANDLER( x68k_spritereg_w )
{
	COMBINE_DATA(x68k_spritereg+offset);
	switch(offset)
	{
	case 0x400:
		tilemap_set_scrollx(x68k_bg0_8,0,(data - x68k_sys.crtc.hbegin) & 0x3ff);
		tilemap_set_scrollx(x68k_bg0_16,0,(data - x68k_sys.crtc.hbegin) & 0x3ff);
		break;
	case 0x401:
		tilemap_set_scrolly(x68k_bg0_8,0,(data - x68k_sys.crtc.vbegin) & 0x3ff);
		tilemap_set_scrolly(x68k_bg0_16,0,(data - x68k_sys.crtc.vbegin) & 0x3ff);
		break;
	case 0x402:
		tilemap_set_scrollx(x68k_bg1_8,0,(data - x68k_sys.crtc.hbegin) & 0x3ff);
		tilemap_set_scrollx(x68k_bg1_16,0,(data - x68k_sys.crtc.hbegin) & 0x3ff);
		break;
	case 0x403:
		tilemap_set_scrolly(x68k_bg1_8,0,(data - x68k_sys.crtc.vbegin) & 0x3ff);
		tilemap_set_scrolly(x68k_bg1_16,0,(data - x68k_sys.crtc.vbegin) & 0x3ff);
		break;
	case 0x405:  // BG H-DISP (like CRTC reg 2)
		if(data != 0x00ff)
		{
			x68k_sys.crtc.bg_visible_width = (x68k_sys.crtc.reg[3] - ((data & 0x003f) - 4)) * 8;
			x68k_sys.crtc.bg_hshift = (x68k_sys.crtc.width - x68k_sys.crtc.bg_visible_width) / 2;
		}
		else
			x68k_sys.crtc.bg_hshift = x68k_sys.crtc.hshift;
		break;
	case 0x406:  // BG V-DISP (like CRTC reg 6)
		x68k_sys.crtc.bg_vshift = x68k_sys.crtc.vshift;
		break;
	}
}

READ16_HANDLER( x68k_spritereg_r )
{
	if(offset >= 0x400 && offset < 0x404)
		return x68k_spritereg[offset] & 0x3ff;
	return x68k_spritereg[offset];
}

WRITE16_HANDLER( x68k_spriteram_w )
{
	COMBINE_DATA(x68k_spriteram+offset);
	x68k_sys.video.tile8_dirty[offset / 16] = 1;
	x68k_sys.video.tile16_dirty[offset / 64] = 1;
	if(offset < 0x2000)
	{
        tilemap_mark_all_tiles_dirty(x68k_bg1_8);
        tilemap_mark_all_tiles_dirty(x68k_bg1_16);
        tilemap_mark_all_tiles_dirty(x68k_bg0_8);
        tilemap_mark_all_tiles_dirty(x68k_bg0_16);
    }
    if(offset >= 0x2000 && offset < 0x3000)
    {
        tilemap_mark_tile_dirty(x68k_bg1_8,offset & 0x0fff);
        tilemap_mark_tile_dirty(x68k_bg1_16,offset & 0x0fff);
    }
    if(offset >= 0x3000)
    {
        tilemap_mark_tile_dirty(x68k_bg0_8,offset & 0x0fff);
        tilemap_mark_tile_dirty(x68k_bg0_16,offset & 0x0fff);
    }
}

READ16_HANDLER( x68k_spriteram_r )
{
	return x68k_spriteram[offset];
}

static void x68k_draw_text(running_machine* machine,bitmap_t* bitmap, int xscr, int yscr, rectangle rect)
{
	unsigned int line,pixel; // location on screen
	UINT32 loc;  // location in TVRAM
	UINT32 colour;
	int bit;

	for(line=rect.min_y;line<=rect.max_y;line++)  // per scanline
	{
		// adjust for scroll registers
		loc = (((line - x68k_sys.crtc.vbegin) + yscr) & 0x3ff) * 64;
		loc += (xscr / 16) & 0x7f;
		loc &= 0xffff;
		bit = 15 - (xscr & 0x0f);
		for(pixel=rect.min_x;pixel<=rect.max_x;pixel++)  // per pixel
		{
			colour = (((x68k_tvram[loc] >> bit) & 0x01) ? 1 : 0)
				+ (((x68k_tvram[loc+0x10000] >> bit) & 0x01) ? 2 : 0)
				+ (((x68k_tvram[loc+0x20000] >> bit) & 0x01) ? 4 : 0)
				+ (((x68k_tvram[loc+0x30000] >> bit) & 0x01) ? 8 : 0);
			if(x68k_sys.video.text_pal[colour] != 0x0000)  // any colour but black
			{
				// Colour 0 is displayable if the text layer is at the priority level 2
				if(colour == 0 && (x68k_sys.video.reg[1] & 0x0c00) == 0x0800)
					*BITMAP_ADDR16(bitmap,line,pixel) = 512 + (x68k_sys.video.text_pal[colour] >> 1);
				else
					if(colour != 0)
						*BITMAP_ADDR16(bitmap,line,pixel) = 512 + (x68k_sys.video.text_pal[colour] >> 1);
			}
			bit--;
			if(bit < 0)
			{
				bit = 15;
				loc++;
				loc &= 0xffff;
			}
		}
	}
}

static void x68k_draw_gfx(bitmap_t* bitmap,rectangle cliprect)
{
	int priority;
	rectangle rect;
	int xscr,yscr;
	int gpage;

	if(x68k_sys.crtc.reg[20] & 0x0800)  // if graphic layers are set to buffer, then they aren't visible
		return;

	for(priority=3;priority>=0;priority--)
	{
		if(x68k_sys.crtc.reg[20] & 0x0400)  // 1024x1024 "real" screen size - use 1024x1024 16-colour gfx layer
		{
			// 16 colour gfx screen
			rect.min_x=x68k_sys.crtc.hshift;
			rect.min_y=x68k_sys.crtc.vshift;
			rect.max_x=rect.min_x + x68k_sys.crtc.visible_width-1;
			rect.max_y=rect.min_y + x68k_sys.crtc.visible_height-1;
			if(x68k_sys.video.reg[2] & 0x0010 && priority == x68k_sys.video.gfxlayer_pri[0])
			{
				xscr = x68k_sys.crtc.hbegin-(x68k_sys.crtc.reg[12]);
				yscr = x68k_sys.crtc.vbegin-(x68k_sys.crtc.reg[13]);
				copyscrollbitmap_trans(bitmap, x68k_gfx_big_bitmap, 1, &xscr, 1, &yscr ,&cliprect,0);
			}
		}
		else  // 512x512 "real" screen size
		{
			switch(x68k_sys.video.reg[0] & 0x03)
			{
			case 0x03:
				// 65,536 colour gfx screen
				if(x68k_sys.video.reg[2] & 0x0001 && priority == x68k_sys.video.gfxlayer_pri[0])
				{
					rect.min_x=x68k_sys.crtc.hshift;
					rect.min_y=x68k_sys.crtc.vshift;
					rect.max_x=rect.min_x + x68k_sys.crtc.visible_width-1;
					rect.max_y=rect.min_y + x68k_sys.crtc.visible_height-1;
					xscr = x68k_sys.crtc.hbegin-(x68k_sys.crtc.reg[12] & 0x1ff);
					yscr = x68k_sys.crtc.vbegin-(x68k_sys.crtc.reg[13] & 0x1ff);
					copyscrollbitmap_trans(bitmap, x68k_gfx_0_bitmap_65536, 1, &xscr, 1, &yscr,&cliprect,0);
				}
				break;
			case 0x01:
				// 256 colour gfx screen
				rect.min_x=x68k_sys.crtc.hshift;
				rect.min_y=x68k_sys.crtc.vshift;
				rect.max_x=rect.min_x + x68k_sys.crtc.visible_width-1;
				rect.max_y=rect.min_y + x68k_sys.crtc.visible_height-1;
				gpage = x68k_sys.video.gfxlayer_pri[priority];
				if(x68k_sys.video.reg[2] & (1 << priority))
				{
					if(gpage == 0 || gpage == 2)  // so that we aren't drawing the same pages twice
					{
						xscr = x68k_sys.crtc.hbegin-(x68k_sys.crtc.reg[12 + (gpage*2)] & 0x1ff);
						yscr = x68k_sys.crtc.vbegin-(x68k_sys.crtc.reg[13 + (gpage*2)] & 0x1ff);
						copyscrollbitmap_trans(bitmap, x68k_get_gfx_page(gpage,GFX256), 1, &xscr, 1, &yscr, &cliprect,0);
					}
				}
				break;
			case 0x00:
				// 16 colour gfx screen
				rect.min_x=x68k_sys.crtc.hshift;
				rect.min_y=x68k_sys.crtc.vshift;
				rect.max_x=rect.min_x + x68k_sys.crtc.visible_width-1;
				rect.max_y=rect.min_y + x68k_sys.crtc.visible_height-1;
				gpage = x68k_sys.video.gfxlayer_pri[priority];
				if(x68k_sys.video.reg[2] & (1 << priority))
				{
					xscr = x68k_sys.crtc.hbegin-(x68k_sys.crtc.reg[12+(gpage*2)] & 0x1ff);
					yscr = x68k_sys.crtc.vbegin-(x68k_sys.crtc.reg[13+(gpage*2)] & 0x1ff);
					copyscrollbitmap_trans(bitmap, x68k_get_gfx_page(gpage,GFX16), 1, &xscr, 1, &yscr ,&cliprect,0);
				}
				break;
			}
		}
	}
}

// Sprite controller "Cynthia" at 0xeb0000
static void x68k_draw_sprites(running_machine *machine, bitmap_t* bitmap, int priority, rectangle cliprect)
{
	/*
       0xeb0000 - 0xeb07ff - Sprite registers (up to 128)
           + 00 : b9-0,  Sprite X position
           + 02 : b9-0,  Sprite Y position
           + 04 : b15,   Vertical Reversing (flipping?)
                  b14,   Horizontal Reversing
                  b11-8, Sprite colour
                  b7-0,  Sprite tile code (in PCG)
           + 06 : b1-0,  Priority
                         00 = Sprite not displayed

       0xeb0800 - BG0 X Scroll  (10-bit)
       0xeb0802 - BG0 Y Scroll
       0xeb0804 - BG1 X Scroll
       0xeb0806 - BG1 Y Scroll
       0xeb0808 - BG control
                  b9,    BG/Sprite display (RAM and register access is faster if 1)
                  b4,    PCG area 1 available
                  b3,    BG1 display enable
                  b1,    PCG area 0 available
                  b0,    BG0 display enable
       0xeb080a - Horizontal total (like CRTC reg 0 - is 0xff if in 256x256?)
       0xeb080c - Horizontal display position (like CRTC reg 2 - +4)
       0xeb080e - Vertical display position (like CRTC reg 6)
       0xeb0810 - Resolution setting
                  b4,    "L/H" (apparently 15kHz/31kHz switch for sprites/BG?)
                  b3-2,  V-Res
                  b1-0,  H-Res (0 = 8x8 tilemaps, 1 = 16x16 tilemaps, 2 or 3 = unknown)
    */
	int ptr,pri;

	for(ptr=508;ptr>=0;ptr-=4)  // stepping through sprites
	{
		pri = x68k_spritereg[ptr+3] & 0x03;
#ifdef MAME_DEBUG
		if(!(input_code_pressed(machine,KEYCODE_I)))
#endif
		if(pri == priority)
		{  // if at the right priority level, draw the sprite
			rectangle rect;
			int code = x68k_spritereg[ptr+2] & 0x00ff;
			int colour = (x68k_spritereg[ptr+2] & 0x0f00) >> 8;
			int xflip = x68k_spritereg[ptr+2] & 0x4000;
			int yflip = x68k_spritereg[ptr+2] & 0x8000;
			int sx = (x68k_spritereg[ptr+0] & 0x3ff) - 16;
			int sy = (x68k_spritereg[ptr+1] & 0x3ff) - 16;

			rect.min_x=x68k_sys.crtc.hshift;
			rect.min_y=x68k_sys.crtc.vshift;
			rect.max_x=rect.min_x + x68k_sys.crtc.visible_width-1;
			rect.max_y=rect.min_y + x68k_sys.crtc.visible_height-1;

			sx += sprite_shift;

			drawgfx_transpen(bitmap,&cliprect,machine->gfx[1],code,colour+0x10,xflip,yflip,x68k_sys.crtc.hbegin+sx,x68k_sys.crtc.vbegin+sy,0x00);
		}
	}
}

PALETTE_INIT( x68000 )
{
	int pal;
	int r,g,b;

	for(pal=0;pal<32768;pal++)
	{  // create 64k colour lookup
		g = (pal & 0x7c00) >> 7;
		r = (pal & 0x03e0) >> 2;
		b = (pal & 0x001f) << 3;
		palette_set_color_rgb(machine,pal+512,r,g,b);
	}
}

static const gfx_layout x68k_pcg_8 =
{
	8,8,
	256,
	4,
	{ 0,1,2,3 },
	{ 8,12,0,4,24,28,16,20 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout x68k_pcg_16 =
{
	16,16,
	256,
	4,
	{ 0,1,2,3 },
	{ 8,12,0,4,24,28,16,20,8+64*8,12+64*8,64*8,4+64*8,24+64*8,28+64*8,16+64*8,20+64*8 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

#if 0
static GFXDECODEINFO_START( x68k )
	GFXDECODE_ENTRY( "user1", 0, x68k_pcg_8, 0x100, 16 )  // 8x8 sprite tiles
	GFXDECODE_ENTRY( "user1", 0, x68k_pcg_16, 0x100, 16 )  // 16x16 sprite tiles
GFXDECODEINFO_END
#endif

static TILE_GET_INFO(x68k_get_bg0_tile)
{
	int code = x68k_spriteram[0x3000+tile_index] & 0x00ff;
	int colour = (x68k_spriteram[0x3000+tile_index] & 0x0f00) >> 8;
	int flags = (x68k_spriteram[0x3000+tile_index] & 0xc000) >> 14;
	SET_TILE_INFO(0,code,colour+16,flags);
}

static TILE_GET_INFO(x68k_get_bg1_tile)
{
	int code = x68k_spriteram[0x2000+tile_index] & 0x00ff;
	int colour = (x68k_spriteram[0x2000+tile_index] & 0x0f00) >> 8;
	int flags = (x68k_spriteram[0x2000+tile_index] & 0xc000) >> 14;
	SET_TILE_INFO(0,code,colour+16,flags);
}

static TILE_GET_INFO(x68k_get_bg0_tile_16)
{
	int code = x68k_spriteram[0x3000+tile_index] & 0x00ff;
	int colour = (x68k_spriteram[0x3000+tile_index] & 0x0f00) >> 8;
	int flags = (x68k_spriteram[0x3000+tile_index] & 0xc000) >> 14;
	SET_TILE_INFO(1,code,colour+16,flags);
}

static TILE_GET_INFO(x68k_get_bg1_tile_16)
{
	int code = x68k_spriteram[0x2000+tile_index] & 0x00ff;
	int colour = (x68k_spriteram[0x2000+tile_index] & 0x0f00) >> 8;
	int flags = (x68k_spriteram[0x2000+tile_index] & 0xc000) >> 14;
	SET_TILE_INFO(1,code,colour+16,flags);
}

VIDEO_START( x68000 )
{
	int gfx_index;

	x68k_text_bitmap = auto_bitmap_alloc(machine, 1024,1024,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_big_bitmap = auto_bitmap_alloc(machine, 1024,1024,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_0_bitmap_16 = auto_bitmap_alloc(machine, 512,512,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_1_bitmap_16 = auto_bitmap_alloc(machine, 512,512,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_2_bitmap_16 = auto_bitmap_alloc(machine, 512,512,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_3_bitmap_16 = auto_bitmap_alloc(machine, 512,512,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_0_bitmap_256 = auto_bitmap_alloc(machine, 512,512,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_1_bitmap_256 = auto_bitmap_alloc(machine, 512,512,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_0_bitmap_65536 = auto_bitmap_alloc(machine, 512,512,BITMAP_FORMAT_INDEXED16);

	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (machine->gfx[gfx_index] == 0)
			break;

	/* create the char set (gfx will then be updated dynamically from RAM) */
	machine->gfx[gfx_index] = gfx_element_alloc(machine, &x68k_pcg_8, memory_region(machine, "user1"), 32, 0);

	gfx_index++;

	machine->gfx[gfx_index] = gfx_element_alloc(machine, &x68k_pcg_16, memory_region(machine, "user1"), 32, 0);
	machine->gfx[gfx_index]->total_colors = 32;

	/* Tilemaps */
	x68k_bg0_8 = tilemap_create(machine, x68k_get_bg0_tile,tilemap_scan_rows,8,8,64,64);
	x68k_bg1_8 = tilemap_create(machine, x68k_get_bg1_tile,tilemap_scan_rows,8,8,64,64);
	x68k_bg0_16 = tilemap_create(machine, x68k_get_bg0_tile_16,tilemap_scan_rows,16,16,64,64);
	x68k_bg1_16 = tilemap_create(machine, x68k_get_bg1_tile_16,tilemap_scan_rows,16,16,64,64);

	tilemap_set_transparent_pen(x68k_bg0_8,0);
	tilemap_set_transparent_pen(x68k_bg1_8,0);
	tilemap_set_transparent_pen(x68k_bg0_16,0);
	tilemap_set_transparent_pen(x68k_bg1_16,0);

//  timer_adjust_periodic(x68k_scanline_timer, attotime_zero, 0, ATTOTIME_IN_HZ(55.45)/568);
}

VIDEO_UPDATE( x68000 )
{
	rectangle rect = {0,0,0,0};
	int priority;
	int xscr,yscr;
	int x;
	tilemap_t* x68k_bg0;
	tilemap_t* x68k_bg1;
	//UINT8 *rom;

	if((x68k_spritereg[0x408] & 0x03) == 0x00)  // Sprite/BG H-Res 0=8x8, 1=16x16, 2 or 3 = undefined.
	{
		x68k_bg0 = x68k_bg0_8;
		x68k_bg1 = x68k_bg1_8;
	}
	else
	{
		x68k_bg0 = x68k_bg0_16;
		x68k_bg1 = x68k_bg1_16;
	}
//  rect.max_x=x68k_sys.crtc.width;
//  rect.max_y=x68k_sys.crtc.height;
	bitmap_fill(bitmap,cliprect,0);

	if(x68k_sys.sysport.contrast == 0)  // if monitor contrast is 0, then don't bother displaying anything
		return 0;

	rect.min_x=x68k_sys.crtc.hbegin;
	rect.min_y=x68k_sys.crtc.vbegin;
//  rect.max_x=rect.min_x + x68k_sys.crtc.visible_width-1;
//  rect.max_y=rect.min_y + x68k_sys.crtc.visible_height-1;
	rect.max_x=x68k_sys.crtc.hend;
	rect.max_y=x68k_sys.crtc.vend;

	if(rect.min_y < cliprect->min_y)
		rect.min_y = cliprect->min_y;
	if(rect.max_y > cliprect->max_y)
		rect.max_y = cliprect->max_y;

	// update tiles
	//rom = memory_region(screen->machine, "user1");
	for(x=0;x<256;x++)
	{
		if(x68k_sys.video.tile16_dirty[x] != 0)
		{
			gfx_element_mark_dirty(screen->machine->gfx[1], x);
			x68k_sys.video.tile16_dirty[x] = 0;
		}
		if(x68k_sys.video.tile8_dirty[x] != 0)
		{
			gfx_element_mark_dirty(screen->machine->gfx[0], x);
			x68k_sys.video.tile8_dirty[x] = 0;
		}
	}

	for(priority=3;priority>=0;priority--)
	{
		// Graphics screen(s)
		if(priority == x68k_sys.video.gfx_pri)
			x68k_draw_gfx(bitmap,rect);

		// Sprite / BG Tiles
		if(priority == x68k_sys.video.sprite_pri /*&& (x68k_spritereg[0x404] & 0x0200)*/ && (x68k_sys.video.reg[2] & 0x0040))
		{
			x68k_draw_sprites(screen->machine, bitmap,1,rect);
			if((x68k_spritereg[0x404] & 0x0008))
			{
				if((x68k_spritereg[0x404] & 0x0030) == 0x10)  // BG1 TXSEL
				{
					tilemap_set_scrollx(x68k_bg0,0,(x68k_spritereg[0x402] - x68k_sys.crtc.hbegin) & 0x3ff);
					tilemap_set_scrolly(x68k_bg0,0,(x68k_spritereg[0x403] - x68k_sys.crtc.vbegin) & 0x3ff);
					tilemap_draw(bitmap,&rect,x68k_bg0,0,0);
				}
				else
				{
					tilemap_set_scrollx(x68k_bg1,0,(x68k_spritereg[0x402] - x68k_sys.crtc.hbegin) & 0x3ff);
					tilemap_set_scrolly(x68k_bg1,0,(x68k_spritereg[0x403] - x68k_sys.crtc.vbegin) & 0x3ff);
					tilemap_draw(bitmap,&rect,x68k_bg1,0,0);
				}
			}
			x68k_draw_sprites(screen->machine,bitmap,2,rect);
			if((x68k_spritereg[0x404] & 0x0001))
			{
				if((x68k_spritereg[0x404] & 0x0006) == 0x02)  // BG0 TXSEL
				{
					tilemap_set_scrollx(x68k_bg0,0,(x68k_spritereg[0x400] - x68k_sys.crtc.hbegin) & 0x3ff);
					tilemap_set_scrolly(x68k_bg0,0,(x68k_spritereg[0x401] - x68k_sys.crtc.vbegin) & 0x3ff);
					tilemap_draw(bitmap,&rect,x68k_bg0,0,0);
				}
				else
				{
					tilemap_set_scrollx(x68k_bg1,0,(x68k_spritereg[0x400] - x68k_sys.crtc.hbegin) & 0x3ff);
					tilemap_set_scrolly(x68k_bg1,0,(x68k_spritereg[0x401] - x68k_sys.crtc.vbegin) & 0x3ff);
					tilemap_draw(bitmap,&rect,x68k_bg1,0,0);
				}
			}
			x68k_draw_sprites(screen->machine,bitmap,3,rect);
		}

		// Text screen
		if(x68k_sys.video.reg[2] & 0x0020 && priority == x68k_sys.video.text_pri)
		{
			xscr = (x68k_sys.crtc.reg[10] & 0x3ff);
			yscr = (x68k_sys.crtc.reg[11] & 0x3ff);
			if(!(x68k_sys.crtc.reg[20] & 0x1000))  // if text layer is set to buffer, then it's not visible
				x68k_draw_text(screen->machine,bitmap,xscr,yscr,rect);
		}
	}

#ifdef MAME_DEBUG
	if(input_code_pressed(screen->machine,KEYCODE_I))
	{
		x68k_sys.mfp.isra = 0;
		x68k_sys.mfp.isrb = 0;
//      mfp_trigger_irq(MFP_IRQ_GPIP6);
//      cputag_set_input_line_and_vector(machine, "maincpu",6,ASSERT_LINE,0x43);
	}
	if(input_code_pressed(screen->machine,KEYCODE_9))
	{
		sprite_shift--;
		popmessage("Sprite shift = %i",sprite_shift);
	}
	if(input_code_pressed(screen->machine,KEYCODE_0))
	{
		sprite_shift++;
		popmessage("Sprite shift = %i",sprite_shift);
	}

#endif

#ifdef MAME_DEBUG
//  popmessage("Layer priorities [%04x] - Txt: %i  Spr: %i  Gfx: %i  Layer Pri0-3: %i %i %i %i",x68k_sys.video.reg[1],x68k_sys.video.text_pri,x68k_sys.video.sprite_pri,
//      x68k_sys.video.gfx_pri,x68k_sys.video.gfxlayer_pri[0],x68k_sys.video.gfxlayer_pri[1],x68k_sys.video.gfxlayer_pri[2],x68k_sys.video.gfxlayer_pri[3]);
//  popmessage("CRTC regs - %i %i %i %i  - %i %i %i %i - %i - %i",x68k_sys.crtc.reg[0],x68k_sys.crtc.reg[1],x68k_sys.crtc.reg[2],x68k_sys.crtc.reg[3],
//      x68k_sys.crtc.reg[4],x68k_sys.crtc.reg[5],x68k_sys.crtc.reg[6],x68k_sys.crtc.reg[7],x68k_sys.crtc.reg[8],x68k_sys.crtc.reg[9]);
//  popmessage("Visible resolution = %ix%i (%s) Screen size = %ix%i",x68k_sys.crtc.visible_width,x68k_sys.crtc.visible_height,x68k_sys.crtc.interlace ? "Interlaced" : "Non-interlaced",x68k_sys.crtc.video_width,x68k_sys.crtc.video_height);
//  popmessage("VBlank : x68k_scanline = %i",x68k_scanline);
//  popmessage("CRTC/BG compare H-TOTAL %i/%i H-DISP %i/%i V-DISP %i/%i BG Res %02x",x68k_sys.crtc.reg[0],x68k_spritereg[0x405],x68k_sys.crtc.reg[2],x68k_spritereg[0x406],
//      x68k_sys.crtc.reg[6],x68k_spritereg[0x407],x68k_spritereg[0x408]);
//  popmessage("IER %02x %02x  IPR %02x %02x  ISR %02x %02x  IMR %02x %02x", x68k_sys.mfp.iera,x68k_sys.mfp.ierb,x68k_sys.mfp.ipra,x68k_sys.mfp.iprb,
//      x68k_sys.mfp.isra,x68k_sys.mfp.isrb,x68k_sys.mfp.imra,x68k_sys.mfp.imrb);
//  popmessage("BG Scroll - BG0 X %i Y %i  BG1 X %i Y %i",x68k_spriteram[0x400],x68k_spriteram[0x401],x68k_spriteram[0x402],x68k_spriteram[0x403]);
//  popmessage("Keyboard buffer position = %i",x68k_sys.keyboard.headpos);
//  popmessage("IERA = 0x%02x, IERB = 0x%02x",x68k_sys.mfp.iera,x68k_sys.mfp.ierb);
//  popmessage("IPRA = 0x%02x, IPRB = 0x%02x",x68k_sys.mfp.ipra,x68k_sys.mfp.iprb);
//  popmessage("uPD72065 status = %02x",upd765_status_r(machine, 0));
//  popmessage("Layer enable - 0x%02x",x68k_sys.video.reg[2] & 0xff);
//  popmessage("Graphic layer scroll - %i, %i - %i, %i - %i, %i - %i, %i",
//      x68k_sys.crtc.reg[12],x68k_sys.crtc.reg[13],x68k_sys.crtc.reg[14],x68k_sys.crtc.reg[15],x68k_sys.crtc.reg[16],x68k_sys.crtc.reg[17],x68k_sys.crtc.reg[18],x68k_sys.crtc.reg[19]);
//  popmessage("IOC IRQ status - %02x",x68k_sys.ioc.irqstatus);
//  popmessage("RAM: mouse data - %02x %02x %02x %02x",messram_get_ptr(devtag_get_device(machine, "messram"))[0x931],messram_get_ptr(devtag_get_device(machine, "messram"))[0x930],messram_get_ptr(devtag_get_device(machine, "messram"))[0x933],messram_get_ptr(devtag_get_device(machine, "messram"))[0x932]);
#endif
	return 0;
}
