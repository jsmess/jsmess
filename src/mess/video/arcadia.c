/******************************************************************************
 PeT mess@utanet.at 2001
******************************************************************************/
#include "driver.h"

#include "includes/arcadia.h"

/*
  emulation of signetics 2637 video/audio device

  seams to me like a special microcontroller
  mask programmed
  1k x 8 ram available; only first 0x300 bytes mapped to cpu
 */

/*
0x18fe bit 4 set means sound off?

Sound is implemented on the latest code version, but I lost the
source to it. I *think* it's a straightforward value to frequency
calculation, might be 7874/(n+1) (from the Acetronic system),
but it is almost certainly something like

Clock Freq / (2^Something) / (Byte Data + 1)

It is more or less right on the release version, so a bit of
experimenting should come up with the right code.

I also believe (and this isn't clear from the tech I think) that
the CPU is frozen during display time.

PSR


             Technical Information about the Emerson Arcadia 2001

Written by Paul Robson (autismuk@aol.com)

Created on 25th June 1998.
Last updated on 22nd July 1998.

Introduction
------------

This document is an attempt to exactly describe the internal workings of
the Emerson Arcadia 2001 Videogame Console.

1. General
----------

- Signetics 2650 CPU at 3.58 Mhz
- 1k x 8 of RAM (physically present but mostly unavailable :-( )
- 128 x 208 pixel resolution (alternate 128 x 104 mode available)
- 16 x 26 background display (2 colour 8 x 8 pixel characters) (can be 16x13=
)
- 4 x 8 x 8 Sprites (2 colour 8 x 8 pixels)
- total of 8 user defined characters available... jaw drops in amazement.
- 2 x 2 axis Analogue Joysticks
- 2 x 12 button Controllers
- 3 buttons on system unit and CPU Reset
- Single channel beeper

2. Memory map
-------------

The memory map of the 2001 is below.

0000..0FFF      4k ROM Block 1 (first 4k of a cartridge)
1000..17FF      (Unmapped)
1800..18CF      Screen display , upper 13 lines, characters/palette high bit=
s
18D0..18EF      Free for user programs.
18F0..18F7      Sprite coordinates y0x0y1x1y2x2y3x3 (Section 10).
                The sprite uses a UDG 0 =3D $1980,1 =3D $1988,2 =3D $1990 3=20=
=3D $1998
18F8..18FB      User memory.
18FC            A control byte, function unknown [is almost always EE ED or=20=
EF]
18FD..18FE      Sound Hardware Pitch & Volume. The 3 high bits of 18FE contr=
ol
                the horizontal scanline scrolling.
18FF            Current Character Line
1900..1902      Keypad Player 1
1903            Unmapped
1904..1906      Keypad Player 2
1907            Unmapped
1908            Controller Buttons
1909..197F      Unmapped
1980..19BF      User defined characters (8 possible, 8 bytes per character)
19C0..19F7      Unknown usage - not used for anything, apparently.
19F8            Screen resolution (bit 6)
19F9            Background Colour / Colour #0 (bits 0..5)
19FA            Sprite Colour 2 & 3 (bits 0..5)
19FB            Sprite Colour 0 & 1 (bits 0..5)
19FC..19FD      Sprite-Sprite & Sprite-Background Collision bits (see later)
19FE..19FF      Paddles (read only) 19FE is Player 2 , 19FF is Player 1
1A00..1ACF      Screen display , lower 13 lines, characters/palette high bit=
s
1AD0..1AFF      User memory
1B00..1FFF      Unmapped
2000..2FFF      4k ROM Block 2 (for 8k carts such as Jungler)
3000..7FFF      Unknown. The flyers talk about the ability to have 28k
                carts. This would only be feasible if 0000-7FFF was all ROM
                except for the '1111' page.

3. Video Memory
---------------

The screen table is at 1800-CF and 1A00-CF. Each page has 13 lines of the
screen (16 bytes per line,26 lines in total, 208 scan lines). The 2 most
significant bits of each byte are colour data, the 6 least significant
are character data. The resolution of the Arcadia is 128 x 208 pixels.

It is possible to halve the screen resolution so 1A00..1ACF can
be used for code. This is controlled by bit 6 of $19F8.

The byte at location $18FF is the current Character Line address, lower
4 bits only. The start line goes from 1800 to 18C0 then from 1900 to 19C0.
The 4 least significant bits of this count 0123456789ABC0123456789ABC,
going to D when in vertical blank. The 4 most significant bits are always
'1'. Some games do use this for scrolling effects - a good example of this
is the routine at $010F in Alien Invaders which scrolls the various bits of
the screen about using the memory locations $18FF and $18FE.

The screen can be scrolled upto 7 pixels to the left using the high 3 bits
of $18FE. This is used in Alien Invaders.

A line beginning with $C0 contains block graphics. Each square contains
3 wide x 2 high pixels, coloured as normal. The 3 least significant bits
are the top, the next 3 bits are the bottom. Alien Invaders uses this for
shields. The graphics are returned to normal for the next line.

The VBlank signal (maybe VSYNC) is connected to the SENSE input. This is
logic '1' when the system is in VBLANK.

The Flag line does.... something graphical - it might make the sprites
half width/double height perhaps. Breakaway sets this when the bats are
double size in vertical mode.

4. Character codes
------------------

Character codes 00..37 to be in a ROM somewhere in the Emerson. These
are known, others may be discovered by comparing the screen snapshots
against the character tables. If the emulator displays an exclamation
mark you've found one. Get a snapshot to see what it looks like
normally and let me know. Codes 38..3F are taken from RAM.

00      (space)
01..0F  Graphic Characters
10..19  0..9
1A..33  A..Z
34      Decimal Point
35..37  Unknown - Control Characters ?
38..3F  User Defined Characters (8 off, from 1980..19FF)

Character data is stored 8 bits per character , as a single plane graphic
The 2nd and 3rd bits of palette data come from the screen tables, so there
are two colours per character and 4 possible palette selections for the
background.

A character set is available from Paul Robson (autismuk@aol.com) on
request.

5. ROM Images
-------------

ROM Images are loaded into 0000-0FFF. If the ROM is an 8k ROM it is
also loaded into 2000-2FFF. I do not know how 12k images or greater
are mapped but I doubt that many exist (there is very little RAM
available for user programs).

6. Controls
-----------

All key controls are indicated by a bit going to '1'. Unused bits at
the memory location are set to zero.

Keypads
-------

1900-1902 (Player 1) 1904-1906 (Player 2)

The keypads are arranged as follows :-

        1       2       3
        4       5       6
        7       8       9
      Enter     0     Clear

Row 1/4/7 is 1900/4, Row 2/5/8/0 is 1901/5 and Row 3/6/9 is 1902/6
The topmost key is bit 3, the lowermost key is bit 0.

Location $1908 contains bit 0 Start,bit 1 Option,bit 2 Difficulty.
These keys are "latched" i.e. a press causes a logic 1 to appear
on the current frame.

The paddles are mapped onto 19FE (player 2) and 19FF (player 1). The
currently selected direction seems to toggle every frame, and the
only way it seems to be possible to work out which axis is being
accessed is to count the number of frames. Each game I have tried
is doing this. "Cat Tracks" swaps the paddles every game ; the reason
is as yet unknown.

The fire buttons are equivalent to Keypad #2 e.g. they are 1901 and
1905 bit 3.

7. Sound
--------

Single channel buzz. Need to do more with this !

18FC control byte ?
18FD pitch
18FE volume ? (lower 4 bits)

8. Sprites
----------

Sprite pointers are at 18F0..18F7 (there are four of them). The graphics
used are the ones in the 1980..19BF UDG table (the first four).

Sprite addresses (x,y) are converted to offsets in the 128 x 208 as follows=20=
:-

1) 1's complement the y coordinate
2) subtract 16 from the y coordinate
3) subtract 44 from the x coordinate

9. Palette
----------

The Palette is encoded between 19F8-19FB. This section describes the method
by which colours are allocated. There are 8 colours, information is coded 3=20
bits per colour (usually 2 colours per byte)

 Colour Code    Name    Colour Elements
 ------ ----    ----    ---------------
  7     111     Black   (GRB =3D 000)
  6     110     Blue    (GRB =3D 001)
  5     101     Red     (GRB =3D 010)
  4     100     Magenta (GRB =3D 011)
  3     011     Green   (GRB =3D 100)
  2     010     Cyan    (GRB =3D 101)
  1     001     Yellow  (GRB =3D 110)
  0     000     White   (GRB =3D 111)

Bits 0..2 of $19F9 are the background colour
Bits 3..5 of $19F9 are the colours of tile set 0
                tiles 1,2 and 3 are generated by adding 2,4,6 to the colour.
                this is probably what the xor gates are for !
Bits 0..5 of $19FB are the colours of Sprites 0 & 1 (sprite 1 is low bits)
Bits 0..5 of $19FA are the colours of Sprites 2 & 3 (sprite 3 is low bits)

Bits 6..7 of $19F9..B and all of $19F8 (except bit 6) currently has
no known function. Brightness ???

10. Collision Detection
-----------------------

Bits are set to zero on a collision - I think they are reset at the
frame start. There are two locations : one is for sprite/background
collisions, one is for sprite/sprite collisions.

19FC    bits 0..3 are collision between sprites 0..3 and the background.

19FD    bit 0 is sprite 0 / 1 collision
        bit 1 is sprite 0 / 2 collision
        bit 2 is sprite 0 / 3 collision
        bit 3 is sprite 1 / 2 collision
        bit 4 is sprite 1 / 3 collision (guess)
        bit 5 is sprite 2 / 3 collision (guess)

11. Other information
---------------------

Interrupts are not supported.. maybe. All cartridges have a RETC UN
at 0003 (presumably the interrupt vector). This may be called on VSYNC
or VBLANK, but until I find a cart which uses it, who cares !

The Read/Write 2650 CPU Commands do not appear to be connected to
anything in hardware. No cartridge has been found which uses them.
The emulator crashes to the debugger with these commands, as it does
with any illegal commands.

12. Frame Description
---------------------

This describes the frame rendition (for NTSC). The description Starts at
the end of a display frame, e.g. scanline 216 on the diagram. For a PAL
machine these will be slightly different - however I don't think that
this would affect anything because of the way timing is usually done on
games, and the fact that scanline level operations are virtually
impossible.

1.   Set the VBLANK (SENSE) line to logic 1, indicating entry of vblank
2.   Set the current character address to $FD (off screen)
3.   Wait for 46+8 Scanlines. (see frame diagram)
4.   Set the VBLANK (SENSE) line to logic 0.
5.   Reset the Collision Bytes to $FF
6.   For 1800..18C0, then for 1A00..1AC0 (if not low resolution)
6a.    Set the current character address to $Fx
6b.    wait for 8 (standard) or 16 (low resolution) Scanlines
6c.    Render the line using the current character & vertical scroll data
6d.    Draw all sprites which terminate in the area just drawn.
7.   Restart the frame (from Stage 1).

Stage 5 can be replaced by execute 208 Scanlines if this is not a 'redraw'
frame.

0       ---------------------------
        ! Initial Vertical Blank  !
8       ---------------------------
        ! Display (208 scanlines) !
216     ---------------------------
        !     Frame Retrace       !
262     ---------------------------

---------------------------------------------------------------

pet additions:
Golf has a 4kbyte block at 0x0000 and a 2kbyte block at 0x4000.

19f9 bit 6
switches between the axes of the analog stick
of both? players
(must not affect ad converter immediately)

Sprites are used around the playfield a lot
(space vultures, grand slam tennis, space attack, ...)
some cartridges think the left 16? pixels are not displayed
so I display a 32 pixel frame left and right,
a 8 pixel frame at the top and a bottom frame of 24 pixel

character color ((ch&0xc0)>>5) | ((mem[0x19f9]&8)>>3)

1903 palladium player 1 4 additional keys
1907 palladium player 2 4 additional keys

19f8 bit 6
 off 13 charlines
 on  26 charlines
19f9
 bit 7
  off doublescan
  on no doublescan

19f8 bit 7 graphics mode on (lower 6 bits descripe rectangles)
0xc0 in line switches to graphics mode in this line
0x40 in line switches to char mode in this line
 22111000
 22111000
 22111000
 22111000
 55444333
 55444333
 55444333
 55444333

sprite doublescanned (vertical enlarged)
sprite 0 0x19fb bit 7
sprite 1 0x19fb bit 6
sprite 2 0x19fa bit 7
sprite 3 0x19fa bit 6

18fc crtc vertical position register
in PAL palladium
 0xff 16*16+6 visible
  about 8 lines above and below
 0x00 in PAL palladium
  7 lines visible

18fd
 bit 7 on
  alternate character mode color2x2
  (2 backgrounds, 2 foreground colors)

color2x2
 character bit 7 -> background select
  0 19f8 bits 2..0
  1 19f9 bits 2..0
 character bit 6 -> foreground select
  0 19f8 bits 5..3
  1 19f9 bits 5..3

normal mode:
 character bit 7..6 foreground colors bits 2..1
 19f9 bits 0..2 ->background color
 19f9 bit3 foreground color bit 0

19f8, 19f9 readback alway 0xff
1900 readback 0x5y
17bf readback 0x54
1b00 holds value for a moment, gets 0x50

sprite y position
 basically the same as crtc vertical psoition register, but 1 line later

sprite x position
 not 44 but 43

*/


static UINT8 arcadia_rectangle[0x40][8];
static UINT8 chars[0x40][8]={
    // read from the screen generated from a palladium
    { 0,0,0,0,0,0,0,0 },			// 00 (space)
    { 1,2,4,8,16,32,64,128 }, //           ; 01 (\)
    { 128,64,32,16,8,4,2,1 }, //           ; 02 (/)
    { 255,255,255,255,255,255,255,255 }, //; 03 (solid block)
    { 0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00 },//  04 (?)
    { 3,3,3,3,3,3,3,3 }, //        ; 05 (half square right on)
    { 0,0,0,0,0,0,255,255 }, //              ; 06 (horz lower line)
    { 0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0 },// 07 (half square left on)
    { 0xff,0xff,3,3,3,3,3,3 },			// 08 (?)
    { 0xff,0xff,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0 },// 09 (?)
    { 192,192,192,192,192,192,255,255 }, //; 0A (!_)
    { 3,3,3,3,3,3,255,255 }, //            ; 0B (_!)
    { 1,3,7,15,31,63,127,255 }, //         ; 0C (diagonal block)
    { 128,192,224,240,248,252,254,255 }, //; 0D (diagonal block)
    { 255,254,252,248,240,224,192,128 }, //; 0E (diagonal block)
    { 255,127,63,31,15,7,3,1 }, //         ; 0F (diagonal block)
    { 0x00,0x1c,0x22,0x26,0x2a,0x32,0x22,0x1c },// 10 0
    { 0x00,0x08,0x18,0x08,0x08,0x08,0x08,0x1c },// 10 1
    { 0x00,0x1c,0x22,0x02,0x0c,0x10,0x20,0x3e },// 10 2
    { 0x00,0x3e,0x02,0x04,0x0c,0x02,0x22,0x1c },// 10 3
    { 0x00,0x04,0x0c,0x14,0x24,0x3e,0x04,0x04 },// 10 4
    { 0x00,0x3e,0x20,0x3c,0x02,0x02,0x22,0x1c },// 10 5
    { 0x00,0x0c,0x10,0x20,0x3c,0x22,0x22,0x1c },// 10 6
    { 0x00,0x7c,0x02,0x04,0x08,0x10,0x10,0x10 },// 10 7
    { 0x00,0x1c,0x22,0x22,0x1c,0x22,0x22,0x1c },// 10 8
    { 0x00,0x1c,0x22,0x22,0x3e,0x02,0x04,0x18 },// 10 9
    { 0x00,0x08,0x14,0x22,0x22,0x3e,0x22,0x22 },// 10 A
    { 0x00,0x3c,0x22,0x22,0x3c,0x22,0x22,0x3c },// 10 B
    { 0x00,0x1c,0x22,0x20,0x20,0x20,0x22,0x1c },// 10 C
    { 0x00,0x3c,0x22,0x22,0x22,0x22,0x22,0x3c },// 10 D
    { 0x00,0x3e,0x20,0x20,0x3c,0x20,0x20,0x3e },// 10 E
    { 0x00,0x3e,0x20,0x20,0x3c,0x20,0x20,0x20 },// 10 F
    { 0x00,0x1e,0x20,0x20,0x20,0x26,0x22,0x1e },// 10 G
    { 0x00,0x22,0x22,0x22,0x3e,0x22,0x22,0x22 },// 10 H
    { 0x00,0x1c,0x08,0x08,0x08,0x08,0x08,0x1c },// 10 I
    { 0x00,0x02,0x02,0x02,0x02,0x02,0x22,0x1c },// 10 J
    { 0x00,0x22,0x24,0x28,0x30,0x28,0x24,0x22 },// 10 K
    { 0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x3e },// 10 L
    { 0x00,0x22,0x36,0x2a,0x2a,0x22,0x22,0x22 },// 10 M
    { 0x00,0x22,0x22,0x32,0x2a,0x26,0x22,0x22 },// 10 N
    { 0x00,0x1c,0x22,0x22,0x22,0x22,0x22,0x1c },// 10 O
    { 0x00,0x3c,0x22,0x22,0x3c,0x20,0x20,0x20 },// 10 P
    { 0x00,0x1c,0x22,0x22,0x22,0x2a,0x24,0x1a },// 10 Q
    { 0x00,0x3c,0x22,0x22,0x3c,0x28,0x24,0x22 },// 10 R
    { 0x00,0x1c,0x22,0x20,0x1c,0x02,0x22,0x1c },// 10 S
    { 0x00,0x3e,0x08,0x08,0x08,0x08,0x08,0x08 },// 10 T
    { 0x00,0x22,0x22,0x22,0x22,0x22,0x22,0x1c },// 10 U
    { 0x00,0x22,0x22,0x22,0x22,0x22,0x14,0x08 },// 10 V
    { 0x00,0x22,0x22,0x22,0x2a,0x2a,0x36,0x22 },// 10 W
    { 0x00,0x22,0x22,0x14,0x08,0x14,0x22,0x22 },// 10 X
    { 0x00,0x22,0x22,0x14,0x08,0x08,0x08,0x08 },// 10 Y
    { 0x00,0x3e,0x02,0x04,0x08,0x10,0x20,0x3e },// 10 Z
    { 0,0,0,0,0,0,0,8 },			// 34 .
    { 0,0,0,0,0,8,8,0x10 },			// 35 ,
    { 0,0,8,8,0x3e,8,8,0 },			// 36 +
    { 0,8,0x1e,0x28,0x1c,0xa,0x3c,8 },		// 37 $
    // 8x user defined
};

static struct {
    int line;
    int charline;
    int shift;
    int ad_delay;
    int ad_select;
    int ypos;
    bool graphics;
    bool doublescan;
    bool lines26;
    bool multicolor;
    struct { int x, y; } pos[4];
    UINT8 bg[262][16+2*XPOS/8];
    int breaker;
    union {
	UINT8 data[0x400];
	struct {
	    // 0x1800
	    UINT8 chars1[13][16];
	    UINT8 ram1[2][16];
	    struct {
		UINT8 y,x;
	    } pos[4];
	    UINT8 ram2[4];
	    UINT8 vpos;
	    UINT8 sound1, sound2;
	    UINT8 char_line;
	    // 0x1900
	    UINT8 pad1a, pad1b, pad1c, pad1d;
	    UINT8 pad2a, pad2b, pad2c, pad2d;
	    UINT8 keys, unmapped3[0x80-9];
	    UINT8 chars[8][8];
	    UINT8 unknown[0x38];
	    UINT8 pal[4];
	    UINT8 collision_bg,
		collision_sprite;
	    UINT8 ad[2];
	    // 0x1a00
	    UINT8 chars2[13][16];
	    UINT8 ram3[3][16];
	} d;
    } reg;
	mame_bitmap *bitmap;
} arcadia_video={ 0 };

VIDEO_START( arcadia )
{
	int i;
	for (i=0; i<0x40; i++)
	{
		arcadia_rectangle[i][0]=0;
		arcadia_rectangle[i][4]=0;
		if (i&1) arcadia_rectangle[i][0]|=3;
		if (i&2) arcadia_rectangle[i][0]|=0x1c;
		if (i&4) arcadia_rectangle[i][0]|=0xe0;
		if (i&8) arcadia_rectangle[i][4]|=3;
		if (i&0x10) arcadia_rectangle[i][4]|=0x1c;
		if (i&0x20) arcadia_rectangle[i][4]|=0xe0;
		arcadia_rectangle[i][1]=arcadia_rectangle[i][2]=arcadia_rectangle[i][3]=arcadia_rectangle[i][0];
		arcadia_rectangle[i][5]=arcadia_rectangle[i][6]=arcadia_rectangle[i][7]=arcadia_rectangle[i][4];
	}

	arcadia_video.bitmap = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, BITMAP_FORMAT_INDEXED16);
	return 0;
}

READ8_HANDLER(arcadia_video_r)
{
    UINT8 data=0;
    switch (offset) {
    case 0xff: data=arcadia_video.charline|0xf0;break;
    case 0x100: data=input_port_1_r(0);break;
    case 0x101: data=input_port_2_r(0);break;
    case 0x102: data=input_port_3_r(0);break;
    case 0x103: data=input_port_4_r(0);break;
    case 0x104: data=input_port_5_r(0);break;
    case 0x105: data=input_port_6_r(0);break;
    case 0x106: data=input_port_7_r(0);break;
    case 0x107: data=input_port_8_r(0);break;
    case 0x108: data=input_port_0_r(0);break;
#if 0
    case 0x1fe:
	if (arcadia_video.ad_select) data=input_port_10_r(0)<<3;
	else data=input_port_9_r(0)<<3;
	break;
    case 0x1ff:
	if (arcadia_video.ad_select) data=input_port_7_r(0)<<3;
	else data=input_port_8_r(0)<<3;
	break;
#else
    case 0x1fe:
	data = 0x80;
	if (arcadia_video.ad_select) {
	    if (input_port_9_r(0)&0x10) data=0;
	    if (input_port_9_r(0)&0x20) data=0xff;
	} else {
	    if (input_port_9_r(0)&0x40) data=0xff;
	    if (input_port_9_r(0)&0x80) data=0;
	}
	break;
    case 0x1ff:
	data = 0x6f; // 0x7f too big for alien invaders (movs right)
	if (arcadia_video.ad_select) {
	    if (input_port_9_r(0)&0x1) data=0;
	    if (input_port_9_r(0)&0x2) data=0xff;
	} else {
	    if (input_port_9_r(0)&0x4) data=0xff;
	    if (input_port_9_r(0)&0x8) data=0;
	}
	break;
#endif
    default:
	data=arcadia_video.reg.data[offset];
    }
    return data;
}

WRITE8_HANDLER(arcadia_video_w)
{
    switch (offset) {
    case 0xfc:
	arcadia_video.reg.data[offset]=data;
	arcadia_video.ypos=255-data+YPOS;
	break;
    case 0xfd:
	arcadia_video.reg.data[offset]=data;
	arcadia_soundport_w(offset&3, data);
	arcadia_video.multicolor=data&0x80;
	break;
    case 0xfe:
	arcadia_video.reg.data[offset]=data;
	arcadia_soundport_w(offset&3, data);
	arcadia_video.shift=(data>>5);
	break;
    case 0xf0: case 0xf2: case 0xf4: case 0xf6:
	arcadia_video.reg.data[offset]=data;
	arcadia_video.pos[(offset>>1)&3].y=(data^0xff)+1;
	break;
    case 0xf1: case 0xf3: case 0xf5: case 0xf7:
	arcadia_video.reg.data[offset]=data;
	arcadia_video.pos[(offset>>1)&3].x=data-43;
	break;
    case 0x180: case 0x181: case 0x182: case 0x183: case 0x184: case 0x185: case 0x186: case 0x187:
    case 0x188: case 0x189: case 0x18a: case 0x18b: case 0x18c: case 0x18d: case 0x18e: case 0x18f:
    case 0x190: case 0x191: case 0x192: case 0x193: case 0x194: case 0x195: case 0x196: case 0x197:
    case 0x198: case 0x199: case 0x19a: case 0x19b: case 0x19c: case 0x19d: case 0x19e: case 0x19f:
    case 0x1a0: case 0x1a1: case 0x1a2: case 0x1a3: case 0x1a4: case 0x1a5: case 0x1a6: case 0x1a7:
    case 0x1a8: case 0x1a9: case 0x1aa: case 0x1ab: case 0x1ac: case 0x1ad: case 0x1ae: case 0x1af:
    case 0x1b0: case 0x1b1: case 0x1b2: case 0x1b3: case 0x1b4: case 0x1b5: case 0x1b6: case 0x1b7:
    case 0x1b8: case 0x1b9: case 0x1ba: case 0x1bb: case 0x1bc: case 0x1bd: case 0x1be: case 0x1bf:
	arcadia_video.reg.data[offset]=data;
	chars[0x38|((offset>>3)&7)][offset&7]=data;
	break;
    case 0x1f8:
	arcadia_video.reg.data[offset]=data;
	arcadia_video.lines26=data&0x40;
	arcadia_video.graphics=data&0x80;
	break;
    case 0x1f9:
	arcadia_video.reg.data[offset]=data;
	arcadia_video.doublescan=!(data&0x80);
	arcadia_video.ad_delay=10;
	break;
    default:
	arcadia_video.reg.data[offset]=data;
    }
}

INLINE void arcadia_draw_char(mame_bitmap *bitmap, UINT8 *ch, int color,
			      int y, int x)
{
    int k,b;
    if (arcadia_video.multicolor) {
	int c;
	if (color&0x40) c=arcadia_video.reg.d.pal[1];
	else c=arcadia_video.reg.d.pal[0];
	Machine->gfx[0]->colortable[1]=Machine->pens[(c>>3)&7];

	if (color&0x80) c=arcadia_video.reg.d.pal[1];
	else c=arcadia_video.reg.d.pal[0];
	Machine->gfx[0]->colortable[0]=Machine->pens[c&7];

    } else {
	Machine->gfx[0]->colortable[1]=
	    Machine->pens[((arcadia_video.reg.d.pal[1]>>3)&1)|((color>>5)&6)];
    }

    if (arcadia_video.doublescan) {
	for (k=0; (k<8)&&(y<bitmap->height); k++, y+=2) {
	    b=ch[k];
	    arcadia_video.bg[y][x>>3]|=b>>(x&7);
	    arcadia_video.bg[y][(x>>3)+1]|=b<<(8-(x&7));

            if (y+1<bitmap->height) {
                arcadia_video.bg[y+1][x>>3]|=b>>(x&7);
                arcadia_video.bg[y+1][(x>>3)+1]|=b<<(8-(x&7));
                drawgfx(bitmap, Machine->gfx[0], b,0,
                        0,0,x,y,
                        0, TRANSPARENCY_NONE,0);
                drawgfx(bitmap, Machine->gfx[0], b,0,
                        0,0,x,y+1,
                        0, TRANSPARENCY_NONE,0);
            }
	}
    } else {
	for (k=0; (k<8)&&(y<bitmap->height); k++, y++) {
	    b=ch[k];
            arcadia_video.bg[y][x>>3]|=b>>(x&7);
	    arcadia_video.bg[y][(x>>3)+1]|=b<<(8-(x&7));

	    drawgfx(bitmap, Machine->gfx[0], b,0,
		    0,0,x,y,
		    0, TRANSPARENCY_NONE,0);
	}
    }
}

INLINE void arcadia_vh_draw_line(mame_bitmap *bitmap,
				 int y, UINT8 chars1[16])
{
    int x, ch, j, h;
    bool graphics=arcadia_video.graphics;
    h=arcadia_video.doublescan?16:8;

    if (bitmap->height-arcadia_video.line<h) h=bitmap->height-arcadia_video.line;
    plot_box(bitmap, 0, y, bitmap->width, h, Machine->gfx[0]->colortable[0]);
    memset(arcadia_video.bg[y], 0, sizeof(arcadia_video.bg[0])*h);
    for (x=XPOS+arcadia_video.shift, j=0; j<16;j++,x+=8) {
	ch=chars1[j];
	// hangman switches with 0x40
	// alien invaders shield lines start with 0xc0
	if ((ch&0x3f)==0) {
	    switch (ch) {
		case 0xc0: graphics=TRUE;break;
		case 0x40: graphics=FALSE;break;
//		case 0x80:
//			alien invaders shields are empty 0x80
//		    popmessage(5, "graphics code 0x80 used");
	    }
	}
	if (graphics)
	    arcadia_draw_char(bitmap, arcadia_rectangle[ch&0x3f], ch, y, x);
	else
	    arcadia_draw_char(bitmap, chars[ch&0x3f], ch, y, x);
    }
}

static int arcadia_sprite_collision(int n1, int n2)
{
    int k, b1, b2, x;
    if (arcadia_video.pos[n1].x+8<=arcadia_video.pos[n2].x) return FALSE;
    if (arcadia_video.pos[n1].x>=arcadia_video.pos[n2].x+8) return FALSE;
    for (k=0; k<8; k++) {
	if (arcadia_video.pos[n1].y+k<arcadia_video.pos[n2].y) continue;
	if (arcadia_video.pos[n1].y+k>=arcadia_video.pos[n2].y+8) break;
	x=arcadia_video.pos[n1].x-arcadia_video.pos[n2].x;
	b1=arcadia_video.reg.d.chars[n1][k];
	b2=arcadia_video.reg.d.chars[n2][arcadia_video.pos[n1].y+k-arcadia_video.pos[n2].y];
	if (x<0) b2>>=-x;
	if (x>0) b1>>=x;
	if (b1&b2) return TRUE;
    }
    return FALSE;
}

static void arcadia_draw_sprites(mame_bitmap *bitmap)
{
    int i, k, x, y;
    UINT8 b;

    arcadia_video.reg.d.collision_bg|=0xf;
    arcadia_video.reg.d.collision_sprite|=0x3f;
    for (i=0; i<4; i++) {
	bool doublescan = FALSE;
	if (arcadia_video.pos[i].y<=-YPOS) continue;
	if (arcadia_video.pos[i].y>=bitmap->height-YPOS-8) continue;
	if (arcadia_video.pos[i].x<=-XPOS) continue;
	if (arcadia_video.pos[i].x>=128+XPOS-8) continue;

	switch (i) {
	case 0:
	    Machine->gfx[0]->colortable[1]=Machine->pens[(arcadia_video.reg.d.pal[3]>>3)&7];
	    doublescan=arcadia_video.reg.d.pal[3]&0x80?FALSE:TRUE;
	    break;
	case 1:
	    Machine->gfx[0]->colortable[1]=Machine->pens[arcadia_video.reg.d.pal[3]&7];
	    doublescan=arcadia_video.reg.d.pal[3]&0x40?FALSE:TRUE;
	    break;
	case 2:
	    Machine->gfx[0]->colortable[1]=Machine->pens[(arcadia_video.reg.d.pal[2]>>3)&7];
	    doublescan=arcadia_video.reg.d.pal[2]&0x80?FALSE:TRUE;
	    break;
	case 3:
	    Machine->gfx[0]->colortable[1]=Machine->pens[arcadia_video.reg.d.pal[2]&7];
	    doublescan=arcadia_video.reg.d.pal[2]&0x40?FALSE:TRUE;
	    break;
	}
	for (k=0; k<8; k++) {
	    b=arcadia_video.reg.d.chars[i][k];
	    x=arcadia_video.pos[i].x+XPOS;
	    if (!doublescan) {
		y=arcadia_video.pos[i].y+YPOS+k;
		drawgfx(bitmap, Machine->gfx[0], b,0,
			0,0,x,y,
			0, TRANSPARENCY_PEN,0);
	    } else {
		y=arcadia_video.pos[i].y+YPOS+k*2;
		drawgfx(bitmap, Machine->gfx[0], b,0,
			0,0,x,y,
			0, TRANSPARENCY_PEN,0);
		drawgfx(bitmap, Machine->gfx[0], b,0,
			0,0,x,y+1,
			0, TRANSPARENCY_PEN,0);
	    }
	    if (arcadia_video.reg.d.collision_bg&(1<<i)) {
		if ( (b<<(8-(x&7))) & ((arcadia_video.bg[y][x>>3]<<8)
				       |arcadia_video.bg[y][(x>>3)+1]) )
		    arcadia_video.reg.d.collision_bg&=~(1<<i);
	    }
	}
    }
    if (arcadia_sprite_collision(0,1)) arcadia_video.reg.d.collision_sprite&=~1;
    if (arcadia_sprite_collision(0,2)) arcadia_video.reg.d.collision_sprite&=~2;
    if (arcadia_sprite_collision(0,3)) arcadia_video.reg.d.collision_sprite&=~4;
    if (arcadia_sprite_collision(1,2)) arcadia_video.reg.d.collision_sprite&=~8;
    if (arcadia_sprite_collision(1,3)) arcadia_video.reg.d.collision_sprite&=~0x10; //guess
    if (arcadia_sprite_collision(2,3)) arcadia_video.reg.d.collision_sprite&=~0x20; //guess
}

INTERRUPT_GEN( arcadia_video_line )
{
	if (arcadia_video.ad_delay<=0)
	arcadia_video.ad_select=arcadia_video.reg.d.pal[1]&0x40;
	else arcadia_video.ad_delay--;

	arcadia_video.line++;
	arcadia_video.line%=262;
	// unbelievable, reflects only charline, but alien invaders uses it for
	// alien scrolling

	Machine->gfx[0]->colortable[0]=Machine->pens[arcadia_video.reg.d.pal[1]&7];

	if (arcadia_video.line<arcadia_video.ypos)
	{
		plot_box(arcadia_video.bitmap, 0, arcadia_video.line, Machine->screen[0].width, 1, Machine->gfx[0]->colortable[0]);
		memset(arcadia_video.bg[arcadia_video.line], 0, sizeof(arcadia_video.bg[0]));
	}
	else
	{
		int h=arcadia_video.doublescan?16:8;

		arcadia_video.charline=(arcadia_video.line-arcadia_video.ypos)/h;

		if (arcadia_video.charline<13)
		{
			if (((arcadia_video.line-arcadia_video.ypos)&(h-1))==0) {
				arcadia_vh_draw_line(arcadia_video.bitmap, arcadia_video.charline*h+arcadia_video.ypos,
					arcadia_video.reg.d.chars1[arcadia_video.charline]);
			}
		}
		else if (arcadia_video.lines26 && (arcadia_video.charline<26))
		{
			if (((arcadia_video.line-arcadia_video.ypos)&(h-1))==0)
			{
				arcadia_vh_draw_line(arcadia_video.bitmap, arcadia_video.charline*h+arcadia_video.ypos,
					arcadia_video.reg.d.chars2[arcadia_video.charline-13]);
			}
			arcadia_video.charline-=13;
		}
		else
		{
			arcadia_video.charline=0xd;
			plot_box(arcadia_video.bitmap, 0, arcadia_video.line, Machine->screen[0].width, 1, Machine->gfx[0]->colortable[0]);
			memset(arcadia_video.bg[arcadia_video.line], 0, sizeof(arcadia_video.bg[0]));
		}
	}
	if (arcadia_video.line==261)
		arcadia_draw_sprites(arcadia_video.bitmap);
}

READ8_HANDLER(arcadia_vsync_r)
{
    return arcadia_video.line>=216?0x80:0;
}

VIDEO_UPDATE( arcadia )
{
	copybitmap(bitmap, arcadia_video.bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
	return 0;
}
