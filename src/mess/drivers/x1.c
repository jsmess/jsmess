/************************************************************************************************

    Sharp X1 (c) 1983 Sharp Corporation

    driver by Angelo Salese & Barry Rodewald

    TODO:
    - Understand how keyboard works and decap/dump the keyboard MCU if possible;
    - Hook-up remaining .tap image formats;
    - Implement .rom format support (needs an image for it);
    - Implement tape commands;
    - Sort out / redump the BIOS gfx roms;
    - X1Turbo: Implement SIO.
    - X1Twin: Hook-up the PC Engine part (actually needs his own driver?);
    - clean-ups!
    - There are various unclear video things, these are:
        - Understand why some games still doesn't upload the proper PCG index;
        - Implement the remaining scrn regs;
        - Interlace mode?
        - Implement the new features of the x1turbo, namely the 4096 color feature amongst other
          things
        - (anything else?)

    per-game/program specific TODO:
    - Bosconian: title screen background is completely white because it reverts the pen used
      (it's gray in the Arcade version),could be either flickering for pseudo-alpha effect or it's
      a btanb;
    - Exoa II - Warroid: major tile width/height positioning bugs (especially during gameplay);
    - Hydlide 2 / 3: can't get the user disk to work properly
    - Gruppe: shows a random bitmap graphic then returns "program load error"
    - Sorcerian / Ys 3 / Psy-O-Blade (and others): they all fails tight loops with the fdc ready bit check
    - Turbo Alpha: has z80dma / fdc bugs, doesn't show the presentation properly and then hangs;
    - Legend of Kage: has serious graphic artifacts, pcg doesn't scroll properly, bitmap-based sprites aren't shown properly, dma bugs?
    - "newtype": dies with a z80dma assert;
    - Ys 2: fills the screen with "syntax errors"
    - Thexder: Can't start a play, keyboard related issue?
    - V.I.P.: can't get inputs to work at all there;

    Notes:
    - An interesting feature of the Sharp X-1 is the extended i/o bank. When the ppi port c bit 5
      does a 1->0 transition, any write to the i/o space accesses 2 or 3 banks gradients of the bitmap RAM
      with a single write (generally used for layer clearances and bitmap-style sprites).
      Any i/o read disables this extended bitmap ram.
    - I/O port $700 bit 7 of X1 Turbo is a sound (dip-)switch / jumper setting. I don't know yet what is for,
      but King's Knight needs it to be active otherwise it refuses to boot.
    - ROM format is:
      0x00 ROM id (must be 0x01)
      0x01 - 0x0e ROM header
      0xff16 - 0xff17 start-up vector
      In theory, you can convert your tape / floppy games into ROM format easily, provided that you know what's the pinout of the
      cartridge slot and it doesn't exceed 64k (0x10000) of size.

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

=================================================================================================

    x1turbo specs (courtesy of Yasuhiro Ogawa):

    upper board: Z80A-CPU
                 Z80A-DMA
                 Z80A-SIO(O)
                 Z80A-CTC
                 uPD8255AC
                 LH5357(28pin mask ROM. for IPL?)
                 YM2149F
                 16.000MHz(X1)

    lower board: IX0526CE(HN61364) (28pin mask ROM. for ANK font?)
                 MB83256x4 (Kanji ROMs)
                 HD46505SP (VDP)
                 M80C49-277 (MCU)
                 uPD8255AC
                 uPD1990 (RTC) + battery
                 6.000MHz(X2)
                 42.9545MHz(X3)

    FDD I/O board: MB8877A (FDC)
                   MB4107 (VFO)

    RAM banks:
    upper board: MB8265A-15 x8 (main memory)
    lower board: MB8416A-12 x3 (VRAM)
                 MB8416A-15 x3 (PCG RAM)
                 MB81416-10 x12 (GRAM)

************************************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80ctc.h"
#include "machine/z80sio.h"
#include "machine/i8255a.h"
#include "machine/z80dma.h"
#include "sound/ay8910.h"
#include "video/mc6845.h"
#include "sound/2151intf.h"
#include "sound/wave.h"

#include "machine/wd17xx.h"
#include "devices/cassette.h"
#include "devices/flopdrv.h"
#include "formats/flopimg.h"
#include "formats/basicdsk.h"
#include "formats/x1_tap.h"
#include "devices/cartslot.h"
#include "includes/x1.h"
//#include <ctype.h>

#define MAIN_CLOCK XTAL_16MHz
#define VDP_CLOCK  XTAL_42_9545MHz
#define MCU_CLOCK  XTAL_6MHz






//static DEVICE_START(x1_daisy){}

/*************************************
 *
 *  Video Functions
 *
 *************************************/

static VIDEO_START( x1 )
{
	x1_state *state = machine->driver_data<x1_state>();
	state->colorram = auto_alloc_array(machine, UINT8, 0x1000);
	state->videoram = auto_alloc_array(machine, UINT8, 0x1000);
	state->gfx_bitmap_ram = auto_alloc_array(machine, UINT8, 0xc000*2);
}

static void draw_fgtilemap(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int w)
{
	x1_state *state = machine->driver_data<x1_state>();
	UINT8 *videoram = state->videoram;
	int y,x,res_x,res_y;
	int screen_mask;

	screen_mask = (w == 2) ? 0xfff : 0x7ff;

	for (y=0;y<50;y++)
	{
		for (x=0;x<40*w;x++)
		{
			int tile = videoram[(x+(y*40*w)+state->crtc_start_addr) & screen_mask];
			int color = state->colorram[(x+(y*40*w)+state->crtc_start_addr) & screen_mask] & 0x1f;
			int width = (state->colorram[(x+(y*40*w)+state->crtc_start_addr) & screen_mask] & 0x80)>>7;
			int height = (state->colorram[(x+(y*40*w)+state->crtc_start_addr) & screen_mask] & 0x40)>>6;
			int pcg_bank = (state->colorram[(x+(y*40*w)+state->crtc_start_addr) & screen_mask] & 0x20)>>5;
			UINT8 *gfx_data = pcg_bank ? memory_region(machine, "pcg") : memory_region(machine, "cgrom");

			/* skip draw if the x/y values are odd and the width/height is active, */
			/* behaviour confirmed by Black Onyx title screen and the X1Demo */
			if((x & 1) == 1 && width) continue;
			if((y & 1) == 1 && height) continue;

			res_x = (x/(width+1))*8*(width+1);
			res_y = (y/(height+1))*(state->tile_height+1)*(height+1);

			{
				int pen[3],pen_mask,pcg_pen,xi,yi;

				pen_mask = color & 7;

				/* We use custom drawing code due of the bitplane disable and the color revert stuff. */
				for(yi=0;yi<(state->tile_height+1)*(height+1);yi+=(height+1))
				{
					for(xi=0;xi<8*(width+1);xi+=(width+1))
					{
						pen[0] = gfx_data[((tile*8)+yi/(height+1))+0x0000]>>(7-xi/(width+1)) & (pen_mask & 1)>>0;
						pen[1] = gfx_data[((tile*8)+yi/(height+1))+0x0800]>>(7-xi/(width+1)) & (pen_mask & 2)>>1;
						pen[2] = gfx_data[((tile*8)+yi/(height+1))+0x1000]>>(7-xi/(width+1)) & (pen_mask & 4)>>2;

						pcg_pen = pen[2]<<2|pen[1]<<1|pen[0]<<0;

						if(color & 0x10 &&	machine->primary_screen->frame_number() & 0x10) //reverse flickering
							pcg_pen^=7;

						if(pcg_pen == 0 && (!(color & 8)))
							continue;

						if(color & 8) //revert the used color pen
							pcg_pen^=7;

						if((state->scrn_reg.blackclip & 8) && (color == (state->scrn_reg.blackclip & 7)))
							pcg_pen = 0; // clip the pen to black

						if((res_x+xi)>machine->primary_screen->visible_area().max_x && (res_y+yi)>machine->primary_screen->visible_area().max_y)
							continue;

						if(state->scrn_reg.v400_mode)
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
	x1_state *state = machine->driver_data<x1_state>();
	int xi,yi,x,y;
	int pen_r,pen_g,pen_b,color;
	int pri_mask_val;

	for (y=0;y<50;y++)
	{
		for(x=0;x<40*w;x++)
		{
			for(yi=0;yi<(state->tile_height+1);yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					pen_b = (state->gfx_bitmap_ram[(((x+(y*40*w)+yi*0x800)+state->crtc_start_addr) & 0x3fff)+0x0000+plane*0xc000]>>(7-xi)) & 1;
					pen_r = (state->gfx_bitmap_ram[(((x+(y*40*w)+yi*0x800)+state->crtc_start_addr) & 0x3fff)+0x4000+plane*0xc000]>>(7-xi)) & 1;
					pen_g = (state->gfx_bitmap_ram[(((x+(y*40*w)+yi*0x800)+state->crtc_start_addr) & 0x3fff)+0x8000+plane*0xc000]>>(7-xi)) & 1;

					color =  pen_g<<2 | pen_r<<1 | pen_b<<0;

					pri_mask_val = priority_mixer_ply(machine,color);
					if(pri_mask_val & pri) continue;

					if(state->scrn_reg.v400_mode)
					{
						if((x*8+xi)<=machine->primary_screen->visible_area().max_x && (( y*(state->tile_height+1)+yi)*2+0)<=machine->primary_screen->visible_area().max_y)
							*BITMAP_ADDR16(bitmap, ( y*(state->tile_height+1)+yi)*2+0, x*8+xi) = machine->pens[color+0x100];
						if((x*8+xi)<=machine->primary_screen->visible_area().max_x && (( y*(state->tile_height+1)+yi)*2+1)<=machine->primary_screen->visible_area().max_y)
							*BITMAP_ADDR16(bitmap, ( y*(state->tile_height+1)+yi)*2+1, x*8+xi) = machine->pens[color+0x100];
					}
					else
					{
						if((x*8+xi)<=machine->primary_screen->visible_area().max_x && (y*(state->tile_height+1)+yi)<=machine->primary_screen->visible_area().max_y)
							*BITMAP_ADDR16(bitmap, y*(state->tile_height+1)+yi, x*8+xi) = machine->pens[color+0x100];
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
	x1_state *state = machine->driver_data<x1_state>();
	int count,xi,yi,x,y;
	int pen_r,pen_g,pen_b,color;
	int yi_size;
	int pri_mask_val;

	count = state->crtc_start_addr;

	yi_size = (w == 1) ? 16 : 8;

	for(yi=0;yi<yi_size;yi++)
	{
		for(y=0;y<200;y+=8)
		{
			for(x=0;x<(320*w);x+=8)
			{
				for(xi=0;xi<8;xi++)
				{
					pen_b = (state->gfx_bitmap_ram[count+0x0000+plane*0xc000]>>(7-xi)) & 1;
					pen_r = (state->gfx_bitmap_ram[count+0x4000+plane*0xc000]>>(7-xi)) & 1;
					pen_g = (state->gfx_bitmap_ram[count+0x8000+plane*0xc000]>>(7-xi)) & 1;

					color =  pen_g<<2 | pen_r<<1 | pen_b<<0;

					pri_mask_val = priority_mixer_ply(machine,color);
					if(pri_mask_val & pri) continue;

					if(w == 1)
					{
						if(yi & 1)
							continue;

						if(state->scrn_reg.v400_mode)
						{
							if((x+xi)<=machine->primary_screen->visible_area().max_x && (y+(yi >> 1)*2+0)<=machine->primary_screen->visible_area().max_y)
								*BITMAP_ADDR16(bitmap, (y+(yi >> 1))*2+0, x+xi) = machine->pens[color+0x100];
							if((x+xi)<=machine->primary_screen->visible_area().max_x && (y+(yi >> 1)*2+1)<=machine->primary_screen->visible_area().max_y)
								*BITMAP_ADDR16(bitmap, (y+(yi >> 1))*2+1, x+xi) = machine->pens[color+0x100];
						}
						else
						{
							if((x+xi)<=machine->primary_screen->visible_area().max_x && (y+(yi >> 1))<=machine->primary_screen->visible_area().max_y)
								*BITMAP_ADDR16(bitmap, y+(yi >> 1), x+xi) = machine->pens[color+0x100];
						}
					}
					else
					{
						if(state->scrn_reg.v400_mode)
						{
							if((x+xi)<=machine->primary_screen->visible_area().max_x && ((y+yi)*2+0)<=machine->primary_screen->visible_area().max_y)
								*BITMAP_ADDR16(bitmap, (y+yi)*2+0, x+xi) = machine->pens[color+0x100];
							if((x+xi)<=machine->primary_screen->visible_area().max_x && ((y+yi)*2+1)<=machine->primary_screen->visible_area().max_y)
								*BITMAP_ADDR16(bitmap, (y+yi)*2+1, x+xi) = machine->pens[color+0x100];
						}
						else
						{
							if((x+xi)<=machine->primary_screen->visible_area().max_x && (y+yi)<=machine->primary_screen->visible_area().max_y)
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
	x1_state *state = screen->machine->driver_data<x1_state>();
	int w;

	w = (screen->width() < 640) ? 1 : 2;

	bitmap_fill(bitmap, cliprect, MAKE_ARGB(0xff,0x00,0x00,0x00));

	draw_gfxbitmap(screen->machine,bitmap,cliprect,w,state->scrn_reg.disp_bank,state->scrn_reg.ply);
	draw_fgtilemap(screen->machine,bitmap,cliprect,w);
	draw_gfxbitmap(screen->machine,bitmap,cliprect,w,state->scrn_reg.disp_bank,state->scrn_reg.ply^0xff);
	/* Y's uses the following as a normal buffer/work ram without anything reasonable to draw */
//  draw_gfxbitmap(screen->machine,bitmap,cliprect,w,1);

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


static UINT8 check_keyboard_press(running_machine *machine)
{
	const char* portnames[3] = { "key1","key2","key3" };
	int i,port_i,scancode;
	UINT8 keymod = input_port_read(machine,"key_modifiers") & 0x1f;
	UINT32 pad = input_port_read(machine,"tenkey");
	UINT32 f_key = input_port_read(machine, "f_keys");
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
					if(scancode == 0x30)
					{
						scancode = 0x3d;
					}
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

	// check function keys
	scancode = 0x71;
	for(i=0;i<5;i++)
	{
		if((f_key >> i) & 0x01)
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
			if(key1 & 0x08000000) ret |= 0x80;  // ESC
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
	x1_state *state = space->machine->driver_data<x1_state>();
	UINT8 ret,bus_res;

	/* Looks like that the HW retains the latest data putted on the bus here, behaviour confirmed by Rally-X */
	if(state->sub_obf)
	{
		bus_res = state->sub_val[state->key_i];
		/* FIXME: likely to be different here. */
		state->key_i++;
		if(state->key_i >= 2) { state->key_i = 0; }

		return bus_res;
	}

#if 0
	if(key_flag == 1)
	{
		key_flag = 0;
		return 0x82; //TODO: this is for shift/ctrl/kana lock etc.
	}
#endif

	state->sub_cmd_length--;
	state->sub_obf = (state->sub_cmd_length) ? 0x00 : 0x20;

	ret = state->sub_val[state->sub_val_ptr];

	state->sub_val_ptr++;
	if(state->sub_cmd_length <= 0)
		state->sub_val_ptr = 0;

	return ret;
}

//static UINT8 ctc_irq_vector; //always 0x5e?

static void cmt_command( running_machine* machine, UINT8 cmd )
{
	x1_state *state = machine->driver_data<x1_state>();
	// CMT deck control command (E9 xx)
	// E9 00 - Eject
	// E9 01 - Stop
	// E9 02 - Play
	// E9 03 - Fast Forward
	// E9 04 - Rewind
	// E9 05 - APSS(?) Fast Forward
	// E9 06 - APSS(?) Rewind
	// E9 0A - Record
	state->cmt_current_cmd = cmd;

	switch(cmd)
	{
		case 0x01:  // Stop
			cassette_change_state(machine->device("cass" ),CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
			cassette_change_state(machine->device("cass" ),CASSETTE_STOPPED,CASSETTE_MASK_UISTATE);
			state->cmt_test = 1;
			popmessage("CMT: Stop");
			break;
		case 0x02:  // Play
			cassette_change_state(machine->device("cass" ),CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
			cassette_change_state(machine->device("cass" ),CASSETTE_PLAY,CASSETTE_MASK_UISTATE);
			popmessage("CMT: Play");
			break;
		case 0x03:  // Fast Forward
			cassette_change_state(machine->device("cass" ),CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
			cassette_change_state(machine->device("cass" ),CASSETTE_STOPPED,CASSETTE_MASK_UISTATE);
			popmessage("CMT: Fast Forward");
			break;
		case 0x04:  // Rewind
			cassette_change_state(machine->device("cass" ),CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
			cassette_change_state(machine->device("cass" ),CASSETTE_STOPPED,CASSETTE_MASK_UISTATE);
			popmessage("CMT: Rewind");
			break;
		case 0x05:  // APSS Fast Forward
			cassette_change_state(machine->device("cass" ),CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
			cassette_change_state(machine->device("cass" ),CASSETTE_STOPPED,CASSETTE_MASK_UISTATE);
			popmessage("CMT: APSS Fast Forward");
			break;
		case 0x06:  // APSS Rewind
			cassette_change_state(machine->device("cass" ),CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
			cassette_change_state(machine->device("cass" ),CASSETTE_STOPPED,CASSETTE_MASK_UISTATE);
			popmessage("CMT: APSS Rewind");
			break;
		case 0x0a:  // Record
			cassette_change_state(machine->device("cass" ),CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
			cassette_change_state(machine->device("cass" ),CASSETTE_RECORD,CASSETTE_MASK_UISTATE);
			popmessage("CMT: Record");
			break;
		default:
			logerror("Unimplemented or invalid CMT command (0x%02x)\n",cmd);
	}
	logerror("CMT: Command 0xe9-0x%02x received.\n",cmd);
}

static TIMER_CALLBACK( cmt_wind_timer )
{
	x1_state *state = machine->driver_data<x1_state>();
	running_device* cmt = machine->device("cass");
	switch(state->cmt_current_cmd)
	{
		case 0x03:
		case 0x05:  // Fast Forwarding tape
			cassette_seek(cmt,1,SEEK_CUR);
			if(cassette_get_position(cmt) >= cassette_get_length(cmt))  // at end?
				cmt_command(machine,0x01);  // Stop tape
			break;
		case 0x04:
		case 0x06:  // Rewinding tape
			cassette_seek(cmt,-1,SEEK_CUR);
			if(cassette_get_position(cmt) <= 0) // at beginning?
				cmt_command(machine,0x01);  // Stop tape
			break;
	}
}

static WRITE8_HANDLER( sub_io_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	/* TODO: check sub-routine at $10e */
	/* $17a -> floppy? */
	/* $094 -> ROM */
	/* $0c0 -> timer*/
	/* $052 -> cmt */
	/* $0f5 -> reload sub-routine? */

	if(state->sub_cmd == 0xe4)
	{
		state->key_irq_vector = data;
		logerror("Key vector set to 0x%02x\n",data);
	}

	if(state->sub_cmd == 0xe9)
		cmt_command(space->machine,data);

	if((data & 0xf0) == 0xd0) //reads here tv recording timer data.
	{
		state->sub_val[0] = 0;
		state->sub_val[1] = 0;
		state->sub_val[2] = 0;
		state->sub_val[3] = 0;
		state->sub_val[4] = 0;
		state->sub_val[5] = 0;
		state->sub_cmd_length = 6;
	}

	switch(data)
	{
		case 0xe3:
			state->sub_cmd_length = 3;
			state->sub_val[0] = get_game_key(space->machine,0);
			state->sub_val[1] = get_game_key(space->machine,1);
			state->sub_val[2] = get_game_key(space->machine,2);
			break;
		case 0xe6:
			state->sub_val[0] = check_keyboard_shift(space->machine);
			state->sub_val[1] = check_keyboard_press(space->machine);
			state->sub_cmd_length = 2;
			break;
		case 0xe8:
			state->sub_val[0] = state->sub_cmd;
			state->sub_cmd_length = 1;
			break;
		case 0xea:  // CMT Control status
			state->sub_val[0] = state->cmt_current_cmd;
			state->sub_cmd_length = 1;
			logerror("CMT: Command 0xEA received, returning 0x%02x.\n",state->sub_val[0]);
			break;
		case 0xeb:  // CMT Tape status
		            // bit 0 = tape end (0=end of tape)
					// bit 1 = tape inserted
					// bit 2 = record status (1=OK, 0=write protect?)
			state->sub_val[0] = 0x05;
			if(cassette_get_image(space->machine->device("cass")) != NULL)
				state->sub_val[0] |= 0x02;
			state->sub_cmd_length = 1;
			logerror("CMT: Command 0xEB received, returning 0x%02x.\n",state->sub_val[0]);
			break;
		case 0xed: //get date
			state->sub_val[0] = state->rtc.day;
			state->sub_val[1] = (state->rtc.month<<4) | (state->rtc.wday & 0xf);
			state->sub_val[2] = state->rtc.year;
			state->sub_cmd_length = 3;
			break;
		case 0xef: //get time
			state->sub_val[0] = state->rtc.hour;
			state->sub_val[1] = state->rtc.min;
			state->sub_val[2] = state->rtc.sec;
			state->sub_cmd_length = 3;
			break;
		#if 0
		default:
		{
			if(data > 0xe3 && data <= 0xef)
				printf("%02x CMD\n",data);
		}
		#endif
	}

	state->sub_cmd = data;

	state->sub_obf = (state->sub_cmd_length) ? 0x00 : 0x20;
	logerror("SUB: Command byte 0x%02x\n",data);
}

/*************************************
 *
 *  ROM Image / Banking Handling
 *
 *************************************/


static READ8_HANDLER( x1_rom_r )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	UINT8 *rom = memory_region(space->machine, "cart_img");

//  printf("%06x\n",state->rom_index[0]<<16|state->rom_index[1]<<8|state->rom_index[2]<<0);

	return rom[state->rom_index[0]<<16|state->rom_index[1]<<8|state->rom_index[2]<<0];
}

static WRITE8_HANDLER( x1_rom_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	state->rom_index[offset] = data;
}

static WRITE8_HANDLER( rom_bank_0_w )
{
	UINT8 *ROM = memory_region(space->machine, "maincpu");

	memory_set_bankptr(space->machine, "bank1", &ROM[0x10000]);
}

static WRITE8_HANDLER( rom_bank_1_w )
{
	UINT8 *ROM = memory_region(space->machine, "maincpu");

	memory_set_bankptr(space->machine, "bank1", &ROM[0x00000]);
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

static READ8_HANDLER( x1_fdc_r )
{
	running_device* dev = space->machine->device("fdc");
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
	running_device* dev = space->machine->device("fdc");

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
			floppy_mon_w(floppy_get_device(space->machine, data & 3), !BIT(data, 7));
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
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

static WRITE_LINE_DEVICE_HANDLER( fdc_drq_w )
{
	z80dma_rdy_w(device->machine->device("dma"), state ^ 1);
}

static const wd17xx_interface x1turbo_mb8877a_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(fdc_drq_w),
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

/*************************************
 *
 *  Programmable Character Generator
 *
 *************************************/

static UINT16 check_pcg_addr(running_machine *machine)
{
	x1_state *state = machine->driver_data<x1_state>();
	if(state->colorram[0x7ff] & 0x20) return 0x7ff;
	if(state->colorram[0x3ff] & 0x20) return 0x3ff;
	if(state->colorram[0x5ff] & 0x20) return 0x5ff;
	if(state->colorram[0x1ff] & 0x20) return 0x1ff;

	return 0x3ff;
}

static UINT16 check_chr_addr(running_machine *machine)
{
	x1_state *state = machine->driver_data<x1_state>();
	if(!(state->colorram[0x7ff] & 0x20)) return 0x7ff;
	if(!(state->colorram[0x3ff] & 0x20)) return 0x3ff;
	if(!(state->colorram[0x5ff] & 0x20)) return 0x5ff;
	if(!(state->colorram[0x1ff] & 0x20)) return 0x1ff;

	return 0x3ff;
}

static READ8_HANDLER( x1_pcg_r )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	UINT8 *videoram = state->videoram;
	int addr;
	int calc_pcg_offset;
	UINT8 res;
	UINT8 *gfx_data;

	addr = (offset & 0x300) >> 8;

	if(state->pcg_reset)
	{
		state->pcg_index_r[0] = state->pcg_index_r[1] = state->pcg_index_r[2] = state->bios_offset = 0;
		state->pcg_reset = 0;
	}

	if(addr == 0)
	{
		if(state->scrn_reg.pcg_mode)
		{
			gfx_data = memory_region(space->machine, "kanji");
			calc_pcg_offset = (videoram[check_chr_addr(space->machine)]+(videoram[check_chr_addr(space->machine)+0x800]<<8)) & 0xfff;

			state->kanji_offset = calc_pcg_offset*0x20;

			res = gfx_data[0x0000+state->bios_offset+state->kanji_offset];

			state->bios_offset++;
			state->bios_offset&=0x1f;
		}
		else
		{
//          printf("%04x %04x %02x\n",state->pcg_write_addr*4*8,state->pcg_write_addr,state->bios_offset);
			gfx_data = memory_region(space->machine, "kanji");
			if(state->bios_offset == 0)
				state->kanji_offset = state->pcg_write_addr*4*8;//TODO: check me

			res = gfx_data[0x0000+state->bios_offset+state->kanji_offset];

			state->bios_offset++;
			state->bios_offset&=0x1f;
		}
		return res;
	}
	else
	{
		gfx_data = memory_region(space->machine, "pcg");
		calc_pcg_offset = (state->pcg_index_r[addr-1]) | ((addr-1)*0x800);
		res = gfx_data[0x0000+calc_pcg_offset+(state->pcg_write_addr*8)];

		state->pcg_index_r[addr-1]++;
		state->pcg_index_r[addr-1]&=0x7;
		return res;
	}

	return mame_rand(space->machine);
}

static WRITE8_HANDLER( x1_pcg_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	UINT8 *videoram = state->videoram;
	int addr,pcg_offset;
	UINT8 *PCG_RAM = memory_region(space->machine, "pcg");

	addr = (offset & 0x300) >> 8;

	if(state->pcg_reset)
	{
		state->pcg_index[0] = state->pcg_index[1] = state->pcg_index[2] = 0;
		state->pcg_reset = 0;
	}

	if(addr == 0)
	{
		/* NOP */
		logerror("Warning: write to the BIOS PCG area! %04x %02x\n",offset,data);
	}
	else
	{
		if(state->scrn_reg.pcg_mode)
		{
			state->used_pcg_addr = videoram[check_pcg_addr(space->machine)]*8;
			state->pcg_index[addr-1] = (offset & 0xe) >> 1;
			pcg_offset = (state->pcg_index[addr-1]+state->used_pcg_addr) & 0x7ff;
			pcg_offset+=((addr-1)*0x800);
			PCG_RAM[pcg_offset] = data;

			pcg_offset &= 0x7ff;

    		gfx_element_mark_dirty(space->machine->gfx[1], pcg_offset >> 3);
		}
		else
		{
			/* TODO: understand what 1942, Herzog and friends wants there */
			state->used_pcg_addr = state->pcg_write_addr*8;
			pcg_offset = (state->pcg_index[addr-1]+state->used_pcg_addr) & 0x7ff;
			pcg_offset+=((addr-1)*0x800);
			PCG_RAM[pcg_offset] = data;

			pcg_offset &= 0x7ff;

    		gfx_element_mark_dirty(space->machine->gfx[1], pcg_offset >> 3);

			state->pcg_index[addr-1]++;
			state->pcg_index[addr-1]&=7;
		}
	}
}

/*************************************
 *
 *  Other Video-related functions
 *
 *************************************/

/* for bitmap mode */

static void set_current_palette(running_machine *machine)
{
	x1_state *state = machine->driver_data<x1_state>();
	UINT8 addr,r,g,b;

	for(addr=0;addr<8;addr++)
	{
		r = ((state->x_r)>>(addr)) & 1;
		g = ((state->x_g)>>(addr)) & 1;
		b = ((state->x_b)>>(addr)) & 1;

		palette_set_color_rgb(machine, addr+0x100, pal1bit(r), pal1bit(g), pal1bit(b));
	}
}

static WRITE8_HANDLER( x1_pal_r_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	state->x_r = data;
	set_current_palette(space->machine);
}

static WRITE8_HANDLER( x1_pal_g_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	state->x_g = data;
	set_current_palette(space->machine);
}

static WRITE8_HANDLER( x1_pal_b_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	state->x_b = data;
	set_current_palette(space->machine);
}

static WRITE8_HANDLER( x1_ex_gfxram_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	UINT8 ex_mask;

	if     (offset >= 0x0000 && offset <= 0x3fff)	{ ex_mask = 7; }
	else if(offset >= 0x4000 && offset <= 0x7fff)	{ ex_mask = 6; }
	else if(offset >= 0x8000 && offset <= 0xbfff)	{ ex_mask = 5; }
	else                                        	{ ex_mask = 3; }

	if(ex_mask & 1) { state->gfx_bitmap_ram[(offset & 0x3fff)+0x0000+(state->scrn_reg.gfx_bank*0xc000)] = data; }
	if(ex_mask & 2) { state->gfx_bitmap_ram[(offset & 0x3fff)+0x4000+(state->scrn_reg.gfx_bank*0xc000)] = data; }
	if(ex_mask & 4) { state->gfx_bitmap_ram[(offset & 0x3fff)+0x8000+(state->scrn_reg.gfx_bank*0xc000)] = data; }
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
	x1_state *state = space->machine->driver_data<x1_state>();
	state->scrn_reg.pcg_mode = (data & 0x20)>>5;
	state->scrn_reg.gfx_bank = (data & 0x10)>>4;
	state->scrn_reg.disp_bank = (data & 0x08)>>3;
	state->scrn_reg.v400_mode = ((data & 0x03) == 3) ? 1 : 0;
	if(data & 0xc4)
		printf("SCRN = %02x\n",data & 0xc4);
}

static WRITE8_HANDLER( x1_ply_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	state->scrn_reg.ply = data;
//  printf("PLY = %02x\n",data);
}

static WRITE8_HANDLER( x1_6845_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	if(offset == 0)
	{
		state->addr_latch = data;
		mc6845_address_w(space->machine->device("crtc"), 0,data);
	}
	else
	{
		/* FIXME: this should be inside the MC6845 core! */
		if(state->addr_latch == 0x09)
			state->tile_height = data;
		if(state->addr_latch == 0x0c)
			state->crtc_start_addr = ((data<<8) & 0x3f00) | (state->crtc_start_addr & 0xff);
		else if(state->addr_latch == 0x0d)
			state->crtc_start_addr = (state->crtc_start_addr & 0x3f00) | (data & 0xff);

		mc6845_register_w(space->machine->device("crtc"), 0,data);

		/* double pump the pixel clock if we are in 640 x 200 mode */
		mc6845_set_clock(space->machine->device("crtc"), (space->machine->primary_screen->width() < 640) ? VDP_CLOCK/48 : VDP_CLOCK/24);
	}
}

static READ8_HANDLER( x1_blackclip_r )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	return state->scrn_reg.blackclip;
}

static WRITE8_HANDLER( x1_blackclip_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	state->scrn_reg.blackclip = data;
}

static READ8_HANDLER( x1turbo_pal_r )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	return state->turbo_reg.pal;
}

static READ8_HANDLER( x1turbo_txpal_r )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	return state->turbo_reg.txt_pal[offset];
}

static READ8_HANDLER( x1turbo_txdisp_r )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	return state->turbo_reg.txt_disp;
}

static READ8_HANDLER( x1turbo_gfxpal_r )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	return state->turbo_reg.gfx_pal;
}

static WRITE8_HANDLER( x1turbo_pal_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	printf("TURBO PAL %02x\n",data);
	state->turbo_reg.pal = data;
}

static WRITE8_HANDLER( x1turbo_txpal_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	int r,g,b;

	printf("TURBO TEXT PAL %02x %02x\n",data,offset);
	state->turbo_reg.txt_pal[offset] = data;

	if(state->turbo_reg.pal & 0x80)
	{
		r = (data & 0x0c) >> 2;
		g = (data & 0x30) >> 4;
		b = (data & 0x03) >> 0;

		palette_set_color_rgb(space->machine, offset, pal2bit(r), pal2bit(g), pal2bit(b));
	}
}

static WRITE8_HANDLER( x1turbo_txdisp_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	printf("TURBO TEXT DISPLAY %02x\n",data);
	state->turbo_reg.txt_disp = data;
}

static WRITE8_HANDLER( x1turbo_gfxpal_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	printf("TURBO GFX PAL %02x\n",data);
	state->turbo_reg.gfx_pal = data;
}


/*
 *  FIXME: this makes no sense atm, the only game that uses this function so far is Hyper Olympics '84 disk version
 */
static READ8_HANDLER( x1_kanji_r )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	UINT8 *kanji_rom = memory_region(space->machine, "kanji");
	UINT8 res;

	//res = 0;

	printf("%08x\n",state->kanji_addr*0x20+state->kanji_count);

	res = kanji_rom[(state->kanji_addr*0x20)+(state->kanji_count)];
	state->kanji_count++;
	state->kanji_count&=0x1f;

	//printf("%04x R\n",offset);

	return res;
}

static WRITE8_HANDLER( x1_kanji_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
//  if(offset < 2)
		printf("%04x %02x W\n",offset,data);

	switch(offset)
	{
		case 0: state->kanji_addr_l = (data & 0xff); break;
		case 1: state->kanji_addr_h = (data & 0xff); state->kanji_count = 0; break;
		case 2:
			if(data)
			{
				state->kanji_addr = (state->kanji_addr_h<<8) | (state->kanji_addr_l);
				//state->kanji_addr &= 0x3fff; //<- temp kludge until the rom is redumped.
				//printf("%08x\n",state->kanji_addr);
				//state->kanji_addr+= state->kanji_count;
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
	x1_state *state = space->machine->driver_data<x1_state>();
	UINT8 *videoram = state->videoram;
	state->io_bank_mode = 0; //any read disables the extended mode.

	//if(offset >= 0x0704 && offset <= 0x0707)      { return z80ctc_r(space->machine->device("ctc"), offset-0x0704); }
	if(offset == 0x0e03)                    		{ return x1_rom_r(space, 0); }
	else if(offset >= 0x0ff8 && offset <= 0x0fff)	{ return x1_fdc_r(space, offset-0xff8); }
	else if(offset >= 0x1400 && offset <= 0x17ff)	{ return x1_pcg_r(space, offset-0x1400); }
	else if(offset >= 0x1900 && offset <= 0x19ff)	{ return sub_io_r(space, 0); }
	else if(offset >= 0x1a00 && offset <= 0x1aff)	{ return i8255a_r(space->machine->device("ppi8255_0"), (offset-0x1a00) & 3); }
	else if(offset >= 0x1b00 && offset <= 0x1bff)	{ return ay8910_r(space->machine->device("ay"), 0); }
//  else if(offset >= 0x1f80 && offset <= 0x1f8f)   { return z80dma_r(space->machine->device("dma"), 0); }
//  else if(offset >= 0x1f90 && offset <= 0x1f91)   { return z80sio_c_r(space->machine->device("sio"), (offset-0x1f90) & 1); }
//  else if(offset >= 0x1f92 && offset <= 0x1f93)   { return z80sio_d_r(space->machine->device("sio"), (offset-0x1f92) & 1); }
	else if(offset >= 0x1fa0 && offset <= 0x1fa3)	{ return z80ctc_r(space->machine->device("ctc"), offset-0x1fa0); }
	else if(offset >= 0x1fa8 && offset <= 0x1fab)	{ return z80ctc_r(space->machine->device("ctc"), offset-0x1fa8); }
//  else if(offset >= 0x1fd0 && offset <= 0x1fdf)   { return x1_scrn_r(space,offset-0x1fd0); }
//  else if(offset == 0x1fe0)                       { return x1_blackclip_r(space,0); }
	else if(offset >= 0x2000 && offset <= 0x2fff)	{ return state->colorram[offset-0x2000]; }
	else if(offset >= 0x3000 && offset <= 0x3fff)	{ return videoram[offset-0x3000]; }
	else if(offset >= 0x4000 && offset <= 0xffff)	{ return state->gfx_bitmap_ram[offset-0x4000+(state->scrn_reg.gfx_bank*0xc000)]; }
	else
	{
		logerror("(PC=%06x) Read i/o address %04x\n",cpu_get_pc(space->cpu),offset);
	}
	return 0xff;
}

static WRITE8_HANDLER( x1_io_w )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	UINT8 *videoram = state->videoram;
	if(state->io_bank_mode == 1)                       	{ x1_ex_gfxram_w(space, offset, data); }
//  else if(offset >= 0x0704 && offset <= 0x0707)   { z80ctc_w(space->machine->device("ctc"), offset-0x0704,data); }
//  else if(offset >= 0x0c00 && offset <= 0x0cff)   { x1_rs232c_w(space->machine, 0, data); }
	else if(offset >= 0x0e00 && offset <= 0x0e02)	{ x1_rom_w(space, offset-0xe00,data); }
//  else if(offset >= 0x0e80 && offset <= 0x0e82)   { x1_kanji_w(space->machine, offset-0xe80,data); }
	else if(offset >= 0x0ff8 && offset <= 0x0fff)	{ x1_fdc_w(space, offset-0xff8,data); }
	else if(offset >= 0x1000 && offset <= 0x10ff)	{ x1_pal_b_w(space, 0,data); }
	else if(offset >= 0x1100 && offset <= 0x11ff)	{ x1_pal_r_w(space, 0,data); }
	else if(offset >= 0x1200 && offset <= 0x12ff)	{ x1_pal_g_w(space, 0,data); }
	else if(offset >= 0x1300 && offset <= 0x13ff)	{ x1_ply_w(space, 0,data); }
	else if(offset >= 0x1400 && offset <= 0x17ff)	{ x1_pcg_w(space, offset-0x1400,data); }
	else if(offset == 0x1800 || offset == 0x1801)	{ x1_6845_w(space, offset-0x1800, data); }
	else if(offset >= 0x1900 && offset <= 0x19ff)	{ sub_io_w(space, 0,data); }
	else if(offset >= 0x1a00 && offset <= 0x1aff)	{ i8255a_w(space->machine->device("ppi8255_0"), (offset-0x1a00) & 3,data); }
	else if(offset >= 0x1b00 && offset <= 0x1bff)	{ ay8910_data_w(space->machine->device("ay"), 0,data); }
	else if(offset >= 0x1c00 && offset <= 0x1cff)	{ ay8910_address_w(space->machine->device("ay"), 0,data); }
	else if(offset >= 0x1d00 && offset <= 0x1dff)	{ rom_bank_1_w(space,0,data); }
	else if(offset >= 0x1e00 && offset <= 0x1eff)	{ rom_bank_0_w(space,0,data); }
//  else if(offset >= 0x1f80 && offset <= 0x1f8f)   { z80dma_w(space->machine->device("dma"), 0,data); }
//  else if(offset >= 0x1f90 && offset <= 0x1f91)   { z80sio_c_w(space->machine->device("sio"), (offset-0x1f90) & 1,data); }
//  else if(offset >= 0x1f92 && offset <= 0x1f93)   { z80sio_d_w(space->machine->device("sio"), (offset-0x1f92) & 1,data); }
	else if(offset >= 0x1fa0 && offset <= 0x1fa3)	{ z80ctc_w(space->machine->device("ctc"), offset-0x1fa0,data); }
	else if(offset >= 0x1fa8 && offset <= 0x1fab)	{ z80ctc_w(space->machine->device("ctc"), offset-0x1fa8,data); }
//  else if(offset == 0x1fb0)                       { x1turbo_pal_w(space,0,data); }
//  else if(offset >= 0x1fb9 && offset <= 0x1fbf)   { x1turbo_txpal_w(space,offset-0x1fb9,data); }
//  else if(offset == 0x1fc0)                       { x1turbo_txdisp_w(space,0,data); }
//  else if(offset == 0x1fc5)                       { x1turbo_gfxpal_w(space,0,data); }
	else if(offset >= 0x1fd0 && offset <= 0x1fdf)	{ x1_scrn_w(space,0,data); }
//  else if(offset == 0x1fe0)                       { x1_blackclip_w(space,0,data); }
	else if(offset >= 0x2000 && offset <= 0x2fff)	{ state->colorram[offset-0x2000] = data; }
	else if(offset >= 0x3000 && offset <= 0x3fff)	{ videoram[offset-0x3000] = state->pcg_write_addr = data; }
	else if(offset >= 0x4000 && offset <= 0xffff)	{ state->gfx_bitmap_ram[offset-0x4000+(state->scrn_reg.gfx_bank*0xc000)] = data; }
	else
	{
		logerror("(PC=%06x) Write %02x at i/o address %04x\n",cpu_get_pc(space->cpu),data,offset);
	}
}

static READ8_HANDLER( x1turbo_io_r )
{
	x1_state *state = space->machine->driver_data<x1_state>();
	UINT8 *videoram = state->videoram;
	state->io_bank_mode = 0; //any read disables the extended mode.

	if(offset == 0x0700)							{ return (ym2151_r(space->machine->device("ym"), offset-0x0700) & 0x7f) | (input_port_read(space->machine, "SOUND_SW") & 0x80); }
	else if(offset == 0x0701)		                { return ym2151_r(space->machine->device("ym"), offset-0x0700); }
	else if(offset >= 0x0704 && offset <= 0x0707)   { return z80ctc_r(space->machine->device("ctc"), offset-0x0704); }
	else if(offset == 0x0e03)                   	{ return x1_rom_r(space, 0); }
	else if(offset >= 0x0e80 && offset <= 0x0e83)	{ return x1_kanji_r(space, offset-0xe80); }
	else if(offset >= 0x0ff8 && offset <= 0x0fff)	{ return x1_fdc_r(space, offset-0xff8); }
	else if(offset >= 0x1400 && offset <= 0x17ff)	{ return x1_pcg_r(space, offset-0x1400); }
	else if(offset >= 0x1900 && offset <= 0x19ff)	{ return sub_io_r(space, 0); }
	else if(offset >= 0x1a00 && offset <= 0x1aff)	{ return i8255a_r(space->machine->device("ppi8255_0"), (offset-0x1a00) & 3); }
	else if(offset >= 0x1b00 && offset <= 0x1bff)	{ return ay8910_r(space->machine->device("ay"), 0); }
	else if(offset >= 0x1f80 && offset <= 0x1f8f)	{ return z80dma_r(space->machine->device("dma"), 0); }
	else if(offset >= 0x1f90 && offset <= 0x1f91)	{ return z80sio_c_r(space->machine->device("sio"), (offset-0x1f90) & 1); }
	else if(offset >= 0x1f92 && offset <= 0x1f93)	{ return z80sio_d_r(space->machine->device("sio"), (offset-0x1f92) & 1); }
	else if(offset >= 0x1fa0 && offset <= 0x1fa3)	{ return z80ctc_r(space->machine->device("ctc"), offset-0x1fa0); }
	else if(offset >= 0x1fa8 && offset <= 0x1fab)	{ return z80ctc_r(space->machine->device("ctc"), offset-0x1fa8); }
	else if(offset == 0x1fb0)						{ return x1turbo_pal_r(space,0); }
	else if(offset >= 0x1fb8 && offset <= 0x1fbf)	{ return x1turbo_txpal_r(space,offset-0x1fb8); }
	else if(offset == 0x1fc0)						{ return x1turbo_txdisp_r(space,0); }
	else if(offset == 0x1fc5)						{ return x1turbo_gfxpal_r(space,0); }
//  else if(offset >= 0x1fd0 && offset <= 0x1fdf)   { return x1_scrn_r(space,offset-0x1fd0); }
	else if(offset == 0x1fe0)						{ return x1_blackclip_r(space,0); }
	else if(offset >= 0x2000 && offset <= 0x2fff)	{ return state->colorram[offset-0x2000]; }
	else if(offset >= 0x3000 && offset <= 0x3fff)	{ return videoram[offset-0x3000]; }
	else if(offset >= 0x4000 && offset <= 0xffff)	{ return state->gfx_bitmap_ram[offset-0x4000+(state->scrn_reg.gfx_bank*0xc000)]; }
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
	x1_state *state = space->machine->driver_data<x1_state>();
	UINT8 *videoram = state->videoram;
	if(state->io_bank_mode == 1)                       	{ x1_ex_gfxram_w(space, offset, data); }
	else if(offset == 0x0700 || offset == 0x0701)	{ ym2151_w(space->machine->device("ym"), offset-0x0700,data); }
	else if(offset >= 0x0704 && offset <= 0x0707)	{ z80ctc_w(space->machine->device("ctc"), offset-0x0704,data); }
//  else if(offset >= 0x0c00 && offset <= 0x0cff)   { x1_rs232c_w(space->machine, 0, data); }
	else if(offset >= 0x0e00 && offset <= 0x0e02)	{ x1_rom_w(space, offset-0xe00,data); }
	else if(offset >= 0x0e80 && offset <= 0x0e83)	{ x1_kanji_w(space, offset-0xe80,data); }
	else if(offset >= 0x0ff8 && offset <= 0x0fff)	{ x1_fdc_w(space, offset-0xff8,data); }
	else if(offset >= 0x1000 && offset <= 0x10ff)	{ x1_pal_b_w(space, 0,data); }
	else if(offset >= 0x1100 && offset <= 0x11ff)	{ x1_pal_r_w(space, 0,data); }
	else if(offset >= 0x1200 && offset <= 0x12ff)	{ x1_pal_g_w(space, 0,data); }
	else if(offset >= 0x1300 && offset <= 0x13ff)	{ x1_ply_w(space, 0,data); }
	else if(offset >= 0x1400 && offset <= 0x17ff)	{ x1_pcg_w(space, offset-0x1400,data); }
	else if(offset == 0x1800 || offset == 0x1801)	{ x1_6845_w(space, offset-0x1800, data); }
	else if(offset >= 0x1900 && offset <= 0x19ff)	{ sub_io_w(space, 0,data); }
	else if(offset >= 0x1a00 && offset <= 0x1aff)	{ i8255a_w(space->machine->device("ppi8255_0"), (offset-0x1a00) & 3,data); }
	else if(offset >= 0x1b00 && offset <= 0x1bff)	{ ay8910_data_w(space->machine->device("ay"), 0,data); }
	else if(offset >= 0x1c00 && offset <= 0x1cff)	{ ay8910_address_w(space->machine->device("ay"), 0,data); }
	else if(offset >= 0x1d00 && offset <= 0x1dff)	{ rom_bank_1_w(space,0,data); }
	else if(offset >= 0x1e00 && offset <= 0x1eff)	{ rom_bank_0_w(space,0,data); }
	else if(offset >= 0x1f80 && offset <= 0x1f8f)	{ z80dma_w(space->machine->device("dma"), 0,data); }
	else if(offset >= 0x1f90 && offset <= 0x1f91)	{ z80sio_c_w(space->machine->device("sio"), (offset-0x1f90) & 1,data); }
	else if(offset >= 0x1f92 && offset <= 0x1f93)	{ z80sio_d_w(space->machine->device("sio"), (offset-0x1f92) & 1,data); }
	else if(offset >= 0x1fa0 && offset <= 0x1fa3)	{ z80ctc_w(space->machine->device("ctc"), offset-0x1fa0,data); }
	else if(offset >= 0x1fa8 && offset <= 0x1fab)	{ z80ctc_w(space->machine->device("ctc"), offset-0x1fa8,data); }
	else if(offset == 0x1fb0)						{ x1turbo_pal_w(space,0,data); }
	else if(offset >= 0x1fb8 && offset <= 0x1fbf)	{ x1turbo_txpal_w(space,offset-0x1fb8,data); }
	else if(offset == 0x1fc0)						{ x1turbo_txdisp_w(space,0,data); }
	else if(offset == 0x1fc5)						{ x1turbo_gfxpal_w(space,0,data); }
	else if(offset >= 0x1fd0 && offset <= 0x1fdf)	{ x1_scrn_w(space,0,data); }
	else if(offset == 0x1fe0)						{ x1_blackclip_w(space,0,data); }
	else if(offset >= 0x2000 && offset <= 0x2fff)	{ state->colorram[offset-0x2000] = data; }
	else if(offset >= 0x3000 && offset <= 0x3fff)	{ videoram[offset-0x3000] = state->pcg_write_addr = data; }
	else if(offset >= 0x4000 && offset <= 0xffff)	{ state->gfx_bitmap_ram[offset-0x4000+(state->scrn_reg.gfx_bank*0xc000)] = data; }
	else
	{
		logerror("(PC=%06x) Write %02x at i/o address %04x\n",cpu_get_pc(space->cpu),data,offset);
	}
}


static ADDRESS_MAP_START( x1_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROMBANK("bank1") AM_WRITE(rom_data_w)
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
	x1_state *state = device->machine->driver_data<x1_state>();
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
	state->vdisp = (device->machine->primary_screen->vpos() < 200) ? 0x80 : 0x00;
	dat = (input_port_read(device->machine, "SYSTEM") & 0x10) | state->sub_obf | state->vsync | state->vdisp;

	if(cassette_input(device->machine->device("cass")) > 0.03)
		dat |= 0x02;

//  if(cassette_get_state(device->machine->device("cass")) & CASSETTE_MOTOR_DISABLED)
//      dat &= ~0x02;  // is zero if not playing

	// CMT test bit is set low when the CMT Stop command is issued, and becomes
	// high again when this bit is read.
	dat |= 0x01;
	if(state->cmt_test != 0)
	{
		state->cmt_test = 0;
		dat &= ~0x01;
	}

	return dat;
}

/* I/O system port */
static READ8_DEVICE_HANDLER( x1_portc_r )
{
	x1_state *state = device->machine->driver_data<x1_state>();
	//printf("PPI Port C read\n");
	/*
    x--x xxxx <unknown>
    -x-- ---- 320 mode (r/w)
    --x- ---- i/o mode (r/w)
    */
	return (state->io_sys & 0x9f) | state->hres_320 | ~state->io_switch;
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
	x1_state *state = device->machine->driver_data<x1_state>();
	state->hres_320 = data & 0x40;

	if(((data & 0x20) == 0) && (state->io_switch & 0x20))
		state->io_bank_mode = 1;

	state->io_switch = data & 0x20;
	state->io_sys = data & 0xff;

	cassette_output(device->machine->device("cass"),(data & 0x01) ? +1.0 : -1.0);
}

static I8255A_INTERFACE( ppi8255_intf )
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
	x1_state *drvstate = device->machine->driver_data<x1_state>();
	drvstate->vsync = (state & 1) ? 0x04 : 0x00;
	if(state & 1 && drvstate->pcg_reset_occurred == 0) { drvstate->pcg_reset = drvstate->pcg_reset_occurred = 1; }
	if(!(state & 1))                         { drvstate->pcg_reset_occurred = 0; }
	//printf("%d %02x\n",device->machine->primary_screen->vpos(),drvstate->vsync);
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

static UINT8 memory_read_byte(address_space *space, offs_t address) { return space->read_byte(address); }
static void memory_write_byte(address_space *space, offs_t address, UINT8 data) { space->write_byte(address, data); }

static Z80DMA_INTERFACE( x1_dma )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_HALT),
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, memory_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, memory_write_byte),
	DEVCB_MEMORY_HANDLER("maincpu", IO, memory_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", IO, memory_write_byte)
};

/*************************************
 *
 *  Inputs
 *
 *************************************/

static INPUT_CHANGED( ipl_reset )
{
	address_space *space = cputag_get_address_space(field->port->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *ROM = memory_region(space->machine, "maincpu");

	cputag_set_input_line(field->port->machine, "maincpu", INPUT_LINE_RESET, newval ? CLEAR_LINE : ASSERT_LINE);
	memory_set_bankptr(space->machine, "bank1", &ROM[0x00000]);

	//anything else?
}

/* Apparently most games doesn't support this (not even the Konami ones!), one that does is...177 :o */
static INPUT_CHANGED( nmi_reset )
{
	cputag_set_input_line(field->port->machine, "maincpu", INPUT_LINE_NMI, newval ? CLEAR_LINE : ASSERT_LINE);
}

static INPUT_PORTS_START( x1 )
	PORT_START("FP_SYS") //front panel buttons, hard-wired with the soft reset/NMI lines
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CHANGED(ipl_reset,0) PORT_NAME("IPL reset")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CHANGED(nmi_reset,0) PORT_NAME("NMI reset")

	PORT_START("SOUND_SW") //FIXME: this is X1Turbo specific
	PORT_DIPNAME( 0x80, 0x80, "OPM Sound Setting?" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("SYSTEM")
	PORT_DIPNAME( 0x10, 0x10, "unk" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("IOSYS") //TODO: implement front-panel DIP-SW here
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
	PORT_DIPNAME( 0x80, 0x80, "Sound Setting?" )
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
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("ESC") PORT_CHAR(27)
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1c ???
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

	PORT_START("f_keys")
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF1") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF3") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF4") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF5") PORT_CODE(KEYCODE_F5)

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
	8,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16
};

/* decoded for debugging purpose, this will be nuked in the end... */
static GFXDECODE_START( x1 )
	GFXDECODE_ENTRY( "cgrom",   0x00000, x1_chars_8x8,    0x200, 0x20 )
	GFXDECODE_ENTRY( "pcg",     0x00000, x1_pcg_8x8,      0x000, 1 )
	GFXDECODE_ENTRY( "font",    0x00000, x1_chars_8x16,   0x200, 0x20 )
	GFXDECODE_ENTRY( "kanji",   0x00000, x1_chars_16x16,  0x200, 0x20 )
GFXDECODE_END

/*************************************
 *
 *  CTC
 *
 *************************************/

static Z80CTC_INTERFACE( ctc_intf )
{
	0,					// timer disables
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),		// interrupt handler
	DEVCB_LINE(z80ctc_trg3_w),		// ZC/TO0 callback
	DEVCB_LINE(z80ctc_trg1_w),		// ZC/TO1 callback
	DEVCB_LINE(z80ctc_trg2_w),		// ZC/TO2 callback
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

static const z80_daisy_config x1_daisy[] =
{
    { "x1kb" },
	{ "ctc" },
	{ NULL }
};

static const z80_daisy_config x1turbo_daisy[] =
{
    { "x1kb" },
	{ "ctc" },
	{ "dma" },
//  { "sio" },
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
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED),
	"x1_cass"
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

#ifdef UNUSED_FUNCTION
static IRQ_CALLBACK(x1_irq_callback)
{
	x1_state *state = device->machine->driver_data<x1_state>();
    if(state->ctc_irq_flag != 0)
    {
        state->ctc_irq_flag = 0;
        if(state->key_irq_flag == 0)  // if no other devices are pulling the IRQ line high
            cpu_set_input_line(device, 0, CLEAR_LINE);
        return state->irq_vector;
    }
    if(state->key_irq_flag != 0)
    {
        state->key_irq_flag = 0;
        if(state->ctc_irq_flag == 0)  // if no other devices are pulling the IRQ line high
            cpu_set_input_line(device, 0, CLEAR_LINE);
        return state->key_irq_vector;
    }
    return state->irq_vector;
}
#endif

static TIMER_CALLBACK(keyboard_callback)
{
	x1_state *state = machine->driver_data<x1_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT32 key1 = input_port_read(machine,"key1");
	UINT32 key2 = input_port_read(machine,"key2");
	UINT32 key3 = input_port_read(machine,"key3");
	UINT32 key4 = input_port_read(machine,"tenkey");
	UINT32 f_key = input_port_read(machine, "f_keys");

	if(state->key_irq_vector)
	{
		if((key1 != state->old_key1) || (key2 != state->old_key2) || (key3 != state->old_key3) || (key4 != state->old_key4) || (f_key != state->old_fkey))
		{
			// generate keyboard IRQ
			sub_io_w(space,0,0xe6);
			state->irq_vector = state->key_irq_vector;
			state->key_irq_flag = 1;
			cputag_set_input_line(machine,"maincpu",0,ASSERT_LINE);
			state->old_key1 = key1;
			state->old_key2 = key2;
			state->old_key3 = key3;
			state->old_key4 = key4;
			state->old_fkey = f_key;
		}
	}
}

static TIMER_CALLBACK(x1_rtc_increment)
{
	x1_state *state = machine->driver_data<x1_state>();
	static const UINT8 dpm[12] = { 0x31, 0x28, 0x31, 0x30, 0x31, 0x30, 0x31, 0x31, 0x30, 0x31, 0x30, 0x31 };

	state->rtc.sec++;

	if((state->rtc.sec & 0x0f) >= 0x0a) 				{ state->rtc.sec+=0x10; state->rtc.sec&=0xf0; }
	if((state->rtc.sec & 0xf0) >= 0x60) 				{ state->rtc.min++; state->rtc.sec = 0; }
	if((state->rtc.min & 0x0f) >= 0x0a) 				{ state->rtc.min+=0x10; state->rtc.min&=0xf0; }
	if((state->rtc.min & 0xf0) >= 0x60) 				{ state->rtc.hour++; state->rtc.min = 0; }
	if((state->rtc.hour & 0x0f) >= 0x0a)				{ state->rtc.hour+=0x10; state->rtc.hour&=0xf0; }
	if((state->rtc.hour & 0xff) >= 0x24)				{ state->rtc.day++; state->rtc.wday++; state->rtc.hour = 0; }
	if((state->rtc.wday & 0x0f) >= 0x07)				{ state->rtc.wday = 0; }
	if((state->rtc.day & 0x0f) >= 0x0a)					{ state->rtc.day+=0x10; state->rtc.day&=0xf0; }
	/* FIXME: very crude leap year support (i.e. it treats the RTC to be with a 2000-2099 timeline), dunno how the real x1 supports this,
       maybe it just have a 1980-1999 timeline since year 0x00 shows as a XX on display */
	if(((state->rtc.year % 4) == 0) && state->rtc.month == 2)
	{
		if((state->rtc.day & 0xff) >= dpm[state->rtc.month-1]+1+1)
			{ state->rtc.month++; state->rtc.day = 0x01; }
	}
	else if((state->rtc.day & 0xff) >= dpm[state->rtc.month-1]+1){ state->rtc.month++; state->rtc.day = 0x01; }
	if(state->rtc.month > 12)							{ state->rtc.year++;  state->rtc.month = 0x01; }
	if((state->rtc.year & 0x0f) >= 0x0a)				{ state->rtc.year+=0x10; state->rtc.year&=0xf0; }
	if((state->rtc.year & 0xf0) >= 0xa0)				{ state->rtc.year = 0; } //roll over
}

static MACHINE_RESET( x1 )
{
	x1_state *state = machine->driver_data<x1_state>();
	UINT8 *ROM = memory_region(machine, "maincpu");
	UINT8 *PCG_RAM = memory_region(machine, "pcg");
	int i;

	memory_set_bankptr(machine, "bank1", &ROM[0x00000]);

	memset(state->gfx_bitmap_ram,0x00,0xc000*2);

	for(i=0;i<0x1800;i++)
	{
		PCG_RAM[i] = 0;
	}

	for(i=0;i<0x0400;i++)
	{
		gfx_element_mark_dirty(machine->gfx[1], i);
	}

	/* X1 Turbo specific */
	state->scrn_reg.blackclip = 0;

	state->io_bank_mode = 0;
	state->pcg_index[0] = state->pcg_index[1] = state->pcg_index[2] = 0;

	//cpu_set_irq_callback(machine->device("maincpu"), x1_irq_callback);

	state->cmt_current_cmd = 0;
	state->cmt_test = 0;
	cassette_change_state(machine->device("cass" ),CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);

	state->key_irq_flag = state->ctc_irq_flag = 0;

	/* Reinitialize palette here if there's a soft reset for the Turbo PAL stuff*/
	for(i=0;i<8;i++)
	{
		palette_set_color_rgb(machine, i+0x000, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
		palette_set_color_rgb(machine, i+0x100, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
	}

	timer_adjust_periodic(state->rtc_timer, attotime_zero, 0, ATTOTIME_IN_SEC(1));
}

static MACHINE_START( x1 )
{
	x1_state *state = machine->driver_data<x1_state>();
	timer_pulse(machine, ATTOTIME_IN_HZ(240), NULL, 0, keyboard_callback);
	timer_pulse(machine, ATTOTIME_IN_HZ(16), NULL, 0, cmt_wind_timer);

	/* set up RTC */
	{
		system_time systime;
		machine->base_datetime(systime);

		state->rtc.day = ((systime.local_time.mday / 10)<<4) | ((systime.local_time.mday % 10) & 0xf);
		state->rtc.month = ((systime.local_time.month+1));
		state->rtc.wday = ((systime.local_time.weekday % 10) & 0xf);
		state->rtc.year = (((systime.local_time.year % 100)/10)<<4) | ((systime.local_time.year % 10) & 0xf);
		state->rtc.hour = ((systime.local_time.hour / 10)<<4) | ((systime.local_time.hour % 10) & 0xf);
		state->rtc.min = ((systime.local_time.minute / 10)<<4) | ((systime.local_time.minute % 10) & 0xf);
		state->rtc.sec = ((systime.local_time.second / 10)<<4) | ((systime.local_time.second % 10) & 0xf);

		state->rtc_timer = timer_alloc(machine, x1_rtc_increment, 0);
	}
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
}

static FLOPPY_OPTIONS_START( x1 )
	FLOPPY_OPTION( img2d, "2d", "2D disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config x1_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSDD_40,
	FLOPPY_OPTIONS_NAME(x1),
	"x1_flop"
};

static MACHINE_CONFIG_START( x1, x1_state )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, MAIN_CLOCK/4)
	MDRV_CPU_PROGRAM_MAP(x1_mem)
	MDRV_CPU_IO_MAP(x1_io)
	MDRV_CPU_VBLANK_INT("screen", x1_vbl)
	MDRV_CPU_CONFIG(x1_daisy)

	MDRV_Z80CTC_ADD( "ctc", MAIN_CLOCK/4 , ctc_intf )

	MDRV_DEVICE_ADD("x1kb", X1_KEYBOARD, 0)

	MDRV_I8255A_ADD( "ppi8255_0", ppi8255_intf )

	MDRV_MACHINE_START(x1)
	MDRV_MACHINE_RESET(x1)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_MC6845_ADD("crtc", H46505, (VDP_CLOCK/48), mc6845_intf) //unknown divider
	MDRV_PALETTE_LENGTH(0x300)
	MDRV_PALETTE_INIT(x1)

	MDRV_GFXDECODE(x1)

	MDRV_VIDEO_START(x1)
	MDRV_VIDEO_UPDATE(x1)
	MDRV_VIDEO_EOF(x1)

	MDRV_MB8877_ADD("fdc",x1_mb8877a_interface)

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom")
	MDRV_CARTSLOT_NOT_MANDATORY

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("ay", AY8910, MAIN_CLOCK/8)
	MDRV_SOUND_CONFIG(ay8910_config)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_WAVE_ADD("wave","cass")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_CASSETTE_ADD("cass",x1_cassette_config)
	MDRV_SOFTWARE_LIST_ADD("cass_list","x1_cass")

	MDRV_FLOPPY_4_DRIVES_ADD(x1_floppy_config)
	MDRV_SOFTWARE_LIST_ADD("flop_list","x1_flop")

MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( x1turbo, x1 )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_IO_MAP(x1turbo_io)
	MDRV_CPU_CONFIG(x1turbo_daisy)

	MDRV_Z80SIO_ADD( "sio", MAIN_CLOCK/4 , sio_intf )
	MDRV_Z80DMA_ADD( "dma", MAIN_CLOCK/4 , x1_dma )

	MDRV_DEVICE_REMOVE("fdc")
	MDRV_MB8877_ADD("fdc",x1turbo_mb8877a_interface)

	MDRV_SOUND_ADD("ym", YM2151, MAIN_CLOCK/8) //option board
//  MDRV_SOUND_CONFIG(ay8910_config)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

/*************************************
 *
 * ROM definitions
 *
 *************************************/

 ROM_START( x1 )
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.x1", 0x0000, 0x1000, CRC(7b28d9de) SHA1(c4db9a6e99873808c8022afd1c50fef556a8b44d) )

	ROM_REGION(0x1000, "mcu", ROMREGION_ERASEFF) //MCU for the Keyboard, "sub cpu"
	ROM_LOAD( "80c48", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION(0x1800, "pcg", ROMREGION_ERASEFF)

	ROM_REGION(0x2000, "font", 0) //TODO: this contains 8x16 charset only, maybe it's possible that it derivates a 8x8 charset by skipping gfx lines?
	ROM_LOAD( "ank.fnt", 0x0000, 0x2000, BAD_DUMP CRC(19689fbd) SHA1(0d4e072cd6195a24a1a9b68f1d37500caa60e599) )

	ROM_REGION(0x4d600, "cgrom", 0)
	ROM_LOAD("fnt0808.x1", 0x00000, 0x00800, CRC(e3995a57) SHA1(1c1a0d8c9f4c446ccd7470516b215ddca5052fb2) )
	ROM_RELOAD(            0x00800, 0x00800)
	ROM_RELOAD(            0x01000, 0x00800)

	ROM_REGION(0x20000, "kanji", ROMREGION_ERASEFF)

	ROM_REGION(0x20000, "raw_kanji", ROMREGION_ERASEFF)

	ROM_REGION( 0x1000000, "cart_img", ROMREGION_ERASE00 )
	ROM_CART_LOAD("cart", 0x0000, 0xffffff, ROM_OPTIONAL | ROM_NOMIRROR)
ROM_END

ROM_START( x1twin )
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.rom", 0x0000, 0x1000, CRC(e70011d3) SHA1(d3395e9aeb5b8bbba7654dd471bcd8af228ee69a) )

	ROM_REGION(0x1000, "mcu", ROMREGION_ERASEFF) //MCU for the Keyboard, "sub cpu"
	ROM_LOAD( "80c48", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION(0x1800, "pcg", ROMREGION_ERASEFF)

	ROM_REGION(0x1000, "font", 0) //TODO: this contains 8x16 charset only, maybe it's possible that it derivates a 8x8 charset by skipping gfx lines?
	ROM_LOAD( "ank16.rom", 0x0000, 0x1000, CRC(8f9fb213) SHA1(4f06d20c997a79ee6af954b69498147789bf1847) )

	ROM_REGION(0x4d600, "cgrom", 0)
	ROM_LOAD("ank8.rom", 0x00000, 0x00800, CRC(e3995a57) SHA1(1c1a0d8c9f4c446ccd7470516b215ddca5052fb2) )
	ROM_RELOAD(          0x00800, 0x00800)
	ROM_RELOAD(          0x01000, 0x00800)

	ROM_REGION(0x20000, "kanji", ROMREGION_ERASEFF)

	ROM_REGION(0x20000, "raw_kanji", ROMREGION_ERASEFF) // these comes from x1 turbo
	ROM_LOAD("kanji4.rom", 0x00000, 0x8000, BAD_DUMP CRC(3e39de89) SHA1(d3fd24892bb1948c4697dedf5ff065ff3eaf7562) )
	ROM_LOAD("kanji2.rom", 0x08000, 0x8000, BAD_DUMP CRC(e710628a) SHA1(103bbe459dc8da27a9400aa45b385255c18fcc75) )
	ROM_LOAD("kanji3.rom", 0x10000, 0x8000, BAD_DUMP CRC(8cae13ae) SHA1(273f3329c70b332f6a49a3a95e906bbfe3e9f0a1) )
	ROM_LOAD("kanji1.rom", 0x18000, 0x8000, BAD_DUMP CRC(5874f70b) SHA1(dad7ada1b70c45f1e9db11db273ef7b385ef4f17) )

	ROM_REGION( 0x1000000, "cart_img", ROMREGION_ERASE00 )
	ROM_CART_LOAD("cart", 0x0000, 0xffffff, ROM_OPTIONAL | ROM_NOMIRROR)
ROM_END

ROM_START( x1turbo )
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.x1t", 0x0000, 0x8000, CRC(2e8b767c) SHA1(44620f57a25f0bcac2b57ca2b0f1ebad3bf305d3) )

	ROM_REGION(0x1000, "mcu", ROMREGION_ERASEFF) //MCU for the Keyboard, "sub cpu"
	ROM_LOAD( "80c48", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION(0x1800, "pcg", ROMREGION_ERASEFF)

	ROM_REGION(0x2000, "font", 0) //TODO: this contains 8x16 charset only, maybe it's possible that it derivates a 8x8 charset by skipping gfx lines?
	ROM_LOAD( "ank.fnt", 0x0000, 0x2000, CRC(19689fbd) SHA1(0d4e072cd6195a24a1a9b68f1d37500caa60e599) )

	ROM_REGION(0x4d600, "cgrom", 0)
	ROM_LOAD("fnt0808_turbo.x1", 0x00000, 0x00800, CRC(84a47530) SHA1(06c0995adc7a6609d4272417fe3570ca43bd0454) )
	ROM_RELOAD(            0x00800, 0x00800)
	ROM_RELOAD(            0x01000, 0x00800)

	ROM_REGION(0x20000, "kanji", ROMREGION_ERASEFF)

	ROM_REGION(0x20000, "raw_kanji", ROMREGION_ERASEFF)
	ROM_LOAD("kanji4.rom", 0x00000, 0x8000, CRC(3e39de89) SHA1(d3fd24892bb1948c4697dedf5ff065ff3eaf7562) )
	ROM_LOAD("kanji2.rom", 0x08000, 0x8000, CRC(e710628a) SHA1(103bbe459dc8da27a9400aa45b385255c18fcc75) )
	ROM_LOAD("kanji3.rom", 0x10000, 0x8000, CRC(8cae13ae) SHA1(273f3329c70b332f6a49a3a95e906bbfe3e9f0a1) )
	ROM_LOAD("kanji1.rom", 0x18000, 0x8000, CRC(5874f70b) SHA1(dad7ada1b70c45f1e9db11db273ef7b385ef4f17) )

	ROM_REGION( 0x1000000, "cart_img", ROMREGION_ERASE00 )
	ROM_CART_LOAD("cart", 0x0000, 0xffffff, ROM_OPTIONAL | ROM_NOMIRROR)
ROM_END

ROM_START( x1turbo40 )
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.bin", 0x0000, 0x8000, CRC(112f80a2) SHA1(646cc3fb5d2d24ff4caa5167b0892a4196e9f843) )

	ROM_REGION(0x1000, "mcu", ROMREGION_ERASEFF) //MCU for the Keyboard, "sub cpu"
	ROM_LOAD( "80c48", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION(0x1800, "pcg", ROMREGION_ERASEFF)

	ROM_REGION(0x2000, "font", 0) //TODO: this contains 8x16 charset only, maybe it's possible that it derivates a 8x8 charset by skipping gfx lines?
	ROM_LOAD( "ank.fnt", 0x0000, 0x2000, CRC(19689fbd) SHA1(0d4e072cd6195a24a1a9b68f1d37500caa60e599) )

	ROM_REGION(0x1800, "cgrom", 0)
	ROM_LOAD("fnt0808_turbo.x1", 0x0000, 0x0800, CRC(84a47530) SHA1(06c0995adc7a6609d4272417fe3570ca43bd0454) )
	ROM_RELOAD(            0x00800, 0x00800)
	ROM_RELOAD(            0x01000, 0x00800)

	ROM_REGION(0x20000, "kanji", ROMREGION_ERASEFF)

	ROM_REGION(0x20000, "raw_kanji", ROMREGION_ERASEFF)
	ROM_LOAD("kanji4.rom", 0x00000, 0x8000, CRC(3e39de89) SHA1(d3fd24892bb1948c4697dedf5ff065ff3eaf7562) )
	ROM_LOAD("kanji2.rom", 0x08000, 0x8000, CRC(e710628a) SHA1(103bbe459dc8da27a9400aa45b385255c18fcc75) )
	ROM_LOAD("kanji3.rom", 0x10000, 0x8000, CRC(8cae13ae) SHA1(273f3329c70b332f6a49a3a95e906bbfe3e9f0a1) )
	ROM_LOAD("kanji1.rom", 0x18000, 0x8000, CRC(5874f70b) SHA1(dad7ada1b70c45f1e9db11db273ef7b385ef4f17) )

	ROM_REGION( 0x1000000, "cart_img", ROMREGION_ERASE00 )
	ROM_CART_LOAD("cart", 0x0000, 0xffffff, ROM_OPTIONAL | ROM_NOMIRROR)
ROM_END


/* Convert the ROM interleaving into something usable by the write handlers */
static DRIVER_INIT( kanji )
{
	UINT32 i,j,k,l;
	UINT8 *kanji = memory_region(machine,"kanji");
	UINT8 *raw_kanji = memory_region(machine,"raw_kanji");

	k = 0;
	for(l=0;l<2;l++)
	{
		for(j=l*16;j<(l*16)+0x10000;j+=32)
		{
			for(i=0;i<16;i++)
			{
				kanji[j+i] = raw_kanji[k];
				kanji[j+i+0x10000] = raw_kanji[0x10000+k];
				k++;
			}
		}
	}
}


/*    YEAR  NAME       PARENT  COMPAT   MACHINE  INPUT  INIT  COMPANY   FULLNAME      FLAGS */
COMP( 1982, x1,        0,      0,       x1,      x1,    0,    "Sharp",  "X1 (CZ-800C)",         0)
COMP( 1986, x1twin,    x1,     0,       x1, 	 x1,    kanji,"Sharp",  "X1 Twin (CZ-830C)",    GAME_NOT_WORKING)
COMP( 1984, x1turbo,   x1,     0,       x1turbo, x1,    kanji,"Sharp",  "X1 Turbo (CZ-850C)",   GAME_NOT_WORKING) //model 10
COMP( 1985, x1turbo40, x1,     0,       x1turbo, x1,    kanji,"Sharp",  "X1 Turbo (CZ-862C)",   GAME_NOT_WORKING) //model 40
