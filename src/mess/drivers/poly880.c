/***************************************************************************

    Poly-Computer 880

    12/05/2009 Skeleton driver.

	http://www.kc85-museum.de/books/poly880/index.html

****************************************************************************/

#include "driver.h"
#include "includes/poly880.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "devices/cassette.h"
#include "machine/z80pio.h"
#include "machine/z80ctc.h"
#include "poly880.lh"

/*

	TODO:

	- display
	- keyboard
	- tape input
	- layout LEDs
	- RAM expansion

*/

static const device_config *cassette_device_image(running_machine *machine)
{
	return devtag_get_device(machine, CASSETTE_TAG);
}

/* Memory Maps */

static ADDRESS_MAP_START( poly880_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_RAM
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( poly880_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x80, 0x83) AM_DEVREADWRITE(Z80PIO1_TAG, z80pio_alt_r, z80pio_alt_w)
	AM_RANGE(0x84, 0x87) AM_DEVREADWRITE(Z80PIO2_TAG, z80pio_alt_r, z80pio_alt_w)
	AM_RANGE(0x88, 0x8b) AM_DEVREADWRITE(Z80CTC_TAG, z80ctc_r, z80ctc_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( poly880 )
	PORT_START("KI1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("KI2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("KI3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
INPUT_PORTS_END

/* Z80-CTC Interface */

static void z80daisy_interrupt(const device_config *device, int state)
{
	cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_IRQ0, state);
}

static WRITE8_DEVICE_HANDLER( ctc_z0_w )
{
	// SEND
}

static WRITE8_DEVICE_HANDLER( ctc_z1_w )
{
}

static WRITE8_DEVICE_HANDLER( ctc_z2_w )
{
	z80ctc_trg_w(device, 3, data);
}

static const z80ctc_interface ctc_intf =
{
	0,              	/* timer disables */
	z80daisy_interrupt,	/* interrupt handler */
	ctc_z0_w,			/* ZC/TO0 callback */
	ctc_z1_w,			/* ZC/TO1 callback */
	ctc_z2_w    		/* ZC/TO2 callback */
};

/* Z80-PIO Interface */

static WRITE8_DEVICE_HANDLER( pio1_port_a_w )
{
	/*
		
		bit		signal	description

		PA0		SD0
		PA1		SD1
		PA2		SD2
		PA3		SD3
		PA4		SD4
		PA5		SD5
		PA6		SD6
		PA7		SD7

	*/
}

static READ8_DEVICE_HANDLER( pio1_port_b_r )
{
	/*
		
		bit		signal	description

		PB0		TTY
		PB1		MIN
		PB2		MOUT	tape output
		PB3		
		PB4		KI1		key row 1 input
		PB5		KI2		key row 2 input
		PB6		SCON
		PB7		KI3		key row 3 input

	*/

	return 0xff;
}

static WRITE8_DEVICE_HANDLER( pio1_port_b_w )
{
	/*
		
		bit		signal	description

		PB0		TTY
		PB1		MIN
		PB2		MOUT	tape output
		PB3		
		PB4		KI1		key row 1 input
		PB5		KI2		key row 2 input
		PB6		SCON
		PB7		KI3		key row 3 input

	*/

	/* tape output */
	cassette_output(cassette_device_image(device->machine), BIT(data, 2) ? +1.0 : -1.0);
}

static const z80pio_interface pio1_intf =
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,						/* port A read callback */
	DEVCB_HANDLER(pio1_port_b_r),	/* port B read callback */
	DEVCB_HANDLER(pio1_port_a_w),	/* port A write callback */
	DEVCB_HANDLER(pio1_port_b_w),	/* port B write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_NULL						/* portB ready active callback */
};

static const z80pio_interface pio2_intf =
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,						/* port A read callback */
	DEVCB_NULL,						/* port B read callback */
	DEVCB_NULL,						/* port A write callback */
	DEVCB_NULL,						/* port B write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_NULL						/* portB ready active callback */
};

/* Z80 Daisy Chain */

static const z80_daisy_chain poly880_daisy_chain[] =
{
	{ Z80PIO1_TAG },
	{ Z80PIO2_TAG },
	{ Z80CTC_TAG },
	{ NULL }
};

/* Machine Initialization */

static MACHINE_START( poly880 )
{
//	poly880_state *state = machine->driver_data;

	/* find devices */
//	state->z80pio2 = devtag_get_device(machine, Z80PIO2_TAG);

	/* register for state saving */
//	state_save_register_global(machine, state->);
}

/* Machine Driver */

static const cassette_config poly880_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED
};

static MACHINE_DRIVER_START( poly880 )
	MDRV_DRIVER_DATA(poly880_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_7_3728MHz/8)
    MDRV_CPU_PROGRAM_MAP(poly880_mem)
    MDRV_CPU_IO_MAP(poly880_io)

    MDRV_MACHINE_START(poly880)

	/* video hardware */
	MDRV_DEFAULT_LAYOUT( layout_poly880 )

	/* devices */
	MDRV_Z80CTC_ADD(Z80CTC_TAG, XTAL_7_3728MHz/16, ctc_intf)
	MDRV_Z80PIO_ADD(Z80PIO1_TAG, pio1_intf)
	MDRV_Z80PIO_ADD(Z80PIO2_TAG, pio2_intf)

	MDRV_CASSETTE_ADD(CASSETTE_TAG, poly880_cassette_config)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( poly880 )
	ROM_REGION( 0x10000, Z80_TAG, ROMREGION_ERASEFF )
	ROM_LOAD( "poly880.i5", 0x0000, 0x0400, CRC(b1c571e8) SHA1(85bfe53d39d6690e79999a1e1240789497e72db0) )
	ROM_LOAD( "poly880.i6", 0x1000, 0x0400, CRC(9efddf5b) SHA1(6ffa2f80b2c6f8ec9e22834f739c82f9754272b8) )
ROM_END

/* System Configuration */

static SYSTEM_CONFIG_START( poly880 )
	CONFIG_RAM_DEFAULT( 1 * 1024 )
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY				FULLNAME				FLAGS */
COMP( 1983, poly880,	0,		0,		poly880,	poly880,	0,		poly880,	"VEB Polytechnik",	"Poly-Computer 880",	GAME_NOT_WORKING)
