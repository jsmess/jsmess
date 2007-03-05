/***************************************************************************

    Atari/Kee Ultra Tank hardware

    Games supported:
        * Ultra Tank

    Known issues:
        - proms need to be added
        - palette should be derived from resistor values
        - driver should make use of the color prom
        - colors in the tilemap are wrong
        - invisible tanks don't work
        - sprites are wrong (e.g. tank explosion sequence)
        - sprite positions may be off by 1
        - collision detection is missing
        - motor sound isn't hooked up properly
        - motor sound can be affected by frame skipping
        - first option switch needs to be inverted
        - order of bits in input ports is a mess
        - "spare" dip is actually unused
        - input ports could use tags
        - PORT_CUSTOM should be used for collision bits
        - dip switch locations are missing
        - interrupt timing isn't accurate
        - VBLANK duration isn't accurate
        - screen size is 262x384 not 256x256
        - refresh rate is 60.11Hz not 60Hz
        - address space should be limited to 14 bits
        - memory maps should be merged
        - memory map should be more densely populated
        - total RAM size is $400 not $500
        - coin lockout needs to be hooked up
        - implementation of D/A latch is a kludge
        - correct CPU speed is 756kHz not 1.5MHz

        Most fixes can be found in MAME 0.112.

***************************************************************************/

#include "driver.h"
#include "ultratnk.h"
#include "sound/discrete.h"

static int ultratnk_controls;
static UINT8 *mirror_ram;

static tilemap *bg_tilemap;

/*************************************
 *
 *  Palette generation
 *
 *************************************/

static unsigned short colortable_source[] =
{
	0x02, 0x01,
	0x02, 0x00,
	0x02, 0x00,
	0x02, 0x01
};

static PALETTE_INIT( ultratnk )
{
	palette_set_color(machine,0,0x00,0x00,0x00); /* BLACK */
	palette_set_color(machine,1,0xff,0xff,0xff); /* WHITE */
	palette_set_color(machine,2,0x80,0x80,0x80); /* LT GREY */
	palette_set_color(machine,3,0x55,0x55,0x55); /* DK GREY */
	memcpy(colortable,colortable_source,sizeof(colortable_source));
}



/*************************************
 *
 *  Sprite rendering
 *
 *************************************/

static void ultratnk_draw_sprites( mame_bitmap *bitmap, const rectangle *rect )
{
	cpuintrf_push_context(0);
	if( (program_read_byte(0x93)&0x80)==0 )
	/*  Probably wrong; game description indicates that one or both tanks can
        be invisible; in this game mode, tanks are visible when hit, bumping
        into a wall, or firing
    */
	{
		drawgfx( bitmap, Machine->gfx[1], /* tank */
			program_read_byte(0x99)>>3,
			0,
			0,0, /* no flip */
			program_read_byte(0x90)-16,program_read_byte(0x98)-16,
			rect,
			TRANSPARENCY_PEN, 0 );

		drawgfx( bitmap, Machine->gfx[1], /* tank */
			program_read_byte(0x9b)>>3,
			1,
			0,0, /* no flip */
			program_read_byte(0x92)-16,program_read_byte(0x9a)-16,
			rect,
			TRANSPARENCY_PEN, 0 );
	}

	drawgfx( bitmap, Machine->gfx[1], /* bullet */
		(program_read_byte(0x9f)>>3)|0x20,
		0,
		0,0, /* no flip */
		program_read_byte(0x96)-16,program_read_byte(0x9e)-16,
		rect,
		TRANSPARENCY_PEN, 0 );

	drawgfx( bitmap, Machine->gfx[1], /* bullet */
		(program_read_byte(0x9d)>>3)|0x20,
		1,
		0,0, /* no flip */
		program_read_byte(0x94)-16,program_read_byte(0x9c)-16,
		rect,
		TRANSPARENCY_PEN, 0 );
	cpuintrf_pop_context();
}



/*************************************
 *
 *  Video update
 *
 *************************************/

static WRITE8_HANDLER( ultratnk_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int attr = videoram[tile_index];
	int code = attr & 0x3f;
	int color = attr >> 6;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( ultratnk )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_UPDATE( ultratnk )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	ultratnk_draw_sprites(bitmap, cliprect);

	/* Weird, but we have to update our sound registers here. */
	discrete_sound_w(ULTRATNK_MOTOR1_DATA, mirror_ram[0x88] % 16);
	discrete_sound_w(ULTRATNK_MOTOR2_DATA, mirror_ram[0x8A] % 16);

	return 0;
}

/*************************************
 *
 *  Control reading
 *
 *************************************/

static WRITE8_HANDLER( da_latch_w )
{
	int joybits = readinputport(4);
	ultratnk_controls = readinputport(3); /* start and fire buttons */

	switch( data )
	{
	case 0x0a:
		if( joybits&0x08 ) ultratnk_controls &= ~0x40;
		if( joybits&0x04 ) ultratnk_controls &= ~0x04;

		if( joybits&0x80 ) ultratnk_controls &= ~0x10;
		if( joybits&0x40 ) ultratnk_controls &= ~0x01;
		break;

	case 0x05:
		if( joybits&0x02 ) ultratnk_controls &= ~0x40;
		if( joybits&0x01 ) ultratnk_controls &= ~0x04;

		if( joybits&0x20 ) ultratnk_controls &= ~0x10;
		if( joybits&0x10 ) ultratnk_controls &= ~0x01;
		break;
	}
}


static READ8_HANDLER( ultratnk_controls_r )
{
	return (ultratnk_controls << offset) & 0x80;
}


static READ8_HANDLER( ultratnk_barrier_r )
{
	return readinputport(2) & 0x80;
}


static READ8_HANDLER( ultratnk_coin_r )
{
	switch (offset & 0x06)
	{
		case 0x00: return (readinputport(2) << 3) & 0x80;	/* left coin */
		case 0x02: return (readinputport(2) << 4) & 0x80;	/* right coin */
		case 0x04: return (readinputport(2) << 1) & 0x80;	/* invisible tanks */
		case 0x06: return (readinputport(2) << 2) & 0x80;	/* rebounding shots */
	}

	return 0;
}


static READ8_HANDLER( ultratnk_tilt_r )
{
	return (readinputport(2) << 5) & 0x80;	/* tilt */
}


static READ8_HANDLER( ultratnk_dipsw_r )
{
	int dipsw = readinputport(0);
	switch( offset )
	{
		case 0x00: return ((dipsw & 0xC0) >> 6); /* language? */
		case 0x01: return ((dipsw & 0x30) >> 4); /* credits */
		case 0x02: return ((dipsw & 0x0C) >> 2); /* game time */
		case 0x03: return ((dipsw & 0x03) >> 0); /* extended time */
	}
	return 0;
}



/*************************************
 *
 *  Sound handlers
 *
 *************************************/
WRITE8_HANDLER( ultratnk_fire_w )
{
	discrete_sound_w(ULTRATNK_FIRE1_EN + (offset >> 1), offset & 1);
}

WRITE8_HANDLER( ultratnk_attract_w )
{
	discrete_sound_w(ULTRATNK_ATTRACT_EN, data & 1);
}

WRITE8_HANDLER( ultratnk_explosion_w )
{
	discrete_sound_w(ULTRATNK_EXPLOSION_DATA, data % 16);
}



/*************************************
 *
 *  Interrupt generation
 *
 *************************************/

static INTERRUPT_GEN( ultratnk_interrupt )
{
	watchdog_enable(readinputport(1) & 0x40);

	if (readinputport(1) & 0x40 )
	{
		/* only do NMI interrupt if not in TEST mode */
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
}



/*************************************
 *
 *  Misc memory handlers
 *
 *************************************/

static READ8_HANDLER( ultratnk_collision_r )
{
	/** Note: hardware collision detection is not emulated.
     *  However, the game is fully playable, since the game software uses it
     *  only as a hint to check for tanks bumping into walls/mines.
     */
	switch( offset )
	{
		case 0x01:	return 0x80;	/* white tank = D7 */
		case 0x03:	return 0x80;	/* black tank = D7 */
	}
	return 0;
}


static WRITE8_HANDLER( ultratnk_leds_w )
{
	set_led_status(offset/2,offset&1);
}


static READ8_HANDLER( mirror_r )
{
	return mirror_ram[offset];
}


static WRITE8_HANDLER( mirror_w )
{
	mirror_ram[offset] = data;
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x00ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x0100, 0x01ff) AM_READ(mirror_r)
	AM_RANGE(0x0800, 0x0bff) AM_READ(MRA8_RAM)
	AM_RANGE(0x0c00, 0x0cff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1000, 0x1000) AM_READ(input_port_1_r) /* self test, vblank */
	AM_RANGE(0x1800, 0x1800) AM_READ(ultratnk_barrier_r) /* barrier */
	AM_RANGE(0x2000, 0x2007) AM_READ(ultratnk_controls_r)
	AM_RANGE(0x2020, 0x2026) AM_READ(ultratnk_coin_r)
	AM_RANGE(0x2040, 0x2043) AM_READ(ultratnk_collision_r)
	AM_RANGE(0x2046, 0x2046) AM_READ(ultratnk_tilt_r)
	AM_RANGE(0x2060, 0x2063) AM_READ(ultratnk_dipsw_r)
	AM_RANGE(0x2800, 0x2fff) AM_READ(MRA8_NOP) /* diagnostic ROM (see code at B1F3) */
	AM_RANGE(0xb000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x00ff) AM_WRITE(MWA8_RAM) AM_BASE(&mirror_ram)
	AM_RANGE(0x0100, 0x01ff) AM_WRITE(mirror_w)
	AM_RANGE(0x0800, 0x0bff) AM_WRITE(ultratnk_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x0c00, 0x0cff) AM_WRITE(MWA8_RAM) /* ? */
	AM_RANGE(0x2000, 0x201f) AM_WRITE(ultratnk_attract_w) /* attract */
	AM_RANGE(0x2020, 0x203f) AM_WRITE(MWA8_NOP) /* collision reset 1-4, 2020-21=cr1, 22-23=cr2, 24-25=cr3, 26,27=cr4 */
	AM_RANGE(0x2040, 0x2041) AM_WRITE(da_latch_w) /* D/A LATCH */
	AM_RANGE(0x2042, 0x2043) AM_WRITE(ultratnk_explosion_w) /* EXPLOSION (sound) */
	AM_RANGE(0x2044, 0x2045) AM_WRITE(watchdog_reset_w) /* TIMER (watchdog) RESET */
	AM_RANGE(0x2066, 0x2067) AM_WRITE(MWA8_NOP) /* LOCKOUT */
	AM_RANGE(0x2068, 0x206b) AM_WRITE(ultratnk_leds_w)
	AM_RANGE(0x206c, 0x206f) AM_WRITE(ultratnk_fire_w) /* fire 1/2 */
	AM_RANGE(0xb000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( ultratnk )
	PORT_START
	PORT_DIPNAME( 0x03, 0x01, "Extended Play" )
	PORT_DIPSETTING(	0x01, "25 Points" )
	PORT_DIPSETTING(	0x02, "50 Points" )
	PORT_DIPSETTING(	0x03, "75 Points" )
	PORT_DIPSETTING(	0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x0c, 0x04, "Game Length" )
	PORT_DIPSETTING(	0x00, "60 Seconds" )
	PORT_DIPSETTING(	0x04, "90 Seconds" )
	PORT_DIPSETTING(	0x08, "120 Seconds" )
	PORT_DIPSETTING(	0x0c, "150 Seconds" )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xc0, 0x00, "Spare" ) /* Language?  Doesn't have any effect. */
	PORT_DIPSETTING(	0x00, "A" )
	PORT_DIPSETTING(	0x40, "B" )
	PORT_DIPSETTING(	0x80, "C" )
	PORT_DIPSETTING(	0xc0, "D" )

	PORT_START
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START /* input#2 (arbitrarily arranged) */
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_SERVICE1 ) PORT_NAME("Option 1") PORT_TOGGLE
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE2 ) PORT_NAME("Option 2") PORT_TOGGLE
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE3 ) PORT_NAME("Option 3") PORT_TOGGLE
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START /* input#3 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL ) /* joystick (taken from below) */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SPECIAL ) /* joystick (taken from below) */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL ) /* joystick (taken from below) */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) /* joystick (taken from below) */

	PORT_START /* input#4 - fake */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0a, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP ) PORT_PLAYER(1)	/* note that this sets 2 bits */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x05, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP ) PORT_PLAYER(1)	/* note that this sets 2 bits */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0xa0, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP ) PORT_PLAYER(2)	/* note that this sets 2 bits */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x50, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP ) PORT_PLAYER(2)	/* note that this sets 2 bits */

	PORT_START_TAG("MOTOR1")
	PORT_ADJUSTER( 5, "Motor 1 RPM" )

	PORT_START_TAG("MOTOR2")
	PORT_ADJUSTER( 10, "Motor 2 RPM" )
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static const gfx_layout playfield_layout =
{
	8,8,
	RGN_FRAC(1,2),
	1,
	{ 0 },
	{ 4, 5, 6, 7, 4 + RGN_FRAC(1,2), 5 + RGN_FRAC(1,2), 6 + RGN_FRAC(1,2), 7 + RGN_FRAC(1,2) },
	{ 0, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static const gfx_layout motion_layout =
{
	16,16,
	RGN_FRAC(1,4),
	1,
	{ 0 },
	{ 7, 6, 5, 4, 7 + RGN_FRAC(1,4), 6 + RGN_FRAC(1,4), 5 + RGN_FRAC(1,4), 4 + RGN_FRAC(1,4),
	  7 + RGN_FRAC(2,4), 6 + RGN_FRAC(2,4), 5 + RGN_FRAC(2,4), 4 + RGN_FRAC(2,4),
	  7 + RGN_FRAC(3,4), 6 + RGN_FRAC(3,4), 5 + RGN_FRAC(3,4), 4 + RGN_FRAC(3,4) },
	{ 0, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &playfield_layout, 0, 4 }, 	/* playfield graphics */
	{ REGION_GFX2, 0, &motion_layout,    0, 4 }, 	/* motion graphics */
	{ -1 }
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( ultratnk )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,1500000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(ultratnk_interrupt,4)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_WATCHDOG_VBLANK_INIT(8)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(sizeof(colortable_source)/sizeof(unsigned short))

	MDRV_PALETTE_INIT(ultratnk)
	MDRV_VIDEO_START(ultratnk)
	MDRV_VIDEO_UPDATE(ultratnk)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG(ultratnk_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( ultratnk )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )
	ROM_LOAD_NIB_LOW ( "030180.n1",	 0xb000, 0x0800, CRC(b6aa6056) SHA1(6de094017b5d87a238053fac88129d20260f8222) ) /* ROM 3 D0-D3 */
	ROM_LOAD_NIB_HIGH( "030181.k1",	 0xb000, 0x0800, CRC(17145c97) SHA1(afe0c9c562c27cd1fba57ea83377b0a4c12496db) ) /* ROM 3 D4-D7 */
	ROM_LOAD_NIB_LOW ( "030182.m1",	 0xb800, 0x0800, CRC(034366a2) SHA1(dc289ce4c79e9937977ca8804ce07b4c8e40e969) ) /* ROM 4 D0-D3 */
	ROM_RELOAD(                      0xf800, 0x0800 ) /* for 6502 vectors */
	ROM_LOAD_NIB_HIGH( "030183.l1",	 0xb800, 0x0800, CRC(be141602) SHA1(17aad9bab9bf6bd22dc3c2214b049bbd68c87380) ) /* ROM 4 D4-D7 */
	ROM_RELOAD(                      0xf800, 0x0800 ) /* for 6502 vectors */

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "30172-01.j6", 0x0000, 0x0200, CRC(1d364b23) SHA1(44c5792ed3f33f40cd8632718b0e82152559ecdf) )
	ROM_LOAD( "30173-01.h6", 0x0200, 0x0200, CRC(5c32f331) SHA1(c1d675891490fbc533eaa0da57545398d7325df8) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "30174-01.n6", 0x0000, 0x0400, CRC(d0e20e73) SHA1(0df1ed4a73255032bb809fb4d0a4bf3f151c749d) )
	ROM_LOAD( "30175-01.m6", 0x0400, 0x0400, CRC(a47459c9) SHA1(4ca92edc172fbac923ba71731a25546c04ffc7b0) )
	ROM_LOAD( "30176-01.l6", 0x0800, 0x0400, CRC(1cc7c2dd) SHA1(7f8aebe8375751183afeae35ea2d241d22ee7a4f) )
	ROM_LOAD( "30177-01.k6", 0x0c00, 0x0400, CRC(3a91b09f) SHA1(1e713cb612eb7d78fc4a003e4e60308f62e0b169) )
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1978, ultratnk, 0, ultratnk, ultratnk, 0, 0, "Atari", "Ultra Tank", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS )
