/*******************************************************************************
 Sega System E (834-5803) Driver (drivers/segasyse.c)
********************************************************************************
 driver by David Haywood

 Special Thanks to:

 Charles Macdonald - for docmentation of the SMS VDP Chip used on this system,
 visit his homepage (cgfm2.emuviews.com) for some excellent documents, and other
 interesting bits and pieces

 Mike Beaver - Mimic, although not used as a reference, it was used as an
 inspiration & motivation :p

 Stephane Humbert - Dipswitch Information, Z80 Help, Lots of Notes of ROP, help
 with the controller for ROP, and generally being an all round great guy to
 work with.

********************************************************************************

 Sega System 'E' is a piece of hardware used for a couple of Arcade Games
 produced by Sega in the Mid 80's, its roughly based on their Sega Master System
 home console unit, using the same '315-5124' VDP (actually in this case 2 of
 them)

 An interesting feature of the system is that the CPU is contained on the ROM
 board, the MAIN System E board contains the Graphics processor, this opens the
 possibility for using processors other than the Standard Z80 to run the main
 game code on, an option which they appear to have made use of for a couple of
 the titles unless of course the roms are simply bad dumps.

 Also interesting is each VDP has double the Video RAM found on the SMS console
 this is banked through Port Writes, the System also allows for the Video RAM
 to be written directly, bypassing the usual procedure of writing to it via the
 '315-5124' data port, it can not however be read directly, the same area used
 for writing is used to access banked ROMs when reading

 Pretty much everything on this hardware is done through port accesses, the
 main memory map consists of simply ROM, BANKED ROM / BANKED RAM, RAM

********************************************************************************

    ROMs + CPU Board (32kb ROMs)

    IC 07 (Good)    IC 05 (Good)
    IC 04 (Good)    IC 03 (Good)
    IC 02 (Good)

    (834-5803) MAIN Board (8kb RAMs)

    IC 49 (Good)    IC 55 (Good)    System RAM (0xc000 - 0xffff)
    IC 03 (Good)    IC 07 (Good)    Front Layer VRAM (Bank 1)   Port F7 -0------
    IC 04 (Good)    IC 08 (Good)    Front Layer VRAM (Bank 2)   Port F7 -1------
    IC 01 (Good)    IC 05 (Good)    Back Layer VRAM (Bank 1)    Port F7 0-------
    IC 02 (Good)    IC 06 (Good)    Back Layer VRAM (Bank 2)    Port F7 1-------
    (or at least this is how it appears from HangOnJr's RAMs Test)

    2x (315-5124)'s here too, these are the VDP chips

    PORTS (to be completed)

    0xba - 0xbb r/w     Back Layer VDP
    0xbe - 0xbf r/w     Front Layer VDP

    0xf7 w/o            Banking Controls

    0xe0 r/o            Inputs (Coins, Start Btns)
    0xe1 r/o            Controls (Transformer)

    0xf2 - 0xf3 r/o     Dipswitches

    0xf8 r/o            Analog Input (Hang On Jr)

    0x7e r/o            V Counter (vertical beam pos in scanlines)
    0x7f r/o            H Counter (horizontal beam pos in 'pixel clock cycles')

********************************************************************************

 Change Log:
 18 Aug 2004 | DH - Added Tetris System E
 14 Jun 2001 | Stephh added the dipswitches to ROP (and coinage to the others
             | I added Save State support
 14 Jun 2001 | Nicola improved the Controls for Riddle, stephh added a New
             | SEGASYSE_COINAGE macro and fixed sorted out the dips in ROP
 13 Jun 2001 | After A Lot of Messing about, Hair Pulling out etc. Riddle is
             | now playable :p
 12 Jun 2001 | First Submitted Version, still a couple of things need doing,
             | Riddle isn't playable at this stage
 12 Jun 2001 | Cleaned Up The Code (removed a lot of now redundant stuff in the
             | interrupt functions, rendering code (dirty tile marking etc.))
 12 Jun 2001 | Major Updates made, Sound Added, Controls, Sprites, Raster
             | Corrected, Horizontal Scrolling Corrected, Colours Corrected,
             | Transformer is now Fully Playable, HangOn-Jr Likewise with
             | minor gfx glitches in places.
 11 Jun 2001 | Progressing Nicely, Improvements to the Banking, inc. Writes to
             | the 0x8000+ range, Raster Interrupts hooked up and sort of
             | working (looks ok in Transformer, not in Hang-On Jr
 07 Jun 2001 | Big Clean-Up of Driver so far, lots of things moved around to
             | make it easier to follow and develop
 06 Jun 2001 | Improved the Banking a bit, based on what the program seems to be
             | trying to do as it goes through its RAM/ROM tests, still some
             | bits of Port 0xF7 writes remain unclear however.
 05 Jun 2001 | Put in place some very crude rendering code, the RAM / ROM checks
             | of transfrm now display on the screen
 04 Jun 2001 | transfrm (Transformer) is showing signs of life, the RAM / ROM
             | check info can clearly be seen in VRAM, now to try and get some
             | rendering code working.  Tiles (SEGA logo, Font etc, now also
             | appear in VRAM)
 03 Jun 2001 | Driver Rewritten from Scratch after a long period of inactivity,
             | the VDP core now seems to be running correctly, valid colours can
             | be seen in CRAM

********************************************************************************

 To Do:
 - Improve Riddle of Pythagoras, doesn't like the sprite code much...
 - Fix Vertical Scrolling, needed for Hang-On Jr. Score Screen
 - Find Register which does the Left column blank, seems to be missing from the
   docs..
 - hook up dsw's in riddle, stephh kindly worked them out (see notes below,
   they just need adding to the input ports
 - Fix Astro Flash service mode (it works in Transformer)

 - Decrypt the other games (Fantasy Zone 2 & Opa Opa) looks tricky..
    WE NEED THE ORIGINAL ***WORKING*** CPUS

********************************************************************************
 Game Notes:
 Riddle of Pythagoras is interesting, it looks like Sega might have planned it
 as a two player game, there is prelimiary code for 2 player support which
 never gets executed, see code around 0x0E95.  Theres also quite a bit of
 pointless code here and there.  Some Interesting Memory Locations

 C000 : level - value (00-0x32)
 C001 : level - display (00-0x50, BCD coded)
 C003 : credits (00-0x99, BCD coded)
 C005 : DSWA put here (coinage, left and right nibbles for left and right slot
        - freeplay when 0x0f or 0xf0)
 C006 : DSWB put here
  bits 0 and 1 : lives ("02", "03", "04", "98")
  bit 3 : difficulty
  bits 5 and 6 : bonus lives ("50K, 100K, 200K, 500K", "100K, 200K, 500K", "100K,
                               200K, 500K, 99999999", "none")
 C009 : lives (for player 1)
 C00A : lives (for player 2)
 C00B : bonus lives counter

 E20B-E20E : score (00000000-0x99999999, BCD coded)
 E215-E218 : hi-score (00000000-0x99999999, BCD coded)

 E543 : bit 0 : ON = player 1 one still has lives
        bit 1 : ON = player 2 one still has lives
        bit 2 : ON = player 1 is the current player - OFF = player 2 is the
         current player

 E572 : table with L. slot infos (5 bytes wide)
 E577 : table with R. slot infos (5 bytes wide)
*******************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/segacrpt.h"
#include "sound/sn76496.h"

/*-- Variables --*/

static UINT8 segae_8000bank;	/* Current VDP Bank Selected for 0x8000 - 0xbfff writes */
static UINT8 port_fa_last;		/* Last thing written to port 0xfa (control related) */
static UINT8 hintcount;			/* line interrupt counter, decreased each scanline */
UINT8 vintpending;				/* vertical interrupt pending flag */
UINT8 hintpending;				/* scanline interrupt pending flag */

/*- in (video/segasyse.c) -*/
extern UINT8 segae_vdp_vrambank[];	/* vdp's vram bank */
extern UINT8 *segae_vdp_vram[];		/* pointer to start of vdp's vram */
extern UINT8 *segae_vdp_regs[];		/* pointer to vdp's registers */

/*-- Prototypes --*/

static WRITE8_HANDLER (segae_mem_8000_w);

static WRITE8_HANDLER (segae_port_f7_w);
static READ8_HANDLER (segae_port_7e_7f_r);

static READ8_HANDLER (segae_port_ba_bb_r);
static READ8_HANDLER (segae_port_be_bf_r);

static WRITE8_HANDLER (segae_port_ba_bb_w);
static WRITE8_HANDLER (segae_port_be_bf_w);

/*- in (video/segasyse.c) -*/

VIDEO_START( segae );
VIDEO_UPDATE( segae );

unsigned char segae_vdp_ctrl_r ( UINT8 chip );
unsigned char segae_vdp_data_r ( UINT8 chip );
void segae_vdp_ctrl_w ( UINT8 chip, UINT8 data );
void segae_vdp_data_w ( UINT8 chip, UINT8 data );
void segae_drawscanline(int line, int chips, int blank);

/*******************************************************************************
 Port & Memory Maps
********************************************************************************
 most things on this hardware are done via port writes, including reading of
 controls dipswitches, reads and writes to the vdp's etc.  see notes at top of
 file, the most noteworthy thing is the use of the 0x8000 - 0xbfff region, reads
 are used to read ram, writes are used as a secondary way of writing to VRAM
*******************************************************************************/

/*-- Memory --*/

static ADDRESS_MAP_START( segae_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)				/* Fixed ROM */
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1)				/* Banked ROM */
	AM_RANGE(0xc000, 0xffff) AM_READ(MRA8_RAM)				/* Main RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( segae_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)				/* Fixed ROM */
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(segae_mem_8000_w)		/* Banked VRAM */
	AM_RANGE(0xc000, 0xffff) AM_WRITE(MWA8_RAM)				/* Main RAM */
ADDRESS_MAP_END

/*-- Ports --*/

static ADDRESS_MAP_START( segae_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x7e, 0x7f) AM_READ(segae_port_7e_7f_r)			/* Vertical / Horizontal Beam Position Read */
	AM_RANGE(0xba, 0xbb) AM_READ(segae_port_ba_bb_r)			/* Back Layer VDP */
	AM_RANGE(0xbe, 0xbf) AM_READ(segae_port_be_bf_r)			/* Front Layer VDP */
	AM_RANGE(0xe0, 0xe0) AM_READ(input_port_2_r) /* Coins + Starts */
	AM_RANGE(0xe1, 0xe1) AM_READ(input_port_3_r) /* Controls */
	AM_RANGE(0xf2, 0xf2) AM_READ(input_port_0_r) /* DSW0 */
	AM_RANGE(0xf3, 0xf3) AM_READ(input_port_1_r) /* DSW1 */
ADDRESS_MAP_END

static ADDRESS_MAP_START( segae_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x7b, 0x7b) AM_WRITE(SN76496_0_w) /* Not sure which chip each is on */
	AM_RANGE(0x7f, 0x7f) AM_WRITE(SN76496_1_w) /* Not sure which chip each is on */
	AM_RANGE(0xba, 0xbb) AM_WRITE(segae_port_ba_bb_w)			/* Back Layer VDP */
	AM_RANGE(0xbe, 0xbf) AM_WRITE(segae_port_be_bf_w)			/* Front Layer VDP */
	AM_RANGE(0xf7, 0xf7) AM_WRITE(segae_port_f7_w)			/* Banking Control */
ADDRESS_MAP_END

/*******************************************************************************
 Read / Write Handlers
********************************************************************************
 the ports 0xf8, 0xf9, 0xfa appear to be in some way control related, their
 behavior is not fully understood, however for HangOnJr it appears that we
 need to read either the accelerator or angle from port 0xf8 depending on the
 last write to port 0xfa, this yields a playable game,

 For Riddle of Pythagoras it doesn't seem so simple, the code we have here
 seems to general do the job but it could probably do with looking at a bit
 more
*******************************************************************************/

/*-- Memory -- */

static WRITE8_HANDLER (segae_mem_8000_w)
{
	/* write the data the non-selected VRAM bank of the opposite number VDP chip stored in segae_8000bank */
	segae_vdp_vram [1-segae_8000bank][offset + (0x4000-(segae_vdp_vrambank[1-segae_8000bank] * 0x4000))] = data;
}

/*-- Ports --*/

/***************************************
 WRITE8_HANDLER (segae_port_f7_w)
****************************************
 writes here control the banking of
 ROM and RAM

 bit:
  7 - Back Layer VDP (0) Vram Bank Select
  6 - Front Layer VDP (1) Vram Bank Select
  5 - Select 0x8000 write location (1 = Back Layer VDP RAM, 0 = Front Layer VDP RAM)
        *writes are to the non-selected bank*
  4 - unknown
  3 - unknown
  2 - \
  1 - | Rom Bank Select for 0x8000 -
  0 - |                 0xbfff reads

***************************************/

static UINT8 rombank;

void segae_bankswitch (void)
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	memory_set_bankptr( 1, &RAM[ 0x10000 + ( rombank * 0x4000 ) ] );
}


static WRITE8_HANDLER (segae_port_f7_w)
{
	segae_vdp_vrambank[0] = (data & 0x80) >> 7; /* Back  Layer VDP (0) VRAM Bank */
	segae_vdp_vrambank[1] = (data & 0x40) >> 6; /* Front Layer VDP (1) VRAM Bank */
	segae_8000bank		  = (data & 0x20) >> 5; /* 0x8000 Write Select */
	rombank				  =  data & 0x07;		/* Rom Banking */

	segae_bankswitch();
}

/*- Beam Position -*/

static READ8_HANDLER (segae_port_7e_7f_r)
{
	UINT8 temp = 0;
	UINT16 sline;

	switch (offset)
	{
		case 0: /* port 0x7e, Vert Position (in scanlines) */
			sline = 261 - cpu_getiloops();
			if (sline > 0xDA) sline -= 6;
			temp = sline-1 ;
			break;
		case 1: /* port 0x7f, Horz Position (in pixel clock cycles)  */
			/* unhandled for now */
			break;
	}
	return temp;
}

/*- VDP Related -*/

static READ8_HANDLER (segae_port_ba_bb_r)
{
	/* These Addresses access the Back Layer VDP (0) */
	UINT8 temp = 0;

	switch (offset)
	{
		case 0: /* port 0xba, VDP 0 DATA Read */
			temp = segae_vdp_data_r(0); break;
		case 1: /* port 0xbb, VDP 0 CTRL Read */
			temp = segae_vdp_ctrl_r(0); break;
	}
	return temp;
}

static READ8_HANDLER (segae_port_be_bf_r)
{
	/* These Addresses access the Front Layer VDP (1) */
	UINT8 temp = 0;

	switch (offset)
	{
		case 0: /* port 0xbe, VDP 1 DATA Read */
			temp = segae_vdp_data_r(1); break ;
		case 1: /* port 0xbf, VDP 1 CTRL Read */
			temp = segae_vdp_ctrl_r(1); break ;
	}
	return temp;
}

static WRITE8_HANDLER (segae_port_ba_bb_w)
{
	/* These Addresses access the Back Layer VDP (0) */
	switch (offset)
	{
		case 0: /* port 0xba, VDP 0 DATA Write */
			segae_vdp_data_w(0, data); break;
		case 1: /* port 0xbb, VDP 0 CTRL Write */
			segae_vdp_ctrl_w(0, data); break;
	}
}

static WRITE8_HANDLER (segae_port_be_bf_w)
{
	/* These Addresses access the Front Layer VDP (1) */
	switch (offset)
	{
		case 0: /* port 0xbe, VDP 1 DATA Write */
			segae_vdp_data_w(1, data); break;
		case 1: /* port 0xbf, VDP 1 CTRL Write */
			segae_vdp_ctrl_w(1, data); break;
	}
}

/*- Hang On Jr. Specific -*/

static READ8_HANDLER (segae_hangonjr_port_f8_r)
{
	UINT8 temp;

	temp = 0;

	if (port_fa_last == 0x08)  /* 0000 1000 */ /* Angle */
		temp = readinputport(4);

	if (port_fa_last == 0x09)  /* 0000 1001 */ /* Accel */
		temp = readinputport(5);

	return temp;
}

static WRITE8_HANDLER (segae_hangonjr_port_fa_w)
{
	/* Seems to write the same pattern again and again bits ---- xx-x used */
	port_fa_last = data;
}

/*- Riddle of Pythagoras Specific -*/

static int port_to_read,last1,last2,diff1,diff2;

static READ8_HANDLER (segae_ridleofp_port_f8_r)
{
	switch (port_to_read)
	{
		default:
		case 0:	return diff1 & 0xff;
		case 1:	return diff1 >> 8;
		case 2:	return diff2 & 0xff;
		case 3:	return diff2 >> 8;
	}
}

static WRITE8_HANDLER (segae_ridleofp_port_fa_w)
{
	/* 0x10 is written before reading the dial (hold counters?) */
	/* 0x03 is written after reading the dial (reset counters?) */

	port_to_read = (data & 0x0c) >> 2;

	if (data & 1)
	{
		int curr = readinputport(4);
		diff1 = ((curr - last1) & 0x0fff) | (curr & 0xf000);
		last1 = curr;
	}
	if (data & 2)
	{
		int curr = readinputport(5) & 0x0fff;
		diff2 = ((curr - last2) & 0x0fff) | (curr & 0xf000);
		last2 = curr;
	}
}


/*******************************************************************************
 Input Ports
********************************************************************************
 mostly unknown for the time being
*******************************************************************************/

	/* The Coinage is similar to Sega System 1 and C2, but
    it seems that Free Play is not used in all games
    (in fact, the only playable game that use it is
    Riddle of Pythagoras) */

#define SEGA_COIN_A \
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x05, "2 Coins/1 Credit 5/3 6/4" ) \
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit, 4/3" ) \
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit, 5/6" ) \
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit, 4/5" ) \
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit, 2/3" ) \
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )

#define SEGA_COIN_B \
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x50, "2 Coins/1 Credit 5/3 6/4" ) \
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit, 4/3" ) \
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit, 5/6" ) \
	PORT_DIPSETTING(    0x20, "1 Coin/1 Credit, 4/5" ) \
	PORT_DIPSETTING(    0x10, "1 Coin/1 Credit, 2/3" ) \
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )

INPUT_PORTS_START( dummy ) /* Used by the Totally Non-Working Games */
INPUT_PORTS_END

INPUT_PORTS_START( transfrm ) /* Used By Transformer */
	PORT_START_TAG("DSW0")	/* Read from Port 0xf2 */
	SEGA_COIN_A
	SEGA_COIN_B

	PORT_START_TAG("DSW1")	/* Read from Port 0xf3 */
	PORT_DIPNAME( 0x01, 0x00, "1 Player Only" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "Infinite (Cheat)")
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x20, "10k, 30k, 50k and 70k" )
	PORT_DIPSETTING(    0x30, "20k, 60k, 100k and 140k"  )
	PORT_DIPSETTING(    0x10, "30k, 80k, 130k and 180k" )
	PORT_DIPSETTING(    0x00, "50k, 150k and 250k" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )

	PORT_START_TAG("IN0")	/* Read from Port 0xe0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START2 )

	PORT_START_TAG("IN1")	/* Read from Port 0xe1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP  ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( tetrisse ) /* Used By Transformer */
	PORT_START_TAG("DSW0")	/* Read from Port 0xf2 */
	SEGA_COIN_A
	SEGA_COIN_B

	PORT_START_TAG("DSW1")	/* Read from Port 0xf3 */
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

	PORT_START_TAG("IN0")	/* Read from Port 0xe0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START2 )

	PORT_START_TAG("IN1")	/* Read from Port 0xe1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP  ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( hangonjr ) /* Used By Hang On Jr */
	PORT_START_TAG("DSW0")	/* Read from Port 0xf2 */
	SEGA_COIN_A
	SEGA_COIN_B

	PORT_START_TAG("DSW1")	/* Read from Port 0xf3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, "Enemies" )
	PORT_DIPSETTING(    0x06, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )  // These three dips seems to be unused
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN0")	/* Read from Port 0xe0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("IN1")	/* Read from Port 0xe1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("IN2")	/* Read from Port 0xf8 */
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_X ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START_TAG("IN3")  /* Read from Port 0xf8 */
	PORT_BIT( 0xff, 0x00, IPT_AD_STICK_Y ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( ridleofp ) /* Used By Riddle Of Pythagoras */
	PORT_START_TAG("DSW0")	/* Read from Port 0xf2 */
	SEGA_COIN_A
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	SEGA_COIN_B
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START_TAG("DSW1")	/* Read from Port 0xf3 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "98 (Cheat)")
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )  // Unknown
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Difficulty?" )	// To be tested ! I don't see what else it could do
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )  // Unknown
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x60, "50K 100K 200K 500K 1M 2M 5M 10M" )
	PORT_DIPSETTING(    0x40, "100K 200K 500K 1M 2M 5M 10M" )
	PORT_DIPSETTING(    0x20, "200K 500K 1M 2M 5M 10M" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )  // Unknown
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN0")	/* Read from Port 0xe0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN ) // Would Be IPT_START2 but the code doesn't use it

	PORT_START_TAG("IN1")	/* Port 0xe1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("IN2")	/* Read from Port 0xf8 */
	PORT_BIT( 0x0fff, 0x0000, IPT_DIAL ) PORT_SENSITIVITY(8) PORT_KEYDELTA(125)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON2 )	/* is this used in the game? */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("IN3")	/* Read from Port 0xf8 */
	PORT_BIT( 0x0fff, 0x0000, IPT_DIAL ) PORT_SENSITIVITY(8) PORT_KEYDELTA(125) PORT_COCKTAIL
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END

/*******************************************************************************
 Interrupt Function
********************************************************************************
 Lines 0 - 191 | Dislay
 Lines 0 - 261 | Non-Display / VBlank Period

 VDP1 Seems to be in Charge of Line Interrupts at Least

 Interrupt enable bits etc are a bit uncertain
*******************************************************************************/

 INTERRUPT_GEN( segae_interrupt )
{
	int sline;
	sline = 261 - cpu_getiloops();

	if (sline ==0) {
		hintcount = segae_vdp_regs[1][10];
	}

	if (sline <= 192) {

		if (sline != 192) segae_drawscanline(sline,1,1);

		if (sline == 192)
			vintpending = 1;

		if (hintcount == 0) {
			hintcount = segae_vdp_regs[1][10];
			hintpending = 1;

			if  ((segae_vdp_regs[1][0] & 0x10)) {
				cpunum_set_input_line(0, 0, HOLD_LINE);
				return;
			}

		} else {
			hintcount--;
		}
	}

	if (sline > 192) {
		hintcount = segae_vdp_regs[1][10];

		if ( (sline<0xe0) && (vintpending) ) {
			cpunum_set_input_line(0, 0, HOLD_LINE);
		}
	}
}

/*******************************************************************************
 General Init
********************************************************************************
 for Save State support
*******************************************************************************/

static MACHINE_START( segasyse )
{
	state_save_register_global(segae_8000bank);
	state_save_register_global(vintpending);
	state_save_register_global(hintpending);
	state_save_register_global(rombank);
	state_save_register_func_postload(segae_bankswitch);
	return 0;
}

/*******************************************************************************
 Machine Driver(s)
********************************************************************************
 a z80, unknown speed
 the two SN76489's are located on the VDP chips, one on each

 some of this could potentially vary between games as the CPU is on the ROM
 board no the main board, for instance Astro Flash appears to have some kind of
 custom cpu
*******************************************************************************/

static MACHINE_DRIVER_START( segae )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,10738600/2) /* correct for hangonjr, and astroflash/transformer at least  */
	MDRV_CPU_PROGRAM_MAP(segae_readmem,segae_writemem)
	MDRV_CPU_IO_MAP(segae_readport,segae_writeport)
	MDRV_CPU_VBLANK_INT(segae_interrupt,262)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START( segasyse )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 192)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 192-1)
	MDRV_PALETTE_LENGTH(64)

	MDRV_VIDEO_START(segae)
	MDRV_VIDEO_UPDATE(segae)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76496, 4000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(SN76496, 4000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

/*******************************************************************************
 Game Inits
********************************************************************************
 Just the One for now (Hang On Jr), Installing the custom READ/WRITE handlers
 we need for the controls
*******************************************************************************/

static DRIVER_INIT( hangonjr )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0xf8, 0xf8, 0, 0, segae_hangonjr_port_f8_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0xfa, 0xfa, 0, 0, segae_hangonjr_port_fa_w);

	state_save_register_global(port_fa_last);
}

static DRIVER_INIT( ridleofp )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0xf8, 0xf8, 0, 0, segae_ridleofp_port_f8_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0xfa, 0xfa, 0, 0, segae_ridleofp_port_fa_w);
}

static DRIVER_INIT( astrofl )
{
	astrofl_decode();
}

/*******************************************************************************
 Rom Loaders / Game Drivers
********************************************************************************
 Good Dumps:
 tetrisse  - Tetris (System E version)
 hangonjr - Hang On Jr.
 ridleofp - Riddle of Pythagoras (Jp.)
 transfrm - Transformer
 astrofl  - Astro Flash (Jp. Version of Transformer) (was encrypted)

 NOT DECRYPTED

 fantzn2  - Fantasy Zone 2 (set 2) *Rom at IC7 Encrypted*
 opaopa   - Opa Opa                *Roms Encrypted/Bad?*
*******************************************************************************/


ROM_START( tetrisse )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "epr12213.7",	0x00000, 0x08000, CRC(ef3c7a38) SHA1(cbb91aef330ab1a37d3e21ecf1d008143d0dd7ec) ) /* Fixed Code */

	/* The following are 8 0x4000 banks that get mapped to reads from 0x8000 - 0xbfff */
	ROM_LOAD( "epr12212.5",	0x10000, 0x08000, CRC(28b550bf) SHA1(445922a62e8a7360335c754ad70dabbe0208207b) )
	ROM_LOAD( "epr12211.4",	0x18000, 0x08000, CRC(5aa114e9) SHA1(f9fc7fe4d0444a264185e74d2abc8475f0976534) )
	/* ic3 unpopulated */
	/* ic2 unpopulated */
ROM_END

ROM_START( hangonjr )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "rom5.ic7",	0x00000, 0x08000, CRC(d63925a7) SHA1(699f222d9712fa42651c753fe75d7b60e016d3ad) ) /* Fixed Code */

	/* The following are 8 0x4000 banks that get mapped to reads from 0x8000 - 0xbfff */
	ROM_LOAD( "rom4.ic5",	0x10000, 0x08000, CRC(ee3caab3) SHA1(f583cf92c579d1ca235e8b300e256ba58a04dc90) )
	ROM_LOAD( "rom3.ic4",	0x18000, 0x08000, CRC(d2ba9bc9) SHA1(85cf2a801883bf69f78134fc4d5075134f47dc03) )
	ROM_LOAD( "rom2.ic3",	0x20000, 0x08000, CRC(e14da070) SHA1(f8781f65be5246a23c1f492905409775bbf82ea8) )
	ROM_LOAD( "rom1.ic2",	0x28000, 0x08000, CRC(3810cbf5) SHA1(c8d5032522c0c903ab3d138f62406a66e14a5c69) )
ROM_END

ROM_START( ridleofp )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "epr10426.bin",	0x00000, 0x08000, CRC(4404c7e7) SHA1(555f44786976a009d96a6395c9173929ad6138a7) ) /* Fixed Code */

	/* The following are 8 0x4000 banks that get mapped to reads from 0x8000 - 0xbfff */
	ROM_LOAD( "epr10425.bin",	0x10000, 0x08000, CRC(35964109) SHA1(a7bc64a87b23139b0edb9c3512f47dcf73feb854) )
	ROM_LOAD( "epr10424.bin",	0x18000, 0x08000, CRC(fcda1dfa) SHA1(b8497b04de28fc0d6b7cb0206ad50948cff07840) )
	ROM_LOAD( "epr10423.bin",	0x20000, 0x08000, CRC(0b87244f) SHA1(c88041614735a9b6cba1edde0a11ed413e115361) )
	ROM_LOAD( "epr10422.bin",	0x28000, 0x08000, CRC(14781e56) SHA1(f15d9d89e1ebff36c3867cfc8f0bdf7f6b3c96bc) )
ROM_END

ROM_START( transfrm )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "ic7.top",	0x00000, 0x08000, CRC(ccf1d123) SHA1(5ade9b00e2a36d034fafdf1902d47a9a00e96fc4) ) /* Fixed Code */

	/* The following are 8 0x4000 banks that get mapped to reads from 0x8000 - 0xbfff */
	ROM_LOAD( "epr-7347.ic5",	0x10000, 0x08000, CRC(df0f639f) SHA1(a09a9841b66de246a585be63d911b9a42a323503) )
	ROM_LOAD( "epr-7348.ic4",	0x18000, 0x08000, CRC(0f38ea96) SHA1(d4d421c5d93832e2bc1f22f39dffb6b80f2750bd) )
	ROM_LOAD( "ic3.top",		0x20000, 0x08000, CRC(9d485df6) SHA1(b25f04803c8f7188021f3039aa13aac80d480823) )
	ROM_LOAD( "epr-7350.ic2",	0x28000, 0x08000, CRC(0052165d) SHA1(cf4b5dffa54238e513515b3fc90faa7ce0b65d34) )
ROM_END

ROM_START( astrofl )
	ROM_REGION( 2*0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "epr-7723.ic7",	0x00000, 0x08000, CRC(66061137) SHA1(cb6a2c7864f9f87bbedfd4b1448ad6c2de65d6ca) ) /* encrypted */

	/* The following are 8 0x4000 banks that get mapped to reads from 0x8000 - 0xbfff */
	ROM_LOAD( "epr-7347.ic5",	0x10000, 0x08000, CRC(df0f639f) SHA1(a09a9841b66de246a585be63d911b9a42a323503) )
	ROM_LOAD( "epr-7348.ic4",	0x18000, 0x08000, CRC(0f38ea96) SHA1(d4d421c5d93832e2bc1f22f39dffb6b80f2750bd) )
	ROM_LOAD( "epr-7349.ic3",	0x20000, 0x08000, CRC(f8c352d5) SHA1(e59565ab6928c67706c6f82f6ea9a64cdfc65a21) )
	ROM_LOAD( "epr-7350.ic2",	0x28000, 0x08000, CRC(0052165d) SHA1(cf4b5dffa54238e513515b3fc90faa7ce0b65d34) )
ROM_END

/* Below are encrypted with an NEC MC-8123 processor... it is ESSENTIAL we find these in working condition
   AS SOON AS POSSIBLE, the batteries on these are dying at an ever increasing rate */

ROM_START( fantzn2 )
	ROM_REGION( 2*0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "epr-11416.ic7",	0x00000, 0x08000, CRC(76db7b7b) SHA1(d60e2961fc893dcb4445aed5f67515cbd25b610f) )
	ROM_LOAD( "epr-11415.ic5",	0x10000, 0x10000, CRC(57b45681) SHA1(1ae6d0d58352e246a4ec4e1ce02b0417257d5d20) )
	ROM_LOAD( "epr-11414.ic4",	0x20000, 0x10000, CRC(6f7a9f5f) SHA1(b53aa2eded781c80466a79b7d81383b9a875d0be) )
	ROM_LOAD( "epr-11413.ic3",	0x30000, 0x10000, CRC(a231dc85) SHA1(45b94fdbde28c02e88546178ef3e8f9f3a96ab86) )
	ROM_LOAD( "epr-11412.ic2",	0x40000, 0x10000, CRC(b14db5af) SHA1(04c7fb659385438b3d8f9fb66800eb7b6373bda9) )
ROM_END

ROM_START( opaopa )
	ROM_REGION( 2*0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "epr11224.ic7",	0x00000, 0x08000, CRC(024b1244) SHA1(59a522ac3d98982cc4ddb1c81f9584d3da453649) ) /* Fixed Code */

	/* The following are 8 0x4000 banks that get mapped to reads from 0x8000 - 0xbfff */
	ROM_LOAD( "epr11223.ic5",	0x10000, 0x08000, CRC(6bc41d6e) SHA1(8997a4ac2a9704f1400d0ec16b259ee496a7efef) )
	ROM_LOAD( "epr11222.ic4",	0x18000, 0x08000, CRC(395c1d0a) SHA1(1594bad13e78c5fad4db644cd85a6bac1eaddbad) )
	ROM_LOAD( "epr11221.ic3",	0x20000, 0x08000, CRC(4ca132a2) SHA1(cb4e4c01b6ab070eef37c0603190caafe6236ccd) )
	ROM_LOAD( "epr11220.ic2",	0x28000, 0x08000, CRC(a165e2ef) SHA1(498ff4c5d3a2658567393378c56be6ed86ac0384) )
ROM_END

/*-- Game Drivers --*/

GAME( 1985, hangonjr, 0,        segae, hangonjr, hangonjr, ROT0,  "Sega", "Hang-On Jr.", 0 )
GAME( 1986, transfrm, 0,        segae, transfrm, 0,        ROT0,  "Sega", "Transformer", 0 )
GAME( 1986, astrofl,  transfrm, segae, transfrm, astrofl,  ROT0,  "Sega", "Astro Flash (Japan)", GAME_IMPERFECT_GRAPHICS )
GAME( 1986, ridleofp, 0,        segae, ridleofp, ridleofp, ROT90, "Sega / Nasco", "Riddle of Pythagoras (Japan)", 0 )
GAME( 1988, fantzn2,  0,        segae, dummy,    0,        ROT0,  "Sega", "Fantasy Zone 2", GAME_NOT_WORKING )	/* encrypted */
GAME( 198?, opaopa,   0,        segae, dummy,    0,        ROT0,  "Sega", "Opa Opa", GAME_NOT_WORKING )	/* either encrypted or bad */
GAME( 1988, tetrisse, 0,        segae, tetrisse, 0,        ROT0,  "Sega", "Tetris (Japan, System E)", 0 )
