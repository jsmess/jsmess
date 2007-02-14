/******************************************************************************

    PLAYMAN POKER
    -------------

    Driver by Roberto Fresca.


    Games running on this hardware:

    * PlayMan Poker (Germany).            1981, PlayMan?
    * Golden Poker Double Up (Big Boy).   1981, Bonanza Enterprises, Ltd.
    * Golden Poker Double Up (Mini Boy).  1981, Bonanza Enterprises, Ltd.
    * Joker Poker.                        1981, Coinmaster-Gaming, Ltd.
    * Jack Potten's Poker.                1981, Unknown.


*******************************************************************************


    I think "Diamond Poker Double Up" from Bonanza Enterprises should run on this hardware too.
    http://www.arcadeflyers.com/?page=thumbs&id=4539

    Big-Boy and Mini-Boy are different sized cabinets for Bonanza Enterprises games.
    http://www.arcadeflyers.com/?page=thumbs&id=4616
    http://www.arcadeflyers.com/?page=thumbs&id=4274


    Preliminary Notes (pmpoker):

    - This set was found as "unknown playman-poker".
    - The ROMs didn't match any currently supported set (0.108u2 romident switch).
    - All this driver was made using reverse engineering in the program roms.


    Game Notes (goldnpkr):

    - Key 9 for Meters/Stats. To reset meters push CANCEL + SMALL buttons. HOLD5 to exit.
    This is a timed function and after 30-40 seconds switch back to the game.
    In goldnpkr you can switch between Permanent/Interim Meters.
    In pmpoker you can see only Permanent Meters.

    - Service (F2) for settings.
    This is a timed function and after 30-40 seconds switch back to the game.
    Press DOUBLE UP to change Double Up 7 settings between 'Even' and 'Lose'.
    Press BET to adjust the maximum bet (20-200).
    Press HOLD4 for Meter Over (5000-50000).
    Press BIG to change Double Up settings (Normal-Hard).
    Press TAKE SCORE to set Half Gamble (Yes/No).
    Press SMALL to set win sound (Yes/No).
    Press HOLD1 to set coinage 1.
    Press HOLD2 to set coinage 2.
    Press HOLD3 to set coinage 3.
    Press HOLD5 to exit.


    Buttons/Inputs:     goldnpkr    goldnpkr    pmpoker     jokerpkr    pottnpkr

    - HOLD (5 buttons)  mapped      mapped      mapped      mapped      mapped
    - CANCEL            mapped      mapped      mapped      mapped      mapped
    - BIG               mapped      mapped      mapped      ---         ---
    - SMALL             mapped      mapped      mapped      ---         ---
    - DOUBLE UP         mapped      mapped      mapped      ---         ---
    - TAKE SCORE        mapped      mapped      mapped      mapped      mapped
    - DEAL/DRAW         mapped      mapped      mapped      mapped      mapped
    - BET               mapped      mapped      mapped      mapped      mapped

    - Coin 1 (coins)    not_mapped  not_mapped  mapped      mapped      mapped
    - Coin 2 (notes)    coin1       coin1       coin1       coin1       coin1
    - Coin 3 (coupons)  coin2       coin2       n/a         n/a         n/a
    - Payout            mapped      mapped      mapped      mapped      mapped

    - SETTINGS          mapped      mapped      mapped      mapped      mapped
    - METERS            mapped      mapped      mapped      mapped      mapped

    Inputs are different for some games. In goldnpkr, each button has only one function.
    In pmpoker, some buttons have different functions.


    "How to play"... (from "Golden Poker Double Up" instruction card):

    1st GAME
    - Insert coin / bank note.
    - Push BET button, 1-10 credits multiple play.
    - Push DEAL/DRAW button.
    - Push HOLD buttons to hold cards you need.
    - Cards held can be cancelled by pushing CANCEL button.
    - Push DEAL/DRAW button to draw cards.

    2nd GAME - Double Up game
    - When you win, choose TAKE SCORE or DOUBLE UP game.
    - Bet winnings on
        "BIG (8 or more number)" or
        "SMALL (6 and less number)" of next one card dealt.
    - Over 5,000 winnings will be storaged automatically.



    Hardware Notes (pmpoker):

    - CPU:      1x M6502.
    - Video:    1x MC6845.
    - RAM:      4x uPD2114LC
    - I/O       2x 6821 PIAs.
    - prg ROMs: 3x 2732 (32Kb) or similar.
    - gfx ROMs: 4x 2716 (16Kb) or similar.
    - sound:    DAC??
    - battery backup: 2x S8423


    PCB Layout (pmpoker): (XX) = unreadable.
     _______________________________________________________________________________
    |   _________                                                                   |
    |  |         |               -- DIP SW x8 --                                    |
    |  | Battery |   _________   _______________   _________  _________   ________  |
    |  |   055   |  | 74LS32  | |1|2|3|4|5|6|7|8| | HCF4011 || HCF4096 | | LM339N | |
    |  |_________|  |_________| |_|_|_|_|_|_|_|_| |_________||_________| |________| |
    |       _________     _________   _________   _________                         |
    |      | 74LS138 |   | S-8423  | | 74LS08N | | 74LS(XX)|                        |
    |      |_________|   |_________| |_________| |_________|                        |
    |  _______________    _________   ____________________                      ____|
    | |               |  | S-8423  | |                    |                    |
    | |     2732      |  |_________| |       6502P        |                    |
    | |_______________|   _________  |____________________|                    |
    |  _______________   |  7432   |  ____________________                     |____
    | |               |  |_________| |                    |                     ____|
    | |     2732      |   _________  |       6821P        |                     ____|
    | |_______________|  | 74LS157 | |____________________|                     ____|
    |  _______________   |_________|  ____________________                      ____|
    | |               |   _________  |                    |                     ____|
    | |     2732      |  | 74LS157 | |       6821P        |                     ____|
    | |_______________|  |_________| |____________________|                     ____|
    |  _______________    _________   ____________________                      ____|
    | |               |  | 74LS157 | |                    |                     ____|
    | |     2732      |  |_________| |       6845SP       |                     ____|
    | |_______________|   _________  |____________________|                     ____|
    |                    | 2114-LC |                                            ____| 28x2
    |                    |_________|                                            ____| connector
    |       _________     _________                                             ____|
    |      | 74LS245 |   | 2114-LC |                                            ____|
    |      |_________|   |_________|                                            ____|
    |       _________     _________               _________                     ____|
    |      | 74LS245 |   | 2114-LC |             | 74LS174 |                    ____|
    |      |_________|   |_________|             |_________|                    ____|
    |  ________________   _________   _________   _________                     ____|
    | |                | | 2114-LC | | 74LS08H | | TI (XX) | <-- socketed.      ____|
    | |      2716      | |_________| |_________| |_________|       PROM?        ____|
    | |________________|              _________   _________                     ____|
    |  ________________              | 74LS04P | | 74LS174 |                    ____|
    | |                |             |_________| |_________|                    ____|
    | |      2716      |              _________   _________                     ____|
    | |________________|             | 74166P  | | 74LS86C |                    ____|
    |  ________________              |_________| |_________|                    ____|
    | |                |              _________    _______                     |
    | |      2716      |             | 74166P  |  | 555TC |                    |
    | |________________|             |_________|  |_______|                    |
    |  ________________                                                        |____
    | |                |                                                        ____|
    | |      2716      |              _________   _________      ________       ____| 5x2
    | |________________|             | 74166P  | |  7407N  |    | LM380N |      ____| connector
    |                                |_________| |_________|    |________|      ____|
    |  ________  ______               _________   _________      ___            ____|
    | | 74LS04 || osc. |             | 74LS193 | |  7407N  |    /   \          |
    | |________||10 MHz|             |_________| |_________|   | POT |         |
    |           |______|                                        \___/          |
    |__________________________________________________________________________|



    Some odds:

    - There are pieces of code like the following sub:

    78DE: 18         clc
    78DF: 69 07      adc  #$07
    78E1: 9D 20 10   sta  $1020,x
    78E4: A9 00      lda  #$00
    78E6: 9D 20 18   sta  $1820,x
    78E9: E8         inx

    78EA: 82         DOP        ; use of DOP (double NOP)
    78EB: A2 0A      dummy (ldx #$0A)

    78ED: AD 82 08   lda  $0882

    78F0: 82         DOP        ; use of DOP (double NOP)
    78F1: 48 08      dummy
    78F3: D0 F6      bne  $78EB ; branch to the 1st DOP dummy arguments (now ldx #$0A).
    78F5: CA         dex
    78F6: D0 F8      bne  $78F0
    78F8: 29 10      and  #$10
    78FA: 60         rts

    Offset $78EA and $78F0 contains an undocumented 6502 opcode (0x82).

    At beginning, I thought that the main processor was a 65sc816, since 0x82 is a documented opcode (BRL) for this CPU.
    Even the vector $FFF8 contain 0x09 (used to indicate "Emulation Mode" for the 65sc816).
    I dropped the idea following the code. Impossible to use BRL (branch relative long) in this structure.

    Some 6502 sources list the 0x82 mnemonic as DOP (double NOP), with 2 dummy bytes as argument.
    The above routine dynamically change the X register value using the DOP undocumented opcode.
    Now all have sense.


    --------------------
    ***  Memory Map  ***
    --------------------

    $0000 - $00FF   RAM     ; zero page (pointers and registers)

                            ; $45 - $47 Coin settings.
                            ; $50 - Input Port.
                            ; $5C - Input Port.
                            ; $5D - Input Port.
                            ; $5E - Input Port.
                            ; $5F - Input Port.

    $0100 - $01FF   RAM     ; 6502 Stack Pointer.

    $0200 - $02FF   RAM     ; R/W. (settings)
    $0300 - $03FF   RAM     ; R/W (mainly to $0383). still not totally understood. $0340 - $035f (settings).

    $0800 - $0801   MC6845  ; mc6845 use $0800 for register addressing and $0801 for register values.

                            *** pmpoker mc6845 init at $65B9 ***
                            register:   00    01    02    03    04    05    06    07    08    09    10    11    12    13    14    15    16    17
                            value:     0x27  0x20  0x22  0x02  0x1F  0x04  0x1D  0x1E  0x00  0x07  0x00  0x00  0x00  0x00  #$00  #$00  #$00  #$00.

                            *** goldnpkr mc6845 init at $5E75 ***
                            register:   00    01    02    03    04    05    06    07    08    09    10    11    12    13    14    15    16    17
                            value:     0x27  0x20  0x23  0x03  0x1F  0x04  0x1D  0x1F  0x00  0x07  0x00  0x00  0x00  0x00  #$00  #$00  #$00  #$00.

    $0844 - $0847   PIA1    ; writes: 0xFF 0x04 0xFF 0x04.  initialized at $5000.
    $0848 - $084b   PIA2    ; writes: 0xFF 0x04 0xFF 0x04.  initialized at $5000.

    $1000 - $13FF   Video RAM   ; initialized in subroutine starting at $5042.
    $1800 - $1BFF   Color RAM   ; initialized in subroutine starting at $5042.

    $5000 - $7FFF   ROM
    $F000 - $FFFF   ROM     ; mirrored from $7000 - $7FFF for vectors/pointers purposes.


    -------------------------------------------------------------------------


    DRIVER UPDATES:


    [2006-09-02]

    - Initial release.


    [2006-09-06]

    - Understood the GFX banks:
        - 1 bank (1bpp) for text layer and minor graphics.
        - 1 bank (3bpp) for the undumped cards deck graphics.

    - Partially added inputs through 6821 PIAs.
        ("Bitte techniker rufen" error messages. Press 'W' to reset the machine)

    - Confirmed the CPU as 6502. (was in doubt due to use of illegal opcodes)


    [2006-09-15]

    - Confirmed the GFX banks (a complete dump appeared!).
    - Improved technical notes and added a PCB layout based on PCB picture.
    - Found and fixed the 3rd bitplane of BigBoy gfx.
    - Renamed Big-Boy to Golden Poker Double Up. (Big-Boy and Mini-Boy are names of cabinet models).
    - Added 'Joker Poker' (Golden Poker version without the 'double-up' feature).
    - Added 'Jack Potten's Poker' (same as Joker Poker, but with 'Aces or better' instead of jacks).
    - Simulated colors for all sets till color PROMs appear.
    - Fixed bit corruption in goldnpkr rom u40_4a.bin.
    - Completed inputs in all sets (except DIP switches).
    - Removed flags GAME_WRONG_COLORS and GAME_IMPERFECT_GRAPHICS in all sets.
    - Removed flag GAME_NOT_WORKING. All sets are now playable. :)


    [2006-10-09]

    - Added service/settings mode to pmpoker.
    - Added PORT_IMPULSE to manage correct timings for most inputs in all games.
      (jokerpkr still trigger more than 1 credit for coin pulse).


    [2007-02-01]

    - Crystal documented via #define.
    - CPU clock derived from #defined crystal value.
    - Replaced simulated colors with proper color prom decode.
    - Renamed "Golden Poker Double Up" to "Golden Poker Double Up (Big Boy)".
    - Added set Golden Poker Double Up (Mini Boy).
    - Cleaned up the driver a bit.
    - Updated technical notes.


    TODO:

    - Check and complete connections for both PIAs.
    - Battery backed RAM.
    - Figure out the sound.
    - Cleanup and split the driver.


*******************************************************************************/


#define MASTER_CLOCK	10000000	/* 10MHz */

#include "driver.h"
#include "video/generic.h"
#include "video/crtc6845.h"
#include "machine/6821pia.h"


/*************************
*     Video Hardware     *
*************************/

static tilemap *bg_tilemap;

WRITE8_HANDLER( pmpoker_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( pmpoker_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
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

	SET_TILE_INFO(bank, code, color, 0)
}

VIDEO_START( pmpoker )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 29);

	return 0;
}

VIDEO_UPDATE( pmpoker )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}

PALETTE_INIT( pottnpkr )
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

	for (i = 0;i < Machine->drv->total_colors;i++)
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


		palette_set_color(machine, i, r, g, b);
	}
}


/*************************
* Memory map information *
*************************/

static ADDRESS_MAP_START( pmpoker_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x004f) AM_RAM    /* AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size) */

	AM_RANGE(0x0050, 0x0050) AM_READ(input_port_1_r)    /* while understand how to connect to PIAs */
	AM_RANGE(0x005c, 0x005c) AM_READ(input_port_2_r)    /* while understand how to connect to PIAs */
	AM_RANGE(0x005d, 0x005d) AM_READ(input_port_3_r)    /* while understand how to connect to PIAs */
	AM_RANGE(0x005e, 0x005e) AM_READ(input_port_4_r)    /* while understand how to connect to PIAs */
	AM_RANGE(0x005f, 0x005f) AM_READ(input_port_5_r)    /* while understand how to connect to PIAs */

	AM_RANGE(0x0060, 0x07ff) AM_RAM    /* should be battery backed ram too */

	AM_RANGE(0x0800, 0x0800) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x0801, 0x0801) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)

	AM_RANGE(0x0844, 0x0847) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0848, 0x084b) AM_READWRITE(pia_1_r, pia_1_w)

	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_WRITE(pmpoker_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x1800, 0x1bff) AM_RAM AM_WRITE(pmpoker_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4000, 0x7fff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( jokerpkr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x004f) AM_RAM    /* AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size) */

	AM_RANGE(0x0050, 0x0050) AM_READ(input_port_1_r)    /* while understand how to connect to PIAs */
	AM_RANGE(0x005c, 0x005c) AM_READ(input_port_2_r)    /* while understand how to connect to PIAs */
	AM_RANGE(0x005d, 0x005d) AM_READ(input_port_3_r)    /* while understand how to connect to PIAs */
	AM_RANGE(0x005e, 0x005e) AM_READ(input_port_4_r)    /* while understand how to connect to PIAs */
	AM_RANGE(0x005f, 0x005f) AM_READ(input_port_5_r)    /* while understand how to connect to PIAs */

	AM_RANGE(0x0060, 0x07ff) AM_RAM    /* should be battery backed ram too */

	AM_RANGE(0x0800, 0x0800) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x0801, 0x0801) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)

	AM_RANGE(0x0844, 0x0847) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0848, 0x084b) AM_READWRITE(pia_1_r, pia_1_w)

	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_WRITE(pmpoker_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x1800, 0x1bff) AM_RAM AM_WRITE(pmpoker_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x2000, 0x3fff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END


/*************************
*      Input ports       *
*************************/

INPUT_PORTS_START( pmpoker )

	PORT_START_TAG("IN0")    /* PIA1 - timing issues */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")    /* 0x50 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )    /* should be coin1 (coin) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )    /* should be coin1 (coin) too */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN2")    /* 0x5c */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE1 ) PORT_NAME("Meters/Stats")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("Deal/Draw") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN3")    /* 0x5d */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_NAME("Payout") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,	IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN4")    /* 0x5e */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_IMPULSE(3) PORT_NAME("Hold1 / Take Score (Kasse)") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_IMPULSE(3) PORT_NAME("Hold2 / Small (Tief)") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_IMPULSE(3) PORT_NAME("Hold3 / Bet (Setze)") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_IMPULSE(3) PORT_NAME("Hold4 / Big (Hoch)") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,	IPT_BUTTON5 ) PORT_IMPULSE(3) PORT_NAME("Hold5 / Double Up (Dopp.)") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN5")    /* 0x5f */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE )    /* service/settings mode */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )    /* "notes in" */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(2)    /* notes */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")    /* still not connected */
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

INPUT_PORTS_START( goldnpkr )

	PORT_START_TAG("IN0")    /* PIA1 - timing issues */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")    /* 0x50 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )    /* PORT_NAME("Coins In"); need to check more closely */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN2")    /* 0x5c */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE1 ) PORT_IMPULSE(1) PORT_NAME("Meters/Stats")    /* GP: Permanent/Interim Meters (timed) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 ) PORT_IMPULSE(1) PORT_NAME("Double Up") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON8 ) PORT_IMPULSE(1) PORT_NAME("Deal/Draw") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,	IPT_BUTTON6 ) PORT_IMPULSE(1) PORT_NAME("Cancel") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN3")    /* 0x5d */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE2 ) PORT_IMPULSE(1) PORT_NAME("Payout") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 ) PORT_IMPULSE(1) PORT_NAME("Take Score") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_IMPULSE(1) PORT_NAME("Big") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,	IPT_BUTTON10 ) PORT_IMPULSE(1) PORT_NAME("Small") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN4")    /* 0x5e */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_IMPULSE(1) PORT_NAME("Hold 1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_IMPULSE(1) PORT_NAME("Hold 2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_IMPULSE(1) PORT_NAME("Hold 3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_IMPULSE(1) PORT_NAME("Hold 4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,	IPT_BUTTON5 ) PORT_IMPULSE(1) PORT_NAME("Hold 5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN5")    /* 0x5f */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE )    /* service/settings mode */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)    /* PORT_NAME("Notes In") */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_IMPULSE(1) PORT_NAME("Bet") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)    /* PORT_NAME("Coupons In") */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")    /* still not connected */
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

INPUT_PORTS_START( jokerpkr )

	PORT_START_TAG("IN0")    /* PIA1 - timing issues */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")    /* 0x50 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )    /* PORT_NAME("Coins In"); need to check more closely */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN2")    /* 0x5c */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE1 ) PORT_IMPULSE(1) PORT_NAME("Meters/Stats")    /* GP: Permanent/Interim Meters (timed) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )    /* Double Up button (unused) */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON8 ) PORT_IMPULSE(1) PORT_NAME("Deal/Draw") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,	IPT_BUTTON6 ) PORT_IMPULSE(1) PORT_NAME("Cancel") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN3")    /* 0x5d */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE2 ) PORT_IMPULSE(1) PORT_NAME("Payout") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 ) PORT_IMPULSE(1) PORT_NAME("Take Score") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )    /* Big button (unused) */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,	IPT_UNKNOWN )    /* Small button (unused) */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN4")    /* 0x5e */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_IMPULSE(1) PORT_NAME("Hold 1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_IMPULSE(1) PORT_NAME("Hold 2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_IMPULSE(1) PORT_NAME("Hold 3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_IMPULSE(1) PORT_NAME("Hold 4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,	IPT_BUTTON5 ) PORT_IMPULSE(1) PORT_NAME("Hold 5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN5")    /* 0x5f */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE )    /* service/settings mode */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )	PORT_IMPULSE(1)    /* PORT_NAME("Notes In") */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_IMPULSE(1) PORT_NAME("Bet") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)    /* PORT_NAME("Coupons In") */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")    /* still not connected */
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
*    PIA Interfaces    *
**********************/

 static const pia6821_interface pia0_intf =
{
	/* PIA inputs: A, B, CA1, CB1, CA2, CB2 */
	input_port_0_r, 0, 0, 0, 0, 0,

	/* PIA outputs: A, B, CA2, CB2 */
	0, 0, 0, 0,

	/* PIA IRQs: A, B */
	0, 0
};

static const pia6821_interface pia1_intf =
{
	/* PIA inputs: A, B, CA1, CB1, CA2, CB2 */
	0, 0, 0, 0, 0, 0,

	/* PIA outputs: A, B, CA2, CB2 */
	0, 0, 0, 0,

	/* PIA IRQs: A, B */
	0, 0
};


/*************************
*     Machine Driver     *
*************************/

static MACHINE_DRIVER_START( pmpoker )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, MASTER_CLOCK/16)	// guessing...
	MDRV_CPU_PROGRAM_MAP(pmpoker_map, 0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

//  MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((39+1)*8, (31+1)*8)                  /* Taken from MC6845 init, registers 00 & 04. Normally programmed with (value-1) */
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 29*8-1)    /* Taken from MC6845 init, registers 01 & 06 */

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(pottnpkr)
	MDRV_COLORTABLE_LENGTH(1024)

	MDRV_VIDEO_START(pmpoker)
	MDRV_VIDEO_UPDATE(pmpoker)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jokerpkr )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pmpoker)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(jokerpkr_map, 0)

MACHINE_DRIVER_END


/*************************
*        Rom Load        *
*************************/

ROM_START( pmpoker )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "2-5.bin",	0x5000, 0x1000, CRC(3446a643) SHA1(e67854e3322e238c17fed4e05282922028b5b5ea) )
	ROM_LOAD( "2-6.bin",	0x6000, 0x1000, CRC(50d2d026) SHA1(7f58ab176de0f0f7666d87271af69a845faec090) )
	ROM_LOAD( "2-7.bin",	0x7000, 0x1000, CRC(a9ab972e) SHA1(477441b7ff3acae3a5d5a3e4c2a428e0b3121534) )
	ROM_RELOAD(				0xf000, 0x1000 )    /* for vectors/pointers */

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1-4.bin",	0x0000, 0x0800, CRC(62b9f90d) SHA1(39c61a01225027572fdb75543bb6a78ed74bb2fb) )	// text layer

	ROM_REGION( 0x1800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1-1.bin",	0x0000, 0x0800, CRC(f2f94661) SHA1(f37f7c0dff680fd02897dae64e13e297d0fdb3e7) )	// cards deck gfx, bitplane1.
	ROM_LOAD( "1-2.bin",	0x0800, 0x0800, CRC(6bbb1e2d) SHA1(51ee282219bf84218886ad11a24bc6a8e7337527) )	// cards deck gfx, bitplane2.
	ROM_LOAD( "1-3.bin",	0x1000, 0x0800, CRC(6e3e9b1d) SHA1(14eb8d14ce16719a6ad7d13db01e47c8f05955f0) )	// cards deck gfx, bitplane3.

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) )
ROM_END

/*  the original goldnpkr u40_4a.bin rom is bit corrupted.
    U43_2A.bin        BADADDR      --xxxxxxxxxxx
    U38_5A.bin        1ST AND 2ND HALF IDENTICAL
    UPS39_12A.bin     0xxxxxxxxxxxxxx = 0xFF
*/
ROM_START( goldnpkr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ups39_12a.bin",	0x0000, 0x8000, CRC(216b45fb) SHA1(fbfcd98cc39b2e791cceb845b166ff697f584add) )
	ROM_RELOAD(					0x8000, 0x8000 )    /* for vectors/pointers */

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u38_5a.bin",	0x0000, 0x2000, CRC(32705e1d) SHA1(84f9305af38179985e0224ae2ea54c01dfef6e12) )    /* text layer */

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "u43_2a.bin",	0x0000, 0x2000, CRC(10b34856) SHA1(52e4cc81b36b4c807b1d4471c0f7bea66108d3fd) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "u40_4a.bin",	0x2000, 0x2000, CRC(5fc965ef) SHA1(d9ecd7e9b4915750400e76ca604bec8152df1fe4) )    /* cards deck gfx, bitplane2 */
	ROM_COPY( REGION_GFX1,	0x0800, 0x4000, 0x0800 )    /* cards deck gfx, bitplane3. found in the 2nd quarter of the text layer rom */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) )

/*  pmpoker                 goldnpkr
    1-4.bin                 u38_5a (1st quarter)    96.582031%  \ 1st and 2nd halves are identical.
    1-3.bin                 u38_5a (2nd quarter)    IDENTICAL   /
    1-1.bin                 u43_2a (1st quarter)    IDENTICAL   ; 4 quarters are identical.
*/
ROM_END

ROM_START( goldnpkb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ups31h.12a",	0x0000, 0x8000, CRC(bee5b07a) SHA1(5da60292ecbbedd963c273eac2a1fb88ad66ada8) )
	ROM_RELOAD(					0x8000, 0x8000 )    /* for vectors/pointers */

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u38.5a.bin",	0x0000, 0x2000, CRC(32705e1d) SHA1(84f9305af38179985e0224ae2ea54c01dfef6e12) )    /* text layer */

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "u43.2a.bin",	0x0000, 0x2000, CRC(10b34856) SHA1(52e4cc81b36b4c807b1d4471c0f7bea66108d3fd) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "u40.4a.bin",	0x2000, 0x2000, CRC(5fc965ef) SHA1(d9ecd7e9b4915750400e76ca604bec8152df1fe4) )    /* cards deck gfx, bitplane2 */
	ROM_COPY( REGION_GFX1,	0x0800, 0x4000, 0x0800 )    /* cards deck gfx, bitplane3. found in the 2nd quarter of the text layer rom */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) )

/*  pmpoker                 goldnpkb
    1-4.bin                 u38.5a (1st quarter)    96.582031%  \ 1st and 2nd halves are identical.
    1-3.bin                 u38.5a (2nd quarter)    IDENTICAL   /
    1-1.bin                 u43.2a (1st quarter)    IDENTICAL   ; 4 quarters are identical.
*/
ROM_END

ROM_START( jokerpkr )    /* is this a Coinmaster game? */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "vp-5.bin",	0x2000, 0x1000, CRC(1443d0ff) SHA1(36625d24d9a871cc8c03bdeda983982ba301b385) )
	ROM_LOAD( "vp-6.bin",	0x3000, 0x1000, CRC(94f82fc1) SHA1(ce95fc429f5389eea45fec877bac992fa7ba2b3c) )
	ROM_RELOAD(				0xf000, 0x1000 )    /* for vectors/pointers */

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vp-4.bin",	0x0000, 0x0800, CRC(2c53493f) SHA1(9e71db51499294bb4b16e7d8013e5daf6f1f9d18) )    /* text layer */

	ROM_REGION( 0x1800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vp-1.bin",	0x0000, 0x0800, CRC(f2f94661) SHA1(f37f7c0dff680fd02897dae64e13e297d0fdb3e7) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "vp-2.bin",	0x0800, 0x0800, CRC(6bbb1e2d) SHA1(51ee282219bf84218886ad11a24bc6a8e7337527) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "vp-3.bin",	0x1000, 0x0800, CRC(6e3e9b1d) SHA1(14eb8d14ce16719a6ad7d13db01e47c8f05955f0) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) )
ROM_END

/* the alternative Jack Potten set is identical, but with different sized roms.

                pot1.bin                1ST AND 2ND HALF IDENTICAL
                pot2.bin                1ST AND 2ND HALF IDENTICAL
pottpok1.bin    pot34.bin (1st half)    IDENTICAL
pottpok2.bin    pot34.bin (2nd half)    IDENTICAL
pottpok3.bin    pot2.bin (1st half)     IDENTICAL
pottpok4.bin    pot1.bin (1st half)     IDENTICAL
pottpok5.bin    pot5.bin                IDENTICAL
pottpok6.bin    pot6.bin                IDENTICAL
*/

ROM_START( pottnpkr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "pottpok5.bin",	0x2000, 0x1000, CRC(d74e50f4) SHA1(c3a8a6322a3f1622898c6759e695b4e702b79b28) )
	ROM_LOAD( "pottpok6.bin",	0x3000, 0x1000, CRC(53237873) SHA1(b640cb3db2513784c8d2d8983a17352276c11e07) )
	ROM_RELOAD(					0xf000, 0x1000 )    /* for vectors/pointers */

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pottpok1.bin",	0x0000, 0x0800, CRC(2c53493f) SHA1(9e71db51499294bb4b16e7d8013e5daf6f1f9d18) )    /* text layer */

	ROM_REGION( 0x1800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pottpok4.bin",	0x0000, 0x0800, CRC(f2f94661) SHA1(f37f7c0dff680fd02897dae64e13e297d0fdb3e7) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "pottpok3.bin",	0x0800, 0x0800, CRC(6bbb1e2d) SHA1(51ee282219bf84218886ad11a24bc6a8e7337527) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "pottpok2.bin",	0x1000, 0x0800, CRC(6e3e9b1d) SHA1(14eb8d14ce16719a6ad7d13db01e47c8f05955f0) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) )
ROM_END



/*************************
*      Driver Init       *
*************************/

static DRIVER_INIT( pmpoker )
{
/* temporary patch to avoid hardware input errors */
/*  UINT8 *ROM = memory_region(REGION_CPU1);

    ROM[0x2dae] = 0x4c;
    ROM[0x2daf] = 0xae;
    ROM[0x2db0] = 0x2d;
*/
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia1_intf);
}


/*************************
*      Game Drivers      *
*************************/

/*    YEAR  NAME        PARENT      MACHINE     INPUT       INIT        ROT     COMPANY                     FULLNAME                            FLAGS        */
GAME( 1981,	pmpoker,	0,			pmpoker,	pmpoker,	pmpoker,	ROT0,	"PlayMan?",					"PlayMan Poker (Germany)",			GAME_NO_SOUND )
GAME( 1981,	goldnpkr,	pmpoker,	pmpoker,	goldnpkr,	pmpoker,	ROT0,	"Bonanza Enterprises, Ltd",	"Golden Poker Double Up (Big Boy)",	GAME_NO_SOUND )
GAME( 1981,	goldnpkb,	pmpoker,	pmpoker,	goldnpkr,	pmpoker,	ROT0,	"Bonanza Enterprises, Ltd",	"Golden Poker Double Up (Mini Boy)",GAME_NO_SOUND )
GAME( 1981,	jokerpkr,	pmpoker,	jokerpkr,	jokerpkr,	pmpoker,	ROT0,	"Coinmaster-Gaming, Ltd",	"Joker Poker",						GAME_NO_SOUND )
GAME( 1981,	pottnpkr,	pmpoker,	jokerpkr,	goldnpkr,	pmpoker,	ROT0,	"Unknown",					"Jack Potten's Poker",				GAME_NO_SOUND )

