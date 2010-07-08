/***************************************************************************

    Explorer 85

    12/05/2009 Skeleton driver.

****************************************************************************/

/*

    TODO:

    - needs a terminal, or a dump of the hexadecimal keyboard monitor ROM
    - serial input/output at SID/SOD pins
    - disable ROM mirror after boot
    - RAM expansions

*/

#include "emu.h"
#include "includes/exp85.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/i8155.h"
#include "machine/i8355.h"
#include "sound/speaker.h"
#include "devices/messram.h"

/* Memory Maps */

static ADDRESS_MAP_START( exp85_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_ROMBANK("bank1")
	AM_RANGE(0xc000, 0xdfff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_ROM
	AM_RANGE(0xf800, 0xf8ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( exp85_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xf0, 0xf3) AM_DEVREADWRITE(I8355_TAG, i8355_r, i8355_w)
	AM_RANGE(0xf8, 0xfd) AM_DEVREADWRITE(I8155_TAG, i8155_r, i8155_w)
//  AM_RANGE(0xfe, 0xff) AM_DEVREADWRITE(I8279_TAG, i8279_r, i8279_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_CHANGED( trigger_reset )
{
	cputag_set_input_line(field->port->machine, I8085A_TAG, INPUT_LINE_RESET, newval ? CLEAR_LINE : ASSERT_LINE);
}

static INPUT_CHANGED( trigger_rst75 )
{
	cputag_set_input_line(field->port->machine, I8085A_TAG, I8085_RST75_LINE, newval ? CLEAR_LINE : ASSERT_LINE);
}

static INPUT_PORTS_START( exp85 )
	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("R") PORT_CODE(KEYCODE_F1) PORT_CHANGED(trigger_reset, 0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("I") PORT_CODE(KEYCODE_F2) PORT_CHANGED(trigger_rst75, 0)
INPUT_PORTS_END

/* Video */

static VIDEO_START( exp85 )
{
}

static VIDEO_UPDATE( exp85 )
{
    return 0;
}

/* 8155 Interface */

static I8155_INTERFACE( i8155_intf )
{
	DEVCB_NULL,	/* port A read */
	DEVCB_NULL,	/* port B read */
	DEVCB_NULL,	/* port C read */
	DEVCB_NULL,	/* port A write */
	DEVCB_NULL,	/* port B write */
	DEVCB_NULL,	/* port C write */
	DEVCB_NULL	/* timer output */
};

/* 8355 Interface */

static READ8_DEVICE_HANDLER( i8355_a_r )
{
	/*

        bit     description

        PA0     tape control
        PA1     jumper S17 (open=+5V closed=GND)
        PA2     J5:13
        PA3
        PA4     J2:22
        PA5
        PA6
        PA7     speaker output

    */

	return 0x02;
}

static WRITE8_DEVICE_HANDLER( i8355_a_w )
{
	/*

        bit     description

        PA0     tape control
        PA1     jumper S17 (open=+5V closed=GND)
        PA2     J5:13
        PA3
        PA4     J2:22
        PA5
        PA6
        PA7     speaker output

    */

	exp85_state *state = (exp85_state *)device->machine->driver_data;

	/* tape control */
	state->tape_control = BIT(data, 0);

	/* speaker output */
	speaker_level_w(state->speaker, !BIT(data, 7));
}

static I8355_INTERFACE( i8355_intf )
{
	DEVCB_HANDLER(i8355_a_r),	/* port A read */
	DEVCB_NULL,					/* port B read */
	DEVCB_HANDLER(i8355_a_w),	/* port A write */
	DEVCB_NULL,					/* port B write */
};

/* I8085A Interface */

static WRITE_LINE_DEVICE_HANDLER( exp85_sod_w )
{
	exp85_state *driver_state = (exp85_state *)device->machine->driver_data;

	cassette_output(driver_state->cassette, state ? -1.0 : +1.0);
}

static READ_LINE_DEVICE_HANDLER( exp85_sid_r )
{
	exp85_state *driver_state = (exp85_state *)device->machine->driver_data;

	int data = 1;

	if (driver_state->tape_control)
	{
		data = cassette_input(driver_state->cassette) > +1.0;
	}

	return data;
}

static I8085_CONFIG( exp85_i8085_config )
{
	DEVCB_NULL,					/* STATUS changed callback */
	DEVCB_NULL,					/* INTE changed callback */
	DEVCB_LINE(exp85_sid_r),	/* SID changed callback (I8085A only) */
	DEVCB_LINE(exp85_sod_w)		/* SOD changed callback (I8085A only) */
};

/* Machine Initialization */

static MACHINE_START( exp85 )
{
	exp85_state *state = (exp85_state *)machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, I8085A_TAG, ADDRESS_SPACE_PROGRAM);

	/* setup memory banking */
	memory_install_read_bank(program, 0x0000, 0x07ff, 0, 0, "bank1");
	memory_unmap_write(program, 0x0000, 0x07ff, 0, 0 );
	memory_configure_bank(machine, "bank1", 0, 1, memory_region(machine, I8085A_TAG) + 0xf000, 0);
	memory_configure_bank(machine, "bank1", 1, 1, memory_region(machine, I8085A_TAG), 0);
	memory_set_bank(machine, "bank1", 0);

	/* find devices */
	state->speaker = devtag_get_device(machine, SPEAKER_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);
}

/* Machine Driver */

static const cassette_config exp85_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};

static MACHINE_DRIVER_START( exp85 )
	MDRV_DRIVER_DATA(exp85_state)

    /* basic machine hardware */
    MDRV_CPU_ADD(I8085A_TAG, I8085A, XTAL_6_144MHz)
    MDRV_CPU_PROGRAM_MAP(exp85_mem)
    MDRV_CPU_IO_MAP(exp85_io)
	MDRV_CPU_CONFIG(exp85_i8085_config)

    MDRV_MACHINE_START(exp85)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)

	MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(exp85)
    MDRV_VIDEO_UPDATE(exp85)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER_TAG, SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_I8155_ADD(I8155_TAG, XTAL_6_144MHz/2, i8155_intf)
	MDRV_I8355_ADD(I8355_TAG, XTAL_6_144MHz/2, i8355_intf)

	MDRV_CASSETTE_ADD(CASSETTE_TAG, exp85_cassette_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("256")
	MDRV_RAM_EXTRA_OPTIONS("4K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( exp85 )
    ROM_REGION( 0x10000, I8085A_TAG, 0 )
	ROM_DEFAULT_BIOS("eia")
	ROM_LOAD( "c000.bin", 0xc000, 0x0800, CRC(73ce4aad) SHA1(2c69cd0b6c4bdc92f4640bce18467e4e99255bab) )
	ROM_LOAD( "c800.bin", 0xc800, 0x0800, CRC(eb3fdedc) SHA1(af92d07f7cb7533841b16e1176401363176857e1) )
	ROM_LOAD( "d000.bin", 0xd000, 0x0800, CRC(c10c4a22) SHA1(30588ba0b27a775d85f8c581ad54400c8521225d) )
	ROM_LOAD( "d800.bin", 0xd800, 0x0800, CRC(dfa43ef4) SHA1(56a7e7a64928bdd1d5f0519023d1594cacef49b3) )
	ROM_SYSTEM_BIOS( 0, "eia", "EIA Terminal" )
	ROMX_LOAD( "eia.u105", 0xf000, 0x0800, CRC(1a99d0d9) SHA1(57b6d48e71257bc4ef2d3dddc9b30edf6c1db766), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "hex", "Hex Keyboard" )
	ROMX_LOAD( "hex.u105", 0xf000, 0x0800, NO_DUMP, ROM_BIOS(2) )

	ROM_REGION( 0x800, I8355_TAG, ROMREGION_ERASE00 )
/*  ROM_DEFAULT_BIOS("terminal")
    ROM_SYSTEM_BIOS( 0, "terminal", "Terminal" )
    ROMX_LOAD( "eia.u105", 0xf000, 0x0800, CRC(1a99d0d9) SHA1(57b6d48e71257bc4ef2d3dddc9b30edf6c1db766), ROM_BIOS(1) )
    ROM_SYSTEM_BIOS( 1, "hexkbd", "Hex Keyboard" )
    ROMX_LOAD( "hex.u105", 0xf000, 0x0800, NO_DUMP, ROM_BIOS(2) )*/
ROM_END

/* System Drivers */
/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY         FULLNAME        FLAGS */
COMP( 1979, exp85,  0,		0,		exp85,	exp85,	0,		"Netronics",	"Explorer/85",	GAME_NOT_WORKING )
