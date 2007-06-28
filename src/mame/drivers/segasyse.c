/* THIS DRIVER IS OBSOLETE, BUT A FEW OTHERS STILL DEPEND ON VARIOUS BITS
 once the megatech / megaplay etc. drivers have been rewritten this should
 be removed.

 System E emulation is now in src\mame\drivers\segae.c
*/

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

Sega System E Hardware Overview
Sega, 1985-1988

This PCB is essentially a Sega Master System home console unit, but using
two '315-5124' VDPs and extra RAM.
The CPU is located on a plug-in board that also holds all of the EPROMs.

The games that run on this hardware include....
Hang-On Jr.             1985
Transformer/Astro Flash 1986
Riddle of Pythagoras    1986
Opa Opa                 1987
Fantasy Zone 2          1988
Tetris                  1988

PCB Layout
----------
834-5803 (C)Sega 1985
|-----------------------------------------------------|
|         D4168      D4168      D4168       D4168     |
|                                                     |
|         D4168      D4168      D4168       D4168     |
|                                                     |
|                                                     |
|    SW1                                              |
|CN1                                                  |
|    SW2                                              |
|                                                     |
|   LED            |---|             |---|            |
|                  | 3 |             | 3 |            |
|                  | 1 |             | 1 |            |
|                  | 5 |             | 5 |            |
|                  | | |             | | |         CN3|
|                  | 5 |             | 5 |     8255   |
|CN4               | 1 |             | 1 |            |
|                  | 2 |             | 2 |            |
|                  | 4 |             | 4 |            |
|                  |---|             |---|            |
|               |--------ROM-BOARD-(above)---------|  |
|               |                                  |  |
|               |CN2                   10.7386MHz  |  |
|               |         D4168                    |  |
|  VOL          |         D4168                    |  |
| LA4460        |----------------------------------|  |
|-----------------------------------------------------|
Notes:
      315-5124 VDP clock - 10.7386MHz
      SN76496 clock      - 3.579533MHz [10.7386/3]
      D4168              - 8k x8 SRAM
      VSync              - 60Hz
      HSync              - 15.58kHz
      CN1                - Connector used for standard controls
      CN2                - connector for CPU/ROM PCB
      CN3                - Connector used for special controls (via a small plug-in interface PCB)
      CN4                - Connector for power

ROM Daughterboard
-----------------
834-6592-01
|--------------------------------------------|
|                                            |
|    |---|                                   |
|C   |   |                           IC6     |
|N   |Z80|                                   |
|2   |   |                                   |
|    |   |   IC2   IC3   IC4   IC5        IC7|
|    |---|                                   |
|     IC1             PAD1 PAD2     PAD3 PAD4|
|--------------------------------------------|
Notes:
       IC1: Z80 clock - 5.3693MHz [10.7386/2]
            On some games this is replaced with a NEC MC-8123 Encrypted CPU Module.
            The clock speed is the same. The MC-8123 contains a Z80 core, plus a RAM chip
            and battery. When the battery dies, the program can no longer be decrypted
            and the PCB does not boot up at all. The battery can not be changed because the
            MC-8123 is sealed, so there is no way to access it.

 IC2 - IC5: 27C256 EPROM (DIP28)

       IC6: 74LS139

       IC7: 27C256 EPROM (DIP28)

    PAD1-4: These are jumper pads used to configure the ROM board for use with the
            Z80 or with the MC8123 CPU.
            PAD1 - Ties Z80 pin 24 (WAIT) to pin1 of the EPROMs at IC2, 3, 4 & 5
            PAD2 - Ties CN2 pin B21 to pin1 of the EPROMs at IC2, 3, 4 & 5
            PAD3 - Ties CN2 pin B21 to pin 2 of the 74LS139 @ IC6
            PAD4 - Ties Z80 pin 24 (WAIT) to pin 2 of the 74LS139 @ IC6

            The pads are configured like this..... (U=Upper, L=Lower)

                                                 |----|      |----|
                                                 |IC6 |      |IC7 |
                                                 |  12|------|22  |
                                                 |    |      |    |
                       IC2   IC3    IC4   IC5    |   1|------|27  |
                       PIN1  PIN1   PIN1  PIN1   |   2|--|   |    |
                        O-----O--+---O------O    |----|  |   |----|
                                 |                       |
                                 |         |----|        |
                              O--+----O    |    O    |---O
            CN2    Z80      PAD1U   PAD2U  |  PAD3U  | PAD4U
            B21    PIN24    PAD1L   PAD2L  |  PAD3L  | PAD4L
             O       O--4.7k--O       O----|    O----|   O
             |                |       |                  |
             |                |-------|------------------|
             |                        |
             |------------------------|

            When using a regular Z80B (and thus, unencrypted code):
            PAD1 - Open
            PAD2 - Open
            PAD3 - Shorted
            PAD4 - Open

            When using an encrypted CPU module (MC-8123):
            PAD1 - Open
            PAD2 - Shorted
            PAD3 - Open
            PAD4 - Open
            Additionally, a wire must be tied from CN2 pin B22 to the side
            of PAD3 nearest IC6 (i.e. PAD3U).

ROMs:
-----

Game                     IC2         IC3         IC4         IC5         IC7
---------------------------------------------------------------------------------
Hang-On Jr.              EPR-?       EPR-?       EPR-?       EPR-?       EPR-?     Hello, Sega Part Numbers....!?
Transformer              EPR-7350    EPR-?       EPR-7348    EPR-7347    EPR-?     Ditto
           /Astro Flash  EPR-7350    EPR-7349    EPR-7348    EPR-7347    EPR-7723
Riddle of Pythagoras     EPR-10422   EPR-10423   EPR-10424   EPR-10425   EPR-10426
Opa Opa                  EPR-11220   EPR-11221   EPR-11222   EPR-11223   EPR-11224
Fantasy Zone 2           EPR-11412   EPR-11413   EPR-11414   EPR-11415   EPR-11416
Tetris                   -           -           EPR-12211   EPR-12212   EPR-12213

A System E PCB can run all of the games simply by swapping the EPROMs plus CPU.
Well, in theory anyway. To run the not-encrypted games, just swap EPROMs and they will work.

To run the encrypted games, use a double sized EPROM in IC7 (i.e. a 27C512)
and program the decrypted opcodes to the lower half and the decrypted data to the upper half,
then connect the highest address pin of the EPROM (A15 pin 1) to the M1 pin on the Z80.
This method has been tested and does not actually work. An update on this may follow....


System E PCB Pinout
-------------------

CN1
---

+12V             1A  1B  Coin switch 1
Coin switch 2    2A  2B  Test switch
Service switch   3A  3B
                 4A  4B  1P start
2P start         5A  5B  1P up
1P down          6A  6B  1P left
1P right         7A  7B  1P button 1
1P button 2      8A  8B
                 9A  9B  2P up
2P down          10A 10B 2P left
2P RIGHT         11A 11B 2P button 1
2P button 2      12A 12B
                 13A 13B Video RED
                 14A 14B Video GREEN
                 15A 15B Video BLUE
                 16A 16B Video SYNC
                 17A 17B
                 18A 18B
Speaker [+]      19A 19B
Speaker [-]      20A 20B
Coin counter GND 21A 21B
GND              22A 22B Coin counter 1
                 23A 23B Coin counter 2
                 24A 24B
                 25A 25B
CN4
---

+5V  1A 1B +5V
+5V  2A 2B +5V
     3A 3B
GND  4A 4B GND
GND  5A 5B GND
+12V 6A 6B +12V
+12V 7A 7B +12V
GND  8A 8B GND

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
#include "machine/mc8123.h"
#include "sound/sn76496.h"

/*-- Variables --*/

static UINT8 segae_8000bank;	/* Current VDP Bank Selected for 0x8000 - 0xbfff writes */
static UINT8 port_fa_last;		/* Last thing written to port 0xfa (control related) */
static UINT8 hintcount;			/* line interrupt counter, decreased each scanline */
UINT8 segae_vintpending;		/* vertical interrupt pending flag */
UINT8 segae_hintpending;		/* scanline interrupt pending flag */

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

UINT8 segae_vdp_ctrl_r ( UINT8 chip );
UINT8 segae_vdp_data_r ( UINT8 chip );
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
  3 - \
  2 - |
  1 - | Rom Bank Select for 0x8000 -
  0 - |                 0xbfff reads

***************************************/

static UINT8 rombank;

void segae_bankswitch (void)
{
	memory_set_bank(1, rombank);
}


static WRITE8_HANDLER (segae_port_f7_w)
{
	segae_vdp_vrambank[0] = (data & 0x80) >> 7; /* Back  Layer VDP (0) VRAM Bank */
	segae_vdp_vrambank[1] = (data & 0x40) >> 6; /* Front Layer VDP (1) VRAM Bank */
	segae_8000bank		  = (data & 0x20) >> 5; /* 0x8000 Write Select */
	rombank				  =  data & 0x0f;		/* Rom Banking */

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
			segae_vintpending = 1;

		if (hintcount == 0) {
			hintcount = segae_vdp_regs[1][10];
			segae_hintpending = 1;

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

		if ( (sline<0xe0) && (segae_vintpending) ) {
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
	memory_configure_bank(1, 0, 16, memory_region(REGION_CPU1) + 0x10000, 0x4000);

	state_save_register_global(segae_8000bank);
	state_save_register_global(segae_vintpending);
	state_save_register_global(segae_hintpending);
	state_save_register_global(rombank);
	state_save_register_func_postload(segae_bankswitch);
}

