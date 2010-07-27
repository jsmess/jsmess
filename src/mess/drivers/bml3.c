/***************************************************************************

        Hitachi Basic Master Level 3 (MB-6890)

        13/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "video/mc6845.h"

static UINT16 cursor_addr,cursor_raster;
static UINT8 attr_latch;

static VIDEO_START( bml3 )
{
}

static VIDEO_UPDATE( bml3 )
{
	int x,y,count;
	int xi,yi;
	static UINT8 *gfx_rom = memory_region(screen->machine, "char");
	static UINT8 *vram = memory_region(screen->machine, "vram");

	count = 0x0000;

	for(y=0;y<25;y++)
	{
		for(x=0;x<40;x++)
		{
			int tile = vram[count+0x0000];
			int color = vram[count+0x4000] & 7;
			//attr & 0x10 is used ... bitmap mode? (apparently bits 4 and 7 are used for that)

			for(yi=0;yi<8;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					int pen;

					pen = (gfx_rom[tile*8+yi] >> (7-xi) & 1) ? color : 0;

					*BITMAP_ADDR16(bitmap, y*8+yi, x*8+xi) = screen->machine->pens[pen];
				}
			}

			if(cursor_addr-0x400 == count)
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
							*BITMAP_ADDR16(bitmap, y*8+yc+7, x*8+xc) = screen->machine->pens[0x7];
						}
					}
				}
			}

			count++;
		}
	}

    return 0;
}

static WRITE8_HANDLER( bml3_6845_w )
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

/*
Keyboard scancode table (notice that we're using Sharp X1 char, so some bits might be different):
0x00: right or space
0x01: up
0x02: ?
0x03: left
0x04: down
0x05: right or space
0x06
0x07: (changes the items at the bottom, caps lock?)
0x08
0x09
0x0a
0x0b
0x0c
0x0d: 8
0x0e: 9
0x0f: *
0x10: 7
0x11: 4
0x12: 6
0x13: 8
0x14: 0
0x15: ^
0x16: -
0x17: 3
0x18: backspace
0x19: 5
0x1a: 1
0x1b: 2
0x1c: 9
0x1d: 7
0x1e: backspace
0x1f: Yen symbol?
0x20: U
0x21: R
0x22: Y
0x23: I
0x24: P
0x25: [
0x26: @
0x27: 0
0x28: Q
0x29: T
0x2a: W
0x2b: E
0x2c: O
0x2d: .
0x2e: HOME
0x2f: ENTER
0x30: J
0x31: F
0x32: H
0x33: K
0x34: ;
0x35: ]
0x36: :
0x37: 4
0x38: A
0x39: G
0x3a: S
0x3b: D
0x3c: L
0x3d: 5
0x3e: 6
0x3f: -
0x40: M
0x41: V
0x42: N
0x43: ,
0x44: /
0x45: /
0x46: _
0x47: 1
0x48: Z
0x49: B
0x4a: X
0x4b: C
0x4c: .
0x4d: 2
0x4e: 3
0x4f: +
0x50: PF1
0x51: PF2
0x52: PF3
0x53: PF4
0x54: PF5
0x55-0x7f: unused
*/

static READ8_HANDLER( bml3_keyboard_r )
{
	static int scancode;

	if(input_code_pressed_once(space->machine, KEYCODE_Z))
		scancode++;

	if(input_code_pressed_once(space->machine, KEYCODE_X))
		scancode--;

	popmessage("%02x",scancode);

	return scancode | 0x80;
}

/* Note: this custom code is there just for simplicity, it'll be nuked in the end */
static READ8_HANDLER( bml3_io_r )
{
	static UINT8 *rom = memory_region(space->machine, "maincpu");

	if(offset == 0xc8) //???
		return 0;

	if(offset == 0xc9)
		return 0x11; //put 320 x 200 mode

	if(offset == 0xe0) return bml3_keyboard_r(space,0);

//	if(offset == 0xcb || offset == 0xc4)

//	if(offset == 0x40 || offset == 0x42 || offset == 0x44 || offset == 0x46)


	if(offset < 0xf0)
	{
		logerror("I/O read [%02x] at PC=%04x\n",offset,cpu_get_pc(space->cpu));
		return 0;
	}

	/* TODO: pretty sure that there's a bankswitch for this */
	return rom[offset+0xff00];
}

static WRITE8_HANDLER( bml3_io_w )
{
	if(offset == 0xc6 || offset == 0xc7) { bml3_6845_w(space,offset-0xc6,data); }
	else if(offset == 0xd8)				 { attr_latch = data; }
	else
	{
		logerror("I/O write %02x -> [%02x] at PC=%04x\n",data,offset,cpu_get_pc(space->cpu));
	}
}

static READ8_HANDLER( bml3_vram_r )
{
	static UINT8 *vram = memory_region(space->machine, "vram");

	/* TODO: this presumably also triggers an attr latch read */

	return vram[offset];
}

static WRITE8_HANDLER( bml3_vram_w )
{
	static UINT8 *vram = memory_region(space->machine, "vram");

	vram[offset] = data;
	vram[offset+0x4000] = attr_latch;
}

static ADDRESS_MAP_START(bml3_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x43ff) AM_READWRITE(bml3_vram_r,bml3_vram_w)
	AM_RANGE(0x4400, 0x9fff) AM_RAM
	AM_RANGE(0xff00, 0xffff) AM_READWRITE(bml3_io_r,bml3_io_w)
	AM_RANGE(0xa000, 0xffff) AM_ROM
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( bml3 )
INPUT_PORTS_END


static MACHINE_RESET(bml3)
{
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
	DEVCB_NULL,	/* VSYNC callback */
	NULL		/* update address callback */
};

static INTERRUPT_GEN( bml3_irq )
{
	cputag_set_input_line(device->machine, "maincpu", M6809_IRQ_LINE, HOLD_LINE);
}

static PALETTE_INIT( bml3 )
{
	int i;

	for(i=0;i<8;i++)
		palette_set_color_rgb(machine, i, pal1bit(i >> 1),pal1bit(i >> 2),pal1bit(i >> 0));
}


static MACHINE_DRIVER_START( bml3 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6809, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(bml3_mem)
	MDRV_CPU_VBLANK_INT("screen", bml3_irq )

    MDRV_MACHINE_RESET(bml3)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(bml3)

	MDRV_MC6845_ADD("crtc", H46505, XTAL_3_579545MHz/4, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */

    MDRV_VIDEO_START(bml3)
    MDRV_VIDEO_UPDATE(bml3)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( bml3 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "l3bas.rom", 0xa000, 0x6000, CRC(d81baa07) SHA1(a8fd6b29d8c505b756dbf5354341c48f9ac1d24d))

    ROM_REGION( 0x800, "char", 0 )
	ROM_LOAD("char.rom", 0x00000, 0x00800, BAD_DUMP CRC(e3995a57) SHA1(1c1a0d8c9f4c446ccd7470516b215ddca5052fb2) ) //Taken from Sharp X1
	ROM_FILL( 0x0000, 0x0008, 0x00)

    ROM_REGION( 0x8000, "vram", ROMREGION_ERASEFF )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1980, bml3,  	0,       0, 		bml3, 	bml3, 	 0,  	   "Hitachi",   "Basic Master Level 3",		GAME_NOT_WORKING | GAME_NO_SOUND)

