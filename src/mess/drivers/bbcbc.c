/***************************************************************************

  bbcbc.c

  BBC Bridge Companion

  TODO:
  Inputs
  Find correct clock frequencies

***************************************************************************/
#include "driver.h"
#include "video/tms9928a.h"
#include "machine/z80pio.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START( bbcbc_prg, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_ROM
	AM_RANGE(0x04000, 0x0bfff) AM_ROM
	AM_RANGE(0x0e000, 0x0e7ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( bbcbc_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x40) AM_READWRITE(z80pioA_0_p_r, z80pioA_0_p_w)
	AM_RANGE(0x80, 0x80) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0x81, 0x81) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
ADDRESS_MAP_END

static void tms_interrupt(int dummy)
{
	cpunum_set_input_line(0, 0, HOLD_LINE);
}

static INTERRUPT_GEN( bbcbc_interrupt )
{
    TMS9928A_interrupt();
}

INPUT_PORTS_START( bbcbc )
INPUT_PORTS_END

static const TMS9928a_interface tms9129_interface =
{
	TMS9929A,
	0x4000,
	0, 0,
	tms_interrupt
};

/* TODO */
static const z80pio_interface pio_intf = 
{
	NULL,
	NULL,
	NULL
};

static MACHINE_START( bbcbc )
{
	TMS9928A_configure(&tms9129_interface);
}

static MACHINE_RESET( bbcbc )
{
}


static MACHINE_DRIVER_START( bbcbc )
	MDRV_CPU_ADD_TAG( "main", Z80, 4000000 )
	MDRV_CPU_PROGRAM_MAP( bbcbc_prg, 0 )
	MDRV_CPU_IO_MAP( bbcbc_io, 0 )

	MDRV_MACHINE_START( bbcbc )
	MDRV_MACHINE_RESET( bbcbc )

	MDRV_CPU_VBLANK_INT( bbcbc_interrupt, 1 )
	MDRV_SCREEN_REFRESH_RATE( 50 )
	MDRV_IMPORT_FROM( tms9928a )
MACHINE_DRIVER_END


ROM_START( bbcbc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD("br_4_1.ic3", 0x0000, 0x2000, CRC(7c880d75) SHA1(954db096bd9e8edfef72946637a12f1083841fb0))
	ROM_LOAD("br_4_2.ic4", 0x2000, 0x2000, CRC(16a33aef) SHA1(9529f9f792718a3715af2063b91a5fb18f741226))
	ROM_CART_LOAD(0, "bin", 0x4000, 0xBFFF, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

SYSTEM_CONFIG_START( bbcbc )
    CONFIG_DEVICE(cartslot_device_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*   YEAR  NAME  PARENT  COMPAT  MACHINE INPUT  INIT  CONFIG  COMPANY  FULLNAME  FLAGS */
CONS(1985, bbcbc,     0, 0,      bbcbc,  bbcbc, 0,    bbcbc,  "BBC",   "Bridge Companion", GAME_NOT_WORKING )

