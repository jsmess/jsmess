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
#include "video/generic.h"
#include "cpu/m6502/m6509.h"
#include "sound/sid6581.h"
#include "machine/6526cia.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "includes/tpi6525.h"
#include "includes/vic6567.h"
#include "includes/crtc6845.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"

#include "includes/cbmb.h"

static ADDRESS_MAP_START( cbmb_readmem , ADDRESS_SPACE_PROGRAM, 8)
//	AM_RANGE(0x00000, 0x00000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x00001, 0x00001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x00002, 0x0ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x10000, 0x10000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x10001, 0x10001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x10002, 0x1ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x20000, 0x20000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x20001, 0x20001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x20002, 0x2ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x30000, 0x30000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x30001, 0x30001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x30002, 0x3ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x40000, 0x40000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x40001, 0x40001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x40002, 0x4ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x50000, 0x50000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x50001, 0x50001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x50002, 0x5ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x60000, 0x60000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x60001, 0x60001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x60002, 0x6ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x70000, 0x70000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x70001, 0x70001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x70002, 0x7ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x80000, 0x80000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x80001, 0x80001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x80002, 0x8ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x90000, 0x90000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x90001, 0x90001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x90002, 0x9ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0xa0000, 0xa0000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0xa0001, 0xa0001) AM_READ( m6509_read_00001 )
	AM_RANGE(0xa0002, 0xaffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0xb0000, 0xb0000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0xb0001, 0xb0001) AM_READ( m6509_read_00001 )
	AM_RANGE(0xb0002, 0xbffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0xc0000, 0xc0000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0xc0001, 0xc0001) AM_READ( m6509_read_00001 )
	AM_RANGE(0xc0002, 0xcffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0xd0000, 0xd0000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0xd0001, 0xd0001) AM_READ( m6509_read_00001 )
	AM_RANGE(0xd0002, 0xdffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0xe0000, 0xe0000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0xe0001, 0xe0001) AM_READ( m6509_read_00001 )
	AM_RANGE(0xe0002, 0xeffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0xf0000, 0xf0000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0xf0001, 0xf0001) AM_READ( m6509_read_00001 )
	AM_RANGE(0xf0002, 0xf07ff) AM_READ( MRA8_RAM )
#if 0
	AM_RANGE(0xf0800, 0xf0fff) AM_READ( MRA8_ROM )
#endif
	AM_RANGE(0xf1000, 0xf1fff) AM_READ( MRA8_ROM ) /* cartridges or ram */
	AM_RANGE(0xf2000, 0xf3fff) AM_READ( MRA8_ROM ) /* cartridges or ram */
	AM_RANGE(0xf4000, 0xf5fff) AM_READ( MRA8_ROM )
	AM_RANGE(0xf6000, 0xf7fff) AM_READ( MRA8_ROM )
	AM_RANGE(0xf8000, 0xfbfff) AM_READ( MRA8_ROM )
	/*	{0xfc000, 0xfcfff, MRA8_ROM }, */
	AM_RANGE(0xfd000, 0xfd7ff) AM_READ( MRA8_ROM )
	AM_RANGE(0xfd800, 0xfd8ff) AM_READ( crtc6845_0_port_r )
	/* disk units */
	AM_RANGE(0xfda00, 0xfdaff) AM_READ( sid6581_0_port_r )
	/* db00 coprocessor */
	AM_RANGE(0xfdc00, 0xfdcff) AM_READ( cia_0_r )
	/* dd00 acia */
	AM_RANGE(0xfde00, 0xfdeff) AM_READ( tpi6525_0_port_r)
	AM_RANGE(0xfdf00, 0xfdfff) AM_READ( tpi6525_1_port_r)
	AM_RANGE(0xfe000, 0xfffff) AM_READ( MRA8_ROM )
ADDRESS_MAP_END

static ADDRESS_MAP_START( cbmb_writemem , ADDRESS_SPACE_PROGRAM, 8)
//	AM_RANGE(0x00000, 0x00000) AM_WRITE( m6509_write_00000) AM_BASE( &cbmb_memory )
//	AM_RANGE(0x00001, 0x00001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x00002, 0x0ffff) AM_WRITE( MWA8_NOP )
//	AM_RANGE(0x10000, 0x10000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x10001, 0x10001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x10002, 0x1ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x20000, 0x20000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x20001, 0x20001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x20002, 0x2ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x30000, 0x30000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x30001, 0x30001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x30002, 0x3ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x40000, 0x40000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x40001, 0x40001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x40002, 0x4ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x50000, 0x50000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x50001, 0x50001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x50002, 0x5ffff) AM_WRITE( MWA8_NOP )
//	AM_RANGE(0x60000, 0x60000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x60001, 0x60001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x60002, 0x6ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x70000, 0x70000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x70001, 0x70001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x70002, 0x7ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x80000, 0x80000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x80001, 0x80001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x80002, 0x8ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x90000, 0x90000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x90001, 0x90001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x90002, 0x9ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0xa0000, 0xa0000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0xa0001, 0xa0001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0xa0002, 0xaffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0xb0000, 0xb0000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0xb0001, 0xb0001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0xb0002, 0xbffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0xc0000, 0xc0000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0xc0001, 0xc0001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0xc0002, 0xcffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0xd0000, 0xd0000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0xd0001, 0xd0001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0xd0002, 0xdffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0xe0000, 0xe0000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0xe0001, 0xe0001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0xe0002, 0xeffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0xf0000, 0xf0000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0xf0001, 0xf0001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0xf0002, 0xf07ff) AM_WRITE( MWA8_RAM )
	AM_RANGE(0xf1000, 0xf1fff) AM_WRITE( MWA8_ROM ) /* cartridges */
	AM_RANGE(0xf2000, 0xf3fff) AM_WRITE( MWA8_ROM ) /* cartridges */
	AM_RANGE(0xf4000, 0xf5fff) AM_WRITE( MWA8_ROM )
	AM_RANGE(0xf6000, 0xf7fff) AM_WRITE( MWA8_ROM )
	AM_RANGE(0xf8000, 0xfbfff) AM_WRITE( MWA8_ROM) AM_BASE( &cbmb_basic )
	AM_RANGE(0xfd000, 0xfd7ff) AM_WRITE( videoram_w) AM_BASE( &videoram) AM_SIZE(&videoram_size ) /* VIDEORAM */
	AM_RANGE(0xfd800, 0xfd8ff) AM_WRITE( crtc6845_0_port_w )
	/* disk units */
	AM_RANGE(0xfda00, 0xfdaff) AM_WRITE( sid6581_0_port_w)
	/* db00 coprocessor */
	AM_RANGE(0xfdc00, 0xfdcff) AM_WRITE( cia_0_w)
	/* dd00 acia */
	AM_RANGE(0xfde00, 0xfdeff) AM_WRITE( tpi6525_0_port_w)
	AM_RANGE(0xfdf00, 0xfdfff) AM_WRITE( tpi6525_1_port_w)
	AM_RANGE(0xfe000, 0xfffff) AM_WRITE( MWA8_ROM) AM_BASE( &cbmb_kernal )
ADDRESS_MAP_END

static ADDRESS_MAP_START( cbm500_readmem , ADDRESS_SPACE_PROGRAM, 8)
//	AM_RANGE(0x00000, 0x00000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x00001, 0x00001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x00002, 0x0ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x10000, 0x10000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x10001, 0x10001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x10002, 0x1ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x20000, 0x20000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x20001, 0x20001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x20002, 0x2ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x30000, 0x30000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x30001, 0x30001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x30002, 0x3ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x40000, 0x40000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x40001, 0x40001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x40002, 0x4ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x50000, 0x50000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x50001, 0x50001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x50002, 0x5ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x60000, 0x60000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x60001, 0x60001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x60002, 0x6ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x70000, 0x70000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x70001, 0x70001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x70002, 0x7ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x80000, 0x80000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x80001, 0x80001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x80002, 0x8ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0x90000, 0x90000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0x90001, 0x90001) AM_READ( m6509_read_00001 )
	AM_RANGE(0x90002, 0x9ffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0xa0000, 0xa0000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0xa0001, 0xa0001) AM_READ( m6509_read_00001 )
	AM_RANGE(0xa0002, 0xaffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0xb0000, 0xb0000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0xb0001, 0xb0001) AM_READ( m6509_read_00001 )
	AM_RANGE(0xb0002, 0xbffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0xc0000, 0xc0000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0xc0001, 0xc0001) AM_READ( m6509_read_00001 )
	AM_RANGE(0xc0002, 0xcffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0xd0000, 0xd0000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0xd0001, 0xd0001) AM_READ( m6509_read_00001 )
	AM_RANGE(0xd0002, 0xdffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0xe0000, 0xe0000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0xe0001, 0xe0001) AM_READ( m6509_read_00001 )
	AM_RANGE(0xe0002, 0xeffff) AM_READ( MRA8_RAM)
//	AM_RANGE(0xf0000, 0xf0000) AM_READ( m6509_read_00000 )
//	AM_RANGE(0xf0001, 0xf0001) AM_READ( m6509_read_00001 )
	AM_RANGE(0xf0002, 0xf07ff) AM_READ( MRA8_RAM )
#if 0
	AM_RANGE(0xf0800, 0xf0fff) AM_READ( MRA8_ROM )
#endif
	AM_RANGE(0xf1000, 0xf1fff) AM_READ( MRA8_ROM ) /* cartridges or ram */
	AM_RANGE(0xf2000, 0xf3fff) AM_READ( MRA8_ROM ) /* cartridges or ram */
	AM_RANGE(0xf4000, 0xf5fff) AM_READ( MRA8_ROM )
	AM_RANGE(0xf6000, 0xf7fff) AM_READ( MRA8_ROM )
	AM_RANGE(0xf8000, 0xfbfff) AM_READ( MRA8_ROM )
	/*	{0xfc000, 0xfcfff, MRA8_ROM }, */
	AM_RANGE(0xfd000, 0xfd3ff) AM_READ( MRA8_RAM ) /* videoram */
	AM_RANGE(0xfd400, 0xfd7ff) AM_READ( MRA8_RAM ) /* colorram */
	AM_RANGE(0xfd800, 0xfd8ff) AM_READ( vic2_port_r )
	/* disk units */
	AM_RANGE(0xfda00, 0xfdaff) AM_READ( sid6581_0_port_r )
	/* db00 coprocessor */
	AM_RANGE(0xfdc00, 0xfdcff) AM_READ( cia_0_r )
	/* dd00 acia */
	AM_RANGE(0xfde00, 0xfdeff) AM_READ( tpi6525_0_port_r)
	AM_RANGE(0xfdf00, 0xfdfff) AM_READ( tpi6525_1_port_r)
	AM_RANGE(0xfe000, 0xfffff) AM_READ( MRA8_ROM )
ADDRESS_MAP_END

static ADDRESS_MAP_START( cbm500_writemem , ADDRESS_SPACE_PROGRAM, 8)
//	AM_RANGE(0x00000, 0x00000) AM_WRITE( m6509_write_00000) AM_BASE( &cbmb_memory )
//	AM_RANGE(0x00001, 0x00001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x00002, 0x0ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x10000, 0x10000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x10001, 0x10001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x10002, 0x1ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x20000, 0x20000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x20001, 0x20001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x20002, 0x2ffff) AM_WRITE( MWA8_NOP )
//	AM_RANGE(0x30000, 0x30000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x30001, 0x30001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x30002, 0x3ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x40000, 0x40000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x40001, 0x40001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x40002, 0x4ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x50000, 0x50000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x50001, 0x50001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x50002, 0x5ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x60000, 0x60000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x60001, 0x60001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x60002, 0x6ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x70000, 0x70000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x70001, 0x70001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x70002, 0x7ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0x80000, 0x80000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x80001, 0x80001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x80002, 0x8ffff) AM_WRITE( MWA8_NOP )
//	AM_RANGE(0x90000, 0x90000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0x90001, 0x90001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0x90002, 0x9ffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0xa0000, 0xa0000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0xa0001, 0xa0001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0xa0002, 0xaffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0xb0000, 0xb0000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0xb0001, 0xb0001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0xb0002, 0xbffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0xc0000, 0xc0000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0xc0001, 0xc0001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0xc0002, 0xcffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0xd0000, 0xd0000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0xd0001, 0xd0001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0xd0002, 0xdffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0xe0000, 0xe0000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0xe0001, 0xe0001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0xe0002, 0xeffff) AM_WRITE( MWA8_RAM )
//	AM_RANGE(0xf0000, 0xf0000) AM_WRITE( m6509_write_00000 )
//	AM_RANGE(0xf0001, 0xf0001) AM_WRITE( m6509_write_00001 )
	AM_RANGE(0xf0002, 0xf07ff) AM_WRITE( MWA8_RAM )
	AM_RANGE(0xf1000, 0xf1fff) AM_WRITE( MWA8_ROM ) /* cartridges */
	AM_RANGE(0xf2000, 0xf3fff) AM_WRITE( MWA8_ROM ) /* cartridges */
	AM_RANGE(0xf4000, 0xf5fff) AM_WRITE( MWA8_ROM )
	AM_RANGE(0xf6000, 0xf7fff) AM_WRITE( MWA8_ROM )
	AM_RANGE(0xf8000, 0xfbfff) AM_WRITE( MWA8_ROM) AM_BASE( &cbmb_basic )
	AM_RANGE(0xfd000, 0xfd3ff) AM_WRITE( MWA8_RAM) AM_BASE( &cbmb_videoram )
	AM_RANGE(0xfd400, 0xfd7ff) AM_WRITE( cbmb_colorram_w) AM_BASE( &cbmb_colorram )
	AM_RANGE(0xfd800, 0xfd8ff) AM_WRITE( vic2_port_w )
	/* disk units */
	AM_RANGE(0xfda00, 0xfdaff) AM_WRITE( sid6581_0_port_w)
	/* db00 coprocessor */
	AM_RANGE(0xfdc00, 0xfdcff) AM_WRITE( cia_0_w)
	/* dd00 acia */
	AM_RANGE(0xfde00, 0xfdeff) AM_WRITE( tpi6525_0_port_w)
	AM_RANGE(0xfdf00, 0xfdfff) AM_WRITE( tpi6525_1_port_w)
	AM_RANGE(0xfe000, 0xfffff) AM_WRITE( MWA8_ROM) AM_BASE( &cbmb_kernal )
ADDRESS_MAP_END

#define DIPS_HELPER(bit, name, keycode) \
   PORT_BIT(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(name) PORT_CODE(keycode)

#define CBMB_KEYBOARD \
	PORT_START \
	DIPS_HELPER( 0x8000, "ESC", KEYCODE_ESC)\
	DIPS_HELPER( 0x4000, "1 !", KEYCODE_1)\
	DIPS_HELPER( 0x2000, "2 @", KEYCODE_2)\
	DIPS_HELPER( 0x1000, "3 #", KEYCODE_3)\
	DIPS_HELPER( 0x0800, "4 $", KEYCODE_4)\
	DIPS_HELPER( 0x0400, "5 %", KEYCODE_5)\
	DIPS_HELPER( 0x0200, "6 Arrow-Up", KEYCODE_6)\
	DIPS_HELPER( 0x0100, "7 &", KEYCODE_7)\
	DIPS_HELPER( 0x0080, "8 *", KEYCODE_8)\
	DIPS_HELPER( 0x0040, "9 (", KEYCODE_9)\
	DIPS_HELPER( 0x0020, "0 )", KEYCODE_0)\
	DIPS_HELPER( 0x0010, "-", KEYCODE_MINUS)\
	DIPS_HELPER( 0x0008, "= +", KEYCODE_EQUALS)\
	DIPS_HELPER( 0x0004, "Arrow-Left Pound", KEYCODE_TILDE)\
	DIPS_HELPER( 0x0002, "DEL INS", KEYCODE_BACKSPACE)\
	DIPS_HELPER( 0x0001, "TAB", KEYCODE_TAB)\
	PORT_START \
	DIPS_HELPER( 0x8000, "Q", KEYCODE_Q)\
	DIPS_HELPER( 0x4000, "W", KEYCODE_W)\
	DIPS_HELPER( 0x2000, "E", KEYCODE_E)\
	DIPS_HELPER( 0x1000, "R", KEYCODE_R)\
	DIPS_HELPER( 0x0800, "T", KEYCODE_T)\
	DIPS_HELPER( 0x0400, "Y", KEYCODE_Y)\
	DIPS_HELPER( 0x0200, "U", KEYCODE_U)\
	DIPS_HELPER( 0x0100, "I", KEYCODE_I)\
	DIPS_HELPER( 0x0080, "O", KEYCODE_O)\
	DIPS_HELPER( 0x0040, "P", KEYCODE_P)\
	DIPS_HELPER( 0x0020, "[", KEYCODE_OPENBRACE)\
        DIPS_HELPER( 0x0010, "]", KEYCODE_CLOSEBRACE)\
        DIPS_HELPER( 0x0008, "RETURN",KEYCODE_ENTER)\
	PORT_DIPNAME( 0x0004, 0, "(shift)SHIFT-LOCK (switch)") PORT_CODE(KEYCODE_CAPSLOCK)\
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )\
	 PORT_DIPSETTING(4, DEF_STR( On ) )\
	DIPS_HELPER( 0x0002, "A", KEYCODE_A)\
	DIPS_HELPER( 0x0001, "S", KEYCODE_S)\
	PORT_START \
	DIPS_HELPER( 0x8000, "D", KEYCODE_D)\
	DIPS_HELPER( 0x4000, "F", KEYCODE_F)\
	DIPS_HELPER( 0x2000, "G", KEYCODE_G)\
	DIPS_HELPER( 0x1000, "H", KEYCODE_H)\
	DIPS_HELPER( 0x0800, "J", KEYCODE_J)\
	DIPS_HELPER( 0x0400, "K", KEYCODE_K)\
	DIPS_HELPER( 0x0200, "L", KEYCODE_L)\
	DIPS_HELPER( 0x0100, "; :", KEYCODE_COLON)\
	DIPS_HELPER( 0x0080, "\' \"", KEYCODE_QUOTE)\
	DIPS_HELPER( 0x0040, "PI", KEYCODE_BACKSLASH)\
	DIPS_HELPER( 0x0020, "(shift)Left-Shift", KEYCODE_LSHIFT)\
	DIPS_HELPER( 0x0010, "Z", KEYCODE_Z)\
	DIPS_HELPER( 0x0008, "X", KEYCODE_X)\
	DIPS_HELPER( 0x0004, "C", KEYCODE_C)\
	DIPS_HELPER( 0x0002, "V", KEYCODE_V)\
	DIPS_HELPER( 0x0001, "B", KEYCODE_B)\
	PORT_START \
	DIPS_HELPER( 0x8000, "N", KEYCODE_N)\
	DIPS_HELPER( 0x4000, "M", KEYCODE_M)\
	DIPS_HELPER( 0x2000, ", <", KEYCODE_COMMA)\
	DIPS_HELPER( 0x1000, ". >", KEYCODE_STOP)\
	DIPS_HELPER( 0x0800, "/ ?", KEYCODE_SLASH)\
	DIPS_HELPER( 0x0400, "(shift)Right-Shift", KEYCODE_RSHIFT)\
	DIPS_HELPER( 0x0200, "CBM", KEYCODE_RALT)\
	DIPS_HELPER( 0x0100, "CTRL", KEYCODE_RCONTROL)\
	DIPS_HELPER( 0x0080, "Space", KEYCODE_SPACE)\
	DIPS_HELPER( 0x0040, "f1", KEYCODE_F1)\
	DIPS_HELPER( 0x0020, "f2", KEYCODE_F2)\
	DIPS_HELPER( 0x0010, "f3", KEYCODE_F3)\
	DIPS_HELPER( 0x0008, "f4", KEYCODE_F4)\
	DIPS_HELPER( 0x0004, "f5", KEYCODE_F5)\
	DIPS_HELPER( 0x0002, "f6", KEYCODE_F6)\
	DIPS_HELPER( 0x0001, "f7", KEYCODE_F7)\
	PORT_START \
	DIPS_HELPER( 0x8000, "f8", KEYCODE_F8)\
	DIPS_HELPER( 0x4000, "f9", KEYCODE_F9)\
	DIPS_HELPER( 0x2000, "f10", KEYCODE_F10)\
	DIPS_HELPER( 0x1000, "CRSR Down", KEYCODE_DOWN)\
	DIPS_HELPER( 0x0800, "CRSR Up", KEYCODE_UP)\
	DIPS_HELPER( 0x0400, "CRSR Left", KEYCODE_LEFT)\
	DIPS_HELPER( 0x0200, "CRSR Right", KEYCODE_RIGHT)\
	DIPS_HELPER( 0x0100, "HOME CLR", KEYCODE_INSERT)

#define CBMB_KEYBOARD2 \
	DIPS_HELPER( 0x0020, "STOP RUN", KEYCODE_DEL)\
	DIPS_HELPER( 0x0010, "NUM ?", KEYCODE_END)\
	DIPS_HELPER( 0x0008, "NUM CE", KEYCODE_PGDN)\
	DIPS_HELPER( 0x0004, "NUM *", KEYCODE_ASTERISK)\
	DIPS_HELPER( 0x0002, "NUM /", KEYCODE_SLASH_PAD)\
	DIPS_HELPER( 0x0001, "NUM 7", KEYCODE_7_PAD)\
	PORT_START \
	DIPS_HELPER( 0x8000, "NUM 8", KEYCODE_8_PAD)\
	DIPS_HELPER( 0x4000, "NUM 9", KEYCODE_9_PAD)\
	DIPS_HELPER( 0x2000, "NUM -", KEYCODE_MINUS_PAD)\
	DIPS_HELPER( 0x1000, "NUM 4", KEYCODE_4_PAD)\
	DIPS_HELPER( 0x0800, "NUM 5", KEYCODE_5_PAD)\
	DIPS_HELPER( 0x0400, "NUM 6", KEYCODE_6_PAD)\
	DIPS_HELPER( 0x0200, "NUM +", KEYCODE_PLUS_PAD)\
	DIPS_HELPER( 0x0100, "NUM 1", KEYCODE_1_PAD)\
	DIPS_HELPER( 0x0080, "NUM 2", KEYCODE_2_PAD)\
	DIPS_HELPER( 0x0040, "NUM 3", KEYCODE_3_PAD)\
	DIPS_HELPER( 0x0020, "NUM Enter", KEYCODE_ENTER_PAD)\
	DIPS_HELPER( 0x0010, "NUM 0", KEYCODE_0_PAD)\
	DIPS_HELPER( 0x0008, "NUM .", KEYCODE_DEL_PAD)\
	DIPS_HELPER( 0x0004, "NUM 00", KEYCODE_NUMLOCK)\

INPUT_PORTS_START (cbm600)
     CBMB_KEYBOARD
     DIPS_HELPER( 0x0080, "RVS OFF", KEYCODE_HOME)
	 DIPS_HELPER( 0x0040, "GRAPH NORM", KEYCODE_PGUP)
	 CBMB_KEYBOARD2
     PORT_START
	 DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
#ifdef PET_TEST_CODE
	 PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	 PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	 DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	 DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	 DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
#endif
	 PORT_BIT (0x200, 0x200, IPT_UNUSED) /* ntsc */
	 PORT_BIT (0x100, 0x000, IPT_UNUSED) /* cbm600 */
#ifdef PET_TEST_CODE
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
#endif
INPUT_PORTS_END

INPUT_PORTS_START (cbm600pal)
     CBMB_KEYBOARD
     DIPS_HELPER( 0x0080, "GRAPH NORM", KEYCODE_HOME)
	 DIPS_HELPER( 0x0040, "ASCII DIN", KEYCODE_PGUP)
	 CBMB_KEYBOARD2
     PORT_START
	 DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
#ifdef PET_TEST_CODE
	 PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	 PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	 DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	 DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	 DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
#endif
	 PORT_BIT (0x200, 0x000, IPT_UNUSED) /* pal */
	 PORT_BIT (0x100, 0x000, IPT_UNUSED) /* cbm600 */
#ifdef PET_TEST_CODE
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
#endif
INPUT_PORTS_END

INPUT_PORTS_START (cbm700)
	CBMB_KEYBOARD
    DIPS_HELPER( 0x0080, "RVS OFF", KEYCODE_HOME)
	DIPS_HELPER( 0x0040, "GRAPH NORM", KEYCODE_PGUP)
	CBMB_KEYBOARD2
    PORT_START
    DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
#ifdef PET_TEST_CODE
	PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
#endif
	PORT_BIT (0x200, 0x000, IPT_UNUSED) /* not used */
	PORT_BIT (0x100, 0x100, IPT_UNUSED) /* cbm700 */
#ifdef PET_TEST_CODE
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
#endif
INPUT_PORTS_END

INPUT_PORTS_START (cbm500)
	CBMB_KEYBOARD
    DIPS_HELPER( 0x0080, "RVS OFF", KEYCODE_HOME)
	DIPS_HELPER( 0x0040, "GRAPH NORM", KEYCODE_PGUP)
	CBMB_KEYBOARD2
    PORT_START
    DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
#ifdef PET_TEST_CODE
	PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
#endif
	PORT_BIT (0x200, 0x000, IPT_UNUSED) /* not used */
	PORT_BIT (0x100, 0x000, IPT_UNUSED) /* not used */
#ifdef PET_TEST_CODE
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
#endif
	/*C64_DIPS */
INPUT_PORTS_END

unsigned char cbm700_palette[] =
{
	0,0,0, /* black */
	0,0x80,0, /* green */
};

static unsigned short cbmb_colortable[] = {
	0, 1
};

static gfx_layout cbm600_charlayout =
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

static gfx_layout cbm700_charlayout =
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

static gfx_decode cbm600_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &cbm600_charlayout, 0, 1 },
	{ REGION_GFX1, 0x1000, &cbm600_charlayout, 0, 1 },
    { -1 } /* end of array */
};

static gfx_decode cbm700_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &cbm700_charlayout, 0, 1 },
	{ REGION_GFX1, 0x1000, &cbm700_charlayout, 0, 1 },
    { -1 } /* end of array */
};

static PALETTE_INIT( cbm700 )
{
	palette_set_colors(machine, 0, cbm700_palette, sizeof(cbm700_palette) / 3);
    memcpy(colortable,cbmb_colortable,sizeof(cbmb_colortable));
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


static MACHINE_DRIVER_START( cbm600 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6509, 7833600)        /* 7.8336 Mhz */
	MDRV_CPU_PROGRAM_MAP(cbmb_readmem, cbmb_writemem)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(0)

	MDRV_MACHINE_RESET( cbmb )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MDRV_GFXDECODE( cbm600_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof (cbm700_palette) / sizeof (cbm700_palette[0]) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (cbmb_colortable) / sizeof(cbmb_colortable[0]))
	MDRV_PALETTE_INIT( cbm700 )

	MDRV_VIDEO_START( generic )
	MDRV_VIDEO_UPDATE( cbmb )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SID6581, 1000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cbm600pal )
	MDRV_IMPORT_FROM( cbm600 )
	MDRV_SCREEN_REFRESH_RATE(50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cbm700 )
	MDRV_IMPORT_FROM( cbm600pal )
	MDRV_SCREEN_SIZE(720, 350)
	MDRV_SCREEN_VISIBLE_AREA(0, 720 - 1, 0, 350 - 1)
	MDRV_GFXDECODE( cbm700_gfxdecodeinfo )

	MDRV_VIDEO_START( cbm700 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cbm500 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6509, VIC6567_CLOCK)        /* 7.8336 Mhz */
	MDRV_CPU_PROGRAM_MAP(cbm500_readmem, cbm500_writemem)
	MDRV_CPU_PERIODIC_INT(vic2_raster_irq, TIME_IN_HZ(VIC2_HRETRACERATE))
	MDRV_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(0)

	MDRV_MACHINE_RESET( cbmb )

	MDRV_IMPORT_FROM( vh_vic2 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SID6581, 1000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static DRIVER_INIT( cbm500 )	{ cbm500_driver_init(); }
static DRIVER_INIT( cbm600 )	{ cbm600_driver_init(); }
static DRIVER_INIT( cbm600hu )	{ cbm600hu_driver_init(); }
static DRIVER_INIT( cbm600pal )	{ cbm600pal_driver_init(); }
static DRIVER_INIT( cbm700 )	{ cbm700_driver_init(); }

static void cbmb_cbmcartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "crt,10,20,40,60"); break;

		default:										cbmcartslot_device_getinfo(devclass, state, info); break;
	}
}

static void cbmb_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "p00,prg"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_cbmb; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_QUICKLOAD_DELAY:				info->d = CBM_QUICKLOAD_DELAY; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(cbmb)
	CONFIG_DEVICE(cbmb_cbmcartslot_getinfo)
	CONFIG_DEVICE(cbmb_quickload_getinfo)
#ifdef PET_TEST_CODE
	/* monitor OR tape routine in kernel */
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
#endif
SYSTEM_CONFIG_END

static void cbm500_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "p00,prg"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_cbm500; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_QUICKLOAD_DELAY:				info->d = CBM_QUICKLOAD_DELAY; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(cbm500)
	CONFIG_DEVICE(cbmb_cbmcartslot_getinfo)
	CONFIG_DEVICE(cbm500_quickload_getinfo)
#ifdef PET_TEST_CODE
	/* monitor OR tape routine in kernel */
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
#endif
SYSTEM_CONFIG_END

/*     YEAR		NAME	  PARENT	COMPAT	MACHINE		INPUT		INIT		CONFIG  COMPANY								FULLNAME */
COMP (1983,	cbm500,	  0,		0,		cbm500,		cbm500,		cbm500,		cbm500,	"Commodore Business Machines Co.",	"Commodore B128-40/Pet-II/P500 60Hz",		GAME_NOT_WORKING)
COMP (1983,	cbm610,   0,		0,		cbm600, 	cbm600, 	cbm600, 	cbmb,	"Commodore Business Machines Co.",  "Commodore B128-80LP/610 60Hz",             GAME_NOT_WORKING)
COMP (1983,	cbm620,	  cbm610,	0,		cbm600pal,	cbm600pal,	cbm600pal,	cbmb,	"Commodore Business Machines Co.",	"Commodore B256-80LP/620 50Hz",	GAME_NOT_WORKING)
COMP (1983,	cbm620hu, cbm610,	0,		cbm600pal,	cbm600pal,	cbm600hu,	cbmb,	"Commodore Business Machines Co.",	"Commodore B256-80LP/620 Hungarian 50Hz",	GAME_NOT_WORKING)
COMP (1983,	cbm710,   cbm610,   0,		cbm700, 	cbm700, 	cbm700, 	cbmb,	"Commodore Business Machines Co.",  "Commodore B128-80HP/710",                  GAME_NOT_WORKING)
COMP (1983,	cbm720,	  cbm610,	0,		cbm700,		cbm700,		cbm700,		cbmb,	"Commodore Business Machines Co.",	"Commodore B256-80HP/720",					GAME_NOT_WORKING)
COMP (1983,	cbm720se, cbm610,	0,		cbm700,     cbm700,		cbm700,		cbmb,	"Commodore Business Machines Co.",	"Commodore B256-80HP/720 Swedish/Finnish",	GAME_NOT_WORKING)
#if 0
COMP (1983,	cbm730,   cbm610,   0,		cbmbx, 		cbmb, 		cbmb, 		cbmb,    "Commodore Business Machines Co.",	"Commodore BX128-80HP/BX256-80HP/730", GAME_NOT_WORKING)
#endif
