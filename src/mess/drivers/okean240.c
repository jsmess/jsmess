/***************************************************************************
   
        Okean-240

        28/12/2011 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"

class okean240_state : public driver_device
{
public:
	okean240_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }
	UINT8 *videoram;
};


static READ8_HANDLER( okean240_rand_r ) { return mame_rand(space->machine); } // so we can start booting


static ADDRESS_MAP_START(okean240_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_RAMBANK("boot")
	AM_RANGE(0x0800, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0x7fff) AM_RAM AM_BASE_MEMBER(okean240_state, videoram)
	AM_RANGE(0x8000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( okean240_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0xff) AM_READ(okean240_rand_r)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( okean240 )
INPUT_PORTS_END


/* after the first 6 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( okean240_boot )
{
	memory_set_bank(machine, "boot", 0);
}

static MACHINE_RESET(okean240) 
{	
	timer_set(machine, ATTOTIME_IN_USEC(10), NULL, 0, okean240_boot);
	memory_set_bank(machine, "boot", 1);
}

DRIVER_INIT( okean240 )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0xe000);
}

static VIDEO_START( okean240 )
{
}

// The video appears to be bitmapped, but atm this produces scrolling garbage
static VIDEO_UPDATE( okean240 )
{
	okean240_state *state = screen->machine->driver_data<okean240_state>();
	UINT8 gfx;
	UINT16 ma=0,x,y;

	for (y = 0; y < 256; y++)
	{
		UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);

		for (x = ma; x < ma+64; x++)
		{
			gfx = state->videoram[x];

			*p++ = ( gfx & 0x80 ) ? 1 : 0;
			*p++ = ( gfx & 0x40 ) ? 1 : 0;
			*p++ = ( gfx & 0x20 ) ? 1 : 0;
			*p++ = ( gfx & 0x10 ) ? 1 : 0;
			*p++ = ( gfx & 0x08 ) ? 1 : 0;
			*p++ = ( gfx & 0x04 ) ? 1 : 0;
			*p++ = ( gfx & 0x02 ) ? 1 : 0;
			*p++ = ( gfx & 0x01 ) ? 1 : 0;
		}
		ma+=64;
	}
	return 0;
}

/* F4 Character Displayer */
static const gfx_layout okean240_charlayout =
{
	8, 7,					/* 8 x 7 characters */
	160,					/* 160 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8 },
	8*7					/* every char takes 7 bytes */
};

static GFXDECODE_START( okean240 )
	GFXDECODE_ENTRY( "maincpu", 0xef63, okean240_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( okean240, okean240_state )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",I8080, XTAL_12MHz / 6)
	MDRV_CPU_PROGRAM_MAP(okean240_mem)
	MDRV_CPU_IO_MAP(okean240_io)	

	MDRV_MACHINE_RESET(okean240)
	
	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_GFXDECODE(okean240)

	MDRV_VIDEO_START(okean240)
	MDRV_VIDEO_UPDATE(okean240)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( okean240 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "Normal", "Normal")
	ROMX_LOAD( "monitor.bin", 0xe000, 0x2000, CRC(bcac5ca0) SHA1(602ab824704d3d5d07b3787f6227ff903c33c9d5), ROM_BIOS(1))
	ROMX_LOAD( "cpm80.bin",   0xc000, 0x2000, CRC(b89a7e16) SHA1(b8f937c04f430be18e48f296ed3ef37194171204), ROM_BIOS(1))

	ROM_SYSTEM_BIOS(1, "Test", "Test")
	ROMX_LOAD( "test.bin",    0xe000, 0x0800, CRC(e9e2b7b9) SHA1(e4e0b6984a2514b6ba3e97500d487ea1a68b7577), ROM_BIOS(2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( ????, okean240,  0,       0, 	okean240, 	okean240, okean240,  "<unknown>",   "Okean-240",		GAME_NOT_WORKING | GAME_NO_SOUND)

