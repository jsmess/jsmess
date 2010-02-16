/***************************************************************************

        Bandai Pippin

        17/07/2009 Skeleton driver.

    Apple ASICs identified:
    -----------------------
    343S1125    Grand Central (SWIM III, Sound, VIA)
    341S0060    Cuda (ADB, PRAM)
    343S1146    ??? (likely SCSI due to position on board)
    343S1191 (x2)   ??? (apparently clock generators)


    Other chips
    -----------
    Z8530 SCC
    CS4217 audio DAC
    Bt856 video DAC


****************************************************************************/

#include "emu.h"
#include "cpu/powerpc/ppc.h"
#include "devices/chd_cd.h"
#include "sound/cdda.h"

static ADDRESS_MAP_START( pippin_mem, ADDRESS_SPACE_PROGRAM, 64 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x005fffff) AM_RAM
	AM_RANGE(0xf3016000, 0xf3016007) AM_NOP // ADB PORTB
	AM_RANGE(0xf3016400, 0xf3016407) AM_NOP // ADB DDRB
	AM_RANGE(0xf3016600, 0xf3016607) AM_NOP // ?
	AM_RANGE(0xf3016800, 0xf3016807) AM_NOP // ?
	AM_RANGE(0xf3016a00, 0xf3016a07) AM_NOP // ?
	AM_RANGE(0xf3016c00, 0xf3016c07) AM_NOP // ?
	AM_RANGE(0xf3017400, 0xf3017407) AM_NOP // ADB SHR
	AM_RANGE(0xf3017600, 0xf3017607) AM_NOP // ADB ACR
	AM_RANGE(0xf3017800, 0xf3017807) AM_NOP // ADB PCR
	AM_RANGE(0xf3017a00, 0xf3017a07) AM_NOP // ADB IFR
	AM_RANGE(0xf3017c00, 0xf3017c07) AM_NOP // ADB IER
	AM_RANGE(0xf3017e00, 0xf3017e07) AM_NOP // ?
	AM_RANGE(0xffc00000, 0xffffffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pippin )
INPUT_PORTS_END


static MACHINE_RESET(pippin)
{
}

static VIDEO_START( pippin )
{
}

static VIDEO_UPDATE( pippin )
{
    return 0;
}

static MACHINE_DRIVER_START( pippin )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",PPC603, 66000000)
    MDRV_CPU_PROGRAM_MAP(pippin_mem)

    MDRV_MACHINE_RESET(pippin)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(pippin)
    MDRV_VIDEO_UPDATE(pippin)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MDRV_SOUND_ADD( "cdda", CDDA, 0 )
	MDRV_SOUND_ROUTE( 0, "lspeaker", 1.00 )
	MDRV_SOUND_ROUTE( 1, "rspeaker", 1.00 )

	MDRV_CDROM_ADD("cdrom")
MACHINE_DRIVER_END

/* ROM definition */
/*

    BIOS versions

    dev
    monitor 341S0241 - 245,247,248,250
    1.0     341S0251..254               U1-U4
    1.2     341S0297..300               U15,U20,U31,U35
    1.3     341S0328..331               U1/U31, U2/U35, U3/U15 and U4/U20

*/

ROM_START( pippin )
    ROM_REGION( 0x400000, "user1",  ROMREGION_64BIT | ROMREGION_BE )
	ROM_LOAD64_WORD_SWAP( "341s0251.u1", 0x000006, 0x100000, CRC(aaea2449) SHA1(2f63e215260a42fb7c5f2364682d5e8c0604646f) )
	ROM_LOAD64_WORD_SWAP( "341s0252.u2", 0x000004, 0x100000, CRC(3d584419) SHA1(e29c764816755662693b25f1fb3c24faef4e9470) )
	ROM_LOAD64_WORD_SWAP( "341s0253.u3", 0x000002, 0x100000, CRC(d8ae5037) SHA1(d46ce4d87ca1120dfe2cf2ba01451f035992b6f6) )
	ROM_LOAD64_WORD_SWAP( "341s0254.u4", 0x000000, 0x100000, CRC(3e2851ba) SHA1(7cbf5d6999e890f5e9ab2bc4b10ca897c4dc2016) )

	ROM_REGION( 0x10000, "cdrom", 0 ) /* MATSUSHITA CR504-L OEM */
	ROM_LOAD( "504par4.0i.ic7", 0x0000, 0x10000, CRC(25f7dd46) SHA1(ec3b3031742807924c6259af865e701827208fec) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1996, pippin,  0,       0,	pippin, 	pippin, 	 0,  "Apple/Bandai",   "Pippin @mark",		GAME_NOT_WORKING)
