/***************************************************************************

PowerVu D9234 STB (c) 1997 Scientific Atlanta

20-mar-2010 skeleton driver

****************************************************************************/

#include "emu.h"
#include "cpu/arm7/arm7.h"

static UINT32 *powervu_ram;

static WRITE32_HANDLER( debug_w )
{
	printf("%02x %c\n",data,data);
}

static ADDRESS_MAP_START(pv9234_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x000080cc, 0x000080cf) AM_WRITE(debug_w)
	AM_RANGE(0x0003e000, 0x0003efff) AM_RAM AM_BASE(&powervu_ram)
	AM_RANGE(0x00000000, 0x0007ffff) AM_ROM AM_REGION("maincpu",0) //FLASH ROM!
	AM_RANGE(0x00080000, 0x00087fff) AM_MIRROR(0x78000) AM_RAM AM_SHARE("share1")//mirror is a guess, writes a prg at 0xc0200 then it jumps at b0200 (!)
	AM_RANGE(0xe0000000, 0xe0007fff) AM_MIRROR(0x0fff8000) AM_RAM AM_SHARE("share1")
	AM_RANGE(0xffffff00, 0xffffffff) AM_RAM //i/o? stack ram?
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pv9234 )
INPUT_PORTS_END


static MACHINE_RESET(pv9234)
{
	int i;

	for(i=0;i<0x1000/4;i++)
		powervu_ram[i] = 0;
}

static VIDEO_START( pv9234 )
{
}

static VIDEO_UPDATE( pv9234 )
{
    return 0;
}

static MACHINE_DRIVER_START( pv9234 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", ARM7, 4915000) //probably a more powerful clone.
    MDRV_CPU_PROGRAM_MAP(pv9234_map)

    MDRV_MACHINE_RESET(pv9234)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(pv9234)
    MDRV_VIDEO_UPDATE(pv9234)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pv9234 )
    ROM_REGION32_LE( 0x80000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "u19.bin", 0x00000, 0x20000, CRC(1e06b0c8) SHA1(f8047f7127919e73675375578bb9fcc0eed2178e))
	ROM_LOAD16_BYTE( "u18.bin", 0x00001, 0x20000, CRC(924487dd) SHA1(fb1d7c9a813ded8c820589fa85ae72265a0427c7))
	ROM_LOAD16_BYTE( "u17.bin", 0x40000, 0x20000, CRC(cac03650) SHA1(edd8aec6fed886d47de39ed4e127de0a93250a45))
	ROM_LOAD16_BYTE( "u16.bin", 0x40001, 0x20000, CRC(bd07d545) SHA1(90a63af4ee82b0f7d0ed5f0e09569377f22dd98c))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1997, pv9234,  0,       0, 	pv9234,	pv9234,	 0, 		 "Scientific Atlanta",   "PowerVu D9234",		GAME_NOT_WORKING | GAME_NO_SOUND)

