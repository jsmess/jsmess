/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "includes/berzerk.h"
#include "machine/74181.h"
#include "machine/74153.h"

#define LS181_12C ( 0 )
#define LS181_10C ( 1 )

#define LS153_7A ( 0 )
#define LS153_8A ( 1 )
#define LS153_10A ( 2 )
#define LS153_11A ( 3 )
#define LS153_9A ( 4 )
#define LS153_11B ( 5 )
#define LS153_10B ( 6 )
#define LS153_9B ( 7 )
#define LS153_8B ( 8 )

static UINT8 latch_7c = 0x7f;
static UINT8 intercept = 0;


PALETTE_INIT( berzerk )
{
	int i;

	/* Simple 1-bit RGBI palette */
	for (i = 0; i < 16; i++)
	{
		int bk = (i & 8) ? 0x40 : 0x00;
		int r = (i & 1) ? 0xff : bk;
		int g = (i & 2) ? 0xff : bk;
		int b = (i & 4) ? 0xff : bk;

		palette_set_color(machine,i,r,g,b);
	}
}


static void shifters_init( void )
{
	TTL74153_config( LS153_7A, 0 );
	TTL74153_enable_w( LS153_7A, 0, 0 );
	TTL74153_enable_w( LS153_7A, 1, 0 );

	TTL74153_config( LS153_8A, 0 );
	TTL74153_enable_w( LS153_8A, 0, 0 );
	TTL74153_enable_w( LS153_8A, 1, 0 );

	TTL74153_config( LS153_10A, 0 );
	TTL74153_enable_w( LS153_10A, 0, 0 );
	TTL74153_enable_w( LS153_10A, 1, 0 );

	TTL74153_config( LS153_11A, 0 );
	TTL74153_enable_w( LS153_11A, 0, 0 );
	TTL74153_enable_w( LS153_11A, 1, 0 );

	TTL74153_config( LS153_9A, 0 );
	TTL74153_enable_w( LS153_9A, 0, 0 );
	TTL74153_enable_w( LS153_9A, 1, 0 );
}


static void shifters_setup( int a, int b )
{
	TTL74153_a_w( LS153_7A, a );
	TTL74153_b_w( LS153_7A, b );
	TTL74153_a_w( LS153_8A, a );
	TTL74153_b_w( LS153_8A, b );
	TTL74153_a_w( LS153_10A, a );
	TTL74153_b_w( LS153_10A, b );
	TTL74153_a_w( LS153_11A, a );
	TTL74153_b_w( LS153_11A, b );
	TTL74153_a_w( LS153_9A, a );
	TTL74153_b_w( LS153_9A, b );
}


static UINT16 shifters_update( UINT8 data )
{
	TTL74153_input_line_w( LS153_7A, 0, 3, ( data >> 0 ) & 1 );
	TTL74153_input_line_w( LS153_7A, 0, 2, ( data >> 2 ) & 1 );
	TTL74153_input_line_w( LS153_7A, 0, 1, ( data >> 4 ) & 1 );
	TTL74153_input_line_w( LS153_7A, 0, 0, ( data >> 6 ) & 1 );
	TTL74153_input_line_w( LS153_7A, 1, 3, ( data >> 1 ) & 1 );
	TTL74153_input_line_w( LS153_7A, 1, 2, ( data >> 3 ) & 1 );
	TTL74153_input_line_w( LS153_7A, 1, 1, ( data >> 5 ) & 1 );
	TTL74153_input_line_w( LS153_7A, 1, 0, ( data >> 7 ) & 1 );
	TTL74153_update( LS153_7A );

	TTL74153_input_line_w( LS153_8A, 0, 3, ( data >> 2 ) & 1 );
	TTL74153_input_line_w( LS153_8A, 0, 2, ( data >> 4 ) & 1 );
	TTL74153_input_line_w( LS153_8A, 0, 1, ( data >> 6 ) & 1 );
	TTL74153_input_line_w( LS153_8A, 0, 0, ( latch_7c >> 0 ) & 1 );
	TTL74153_input_line_w( LS153_8A, 1, 3, ( data >> 3 ) & 1 );
	TTL74153_input_line_w( LS153_8A, 1, 2, ( data >> 5 ) & 1 );
	TTL74153_input_line_w( LS153_8A, 1, 1, ( data >> 7 ) & 1 );
	TTL74153_input_line_w( LS153_8A, 1, 0, ( latch_7c >> 1 ) & 1 );
	TTL74153_update( LS153_8A );

	TTL74153_input_line_w( LS153_10A, 0, 3, ( data >> 4 ) & 1 );
	TTL74153_input_line_w( LS153_10A, 0, 2, ( data >> 6 ) & 1 );
	TTL74153_input_line_w( LS153_10A, 0, 1, ( latch_7c >> 0 ) & 1 );
	TTL74153_input_line_w( LS153_10A, 0, 0, ( latch_7c >> 2 ) & 1 );
	TTL74153_input_line_w( LS153_10A, 1, 3, ( data >> 5 ) & 1 );
	TTL74153_input_line_w( LS153_10A, 1, 2, ( data >> 7 ) & 1 );
	TTL74153_input_line_w( LS153_10A, 1, 1, ( latch_7c >> 1 ) & 1 );
	TTL74153_input_line_w( LS153_10A, 1, 0, ( latch_7c >> 3 ) & 1 );
	TTL74153_update( LS153_10A );

	TTL74153_input_line_w( LS153_11A, 0, 3, ( data >> 6 ) & 1 );
	TTL74153_input_line_w( LS153_11A, 0, 2, ( latch_7c >> 0 ) & 1 );
	TTL74153_input_line_w( LS153_11A, 0, 1, ( latch_7c >> 2 ) & 1 );
	TTL74153_input_line_w( LS153_11A, 0, 0, ( latch_7c >> 4 ) & 1 );
	TTL74153_input_line_w( LS153_11A, 1, 3, ( data >> 7 ) & 1 );
	TTL74153_input_line_w( LS153_11A, 1, 2, ( latch_7c >> 1 ) & 1 );
	TTL74153_input_line_w( LS153_11A, 1, 1, ( latch_7c >> 3 ) & 1 );
	TTL74153_input_line_w( LS153_11A, 1, 0, ( latch_7c >> 5 ) & 1 );
	TTL74153_update( LS153_11A );

	TTL74153_input_line_w( LS153_9A, 0, 3, ( latch_7c >> 0 ) & 1 );
	TTL74153_input_line_w( LS153_9A, 0, 2, ( latch_7c >> 2 ) & 1 );
	TTL74153_input_line_w( LS153_9A, 0, 1, ( latch_7c >> 4 ) & 1 );
	TTL74153_input_line_w( LS153_9A, 0, 0, ( latch_7c >> 6 ) & 1 );
	TTL74153_update( LS153_9A );

	return ( TTL74153_output_r( LS153_7A, 0 ) << 0 ) |
		( TTL74153_output_r( LS153_7A, 1 ) << 1 ) |
		( TTL74153_output_r( LS153_8A, 0 ) << 2 ) |
		( TTL74153_output_r( LS153_8A, 1 ) << 3 ) |
		( TTL74153_output_r( LS153_10A, 0 ) << 4 ) |
		( TTL74153_output_r( LS153_10A, 1 ) << 5 ) |
		( TTL74153_output_r( LS153_11A, 0 ) << 6 ) |
		( TTL74153_output_r( LS153_11A, 1 ) << 7 ) |
		( TTL74153_output_r( LS153_9A, 0 ) << 8 );
}


static void shifters_and_floppers_init( void )
{
	TTL74153_config( LS153_11B, 0 );
	TTL74153_enable_w( LS153_11B, 0, 0 );
	TTL74153_enable_w( LS153_11B, 1, 0 );

	TTL74153_config( LS153_10B, 0 );
	TTL74153_enable_w( LS153_10B, 0, 0 );
	TTL74153_enable_w( LS153_10B, 1, 0 );

	TTL74153_config( LS153_9B, 0 );
	TTL74153_enable_w( LS153_9B, 0, 0 );
	TTL74153_enable_w( LS153_9B, 1, 0 );

	TTL74153_config( LS153_8B, 0 );
	TTL74153_enable_w( LS153_8B, 0, 0 );
	TTL74153_enable_w( LS153_8B, 1, 0 );
}


static void shifters_and_floppers_setup( int a, int b )
{
	TTL74153_a_w( LS153_11B, a );
	TTL74153_b_w( LS153_11B, b );
	TTL74153_a_w( LS153_10B, a );
	TTL74153_b_w( LS153_10B, b );
	TTL74153_a_w( LS153_9B, a );
	TTL74153_b_w( LS153_9B, b );
	TTL74153_a_w( LS153_8B, a );
	TTL74153_b_w( LS153_8B, b );
}


static UINT8 shifters_and_floppers_update( UINT16 data )
{
	TTL74153_input_line_w( LS153_11B, 0, 3, ( data >> 0 ) & 1 );
	TTL74153_input_line_w( LS153_11B, 0, 2, ( data >> 1 ) & 1 );
	TTL74153_input_line_w( LS153_11B, 0, 1, ( data >> 7 ) & 1 );
	TTL74153_input_line_w( LS153_11B, 0, 0, ( data >> 8 ) & 1 );
	TTL74153_input_line_w( LS153_11B, 1, 3, ( data >> 1 ) & 1 );
	TTL74153_input_line_w( LS153_11B, 1, 2, ( data >> 2 ) & 1 );
	TTL74153_input_line_w( LS153_11B, 1, 1, ( data >> 6 ) & 1 );
	TTL74153_input_line_w( LS153_11B, 1, 0, ( data >> 7 ) & 1 );
	TTL74153_update( LS153_11B );

	TTL74153_input_line_w( LS153_10B, 0, 3, ( data >> 2 ) & 1 );
	TTL74153_input_line_w( LS153_10B, 0, 2, ( data >> 3 ) & 1 );
	TTL74153_input_line_w( LS153_10B, 0, 1, ( data >> 5 ) & 1 );
	TTL74153_input_line_w( LS153_10B, 0, 0, ( data >> 6 ) & 1 );
	TTL74153_input_line_w( LS153_10B, 1, 3, ( data >> 3 ) & 1 );
	TTL74153_input_line_w( LS153_10B, 1, 2, ( data >> 4 ) & 1 );
	TTL74153_input_line_w( LS153_10B, 1, 1, ( data >> 4 ) & 1 );
	TTL74153_input_line_w( LS153_10B, 1, 0, ( data >> 5 ) & 1 );
	TTL74153_update( LS153_10B );

	TTL74153_input_line_w( LS153_9B, 0, 3, ( data >> 4 ) & 1 );
	TTL74153_input_line_w( LS153_9B, 0, 2, ( data >> 5 ) & 1 );
	TTL74153_input_line_w( LS153_9B, 0, 1, ( data >> 3 ) & 1 );
	TTL74153_input_line_w( LS153_9B, 0, 0, ( data >> 4 ) & 1 );
	TTL74153_input_line_w( LS153_9B, 1, 3, ( data >> 5 ) & 1 );
	TTL74153_input_line_w( LS153_9B, 1, 2, ( data >> 6 ) & 1 );
	TTL74153_input_line_w( LS153_9B, 1, 1, ( data >> 2 ) & 1 );
	TTL74153_input_line_w( LS153_9B, 1, 0, ( data >> 3 ) & 1 );
	TTL74153_update( LS153_9B );

	TTL74153_input_line_w( LS153_8B, 0, 3, ( data >> 6 ) & 1 );
	TTL74153_input_line_w( LS153_8B, 0, 2, ( data >> 7 ) & 1 );
	TTL74153_input_line_w( LS153_8B, 0, 1, ( data >> 1 ) & 1 );
	TTL74153_input_line_w( LS153_8B, 0, 0, ( data >> 2 ) & 1 );
	TTL74153_input_line_w( LS153_8B, 1, 3, ( data >> 7 ) & 1 );
	TTL74153_input_line_w( LS153_8B, 1, 2, ( data >> 8 ) & 1 );
	TTL74153_input_line_w( LS153_8B, 1, 1, ( data >> 0 ) & 1 );
	TTL74153_input_line_w( LS153_8B, 1, 0, ( data >> 1 ) & 1 );
	TTL74153_update( LS153_8B );

	return ( TTL74153_output_r( LS153_11B, 0 ) << 0 ) |
		( TTL74153_output_r( LS153_11B, 1 ) << 1 ) |
		( TTL74153_output_r( LS153_10B, 0 ) << 2 ) |
		( TTL74153_output_r( LS153_10B, 1 ) << 3 ) |
		( TTL74153_output_r( LS153_9B, 0 ) << 4 ) |
		( TTL74153_output_r( LS153_9B, 1 ) << 5 ) |
		( TTL74153_output_r( LS153_8B, 0 ) << 6 ) |
		( TTL74153_output_r( LS153_8B, 1 ) << 7 );
}


static void alu_init( void )
{
	TTL74181_init( LS181_12C );
	TTL74181_write( LS181_12C, TTL74181_INPUT_M, 1, 1 );

	TTL74181_init( LS181_10C );
	TTL74181_write( LS181_10C, TTL74181_INPUT_M, 1, 1 );
}


static void alu_setup( int s )
{
	TTL74181_write( LS181_12C, TTL74181_INPUT_S0, 4, s );
	TTL74181_write( LS181_10C, TTL74181_INPUT_S0, 4, s );
}


static UINT8 alu_update( UINT8 a, UINT8 b )
{
	TTL74181_write( LS181_12C, TTL74181_INPUT_A0, 4, a >> 0 );
	TTL74181_write( LS181_10C, TTL74181_INPUT_A0, 4, a >> 4 );

	TTL74181_write( LS181_12C, TTL74181_INPUT_B0, 4, b >> 0 );
	TTL74181_write( LS181_10C, TTL74181_INPUT_B0, 4, b >> 4 );

	return ( TTL74181_read( LS181_12C, TTL74181_OUTPUT_F0, 4 ) << 0 ) |
		( TTL74181_read( LS181_10C, TTL74181_OUTPUT_F0, 4 ) << 4 );
}


VIDEO_START( berzerk )
{
	shifters_init();
	shifters_and_floppers_init();
	alu_init();

	return video_start_generic_bitmapped(machine);
}


INLINE void copy_byte(UINT8 x, UINT8 y, UINT8 data, UINT8 col)
{
	pen_t fore, back;

	fore  = Machine->pens[col >> 4];
	back  = Machine->pens[0];

	plot_pixel(tmpbitmap, x  , y, (data & 0x80) ? fore : back);
	plot_pixel(tmpbitmap, x+1, y, (data & 0x40) ? fore : back);
	plot_pixel(tmpbitmap, x+2, y, (data & 0x20) ? fore : back);
	plot_pixel(tmpbitmap, x+3, y, (data & 0x10) ? fore : back);

	fore  = Machine->pens[col & 0x0f];

	plot_pixel(tmpbitmap, x+4, y, (data & 0x08) ? fore : back);
	plot_pixel(tmpbitmap, x+5, y, (data & 0x04) ? fore : back);
	plot_pixel(tmpbitmap, x+6, y, (data & 0x02) ? fore : back);
	plot_pixel(tmpbitmap, x+7, y, (data & 0x01) ? fore : back);
}


WRITE8_HANDLER( berzerk_videoram_w )
{
	offs_t coloroffset;
	UINT8 x, y;

	videoram[offset] = data;

	/* Get location of color RAM for this offset */
	coloroffset = ((offset & 0xff80) >> 2) | (offset & 0x1f);

	y = offset >> 5;
	x = offset << 3;

	copy_byte(x, y, data, colorram[coloroffset]);
}


WRITE8_HANDLER( berzerk_colorram_w )
{
	int i;
	UINT8 x, y;

	colorram[offset] = data;

	/* Need to change the affected pixels' colors */
	y = (offset >> 3) & 0xfc;
	x = offset << 3;

	for (i = 0; i < 4; i++, y++)
	{
		UINT8 byte = videoram[(y << 5) | (x >> 3)];

		copy_byte(x, y, byte, data);
	}
}


WRITE8_HANDLER( berzerk_magicram_w )
{
	/* shift data towards lsb by 0, 2, 4 or 6. msb bits are filled by data from latch_7c. */
	UINT16 shifters_output = shifters_update( data );

	/* shift data towards lsb by 0 or 1 & optionally reverse the order of bits. */
	UINT8 shifters_and_floppers_output = shifters_and_floppers_update( shifters_output );

	UINT8 rd = videoram[ offset ];

	UINT8 alu_output = alu_update( shifters_and_floppers_output, rd );

	berzerk_videoram_w( offset, alu_output ^ 0xff );

	/* collision detection. */
	if ( ( shifters_and_floppers_output & rd ) != 0 )
	{
		intercept |= 0x80;
	}

	/* save data for next time. */
	latch_7c = data & 0x7f;
}


WRITE8_HANDLER( berzerk_magicram_control_w )
{
	shifters_setup( ( ~data >> 1 ) & 1, ( ~data >> 2 ) & 1 );
	shifters_and_floppers_setup( ( ~data >> 0 ) & 1, ( ~data >> 3 ) & 1 );
	alu_setup( data >> 4 );

	latch_7c = 0;
	intercept = 0;
}


READ8_HANDLER( berzerk_port_4e_r )
{
	return input_port_3_r(0) | intercept;
}
