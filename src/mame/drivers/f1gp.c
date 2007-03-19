/***************************************************************************

F-1 Grand Prix       (c) 1991 Video System Co.

driver by Nicola Salmoria

Notes:
- The ROZ layer generator is a Konami 053936.
- f1gp2's hardware is very similar to Lethal Crash Race, main difference
  being an extra 68000.

TODO:
f1gp:
- gfxctrl register not understood - handling of fg/sprite priority to fix
  "continue" screen is just a kludge.
f1gp2:
- sprite lag noticeable in the animation at the end of a race (the wheels
  of the car are sprites while the car is the fg tilemap)

***************************************************************************/

#include "driver.h"
#include "video/konamiic.h"
#include "f1gp.h"
#include "sound/2610intf.h"



static UINT16 *sharedram;

static READ16_HANDLER( sharedram_r )
{
	return sharedram[offset];
}

static WRITE16_HANDLER( sharedram_w )
{
	COMBINE_DATA(&sharedram[offset]);
}

static READ16_HANDLER( extrarom_r )
{
	UINT8 *rom = memory_region(REGION_USER1);

	offset *= 2;

	return rom[offset] | (rom[offset+1] << 8);
}

static READ16_HANDLER( extrarom2_r )
{
	UINT8 *rom = memory_region(REGION_USER2);

	offset *= 2;

	return rom[offset] | (rom[offset+1] << 8);
}

static WRITE8_HANDLER( f1gp_sh_bankswitch_w )
{
	UINT8 *rom = memory_region(REGION_CPU3) + 0x10000;

	memory_set_bankptr(1,rom + (data & 0x01) * 0x8000);
}


static int pending_command;

static WRITE16_HANDLER( sound_command_w )
{
	if (ACCESSING_LSB)
	{
		pending_command = 1;
		soundlatch_w(offset,data & 0xff);
		cpunum_set_input_line(2, INPUT_LINE_NMI, PULSE_LINE);
	}
}

static READ16_HANDLER( command_pending_r )
{
	return (pending_command ? 0xff : 0);
}

static WRITE8_HANDLER( pending_command_clear_w )
{
	pending_command = 0;
}



static ADDRESS_MAP_START( f1gp_readmem1, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x2fffff) AM_READ(extrarom_r)
	AM_RANGE(0xa00000, 0xbfffff) AM_READ(extrarom2_r)
	AM_RANGE(0xc00000, 0xc3ffff) AM_READ(f1gp_zoomdata_r)
	AM_RANGE(0xd00000, 0xd01fff) AM_READ(f1gp_rozvideoram_r)
	AM_RANGE(0xd02000, 0xd03fff) AM_READ(f1gp_rozvideoram_r)	/* mirror */
	AM_RANGE(0xd04000, 0xd05fff) AM_READ(f1gp_rozvideoram_r)	/* mirror */
	AM_RANGE(0xd06000, 0xd07fff) AM_READ(f1gp_rozvideoram_r)	/* mirror */
	AM_RANGE(0xe00000, 0xe03fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xe04000, 0xe07fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xf00000, 0xf003ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xf10000, 0xf103ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xff8000, 0xffbfff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffc000, 0xffcfff) AM_READ(sharedram_r)
	AM_RANGE(0xffd000, 0xffdfff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffe000, 0xffefff) AM_READ(MRA16_RAM)
	AM_RANGE(0xfff000, 0xfff001) AM_READ(input_port_0_word_r)
//  AM_RANGE(0xfff002, 0xfff003)    analog wheel?
	AM_RANGE(0xfff004, 0xfff005) AM_READ(input_port_1_word_r)
	AM_RANGE(0xfff006, 0xfff007) AM_READ(input_port_2_word_r)
	AM_RANGE(0xfff008, 0xfff009) AM_READ(command_pending_r)
	AM_RANGE(0xfff050, 0xfff051) AM_READ(input_port_3_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( f1gp_writemem1, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0xc00000, 0xc3ffff) AM_WRITE(f1gp_zoomdata_w)
	AM_RANGE(0xd00000, 0xd01fff) AM_WRITE(f1gp_rozvideoram_w) AM_BASE(&f1gp_rozvideoram)					// BACK VRAM
	AM_RANGE(0xd02000, 0xd03fff) AM_WRITE(f1gp_rozvideoram_w)	/* mirror */
	AM_RANGE(0xd04000, 0xd05fff) AM_WRITE(f1gp_rozvideoram_w)	/* mirror */
	AM_RANGE(0xd06000, 0xd07fff) AM_WRITE(f1gp_rozvideoram_w)	/* mirror */
	AM_RANGE(0xe00000, 0xe03fff) AM_WRITE(MWA16_RAM) AM_BASE(&f1gp_spr1cgram) AM_SIZE(&f1gp_spr1cgram_size)		// SPR-1 CG RAM
	AM_RANGE(0xe04000, 0xe07fff) AM_WRITE(MWA16_RAM) AM_BASE(&f1gp_spr2cgram) AM_SIZE(&f1gp_spr2cgram_size)		// SPR-2 CG RAM
	AM_RANGE(0xf00000, 0xf003ff) AM_WRITE(MWA16_RAM) AM_BASE(&f1gp_spr1vram)								// SPR-1 VRAM
	AM_RANGE(0xf10000, 0xf103ff) AM_WRITE(MWA16_RAM) AM_BASE(&f1gp_spr2vram)								// SPR-2 VRAM
	AM_RANGE(0xff8000, 0xffbfff) AM_WRITE(MWA16_RAM)												// WORK RAM-1
	AM_RANGE(0xffc000, 0xffcfff) AM_WRITE(sharedram_w) AM_BASE(&sharedram)								// DUAL RAM
	AM_RANGE(0xffd000, 0xffdfff) AM_WRITE(f1gp_fgvideoram_w) AM_BASE(&f1gp_fgvideoram)					// CHARACTER
	AM_RANGE(0xffe000, 0xffefff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)	// PALETTE
	AM_RANGE(0xfff000, 0xfff001) AM_WRITE(f1gp_gfxctrl_w)
	AM_RANGE(0xfff002, 0xfff005) AM_WRITE(f1gp_fgscroll_w)
	AM_RANGE(0xfff008, 0xfff009) AM_WRITE(sound_command_w)
	AM_RANGE(0xfff040, 0xfff05f) AM_WRITE(MWA16_RAM) AM_BASE(&K053936_0_ctrl)
ADDRESS_MAP_END

static ADDRESS_MAP_START( f1gp2_readmem1, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x2fffff) AM_READ(extrarom_r)
	AM_RANGE(0xa00000, 0xa07fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xd00000, 0xd01fff) AM_READ(f1gp_rozvideoram_r)
	AM_RANGE(0xe00000, 0xe00fff) AM_READ(MRA16_RAM)
	AM_RANGE(0xff8000, 0xffbfff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffc000, 0xffcfff) AM_READ(sharedram_r)
	AM_RANGE(0xffd000, 0xffdfff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffe000, 0xffefff) AM_READ(MRA16_RAM)
	AM_RANGE(0xfff000, 0xfff001) AM_READ(input_port_0_word_r)
//  AM_RANGE(0xfff002, 0xfff003)    analog wheel?
	AM_RANGE(0xfff004, 0xfff005) AM_READ(input_port_1_word_r)
	AM_RANGE(0xfff006, 0xfff007) AM_READ(input_port_2_word_r)
	AM_RANGE(0xfff008, 0xfff009) AM_READ(command_pending_r)
	AM_RANGE(0xfff00a, 0xfff00b) AM_READ(input_port_3_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( f1gp2_writemem1, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0xa00000, 0xa07fff) AM_WRITE(MWA16_RAM) AM_BASE(&f1gp2_sprcgram)								// SPR-1 CG RAM + SPR-2 CG RAM
	AM_RANGE(0xd00000, 0xd01fff) AM_WRITE(f1gp_rozvideoram_w) AM_BASE(&f1gp_rozvideoram)					// BACK VRAM
	AM_RANGE(0xe00000, 0xe00fff) AM_WRITE(MWA16_RAM) AM_BASE(&f1gp2_spritelist)							// not checked + SPR-1 VRAM + SPR-2 VRAM
	AM_RANGE(0xff8000, 0xffbfff) AM_WRITE(MWA16_RAM)												// WORK RAM-1
	AM_RANGE(0xffc000, 0xffcfff) AM_WRITE(sharedram_w) AM_BASE(&sharedram)								// DUAL RAM
	AM_RANGE(0xffd000, 0xffdfff) AM_WRITE(f1gp_fgvideoram_w) AM_BASE(&f1gp_fgvideoram)					// CHARACTER
	AM_RANGE(0xffe000, 0xffefff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)	// PALETTE
	AM_RANGE(0xfff000, 0xfff001) AM_WRITE(f1gp2_gfxctrl_w)
	AM_RANGE(0xfff008, 0xfff009) AM_WRITE(sound_command_w)
	AM_RANGE(0xfff020, 0xfff02f) AM_WRITE(MWA16_RAM) AM_BASE(&K053936_0_ctrl)
	AM_RANGE(0xfff044, 0xfff047) AM_WRITE(f1gp_fgscroll_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0xff8000, 0xffbfff) AM_READ(MRA16_RAM)
	AM_RANGE(0xffc000, 0xffcfff) AM_READ(sharedram_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0xff8000, 0xffbfff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0xffc000, 0xffcfff) AM_WRITE(sharedram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x77ff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7800, 0x7fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x77ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x7800, 0x7fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x14, 0x14) AM_READ(soundlatch_r)
	AM_RANGE(0x18, 0x18) AM_READ(YM2610_status_port_0_A_r)
	AM_RANGE(0x1a, 0x1a) AM_READ(YM2610_status_port_0_B_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(f1gp_sh_bankswitch_w)	// f1gp
	AM_RANGE(0x0c, 0x0c) AM_WRITE(f1gp_sh_bankswitch_w)	// f1gp2
	AM_RANGE(0x14, 0x14) AM_WRITE(pending_command_clear_w)
	AM_RANGE(0x18, 0x18) AM_WRITE(YM2610_control_port_0_A_w)
	AM_RANGE(0x19, 0x19) AM_WRITE(YM2610_data_port_0_A_w)
	AM_RANGE(0x1a, 0x1a) AM_WRITE(YM2610_control_port_0_B_w)
	AM_RANGE(0x1b, 0x1b) AM_WRITE(YM2610_data_port_0_B_w)
ADDRESS_MAP_END



INPUT_PORTS_START( f1gp )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e00, 0x0e00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0a00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0e00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x7000, 0x7000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x5000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x7000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x8000, 0x8000, "2 to Start, 1 to Cont." )	// Other desc. was too long !
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Game Mode" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Single ) )
	PORT_DIPSETTING(      0x0000, "Multiple" )
	PORT_DIPNAME( 0x0008, 0x0008, "Multi Player" )
	PORT_DIPSETTING(      0x0008, "Type 1" )
	PORT_DIPSETTING(      0x0000, "Type 2" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x001f, 0x0010, "Country" )
	PORT_DIPSETTING(      0x0010, DEF_STR( World ) )
	PORT_DIPSETTING(      0x0001, "USA & Canada" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Japan ) )
	PORT_DIPSETTING(      0x0002, "Korea" )
	PORT_DIPSETTING(      0x0004, "Hong Kong" )
	PORT_DIPSETTING(      0x0008, "Taiwan" )
	/* all other values are invalid */
INPUT_PORTS_END


/* the same as f1gp, but with an extra button */
INPUT_PORTS_START( f1gp2 )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e00, 0x0e00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0a00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0e00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x7000, 0x7000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x5000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x7000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x8000, 0x8000, "2 to Start, 1 to Cont." )	// Other desc. was too long !
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Game Mode" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Single ) )
	PORT_DIPSETTING(      0x0000, "Multiple" )
	PORT_DIPNAME( 0x0008, 0x0008, "Multi Player" )
	PORT_DIPSETTING(      0x0008, "Type 1" )
	PORT_DIPSETTING(      0x0000, "Type 2" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "Country" )
	PORT_DIPSETTING(      0x0001, DEF_STR( World ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Japan ) )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

static const gfx_layout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4,
			10*4, 11*4, 8*4, 9*4, 14*4, 15*4, 12*4, 13*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	64*16
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4,
			9*4, 8*4, 11*4, 10*4, 13*4, 12*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static const gfx_decode f1gp_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x000,  1 },
	{ REGION_GFX2, 0, &spritelayout, 0x100, 16 },
	{ REGION_GFX3, 0, &spritelayout, 0x200, 16 },
	{ REGION_GFX4, 0, &tilelayout,   0x300, 16 },	/* changed at runtime */
	{ -1 } /* end of array */
};

static const gfx_decode f1gp2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x000,  1 },
	{ REGION_GFX2, 0, &spritelayout, 0x200, 32 },
	{ REGION_GFX3, 0, &tilelayout,   0x100, 16 },
	{ -1 } /* end of array */
};



static void irqhandler(int irq)
{
	cpunum_set_input_line(2,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	irqhandler,
	REGION_SOUND1,
	REGION_SOUND2
};



static MACHINE_DRIVER_START( f1gp )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",M68000,10000000)	/* 10 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(f1gp_readmem1,f1gp_writemem1)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(M68000,10000000)	/* 10 MHz ??? */
	MDRV_CPU_PROGRAM_MAP(readmem2,writemem2)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)	/* 4 MHz ??? */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_INTERLEAVE(100) /* 100 CPU slices per frame */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(f1gp_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(f1gp)
	MDRV_VIDEO_UPDATE(f1gp)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.25)
	MDRV_SOUND_ROUTE(0, "right", 0.25)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( f1gp2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(f1gp)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(f1gp2_readmem1,f1gp2_writemem1)

	/* video hardware */
	MDRV_GFXDECODE(f1gp2_gfxdecodeinfo)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)

	MDRV_VIDEO_START(f1gp2)
	MDRV_VIDEO_UPDATE(f1gp2)
MACHINE_DRIVER_END



ROM_START( f1gp )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "rom1-a.3",     0x000000, 0x20000, CRC(2d8f785b) SHA1(6eca42ad2d57a31e055496141c89cb537f284378) )

	ROM_REGION( 0x200000, REGION_USER1, 0 )	/* extra ROMs mapped at 100000 */
	ROM_LOAD16_BYTE( "rom11-a.2",    0x000000, 0x40000, CRC(53df8ea1) SHA1(25d50bb787f3bd35c9a8ae2b0ab9a21e000debb0) )
	ROM_LOAD16_BYTE( "rom10-a.1",    0x000001, 0x40000, CRC(46a289fb) SHA1(6a8c19e08b6d836fe83378fd77fead82a0b2db7c) )
	ROM_LOAD16_BYTE( "rom13-a.4",    0x080000, 0x40000, CRC(7d92e1fa) SHA1(c23f5beea85b0804c61ef9e7f131b186d076221f) )
	ROM_LOAD16_BYTE( "rom12-a.3",    0x080001, 0x40000, CRC(d8c1bcf4) SHA1(d6d77354eb1ab413ba8cfa5d973cf5b0c851c23b) )
	ROM_LOAD16_BYTE( "rom6-a.6",     0x100000, 0x40000, CRC(6d947a3f) SHA1(2cd01ee2a73ab105a45a5464a29fd75aa43ba2db) )
	ROM_LOAD16_BYTE( "rom7-a.5",     0x100001, 0x40000, CRC(7a014ba6) SHA1(8f0abbb68100e396e5a41337254cb6bf1a2ed00b) )
	ROM_LOAD16_BYTE( "rom9-a.8",     0x180000, 0x40000, CRC(49286572) SHA1(c5e16bd1ccd43452337a4cd76db70db079ca0706) )
	ROM_LOAD16_BYTE( "rom8-a.7",     0x180001, 0x40000, CRC(0ed783c7) SHA1(c0c467ede51c08d84999897c6d5cc8b584b23b67) )

	ROM_REGION( 0x200000, REGION_USER2, 0 )	/* extra ROMs mapped at a00000 */
											/* containing gfx data for the 053936 */
	ROM_LOAD( "rom2-a.06",    0x000000, 0x100000, CRC(747dd112) SHA1(b9264bec61467ab256cf6cb698b6e0ea8f8006e0) )
	ROM_LOAD( "rom3-a.05",    0x100000, 0x100000, CRC(264aed13) SHA1(6f0de860d4299befffc530b7a8f19656982a51c4) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "rom4-a.4",     0x000000, 0x20000, CRC(8e811d36) SHA1(2b806b50a3a307a21894687f16485ace287a7c4c) )

	ROM_REGION( 0x30000, REGION_CPU3, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "rom5-a.8",     0x00000, 0x08000, CRC(9ea36e35) SHA1(9254dea8362318d8cfbd5e36e476e0e235e6326a) )
	ROM_CONTINUE(             0x10000, 0x18000 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom3-b.07",    0x000000, 0x100000, CRC(ffb1d489) SHA1(9330b67e0eaaf67d6c38f40a02c72419bd38fb81) )
	ROM_LOAD( "rom2-b.04",    0x100000, 0x100000, CRC(d1b3471f) SHA1(d1a95fbaad1c3d9ec2121bf65abbcdb5441bd0ac) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_WORD( "rom5-b.2",     0x000000, 0x80000, CRC(17572b36) SHA1(c58327c2f708783a3e8470e290cae0d71454f1da) )
	ROM_LOAD32_WORD( "rom4-b.3",     0x000002, 0x80000, CRC(72d12129) SHA1(11da6990a54ae1b6f6d0bed5d0431552f83a0dda) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD32_WORD( "rom7-b.17",    0x000000, 0x40000, CRC(2aed9003) SHA1(45ff9953ad98063573e7fd7b930ae8b0183cdd04) )
	ROM_LOAD32_WORD( "rom6-b.16",    0x000002, 0x40000, CRC(6789ef12) SHA1(9b0d1cc6e9c6398ccb7f635c4c148fddd224a21f) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_ERASE00 )	/* gfx data for the 053936 */
	/* RAM, not ROM - handled at run time */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "rom14-a.09",   0x000000, 0x100000, CRC(b4c1ac31) SHA1(acab2e1b5ce4ca3a5c4734562481b54db4b46995) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "rom17-a.08",   0x000000, 0x100000, CRC(ea70303d) SHA1(8de1a0e6d47cd80a622663c1745a1da54cd0ea05) )
ROM_END

ROM_START( f1gp2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "rom12.v1",     0x000000, 0x20000, CRC(c5c5f199) SHA1(56fcbf1d9b15a37204296c578e1585599f76a107) )
	ROM_LOAD16_BYTE( "rom14.v2",     0x000001, 0x20000, CRC(dd5388e2) SHA1(66e88f86edc2407e5794519f988203a52d65636d) )

	ROM_REGION( 0x200000, REGION_USER1, 0 )	/* extra ROMs mapped at 100000 */
	ROM_LOAD( "rom2",         0x100000, 0x100000, CRC(3b0cfa82) SHA1(ea6803dd8d30aa9f3bd578e113fc26f20c640751) )
	ROM_CONTINUE(             0x000000, 0x100000 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "rom13.v3",     0x000000, 0x20000, CRC(c37aa303) SHA1(0fe09b398191888620fb676ed0f1593be575512d) )

	ROM_REGION( 0x30000, REGION_CPU3, 0 )	/* 64k for the audio CPU + banks */
	ROM_LOAD( "rom5.v4",      0x00000, 0x08000, CRC(6a9398a1) SHA1(e907fe5f9c135c5b10ec650ec0c6d08cb856230c) )
	ROM_CONTINUE(             0x10000, 0x18000 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom1",         0x000000, 0x200000, CRC(f2d55ad7) SHA1(2f2d9dc4fab63b06ed7cba0ef1ced286dbfaa7b4) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rom15",        0x000000, 0x200000, CRC(1ac03e2e) SHA1(9073d0ae24364229a993046bd71e403988692993) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "rom11",        0x000000, 0x100000, CRC(b22a2c1f) SHA1(b5e67726be5a8561cd04c3c07895b8518b73b89c) )
	ROM_LOAD( "rom10",        0x100000, 0x100000, CRC(43fcbe23) SHA1(54ab58d904890a0b907e674f855092e974c45edc) )
	ROM_LOAD( "rom9",         0x200000, 0x100000, CRC(1bede8a1) SHA1(325ecc3afb30d281c2c8a56719e83e4dc20545bb) )
	ROM_LOAD( "rom8",         0x300000, 0x100000, CRC(98baf2a1) SHA1(df7bd1a743ad0a6e067641e2b7a352c466875ef6) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "rom4",         0x000000, 0x080000, CRC(c2d3d7ad) SHA1(3178096741583cfef1ca8f53e6efa0a59e1d5cb6) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "rom3",         0x000000, 0x100000, CRC(7f8f066f) SHA1(5e051d5feb327ac818e9c7f7ac721dada3a102b6) )
ROM_END



GAME( 1991, f1gp,  0, f1gp,  f1gp,  0, ROT90, "Video System Co.", "F-1 Grand Prix",         GAME_NO_COCKTAIL )
GAME( 1992, f1gp2, 0, f1gp2, f1gp2, 0, ROT90, "Video System Co.", "F-1 Grand Prix Part II", GAME_NO_COCKTAIL )
