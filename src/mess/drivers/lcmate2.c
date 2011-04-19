/***************************************************************************

        Laser Compumate2

        17/04/2011 Initial driver by Robbbert, using info supplied by
        DMEnduro. He owns one of these machines.

        If you have further info, this driver is yours!

        Chips: Z80 CPU, 8K of RAM, 32K ROM, 'VTCL', '8826KX',
               RP5C15 RTC.

        Display: 2 lines of 20 characters LCD.

        There are no devices for file saving or loading, is more of
        a PDA format with all programs (address book, etc) in the ROM.

        Sound: a piezo device which is assumed to be a beeper at this time.

        ToDo:
        - Absolutely everything!


****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/beep.h"


class lcmate2_state : public driver_device
{
public:
	lcmate2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
	m_maincpu(*this, "maincpu"),
	//m_rtc(*this, "rtc"),
	m_beep(*this, "beep")
	{ }

	required_device<cpu_device> m_maincpu;
	//required_device<cpu_device> m_rtc;
	required_device<cpu_device> m_beep;
};


static ADDRESS_MAP_START(lcmate2_mem, AS_PROGRAM, 8, lcmate2_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_ROM
	AM_RANGE( 0x8000, 0x9fff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(lcmate2_io, AS_IO, 8, lcmate2_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( lcmate2 )
INPUT_PORTS_END


static MACHINE_RESET(lcmate2)
{
	//lcmate2_state *state = machine.driver_data<lcmate2_state>();
}

static MACHINE_CONFIG_START( lcmate2, lcmate2_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_3_579545MHz) // confirmed
	MCFG_CPU_PROGRAM_MAP(lcmate2_mem)
	MCFG_CPU_IO_MAP(lcmate2_io)

	MCFG_MACHINE_RESET(lcmate2)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 250)
	MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 249)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("beep", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* Devices */
	// RP5C15 RTC "rtc"
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( lcmate2 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "u2.bin",  0x0000, 0x8000, CRC(521931b9) SHA1(743a6e2928c4365fbf5ed9a173e2c1bfe695850f) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1984, lcmate2,  0,       0,	lcmate2, 	lcmate2, 	 0,   "Vtech",   "Laser Compumate 2", GAME_NOT_WORKING | GAME_NO_SOUND_HW)

