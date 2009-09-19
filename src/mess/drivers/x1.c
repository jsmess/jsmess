/************************************************************************************************

	Sharp X1 (c) 1983 Sharp Corporation

	driver by Angelo Salese & Barry Rodewald

	TODO:
	- Understand how keyboard works and decap/dump the keyboard MCU if possible;
	- Hook-up .tap image formats (.rom too?);
	- Implement tape commands;
	- Sort out / redump the BIOS gfx roms;
	- Implement the interrupts (uses IM 2), basically used by the keyboard.
	- Implement DMA,CTC and SIO, they are X1Turbo only features.
	- Improve the z80DMA core, it's so incomplete that nothing that uses it works;
	- clean-ups!
	- x1turbo: understand how irq generation works for the ym-2151;
	- There are various unclear video things, these are:
		- Understand why some games still doesn't upload the proper PCG index;
		- Implement PCG reading, used by Maison Ikkoku, Mule and Gyajiko for kanji reading.
		  Is there a way to select which ROM to use?
		- Implement the scrn regs;
		- Interlace mode?
		- Implement the new features of the x1turbo, namely the 4096 color feature amongst other
		  things
		- (anything else?)

	per-game specific TODO:
	- Bosconian: title screen background is completely white because it reverts the pen used
	  (it's gray in the Arcade version),could be either flickering for pseudo-alpha effect or it's
	  a btanb;
	- Mappy/Gradius: they sets 320x200 with fps = 30 Hz, due of that they are too slow (they must run at 60 Hz),
	  HW limitation/MC6845 bug?
	- Exoa II - Warroid: major tile width/height positioning bugs (especially during gameplay);

	Notes:
	- An interesting feature of the Sharp X-1 is the extended i/o bank. When the ppi port c bit 5
	  does a 1->0 transition, any write to the i/o space accesses 2 or 3 banks gradients of the bitmap RAM
	  with a single write (generally used for layer clearances and bitmap-style sprites).
	  Any i/o read disables this extended bitmap ram.

=================================================================================================

	X1 (CZ-800C) - November, 1982
	 * CPU: z80A @ 4MHz, 80C49 x 2 (one for key scan, the other for TV & Cas Ctrl)
	 * ROM: IPL (4KB) + chargen (2KB)
	 * RAM: Main memory (64KB) + VRAM (4KB) + RAM for PCG (6KB) + GRAM (48KB, Option)
	 * Text Mode: 80x25 or 40x25
	 * Graphic Mode: 640x200 or 320x200, 8 colors
	 * Sound: PSG 8 octave
	 * I/O Ports: Centronic ports, 2 Joystick ports, Cassette port (2700 baud)

	X1C (CZ-801C) - October, 1983
	 * same but only 48KB GRAM

	X1D (CZ-802C) - October, 1983
	 * same as X1C but with a 3" floppy drive (notice: 3" not 3" 1/2!!)

	X1Cs (CZ-803C) - June, 1984
	 * two expansion I/O ports

	X1Ck (CZ-804C) - June, 1984
	 * same as X1Cs
	 * ROM: IPL (4KB) + chargen (2KB) + Kanji 1st level

	X1F Model 10 (CZ-811C) - July, 1985
	 * Re-designed
	 * ROM: IPL (4KB) + chargen (2KB)

	X1F Model 20 (CZ-812C) - July, 1985
	 * Re-designed (same as Model 10)
	 * ROM: IPL (4KB) + chargen (2KB) + Kanji
	 * Built Tape drive plus a 5" floppy drive was available

	X1G Model 10 (CZ-820C) - July, 1986
	 * Re-designed again
	 * ROM: IPL (4KB) + chargen (2KB)

	X1G Model 30 (CZ-822C) - July, 1986
	 * Re-designed again (same as Model 10)
	 * ROM: IPL (4KB) + chargen (2KB) + Kanji
	 * Built Tape drive plus a 5" floppy drive was available

	X1twin (CZ-830C) - December, 1986
	 * Re-designed again (same as Model 10)
	 * ROM: IPL (4KB) + chargen (2KB) + Kanji
	 * Built Tape drive plus a 5" floppy drive was available
	 * It contains a PC-Engine

	=============  X1 Turbo series  =============

	X1turbo Model 30 (CZ-852C) - October, 1984
	 * CPU: z80A @ 4MHz, 80C49 x 2 (one for key scan, the other for TV & Cas Ctrl)
	 * ROM: IPL (32KB) + chargen (8KB) + Kanji (128KB)
	 * RAM: Main memory (64KB) + VRAM (6KB) + RAM for PCG (6KB) + GRAM (96KB)
	 * Text Mode: 80xCh or 40xCh with Ch = 10, 12, 20, 25 (same for Japanese display)
	 * Graphic Mode: 640x200 or 320x200, 8 colors
	 * Sound: PSG 8 octave
	 * I/O Ports: Centronic ports, 2 Joystick ports, built-in Cassette interface,
		2 Floppy drive for 5" disks, two expansion I/O ports

	X1turbo Model 20 (CZ-851C) - October, 1984
	 * same as Model 30, but only 1 Floppy drive is included

	X1turbo Model 10 (CZ-850C) - October, 1984
	 * same as Model 30, but Floppy drive is optional and GRAM is 48KB (it can
		be expanded to 96KB however)

	X1turbo Model 40 (CZ-862C) - July, 1985
	 * same as Model 30, but uses tv screen (you could watch television with this)

	X1turboII (CZ-856C) - November, 1985
	 * same as Model 30, but restyled, cheaper and sold with utility software

	X1turboIII (CZ-870C) - November, 1986
	 * with two High Density Floppy driver

	X1turboZ (CZ-880C) - December, 1986
	 * CPU: z80A @ 4MHz, 80C49 x 2 (one for key scan, the other for TV & Cas Ctrl)
	 * ROM: IPL (32KB) + chargen (8KB) + Kanji 1st & 2nd level
	 * RAM: Main memory (64KB) + VRAM (6KB) + RAM for PCG (6KB) + GRAM (96KB)
	 * Text Mode: 80xCh or 40xCh with Ch = 10, 12, 20, 25 (same for Japanese display)
	 * Graphic Mode: 640x200 or 320x200, 8 colors [in compatibility mode],
		640x400, 8 colors (out of 4096); 320x400, 64 colors (out of 4096);
		320x200, 4096 colors [in multimode],
	 * Sound: PSG 8 octave + FM 8 octave
	 * I/O Ports: Centronic ports, 2 Joystick ports, built-in Cassette interface,
		2 Floppy drive for HD 5" disks, two expansion I/O ports

	X1turboZII (CZ-881C) - December, 1987
	 * same as turboZ, but added 64KB expansion RAM

	X1turboZIII (CZ-888C) - December, 1988
	 * same as turboZII, but no more built-in cassette drive

	Please refer to http://www2s.biglobe.ne.jp/~ITTO/x1/x1menu.html for
	more info

	BASIC has to be loaded from external media (tape or disk), the
	computer only has an Initial Program Loader (IPL)

************************************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80ctc.h"
#include "machine/z80sio.h"
#include "machine/8255ppi.h"
#include "machine/z80dma.h"
#include "sound/ay8910.h"
#include "video/mc6845.h"
#include "sound/2151intf.h"
#include "sound/wave.h"

#include "machine/wd17xx.h"
#include "devices/cassette.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "formats/fm7_dsk.h"
#include "formats/x1_tap.h"
#include <ctype.h>

static UINT8 hres_320,io_switch,io_sys,vsync,vdisp;
static UINT8 io_bank_mode;
static UINT8 *gfx_bitmap_ram;
static UINT16 pcg_index[3];
static UINT8 pcg_reset;
static UINT16 crtc_start_addr;
static UINT8 tile_height;
static UINT8 sub_obf;
static UINT8 key_irq_flag;
static UINT8 ctc_irq_flag;
static struct
{
	UINT8 gfx_bank;
	UINT8 disp_bank;
	UINT8 pcg_mode;
	UINT8 v400_mode;

	UINT8 ply;
	UINT8 blackclip; // x1 turbo specific
}scrn_reg;

static struct
{
	UINT8 pal;
	UINT8 gfx_pal;
	UINT8 txt_pal;
	UINT8 txt_disp;
}turbo_reg;

static UINT8 pcg_write_addr;

static DEVICE_START(x1_daisy){}

/*************************************
 *
 *  Video Functions
 *
 *************************************/

static VIDEO_START( x1 )
{
	colorram = auto_alloc_array(machine, UINT8, 0x1000);
	videoram = auto_alloc_array(machine, UINT8, 0x1000);
	gfx_bitmap_ram = auto_alloc_array(machine, UINT8, 0xc000*2);
}

static void draw_fgtilemap(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int w)
{
	int y,x,res_x,res_y;
	int screen_mask;

	screen_mask = (w == 2) ? 0xfff : 0x7ff;

	for (y=0;y<50;y++)
	{
		for (x=0;x<40*w;x++)
		{
			int tile = videoram[(x+(y*40*w)+crtc_start_addr) & screen_mask];
			int color = colorram[(x+(y*40*w)+crtc_start_addr) & screen_mask] & 0x1f;
			int width = (colorram[(x+(y*40*w)+crtc_start_addr) & screen_mask] & 0x80)>>7;
			int height = (colorram[(x+(y*40*w)+crtc_start_addr) & screen_mask] & 0x40)>>6;
			int pcg_bank = (colorram[(x+(y*40*w)+crtc_start_addr) & screen_mask] & 0x20)>>5;
			UINT8 *gfx_data = pcg_bank ? memory_region(machine, "pcg") : memory_region(machine, "cgrom");

			/* skip draw if the x/y values are odd and the width/height is active, */
			/* behaviour confirmed by Black Onyx title screen and the X1Demo */
			if((x & 1) == 1 && width) continue;
			if((y & 1) == 1 && height) continue;

			res_x = (x/(width+1))*8*(width+1);
			res_y = (y/(height+1))*(tile_height+1)*(height+1);

			{
				int pen[3],pen_mask,pcg_pen,xi,yi;

				pen_mask = color & 7;

				/* We use custom drawing code due of the bitplane disable and the color revert stuff. */
				for(yi=0;yi<(tile_height+1)*(height+1);yi+=(height+1))
				{
					for(xi=0;xi<8*(width+1);xi+=(width+1))
					{
						pen[0] = gfx_data[((tile*8)+yi/(height+1))+0x0000]>>(7-xi/(width+1)) & (pen_mask & 1)>>0;
						pen[1] = gfx_data[((tile*8)+yi/(height+1))+0x0800]>>(7-xi/(width+1)) & (pen_mask & 2)>>1;
						pen[2] = gfx_data[((tile*8)+yi/(height+1))+0x1000]>>(7-xi/(width+1)) & (pen_mask & 4)>>2;

						pcg_pen = pen[2]<<2|pen[1]<<1|pen[0]<<0;

						if(color & 0x10 && 	video_screen_get_frame_number(machine->primary_screen) & 0x10) //reverse flickering
							pcg_pen^=7;

						if(pcg_pen == 0 && (!(color & 8)))
							continue;

						if(color & 8) //revert the used color pen
							pcg_pen^=7;

						if((scrn_reg.blackclip & 8) && (color == (scrn_reg.blackclip & 7)))
							pcg_pen = 0; // clip the pen to black

						if((res_x+xi)>video_screen_get_visible_area(machine->primary_screen)->max_x && (res_y+yi)>video_screen_get_visible_area(machine->primary_screen)->max_y)
							continue;

						if(scrn_reg.v400_mode)
						{
							*BITMAP_ADDR16(bitmap, (res_y+yi)*2+0, res_x+xi) = machine->pens[pcg_pen];
							*BITMAP_ADDR16(bitmap, (res_y+yi)*2+1, res_x+xi) = machine->pens[pcg_pen];
							if(width)
							{
								*BITMAP_ADDR16(bitmap, (res_y+yi)*2+0, res_x+xi+1) = machine->pens[pcg_pen];
								*BITMAP_ADDR16(bitmap, (res_y+yi)*2+1, res_x+xi+1) = machine->pens[pcg_pen];
							}
							if(height)
							{
								*BITMAP_ADDR16(bitmap, (res_y+yi+1)*2+0, res_x+xi) = machine->pens[pcg_pen];
								*BITMAP_ADDR16(bitmap, (res_y+yi+1)*2+1, res_x+xi) = machine->pens[pcg_pen];
							}
							if(width && height)
							{
								*BITMAP_ADDR16(bitmap, (res_y+yi+1)*2+0, res_x+xi+1) = machine->pens[pcg_pen];
								*BITMAP_ADDR16(bitmap, (res_y+yi+1)*2+1, res_x+xi+1) = machine->pens[pcg_pen];
							}
						}
						else
						{
							*BITMAP_ADDR16(bitmap, res_y+yi, res_x+xi) = machine->pens[pcg_pen];
							if(width)
								*BITMAP_ADDR16(bitmap, res_y+yi, res_x+xi+1) = machine->pens[pcg_pen];
							if(height)
								*BITMAP_ADDR16(bitmap, res_y+yi+1, res_x+xi) = machine->pens[pcg_pen];
							if(width && height)
								*BITMAP_ADDR16(bitmap, res_y+yi+1, res_x+xi+1) = machine->pens[pcg_pen];
						}
					}
				}
			}
		}
	}
}

/*
 * Priority Mixer Calculation (PLY)
 *
 * If ply is 0xff then the bitmap entirely covers the tilemap, if it's 0x00 then
 * the tilemap priority is entirely above the bitmap. Any other value mixes the
 * bitmap and the tilemap priorities based on the pen value, bit 0 = entry 0 <-> bit 7 = entry 7
 * of the bitmap.
 *
 */
static int priority_mixer_ply(running_machine *machine,int color)
{
	int pri_i,pri_mask_calc;

	pri_i = 0;
	pri_mask_calc = 1;

	while(pri_i < 7)
	{
		if((color & 7) == pri_i)
			break;

		pri_i++;
		pri_mask_calc<<=1;
	}

	return pri_mask_calc;
}

static void draw_gfxbitmap(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int w,int plane,int pri)
{
	int xi,yi,x,y;
	int pen_r,pen_g,pen_b,color;
	int pri_mask_val;

	for (y=0;y<50;y++)
	{
		for(x=0;x<40*w;x++)
		{
			for(yi=0;yi<(tile_height+1);yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					pen_b = (gfx_bitmap_ram[(((x+(y*40*w)+yi*0x800)+crtc_start_addr) & 0x3fff)+0x0000+plane*0xc000]>>(7-xi)) & 1;
					pen_r = (gfx_bitmap_ram[(((x+(y*40*w)+yi*0x800)+crtc_start_addr) & 0x3fff)+0x4000+plane*0xc000]>>(7-xi)) & 1;
					pen_g = (gfx_bitmap_ram[(((x+(y*40*w)+yi*0x800)+crtc_start_addr) & 0x3fff)+0x8000+plane*0xc000]>>(7-xi)) & 1;

					color =  pen_g<<2 | pen_r<<1 | pen_b<<0;

					pri_mask_val = priority_mixer_ply(machine,color);
					if(pri_mask_val & pri) continue;

					if(scrn_reg.v400_mode)
					{
						if((x+xi)<=video_screen_get_visible_area(machine->primary_screen)->max_x && ((y+yi)*2+0)<=video_screen_get_visible_area(machine->primary_screen)->max_y)
							*BITMAP_ADDR16(bitmap, (y+yi)*2+0, x+xi) = machine->pens[color+0x100];
						if((x+xi)<=video_screen_get_visible_area(machine->primary_screen)->max_x && ((y+yi)*2+1)<=video_screen_get_visible_area(machine->primary_screen)->max_y)
							*BITMAP_ADDR16(bitmap, (y+yi)*2+1, x+xi) = machine->pens[color+0x100];
					}
					else
					{
						if((x+xi)<=video_screen_get_visible_area(machine->primary_screen)->max_x && (y+yi)<=video_screen_get_visible_area(machine->primary_screen)->max_y)
							*BITMAP_ADDR16(bitmap, y*(tile_height+1)+yi, x*8+xi) = machine->pens[color+0x100];
					}
				}
			}
		}
	}
}

#if 0
/* Old code for reference, to be removed soon */
static void draw_gfxbitmap(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int w,int plane,int pri)
{
	int count,xi,yi,x,y;
	int pen_r,pen_g,pen_b,color;
	int yi_size;
	int pri_mask_val;

	count = crtc_start_addr;

	yi_size = (w == 1) ? 16 : 8;

	for(yi=0;yi<yi_size;yi++)
	{
		for(y=0;y<200;y+=8)
		{
			for(x=0;x<(320*w);x+=8)
			{
				for(xi=0;xi<8;xi++)
				{
					pen_b = (gfx_bitmap_ram[count+0x0000+plane*0xc000]>>(7-xi)) & 1;
					pen_r = (gfx_bitmap_ram[count+0x4000+plane*0xc000]>>(7-xi)) & 1;
					pen_g = (gfx_bitmap_ram[count+0x8000+plane*0xc000]>>(7-xi)) & 1;

					color =  pen_g<<2 | pen_r<<1 | pen_b<<0;

					pri_mask_val = priority_mixer_ply(machine,color);
					if(pri_mask_val & pri) continue;

					if(w == 1)
					{
						if(yi & 1)
							continue;

						if(scrn_reg.v400_mode)
						{
							if((x+xi)<=video_screen_get_visible_area(machine->primary_screen)->max_x && (y+(yi >> 1)*2+0)<=video_screen_get_visible_area(machine->primary_screen)->max_y)
								*BITMAP_ADDR16(bitmap, (y+(yi >> 1))*2+0, x+xi) = machine->pens[color+0x100];
							if((x+xi)<=video_screen_get_visible_area(machine->primary_screen)->max_x && (y+(yi >> 1)*2+1)<=video_screen_get_visible_area(machine->primary_screen)->max_y)
								*BITMAP_ADDR16(bitmap, (y+(yi >> 1))*2+1, x+xi) = machine->pens[color+0x100];
						}
						else
						{
							if((x+xi)<=video_screen_get_visible_area(machine->primary_screen)->max_x && (y+(yi >> 1))<=video_screen_get_visible_area(machine->primary_screen)->max_y)
								*BITMAP_ADDR16(bitmap, y+(yi >> 1), x+xi) = machine->pens[color+0x100];
						}
					}
					else
					{
						if(scrn_reg.v400_mode)
						{
							if((x+xi)<=video_screen_get_visible_area(machine->primary_screen)->max_x && ((y+yi)*2+0)<=video_screen_get_visible_area(machine->primary_screen)->max_y)
								*BITMAP_ADDR16(bitmap, (y+yi)*2+0, x+xi) = machine->pens[color+0x100];
							if((x+xi)<=video_screen_get_visible_area(machine->primary_screen)->max_x && ((y+yi)*2+1)<=video_screen_get_visible_area(machine->primary_screen)->max_y)
								*BITMAP_ADDR16(bitmap, (y+yi)*2+1, x+xi) = machine->pens[color+0x100];
						}
						else
						{
							if((x+xi)<=video_screen_get_visible_area(machine->primary_screen)->max_x && (y+yi)<=video_screen_get_visible_area(machine->primary_screen)->max_y)
								*BITMAP_ADDR16(bitmap, y+yi, x+xi) = machine->pens[color+0x100];
						}
					}
				}
				count++;
			}
		}
		count+= (w == 2) ? 48 : 24;
		count&=0x3fff;
	}
}
#endif

static VIDEO_UPDATE( x1 )
{
	int w;

	w = (video_screen_get_width(screen) < 640) ? 1 : 2;

	bitmap_fill(bitmap, cliprect, MAKE_ARGB(0xff,0x00,0x00,0x00));

	draw_gfxbitmap(screen->machine,bitmap,cliprect,w,scrn_reg.disp_bank,scrn_reg.ply);
	draw_fgtilemap(screen->machine,bitmap,cliprect,w);
	draw_gfxbitmap(screen->machine,bitmap,cliprect,w,scrn_reg.disp_bank,scrn_reg.ply^0xff);
	/* Y's uses the following as a normal buffer/work ram without anything reasonable to draw */
//	draw_gfxbitmap(screen->machine,bitmap,cliprect,w,1);

	return 0;
}

static VIDEO_EOF( x1 )
{
}

/*************************************
 *
 *  Keyboard MCU simulation
 *
 *************************************/

static UINT8 sub_cmd;//,key_flag;
static UINT8 sub_cmd_length;
static UINT8 sub_val[4];

static UINT8 check_keyboard_press(running_machine *machine)
{
	const char* portnames[3] = { "key1","key2","key3" };
	int i,port_i,scancode;
	UINT8 keymod = input_port_read(machine,"key_modifiers") & 0x1f;
	UINT32 pad = input_port_read(machine,"tenkey");
	scancode = 0;

	for(port_i=0;port_i<3;port_i++)
	{
		for(i=0;i<32;i++)
		{
			if((input_port_read(machine,portnames[port_i])>>i) & 1)
			{
				//key_flag = 1;
				if(keymod & 0x02)  // shift not pressed
				{
					if(scancode >= 0x41 && scancode < 0x5a)
						scancode += 0x20;  // lowercase
				}
				else
				{
					if(scancode >= 0x31 && scancode < 0x3a)
						scancode -= 0x10;
				}
				return scancode;
			}
			scancode++;
		}
	}

	// check numpad
	scancode = 0x30;
	for(i=0;i<10;i++)
	{
		if((pad >> i) & 0x01)
		{
			return scancode;
		}
		scancode++;
	}

	return 0;
}

static UINT8 check_keyboard_shift(running_machine *machine)
{
	UINT8 val = 0xe0;
	val |= input_port_read(machine,"key_modifiers") & 0x1f;

	if(check_keyboard_press(machine) != 0)
		val &= ~0x40;

	return val;
}

static UINT8 get_game_key(running_machine *machine, int port)
{
	// key status returned by sub CPU function 0xE3.
	// in order from bit 7 to 0:
	// port 0: Q,W,E,A,D,Z,X,C
	// port 1: numpad 7,4,1,8,2,9,6,3
	// port 2: ESC,1,[-],[+],[*],TAB,SPC,RET ([] = numpad)
	// bits are active high
	UINT8 ret = 0;
	UINT32 key1 = input_port_read(machine,"key1");
	UINT32 key2 = input_port_read(machine,"key2");
	UINT32 key3 = input_port_read(machine,"key3");
	UINT32 pad = input_port_read(machine,"tenkey");

	switch(port)
	{
		case 0:
			if(key3 & 0x00020000) ret |= 0x80;  // Q
			if(key3 & 0x00800000) ret |= 0x40;  // W
			if(key3 & 0x00000020) ret |= 0x20;  // E
			if(key3 & 0x00000002) ret |= 0x10;  // A
			if(key3 & 0x00000010) ret |= 0x08;  // D
			if(key3 & 0x04000000) ret |= 0x04;  // Z
			if(key3 & 0x01000000) ret |= 0x02;  // X
			if(key3 & 0x00000008) ret |= 0x01;  // C
			break;
		case 1:
			if(pad & 0x00000080) ret |= 0x80;  // Tenkey 7
			if(pad & 0x00000010) ret |= 0x40;  // Tenkey 4
			if(pad & 0x00000002) ret |= 0x20;  // Tenkey 1
			if(pad & 0x00000100) ret |= 0x10;  // Tenkey 8
			if(pad & 0x00000004) ret |= 0x08;  // Tenkey 2
			if(pad & 0x00000200) ret |= 0x04;  // Tenkey 9
			if(pad & 0x00000040) ret |= 0x02;  // Tenkey 6
			if(pad & 0x00000008) ret |= 0x01;  // Tenkey 3
			break;
		case 2:
			if(key1 & 0x10000000) ret |= 0x80;  // ESC
			if(key2 & 0x00020000) ret |= 0x40;  // 1
			if(pad & 0x00000400) ret |= 0x20;  // Tenkey -
			if(pad & 0x00000800) ret |= 0x10;  // Tenkey +
			if(pad & 0x00001000) ret |= 0x08;  // Tenkey *
			if(key1 & 0x00000200) ret |= 0x04;  // TAB
			if(key2 & 0x00000001) ret |= 0x02;  // SPC
			if(key1 & 0x00002000) ret |= 0x01;  // RET
			break;
	}

	return ret;
}

static READ8_HANDLER( sub_io_r )
{
	static int sub_val_ptr,key_i = 0;
	UINT8 ret,bus_res;

	/* Looks like that the HW retains the latest data putted on the bus here, behaviour confirmed by Rally-X */
	if(sub_obf)
	{
		bus_res = sub_val[key_i];
		/* FIXME: likely to be different here. */
		key_i++;
		if(key_i >= 2) { key_i = 0; }

		return bus_res;
	}

	/*if(key_flag == 1)
	{
		key_flag = 0;
		return 0x82; //TODO: this is for shift/ctrl/kana lock etc.
	}*/

	sub_cmd_length--;
	sub_obf = (sub_cmd_length) ? 0x00 : 0x20;

	ret = sub_val[sub_val_ptr];

	sub_val_ptr++;
	if(sub_cmd_length <= 0)
		sub_val_ptr = 0;

	return ret;
}

static UINT8 x1_irq_vector;
//static UINT8 ctc_irq_vector; //always 0x5e?
static UINT8 key_irq_vector;
static UINT8 cmt_current_cmd;
static UINT8 cmt_test;

static void cmt_command( running_machine* machine, UINT8 cmd )
{
	// CMT deck control command (E9 xx)
	// E9 00 - Eject
	// E9 01 - Stop
	// E9 02 - Play
	// E9 03 - Fast Forward
	// E9 04 - Rewind
	// E9 05 - APSS(?) Fast Forward
	// E9 06 - APSS(?) Rewind
	// E9 0A - Record
	cmt_current_cmd = cmd;

	switch(cmd)
	{
		case 0x01:  // Stop
			cassette_change_state(devtag_get_device(machine, "cass" ),CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
			cassette_change_state(devtag_get_device(machine, "cass" ),CASSETTE_STOPPED,CASSETTE_MASK_UISTATE);
			cmt_test = 1;
			break;
		case 0x02:  // Play
			cassette_change_state(devtag_get_device(machine, "cass" ),CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
			cassette_change_state(devtag_get_device(machine, "cass" ),CASSETTE_PLAY,CASSETTE_MASK_UISTATE);
			break;
		case 0x0a:  // Record
			cassette_change_state(devtag_get_device(machine, "cass" ),CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
			cassette_change_state(devtag_get_device(machine, "cass" ),CASSETTE_RECORD,CASSETTE_MASK_UISTATE);
			break;
		default:
			logerror("Unimplemented or invalid CMT command (0x%02x)\n",cmd);
	}
	logerror("CMT: Command 0xe9-0x%02x received.\n",cmd);
}

static WRITE8_HANDLER( sub_io_w )
{
	mame_system_time systime;
	mame_get_base_datetime(space->machine, &systime);
	/* TODO: check sub-routine at $10e */
	/* $17a -> floppy? */
	/* $094 -> ROM */
	/* $0c0 -> timer*/
	/* $052 -> cmt */
	/* $0f5 -> reload sub-routine? */

	if(sub_cmd == 0xe4)
	{
		key_irq_vector = data;
		logerror("Key vector set to 0x%02x\n",data);
	}

	if(sub_cmd == 0xe9)
		cmt_command(space->machine,data);

	switch(data)
	{
		case 0xe3:
			sub_cmd_length = 3;
			sub_val[0] = get_game_key(space->machine,0);
			sub_val[1] = get_game_key(space->machine,1);
			sub_val[2] = get_game_key(space->machine,2);
			break;
		case 0xe6:
			sub_val[0] = check_keyboard_shift(space->machine);
			sub_val[1] = check_keyboard_press(space->machine);
			sub_cmd_length = 2;
			break;
		case 0xe8:
			sub_val[0] = sub_cmd;
			sub_cmd_length = 1;
			break;
		case 0xea:  // CMT Control status
			sub_val[0] = cmt_current_cmd;
			sub_cmd_length = 1;
			logerror("CMT: Command 0xEA received, returning 0x%02x.\n",sub_val[0]);
			break;
		case 0xeb:  // CMT Tape status
		            // bit 0 = tape end (0=end of tape)
					// bit 1 = tape inserted
					// bit 2 = record status (1=OK, 0=write protect?)
			sub_val[0] = 0x05;
			if(cassette_get_image(devtag_get_device(space->machine,"cass")) != NULL)
				sub_val[0] |= 0x02;
			sub_cmd_length = 1;
			logerror("CMT: Command 0xEB received, returning 0x%02x.\n",sub_val[0]);
			break;
		case 0xed: //get date
			sub_val[0] = ((systime.local_time.mday / 10)<<4) | ((systime.local_time.mday % 10) & 0xf);
			sub_val[1] = ((systime.local_time.month+1)<<4) | ((systime.local_time.weekday % 10) & 0xf);
			sub_val[2] = (((systime.local_time.year % 100)/10)<<4) | ((systime.local_time.year % 10) & 0xf);
			sub_cmd_length = 3;
			break;
		case 0xef: //get time
			sub_val[0] = ((systime.local_time.hour / 10)<<4) | ((systime.local_time.hour % 10) & 0xf);
			sub_val[1] = ((systime.local_time.minute / 10)<<4) | ((systime.local_time.minute % 10) & 0xf);
			sub_val[2] = ((systime.local_time.second / 10)<<4) | ((systime.local_time.second % 10) & 0xf);
			sub_cmd_length = 3;
			break;
		#if 0
		default:
		{
			if(data > 0xe3 && data <= 0xef)
				printf("%02x CMD\n",data);
		}
		#endif
	}

	sub_cmd = data;

	sub_obf = (sub_cmd_length) ? 0x00 : 0x20;
	logerror("SUB: Command byte 0x%02x\n",data);
}

static int x1_keyboard_irq_state(const device_config* device)
{
	if(key_irq_flag != 0)
		return Z80_DAISY_INT;

	return 0;
}

static int x1_keyboard_irq_ack(const device_config* device)
{
	key_irq_flag = 0;
	cputag_set_input_line(device->machine,"maincpu",INPUT_LINE_IRQ0,CLEAR_LINE);
	return key_irq_vector;
}

static DEVICE_GET_INFO( x1_keyboard_getinfo )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = 4;											break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;											break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;						break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(x1_daisy);		break;
		case DEVINFO_FCT_IRQ_STATE:						info->f = (genf *)x1_keyboard_irq_state;	break;
		case DEVINFO_FCT_IRQ_ACK:						info->f = (genf *)x1_keyboard_irq_ack;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "X1 keyboard");		break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "X1 daisy chain");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");									break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);								break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");				break;
	}
}

/*************************************
 *
 *  ROM Image / Banking Handling
 *
 *************************************/

static UINT8 rom_index[3];

static READ8_HANDLER( x1_rom_r )
{
	//UINT8 *ROM = memory_region(space->machine, "rom");

//	popmessage("%06x",rom_index[0]<<16|rom_index[1]<<8|rom_index[2]<<0);

	return 0; //ROM[rom_index[0]<<16|rom_index[1]<<8|rom_index[2]<<0];
}

static WRITE8_HANDLER( x1_rom_w )
{
	rom_index[offset] = data;
}

static WRITE8_HANDLER( rom_bank_0_w )
{
	UINT8 *ROM = memory_region(space->machine, "maincpu");

	memory_set_bankptr(space->machine, 1, &ROM[0x10000]);
}

static WRITE8_HANDLER( rom_bank_1_w )
{
	UINT8 *ROM = memory_region(space->machine, "maincpu");

	memory_set_bankptr(space->machine, 1, &ROM[0x00000]);
}

static WRITE8_HANDLER( rom_data_w )
{
	UINT8 *ROM = memory_region(space->machine, "maincpu");

	ROM[0x10000+offset] = data;
}


/*************************************
 *
 *  MB8877A FDC (wd17XX compatible)
 *
 *************************************/

//static UINT8 fdc_irq_flag;
//static UINT8 fdc_drq_flag;
//static UINT8 fdc_side;
//static UINT8 fdc_drive;

static WD17XX_CALLBACK( x1_fdc_irq )
{
	#if 0
	switch(state)
	{
		case WD17XX_IRQ_CLR:
			fdc_irq_flag = 0;
			break;
		case WD17XX_IRQ_SET:
			fdc_irq_flag = 1;
			break;
		case WD17XX_DRQ_CLR:
			fdc_drq_flag = 0;
			break;
		case WD17XX_DRQ_SET:
			fdc_drq_flag = 1;
			break;
	}
	#endif
}

static READ8_HANDLER( x1_fdc_r )
{
	const device_config* dev = devtag_get_device(space->machine,"fdc");
	//UINT8 ret = 0;

	switch(offset+0xff8)
	{
		case 0x0ff8:
			return wd17xx_status_r(dev,offset);
		case 0x0ff9:
			return wd17xx_track_r(dev,offset);
		case 0x0ffa:
			return wd17xx_sector_r(dev,offset);
		case 0x0ffb:
			return wd17xx_data_r(dev,offset);
		case 0x0ffc:
		case 0x0ffd:
		case 0x0ffe:
		case 0x0fff:
			logerror("FDC: read from %04x\n",offset+0xff8);
			return 0xff;
	}

	return 0x00;
}

static WRITE8_HANDLER( x1_fdc_w )
{
	const device_config* dev = devtag_get_device(space->machine,"fdc");

	switch(offset+0xff8)
	{
		case 0x0ff8:
			wd17xx_command_w(dev,offset,data);
			break;
		case 0x0ff9:
			wd17xx_track_w(dev,offset,data);
			break;
		case 0x0ffa:
			wd17xx_sector_w(dev,offset,data);
			break;
		case 0x0ffb:
			wd17xx_data_w(dev,offset,data);
			break;
		case 0x0ffc:
			wd17xx_set_drive(dev,data & 3);
			floppy_drive_set_motor_state(floppy_get_device(space->machine, data & 3), data & 0x80);
			floppy_drive_set_ready_state(floppy_get_device(space->machine, data & 3), data & 0x80,0);
			wd17xx_set_side(dev,(data & 0x10)>>4);
			break;
		case 0x0ffd:
		case 0x0ffe:
		case 0x0fff:
			logerror("FDC: write to %04x = %02x\n",offset+0xff8,data);
			break;
	}
}

static const wd17xx_interface x1_mb8877a_interface =
{
	x1_fdc_irq,
	NULL,
	{FLOPPY_0,FLOPPY_1,FLOPPY_2,FLOPPY_3}
};

/*************************************
 *
 *  Programmable Character Generator
 *
 *************************************/

static UINT16 check_pcg_addr(running_machine *machine)
{
	if(colorram[0x7ff] & 0x20) return 0x7ff;
	if(colorram[0x3ff] & 0x20) return 0x3ff;
	if(colorram[0x5ff] & 0x20) return 0x5ff;
	if(colorram[0x1ff] & 0x20) return 0x1ff;

	return 0x3ff;
}

static READ8_HANDLER( x1_pcg_r )
{
	int addr;
	int calc_pcg_offset;
	static UINT8 bios_offset,pcg_index_r[3],res;
	UINT8 *gfx_data;

	addr = (offset & 0x300) >> 8;

	if(addr == 0)
	{
		gfx_data = memory_region(space->machine, "cgrom");
		res = gfx_data[0x0000+bios_offset+(pcg_write_addr*8)];

		bios_offset++;
		bios_offset&=0x7;
		return res;
	}
	else
	{
 		gfx_data = memory_region(space->machine, "pcg");
 		calc_pcg_offset = (pcg_index_r[addr-1]) | ((addr-1)*0x800);
 		res = gfx_data[0x0000+calc_pcg_offset+(pcg_write_addr*8)];

 		pcg_index_r[addr-1]++;
		pcg_index_r[addr-1]&=0x7;
 		return res;
 	}

	return mame_rand(space->machine);
}

static WRITE8_HANDLER( x1_pcg_w )
{
	int addr,pcg_offset;
	UINT8 *PCG_RAM = memory_region(space->machine, "pcg");
	static UINT16 used_pcg_addr;

	addr = (offset & 0x300) >> 8;

	if(pcg_reset)
	{
		pcg_index[0] = pcg_index[1] = pcg_index[2] = 0;
		pcg_reset = 0;
	}

	if(addr == 0)
	{
		/* NOP */
		logerror("Warning: write to the BIOS PCG area! %04x %02x\n",offset,data);
	}
	else
	{
		if(scrn_reg.pcg_mode)
		{
			used_pcg_addr = videoram[check_pcg_addr(space->machine)]*8;
			pcg_index[addr-1] = (offset & 0xe) >> 1;
			pcg_offset = (pcg_index[addr-1]+used_pcg_addr) & 0x7ff;
			pcg_offset+=((addr-1)*0x800);
			PCG_RAM[pcg_offset] = data;

			pcg_offset &= 0x7ff;

    		gfx_element_mark_dirty(space->machine->gfx[1], pcg_offset >> 3);
		}
		else
		{
			/* TODO: understand what 1942, Herzog and friends wants there */
			used_pcg_addr = pcg_write_addr*8;
			pcg_offset = (pcg_index[addr-1]+used_pcg_addr) & 0x7ff;
			pcg_offset+=((addr-1)*0x800);
			PCG_RAM[pcg_offset] = data;

			pcg_offset &= 0x7ff;

    		gfx_element_mark_dirty(space->machine->gfx[1], pcg_offset >> 3);

  			pcg_index[addr-1]++;
  			pcg_index[addr-1]&=7;
		}
	}
}

/*************************************
 *
 *  Other Video-related functions
 *
 *************************************/

/* for bitmap mode */
static UINT8 x_b,x_g,x_r;

static void set_current_palette(running_machine *machine)
{
	UINT8 addr,r,g,b;

	for(addr=0;addr<8;addr++)
	{
		r = ((x_r)>>(addr)) & 1;
		g = ((x_g)>>(addr)) & 1;
		b = ((x_b)>>(addr)) & 1;

		palette_set_color_rgb(machine, addr+0x100, pal1bit(r), pal1bit(g), pal1bit(b));
	}
}

static WRITE8_HANDLER( x1_pal_r_w ) { x_r = data; set_current_palette(space->machine); }
static WRITE8_HANDLER( x1_pal_g_w ) { x_g = data; set_current_palette(space->machine); }
static WRITE8_HANDLER( x1_pal_b_w ) { x_b = data; set_current_palette(space->machine); }

static WRITE8_HANDLER( x1_ex_gfxram_w )
{
	static UINT8 ex_mask;

	if     (offset >= 0x0000 && offset <= 0x3fff)	{ ex_mask = 7; }
	else if(offset >= 0x4000 && offset <= 0x7fff)	{ ex_mask = 6; }
	else if(offset >= 0x8000 && offset <= 0xbfff)	{ ex_mask = 5; }
	else                                        	{ ex_mask = 3; }

	if(ex_mask & 1) { gfx_bitmap_ram[(offset & 0x3fff)+0x0000+(scrn_reg.gfx_bank*0xc000)] = data; }
	if(ex_mask & 2) { gfx_bitmap_ram[(offset & 0x3fff)+0x4000+(scrn_reg.gfx_bank*0xc000)] = data; }
	if(ex_mask & 4) { gfx_bitmap_ram[(offset & 0x3fff)+0x8000+(scrn_reg.gfx_bank*0xc000)] = data; }
}

/*
	SCRN flags

	d0(01) = 0:low res            1:high res
    d1(02) = 0:1 raster           1:2 raster  <- 0=400-line mode, legal when 'd0=1'
    d2(04) = 0:25(20) lines       1:12(10) lines
    d3(08) = 0:bank 0             0:bank 1    <- display
    d4(10) = 0:bank 0             0:bank 1    <- access
    d5(20) = 0:compatibility      1:high speed  <- define PCG mode
    d6(40) = 0:8-raster graphics  1:16-raster graphics
    d7(80) = 0:don't display      1:display  <- underline (when 1, graphics are not displayed)

    The value output by the X1turbo's IOCS at 0x1fd*
    is stored in memory at 0xf8d6
*/
static WRITE8_HANDLER( x1_scrn_w )
{
	scrn_reg.pcg_mode = (data & 0x20)>>5;
	scrn_reg.gfx_bank = (data & 0x10)>>4;
	scrn_reg.disp_bank = (data & 0x08)>>3;
	scrn_reg.v400_mode = ((data & 0x03) == 3) ? 1 : 0;
	if(data & 0xc4)
		printf("SCRN = %02x\n",data & 0xc4);
}

static WRITE8_HANDLER( x1_ply_w )
{
	scrn_reg.ply = data;
//	printf("PLY = %02x\n",data);
}

static WRITE8_HANDLER( x1_6845_w )
{
	static int addr_latch;

	if(offset == 0)
	{
		addr_latch = data;
		mc6845_address_w(devtag_get_device(space->machine, "crtc"), 0,data);
	}
	else
	{
		/* FIXME: this should be inside the MC6845 core! */
		if(addr_latch == 0x09)
			tile_height = data;
		if(addr_latch == 0x0c)
			crtc_start_addr = ((data<<8) & 0x3f00) | (crtc_start_addr & 0xff);
		else if(addr_latch == 0x0d)
			crtc_start_addr = (crtc_start_addr & 0x3f00) | (data & 0xff);

		mc6845_register_w(devtag_get_device(space->machine, "crtc"), 0,data);
	}
}

static READ8_HANDLER( x1_blackclip_r )
{
	return scrn_reg.blackclip;
}

static WRITE8_HANDLER( x1_blackclip_w )
{
	scrn_reg.blackclip = data;
}

static READ8_HANDLER( x1turbo_pal_r )    { return turbo_reg.pal; }
static READ8_HANDLER( x1turbo_txpal_r )  { return turbo_reg.txt_pal; }
static READ8_HANDLER( x1turbo_txdisp_r ) { return turbo_reg.txt_disp; }
static READ8_HANDLER( x1turbo_gfxpal_r ) { return turbo_reg.gfx_pal; }

static WRITE8_HANDLER( x1turbo_pal_w )
{
	printf("TURBO PAL %02x\n",data);
	turbo_reg.pal = data;
}

static WRITE8_HANDLER( x1turbo_txpal_w )
{
	printf("TURBO TEXT PAL %02x\n",data);
	turbo_reg.txt_pal = data;
}

static WRITE8_HANDLER( x1turbo_txdisp_w )
{
	printf("TURBO TEXT DISPLAY %02x\n",data);
	turbo_reg.txt_disp = data;
}

static WRITE8_HANDLER( x1turbo_gfxpal_w )
{
	printf("TURBO GFX PAL %02x\n",data);
	turbo_reg.gfx_pal = data;
}

static UINT8 kanji_addr_l,kanji_addr_h,kanji_count;
static UINT32 kanji_addr;

/*
 *	FIXME: recheck this once that the kanji rom is redumped, it's confirmed bad for the following reasons:
 *	- A game lets you type stuff that's typed with this function, the index doesn't match with the typed char;
 *	- It search for chars that are out of the rom range;
 */
static READ8_HANDLER( x1_kanji_r )
{
	UINT8 *kanji_rom = memory_region(space->machine, "cgrom");
	UINT8 res;

	//res = 0;

	//printf("%08x\n",kanji_addr*0x20);

	res = kanji_rom[(kanji_addr*0x20)+(kanji_count)+0x2800];
	kanji_count++;
	kanji_count&=0x1f;

	//printf("%04x R\n",offset);

	return res;
}

static WRITE8_HANDLER( x1_kanji_w )
{
	//printf("%04x %02x W\n",offset,data);
	switch(offset)
	{
		case 0: kanji_addr_l = (data & 0xff); break;
		case 1: kanji_addr_h = (data & 0xff); kanji_count = 0; break;
		case 2:
			if(data)
			{
				kanji_addr = (kanji_addr_h<<8) | (kanji_addr_l);
				kanji_addr &= 0x3fff; //<- temp kludge until the rom is redumped.
				//printf("%08x\n",kanji_addr);
				//kanji_addr+= kanji_count;
			}
			break;
	}
}

/*************************************
 *
 *  Memory maps
 *
 *************************************/

static READ8_HANDLER( x1_io_r )
{
	io_bank_mode = 0; //any read disables the extended mode.

	//if(offset >= 0x0704 && offset <= 0x0707)   	{ return z80ctc_r(devtag_get_device(space->machine, "ctc"), offset-0x0704); }
	if(offset == 0x0e03)                    		{ return x1_rom_r(space, 0); }
	else if(offset >= 0x0ff8 && offset <= 0x0fff)	{ return x1_fdc_r(space, offset-0xff8); }
	else if(offset >= 0x1400 && offset <= 0x17ff)	{ return x1_pcg_r(space, offset-0x1400); }
	else if(offset >= 0x1900 && offset <= 0x19ff)	{ return sub_io_r(space, 0); }
	else if(offset >= 0x1a00 && offset <= 0x1aff)	{ return ppi8255_r(devtag_get_device(space->machine, "ppi8255_0"), (offset-0x1a00) & 3); }
	else if(offset >= 0x1b00 && offset <= 0x1bff)	{ return ay8910_r(devtag_get_device(space->machine, "ay"), 0); }
	else if(offset >= 0x1f80 && offset <= 0x1f8f)	{ return z80dma_r(devtag_get_device(space->machine, "dma"), 0); }
	else if(offset >= 0x1f90 && offset <= 0x1f91)	{ return z80sio_c_r(devtag_get_device(space->machine, "sio"), (offset-0x1f90) & 1); }
	else if(offset >= 0x1f92 && offset <= 0x1f93)	{ return z80sio_d_r(devtag_get_device(space->machine, "sio"), (offset-0x1f92) & 1); }
	else if(offset >= 0x1fa0 && offset <= 0x1fa3)	{ return z80ctc_r(devtag_get_device(space->machine, "ctc"), offset-0x1fa0); }
	else if(offset >= 0x1fa8 && offset <= 0x1fab)	{ return z80ctc_r(devtag_get_device(space->machine, "ctc"), offset-0x1fa8); }
//	else if(offset >= 0x1fd0 && offset <= 0x1fdf)	{ return x1_scrn_r(space,offset-0x1fd0); }
	else if(offset == 0x1fe0)						{ return x1_blackclip_r(space,0); }
	else if(offset >= 0x2000 && offset <= 0x2fff)	{ return colorram[offset-0x2000]; }
	else if(offset >= 0x3000 && offset <= 0x3fff)	{ return videoram[offset-0x3000]; }
	else if(offset >= 0x4000 && offset <= 0xffff)	{ return gfx_bitmap_ram[offset-0x4000+(scrn_reg.gfx_bank*0xc000)]; }
	else
	{
		logerror("(PC=%06x) Read i/o address %04x\n",cpu_get_pc(space->cpu),offset);
	}
	return 0xff;
}

static WRITE8_HANDLER( x1_io_w )
{
	if(io_bank_mode == 1)                        	{ x1_ex_gfxram_w(space, offset, data); }
//	else if(offset >= 0x0704 && offset <= 0x0707)	{ z80ctc_w(devtag_get_device(space->machine, "ctc"), offset-0x0704,data); }
//	else if(offset >= 0x0c00 && offset <= 0x0cff)	{ x1_rs232c_w(space->machine, 0, data); }
	else if(offset >= 0x0e00 && offset <= 0x0e02)  	{ x1_rom_w(space, offset-0xe00,data); }
//	else if(offset >= 0x0e80 && offset <= 0x0e82)	{ x1_kanji_w(space->machine, offset-0xe80,data); }
	else if(offset >= 0x0ff8 && offset <= 0x0fff)	{ x1_fdc_w(space, offset-0xff8,data); }
	else if(offset >= 0x1000 && offset <= 0x10ff)	{ x1_pal_b_w(space, 0,data); }
	else if(offset >= 0x1100 && offset <= 0x11ff)	{ x1_pal_r_w(space, 0,data); }
	else if(offset >= 0x1200 && offset <= 0x12ff)	{ x1_pal_g_w(space, 0,data); }
	else if(offset >= 0x1300 && offset <= 0x13ff)	{ x1_ply_w(space, 0,data); }
	else if(offset >= 0x1400 && offset <= 0x17ff)	{ x1_pcg_w(space, offset-0x1400,data); }
	else if(offset == 0x1800 || offset == 0x1801)	{ x1_6845_w(space, offset-0x1800, data); }
	else if(offset >= 0x1900 && offset <= 0x19ff)	{ sub_io_w(space, 0,data); }
	else if(offset >= 0x1a00 && offset <= 0x1aff)	{ ppi8255_w(devtag_get_device(space->machine, "ppi8255_0"), (offset-0x1a00) & 3,data); }
	else if(offset >= 0x1b00 && offset <= 0x1bff)	{ ay8910_data_w(devtag_get_device(space->machine, "ay"), 0,data); }
	else if(offset >= 0x1c00 && offset <= 0x1cff)	{ ay8910_address_w(devtag_get_device(space->machine, "ay"), 0,data); }
	else if(offset >= 0x1d00 && offset <= 0x1dff)	{ rom_bank_1_w(space,0,data); }
	else if(offset >= 0x1e00 && offset <= 0x1eff)	{ rom_bank_0_w(space,0,data); }
	else if(offset >= 0x1f80 && offset <= 0x1f8f)	{ z80dma_w(devtag_get_device(space->machine, "dma"), 0,data); }
	else if(offset >= 0x1f90 && offset <= 0x1f91)	{ z80sio_c_w(devtag_get_device(space->machine, "sio"), (offset-0x1f90) & 1,data); }
	else if(offset >= 0x1f92 && offset <= 0x1f93)	{ z80sio_d_w(devtag_get_device(space->machine, "sio"), (offset-0x1f92) & 1,data); }
	else if(offset >= 0x1fa0 && offset <= 0x1fa3)	{ z80ctc_w(devtag_get_device(space->machine, "ctc"), offset-0x1fa0,data); }
	else if(offset >= 0x1fa8 && offset <= 0x1fab)	{ z80ctc_w(devtag_get_device(space->machine, "ctc"), offset-0x1fa8,data); }
	else if(offset == 0x1fb0)						{ x1turbo_pal_w(space,0,data); }
	else if(offset >= 0x1fb9 && offset <= 0x1fbf)	{ x1turbo_txpal_w(space,offset-0x1fb9,data); }
	else if(offset == 0x1fc0)						{ x1turbo_txdisp_w(space,0,data); }
	else if(offset == 0x1fc5)						{ x1turbo_gfxpal_w(space,0,data); }
	else if(offset >= 0x1fd0 && offset <= 0x1fdf)	{ x1_scrn_w(space,0,data); }
	else if(offset == 0x1fe0)						{ x1_blackclip_w(space,0,data); }
	else if(offset >= 0x2000 && offset <= 0x2fff)	{ colorram[offset-0x2000] = data; }
	else if(offset >= 0x3000 && offset <= 0x3fff)	{ videoram[offset-0x3000] = pcg_write_addr = data; }
	else if(offset >= 0x4000 && offset <= 0xffff)	{ gfx_bitmap_ram[offset-0x4000+(scrn_reg.gfx_bank*0xc000)] = data; }
	else
	{
		logerror("(PC=%06x) Write %02x at i/o address %04x\n",cpu_get_pc(space->cpu),data,offset);
	}
}

static READ8_HANDLER( x1turbo_io_r )
{
	io_bank_mode = 0; //any read disables the extended mode.

	if(offset == 0x0700 || offset == 0x0701)		{ return ym2151_r(devtag_get_device(space->machine, "ym"), offset-0x0700); }
	else if(offset >= 0x0704 && offset <= 0x0707)   { return z80ctc_r(devtag_get_device(space->machine, "ctc"), offset-0x0704); }
	else if(offset == 0x0e03)                    	{ return x1_rom_r(space, 0); }
	else if(offset >= 0x0e80 && offset <= 0x0e83)	{ return x1_kanji_r(space, offset-0xe80); }
	else if(offset >= 0x0ff8 && offset <= 0x0fff)	{ return x1_fdc_r(space, offset-0xff8); }
	else if(offset >= 0x1400 && offset <= 0x17ff)	{ return x1_pcg_r(space, offset-0x1400); }
	else if(offset >= 0x1900 && offset <= 0x19ff)	{ return sub_io_r(space, 0); }
	else if(offset >= 0x1a00 && offset <= 0x1aff)	{ return ppi8255_r(devtag_get_device(space->machine, "ppi8255_0"), (offset-0x1a00) & 3); }
	else if(offset >= 0x1b00 && offset <= 0x1bff)	{ return ay8910_r(devtag_get_device(space->machine, "ay"), 0); }
	else if(offset >= 0x1f80 && offset <= 0x1f8f)	{ return z80dma_r(devtag_get_device(space->machine, "dma"), 0); }
	else if(offset >= 0x1f90 && offset <= 0x1f91)	{ return z80sio_c_r(devtag_get_device(space->machine, "sio"), (offset-0x1f90) & 1); }
	else if(offset >= 0x1f92 && offset <= 0x1f93)	{ return z80sio_d_r(devtag_get_device(space->machine, "sio"), (offset-0x1f92) & 1); }
	else if(offset >= 0x1fa0 && offset <= 0x1fa3)	{ return z80ctc_r(devtag_get_device(space->machine, "ctc"), offset-0x1fa0); }
	else if(offset >= 0x1fa8 && offset <= 0x1fab)	{ return z80ctc_r(devtag_get_device(space->machine, "ctc"), offset-0x1fa8); }
	else if(offset == 0x1fb0)						{ return x1turbo_pal_r(space,0); }
	else if(offset >= 0x1fb9 && offset <= 0x1fbf)	{ return x1turbo_txpal_r(space,offset-0x1fb9); }
	else if(offset == 0x1fc0)						{ return x1turbo_txdisp_r(space,0); }
	else if(offset == 0x1fc5)						{ return x1turbo_gfxpal_r(space,0); }
//	else if(offset >= 0x1fd0 && offset <= 0x1fdf)	{ return x1_scrn_r(space,offset-0x1fd0); }
	else if(offset == 0x1fe0)						{ return x1_blackclip_r(space,0); }
	else if(offset >= 0x2000 && offset <= 0x2fff)	{ return colorram[offset-0x2000]; }
	else if(offset >= 0x3000 && offset <= 0x3fff)	{ return videoram[offset-0x3000]; }
	else if(offset >= 0x4000 && offset <= 0xffff)	{ return gfx_bitmap_ram[offset-0x4000+(scrn_reg.gfx_bank*0xc000)]; }
	else
	{
		//0xfd0-0xfd7: FDC extended area?
		//0x1ff0: if 0, game screen is interlaced
		logerror("(PC=%06x) Read i/o address %04x\n",cpu_get_pc(space->cpu),offset);
	}
	return 0xff;
}

static WRITE8_HANDLER( x1turbo_io_w )
{
	if(io_bank_mode == 1)                        	{ x1_ex_gfxram_w(space, offset, data); }
	else if(offset == 0x0700 || offset == 0x0701)	{ ym2151_w(devtag_get_device(space->machine, "ym"), offset-0x0700,data); }
	else if(offset >= 0x0704 && offset <= 0x0707)	{ z80ctc_w(devtag_get_device(space->machine, "ctc"), offset-0x0704,data); }
//	else if(offset >= 0x0c00 && offset <= 0x0cff)	{ x1_rs232c_w(space->machine, 0, data); }
	else if(offset >= 0x0e00 && offset <= 0x0e02)  	{ x1_rom_w(space, offset-0xe00,data); }
	else if(offset >= 0x0e80 && offset <= 0x0e83)	{ x1_kanji_w(space, offset-0xe80,data); }
	else if(offset >= 0x0ff8 && offset <= 0x0fff)	{ x1_fdc_w(space, offset-0xff8,data); }
	else if(offset >= 0x1000 && offset <= 0x10ff)	{ x1_pal_b_w(space, 0,data); }
	else if(offset >= 0x1100 && offset <= 0x11ff)	{ x1_pal_r_w(space, 0,data); }
	else if(offset >= 0x1200 && offset <= 0x12ff)	{ x1_pal_g_w(space, 0,data); }
	else if(offset >= 0x1300 && offset <= 0x13ff)	{ x1_ply_w(space, 0,data); }
	else if(offset >= 0x1400 && offset <= 0x17ff)	{ x1_pcg_w(space, offset-0x1400,data); }
	else if(offset == 0x1800 || offset == 0x1801)	{ x1_6845_w(space, offset-0x1800, data); }
	else if(offset >= 0x1900 && offset <= 0x19ff)	{ sub_io_w(space, 0,data); }
	else if(offset >= 0x1a00 && offset <= 0x1aff)	{ ppi8255_w(devtag_get_device(space->machine, "ppi8255_0"), (offset-0x1a00) & 3,data); }
	else if(offset >= 0x1b00 && offset <= 0x1bff)	{ ay8910_data_w(devtag_get_device(space->machine, "ay"), 0,data); }
	else if(offset >= 0x1c00 && offset <= 0x1cff)	{ ay8910_address_w(devtag_get_device(space->machine, "ay"), 0,data); }
	else if(offset >= 0x1d00 && offset <= 0x1dff)	{ rom_bank_1_w(space,0,data); }
	else if(offset >= 0x1e00 && offset <= 0x1eff)	{ rom_bank_0_w(space,0,data); }
	else if(offset >= 0x1f80 && offset <= 0x1f8f)	{ z80dma_w(devtag_get_device(space->machine, "dma"), 0,data); }
	else if(offset >= 0x1f90 && offset <= 0x1f91)	{ z80sio_c_w(devtag_get_device(space->machine, "sio"), (offset-0x1f90) & 1,data); }
	else if(offset >= 0x1f92 && offset <= 0x1f93)	{ z80sio_d_w(devtag_get_device(space->machine, "sio"), (offset-0x1f92) & 1,data); }
	else if(offset >= 0x1fa0 && offset <= 0x1fa3)	{ z80ctc_w(devtag_get_device(space->machine, "ctc"), offset-0x1fa0,data); }
	else if(offset >= 0x1fa8 && offset <= 0x1fab)	{ z80ctc_w(devtag_get_device(space->machine, "ctc"), offset-0x1fa8,data); }
	else if(offset == 0x1fb0)						{ x1turbo_pal_w(space,0,data); }
	else if(offset >= 0x1fb9 && offset <= 0x1fbf)	{ x1turbo_txpal_w(space,offset-0x1fb9,data); }
	else if(offset == 0x1fc0)						{ x1turbo_txdisp_w(space,0,data); }
	else if(offset == 0x1fc5)						{ x1turbo_gfxpal_w(space,0,data); }
	else if(offset >= 0x1fd0 && offset <= 0x1fdf)	{ x1_scrn_w(space,0,data); }
	else if(offset == 0x1fe0)						{ x1_blackclip_w(space,0,data); }
	else if(offset >= 0x2000 && offset <= 0x2fff)	{ colorram[offset-0x2000] = data; }
	else if(offset >= 0x3000 && offset <= 0x3fff)	{ videoram[offset-0x3000] = pcg_write_addr = data; }
	else if(offset >= 0x4000 && offset <= 0xffff)	{ gfx_bitmap_ram[offset-0x4000+(scrn_reg.gfx_bank*0xc000)] = data; }
	else
	{
		logerror("(PC=%06x) Write %02x at i/o address %04x\n",cpu_get_pc(space->cpu),data,offset);
	}
}


static ADDRESS_MAP_START( x1_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROMBANK(1) AM_WRITE(rom_data_w)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( x1_io , ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(x1_io_r, x1_io_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( x1turbo_io , ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(x1turbo_io_r, x1turbo_io_w)
ADDRESS_MAP_END

/*************************************
 *
 *  PPI8255
 *
 *************************************/

static READ8_DEVICE_HANDLER( x1_porta_r )
{
	printf("PPI Port A read\n");
	return 0xff;
}

/* this port is system related */
static READ8_DEVICE_HANDLER( x1_portb_r )
{
	//printf("PPI Port B read\n");
	/*
	x--- ---- "v disp"
	-x-- ---- "sub cpu ibf"
	--x- ---- "sub cpu obf"
	---x ---- ROM/RAM flag (0=ROM, 1=RAM)
	---- x--- "busy"
	---- -x-- "v sync"
	---- --x- "cmt read"
	---- ---x "cmt test" (active low)
	*/
	UINT8 dat = 0;
	vdisp = (video_screen_get_vpos(device->machine->primary_screen) < 200) ? 0x80 : 0x00;
	dat = (input_port_read(device->machine, "SYSTEM") & 0x10) | sub_obf | vsync | vdisp;

	if(cassette_input(devtag_get_device(device->machine,"cass")) > 0.03)
		dat |= 0x02;

//	if(cassette_get_state(devtag_get_device(device->machine,"cass")) & CASSETTE_MOTOR_DISABLED)
//		dat &= ~0x02;  // is zero if not playing

	// CMT test bit is set low when the CMT Stop command is issued, and becomes
	// high again when this bit is read.
	dat |= 0x01;
	if(cmt_test != 0)
	{
		cmt_test = 0;
		dat &= ~0x01;
	}

	return dat;
}

/* I/O system port */
static READ8_DEVICE_HANDLER( x1_portc_r )
{
	//printf("PPI Port C read\n");
	/*
	x--x xxxx <unknown>
	-x-- ---- 320 mode (r/w)
	--x- ---- i/o mode (r/w)
	*/
	return (io_sys & 0x9f) | hres_320 | ~io_switch;
}

static WRITE8_DEVICE_HANDLER( x1_porta_w )
{
	//printf("PPI Port A write %02x\n",data);
}

static WRITE8_DEVICE_HANDLER( x1_portb_w )
{
	//printf("PPI Port B write %02x\n",data);
}

static WRITE8_DEVICE_HANDLER( x1_portc_w )
{
	hres_320 = data & 0x40;

	if(((data & 0x20) == 0) && (io_switch & 0x20))
		io_bank_mode = 1;

	io_switch = data & 0x20;
	io_sys = data & 0xff;

	cassette_output(devtag_get_device(device->machine,"cass"),(data & 0x01) ? +1.0 : -1.0);
}

static const ppi8255_interface ppi8255_intf =
{
	DEVCB_HANDLER(x1_porta_r),						/* Port A read */
	DEVCB_HANDLER(x1_portb_r),						/* Port B read */
	DEVCB_HANDLER(x1_portc_r),						/* Port C read */
	DEVCB_HANDLER(x1_porta_w),						/* Port A write */
	DEVCB_HANDLER(x1_portb_w),						/* Port B write */
	DEVCB_HANDLER(x1_portc_w)						/* Port C write */
};

static WRITE_LINE_DEVICE_HANDLER(vsync_changed)
{
	static UINT8 pcg_reset_occurred;

	vsync = (state & 1) ? 0x04 : 0x00;
	if(state & 1 && pcg_reset_occurred == 0) { pcg_reset = pcg_reset_occurred = 1; }
	if(!(state & 1))                         { pcg_reset_occurred = 0; }
	//printf("%d %02x\n",video_screen_get_vpos(device->machine->primary_screen),vsync);
}

static const mc6845_interface mc6845_intf =
{
	"screen",	/* screen we are acting on */
	8,			/* number of pixels per video memory address */
	NULL,		/* before pixel update callback */
	NULL,		/* row update callback */
	NULL,		/* after pixel update callback */
	DEVCB_NULL,	/* callback for display state changes */
	DEVCB_NULL,	/* callback for cursor state changes */
	DEVCB_NULL,	/* HSYNC callback */
	DEVCB_LINE(vsync_changed),	/* VSYNC callback */
	NULL		/* update address callback */
};

static READ8_DEVICE_HANDLER(x1_dma_read_byte)
{
	const address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	return memory_read_byte(space, offset);
}

static WRITE8_DEVICE_HANDLER(x1_dma_write_byte)
{
	const address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	memory_write_byte(space, offset, data);
}

static READ8_DEVICE_HANDLER(x1_dma_read_io)
{
	const address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_IO);
	return memory_read_byte(space, offset);
}

static WRITE8_DEVICE_HANDLER(x1_dma_write_io)
{
	const address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_IO);
	memory_write_byte(space, offset, data);
}


static const z80dma_interface x1_dma =
{
	"maincpu",

	x1_dma_read_byte,
	x1_dma_write_byte,
	x1_dma_read_io, x1_dma_write_io, x1_dma_read_io, x1_dma_write_io,
	NULL
};

/*************************************
 *
 *  Inputs
 *
 *************************************/

static INPUT_PORTS_START( x1 )
	PORT_START("SYSTEM")
	PORT_DIPNAME( 0x01, 0x01, "SYSTEM" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START("IOSYS")
	PORT_DIPNAME( 0x01, 0x01, "IOSYS" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("key1") //0x00-0x1f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_UNUSED) //0x00 null
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_UNUSED) //0x01 soh
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_UNUSED) //0x02 stx
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_UNUSED) //0x03 etx
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_UNUSED) //0x04 etx
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_UNUSED) //0x05 eot
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_UNUSED) //0x06 enq
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_UNUSED) //0x07 ack
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0a
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0b lf
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0c vt
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(27)
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0e cr
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0f so

	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x10 si
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x11 dle
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x12 dc1
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x13 dc2
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x14 dc3
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x15 dc4
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x16 nak
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x17 syn
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x18 etb
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x19 cancel
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1a em
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1b sub
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_TILDE) PORT_CHAR(27)
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1d fs
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1e gs
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1f us

	PORT_START("key2") //0x20-0x3f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_UNUSED) //0x21 !
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_UNUSED) //0x22 "
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_UNUSED) //0x23 #
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_UNUSED) //0x24 $
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_UNUSED) //0x25 %
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_UNUSED) //0x26 &
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_UNUSED) //0x27 '
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_UNUSED) //0x28 (
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_UNUSED) //0x29 )
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2a *
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2b +
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2c ,
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2e .
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2f /

	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':')
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';')
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x3c <
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x3d =
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x3e >
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x3f ?

	PORT_START("key3") //0x40-0x5f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@')
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('[')
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_UNUSED)
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(']')
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^')
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("_")

	PORT_START("tenkey")
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey -") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey +") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey *") PORT_CODE(KEYCODE_ASTERISK)
	// TODO: add other numpad keys

	PORT_START("key_modifiers")
	PORT_BIT(0x00000001,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x00000002,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x00000004,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("KANA") PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(0x00000008,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("CAPS") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT(0x00000010,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("GRPH") PORT_CODE(KEYCODE_LALT)

	#if 0
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',')
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.')
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/')
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey *") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey /") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey +") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey -") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey =")
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("\xEF\xBF\xA5")

	PORT_START("key3")
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey ,")
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey Enter") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey .") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("INS") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("EL") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CLS") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("DUP") PORT_CODE(KEYCODE_END)
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("BREAK") PORT_CODE(KEYCODE_ESC)
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF1") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF3") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF4") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF5") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF6") PORT_CODE(KEYCODE_F6)
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF7") PORT_CODE(KEYCODE_F7)
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF8") PORT_CODE(KEYCODE_F8)
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF9") PORT_CODE(KEYCODE_F9)
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF10") PORT_CODE(KEYCODE_F10)

	#endif
INPUT_PORTS_END

/*************************************
 *
 *  GFX decoding
 *
 *************************************/

static const gfx_layout x1_chars_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout x1_chars_8x16 =
{
	8,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	8*16
};

static const gfx_layout x1_pcg_8x8 =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3),RGN_FRAC(1,3),RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout x1_chars_16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,8,9,10,11,12,13,14,15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

/* decoded for debugging purpose, this will be nuked in the end... */
static GFXDECODE_START( x1 )
	GFXDECODE_ENTRY( "cgrom",   0x00000, x1_chars_8x8,    0x200, 0x20 )
	GFXDECODE_ENTRY( "pcg",     0x00000, x1_pcg_8x8,      0x000, 1 )
	GFXDECODE_ENTRY( "cgrom",   0x01800, x1_chars_8x16,   0x200, 0x20 )
//	GFXDECODE_ENTRY( "kanji",   0x27000, x1_chars_16x16,  0, 0x20 ) //needs to be checked when the ROM will be redumped
	GFXDECODE_ENTRY( "cgrom",   0x02800, x1_chars_16x16,  0, 0x20 ) //needs to be checked when the ROM will be redumped
GFXDECODE_END

/*************************************
 *
 *  CTC
 *
 *************************************/

static void ctc0_interrupt(const device_config *device, int state)
{
	cputag_set_input_line(device->machine,"maincpu",INPUT_LINE_IRQ0,state);
}


static const z80ctc_interface ctc_intf =
{
	0,					// timer disables
	ctc0_interrupt,		// interrupt handler
	z80ctc_trg3_w,		// ZC/TO0 callback
	z80ctc_trg1_w,		// ZC/TO1 callback
	z80ctc_trg2_w,		// ZC/TO2 callback
};

static const z80sio_interface sio_intf =
{
	0,					/* interrupt handler */
	0,					/* DTR changed handler */
	0,					/* RTS changed handler */
	0,					/* BREAK changed handler */
	0,					/* transmit handler */
	0					/* receive handler */
};

static const z80_daisy_chain x1_daisy[] =
{
	{ "x1kb" },
	{ "ctc" },
//	{ "dma" },
//	{ "sio" },
	{ NULL }
};

/*************************************
 *
 *  Sound HW Config
 *
 *************************************/

static const ay8910_interface ay8910_config =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_INPUT_PORT("P1"),
	DEVCB_INPUT_PORT("P2"),
	DEVCB_NULL,
	DEVCB_NULL
};

// (ym-2151 handler here)

/*************************************
 *
 *  Cassette configuration
 *
 *************************************/

static const cassette_config x1_cassette_config =
{
	x1_cassette_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED
};


/*************************************
 *
 *  Machine Functions
 *
 *************************************/

static INTERRUPT_GEN( x1_vbl )
{
	//...
}

/*static IRQ_CALLBACK(x1_irq_callback)
{
	if(ctc_irq_flag != 0)
	{
		ctc_irq_flag = 0;
		if(key_irq_flag == 0)  // if no other devices are pulling the IRQ line high
			cpu_set_input_line(device, 0, CLEAR_LINE);
		return x1_irq_vector;
	}
	if(key_irq_flag != 0)
	{
		key_irq_flag = 0;
		if(ctc_irq_flag == 0)  // if no other devices are pulling the IRQ line high
			cpu_set_input_line(device, 0, CLEAR_LINE);
		return key_irq_vector;
	}
	return x1_irq_vector;
}
*/
static TIMER_CALLBACK(keyboard_callback)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT32 key1 = input_port_read(machine,"key1");
	UINT32 key2 = input_port_read(machine,"key2");
	UINT32 key3 = input_port_read(machine,"key3");
	UINT32 key4 = input_port_read(machine,"tenkey");
	static UINT32 old_key1,old_key2,old_key3,old_key4;

	if(key_irq_vector)
	{
		if((key1 != old_key1) || (key2 != old_key2) || (key3 != old_key3) || (key4 != old_key4))
		{
			// generate keyboard IRQ
			sub_io_w(space,0,0xe6);
			x1_irq_vector = key_irq_vector;
			key_irq_flag = 1;
			cputag_set_input_line(machine,"maincpu",0,ASSERT_LINE);
			old_key1 = key1;
			old_key2 = key2;
			old_key3 = key3;
			old_key4 = key4;
		}
	}
}

static MACHINE_RESET( x1 )
{
	UINT8 *ROM = memory_region(machine, "maincpu");
	UINT8 *PCG_RAM = memory_region(machine, "pcg");
	int i;

	memory_set_bankptr(machine, 1, &ROM[0x00000]);

	memset(gfx_bitmap_ram,0x00,0xc000*2);

	for(i=0;i<0x1800;i++)
	{
		PCG_RAM[i] = 0;
	}

	for(i=0;i<0x0400;i++)
	{
		gfx_element_mark_dirty(machine->gfx[1], i);
	}

	/* X1 Turbo specific */
	scrn_reg.blackclip = 0;

	io_bank_mode = 0;
	pcg_index[0] = pcg_index[1] = pcg_index[2] = 0;

	//cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), x1_irq_callback);
	timer_pulse(machine, ATTOTIME_IN_HZ(240), NULL, 0, keyboard_callback);

	cmt_current_cmd = 0;
	cmt_test = 0;
	cassette_change_state(devtag_get_device(machine, "cass" ),CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);

	key_irq_flag = ctc_irq_flag = 0;
}

static PALETTE_INIT(x1)
{
	int i;

	for(i=0;i<0x300;i++)
		palette_set_color(machine, i,MAKE_RGB(0x00,0x00,0x00));

	for (i = 0x200; i < 0x220; i++)
	{
		UINT8 r,g,b;

		if(i & 0x01)
		{
			r = ((i >> 3) & 1) ? 0xff : 0x00;
			g = ((i >> 2) & 1) ? 0xff : 0x00;
			b = ((i >> 1) & 1) ? 0xff : 0x00;
		}
		else { r = g = b = 0x00; }

		if(i & 0x10) { r^=0xff; g^=0xff; b^=0xff; }

		palette_set_color_rgb(machine, i, r,g,b);
	}

	/* TODO: fix this */
	for(i=0;i<8;i++)
	{
		palette_set_color_rgb(machine, i+0x000, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
		palette_set_color_rgb(machine, i+0x100, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
	}
}

FLOPPY_OPTIONS_START( x1 )
	FLOPPY_OPTION( d88, "d88",	"D88 Floppy Disk image",	fm7_d77_identify,	fm7_d77_construct, NULL)
	FLOPPY_OPTION( img2d, "2d", "2D disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config x1_floppy_config =
{
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(x1)
};

static MACHINE_DRIVER_START( x1 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(x1_mem)
	MDRV_CPU_IO_MAP(x1_io)
	MDRV_CPU_VBLANK_INT("screen", x1_vbl)
	MDRV_CPU_CONFIG(x1_daisy)

	MDRV_Z80CTC_ADD( "ctc", XTAL_4MHz , ctc_intf )
	MDRV_Z80SIO_ADD( "sio", XTAL_4MHz , sio_intf )
	MDRV_Z80DMA_ADD( "dma", XTAL_4MHz , x1_dma )

	MDRV_DEVICE_ADD("x1kb", DEVICE_GET_INFO_NAME(x1_keyboard_getinfo), 0)

	MDRV_PPI8255_ADD( "ppi8255_0", ppi8255_intf )

	MDRV_MACHINE_RESET(x1)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(0x300)
	MDRV_PALETTE_INIT(x1)

	MDRV_MC6845_ADD("crtc", MC6845, XTAL_3_579545MHz/4, mc6845_intf)	/* unknown type and clock / divider, hand tuned to get ~60 fps */

	MDRV_GFXDECODE(x1)

	MDRV_VIDEO_START(x1)
	MDRV_VIDEO_UPDATE(x1)
	MDRV_VIDEO_EOF(x1)

	MDRV_MB8877_ADD("fdc",x1_mb8877a_interface)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("ay", AY8910, XTAL_4MHz/2) //unknown clock / divider
	MDRV_SOUND_CONFIG(ay8910_config)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_WAVE_ADD("wave","cass")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_CASSETTE_ADD("cass",x1_cassette_config)

	MDRV_FLOPPY_4_DRIVES_ADD(x1_floppy_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( x1turbo )
	MDRV_IMPORT_FROM( x1 )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_IO_MAP(x1turbo_io)

	MDRV_SOUND_ADD("ym", YM2151, XTAL_4MHz) //unknown clock / divider
//	MDRV_SOUND_CONFIG(ay8910_config)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

/*************************************
 *
 * ROM definitions
 *
 *************************************/

 ROM_START( x1 )
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "default", "X1 IPL" )
	ROMX_LOAD( "ipl.x1", 0x0000, 0x1000, CRC(7b28d9de) SHA1(c4db9a6e99873808c8022afd1c50fef556a8b44d), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "alt", "X1 IPL (alt)" )
	ROMX_LOAD( "ipl_alt.x1", 0x0000, 0x1000, CRC(e70011d3) SHA1(d3395e9aeb5b8bbba7654dd471bcd8af228ee69a), ROM_BIOS(2) )

	ROM_REGION(0x1000, "mcu", ROMREGION_ERASEFF) //MCU for the Keyboard, "sub cpu"
	ROM_LOAD( "80c48", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION(0x1800, "pcg", ROMREGION_ERASEFF)

	ROM_REGION(0x4d600, "cgrom", 0)
	ROM_LOAD("fnt0808.x1", 0x00000, 0x00800, CRC(e3995a57) SHA1(1c1a0d8c9f4c446ccd7470516b215ddca5052fb2) )
	ROM_RELOAD(            0x00800, 0x00800)
	ROM_RELOAD(            0x01000, 0x00800)
	ROM_LOAD("fnt0816.x1", 0x01800, 0x1000, BAD_DUMP CRC(34818d54) SHA1(2c5fdd73249421af5509e48a94c52c4e423402bf) )

	ROM_REGION(0x4ac00, "kanji", ROMREGION_ERASEFF)
ROM_END

ROM_START( x1ck )
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "default", "X1 IPL" )
	ROMX_LOAD( "ipl.x1", 0x0000, 0x1000, CRC(7b28d9de) SHA1(c4db9a6e99873808c8022afd1c50fef556a8b44d), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "alt", "X1 IPL (alt)" )
	ROMX_LOAD( "ipl_alt.x1", 0x0000, 0x1000, CRC(e70011d3) SHA1(d3395e9aeb5b8bbba7654dd471bcd8af228ee69a), ROM_BIOS(2) )

	ROM_REGION(0x1000, "mcu", ROMREGION_ERASEFF) //MCU for the Keyboard, "sub cpu"
	ROM_LOAD( "80c48", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION(0x1800, "pcg", ROMREGION_ERASEFF)

	ROM_REGION(0x4d600, "cgrom", 0)
	ROM_LOAD("fnt0808.x1", 0x00000, 0x00800, CRC(e3995a57) SHA1(1c1a0d8c9f4c446ccd7470516b215ddca5052fb2) )
	ROM_RELOAD(            0x00800, 0x00800)
	ROM_RELOAD(            0x01000, 0x00800)
	ROM_LOAD("fnt0816.x1", 0x01800, 0x01000, BAD_DUMP CRC(34818d54) SHA1(2c5fdd73249421af5509e48a94c52c4e423402bf) )

	ROM_REGION(0x4ac00, "kanji", 0)
	/* This is clearly a bad dump: size does not make sense and from 0x28000 on there are only 0xff */
	ROM_LOAD("kanji2.rom", 0x0000, 0x4ac00, BAD_DUMP CRC(33800ef2) SHA1(fc07a31ee30db312c7995e887519a9173cb38c0d) )
ROM_END

ROM_START( x1turbo )
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.x1t", 0x0000, 0x8000, CRC(2e8b767c) SHA1(44620f57a25f0bcac2b57ca2b0f1ebad3bf305d3) )

	ROM_REGION(0x1000, "mcu", ROMREGION_ERASEFF) //MCU for the Keyboard, "sub cpu"
	ROM_LOAD( "80c48", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION(0x1800, "pcg", ROMREGION_ERASEFF)

	ROM_REGION(0x4d600, "cgrom", 0)
	/* This should be larger... hence, we are missing something (maybe part of the other fnt roms?) */
	ROM_LOAD("fnt0808_turbo.x1", 0x00000, 0x00800, CRC(84a47530) SHA1(06c0995adc7a6609d4272417fe3570ca43bd0454) )
	ROM_RELOAD(            0x00800, 0x00800)
	ROM_RELOAD(            0x01000, 0x00800)
	ROM_LOAD("fnt0816.x1", 0x01800, 0x01000, BAD_DUMP CRC(34818d54) SHA1(2c5fdd73249421af5509e48a94c52c4e423402bf) )
	ROM_LOAD("fnt1616.x1", 0x02800, 0x4ac00, BAD_DUMP CRC(46826745) SHA1(b9e6c320611f0842df6f45673c47c3e23bc14272) )

	ROM_REGION(0x4ac00, "kanji", 0)
	/* This is clearly a bad dump: size does not make sense and from 0x28000 on there are only 0xff */
	ROM_LOAD("kanji2.rom", 0x0000, 0x4ac00, BAD_DUMP CRC(33800ef2) SHA1(fc07a31ee30db312c7995e887519a9173cb38c0d) )
ROM_END

/* X1 Turbo Z: IPL is supposed to be the same as X1 Turbo, but which dumps should be in "cgrom"?
Many emulators come with fnt0816.x1 & fnt1616.x1 but I am not sure about what was present on the real
X1 Turbo / Turbo Z */
ROM_START( x1turboz )
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.x1t", 0x0000, 0x8000, CRC(2e8b767c) SHA1(44620f57a25f0bcac2b57ca2b0f1ebad3bf305d3) )

	ROM_REGION(0x1000, "mcu", ROMREGION_ERASEFF) //MCU for the Keyboard, "sub cpu"
	ROM_LOAD( "80c48", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION(0x1800, "pcg", ROMREGION_ERASEFF)

	ROM_REGION(0x4d600, "cgrom", 0)
	ROM_LOAD("fnt0808_turbo.x1", 0x0000, 0x0800, CRC(84a47530) SHA1(06c0995adc7a6609d4272417fe3570ca43bd0454) )
	ROM_RELOAD(            0x00800, 0x00800)
	ROM_RELOAD(            0x01000, 0x00800)
	ROM_SYSTEM_BIOS( 0, "font1", "Font set 1" )
	ROMX_LOAD("fnt0816.x1", 0x1800, 0x1000, CRC(34818d54) SHA1(2c5fdd73249421af5509e48a94c52c4e423402bf), ROM_BIOS(1) )
	/* I strongly suspect this is not genuine */
	ROMX_LOAD("fnt1616.x1", 0x02800, 0x4ac00, BAD_DUMP CRC(46826745) SHA1(b9e6c320611f0842df6f45673c47c3e23bc14272), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "font2", "Font set 2" )
	ROMX_LOAD("fnt0816_a.x1", 0x01800, 0x1000, BAD_DUMP CRC(98db5a6b) SHA1(adf1492ef326b0f8820a3caa1915ad5ab8138f49), ROM_BIOS(2) )
	/* I strongly suspect this is not genuine */
	ROMX_LOAD("fnt1616_a.x1", 0x02800, 0x4ac00, BAD_DUMP CRC(bc5689ae) SHA1(a414116e261eb92bbdd407f63c8513257cd5a86f), ROM_BIOS(2) )

	ROM_REGION(0x4ac00, "kanji", 0)
	/* This is clearly a bad dump: size does not make sense and from 0x28000 on there are only 0xff */
	ROM_LOAD("kanji2.rom", 0x0000, 0x4ac00, BAD_DUMP CRC(33800ef2) SHA1(fc07a31ee30db312c7995e887519a9173cb38c0d) )
ROM_END



/*    YEAR  NAME       PARENT  COMPAT   MACHINE  INPUT  INIT  CONFIG COMPANY   FULLNAME      FLAGS */
COMP( 1982, x1,        0,      0,       x1,      x1,    0,    0,    "Sharp",  "X1 (CZ-800C)",         GAME_NOT_WORKING)
COMP( 1984, x1ck,      x1,     0,       x1,      x1,    0,    0,    "Sharp",  "X1Ck (CZ-804C)",       GAME_NOT_WORKING)
COMP( 1984, x1turbo,   x1,     0,       x1turbo, x1,    0,    0,    "Sharp",  "X1 Turbo (CZ-850C)",   GAME_NOT_WORKING)
COMP( 1986, x1turboz,  x1,     0,       x1turbo, x1,    0,    0,    "Sharp",  "X1 Turbo Z (CZ-880C)", GAME_NOT_WORKING)
