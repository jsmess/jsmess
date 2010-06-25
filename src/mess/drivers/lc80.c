/***************************************************************************

    LC-80

    12/05/2009 Skeleton driver.

****************************************************************************/

/*

    TODO:

    - HALT led
    - KSD11 switch
    - banking for ROM 4-5
    - Schachcomputer SC-80
    - CTC clock inputs

*/

#include "emu.h"
#include "includes/lc80.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "devices/cassette.h"
#include "machine/z80pio.h"
#include "machine/z80ctc.h"
#include "sound/speaker.h"
#include "devices/messram.h"
#include "lc80.lh"

/* Memory Maps */

static ADDRESS_MAP_START( lc80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x3fff)
	AM_RANGE(0x0000, 0x07ff) AM_ROMBANK("bank1")
	AM_RANGE(0x0800, 0x0fff) AM_ROMBANK("bank2")
	AM_RANGE(0x1000, 0x17ff) AM_ROMBANK("bank3")
	AM_RANGE(0x2000, 0x23ff) AM_RAM
	AM_RANGE(0x2400, 0x2fff) AM_RAMBANK("bank4")
ADDRESS_MAP_END

static ADDRESS_MAP_START( sc80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_IMPORT_FROM(lc80_mem)
	AM_RANGE(0xc000, 0xcfff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( lc80_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1f)
	AM_RANGE(0xf4, 0xf7) AM_DEVREADWRITE(Z80PIO1_TAG, z80pio_cd_ba_r, z80pio_cd_ba_w)
	AM_RANGE(0xf8, 0xfb) AM_DEVREADWRITE(Z80PIO2_TAG, z80pio_cd_ba_r, z80pio_cd_ba_w)
	AM_RANGE(0xec, 0xef) AM_DEVREADWRITE(Z80CTC_TAG, z80ctc_r, z80ctc_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_CHANGED( trigger_reset )
{
	cputag_set_input_line(field->port->machine, Z80_TAG, INPUT_LINE_RESET, newval ? CLEAR_LINE : ASSERT_LINE);
}

static INPUT_CHANGED( trigger_nmi )
{
	cputag_set_input_line(field->port->machine, Z80_TAG, INPUT_LINE_NMI, newval ? CLEAR_LINE : ASSERT_LINE);
}

static INPUT_PORTS_START( lc80 )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("-") PORT_CODE(KEYCODE_DOWN)

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("LD") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("+") PORT_CODE(KEYCODE_UP)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("ST") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("DAT") PORT_CODE(KEYCODE_T)

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("EX") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("ADR") PORT_CODE(KEYCODE_R)

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("RES") PORT_CODE(KEYCODE_F10) PORT_CHANGED(trigger_reset, 0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("NMI") PORT_CODE(KEYCODE_ESC) PORT_CHANGED(trigger_nmi, 0)
INPUT_PORTS_END

/* Z80-CTC Interface */

static WRITE_LINE_DEVICE_HANDLER( ctc_z0_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z1_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z2_w )
{
}

static Z80CTC_INTERFACE( ctc_intf )
{
	0,              	/* timer disables */
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_LINE(ctc_z0_w),			/* ZC/TO0 callback */
	DEVCB_LINE(ctc_z1_w),			/* ZC/TO1 callback */
	DEVCB_LINE(ctc_z2_w)    		/* ZC/TO2 callback */
};

/* Z80-PIO Interface */

static void update_display(lc80_state *state)
{
	int i;

	for (i = 0; i < 6; i++)
	{
		if (!BIT(state->digit, i)) output_set_digit_value(5 - i, state->segment);
	}
}

static WRITE8_DEVICE_HANDLER( pio1_port_a_w )
{
	/*

        bit     description

        PA0     VQE23 segment B
        PA1     VQE23 segment F
        PA2     VQE23 segment A
        PA3     VQE23 segment G
        PA4     VQE23 segment DP
        PA5     VQE23 segment C
        PA6     VQE23 segment E
        PA7     VQE23 segment D

    */

	lc80_state *state = (lc80_state *)device->machine->driver_data;

	state->segment = BITSWAP8(~data, 4, 3, 1, 6, 7, 5, 0, 2);

	update_display(state);
}

static READ8_DEVICE_HANDLER( pio1_port_b_r )
{
	/*

        bit     description

        PB0     tape input
        PB1     tape output
        PB2     digit 0
        PB3     digit 1
        PB4     digit 2
        PB5     digit 3
        PB6     digit 4
        PB7     digit 5

    */

	lc80_state *state = (lc80_state *)device->machine->driver_data;

	return (cassette_input(state->cassette) < +0.0);
}

static WRITE8_DEVICE_HANDLER( pio1_port_b_w )
{
	/*

        bit     description

        PB0     tape input
        PB1     tape output, speaker output, OUT led
        PB2     digit 0
        PB3     digit 1
        PB4     digit 2
        PB5     digit 3
        PB6     digit 4
        PB7     digit 5

    */

	lc80_state *state = (lc80_state *)device->machine->driver_data;

	/* tape output */
	cassette_output(state->cassette, BIT(data, 1) ? +1.0 : -1.0);

	/* speaker */
	speaker_level_w(state->speaker, !BIT(data, 1));

	/* OUT led */
	output_set_led_value(0, !BIT(data, 1));

	/* keyboard */
	state->digit = data >> 2;

	/* display */
	update_display(state);
}

static Z80PIO_INTERFACE( pio1_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,						/* port A read callback */
	DEVCB_HANDLER(pio1_port_a_w),	/* port A write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_HANDLER(pio1_port_b_r),	/* port B read callback */
	DEVCB_HANDLER(pio1_port_b_w),	/* port B write callback */
	DEVCB_NULL						/* portB ready active callback */
};

static READ8_DEVICE_HANDLER( pio2_port_b_r )
{
	/*

        bit     description

        PB0
        PB1
        PB2
        PB3
        PB4     key row 0
        PB5     key row 1
        PB6     key row 2
        PB7     key row 3

    */

	lc80_state *state = (lc80_state *)device->machine->driver_data;
	UINT8 data = 0xf0;
	int i;

	for (i = 0; i < 6; i++)
	{
		if (!BIT(state->digit, i))
		{
			if (!BIT(input_port_read(device->machine, "ROW0"), i)) data &= ~0x10;
			if (!BIT(input_port_read(device->machine, "ROW1"), i)) data &= ~0x20;
			if (!BIT(input_port_read(device->machine, "ROW2"), i)) data &= ~0x40;
			if (!BIT(input_port_read(device->machine, "ROW3"), i)) data &= ~0x80;
		}
	}

	return data;
}

static Z80PIO_INTERFACE( pio2_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,						/* port A read callback */
	DEVCB_NULL,						/* port A write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_HANDLER(pio2_port_b_r),	/* port B read callback */
	DEVCB_NULL,						/* port B write callback */
	DEVCB_NULL						/* portB ready active callback */
};

/* Z80 Daisy Chain */

static const z80_daisy_config lc80_daisy_chain[] =
{
	{ Z80CTC_TAG },
	{ Z80PIO2_TAG },
	{ Z80PIO1_TAG },
	{ NULL }
};

/* Machine Initialization */

static MACHINE_START( lc80 )
{
	lc80_state *state = (lc80_state *)machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->z80pio2 = devtag_get_device(machine, Z80PIO2_TAG);
	state->speaker = devtag_get_device(machine, SPEAKER_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* setup memory banking */
	memory_configure_bank(machine, "bank1", 0, 1, memory_region(machine, Z80_TAG), 0); // TODO
	memory_configure_bank(machine, "bank1", 1, 1, memory_region(machine, Z80_TAG), 0);
	memory_set_bank(machine, "bank1", 1);

	memory_configure_bank(machine, "bank2", 0, 1, memory_region(machine, Z80_TAG) + 0x800, 0); // TODO
	memory_configure_bank(machine, "bank2", 1, 1, memory_region(machine, Z80_TAG) + 0x800, 0);
	memory_set_bank(machine, "bank2", 1);

	memory_configure_bank(machine, "bank3", 0, 1, memory_region(machine, Z80_TAG) + 0x1000, 0); // TODO
	memory_configure_bank(machine, "bank3", 1, 1, memory_region(machine, Z80_TAG) + 0x1000, 0);
	memory_set_bank(machine, "bank3", 1);

	memory_configure_bank(machine, "bank4", 0, 1, memory_region(machine, Z80_TAG) + 0x2000, 0);
	memory_set_bank(machine, "bank4", 0);

	memory_install_readwrite_bank(program, 0x0000, 0x07ff, 0, 0, "bank1");
	memory_install_readwrite_bank(program, 0x0800, 0x0fff, 0, 0, "bank2");
	memory_install_readwrite_bank(program, 0x1000, 0x17ff, 0, 0, "bank3");

	switch (messram_get_size(devtag_get_device(machine, "messram")))
	{
	case 1*1024:
		memory_install_readwrite_bank(program, 0x2000, 0x23ff, 0, 0, "bank4");
		memory_unmap_readwrite(program, 0x2400, 0x2fff, 0, 0);
		break;

	case 2*1024:
		memory_install_readwrite_bank(program, 0x2000, 0x27ff, 0, 0, "bank4");
		memory_unmap_readwrite(program, 0x2800, 0x2fff, 0, 0);
		break;

	case 3*1024:
		memory_install_readwrite_bank(program, 0x2000, 0x2bff, 0, 0, "bank4");
		memory_unmap_readwrite(program, 0x2c00, 0x2fff, 0, 0);
		break;

	case 4*1024:
		memory_install_readwrite_bank(program, 0x2000, 0x2fff, 0, 0, "bank4");
		break;
	}

	/* register for state saving */
	state_save_register_global(machine, state->digit);
	state_save_register_global(machine, state->segment);
}

/* Machine Driver */

static const cassette_config lc80_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED)
};

static MACHINE_DRIVER_START( lc80 )
	MDRV_DRIVER_DATA(lc80_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, 900000) /* UD880D */
    MDRV_CPU_PROGRAM_MAP(lc80_mem)
    MDRV_CPU_IO_MAP(lc80_io)

    MDRV_MACHINE_START(lc80)

	/* video hardware */
	MDRV_DEFAULT_LAYOUT( layout_lc80 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER_TAG, SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_Z80CTC_ADD(Z80CTC_TAG, 900000, ctc_intf)
	MDRV_Z80PIO_ADD(Z80PIO1_TAG, 900000, pio1_intf)
	MDRV_Z80PIO_ADD(Z80PIO2_TAG, 900000, pio2_intf)

	MDRV_CASSETTE_ADD(CASSETTE_TAG, lc80_cassette_config)

	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1K")
	MDRV_RAM_EXTRA_OPTIONS("2K,3K,4K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( lc80_2 )
	MDRV_DRIVER_DATA(lc80_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, 1800000) /* UD880D */
    MDRV_CPU_PROGRAM_MAP(lc80_mem)
    MDRV_CPU_IO_MAP(lc80_io)

    MDRV_MACHINE_START(lc80)

	/* video hardware */
	MDRV_DEFAULT_LAYOUT( layout_lc80 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER_TAG, SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_Z80CTC_ADD(Z80CTC_TAG, 900000, ctc_intf)
	MDRV_Z80PIO_ADD(Z80PIO1_TAG, 900000, pio1_intf)
	MDRV_Z80PIO_ADD(Z80PIO2_TAG, 900000, pio2_intf)

	MDRV_CASSETTE_ADD(CASSETTE_TAG, lc80_cassette_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("4K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sc80 )
	MDRV_IMPORT_FROM( lc80_2 )

	/* basic machine hardware */
    MDRV_CPU_MODIFY(Z80_TAG)
    MDRV_CPU_PROGRAM_MAP(sc80_mem)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( lc80 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "u505", "LC 80 (2x U505)" )
	ROMX_LOAD( "lc80.d202", 0x0000, 0x0400, CRC(e754ef53) SHA1(044440b13e62addbc3f6a77369cfd16f99b39752), ROM_BIOS(1) )
	ROMX_LOAD( "lc80.d203", 0x0800, 0x0400, CRC(2b544da1) SHA1(3a6cbd0c57c38eadb7055dca4b396c348567d1d5), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "2716", "LC 80 (2716)" )
	ROMX_LOAD( "lc80_2716.bin", 0x0000, 0x0800, CRC(b3025934) SHA1(6fff953f0f1eee829fd774366313ab7e8053468c), ROM_BIOS(2))
ROM_END

ROM_START( lc80_2 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "lc80_2.bin", 0x0000, 0x1000, CRC(2e06d768) SHA1(d9cddaf847831e4ab21854c0f895348b7fda20b8) )
ROM_END

ROM_START( sc80 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "lc80e-0000-schach.rom", 0x0000, 0x1000, CRC(e3cca61d) SHA1(f2be3f2a9d3780d59657e49b3abeffb0fc13db89) )
	ROM_LOAD( "lc80e-1000-schach.rom", 0x1000, 0x1000, CRC(b0323160) SHA1(0ea019b0944736ae5b842bf9aa3537300f259b98) )
	ROM_LOAD( "lc80e-c000-schach.rom", 0xc000, 0x1000, CRC(9c858d9c) SHA1(2f7b3fd046c965185606253f6cd9372da289ca6f) )
ROM_END

/* System Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY                 FULLNAME                FLAGS */
COMP( 1984, lc80,	0,		0,		lc80,	lc80,	0,		"VEB Mikroelektronik",	"Lerncomputer LC 80",	GAME_SUPPORTS_SAVE )
COMP( 1984, lc80_2,	lc80,	0,		lc80_2,	lc80,	0,		"VEB Mikroelektronik",	"Lerncomputer LC 80.2",	GAME_SUPPORTS_SAVE )
COMP( 1984, sc80,	lc80,	0,		lc80_2,	lc80,	0,		"VEB Mikroelektronik",	"Schachcomputer SC-80",	GAME_SUPPORTS_SAVE )
