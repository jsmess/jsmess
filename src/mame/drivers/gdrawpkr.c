/**********************************************************************************


    GAMING DRAW POKER (CEI)

    Driver by Roberto Fresca.
    Based on a preliminary work of Curt Coder.


    --- Technical Notes ---

    Name:    Gaming Draw Poker.
    Version: V23.9 SCH.07-0T 01/14/85
    Company: Cal Omega / Casino Electronics Inc.
    Year:    1981-1985

    Hardware:

    CPU =  1x SY6502             ; M6502 compatible.
    CRCT = 1x SY6545             ; M6845 compatible mode. Doesn't seems that are using
                                 ; the first 4 bits of register R03 (HSync width).
    SND =  1x AY8912             ; AY8910 compatible.
    I/O =  2x PIAs SY6520/6820   ; M6821 compatible.
    Clk =  1x Xtal @10MHz


***********************************************************************************


    --- General Notes ---


    Gaming Draw Poker, formerly known as Jack Potten's poker, is an evolution of the
    mentioned game. It was created by Cal Omega in 1981 for Casino Electronics Inc.
    Cal Omega was bought out by CEI (Casino Electronics Inc.), and CEI was bought by UCMC.

    This game was made before the CEI 906iii system.

    The game use the same GFX set that Jack Potten's poker for cards and have similar
    layout, but the game is different and the old discrete pitched sounds were replaced
    with a better set of sounds through a AY8910/12 implementation. The empty socket in
    the pcb is maybe for future upgrades instead of sound ROM, since sounds are hardcoded.

    Inputs are multiplexed and selected through PIA1, portB.

    In game, use "Stand" instead of "Deal/Draw" to conserve all cards without discards.
    To enter to TEST MODE press F2. Press "Hold1" + "Hold2" + "Hold3" to exit.
    To enter the STATS MODE press "Show Stats". Press "Deal/Draw" to exit.
    To pass coin and hopper errors press "Hopper SW". Also keep it pressed to see the status.
    For payout, press "Manual Collect" an then "Payout" for each credit (manual mode).


***********************************************************************************?


    --- Dumper notes (old) ---


    Program roms are roms 23*.*, on the board, there is a number near each roms
    looks to be the address of the rom :

            23-91   1800
            23-92   2000
            23-93   2800
            23-94   3000
            23-9    3800

    Graphics are in roms CG*.*, there is no type indication on these rams, i hope
    i read them correctly.

    There is also 3 sets of switches on the board :

            SW1     1       300     SW2     1       OPT1
                    2       600             2       OPT2
                    3       1200            3       OPT3
                    4       2400            4       OPT4
                    5       4800            5       OPT5
                    6       9600            6       DIS
                    7       -               7       +VPOL
                    8       -               8       +HPOL

            SW3     no indications on the board

    The sound rom is missing on the board :(


***********************************************************************************


    *** Memory Map ***


    $0000 - $07FF   NVRAM           ; All registers and settings.
    $0840 - $0840   AY-8912 (R/C)   ; Read/Control.
    $0841 - $0841   AY-8912 (W)     ; Write.
    $0880 - $0880   CRTC6845 (A)    ; MC6845 adressing.
    $0881 - $0881   CRTC6845 (R/W)  ; MC6845 Read/Write.
    $08C4 - $08C7   PIA0            ; I/O Ports 0 & 1.
    $08C8 - $08CB   PIA1            ; I/O Ports 2 & 3.

    $1000 - $13FF   VideoRAM
    $1400 - $17FF   ColorRAM

    $D800 - $FFFF   ROM



    *** MC6545 Initialization ***

    ----------------------------------------------------------------------------------------------------------------------
    register:  R00   R01   R02   R03   R04   R05   R06   R07   R08   R09   R10   R11   R12   R13   R14   R15   R16   R17
    ----------------------------------------------------------------------------------------------------------------------
    value:     0x27  0x20  0x23  0x03  0x1F  0x04  0x1F  0x1F  0x00  0x07  0x00  0x00  0x00  0x00  0x00  0x00  0x00  0x00.



***********************************************************************************


    [2007-03-24 to 2007-04-27]

    - Rearranged GFX in two different banks.
    - Decoded GFX properly.
    - Rewrote the memory map based on program ROMs analysis.
    - Hooked two SY6520/6280 (M6821) PIAs for I/O.
    - Hooked the SY6545 (6845) CRT controller.
    - Fixed size for screen total and visible area based on SY6545 CRTC registers.
    - Added partial inputs through PIAs.
    - Added proper sound through AY8910 (mapped at $0840-$0841).
    - Fixed AY8910 volume to avoid clips.
    - Proper colors through color PROM decode.
    - Demuxed inputs (thanks to Dox that pointed me in the right direction!)
    - Added some game-protection workaround.
    - Added NVRAM support.
    - Renamed driver, set and description to match the real game.
    - Added technical notes.


    TODO (in progress):

    - Fix lamps.
    - Clean up the driver.
    - Merge the driver with pmpoker.c (goldnpkr) once finished.


***********************************************************************************/


#include "driver.h"
#include "sound/ay8910.h"
#include "video/crtc6845.h"
#include "machine/6821pia.h"

static tilemap *bg_tilemap;


/*************************
*     Video Hardware     *
*************************/

WRITE8_HANDLER( gdrawpkr_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

WRITE8_HANDLER( gdrawpkr_colorram_w )
{
	colorram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

static TILE_GET_INFO( get_bg_tile_info )
{
/*  - bits -
    7654 3210
    --xx xx--   tiles color.
    ---- --x-   tiles bank.
    xx-- ---x   seems unused. */

	int attr = colorram[tile_index];
	int code = videoram[tile_index];
	int bank = (attr & 0x02) >> 1;	/* bit 1 switch the gfx banks */
	int color = (attr & 0x3c);	/* bits 2-3-4-5 for color */

	if (attr == 0x3a)	/* Is the palette wrong? */
		color = 0x3b;	/* 0x3b is the best match */

//  if (attr == 0x36)   /* Is the palette wrong? */
//      color = 0x3e;   /* 0x3e is the best match */

//  if (attr == 0x32)   /* Is the palette wrong? */
//      color = 0x3a;   /* 0x3a is the best match */

	SET_TILE_INFO(bank, code, color, 0)
}

VIDEO_START( gdrawpkr )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 31);
}

VIDEO_UPDATE( gdrawpkr )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}

PALETTE_INIT( gdrawpkr )
{
/*  prom bits
    7654 3210
    ---- ---x   red component.
    ---- --x-   green component.
    ---- -x--   blue component.
    ---- x---   unknown, aparently unused.
    xxxx ----   unused.
*/
	int i;

	/* 00000BGR */
	if (color_prom == 0) return;

	for (i = 0;i < machine->drv->total_colors;i++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		r = bit0 * 0xff;

		/* green component */
		bit1 = (color_prom[i] >> 1) & 0x01;
		g = bit1 * 0xff;

		/* blue component */
		bit2 = (color_prom[i] >> 2) & 0x01;
		b = bit2 * 0xff;


		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}
}


/*************************
* Memory map information *
*************************/

static ADDRESS_MAP_START( gdrawpkr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x0840, 0x0840) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x0841, 0x0841) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x0880, 0x0880) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x0881, 0x0881) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)
	AM_RANGE(0x08c4, 0x08c7) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x08c8, 0x08cb) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_WRITE(gdrawpkr_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x1400, 0x17ff) AM_RAM AM_WRITE(gdrawpkr_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xd800, 0xffff) AM_ROM
ADDRESS_MAP_END


/*************************
*      Input ports       *
*************************/

INPUT_PORTS_START( gdrawpkr )
	PORT_START_TAG("IN0-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Credits") PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Manual Collect") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Double Up") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Deal/Draw") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Cancel Holds") PORT_CODE(KEYCODE_N)
//  PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Stand") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Unknown R") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Unknown T") PORT_CODE(KEYCODE_T)

	PORT_START_TAG("IN0-1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Payout") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Unknown W") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Service") PORT_CODE(KEYCODE_8)
//  PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 3") PORT_CODE(KEYCODE_F)
//  PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 4") PORT_CODE(KEYCODE_G)
//  PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 5") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Test Mode") PORT_CODE(KEYCODE_F2)	/* should be splitted? */
	PORT_BIT( 0x14, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Hopper SW") PORT_CODE(KEYCODE_9)	/* should be splitted? */
	PORT_BIT( 0x24, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Show Stats") PORT_CODE(KEYCODE_0)	/* should be splitted? */
//  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Unknown Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Unknown U") PORT_CODE(KEYCODE_U)

	PORT_START_TAG("IN0-2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Small") PORT_CODE(KEYCODE_S)
//  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Unknown I") PORT_CODE(KEYCODE_I)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Unknown O") PORT_CODE(KEYCODE_O)

	PORT_START_TAG("IN0-3")
//  PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit D") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bet") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Take") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Big") PORT_CODE(KEYCODE_A)
//  PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Unknown F") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Unknown G") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Unknown H") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Unknown J") PORT_CODE(KEYCODE_J)

	PORT_START_TAG("SW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xC0, 0x40, "Maximum Bet")
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x40, "10" )
	PORT_DIPSETTING(    0x80, "40" )
	PORT_DIPSETTING(    0xC0, "80" )

	PORT_START_TAG("SW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("SW3")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


/*************************
*    Graphics Layouts    *
*************************/

static const gfx_layout charlayout =
{
	8, 8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout tilelayout =
{
	8, 8,
	RGN_FRAC(1,3),
	3,
	{ 0, RGN_FRAC(1,3), RGN_FRAC(2,3) },    /* bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


/******************************
* Graphics Decode Information *
******************************/

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16 },
	{ REGION_GFX2, 0, &tilelayout, (8 * 3) + 128, 16 },
	{ -1 }
};


/***********************
*    Write Handlers    *
***********************/

static int mux_data = 0;

static READ8_HANDLER( mux_port_r )
{
	switch( mux_data&0xf0 )	/* bits 4-7 */
	{
		case 0x10: return input_port_0_r(0);
		case 0x20: return input_port_1_r(0);
		case 0x40: return input_port_2_r(0);
		case 0x80: return input_port_3_r(0);
	}
	return 0xff;
}

static WRITE8_HANDLER( mux_w )
{
	mux_data = data ^ 0xff;	/* inverted */
}


/****  Lamps debug  ****

    PIA0-B  PIA1-A

    0xff    0x7f    = OFF
    0xfd    0x7f    = Deal
    0xf1    0x7b    = 12345 Holds
    0xef    0x7f    = Double Up & Take
    0xff    0x7e    = Big & Small

    0xfe    0x7f    = Hold 1
    0xfd    0x7f    = Hold 2
    0xfb    0x7f    = Hold 3
    0xf7    0x7f    = Hold 4
    0xef    0x7f    = Hold 5
    0xff    0x7e    = Cancel
    0xff    0x7d    = Bet
    0xff    0x7b    = Take

*/
static WRITE8_HANDLER( lamps_a_w )
{
	output_set_lamp_value(0, 1-((data) & 1));		// 0
	output_set_lamp_value(1, 1-((data >> 1) & 1));	// 1
	output_set_lamp_value(2, 1-((data >> 2) & 1));	// 2
	output_set_lamp_value(3, 1-((data >> 3) & 1));	// 3
	output_set_lamp_value(4, 1-((data >> 4) & 1));	// 4
	output_set_lamp_value(5, 1-((data >> 5) & 1));	// 5
	output_set_lamp_value(6, 1-((data >> 6) & 1));	// 6
	output_set_lamp_value(7, 1-((data >> 7) & 1));	// 7
}

static WRITE8_HANDLER( lamps_b_w )
{
	output_set_lamp_value(8, 1-((data) & 1));		// 0
	output_set_lamp_value(9, 1-((data >> 1) & 1));	// 1
	output_set_lamp_value(10, 1-((data >> 2) & 1));	// 2
	output_set_lamp_value(11, 1-((data >> 3) & 1));	// 3
	output_set_lamp_value(12, 1-((data >> 4) & 1));	// 4
	output_set_lamp_value(13, 1-((data >> 5) & 1));	// 5
	output_set_lamp_value(14, 1-((data >> 6) & 1));	// 6
	output_set_lamp_value(15, 1-((data >> 7) & 1));	// 7
}


/***********************
*    PIA Interfaces    *
***********************/

static const pia6821_interface pia0_intf =
{
	/* PIA inputs: A, B, CA1, CB1, CA2, CB2 */
	mux_port_r, 0, 0, 0, 0, 0,

	/* PIA outputs: A, B, CA2, CB2 */
	0, lamps_a_w, 0, 0,

	/* PIA IRQs: A, B */
	0, 0
};

static const pia6821_interface pia1_intf =
{
	/* PIA inputs: A, B, CA1, CB1, CA2, CB2 */
	input_port_4_r, input_port_5_r, input_port_6_r, 0, 0, 0,	/* 5 & 6 not tested */

	/* PIA outputs: A, B, CA2, CB2 */

	lamps_b_w, mux_w, 0, 0,

	/* PIA IRQs: A, B */
	0, 0
};


/*************************
*     Machine Driver     *
*************************/

static MACHINE_DRIVER_START( gdrawpkr )
	// basic machine hardware
	MDRV_CPU_ADD(M6502, 10000000/12)	/* guessing... */
	MDRV_CPU_PROGRAM_MAP(gdrawpkr_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((39+1)*8, (31+1)*8)                  /* Taken from MC6845 init, registers 00 & 04. Normally programmed with (value-1) */
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 31*8-1)    /* Taken from MC6845 init, registers 01 & 06 */

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(1024)

	MDRV_PALETTE_INIT(gdrawpkr)
	MDRV_VIDEO_START(gdrawpkr)
	MDRV_VIDEO_UPDATE(gdrawpkr)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 10000000/12)	/* guessing again... */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

MACHINE_DRIVER_END


/*************************
*        Rom Load        *
*************************/

ROM_START( gdrawpkr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "23-91.u5",	 0xd800, 0x0800, CRC(b49035e2) SHA1(b94a0245ca64d15b1496d1b272ffc0ce80f85526) )
	ROM_LOAD( "23-92.u6",	 0xe000, 0x0800, CRC(d9ffaa73) SHA1(e39d10121e16f89cd8d30a5391a14dc3d4b13a46) )
	ROM_LOAD( "23-93.u7",	 0xe800, 0x0800, CRC(f4e44280) SHA1(a03e5f03ed86c8ad7900fab0ef6a71c76eba3232) )
	ROM_LOAD( "23-94.u8",	 0xf000, 0x0800, CRC(8372f4d0) SHA1(de289b65cbe30c92b46fa87b9262ff7f9cfa0431) )
	ROM_LOAD( "23-9.u9",	 0xf800, 0x0800, CRC(bfcb934d) SHA1(b7cfa049bdd773368cb8326bcdfabbf474d15bb4) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cg-0.u67",	 0x0000, 0x0800, CRC(b626ad89) SHA1(551b75f4559d11a4f8f56e38982114a21c77d4e7) )

	ROM_REGION( 0x1800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cg-2c.u70",	 0x0000, 0x0800, CRC(f2f94661) SHA1(f37f7c0dff680fd02897dae64e13e297d0fdb3e7) )
	ROM_LOAD( "cg-2b.u69",	 0x0800, 0x0800, CRC(6bbb1e2d) SHA1(51ee282219bf84218886ad11a24bc6a8e7337527) )
	ROM_LOAD( "cg-2a.u68",	 0x1000, 0x0800, CRC(6e3e9b1d) SHA1(14eb8d14ce16719a6ad7d13db01e47c8f05955f0) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "82s129n.u28", 0x0000, 0x0100, CRC(6db5a344) SHA1(5f1a81ac02a2a74252decd3bb95a5436cc943930) )
ROM_END


/*************************
*      Driver Init       *
*************************/

static DRIVER_INIT( gdrawpkr )
{
	/* Palette transformed by PLDs? */
	int x;
	UINT8 *srcp = memory_region( REGION_PROMS );

	for (x=0x0000;x<0x0100;x++)
	{
		if (srcp[x] == 0x07)
			srcp[x] = 0x04;	/* blue background */
//          srcp[x] = 0x00; /* black background */
	}

	/* Initializing PIAs... */
	pia_config(0, &pia0_intf);
	pia_config(1, &pia1_intf);
}


/*************************
*      Game Drivers      *
*************************/

/*    YEAR  NAME      PARENT  MACHINE   INPUT     INIT      ROT    COMPANY                                FULLNAME            FLAGS   */
GAME( 1981, gdrawpkr, 0,      gdrawpkr, gdrawpkr, gdrawpkr, ROT0, "Cal Omega / Casino Electronics Inc.", "Gaming Draw Poker", 0 )
