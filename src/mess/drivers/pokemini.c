/********************************************************************

Driver file to handle emulation of the Nintendo Pokemon Mini handheld
by Wilbert Pol.

The LCD is likely to be a SSD1828 LCD.

********************************************************************/

#include "driver.h"
#include "sound/speaker.h"
#include "machine/i2cmem.h"
#include "includes/pokemini.h"
#include "cpu/minx/minx.h"
#include "devices/cartslot.h"


static ADDRESS_MAP_START( pokemini_mem_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x000000, 0x000FFF )  AM_ROM							/* bios */
	AM_RANGE( 0x001000, 0x001FFF )	AM_RAM AM_BASE( &pokemini_ram)				/* VRAM/RAM */
	AM_RANGE( 0x002000, 0x0020FF )  AM_READWRITE( pokemini_hwreg_r, pokemini_hwreg_w )	/* hardware registers */
	AM_RANGE( 0x002100, 0x1FFFFF )  AM_ROM							/* cartridge area */
ADDRESS_MAP_END


static INPUT_PORTS_START( pokemini )
	PORT_START("INPUTS")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("Button A")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("Button B")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_NAME("Button C")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_NAME("Up")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_NAME("Down")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_NAME("Left")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_NAME("Right")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1) PORT_NAME("Power")
INPUT_PORTS_END


static PALETTE_INIT( pokemini )
{
	palette_set_color(machine, 0, MAKE_RGB(0xff, 0xfb, 0x87));
	palette_set_color(machine, 1, MAKE_RGB(0xb1, 0xae, 0x4e));
	palette_set_color(machine, 2, MAKE_RGB(0x84, 0x80, 0x4e));
	palette_set_color(machine, 3, MAKE_RGB(0x4e, 0x4e, 0x4e));
}


static const speaker_interface pokemini_speaker_interface =
{
	3,				/* optional: number of different levels */
	NULL			/* optional: level lookup table */
};

static MACHINE_DRIVER_START( pokemini )
	/* basic machine hardware */
	MDRV_CPU_ADD( "maincpu", MINX, 4000000 )
	MDRV_CPU_PROGRAM_MAP( pokemini_mem_map)

	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_RESET( pokemini )

	MDRV_NVRAM_HANDLER( i2cmem_0 )

	/* video hardware */
	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	/* This still needs to be improved to actually match the hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( 96, 64 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 95, 0, 63 )
	MDRV_DEFAULT_LAYOUT(layout_lcd)
	MDRV_PALETTE_LENGTH( 4 )
	MDRV_PALETTE_INIT( pokemini )
	MDRV_SCREEN_REFRESH_RATE( 72 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_CONFIG(pokemini_speaker_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("min,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_START(pokemini_cart)
	MDRV_CARTSLOT_LOAD(pokemini_cart)
MACHINE_DRIVER_END


static DRIVER_INIT( pokemini )
{
	i2cmem_init( machine, 0, I2CMEM_SLAVE_ADDRESS, 0, 0x2000, NULL);
}

ROM_START( pokemini )
	ROM_REGION( 0x200000, "maincpu", 0 )
	ROM_LOAD( "bios.min", 0x0000, 0x1000, CRC(aed3c14d) SHA1(daad4113713ed776fbd47727762bca81ba74915f) )
ROM_END


CONS( 1999, pokemini, 0, 0, pokemini, pokemini, pokemini, 0, "Nintendo", "Pokemon Mini", GAME_NOT_WORKING )

