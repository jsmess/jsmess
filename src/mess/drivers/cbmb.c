/***************************************************************************
    commodore b series computer

    PeT mess@utanet.at

    documentation
     vice emulator
     www.funet.fi
***************************************************************************/

/*
CBM B Series:   6509 @ 2MHz, 6545/6845 Video, 6526 CIA, 6581 SID, BASIC 4.0+
                (Sometimes called BASIC 4.5)
                Commodore differentiated between the HP (High Profile) and
                LP (Low Profile) series by naming all HP machine CBM.
                (B128-80HP was CBM128-80).  Also, any machine with optional
                8088 CPU card had 'X' after B or CBM (BX128-80).
* CBM B128-80HP 128kB, Detached Keyboard, Cream.                            GP
* CBM B128-80LP 128kB, One-Piece, Cream, New Keyboard.                      GP
* CBM B256-80HP 256kB, Detached Keyboard, Cream.
* CBM B256-80LP 256kB, One-Piece, Cream.                                    GP
* CBM B128-40   6567, 6581, 6509, 6551, 128kB.  In B128-80LP case.
  CBM B256-40   6567, 6581, 6509, 6551, 256kB.  In B128-80LP case.
* CBM B500
* CBM B500      256kB. board same as B128-80.                               GP

CBM 500 Series: 6509, 6567, 6581, 6551.
                Sometimes called PET II series.
* CBM 500       256kB. (is this the 500, or should it 515?)                 EC
* CBM 505       64kB.
* CBM 510       128kB.
* CBM P500      64kB                                                        GP

CBM 600 Series: Same as B series LP
* CBM 610       B128-80 LP                                                  CS
* CBM 620       B256-80 LP                                                  CS

CBM 700 Series: Same as B series HP.  Also named PET 700 Series
* CBM 700       B128-80 LP (Note this unit is out of place here)
* CBM 710       B128-80 HP                                                  SL
* CBM 720       B256-80 HP                                                  GP
* CBM 730       720 with 8088 coprocessor card

CBM B or II Series
B128-80LP/610/B256-80LP/620
Pet700 Series B128-80HP/710/B256-80HP/720/BX256-80HP/730
---------------------------
M6509 2 MHZ
CRTC6545 6845 video chip
RS232 Port/6551
TAPE Port
IEEE488 Port
ROM Port
Monitor Port
Audio
reset
internal user port
internal processor/dram slot
 optional 8088 cpu card
2 internal system bus slots

LP/600 series
-------------
case with integrated powersupply

HP/700 series
-------------
separated keyboard, integrated monitor, no monitor port
no standard monitor with tv frequencies, 25 character lines
with 14 lines per character (like hercules/pc mda)

B128-80LP/610
-------------
128 KB RAM

B256-80LP/620
-------------
256 KB RAM

B128-80HP/710
-------------
128 KB RAM

B256-80HP/720
-------------
256 KB RAM

BX256-80LP/730
--------------
(720 with cpu upgrade)
8088 upgrade CPU

CBM Pet II Series
500/505/515/P500/B128-40/B256-40
--------------------------------
LP/600 case
videochip vic6567
2 gameports
m6509 clock 1? MHZ

CBM 500
-------
256 KB RAM

CBM 505/P500
-------
64 KB RAM

CBM 510
-------
128 KB RAM


state
-----
keyboard
no sound
no tape support
 no system roms supporting build in tape connector found
no ieee488 support
 no floppy support
no internal userport support
no internal slot1/slot2 support
no internal cpu/dram slot support
preliminary quickloader

state 600/700
-------------
dirtybuffer based video system
 no lightpen support
 no rasterline

state 500
-----
rasterline based video system
 no lightpen support
 no rasterline support
 memory access not complete
no gameport a
 no paddles 1,2
 no joystick 1
 no 2 button joystick/mouse joystick emulation
 no mouse
 no lightpen
no gameport b
 paddles 3,4
 joystick 2
 2 button joystick/mouse joystick emulation
 no mouse

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

when problems start with -log and look into error.log file
 */

#include "driver.h"
#include "cpu/m6502/m6509.h"
#include "sound/sid6581.h"
#include "machine/6526cia.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "machine/tpi6525.h"
#include "machine/cbmipt.h"
#include "video/vic6567.h"
#include "video/mc6845.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"

#include "includes/cbmb.h"

static ADDRESS_MAP_START( cbmb_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0xf07ff) AM_READ( SMH_RAM )
#if 0
	AM_RANGE(0xf0800, 0xf0fff) AM_READ( SMH_ROM )
#endif
	AM_RANGE(0xf1000, 0xf1fff) AM_READ( SMH_ROM ) /* cartridges or ram */
	AM_RANGE(0xf2000, 0xf3fff) AM_READ( SMH_ROM ) /* cartridges or ram */
	AM_RANGE(0xf4000, 0xf5fff) AM_READ( SMH_ROM )
	AM_RANGE(0xf6000, 0xf7fff) AM_READ( SMH_ROM )
	AM_RANGE(0xf8000, 0xfbfff) AM_READ( SMH_ROM )
	/*  {0xfc000, 0xfcfff, SMH_ROM }, */
	AM_RANGE(0xfd000, 0xfd7ff) AM_READ( SMH_ROM )
//  AM_RANGE(0xfd800, 0xfd8ff)
	AM_RANGE(0xfd801, 0xfd801) AM_MIRROR( 0xfe ) AM_DEVREAD(MC6845, "crtc", mc6845_register_r)
	/* disk units */
	AM_RANGE(0xfda00, 0xfdaff) AM_READ( sid6581_0_port_r )
	/* db00 coprocessor */
	AM_RANGE(0xfdc00, 0xfdcff) AM_READ( cia_0_r )
	/* dd00 acia */
	AM_RANGE(0xfde00, 0xfdeff) AM_READ( tpi6525_0_port_r)
	AM_RANGE(0xfdf00, 0xfdfff) AM_READ( tpi6525_1_port_r)
	AM_RANGE(0xfe000, 0xfffff) AM_READ( SMH_ROM )
ADDRESS_MAP_END

static ADDRESS_MAP_START( cbmb_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x0ffff) AM_WRITE( SMH_NOP )
	AM_RANGE(0x10000, 0x4ffff) AM_WRITE( SMH_RAM )
	AM_RANGE(0x50002, 0x5ffff) AM_WRITE( SMH_NOP )
	AM_RANGE(0x60000, 0xf07ff) AM_WRITE( SMH_RAM )
	AM_RANGE(0xf1000, 0xf1fff) AM_WRITE( SMH_ROM ) /* cartridges */
	AM_RANGE(0xf2000, 0xf3fff) AM_WRITE( SMH_ROM ) /* cartridges */
	AM_RANGE(0xf4000, 0xf5fff) AM_WRITE( SMH_ROM )
	AM_RANGE(0xf6000, 0xf7fff) AM_WRITE( SMH_ROM )
	AM_RANGE(0xf8000, 0xfbfff) AM_WRITE( SMH_ROM) AM_BASE( &cbmb_basic )
	AM_RANGE(0xfd000, 0xfd7ff) AM_WRITE( SMH_RAM) AM_BASE( &videoram) AM_SIZE(&videoram_size ) /* VIDEORAM */
	AM_RANGE(0xfd800, 0xfd800) AM_MIRROR( 0xfe ) AM_DEVWRITE(MC6845, "crtc", mc6845_address_w)
	AM_RANGE(0xfd801, 0xfd801) AM_MIRROR( 0xfe ) AM_DEVWRITE(MC6845, "crtc", mc6845_register_w)
	/* disk units */
	AM_RANGE(0xfda00, 0xfdaff) AM_WRITE( sid6581_0_port_w)
	/* db00 coprocessor */
	AM_RANGE(0xfdc00, 0xfdcff) AM_WRITE( cia_0_w)
	/* dd00 acia */
	AM_RANGE(0xfde00, 0xfdeff) AM_WRITE( tpi6525_0_port_w)
	AM_RANGE(0xfdf00, 0xfdfff) AM_WRITE( tpi6525_1_port_w)
	AM_RANGE(0xfe000, 0xfffff) AM_WRITE( SMH_ROM) AM_BASE( &cbmb_kernal )
ADDRESS_MAP_END

static ADDRESS_MAP_START( cbm500_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0xf07ff) AM_READ( SMH_RAM )
#if 0
	AM_RANGE(0xf0800, 0xf0fff) AM_READ( SMH_ROM )
#endif
	AM_RANGE(0xf1000, 0xf1fff) AM_READ( SMH_ROM ) /* cartridges or ram */
	AM_RANGE(0xf2000, 0xf3fff) AM_READ( SMH_ROM ) /* cartridges or ram */
	AM_RANGE(0xf4000, 0xf5fff) AM_READ( SMH_ROM )
	AM_RANGE(0xf6000, 0xf7fff) AM_READ( SMH_ROM )
	AM_RANGE(0xf8000, 0xfbfff) AM_READ( SMH_ROM )
	/*  {0xfc000, 0xfcfff, SMH_ROM }, */
	AM_RANGE(0xfd000, 0xfd3ff) AM_READ( SMH_RAM ) /* videoram */
	AM_RANGE(0xfd400, 0xfd7ff) AM_READ( SMH_RAM ) /* colorram */
	AM_RANGE(0xfd800, 0xfd8ff) AM_READ( vic2_port_r )
	/* disk units */
	AM_RANGE(0xfda00, 0xfdaff) AM_READ( sid6581_0_port_r )
	/* db00 coprocessor */
	AM_RANGE(0xfdc00, 0xfdcff) AM_READ( cia_0_r )
	/* dd00 acia */
	AM_RANGE(0xfde00, 0xfdeff) AM_READ( tpi6525_0_port_r)
	AM_RANGE(0xfdf00, 0xfdfff) AM_READ( tpi6525_1_port_r)
	AM_RANGE(0xfe000, 0xfffff) AM_READ( SMH_ROM )
ADDRESS_MAP_END

static ADDRESS_MAP_START( cbm500_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x1ffff) AM_WRITE( SMH_RAM )
	AM_RANGE(0x20000, 0x2ffff) AM_WRITE( SMH_NOP )
	AM_RANGE(0x30000, 0x7ffff) AM_WRITE( SMH_RAM )
	AM_RANGE(0x80000, 0x8ffff) AM_WRITE( SMH_NOP )
	AM_RANGE(0x90000, 0xf07ff) AM_WRITE( SMH_RAM )
	AM_RANGE(0xf1000, 0xf1fff) AM_WRITE( SMH_ROM ) /* cartridges */
	AM_RANGE(0xf2000, 0xf3fff) AM_WRITE( SMH_ROM ) /* cartridges */
	AM_RANGE(0xf4000, 0xf5fff) AM_WRITE( SMH_ROM )
	AM_RANGE(0xf6000, 0xf7fff) AM_WRITE( SMH_ROM )
	AM_RANGE(0xf8000, 0xfbfff) AM_WRITE( SMH_ROM) AM_BASE( &cbmb_basic )
	AM_RANGE(0xfd000, 0xfd3ff) AM_WRITE( SMH_RAM) AM_BASE( &cbmb_videoram )
	AM_RANGE(0xfd400, 0xfd7ff) AM_WRITE( cbmb_colorram_w) AM_BASE( &cbmb_colorram )
	AM_RANGE(0xfd800, 0xfd8ff) AM_WRITE( vic2_port_w )
	/* disk units */
	AM_RANGE(0xfda00, 0xfdaff) AM_WRITE( sid6581_0_port_w)
	/* db00 coprocessor */
	AM_RANGE(0xfdc00, 0xfdcff) AM_WRITE( cia_0_w)
	/* dd00 acia */
	AM_RANGE(0xfde00, 0xfdeff) AM_WRITE( tpi6525_0_port_w)
	AM_RANGE(0xfdf00, 0xfdfff) AM_WRITE( tpi6525_1_port_w)
	AM_RANGE(0xfe000, 0xfffff) AM_WRITE( SMH_ROM) AM_BASE( &cbmb_kernal )
ADDRESS_MAP_END


/*************************************
 *
 *  Input Ports
 *
 *************************************/


static INPUT_PORTS_START (cbm500)
	PORT_INCLUDE( cbmb_keyboard )			/* ROW0 -> ROW11 */

	PORT_INCLUDE( cbmb_special )			/* SPECIAL */
	
	PORT_INCLUDE( c64_controls )			/* JOY0, JOY1, PADDLE0 -> PADDLE3, TRACKX, TRACKY, TRACKIPT */

	PORT_INCLUDE( c64_control_cfg )			/* DSW0 */
INPUT_PORTS_END


static INPUT_PORTS_START (cbm600)
	PORT_INCLUDE( cbm500 )

	PORT_MODIFY("ROW0")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Graph  Norm") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RVS  Off") PORT_CODE(KEYCODE_HOME)
INPUT_PORTS_END


static INPUT_PORTS_START (cbm600pal)
	PORT_INCLUDE( cbm500 )

	PORT_MODIFY("ROW0")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Graph  Norm") PORT_CODE(KEYCODE_HOME)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ASCII  DIN") PORT_CODE(KEYCODE_PGUP)
INPUT_PORTS_END


static INPUT_PORTS_START (cbm700)
	PORT_INCLUDE( cbm500 )

	PORT_MODIFY("ROW0")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RVS  Off") PORT_CODE(KEYCODE_HOME)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Graph  Norm") PORT_CODE(KEYCODE_PGUP)
INPUT_PORTS_END



static const unsigned char cbm700_palette[] =
{
	0,0,0, /* black */
	0,0x80,0, /* green */
};

static const unsigned short cbmb_colortable[] = {
	0, 1
};

static const gfx_layout cbm600_charlayout =
{
	8,16,
	256,                                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8
	},
	8*16
};

static const gfx_layout cbm700_charlayout =
{
	9,16,
	256,                                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7,7 }, // 8.column will be cleared in cbm700_vh_start
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8
	},
	8*16
};

static GFXDECODE_START( cbm600 )
	GFXDECODE_ENTRY( REGION_GFX1, 0x0000, cbm600_charlayout, 0, 1 )
	GFXDECODE_ENTRY( REGION_GFX1, 0x1000, cbm600_charlayout, 0, 1 )
GFXDECODE_END

static GFXDECODE_START( cbm700 )
	GFXDECODE_ENTRY( REGION_GFX1, 0x0000, cbm700_charlayout, 0, 1 )
	GFXDECODE_ENTRY( REGION_GFX1, 0x1000, cbm700_charlayout, 0, 1 )
GFXDECODE_END

static PALETTE_INIT( cbm700 )
{
	int i;

	for ( i = 0; i < 2; i++ ) {
		palette_set_color_rgb(machine, i, cbm700_palette[i*3], cbm700_palette[i*3+1], cbm700_palette[i*3+2]);
	}
}

ROM_START (cbm610)
	ROM_REGION (0x100000, REGION_CPU1, 0)
	ROM_LOAD ("901243.04a", 0xf8000, 0x2000, CRC(b0dcb56d) SHA1(08d333208060ee2ce84d4532028d94f71c016b96))
	ROM_LOAD ("901242.04a", 0xfa000, 0x2000, CRC(de04ea4f) SHA1(7c6de17d46a3343dc597d9b9519cf63037b31908))
	ROM_LOAD ("901244.04a", 0xfe000, 0x2000, CRC(09a5667e) SHA1(abb26418b9e1614a8f52bdeee0822d4a96071439))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901237.01", 0x0000, 0x1000, CRC(1acf5098) SHA1(e63bf18da48e5a53c99ef127c1ae721333d1d102))
ROM_END

ROM_START (cbm620)
	ROM_REGION (0x100000, REGION_CPU1, 0)
    ROM_LOAD ("901241.03", 0xf8000, 0x2000, CRC(5c1f3347) SHA1(2d46be2cd89594b718cdd0a86d51b6f628343f42))
    ROM_LOAD ("901240.03", 0xfa000, 0x2000, CRC(72aa44e1) SHA1(0d7f77746290afba8d0abeb87c9caab9a3ad89ce))
    ROM_LOAD ("901244.04a", 0xfe000, 0x2000, CRC(09a5667e) SHA1(abb26418b9e1614a8f52bdeee0822d4a96071439))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901237.01", 0x0000, 0x1000, CRC(1acf5098) SHA1(e63bf18da48e5a53c99ef127c1ae721333d1d102))
ROM_END

ROM_START (cbm620hu)
	ROM_REGION (0x100000, REGION_CPU1, 0)
	ROM_LOAD ("610u60.bin", 0xf8000, 0x4000, CRC(8eed0d7e) SHA1(9d06c5c3c012204eaaef8b24b1801759b62bf57e))
	ROM_LOAD ("kernhun.bin", 0xfe000, 0x2000, CRC(0ea8ca4d) SHA1(9977c9f1136ee9c04963e0b50ae0c056efa5663f))
	ROM_REGION (0x2000, REGION_GFX1, 0)
	ROM_LOAD ("charhun.bin", 0x0000, 0x2000, CRC(1fb5e596) SHA1(3254e069f8691b30679b19a9505b6afdfedce6ac))
ROM_END

ROM_START (cbm710)
	ROM_REGION (0x100000, REGION_CPU1, 0)
	ROM_LOAD ("901243.04a", 0xf8000, 0x2000, CRC(b0dcb56d) SHA1(08d333208060ee2ce84d4532028d94f71c016b96))
	ROM_LOAD ("901242.04a", 0xfa000, 0x2000, CRC(de04ea4f) SHA1(7c6de17d46a3343dc597d9b9519cf63037b31908))
	ROM_LOAD ("901244.04a", 0xfe000, 0x2000, CRC(09a5667e) SHA1(abb26418b9e1614a8f52bdeee0822d4a96071439))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901232.01", 0x0000, 0x1000, CRC(3a350bc3) SHA1(e7f3cbc8e282f79a00c3e95d75c8d725ee3c6287))
ROM_END

ROM_START (cbm720)
	ROM_REGION (0x100000, REGION_CPU1, 0)
    ROM_LOAD ("901241.03", 0xf8000, 0x2000, CRC(5c1f3347) SHA1(2d46be2cd89594b718cdd0a86d51b6f628343f42))
    ROM_LOAD ("901240.03", 0xfa000, 0x2000, CRC(72aa44e1) SHA1(0d7f77746290afba8d0abeb87c9caab9a3ad89ce))
    ROM_LOAD ("901244.04a", 0xfe000, 0x2000, CRC(09a5667e) SHA1(abb26418b9e1614a8f52bdeee0822d4a96071439))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901232.01", 0x0000, 0x1000, CRC(3a350bc3) SHA1(e7f3cbc8e282f79a00c3e95d75c8d725ee3c6287))
ROM_END

ROM_START (cbm720se)
	ROM_REGION (0x100000, REGION_CPU1, 0)
    ROM_LOAD ("901241.03", 0xf8000, 0x2000, CRC(5c1f3347) SHA1(2d46be2cd89594b718cdd0a86d51b6f628343f42))
    ROM_LOAD ("901240.03", 0xfa000, 0x2000, CRC(72aa44e1) SHA1(0d7f77746290afba8d0abeb87c9caab9a3ad89ce))
    ROM_LOAD ("901244.03", 0xfe000, 0x2000, CRC(87bc142b) SHA1(fa711f6082741b05a9c80744f5aee68dc8c1dcf4))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901233.03", 0x0000, 0x1000, CRC(09518b19) SHA1(2e28491e31e2c0a3b6db388055216140a637cd09))
ROM_END


ROM_START (cbm500)
	ROM_REGION (0x101000, REGION_CPU1, 0)
	ROM_LOAD ("901236.02", 0xf8000, 0x2000, CRC(c62ab16f) SHA1(f50240407bade901144f7e9f489fa9c607834eca))
	ROM_LOAD ("901235.02", 0xfa000, 0x2000, CRC(20b7df33) SHA1(1b9a55f12f8cf025754d8029cc5324b474c35841))
	ROM_LOAD ("901234.02", 0xfe000, 0x2000, CRC(f46bbd2b) SHA1(097197d4d08e0b82e0466a5f1fbd49a24f3d2523))
	ROM_LOAD ("901225.01", 0x100000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
ROM_END

#if 0
/* in c16 and some other commodore machines:
   cbm version in kernel at 0xff80 (offset 0x3f80)
   0x80 means pal version */

    /* scrap */
	 /* 0000 1fff --> 0000
                      inverted 2000
        2000 3fff --> 4000
                      inverted 6000 */

	 /* 128 kb basic version */
    ROM_LOAD ("b128-8000.901243-02b.bin", 0xf8000, 0x2000, CRC(9d0366f9))
    ROM_LOAD ("b128-a000.901242-02b.bin", 0xfa000, 0x2000, CRC(837978b5))
	 /* merged df83bbb9 */

    ROM_LOAD ("b128-8000.901243-04a.bin", 0xf8000, 0x2000, CRC(b0dcb56d) SHA1(08d333208060ee2ce84d4532028d94f71c016b96))
    ROM_LOAD ("b128-a000.901242-04a.bin", 0xfa000, 0x2000, CRC(de04ea4f) SHA1(7c6de17d46a3343dc597d9b9519cf63037b31908))
	 /* merged a8ff9372 */

	 /* some additions to 901242-04a */
    ROM_LOAD ("b128-a000.901242-04_3f.bin", 0xfa000, 0x2000, CRC(5a680d2a))

     /* 256 kbyte basic version */
    ROM_LOAD ("b256-8000.610u60.bin", 0xf8000, 0x4000, CRC(8eed0d7e) SHA1(9d06c5c3c012204eaaef8b24b1801759b62bf57e))

    ROM_LOAD ("b256-8000.901241-03.bin", 0xf8000, 0x2000, CRC(5c1f3347) SHA1(2d46be2cd89594b718cdd0a86d51b6f628343f42))
    ROM_LOAD ("b256-a000.901240-03.bin", 0xfa000, 0x2000, CRC(72aa44e1) SHA1(0d7f77746290afba8d0abeb87c9caab9a3ad89ce))
	 /* merged 5db15870 */

     /* monitor instead of tape */
    ROM_LOAD ("kernal.901244-03b.bin", 0xfe000, 0x2000, CRC(4276dbba))
     /* modified 03b for usage of vc1541 on tape port ??? */
    ROM_LOAD ("kernelnew", 0xfe000, 0x2000, CRC(19bf247e))
    ROM_LOAD ("kernal.901244-04a.bin", 0xfe000, 0x2000, CRC(09a5667e) SHA1(abb26418b9e1614a8f52bdeee0822d4a96071439))
    ROM_LOAD ("kernal.hungarian.bin", 0xfe000, 0x2000, CRC(0ea8ca4d) SHA1(9977c9f1136ee9c04963e0b50ae0c056efa5663f))


	 /* 600 8x16 chars for 8x8 size
        128 ascii, 128 ascii graphics
        inversion logic in hardware */
    ROM_LOAD ("characters.901237-01.bin", 0x0000, 0x1000, CRC(1acf5098) SHA1(e63bf18da48e5a53c99ef127c1ae721333d1d102))
	 /* packing 128 national, national graphics, ascii, ascii graphics */
    ROM_LOAD ("characters-hungarian.bin", 0x0000, 0x2000, CRC(1fb5e596) SHA1(3254e069f8691b30679b19a9505b6afdfedce6ac))
	 /* 700 8x16 chars for 9x14 size*/
    ROM_LOAD ("characters.901232-01.bin", 0x0000, 0x1000, CRC(3a350bc3) SHA1(e7f3cbc8e282f79a00c3e95d75c8d725ee3c6287))

    ROM_LOAD ("vt52emu.bin", 0xf4000, 0x2000, CRC(b3b6173a))
	 /* load address 0xf4000? */
    ROM_LOAD ("moni.bin", 0xfe000, 0x2000, CRC(43b08d1f))

    ROM_LOAD ("profitext.bin", 0xf2000, 0x2000, CRC(ac622a2b))
	 /* address ?*/
    ROM_LOAD ("sfd1001-copy-u59.bin", 0xf1000, 0x1000, CRC(1c0fd916))

    ROM_LOAD ("", 0xfe000, 0x2000, CRC())

	 /* 500 */
	 /* 128 basic, other colors than cbmb series basic */
    ROM_LOAD ("basic-lo.901236-02.bin", 0xf8000, 0x2000, CRC(c62ab16f) SHA1(f50240407bade901144f7e9f489fa9c607834eca))
    ROM_LOAD ("basic-hi.901235-02.bin", 0xfa000, 0x2000, CRC(20b7df33) SHA1(1b9a55f12f8cf025754d8029cc5324b474c35841))
     /* monitor instead of tape */
    ROM_LOAD ("kernal.901234-02.bin", 0xfe000, 0x2000, CRC(f46bbd2b) SHA1(097197d4d08e0b82e0466a5f1fbd49a24f3d2523))
    ROM_LOAD ("characters.901225-01.bin", 0x100000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))

#endif

static const mc6845_interface cbm600_crtc = {
	"main",
	XTAL_18MHz / 8 /*?*/,	/*  I do not know if this is correct, please verify */
	8 /*?*/,
	NULL,
	cbm600_update_row,
	NULL,
	cbmb_display_enable_changed,
	NULL,
	NULL
};

static const mc6845_interface cbm700_crtc = {
	"main",
	XTAL_18MHz / 8 /*?*/,	/* I do not know if this is correct, please verify */
	9 /*?*/,
	NULL,
	cbm700_update_row,
	NULL,
	cbmb_display_enable_changed,
	NULL,
	NULL
};


static MACHINE_DRIVER_START( cbm600 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", M6509, 7833600)        /* 7.8336 Mhz */
	MDRV_CPU_PROGRAM_MAP(cbmb_readmem, cbmb_writemem)
	MDRV_INTERLEAVE(0)

	MDRV_MACHINE_RESET( cbmb )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MDRV_GFXDECODE( cbm600 )
	MDRV_PALETTE_LENGTH(sizeof (cbm700_palette) / sizeof (cbm700_palette[0]) / 3)
	MDRV_PALETTE_INIT( cbm700 )

	MDRV_DEVICE_ADD("crtc", MC6845)
	MDRV_DEVICE_CONFIG( cbm600_crtc )

	MDRV_VIDEO_START( cbmb_crtc )
	MDRV_VIDEO_UPDATE( cbmb_crtc )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sid6581", SID6581, 1000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_QUICKLOAD_ADD(cbmb, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cbm600pal )
	MDRV_IMPORT_FROM( cbm600 )
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cbm700 )
	MDRV_IMPORT_FROM( cbm600pal )
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_SIZE(720, 350)
	MDRV_SCREEN_VISIBLE_AREA(0, 720 - 1, 0, 350 - 1)
	MDRV_GFXDECODE( cbm700 )

	MDRV_DEVICE_MODIFY("crtc", MC6845)
	MDRV_DEVICE_CONFIG( cbm700_crtc )

	MDRV_VIDEO_START( cbm700 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cbm500 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", M6509, VIC6567_CLOCK)        /* 7.8336 Mhz */
	MDRV_CPU_PROGRAM_MAP(cbm500_readmem, cbm500_writemem)
	MDRV_CPU_PERIODIC_INT(vic2_raster_irq, VIC2_HRETRACERATE)
	MDRV_INTERLEAVE(0)

	MDRV_MACHINE_RESET( cbmb )

	/* video hardware */
	MDRV_IMPORT_FROM( vh_vic2 )
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sid6581", SID6581, 1000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_QUICKLOAD_ADD(cbm500, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)
MACHINE_DRIVER_END

static void cbmb_cbmcartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "crt,10,20,40,60"); break;

		default:										cbmcartslot_device_getinfo(devclass, state, info); break;
	}
}


static SYSTEM_CONFIG_START(cbmb)
	CONFIG_DEVICE(cbmb_cbmcartslot_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(cbm500)
	CONFIG_DEVICE(cbmb_cbmcartslot_getinfo)
SYSTEM_CONFIG_END

/*     YEAR     NAME      PARENT    COMPAT  MACHINE     INPUT       INIT        CONFIG  COMPANY                             FULLNAME */
COMP (1983,	cbm500,	  0,		0,		cbm500,		cbm500,		cbm500,		cbm500,	"Commodore Business Machines Co.",	"Commodore B128-40/Pet-II/P500 60Hz",		GAME_NOT_WORKING)
COMP (1983,	cbm610,   0,		0,		cbm600, 	cbm600, 	cbm600, 	cbmb,	"Commodore Business Machines Co.",  "Commodore B128-80LP/610 60Hz",             GAME_NOT_WORKING)
COMP (1983,	cbm620,	  cbm610,	0,		cbm600pal,	cbm600pal,	cbm600pal,	cbmb,	"Commodore Business Machines Co.",	"Commodore B256-80LP/620 50Hz",	GAME_NOT_WORKING)
COMP (1983,	cbm620hu, cbm610,	0,		cbm600pal,	cbm600pal,	cbm600hu,	cbmb,	"Commodore Business Machines Co.",	"Commodore B256-80LP/620 Hungarian 50Hz",	GAME_NOT_WORKING)
COMP (1983,	cbm710,   cbm610,   0,		cbm700, 	cbm700, 	cbm700, 	cbmb,	"Commodore Business Machines Co.",  "Commodore B128-80HP/710",                  GAME_NOT_WORKING)
COMP (1983,	cbm720,	  cbm610,	0,		cbm700,		cbm700,		cbm700,		cbmb,	"Commodore Business Machines Co.",	"Commodore B256-80HP/720",					GAME_NOT_WORKING)
COMP (1983,	cbm720se, cbm610,	0,		cbm700,     cbm700,		cbm700,		cbmb,	"Commodore Business Machines Co.",	"Commodore B256-80HP/720 Swedish/Finnish",	GAME_NOT_WORKING)
