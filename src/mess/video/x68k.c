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

#include "driver.h"
#include "inputx.h"
#include "render.h"
#include "includes/x68k.h"
#include "machine/68901mfp.h"

extern struct x68k_system sys;

extern mame_timer* scanline_timer;
extern mame_timer* raster_irq;
extern mame_timer* vblank_irq;

UINT16* gvram;  // Graphic VRAM
UINT16* tvram;  // Text VRAM
UINT16* x68k_spriteram;  // sprite/background RAM
UINT16* x68k_spritereg;  // sprite/background registers

mame_bitmap* x68k_text_bitmap;  // 1024x1024 4x1bpp planes text
mame_bitmap* x68k_gfx_0_bitmap_16;  // 16 colour, 512x512, 4 pages
mame_bitmap* x68k_gfx_1_bitmap_16; 
mame_bitmap* x68k_gfx_2_bitmap_16; 
mame_bitmap* x68k_gfx_3_bitmap_16; 
mame_bitmap* x68k_gfx_0_bitmap_256;  // 256 colour, 512x512, 2 pages
mame_bitmap* x68k_gfx_1_bitmap_256; 
mame_bitmap* x68k_gfx_0_bitmap_65536;  // 65536 colour, 512x512, 1 page

tilemap* x68k_bg0_8;  // two 64x64 tilemaps, 8x8 characters
tilemap* x68k_bg1_8;
tilemap* x68k_bg0_16;  // two 64x64 tilemaps, 16x16 characters
tilemap* x68k_bg1_16;

extern unsigned int x68k_scanline;
int sprite_shift;

void x68k_render_video_word(int offset);
void x68k_crtc_refresh_mode(void);
void x68k_scanline_check(int);

INLINE void x68k_plot_pixel(bitmap_t *bitmap, int x, int y, UINT32 color)
{
	*BITMAP_ADDR16(bitmap, y, x) = (UINT16)color;
}

mame_bitmap* x68k_get_gfx_pri(int pri,int type)
{
	if(type == GFX16)
	{
		switch(sys.video.gfxlayer_pri[pri])
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
		switch(sys.video.gfxlayer_pri[pri])
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

void x68k_crtc_text_copy(int src, int dest)
{
	// copys one raster in T-VRAM to another raster
	int src_ram = src * 256;  // 128 bytes per scanline
	int dest_ram = dest * 256;
	int x;
	int line;

	if(dest > 250)
		return;  // for some reason, Salamander causes a SIGSEGV in a debug build in this function.

	for(line=0;line<8;line++)
	{
		// update RAM in each plane
		memcpy(tvram+dest_ram,tvram+src_ram,128);
		memcpy(tvram+dest_ram+0x10000,tvram+src_ram+0x10000,128);
		memcpy(tvram+dest_ram+0x20000,tvram+src_ram+0x20000,128);
		memcpy(tvram+dest_ram+0x30000,tvram+src_ram+0x30000,128);

		// redraw scanline
		for(x=0;x<64;x++)
			x68k_render_video_word(dest_ram+x);

		src_ram+=64;
		dest_ram+=64;
	}

}

static TIMER_CALLBACK(x68k_crtc_operation_end)
{
	int bit = param;
	sys.crtc.operation &= ~bit;
}

void x68k_crtc_refresh_mode()
{
	rectangle rect;
//	double scantime;

	rect.min_x = rect.min_y = 0;
	sys.crtc.visible_width = (sys.crtc.reg[3] - sys.crtc.reg[2]) * 8;
	if(sys.crtc.height == 256)
		sys.crtc.visible_height = (sys.crtc.reg[7] - sys.crtc.reg[6]) / 2;
	else
		sys.crtc.visible_height = sys.crtc.reg[7] - sys.crtc.reg[6];
	if(!(sys.crtc.reg[20] & 0x10))  // 15kHz horizontal frequency
		sys.crtc.visible_height *= 2;

	rect.max_x = sys.crtc.width - 1;
	rect.max_y = sys.crtc.height - 1;

	sys.crtc.video_width = sys.crtc.reg[0] * 8;
	sys.crtc.video_height = sys.crtc.reg[4] + 1;
	if(sys.crtc.height == 256)
		sys.crtc.video_height = (sys.crtc.reg[4] / 2) + 1;
	if(rect.max_x < 1 || rect.max_y < 1)
		return;  // bail out

	if(sys.crtc.video_width < rect.max_x)
		sys.crtc.video_width = rect.max_x + 1;
	if(sys.crtc.video_height < rect.max_y)
		sys.crtc.video_height = rect.max_y + 1;

	// for now, we'll just center the display area, rather than calculate the display position from the CRTC regs
	sys.crtc.hshift = (sys.crtc.width - sys.crtc.visible_width) / 2;
	sys.crtc.vshift = (sys.crtc.height - sys.crtc.visible_height) / 2;

	video_screen_configure(0,sys.crtc.video_width,sys.crtc.video_height,&rect,HZ_TO_SUBSECONDS(55.45));
	logerror("video_screen_configure(0,%i,%i,[%i,%i,%i,%i],55.45)\n",sys.crtc.video_width,sys.crtc.video_height,rect.min_x,rect.min_y,rect.max_x,rect.max_y);
//	x68k_scanline = video_screen_get_vpos(0);
	if(sys.crtc.reg[4] != 0)
	{
//		scantime = MAME_TIME_IN_HZ(55.45) / sys.crtc.reg[4];
//		mame_timer_adjust(scanline_timer,time_zero,0,scantime);
	}
}

TIMER_CALLBACK(x68k_hsync)
{
	int state = param;
	mame_time irq_time = video_screen_get_scan_period(0);
	mame_time hsync_time = MAME_TIME_IN_CYCLES(0,32);

	sys.crtc.hblank = state;	
	if(state == 1)
	{
//		mfp_trigger_irq(MFP_IRQ_GPIP7);  // HSync
		mame_timer_adjust(scanline_timer,hsync_time,0,time_never);
	}
	if(state == 0)
	{
		double time_to_irq = mame_time_to_double(irq_time) - mame_time_to_double(hsync_time);
		mame_timer_adjust(scanline_timer,double_to_mame_time(time_to_irq),1,time_never);
		if(!(sys.mfp.gpio & 0x40))  // if GPIP6 is active, clear it
			sys.mfp.gpio |= 0x40;
	}
}


TIMER_CALLBACK(x68k_crtc_raster_irq)
{
	int scan = param;
	mame_time irq_time;
//	mfp_trigger_irq(MFP_IRQ_GPIP6);
	sys.mfp.gpio &= ~0x40;  // GPIP6
	if((readinputportbytag("options") & 0x01))
	{
		video_screen_update_partial(0,scan);
	}

	irq_time = video_screen_get_time_until_pos(0,scan,2);
	if(mame_time_to_double(irq_time) > 0)
		mame_timer_adjust(raster_irq,irq_time,scan,time_never);
	logerror("GPIP6: Raster triggered at line %i (%i)\n",scan,video_screen_get_vpos(0));
}

TIMER_CALLBACK(x68k_crtc_vblank_irq)
{
	int val = param;
	mame_time irq_time;
	int vblank_line;

	if(val == 1)  // VBlank on
	{
		sys.crtc.vblank = 1;
		vblank_line = sys.crtc.reg[7];
		if(vblank_line > sys.crtc.reg[4])
			vblank_line = sys.crtc.reg[4];
		if(sys.crtc.height == 256)
			irq_time = video_screen_get_time_until_pos(0,vblank_line / 2,2);
		else
			irq_time = video_screen_get_time_until_pos(0,vblank_line,2);
		mame_timer_adjust(vblank_irq,irq_time,0,time_never);
		logerror("CRTC: VBlank on\n");
	}
	if(val == 0)  // VBlank off
	{
		sys.crtc.vblank = 0;
		vblank_line = sys.crtc.reg[6];
		if(sys.crtc.height == 256)
			irq_time = video_screen_get_time_until_pos(0,vblank_line / 2,2);
		else
			irq_time = video_screen_get_time_until_pos(0,vblank_line,2);
		mame_timer_adjust(vblank_irq,irq_time,1,time_never);
		logerror("CRTC: VBlank off\n");
	}
}

// CRTC "VINAS 1+2 / VICON" at 0xe80000
WRITE16_HANDLER( x68k_crtc_w )
{
	COMBINE_DATA(sys.crtc.reg+offset);
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
		x68k_crtc_refresh_mode();
		break;
	case 9:  // CRTC raster IRQ (GPIP6)
		mame_timer_adjust(raster_irq,time_never,0,time_never);  // disable timer
		if(sys.crtc.height == 256)  // adjust to visible area
		{
			data = data / 2;
			data -= (sys.crtc.reg[6] / 2);
			if(data > (sys.crtc.reg[4] / 2))  // data is unsigned
				data += sys.crtc.reg[4] / 2;
		}
		else
		{
			data -= sys.crtc.reg[6];
			if(data > sys.crtc.reg[4])
				data += sys.crtc.reg[4];
		}
		if(data <= sys.crtc.video_height)
		{
			mame_time irq_time;
			irq_time = video_screen_get_time_until_pos(0,data - 1,2);

			if(mame_time_to_double(irq_time) > 0)
				mame_timer_adjust(raster_irq,irq_time,data - 1,time_never);
			logerror("CRTC: Time until next raster IRQ = %f\n",mame_time_to_double(irq_time));
		}
		logerror("CRTC: Write to raster IRQ register - %i\n",data);
		break;
	case 20:
		if(ACCESSING_LSB)
		{
			switch(data & 0x0c)
			{
			case 0x00:
				sys.crtc.height = 256;
				break;
			case 0x08:
			case 0x0c:  // TODO: 1024 vertical, if horizontal freq = 31kHz
				sys.crtc.height = 512;
				break;
			case 0x04:
				sys.crtc.height = 512;
				break;
			}
			switch(data & 0x03)
			{
			case 0x00:
				sys.crtc.width = 256;
				break;
			case 0x01:
				sys.crtc.width = 512;
				break;
			case 0x02:
			case 0x03:  // 0x03 = 50MHz clock mode (XVI only)
				sys.crtc.width = 768;
				break;
			}
		}
		if(ACCESSING_MSB)
		{
			sys.crtc.interlace = 0;
			if(data & 0x0400)  // real size 1024x1024
				sys.crtc.interlace = 1;			
		}
		x68k_crtc_refresh_mode();
		break;
	case 576:  // operation register
		sys.crtc.operation = data;
		if(data & 0x08)  // text screen raster copy
		{
			x68k_crtc_text_copy((sys.crtc.reg[22] & 0xff00) >> 8,(sys.crtc.reg[22] & 0x00ff));
			mame_timer_set(MAME_TIME_IN_MSEC(1),0x02,x68k_crtc_operation_end);  // time taken to do operation is a complete guess.
		}
		if(data & 0x02)  // high-speed graphic screen clear
		{
			rectangle rect = {0,0,0,0};
			rect.max_x = 512;
			rect.max_y = 512;
			if(sys.crtc.reg[21] & 0x01)
			{
				fillbitmap(x68k_gfx_0_bitmap_16,0,&rect);
				fillbitmap(x68k_gfx_0_bitmap_256,0,&rect);
				fillbitmap(x68k_gfx_0_bitmap_65536,0,&rect);
			}
			if(sys.crtc.reg[21] & 0x02)
			{
				fillbitmap(x68k_gfx_1_bitmap_16,0,&rect);
				fillbitmap(x68k_gfx_1_bitmap_256,0,&rect);
			}
			if(sys.crtc.reg[21] & 0x04)
			{
				fillbitmap(x68k_gfx_2_bitmap_16,0,&rect);
			}
			if(sys.crtc.reg[21] & 0x08)
			{
				fillbitmap(x68k_gfx_3_bitmap_16,0,&rect);
			}
			mame_timer_set(MAME_TIME_IN_MSEC(10),0x02,x68k_crtc_operation_end);  // time taken to do operation is a complete guess.
//			popmessage("CRTC: High-speed gfx screen clear [0x%02x]",sys.crtc.reg[21] & 0x0f);
		}
		break;
	}
//	logerror("CRTC: [%08x] Wrote %04x to CRTC register %i\n",activecpu_get_pc(),data,offset);
}

READ16_HANDLER( x68k_crtc_r )
{
/*	switch(offset)
	{
	default:
		logerror("CRTC: [%08x] Read from CRTC register %i\n",activecpu_get_pc(),offset);
		return 0xff;
	}*/
	if(offset < 24)
	{
//		logerror("CRTC: [%08x] Read %04x from CRTC register %i\n",activecpu_get_pc(),sys.crtc.reg[offset],offset);
		switch(offset)
		{
		case 9:
			return 0;
		case 10:  // Text X/Y scroll
		case 11:
		case 12:  // Graphic layer 0 scroll
		case 13:
			return sys.crtc.reg[offset] & 0x3ff;
		case 14:  // Graphic layer 1 scroll
		case 15:
		case 16:  // Graphic layer 2 scroll
		case 17:
		case 18:  // Graphic layer 3 scroll
		case 19:
			return sys.crtc.reg[offset] & 0x1ff;
		default:
			return sys.crtc.reg[offset];
		}
	}
	if(offset == 576) // operation port, operation bits are set to 0 when operation is complete
		return sys.crtc.operation;
//	logerror("CRTC: [%08x] Read from unknown CRTC register %i\n",activecpu_get_pc(),offset);
	return 0xffff;
}

int x68k_get_text_pixel(int offset, int bit)
{
	int ret = 0;

//	if(offset % 2 == 0)
//		bit += 8;
//	offset = offset/2;
	if(tvram[offset] & (1 << bit))
		ret |= 0x01;
	if(tvram[offset+0x10000] & (1 << bit))
		ret |= 0x02;
	if(tvram[offset+0x20000] & (1 << bit))
		ret |= 0x04;
	if(tvram[offset+0x30000] & (1 << bit))
		ret |= 0x08;

	return ret;
}

void x68k_render_video_word(int offset)
{
	int x,y;
	int l;

	offset &= 0xffff;
	y = offset / 64;
	x = ((offset % 64)) * 16;

	for(l=0;l<16;l++)
	{
		x68k_plot_pixel(x68k_text_bitmap,x+(15-l),y,0x100+x68k_get_text_pixel(offset,l));
	}
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
	COMBINE_DATA(gvram+offset);
	
	pageoffset = offset & 0x3ffff;
	xloc = pageoffset % 512;
	yloc = pageoffset / 512;
	if(sys.crtc.interlace == 1)  // 1024 vertical displayed
	{
		// we'll just draw every other scanline for now
		yloc = pageoffset / 1024;
	}

	if(offset < 0x40000)  // first page, all colour depths
	{
		x68k_plot_pixel(x68k_gfx_0_bitmap_65536,xloc,yloc,(gvram[offset] >> 1) + 512);
		x68k_plot_pixel(x68k_gfx_0_bitmap_256,xloc,yloc,data & 0x00ff);
		x68k_plot_pixel(x68k_gfx_0_bitmap_16,xloc,yloc,data & 0x000f);
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
	if(sys.crtc.reg[21] & 0x0100)
	{  // simultaneous T-VRAM plane access (I think ;))
		int plane,wr;
		offset = offset & 0x00ffff;
		wr = (sys.crtc.reg[21] & 0x00f0) >> 4;
		for(plane=0;plane<4;plane++)
		{
			if(wr & (1 << plane))
			{
				COMBINE_DATA(tvram+offset+(0x10000*plane));
			}
		}
		x68k_render_video_word(offset);
	}
	else
	{
		COMBINE_DATA(tvram+offset);
		x68k_render_video_word(offset);
	}
}

READ16_HANDLER( x68k_gvram_r )
{
	return gvram[offset];
}

READ16_HANDLER( x68k_tvram_r )
{
	return tvram[offset];
}

WRITE16_HANDLER( x68k_spritereg_w )
{
	COMBINE_DATA(x68k_spritereg+offset);
	switch(offset)
	{
	case 0x400:
		tilemap_set_scrollx(x68k_bg0_8,0,(data - sys.crtc.hshift) & 0x3ff);
		tilemap_set_scrollx(x68k_bg0_16,0,(data - sys.crtc.hshift) & 0x3ff);
		break;
	case 0x401:
		tilemap_set_scrolly(x68k_bg0_8,0,(data - sys.crtc.vshift) & 0x3ff);
		tilemap_set_scrolly(x68k_bg0_16,0,(data - sys.crtc.vshift) & 0x3ff);
		break;
	case 0x402:
		tilemap_set_scrollx(x68k_bg1_8,0,(data - sys.crtc.hshift) & 0x3ff);
		tilemap_set_scrollx(x68k_bg1_16,0,(data - sys.crtc.hshift) & 0x3ff);
		break;
	case 0x403:
		tilemap_set_scrolly(x68k_bg1_8,0,(data - sys.crtc.vshift) & 0x3ff);
		tilemap_set_scrolly(x68k_bg1_16,0,(data - sys.crtc.vshift) & 0x3ff);
		break;
	case 0x405:  // BG H-DISP (like CRTC reg 2)
		if(data != 0x00ff)
		{
			sys.crtc.bg_visible_width = (sys.crtc.reg[3] - ((data & 0x003f) - 4)) * 8;
			sys.crtc.bg_hshift = (sys.crtc.width - sys.crtc.bg_visible_width) / 2;
		}
		else
			sys.crtc.bg_hshift = sys.crtc.hshift;
		break;
	case 0x406:  // BG V-DISP (like CRTC reg 6)
		sys.crtc.bg_vshift = sys.crtc.vshift;
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
	sys.video.tile8_dirty[offset / 16] = 1;
	sys.video.tile16_dirty[offset / 64] = 1;
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
void x68k_draw_gfx(mame_bitmap* bitmap,rectangle cliprect)
{
	int priority;
	rectangle rect;
	int xscr,yscr;

	for(priority=3;priority>=0;priority--)
	{
		if(sys.video.reg[0] & 0x04)  // 1024x1024 "real" screen size - use 1024x1024 16-colour gfx layer
		{
			// 16 colour gfx screen
			rect.min_x=sys.crtc.hshift;
			rect.min_y=sys.crtc.vshift;
			rect.max_x=rect.min_x + sys.crtc.visible_width-1;
			rect.max_y=rect.min_y + sys.crtc.visible_height-1;
			if(sys.video.reg[2] & 0x0010 && priority == sys.video.gfxlayer_pri[0])
			{
				xscr = sys.crtc.hshift-(sys.crtc.reg[12] & 0x1ff);
				yscr = sys.crtc.vshift-(sys.crtc.reg[13] & 0x1ff);
				copyscrollbitmap(bitmap, x68k_gfx_0_bitmap_16, 1, &xscr, 1, &yscr ,&cliprect, TRANSPARENCY_PEN,0); 
				xscr+=512;
				copyscrollbitmap(bitmap, x68k_gfx_1_bitmap_16, 1, &xscr, 1, &yscr ,&cliprect, TRANSPARENCY_PEN,0); 
				yscr+=512;
				copyscrollbitmap(bitmap, x68k_gfx_2_bitmap_16, 1, &xscr, 1, &yscr ,&cliprect, TRANSPARENCY_PEN,0); 
				xscr-=512;
				copyscrollbitmap(bitmap, x68k_gfx_3_bitmap_16, 1, &xscr, 1, &yscr ,&cliprect, TRANSPARENCY_PEN,0); 
			}
		}
		else  // 512x512 "real" screen size
		{
			switch(sys.video.reg[0] & 0x03)
			{
			case 0x03:
				// 65,536 colour gfx screen
				if(sys.video.reg[2] & 0x0001 && priority == sys.video.gfxlayer_pri[0])
				{
					rect.min_x=sys.crtc.hshift;
					rect.min_y=sys.crtc.vshift;
					rect.max_x=rect.min_x + sys.crtc.visible_width-1;
					rect.max_y=rect.min_y + sys.crtc.visible_height-1;
					xscr = sys.crtc.hshift-(sys.crtc.reg[12] & 0x1ff);
					yscr = sys.crtc.vshift-(sys.crtc.reg[13] & 0x1ff);
					copyscrollbitmap(bitmap, x68k_gfx_0_bitmap_65536, 1, &xscr, 1, &yscr,&cliprect, TRANSPARENCY_PEN,0); 
				}
				break;
			case 0x01:
				// 256 colour gfx screen
				rect.min_x=sys.crtc.hshift;
				rect.min_y=sys.crtc.vshift;
				rect.max_x=rect.min_x + sys.crtc.visible_width-1;
				rect.max_y=rect.min_y + sys.crtc.visible_height-1;
				if(sys.video.reg[2] & 0x0004 && sys.video.reg[2] & 0x0008 && priority == sys.video.gfxlayer_pri[2])
				{
					xscr = sys.crtc.hshift-(sys.crtc.reg[16] & 0x1ff);
					yscr = sys.crtc.vshift-(sys.crtc.reg[17] & 0x1ff);
					copyscrollbitmap(bitmap, x68k_gfx_1_bitmap_256, 1, &xscr, 1, &yscr, &cliprect, TRANSPARENCY_PEN,0); 
				}
				if(sys.video.reg[2] & 0x0001 && sys.video.reg[2] & 0x0002 && priority == sys.video.gfxlayer_pri[0])
				{
					xscr = sys.crtc.hshift-(sys.crtc.reg[12] & 0x1ff);
					yscr = sys.crtc.vshift-(sys.crtc.reg[13] & 0x1ff);
					copyscrollbitmap(bitmap, x68k_gfx_0_bitmap_256, 1, &xscr, 1, &yscr, &cliprect, TRANSPARENCY_PEN,0); 
				}
				break;
			case 0x00:
				// 16 colour gfx screen
				rect.min_x=sys.crtc.hshift;
				rect.min_y=sys.crtc.vshift;
				rect.max_x=rect.min_x + sys.crtc.visible_width-1;
				rect.max_y=rect.min_y + sys.crtc.visible_height-1;
				if(sys.video.reg[2] & 0x0008)  // Pri3
				{
					xscr = sys.crtc.hshift-(sys.crtc.reg[18] & 0x1ff);
					yscr = sys.crtc.vshift-(sys.crtc.reg[19] & 0x1ff);
					copyscrollbitmap(bitmap, x68k_get_gfx_pri(3,GFX16), 1, &xscr, 1, &yscr ,&cliprect, TRANSPARENCY_PEN,0); 
				}
				if(sys.video.reg[2] & 0x0004)  // Pri2
				{
					xscr = sys.crtc.hshift-(sys.crtc.reg[16] & 0x1ff);
					yscr = sys.crtc.vshift-(sys.crtc.reg[17] & 0x1ff);
					copyscrollbitmap(bitmap, x68k_get_gfx_pri(2,GFX16), 1, &xscr, 1, &yscr ,&cliprect, TRANSPARENCY_PEN,0); 
				}
				if(sys.video.reg[2] & 0x0002)  // Pri1
				{
					xscr = sys.crtc.hshift-(sys.crtc.reg[14] & 0x1ff);
					yscr = sys.crtc.vshift-(sys.crtc.reg[15] & 0x1ff);
					copyscrollbitmap(bitmap, x68k_get_gfx_pri(1,GFX16), 1, &xscr, 1, &yscr ,&cliprect, TRANSPARENCY_PEN,0); 
				}
				if(sys.video.reg[2] & 0x0001)  // Pri0
				{
					xscr = sys.crtc.hshift-(sys.crtc.reg[12] & 0x1ff);
					yscr = sys.crtc.vshift-(sys.crtc.reg[13] & 0x1ff);
					copyscrollbitmap(bitmap, x68k_get_gfx_pri(0,GFX16), 1, &xscr, 1, &yscr ,&cliprect, TRANSPARENCY_PEN,0); 
				}
				break;
			}
		}
	}
}

// Sprite controller "Cynthia" at 0xeb0000
void x68k_draw_sprites(mame_bitmap* bitmap, int priority, rectangle cliprect)
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
		if(!(input_code_pressed(KEYCODE_I)))
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

			rect.min_x=sys.crtc.hshift;
			rect.min_y=sys.crtc.vshift;
			rect.max_x=rect.min_x + sys.crtc.visible_width-1;
			rect.max_y=rect.min_y + sys.crtc.visible_height-1;

			sx += sprite_shift;

			drawgfx(bitmap,Machine->gfx[1],code,colour+0x10,xflip,yflip,sys.crtc.hshift+sx,sys.crtc.vshift+sy,&cliprect,TRANSPARENCY_PEN,0x00);
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

/*
static gfx_decode x68k_gfxdecodeinfo[] =
{
	{ REGION_USER1, 0, &x68k_pcg_8, 0x100, 16 },  // 8x8 sprite tiles
	{ REGION_USER1, 0, &x68k_pcg_16, 0x100, 16 },  // 16x16 sprite tiles
	{ -1 }
};
*/

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

	x68k_text_bitmap = auto_bitmap_alloc(1024,1024,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_0_bitmap_16 = auto_bitmap_alloc(512,512,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_1_bitmap_16 = auto_bitmap_alloc(512,512,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_2_bitmap_16 = auto_bitmap_alloc(512,512,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_3_bitmap_16 = auto_bitmap_alloc(512,512,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_0_bitmap_256 = auto_bitmap_alloc(512,512,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_1_bitmap_256 = auto_bitmap_alloc(512,512,BITMAP_FORMAT_INDEXED16);
	x68k_gfx_0_bitmap_65536 = auto_bitmap_alloc(512,512,BITMAP_FORMAT_INDEXED16);

	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;

	/* create the char set (gfx will then be updated dynamically from RAM) */
	Machine->gfx[gfx_index] = allocgfx(&x68k_pcg_8);
	decodegfx(Machine->gfx[gfx_index] , memory_region(REGION_USER1), 0, 256);
	/* 7-Sep-2007 - After 0.118u5, you cannot change the colortable */
	/* Machine->gfx[gfx_index]->colortable = (Machine->remapped_colortable+0x100); */
	Machine->gfx[gfx_index]->total_colors = 32;

	gfx_index++;

	Machine->gfx[gfx_index] = allocgfx(&x68k_pcg_16);
	decodegfx(Machine->gfx[gfx_index] , memory_region(REGION_USER1), 0, 256);
	/* 7-Sep-2007 - After 0.118u5, you cannot change the colortable */
	/* Machine->gfx[gfx_index]->colortable = (Machine->remapped_colortable+0x100); */
	Machine->gfx[gfx_index]->total_colors = 32;

	/* Tilemaps */
	x68k_bg0_8 = tilemap_create(x68k_get_bg0_tile,tilemap_scan_rows,TILEMAP_TYPE_PEN,8,8,64,64);
	x68k_bg1_8 = tilemap_create(x68k_get_bg1_tile,tilemap_scan_rows,TILEMAP_TYPE_PEN,8,8,64,64);
	x68k_bg0_16 = tilemap_create(x68k_get_bg0_tile_16,tilemap_scan_rows,TILEMAP_TYPE_PEN,16,16,64,64);
	x68k_bg1_16 = tilemap_create(x68k_get_bg1_tile_16,tilemap_scan_rows,TILEMAP_TYPE_PEN,16,16,64,64);

	tilemap_set_transparent_pen(x68k_bg0_8,0);
	tilemap_set_transparent_pen(x68k_bg1_8,0);
	tilemap_set_transparent_pen(x68k_bg0_16,0);
	tilemap_set_transparent_pen(x68k_bg1_16,0);

//	mame_timer_adjust(scanline_timer,time_zero,0,MAME_TIME_IN_HZ(55.45)/568);
}

VIDEO_UPDATE( x68000 )
{
	rectangle rect = {0,0,0,0};
	int priority;
	int xscr,yscr;
	int x;
	tilemap* x68k_bg0;
	tilemap* x68k_bg1;

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
//	rect.max_x=sys.crtc.width;
//	rect.max_y=sys.crtc.height;
	fillbitmap(bitmap,0,cliprect);

	rect.min_x=sys.crtc.hshift;
	rect.min_y=sys.crtc.vshift;
	rect.max_x=rect.min_x + sys.crtc.visible_width-1;
	rect.max_y=rect.min_y + sys.crtc.visible_height-1;
	if(rect.min_y < cliprect->min_y)
		rect.min_y = cliprect->min_y;
	if(rect.max_y > cliprect->max_y)
		rect.max_y = cliprect->max_y;

	// update tiles
	for(x=0;x<256;x++)
	{
		if(sys.video.tile16_dirty[x] != 0)
		{
			decodechar(Machine->gfx[1], x,memory_region(REGION_USER1), &x68k_pcg_16);
			sys.video.tile16_dirty[x] = 0;
		}
		if(sys.video.tile8_dirty[x] != 0)
		{
			decodechar(Machine->gfx[0], x,memory_region(REGION_USER1), &x68k_pcg_8);
			sys.video.tile8_dirty[x] = 0;
		}
	}

	for(priority=3;priority>=0;priority--)
	{
		// Graphics screen(s)
		if(priority == sys.video.gfx_pri)
			x68k_draw_gfx(bitmap,rect);

		// Sprite / BG Tiles
		if(priority == sys.video.sprite_pri /*&& (x68k_spritereg[0x404] & 0x0200)*/ && (sys.video.reg[2] & 0x0040))
		{
			x68k_draw_sprites(bitmap,1,rect);
			if((x68k_spritereg[0x404] & 0x0008))
			{
				if((x68k_spritereg[0x404] & 0x0030) == 0x10)  // BG1 TXSEL
				{
					tilemap_set_scrollx(x68k_bg0,0,(x68k_spritereg[0x402] - sys.crtc.hshift) & 0x3ff);
					tilemap_set_scrolly(x68k_bg0,0,(x68k_spritereg[0x403] - sys.crtc.vshift) & 0x3ff);
					tilemap_draw(bitmap,&rect,x68k_bg0,0,0);
				}
				else
				{
					tilemap_set_scrollx(x68k_bg1,0,(x68k_spritereg[0x402] - sys.crtc.hshift) & 0x3ff);
					tilemap_set_scrolly(x68k_bg1,0,(x68k_spritereg[0x403] - sys.crtc.vshift) & 0x3ff);
					tilemap_draw(bitmap,&rect,x68k_bg1,0,0);
				}
			}
			x68k_draw_sprites(bitmap,2,rect);
			if((x68k_spritereg[0x404] & 0x0001))
			{
				if((x68k_spritereg[0x404] & 0x0006) == 0x02)  // BG0 TXSEL
				{
					tilemap_set_scrollx(x68k_bg0,0,(x68k_spritereg[0x400] - sys.crtc.hshift) & 0x3ff);
					tilemap_set_scrolly(x68k_bg0,0,(x68k_spritereg[0x401] - sys.crtc.vshift) & 0x3ff);
					tilemap_draw(bitmap,&rect,x68k_bg0,0,0);
				}
				else
				{
					tilemap_set_scrollx(x68k_bg1,0,(x68k_spritereg[0x400] - sys.crtc.hshift) & 0x3ff);
					tilemap_set_scrolly(x68k_bg1,0,(x68k_spritereg[0x401] - sys.crtc.vshift) & 0x3ff);
					tilemap_draw(bitmap,&rect,x68k_bg1,0,0);
				}
			}
			x68k_draw_sprites(bitmap,3,rect);
		}

		// Text screen
		if(sys.video.reg[2] & 0x0020 && priority == sys.video.text_pri)
		{
			xscr = sys.crtc.hshift-(sys.crtc.reg[10] & 0x3ff);
			yscr = sys.crtc.vshift-(sys.crtc.reg[11] & 0x3ff);
#ifdef MAME_DEBUG
			if(!input_code_pressed(KEYCODE_Q))
#endif
				copyscrollbitmap(bitmap, x68k_text_bitmap, 1, &xscr, 1, &yscr, &rect, TRANSPARENCY_PEN,0x100); 
		}
	}

#ifdef MAME_DEBUG
	if(input_code_pressed(KEYCODE_I))
	{
		sys.mfp.isra = 0;
		sys.mfp.isrb = 0;
//		mfp_trigger_irq(MFP_IRQ_GPIP6);
//		cpunum_set_input_line_and_vector(0,6,ASSERT_LINE,0x43);
	}
	if(input_code_pressed(KEYCODE_9))
	{
		sprite_shift--;
		popmessage("Sprite shift = %i",sprite_shift);
	}
	if(input_code_pressed(KEYCODE_0))
	{
		sprite_shift++;
		popmessage("Sprite shift = %i",sprite_shift);
	}

#endif

#ifdef MAME_DEBUG
//	popmessage("CRTC regs - %i %i %i %i  - %i %i %i %i - %i - %i",sys.crtc.reg[0],sys.crtc.reg[1],sys.crtc.reg[2],sys.crtc.reg[3],
//		sys.crtc.reg[4],sys.crtc.reg[5],sys.crtc.reg[6],sys.crtc.reg[7],sys.crtc.reg[8],sys.crtc.reg[9]);
//	popmessage("Visible resolution = %ix%i (%s) Screen size = %ix%i",sys.crtc.visible_width,sys.crtc.visible_height,sys.crtc.interlace ? "Interlaced" : "Non-interlaced",sys.crtc.video_width,sys.crtc.video_height);
//	popmessage("VBlank : x68k_scanline = %i",x68k_scanline);
//	popmessage("CRTC/BG compare H-TOTAL %i/%i H-DISP %i/%i V-DISP %i/%i BG Res %02x",sys.crtc.reg[0],x68k_spritereg[0x405],sys.crtc.reg[2],x68k_spritereg[0x406],
//		sys.crtc.reg[6],x68k_spritereg[0x407],x68k_spritereg[0x408]);
//	popmessage("IER %02x %02x  IPR %02x %02x  ISR %02x %02x  IMR %02x %02x", sys.mfp.iera,sys.mfp.ierb,sys.mfp.ipra,sys.mfp.iprb,
//		sys.mfp.isra,sys.mfp.isrb,sys.mfp.imra,sys.mfp.imrb);
//	popmessage("BG Scroll - BG0 X %i Y %i  BG1 X %i Y %i",x68k_spriteram[0x400],x68k_spriteram[0x401],x68k_spriteram[0x402],x68k_spriteram[0x403]);
//	popmessage("Keyboard buffer position = %i",sys.keyboard.headpos);
//	popmessage("IERA = 0x%02x, IERB = 0x%02x",sys.mfp.iera,sys.mfp.ierb);
//	popmessage("IPRA = 0x%02x, IPRB = 0x%02x",sys.mfp.ipra,sys.mfp.iprb);
//	popmessage("uPD72065 status = %02x",nec765_status_r(0));
//	popmessage("Layer enable - 0x%02x",sys.video.reg[2] & 0xff);
//	popmessage("Graphic layer scroll - %i, %i - %i, %i - %i, %i - %i, %i",
//		sys.crtc.reg[12],sys.crtc.reg[13],sys.crtc.reg[14],sys.crtc.reg[15],sys.crtc.reg[16],sys.crtc.reg[17],sys.crtc.reg[18],sys.crtc.reg[19]);
//	popmessage("Layer priorities - Txt: %i  Spr: %i  Gfx: %i  Pages 0-3: %i %i %i %i",sys.video.text_pri,sys.video.sprite_pri,
//		sys.video.gfx_pri,sys.video.gfx0_pri,sys.video.gfx1_pri,sys.video.gfx2_pri,sys.video.gfx3_pri);
//	popmessage("Video Controller registers - %04x - %04x - %04x",sys.video.reg[0],sys.video.reg[1],sys.video.reg[2]);
//	popmessage("IOC IRQ status - %02x",sys.ioc.irqstatus);
//	popmessage("RAM: mouse data - %02x %02x %02x %02x",mess_ram[0x931],mess_ram[0x930],mess_ram[0x933],mess_ram[0x932]);
#endif
	return 0;
}
