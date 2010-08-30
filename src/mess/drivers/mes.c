/***************************************************************************

        Schleicher MES

        30/08/2010 Skeleton driver
		
****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(mes_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x0000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mes_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( mes )
INPUT_PORTS_END

static MACHINE_RESET(mes)
{
}

static VIDEO_START( mes )
{
}

static VIDEO_UPDATE( mes )
{
	return 0;
}

static MACHINE_DRIVER_START( mes )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
    MDRV_CPU_PROGRAM_MAP(mes_mem)
	MDRV_CPU_IO_MAP(mes_io)

    MDRV_MACHINE_RESET(mes)

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 192) /* border size not accurate */
	MDRV_SCREEN_VISIBLE_AREA(0, 256 - 1, 0, 192 - 1)

    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(mes)
    MDRV_VIDEO_UPDATE(mes)
MACHINE_DRIVER_END


/* ROM definition */
ROM_START( mes )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mescpu.bin",   0x0000, 0x1000, CRC(b6d90cf4) SHA1(19e608af5bdaabb00a134e1106b151b00e2a0b04))
    ROM_REGION( 0x10000, "xebec", ROMREGION_ERASEFF )
	ROM_LOAD( "mesxebec.bin", 0x0000, 0x2000, CRC(061b7212) SHA1(c5d600116fb7563c69ebd909eb9613269b2ada0f))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT  		COMPANY   FULLNAME       FLAGS */
COMP( 198?, mes,  0,       0, 	mes, 	mes, 	 0,  	  "Schleicher",   "MES",		GAME_NOT_WORKING | GAME_NO_SOUND)
