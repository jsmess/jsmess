#include "driver.h"
#include "cpu/upd7810/upd7810.h"
#include "devices/cartslot.h"
#include "includes/gamepock.h"

static ADDRESS_MAP_START(gamepock_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000,0x0fff) AM_ROM
	AM_RANGE(0x1000,0x3fff) AM_NOP
	AM_RANGE(0x4000,0xBfff) AM_ROMBANK(1)
	AM_RANGE(0xC000,0xC7ff) AM_MIRROR(0x0800) AM_RAM
	AM_RANGE(0xff80,0xffff) AM_RAM				/* 128 bytes microcontroller RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START(gamepock_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( 0x00, 0x00 ) AM_WRITE( gamepock_port_a_w )
	AM_RANGE( 0x01, 0x01 ) AM_READWRITE( gamepock_port_b_r, gamepock_port_b_w )
	AM_RANGE( 0x02, 0x02 ) AM_READ( gamepock_port_c_r )
ADDRESS_MAP_END

static INPUT_PORTS_START( gamepock )
	PORT_START
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_SELECT )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_START )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
INPUT_PORTS_END

static const UPD7810_CONFIG gamepock_cpu_config = { TYPE_78C06, NULL };

static MACHINE_DRIVER_START( gamepock )
	MDRV_CPU_ADD_TAG("main", UPD78C06, 6000000)	/* uPD78C06AG */
	MDRV_CPU_PROGRAM_MAP( gamepock_mem, 0 )
	MDRV_CPU_IO_MAP( gamepock_io, 0 )
	MDRV_CPU_CONFIG( gamepock_cpu_config )

	MDRV_MACHINE_RESET( gamepock )

	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 75, 64 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 74, 0, 63 )

	MDRV_PALETTE_LENGTH( 2 )
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE( gamepock )

MACHINE_DRIVER_END

static DEVICE_INIT(gamepock_cart) {
	memory_set_bankptr( 1, memory_region( REGION_USER1 ) );
	return INIT_PASS;
}

static DEVICE_LOAD(gamepock_cart) {
	UINT8 *cart = memory_region( REGION_USER1 );
	int size = image_length( image );

	if ( image_fread( image, cart, size ) != size ) {
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
		return INIT_FAIL;
	}

	memory_set_bankptr( 1, memory_region( REGION_USER1 ) );

	return INIT_PASS;
}

static void gamepock_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info) {
	switch( state ) {
	case DEVINFO_INT_COUNT:										info->i = 1; break;
	case DEVINFO_INT_MUST_BE_LOADED:							info->i = 0; break;
	case DEVINFO_PTR_INIT:										info->init = device_init_gamepock_cart; break;
	case DEVINFO_PTR_LOAD:										info->load = device_load_gamepock_cart; break;
	case DEVINFO_STR_FILE_EXTENSIONS:							strcpy( info->s = device_temp_str(), "bin" ); break;
	default:													cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(gamepock)
	CONFIG_DEVICE(gamepock_cartslot_getinfo)
SYSTEM_CONFIG_END

ROM_START( gamepock )
	ROM_REGION( 0x1000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_LOAD( "egpcboot.bin", 0x0000, 0x1000, CRC(ee1ea65d) SHA1(9c7731b5ead721d2cc7f7e2655c5fed9e56db8b0) )
	ROM_REGION( 0x8000, REGION_USER1, ROMREGION_ERASEFF )
ROM_END

CONS( 1984, gamepock, 0, 0, gamepock, gamepock, 0, gamepock, "Epoch", "Game Pocket Computer", 0 )

