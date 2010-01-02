/***************************************************************************

    Driver for Epoch Super Cassette Vision


***************************************************************************/

#include "driver.h"
#include "cpu/upd7810/upd7810.h"
#include "devices/cartslot.h"


static READ8_HANDLER( scv_porta_r );
static WRITE8_HANDLER( scv_porta_w );
static READ8_HANDLER( scv_portb_r );
static READ8_HANDLER( scv_portc_r );
static WRITE8_HANDLER( scv_portc_w );


static UINT8 *scv_vram;
static UINT8 scv_porta;
static UINT8 scv_portc;
static emu_timer *scv_vb_timer;


static ADDRESS_MAP_START( scv_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x0fff ) AM_ROM		/* BIOS */

	AM_RANGE( 0x2000, 0x3403 ) AM_RAM AM_BASE( &scv_vram )	/* VRAM + 4 registers */

	AM_RANGE( 0x8000, 0xff7f ) AM_ROM AM_REGION("cart", 0)
	AM_RANGE( 0xff80, 0xffff ) AM_RAM		/* upd7801 internal RAM */
ADDRESS_MAP_END


static ADDRESS_MAP_START( scv_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( 0x00, 0x00 ) AM_READWRITE( scv_porta_r, scv_porta_w )
	AM_RANGE( 0x01, 0x01 ) AM_READ( scv_portb_r )
	AM_RANGE( 0x02, 0x02 ) AM_READWRITE( scv_portc_r, scv_portc_w )
ADDRESS_MAP_END


static INPUT_PORTS_START( scv )
	PORT_START( "PA0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) 

	PORT_START( "PA1" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)

	PORT_START( "PA2" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("1") PORT_CODE(KEYCODE_1)

	PORT_START( "PA3" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("3") PORT_CODE(KEYCODE_3)

	PORT_START( "PA4" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("5") PORT_CODE(KEYCODE_5)

	PORT_START( "PA5" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("7") PORT_CODE(KEYCODE_7)

	PORT_START( "PA6" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("9") PORT_CODE(KEYCODE_9)

	PORT_START( "PA7" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Cl") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("En") PORT_CODE(KEYCODE_ENTER)

	PORT_START( "PC0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(DEF_STR(Pause)) PORT_CODE(KEYCODE_O)
INPUT_PORTS_END


static READ8_HANDLER( scv_porta_r )
{
	return 0xff;
}


static WRITE8_HANDLER( scv_porta_w )
{
	scv_porta = data;
}

static READ8_HANDLER( scv_portb_r )
{
	return 0xff;
}


static READ8_HANDLER( scv_portc_r )
{
	return 0xff;
}


static WRITE8_HANDLER( scv_portc_w )
{
	scv_portc = data;
}


static DEVICE_IMAGE_LOAD( scv_cart )
{

	UINT8 *cart = memory_region( image->machine, "cart" );
	int size = image_length( image );

	if ( size > 0x8000 )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unsupported cartridge size" );
		return INIT_FAIL;
	}

	if ( image_fread( image, cart, size ) != size )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
		return INIT_FAIL;
	}

	return INIT_PASS;
}


static PALETTE_INIT( scv )
{
	/* Palette borrowed from eSCV. No idea if this is correct */
	palette_set_color_rgb( machine,  0,   0,  90, 156 );
	palette_set_color_rgb( machine,  1,   0,   0,   0 );
	palette_set_color_rgb( machine,  2,  58, 148, 255 );
	palette_set_color_rgb( machine,  3,   0,   0, 255 );
	palette_set_color_rgb( machine,  4,  16, 214,   0 );
	palette_set_color_rgb( machine,  5,  66, 255,  16 );
	palette_set_color_rgb( machine,  6, 123, 230, 197 );
	palette_set_color_rgb( machine,  7,   0, 173,   0 );
	palette_set_color_rgb( machine,  8, 255,  41, 148 );
	palette_set_color_rgb( machine,  9, 255,  49,  16 );
	palette_set_color_rgb( machine, 10, 255,  58, 255 );
	palette_set_color_rgb( machine, 11, 239, 156, 255 );
	palette_set_color_rgb( machine, 12, 255, 206,  33 );
	palette_set_color_rgb( machine, 13,  74, 123,  16 );
	palette_set_color_rgb( machine, 14, 165, 148, 165 );
	palette_set_color_rgb( machine, 15, 255, 255, 255 );
}


static TIMER_CALLBACK( scv_vb_callback )
{
	int vpos = video_screen_get_vpos(machine->primary_screen);

	switch( vpos )
	{
	case 240:
		cputag_set_input_line(machine, "maincpu", UPD7810_INTF2, CLEAR_LINE);
		break;
	case 0:
		cputag_set_input_line(machine, "maincpu", UPD7810_INTF2, ASSERT_LINE);
		break;
	}

	timer_adjust_oneshot( scv_vb_timer, video_screen_get_time_until_pos( machine->primary_screen, ( vpos + 1 ) % 262, 0 ), 0 );
}


INLINE void plot_sprite_part( bitmap_t *bitmap, UINT8 x, UINT8 y, UINT8 pat, UINT8 col )
{
	if ( pat & 0x08 )
		*BITMAP_ADDR16( bitmap, y, x ) = col;
	if ( pat & 0x04 && x < 255 )
		*BITMAP_ADDR16( bitmap, y, x + 1 ) = col;
	if ( pat & 0x02 && x < 254 )
		*BITMAP_ADDR16( bitmap, y, x + 2 ) = col;
	if ( pat & 0x01 && x < 253 )
		*BITMAP_ADDR16( bitmap, y, x + 3 ) = col;
}


INLINE void draw_sprite( bitmap_t *bitmap, UINT8 x, UINT8 y, UINT8 tile_idx, UINT8 col, UINT8 left, UINT8 right, UINT8 top, UINT8 bottom, UINT8 clip_y )
{
	int j;

	y += clip_y * 2;
	for ( j = clip_y * 4; j < 32; j += 4 )
	{
		UINT8 pat0 = scv_vram[ tile_idx * 32 + j + 0 ];
		UINT8 pat1 = scv_vram[ tile_idx * 32 + j + 1 ];
		UINT8 pat2 = scv_vram[ tile_idx * 32 + j + 2 ];
		UINT8 pat3 = scv_vram[ tile_idx * 32 + j + 3 ];

		if ( ( top && j < 16 ) || ( bottom && j >= 16 ) )
		{
			if ( left )
			{
				plot_sprite_part( bitmap, x     , y, pat0 >> 4, col );
				plot_sprite_part( bitmap, x +  4, y, pat1 >> 4, col );
			}
			if ( right )
			{
				plot_sprite_part( bitmap, x +  8, y, pat2 >> 4, col );
				plot_sprite_part( bitmap, x + 12, y, pat3 >> 4, col );
			}

			if ( left )
			{
				plot_sprite_part( bitmap, x     , y + 1, pat0 & 0x0f, col );
				plot_sprite_part( bitmap, x +  4, y + 1, pat1 & 0x0f, col );
			}
			if ( right )
			{
				plot_sprite_part( bitmap, x +  8, y + 1, pat2 & 0x0f, col );
				plot_sprite_part( bitmap, x + 12, y + 1, pat3 & 0x0f, col );
			}
		}

		y += 2;
	}
}


INLINE void draw_semi_graph( bitmap_t *bitmap, UINT8 x, UINT8 y, UINT8 data, UINT8 fg, UINT8 bg )
{
	UINT8 col = data ? fg : bg;
	int i;

	for ( i = 0; i < 4; i++ )
	{
		*BITMAP_ADDR16(bitmap, y + i, x + 0) = col;
		*BITMAP_ADDR16(bitmap, y + i, x + 1) = col;
		*BITMAP_ADDR16(bitmap, y + i, x + 2) = col;
		*BITMAP_ADDR16(bitmap, y + i, x + 3) = col;
	}
}


INLINE void draw_block_graph( bitmap_t *bitmap, UINT8 x, UINT8 y, UINT8 col )
{
	int i;

	for ( i = 0; i < 8; i++ )
	{
		*BITMAP_ADDR16(bitmap, y + i, x + 0) = col;
		*BITMAP_ADDR16(bitmap, y + i, x + 1) = col;
		*BITMAP_ADDR16(bitmap, y + i, x + 2) = col;
		*BITMAP_ADDR16(bitmap, y + i, x + 3) = col;
		*BITMAP_ADDR16(bitmap, y + i, x + 4) = col;
		*BITMAP_ADDR16(bitmap, y + i, x + 5) = col;
		*BITMAP_ADDR16(bitmap, y + i, x + 6) = col;
		*BITMAP_ADDR16(bitmap, y + i, x + 7) = col;
	}
}


static VIDEO_UPDATE( scv )
{
	int x, y;
//	UINT8 fg = scv_vram[0x1403] >> 4;
	UINT8 bg = scv_vram[0x1403] & 0x0f;
	UINT8 gr_fg = scv_vram[0x1401] >> 4;
	UINT8 gr_bg = scv_vram[0x1401] & 0x0f;
	int clip_x = ( scv_vram[0x1402] & 0x0f ) * 2;
	int clip_y = scv_vram[0x1402] >> 4;

	/* Clear the screen */
	bitmap_fill( bitmap, cliprect, bg );

	/* Draw background */
	for ( y = 0; y < 16; y++ )
	{
		int text_y = 0;

		if ( y < clip_y )
			text_y = ( scv_vram[0x1400] & 0x80 ) ? 0 : 1;
		else
			text_y = ( scv_vram[0x1400] & 0x80 ) ? 1 : 0;

		for ( x = 0; x < 32; x++ )
		{
			int text_x = 0;
			UINT8 d = scv_vram[ 0x1000 + y * 32 + x ];

			if ( x < clip_x )
				text_x = ( scv_vram[0x1400] & 0x40 ) ? 0 : 1;
			else
				text_x = ( scv_vram[0x1400] & 0x40 ) ? 1 : 0;

			if ( text_x && text_y )
			{
				/* Text mode */
			}
			else
			{
				switch ( scv_vram[0x1400] & 0x03 )
				{
				case 0x01:		/* Semi graphics mode */
					draw_semi_graph( bitmap, x * 8    , y * 16     , d & 0x80, gr_fg, gr_bg );
					draw_semi_graph( bitmap, x * 8 + 4, y * 16     , d & 0x40, gr_fg, gr_bg );
					draw_semi_graph( bitmap, x * 8    , y * 16 +  4, d & 0x20, gr_fg, gr_bg );
					draw_semi_graph( bitmap, x * 8 + 4, y * 16 +  4, d & 0x10, gr_fg, gr_bg );
					draw_semi_graph( bitmap, x * 8    , y * 16 +  8, d & 0x08, gr_fg, gr_bg );
					draw_semi_graph( bitmap, x * 8 + 4, y * 16 +  8, d & 0x04, gr_fg, gr_bg );
					draw_semi_graph( bitmap, x * 8    , y * 16 + 12, d & 0x02, gr_fg, gr_bg );
					draw_semi_graph( bitmap, x * 8 + 4, y * 16 + 12, d & 0x01, gr_fg, gr_bg );
					break;
				case 0x03:		/* Block graphics mode */
					draw_block_graph( bitmap, x * 8, y * 16    , d >> 4 );
					draw_block_graph( bitmap, x * 8, y * 16 + 8, d & 0x0f );
					break;
				default:		/* Otherwise draw nothing? */
					break;
				}
			}
		}
	}

	/* Draw sprites if enabled */
	if ( scv_vram[0x1400] & 0x10 )
	{
		int i;

		for ( i = 0; i < 128; i++ )
		{
			UINT8 spr_y = scv_vram[ 0x1200 + i * 4 ] & 0xfe;
			UINT8 y_32 = scv_vram[ 0x1200 + i * 4 ] & 0x01;		/* Xx32 sprite */
			UINT8 clip = scv_vram[ 0x1201 + i * 4 ] >> 4;
			UINT8 col = scv_vram[ 0x1201 + i * 4 ] & 0x0f;
			UINT8 spr_x = scv_vram[ 0x1202 + i * 4 ] & 0xfe;
			UINT8 x_32 = scv_vram[ 0x1202 + i * 4 ] & 0x01;		/* 32xX sprite */
			UINT8 tile_idx = scv_vram[ 0x1203 + i * 4 ] & 0x7f;
			UINT8 half = scv_vram[ 0x1203 + i * 4] & 0x80;
			UINT8 left = 1;
			UINT8 right = 1;
			UINT8 top = 1;
			UINT8 bottom = 1;

			if ( !col )
				continue;

			if ( half )
			{
				if ( tile_idx & 0x40 )
				{
					if ( y_32 )
					{
						spr_y -= 8;
						top = 0;
						bottom = 1;
						y_32 = 0;
					}
					else
					{
						top = 1;
						bottom = 0;
					}
				}
				if ( x_32 )
				{
					spr_x -= 8;
					left = 0;
					right = 1;
					x_32 = 0;
				}
				else
				{
					left = 1;
					right = 0;
				}
			}

			/* Check if 2 color sprites are enabled */
			if ( ( scv_vram[0x1400] & 0x20 ) && ( i & 0x20 ) )
			{
				/* 2 color sprite handling */
				draw_sprite( bitmap, spr_x, spr_y, tile_idx, col, left, right, top, bottom, clip );
				if ( x_32 || y_32 )
				{
					static const UINT8 spr_2col_lut0[16] = { 0, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 1, 1 };
					static const UINT8 spr_2col_lut1[16] = { 0, 1, 8, 11, 2, 3, 10, 9, 4, 5, 12, 13, 6, 7, 14, 15 };

					draw_sprite( bitmap, spr_x, spr_y, tile_idx + 8 * x_32 + y_32, ( i & 0x40 ) ? spr_2col_lut1[col] : spr_2col_lut0[col], left, right, top, bottom, clip );
				}
			}
			else
			{
				/* regular sprite handling */
				draw_sprite( bitmap, spr_x, spr_y, tile_idx, col, left, right, top, bottom, clip );
				if ( x_32 )
				{
					draw_sprite( bitmap, spr_x + 16, spr_y, tile_idx + 8, col, 1, 1, top, bottom, clip );
				}
				if ( y_32 )
				{
					clip &= 0x07;
					draw_sprite( bitmap, spr_x, spr_y + 16, tile_idx + 1, col, left, right, 1, 1, clip );
					if ( x_32 )
					{
						draw_sprite( bitmap, spr_x + 16, spr_y + 16, tile_idx + 9, col, 1, 1, 1, 1, clip );
					}
				}
			}
		}
	}

	return 0;
}


static MACHINE_START( scv )
{
	scv_vb_timer = timer_alloc( machine, scv_vb_callback, NULL );
}


static MACHINE_RESET( scv )
{
	timer_adjust_oneshot( scv_vb_timer, video_screen_get_time_until_pos( machine->primary_screen, 0, 0 ), 0 );
}


static const UPD7810_CONFIG scv_cpu_config = { TYPE_7801, NULL };


static MACHINE_DRIVER_START( scv )
	MDRV_CPU_ADD( "maincpu", UPD7801, XTAL_4MHz )
	MDRV_CPU_PROGRAM_MAP( scv_mem )
	MDRV_CPU_IO_MAP( scv_io )
	MDRV_CPU_CONFIG( scv_cpu_config )

	MDRV_MACHINE_START( scv )
	MDRV_MACHINE_RESET( scv )

	MDRV_SCREEN_ADD( "screen", RASTER )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_RAW_PARAMS( XTAL_14_31818MHz/2, 456, 32, 224, 262, 32, 232 )	/* TODO: Verify */

	MDRV_PALETTE_LENGTH( 16 )
	MDRV_PALETTE_INIT( scv )

	/* Video chip is EPOCH TV-1 */
	MDRV_VIDEO_UPDATE( scv )

	/* Sound is generated by UPD1771C clocked at XTAL_6MHz */

	MDRV_CARTSLOT_ADD( "cart" )
	MDRV_CARTSLOT_EXTENSION_LIST( "bin" )
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD( scv_cart )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( scv_pal )
	MDRV_CPU_ADD( "maincpu", UPD7801, 3780000 ) 
	MDRV_CPU_PROGRAM_MAP( scv_mem )
	MDRV_CPU_IO_MAP( scv_io )
	MDRV_CPU_CONFIG( scv_cpu_config )

	MDRV_MACHINE_START( scv )
	MDRV_MACHINE_RESET( scv )

	MDRV_SCREEN_ADD( "screen", RASTER )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_RAW_PARAMS( XTAL_13_4MHz/2, 456, 32, 224, 342, 32, 232 )		/* TODO: Verify */

	MDRV_PALETTE_LENGTH( 16 )
	MDRV_PALETTE_INIT( scv )

	/* Video chip is EPOCH TV-1A */

	/* Sound is generated by UPD1771C clocked at XTAL_6MHz */
	MDRV_CARTSLOT_ADD( "cart" )
	MDRV_CARTSLOT_EXTENSION_LIST( "bin" )
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD( scv_cart )
MACHINE_DRIVER_END


/* The same bios is used in both the NTSC and PAL versions of the console */
ROM_START( scv )
	ROM_REGION( 0x1000, "maincpu", 0 )
	ROM_LOAD( "upd7801g.s01", 0, 0x1000, CRC(7ac06182) SHA1(6e89d1227581c76441a53d605f9e324185f1da33) )

	ROM_REGION( 0x8000, "cart", ROMREGION_ERASEFF )
ROM_END


ROM_START( scv_pal )
	ROM_REGION( 0x1000, "maincpu", 0 )
	ROM_LOAD( "upd7801g.s01", 0, 0x1000, CRC(7ac06182) SHA1(6e89d1227581c76441a53d605f9e324185f1da33) )

	ROM_REGION( 0x8000, "cart", ROMREGION_ERASEFF )
ROM_END


/*    YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT  INIT    COMPANY  FULLNAME                 FLAGS */
CONS( 199?, scv,     0,      0,      scv,     scv,   0,      "Epoch", "Super Cassette Vision", GAME_NOT_WORKING )
CONS( 199?, scv_pal, scv,    0,      scv_pal, scv,   0,      "Epoch", "Super Cassette Vision (PAL)", GAME_NOT_WORKING )

