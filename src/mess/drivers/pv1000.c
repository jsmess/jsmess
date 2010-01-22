/***************************************************************************

    Driver for Casio PV-1000

***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "devices/cartslot.h"
#include "streams.h"


#define SOUND_PV1000	DEVICE_GET_INFO_NAME(pv1000_sound)


static UINT8 *pv1000_ram;
static UINT8 pv1000_io_regs[8];
static UINT8 pv1000_fd_data;
static sound_stream *pv1000_sh_channel;


static WRITE8_HANDLER( pv1000_io_w );
static READ8_HANDLER( pv1000_io_r );
static WRITE8_HANDLER( pv1000_gfxram_w );


static ADDRESS_MAP_START( pv1000, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x3fff ) AM_MIRROR( 0x4000 ) AM_ROM AM_REGION( "cart", 0 )
	AM_RANGE( 0xb800, 0xbbff ) AM_RAM AM_BASE( &pv1000_ram )
	AM_RANGE( 0xbc00, 0xbfff ) AM_RAM_WRITE( pv1000_gfxram_w ) AM_REGION( "gfxram", 0 )
ADDRESS_MAP_END


static ADDRESS_MAP_START( pv1000_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK( 0xff )
	AM_RANGE( 0xf8, 0xff ) AM_READWRITE( pv1000_io_r, pv1000_io_w )
ADDRESS_MAP_END


static WRITE8_HANDLER( pv1000_gfxram_w )
{
	UINT8 *gfxram = memory_region( space->machine, "gfxram" );

	gfxram[ offset ] = data;
	gfx_element_mark_dirty(space->machine->gfx[1], offset/32);
}


static WRITE8_HANDLER( pv1000_io_w )
{
//	logerror("pv1000_io_w offset=%02x, data=%02x\n", offset, data );
	switch ( offset )
	{
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
		/* Sound i/O ports */
		break;

	case 0x05:
		pv1000_fd_data = 1;
		break;
	}

	pv1000_io_regs[offset] = data;
}


static READ8_HANDLER( pv1000_io_r )
{
	UINT8 data = pv1000_io_regs[offset];

//	logerror("pv1000_io_r offset=%02x\n", offset );

	switch ( offset )
	{
	case 0x04:
		/* Bit 1 = 1 => to pass checks in Amidar? */
		/* Bit 0 = 0 => done reading joystick interface? */
		data = pv1000_fd_data ? 0x02 : 0x01;
		break;
	case 0x05:
		data = 0;
		if ( pv1000_io_regs[5] & 0x08 )
		{
			data = input_port_read( space->machine, "IN3" );
		}
		if ( pv1000_io_regs[5] & 0x04 )
		{
			data = input_port_read( space->machine, "IN2" );
		}
		if ( pv1000_io_regs[5] & 0x02 )
		{
			data = input_port_read( space->machine, "IN1" );
		}
		if ( pv1000_io_regs[5] & 0x01 )
		{
			data = input_port_read( space->machine, "IN0" );
		}
		pv1000_fd_data = 0;
		break;
	}

	return data;
}


static INPUT_PORTS_START( pv1000 )
	PORT_START( "IN0" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START ) PORT_PLAYER(2)

	PORT_START( "IN1" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY

	PORT_START( "IN2" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_8WAY

	PORT_START( "IN3" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
INPUT_PORTS_END


static PALETTE_INIT( pv1000 )
{
	palette_set_color_rgb( machine,  0,   0,   0,   0 );
	palette_set_color_rgb( machine,  1,   0,   0, 255 );
	palette_set_color_rgb( machine,  2,   0, 255,   0 );
	palette_set_color_rgb( machine,  3,   0, 255, 255 );
	palette_set_color_rgb( machine,  4, 255,   0,   0 );
	palette_set_color_rgb( machine,  5, 255,   0, 255 );
	palette_set_color_rgb( machine,  6, 255, 255,   0 );
	palette_set_color_rgb( machine,  7, 255, 255, 255 );
}


static DEVICE_IMAGE_LOAD( pv1000_cart )
{
	UINT8 *cart = memory_region( image->machine, "cart" );
	int size = image_length( image );

	if ( size != 0x2000 && size != 0x4000 )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unsupported cartridge size" );
		return INIT_FAIL;
	}

	if ( image_fread( image, cart, size ) != size )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
		return INIT_FAIL;
	}

	/* Mirror 8KB rom */
	if ( size == 0x2000 )
	{
		memcpy( cart+0x2000, cart, 0x2000 );
	}

	return INIT_PASS;
}


static VIDEO_UPDATE( pv1000 )
{
	int x, y;

	for ( y = 0; y < 24; y++ )
	{
		for ( x = 0; x < 32; x++ )
		{
			UINT16 tile = pv1000_ram[ y * 32 + x ];

			if ( tile < 0xe0 )
			{
				tile += ( pv1000_io_regs[7] * 8 );
				drawgfx_opaque( bitmap, cliprect, screen->machine->gfx[0], tile, 0, 0, 0, x*8, y*8 );
			}
			else
			{
				tile -= 0xe0;
				drawgfx_opaque( bitmap, cliprect, screen->machine->gfx[1], tile, 0, 0, 0, x*8, y*8 );
			}
		}
	}

	return 0;
}


static STREAM_UPDATE( pv1000_sound_update )
{
	stream_sample_t *buffer = outputs[0];

	while( samples > 0 )
	{
		*buffer = 0;
		buffer++;
		samples--;
	}
}


static DEVICE_START( pv1000_sound )
{
	pv1000_sh_channel = stream_create( device, 0, 1, device->clock, 0, pv1000_sound_update );
}


static DEVICE_GET_INFO( pv1000_sound )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(pv1000_sound);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "NEC D65010G031");				break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
	}
}


static TIMER_DEVICE_CALLBACK( pv1000_irq )
{
	const device_config *cpu = devtag_get_device( timer->machine, "maincpu" );
	cpu_set_input_line(cpu, 0, HOLD_LINE);
}


static MACHINE_RESET( pv1000 )
{
	pv1000_io_regs[5] = 0;
	pv1000_fd_data = 0;
}


static const gfx_layout pv1000_3bpp_gfx =
{
	8, 8,			/* 8x8 characters */
	RGN_FRAC(1,1),
	3,
	{ 0, 8*8, 16*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8*4
};


static GFXDECODE_START( pv1000 )
	GFXDECODE_ENTRY( "cart", 8, pv1000_3bpp_gfx, 0, 8 )
	GFXDECODE_ENTRY( "gfxram", 8, pv1000_3bpp_gfx, 0, 8 )
GFXDECODE_END


static MACHINE_DRIVER_START( pv1000 )
	MDRV_CPU_ADD( "maincpu", Z80, 17897725/5 )
	MDRV_CPU_PROGRAM_MAP( pv1000 )
	MDRV_CPU_IO_MAP( pv1000_io )

	MDRV_TIMER_ADD_PERIODIC("pv1000_irq", pv1000_irq, HZ(240))

	MDRV_MACHINE_RESET( pv1000 )

	MDRV_SCREEN_ADD( "screen", RASTER )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_SIZE(256, 192)
	MDRV_SCREEN_VISIBLE_AREA( 0, 255, 0, 191 )

	MDRV_PALETTE_LENGTH( 8 )
	MDRV_PALETTE_INIT( pv1000 )
	MDRV_GFXDECODE( pv1000 )

	/* D65010G031 - Video & sound chip */
	MDRV_VIDEO_UPDATE( pv1000 )

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD( "pv1000_sound", PV1000, 17897725 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )

	/* Cartridge slot */
	MDRV_CARTSLOT_ADD( "cart" )
	MDRV_CARTSLOT_EXTENSION_LIST( "bin" )
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_LOAD( pv1000_cart )
MACHINE_DRIVER_END


ROM_START( pv1000 )
	ROM_REGION( 0x4000, "cart", ROMREGION_ERASE00 ) 
	ROM_REGION( 0x400, "gfxram", ROMREGION_ERASE00 )
ROM_END


/*    YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT  INIT    COMPANY   FULLNAME    FLAGS */
CONS( 1983, pv1000,  0,      0,      pv1000,  pv1000,   0,   "Casio",  "PV-1000",  GAME_NOT_WORKING )

