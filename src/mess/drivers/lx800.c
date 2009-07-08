/***************************************************************************

    Epson LX-800 dot matrix printer

    Skeleton driver

***************************************************************************/

#include "driver.h"
#include "cpu/upd7810/upd7810.h"
#include "sound/beep.h"
#include "lx800.lh"


/***************************************************************************
    GATE ARRAY
***************************************************************************/

static READ8_HANDLER( lx800_ga_r )
{
	logerror("%s: lx800_ga_r(%02x)\n", cpuexec_describe_context(space->machine), offset);

	return 0xff;
}

static WRITE8_HANDLER( lx800_ga_w )
{
	logerror("%s: lx800_ga_w(%02x): %02x\n", cpuexec_describe_context(space->machine), offset, data);
}


/***************************************************************************
    I/O PORTS
***************************************************************************/

static READ8_HANDLER( lx800_porta_r )
{
	logerror("%s: lx800_porta_r(%02x)\n", cpuexec_describe_context(space->machine), offset);

	return 0xff;
}

static WRITE8_HANDLER( lx800_porta_w )
{
	logerror("%s: lx800_porta_w(%02x): %02x\n", cpuexec_describe_context(space->machine), offset, data);
}

static READ8_HANDLER( lx800_portc_r )
{
	logerror("%s: lx800_portc_r(%02x)\n", cpuexec_describe_context(space->machine), offset);

	return 0xff;
}

static WRITE8_HANDLER( lx800_portc_w )
{
	logerror("%s: lx800_portc_w(%02x): %02x\n", cpuexec_describe_context(space->machine), offset, data);
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( lx800_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM /* 32k firmware */
	AM_RANGE(0x8000, 0x9fff) AM_RAM /* 8k external RAM */
	AM_RANGE(0xa000, 0xbfff) AM_NOP /* not used */
	AM_RANGE(0xc000, 0xdfff) AM_MIRROR(0x1ff8) AM_READWRITE(lx800_ga_r, lx800_ga_w)
	AM_RANGE(0xe000, 0xfeff) AM_NOP /* not used */
	AM_RANGE(0xff00, 0xffff) AM_RAM /* internal CPU RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( lx800_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(UPD7810_PORTA, UPD7810_PORTA) AM_READWRITE(lx800_porta_r, lx800_porta_w)
	AM_RANGE(UPD7810_PORTB, UPD7810_PORTB) AM_READ_PORT("DIPSW1")
	AM_RANGE(UPD7810_PORTC, UPD7810_PORTC) AM_READWRITE(lx800_portc_r, lx800_portc_w)
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( lx800 )
	PORT_START("DIPSW1")
	PORT_DIPNAME(0x01, 0x00, "Typeface")
	PORT_DIPLOCATION("DIP:8")
	PORT_DIPSETTING(0x01, "Condensed")
	PORT_DIPSETTING(0x00, DEF_STR(Normal))
	PORT_DIPNAME(0x02, 0x00, "ZERO font")
	PORT_DIPLOCATION("DIP:7")
	PORT_DIPSETTING(0x02, "0")
	PORT_DIPSETTING(0x00, "0")
	PORT_DIPNAME(0x04, 0x00, "Character Table")
	PORT_DIPLOCATION("DIP:6")
	PORT_DIPSETTING(0x04, "Graphic")
	PORT_DIPSETTING(0x00, "Italic")
	PORT_DIPNAME(0x08, 0x00, "Paper-out detection")
	PORT_DIPLOCATION("DIP:5")
	PORT_DIPSETTING(0x08, "Valid")
	PORT_DIPSETTING(0x00, "Invalid")
	PORT_DIPNAME(0x10, 0x00, "Printing quality")
	PORT_DIPLOCATION("DIP:4")
	PORT_DIPSETTING(0x10, "NLQ")
	PORT_DIPSETTING(0x00, "Draft")
	PORT_DIPNAME(0xe0, 0xe0, "International character set")
	PORT_DIPLOCATION("DIP:3,2,1")
	PORT_DIPSETTING(0xe0, "U.S.A.")
	PORT_DIPSETTING(0x60, "France")
	PORT_DIPSETTING(0xa0, "Germany")
	PORT_DIPSETTING(0x20, "U.K.")
	PORT_DIPSETTING(0xc0, "Denmark")
	PORT_DIPSETTING(0x40, "Sweden")
	PORT_DIPSETTING(0x80, "Italy")
	PORT_DIPSETTING(0x00, "Spain")

	PORT_START("DIPSW2")
	PORT_DIPNAME(0x01, 0x00, "Page length")
	PORT_DIPLOCATION("DIP:4")
	PORT_DIPSETTING(0x01, "12\"")
	PORT_DIPSETTING(0x00, "11\"")
	PORT_DIPNAME(0x02, 0x00, "Cut sheet feeder mode")
	PORT_DIPLOCATION("DIP:3")
	PORT_DIPSETTING(0x02, "Valid")
	PORT_DIPSETTING(0x00, "Invalid")
	PORT_DIPNAME(0x04, 0x00, "1\" skip over perforation")
	PORT_DIPLOCATION("DIP:2")
	PORT_DIPSETTING(0x04, "Valid")
	PORT_DIPSETTING(0x00, "Invalid")
	PORT_DIPNAME(0x08, 0x00, "AUTO FEED XT control")
	PORT_DIPLOCATION("DIP:1")
	PORT_DIPSETTING(0x08, "Fix to LOW")
	PORT_DIPSETTING(0x00, "Depends on external signal")
INPUT_PORTS_END


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static const UPD7810_CONFIG lx800_cpu_config =
{
    TYPE_7810,
    0
};

static MACHINE_DRIVER_START( lx800 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", UPD7810, XTAL_14_7456MHz)
	MDRV_CPU_CONFIG(lx800_cpu_config)
	MDRV_CPU_PROGRAM_MAP(lx800_mem)
	MDRV_CPU_IO_MAP(lx800_io)

	MDRV_DEFAULT_LAYOUT(layout_lx800)

	/* audio hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( lx800 )
    ROM_REGION(0x8000, "maincpu", 0)
    ROM_LOAD("lx800.bin", 0x0000, 0x8000, CRC(da06c45b) SHA1(9618c940dd10d5b43cd1edd5763b90e6447de667))
ROM_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME   PARENT  COMPAT  MACHINE  INPUT  INIT  CONFIG  COMPANY  FULLNAME  FLAGS */
COMP( 1987, lx800, 0,      0,      lx800,   lx800, 0,    0,      "Epson", "LX-800", GAME_NOT_WORKING )
