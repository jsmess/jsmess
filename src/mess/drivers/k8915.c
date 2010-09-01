/***************************************************************************

        Robotron K8915

        30/08/2010 Skeleton driver
		
****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static UINT8 *k8915_ram;

static ADDRESS_MAP_START(k8915_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE(&k8915_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( k8915_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( k8915 )
INPUT_PORTS_END

static MACHINE_RESET(k8915)
{
	UINT8* bios = memory_region(machine, "maincpu");
	memcpy(k8915_ram,bios, 0x1000);
}

static VIDEO_START( k8915 )
{
}

static VIDEO_UPDATE( k8915 )
{
	return 0;
}

static MACHINE_CONFIG_START( k8915, driver_data_t )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
    MDRV_CPU_PROGRAM_MAP(k8915_mem)
	MDRV_CPU_IO_MAP(k8915_io)

    MDRV_MACHINE_RESET(k8915)

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 192) /* border size not accurate */
	MDRV_SCREEN_VISIBLE_AREA(0, 256 - 1, 0, 192 - 1)

    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(k8915)
    MDRV_VIDEO_UPDATE(k8915)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( k8915 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "k8915.bin", 0x0000, 0x1000, CRC(ca70385f) SHA1(a34c14adae9be821678aed7f9e33932ee1f3e61c))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT  		COMPANY   FULLNAME       FLAGS */
COMP( 1982, k8915,  0,       0, 	k8915, 	k8915, 	 0,  	  "Robotron",   "K8915",		GAME_NOT_WORKING | GAME_NO_SOUND)
