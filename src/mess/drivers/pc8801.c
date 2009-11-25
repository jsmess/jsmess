/*****************************************************************************************************

    NEC PC-8801 MESS Emulation

    PC-88xx Models (and similar machines like PC-80xx and PC-98DO)

    Model            | release |      CPU     |                      BIOS components                        |       |
                     |         |     clock    | N-BASIC | N88-BASIC | N88-BASIC Enh |  Sound  |  CD |  Dict |  Disk | Notes
    ==================================================================================================================================
    PC-8001          | 1979-03 |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   |
    PC-8001A         |   ??    |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   | (U)
    PC-8801          | 1981-11 |   z80A @ 4   |    X    |     X     |       -       |    -    |  -  |   -   |   -   | (KO)
    PC-8801A         |   ??    |   z80A @ 4   |    X    |     X     |       -       |    -    |  -  |   -   |   -   | (U)
    PC-8001 mkII     | 1983-03 |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   | (GE),(KO)
    PC-8001 mkIIA    |   ??    |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   | (U),(GE)
    PC-8801 mkII     | 1983-11 |   z80A @ 4   |    X    |     X     |       -       |    -    |  -  |   -   | (FDM) | (K1)
    PC-8001 mkII SR  | 1985-01 |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   | (GE),(NE),(KO)
    PC-8801 mkII SR  | 1985-03 |   z80A @ 4   |    X    |     X     |       X       |    X    |  -  |   -   | (FDM) | (K1)
    PC-8801 mkII TR  | 1985-10 |   z80A @ 4   |    X    |     X     |       X       |    X    |  -  |   -   | (FD2) | (K1)
    PC-8801 mkII FR  | 1985-11 |   z80A @ 4   |    X    |     X     |       X       |    X    |  -  |   -   | (FDM) | (K1)
    PC-8801 mkII MR  | 1985-11 |   z80A @ 4   |    X    |     X     |       X       |    X    |  -  |   -   | (FDH) | (K2)
    PC-8801 FH       | 1986-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FDM) | (K2)
    PC-8801 MH       | 1986-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FDH) | (K2)
    PC-88 VA         | 1987-03 | z80H+v30 @ 8 |    -    |     X     |       X       |    X    |  -  |   X   | (FDH) | (K2)
    PC-8801 FA       | 1987-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FD2) | (K2)
    PC-8801 MA       | 1987-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   X   | (FDH) | (K2)
    PC-88 VA2        | 1988-03 | z80H+v30 @ 8 |    -    |     X     |       X       |    X    |  -  |   X   | (FDH) | (K2)
    PC-88 VA3        | 1988-03 | z80H+v30 @ 8 |    -    |     X     |       X       |    X    |  -  |   X   | (FD3) | (K2)
    PC-8801 FE       | 1988-10 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FD2) | (K2)
    PC-8801 MA2      | 1988-10 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   X   | (FDH) | (K2)
    PC-98 DO         | 1989-06 |   z80H @ 8   |    X    |     X     |       X       |    X    |  -  |   -   | (FDH) | (KE)
    PC-8801 FE2      | 1989-10 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FD2) | (K2)
    PC-8801 MC       | 1989-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  X  |   X   | (FDH) | (K2)
    PC-98 DO+        | 1990-10 |   z80H @ 8   |    X    |     X     |       X       |    X    |  -  |   -   | (FDH) | (KE)

    info for PC-98 DO & DO+ refers to their 88-mode

    Disk Drive options:
    (FDM): there exist three model of this computer: Model 10 (base model, only optional floppy drive), Model 20
        (1 floppy drive for 5.25" 2D disks) and Model 30 (2 floppy drive for 5.25" 2D disks)
    (FD2): 2 floppy drive for 5.25" 2D disks
    (FDH): 2 floppy drive for both 5.25" 2D disks and 5.25" HD disks
    (FD3): 2 floppy drive for both 5.25" 2D disks and 5.25" HD disks + 1 floppy drive for 3.5" 2TD disks

    Notes:
    (U): US version
    (GE): Graphic Expansion for PC-8001
    (NE): N-BASIC Expansion for PC-8001 (similar to N88-BASIC Expansion for PC-88xx)
    (KO): Optional Kanji ROM
    (K1): Kanji 1st Level ROM
    (K2): Kanji 2nd Level ROM
    (KE): Kanji Enhanced ROM

    Memory mounting locations:
     * N-BASIC 0x0000 - 0x5fff, N-BASIC Expansion & Graph Enhhancement 0x6000 - 0x7fff
     * N-BASIC 0x0000 - 0x5fff, N-BASIC Expansion & Graph Enhhancement 0x6000 - 0x7fff
     * N88-BASIC 0x0000 - 0x7fff, N88-BASIC Expansion & Graph Enhhancement 0x6000 - 0x7fff
     * Sound BIOS: 0x6000 - 0x7fff
     * CD-ROM BISO: 0x0000 - 0x7fff
     * Dictionary: 0xc000 - 0xffff (32 Banks)

    info from http://www.geocities.jp/retro_zzz/machines/nec/cmn_roms.html
    also, refer to http://www.geocities.jp/retro_zzz/machines/nec/cmn_vers.html for
        info about BASIC revisions in the various models (BASIC V2 is the BASIC
        Expansion, if I unerstood correctly)

    TODO:
        - Shouldn't it be possible to switch resolution during emulation, like many MAME
        games do? If yes, there would be no need of separate LoRes and HiRes drivers.
        - Add correct CPU to various computers (right now they all use the MkIISR settings)
        - Dump the Sound BIOS
        - We need to support no Expanded BASIC machines (early models). Right now emulation
        always start from BASIC V2, so machines without it only shows a black screen
        - We need to emulate the differences in the new sets (MkIIFR, MA, MA2, MC) compared
        to MkIISR.

    Usage:
        These machines take a long time to start. Be patient (or press Insert to FastForward)



*****************************************************************************************************/

/*
	
	TODO:

	- fix floppy
	- rewrite memory banking
	- rewrite extended memory handling
	- remove MEM port
	- remove CFG port

*/

#include "driver.h"
#include "includes/pc8801.h"
#include "cpu/z80/z80.h"
#include "cpu/v30mz/nec.h"
#include "devices/cassette.h"
#include "devices/flopdrv.h"
#include "machine/ctronics.h"
#include "machine/i8255a.h"
#include "machine/upd1990a.h"
#include "machine/upd765.h"
#include "sound/2203intf.h"
#include "sound/beep.h"

/* Memory Maps */

static ADDRESS_MAP_START( pc8801_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_RAMBANK(1)
	AM_RANGE(0x6000, 0x7fff) AM_RAMBANK(2)
	AM_RANGE(0x8000, 0x83ff) AM_RAMBANK(3)
	AM_RANGE(0x8400, 0xbfff) AM_RAMBANK(4)
	AM_RANGE(0xc000, 0xefff) AM_RAMBANK(5)
	AM_RANGE(0xf000, 0xffff) AM_RAMBANK(6)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc88sr_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x00) AM_READ_PORT("KEY0")
	AM_RANGE(0x01, 0x01) AM_READ_PORT("KEY1")
	AM_RANGE(0x02, 0x02) AM_READ_PORT("KEY2")
	AM_RANGE(0x03, 0x03) AM_READ_PORT("KEY3")
	AM_RANGE(0x04, 0x04) AM_READ_PORT("KEY4")
	AM_RANGE(0x05, 0x05) AM_READ_PORT("KEY5")
	AM_RANGE(0x06, 0x06) AM_READ_PORT("KEY6")
	AM_RANGE(0x07, 0x07) AM_READ_PORT("KEY7")
	AM_RANGE(0x08, 0x08) AM_READ_PORT("KEY8")
	AM_RANGE(0x09, 0x09) AM_READ_PORT("KEY9")
	AM_RANGE(0x0a, 0x0a) AM_READ_PORT("KEY10")
	AM_RANGE(0x0b, 0x0b) AM_READ_PORT("KEY11")
	AM_RANGE(0x0c, 0x0c) AM_READ_PORT("KEY12")
	AM_RANGE(0x0d, 0x0d) AM_READ_PORT("KEY13")
	AM_RANGE(0x0e, 0x0e) AM_READ_PORT("KEY14")
	AM_RANGE(0x0f, 0x0f) AM_READ_PORT("KEY15")
	AM_RANGE(0x10, 0x10) AM_WRITE(pc88_rtc_w)
//	AM_RANGE(0x20, 0x21) AM_NOP										/* RS-232C and cassette (not yet) */
	AM_RANGE(0x30, 0x30) AM_READ_PORT("DSW1") AM_WRITE(pc88sr_outport_30)
	AM_RANGE(0x31, 0x31) AM_READWRITE(pc88sr_inport_31, pc88sr_outport_31)	/* DIP-SW2 */
	AM_RANGE(0x32, 0x32) AM_READWRITE(pc88sr_inport_32, pc88sr_outport_32)
	AM_RANGE(0x34, 0x35) AM_WRITE(pc88sr_alu)
	AM_RANGE(0x40, 0x40) AM_READWRITE(pc88sr_inport_40, pc88sr_outport_40)
	AM_RANGE(0x44, 0x45) AM_DEVREAD(YM2203_TAG, ym2203_r)
//	AM_RANGE(0x46, 0x47) AM_NOP										/* OPNA extra port (not yet) */
	AM_RANGE(0x50, 0x51) AM_READWRITE(pc88_crtc_r, pc88_crtc_w)
	AM_RANGE(0x52, 0x5b) AM_WRITE(pc88_palette_w)
	AM_RANGE(0x5c, 0x5c) AM_READ(pc88_vramtest_r)
	AM_RANGE(0x5c, 0x5f) AM_WRITE(pc88_vramsel_w)
	AM_RANGE(0x60, 0x68) AM_READWRITE(pc88_dmac_r, pc88_dmac_w)
//	AM_RANGE(0x6e, 0x6e) AM_NOP										/* CPU clock info (not yet) */
//	AM_RANGE(0x6f, 0x6f) AM_NOP										/* RS-232C speed ctrl (not yet) */
	AM_RANGE(0x70, 0x70) AM_READWRITE(pc8801_inport_70, pc8801_outport_70)
	AM_RANGE(0x71, 0x71) AM_READWRITE(pc88sr_inport_71, pc88sr_outport_71)
	AM_RANGE(0x78, 0x78) AM_WRITE(pc8801_outport_78)				/* text window increment */
//	AM_RANGE(0x90, 0x9f) AM_NOP 									/* CD-ROM (unknown -- not yet) */
//	AM_RANGE(0xa0, 0xa3) AM_NOP 									/* music & network (unknown -- not yet) */
//	AM_RANGE(0xa8, 0xad) AM_NOP										/* second sound board (not yet) */
//	AM_RANGE(0xb4, 0xb5) AM_NOP 									/* Video art board (unknown -- not yet) */
//	AM_RANGE(0xc1, 0xc1) AM_NOP										/* (unknown -- not yet) */
//	AM_RANGE(0xc2, 0xcf) AM_NOP 									/* music (unknown -- not yet) */
//	AM_RANGE(0xd0, 0xd7) AM_NOP 									/* music & GP-IB (unknown -- not yet) */
//	AM_RANGE(0xd8, 0xd8) AM_NOP 									/* GP-IB (unknown -- not yet) */
//	AM_RANGE(0xdc, 0xdf) AM_NOP 									/* MODEM (unknown -- not yet) */
	AM_RANGE(0xe2, 0xe3) AM_READWRITE(pc88_extmem_r, pc88_extmem_w) /* expand RAM select */
	AM_RANGE(0xe4, 0xe4) AM_WRITE(pc8801_write_interrupt_level)
	AM_RANGE(0xe6, 0xe6) AM_WRITE(pc8801_write_interrupt_mask)
//	AM_RANGE(0xe7, 0xe7) AM_NOP 									/* (unknown -- not yet) */
	AM_RANGE(0xe8, 0xeb) AM_READWRITE(pc88_kanji_r, pc88_kanji_w)
	AM_RANGE(0xec, 0xed) AM_READWRITE(pc88_kanji2_r, pc88_kanji2_w) /* JIS level2 Kanji ROM */
//	AM_RANGE(0xf0, 0xf1) AM_NOP 									/* Kana to Kanji dictionary ROM select (not yet) */
//	AM_RANGE(0xf3, 0xf3) AM_NOP 									/* DMA floppy (unknown -- not yet) */
//	AM_RANGE(0xf4, 0xf7) AM_NOP										/* DMA 5'floppy (may be not released) */
//	AM_RANGE(0xf8, 0xfb) AM_NOP 									/* DMA 8'floppy (unknown -- not yet) */
	AM_RANGE(0xfc, 0xff) AM_DEVREADWRITE(CPU_I8255A_TAG, i8255a_r, i8255a_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc88va_mem, ADDRESS_SPACE_PROGRAM, 8 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc88va_io, ADDRESS_SPACE_IO, 8 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc88va_v30_mem, ADDRESS_SPACE_PROGRAM, 8 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8801fd_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x4000, 0x7fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8801fd_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf8, 0xf8) AM_READ(pc8801fd_upd765_tc)
	AM_RANGE(0xfa, 0xfa) AM_DEVREAD(UPD765_TAG, upd765_status_r )
	AM_RANGE(0xfb, 0xfb) AM_DEVREADWRITE(UPD765_TAG, upd765_data_r, upd765_data_w )
	AM_RANGE(0xfc, 0xff) AM_DEVREADWRITE(FDC_I8255A_TAG, i8255a_r, i8255a_w )
ADDRESS_MAP_END

/* Input Ports */

/* 2008-05 FP:
Small note about the strange default mapping of function keys:
the top line of keys in PC8801 keyboard is as follows
[STOP][COPY]      [F1][F2][F3][F4][F5]      [ROLL UP][ROLL DOWN]
Therefore, in Full Emulation mode, "F1" goes to 'F3' and so on

Also, the Keypad has 16 keys, making impossible to map it in a satisfactory
way to a PC keypad. Therefore, default settings for these keys in Full
Emulation are currently based on the effect of the key rather than on
their real position

About natural keyboards: currently,
- "Keypad =" and "Keypad ," are not mapped
- "Stop" is mapped to 'Pause'
- "Copy" is mapped to 'Print Screen'
- "Kana" is mapped to 'F6'
- "Grph" is mapped to 'F7'
- "Roll Up" and "Roll Down" are mapped to 'Page Up' and 'Page Down'
- "Help" is mapped to 'F8'
 */

static INPUT_PORTS_START( pc8001 )
	PORT_START("KEY0")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)		PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)		PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)		PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(7_PAD))

	PORT_START("KEY1")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ASTERISK) 	PORT_CHAR(UCHAR_MAMEKEY(ASTERISK))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_PLUS_PAD) 	PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad =") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad ,") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD) 	PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)

	PORT_START("KEY2")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) 	PORT_CHAR('@')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G')

	PORT_START("KEY3")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) 			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) 			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("KEY4")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')

	PORT_START("KEY5")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('\xA5') PORT_CHAR('|')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('^')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-') PORT_CHAR('=')

	PORT_START("KEY6")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) 			PORT_CHAR('0')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('\'')

	PORT_START("KEY7")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) 		PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) 		PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) 		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) 		PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) 		PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("  _") PORT_CODE(KEYCODE_DEL)			PORT_CHAR(0) PORT_CHAR('_')

	PORT_START("KEY8")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Clr Home") PORT_CODE(KEYCODE_HOME)		PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP)	PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT)	PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del Ins") PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(UCHAR_MAMEKEY(DEL)) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Grph") PORT_CODE(KEYCODE_LALT)	PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Kana") PORT_CODE(KEYCODE_LCONTROL) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RCONTROL)						PORT_CHAR(UCHAR_SHIFT_2)

	PORT_START("KEY9")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Stop") PORT_CODE(KEYCODE_F1)			PORT_CHAR(UCHAR_MAMEKEY(PAUSE))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F3)								PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F4)								PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F5)								PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F6)								PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F7)								PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)							PORT_CHAR(' ')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC)								PORT_CHAR(UCHAR_MAMEKEY(ESC))

	PORT_START("KEY10")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)								PORT_CHAR('\t')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN)	PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT)	PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Help") PORT_CODE(KEYCODE_END)			PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Copy") PORT_CODE(KEYCODE_F2)			PORT_CHAR(UCHAR_MAMEKEY(PRTSCR))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS_PAD)						PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH_PAD)						PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))

	PORT_START("KEY11")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Roll Up") PORT_CODE(KEYCODE_F8)			PORT_CHAR(UCHAR_MAMEKEY(PGUP))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Roll Down") PORT_CODE(KEYCODE_F9)		PORT_CHAR(UCHAR_MAMEKEY(PGDN))
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY12")		/* port 0x0c */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY13")		/* port 0x0d */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY14")		/* port 0x0e */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY15")		/* port 0x0f */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x00, "BASIC" )
	PORT_DIPSETTING(    0x01, "N88-BASIC" )
	PORT_DIPSETTING(    0x00, "N-BASIC" )
	PORT_DIPNAME( 0x02, 0x02, "Terminal mode" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Text width" )
	PORT_DIPSETTING(    0x04, "40 chars/line" )
	PORT_DIPSETTING(    0x00, "80 chars/line" )
	PORT_DIPNAME( 0x08, 0x00, "Text height" )
	PORT_DIPSETTING(    0x08, "20 lines/screen" )
	PORT_DIPSETTING(    0x00, "25 lines/screen" )
	PORT_DIPNAME( 0x10, 0x10, "Enable S parameter" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Enable DEL code" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Memory wait" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Disable CMD SING" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "Parity generate" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Parity type" )
	PORT_DIPSETTING(    0x00, "Even" )
	PORT_DIPSETTING(    0x02, "Odd" )
	PORT_DIPNAME( 0x04, 0x00, "Serial character length" )
	PORT_DIPSETTING(    0x04, "7 bits/char" )
	PORT_DIPSETTING(    0x00, "8 bits/char" )
	PORT_DIPNAME( 0x08, 0x08, "Stop bit length" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x10, 0x10, "Enable X parameter" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Duplex" )
	PORT_DIPSETTING(    0x20, "Half" )
	PORT_DIPSETTING(    0x00, "Full" )
	PORT_DIPNAME( 0x40, 0x00, "Boot from floppy" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Disable floppy" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("CFG")		/* EXSWITCH */
	PORT_DIPNAME( 0x03, 0x01, "Basic mode" )
	PORT_DIPSETTING(    0x02, "N-BASIC" )
	PORT_DIPSETTING(    0x03, "N88-BASIC (V1)" )
	PORT_DIPSETTING(    0x01, "N88-BASIC (V2)" )
	PORT_DIPNAME( 0x04, 0x04, "Speed mode" )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x04, DEF_STR( High ) )
	PORT_DIPNAME( 0x08, 0x00, "Main CPU clock" )
	PORT_DIPSETTING(    0x00, "4MHz" )
	PORT_DIPSETTING(    0x08, "8MHz" )
	PORT_DIPNAME( 0xf0, 0x80, "Serial speed" )
	PORT_DIPSETTING(    0x10, "75bps" )
	PORT_DIPSETTING(    0x20, "150bps" )
	PORT_DIPSETTING(    0x30, "300bps" )
	PORT_DIPSETTING(    0x40, "600bps" )
	PORT_DIPSETTING(    0x50, "1200bps" )
	PORT_DIPSETTING(    0x60, "2400bps" )
	PORT_DIPSETTING(    0x70, "4800bps" )
	PORT_DIPSETTING(    0x80, "9600bps" )
	PORT_DIPSETTING(    0x90, "19200bps" )

	PORT_START("MEM")
	PORT_CONFNAME( 0x1f, 0x00, "Extension memory" )
	PORT_CONFSETTING(    0x00, DEF_STR( None ) )
	PORT_CONFSETTING(    0x01, "32KB (PC-8012-02 x 1)" )
	PORT_CONFSETTING(    0x02, "64KB (PC-8012-02 x 2)" )
	PORT_CONFSETTING(    0x03, "128KB (PC-8012-02 x 4)" )
	PORT_CONFSETTING(    0x04, "128KB (PC-8801-02N x 1)" )
	PORT_CONFSETTING(    0x05, "256KB (PC-8801-02N x 2)" )
	PORT_CONFSETTING(    0x06, "512KB (PC-8801-02N x 4)" )
	PORT_CONFSETTING(    0x07, "1M (PIO-8234H-1M x 1)" )
	PORT_CONFSETTING(    0x08, "2M (PIO-8234H-2M x 1)" )
	PORT_CONFSETTING(    0x09, "4M (PIO-8234H-2M x 2)" )
	PORT_CONFSETTING(    0x0a, "8M (PIO-8234H-2M x 4)" )
	PORT_CONFSETTING(    0x0b, "1.1M (PIO-8234H-1M x 1 + PC-8801-02N x 1)" )
	PORT_CONFSETTING(    0x0c, "2.1M (PIO-8234H-2M x 1 + PC-8801-02N x 1)" )
	PORT_CONFSETTING(    0x0d, "4.1M (PIO-8234H-2M x 2 + PC-8801-02N x 1)" )
INPUT_PORTS_END

static INPUT_PORTS_START( pc88sr )
	PORT_INCLUDE( pc8001 )

	PORT_MODIFY("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "BASIC" )
	PORT_DIPSETTING(    0x01, "N88-BASIC" )
	PORT_DIPSETTING(    0x00, "N-BASIC" )
INPUT_PORTS_END

/* Graphics Layouts */

static const gfx_layout char_layout_40L_h =
{
	8, 4,						/* 16 x 4 graphics */
	1024,						/* 256 codes */
	1,							/* 1 bit per pixel */
	{ 0x1000*8 },				/* no bitplanes */
	{ 0, 0, 1, 1, 2, 2, 3, 3 },
	{ 0*8, 0*8, 1*8, 1*8 },
	8 * 2						/* code takes 8 times 8 bits */
};

static const gfx_layout char_layout_40R_h =
{
	8, 4,						/* 16 x 4 graphics */
	1024,						/* 256 codes */
	1,							/* 1 bit per pixel */
	{ 0x1000*8 },				/* no bitplanes */
	{ 4, 4, 5, 5, 6, 6, 7, 7 },
	{ 0*8, 0*8, 1*8, 1*8 },
	8 * 2						/* code takes 8 times 8 bits */
};

static const gfx_layout char_layout_80_h =
{
	8, 4,           /* 16 x 4 graphics */
	1024,            /* 256 codes */
	1,                      /* 1 bit per pixel */
	{ 0x1000*8 },          /* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 0*8, 1*8, 1*8 },
	8 * 2           /* code takes 8 times 8 bits */
};

static const gfx_layout char_layout_40L_l =
{
	8, 2,           /* 16 x 4 graphics */
	1024,            /* 256 codes */
	1,                      /* 1 bit per pixel */
	{ 0x1000*8 },          /* no bitplanes */
	{ 0, 0, 1, 1, 2, 2, 3, 3 },
	{ 0*8, 1*8 },
	8 * 2           /* code takes 8 times 8 bits */
};

static const gfx_layout char_layout_40R_l =
{
	8, 2,           /* 16 x 4 graphics */
	1024,            /* 256 codes */
	1,                      /* 1 bit per pixel */
	{ 0x1000*8 },          /* no bitplanes */
	{ 4, 4, 5, 5, 6, 6, 7, 7 },
	{ 0*8, 1*8 },
	8 * 2           /* code takes 8 times 8 bits */
};

static const gfx_layout char_layout_80_l =
{
	8, 2,           /* 16 x 4 graphics */
	1024,            /* 256 codes */
	1,                      /* 1 bit per pixel */
	{ 0x1000*8 },          /* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8 },
	8 * 2           /* code takes 8 times 8 bits */
};

static GFXDECODE_START( pc8801 )
	GFXDECODE_ENTRY( "gfx1", 0, char_layout_80_l, 0, 16 )
	GFXDECODE_ENTRY( "gfx1", 0, char_layout_40L_l, 0, 16 )
	GFXDECODE_ENTRY( "gfx1", 0, char_layout_40R_l, 0, 16 )
	GFXDECODE_ENTRY( "gfx1", 0, char_layout_80_h, 0, 16 )
	GFXDECODE_ENTRY( "gfx1", 0, char_layout_40L_h, 0, 16 )
	GFXDECODE_ENTRY( "gfx1", 0, char_layout_40R_h, 0, 16 )
GFXDECODE_END

/* uPD1990A Interface */

static UPD1990A_INTERFACE( pc88_upd1990a_intf )
{
	DEVCB_NULL,
	DEVCB_NULL
};

/* YM2203 Interface */

static READ8_DEVICE_HANDLER( opn_dummy_input ) { return 0xff; }

static const ym2203_interface pc88_ym2203_intf =
{
	{
		AY8910_LEGACY_OUTPUT,
		AY8910_DEFAULT_LOADS,
		DEVCB_HANDLER(opn_dummy_input),
		DEVCB_HANDLER(opn_dummy_input),
		DEVCB_NULL,
		DEVCB_NULL
	},
	pc88sr_sound_interupt
};

/* Floppy Configuration */

static const floppy_config pc88_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(default),
	DO_NOT_KEEP_GEOMETRY
};

/* Cassette Configuration */

static const cassette_config pc88_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED
};

/* Machine Drivers */

static MACHINE_DRIVER_START( pc88srl )
	MDRV_DRIVER_DATA(pc88_state)

	/* basic machine hardware */

	/* main CPU */
	MDRV_CPU_ADD("maincpu", Z80, 4000000)        /* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(pc8801_mem)
	MDRV_CPU_IO_MAP(pc88sr_io)
	MDRV_CPU_VBLANK_INT(SCREEN_TAG, pc8801_interrupt)

	/* sub CPU(5 inch floppy drive) */
	MDRV_CPU_ADD("sub", Z80, 4000000)		/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(pc8801fd_mem)
	MDRV_CPU_IO_MAP(pc8801fd_io)

	MDRV_QUANTUM_TIME(HZ(300000))

	MDRV_MACHINE_START( pc88srl )
	MDRV_MACHINE_RESET( pc88srl )

	MDRV_I8255A_ADD( CPU_I8255A_TAG, pc8801_8255_config_0 )
	MDRV_I8255A_ADD( FDC_I8255A_TAG, pc8801_8255_config_1 )

	/* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	/*MDRV_ASPECT_RATIO(8,5)*/
	MDRV_SCREEN_SIZE(640, 220)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MDRV_GFXDECODE( pc8801 )
	MDRV_PALETTE_LENGTH(32)
	MDRV_PALETTE_INIT( pc8801 )

	MDRV_VIDEO_START(pc8801)
	MDRV_VIDEO_UPDATE(pc8801)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2203_TAG, YM2203, 3993600)
	MDRV_SOUND_CONFIG(pc88_ym2203_intf)	/* Should be accurate */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	MDRV_UPD765A_ADD(UPD765_TAG, pc8801_fdc_interface)
	MDRV_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, pc88_upd1990a_intf)
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)
	MDRV_CASSETTE_ADD(CASSETTE_TAG, pc88_cassette_config)

	MDRV_FLOPPY_2_DRIVES_ADD(pc88_floppy_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc88srh )
	MDRV_IMPORT_FROM( pc88srl )
	MDRV_QUANTUM_TIME(HZ(360000))

	MDRV_MACHINE_START( pc88srh )
	MDRV_MACHINE_RESET( pc88srh )

	MDRV_SCREEN_MODIFY(SCREEN_TAG)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	/*MDRV_ASPECT_RATIO(8, 5)*/
	MDRV_SCREEN_SIZE(640, 440)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 400-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc88va )
	MDRV_DRIVER_DATA(pc88_state)

	MDRV_CPU_ADD("maincpu", Z80, 8000000)        /* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(pc88va_mem)
	MDRV_CPU_IO_MAP(pc88va_io)

	MDRV_CPU_ADD("cpu2", V30MZ, 8000000)        /* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(pc88va_v30_mem)

	/* No accurate at all, it's just skeleton code */
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	/*MDRV_ASPECT_RATIO(8, 5)*/
	MDRV_SCREEN_SIZE(640, 440)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 400-1)
	MDRV_PALETTE_LENGTH(32)
	MDRV_PALETTE_INIT( pc8801 )
MACHINE_DRIVER_END

/* ROMs */

ROM_START( pc8801 )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "n80.rom",   0x00000, 0x8000, CRC(5cb8b584) SHA1(063609dd518c124a4fc9ba35d1bae35771666a34) )
	ROM_LOAD( "n88.rom",   0x08000, 0x8000, CRC(ffd68be0) SHA1(3518193b8207bdebf22c1380c2db8c554baff329) )
	ROM_LOAD( "n88_0.rom", 0x10000, 0x2000, CRC(61984bab) SHA1(d1ae642aed4f0584eeb81ff50180db694e5101d4) )

	/* hack - this should not be here, but we still don't support these early models*/
	ROM_REGION( 0x10000, "sub", ROMREGION_ERASEFF)

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "font.rom", 0x0000, 0x0800, CRC(56653188) SHA1(84b90f69671d4b72e8f219e1fe7cd667e976cf7f) )
ROM_END

/* The dump only included "maincpu". Other roms arbitrariely taken from PC-8801 & PC-8801 MkIISR (there should be
at least 1 Kanji ROM). */
ROM_START( pc8801mk2 )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "m2_n80.rom",   0x00000, 0x8000, CRC(91d84b1a) SHA1(d8a1abb0df75936b3fc9d226ccdb664a9070ffb1) )
	ROM_LOAD( "m2_n88.rom",   0x08000, 0x8000, CRC(f35169eb) SHA1(ef1f067f819781d9fb2713836d195866f0f81501) )
	ROM_LOAD( "m2_n88_0.rom", 0x10000, 0x2000, CRC(5eb7a8d0) SHA1(95a70af83b0637a5a0f05e31fb0452bb2cb68055) )

	/* hack - this should not be here, but we still don't support these early models*/
	ROM_REGION( 0x10000, "sub", ROMREGION_ERASEFF)

	/* should this be here? */
	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )
ROM_END

ROM_START( pc8001mk2sr )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "mk2sr_n80.rom",   0x00000, 0x8000, CRC(27e1857d) SHA1(5b922ed9de07d2a729bdf1da7b57c50ddf08809a) )
	ROM_LOAD( "mk2sr_n88.rom",   0x08000, 0x8000, CRC(a0fc0473) SHA1(3b31fc68fa7f47b21c1a1cb027b86b9e87afbfff) )
	ROM_LOAD( "mk2sr_n88_0.rom", 0x10000, 0x2000, CRC(710a63ec) SHA1(d239c26ad7ac5efac6e947b0e9549b1534aa970d) )
	ROM_LOAD( "n88_1.rom", 0x12000, 0x2000, CRC(c0bd2aa6) SHA1(8528eef7946edf6501a6ccb1f416b60c64efac7c) )
	ROM_LOAD( "n88_2.rom", 0x14000, 0x2000, CRC(af2b6efa) SHA1(b7c8bcea219b77d9cc3ee0efafe343cc307425d1) )
	ROM_LOAD( "n88_3.rom", 0x16000, 0x2000, CRC(7713c519) SHA1(efce0b51cab9f0da6cf68507757f1245a2867a72) )

	ROM_REGION( 0x10000, "sub", 0)
	ROM_LOAD( "disk.rom", 0x0000, 0x0800, CRC(2158d307) SHA1(bb7103a0818850a039c67ff666a31ce49a8d516f) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )
	ROM_LOAD( "kanji2.rom", 0x20000, 0x20000, CRC(154803cc) SHA1(7e6591cd465cbb35d6d3446c5a83b46d30fafe95) )	// it should not be here
ROM_END

/* They differ for the monitor resolution */
#define rom_pc8801mk2sr		rom_pc8001mk2sr

ROM_START( pc8801mk2fr )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "m2fr_n80.rom",   0x00000, 0x8000, CRC(27e1857d) SHA1(5b922ed9de07d2a729bdf1da7b57c50ddf08809a) )
	ROM_LOAD( "m2fr_n88.rom",   0x08000, 0x8000, CRC(b9daf1aa) SHA1(696a480232bcf8c827c7aeea8329db5c44420d2a) )
	ROM_LOAD( "m2fr_n88_0.rom", 0x10000, 0x2000, CRC(710a63ec) SHA1(d239c26ad7ac5efac6e947b0e9549b1534aa970d) )
	ROM_LOAD( "m2fr_n88_1.rom", 0x12000, 0x2000, CRC(e3e78a37) SHA1(85ecd287fe72b56e54c8b01ea7492ca4a69a7470) )
	ROM_LOAD( "m2fr_n88_2.rom", 0x14000, 0x2000, CRC(98c3a7b2) SHA1(fc4980762d3caa56964d0ae583424756f511d186) )
	ROM_LOAD( "m2fr_n88_3.rom", 0x16000, 0x2000, CRC(0ca08abd) SHA1(a5a42d0b7caa84c3bc6e337c9f37874d82f9c14b) )

	ROM_REGION( 0x10000, "sub", 0)
	ROM_LOAD( "m2fr_disk.rom", 0x0000, 0x0800, CRC(2163b304) SHA1(80da2dee49d4307f00895a129a5cfeff00cf5321) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )
ROM_END

ROM_START( pc8801mk2mr )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "m2mr_n80.rom",   0x00000, 0x8000, CRC(f074b515) SHA1(ebe9cf4cf57f1602c887f609a728267f8d953dce) )
	ROM_LOAD( "m2mr_n88.rom",   0x08000, 0x8000, CRC(69caa38e) SHA1(3c64090237152ee77c76e04d6f36bad7297bea93) )
	ROM_LOAD( "m2mr_n88_0.rom", 0x10000, 0x2000, CRC(710a63ec) SHA1(d239c26ad7ac5efac6e947b0e9549b1534aa970d) )
	ROM_LOAD( "m2mr_n88_1.rom", 0x12000, 0x2000, CRC(e3e78a37) SHA1(85ecd287fe72b56e54c8b01ea7492ca4a69a7470) )
	ROM_LOAD( "m2mr_n88_2.rom", 0x14000, 0x2000, CRC(11176e0b) SHA1(f13f14f3d62df61498a23f7eb624e1a646caea45) )
	ROM_LOAD( "m2mr_n88_3.rom", 0x16000, 0x2000, CRC(0ca08abd) SHA1(a5a42d0b7caa84c3bc6e337c9f37874d82f9c14b) )

	ROM_REGION( 0x10000, "sub", 0)
	ROM_LOAD( "m2mr_disk.rom", 0x0000, 0x2000, CRC(2447516b) SHA1(1492116f15c426f9796dc2bb6fcccf2656c0ca75) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji2", 0)
	ROM_LOAD( "m2mr_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )
ROM_END

ROM_START( pc8801mh )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "mh_n80.rom",   0x00000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )
	ROM_LOAD( "mh_n88.rom",   0x08000, 0x8000, CRC(64c5d162) SHA1(3e0aac76fb5d7edc99df26fa9f365fd991742a5d) )
	ROM_LOAD( "mh_n88_0.rom", 0x10000, 0x2000, CRC(deb384fb) SHA1(5f38cafa8aab16338038c82267800446fd082e79) )
	ROM_LOAD( "mh_n88_1.rom", 0x12000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "mh_n88_2.rom", 0x14000, 0x2000, CRC(6aa6b6d8) SHA1(2a077ab444a4fd1470cafb06fd3a0f45420c39cc) )
	ROM_LOAD( "mh_n88_3.rom", 0x16000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "sub", 0)
	ROM_LOAD( "mh_disk.rom", 0x0000, 0x2000, CRC(a222ecf0) SHA1(79e9c0786a14142f7a83690bf41fb4f60c5c1004) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji2", 0)
	ROM_LOAD( "mh_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )
ROM_END

ROM_START( pc8801fa )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "fa_n80.rom",   0x00000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )
	ROM_LOAD( "fa_n88.rom",   0x08000, 0x8000, CRC(73573432) SHA1(9b1346d44044eeea921c4cce69b5dc49dbc0b7e9) )
	ROM_LOAD( "fa_n88_0.rom", 0x10000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )
	ROM_LOAD( "fa_n88_1.rom", 0x12000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "fa_n88_2.rom", 0x14000, 0x2000, CRC(6aee9a4e) SHA1(e94278682ef9e9bbb82201f72c50382748dcea2a) )
	ROM_LOAD( "fa_n88_3.rom", 0x16000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "sub", 0 )
	ROM_LOAD( "fa_disk.rom", 0x0000, 0x0800, CRC(2163b304) SHA1(80da2dee49d4307f00895a129a5cfeff00cf5321) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji2", 0)
	ROM_LOAD( "fa_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )
ROM_END

ROM_START( pc8801ma )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "ma_n80.rom",   0x00000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )
	ROM_LOAD( "ma_n88.rom",   0x08000, 0x8000, CRC(73573432) SHA1(9b1346d44044eeea921c4cce69b5dc49dbc0b7e9) )
	ROM_LOAD( "ma_n88_0.rom", 0x10000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )
	ROM_LOAD( "ma_n88_1.rom", 0x12000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "ma_n88_2.rom", 0x14000, 0x2000, CRC(6aee9a4e) SHA1(e94278682ef9e9bbb82201f72c50382748dcea2a) )
	ROM_LOAD( "ma_n88_3.rom", 0x16000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "sub", 0 )
	ROM_LOAD( "ma_disk.rom", 0x0000, 0x2000, CRC(a222ecf0) SHA1(79e9c0786a14142f7a83690bf41fb4f60c5c1004) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji2", 0)
	ROM_LOAD( "ma_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	/* 32 banks, to be loaded at 0xc000 - 0xffff */
	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "ma_jisyo.rom", 0x00000, 0x80000, CRC(a6108f4d) SHA1(3665db538598abb45d9dfe636423e6728a812b12) )
ROM_END

ROM_START( pc8801ma2 )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "ma2_n80.rom",   0x00000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )
	ROM_LOAD( "ma2_n88.rom",   0x08000, 0x8000, CRC(ae1a6ebc) SHA1(e53d628638f663099234e07837ffb1b0f86d480d) )
	ROM_LOAD( "ma2_n88_0.rom", 0x10000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )
	ROM_LOAD( "ma2_n88_1.rom", 0x12000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "ma2_n88_2.rom", 0x14000, 0x2000, CRC(1d6277b6) SHA1(dd9c3e50169b75bb707ef648f20d352e6a8bcfe4) )
	ROM_LOAD( "ma2_n88_3.rom", 0x16000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "sub", 0 )
	ROM_LOAD( "ma2_disk.rom", 0x0000, 0x2000, CRC(a222ecf0) SHA1(79e9c0786a14142f7a83690bf41fb4f60c5c1004) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji2", 0)
	ROM_LOAD( "ma2_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	/* 32 banks, to be loaded at 0xc000 - 0xffff */
	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "ma2_jisyo.rom", 0x00000, 0x80000, CRC(856459af) SHA1(06241085fc1d62d4b2968ad9cdbdadc1e7d7990a) )
ROM_END

ROM_START( pc8801mc )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "mc_n80.rom",   0x00000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )
	ROM_LOAD( "mc_n88.rom",   0x08000, 0x8000, CRC(356d5719) SHA1(5d9ba80d593a5119f52aae1ccd61a1457b4a89a1) )
	ROM_LOAD( "mc_n88_0.rom", 0x10000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )
	ROM_LOAD( "mc_n88_1.rom", 0x12000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "mc_n88_2.rom", 0x14000, 0x2000, CRC(1d6277b6) SHA1(dd9c3e50169b75bb707ef648f20d352e6a8bcfe4) )
	ROM_LOAD( "mc_n88_3.rom", 0x16000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "sub", 0 )
	ROM_LOAD( "mc_disk.rom", 0x0000, 0x2000, CRC(a222ecf0) SHA1(79e9c0786a14142f7a83690bf41fb4f60c5c1004) )

	ROM_REGION( 0x10000, "cdrom", 0 )
	ROM_LOAD( "cdbios.rom", 0x0000, 0x10000, CRC(5c230221) SHA1(6394a8a23f44ea35fcfc3e974cf940bc8f84d62a) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji2", 0)
	ROM_LOAD( "mc_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	/* 32 banks, to be loaded at 0xc000 - 0xffff */
	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "mc_jisyo.rom", 0x00000, 0x80000, CRC(bd6eb062) SHA1(deef0cc2a9734ba891a6d6c022aa70ffc66f783e) )
ROM_END

ROM_START( pc88va )
	ROM_REGION( 0xb0000, "maincpu", 0 )
	/* FWIW, varom00 contains some piece of N88-BASIC */
	ROM_LOAD( "varom00.rom",   0x10000, 0x80000, CRC(98c9959a) SHA1(bcaea28c58816602ca1e8290f534360f1ca03fe8) )	// multiple banks to be loaded at 0x0000?
	ROM_LOAD( "varom08.rom",   0x90000, 0x20000, CRC(eef6d4a0) SHA1(47e5f89f8b0ce18ff8d5d7b7aef8ca0a2a8e3345) )	// multiple banks to be loaded at 0x8000?

	ROM_REGION( 0x20000, "cpu2", 0 )	// not sure, it may be the V30 program
	ROM_LOAD( "varom1.rom", 0x0000, 0x20000, CRC(7e767f00) SHA1(dd4f4521bfbb068f15ab3bcdb8d47c7d82b9d1d4) )

	ROM_REGION( 0x2000, "sub", 0 )		// not sure what this should do...
	ROM_LOAD( "vasubsys.rom", 0x0000, 0x2000, CRC(08962850) SHA1(a9375aa480f85e1422a0e1385acb0ea170c5c2e0) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x50000, "gfx1", 0 )
	ROM_LOAD( "vafont.rom", 0x00000, 0x50000, CRC(b40d34e4) SHA1(a0227d1fbc2da5db4b46d8d2c7e7a9ac2d91379f) )

	/* 32 banks, to be loaded at 0xc000 - 0xffff */
	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "vadic.rom", 0x00000, 0x80000, CRC(a6108f4d) SHA1(3665db538598abb45d9dfe636423e6728a812b12) )
ROM_END

/* System Drivers */

/*    YEAR  NAME			PARENT	COMPAT  MACHINE   INPUT    INIT  CONFIG  COMPANY FULLNAME */
COMP( 1985, pc8001mk2sr,	0,		0,     pc88srl,  pc8001,  0,    0,   "NEC",  "PC-8001mkIISR", GAME_NOT_WORKING )

COMP( 1981, pc8801,			0,		0,     pc88srl,  pc88sr,  0,    0,   "NEC",  "PC-8801", GAME_NOT_WORKING )
COMP( 1983, pc8801mk2,		pc8801,	0,     pc88srl,  pc88sr,  0,    0,   "NEC",  "PC-8801mkII", GAME_NOT_WORKING )	// not sure about this dump
COMP( 1985, pc8801mk2sr,	pc8801,	0,     pc88srh,  pc88sr,  0,    0,   "NEC",  "PC-8801mkIISR", GAME_NOT_WORKING )
//COMP( 1985, pc8801mk2tr,	pc8801,	0,     pc88srh,  pc88sr,  0,    0,   "NEC",  "PC-8801mkIITR", GAME_NOT_WORKING )
COMP( 1985, pc8801mk2fr,	pc8801,	0,     pc88srh,  pc88sr,  0,    0,   "NEC",  "PC-8801mkIIFR", GAME_NOT_WORKING )
COMP( 1985, pc8801mk2mr,	pc8801,	0,     pc88srh,  pc88sr,  0,    0,   "NEC",  "PC-8801mkIIMR", GAME_NOT_WORKING )

//COMP( 1986, pc8801fh,		0,		0,     pc88srh,  pc88sr,  0,	0,   "NEC",  "PC-8801FH", GAME_NOT_WORKING )
COMP( 1986, pc8801mh,		pc8801,	0,     pc88srh,  pc88sr,  0,	0,   "NEC",  "PC-8801MH", GAME_NOT_WORKING )
COMP( 1987, pc8801fa,		pc8801,	0,     pc88srh,  pc88sr,  0,    0,   "NEC",  "PC-8801FA", GAME_NOT_WORKING )
COMP( 1987, pc8801ma,		pc8801,	0,     pc88srh,  pc88sr,  0,    0,   "NEC",  "PC-8801MA", GAME_NOT_WORKING )
//COMP( 1988, pc8801fe,		pc8801,	0,     pc88srh,  pc88sr,  0,	0,   "NEC",  "PC-8801FE", GAME_NOT_WORKING )
COMP( 1988, pc8801ma2,		pc8801,	0,     pc88srh,  pc88sr,  0,    0,   "NEC",  "PC-8801MA2", GAME_NOT_WORKING )
//COMP( 1989, pc8801fe2,	pc8801,	0,     pc88srh,  pc88sr,  0,	0,   "NEC",  "PC-8801FE2", GAME_NOT_WORKING )
COMP( 1989, pc8801mc,		pc8801,	0,     pc88srh,  pc88sr,  0,    0,   "NEC",  "PC-8801MC", GAME_NOT_WORKING )

COMP( 1987, pc88va,			0,		0,     pc88va,   pc88sr,  0,    0,   "NEC",  "PC-88VA", GAME_NOT_WORKING )
//COMP( 1988, pc88va2,		pc88va,	0,     pc88va,   pc88sr,  0,    0,   "NEC",  "PC-88VA2", GAME_NOT_WORKING )
//COMP( 1988, pc88va3,		pc88va,	0,     pc88va,   pc88sr,  0,    0,   "NEC",  "PC-88VA3", GAME_NOT_WORKING )

//COMP( 1989, pc98do,		0,		0,     pc88va,   pc88sr,  0,    0,   "NEC",  "PC-98DO", GAME_NOT_WORKING )
//COMP( 1990, pc98dop,		0,		0,     pc88va,   pc88sr,  0,    0,   "NEC",  "PC-98DO+", GAME_NOT_WORKING )
