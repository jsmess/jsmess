/***************************************************************************

        SBC6510 from Josip Perusanec

        18/12/2009 Skeleton driver.

    CPU MOS 6510 (1MHz)
    ROM 4KB
    RAM 128KB
    CIA 6526 - for interrupt gen and scan of keyboard
    YM2149/AY-3-8910 - sound + HDD/CF IDE
    GAL16V8 - address decoder
    ATMEGA8 - gen. of PAL video signal (modified TellyMate)
    keyboard of C64 computer used

****************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "machine/6526cia.h"
#include "sound/ay8910.h"


class sbc6510_state : public driver_device
{
public:
	sbc6510_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(sbc6510_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xdfff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sbc6510 )
INPUT_PORTS_END


static MACHINE_RESET(sbc6510)
{
}

static VIDEO_START( sbc6510 )
{
}

static SCREEN_UPDATE( sbc6510 )
{
    return 0;
}

static const m6502_interface sbc6510_m6510_interface =
{
	NULL,
	NULL,
	NULL,
	NULL
};

static const ay8910_interface sbc6510_ay_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL
};

const mos6526_interface sbc6510_ntsc_cia0 =
{
	10, /* 1/10 second */
	DEVCB_NULL,
	DEVCB_NULL,	/* pc_func */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static MACHINE_CONFIG_START( sbc6510, sbc6510_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",M6510, XTAL_1MHz)
	MCFG_CPU_CONFIG( sbc6510_m6510_interface )
    MCFG_CPU_PROGRAM_MAP(sbc6510_mem)

    MCFG_MACHINE_RESET(sbc6510)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(sbc6510)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(sbc6510)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("ay8910", AY8910, XTAL_1MHz)
	MCFG_SOUND_CONFIG(sbc6510_ay_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_MOS6526R1_ADD("cia_0", XTAL_1MHz, sbc6510_ntsc_cia0)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sbc6510 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "sbc6510.rom", 0xf000, 0x1000, CRC(e13a5e62) SHA1(1e7482e9b98b39d0cc456254fbe8fd0981e9377e))
	ROM_REGION( 0x2000, "videocpu", ROMREGION_ERASEFF ) // ATMEGA8 at 16MHz
	ROM_LOAD( "video.bin",   0x0000, 0x2000, CRC(809f31ce) SHA1(4639de5f7b8f6c036d74f217ba85e7e897039094))
	ROM_REGION( 0x200, "gal", ROMREGION_ERASEFF )
	ROM_LOAD( "sbc6510.gal", 0x0000, 0x0117, CRC(f78f9927) SHA1(b951163958f5722032826d0d17a07c81dbd5f68e))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 2009, sbc6510,  0,       0,	sbc6510,	sbc6510,	 0,   "Josip Perusanec",   "SBC6510",		GAME_NOT_WORKING)

