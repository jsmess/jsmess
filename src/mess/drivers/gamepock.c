#include "emu.h"
#include "softlist.h"
#include "cpu/upd7810/upd7810.h"
#include "sound/speaker.h"
#include "devices/cartslot.h"
#include "includes/gamepock.h"


static ADDRESS_MAP_START(gamepock_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0x0fff) AM_ROM
	AM_RANGE(0x1000,0x3fff) AM_NOP
	AM_RANGE(0x4000,0xBfff) AM_ROMBANK("bank1")
	AM_RANGE(0xC000,0xC7ff) AM_MIRROR(0x0800) AM_RAM
	AM_RANGE(0xff80,0xffff) AM_RAM				/* 128 bytes microcontroller RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START(gamepock_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( 0x00, 0x00 ) AM_WRITE( gamepock_port_a_w )
	AM_RANGE( 0x01, 0x01 ) AM_READWRITE( gamepock_port_b_r, gamepock_port_b_w )
	AM_RANGE( 0x02, 0x02 ) AM_READ( gamepock_port_c_r )
ADDRESS_MAP_END

static INPUT_PORTS_START( gamepock )
	PORT_START("IN0")
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_SELECT )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START("IN1")
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_START )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
INPUT_PORTS_END

static const UPD7810_CONFIG gamepock_cpu_config = { TYPE_78C06, gamepock_io_callback };

static DEVICE_START(gamepock_cart)
{
	memory_set_bankptr( device->machine, "bank1", memory_region(device->machine,  "user1" ) );
}

static DEVICE_IMAGE_LOAD(gamepock_cart) {
	UINT8 *cart = memory_region(image->machine,  "user1" );

	if ( image_software_entry(image) == NULL )
	{
		int size = image_length( image );
		if ( image_fread( image, cart, size ) != size ) {
			image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
			return INIT_FAIL;
		}
	}
	else
	{
		cart = image_get_software_region( image, CARTRIDGE_REGION_ROM );
	}

	memory_set_bankptr( image->machine, "bank1", cart );

	return INIT_PASS;
}

static MACHINE_DRIVER_START( gamepock )
	MDRV_CPU_ADD("maincpu", UPD78C06, XTAL_6MHz)	/* uPD78C06AG */
	MDRV_CPU_PROGRAM_MAP( gamepock_mem)
	MDRV_CPU_IO_MAP( gamepock_io)
	MDRV_CPU_CONFIG( gamepock_cpu_config )

	MDRV_MACHINE_RESET( gamepock )

	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 75, 64 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 74, 0, 63 )
	MDRV_DEFAULT_LAYOUT(layout_lcd)

	MDRV_PALETTE_LENGTH( 2 )
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE( gamepock )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_START(gamepock_cart)
	MDRV_CARTSLOT_LOAD(gamepock_cart)
	MDRV_CARTSLOT_SOFTWARE_LIST(gamepock_cart)
MACHINE_DRIVER_END


ROM_START( gamepock )
	ROM_REGION( 0x1000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "egpcboot.bin", 0x0000, 0x1000, CRC(ee1ea65d) SHA1(9c7731b5ead721d2cc7f7e2655c5fed9e56db8b0) )
	ROM_REGION( 0x8000, "user1", ROMREGION_ERASEFF )
ROM_END


CONS( 1984, gamepock, 0, 0, gamepock, gamepock, 0, "Epoch", "Game Pocket Computer", 0 )

