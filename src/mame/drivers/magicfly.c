/******************************************************************************

    MAGIC FLY
    ----------

    Driver by Roberto Fresca.


    Games running in this hardware:

    * Magic Fly (P&A Games), 198?
    * 7 e Mezzo (Unknown),   198?    (Not sure about the title. Only english text inside the game)


    **** NOTE ****
    You can find a complete hardware & software analysis here:
    http://www.mameworld.net/robbie/magicfly


*******************************************************************************

    Hardware notes:

    - CPU 1x R6502P
    - 1x MC6845P
    - 1x oscillator 10.000MHz
    - ROMs: 1x AM27128 (NS3.1)
            2x SEEQ DQ2764 (1, 2)
            1x SGS M2764 (NS1)
    - PLDs: 1x PAL16R4A
    - 1x 4 DIP switches
    - 1x 30x2 edge connector
    - 1x 10 pins male connector
    - 1x trimmer (volume)


    PCB Layout:
     _________________________________________________________________
    |                                                                 |
    |                                                                 |
    |                                    _________   _________        |
    |                                   | 74LS08N | | 74LS32  |       |
    |                                   |_________| |_________|       |
    |                                    _________   _________        |
    |                                   | 74LS138 | | 74HC00  |       |
    |                                   |_________| |_________|       |
    |    ______________                     __________________        |
    |   |              |                   |                  |   ____|
    |   | MK48Z02B-20  |                   |     R6502P       |  |
    |   |______________|                   |__________________|  |
    |  ________________   _________         __________________   |
    | |                | | 74LS157 |       |                  |  |____
    | |    AM27128     | |_________|       |     MC6845P      |   ____|
    | |________________|  _________        |__________________|   ____|
    |                    | 74LS157 |      ________   _________    ____|
    |                    |_________|     | 74LS14 | | 74LS374 |   ____|
    |  ____________       _________      |________| |_________|   ____|
    | | 74LS245    |     | 74LS157 |                 _________    ____|
    | |____________|     |_________|                | 74HCZ44 |   ____|
    |  ____________       _________                 |_________|   ____|
    | | 74LS245    |     | 74LS32  |      _______                 ____|  30x2
    | |____________|     |_________|     | | | | |                ____|  connector
    |  ______________                    |4|3|2|1|                ____|
    | | HM6116       |                   |_|_|_|_|                ____|
    | | o MSM2128    |                                            ____|
    | |______________|                   DIP SW x4                ____|
    |  ______________                                             ____|
    | | HM6116       |    ________       _________                ____|
    | | o MSM2128    |   | 74LS08 |     | 74LS174 |               ____|
    | |______________|   |________|     |_________|               ____|
    |  ________________   __________                              ____|
    | |                | | PAL16R4A |                             ____|
    | |     2764       | |__________|                             ____|
    | |________________|  __________                              ____|
    |  ________________  | 74LS166  |                             ____|
    | |                | |__________|                            |
    | |     2764       |  __________                             |
    | |________________| | 74LS166  |                            |____
    |  ________________  |__________|                               __|
    | |                |  __________       _________               |  |
    | |     2764       | | 74LS166  |     | 74LS05  |              |8 |  10
    | |________________| |__________|     |_________|              |8 |  pins
    |  ________  ______   __________       _________               |8 |  male
    | | 74LS04 || osc. | | 74LS193  |     | 74LS86  |              |8 |  connector
    | |________||10 MHz| |__________|     |_________|              |8 |
    |           |______|                                           |__|
    |_________________________________________________________________|


    NOTE: Magic Fly & 7 e Mezzo PAL16R4A Fuse Maps were converted to the new bin format and added to respective sets.


    Pinouts (7mezzo):
    -----------------
    ************ Edge connector ************

    (Solder Side)               (Parts Side)

    GND                 30      GND
    +10v.               29      +10v.
    +10v.               28      +10v.
    unused              27      unused
    unused              26      unused
    GND                 25      GND
    +12v.               24      +12v.
    +12v.               23      +12v.
    unused              22      unused
    common C (3)        21      common A (1)
    common D (4)        20      common B (2)
    DEAL                19      DOUBLE
    HOLD 1              18      (unreadable)
    HOLD 2              17      HOLD 5
    HOLD 3              16      HOLD 4
    METER               15      BET
    COUPON              14
                        13      COIN 1
    (unreadable)        12      COIN 2
    TAKE                11      PAY
    SMALL (play1)       10      BIG (play3)
    unused              09      unused
    unused              08      unused
    unused              07      unused
    (unreadable)        06      (unreadable)
    sync                05      (unreadable)
    GND                 04      GND
    speaker+            03      speaker+
    speaker- (GND)      02      speaker- (GND)
    +5v.                01      +5v.

    (1) = DOUBLE, DEAL, (unreadable), BET, METER
    (2) = TAKE, SMALL, BIG, PAY
    (3) = HOLD 1, HOLD 2, HOLD 3, HOLD 4, HOLD 5
    (4) = COIN 1, COIN 2, COUPON


    **** Pins connector ****

    pin 01:
    pin 02:
    pin 03:
    pin 04:
    pin 05:  (soldered to pin 01)
    pin 06:  (unreadable)
    pin 07:  (unreadable)
    pin 08:  (unreadable)
    pin 09:  (unreadable)
    pin 10:  common (GND)



    Memory Map (magicfly)
    ------------------------

    $0000 - $00FF    RAM    // Zero page (pointers and registers)

                         ($000D)            // Incremented each time a NMI is triggered.
                         ($001D)            // in case of #$00, NMI do nothing and return.
                         ($0011)            // Store lenghts for text handling.
                         ($0012)            // Store values to be written in color RAM.
                         ($0013 - $0014)    // Pointer to text offset.
                         ($0015 - $0016)    // Pointer to video ram.
                         ($0017 - $0018)    // Pointer to color ram.
                         ($0019)            // Program loops waiting for a value to be written here through NMI (see code at $CA71).
                         ($003A - $003D)    // Store new read values from $2800 (input port) through NMI.
                         ($003F - $0042)    // Store old read values from $2800 (input port) through NMI.
                         ($005E - $005F)    // Store values to be written in video and color ram, respectively.
                         ($0067 - $0067)    // Program compare the content with #$02, #$03, #$04, #$05 and #$E1.
                                            // If #$E1 is found here, the machine hangs showing "I/O ERROR" (see code at $C1A2).
                         ($0096 - $0098)    // Store values from content of $2800 (input port) AND #$80, AND #$40, AND #$10.
                         ($009B - $00A8)    // Text scroll buffer.

    $0100 - $01FF    RAM    // 6502 Stack Pointer.

    $0200 - $07FF    RAM

    $0800 - $0801    MC6845    // MC6845 use $0800 for register addressing and $0801 for register values (see code at $CE86).

                                  *** MC6845 init ***

                                  Register:   00    01    02    03    04    05    06    07    08    09    10    11    12    13    14    15    16    17
                                  Value:     #$27  #$20  #$23  #$03  #$1F  #$04  #$1D  #$1E  #$00  #$07  #$00  #$00  #$00  #$00  #$00  #$00  #$00  #$00.

    $1000 - $13FF    Video RAM    // Initialized in subroutine starting at $CF83, filled with value stored in $5E.

    $1800 - $1BFF    Color RAM    // Initialized in subroutine starting at $CF83, filled with value stored in $5F.
                                  // (In 7mezzo is located at $CB13 using $64 and $65 to store video and color ram values.)

    $1C00 - $27FF    RAM

    $2800 - $2800    Input port    // Input port (code at $CE96). No writes, only reads.
                                   // NMI routine read from here and store new values to $003A - $003D and copy old ones to $003F - $0042.
                                   // Code accept only bits 4, 6 & 7 as valid. If another bit is activated, will produce an I/O error. (code at $CD0C)

    $2801 - $2FFF    RAM

    $3000 - $3000    ???    // Something seems to be mapped here. Only writes, no reads.
                            // Code at $C152 do a complex loop with boolean operations and write #$00/#$80 to $3000. Also NMI routine write another values.

    $3001 - $BFFF    RAM

    $C000 - $FFFF    ROM    // Program ROM.

    -------------------------------------------------------------------------

    Driver updates:

    [2006-07-07]
    - Initial release.

    [2006-07-11]
    - Corrected the total number of chars to decode by rom.
    - Fixed the graphics offset for the text layer.
    - Adjusted the gfx rom region bounds properly.

    [2006-07-21]
    - Rewrote the technical info.
    - Removed fuse maps and unaccurate things.
    - Added new findings, pinouts, and pieces of code.
    - Confirmed and partially mapped one input port.
    - Added a little patch to pass over some checks (for debugging purposes).

    [2006-07-26]
    - Figured out the MC6845 (mapped at $0800-$0801).
    - Fixed the screen size based on MC6845 registers.
    - Fixed the visible area based on MC6845 registers.
    - Corrected the gfx rom region.
    - Solved the NMI/vblank issue. Now attract works.
    - Changed CPU clock to 625khz. (text scroll looks so fast with the former value)
    - Added new findings to the technical notes.
    - Added version/revision number to magicfly.
    - Marked magicfly PAL as NO_DUMP (read protected).
    - Added flags GAME_IMPERFECT_GRAPHICS and GAME_WRONG_COLORS.

    [2006-08-06]
    - Figured out how the gfx banks works.
    - Fixed the gfx layers.
    - Fixed the gfx decode.
    - Removed flag GAME_IMPERFECT_GRAPHICS.


    TODO:

    - Map inputs & DIP switches.
    - Correct colors.
    - Figure out $3000 writes (???).
    - Figure out the sound.
    - Clean and sort out a lot of things.


*******************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "video/crtc6845.h"


/*************************
*     Video Hardware     *
*************************/

static tilemap *bg_tilemap;

WRITE8_HANDLER( magicfly_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( magicfly_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
//  ...x ....   tiles bank.
//  .... xxxx   tiles color.

	int attr = colorram[tile_index];
	int code = videoram[tile_index];
	int bank = (attr & 0x10) >> 4;    // bit 4 switch the gfx banks.
	int color = attr & 0x0f;          // bits 0-3 for color.

	if (bank == 1)	// gfx banks are inverted.
		bank = 0;
	else
		bank = 1;

	SET_TILE_INFO(bank, code, color, 0)
}

VIDEO_START(magicfly)
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 29);

	return 0;
}

VIDEO_UPDATE(magicfly)
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}


/*************************
* Memory map information *
*************************/

static ADDRESS_MAP_START( magicfly_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x00ff) AM_RAM    // zero page (pointers and registers)
	AM_RANGE(0x0100, 0x01ff) AM_RAM    // stack pointer
	AM_RANGE(0x0200, 0x07ff) AM_RAM
	AM_RANGE(0x0800, 0x0800) AM_WRITE(crtc6845_address_w)    // crtc6845 register addressing
	AM_RANGE(0x0801, 0x0801) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)    // crtc6845 register values
	AM_RANGE(0x0802, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_WRITE(magicfly_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x1400, 0x17ff) AM_RAM
	AM_RANGE(0x1800, 0x1bff) AM_RAM AM_WRITE(magicfly_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x1c00, 0x27ff) AM_RAM
	AM_RANGE(0x2800, 0x2800) AM_READ(input_port_0_r)
	AM_RANGE(0x2801, 0x2fff) AM_RAM
	AM_RANGE(0x3000, 0x3000) AM_WRITENOP    // main code write #$00/#$80 here. NMI write #$01, #$02, #$04, #$08.
	AM_RANGE(0x3001, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


/*************************
*      Input ports       *
*************************/

INPUT_PORTS_START( magicfly )

  /* Code accept only bits 4, 6 & 7 as valid. If another bit is activated, will produce an I/O error. */
  PORT_START_TAG("IN0")
  PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 0") PORT_CODE(KEYCODE_Q)
  PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 1") PORT_CODE(KEYCODE_W)
  PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 2") PORT_CODE(KEYCODE_E)
  PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 3") PORT_CODE(KEYCODE_R)
  PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 4") PORT_CODE(KEYCODE_T)
  PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 5") PORT_CODE(KEYCODE_Y)
  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 6") PORT_CODE(KEYCODE_U)
  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 7") PORT_CODE(KEYCODE_I)

//  PORT_START_TAG("IN1")
//  PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 0") PORT_CODE(KEYCODE_A)
//  PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 1") PORT_CODE(KEYCODE_S)
//  PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 2") PORT_CODE(KEYCODE_D)
//  PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 3") PORT_CODE(KEYCODE_F)
//  PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 4") PORT_CODE(KEYCODE_G)
//  PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 5") PORT_CODE(KEYCODE_H)
//  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 6") PORT_CODE(KEYCODE_J)
//  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 7") PORT_CODE(KEYCODE_K)
//
//  PORT_START_TAG("DSW0")    // Only 4 DIP switches
//  PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
//  PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( On ) )
//  PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
//  PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( On ) )
//  PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
//  PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( On ) )
//  PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
//  PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
//  PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/*************************
*    Graphics Layouts    *
*************************/

static const gfx_layout charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout tilelayout =
{
	8, 8,
	256,
	3,
	{ 0, (0x2800*8), (0x4800*8) },    // bitplanes are separated.
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

/******************************
* Graphics Decode Information *
******************************/

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1,	0x1800,	&charlayout, 2, 16 },
	{ REGION_GFX1,	0x1000,	&tilelayout, 0, 16 },
	{ -1 }
};


/*************************
*     Machine Driver     *
*************************/

static MACHINE_DRIVER_START( magicfly )
	// basic machine hardware
	MDRV_CPU_ADD(M6502, 10000000/16)	// a guess
	MDRV_CPU_PROGRAM_MAP(magicfly_map, 0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((39+1)*8, (31+1)*8)           // Taken from MC6845 init, registers 00 & 04. Normally programmed with (value-1).
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 29*8-1)    // Taken from MC6845 init, registers 01 & 06.

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(256)

	MDRV_VIDEO_START(magicfly)
	MDRV_VIDEO_UPDATE(magicfly)

MACHINE_DRIVER_END


static DRIVER_INIT( magicfly )
{

/***************************************************************
*
* Temporary patch to avoid a loop of checks for debug purposes.
*
* After check the last bit of $1800, code jump into a loop ($DA30)
*
*   changed to :
*
*    DA30: 60         rts
*
***************************************************************/

	UINT8 *ROM = memory_region(REGION_CPU1);

	ROM[0xda30] = 0x60;
}


static DRIVER_INIT( 7mezzo )
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	ROM[0xd21a] = 0x60;    // Similar to magicfly, but another offset.
}


/*************************
*        Rom Load        *
*************************/

ROM_START( magicfly )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "magicfly3.3",    0xc000, 0x4000, CRC(c29798d5) SHA1(bf92ac93d650398569b3ab79d01344e74a6d35be) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "magicfly2.bin",    0x0000, 0x2000, CRC(3596a45b) SHA1(7ec32ec767d0883d05606beb588d8f27ba8f10a4) )
	ROM_LOAD( "magicfly1.bin",    0x2000, 0x2000, CRC(724d330c) SHA1(cce3923ce48634b27f0e7d29979cd36e7394ab37) )
	ROM_LOAD( "magicfly0.bin",    0x4000, 0x2000, CRC(44e3c9d6) SHA1(677d25360d261bf2400f399b8015eeb529ad405e) )

	ROM_REGION( 0x0200, REGION_PLDS, ROMREGION_DISPOSE )
	ROM_LOAD( "pal16r4a-magicfly.bin",    0x0000, 0x0104, NO_DUMP )    /* PAL is read protected */

ROM_END

ROM_START( 7mezzo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ns3.1",      0xc000, 0x4000, CRC(b1867b76) SHA1(eb76cffb81c865352f4767015edade54801f6155) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1.bin",      0x0000, 0x2000, CRC(7983a41c) SHA1(68805ea960c2738d3cd2c7490ffed84f90da029b) )    // should be named ns2.bin regarding pcb location and content.
	ROM_LOAD( "ns1.bin",    0x2000, 0x2000, CRC(a6ada872) SHA1(7f531a76e73d479161e485bdcf816eb8eb9fdc62) )
	ROM_LOAD( "2.bin",      0x4000, 0x2000, CRC(e04fb210) SHA1(81e764e296fe387daf8ca67064d5eba2a4fc3c26) )    // should be named ns0.bin regarding pcb location and content.

	ROM_REGION( 0x0200, REGION_PLDS, ROMREGION_DISPOSE )
	ROM_LOAD( "pal16r4a-7mezzo.bin",    0x0000, 0x0104, CRC(61ac7372) SHA1(7560506468a7409075094787182ded24e2d0c0a3) )
ROM_END


/*************************
*      Game Drivers      *
*************************/

//    YEAR  NAME      PARENT    MACHINE   INPUT     INIT             COMPANY        FULLNAME
GAME( 198?, magicfly, 0,        magicfly, magicfly, magicfly, ROT0, "P&A Games",   "Magic Fly (Ver 0.3)", GAME_WRONG_COLORS | GAME_NO_SOUND | GAME_NOT_WORKING )
GAME( 198?, 7mezzo,   0,        magicfly, magicfly, 7mezzo,   ROT0, "Unknown",     "7 e Mezzo",           GAME_WRONG_COLORS | GAME_NO_SOUND | GAME_NOT_WORKING )

