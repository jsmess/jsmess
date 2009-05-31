/******************************************************************************
*
*  V-tech Socrates Driver
*  By Jonathan Gevaryahu AKA Lord Nightmare
*  with dumping help from Kevin 'kevtris' Horton
*  
*  (driver structure copied from vtech1.c)
TODO: (practically everything!)
	hook up ram banking
	hook up rom banking
	fix layout to sane dimensions
	write renderer (composite ntsc fun!) and hook up scroll regs
	hook up cartridges using the bios system
	hook up sound regs
	find and hook up keyboard/mouse IR input
	find and hook up any timers/interrupt controls
	find and document ports for speech synthesizer
	


  Socrates Educational Video System
        FFFF|----------------|
            | RAM (window 1) |
            |                |
        C000|----------------|
            | RAM (window 0) |
            |                |
        8000|----------------|
            | ROM (banked)   |
            | *Cartridge     |
        4000|----------------|
            | ROM (fixed)    |
            |                |
        0000|----------------|

	* cartridge lives in banks 10 onward, see below

        Banked rom area (4000-7fff) bankswitching
        Bankswitching is achieved by writing to I/O port 0 (mirrored on 1-7)
	Bank 	   ROM_REGION        Contents
	0          0x00000 - 0x03fff System ROM page 0
	1          0x04000 - 0x07fff System ROM page 1
	2          0x08000 - 0x0bfff System ROM page 2
        ... etc ...
	E          0x38000 - 0x38fff System ROM page E
	F          0x3c000 - 0x3ffff System ROM page F
       10          0x40000 - 0x43fff Expansion Cartridge page 0 (cart ROM 0x0000-0x3fff)
       11          0x44000 - 0x47fff Expansion Cartridge page 1 (cart ROM 0x4000-0x7fff)
        ... etc ...

        Banked ram area (z80 0x8000-0xbfff window 0 and z80 0xc000-0xffff window 1)
        Bankswitching is achieved by writing to I/O port 8 (mirrored to 9-F), only low nybble
        byte written: 0b****BBAA
        where BB controls ram window 1 and AA controls ram window 0
        hence:
        Write    [window 0]         [window 1]
        0        0x0000-0x3fff      0x0000-0x3fff
        1        0x4000-0x7fff      0x0000-0x3fff
        2        0x8000-0xbfff      0x0000-0x3fff
        3        0xc000-0xffff      0x0000-0x3fff
        4        0x0000-0x3fff      0x4000-0x7fff
        5        0x4000-0x7fff      0x4000-0x7fff
        6        0x8000-0xbfff      0x4000-0x7fff
        7        0xc000-0xffff      0x4000-0x7fff
        8        0x0000-0x3fff      0x8000-0xbfff
        9        0x4000-0x7fff      0x8000-0xbfff
        A        0x8000-0xbfff      0x8000-0xbfff
        B        0xc000-0xffff      0x8000-0xbfff
        C        0x0000-0x3fff      0xc000-0xffff
        D        0x4000-0x7fff      0xc000-0xffff
        E        0x8000-0xbfff      0xc000-0xffff
        F        0xc000-0xffff      0xc000-0xffff

video output:
1. read byte at address in vram
2. compare the lower nybble >=8 or not
3. if 2 is false, go to 6; if it is true, go to 4
4. retrieve the value from ram address 0xFNLL where N is the nybble and LL is the low 8 bits of the current display line offset.
5. take this newly retrieved byte and shift the bits out to video in the order 4 5 6 7 0 1 2 3 at the 2048-pixels-per-line rate. go to 7.
6. take the retrieved byte and shift the bits out to video in the order 0 1 2 3 at the 1024-pixels-per-line rate.
7. compare the upper nybble >= 8 or not
8. if 7 is false, go to 11; if it is true, go to 9
9. retrieve the value from ram address 0xFNLL where N is the nybble and LL is the low 8 bits of the current display line offset.
10. take this newly retrieved byte and shift the bits out to video in the order 4 5 6 7 0 1 2 3 at the 2048-pixels-per-line rate. go to 12.
11. take the retrieved byte and shift the bits out to video in the order 4 5 6 7 at the 1024-pixels-per-line rate.
12. are we in hblank? if we are not in hblank or hblank is now over, increment the vram address and go to 1. otherwise, just go to 1.

Yes, the above means that all colors, shades, etc produced by video are entirely composite ntsc artifacts.

******************************************************************************/

/* Core includes */
#include "driver.h"
#include "cpu/z80/z80.h"
#include "socrates.lh"

/* Components */

/* Devices */


/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(z80_mem, ADDRESS_SPACE_PROGRAM, 8)
    ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE(0x0000, 0x3fff) AM_ROM /* system rom, bank 0 (fixed) */
    AM_RANGE(0x4000, 0x7fff) AM_NOP /* banked rom space; system rom is banks 0 thru F, cartridge rom is banks 10 onward, usually banks 10 thru 17. area past the end of the cartridge, and the whole 10-ff area when no cartridge is inserted, reads as 0xF3 */
    AM_RANGE(0x8000, 0xbfff) AM_RAM /* banked ram 'window' 0 */
    AM_RANGE(0xc000, 0xffff) AM_RAM /* banked ram 'window' 1 */
ADDRESS_MAP_END

static ADDRESS_MAP_START(z80_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_RAM AM_MIRROR(0x7) /* rom bank select - RW - 8 bits */
	AM_RANGE(0x08, 0x08) AM_RAM AM_MIRROR(0x7) /* ram banks select - RW - 4 low bits; Format: 0b****HHLL where LL controls whether window 0 points at ram area: 0b00: 0x0000-0x3fff; 0b01: 0x4000-0x7fff; 0b10: 0x8000-0xbfff; 0b11: 0xc000-0xffff. HH controls the same thing for window 1 */
	AM_RANGE(0x10, 0x17) AM_NOP AM_MIRROR (0x8) /* sound section:
        0x10 - W - frequency control for channel 1 (louder channel) - 01=high pitch, ff=low, write 0 to silence
	0x11 - W - frequency control for channel 2 (softer channel) - 01=high pitch, ff=low, write 0 to silence
	0x12 - volume control for channel 1
	0x13 - volume control for channel 2
	0x14-0x17 - ?DAC? related? makes noise when written to...
	*/
	AM_RANGE(0x20, 0x21) AM_NOP AM_MIRROR (0xe) /* graphics section:
	0x20 - W - lsb offset of screen display
	0x21 - W - msb offset of screen display
	resulting screen line is one of 512 total offsets on 128-byte boundaries in the whole 64k ram
	*/
	AM_RANGE(0x30, 0x30) AM_NOP /* unknown, write only */
	AM_RANGE(0x40, 0x40) AM_RAM AM_MIRROR(0xF)/* unknown, read and write low 4 bits plus bit 5, bit 7 seems to be fixed at 0, bit 6 and 4 are fixed at 1? is this some sort of control register for timers perhaps? */
	AM_RANGE(0x50, 0x50) AM_RAM AM_MIRROR(0xE) /* unknown, read and write (bits?) */
	AM_RANGE(0x51, 0x51) AM_RAM AM_MIRROR(0xE) /* unknown, read and write (bits?) */
	AM_RANGE(0x60, 0x60) AM_NOP AM_MIRROR(0xF) /* unknown, write only  */
ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/



/******************************************************************************
 Machine Drivers
******************************************************************************/

static MACHINE_DRIVER_START(socrates)
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, XTAL_21_4772MHz/6)  /* Toshiba TMPZ84C00AP @ 3.579545 MHz, verified, xtal is divided by 6 */
    MDRV_CPU_PROGRAM_MAP(z80_mem)
    MDRV_CPU_IO_MAP(z80_io)
    MDRV_QUANTUM_TIME(HZ(60))

    /* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_socrates)

    /* sound hardware */
	//MDRV_SPEAKER_STANDARD_MONO("mono")
	//MDRV_SOUND_ADD("soc_snd", SOCRATES, XTAL_21_47727MHz/12) /* divider guessed */
	//MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)


MACHINE_DRIVER_END



/******************************************************************************
 ROM Definitions
******************************************************************************/

ROM_START(socrates)
    ROM_REGION(0x80000, "maincpu", 0)
    /* Socrates US NTSC */
    ROM_LOAD("27-00817-000-000.u1", 0x00000, 0x40000, CRC(80f5aa20) SHA1(4fd1ff7f78b5dd2582d5de6f30633e4e4f34ca8f)) 
ROM_END
    
ROM_START(socratfc)
    ROM_REGION(0x80000, "maincpu", 0)
    /* Socrates SAITOUT (French Canadian) NTSC */
    ROM_LOAD("socratfc.u1", 0x00000, 0x40000, CRC(042d9d21) SHA1(9ffc67b2721683b2536727d0592798fbc4d061cb)) /* fix label/name */
ROM_END



/******************************************************************************
 System Config
******************************************************************************/
static SYSTEM_CONFIG_START(socrates)
// write me!
SYSTEM_CONFIG_END


/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT CONFIG      COMPANY                     FULLNAME                            FLAGS */
COMP( 1988, socrates,   0,          0,      socrates,   0, 0,   socrates,   "V-tech",        "Socrates Educational Video System",                        GAME_NOT_WORKING )
COMP( 1988, socratfc,   socrates,   0,      socrates,   0, 0,   socrates,   "V-tech",        "Socrates SAITOUT",                        GAME_NOT_WORKING )
