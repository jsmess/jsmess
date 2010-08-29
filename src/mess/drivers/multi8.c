/***************************************************************************

	Multi 8 (c) 1983 Mitsubishi

	preliminary driver by Angelo Salese

	TODO:
	- dunno how to trigger the text color mode in BASIC, I just modify
	  $f0b1 to 1 for now
	- bitmap B/W mode is untested

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/2203intf.h"
#include "video/mc6845.h"
#include "machine/i8255a.h"
#include "sound/beep.h"

static UINT8 mcu_init;
static UINT8 keyb_press,keyb_press_flag,shift_press_flag,display_reg;
static UINT16 cursor_addr,cursor_raster;
static UINT8 vram_bank;
static UINT8 pen_clut[8],bw_mode;

static VIDEO_START( multi8 )
{
	keyb_press = keyb_press_flag = shift_press_flag = display_reg = 0;
	cursor_addr = cursor_raster = 0;
	for (bw_mode = 0; bw_mode < 8; bw_mode++) pen_clut[bw_mode]=0;
	vram_bank = 8;
	bw_mode = 0;	
}

static VIDEO_UPDATE( multi8 )
{
	int x,y,count;
	int x_width;
	int xi,yi;
	UINT8 *vram = memory_region(screen->machine, "vram");
	UINT8 *gfx_rom = memory_region(screen->machine, "chargen");

	count = 0x0000;

	x_width = (display_reg & 0x40) ? 640 : 320;

	for(y=0;y<200;y++)
	{
		for(x=0;x<x_width;x+=8)
		{
			for(xi=0;xi<8;xi++)
			{
				int pen_r,pen_g,pen_b,color;

				pen_b = (vram[count | 0x0000] >> (7-xi)) & 1;
				pen_r = (vram[count | 0x4000] >> (7-xi)) & 1;
				pen_g = (vram[count | 0x8000] >> (7-xi)) & 1;

				if(bw_mode)
				{
					pen_b = (display_reg & 1) ? pen_b : 0;
					pen_r = (display_reg & 2) ? pen_r : 0;
					pen_g = (display_reg & 4) ? pen_g : 0;

					color = ((pen_b) | (pen_r) | (pen_g)) ? 7 : 0;
				}
				else
					color = (pen_b) | (pen_r << 1) | (pen_g << 2);

				*BITMAP_ADDR16(bitmap, y, x+xi) = screen->machine->pens[pen_clut[color]];
			}
			count++;
		}
	}

	x_width = (display_reg & 0x40) ? 80 : 40;
	count = 0xc000;

	for(y=0;y<25;y++)
	{
		for(x=0;x<x_width;x++)
		{
			int tile = vram[count];
			int attr = vram[count+0x800];
			int color = (display_reg & 0x80) ? 7 : (attr & 0x07);

			for(yi=0;yi<8;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					int pen;

					if(attr & 0x20)
						pen = (gfx_rom[tile*8+yi] >> (7-xi) & 1) ? 0 : color;
					else
						pen = (gfx_rom[tile*8+yi] >> (7-xi) & 1) ? color : 0;

					if(pen)
						*BITMAP_ADDR16(bitmap, y*8+yi, x*8+xi) = screen->machine->pens[pen];
				}
			}

			//drawgfx_opaque(bitmap, cliprect, screen->machine->gfx[0], tile,color >> 5, 0, 0, x*8, y*8);

			// draw cursor
			if(cursor_addr+0xc000 == count)
			{
				int xc,yc,cursor_on;

				cursor_on = 0;
				switch(cursor_raster & 0x60)
				{
					case 0x00: cursor_on = 1; break; //always on
					case 0x20: cursor_on = 0; break; //always off
					case 0x40: if(screen->machine->primary_screen->frame_number() & 0x10) { cursor_on = 1; } break; //fast blink
					case 0x60: if(screen->machine->primary_screen->frame_number() & 0x20) { cursor_on = 1; } break; //slow blink
				}

				if(cursor_on)
				{
					for(yc=0;yc<(8-(cursor_raster & 7));yc++)
					{
						for(xc=0;xc<8;xc++)
						{
							*BITMAP_ADDR16(bitmap, y*8+yc, x*8+xc) = screen->machine->pens[0x7];
						}
					}
				}
			}

			(display_reg & 0x40) ? count++ : count+=2;
		}
	}
    return 0;
}

static WRITE8_HANDLER( multi8_6845_w )
{
	static int addr_latch;

	if(offset == 0)
	{
		addr_latch = data;
		//mc6845_address_w(space->machine->device("crtc"), 0,data);
	}
	else
	{
		if(addr_latch == 0x0a)
			cursor_raster = data;
		if(addr_latch == 0x0e)
			cursor_addr = ((data<<8) & 0x3f00) | (cursor_addr & 0xff);
		else if(addr_latch == 0x0f)
			cursor_addr = (cursor_addr & 0x3f00) | (data & 0xff);

		//mc6845_register_w(space->machine->device("crtc"), 0,data);
	}
}

static READ8_HANDLER( key_input_r )
{
	if(mcu_init == 0){ mcu_init++;	return 3; }

	keyb_press_flag &= 0xfe;

	return keyb_press;
}

static READ8_HANDLER( key_status_r )
{
	if(mcu_init == 0){ 				return 1; }
	if(mcu_init == 1){ mcu_init++;	return 1; }
	if(mcu_init == 2){ mcu_init++;	return 0; }

	return keyb_press_flag | (shift_press_flag << 7);
}

static READ8_HANDLER( multi8_vram_r )
{
	UINT8 *vram = memory_region(space->machine, "vram");
	UINT8 *wram = memory_region(space->machine, "wram");
	UINT8 res;

	if(!(vram_bank & 0x10)) //select plain work ram
		return wram[offset];

	res = 0xff;
	if(!(vram_bank & 1)) { res &= vram[offset | 0x0000]; }
	if(!(vram_bank & 2)) { res &= vram[offset | 0x4000]; }
	if(!(vram_bank & 4)) { res &= vram[offset | 0x8000]; }
	if(!(vram_bank & 8)) { res &= vram[offset | 0xc000]; }

	return res;
}

static WRITE8_HANDLER( multi8_vram_w )
{
	UINT8 *vram = memory_region(space->machine, "vram");
	UINT8 *wram = memory_region(space->machine, "wram");

	if(!(vram_bank & 0x10)) //select plain work ram
	{
		wram[offset] = data;
		return;
	}

	if(!(vram_bank & 1)) { vram[offset | 0x0000] = data; }
	if(!(vram_bank & 2)) { vram[offset | 0x4000] = data; }
	if(!(vram_bank & 4)) { vram[offset | 0x8000] = data; }
	if(!(vram_bank & 8)) { vram[offset | 0xc000] = data; }
}

static READ8_HANDLER( pal_r )
{
	return pen_clut[offset];
}

static WRITE8_HANDLER( pal_w )
{
	pen_clut[offset] = data;

	{
		int i;
		for(i=0;i<8;i++)
		{
			if(pen_clut[i]) { bw_mode = 0; return; }
		}
		bw_mode = 1;
	}
}

static ADDRESS_MAP_START(multi8_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE( multi8_vram_r, multi8_vram_w )
	AM_RANGE(0xc000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( multi8_io , ADDRESS_SPACE_IO, 8)
//	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READ(key_input_r) AM_WRITENOP//keyboard
	AM_RANGE(0x01, 0x01) AM_READ(key_status_r) AM_WRITENOP//keyboard
	AM_RANGE(0x18, 0x19) AM_DEVWRITE("ymsnd", ym2203_w)
//	AM_RANGE(0x18, 0x18) //opn read 0
	AM_RANGE(0x1a, 0x1a) AM_DEVREAD("ymsnd", ym2203_r)
	AM_RANGE(0x1c, 0x1d) AM_WRITE(multi8_6845_w)
//	AM_RANGE(0x20, 0x21) //sio, cmt
//	AM_RANGE(0x24, 0x27) //pit
	AM_RANGE(0x28, 0x2b) AM_DEVREADWRITE("ppi8255_0", i8255a_r, i8255a_w)
//	AM_RANGE(0x2c, 0x2d) //i8259
	AM_RANGE(0x30, 0x37) AM_READWRITE(pal_r,pal_w)
//	AM_RANGE(0x40, 0x41) //kanji regs
//	AM_RANGE(0x70, 0x74) //upd765a fdc
//	AM_RANGE(0x78, 0x78) //memory banking
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( multi8 )
	PORT_START("VBLANK")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK)

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
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x15 dc4
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x17 syn
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x18 etb
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
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
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_UNUSED)
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
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("<") PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('<')
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

	PORT_START("key_modifiers")
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("KANA") PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CAPS") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("GRPH") PORT_CODE(KEYCODE_LALT)
INPUT_PORTS_END

static TIMER_CALLBACK( keyboard_callback )
{
	const char* portnames[3] = { "key1","key2","key3" };
	int i,port_i,scancode;
	UINT8 keymod = input_port_read(machine,"key_modifiers") & 0x1f;
	scancode = 0;

	shift_press_flag = ((keymod & 0x02) >> 1);

	for(port_i=0;port_i<3;port_i++)
	{
		for(i=0;i<32;i++)
		{
			if((input_port_read(machine,portnames[port_i])>>i) & 1)
			{
				//key_flag = 1;
				if(!shift_press_flag)  // shift not pressed
				{
					if(scancode >= 0x41 && scancode < 0x5b)
						scancode += 0x20;  // lowercase
				}
				else
				{
					if(scancode >= 0x31 && scancode < 0x3a)
						scancode -= 0x10;
					if(scancode == 0x30)
						scancode = 0x3d;

					if(scancode == 0x3b)
						scancode = 0x2c;

					if(scancode == 0x3a)
						scancode = 0x2e;
					if(scancode == 0x5b)
						scancode = 0x2b;
					if(scancode == 0x3c)
						scancode = 0x3e;
				}
				keyb_press = scancode;
				keyb_press_flag = 1;
				return;
			}
			scancode++;
		}
	}
}

/* F4 Character Displayer */
static const gfx_layout multi8_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( multi8 )
	GFXDECODE_ENTRY( "chargen", 0x0000, multi8_charlayout, 0, 4 )
GFXDECODE_END

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
	DEVCB_NULL,	/* VSYNC callback */
	NULL		/* update address callback */
};

static PALETTE_INIT( multi8 )
{
	int i;

	for(i=0;i<8;i++)
		palette_set_color_rgb(machine, i, pal1bit(i >> 1),pal1bit(i >> 2),pal1bit(i >> 0));
}

static READ8_DEVICE_HANDLER( porta_r )
{
	int vsync = (input_port_read(device->machine, "VBLANK") & 0x1) << 5;

	return ~0x60 | vsync;
}


static WRITE8_DEVICE_HANDLER( portb_w )
{
	/*
		x--- ---- color mode
		-x-- ---- screen width (80 / 40)
		---- x--- memory bank status
		---- -xxx page screen graphics in B/W mode
	*/
//	address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

//	printf("Port B w = %02x %04x\n",data,cpu_get_pc(space->cpu));

	{
		if((display_reg & 0x40) != (data & 0x40))
		{
			rectangle visarea = device->machine->primary_screen->visible_area();
			int x_width;

			x_width = (data & 0x40) ? 640 : 320;

			visarea.min_x = visarea.min_y = 0;
			visarea.max_y = (200) - 1;
			visarea.max_x = (x_width) - 1;

			device->machine->primary_screen->configure(640, 200, visarea, device->machine->primary_screen->frame_period().attoseconds);
		}
	}

	display_reg = data;
}

static WRITE8_DEVICE_HANDLER( portc_w )
{
//	printf("Port C w = %02x\n",data);
	vram_bank = data & 0x1f;

	if(data & 0x20 && data != 0xff)
		printf("Work RAM bank selected!\n");
//		fatalerror("Work RAM bank selected");
}


static I8255A_INTERFACE( ppi8255_intf_0 )
{
	DEVCB_HANDLER(porta_r),			/* Port A read */
	DEVCB_NULL,						/* Port B read */
	DEVCB_NULL,						/* Port C read */
	DEVCB_NULL,						/* Port A write */
	DEVCB_HANDLER(portb_w),			/* Port B write */
	DEVCB_HANDLER(portc_w)			/* Port C write */
};

static WRITE8_DEVICE_HANDLER( ym2203_porta_w )
{
	beep_set_state(device->machine->device("beeper"),!(data & 0x08));
}

static const ym2203_interface ym2203_config =
{
	{
		AY8910_LEGACY_OUTPUT,
		AY8910_DEFAULT_LOADS,
		DEVCB_NULL, DEVCB_NULL, DEVCB_HANDLER( ym2203_porta_w ), DEVCB_NULL
	},
	NULL
};


static MACHINE_START(multi8)
{
	timer_pulse(machine, ATTOTIME_IN_HZ(240/32), NULL, 0, keyboard_callback);
}

static MACHINE_RESET(multi8)
{
	beep_set_frequency(machine->device("beeper"),1200); //guesswork
	beep_set_state(machine->device("beeper"),0);
	mcu_init = 0;
}

static MACHINE_DRIVER_START( multi8 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(multi8_mem)
    MDRV_CPU_IO_MAP(multi8_io)

    MDRV_MACHINE_START(multi8)
    MDRV_MACHINE_RESET(multi8)

	MDRV_I8255A_ADD( "ppi8255_0", ppi8255_intf_0 )

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 200)
    MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 200-1)
    MDRV_PALETTE_LENGTH(8)
    MDRV_PALETTE_INIT(multi8)
	MDRV_GFXDECODE(multi8)

	MDRV_MC6845_ADD("crtc", H46505, XTAL_3_579545MHz/2, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */

    MDRV_VIDEO_START(multi8)
    MDRV_VIDEO_UPDATE(multi8)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("ymsnd", YM2203, 1500000) //unknown clock / divider
	MDRV_SOUND_CONFIG(ym2203_config)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD("beeper", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.50)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( multi8 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basic.rom", 0x0000, 0x8000, CRC(2131a3a8) SHA1(0f5a565ecfbf79237badbf9011dcb374abe74a57))

	ROM_REGION( 0x0800, "chargen", 0 )
	ROM_LOAD( "font.rom",  0x0000, 0x0800, CRC(08f9ed0e) SHA1(57480510fb30af1372df5a44b23066ca61c6f0d9))

	ROM_REGION( 0x1000, "fdc_bios", 0 )
	ROM_LOAD( "disk.rom",  0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x10000, "vram", ROMREGION_ERASE00 )

	ROM_REGION( 0x4000, "wram", ROMREGION_ERASEFF )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1983, multi8,  0,       0, 	multi8, 	multi8, 	 0,   "Mitsubishi",   "Multi 8",		GAME_NOT_WORKING | GAME_NO_SOUND)

