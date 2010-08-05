#include "emu.h"
#include "cpu/m6805/m6805.h"

class comxpl80_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, comxpl80_state(machine)); }

	comxpl80_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* printer state */
	UINT8 centronics_data;	/* centronics data */

	/* PL-80 plotter state */
	UINT16 font_addr;		/* font ROM pack address latch */
	UINT8 x_motor_phase;	/* X motor phase */
	UINT8 y_motor_phase;	/* Y motor phase */
	UINT8 z_motor_phase;	/* Z motor phase */
	UINT8 plotter_data;		/* plotter data bus */
	int plotter_ack;		/* plotter acknowledge */
	int plotter_online;		/* online LED */
};

/* Read/Write Handlers */

static WRITE8_HANDLER( pl80_port_a_w )
{
	/*

        bit     description

        0       Y motor phase A
        1       Y motor phase B
        2       Y motor phase C
        3       Y motor phase D
        4       ROM A12
        5       ROM CE /PT5 CK
        6       PT4 OE
        7       SW & PE ENABLE

    */

	comxpl80_state *state = space->machine->driver_data<comxpl80_state>();

	state->y_motor_phase = data & 0x0f;
	state->font_addr = (BIT(data, 4) << 12) | (state->font_addr & 0xfff);

	state->plotter_data = 0xff;

	if (BIT(data, 5))
	{
		// write motor phase data
	}
	else
	{
		// read data from font ROM
		int font_rom = (input_port_read(space->machine, "FONT") & 0x03) * 0x2000;

		state->plotter_data = memory_region(space->machine, "gfx2")[font_rom | state->font_addr];
	}

	if (!BIT(data, 6))
	{
		// read data from Centronics bus
		state->plotter_data = state->centronics_data;
	}

	if (BIT(data, 7))
	{
		// read switches
		state->plotter_data = input_port_read(space->machine, "SW");
	}
}

static WRITE8_HANDLER( pl80_port_b_w )
{
	/*

        bit     description

        0       Z motor phase A
        1       Z motor phase B
        2       Z motor phase C
        3       Z motor phase D
        4       ROM A8
        5       ROM A9
        6       ROM A10
        7       ROM A11

    */

	comxpl80_state *state = space->machine->driver_data<comxpl80_state>();

	state->z_motor_phase = data & 0x0f;

	state->font_addr = (state->font_addr & 0x10ff) | (data << 4);
}

static WRITE8_HANDLER( pl80_port_c_w )
{
	/*

        bit     description

        0       ROM A0 /X motor phase A
        1       ROM A1 /X motor phase B
        2       ROM A2 /X motor phase C
        3       ROM A3 /X motor phase D
        4       ROM A4 /ACK
        5       ROM A5 /On-line LED
        6       ROM A6
        7       ROM A7

    */

	comxpl80_state *state = space->machine->driver_data<comxpl80_state>();

	state->font_addr = (state->font_addr & 0x1f00) | data;

	state->x_motor_phase = data & 0x0f;

	state->plotter_ack = BIT(data, 4);
	state->plotter_online = BIT(data, 5);
}

static READ8_HANDLER( pl80_port_d_r )
{
	/*

        bit     description

        0       D0 /ROM D0 /DOWN SW
        1       D1 /ROM D1 /PEN-SEL SW
        2       D2 /ROM D2 /UP SW
        3       D3 /ROM D3 /CRSW
        4       D4 /ROM D4 /ON LINE SW
        5       D5 /ROM D5 /PE Sensor
        6       D6 /ROM D6 /RIGHT SW
        7       D7 /ROM D7 /LEFT SW

    */

	comxpl80_state *state = space->machine->driver_data<comxpl80_state>();

	return state->plotter_data;
}

/* Memory Maps */

static ADDRESS_MAP_START( pl80_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
/*  AM_RANGE(0x000, 0x000) AM_READWRITE(cx005_port_a_r, cx005_port_a_w)
    AM_RANGE(0x001, 0x001) AM_READWRITE(cx005_port_b_r, cx005_port_b_w)
    AM_RANGE(0x002, 0x002) AM_READWRITE(cx005_port_c_r, cx005_port_c_w)
    AM_RANGE(0x003, 0x003) AM_READ(cx005_port_d_digital_r)
    AM_RANGE(0x004, 0x004) AM_WRITE(cx005_port_a_ddr_w)
    AM_RANGE(0x005, 0x005) AM_WRITE(cx005_port_b_ddr_w)
    AM_RANGE(0x006, 0x006) AM_WRITE(cx005_port_c_ddr_w)
    AM_RANGE(0x007, 0x007) AM_READ(cx005_port_d_analog_r)
    AM_RANGE(0x008, 0x008) AM_READWRITE(cx005_timer_data_r, cx005_timer_data_w)
    AM_RANGE(0x008, 0x008) AM_READWRITE(cx005_timer_ctrl_r, cx005_timer_ctrl_w)*/
	AM_RANGE(0x00a, 0x01f) AM_NOP // Not Used
	AM_RANGE(0x020, 0x07f) AM_RAM // Internal RAM
	AM_RANGE(0x080, 0xf7f) AM_ROM // Internal ROM
	AM_RANGE(0xf80, 0xff7) AM_ROM // Self-Test
	AM_RANGE(0xff8, 0xfff) AM_ROM // Interrupt Vectors
ADDRESS_MAP_END

static ADDRESS_MAP_START( pl80_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x00) AM_WRITE(pl80_port_a_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(pl80_port_b_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(pl80_port_c_w)
	AM_RANGE(0x03, 0x03) AM_READ(pl80_port_d_r)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( comxpl80 )
	PORT_START("SW")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("DOWN")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("PEN-SEL")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("UP")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("CR")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("ON LINE")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("PE")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("RIGHT")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("LEFT")

	PORT_START("FONT")
	PORT_CONFNAME( 0x03, 0x00, "COMX PL-80 Font Pack")
	PORT_CONFSETTING( 0x00, DEF_STR( None ) )
	PORT_CONFSETTING( 0x01, "Italic, Emphasized and Outline" )
	PORT_CONFSETTING( 0x02, "Tiny" )
INPUT_PORTS_END

/* Machine Driver */

static MACHINE_DRIVER_START( comxpl80 )
	MDRV_DRIVER_DATA(comxpl80_state)

	// basic system hardware

	MDRV_CPU_ADD("maincpu", M6805, 4000000) // CX005: some kind of MC6805/MC68HC05 clone
	MDRV_CPU_PROGRAM_MAP(pl80_map)
	MDRV_CPU_IO_MAP(pl80_io_map)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( comxpl80 )
	ROM_REGION( 0x1000, "maincpu", 0 )
	ROM_LOAD( "pl80.pt6",		0x0080, 0x0e00, CRC(ae059e5b) SHA1(f25812606b0082d32eb603d0a702a2187089d332) )

	ROM_REGION( 0x6000, "gfx1", ROMREGION_ERASEFF ) // Plotter fonts
	ROM_LOAD( "it.em.ou.bin",	0x2000, 0x2000, CRC(1b4a3198) SHA1(138ff6666a31c2d18cd63e609dd94d9cd1529931) )
	ROM_LOAD( "tiny.bin",		0x4000, 0x0400, CRC(940ec1ed) SHA1(ad83a3b57e2f0fbaa1e40644cd999b3f239635e8) )
ROM_END

/* System Drivers */

//    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT     INIT  COMPANY                         FULLNAME        FLAGS
COMP( 1987, comxpl80,	0,		0,		comxpl80,	comxpl80, 0,	"Comx World Operations Ltd",	"COMX PL-80",	GAME_NOT_WORKING | GAME_NO_SOUND )
