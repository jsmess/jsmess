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
//	static UINT8 *attr = memory_region(screen->machine, "attr");

	count = 0x0000;

	for(y=0;y<25;y++)
	{
		for(x=0;x<40;x++)
		{
			int tile = vram[count] | 0x80;
//			int color = attr[count];

			drawgfx_opaque(bitmap,cliprect,screen->machine->gfx[0],tile,0,0,0,x*8,y*8);

			if(((cursor_addr) == count) && screen->machine->primary_screen->frame_number() & 0x10) // draw cursor
				drawgfx_transpen(bitmap,cliprect,screen->machine->gfx[0],0x107,0,0,0,x*8,y*8,0);


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

static ADDRESS_MAP_START(smc777_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( smc777_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x07) AM_WRITE(smc777_vram_w)
	AM_RANGE(0x08, 0x0f) AM_WRITE(smc777_attr_w)
	AM_RANGE(0x10, 0x17) AM_WRITENOP //PCG data is written there, in a backwards fashion???
	AM_RANGE(0x18, 0x19) AM_WRITE(smc777_6845_w)
	AM_RANGE(0x1b, 0x1b) AM_READ(test_r) //presumably video related
//	AM_RANGE(0x1c, 0x1c) AM_WRITENOP //???
//	AM_RANGE(0x30, 0x37) AM_NOP //fdc, wd1793?
	AM_RANGE(0x53, 0x53) AM_DEVWRITE("sn1", sn76496_w)
//	AM_RANGE(0x80, 0xff) AM_WRITENOP //cleared with the same backwards fashion as the vram / PCG data, bitmap?
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( smc777 )
INPUT_PORTS_END


static MACHINE_RESET(smc777)
{
}


/* F4 Character Displayer */
static const gfx_layout smc777_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	RGN_FRAC(1,1),					/* 320 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( smc777 )
	GFXDECODE_ENTRY( "maincpu", 0x0000, smc777_charlayout, 0, 1 )
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


static MACHINE_DRIVER_START( smc777 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
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
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)
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

    ROM_REGION( 0x2000, "vram", ROMREGION_ERASE00 )

    ROM_REGION( 0x2000, "attr", ROMREGION_ERASE00 )

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1983, smc777,  0,       0, 	smc777, 	smc777, 	 0,  "Sony",   "SMC-777",		GAME_NOT_WORKING | GAME_NO_SOUND)

