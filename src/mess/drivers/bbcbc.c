/***************************************************************************

  bbcbc.c

  BBC Bridge Companion

  Inputs hooked up - 2009-03-14 - Robbbert
  Clock Freq added - 2009-05-18 - incog

***************************************************************************/
#include "emu.h"
#include "cpu/z80/z80.h"
#include "video/tms9928a.h"
#include "machine/z80pio.h"
#include "cpu/z80/z80daisy.h"
#include "devices/cartslot.h"

#define MAIN_CLOCK XTAL_4_433619MHz


static ADDRESS_MAP_START( bbcbc_prg, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_ROM
	AM_RANGE(0x04000, 0x0bfff) AM_ROM
	AM_RANGE(0x0e000, 0x0e02f) AM_RAM
	AM_RANGE(0x0e030, 0x0e030) AM_READ_PORT("LINE01")
	AM_RANGE(0x0e031, 0x0e031) AM_READ_PORT("LINE02")
	AM_RANGE(0x0e032, 0x0e032) AM_READ_PORT("LINE03")
	AM_RANGE(0x0e033, 0x0e033) AM_READ_PORT("LINE04")
	AM_RANGE(0x0e034, 0x0e034) AM_READ_PORT("LINE05")
	AM_RANGE(0x0e035, 0x0e035) AM_READ_PORT("LINE06")
	AM_RANGE(0x0e036, 0x0e036) AM_READ_PORT("LINE07")
	AM_RANGE(0x0e037, 0x0e037) AM_READ_PORT("LINE08")
	AM_RANGE(0x0e038, 0x0e038) AM_READ_PORT("LINE09")
	AM_RANGE(0x0e039, 0x0e039) AM_READ_PORT("LINE10")
	AM_RANGE(0x0e03a, 0x0e03a) AM_READ_PORT("LINE11")
	AM_RANGE(0x0e03b, 0x0e03b) AM_READ_PORT("LINE12")
	AM_RANGE(0x0e03c, 0x0e7ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( bbcbc_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x40, 0x43) AM_DEVREADWRITE("z80pio", z80pio_cd_ba_r, z80pio_cd_ba_w)
	AM_RANGE(0x80, 0x80) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0x81, 0x81) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( bbcbc )
	PORT_START("LINE01")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Pass") PORT_CODE(KEYCODE_W) PORT_IMPULSE(1)

	PORT_START("LINE02")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Clubs") PORT_CODE(KEYCODE_G) PORT_IMPULSE(1)

	PORT_START("LINE03")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Spades") PORT_CODE(KEYCODE_D) PORT_IMPULSE(1)

	PORT_START("LINE04")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Rdbl") PORT_CODE(KEYCODE_T) PORT_IMPULSE(1)

	PORT_START("LINE05")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NT") PORT_CODE(KEYCODE_E) PORT_IMPULSE(1)

	PORT_START("LINE06")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Hearts, Up") PORT_CODE(KEYCODE_S) PORT_IMPULSE(1)

	PORT_START("LINE07")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Play, Yes") PORT_CODE(KEYCODE_X) PORT_IMPULSE(1)

	PORT_START("LINE08")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Back") PORT_CODE(KEYCODE_A) PORT_IMPULSE(1)

	PORT_START("LINE09")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Dbl") PORT_CODE(KEYCODE_R) PORT_IMPULSE(1)

	PORT_START("LINE10")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Diamonds, Down") PORT_CODE(KEYCODE_F) PORT_IMPULSE(1)

	PORT_START("LINE11")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Start") PORT_CODE(KEYCODE_Q) PORT_IMPULSE(1)

	PORT_START("LINE12")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Play, No") PORT_CODE(KEYCODE_B) PORT_IMPULSE(1)
INPUT_PORTS_END

static void tms_interrupt(running_machine *machine, int dummy)
{
	cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
}

static INTERRUPT_GEN( bbcbc_interrupt )
{
	TMS9928A_interrupt(device->machine);
}

static const TMS9928a_interface tms9129_interface =
{
	TMS9929A,
	0x4000,
	0, 0,
	tms_interrupt
};

/* TODO */
static Z80PIO_INTERFACE( bbcbc_z80pio_intf )
{
	DEVCB_NULL,	/* int callback */
	DEVCB_NULL,	/* port a read */
	DEVCB_NULL,	/* port b read */
	DEVCB_NULL,	/* port a write */
	DEVCB_NULL,	/* port b write */
	DEVCB_NULL,	/* ready a */
	DEVCB_NULL	/* ready b */
};

static const z80_daisy_config bbcbc_daisy_chain[] =
{
	{ "z80pio" },
	{ NULL }
};


static DEVICE_START( bbcbc_cart )
{
	UINT8 *cart = memory_region(device->machine,  "maincpu" ) + 0x4000;

	memset( cart, 0xFF, 0x8000 );
}


static DEVICE_IMAGE_LOAD( bbcbc_cart )
{
	UINT8 *cart = memory_region(image.device().machine,  "maincpu" ) + 0x4000;

	if ( image.software_entry() == NULL )
	{
		int size = image.length();
		if ( image.fread(cart, size ) != size ) {
			image.seterror( IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
			return IMAGE_INIT_FAIL;
		}
	}
	else
	{
		UINT8 *reg = image.get_software_region( "rom" );
		int reg_len = image.get_software_region_length( "rom" );

		memcpy( cart, reg, MIN(reg_len, 0x8000) );
	}

	return IMAGE_INIT_PASS;

}

static MACHINE_START( bbcbc )
{
	TMS9928A_configure(&tms9129_interface);
}

static MACHINE_RESET( bbcbc )
{
}


static MACHINE_DRIVER_START( bbcbc )
	MDRV_CPU_ADD( "maincpu", Z80, MAIN_CLOCK / 8 )
	MDRV_CPU_PROGRAM_MAP( bbcbc_prg)
	MDRV_CPU_IO_MAP( bbcbc_io)
	MDRV_CPU_CONFIG(bbcbc_daisy_chain)
	MDRV_CPU_VBLANK_INT("screen", bbcbc_interrupt)

	MDRV_MACHINE_START( bbcbc )
	MDRV_MACHINE_RESET( bbcbc )

	MDRV_Z80PIO_ADD( "z80pio", MAIN_CLOCK / 8, bbcbc_z80pio_intf )

	MDRV_IMPORT_FROM( tms9928a )
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE( 50 )

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_INTERFACE("bbcbc_cart")
	MDRV_CARTSLOT_START( bbcbc_cart )
	MDRV_CARTSLOT_LOAD( bbcbc_cart )

	/* Software lists */
	MDRV_SOFTWARE_LIST_ADD("cart_list","bbcbc")
MACHINE_DRIVER_END


ROM_START( bbcbc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("br_4_1.ic3", 0x0000, 0x2000, CRC(7c880d75) SHA1(954db096bd9e8edfef72946637a12f1083841fb0))
	ROM_LOAD("br_4_2.ic4", 0x2000, 0x2000, CRC(16a33aef) SHA1(9529f9f792718a3715af2063b91a5fb18f741226))
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*   YEAR  NAME  PARENT  COMPAT  MACHINE INPUT  INIT  COMPANY  FULLNAME  FLAGS */
CONS(1985, bbcbc,     0, 0,      bbcbc,  bbcbc, 0,    "BBC",   "Bridge Companion", GAME_NO_SOUND )
