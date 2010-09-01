/***************************************************************************

	MYCOMZ-80A (c) 1981 Japan Electronics College

	preliminary driver by Angelo Salese

	TODO:
	- SN doesn't seem to work properly, needs to find out what's the WE bit
	  for.
	- Hook-up FDC and CMT load / save;
	- Printer
	- RTC

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"
#include "machine/i8255a.h"
#include "sound/sn76496.h"

static UINT16 mycom_amask;
static UINT8 ram_bank;
static UINT16 vram_addr;
static UINT16 cursor_addr,cursor_raster;
static UINT8 keyb_press,keyb_press_flag,shift_press_flag;
static UINT8 video_mode,sn_we;

static VIDEO_START( mycom )
{
}

static VIDEO_UPDATE( mycom )
{
	int x,y,count;
	int xi,yi;
	int width;
	static UINT8 *vram = memory_region(screen->machine, "vram");
	static UINT8 *gfx_rom = memory_region(screen->machine, "gfx");

	count = 0;

	if(video_mode & 0x40)
	{
		width = (video_mode & 0x80) ? 160 : 320;

		for(y=0;y<24;y++)
		{
			for(x=0;x<width;x+=4)
			{
				//int xi,yi;
				int xi,yi;

				for(yi=0;yi<4;yi++)
				{
					for(xi=0;xi<2;xi++)
					{
						int pen;

						pen = vram[count] >> (yi+(xi*4));
						pen &= 1;

						*BITMAP_ADDR16(bitmap, y*8+(yi*2+0), (x+(xi*2+0))*2+0) = screen->machine->pens[pen];
						*BITMAP_ADDR16(bitmap, y*8+(yi*2+0), (x+(xi*2+1))*2+0) = screen->machine->pens[pen];
						*BITMAP_ADDR16(bitmap, y*8+(yi*2+1), (x+(xi*2+0))*2+0) = screen->machine->pens[pen];
						*BITMAP_ADDR16(bitmap, y*8+(yi*2+1), (x+(xi*2+1))*2+0) = screen->machine->pens[pen];
						*BITMAP_ADDR16(bitmap, y*8+(yi*2+0), (x+(xi*2+0))*2+1) = screen->machine->pens[pen];
						*BITMAP_ADDR16(bitmap, y*8+(yi*2+0), (x+(xi*2+1))*2+1) = screen->machine->pens[pen];
						*BITMAP_ADDR16(bitmap, y*8+(yi*2+1), (x+(xi*2+0))*2+1) = screen->machine->pens[pen];
						*BITMAP_ADDR16(bitmap, y*8+(yi*2+1), (x+(xi*2+1))*2+1) = screen->machine->pens[pen];
					}
				}

				count++;
			}
		}

		return 0;
	}
	else
	{
		width = (video_mode & 0x80) ? 40 : 80;

		for(y=0;y<24;y++)
		{
			for(x=0;x<width;x++)
			{
				int tile = vram[count];
				//int color = (display_reg & 0x80) ? 7 : (attr & 0x07);

					for(yi=0;yi<8;yi++)
					{
						for(xi=0;xi<8;xi++)
						{
							int pen;

							pen = (gfx_rom[tile*8+yi] >> (7-xi) & 1) ? 1 : 0;

							//if(pen)
								*BITMAP_ADDR16(bitmap, y*8+yi, x*8+xi) = screen->machine->pens[pen];
						}
					}

					if(cursor_addr == count)
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
									*BITMAP_ADDR16(bitmap, y*8+yc, x*8+xc) = screen->machine->pens[0x1];
								}
							}
						}
					}

				count++;
			}
		}
	}

    return 0;
}

static READ8_HANDLER( wram_r )
{
	static UINT8 *ram = memory_region(space->machine, "maincpu");

	if(offset & 0xc000 && ram_bank)
		return ram[(offset & 0x3fff) | 0x10000];

	return ram[offset | mycom_amask];
}

static WRITE8_HANDLER( wram_w )
{
	static UINT8 *ram = memory_region(space->machine, "maincpu");

	ram[offset | mycom_amask] = data;
}

static WRITE8_HANDLER( sys_control_w )
{
	switch(data)
	{
		case 0x00: mycom_amask = 0xc000; break;
		case 0x01: mycom_amask = 0x0000; break;
		case 0x02: ram_bank = 0; break;
		case 0x03: ram_bank = 1; break;
	}
}

static WRITE8_HANDLER( mycom_6845_w )
{
	static int addr_latch;

	if(offset == 0)
	{
		addr_latch = data;
		mc6845_address_w(space->machine->device("crtc"), 0,data);
	}
	else
	{
		if(addr_latch == 0x0a)
			cursor_raster = data;
		if(addr_latch == 0x0e)
			cursor_addr = ((data<<8) & 0x3f00) | (cursor_addr & 0xff);
		else if(addr_latch == 0x0f)
			cursor_addr = (cursor_addr & 0x3f00) | (data & 0xff);

		mc6845_register_w(space->machine->device("crtc"), 0,data);
	}
}

static READ8_HANDLER( vram_data_r )
{
	static UINT8 *vram = memory_region(space->machine,"vram");

	return vram[vram_addr];
}

static WRITE8_HANDLER( vram_data_w )
{
	static UINT8 *vram = memory_region(space->machine,"vram");

	vram[vram_addr] = data;
}

static ADDRESS_MAP_START(mycom_map, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(wram_r,wram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mycom_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_WRITE(sys_control_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(vram_data_r,vram_data_w)
	AM_RANGE(0x02, 0x03) AM_DEVREAD("crtc", mc6845_register_r) AM_WRITE(mycom_6845_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE("ppi8255_0", i8255a_r, i8255a_w)
	AM_RANGE(0x08, 0x0b) AM_DEVREADWRITE("ppi8255_1", i8255a_r, i8255a_w)
	AM_RANGE(0x0c, 0x0f) AM_DEVREADWRITE("ppi8255_2", i8255a_r, i8255a_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mycom )

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
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("*") PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR('*') //0x2a *
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

/* F4 Character Displayer */
static const gfx_layout mycom_charlayout =
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

static GFXDECODE_START( mycom )
	GFXDECODE_ENTRY( "gfx", 0x0000, mycom_charlayout, 0, 1 )
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

static WRITE8_DEVICE_HANDLER( vram_laddr_w )
{
	vram_addr = (vram_addr & 0x700) | (data & 0x0ff);

	/* doesn't work? */
	//printf("%02x %02x\n",data,sn_we);
	//if(sn_we)
	//	sn76496_w(device->machine->device("sn1"), 0, data);
}

static WRITE8_DEVICE_HANDLER( vram_haddr_w )
{
	vram_addr = (vram_addr & 0x0ff) | ((data & 0x007) << 8);
}

static READ8_DEVICE_HANDLER( input_1a )
{
	/*
	x--- ---- display flag
	---- --x- keyboard shift
	---- ---x keyboard strobe
	*/
	int res;

	res = keyb_press_flag;
	keyb_press_flag = 0;

	return 0x00 | (shift_press_flag << 1) | res;
}

static READ8_DEVICE_HANDLER( input_0c )
{
	/*
	xxxx ---- keyboard related, it expects to return 1101 here
	*/
	return (mame_rand(device->machine) & 0x20) | 0x00;
}

static READ8_DEVICE_HANDLER( key_r )
{
	//keyb_press_flag = 0;
	return keyb_press;
}

static WRITE8_DEVICE_HANDLER( output_1c )
{
	/*
	x--- ---- width 80/40 (0 = 80, 1 = 40)
	-x-- ---- video mode (0= tile, 1 = bitmap)
	--x- ---- PSG Chip Select bit
	---x ---- PSG Write Enable bit
	---- x--- cmt remote
	---- -x-- cmt output
	---- --x- printer reset
	---- ---x printer strobe
	*/

	video_mode = data & 0xc0;
	sn_we = data & 0x10;
//	printf("%02x\n",data & 0x10);
}

static I8255A_INTERFACE( ppi8255_intf_0 )
{
	DEVCB_NULL,						/* Port A read */
	DEVCB_HANDLER(key_r),			/* Port B read */
	DEVCB_HANDLER(input_0c),		/* Port C read */
	DEVCB_HANDLER(vram_laddr_w),	/* Port A write */
	DEVCB_NULL,						/* Port B write */
	DEVCB_HANDLER(vram_haddr_w)		/* Port C write */
};

static I8255A_INTERFACE( ppi8255_intf_1 )
{
	DEVCB_HANDLER(input_1a),			/* Port A read */
	DEVCB_NULL,			/* Port B read */
	DEVCB_NULL,			/* Port C read */
	DEVCB_NULL,			/* Port A write */
	DEVCB_NULL,			/* Port B write */
	DEVCB_HANDLER(output_1c)		/* Port C write */
};

static I8255A_INTERFACE( ppi8255_intf_2 )
{
	DEVCB_NULL,			/* Port A read */
	DEVCB_NULL,			/* Port B read */
	DEVCB_NULL,			/* Port C read */
	DEVCB_NULL,			/* Port A write */
	DEVCB_NULL,			/* Port B write */
	DEVCB_NULL			/* Port C write */
};

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
					//if(scancode >= 0x41 && scancode < 0x5b)
					//	scancode += 0x20;  // lowercase, FIXME: 'A' doesn't work properly?
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

static MACHINE_START(mycom)
{
	timer_pulse(machine, ATTOTIME_IN_HZ(240/32), NULL, 0, keyboard_callback);
}

static MACHINE_RESET(mycom)
{
	mycom_amask = 0xc000;
}

static MACHINE_CONFIG_START( mycom, driver_data_t )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(mycom_map)
    MDRV_CPU_IO_MAP(mycom_io)

    MDRV_MACHINE_START(mycom)
    MDRV_MACHINE_RESET(mycom)

	MDRV_I8255A_ADD( "ppi8255_0", ppi8255_intf_0 )
	MDRV_I8255A_ADD( "ppi8255_1", ppi8255_intf_1 )
	MDRV_I8255A_ADD( "ppi8255_2", ppi8255_intf_2 )

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 192-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)
	MDRV_GFXDECODE(mycom)

	MDRV_MC6845_ADD("crtc", H46505, XTAL_3_579545MHz/4, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */

    MDRV_VIDEO_START(mycom)
    MDRV_VIDEO_UPDATE(mycom)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("sn1", SN76489A, 1996800) // unknown clock / divider
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mycom )
    ROM_REGION( 0x14000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bios.rom", 0xc000, 0x3000, CRC(e6f50355) SHA1(5d3acea360c0a8ab547db03a43e1bae5125f9c2a))
	ROM_LOAD( "basic.rom",0xf000, 0x1000, CRC(3b077465) SHA1(777427182627f371542c5e0521ed3ca1466a90e1))

	ROM_REGION( 0x0800, "vram", ROMREGION_ERASE00 )

	ROM_REGION( 0x0800, "gfx", ROMREGION_ERASEFF )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, CRC(4039bb6f) SHA1(086ad303bf4bcf983fd6472577acbf744875fea8))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1981, mycom,  	0,       0, 		mycom, 	mycom, 	 0,  	   "Japan Electronics College",   "MYCOMZ-80A",		GAME_NOT_WORKING | GAME_NO_SOUND)

