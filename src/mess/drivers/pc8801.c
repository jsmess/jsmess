/***************************************************************************


	Note: These emulations take a long time to start. Be patient!


	NEC PC-88xx Models (and similar machines like PC-80xx and PC-98DO)

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
        - We need to emulate the differences in the new sets (MkIIFR, MA, MA2, MC) compared 
		to MkIISR.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/beep.h"
#include "machine/8255ppi.h"
#include "includes/pc8801.h"
#include "machine/nec765.h"
#include "includes/d88.h"
#include "devices/basicdsk.h"
#include "sound/2203intf.h"

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

/* Macro for DIPSW-1 */
#define DIPSW_1_1 \
	PORT_DIPNAME( 0x01, 0x01, "Terminal mode" ) \
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_1_2 \
	PORT_DIPNAME( 0x02, 0x00, "Text width" ) \
	PORT_DIPSETTING(    0x02, "40 chars/line" ) \
	PORT_DIPSETTING(    0x00, "80 chars/line" )
#define DIPSW_1_3 \
	PORT_DIPNAME( 0x04, 0x00, "Text height" ) \
	PORT_DIPSETTING(    0x04, "20 lines/screen" ) \
	PORT_DIPSETTING(    0x00, "25 lines/screen" )
#define DIPSW_1_4 \
	PORT_DIPNAME( 0x08, 0x08, "Enable S parameter" ) \
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_1_5 \
	PORT_DIPNAME( 0x10, 0x00, "Enable DEL code" ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_1_6 \
	PORT_DIPNAME( 0x20, 0x20, "Memory wait" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_1_7 \
	PORT_DIPNAME( 0x40, 0x40, "Disable CMD SING" ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

/* Macro for DIPSW-2 */
#define DIPSW_2_1 \
	PORT_DIPNAME( 0x01, 0x01, "Parity generate" ) \
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_2_2 \
	PORT_DIPNAME( 0x02, 0x00, "Parity type" ) \
	PORT_DIPSETTING(    0x00, "Even" ) \
	PORT_DIPSETTING(    0x02, "Odd" )
#define DIPSW_2_3 \
	PORT_DIPNAME( 0x04, 0x00, "Serial character length" ) \
	PORT_DIPSETTING(    0x04, "7 bits/char" ) \
	PORT_DIPSETTING(    0x00, "8 bits/char" )
#define DIPSW_2_4 \
	PORT_DIPNAME( 0x08, 0x08, "Stop bit length" ) \
	PORT_DIPSETTING(    0x08, "1" ) \
	PORT_DIPSETTING(    0x00, "2" )
#define DIPSW_2_5 \
	PORT_DIPNAME( 0x10, 0x10, "Enable X parameter" ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_2_6 \
	PORT_DIPNAME( 0x20, 0x20, "Duplex mode" ) \
	PORT_DIPSETTING(    0x20, "half duplex" ) \
	PORT_DIPSETTING(    0x00, "full duplex" )
#define DIPSW_2_7 \
	PORT_DIPNAME( 0x40, 0x00, "Boot from internal FD" ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_2_8 \
	PORT_DIPNAME( 0x80, 0x80, "Disable internal FD" ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

/* Macro for other switch */
#define SW_V1V2N \
	PORT_DIPNAME( 0x03, 0x01, "Basic mode" ) \
	PORT_DIPSETTING(    0x02, "N-BASIC" ) \
	PORT_DIPSETTING(    0x03, "N88-BASIC (V1)" ) \
	PORT_DIPSETTING(    0x01, "N88-BASIC (V2)" )

#define SW_HS \
	PORT_DIPNAME( 0x04, 0x04, "Speed mode" ) \
	PORT_DIPSETTING(    0x00, "Slow" ) \
	PORT_DIPSETTING(    0x04, DEF_STR( High ) )

#define SW_8MHZ \
	PORT_DIPNAME( 0x08, 0x00, "Main CPU clock" ) \
	PORT_DIPSETTING(    0x00, "4MHz" ) \
	PORT_DIPSETTING(    0x08, "8MHz" )

#define SW_4MHZ_ONLY \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )

#define SW_SERIAL \
	PORT_DIPNAME( 0xf0, 0x80, "Serial speed" ) \
	PORT_DIPSETTING(    0x10, "75bps" ) \
	PORT_DIPSETTING(    0x20, "150bps" ) \
	PORT_DIPSETTING(    0x30, "300bps" ) \
	PORT_DIPSETTING(    0x40, "600bps" ) \
	PORT_DIPSETTING(    0x50, "1200bps" ) \
	PORT_DIPSETTING(    0x60, "2400bps" ) \
	PORT_DIPSETTING(    0x70, "4800bps" ) \
	PORT_DIPSETTING(    0x80, "9600bps" ) \
	PORT_DIPSETTING(    0x90, "19200bps" )

#define SW_EXTMEM \
	PORT_DIPNAME( 0x1f, 0x00, "Extension memory" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( None ) ) \
	PORT_DIPSETTING(    0x01, "32KB (PC-8012-02 x 1)" ) \
	PORT_DIPSETTING(    0x02, "64KB (PC-8012-02 x 2)" ) \
	PORT_DIPSETTING(    0x03, "128KB (PC-8012-02 x 4)" ) \
	PORT_DIPSETTING(    0x04, "128KB (PC-8801-02N x 1)" ) \
	PORT_DIPSETTING(    0x05, "256KB (PC-8801-02N x 2)" ) \
	PORT_DIPSETTING(    0x06, "512KB (PC-8801-02N x 4)" ) \
	PORT_DIPSETTING(    0x07, "1M (PIO-8234H-1M x 1)" ) \
	PORT_DIPSETTING(    0x08, "2M (PIO-8234H-2M x 1)" ) \
	PORT_DIPSETTING(    0x09, "4M (PIO-8234H-2M x 2)" ) \
	PORT_DIPSETTING(    0x0a, "8M (PIO-8234H-2M x 4)" ) \
	PORT_DIPSETTING(    0x0b, "1.1M (PIO-8234H-1M x 1 + PC-8801-02N x 1)" ) \
	PORT_DIPSETTING(    0x0c, "2.1M (PIO-8234H-2M x 1 + PC-8801-02N x 1)" ) \
	PORT_DIPSETTING(    0x0d, "4.1M (PIO-8234H-2M x 2 + PC-8801-02N x 1)" )

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
 
static INPUT_PORTS_START( pc88sr )
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
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Clr  Home") PORT_CODE(KEYCODE_HOME)		PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)								PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)							PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del  Ins") PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(UCHAR_MAMEKEY(DEL)) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
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
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)							PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)							PORT_CHAR(UCHAR_MAMEKEY(LEFT))
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

	PORT_START("CFG")		/* EXSWITCH */
	SW_V1V2N
	SW_HS
	SW_4MHZ_ONLY
	SW_SERIAL

	PORT_START("DSW1")		/* DIP-SW1 */
	DIPSW_1_1
	DIPSW_1_2
	DIPSW_1_3
	DIPSW_1_4
	DIPSW_1_5
	DIPSW_1_6
	DIPSW_1_7

	PORT_START("DSW2")		/* DIP-SW2 */
	DIPSW_2_1
	DIPSW_2_2
	DIPSW_2_3
	DIPSW_2_4
	DIPSW_2_5
	DIPSW_2_6
	DIPSW_2_7
	DIPSW_2_8

	PORT_START("MEM")		/* extension memory setting */
	SW_EXTMEM

INPUT_PORTS_END


static ADDRESS_MAP_START( pc8801_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x5fff) AM_RAMBANK(1)
	AM_RANGE(0x6000, 0x7fff) AM_RAMBANK(2)
	AM_RANGE(0x8000, 0x83ff) AM_RAMBANK(3)
	AM_RANGE(0x8400, 0xbfff) AM_RAMBANK(4)
	AM_RANGE(0xc000, 0xefff) AM_RAMBANK(5)
	AM_RANGE(0xf000, 0xffff) AM_RAMBANK(6)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc88sr_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x00) AM_READ_PORT( "KEY0" )  					/* keyboard */
	AM_RANGE(0x01, 0x01) AM_READ_PORT( "KEY1" )  					/* keyboard */
	AM_RANGE(0x02, 0x02) AM_READ_PORT( "KEY2" )  					/* keyboard */
	AM_RANGE(0x03, 0x03) AM_READ_PORT( "KEY3" )  					/* keyboard */
	AM_RANGE(0x04, 0x04) AM_READ_PORT( "KEY4" )  					/* keyboard */
	AM_RANGE(0x05, 0x05) AM_READ_PORT( "KEY5" ) 					/* keyboard */
	AM_RANGE(0x06, 0x06) AM_READ_PORT( "KEY6" )  					/* keyboard */
	AM_RANGE(0x07, 0x07) AM_READ_PORT( "KEY7" )  					/* keyboard */
	AM_RANGE(0x08, 0x08) AM_READ_PORT( "KEY8" ) 					/* keyboard */
	AM_RANGE(0x09, 0x09) AM_READ_PORT( "KEY9" ) 					/* keyboard */
	AM_RANGE(0x0a, 0x0a) AM_READ_PORT( "KEY10" ) 					/* keyboard */
	AM_RANGE(0x0b, 0x0b) AM_READ_PORT( "KEY11" ) 					/* keyboard */
	AM_RANGE(0x0c, 0x0c) AM_READ_PORT( "KEY12" ) 					/* keyboard */
	AM_RANGE(0x0d, 0x0d) AM_READ_PORT( "KEY13" ) 					/* keyboard */
	AM_RANGE(0x0e, 0x0e) AM_READ_PORT( "KEY14" ) 					/* keyboard */
	AM_RANGE(0x0f, 0x0f) AM_READ_PORT( "KEY15" )					/* keyboard */
	AM_RANGE(0x10, 0x10) AM_WRITE( pc8801_calender ) 				/* printer and clock and UOP3 */
	AM_RANGE(0x20, 0x21) AM_NOP										/* RS-232C and cassette (not yet) */
	AM_RANGE(0x30, 0x30) AM_READWRITE( pc88sr_inport_30, pc88sr_outport_30 )	/* DIP-SW1 */
	AM_RANGE(0x31, 0x31) AM_READWRITE( pc88sr_inport_31, pc88sr_outport_31 )	/* DIP-SW2 */
	AM_RANGE(0x32, 0x32) AM_READWRITE( pc88sr_inport_32, pc88sr_outport_32 )
	AM_RANGE(0x34, 0x35) AM_WRITE( pc88sr_ALU )
	AM_RANGE(0x40, 0x40) AM_READWRITE( pc88sr_inport_40, pc88sr_outport_40 )
	AM_RANGE(0x44, 0x45) AM_DEVREAD( "ym2203", ym2203_r )
	AM_RANGE(0x46, 0x47) AM_NOP										/* OPNA extra port (not yet) */
	AM_RANGE(0x50, 0x51) AM_READWRITE( pc8801_crtc_read, pc8801_crtc_write )
	AM_RANGE(0x52, 0x5b) AM_WRITE( pc8801_palette_out )
	AM_RANGE(0x5c, 0x5c) AM_READ( pc8801_vramtest )
	AM_RANGE(0x5c, 0x5f) AM_WRITE( pc8801_vramsel )
	AM_RANGE(0x60, 0x68) AM_READWRITE( pc8801_dmac_read, pc8801_dmac_write )
	AM_RANGE(0x6e, 0x6e) AM_NOP										/* CPU clock info (not yet) */
	AM_RANGE(0x6f, 0x6f) AM_NOP										/* RS-232C speed ctrl (not yet) */
	AM_RANGE(0x70, 0x70) AM_READWRITE( pc8801_inport_70, pc8801_outport_70 )
	AM_RANGE(0x71, 0x71) AM_READWRITE( pc88sr_inport_71, pc88sr_outport_71 )
	AM_RANGE(0x78, 0x78) AM_WRITE( pc8801_outport_78 )				/* text window increment */
	AM_RANGE(0x90, 0x9f) AM_NOP 									/* CD-ROM (unknown -- not yet) */
	AM_RANGE(0xa0, 0xa3) AM_NOP 									/* music & network (unknown -- not yet) */
	AM_RANGE(0xa8, 0xad) AM_NOP										/* second sound board (not yet) */
	AM_RANGE(0xb4, 0xb5) AM_NOP 									/* Video art board (unknown -- not yet) */
	AM_RANGE(0xc1, 0xc1) AM_NOP										/* (unknown -- not yet) */
	AM_RANGE(0xc2, 0xcf) AM_NOP 									/* music (unknown -- not yet) */
	AM_RANGE(0xd0, 0xd7) AM_NOP 									/* music & GP-IB (unknown -- not yet) */
	AM_RANGE(0xd8, 0xd8) AM_NOP 									/* GP-IB (unknown -- not yet) */
	AM_RANGE(0xdc, 0xdf) AM_NOP 									/* MODEM (unknown -- not yet) */
	AM_RANGE(0xe2, 0xe3) AM_READWRITE( pc8801_read_extmem, pc8801_write_extmem ) /* expand RAM select */
	AM_RANGE(0xe4, 0xe4) AM_WRITE( pc8801_write_interrupt_level )
	AM_RANGE(0xe6, 0xe6) AM_WRITE( pc8801_write_interrupt_mask )
	AM_RANGE(0xe7, 0xe7) AM_NOP 									/* (unknown -- not yet) */
	AM_RANGE(0xe8, 0xeb) AM_READWRITE( pc8801_read_kanji1, pc8801_write_kanji1 )
	AM_RANGE(0xec, 0xed) AM_READWRITE( pc8801_read_kanji2, pc8801_write_kanji2 ) /* JIS level2 Kanji ROM */
	AM_RANGE(0xf0, 0xf1) AM_NOP 									/* Kana to Kanji dictionary ROM select (not yet) */
	AM_RANGE(0xf3, 0xf3) AM_NOP 									/* DMA floppy (unknown -- not yet) */
	AM_RANGE(0xf4, 0xf7) AM_NOP										/* DMA 5'floppy (may be not released) */
	AM_RANGE(0xf8, 0xfb) AM_NOP 									/* DMA 8'floppy (unknown -- not yet) */
	AM_RANGE(0xfc, 0xff) AM_DEVREADWRITE("ppi8255_0", ppi8255_r, ppi8255_w )
ADDRESS_MAP_END

static INTERRUPT_GEN( pc8801fd_interrupt )
{
}

static ADDRESS_MAP_START( pc8801fd_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x07ff) AM_ROM
	AM_RANGE( 0x4000, 0x7fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8801fd_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf8, 0xf8) AM_READ( pc8801fd_nec765_tc )
	AM_RANGE(0xfa, 0xfa) AM_DEVREAD("nec765", nec765_status_r )
	AM_RANGE(0xfb, 0xfb) AM_DEVREADWRITE("nec765", nec765_data_r, nec765_data_w )
	AM_RANGE(0xfc, 0xff) AM_DEVREADWRITE("ppi8255_1", ppi8255_r, ppi8255_w )
ADDRESS_MAP_END


static READ8_DEVICE_HANDLER(opn_dummy_input) { return 0xff; }

static const ym2203_interface pc8801_ym2203_interface =
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


static MACHINE_DRIVER_START( pc88srl )
	/* basic machine hardware */

	/* main CPU */
	MDRV_CPU_ADD("maincpu", Z80, 4000000)        /* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(pc8801_mem)
	MDRV_CPU_IO_MAP(pc88sr_io)
	MDRV_CPU_VBLANK_INT("screen", pc8801_interrupt)

	/* sub CPU(5 inch floppy drive) */
	MDRV_CPU_ADD("sub", Z80, 4000000)		/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(pc8801fd_mem)
	MDRV_CPU_IO_MAP(pc8801fd_io)
	MDRV_CPU_VBLANK_INT("screen", pc8801fd_interrupt)

	MDRV_QUANTUM_TIME(HZ(300000))

	MDRV_MACHINE_RESET( pc88srl )

	MDRV_PPI8255_ADD( "ppi8255_0", pc8801_8255_config_0 )

	MDRV_PPI8255_ADD( "ppi8255_1", pc8801_8255_config_1 )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
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
	MDRV_SOUND_ADD("ym2203", YM2203, 3993600)
	MDRV_SOUND_CONFIG(pc8801_ym2203_interface)	/* Should be accurate */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)
	
	MDRV_NEC765A_ADD("nec765", pc8801_fdc_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pc88srh )
	MDRV_IMPORT_FROM( pc88srl )
	MDRV_QUANTUM_TIME(HZ(360000))

	MDRV_MACHINE_RESET( pc88srh )

	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	/*MDRV_ASPECT_RATIO(8, 5)*/
	MDRV_SCREEN_SIZE(640, 440)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 400-1)
MACHINE_DRIVER_END


ROM_START( pc88m2sl )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "n80.rom",   0x00000, 0x8000, CRC(27e1857d) SHA1(5b922ed9de07d2a729bdf1da7b57c50ddf08809a) )	// it should be v1.5
	ROM_LOAD( "n88.rom",   0x08000, 0x8000, CRC(a0fc0473) SHA1(3b31fc68fa7f47b21c1a1cb027b86b9e87afbfff) )	// it should be v1.4
	ROM_LOAD( "n88_0.rom", 0x10000, 0x2000, CRC(710a63ec) SHA1(d239c26ad7ac5efac6e947b0e9549b1534aa970d) )	// it should be v2.0
	ROM_LOAD( "n88_1.rom", 0x12000, 0x2000, CRC(c0bd2aa6) SHA1(8528eef7946edf6501a6ccb1f416b60c64efac7c) )
	ROM_LOAD( "n88_2.rom", 0x14000, 0x2000, CRC(af2b6efa) SHA1(b7c8bcea219b77d9cc3ee0efafe343cc307425d1) )
	ROM_LOAD( "n88_3.rom", 0x16000, 0x2000, CRC(7713c519) SHA1(efce0b51cab9f0da6cf68507757f1245a2867a72) )

	ROM_REGION( 0x10000, "sub", 0)
	ROM_LOAD( "disk.rom", 0x0000, 0x0800, CRC(2158d307) SHA1(bb7103a0818850a039c67ff666a31ce49a8d516f) )

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )
	ROM_LOAD( "kanji2.rom", 0x20000, 0x20000, CRC(154803cc) SHA1(7e6591cd465cbb35d6d3446c5a83b46d30fafe95) )
ROM_END

/* They differ for the monitor resolution */
#define rom_pc88m2sh		rom_pc88m2sl


/* The dump only included "maincpu". Other roms arbitrariely taken from PC-8801 MkIISR. */
/* According to some sources, there should be no BASIC Expansion. Right now the driver requires such expansion 
to be present (without n88_1, n88_2, n88_3, we start with a black screen) */
ROM_START( pc88m2 )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "n80.rom",   0x00000, 0x8000, CRC(91d84b1a) SHA1(d8a1abb0df75936b3fc9d226ccdb664a9070ffb1) )	// it should be v1.4
	ROM_LOAD( "n88.rom",   0x08000, 0x8000, CRC(f35169eb) SHA1(ef1f067f819781d9fb2713836d195866f0f81501) )	// it should be v1.3
	ROM_LOAD( "n88_0.rom", 0x10000, 0x2000, CRC(5eb7a8d0) SHA1(95a70af83b0637a5a0f05e31fb0452bb2cb68055) )	// I would have expected nothing here. Not sure why the dump included this.

	ROM_REGION( 0x10000, "sub", 0)
	ROM_LOAD( "disk.rom", 0x0000, 0x0800, CRC(2158d307) SHA1(bb7103a0818850a039c67ff666a31ce49a8d516f) )

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )
	ROM_LOAD( "kanji2.rom", 0x20000, 0x20000, CRC(154803cc) SHA1(7e6591cd465cbb35d6d3446c5a83b46d30fafe95) )
ROM_END

/* n80.rom and n88_0.rom not included in the dump. Assumed to be the same as PC-8801 MkIISR. */
ROM_START( pc88m2fr )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "n80.rom",   0x00000, 0x8000, CRC(27e1857d) SHA1(5b922ed9de07d2a729bdf1da7b57c50ddf08809a) )	// it should be a version v1.5-1.8
	ROM_LOAD( "n88.rom",   0x08000, 0x8000, CRC(b9daf1aa) SHA1(696a480232bcf8c827c7aeea8329db5c44420d2a) )	// it should be a version v1.5-1.7
	ROM_LOAD( "n88_0.rom", 0x10000, 0x2000, CRC(710a63ec) SHA1(d239c26ad7ac5efac6e947b0e9549b1534aa970d) )	// it is v2.1
	ROM_LOAD( "n88_1.rom", 0x12000, 0x2000, CRC(e3e78a37) SHA1(85ecd287fe72b56e54c8b01ea7492ca4a69a7470) )
	ROM_LOAD( "n88_2.rom", 0x14000, 0x2000, CRC(98c3a7b2) SHA1(fc4980762d3caa56964d0ae583424756f511d186) )
	ROM_LOAD( "n88_3.rom", 0x16000, 0x2000, CRC(0ca08abd) SHA1(a5a42d0b7caa84c3bc6e337c9f37874d82f9c14b) )

	ROM_REGION( 0x10000, "sub", 0)
	ROM_LOAD( "disk.rom", 0x0000, 0x0800, CRC(2163b304) SHA1(80da2dee49d4307f00895a129a5cfeff00cf5321) )

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )
	ROM_LOAD( "kanji2.rom", 0x20000, 0x20000, CRC(154803cc) SHA1(7e6591cd465cbb35d6d3446c5a83b46d30fafe95) )
ROM_END

ROM_START( pc88ma )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "n80.rom",   0x00000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )	// it should be v1.8
	ROM_LOAD( "n88.rom",   0x08000, 0x8000, CRC(73573432) SHA1(9b1346d44044eeea921c4cce69b5dc49dbc0b7e9) )	// it should be v1.9
	ROM_LOAD( "n88_0.rom", 0x10000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )	// it should be v2.3
	ROM_LOAD( "n88_1.rom", 0x12000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "n88_2.rom", 0x14000, 0x2000, CRC(6aee9a4e) SHA1(e94278682ef9e9bbb82201f72c50382748dcea2a) )
	ROM_LOAD( "n88_3.rom", 0x16000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "sub", 0 )
	ROM_LOAD( "disk.rom", 0x0000, 0x2000, CRC(a222ecf0) SHA1(79e9c0786a14142f7a83690bf41fb4f60c5c1004) )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )	// not included in the dump. unsure if it should be the same of mkIISR
	ROM_LOAD( "kanji2.rom", 0x20000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	/* 32 banks, to be loaded at 0xc000 - 0xffff */
	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "jisyo.rom", 0x00000, 0x80000, CRC(a6108f4d) SHA1(3665db538598abb45d9dfe636423e6728a812b12) )
ROM_END

/* This dump only included n88, n88_2 and dictionary. Unsure about other dumps: they could be mismatched (come from PC-8801MA)! */
ROM_START( pc88ma2 )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "n80.rom",   0x00000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )	// it should be v1.8
	ROM_LOAD( "n88.rom",   0x08000, 0x8000, CRC(ae1a6ebc) SHA1(e53d628638f663099234e07837ffb1b0f86d480d) )	// it should be v1.91
	ROM_LOAD( "n88_0.rom", 0x10000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )	// it should be v2.3 (but n88_2 differs from ma)
	ROM_LOAD( "n88_1.rom", 0x12000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "n88_2.rom", 0x14000, 0x2000, CRC(1d6277b6) SHA1(dd9c3e50169b75bb707ef648f20d352e6a8bcfe4) )
	ROM_LOAD( "n88_3.rom", 0x16000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "sub", 0 )
	ROM_LOAD( "disk.rom", 0x0000, 0x2000, CRC(a222ecf0) SHA1(79e9c0786a14142f7a83690bf41fb4f60c5c1004) )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )
	ROM_LOAD( "kanji2.rom", 0x20000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	/* 32 banks, to be loaded at 0xc000 - 0xffff */
	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "jisyo.rom", 0x00000, 0x80000, CRC(856459af) SHA1(06241085fc1d62d4b2968ad9cdbdadc1e7d7990a) )
ROM_END

/* This dump only included n88, cdbios and dictionary. Unsure about other dumps: I took the 8801MA ones because the BASIC versions should be the same! */
ROM_START( pc88mc )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "n80.rom",   0x00000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )	// it should be v1.8
	ROM_LOAD( "n88.rom",   0x08000, 0x8000, CRC(356d5719) SHA1(5d9ba80d593a5119f52aae1ccd61a1457b4a89a1) )	// it should be v1.93
	ROM_LOAD( "n88_0.rom", 0x10000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )	// it should be v2.3
	ROM_LOAD( "n88_1.rom", 0x12000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "n88_2.rom", 0x14000, 0x2000, CRC(6aee9a4e) SHA1(e94278682ef9e9bbb82201f72c50382748dcea2a) )
	ROM_LOAD( "n88_3.rom", 0x16000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "sub", 0 )
	ROM_LOAD( "disk.rom", 0x0000, 0x2000, CRC(a222ecf0) SHA1(79e9c0786a14142f7a83690bf41fb4f60c5c1004) )	// not included in the dump. unsure if it should be the same of ma

	ROM_REGION( 0x10000, "cdrom", 0 )
	ROM_LOAD( "cdbios.rom", 0x0000, 0x10000, CRC(5c230221) SHA1(6394a8a23f44ea35fcfc3e974cf940bc8f84d62a) )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )	// not included in the dump. unsure if it should be the same of ma
	ROM_LOAD( "kanji2.rom", 0x20000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )	// not included in the dump. unsure if it should be the same of ma

	/* 32 banks, to be loaded at 0xc000 - 0xffff */
	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "jisyo.rom", 0x00000, 0x80000, CRC(bd6eb062) SHA1(deef0cc2a9734ba891a6d6c022aa70ffc66f783e) )
ROM_END


static void pc88_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:							info->i = IO_FLOPPY; break;
		case MESS_DEVINFO_INT_READABLE:						info->i = 1; break;
		case MESS_DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case MESS_DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:							info->start = DEVICE_START_NAME(d88image_floppy); break;
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(d88image_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "d88"); break;
	}
}

static SYSTEM_CONFIG_START(pc88)
	CONFIG_DEVICE(pc88_floppy_getinfo)
SYSTEM_CONFIG_END


/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT    INIT  CONFIG  COMPANY FULLNAME */
COMP( 1985, pc88m2sl, 0,        0,      pc88srl,  pc88sr,  0,    pc88,   "Nippon Electronic Company",  "PC-8801 MkIISR (Lores display, VSYNC 15KHz)", 0 )
COMP( 1983, pc88m2,   pc88m2sl, 0,      pc88srl,  pc88sr,  0,    pc88,   "Nippon Electronic Company",  "PC-8801 MkII (Lores display, VSYNC 15KHz)", GAME_NOT_WORKING )	// This could be made parent, but I'm not 100% confident the dump is correct
COMP( 1985, pc88m2sh, pc88m2sl, 0,      pc88srh,  pc88sr,  0,    pc88,   "Nippon Electronic Company",  "PC-8801 MkIISR (Hires display, VSYNC 24KHz)", 0 )
COMP( 1985, pc88m2fr, pc88m2sl, 0,      pc88srh,  pc88sr,  0,    pc88,   "Nippon Electronic Company",  "PC-8801 MkIIFR (Hires display, VSYNC 24KHz)", 0 )
COMP( 1987, pc88ma,   pc88m2sl, 0,      pc88srh,  pc88sr,  0,    pc88,   "Nippon Electronic Company",  "PC-8801 MA (Hires display, VSYNC 24KHz)", 0 )
COMP( 1988, pc88ma2,  pc88m2sl, 0,      pc88srh,  pc88sr,  0,    pc88,   "Nippon Electronic Company",  "PC-8801 MA2 (Hires display, VSYNC 24KHz)", 0 )
COMP( 1989, pc88mc,   pc88m2sl, 0,      pc88srh,  pc88sr,  0,    pc88,   "Nippon Electronic Company",  "PC-8801 MC (Hires display, VSYNC 24KHz)", 0 )
