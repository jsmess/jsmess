/****************************************************************************

Bally Astrocade style games

driver by Nicola Salmoria, Mike Coates, Frank Palazzolo

TODO:
    add rotate support
    profpac_vm - self test
    look into NVRAM problems in demndrgn
    try 10-pin deluxe roms?
    finish looking at noise gen bug
    optimize sound code
    dig into gorf timing hacks

Notes:
- In seawolf2, service mode dip switch turns on memory test. Reset with 2 pressed
  to get to an input check screen, reset with 1+2 pressed to get to a convergence
  test screen.


memory map (preliminary)

0000-3fff ROM but also "magic" RAM (which processes data and copies it to the video RAM)
4000-7fff SCREEN RAM (bitmap)
8000-cfff ROM
d000-d3ff STATIC RAM

I/O ports:
IN:
08        intercept register (collision detector)
          bit 0: intercept in pixel 3 in an OR or XOR write since last reset
          bit 1: intercept in pixel 2 in an OR or XOR write since last reset
          bit 2: intercept in pixel 1 in an OR or XOR write since last reset
          bit 3: intercept in pixel 0 in an OR or XOR write since last reset
          bit 4: intercept in pixel 3 in last OR or XOR write
          bit 5: intercept in pixel 2 in last OR or XOR write
          bit 6: intercept in pixel 1 in last OR or XOR write
          bit 7: intercept in pixel 0 in last OR or XOR write
10        IN0
11        IN1
12        IN2
13        DSW
14        Video Retrace
15        ?
17        Speech Synthesizer (Output)

OUT:
00-07     palette (00-03 = right part of screen; 04-07 left part)
08        select video mode (0 = low res 160x102, 1 = high res 320x204)
09        --xxxxxx position where to switch from the "left" to the "right" palette (/2).
          xx------ background color (portion of screen after vblank)
0a        screen height
0b        color block transfer
0c        magic RAM control
          bit 7: ?
          bit 6: flip
          bit 5: draw in XOR mode
          bit 4: draw in OR mode
          bit 3: "expand" mode (convert 1bpp data to 2bpp)
          bit 2: "rotate" mode (rotate 90 degrees - NOT EMULATED)
          bit 1:\ shift amount to be applied before copying
          bit 0:/
0d        set interrupt vector
10-18     sound
19        magic RAM expand mode color
78-7e     pattern board (see video.c for details)

****************************************************************************/

#include "driver.h"
#include "includes/astrocde.h"
#include "sound/samples.h"
#include "sound/astrocde.h"

static UINT8 game_on = 0;

static WRITE8_HANDLER( seawolf2_lamps_w )
{
	/* 0x42 = player 2 (left), 0x43 = player 1 (right) */
	/* --x----- explosion */
	/* ---x---- RELOAD (active low) */
	/* ----x--- torpedo 1 available */
	/* -----x-- torpedo 2 available */
	/* ------x- torpedo 3 available */
	/* -------x torpedo 4 available */

	/* I'm only supporting the "RELOAD" lamp since we don't have enough leds ;-) */
	set_led_status(offset^1,data & 0x10);
}

static WRITE8_HANDLER( seawolf2_sound_1_w )  // Port 40
{
	if (game_on)
	{
		if (data & 0x01)
			sample_start (1, 1, 0);  /* Left Torpedo */
		if (data & 0x02)
			sample_start (0, 0, 0);  /* Left Ship Hit */
		if (data & 0x04)
			sample_start (4, 4, 0);  /* Left Mine Hit */
		if (data & 0x08)
			sample_start (6, 1, 0);  /* Right Torpedo */
		if (data & 0x10)
			sample_start (5, 0, 0);  /* Right Ship Hit */
		if (data & 0x20)
			sample_start (9, 4, 0);  /* Right Mine Hit */
	}
}

static WRITE8_HANDLER( seawolf2_sound_2_w )  // Port 41
{
	game_on = data & 0x80;

	if (game_on)
	{
		// data & 0x07 control dive L/R panning - not implemented
		if (data & 0x08)
			sample_start (2, 2, 0);  /* Dive */
		if (data & 0x10)
			sample_start (8, 3, 0);  /* Right Sonar */
		if (data & 0x20)
			sample_start (3, 3, 0);  /* Left Sonar */
	}

	coin_counter_w(0, data & 0x40);    /* Coin Counter */
}

void astrocade_state_save_register_main(void)
{
	state_save_register_global(game_on);
}

/*************************************
 *
 *  Memory maps
 *
 *************************************/

static ADDRESS_MAP_START( seawolf2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(wow_magicram_w)
	AM_RANGE(0x4000, 0x7fff) AM_RAM AM_BASE(&wow_videoram)
	AM_RANGE(0xc000, 0xc3ff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( astrocde_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(wow_magicram_w)
	AM_RANGE(0x4000, 0x7fff) AM_RAM AM_BASE(&wow_videoram)
	AM_RANGE(0x8000, 0xcfff) AM_ROM
	AM_RANGE(0xd000, 0xdfff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( wow_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(wow_magicram_w)
	AM_RANGE(0x4000, 0x7fff) AM_RAM AM_BASE(&wow_videoram)
	AM_RANGE(0x8000, 0xcfff) AM_ROM
	AM_RANGE(0xd000, 0xd3ff) AM_RAM //AM_READWRITE(wow_protected_ram_r, wow_protected_ram_w) AM_BASE(&wow_protected_ram)
	AM_RANGE(0xd400, 0xdfff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( robby_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(wow_magicram_w)
	AM_RANGE(0x4000, 0x7fff) AM_RAM AM_BASE(&wow_videoram)
	AM_RANGE(0x8000, 0xdfff) AM_ROM
	AM_RANGE(0xe000, 0xe7ff) AM_READWRITE(robby_nvram_r, robby_nvram_w) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xe800, 0xffff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( demndrgn_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(wow_magicram_w)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK1, profpac_videoram_w)
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(2)
	AM_RANGE(0xc000, 0xdfff) AM_ROM
  	AM_RANGE(0xe000, 0xe7ff) AM_READWRITE(demndrgn_nvram_r, demndrgn_nvram_w) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xe800, 0xffff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( profpac_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(wow_magicram_w)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK1, profpac_videoram_w)
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(2)
	AM_RANGE(0xc000, 0xdfff) AM_ROM
  	AM_RANGE(0xe000, 0xe7ff) AM_READWRITE(profpac_nvram_r, profpac_nvram_w) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xe800, 0xffff) AM_RAM
ADDRESS_MAP_END



/*************************************
 *
 *  Port maps
 *
 *************************************/

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x08, 0x08) AM_MIRROR(0xff00) AM_READ(wow_intercept_r)
	AM_RANGE(0x0e, 0x0e) AM_MIRROR(0xff00) AM_READ(wow_video_retrace_r)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0xff00) AM_READ(input_port_0_r)
	AM_RANGE(0x11, 0x11) AM_MIRROR(0xff00) AM_READ(input_port_1_r)
  	AM_RANGE(0x12, 0x12) AM_MIRROR(0xff00) AM_READ(input_port_2_r)
	AM_RANGE(0x13, 0x13) AM_MIRROR(0xff00) AM_READ(input_port_3_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( demndrgn_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x08, 0x08) AM_MIRROR(0xff00) AM_READ(wow_intercept_r)
	AM_RANGE(0x0e, 0x0e) AM_MIRROR(0xff00) AM_READ(wow_video_retrace_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( seawolf2_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x07) AM_MIRROR(0xff00) AM_WRITE(astrocde_colour_register_w)
	AM_RANGE(0x08, 0x08) AM_MIRROR(0xff00) AM_WRITE(astrocde_mode_w)
	AM_RANGE(0x09, 0x09) AM_MIRROR(0xff00) AM_WRITE(astrocde_colour_split_w)
	AM_RANGE(0x0a, 0x0a) AM_MIRROR(0xff00) AM_WRITE(astrocde_vertical_blank_w)
	AM_RANGE(0x0b, 0x0b) AM_MIRROR(0xff00) AM_WRITE(astrocde_colour_block_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0xff00) AM_WRITE(astrocde_magic_control_w)
	AM_RANGE(0x0d, 0x0d) AM_MIRROR(0xff00) AM_WRITE(astrocde_interrupt_vector_w)
	AM_RANGE(0x0e, 0x0e) AM_MIRROR(0xff00) AM_WRITE(astrocde_interrupt_enable_w)
	AM_RANGE(0x0f, 0x0f) AM_MIRROR(0xff00) AM_WRITE(astrocde_interrupt_w)

	AM_RANGE(0x19, 0x19) AM_MIRROR(0xff00) AM_WRITE(astrocde_magic_expand_color_w)

	AM_RANGE(0x40, 0x40) AM_MIRROR(0xff00) AM_WRITE(seawolf2_sound_1_w) /* analog sound */
	AM_RANGE(0x41, 0x41) AM_MIRROR(0xff00) AM_WRITE(seawolf2_sound_2_w) /* analog sound */
	AM_RANGE(0x42, 0x43) AM_MIRROR(0xff00) AM_WRITE(seawolf2_lamps_w)	/* cabinet lamps */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x07) AM_MIRROR(0xff00) AM_WRITE(astrocde_colour_register_w)
	AM_RANGE(0x08, 0x08) AM_MIRROR(0xff00) AM_WRITE(astrocde_mode_w)
	AM_RANGE(0x09, 0x09) AM_MIRROR(0xff00) AM_WRITE(astrocde_colour_split_w)
	AM_RANGE(0x0a, 0x0a) AM_MIRROR(0xff00) AM_WRITE(astrocde_vertical_blank_w)
	AM_RANGE(0x0b, 0x0b) AM_MIRROR(0xff00) AM_WRITE(astrocde_colour_block_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0xff00) AM_WRITE(astrocde_magic_control_w)
	AM_RANGE(0x0d, 0x0d) AM_MIRROR(0xff00) AM_WRITE(astrocde_interrupt_vector_w)
	AM_RANGE(0x0e, 0x0e) AM_MIRROR(0xff00) AM_WRITE(astrocde_interrupt_enable_w)
	AM_RANGE(0x0f, 0x0f) AM_MIRROR(0xff00) AM_WRITE(astrocde_interrupt_w)

	AM_RANGE(0x10, 0x17) AM_MIRROR(0xff00) AM_WRITE(astrocade_sound1_w)
	AM_SPACE(0x18, 0xff) AM_WRITE(astrocade_soundblock1_w)

	AM_RANGE(0x19, 0x19) AM_MIRROR(0xff00) AM_WRITE(astrocde_magic_expand_color_w)

	/* These two are not part of seawolf2 or ebases */
	AM_RANGE(0xa55b, 0xa55b) AM_WRITE(wow_ramwrite_enable_w) /* ram write enable */
	AM_RANGE(0x78, 0x7e) AM_MIRROR(0xff00) AM_WRITE(astrocde_pattern_board_w)

/*  AM_RANGE(0xf8, 0xff) AM_MIRROR(0xff00) AM_WRITE(MWA8_NOP) */ /* Gorf uses these */
ADDRESS_MAP_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

INPUT_PORTS_START( seawolf2 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x3f, 0x20, IPT_PADDLE ) PORT_MINMAX(0,0x3f) PORT_SENSITIVITY(20) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_REVERSE PORT_PLAYER(1)
	PORT_DIPNAME( 0x40, 0x00, "Language 1" )
	PORT_DIPSETTING(    0x00, "Language 2" )
	PORT_DIPSETTING(    0x40, DEF_STR( French ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x3f, 0x20, IPT_PADDLE ) PORT_MINMAX(0,0x3f) PORT_SENSITIVITY(20) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_REVERSE PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x08, 0x00, "Language 2" )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x08, DEF_STR( German ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW") /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x06, 0x00, "Play Time" )
	PORT_DIPSETTING(    0x06, "40" )
	PORT_DIPSETTING(    0x04, "50" )
	PORT_DIPSETTING(    0x02, "60" )
	PORT_DIPSETTING(    0x00, "70" )
	PORT_DIPNAME( 0x08, 0x08, "2 Players Game" )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Credits" )
	PORT_DIPNAME( 0x30, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x10, "5000" )
	PORT_DIPSETTING(    0x20, "6000" )
	PORT_DIPSETTING(    0x30, "7000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x40, 0x40, "Monitor" )
	PORT_DIPSETTING(    0x40, "Color" )
	PORT_DIPSETTING(    0x00, "B/W" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


INPUT_PORTS_START( spacezap )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )	// starts a 1 player game if 1 credit
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW") /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( ebases )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Monitor" )
	PORT_DIPSETTING(    0x00, "Color" )
	PORT_DIPSETTING(    0x10, "B/W" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x00, "2 Players Game" )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Credits" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("IN3")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_RESET PORT_PLAYER(2)	\

	PORT_START_TAG("IN4")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_RESET PORT_PLAYER(2)	\

	PORT_START_TAG("IN5")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_RESET	\

	PORT_START_TAG("IN6")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_RESET	\
INPUT_PORTS_END


INPUT_PORTS_START( wow )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* speech status */

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x08, DEF_STR( English ) )
	PORT_DIPSETTING(    0x00, "Foreign (NEED ROM)" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Lives ) )
 	PORT_DIPSETTING(    0x10, "2 for 1 Credit / 5 for 2 Credits" )
 	PORT_DIPSETTING(    0x00, "3 for 1 Credit / 7 for 2 Credits" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x20, "3rd Level" )
	PORT_DIPSETTING(    0x00, "4th Level" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, "On only when controls are touched" )
	PORT_DIPSETTING(    0x80, "Always On"  )
INPUT_PORTS_END


INPUT_PORTS_START( gorf )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* speech status */

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x08, DEF_STR( English ) )
	PORT_DIPSETTING(    0x00, "Foreign (NEED ROM)" )
	PORT_DIPNAME( 0x10, 0x00, "Lives per Credit" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "Mission 5" )
	PORT_DIPSETTING(    0x20, DEF_STR( None ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( robby )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Use NVRAM" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, "Use Service Mode Settings" )
	PORT_DIPSETTING(    0x00, "Reset" )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( demndrgn )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)

	PORT_START_TAG("IN3")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)
INPUT_PORTS_END


INPUT_PORTS_START( profpac )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) /* Left A */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) /* Left B */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) /* Left C */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* Right A */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* Right B */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) /* Right C */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, "Reset on powerup" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "Halt on error" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Beep" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "ROM" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END



/*************************************
 *
 *  Sound definitions
 *
 *************************************/

static const char *seawolf_sample_names[] =
{
	"*seawolf",
	"shiphit.wav",
	"torpedo.wav",
	"dive.wav",
	"sonar.wav",
	"minehit.wav",
	0       /* end of array */
};

struct Samplesinterface seawolf2_samples_interface =
{
	10,	/* 5*2 channels */
	seawolf_sample_names
};

static struct Samplesinterface wow_samples_interface =
{
	8,	/* 8 channels */
	wow_sample_names
};

static struct Samplesinterface gorf_samples_interface =
{
	8,	/* 8 channels */
	gorf_sample_names
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( seawolf2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_PROGRAM_MAP(seawolf2_map,0)
	MDRV_CPU_IO_MAP(readport,seawolf2_writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(astrocde)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(astrocde)
	MDRV_VIDEO_START(astrocde)
	MDRV_VIDEO_UPDATE(seawolf2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(seawolf2_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( spacezap )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_PROGRAM_MAP(wow_map,0)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(astrocde)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(astrocde)
	MDRV_VIDEO_START(astrocde)
	MDRV_VIDEO_UPDATE(astrocde)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(ASTROCADE, 1789773)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ebases )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_PROGRAM_MAP(astrocde_map,0)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(astrocde)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(astrocde)
	MDRV_VIDEO_START(astrocde)
	MDRV_VIDEO_UPDATE(astrocde)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(ASTROCADE, 1789773)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wow )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_PROGRAM_MAP(wow_map,0)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(astrocde)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(astrocde)

	MDRV_VIDEO_START(astrocde_stars)
	MDRV_VIDEO_UPDATE(astrocde)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(ASTROCADE, 1789773)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)

	MDRV_SOUND_ADD(ASTROCADE, 1789773)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(wow_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.25)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gorf )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_PROGRAM_MAP(wow_map,0)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(gorf_interrupt,256)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(astrocde)

	/* video hardware */
	/* it may look like the right hand side of the screen needs clipping, but */
	/* this isn't the case: cocktail mode would be clipped on the wrong side */

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(astrocde)
	MDRV_VIDEO_START(astrocde_stars)
	MDRV_VIDEO_UPDATE(astrocde)

	/* sound hardware */
	MDRV_SPEAKER_ADD("upper", 0.0, 0.0, 1.0)
	MDRV_SPEAKER_ADD("lower", 0.0, -0.5, 1.0)

	MDRV_SOUND_ADD(ASTROCADE, 1789773)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "lower", 1.0)

	MDRV_SOUND_ADD(ASTROCADE, 1789773)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "upper", 1.0)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(gorf_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "upper", 0.25)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "lower", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( robby )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_PROGRAM_MAP(robby_map,0)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(astrocde)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(astrocde)

	MDRV_VIDEO_START(astrocde)
	MDRV_VIDEO_UPDATE(astrocde)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(ASTROCADE, 1789773)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)

	MDRV_SOUND_ADD(ASTROCADE, 1789773)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( demndrgn )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_PROGRAM_MAP(demndrgn_map,0)
	MDRV_CPU_IO_MAP(demndrgn_readport,writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(profpac)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(astrocde)
	MDRV_VIDEO_START(profpac)
	MDRV_VIDEO_UPDATE(profpac)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( profpac )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_PROGRAM_MAP(profpac_map,0)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(profpac)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(astrocde)
	MDRV_VIDEO_START(profpac)
	MDRV_VIDEO_UPDATE(profpac)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(ASTROCADE, 1789773)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)

	MDRV_SOUND_ADD(ASTROCADE, 1789773)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( seawolf2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "sw2x1.bin",    0x0000, 0x0800, CRC(ad0103f6) SHA1(c6e411444a824ce54b0eee10f7dc15e4229ec070) )
	ROM_LOAD( "sw2x2.bin",    0x0800, 0x0800, CRC(e0430f0a) SHA1(63d8c6b77e0aa536b4f5bb774bc9285f736d4265) )
	ROM_LOAD( "sw2x3.bin",    0x1000, 0x0800, CRC(05ad1619) SHA1(c9dbeaa4540dc95f98970f501a420b18b9898c91) )
	ROM_LOAD( "sw2x4.bin",    0x1800, 0x0800, CRC(1a1a14a2) SHA1(57d0ddea9f8bf082f50d0468a726fd91aaabf4e4) )
ROM_END


ROM_START( spacezap )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "0662.01",      0x0000, 0x1000, CRC(a92de312) SHA1(784ac67c75c7c101f97ebfd39b2b3f7bf7fa470a) )
	ROM_LOAD( "0663.xx",      0x1000, 0x1000, CRC(4836ebf1) SHA1(ad0e8c34a209c827c1336f0250cc61fee667fb03) )
	ROM_LOAD( "0664.xx",      0x2000, 0x1000, CRC(d8193a80) SHA1(72151e773562da62acd2c1d9638711711cbc13a3) )
	ROM_LOAD( "0665.xx",      0x3000, 0x1000, CRC(3784228d) SHA1(5aabd720a106158a892368c4920d9cd0f5235e34) )
ROM_END


ROM_START( ebases )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "m761a",        0x0000, 0x1000, CRC(34422147) SHA1(6483ca1359b675b0dd739605db2a1dbd4b7fb8cb) )
	ROM_LOAD( "m761b",        0x1000, 0x1000, CRC(4f28dfd6) SHA1(52e571e671fa61b0f9ab397a5947094c24f6c388) )
	ROM_LOAD( "m761c",        0x2000, 0x1000, CRC(bff6c97e) SHA1(e41fb9db919039c8a48b4caebf80821a066d7ccf) )
	ROM_LOAD( "m761d",        0x3000, 0x1000, CRC(5173781a) SHA1(e60c3f4b075f8b811ff6a8637c4aa0b089847a82) )
ROM_END


ROM_START( wow )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "wow.x1",       0x0000, 0x1000, CRC(c1295786) SHA1(1e4f30cc15537aed6603b4e664e6e60f4bccb5c5) )
	ROM_LOAD( "wow.x2",       0x1000, 0x1000, CRC(9be93215) SHA1(0bc8ee6d8391104eb217b612f32856b105946682) )
	ROM_LOAD( "wow.x3",       0x2000, 0x1000, CRC(75e5a22e) SHA1(50a8ca11909ce49412c47de4da69e39a083ce5af) )
	ROM_LOAD( "wow.x4",       0x3000, 0x1000, CRC(ef28eb84) SHA1(d6318b3649fccafc2d0a05e5530e88819d299356) )
	ROM_LOAD( "wow.x5",       0x8000, 0x1000, CRC(16912c2b) SHA1(faf9c96d99bc111c5f1618f6863f22fd9269027b) )
	ROM_LOAD( "wow.x6",       0x9000, 0x1000, CRC(35797f82) SHA1(376bba29e88c16d95438fa996913b76581df0937) )
	ROM_LOAD( "wow.x7",       0xa000, 0x1000, CRC(ce404305) SHA1(a52c6c7b77842f25c79515460be6b7ed959b5edb) )
/*  ROM_LOAD( "wow.x8",       0xc000, CRC(00001000) , ? )   here would go the foreign language ROM */
ROM_END


ROM_START( gorf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "gorf-a.bin",   0x0000, 0x1000, CRC(5b348321) SHA1(76e2e3ad1a66755f1a369167fdb157690fd44a52) )
	ROM_LOAD( "gorf-b.bin",   0x1000, 0x1000, CRC(62d6de77) SHA1(2601faf12d0ab4972c5535ffd722b03ecd8c097c) )
	ROM_LOAD( "gorf-c.bin",   0x2000, 0x1000, CRC(1d3bc9c9) SHA1(0b363a71d7585a4828e08668ebb2999c55e02721) )
	ROM_LOAD( "gorf-d.bin",   0x3000, 0x1000, CRC(70046e56) SHA1(392214cc6ed4155bfe022d36f0f86c2594a5ab57) )
	ROM_LOAD( "gorf-e.bin",   0x8000, 0x1000, CRC(2d456eb5) SHA1(720fb8b48e20c1fc281d8804259016c3c5364a07) )
	ROM_LOAD( "gorf-f.bin",   0x9000, 0x1000, CRC(f7e4e155) SHA1(9c9d6d3bfee6556dc7a01de81d6148dd02f04fc9) )
	ROM_LOAD( "gorf-g.bin",   0xa000, 0x1000, CRC(4e2bd9b9) SHA1(9edccceea5af015275582553ed238c40c73d8f4f) )
	ROM_LOAD( "gorf-h.bin",   0xb000, 0x1000, CRC(fe7b863d) SHA1(5aa8d824814ee1c30eaf0044da78d3aa8220dcaa) )
ROM_END


ROM_START( gorfpgm1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "873a",         0x0000, 0x1000, CRC(97cb4a6a) SHA1(efdae9a437c665fb861665a38c6cb13fd848ad91) )
	ROM_LOAD( "873b",         0x1000, 0x1000, CRC(257236f8) SHA1(d1e8555fe5e6705ef88535bcd6071d1072b01386) )
	ROM_LOAD( "873c",         0x2000, 0x1000, CRC(16b0638b) SHA1(65e1e2e4df80140976915e0982ce3219b14beece) )
	ROM_LOAD( "873d",         0x3000, 0x1000, CRC(b5e821dc) SHA1(152840e353d567cbf5a86206dde70e5b64b27236) )
	ROM_LOAD( "873e",         0x8000, 0x1000, CRC(8e82804b) SHA1(24250edb30efa63c80514629c86c9372b7ca3020) )
	ROM_LOAD( "873f",         0x9000, 0x1000, CRC(715fb4d9) SHA1(c9f33162093e6ed7e3cb6bb716419e5bc43c0381) )
	ROM_LOAD( "873g",         0xa000, 0x1000, CRC(8a066456) SHA1(f64bcdadbc62566b55573039b03baf5358e24a36) )
	ROM_LOAD( "873h",         0xb000, 0x1000, CRC(56d40c7c) SHA1(c7c9a618d9438a76121972ac029ad7036bcf8c6f) )
ROM_END


ROM_START( robby )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rotox1.bin",   0x0000, 0x1000, CRC(a431b85a) SHA1(3478da56addba1cdd98cbef7a15b17fca9aed2cd) )
	ROM_LOAD( "rotox2.bin",   0x1000, 0x1000, CRC(33cdda83) SHA1(ccbc741a2fc0b7385ca42afe5b377432249b44cb) )
	ROM_LOAD( "rotox3.bin",   0x2000, 0x1000, CRC(dbf97491) SHA1(11574baf04af02b38ae147be8409de7c34e87611) )
	ROM_LOAD( "rotox4.bin",   0x3000, 0x1000, CRC(a3b90ac8) SHA1(8c585d26011c9ea047895a0388835ff2bb80e1ff) )
	ROM_LOAD( "rotox5.bin",   0x8000, 0x1000, CRC(46ae8a94) SHA1(218edcc5257c9cc58c5e667fff64767b313daaab) )
	ROM_LOAD( "rotox6.bin",   0x9000, 0x1000, CRC(7916b730) SHA1(c5166625a404da4a93a1a7ae21d01fdb6e78680e) )
	ROM_LOAD( "rotox7.bin",   0xa000, 0x1000, CRC(276dc4a5) SHA1(d740b30c28f6a94ee2348291e80d57af5c2e2d99) )
	ROM_LOAD( "rotox8.bin",   0xb000, 0x1000, CRC(1ef13457) SHA1(4dc1ee9ce2a28c4ba75e630fbfe4659cd68d3a66) )
  	ROM_LOAD( "rotox9.bin",   0xc000, 0x1000, CRC(370352bf) SHA1(72cd35b4306b46de3d2a3e4e46fa4917ed9d18cb) )
	ROM_LOAD( "rotox10.bin",  0xd000, 0x1000, CRC(e762cbda) SHA1(48c274a859963097a90f80c48366250301eddb5f) )
ROM_END


ROM_START( demndrgn )
	ROM_REGION( 0x2a000, REGION_CPU1, 0 )
	ROM_LOAD( "dd-x1.bin",     0x0000, 0x2000, CRC(9aeaf79e) SHA1(c70aa1a31bc085cca904d497c34e55d49fef49b7) )
	ROM_LOAD( "dd-x2.bin",     0x2000, 0x2000, CRC(0c63b624) SHA1(3eaeb4e0820e9dda7233a13bb146acc44402addd) )
	ROM_LOAD( "dd-x9.bin",     0xc000, 0x2000, CRC(3792d632) SHA1(da053df344f39a8f25a2c57fb1a908131c10f248) )
	ROM_LOAD( "dd-x5.bin",     0x14000, 0x2000, CRC(e377e831) SHA1(f53e74b3138611f9385845d6bdeab891b5d15931) )
	ROM_LOAD( "dd-x6.bin",     0x16000, 0x2000, CRC(0fcb46ad) SHA1(5611135f9e341bd394d6da7912167b05fff17a93) )
	ROM_LOAD( "dd-x7.bin",     0x18000, 0x2000, CRC(0675e4fa) SHA1(59668e32271ff9bac0b4411cc0c541d2825ee145) )
	ROM_LOAD( "dd-x10.bin",    0x1c000, 0x2000, CRC(4a22c4f9) SHA1(d86ca38727fcf1896ea64c3b6255e3230054b5d6) )
	ROM_LOAD( "dd-x11.bin",    0x1e000, 0x2000, CRC(d3158845) SHA1(510bb8d459625263370ee68f6f63d6d7abc6d26d) )
	ROM_LOAD( "dd-x12.bin",    0x20000, 0x2000, CRC(592c1d9a) SHA1(c884aabf4cff9f9b974e497fc3a1f8cdd0753680) )
	ROM_LOAD( "dd-x13.bin",    0x22000, 0x2000, CRC(492d7b7e) SHA1(a9a89a61179b154a8afa429e11e984609f787d74) )
	ROM_LOAD( "dd-x14.bin",    0x24000, 0x2000, CRC(7843c818) SHA1(4756bf7dd07071b86645908d61891edcdffdde83) )
	ROM_LOAD( "dd-x15.bin",    0x26000, 0x2000, CRC(6e6bc1b6) SHA1(b8c5ed8df6a709a6502dac47be88271ad22b9203) )
	ROM_LOAD( "dd-x16.bin",    0x28000, 0x2000, CRC(7a4a343b) SHA1(4eb82ae38ce1b14778fb29d8549c61a46bc3ee66) )
ROM_END


ROM_START( profpac )
	ROM_REGION( 0x2c000, REGION_CPU1, 0 )
	ROM_LOAD( "pps1",         0x0000, 0x2000, CRC(a244a62d) SHA1(f7a9606ce6d66c3e6d210cc25572904aeab2b6c8) )
	ROM_LOAD( "pps2",         0x2000, 0x2000, CRC(8a9a6653) SHA1(b730b24088dcfddbe954670ff9212b7383c923f6) )
	ROM_LOAD( "pps3",         0x8000, 0x2000, CRC(15717fd8) SHA1(ffbb156f417d20478117b39de28a15680993b528) )
	ROM_LOAD( "pps4",         0xa000, 0x2000, CRC(36540598) SHA1(33c797c690801afded45091d822347e1ecc72b54) )
	ROM_LOAD( "pps9",         0xc000, 0x2000, CRC(17a0b418) SHA1(8b7ed84090dbc5181deef6f55ec755c05d4c0d5e) )
	ROM_LOAD( "pps5",         0x14000, 0x2000, CRC(8dc89a59) SHA1(fb4d3ba40697425d69ee19bfdcf00aea1df5fa80) )
	ROM_LOAD( "pps6",         0x16000, 0x2000, CRC(5a2186c3) SHA1(f706cef6518b7d839377aa8a7c75fdeed4985c57) )
	ROM_LOAD( "pps7",         0x18000, 0x2000, CRC(f9c26aba) SHA1(201b930cca9669114ffc97978cade69587e34a0f) )
	ROM_LOAD( "pps8",         0x1a000, 0x2000, CRC(4d201e41) SHA1(786b30cd7a7db55bdde05909d7a1a7f122b6e546) )
	/* the rest of the sockets are empty */

	/* Each of these can get mapped from 0x4000-0x7fff */
	ROM_REGION( 0x38000, REGION_USER1, 0 )
	ROM_LOAD( "ppq1",         0x00000, 0x4000, CRC(dddc2ccc) SHA1(d81caaa639f63d971a0d3199b9da6359211edf3d) )
	ROM_LOAD( "ppq2",         0x04000, 0x4000, CRC(33bbcabe) SHA1(f9455868c70f479ede0e0621f21f69da165d9b7a) )
	ROM_LOAD( "ppq3",         0x08000, 0x4000, CRC(3534d895) SHA1(24fb14c6b31b7f27e0737605cfbf963d29dd3fc5) )
	ROM_LOAD( "ppq4",         0x0c000, 0x4000, CRC(17e3581d) SHA1(92d2391e4c8aef46cc8e92b8cf9a8ec9a1b5ff68) )
	ROM_LOAD( "ppq5",         0x10000, 0x4000, CRC(80882a93) SHA1(d5d6afaadb022b109c14c3911eceb0769204df6c) )
	ROM_LOAD( "ppq6",         0x14000, 0x4000, CRC(e5ddaee5) SHA1(45b4925709da6790676319268398f6cfcf12794b) )
	ROM_LOAD( "ppq7",         0x18000, 0x4000, CRC(c029cd34) SHA1(f2f09fdb13920012a6a43958b640d7a06c0c8e69) )
	ROM_LOAD( "ppq8",         0x1c000, 0x4000, CRC(fb3a1ac9) SHA1(e8fe02c85e90320680a14ad560204d5c235730ad) )
	ROM_LOAD( "ppq9",         0x20000, 0x4000, CRC(5e944488) SHA1(2f03f799c319309b5ebf9a5299891d1824398ba5) )
	ROM_LOAD( "ppq10",        0x24000, 0x4000, CRC(ed72a81f) SHA1(db991b93001d2da16b398ee8e9b01b8f0dfe5740) )
	ROM_LOAD( "ppq11",        0x28000, 0x4000, CRC(98295020) SHA1(7f68a8b89117b7ab8724869401a861fe7cff28d9) )
	ROM_LOAD( "ppq12",        0x2c000, 0x4000, CRC(e01a8dbe) SHA1(c7052bf9ce9d2006dda5ddc07ad164d0119b86ea) )
	ROM_LOAD( "ppq13",        0x30000, 0x4000, CRC(87165d4f) SHA1(d47655300c8747698a46f30deb65fe762073e869) )
	ROM_LOAD( "ppq14",        0x34000, 0x4000, CRC(ecb861de) SHA1(73d28a79b76795d3016dd608f9ab3d255f40e477) )
ROM_END



/*************************************
 *
 *  Game-specific driver inits
 *
 *************************************/

static DRIVER_INIT( seawolf2 )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x10, 0x10, 0, 0xff00, seawolf2_controller2_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x11, 0x11, 0, 0xff00, seawolf2_controller1_r);
}

static DRIVER_INIT( ebases )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x13, 0x13, 0, 0xff00, ebases_trackball_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x20, 0x20, 0, 0xff00, ebases_io_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x28, 0x28, 0, 0xff00, ebases_trackball_select_w);
}

static DRIVER_INIT( wow )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x12, 0x12, 0, 0xff00, wow_port_2_r);
	memory_install_read8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x15, 0xff, 0, 0, wow_io_r);
	memory_install_read8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x17, 0xff, 0, 0, wow_speech_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x50, 0x57, 0, 0xff00, astrocade_sound2_w);
	memory_install_write8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x58, 0xff, 0, 0, astrocade_soundblock2_w);
}

static DRIVER_INIT( spacezap )
{
	memory_install_read8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x13, 0xff, 0, 0, spacezap_io_r);
}

static DRIVER_INIT( gorf )
{
	/* This is part of the timing/interrupt hack stuff */
//  extern UINT8 *gorf_timer_ram;
//  gorf_timer_ram = memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd0a5, 0xd0a5, 0, 0, gorf_timer_r);

	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x12, 0x12, 0, 0xff00, gorf_port_2_r);
	memory_install_read8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x15, 0xff, 0, 0, gorf_io_1_r);
	memory_install_read8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x16, 0xff, 0, 0, gorf_io_2_r);
	memory_install_read8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x17, 0xff, 0, 0, gorf_speech_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x50, 0x57, 0, 0xff00, astrocade_sound2_w);
	memory_install_write8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x58, 0xff, 0, 0, astrocade_soundblock2_w);
}

static DRIVER_INIT( robby )
{
	memory_install_read8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x15, 0xff, 0, 0, robby_io_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x50, 0x57, 0, 0xff00, astrocade_sound2_w);
	memory_install_write8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x58, 0xff, 0, 0, astrocade_soundblock2_w);
}

static DRIVER_INIT( demndrgn )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x10, 0x10, 0, 0xff00, input_port_0_r );

	/* 0x00 is the middle value, range is up to 0x7f, and down to 0x80 */
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x11, 0x11, 0, 0xff00, demndrgn_move_r );

	memory_install_read8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x14, 0xff, 0, 0, demndrgn_io_r );

	/* analog joystick, converted to digital in software */
	/* 0x80 is the middle, values of > 0xd8 and < 0x28 are the thresholds */
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x1c, 0x1c, 0, 0xff00, demndrgn_fire_x_r );
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x1d, 0x1d, 0, 0xff00, demndrgn_fire_y_r );

	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x97, 0x97, 0, 0xff00, demndrgn_sound_w );

	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0xbf, 0xbf, 0, 0xff00, profpac_page_select_w);
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0xc3, 0xc3, 0, 0xff00, profpac_intercept_r );
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0xc0, 0xc5, 0, 0xff00, profpac_screenram_ctrl_w );
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0xf3, 0xf3, 0, 0xff00, profpac_banksw_w);
}

static DRIVER_INIT( profpac )
{
	memory_install_read8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x14, 0xff, 0, 0, profpac_io_1_r);
	memory_install_read8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x15, 0xff, 0, 0, profpac_io_2_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x50, 0x57, 0, 0xff00, astrocade_sound2_w);
	memory_install_write8_matchmask_handler(0, ADDRESS_SPACE_IO, 0x58, 0xff, 0, 0, astrocade_soundblock2_w);

	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0xbf, 0xbf, 0, 0xff00, profpac_page_select_w);
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0xc3, 0xc3, 0, 0xff00, profpac_intercept_r );
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0xc0, 0xc5, 0, 0xff00, profpac_screenram_ctrl_w );
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0xf3, 0xf3, 0, 0xff00, profpac_banksw_w);
}



/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1978, seawolf2, 0,    seawolf2, seawolf2, seawolf2, ROT0,   "Midway", "Sea Wolf II", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
GAME( 1980, spacezap, 0,    spacezap, spacezap, spacezap, ROT0,   "Midway", "Space Zap", GAME_SUPPORTS_SAVE )
GAME( 1980, ebases,   0,    ebases,   ebases,   ebases,   ROT0,   "Midway", "Extra Bases", GAME_SUPPORTS_SAVE )
GAME( 1980, wow,      0,    wow,      wow,      wow,      ROT0,   "Midway", "Wizard of Wor", GAME_SUPPORTS_SAVE )
GAME( 1981, gorf,     0,    gorf,     gorf,     gorf,     ROT270, "Midway", "Gorf", GAME_SUPPORTS_SAVE )
GAME( 1981, gorfpgm1, gorf, gorf,     gorf,     gorf,     ROT270, "Midway", "Gorf (Program 1)", GAME_SUPPORTS_SAVE )
GAME( 1981, robby,    0,    robby,    robby,    robby,    ROT0,   "Bally Midway", "Robby Roto", GAME_SUPPORTS_SAVE )
GAME( 1982, demndrgn, 0,    demndrgn, demndrgn, demndrgn, ROT0,   "Bally Midway", "Demons and Dragons (prototype)", GAME_NO_SOUND | GAME_SUPPORTS_SAVE )
GAME( 1983, profpac,  0,    profpac,  profpac,  profpac,  ROT0,   "Bally Midway", "Professor PacMan", GAME_SUPPORTS_SAVE )
