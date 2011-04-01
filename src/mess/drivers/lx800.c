/***************************************************************************

    Epson LX-800 dot matrix printer

    Skeleton driver

***************************************************************************/

#include "emu.h"
#include "cpu/upd7810/upd7810.h"
#include "machine/e05a03.h"
#include "sound/beep.h"
#include "lx800.lh"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

class lx800_state : public driver_device
{
public:
	lx800_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	device_t *m_speaker;
};


/***************************************************************************
    I/O PORTS
***************************************************************************/

/* PA0   W  CRCOM  carriage motor, 0 = holding voltage, 1 = drive voltage
 * PA1             not used
 * PA2   W  PFCOM  paper feed motor, 0 = holding voltage, 1 = drive voltage
 * PA3  R   LF SW  line feed switch
 * PA4  R   FF SW  form feed switch
 * PA5  R   PE SW  paper end sensor, 0 = no paper, 1 = paper
 * PA6             not used
 * PA7  R   P/S    P/S signal from the optional interface
 */
static READ8_HANDLER( lx800_porta_r )
{
	UINT8 result = 0;

	logerror("%s: lx800_porta_r(%02x)\n", space->machine().describe_context(), offset);

	result |= input_port_read(space->machine(), "LINEFEED") << 3;
	result |= input_port_read(space->machine(), "FORMFEED") << 4;
	result |= 1 << 5;

	result |= 1 << 7;

	return result;
}

static WRITE8_HANDLER( lx800_porta_w )
{
	logerror("%s: lx800_porta_w(%02x): %02x\n", space->machine().describe_context(), offset, data);
	logerror("--> carriage: %d, paper feed: %d\n", BIT(data, 0), BIT(data, 2));
}

/* PC0   W  TXD        serial i/o txd
 * PC1  R   RXD        serial i/o rxd
 * PC2   W  ONLINE LP  online led
 * PC3  R   ONLINE SW  online switch
 * PC4   W  ERR        centronics error
 * PC5   W  ACK        centronics acknowledge
 * PC6   W  FIRE       drive pulse width signal
 * PC7   W  BUZZER     buzzer signal
 */
static READ8_HANDLER( lx800_portc_r )
{
	UINT8 result = 0;

	logerror("%s: lx800_portc_r(%02x)\n", space->machine().describe_context(), offset);

	result |= input_port_read(space->machine(), "ONLINE") << 3;

	return result;
}

static WRITE8_HANDLER( lx800_portc_w )
{
	lx800_state *lx800 = space->machine().driver_data<lx800_state>();

	logerror("%s: lx800_portc_w(%02x): %02x\n", space->machine().describe_context(), offset, data);
	logerror("--> err: %d, ack: %d, fire: %d, buzzer: %d\n", BIT(data, 4), BIT(data, 5), BIT(data, 6), BIT(data, 7));

	output_set_value("online_led", !BIT(data, 2));
	beep_set_state(lx800->m_speaker, !BIT(data, 7));
}


/***************************************************************************
    GATE ARRAY
***************************************************************************/

static READ8_DEVICE_HANDLER( lx800_centronics_data_r )
{
	logerror("centronics: data read\n");
	return 0x55;
}

static WRITE_LINE_DEVICE_HANDLER( lx800_centronics_pe_w )
{
	logerror("centronics: pe = %d\n", state);
}

static WRITE_LINE_DEVICE_HANDLER( lx800_paperempty_led_w )
{
	output_set_value("paperout_led", state);
}

static WRITE_LINE_DEVICE_HANDLER( lx800_reset_w )
{
	device->machine().device("maincpu")->reset();
}


/***************************************************************************
    MACHINE EMULATION
***************************************************************************/

static MACHINE_START( lx800 )
{
	lx800_state *lx800 = machine.driver_data<lx800_state>();

	lx800->m_speaker = machine.device("beep");

	beep_set_state(lx800->m_speaker, 0);
	beep_set_frequency(lx800->m_speaker, 4000); /* ? */
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( lx800_mem, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM /* 32k firmware */
	AM_RANGE(0x8000, 0x9fff) AM_RAM /* 8k external RAM */
	AM_RANGE(0xa000, 0xbfff) AM_NOP /* not used */
	AM_RANGE(0xc000, 0xdfff) AM_MIRROR(0x1ff8) AM_DEVREADWRITE("ic3b", e05a03_r, e05a03_w)
	AM_RANGE(0xe000, 0xfeff) AM_NOP /* not used */
	AM_RANGE(0xff00, 0xffff) AM_RAM /* internal CPU RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( lx800_io, AS_IO, 8 )
	AM_RANGE(UPD7810_PORTA, UPD7810_PORTA) AM_READWRITE(lx800_porta_r, lx800_porta_w)
	AM_RANGE(UPD7810_PORTB, UPD7810_PORTB) AM_READ_PORT("DIPSW1")
	AM_RANGE(UPD7810_PORTC, UPD7810_PORTC) AM_READWRITE(lx800_portc_r, lx800_portc_w)
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( lx800 )
	PORT_START("ONLINE")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("On Line") PORT_CODE(KEYCODE_O)

	PORT_START("FORMFEED")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Form Feed") PORT_CODE(KEYCODE_F)

	PORT_START("LINEFEED")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Line Feed") PORT_CODE(KEYCODE_L)

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

static const e05a03_interface lx800_e05a03_intf =
{
	DEVCB_HANDLER(lx800_centronics_data_r),
	DEVCB_NULL,
	DEVCB_LINE(lx800_paperempty_led_w),
	DEVCB_LINE(lx800_centronics_pe_w),
	DEVCB_LINE(lx800_reset_w)
};

static MACHINE_CONFIG_START( lx800, lx800_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", UPD7810, XTAL_14_7456MHz)
	MCFG_CPU_CONFIG(lx800_cpu_config)
	MCFG_CPU_PROGRAM_MAP(lx800_mem)
	MCFG_CPU_IO_MAP(lx800_io)

	MCFG_MACHINE_START(lx800)

	MCFG_DEFAULT_LAYOUT(layout_lx800)

	/* audio hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("beep", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	/* gate array */
	MCFG_E05A03_ADD("ic3b", lx800_e05a03_intf)
MACHINE_CONFIG_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( lx800 )
    ROM_REGION(0x8000, "maincpu", 0)
    ROM_LOAD("lx800.ic3c", 0x0000, 0x8000, CRC(da06c45b) SHA1(9618c940dd10d5b43cd1edd5763b90e6447de667))
ROM_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME   PARENT  COMPAT  MACHINE  INPUT  INIT  COMPANY  FULLNAME  FLAGS */
COMP( 1987, lx800, 0,      0,      lx800,   lx800, 0,    "Epson", "LX-800", GAME_NOT_WORKING )
