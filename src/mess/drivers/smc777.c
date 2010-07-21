/***************************************************************************

    SMC-777 (c) 1983 Sony

    preliminary driver by Angelo Salese

    TODO:
    - no documentation, the entire driver is just a bunch of educated
      guesses ...
    - video emulation is just hacked up together to show something simple,
      dunno yet how it really works.
    - Drop M6845 support and write custom code in place of it.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/sn76496.h"
#include "video/mc6845.h"

static UINT16 cursor_addr;

static VIDEO_START( smc777 )
{
}

static VIDEO_UPDATE( smc777 )
{
	int x,y,count;
	static UINT8 *vram = memory_region(screen->machine, "vram");
	static UINT8 *attr = memory_region(screen->machine, "attr");

	count = 0x0000;

	for(y=0;y<25;y++)
	{
		for(x=0;x<40;x++)
		{
			int tile = vram[count];
			int color = attr[count] & 7;

			drawgfx_opaque(bitmap,cliprect,screen->machine->gfx[0],tile,color,0,0,x*8,y*8);

			if(((cursor_addr) == count) && screen->machine->primary_screen->frame_number() & 0x10) // draw cursor
				drawgfx_transpen(bitmap,cliprect,screen->machine->gfx[0],0x87,7,0,0,x*8,y*8,0);


			count+=2;
		}
	}

    return 0;
}

static READ8_HANDLER( test_r )
{
	return mame_rand(space->machine);
}

static WRITE8_HANDLER( smc777_6845_w )
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
		if(addr_latch == 0x0e)
			cursor_addr = ((data<<8) & 0x3f00) | (cursor_addr & 0xff);
		else if(addr_latch == 0x0f)
			cursor_addr = (cursor_addr & 0x3f00) | (data & 0xff);

		mc6845_register_w(devtag_get_device(space->machine, "crtc"), 0,data);
	}
}

static READ8_HANDLER( smc777_vram_r )
{
	static UINT8 *vram = memory_region(space->machine, "vram");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(devtag_get_device(space->machine, "maincpu"), Z80_B);

	return vram[vram_index | offset*0x100];
}

static READ8_HANDLER( smc777_attr_r )
{
	static UINT8 *attr = memory_region(space->machine, "attr");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(devtag_get_device(space->machine, "maincpu"), Z80_B);

	return attr[vram_index | offset*0x100];
}

static READ8_HANDLER( smc777_pcg_r )
{
	static UINT8 *pcg = memory_region(space->machine, "pcg");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(devtag_get_device(space->machine, "maincpu"), Z80_B);

	return pcg[vram_index | offset*0x100];
}

static WRITE8_HANDLER( smc777_vram_w )
{
	static UINT8 *vram = memory_region(space->machine, "vram");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(devtag_get_device(space->machine, "maincpu"), Z80_B);

	vram[vram_index | offset*0x100] = data;
}

static WRITE8_HANDLER( smc777_attr_w )
{
	static UINT8 *attr = memory_region(space->machine, "attr");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(devtag_get_device(space->machine, "maincpu"), Z80_B);

	attr[vram_index | offset*0x100] = data;
}

static WRITE8_HANDLER( smc777_pcg_w )
{
	static UINT8 *pcg = memory_region(space->machine, "pcg");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(devtag_get_device(space->machine, "maincpu"), Z80_B);

	pcg[vram_index | offset*0x100] = data;

    gfx_element_mark_dirty(space->machine->gfx[0], vram_index >> 3);
}

static READ8_HANDLER( smc777_fbuf_r )
{
	static UINT8 *fbuf = memory_region(space->machine, "fbuf");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(devtag_get_device(space->machine, "maincpu"), Z80_B);

	return fbuf[vram_index | offset*0x100];
}

static WRITE8_HANDLER( smc777_fbuf_w )
{
	static UINT8 *fbuf = memory_region(space->machine, "fbuf");
	static UINT16 vram_index;

	vram_index = cpu_get_reg(devtag_get_device(space->machine, "maincpu"), Z80_B);

	fbuf[vram_index | offset*0x100] = data;
}

static ADDRESS_MAP_START(smc777_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( smc777_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x07) AM_READWRITE(smc777_vram_r,smc777_vram_w)
	AM_RANGE(0x08, 0x0f) AM_READWRITE(smc777_attr_r,smc777_attr_w)
	AM_RANGE(0x10, 0x17) AM_READWRITE(smc777_pcg_r, smc777_pcg_w)
	AM_RANGE(0x18, 0x19) AM_WRITE(smc777_6845_w)
	AM_RANGE(0x1a, 0x1b) AM_READ(test_r) AM_WRITENOP//keyboard data
//	AM_RANGE(0x1c, 0x1c) AM_WRITENOP //status and control data / Printer strobe
//	AM_RANGE(0x1d, 0x1d) AM_WRITENOP //status and control data / Printer status / strobe
//	AM_RANGE(0x1e, 0x1f) AM_WRITENOP //RS232C irq control
//	AM_RANGE(0x20, 0x20) AM_WRITENOP //display mode switching
//	AM_RANGE(0x21, 0x21) AM_WRITENOP //60 Hz irq control
//	AM_RANGE(0x22, 0x22) AM_WRITENOP //printer output data
//	AM_RANGE(0x23, 0x23) AM_WRITENOP //border area control
//	AM_RANGE(0x24, 0x24) AM_WRITENOP //Timer write / specify address (RTC?)
//	AM_RANGE(0x25, 0x25) AM_READNOP  //Timer read (RTC?)
//	AM_RANGE(0x26, 0x26) AM_WRITENOP //RS232C RX / TX
//	AM_RANGE(0x27, 0x27) AM_WRITENOP //RS232C Mode / Command / Status

//	AM_RANGE(0x28, 0x2c) AM_NOP //fdc 2, MB8876 -> FD1791
//	AM_RANGE(0x2d, 0x2f) AM_NOP //rs-232c no. 2
//	AM_RANGE(0x30, 0x34) AM_NOP //fdc 1, MB8876 -> FD1791
//	AM_RANGE(0x35, 0x37) AM_NOP //rs-232c no. 3
//	AM_RANGE(0x38, 0x3b) AM_NOP //cache disk unit
//	AM_RANGE(0x3c, 0x3d) AM_NOP //RGB Superimposer
//	AM_RANGE(0x40, 0x47) AM_NOP //IEEE-488 interface unit
//	AM_RANGE(0x48, 0x50) AM_NOP //HDD (Winchester)
//	AM_RANGE(0x54, 0x59) AM_NOP //VTR Controller
	AM_RANGE(0x53, 0x53) AM_DEVWRITE("sn1", sn76496_w) //not in the datasheet ... almost certainly SMC-777 specific
//	AM_RANGE(0x5a, 0x5b) AM_WRITENOP //RAM banking
//	AM_RANGE(0x70, 0x70) AM_NOP //Auto Start ROM
//	AM_RANGE(0x74, 0x74) AM_NOP //IEEE-488 ROM
//	AM_RANGE(0x75, 0x75) AM_NOP //VTR Controller ROM
	AM_RANGE(0x80, 0xff) AM_READWRITE(smc777_fbuf_r, smc777_fbuf_w) //GRAM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( smc777 )
INPUT_PORTS_END


static MACHINE_RESET(smc777)
{
}


static const gfx_layout smc777_charlayout =
{
	8, 8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( smc777 )
	GFXDECODE_ENTRY( "pcg", 0x0000, smc777_charlayout, 0, 8 )
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

static PALETTE_INIT( smc777 )
{
	int i;

	for (i = 0x000; i < 0x010; i++)
	{
		UINT8 r,g,b;

		if((i & 0x01) == 0x01)
		{
			r = ((i >> 2) & 1) ? 0xff : 0x00;
			g = ((i >> 3) & 1) ? 0xff : 0x00;
			b = ((i >> 1) & 1) ? 0xff : 0x00;
		}
		else { r = g = b = 0x00; }

		palette_set_color_rgb(machine, i, r,g,b);
	}
}

static MACHINE_DRIVER_START( smc777 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz) //4,028 Mhz!
    MDRV_CPU_PROGRAM_MAP(smc777_mem)
    MDRV_CPU_IO_MAP(smc777_io)

    MDRV_MACHINE_RESET(smc777)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 200)
    MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 200-1)
    MDRV_PALETTE_LENGTH(0x10)
    MDRV_PALETTE_INIT(smc777)
	MDRV_GFXDECODE(smc777)

	MDRV_MC6845_ADD("crtc", H46505, XTAL_3_579545MHz/2, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */

    MDRV_VIDEO_START(smc777)
    MDRV_VIDEO_UPDATE(smc777)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("sn1", SN76489A, 1996800) // unknown clock / divider
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( smc777 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "smcrom.dat", 0x0000, 0x4000, CRC(b2520d31) SHA1(3c24b742c38bbaac85c0409652ba36e20f4687a1))

    ROM_REGION( 0x800, "vram", ROMREGION_ERASE00 )

    ROM_REGION( 0x800, "attr", ROMREGION_ERASE00 )

    ROM_REGION( 0x800, "pcg", ROMREGION_ERASE00 )

    ROM_REGION( 0x8000,"fbuf", ROMREGION_ERASE00 )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1983, smc777,  0,       0, 	smc777, 	smc777, 	 0,  "Sony",   "SMC-777",		GAME_NOT_WORKING | GAME_NO_SOUND)

