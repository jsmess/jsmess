/***************************************************************************

Mad Alien
(c) 1980 Data East Corporation

Driver by Norbert Kehrer (February 2004).

****************************************************************************

Hardware:
---------

Main CPU    6502
Sound CPU   6502
Sound chip  AY-3-8910


Memory map of main board:
-------------------------

$0000 - $03ff   Program RAM
$6000 - $63ff   Video RAM
$6800 - $7fff   Character generator RAM
$8004       Flip screen in cocktail mode (?)
$8006       Write: send command to sound board, read: input from sound board
$8008       Write: set counter for shifter circuitry, read: low byte of shift reg. after shift operation
$8009       Write: set data to be shifted, read: high byte of shift reg. after shift operation
$8008       Video register (control of headlight, color, and landscape)
$800d       Background horiz. scroll register left part
$800e       Background horiz. scroll register right part
$800f       Background vertical scroll register
$9000       Input port 1: player 1 controls
$9001       DIP switch port
$9002       Input port 2: player 2 controls
$c000 - $ffff   Program ROM

Main board interrupts: NMI triggered, when coin is inserted


Memory map of sound board:
--------------------------

$0000 - $03ff   Sound board program RAM
$6500       Sound board register, just to write in someting (?)
$6800       Sound board register, just to write in and read out (?)
$8000       Write: AY-3-8910 control port, read: input from master CPU
$8001       AY-3-8910 data port
$8006       Output to master CPU
$f800 - $ffff   Sound board program ROM

Sound board interrupts: NMI triggered by VBlank signal


Input ports:
------------

Input port 1, mapped to memory address $9000:
7654 3210
1          Unused
 1         Start 2 players game
  1        Start 1 player game
   1       Player 1 fire button
     1     Player 1 down (not used in Mad Alien)
      1    Player 1 up (not used in Mad Alien)
       1   Player 1 left
        1  Player 1 right

DIP switch port, mapped to memory address $9001:
7654 3210
1          VBlank (0 = off, 1 = on)
 1         Game screen (0 = prevent turning, 1 = to turn) (?)
  00       Bonus points (0 = 3000, 1 = 7000, 2 = 5000, 3 = nil)
     00    Game charge (0 = 1 coin/1 play, 1 = 1 coin/2 plays, 2 and 3 = 2 coins 1 play)
       00  Number of cars (0 = 3 cars, 1 = 4 cars, 2 = 5 cars, 3 = 6cars)

Input port 2, mapped to memory address $9002:
7654 3210
111        Unused
   1       Player 2 fire button
     1     Player 2 down (not used in Mad Alien)
      1    Player 2 up (not used in Mad Alien)
       1   Player 2 left
        1  Player 2 right

***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"

static tilemap *fg_tilemap, *bg_tilemap_l, *bg_tilemap_r;

static rectangle bg_tilemap_l_clip;
static rectangle bg_tilemap_r_clip;

static mame_bitmap *headlight_bitmap, *flip_bitmap;

static UINT8 *madalien_videoram;
static UINT8 *madalien_charram;
static UINT8 *madalien_bgram;
static UINT8 *madalien_shift_data;

static UINT8 madalien_scroll_v;
static UINT8 madalien_scroll_l;
static UINT8 madalien_scroll_r;
static UINT8 madalien_scroll_light;

static UINT8 madalien_shift_counter;
static UINT8 madalien_shift_reg_lo;
static UINT8 madalien_shift_reg_hi;

static UINT8 madalien_video_register;

static UINT8 madalien_bg_map_selector;

static int madalien_select_color_1;
static int madalien_select_color_2;
static int madalien_swap_colors;

static int madalien_headlight_on;

static int madalien_flip_screen;

static int madalien_headlight_source[128][128];

static UINT8 madalien_sound_reg;


PALETTE_INIT( madalien )
{
	int i, j, n, bit0, bit1, r, g, b;

	n = machine->drv->total_colors / 2;

	for (i = 0; i < n; i++)
	{
		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		r = 0x40 * bit0 + 0x80 * bit1;
		/* green component */
		bit0 = (color_prom[i] >> 2) & 0x01;
		bit1 = (color_prom[i] >> 3) & 0x01;
		g = 0x40 * bit0 + 0x80 * bit1;
		/* blue component */
		bit0 = (color_prom[i] >> 4) & 0x01;
		bit1 = (color_prom[i] >> 5) & 0x01;
		b = 0x40 * bit0 + 0x80 * bit1;

		palette_set_color(machine, i, MAKE_RGB(r, g, b));

		/* Colors for bits 1 and 2 swapped */
		j = i;
		switch (j & 6) {
			case 2:
				j &= 0xf9;
				j |= 0x04;
				break;
			case 4:
				j &= 0xf9;
				j |= 0x02;
				break;
		};
		palette_set_color(machine, j+n, MAKE_RGB(r, g, b));
	}
}

static TILE_GET_INFO( get_fg_tile_info )
{
	int code, color;

	code = madalien_videoram[tile_index];
	color = 2 * madalien_select_color_1;

	SET_TILE_INFO(0, code, color, 0);
}

static TILE_GET_INFO( get_bg_tile_info_l )
{
	int x, y, code, color, bg_base;

	UINT8 *bgrom = memory_region(REGION_GFX2);

	x = tile_index & 0x0f;
	y = (tile_index / 16) & 0x0f;

	bg_base = madalien_bg_map_selector * 0x0100;

	if (x < 8)
		code = bgrom[bg_base + 8*y + x];
	else
		code = 16;

	color = madalien_swap_colors * 4;

	SET_TILE_INFO(1, code, color, 0);
}

static TILE_GET_INFO( get_bg_tile_info_r )
{
	int x, y, code, color, bg_base;

	UINT8 *bgrom = memory_region(REGION_GFX2);

	x = tile_index & 0x0f;
	y = (tile_index / 16) & 0x0f;

	bg_base = madalien_bg_map_selector * 0x0100;

	if (x < 8)
		code = bgrom[bg_base + 0x80 + 8*y + x];
	else
		code = 16;

	color = madalien_swap_colors * 4;

	SET_TILE_INFO(1, code, color, 0);
}




VIDEO_START( madalien )
{
	int x, y, data1, data2, bit;
	const UINT8 *headlight_rom;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_cols_flip_x,
		TILEMAP_TYPE_PEN, 8, 8, 32, 32);

	bg_tilemap_l = tilemap_create(get_bg_tile_info_l, tilemap_scan_cols_flip_x,
		TILEMAP_TYPE_PEN, 16, 16, 16, 16);

	bg_tilemap_r = tilemap_create(get_bg_tile_info_r, tilemap_scan_cols_flip_x,
		TILEMAP_TYPE_PEN, 16, 16, 16, 16);

	tilemap_set_transparent_pen( fg_tilemap, 0 );

	bg_tilemap_l_clip = machine->screen[0].visarea;
	bg_tilemap_l_clip.max_y = machine->screen[0].height / 2;

	bg_tilemap_r_clip = machine->screen[0].visarea;
	bg_tilemap_r_clip.min_y = machine->screen[0].height / 2;

	tilemap_set_flip(bg_tilemap_r, TILEMAP_FLIPY);

	headlight_bitmap = auto_bitmap_alloc(128, 128, machine->screen[0].format);

	flip_bitmap = auto_bitmap_alloc(machine->screen[0].width,machine->screen[0].height,machine->screen[0].format);

	madalien_bgram = auto_malloc(0x1000);	/* ficticiuos background RAM for empty tile */

	memset(madalien_bgram, 0, 0x1000);

	madalien_scroll_v = madalien_scroll_l = madalien_scroll_r = 0;

	madalien_shift_data = memory_region( REGION_GFX4 );

	/* Generate headlight shape data: */
	headlight_rom = memory_region( REGION_GFX3 );
	for( x=0; x<64; x++ )
		for( y=0; y<64; y+=8 )
		{
			data1 = headlight_rom[x*16 + (y/8)];
			data2 = headlight_rom[x*16 + (y/8) + 8];
			for( bit=0; bit<8; bit++ )
			{
				madalien_headlight_source[127-(y+bit)][x] = (data1 & 0x01);
				madalien_headlight_source[63-(y+bit)][x] = (data2 & 0x01);
				data1 >>= 1;
				data2 >>= 1;
			}
		}
	for( x=0; x<64; x++ )
		for( y=0; y<128; y++ )
			madalien_headlight_source[y][64+x] = madalien_headlight_source[y][63-x];
}



VIDEO_UPDATE( madalien )
{
	rectangle clip;
	int i, yh, x, y, xp, yp;

	for (i=0; i<256; i++)
		decodechar(machine->gfx[0], i, madalien_charram, machine->drv->gfxdecodeinfo[0].gfxlayout);

	decodechar(machine->gfx[1], 16, madalien_bgram, machine->drv->gfxdecodeinfo[1].gfxlayout); /* empty tile */

	tilemap_set_scrolly( bg_tilemap_l, 0, madalien_scroll_l );
	tilemap_set_scrollx( bg_tilemap_l, 0, madalien_scroll_v );

	tilemap_set_scrolly( bg_tilemap_r, 0, madalien_scroll_r );
	tilemap_set_scrollx( bg_tilemap_r, 0, madalien_scroll_v );

	clip = bg_tilemap_l_clip;
	sect_rect(&clip, cliprect);
	tilemap_draw(bitmap, &clip, bg_tilemap_l, 0, 0);

	clip = bg_tilemap_r_clip;
	sect_rect(&clip, cliprect);
	tilemap_draw(bitmap, &clip, bg_tilemap_r, 0, 0);

	tilemap_draw(bitmap, &machine->screen[0].visarea, fg_tilemap, 0, 0);

	/* Draw headlight area using lighter colors: */
	if (madalien_headlight_on && (madalien_bg_map_selector & 1))
	{
		yh = (256 - madalien_scroll_light) & 0xff;
		if (yh >= 192)
			yh = -(255-yh);

		copybitmap(
			headlight_bitmap,	/* dest */
			bitmap,			/* source */
			0, 0, 			/* flipx, flipy */
			0, -yh,			/* scroll x, scroll y */
			cliprect, 		/* clip */
			TRANSPARENCY_NONE, 0	);

		for( x=0; x<128; x++ )
			for( y=0; y<128; y++ )
				if (madalien_headlight_source[x][y])
				{
					xp = x;
					yp = yh + y;
					if( xp >= machine->screen[0].visarea.min_x &&
					    yp >= machine->screen[0].visarea.min_y &&
					    xp <= machine->screen[0].visarea.max_x &&
					    yp <= machine->screen[0].visarea.max_y )
					{
						int color = *BITMAP_ADDR16(headlight_bitmap, y, x);
						*BITMAP_ADDR16(bitmap, yp, xp) = machine->pens[color+8];
					}
				}
	};

	/* Flip screen (cocktail mode): */
	if (madalien_flip_screen) {
		copybitmap(flip_bitmap, bitmap, 1, 1, 0, 0, &machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
		copybitmap(bitmap, flip_bitmap, 0, 0, 0, 0, &machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
	};
	return 0;
}



INTERRUPT_GEN( madalien_interrupt )
{
	static int coin;

	int port = readinputport(3) & 0x01;

	if (port != 0x00)    /* Coin insertion triggers an NMI */
	{
		if (coin == 0)
		{
			coin = 1;
			cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
		}
	}
	else
		coin = 0;
}


READ8_HANDLER( madalien_shift_reg_lo_r )
{
	return madalien_shift_reg_lo;
}

READ8_HANDLER( madalien_shift_reg_hi_r )
{
	return madalien_shift_reg_hi;
}

READ8_HANDLER( madalien_videoram_r )
{
	return madalien_videoram[offset];
}

READ8_HANDLER( madalien_charram_r )
{
	return madalien_charram[offset];
}

WRITE8_HANDLER(madalien_video_register_w)
{
	int bit0, bit2, bit3;

	if ( madalien_video_register != data )
	{
		madalien_video_register = data;

		bit0 = data & 0x01;
		bit2 = ((data & 0x04) >> 2);
		bit3 = ((data & 0x08) >> 3);

		/* Headlight on, if bit 0 is on */
		madalien_headlight_on = bit0;

		/* Swap color bits 1 and 2 of background */
		if (bit2 != madalien_swap_colors)
		{
			madalien_swap_colors = bit2;
			tilemap_mark_all_tiles_dirty( fg_tilemap );
		};

		/* Select map (0=landscape A, 1=tunnel, 2=landscape B, 3=tunnel) */
		madalien_bg_map_selector &= 0x01;
		madalien_bg_map_selector |= (bit3 << 1);

		tilemap_mark_all_tiles_dirty( bg_tilemap_l );
		tilemap_mark_all_tiles_dirty( bg_tilemap_r );
	};
}

WRITE8_HANDLER( madalien_charram_w )
{
	if (madalien_charram[offset] != data)
	{
		madalien_charram[offset] = data;
		tilemap_mark_all_tiles_dirty(fg_tilemap);
	}
}

WRITE8_HANDLER( madalien_scroll_light_w )
{
	madalien_scroll_light = data;
}

WRITE8_HANDLER( madalien_scroll_v_w )
{
	madalien_scroll_v = (255 - (data & 0xfc));

	madalien_bg_map_selector &= 0x02;
	madalien_bg_map_selector |= (data & 0x01);

	madalien_select_color_1 = (data & 0x01);

	tilemap_mark_all_tiles_dirty( bg_tilemap_l );
	tilemap_mark_all_tiles_dirty( bg_tilemap_r );
	tilemap_mark_all_tiles_dirty( fg_tilemap );
}

WRITE8_HANDLER( madalien_scroll_l_w )
{
	madalien_scroll_l = data;
}

WRITE8_HANDLER( madalien_scroll_r_w )
{
	madalien_scroll_r = data;
}

WRITE8_HANDLER( madalien_shift_counter_w )
{
	madalien_shift_counter = data & 0x07;
}

static UINT8 reverse_bits( int x )	/* bit reversal by wiring in Mad Alien hardware */
{
	int bit, n;
	n = 0;
	for (bit=0; bit<8; bit++)
		if (x & (1 << bit))
			n |= (1 << (7-bit));
	return n;
}

static UINT8 swap_bits( int x )	/* special bit swap by wiring in Mad Alien hardware */
{
	int n = 0;
	if (x & 0x40) n |= 0x01;
	if (x & 0x20) n |= 0x02;
	if (x & 0x10) n |= 0x04;
	if (x & 0x08) n |= 0x08;
	if (x & 0x04) n |= 0x10;
	if (x & 0x02) n |= 0x20;
	if (x & 0x01) n |= 0x40;
	return n;
}

WRITE8_HANDLER( madalien_shift_reg_w )
{
	int rom_addr_0, rom_addr_1;
	rom_addr_0 = madalien_shift_counter * 256 + data;
	rom_addr_1 = ((madalien_shift_counter^0x07) & 0x07) * 256 + reverse_bits(data);
	madalien_shift_reg_lo = madalien_shift_data[rom_addr_0];
	madalien_shift_reg_hi = swap_bits( madalien_shift_data[rom_addr_1] );
}

WRITE8_HANDLER( madalien_videoram_w )
{
	madalien_videoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap, offset);
}

WRITE8_HANDLER( madalien_flip_screen_w )
{
	if (readinputport(1) & 0x40)	/* hack for screen flipping in cocktail mode - main board schematics needed */
		madalien_flip_screen = (data & 1);
	else
		madalien_flip_screen = 0;
}

WRITE8_HANDLER( madalien_sound_command_w )
{
	soundlatch_w(offset, data);
	cpunum_set_input_line(1, 0, HOLD_LINE);
}

WRITE8_HANDLER( madalien_soundreg_w )
{
	madalien_sound_reg = data;
}

READ8_HANDLER( madalien_soundreg_r )
{
	return madalien_sound_reg;
}


static const gfx_layout charlayout_memory =
{
	8,8,    /* 8*8 characters */
	256,	/* 256 characters */
	3,      /* 3 bits per pixel */
	{ 2*256*8*8, 256*8*8, 0 },  /* the 3 bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static const gfx_layout tilelayout =
{
	16,16,  /* 16*16 tiles */
	16+1,	/* 16 tiles + 1 empty tile */
	3,      /* 3 bits per pixel */
	{ 4*16*16*16+4, 2*16*16*16+4, 4 },
	{ 3*16*8+0, 3*16*8+1, 3*16*8+2, 3*16*8+3, 2*16*8+0, 2*16*8+1, 2*16*8+2, 2*16*8+3,
	  16*8+0, 16*8+1, 16*8+2, 16*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8    /* every tile takes 64 consecutive bytes */
};


static const gfx_decode madalien_gfxdecodeinfo[] =
{
	{ 0, 0, &charlayout_memory,	0, 8 }, /* characters (the game dynamically modifies them) */
	{ REGION_GFX1, 0, &tilelayout,	0, 8 },	/* background tiles */
	{ -1 }
};


static ADDRESS_MAP_START( madalien_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_READ(MRA8_RAM)			/* Program RAM */
	AM_RANGE(0x6000, 0x63ff) AM_READ(madalien_videoram_r)		/* Video RAM   */
	AM_RANGE(0x6800, 0x7fff) AM_READ(madalien_charram_r)		/* Character generator RAM */
	AM_RANGE(0x8006, 0x8006) AM_READ(soundlatch2_r)			/* Input from sound board */
	AM_RANGE(0x8008, 0x8008) AM_READ(madalien_shift_reg_lo_r)	/* Low byte of shift reg. after shift operation */
	AM_RANGE(0x8009, 0x8009) AM_READ(madalien_shift_reg_hi_r)	/* Low byte of shift reg. after shift operation */
	AM_RANGE(0x9000, 0x9000) AM_READ(input_port_0_r)    		/* Input ports */
	AM_RANGE(0x9001, 0x9001) AM_READ(input_port_1_r)
	AM_RANGE(0x9002, 0x9002) AM_READ(input_port_2_r)
	AM_RANGE(0xb000, 0xffff) AM_READ(MRA8_ROM)			/* Program ROM */
ADDRESS_MAP_END



static ADDRESS_MAP_START( madalien_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_WRITE(MWA8_RAM)						/* Normal RAM */
	AM_RANGE(0x6000, 0x63ff) AM_WRITE(madalien_videoram_w) AM_BASE(&madalien_videoram)	/* Video RAM   */
	AM_RANGE(0x6800, 0x7fff) AM_WRITE(madalien_charram_w)  AM_BASE(&madalien_charram)	/* Character generator RAM */
	AM_RANGE(0x8004, 0x8004) AM_WRITE(madalien_flip_screen_w)		/* Flip screen in cocktail mode */
	AM_RANGE(0x8006, 0x8006) AM_WRITE(madalien_sound_command_w)		/* Command to sound board */
	AM_RANGE(0x8008, 0x8008) AM_WRITE(madalien_shift_counter_w) 		/* Set counter for shifter circuitry */
	AM_RANGE(0x8009, 0x8009) AM_WRITE(madalien_shift_reg_w) 		/* Put data to be shifted */
	AM_RANGE(0x800b, 0x800b) AM_WRITE(madalien_video_register_w)		/* Video register (light/color/landscape) */
	AM_RANGE(0x800c, 0x800c) AM_WRITE(madalien_scroll_light_w)		/* Car headlight horiz. scroll register */
	AM_RANGE(0x800d, 0x800d) AM_WRITE(madalien_scroll_l_w)		/* Background horiz. scroll register left part */
	AM_RANGE(0x800e, 0x800e) AM_WRITE(madalien_scroll_r_w)		/* Background horiz. scroll register right part */
	AM_RANGE(0x800f, 0x800f) AM_WRITE(madalien_scroll_v_w) 		/* Background vertical scroll register */
	AM_RANGE(0xb000, 0xffff) AM_WRITE(MWA8_ROM)			/* Program ROM */
ADDRESS_MAP_END


static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_READ(MRA8_RAM)		/* Sound board RAM */
	AM_RANGE(0x6800, 0x6800) AM_READ(madalien_soundreg_r)	/* Sound board register, just to write in and read out */
	AM_RANGE(0x8000, 0x8000) AM_READ(soundlatch_r)		/* Sound board input from master CPU */
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_ROM)		/* Sound board program ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_WRITE(MWA8_RAM)			/* Sound board RAM */
	AM_RANGE(0x6500, 0x6500) AM_WRITE(madalien_soundreg_w)		/* Sound board register */
	AM_RANGE(0x8000, 0x8000) AM_WRITE(AY8910_control_port_0_w)	/* AY-3-8910 control port */
	AM_RANGE(0x8001, 0x8001) AM_WRITE(AY8910_write_port_0_w)	/* AY-3-8910 data port */
	AM_RANGE(0x8006, 0x8006) AM_WRITE(soundlatch2_w)		/* Sound board output to master CPU */
	AM_RANGE(0xf800, 0xffff) AM_WRITE(MWA8_ROM)			/* Sound board program ROM */
ADDRESS_MAP_END



INPUT_PORTS_START( madalien )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x30, 0x30, "Bonus_points" )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x20, "5000" )
	PORT_DIPSETTING(    0x10, "7000"  )
	PORT_DIPSETTING(    0x30, "nil"  )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* Fake port or coin: coin insertion triggers an NMI */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



static MACHINE_DRIVER_START( madalien )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 750000)    /* 750 kHz ? */
	MDRV_CPU_PROGRAM_MAP(madalien_readmem, madalien_writemem)
	MDRV_CPU_VBLANK_INT(madalien_interrupt, 1)

	MDRV_CPU_ADD_TAG("sound", M6502, 500000)   /* 500 kHz ? */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem, sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(USEC_TO_SUBSECONDS(3072))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(madalien_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2*32)
	MDRV_PALETTE_INIT(madalien)
	MDRV_VIDEO_START(madalien)
	MDRV_VIDEO_UPDATE(madalien)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.23)
MACHINE_DRIVER_END


ROM_START( madalien )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) 			 /* 64k for 6502 code of main CPU */
	ROM_LOAD( "m7",	0xc000, 0x0800, CRC(4d12f89d) SHA1(e155f9135bc2bea56e211052f2b74d25e76308c8) )
	ROM_LOAD( "m6",	0xc800, 0x0800, CRC(1bc4a57b) SHA1(02252b868d0c07c0a18240e9d831c303cdcfa9a6) )
	ROM_LOAD( "m5",	0xd000, 0x0800, CRC(8db99572) SHA1(f8cf22f8c134b47756b7f02c5ca0217100466744) )
	ROM_LOAD( "m4",	0xd800, 0x0800, CRC(fba671af) SHA1(dd74bd357c82d525948d836a7f860bbb3182c825) )
	ROM_LOAD( "m3",	0xe000, 0x0800, CRC(1aad640d) SHA1(9ace7d2c5ef9e789c2b8cc65420b19ce72cd95fa) )
	ROM_LOAD( "m2",	0xe800, 0x0800, CRC(cbd533a0) SHA1(d3be81fb9ba40e30e5ff0171efd656b11dd20f2b) )
	ROM_LOAD( "m1",	0xf000, 0x0800, CRC(ad654b1d) SHA1(f8b365dae3801e97e04a10018a790d3bdb5d9439) )
	ROM_LOAD( "m0",	0xf800, 0x0800, CRC(cf7aa787) SHA1(f852cc806ecc582661582326747974a14f50174a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     		/* 64k for 6502 code of audio CPU */
	ROM_LOAD( "m8", 0xf800, 0x0400, CRC(cfd19dab) SHA1(566dc84ffe9bcaeb112250a9e1882bf62f47b579) )
	ROM_LOAD( "m9", 0xfc00, 0x0400, CRC(48f30f24) SHA1(9c0bf6e43b143d6af1ebb9dad2bdc2b53eb2e48e) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )   /* Background tiles */
	ROM_LOAD( "mc", 0x0000, 0x0400, CRC(2daadfb7) SHA1(8be084a39b256e538fd57111e92d47115cb142cd) )
	ROM_LOAD( "md", 0x0400, 0x0400, CRC(3ee1287a) SHA1(33bc59a8d09d22f3db80f881c2f37aa788718138) )
	ROM_LOAD( "me", 0x0800, 0x0400, CRC(45a5c201) SHA1(ac600afeabf494634c3189d8e96644bd0deb45f3) )

	ROM_REGION( 0x0400, REGION_GFX2, 0 )  			/* Background tile maps */
	ROM_LOAD( "mf", 0x0000, 0x0400, CRC(e9cba773) SHA1(356c7edb1b412a9e04f0747e780c945af8791c55) )

	ROM_REGION( 0x0400, REGION_GFX3, 0 )			/* Car headlight */
	ROM_LOAD( "ma", 0x0000, 0x0400, CRC(aab16446) SHA1(d2342627cc2766004343f27515d8a7989d5fe932) )

	ROM_REGION( 0x0800, REGION_GFX4, 0 )			/* Shifting data */
	ROM_LOAD( "mb", 0x0000, 0x0800, CRC(cb801e49) SHA1(7444c4af7cf07e5fdc54044d62ea4fcb201b2b8b) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 ) 			/* Color PROM */
	ROM_LOAD( "mg",	0x0000, 0x0020, CRC(3395b31f) SHA1(26235fb448a4180c58f0887e53a29c17857b3b34) )
ROM_END


ROM_START( madalina )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) 			/* 64k for 6502 code of main CPU */
	ROM_LOAD( "2716.4c",        0xb000, 0x0800, CRC(90be68af) SHA1(472ccfd2e04d6d49be47d919cba0c55d850b2887) )
	ROM_LOAD( "2716.4e",        0xb800, 0x0800, CRC(aba10cbb) SHA1(6ca213ded8ed7f4f310ab5ae25220cf867dd1d00) )
	ROM_LOAD( "2716.3f",        0xc000, 0x0800, CRC(c3af484c) SHA1(c3667526d3b5aeee68823f92826053e657512851) )
	ROM_LOAD( "2716.3j",        0xc800, 0x0800, CRC(78ca5a87) SHA1(729d69ee63c710241a098471e9769063dfe8ef1e) )
	ROM_LOAD( "2716.3k",        0xd000, 0x0800, CRC(070e81ea) SHA1(006831f4bf289812e4e87a3ece7885e8b901f2f5) )
	ROM_LOAD( "2716.3l",        0xd800, 0x0800, CRC(98225cb0) SHA1(ca74f5e33fa9116215b03abadd5d09840c04fb0b) )
	ROM_LOAD( "2716.4f",        0xe000, 0x0800, CRC(52fea0fc) SHA1(443fd859daf4279d5976256a4b1c970b520661a2) )
	ROM_LOAD( "2716.4j",        0xe800, 0x0800, CRC(dba6c4f6) SHA1(51f815fc7eb99a05eee6204de2d4cad1734adc52) )
	ROM_LOAD( "2716.4k",        0xf000, 0x0800, CRC(06991af6) SHA1(19112306529721222b6e1c07920348c263d8b8aa) )
	ROM_LOAD( "2716.4l",        0xf800, 0x0800, CRC(57752b47) SHA1(a34d3150ea9082889154042dbea3386f71322a78) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     		/* 64k for 6502 code of audio CPU */
	ROM_LOAD( "8_2708.4d",      0xf800, 0x0400, CRC(46162e7e) SHA1(7ed85f4a9ac58d6d9bafba0c843a16c269656563) )
	ROM_LOAD( "9_2708.3d",      0xfc00, 0x0400, CRC(4175f5c4) SHA1(45cae8a1fcfd34b91c63cc7e544a32922da14f16) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )    /* Background tiles */
	ROM_LOAD( "mc-1_82s137.3k", 0x0000, 0x0400, NO_DUMP )
	ROM_LOAD( "me-1_82s137.3l", 0x0400, 0x0400, CRC(7328a425) SHA1(327adc8b0e25d93f1ae98a44c26d0aaaac1b1a9c) )
	ROM_LOAD( "md-1_82s137.3m", 0x0800, 0x0400, CRC(b5329929) SHA1(86890e1b7cc8cb31fc0dcbc2d3cff02e4cf95619) )

	ROM_REGION( 0x0400, REGION_GFX2, 0 )  			/* Background tile maps */
	ROM_LOAD( "mf-1_82s137.4h", 0x0000, 0x0400, CRC(9b04c446) SHA1(918013f3c0244ab6a670b9d1b6b642298e2c5ab8) )

	ROM_REGION( 0x0400, REGION_GFX3, 0 )			/* Car headlight */
	ROM_LOAD( "ma-_2708.2b",    0x0000, 0x0400, CRC(aab16446) SHA1(d2342627cc2766004343f27515d8a7989d5fe932) )

	ROM_REGION( 0x0800, REGION_GFX4, 0 )			/* Shifting data */
	ROM_LOAD( "mb-_2716.5c",    0x0000, 0x0800, CRC(cb801e49) SHA1(7444c4af7cf07e5fdc54044d62ea4fcb201b2b8b) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 ) 			/* Color PROM */
	ROM_LOAD( "mg-1_82s123.7e", 0x0000, 0x0020, CRC(e622396a) SHA1(8972704bd25fed462e25c453771cc5ca4fc74034) )
ROM_END


DRIVER_INIT( madalien )
{
	madalien_shift_counter = 0;
	madalien_shift_reg_lo  = 0;
	madalien_shift_reg_hi  = 0;
	madalien_bg_map_selector = 0;
	madalien_scroll_light = 0;
	madalien_select_color_1 = 0;
	madalien_select_color_2 = 0;
	madalien_headlight_on = 0;
	madalien_swap_colors = 0;
	madalien_video_register = 0;
	madalien_flip_screen = 0;
}


/*          rom       parent     machine   inp       init */
GAME( 1980, madalien, 0,         madalien, madalien, madalien, ROT270, "Data East Corporation", "Mad Alien", 0 )
GAME( 1980, madalina, madalien,  madalien, madalien, madalien, ROT270, "Data East Corporation", "Mad Alien (Highway Chase)", 0 )
