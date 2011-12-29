/***************************************************************************

        Schachcomputer SC1

        12/05/2009 Skeleton driver.

ToDo:
- Everything I/O related (ROM & RAM are correct)
- Display
- Inputs
- Layout (was copied from slc1; needs to be made suitable for sc1)

Port 80-83 could be a device

This happens at the start:
'maincpu' (04EF): unmapped i/o memory write to 0081 = 0F & FF
'maincpu' (04F3): unmapped i/o memory write to 0083 = CF & FF
'maincpu' (04F7): unmapped i/o memory write to 0083 = BB & FF
'maincpu' (04FB): unmapped i/o memory write to 0082 = 01 & FF
'maincpu' (0523): unmapped i/o memory write to 00FC = 02 & FF  **
'maincpu' (0523): unmapped i/o memory write to 00FC = 04 & FF  **

** These two happen for a while (making a tone from a speaker?)

Then:
'maincpu' (0523): unmapped i/o memory write to 00FC = 00 & FF
'maincpu' (0075): unmapped i/o memory write to 0080 = 02 & FF
'maincpu' (0523): unmapped i/o memory write to 00FC = 20 & FF

Then this happens continuously:
Port 80 out - 00, 02, FF
Port FC out - 00, 01, 02, 04, 08, 10, 20, 40, 80 (selecting rows?)
Port 80 in - upper byte = 20 thru 26
Port 82 in - upper byte = 0 thru 7

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"
#include "sc1.lh"


class sc1_state : public driver_device
{
public:
	sc1_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_speaker(*this, SPEAKER_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_speaker;
};

/***************************************************************************

    Display

***************************************************************************/




/***************************************************************************

    Keyboard

***************************************************************************/


static ADDRESS_MAP_START(sc1_mem, AS_PROGRAM, 8, sc1_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x0fff ) AM_ROM
	AM_RANGE( 0x4000, 0x43ff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(sc1_io, AS_IO, 8, sc1_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sc1 )
INPUT_PORTS_END


static MACHINE_RESET(sc1)
{
}

static MACHINE_CONFIG_START( sc1, sc1_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(sc1_mem)
	MCFG_CPU_IO_MAP(sc1_io)

	MCFG_MACHINE_RESET(sc1)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_sc1)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sc1 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "sc1.rom", 0x0000, 0x1000, CRC(26965b23) SHA1(01568911446eda9f05ec136df53da147b7c6f2bf))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                           FULLNAME       FLAGS */
COMP( 1989, sc1,    0,      0,       sc1,       sc1,     0,  "VEB Mikroelektronik Erfurt", "Schachcomputer SC1", GAME_NOT_WORKING | GAME_NO_SOUND )
