/***************************************************************************

                            -=  SunA 16 Bit Games =-

                    driver by   Luca Elia (l.elia@tin.it)


CPU:    68000   +  Z80 [Music]  +  Z80 x 2 [4 Bit PCM]
Sound:  YM2151  +  DAC x 4


---------------------------------------------------------------------------
Year + Game                 By      Hardware
---------------------------------------------------------------------------
94  Suna Quiz 6000          SunA    68000 + Z80 x 2 + YM2151 + DAC x 2
96  Ultra Balloon           SunA    68000 + Z80 x 2 + YM2151 + DAC x 2
96  Back Street Soccer      SunA    68000 + Z80 x 3 + YM2151 + DAC x 4
---------------------------------------------------------------------------


***************************************************************************/

#include "driver.h"
#include "sound/dac.h"
#include "sound/2151intf.h"

/* Variables and functions defined in video: */

WRITE16_HANDLER( suna16_flipscreen_w );

READ16_HANDLER ( suna16_paletteram16_r );
WRITE16_HANDLER( suna16_paletteram16_w );

VIDEO_START( suna16 );
VIDEO_UPDATE( suna16 );


/***************************************************************************


                                Main CPU


***************************************************************************/

WRITE16_HANDLER( suna16_soundlatch_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w( 0, data & 0xff );
	}
	if (data & ~0xff)	logerror("CPU#0 PC %06X - Sound latch unknown bits: %04X\n", activecpu_get_pc(), data);
}


WRITE16_HANDLER( bssoccer_leds_w )
{
	if (ACCESSING_LSB)
	{
		set_led_status(0, data & 0x01);
		set_led_status(1, data & 0x02);
		set_led_status(2, data & 0x04);
		set_led_status(3, data & 0x08);
		coin_counter_w(0, data & 0x10);
	}
	if (data & ~0x1f)	logerror("CPU#0 PC %06X - Leds unknown bits: %04X\n", activecpu_get_pc(), data);
}


WRITE16_HANDLER( uballoon_leds_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0, data & 0x01);
		set_led_status(0, data & 0x02);
		set_led_status(1, data & 0x04);
	}
	if (data & ~0x07)	logerror("CPU#0 PC %06X - Leds unknown bits: %04X\n", activecpu_get_pc(), data);
}


/***************************************************************************
                            Back Street Soccer
***************************************************************************/

static ADDRESS_MAP_START( bssoccer_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA16_ROM 				)	// ROM
	AM_RANGE(0x200000, 0x203fff) AM_READ(MRA16_RAM 				)	// RAM
	AM_RANGE(0x400000, 0x4001ff) AM_READ(suna16_paletteram16_r	)	// Banked Palette
	AM_RANGE(0x400200, 0x400fff) AM_READ(MRA16_RAM 				)	//
	AM_RANGE(0x600000, 0x61ffff) AM_READ(MRA16_RAM 				)	// Sprites
	AM_RANGE(0xa00000, 0xa00001) AM_READ(input_port_0_word_r		)	// P1 (Inputs)
	AM_RANGE(0xa00002, 0xa00003) AM_READ(input_port_1_word_r		)	// P2
	AM_RANGE(0xa00004, 0xa00005) AM_READ(input_port_2_word_r		)	// P3
	AM_RANGE(0xa00006, 0xa00007) AM_READ(input_port_3_word_r		)	// P4
	AM_RANGE(0xa00008, 0xa00009) AM_READ(input_port_4_word_r		)	// DSWs
	AM_RANGE(0xa0000a, 0xa0000b) AM_READ(input_port_5_word_r		)	// Coins
ADDRESS_MAP_END

static ADDRESS_MAP_START( bssoccer_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA16_ROM 				)	// ROM
	AM_RANGE(0x200000, 0x203fff) AM_WRITE(MWA16_RAM 				)	// RAM
	AM_RANGE(0x400000, 0x4001ff) AM_WRITE(suna16_paletteram16_w) AM_BASE(&paletteram16)  // Banked Palette
	AM_RANGE(0x400200, 0x400fff) AM_WRITE(MWA16_RAM 				)	//
	AM_RANGE(0x600000, 0x61ffff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16	)	// Sprites
	AM_RANGE(0xa00000, 0xa00001) AM_WRITE(suna16_soundlatch_w		)	// To Sound CPU
	AM_RANGE(0xa00002, 0xa00003) AM_WRITE(suna16_flipscreen_w		)	// Flip Screen
	AM_RANGE(0xa00004, 0xa00005) AM_WRITE(bssoccer_leds_w			)	// Leds
	AM_RANGE(0xa00006, 0xa00007) AM_WRITE(MWA16_NOP					)	// ? IRQ 1 Ack
	AM_RANGE(0xa00008, 0xa00009) AM_WRITE(MWA16_NOP					)	// ? IRQ 2 Ack
ADDRESS_MAP_END


/***************************************************************************
                                Ultra Balloon
***************************************************************************/

WRITE16_HANDLER( uballoon_spriteram16_w )
{
	COMBINE_DATA( &spriteram16[offset] );
}

static ADDRESS_MAP_START( uballoon_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM 				)	// ROM
	AM_RANGE(0x800000, 0x803fff) AM_READ(MRA16_RAM 				)	// RAM
	AM_RANGE(0x200000, 0x2001ff) AM_READ(suna16_paletteram16_r	)	// Banked Palette
	AM_RANGE(0x200200, 0x200fff) AM_READ(MRA16_RAM 				)	//
	AM_RANGE(0x400000, 0x41ffff) AM_READ(MRA16_RAM 				)	// Sprites
	AM_RANGE(0x600000, 0x600001) AM_READ(input_port_0_word_r		)	// P1 + Coins(Inputs)
	AM_RANGE(0x600002, 0x600003) AM_READ(input_port_1_word_r		)	// P2 + Coins
	AM_RANGE(0x600004, 0x600005) AM_READ(input_port_2_word_r		)	// DSW 1
	AM_RANGE(0x600006, 0x600007) AM_READ(input_port_3_word_r		)	// DSW 2
	AM_RANGE(0xa00000, 0xa0ffff) AM_READ(MRA16_NOP					)	// Protection
ADDRESS_MAP_END

static ADDRESS_MAP_START( uballoon_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM 				)	// ROM
	AM_RANGE(0x800000, 0x803fff) AM_WRITE(MWA16_RAM 				)	// RAM
	AM_RANGE(0x200000, 0x2001ff) AM_WRITE(suna16_paletteram16_w) AM_BASE(&paletteram16)  // Banked Palette
	AM_RANGE(0x200200, 0x200fff) AM_WRITE(MWA16_RAM 				)	//
	AM_RANGE(0x400000, 0x41ffff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16	)	// Sprites
	AM_RANGE(0x5c0000, 0x5dffff) AM_WRITE(uballoon_spriteram16_w	)	// Sprites (Mirror?)
	AM_RANGE(0x600000, 0x600001) AM_WRITE(suna16_soundlatch_w		)	// To Sound CPU
	AM_RANGE(0x600004, 0x600005) AM_WRITE(suna16_flipscreen_w		)	// Flip Screen
	AM_RANGE(0x600008, 0x600009) AM_WRITE(uballoon_leds_w			)	// Leds
	AM_RANGE(0x60000c, 0x60000d) AM_WRITE(MWA16_NOP					)	// ? IRQ 1 Ack
	AM_RANGE(0x600010, 0x600011) AM_WRITE(MWA16_NOP					)	// ? IRQ 1 Ack
	AM_RANGE(0xa00000, 0xa0ffff) AM_WRITE(MWA16_NOP					)	// Protection
ADDRESS_MAP_END

/***************************************************************************
                                Suna Quiz 6000
***************************************************************************/

static ADDRESS_MAP_START( sunaq_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM 				)	// ROM
	AM_RANGE(0x500000, 0x500001) AM_READ(input_port_0_word_r		)	// P1 + Coins
	AM_RANGE(0x500002, 0x500003) AM_READ(input_port_1_word_r		)	// P2 + Coins
	AM_RANGE(0x500004, 0x500005) AM_READ(input_port_2_word_r		)	// DSW
	AM_RANGE(0x500006, 0x500007) AM_READ(input_port_3_word_r		)	// (unused?)
	AM_RANGE(0x540000, 0x5401ff) AM_READ(suna16_paletteram16_r	)	// Banked Palette
	AM_RANGE(0x540200, 0x540fff) AM_READ(MRA16_RAM 				)	// RAM
	AM_RANGE(0x580000, 0x583fff) AM_READ(MRA16_RAM 				)	// RAM
	AM_RANGE(0x5c0000, 0x5dffff) AM_READ(MRA16_RAM 				)	// RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sunaq_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM 				)	// ROM
	AM_RANGE(0x500000, 0x500001) AM_WRITE(suna16_soundlatch_w		)	// To Sound CPU
	AM_RANGE(0x500002, 0x500003) AM_WRITE(suna16_flipscreen_w		)	// Flip Screen
	AM_RANGE(0x540000, 0x5401ff) AM_WRITE(suna16_paletteram16_w) AM_BASE(&paletteram16)
	AM_RANGE(0x540200, 0x540fff) AM_WRITE(MWA16_RAM                 )   // RAM
	AM_RANGE(0x580000, 0x583fff) AM_WRITE(MWA16_RAM 				)	// RAM
	AM_RANGE(0x5c0000, 0x5dffff) AM_WRITE(uballoon_spriteram16_w) AM_BASE(&spriteram16 )	// Sprites
ADDRESS_MAP_END

/***************************************************************************


                                    Z80 #1

        Plays the music (YM2151) and controls the 2 Z80s in charge
        of playing the PCM samples


***************************************************************************/

/***************************************************************************
                            Back Street Soccer
***************************************************************************/

static ADDRESS_MAP_START( bssoccer_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM					)	// ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READ(MRA8_RAM					)	// RAM
	AM_RANGE(0xf801, 0xf801) AM_READ(YM2151_status_port_0_r	)	// YM2151
	AM_RANGE(0xfc00, 0xfc00) AM_READ(soundlatch_r				)	// From Main CPU
ADDRESS_MAP_END

static ADDRESS_MAP_START( bssoccer_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM					)	// ROM
	AM_RANGE(0xf000, 0xf7ff) AM_WRITE(MWA8_RAM					)	// RAM
	AM_RANGE(0xf800, 0xf800) AM_WRITE(YM2151_register_port_0_w	)	// YM2151
	AM_RANGE(0xf801, 0xf801) AM_WRITE(YM2151_data_port_0_w		)	//
	AM_RANGE(0xfd00, 0xfd00) AM_WRITE(soundlatch2_w 			)	// To PCM Z80 #1
	AM_RANGE(0xfe00, 0xfe00) AM_WRITE(soundlatch3_w 			)	// To PCM Z80 #2
ADDRESS_MAP_END

/***************************************************************************
                                Ultra Balloon
***************************************************************************/

static ADDRESS_MAP_START( uballoon_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_READ(MRA8_ROM					)	// ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READ(MRA8_RAM					)	// RAM
	AM_RANGE(0xf801, 0xf801) AM_READ(YM2151_status_port_0_r	)	// YM2151
	AM_RANGE(0xfc00, 0xfc00) AM_READ(soundlatch_r				)	// From Main CPU
ADDRESS_MAP_END

static ADDRESS_MAP_START( uballoon_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_WRITE(MWA8_ROM					)	// ROM
	AM_RANGE(0xf000, 0xf7ff) AM_WRITE(MWA8_RAM					)	// RAM
	AM_RANGE(0xf800, 0xf800) AM_WRITE(YM2151_register_port_0_w	)	// YM2151
	AM_RANGE(0xf801, 0xf801) AM_WRITE(YM2151_data_port_0_w		)	//
	AM_RANGE(0xfc00, 0xfc00) AM_WRITE(soundlatch2_w				)	// To PCM Z80
ADDRESS_MAP_END

/***************************************************************************
                                Suna Quiz 6000
***************************************************************************/

static ADDRESS_MAP_START( sunaq_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xe82f) AM_READ(MRA8_ROM					)	// ROM
	AM_RANGE(0xe830, 0xf7ff) AM_READ(MRA8_RAM					)	// RAM
	AM_RANGE(0xf801, 0xf801) AM_READ(YM2151_status_port_0_r	)	// YM2151
	AM_RANGE(0xfc00, 0xfc00) AM_READ(soundlatch_r				)	// From Main CPU
ADDRESS_MAP_END

static ADDRESS_MAP_START( sunaq_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xe82f) AM_WRITE(MWA8_ROM					)	// ROM
	AM_RANGE(0xe830, 0xf7ff) AM_WRITE(MWA8_RAM					)	// RAM (writes to efxx, could be a program bug tho)
	AM_RANGE(0xf800, 0xf800) AM_WRITE(YM2151_register_port_0_w	)	// YM2151
	AM_RANGE(0xf801, 0xf801) AM_WRITE(YM2151_data_port_0_w		)	//
	AM_RANGE(0xfc00, 0xfc00) AM_WRITE(soundlatch2_w				)	// To PCM Z80
ADDRESS_MAP_END

/***************************************************************************


                                Z80 #2 & #3

        Dumb PCM samples players (e.g they don't even have RAM!)


***************************************************************************/

/***************************************************************************
                            Back Street Soccer
***************************************************************************/

/* Bank Switching */

static WRITE8_HANDLER( bssoccer_pcm_1_bankswitch_w )
{
	UINT8 *RAM = memory_region(REGION_CPU3);
	int bank = data & 7;
	if (bank & ~7)	logerror("CPU#2 PC %06X - ROM bank unknown bits: %02X\n", activecpu_get_pc(), data);
	memory_set_bankptr(1, &RAM[bank * 0x10000 + 0x1000]);
}

static WRITE8_HANDLER( bssoccer_pcm_2_bankswitch_w )
{
	UINT8 *RAM = memory_region(REGION_CPU4);
	int bank = data & 7;
	if (bank & ~7)	logerror("CPU#3 PC %06X - ROM bank unknown bits: %02X\n", activecpu_get_pc(), data);
	memory_set_bankptr(2, &RAM[bank * 0x10000 + 0x1000]);
}



/* Memory maps: Yes, *no* RAM */

static ADDRESS_MAP_START( bssoccer_pcm_1_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(MRA8_ROM			)	// ROM
	AM_RANGE(0x1000, 0xffff) AM_READ(MRA8_BANK1 		)	// Banked ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START( bssoccer_pcm_1_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_WRITE(MWA8_ROM			)	// ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( bssoccer_pcm_2_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(MRA8_ROM			)	// ROM
	AM_RANGE(0x1000, 0xffff) AM_READ(MRA8_BANK2 		)	// Banked ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START( bssoccer_pcm_2_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_WRITE(MWA8_ROM			)	// ROM
ADDRESS_MAP_END



/* 2 DACs per CPU - 4 bits per sample */

static WRITE8_HANDLER( bssoccer_DAC_1_w )
{
	DAC_data_w( 0 + (offset & 1), (data & 0xf) * 0x11 );
}

static WRITE8_HANDLER( bssoccer_DAC_2_w )
{
	DAC_data_w( 2 + (offset & 1), (data & 0xf) * 0x11 );
}



static ADDRESS_MAP_START( bssoccer_pcm_1_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch2_r 				)	// From The Sound Z80
ADDRESS_MAP_END
static ADDRESS_MAP_START( bssoccer_pcm_1_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_WRITE(bssoccer_DAC_1_w				)	// 2 x DAC
	AM_RANGE(0x03, 0x03) AM_WRITE(bssoccer_pcm_1_bankswitch_w	)	// Rom Bank
ADDRESS_MAP_END

static ADDRESS_MAP_START( bssoccer_pcm_2_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch3_r 				)	// From The Sound Z80
ADDRESS_MAP_END
static ADDRESS_MAP_START( bssoccer_pcm_2_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_WRITE(bssoccer_DAC_2_w				)	// 2 x DAC
	AM_RANGE(0x03, 0x03) AM_WRITE(bssoccer_pcm_2_bankswitch_w	)	// Rom Bank
ADDRESS_MAP_END



/***************************************************************************
                                Ultra Balloon
***************************************************************************/

/* Bank Switching */

static WRITE8_HANDLER( uballoon_pcm_1_bankswitch_w )
{
	UINT8 *RAM = memory_region(REGION_CPU3);
	int bank = data & 1;
	if (bank & ~1)	logerror("CPU#2 PC %06X - ROM bank unknown bits: %02X\n", activecpu_get_pc(), data);
	memory_set_bankptr(1, &RAM[bank * 0x10000 + 0x400]);
}

/* Memory maps: Yes, *no* RAM */

static ADDRESS_MAP_START( uballoon_pcm_1_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_READ(MRA8_ROM			)	// ROM
	AM_RANGE(0x0400, 0xffff) AM_READ(MRA8_BANK1 		)	// Banked ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START( uballoon_pcm_1_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_WRITE(MWA8_ROM			)	// ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( uballoon_pcm_1_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch2_r 				)	// From The Sound Z80
ADDRESS_MAP_END
static ADDRESS_MAP_START( uballoon_pcm_1_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_WRITE(bssoccer_DAC_1_w				)	// 2 x DAC
	AM_RANGE(0x03, 0x03) AM_WRITE(uballoon_pcm_1_bankswitch_w	)	// Rom Bank
ADDRESS_MAP_END


/***************************************************************************


                                Input Ports


***************************************************************************/

#define JOY(_n_) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	  ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN   ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT   ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT  ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1		  ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2		  ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3		  ) PORT_PLAYER(_n_) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START##_n_						 )


/***************************************************************************
                            Back Street Soccer
***************************************************************************/

INPUT_PORTS_START( bssoccer )

	PORT_START	// IN0 - $a00001.b - Player 1
	JOY(1)

	PORT_START	// IN1 - $a00003.b - Player 2
	JOY(2)

	PORT_START	// IN2 - $a00005.b - Player 3
	JOY(3)

	PORT_START	// IN3 - $a00007.b - Player 4
	JOY(4)

	PORT_START	// IN4 - $a00008.w - DSW x 2
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	  0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	  0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	  0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	  0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	  0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	  0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	  0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	  0x0010, DEF_STR( Easy )     )
	PORT_DIPSETTING(	  0x0018, DEF_STR( Normal )   )
	PORT_DIPSETTING(	  0x0008, DEF_STR( Hard )     )
	PORT_DIPSETTING(	  0x0000, "Hardest?"  )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	  0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0300, 0x0300, "Play Time P1" )
	PORT_DIPSETTING(	  0x0300, "1:30" )
	PORT_DIPSETTING(	  0x0200, "1:45" )
	PORT_DIPSETTING(	  0x0100, "2:00" )
	PORT_DIPSETTING(	  0x0000, "2:15" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Play Time P2" )
	PORT_DIPSETTING(	  0x0c00, "1:30" )
	PORT_DIPSETTING(	  0x0800, "1:45" )
	PORT_DIPSETTING(	  0x0400, "2:00" )
	PORT_DIPSETTING(	  0x0000, "2:15" )
	PORT_DIPNAME( 0x3000, 0x3000, "Play Time P3" )
	PORT_DIPSETTING(	  0x3000, "1:30" )
	PORT_DIPSETTING(	  0x2000, "1:45" )
	PORT_DIPSETTING(	  0x1000, "2:00" )
	PORT_DIPSETTING(	  0x0000, "2:15" )
	PORT_DIPNAME( 0xc000, 0xc000, "Play Time P4" )
	PORT_DIPSETTING(	  0xc000, "1:30" )
	PORT_DIPSETTING(	  0x8000, "1:45" )
	PORT_DIPSETTING(	  0x4000, "2:00" )
	PORT_DIPSETTING(	  0x0000, "2:15" )

	PORT_START	// IN5 - $a0000b.b - Coins
	PORT_DIPNAME( 0x0001, 0x0001, "Copyright" )         // these 4 are shown in test mode
	PORT_DIPSETTING(	  0x0001, "Distributer Unico" )
	PORT_DIPSETTING(	  0x0000, "All Rights Reserved" )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )	// used!
	PORT_DIPSETTING(	  0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_COIN1   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_COIN2   )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_COIN3   )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_COIN4   )

INPUT_PORTS_END


/***************************************************************************
                                Ultra Balloon
***************************************************************************/

INPUT_PORTS_START( uballoon )

	PORT_START	// IN0 - $600000.w - Player 1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_COIN1    )

	PORT_START	// IN1 - $600002.w - Player 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_DIPNAME( 0x3000, 0x3000, "Copyright" )	// Jumpers
	PORT_DIPSETTING(	  0x3000, "Distributer Unico" )
	PORT_DIPSETTING(	  0x2000, "All Rights Reserved" )
//  PORT_DIPSETTING(      0x1000, "Distributer Unico" )
//  PORT_DIPSETTING(      0x0000, "All Rights Reserved" )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_START	// IN2 - $600005.b - DSW 1
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	  0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	  0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	  0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	  0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	  0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	  0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	  0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Lives ) )
	PORT_DIPSETTING(	  0x0010, "2" )
	PORT_DIPSETTING(	  0x0018, "3" )
	PORT_DIPSETTING(	  0x0008, "4" )
	PORT_DIPSETTING(	  0x0000, "5" )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	  0x0040, DEF_STR( Easy )    )
	PORT_DIPSETTING(	  0x0060, DEF_STR( Normal )  )
	PORT_DIPSETTING(	  0x0020, DEF_STR( Hard )    )
	PORT_DIPSETTING(	  0x0000, DEF_STR( Hardest ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	// IN3 - $600007.b - DSW 2
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	  0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	  0x0002, DEF_STR( Upright ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	  0x001c, "200K" )
	PORT_DIPSETTING(	  0x0010, "300K, 1000K" )
	PORT_DIPSETTING(	  0x0018, "400K" )
	PORT_DIPSETTING(	  0x000c, "500K, 1500K" )
	PORT_DIPSETTING(	  0x0008, "500K, 2000K" )
	PORT_DIPSETTING(	  0x0004, "500K, 3000K" )
	PORT_DIPSETTING(	  0x0014, "600K" )
	PORT_DIPSETTING(	  0x0000, DEF_STR( None ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5*" )
	PORT_DIPSETTING(	  0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6*" )
	PORT_DIPSETTING(	  0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	  0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )

INPUT_PORTS_END

/***************************************************************************
                                Suna Quiz 6000
***************************************************************************/

INPUT_PORTS_START( sunaq )
	PORT_START	// Player 1
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN1    )


	PORT_START	// Player 2
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_START	// Single 8 switch DSW
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	  0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	  0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	  0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	  0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	  0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	  0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	  0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0018, 0x0008, DEF_STR( Difficulty ) )	/* Should this be Difficulty or Lives ?? */
	PORT_DIPSETTING(	  0x0000, DEF_STR( Easy ) )	/* 5 Hearts */
	PORT_DIPSETTING(	  0x0008, DEF_STR( Normal ) )	/* 5 Hearts */
	PORT_DIPSETTING(	  0x0010, DEF_STR( Hard ) )	/* 4 Hearts */
	PORT_DIPSETTING(	  0x0018, DEF_STR( Hardest ) )	/* 3 Hearts */
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	  0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	  0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************


                                Graphics Layouts


***************************************************************************/

/* Tiles are 8x8x4 but the minimum sprite size is 2x2 tiles */

static const gfx_layout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+4,	0,4 },
	{ 3,2,1,0, 11,10,9,8 },
	{ STEP8(0,16) },
	8*8*4/2
};

static const gfx_decode suna16_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4, 0, 16*2 }, // [0] Sprites
	{ -1 }
};




/***************************************************************************


                                Machine drivers


***************************************************************************/


/***************************************************************************
                            Back Street Soccer
***************************************************************************/

INTERRUPT_GEN( bssoccer_interrupt )
{
	switch (cpu_getiloops())
	{
		case 0: 	cpunum_set_input_line(0, 1, HOLD_LINE);	break;
		case 1: 	cpunum_set_input_line(0, 2, HOLD_LINE);	break;
	}
}

static MACHINE_DRIVER_START( bssoccer )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* ? */
	MDRV_CPU_PROGRAM_MAP(bssoccer_readmem,bssoccer_writemem)
	MDRV_CPU_VBLANK_INT(bssoccer_interrupt,2)

	MDRV_CPU_ADD(Z80, 3579545)		/* Z80B */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(bssoccer_sound_readmem,bssoccer_sound_writemem)

	MDRV_CPU_ADD(Z80, 5000000)		/* Z80B */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(bssoccer_pcm_1_readmem,bssoccer_pcm_1_writemem)
	MDRV_CPU_IO_MAP(bssoccer_pcm_1_readport,bssoccer_pcm_1_writeport)

	MDRV_CPU_ADD(Z80, 5000000)		/* Z80B */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(bssoccer_pcm_2_readmem,bssoccer_pcm_2_writemem)
	MDRV_CPU_IO_MAP(bssoccer_pcm_2_readport,bssoccer_pcm_2_writeport)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(suna16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(suna16)
	MDRV_VIDEO_UPDATE(suna16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_ROUTE(0, "left", 0.20)
	MDRV_SOUND_ROUTE(1, "right", 0.20)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.40)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.40)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.40)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.40)
MACHINE_DRIVER_END



/***************************************************************************
                                Ultra Balloon
***************************************************************************/

static MACHINE_DRIVER_START( uballoon )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_PROGRAM_MAP(uballoon_readmem,uballoon_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80, 3579545)
	/* audio CPU */	/* ? */
	MDRV_CPU_PROGRAM_MAP(uballoon_sound_readmem,uballoon_sound_writemem)

	MDRV_CPU_ADD(Z80, 5000000)
	/* audio CPU */	/* ? */
	MDRV_CPU_PROGRAM_MAP(uballoon_pcm_1_readmem,uballoon_pcm_1_writemem)
	MDRV_CPU_IO_MAP(uballoon_pcm_1_readport,uballoon_pcm_1_writeport)

	/* 2nd PCM Z80 missing */

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(suna16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(suna16)
	MDRV_VIDEO_UPDATE(suna16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.50)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.50)
MACHINE_DRIVER_END

/***************************************************************************
                                Suna Quiz 6000
***************************************************************************/

static MACHINE_DRIVER_START( sunaq )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 24000000/4)
	MDRV_CPU_PROGRAM_MAP(sunaq_readmem,sunaq_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80, 14318000/4)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sunaq_sound_readmem,sunaq_sound_writemem)


	MDRV_CPU_ADD(Z80, 24000000/4)		/* Z80B */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(bssoccer_pcm_1_readmem,bssoccer_pcm_1_writemem)
	MDRV_CPU_IO_MAP(bssoccer_pcm_1_readport,bssoccer_pcm_1_writeport)

	/* 2nd PCM Z80 missing */

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(suna16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(suna16)
	MDRV_VIDEO_UPDATE(suna16)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 14318000/4)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.50)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.50)
MACHINE_DRIVER_END


/***************************************************************************


                                ROMs Loading


***************************************************************************/


/***************************************************************************

                            [ Back Street Soccer ]

  68000-10  32MHz
            14.318MHz
  01   02                    12
  03   04                   Z80B
  6264 6264       YM2151
                  6116
                   11      13
  62256           Z80B    Z80B
  62256
  62256   05 06                  SW2
          07 08                  SW1
          09 10          6116-45
                                     6116-45
                         6116-45     6116-45

***************************************************************************/

ROM_START( bssoccer )

	ROM_REGION( 0x200000, REGION_CPU1, 0 ) 	/* 68000 Code */
	ROM_LOAD16_BYTE( "02", 0x000000, 0x080000, CRC(32871005) SHA1(b094ee3f4fc24c0521915d565f6e203d51e51f6d) )
	ROM_LOAD16_BYTE( "01", 0x000001, 0x080000, CRC(ace00db6) SHA1(6bd146f9b44c97be77578b4f0ffa28cbf66283c2) )
	ROM_LOAD16_BYTE( "04", 0x100000, 0x080000, CRC(25ee404d) SHA1(1ab7cb1b4836caa05be73ea441deed80f1e1ba81) )
	ROM_LOAD16_BYTE( "03", 0x100001, 0x080000, CRC(1a131014) SHA1(4d21264da3ee9b9912d1205999a555657ba33bd7) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) 	/* Z80 #1 - Music */
	ROM_LOAD( "11", 0x000000, 0x010000, CRC(df7ae9bc) SHA1(86660e723b0712c131dc57645b6a659d5100e962) ) // 1xxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x080000, REGION_CPU3, 0 ) 	/* Z80 #2 - PCM */
	ROM_LOAD( "13", 0x000000, 0x080000, CRC(2b273dca) SHA1(86e1bac9d1e39457c565390b9053986453db95ab) )

	ROM_REGION( 0x080000, REGION_CPU4, 0 ) 	/* Z80 #3 - PCM */
	ROM_LOAD( "12", 0x000000, 0x080000, CRC(6b73b87b) SHA1(52c7dc7da6c21eb7e0dad13deadb1faa94a87bb3) )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )	/* Sprites */
	ROM_LOAD( "05", 0x000000, 0x080000, CRC(a5245bd4) SHA1(d46a8db437e49158c020661536eb0be8a6e2e8b0) )
	ROM_LOAD( "07", 0x080000, 0x080000, CRC(fdb765c2) SHA1(f9852fd3734d10e18c91cd572ca62e66d74ccb72) )
	ROM_LOAD( "09", 0x100000, 0x080000, CRC(0e82277f) SHA1(4bdfd0ff310bf8326806a83767a6c98905debbd0) )
	ROM_LOAD( "06", 0x180000, 0x080000, CRC(d42ce84b) SHA1(3a3d07d571793ecf4c936d3af244c63b9e4b4bb9) )
	ROM_LOAD( "08", 0x200000, 0x080000, CRC(96cd2136) SHA1(1241859d6c5e64de73898763f0358171ea4aeae3) )
	ROM_LOAD( "10", 0x280000, 0x080000, CRC(1ca94d21) SHA1(23d892b840e37064a175584f955f25f990d9179d) )

ROM_END



/***************************************************************************

                            [ Ultra Ballon ]

the gameplay on this game a like bubble bobble in many ways,it uses a
68k@8MHz as the main cpu,2 z80's and a ym2151,the names of the rom files
are just my guess.

prg1.rom      27c040
prg2.rom      27c040
gfx1.rom      27c040
gfx2.rom      27c040
gfx3.rom      27c040
gfx4.rom      27c040
audio1.rom    27c512
audio2.rom    27c010

***************************************************************************/

ROM_START( uballoon )

	ROM_REGION( 0x100000, REGION_CPU1, 0 ) 	/* 68000 Code */
	ROM_LOAD16_BYTE( "prg2.rom", 0x000000, 0x080000, CRC(72ab80ea) SHA1(b755940877cf286559208106dd5e6933aeb72242) )
	ROM_LOAD16_BYTE( "prg1.rom", 0x000001, 0x080000, CRC(27a04f55) SHA1(a530294b000654db8d84efe4835b72e0dca62819) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) 	/* Z80 #1 - Music */
	ROM_LOAD( "audio1.rom", 0x000000, 0x010000, CRC(c771f2b4) SHA1(6da4c526c0ea3be5d5bb055a31bf1171a6ddb51d) )

	ROM_REGION( 0x020000, REGION_CPU3, 0 ) 	/* Z80 #2 - PCM */
	ROM_LOAD( "audio2.rom", 0x000000, 0x020000, CRC(c7f75347) SHA1(5bbbd39285c593441c6da6a12f3632d60b103216) )

	/* There's no Z80 #3 - PCM */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )	/* Sprites */
	ROM_LOAD( "gfx1.rom", 0x000000, 0x080000, CRC(fd2ec297) SHA1(885834d9b58ccfd9a32ecaa51c45e70fbbe935db) )
	ROM_LOAD( "gfx2.rom", 0x080000, 0x080000, CRC(6307aa60) SHA1(00406eba98ec368e72ee53c08b9111dec4f2552f) )
	ROM_LOAD( "gfx3.rom", 0x100000, 0x080000, CRC(718f3150) SHA1(5971f006203f86743ebc825e4ab1ed1f811e3165) )
	ROM_LOAD( "gfx4.rom", 0x180000, 0x080000, CRC(af7e057e) SHA1(67a03b54ffa1483c8ed044f27287b7f3f1150455) )

ROM_END


DRIVER_INIT( uballoon )
{
	UINT16 *RAM = (UINT16 *) memory_region(REGION_CPU1);

	/* Patch out the protection checks */
	RAM[0x0113c/2] = 0x4e71;	// bne $646
	RAM[0x0113e/2] = 0x4e71;	// ""
	RAM[0x01784/2] = 0x600c;	// beq $1792
	RAM[0x018e2/2] = 0x600c;	// beq $18f0
	RAM[0x03c54/2] = 0x600C;	// beq $3c62
	RAM[0x126a0/2] = 0x4e71;	// bne $1267a (ROM test)
}

/***************************************************************************
                                Suna Quiz 6000

  KRB-0027A mainboard

  68000 6 MHz
  Z80B x 2
  Actel A1020B
  OSC: 24.000 MHz, 14.318 MHz
  YM2151, YM3012
  RAM:
  62256 x 3
  6264 x 2
  6116 x 5
  Single 8 switch DSW

***************************************************************************/

ROM_START( sunaq )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) 	/* 68000 Code */
	ROM_LOAD16_BYTE( "prog2.bin", 0x000000, 0x080000, CRC(a92bce45) SHA1(258b2a21c27effa1d3380e4c08558542b1d05175) )
	ROM_LOAD16_BYTE( "prog1.bin", 0x000001, 0x080000, CRC(ff690e7e) SHA1(43b9c67f8d8d791be922966632613a077807b755) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) 	/* Z80 #1 - Music */
	ROM_LOAD( "audio1.bin", 0x000000, 0x010000, CRC(3df42f82) SHA1(91c1037c9d5d1ec82ed4cdfb35de5a6d626ecb3b) )

	ROM_REGION( 0x080000, REGION_CPU3, 0 ) 	/* Z80 #2 - PCM */
	ROM_LOAD( "audio2.bin", 0x000000, 0x080000, CRC(cac85ba9) SHA1(e5fbe813022c17d9eaf2a57184341666e2af365a) )

	/* There's no Z80 #3 - PCM */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )	/* Sprites */
	ROM_LOAD( "gfx1.bin", 0x000000, 0x080000, CRC(0bde5acf) SHA1(a9befb5f9a663bf48537471313f606853ea1f274) )
	ROM_LOAD( "gfx2.bin", 0x100000, 0x080000, CRC(24b74826) SHA1(cb3f665d1b1f5c9d385a3a3193866c9cae6c7002) )
ROM_END


/***************************************************************************


                                Games Drivers


***************************************************************************/

GAME( 1996, bssoccer, 0, bssoccer, bssoccer, 0,        ROT0, "SunA", "Back Street Soccer", 0 )
GAME( 1996, uballoon, 0, uballoon, uballoon, uballoon, ROT0, "SunA", "Ultra Balloon", 0 )
// Date/Version on-screen is 940620-6, but in the program rom it's 1994,6,30  K.H.T  V6.00
GAME( 1994, sunaq,    0, sunaq,    sunaq,    0,        ROT0, "SunA", "SunA Quiz 6000 Academy (940620-6)", 0 )
