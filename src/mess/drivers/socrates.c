/******************************************************************************
*
*  V-tech Socrates Driver
*  By Jonathan Gevaryahu AKA Lord Nightmare
*  with dumping help from Kevin 'kevtris' Horton
*  
*  (driver structure copied from vtech1.c)
TODO:
	add waitstates for ram access (lack of this causes the system to run way too fast)
	hook up cartridges using the bios system
	hook up sound regs
	find and hook up keyboard/mouse IR input
	find and hook up any timers/interrupt controls
	find and document ports for speech synthesizer
	


  Socrates Educational Video System
        FFFF|----------------|
            | RAM (window 1) |
            |                |
        C000|----------------|
            | RAM (window 0) |
            |                |
        8000|----------------|
            | ROM (banked)   |
            | *Cartridge     |
        4000|----------------|
            | ROM (fixed)    |
            |                |
        0000|----------------|

	* cartridge lives in banks 10 onward, see below

        Banked rom area (4000-7fff) bankswitching
        Bankswitching is achieved by writing to I/O port 0 (mirrored on 1-7)
	Bank 	   ROM_REGION        Contents
	0          0x00000 - 0x03fff System ROM page 0
	1          0x04000 - 0x07fff System ROM page 1
	2          0x08000 - 0x0bfff System ROM page 2
        ... etc ...
	E          0x38000 - 0x38fff System ROM page E
	F          0x3c000 - 0x3ffff System ROM page F
       10          0x40000 - 0x43fff Expansion Cartridge page 0 (cart ROM 0x0000-0x3fff)
       11          0x44000 - 0x47fff Expansion Cartridge page 1 (cart ROM 0x4000-0x7fff)
        ... etc ...

        Banked ram area (z80 0x8000-0xbfff window 0 and z80 0xc000-0xffff window 1)
        Bankswitching is achieved by writing to I/O port 8 (mirrored to 9-F), only low nybble
        byte written: 0b****BBAA
        where BB controls ram window 1 and AA controls ram window 0
        hence:
        Write    [window 0]         [window 1]
        0        0x0000-0x3fff      0x0000-0x3fff
        1        0x4000-0x7fff      0x0000-0x3fff
        2        0x8000-0xbfff      0x0000-0x3fff
        3        0xc000-0xffff      0x0000-0x3fff
        4        0x0000-0x3fff      0x4000-0x7fff
        5        0x4000-0x7fff      0x4000-0x7fff
        6        0x8000-0xbfff      0x4000-0x7fff
        7        0xc000-0xffff      0x4000-0x7fff
        8        0x0000-0x3fff      0x8000-0xbfff
        9        0x4000-0x7fff      0x8000-0xbfff
        A        0x8000-0xbfff      0x8000-0xbfff
        B        0xc000-0xffff      0x8000-0xbfff
        C        0x0000-0x3fff      0xc000-0xffff
        D        0x4000-0x7fff      0xc000-0xffff
        E        0x8000-0xbfff      0xc000-0xffff
        F        0xc000-0xffff      0xc000-0xffff

******************************************************************************/

/* Core includes */
#include "driver.h"
#include "cpu/z80/z80.h"
#include "socrates.lh"
//#include "sound/beep.h"


/* Components */

/* Devices */

/* stuff below belongs in machine/socrates.c */
static struct
{
UINT8 data[8];
UINT8 rom_bank;
UINT8 ram_bank;
UINT16 scroll_offset;
UINT8* videoram;
UINT8 kb_latch_low;
UINT8 kb_latch_high;
UINT8 kb_latch_mouse;
} socrates={ {0}};

static void socrates_set_rom_bank( running_machine *machine )
{
 memory_set_bankptr( machine, 1, memory_region(machine, "maincpu") + ( socrates.rom_bank * 0x4000 ));
}

static void socrates_set_ram_bank( running_machine *machine )
{
 memory_set_bankptr( machine, 2, memory_region(machine, "vram") + ( (socrates.ram_bank&0x3) * 0x4000 )); // window 0
 memory_set_bankptr( machine, 3, memory_region(machine, "vram") + ( ((socrates.ram_bank&0xC)>>2) * 0x4000 )); // window 1
}

MACHINE_RESET( socrates )
{
 socrates.rom_bank = 0; // actually set semi-randomly on real console but we need to initialize it somewhere...
 socrates_set_rom_bank( machine );
 socrates.ram_bank = 0;  // the actual console sets it semi randomly on power up, and the bios cleans it up.
 socrates_set_ram_bank( machine );
 socrates.kb_latch_low = 0; // this is really random on startup but its best to set it to the 'sane' 00 01 value
 socrates.kb_latch_high = 1; // this is really random on startup but its best to set it to the 'sane' 00 01 value
 socrates.kb_latch_mouse = 0;
}

DRIVER_INIT( socrates )
{
	UINT8 *gfx = memory_region(machine, "vram");
	int i;
    /* fill vram with its init powerup bit pattern, so startup has the checkerboard screen */
    for (i = 0; i < 0x10000; i++)
        gfx[i] = (((i&0x1)?0x00:0xFF)^((i&0x100)?0x00:0xff));
// init sound channels to both be on lowest pitch and max volume
    cpu_set_clockscale(cputag_get_cpu(machine, "maincpu"), 0.45f); /* RAM access waitstates etc. aren't emulated - slow the CPU to compensate */
}

READ8_HANDLER( socrates_rom_bank_r )
{
 UINT8 data = 0xFF;
 data = socrates.rom_bank;
 return data;
}

WRITE8_HANDLER( socrates_rom_bank_w )
{
 socrates.rom_bank = data;
 socrates_set_rom_bank( space->machine );
}

READ8_HANDLER( socrates_ram_bank_r )
{
 UINT8 data = 0xFF;
 data = socrates.ram_bank;
 return data;
}

WRITE8_HANDLER( socrates_ram_bank_w )
{
 socrates.ram_bank = data&0xF;
 socrates_set_ram_bank( space->machine );
}

READ8_HANDLER( read_f3 ) // used for read-only i/o ports as mame/mess doesn't have a way to set the unmapped area to read as 0xF3
{
 return 0xF3;
}

WRITE8_HANDLER( unknownlatch_30 ) // writes to i/o 0x3x do SOMETHING, probably reset a latch, but I don't know yet
{
//popmessage("write to i/o 0x30");
}

READ8_HANDLER( socrates_keyboard_50_r ) // keyboard code low
{
 return socrates.kb_latch_low;
}

READ8_HANDLER( socrates_keyboard_51_r ) // keyboard code high
{
 return socrates.kb_latch_high;
}

WRITE8_HANDLER( socrates_keyboard_clear ) // keyboard latch clear (or show mouse coords next if they have updated)
{
	if ((data&0x2) == 0x2)
	{
		if (socrates.kb_latch_mouse == 0)
		{
			socrates.kb_latch_low = 0;
			socrates.kb_latch_high = 1;
		}
		else
		{
			socrates.kb_latch_low = socrates.kb_latch_mouse&0xFF; // y coord
			socrates.kb_latch_high = (socrates.kb_latch_mouse&0xFF00)>>8; // x coord
			socrates.kb_latch_mouse = 0;
		}
	}
}

WRITE8_HANDLER( unknown_6x ) // writes to i/o 0x6x happens on startup once, with 0x01. no idea what it does.
{
//popmessage("write to i/o 0x60");
}

/* stuff below belongs in video/socrates.c */

WRITE8_HANDLER( socrates_scroll_w )
{
 if (offset == 0)
 socrates.scroll_offset = (socrates.scroll_offset&0x100) | data;
 else
 socrates.scroll_offset = (socrates.scroll_offset&0xFF) | ((data&1)<<8);
}

/* NTSC-based Palette stuff */
// max for I and Q
#define M_I 0.5957
#define M_Q 0.5226 
 /* luma amplitudes, measured on scope */
#define LUMAMAX 1.420
#define LUMA_COL_0 0.355, 0.139, 0.205, 0, 0.569, 0.355, 0.419, 0.205, 0.502, 0.288, 0.358, 0.142, 0.720, 0.502, 0.571, 0.358,
#define LUMA_COL_COMMON 0.52, 0.52, 0.52, 0.52, 0.734, 0.734, 0.734, 0.734, 0.667, 0.667, 0.667, 0.667, 0.885, 0.885, 0.885, 0.885,
#define LUMA_COL_2 0.574, 0.6565, 0.625, 0.71, 0.792, 0.87, 0.8425, 0.925, 0.724, 0.8055, 0.7825, 0.865, 0.94275, 1.0225, 0.99555, 1.07525,
#define LUMA_COL_5 0.4585, 0.382, 0.4065, 0.337, 0.6715, 0.5975, 0.6205, 0.5465, 0.6075, 0.531, 0.5555, 0.45, 0.8255, 0.7455, 0.774, 0.6985,
#define LUMA_COL_F 0.690, 0.904, 0.830, 1.053, 0.910, 1.120, 1.053, 1.270, 0.840, 1.053, 0.990, 1.202, 1.053, 1.270, 1.202, 1.420 
 /* chroma amplitudes, measured on scope */
#define CHROMAMAX 0.42075
#define CHROMA_COL_COMMON 0.148, 0.3125, 0.26475, 0.42075, 0.148, 0.3125, 0.26475, 0.42075, 0.148, 0.3125, 0.26475, 0.42075, 0.148, 0.3125, 0.26475, 0.42075,
#define CHROMA_COL_2 0.125125, 0.27525, 0.230225, 0.384875, 0.125125, 0.27525, 0.230225, 0.384875, 0.125125, 0.27525, 0.230225, 0.384875, 0.125125, 0.27525, 0.230225, 0.384875,
#define CHROMA_COL_5 0.1235, 0.2695, 0.22625, 0.378, 0.1235, 0.2695, 0.22625, 0.378, 0.1235, 0.2695, 0.22625, 0.378, 0.1235, 0.2695, 0.22625, 0.378,
// gamma: this needs to be messed with... may differ on different systems... attach to slider somehow?
#define GAMMA 1.5 

static rgb_t socrates_create_color(UINT8 color)
{
rgb_t composedcolor;
double lumatable[256] = { 
    LUMA_COL_0
    LUMA_COL_COMMON
    LUMA_COL_2
    LUMA_COL_COMMON
    LUMA_COL_COMMON
    LUMA_COL_5
    LUMA_COL_COMMON
    LUMA_COL_COMMON
    LUMA_COL_COMMON
    LUMA_COL_COMMON
    LUMA_COL_COMMON
    LUMA_COL_COMMON
    LUMA_COL_COMMON
    LUMA_COL_COMMON
    LUMA_COL_COMMON
    LUMA_COL_F
  };
double chromaintensity[256] = { 
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    CHROMA_COL_COMMON
    CHROMA_COL_2
    CHROMA_COL_COMMON
    CHROMA_COL_COMMON
    CHROMA_COL_5
    CHROMA_COL_COMMON
    CHROMA_COL_COMMON
    CHROMA_COL_COMMON
    CHROMA_COL_COMMON
    CHROMA_COL_COMMON
    CHROMA_COL_COMMON
    CHROMA_COL_COMMON
    CHROMA_COL_COMMON
    CHROMA_COL_COMMON
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  };
  /* chroma colors and phases:
     0: black-thru-grey (0 assumed chroma)
     1: purple (90 chroma seems correct)
     2: light blue/green (210 or 240 chroma, 210 seems slightly closer)
     3: bright blue (150 seems correct)
     4: green (270 seems correct)
     5: red (30 seems correct, does have some blue in it)
     6: orange (0 seems correct, does have some red in it)
     7: yellow/gold (330 is closest but conflicts with color C, hence 315 seems close, and must have its own delay line separate from the other phases which use a standard 12 phase scheme)
     8: blue with a hint of green in it (180 seems correct)
     9: blue-green (210 seems correct)
     A: forest green (240 seems correct)
     B: yellow-green (300 seems correct)
     C: yellow-orange (330 is close but this conflicts with color 7, and is not quite the same; color 7 has more green in it than color C)
     D: magenta (60 is closest)
     E: blue-purple (more blue than color 1, 120 is closest)
     F: grey-thru-white (0 assumed chroma)
  */
  double phaseangle[16] = { 0, 90, 210, 150, 270, 30, 0, 315, 180, 210, 240, 300, 330, 60, 120, 0 }; // note: these are guessed, not measured yet!
  int chromaindex = color&0x0F;
  int swappedcolor = ((color&0xf0)>>4)|((color&0x0f)<<4);
  double finalY, finalI, finalQ, finalR, finalG, finalB;
  finalY = (1/LUMAMAX) * lumatable[swappedcolor];
  finalI = (M_I * (cos((phaseangle[chromaindex]/180)*3.141592653589793)))* ((1/CHROMAMAX)*chromaintensity[swappedcolor]);
  finalQ = (M_Q * (sin((phaseangle[chromaindex]/180)*3.141592653589793)))* ((1/CHROMAMAX)*chromaintensity[swappedcolor]);
  if (finalY > 1) finalY = 1; // clamp luma
  /* calculate the R, G and B values here, neato matrix math */
  finalR = (finalY*1)+(finalI*0.9563)+(finalQ*0.6210);
  finalG = (finalY*1)+(finalI*-0.2721)+(finalQ*-0.6474);
  finalB = (finalY*1)+(finalI*-1.1070)+(finalQ*1.7046);
  /* scale/clamp to 0-255 range */
  if (finalR<0) finalR = 0;
  if (finalR>1) finalR = 1;
  if (finalG<0) finalG = 0;
  if (finalG>1) finalG = 1;
  if (finalB<0) finalB = 0;
  if (finalB>1) finalB = 1;
  // gamma correction: 1.0 to GAMMA:
  finalR = pow(finalR, 1/GAMMA)*255;
  finalG = pow(finalG, 1/GAMMA)*255;
  finalB = pow(finalB, 1/GAMMA)*255;
composedcolor = MAKE_RGB((int)finalR,(int)finalG,(int)finalB);
return composedcolor;
}

static rgb_t socrates_palette[256];

PALETTE_INIT( socrates )
{
	int i; // iterator
	for (i = 0; i < 256; i++)
	{
	 socrates_palette[i] = socrates_create_color(i);
	}
	palette_set_colors(machine, 0, socrates_palette, ARRAY_LENGTH(socrates_palette));
}

static VIDEO_START( socrates )
{
	socrates.videoram = memory_region(machine, "vram");
	socrates.scroll_offset = 0;
}

static VIDEO_UPDATE( socrates )
{
	UINT8 fixedcolors[8] =
	{
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0xF7
	};
	int x, y, colidx, color;
	int lineoffset = 0; // if display ever tries to display data at 0xfxxx, offset line displayed by 0x1000
	for (y = 0; y < 228; y++)
	{
		if ((((y+socrates.scroll_offset)*128)&0xffff) >= 0xf000) lineoffset = 0x1000; // see comment above
		for (x = 0; x < 256; x++)
		{
			colidx = socrates.videoram[(((y+socrates.scroll_offset)*128)+(x>>1)+lineoffset)&0xffff];
			if (x&1) colidx >>=4;
			colidx &= 0xF;
			if (colidx > 7) color=socrates.videoram[0xF000+(colidx<<8)+((y+socrates.scroll_offset)&0xFF)];
			else color=fixedcolors[colidx];
			*BITMAP_ADDR16(bitmap, y,  x) = color;
		}
	}
	return 0;
}

/* below belongs in sound/socrates.c */

static WRITE8_HANDLER(socrates_sound_w)
{
	switch(offset)
	{
		case 0:
		//beep_set_frequency(channel1, (int)((13982.6)/(data+1)));
		break;
		case 1:
		//beep_set_frequency(channel2, (int)((13982.6)/(data+1)));
		break;
		case 2:
		//beep_set_volume(channel1, data/4);
		break;
		case 3:
		//beep_set_volume(channel2, data/8);
		break;
		case 4: case 5: case 6: case 7: default:
		break;
	}
}

/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(z80_mem, ADDRESS_SPACE_PROGRAM, 8)
    ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE(0x0000, 0x3fff) AM_ROM /* system rom, bank 0 (fixed) */
    AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(1) /* banked rom space; system rom is banks 0 thru F, cartridge rom is banks 10 onward, usually banks 10 thru 17. area past the end of the cartridge, and the whole 10-ff area when no cartridge is inserted, reads as 0xF3 */
    AM_RANGE(0x8000, 0xbfff) AM_RAMBANK(2) /* banked ram 'window' 0 */
    AM_RANGE(0xc000, 0xffff) AM_RAMBANK(3) /* banked ram 'window' 1 */
ADDRESS_MAP_END

static ADDRESS_MAP_START(z80_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(socrates_rom_bank_r, socrates_rom_bank_w) AM_MIRROR(0x7) /* rom bank select - RW - 8 bits */
	AM_RANGE(0x08, 0x08) AM_READWRITE(socrates_ram_bank_r, socrates_ram_bank_w) AM_MIRROR(0x7) /* ram banks select - RW - 4 low bits; Format: 0b****HHLL where LL controls whether window 0 points at ram area: 0b00: 0x0000-0x3fff; 0b01: 0x4000-0x7fff; 0b10: 0x8000-0xbfff; 0b11: 0xc000-0xffff. HH controls the same thing for window 1 */
	AM_RANGE(0x10, 0x17) AM_READWRITE(read_f3, socrates_sound_w) AM_MIRROR (0x8) /* sound section:
        0x10 - W - frequency control for channel 1 (louder channel) - 01=high pitch, ff=low; time between 1->0/0->1 transitions = (XTAL_21_4772MHz/(512+256) / (freq_reg+1)) (note that this is double the actual frequency since each full low and high squarewave pulse is two transitions)
	0x11 - W - frequency control for channel 2 (softer channel) - 01=high pitch, ff=low; same equation as above
	0x12 - W - 0b****VVVV volume control for channel 1
	0x13 - W - 0b****VVVV volume control for channel 2
	0x14-0x17 - ?DAC? related? makes noise when written to... code implies 0x16 and 0x17 are mirrors of 0x14 and 0x15 respectively.
	*/
	AM_RANGE(0x20, 0x21) AM_READWRITE(read_f3, socrates_scroll_w) AM_MIRROR (0xe) /* graphics section:
	0x20 - W - lsb offset of screen display
	0x21 - W - msb offset of screen display
	resulting screen line is one of 512 total offsets on 128-byte boundaries in the whole 64k ram
	*/
	AM_RANGE(0x30, 0x30) AM_READWRITE(read_f3, unknownlatch_30) AM_MIRROR (0xf) /* unknown, write only */
	//AM_RANGE(0x40, 0x40) AM_RAM AM_MIRROR(0xF)/* unknown, read and write low 4 bits plus bit 5, bit 7 seems to be fixed at 0, bit 6 and 4 are fixed at 1? is this some sort of control register for timers perhaps? gets a slew of data written to it a few times during startup, may be IR or timer related? */
	AM_RANGE(0x50, 0x50) AM_READWRITE(socrates_keyboard_50_r, socrates_keyboard_clear) AM_MIRROR(0xE) /* Keyboard keycode low, latched on keypress, can only be unlatched by writing 0F here */
	AM_RANGE(0x51, 0x51) AM_READWRITE(socrates_keyboard_51_r, socrates_keyboard_clear) AM_MIRROR(0xE) /* Keyboard keycode high, latched as above */
	AM_RANGE(0x60, 0x60) AM_READWRITE(read_f3, unknown_6x) AM_MIRROR(0xF) /* unknown, write only  */
	AM_RANGE(0x70, 0xFF) AM_READ(read_f3) // nothing mapped here afaik
ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/

/* socrates keyboard codes:
keycode low
|   keycode high
|   |   key name
00	01	No key pressed
// pads on the sides of the kb; this acts like a giant bitfield, both dpads/buttons can send data at once
00	81	left dpad right
00	82	left dpad up 
00	84	left dpad down
00	88	left dpad left
01	80	right dpad down
02	80	right dpad left
04	80	right dpad up
08	80	right dpad right
10	80	left red button
20	80	right red button
// top row (right to left)
44	82	ENTER
44	83	MENU
44	84	ANSWER
44	85	HELP
44	86	ERASE
44	87	divide_sign
44	88	multiply_sign
44	89	minus_sign
44	80	plus_sign
//second row (right to left)
43	81	0
43	82	9
43	83	8
43	84	7
43	85	6
43	86	5
43	87	4
43	88	3
43	89	2
43	80	1
// third row (right to left)
42	82	I/La
42	83	H/So
42	84	G/Fa
42	85	F/Mi
42	86	E/Re
42	87	D/Do
42	88	C/Ti.
42	89	B/La.
42	80	A/So.
42	81	hyphen/period
// fourth row (right to left)
41	81	S
41	82	R
41	83	Q/NEW
41	84	P/PLAY
41	85	O/PAUSE
41	86	N/Fa`
41	87	M/Mi`
41	88	L/Re`
41	89	K/Do`
41	80	J/Ti
// fifth row (right to left)
40	82	SPACE
40	83	Z
40	84	Y
40	85	X
40	86	W
40	87	V
40	88	U
40	89	T
50	80	SHIFT
// socrates mouse pad (separate from keyboard)
8x	8y	mouse movement
x: down = 1 (small) thru 7 (large), up = 8 (small) thru F (large)
y: right = 1 (small) thru 7 (large), left = 8 (small) thru F (large)
90	80	right click
A0	80	left click
B0	80	both buttons click
90	81	right click (mouse movement in queue, will be in regs after next latch clear)
A0	81	left click (mouse movement in queue, will be in regs after next latch clear)
B0	81	both buttons click (mouse movement in queue, will be in regs after next latch clear)
// socrates touch pad
// unknown yet, but probably uses the 60/70/c0/d0/e0/f0 low reg vals
*/

/******************************************************************************
 Machine Drivers
******************************************************************************/
static TIMER_CALLBACK( clear_irq_cb )
{
	cputag_set_input_line(machine, "maincpu", 0, CLEAR_LINE);
}

static INTERRUPT_GEN( assert_irq )
{
	cpu_set_input_line(device, 0, ASSERT_LINE);
	timer_set(device->machine, cpu_clocks_to_attotime(device, 144), NULL, 0, clear_irq_cb);
// 144 is a complete and total guess, need to properly measure how many clocks/microseconds the int line is high for.
}

static MACHINE_DRIVER_START(socrates)
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, XTAL_21_4772MHz/6)  /* Toshiba TMPZ84C00AP @ 3.579545 MHz, verified, xtal is divided by 6 */
    MDRV_CPU_PROGRAM_MAP(z80_mem)
    MDRV_CPU_IO_MAP(z80_io)
    MDRV_QUANTUM_TIME(HZ(60))
    MDRV_CPU_VBLANK_INT("screen", assert_irq)
    //MDRV_MACHINE_START(socrates)
    MDRV_MACHINE_RESET(socrates)

    /* video hardware */
	//MDRV_DEFAULT_LAYOUT(layout_socrates)
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 228)
	MDRV_SCREEN_VISIBLE_AREA(0, 255, 0, 227)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(socrates)
	MDRV_VIDEO_START(socrates)
	MDRV_VIDEO_UPDATE(socrates)

    /* sound hardware */
	//MDRV_SPEAKER_STANDARD_MONO("mono")
	//MDRV_SOUND_ADD("beep", BEEP, 0)
	//MDRV_SOUND_ADD("beep2", BEEP, 0)
	//MDRV_SOUND_ADD("soc_snd", SOCRATES, XTAL_21_4772MHz/(512+256)) /* this is correct, as strange as it sounds. */
	//MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)


MACHINE_DRIVER_END



/******************************************************************************
 ROM Definitions
******************************************************************************/

ROM_START(socrates)
    ROM_REGION(0x80000, "maincpu", 0)
    /* Socrates US NTSC */
    ROM_LOAD("27-00817-000-000.u1", 0x00000, 0x40000, CRC(80f5aa20) SHA1(4fd1ff7f78b5dd2582d5de6f30633e4e4f34ca8f))
    ROM_FILL(0x40000, 0x40000, 0xf3) /* fill empty space with 0xf3 */

    ROM_REGION(0x10000, "vram", 0)
    ROM_FILL(0x0000, 0xffff, 0xff) /* fill with ff, driver_init changes this to the 'correct' startup pattern */
ROM_END
    
ROM_START(socratfc)
    ROM_REGION(0x80000, "maincpu", 0)
    /* Socrates SAITOUT (French Canadian) NTSC */
    ROM_LOAD("socratfc.u1", 0x00000, 0x40000, CRC(042d9d21) SHA1(9ffc67b2721683b2536727d0592798fbc4d061cb)) /* fix label/name */
    ROM_FILL(0x40000, 0x40000, 0xf3) /* fill empty space with 0xf3 */

    ROM_REGION(0x10000, "vram", 0)
    ROM_FILL(0x0000, 0xffff, 0xff) /* fill with ff, driver_init changes this to the 'correct' startup pattern */
ROM_END



/******************************************************************************
 System Config
******************************************************************************/
static SYSTEM_CONFIG_START(socrates)
// write me!
SYSTEM_CONFIG_END


/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT CONFIG      COMPANY                     FULLNAME                            FLAGS */
COMP( 1988, socrates,   0,          0,      socrates,   0, socrates,   socrates,   "V-tech",        "Socrates Educational Video System",                        GAME_NOT_WORKING )
COMP( 1988, socratfc,   socrates,   0,      socrates,   0, socrates,   socrates,   "V-tech",        "Socrates SAITOUT",                        GAME_NOT_WORKING )
