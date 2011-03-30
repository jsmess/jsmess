/***************************************************************************

        dolphin

        08/04/2010 Skeleton driver.

    Minimal Setup:
        0000-00FF ROM "MO" (74S471)
        0100-01FF ROM "MONI" (74S471)
        0200-02FF RAM (2x 2112)
        18 pushbuttons for programming (0-F, ADR, NXT).
        4-digit LED display.

    Other options:
        0400-07FF Expansion RAM (8x 2112)
        0800-08FF Pulse for operation of an optional EPROM programmer
        0C00-0FFF ROM "MONA" (2708)
        LEDs connected to all Address and Data Lines
        LEDs connected to WAIT and FLAG lines.
        Speaker with a LED wired across it.
        PAUSE switch.
        RUN/STOP switch.
        STEP switch.
        CLOCK switch.

        Cassette player connected to SENSE and FLAG lines.

        Keyboard encoder: AY-5-2376 (57 keys)

        CRT interface: (512 characters on a separate bus)
        2114 video ram (one half holds the lower 4 data bits, other half the upper bits)
        74LS175 holds the upper bits for the 74LS472
        74LS472 Character Generator

        NOTE: a rom is missing, when the ADR button (- key) is pressed,
        it causes a freeze in nodebug mode, and a crash in debug mode.
        To see it, start in debug mode. g 6c. In the emulation, press the
        minus key. The debugger will stop and you can see an instruction
        referencing location 0100, which is in the missing rom.

        TODO:
        - Proper artwork
        - Find missing roms
        - Add speaker/beeper
        - Add optional hardware listed above
        - Find out how to use it; the keys seem to give nonsense outputs


        Thanks to Amigan site for various documents.
        We used Winarcadia emulator to get an idea of what the computer
        should look like, but we DID NOT look at the source code.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/s2650/s2650.h"
#include "dolphin.lh"


class dolphin_state : public driver_device
{
public:
	dolphin_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, "maincpu")
	{ }
	required_device<cpu_device> m_maincpu;
	DECLARE_READ8_MEMBER( dolphin_07_r );
	DECLARE_WRITE8_MEMBER( dolphin_00_w );
};

WRITE8_MEMBER( dolphin_state::dolphin_00_w )
{
// don't know which offset does which digit

	output_set_digit_value(offset, data);
}

READ8_MEMBER( dolphin_state::dolphin_07_r )
{
	UINT8 keyin, i, data = 0xff;

	keyin = input_port_read(m_machine, "X0");
	if (keyin != 0xff)
		for (i = 0; i < 8; i++)
			if BIT(~keyin, i)
			{
				data = i | 0x70;
				break;
			}

	keyin = input_port_read(m_machine, "X1");
	if (keyin != 0xff)
		for (i = 0; i < 8; i++)
			if BIT(~keyin, i)
			{
				data = i | 0x78;
				break;
			}

	data &= input_port_read(m_machine, "X2");

	return data;
}

static ADDRESS_MAP_START(dolphin_mem, AS_PROGRAM, 8, dolphin_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x01ff) AM_ROM
	AM_RANGE( 0x0200, 0x02ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( dolphin_io , AS_IO, 8, dolphin_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x03) AM_WRITE(dolphin_00_w) // 4-led display
	//AM_RANGE(0x06, 0x06) AM_WRITE(dolphin_06_w)  // speaker (NOT a keyclick)
	AM_RANGE(0x07, 0x07) AM_READ(dolphin_07_r) // pushbuttons
	//AM_RANGE(S2650_SENSE_PORT, S2650_FO_PORT) AM_READWRITE(dolphin_cass_in,dolphin_cass_out)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( dolphin )
	PORT_START("X0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')

	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F") PORT_CODE(KEYCODE_F)

	PORT_START("X2")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("NXT") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ADR") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT( 0xcf, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static MACHINE_RESET(dolphin)
{
}

static MACHINE_CONFIG_START( dolphin, dolphin_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",S2650, XTAL_1MHz)
	MCFG_CPU_PROGRAM_MAP(dolphin_mem)
	MCFG_CPU_IO_MAP(dolphin_io)

	MCFG_MACHINE_RESET(dolphin)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_dolphin)

	/* sound hardware */
	//MCFG_SPEAKER_STANDARD_MONO("mono")
	//MCFG_SOUND_ADD("beep", BEEP, 0)
	//MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( dolphin )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "dolphin_mo.rom", 0x0000, 0x0100, CRC(1ac4ac18) SHA1(62a63de6fcd6cd5fcee930d31c73fe603647f06c))
	ROM_LOAD( "dolphin_moni.rom", 0x0100, 0x0100, NO_DUMP )
	ROM_LOAD_OPTIONAL( "dolphin_mona.rom", 0x0c00, 0x0400, NO_DUMP )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1979, dolphin,  0,       0,	dolphin,	dolphin,	 0,   "<unknown>",   "Dolphin",	GAME_NOT_WORKING | GAME_NO_SOUND )

