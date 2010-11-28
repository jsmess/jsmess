/***************************************************************************

  pcfx.c

  Driver file to handle emulation of the NEC PC-FX.

***************************************************************************/

#include "emu.h"
#include "cpu/v810/v810.h"
#include "video/vdc.h"


class pcfx_state : public driver_device
{
public:
	pcfx_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START( pcfx_mem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE( 0x00000000, 0x001FFFFF ) AM_RAM	/* RAM */
	AM_RANGE( 0x80700000, 0x807FFFFF ) AM_NOP	/* EXTIO */
	AM_RANGE( 0xE0000000, 0xE7FFFFFF ) AM_NOP
	AM_RANGE( 0xE8000000, 0xE9FFFFFF ) AM_NOP
	AM_RANGE( 0xF8000000, 0xF8000007 ) AM_NOP	/* PIO */
	AM_RANGE( 0xFFF00000, 0xFFFFFFFF ) AM_ROMBANK("bank1")	/* ROM */
ADDRESS_MAP_END


static ADDRESS_MAP_START( pcfx_io, ADDRESS_SPACE_IO, 32 )
	AM_RANGE( 0x00000000, 0x000000FF ) AM_NOP	/* PAD */
	AM_RANGE( 0x00000100, 0x000001FF ) AM_NOP	/* HuC6230 */
	AM_RANGE( 0x00000200, 0x000002FF ) AM_NOP	/* HuC6271 */
	AM_RANGE( 0x00000300, 0x000003FF ) AM_NOP	/* HuC6261 */
	AM_RANGE( 0x00000400, 0x000004FF ) AM_NOP	/* HuC6270-A */
	AM_RANGE( 0x00000500, 0x000005FF ) AM_NOP	/* HuC6270-B */
	AM_RANGE( 0x00000600, 0x000006FF ) AM_NOP	/* HuC6272 */
	AM_RANGE( 0x00000C80, 0x00000C83 ) AM_NOP
	AM_RANGE( 0x00000E00, 0x00000EFF ) AM_NOP
	AM_RANGE( 0x00000F00, 0x00000FFF ) AM_NOP
	AM_RANGE( 0x80500000, 0x805000FF ) AM_NOP	/* HuC6273 */
ADDRESS_MAP_END

static INPUT_PORTS_START( pcfx )
INPUT_PORTS_END


static MACHINE_RESET( pcfx )
{
	memory_set_bankptr( machine, "bank1", memory_region(machine, "user1") );
}


static MACHINE_CONFIG_START( pcfx, pcfx_state )
	MDRV_CPU_ADD( "maincpu", V810, 21477270 )
	MDRV_CPU_PROGRAM_MAP( pcfx_mem)
	MDRV_CPU_IO_MAP( pcfx_io)

	MDRV_MACHINE_RESET( pcfx )

	MDRV_SCREEN_ADD( "screen", RASTER )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_RGB32 )
	MDRV_SCREEN_RAW_PARAMS(21477270/2, VDC_WPF, 70, 70 + 512 + 32, VDC_LPF, 14, 14+242)
MACHINE_CONFIG_END


ROM_START( pcfx )
	ROM_REGION( 0x100000, "user1", 0 )
	ROM_LOAD( "pcfxbios.bin", 0x000000, 0x100000, CRC(76ffb97a) SHA1(1a77fd83e337f906aecab27a1604db064cf10074) )
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT   INIT    COMPANY FULLNAME        FLAGS */
CONS( 1994, pcfx,       0,      0,      pcfx,       pcfx,      0,      "Nippon Electronic Company",  "PC-FX",        GAME_NOT_WORKING | GAME_NO_SOUND )

