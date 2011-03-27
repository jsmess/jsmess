/***************************************************************************

        IQ-151

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"


class iq151_state : public driver_device
{
public:
	iq151_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(iq151_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xefff ) AM_RAM
	AM_RANGE( 0xf000, 0xffff ) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( iq151_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( iq151 )
INPUT_PORTS_END


static MACHINE_RESET(iq151)
{
}

static VIDEO_START( iq151 )
{
}

static SCREEN_UPDATE( iq151 )
{
    return 0;
}

/* F4 Character Displayer */
static const gfx_layout iq151_32_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	128,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static const gfx_layout iq151_64_charlayout =
{
	6, 8,					/* 6 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( iq151 )
	GFXDECODE_ENTRY( "chargen", 0x0000, iq151_32_charlayout, 0, 1 )
	GFXDECODE_ENTRY( "chargen", 0x0400, iq151_64_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( iq151, iq151_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(iq151_mem)
    MCFG_CPU_IO_MAP(iq151_io)

    MCFG_MACHINE_RESET(iq151)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(iq151)

	MCFG_GFXDECODE(iq151)
    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(iq151)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( iq151 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	/* A number of bios versions here. The load address is shown for each */
	ROM_LOAD( "iq151_disc2_12_5_1987_v4_0.rom", 0xe000, 0x0800, CRC(b189b170) SHA1(3e2ca80934177e7a32d0905f5a0ad14072f9dabf))
	ROM_LOAD( "iq151_monitor_cpm.rom", 0xf000, 0x1000, CRC(26f57013) SHA1(4df396edc375dd2dd3c82c4d2affb4f5451066f1))
	ROM_LOAD( "iq151_monitor_cpm_old.rom", 0xf000, 0x1000, CRC(6743e1b7) SHA1(ae4f3b1ba2511a1f91c4e8afdfc0e5aeb0fb3a42))
	ROM_LOAD( "iq151_monitor_disasm.rom", 0xf000, 0x1000, CRC(45c2174e) SHA1(703e3271a124c3ef9330ae399308afd903316ab9))
	ROM_LOAD( "iq151_monitor_orig.rom", 0xf000, 0x1000, CRC(acd10268) SHA1(4d75c73f155ed4dc2ac51a9c22232f869cca95e2))

	ROM_REGION( 0x0c00, "chargen", 0 )
	ROM_LOAD( "iq151_video32font.rom", 0x0000, 0x0400, CRC(395567a7) SHA1(18800543daf4daed3f048193c6ae923b4b0e87db))
	ROM_LOAD( "iq151_video64font.rom", 0x0400, 0x0800, CRC(cb6f43c0) SHA1(4b2c1d41838d569228f61568c1a16a8d68b3dadf))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( 198?, iq151,  0,       0, 	iq151,	iq151,	 0, 	"ZPA Novy Bor", "IQ-151", GAME_NOT_WORKING | GAME_NO_SOUND)

