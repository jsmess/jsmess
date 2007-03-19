/*
 * 74181
 *
 * 4-bit arithmetic Logic Unit
 *
 */

#include "driver.h"
#include "74181.h"

struct TTL74181_chip
{
	int inputs[ TTL74181_INPUT_TOTAL ];
	int outputs[ TTL74181_OUTPUT_TOTAL ];
	int dirty;
};

static struct TTL74181_chip *chips[ TTL74181_MAX_CHIPS ];

void TTL74181_init( int chip )
{
	struct TTL74181_chip *c = auto_malloc( sizeof( struct TTL74181_chip ) );

	memset( c->inputs, 0, sizeof( c->inputs ) );
	memset( c->outputs, 0, sizeof( c->outputs ) );
	c->dirty = 1;

	state_save_register_item_array( "TTL74181", chip, c->inputs );
	state_save_register_item_array( "TTL74181", chip, c->outputs );
	state_save_register_item( "TTL74181", chip, c->dirty );

	chips[ chip ] = c;
}

static void TTL74181_update( int chip )
{
	struct TTL74181_chip *c = chips[ chip ];

	int a0 = c->inputs[ TTL74181_INPUT_A0 ];
	int a1 = c->inputs[ TTL74181_INPUT_A1 ];
	int a2 = c->inputs[ TTL74181_INPUT_A2 ];
	int a3 = c->inputs[ TTL74181_INPUT_A3 ];

	int b0 = c->inputs[ TTL74181_INPUT_B0 ];
	int b1 = c->inputs[ TTL74181_INPUT_B1 ];
	int b2 = c->inputs[ TTL74181_INPUT_B2 ];
	int b3 = c->inputs[ TTL74181_INPUT_B3 ];

	int s0 = c->inputs[ TTL74181_INPUT_S0 ];
	int s1 = c->inputs[ TTL74181_INPUT_S1 ];
	int s2 = c->inputs[ TTL74181_INPUT_S2 ];
	int s3 = c->inputs[ TTL74181_INPUT_S3 ];

	int cp = c->inputs[ TTL74181_INPUT_C ];
	int mp = ~c->inputs[ TTL74181_INPUT_M ];

	int ap0 = ~( a0 | ( b0 & s0 ) | ( s1 & ~b0 ) );
	int bp0 = ~( ( ~b0 & s2 & a0 ) | ( a0 & b0 & s3 ) );
	int ap1 = ~( a1 | ( b1 & s0 ) | ( s1 & ~b1 ) );
	int bp1 = ~( ( ~b1 & s2 & a1 ) | ( a1 & b1 & s3 ) );
	int ap2 = ~( a2 | ( b2 & s0 ) | ( s1 & ~b2 ) );
	int bp2 = ~( ( ~b2 & s2 & a2 ) | ( a2 & b2 & s3 ) );
	int ap3 = ~( a3 | ( b3 & s0 ) | ( s1 & ~b3 ) );
	int bp3 = ~( ( ~b3 & s2 & a3 ) | ( a3 & b3 & s3 ) );

	int fp0 = ( ( ~( cp & mp ) ) ^ ( ~ap0 & bp0 ) ) & 1;
	int fp1 = ( ( ~( ( mp & ap0 ) | ( mp & bp0 & cp ) ) ) ^ ( ~ap1 & bp1 ) ) & 1;
	int fp2 = ( ( ~( ( mp & ap1 ) | ( mp & ap0 & bp1 ) | ( mp & cp & bp0 & bp1 ) ) ) ^ ( ~ap2 & bp2 ) ) & 1;
	int fp3 = ( ( ~( ( mp & ap2 ) | ( mp & ap1 & bp2 ) | ( mp & ap0 & bp1 & bp2 ) | ( mp & cp & bp0 & bp1 & bp2 ) ) ) ^ ( ~ap3 & bp3 ) ) & 1;

	int aeqbp = fp0 & fp1 & fp2 & fp3;
	int pp = ( ~( bp0 & bp1 & bp2 & bp3 ) ) & 1;
	int gp = ( ~( ( ap0 & bp1 & bp2 & bp3 ) | ( ap1 & bp2 & bp3 ) | ( ap2 & bp3 ) | ap3 ) ) & 1;
	int cn4p = ( ( ~( cp & bp0 & bp1 & bp2 & bp3 ) ) | gp ) & 1;

	c->outputs[ TTL74181_OUTPUT_F0 ] = fp0;
	c->outputs[ TTL74181_OUTPUT_F1 ] = fp1;
	c->outputs[ TTL74181_OUTPUT_F2 ] = fp2;
	c->outputs[ TTL74181_OUTPUT_F3 ] = fp3;
	c->outputs[ TTL74181_OUTPUT_AEQB ] = aeqbp;
	c->outputs[ TTL74181_OUTPUT_P ] = pp;
	c->outputs[ TTL74181_OUTPUT_G ] = gp;
	c->outputs[ TTL74181_OUTPUT_CN4 ] = cn4p;
}

void TTL74181_write( int chip, int startline, int lines, int data )
{
	int line;
	struct TTL74181_chip *c;
	if( chip >= TTL74181_MAX_CHIPS )
	{
		logerror( "%08x: TTL74181_write( %d, %d, %d, %d ) chip out of range\n", activecpu_get_pc(), chip, startline, lines, data );
		return;
	}
	if( startline + lines > TTL74181_INPUT_TOTAL )
	{
		logerror( "%08x: TTL74181_read( %d, %d, %d, %d ) line out of range\n", activecpu_get_pc(), chip, startline, lines, data );
		return;
	}
	c = chips[ chip ];
	if( c == NULL )
	{
		logerror( "%08x: TTL74181_write( %d, %d, %d, %d ) chip not initialised\n", activecpu_get_pc(), chip, startline, lines, data );
		return;
	}

	for( line = 0; line < lines; line++ )
	{
		int input = data >> line;
		if( c->inputs[ startline + line ] != input )
		{
			c->inputs[ startline + line ] = input;
			c->dirty = 1;
		}
	}
}

int TTL74181_read( int chip, int startline, int lines )
{
	int line;
	int data;
	struct TTL74181_chip *c;
	if( chip >= TTL74181_MAX_CHIPS )
	{
		logerror( "%08x: TTL74181_read( %d, %d, %d ) chip out of range\n", activecpu_get_pc(), chip, startline, lines );
		return 0;
	}
	if( startline + lines > TTL74181_OUTPUT_TOTAL )
	{
		logerror( "%08x: TTL74181_read( %d, %d, %d ) line out of range\n", activecpu_get_pc(), chip, startline, lines );
		return 0;
	}
	c = chips[ chip ];
	if( c == NULL )
	{
		logerror( "%08x: TTL74181_read( %d, %d, %d ) chip not initialised\n", activecpu_get_pc(), chip, startline, lines );
		return 0;
	}

	if( c->dirty )
	{
		TTL74181_update( chip );
		c->dirty = 0;
	}

	data = 0;
	for( line = 0; line < lines; line++ )
	{
		data |= c->outputs[ startline + line ] << line;
	}
	return data;
}
