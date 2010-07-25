/***************************************************************************

        Hitachi Basic Master Level 3 (MB-6890)

        13/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "video/mc6845.h"

static VIDEO_START( bml3 )
{
}

static VIDEO_UPDATE( bml3 )
{
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
		/*if(addr_latch == 0x0a)
			cursor_raster = data;
		if(addr_latch == 0x0e)
			cursor_addr = ((data<<8) & 0x3f00) | (cursor_addr & 0xff);
		else if(addr_latch == 0x0f)
			cursor_addr = (cursor_addr & 0x3f00) | (data & 0xff);*/

		mc6845_register_w(space->machine->device("crtc"), 0,data);
	}
}

/* Note: this custom code is there just for simplicity, it'll be nuked in the end */
static READ8_HANDLER( bml3_io_r )
{
	static UINT8 *rom = memory_region(space->machine, "maincpu");

	if(offset < 0xc0)
	{
		logerror("I/O read [%02x] at PC=%04x\n",offset,cpu_get_pc(space->cpu));
		return 0;
	}

	if(offset == 0xc8) //this seems the keyboard status
		return 0;

	/* TODO: pretty sure that there's a bankswitch for this */
	return rom[offset+0xff00];
}

static WRITE8_HANDLER( bml3_io_w )
{
	if(offset == 0xc6 || offset == 0xc7) { bml3_6845_w(space,offset-0xc6,data); }
	else
	{
		logerror("I/O write %02x -> [%02x] at PC=%04x\n",data,offset,cpu_get_pc(space->cpu));
	}
}

static ADDRESS_MAP_START(bml3_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x9fff) AM_RAM
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


static MACHINE_DRIVER_START( bml3 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6809, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(bml3_mem)

    MDRV_MACHINE_RESET(bml3)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

	MDRV_MC6845_ADD("crtc", H46505, XTAL_3_579545MHz, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */


    MDRV_VIDEO_START(bml3)
    MDRV_VIDEO_UPDATE(bml3)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( bml3 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "l3bas.rom", 0xa000, 0x6000, CRC(d81baa07) SHA1(a8fd6b29d8c505b756dbf5354341c48f9ac1d24d))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1980, bml3,  	0,       0, 		bml3, 	bml3, 	 0,  	   "Hitachi",   "Basic Master Level 3",		GAME_NOT_WORKING | GAME_NO_SOUND)

