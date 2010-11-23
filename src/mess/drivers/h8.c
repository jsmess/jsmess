/***************************************************************************

        Heathkit H8

        12/05/2009 Skeleton driver.

	STATUS:
	- It runs, keyboard works, you can enter data

	TODO:
	- Proper artwork
	- Add status LEDs
	- Add extra interrupt stuff
	- Assign more appropriate keys
	- Add load/dump facility (cassette port)

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "sound/beep.h"
#include "h8.lh"

#define H8_CLOCK (XTAL_12_288MHz / 6)
#define H8_BEEP_FRQ (H8_CLOCK / 1024)
#define H8_IRQ_PULSE (H8_BEEP_FRQ / 2)

static UINT8 h8_digit;
static UINT8 h8_segment;
static running_device *h8_beeper;

static TIMER_DEVICE_CALLBACK( h8_irq_pulse )
{
	cpu_set_input_line_and_vector(timer.machine->device("maincpu"), INPUT_LINE_IRQ0, ASSERT_LINE, 0xcf);
}

static READ8_HANDLER( h8_f0_r )
{
    // reads the keyboard
	UINT8 i,keyin,data = 0xff;

	keyin = input_port_read(space->machine, "X0");
	if (keyin != 0xff)
	{
		for (i = 1; i < 8; i++)
			if (!BIT(keyin,i))
				data &= ~(i<<1);
		data &= 0xfe;
	}

	keyin = input_port_read(space->machine, "X1");
	if (keyin != 0xff)
	{
		for (i = 1; i < 8; i++)
			if (!BIT(keyin,i))
				data &= ~(i<<5);
		data &= 0xef;
	}
	return data;
}

static WRITE8_HANDLER( h8_f0_w )
{
    // this will always turn off int10 that was set by the timer
    // d0-d3 = digit select
    // d4 = int20 (hi)
    // d5 = mon LED (lo)
    // d6 = int10 (lo)
    // d7 = beeper enable (lo)

	h8_digit = data & 15;
	if (h8_digit) output_set_digit_value(h8_digit, h8_segment);
	//if (~data & 0x40) h8_irqset(space->machine, 160, 0xcf);
	//if (data & 0x10) h8_irqset(space->machine, 160, 0xd7);

	cpu_set_input_line(space->machine->device("maincpu"), INPUT_LINE_IRQ0, CLEAR_LINE);
	beep_set_state(h8_beeper, (data & 0x80) ? 0 : 1);
}

static WRITE8_HANDLER( h8_f1_w )
{
    //d7 segment dot
    //d6 segment f
    //d5 segment e
    //d4 segment d
    //d3 segment c
    //d2 segment b
    //d1 segment a
    //d0 segment g

	h8_segment = 0xff ^ BITSWAP8(data, 7, 0, 6, 5, 4, 3, 2, 1);
	if (h8_digit) output_set_digit_value(h8_digit, h8_segment);
}

static ADDRESS_MAP_START(h8_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x03ff) AM_ROM
	AM_RANGE(0x2000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( h8_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf0, 0xf0) AM_READWRITE(h8_f0_r,h8_f0_w)
	AM_RANGE(0xf1, 0xf1) AM_WRITE(h8_f1_w)
	//AM_RANGE(0xf8, 0xf8) load and dump data port
	//AM_RANGE(0xf9, 0xf9) load and dump control port
	//AM_RANGE(0xfa, 0xfa) console data port
	//AM_RANGE(0xfb, 0xfb) console control port
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( h8 )
	PORT_START("X0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')

	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('+')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('-')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('/')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('#')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('@')
INPUT_PORTS_END


static MACHINE_RESET(h8)
{
	h8_beeper = machine->device("beep");
	beep_set_frequency(h8_beeper, H8_BEEP_FRQ);
}

static WRITE_LINE_DEVICE_HANDLER( h8_inte_callback )
{
        // operate the ION LED
}

static WRITE8_DEVICE_HANDLER( h8_status_callback )
{
	if (data & I8085_STATUS_INTA)
	{
        // This is when the vector is presented to the CPU
	}
	if (data & I8085_STATUS_M1)
	{
        // operate the RUN LED
	}
}

static I8085_CONFIG( h8_cpu_config )
{
	DEVCB_HANDLER(h8_status_callback),		/* Status changed callback */
	DEVCB_LINE(h8_inte_callback),			/* INTE changed callback */
	DEVCB_NULL,					/* SID changed callback (I8085A only) */
	DEVCB_NULL					/* SOD changed callback (I8085A only) */
};

static MACHINE_CONFIG_START( h8, driver_device )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I8080, H8_CLOCK)
	MDRV_CPU_PROGRAM_MAP(h8_mem)
	MDRV_CPU_IO_MAP(h8_io)
	MDRV_CPU_CONFIG(h8_cpu_config)

	MDRV_MACHINE_RESET(h8)
	MDRV_TIMER_ADD_PERIODIC("h8_timer", h8_irq_pulse, HZ(H8_IRQ_PULSE) )

	/* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_h8)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( h8 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "2708_444-13_pam8.rom", 0x0000, 0x0400, CRC(e0745513) SHA1(0e170077b6086be4e5cd10c17e012c0647688c39))
	ROM_REGION( 0x10000, "otherroms", ROMREGION_ERASEFF )
	ROM_LOAD( "2708_444-13_pam8go.rom", 0x0000, 0x0400, CRC(9dbad129) SHA1(72421102b881706877f50537625fc2ab0b507752))
	ROM_LOAD( "2716_444-13_pam8at.rom", 0x0000, 0x0800, CRC(fd95ddc1) SHA1(eb1f272439877239f745521139402f654e5403af))

	ROM_LOAD( "2716_444-19_h17.rom", 0x0000, 0x0800, CRC(26e80ae3) SHA1(0c0ee95d7cb1a760f924769e10c0db1678f2435c))

	ROM_LOAD( "2732_444-70_xcon8.rom", 0x0000, 0x1000, CRC(b04368f4) SHA1(965244277a3a8039a987e4c3593b52196e39b7e7))
	ROM_LOAD( "2732_444-140_pam37.rom", 0x0000, 0x1000, CRC(53a540db) SHA1(90082d02ffb1d27e8172b11fff465bd24343486e))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1977, h8,  0,       0,	h8, 	h8, 	 0, "Heath, Inc.", "Heathkit H8", GAME_NOT_WORKING )
