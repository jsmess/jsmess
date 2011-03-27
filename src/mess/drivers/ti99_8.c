/*
    MESS Driver for TI-99/8 Computer.
    Raphael Nabet, 2003.
*/
/*
    TI-99/8 preliminary info:

Name: Texas Instruments Computer TI-99/8 (no "Home")

References:
    * machine room <http://...>
    * TI99/8 user manual
    * TI99/8 schematics
    * TI99/8 ROM source code
    * Message on TI99 yahoo group for CPU info

General:
    * a few dozen units were built in 1983, never released
    * CPU is a custom variant of tms9995 (part code MP9537): the 16-bit RAM and
      (presumably) the on-chip decrementer are disabled
    * 220kb(?) of ROM, including monitor, GPL interpreter, TI-extended basic
      II, and a P-code interpreter with a few utilities.  More specifically:
      - 32kb system ROM with GPL interpreter, TI-extended basic II and a few
        utilities (no dump, but 90% of source code is available and has been
        compiled)
      - 18kb system GROMs, with monitor and TI-extended basic II (no dump,
        but source code is available and has been compiled)
      - 4(???)kb DSR ROM for hexbus (no dump)
      - 32(?)kb speech ROM: contents are slightly different from the 99/4(a)
        speech ROMs, due to the use of a tms5220 speech synthesizer instead of
        the older tms0285 (no dump, but 99/4(a) speech ROMs should work mostly
        OK)
      - 12(???)kb ROM with PCode interpreter (no dump)
      - 2(3???)*48kb of GROMs with PCode data files (no dump)
    * 2kb SRAM (16 bytes of which are hidden), 64kb DRAM (expandable to almost
      16MBytes), 16kb vdp RAM
    * tms9118 vdp (similar to tms9918a, slightly different bus interface and
      timings)
    * I/O
      - 50-key keyboard, plus 2 optional joysticks
      - sound and speech (both ti99/4(a)-like)
      - Hex-Bus
      - Cassette
    * cartridge port on the top
    * 50-pin(?) expansion port on the back
    * Programs can enable/disable the ROM and memory mapped register areas.

Mapper:
    Mapper has 4kb page size (-> 16 pages per map file), 32 bits per page
    entry.  Address bits A0-A3 are the page index, whereas bits A4-A15 are the
    offset in the page.  Physical address space is 16Mbytes.  All pages are 4
    kBytes in lenght, and they can start anywhere in the 24-bit physical
    address space.  The mapper can load any of 4 map files from SRAM by DMA.
    Map file 0 is used by BIOS, file 1 by memory XOPs(?), file 2 by P-code
    interpreter(???).

    Format of map table entry:
    * bit 0: WTPROT: page is write protected if 1
    * bit 1: XPROT: page is execute protected if 1
    * bit 2: RDPROT: page is read protected if 1
    * bit 3: reserved, value is ignored
    * bits 4-7: reserved, always forced to 0
    * bits 8-23: page base address in 24-bit virtual address space

    Format of mapper control register:
    * bit 0-4: unused???
    * bit 5-6: map file to load/save (0 for file 0, 1 for file 1, etc.)
    * bit 7: 0 -> load map file from RAM, 1 -> save map file to RAM

    Format of mapper status register (cleared by read):
    * bit 0: WPE - Write-Protect Error
    * bit 1: XCE - eXeCute Error
    * bit 2: RPE - Read-Protect Error
    * bits 3-7: unused???

    Memory error interrupts are enabled by setting WTPROT/XPROT/RDPROT.  When
    an error occurs, the tms9901 INT1* pin is pulled low (active).  The pin
    remains low until the mapper status register is read.

24-bit address map:
    * >000000->00ffff: console RAM
    * >010000->feffff: expansion?
    * >ff0000->ff0fff: empty???
    * >ff1000->ff3fff: unused???
    * >ff4000->ff5fff: DSR space
    * >ff6000->ff7fff: cartridge space
    * >ff8000->ff9fff(???): >4000 ROM (normally enabled with a write to CRU >2700)
    * >ffa000->ffbfff(?): >2000 ROM
    * >ffc000->ffdfff(?): >6000 ROM


CRU map:
    Since the tms9995 supports full 15-bit CRU addresses, the >1000->17ff
    (>2000->2fff) range was assigned to support up to 16 extra expansion slot.
    The good thing with using >1000->17ff is the fact that older expansion
    cards that only decode 12 address bits will think that addresses
    >1000->17ff refer to internal TI99 peripherals (>000->7ff range), which
    suppresses any risk of bus contention.
    * >0000->001f (>0000->003e): tms9901
      - P4: 1 -> MMD (Memory Mapped Devices?) at >8000, ROM enabled
      - P5: 1 -> no P-CODE GROMs
    * >0800->17ff (>1000->2ffe): Peripheral CRU space
    * >1380->13ff (>2700->27fe): Internal DSR, with two output bits:
      - >2700: Internal DSR select (parts of Basic and various utilities)
      - >2702: SBO -> hardware reset


Memory map (TMS9901 P4 == 1):
    When TMS9901 P4 output is set, locations >8000->9fff are ignored by mapper.
    * >8000->83ff: SRAM (>8000->80ff is used by the mapper DMA controller
      to hold four map files) (r/w)
    * >8400: sound port (w)
    * >8410->87ff: SRAM (r/w)
    * >8800: VDP data read port (r)
    * >8802: VDP status read port (r)
    * >8810: memory mapper status and control registers (r/w)
    * >8c00: VDP data write port (w)
    * >8c02: VDP address and register write port (w)
    * >9000: speech synthesizer read port (r)
    * >9400: speech synthesizer write port (w)
    * >9800 GPL data read port (r)
    * >9802 GPL address read port (r)
    * >9c00 GPL data write port -- unused (w)
    * >9c02 GPL address write port (w)


Memory map (TMS9901 P5 == 0):
    When TMS9901 P5 output is cleared, locations >f840->f8ff(?) are ignored by
    mapper.
    * >f840: data port for P-code grom library 0 (r?)
    * >f880: data port for P-code grom library 1 (r?)
    * >f8c0: data port for P-code grom library 2 (r?)
    * >f842: address port for P-code grom library 0 (r/w?)
    * >f882: address port for P-code grom library 1 (r/w?)
    * >f8c2: address port for P-code grom library 2 (r/w?)


Cassette interface:
    Identical to ti99/4(a), except that the CS2 unit is not implemented.


Keyboard interface:
    The keyboard interface uses the console tms9901 PSI, but the pin assignment
    and key matrix are different from both 99/4 and 99/4a.
    - P0-P3: column select
    - INT6*-INT11*: row inputs (int6* is only used for joystick fire)

ROM file contents:
  0000-1fff ROM0                0x0000 (logical address)
  2000-3fff ROM1                0xffa000 - 0xffbfff
  4000-5fff DSR1                0xff4000 (TTS)
  6000-7fff ROM1a               0xffc000 - 0xffdfff
  8000-9fff DSR2                0xff4000 (missing; Hexbus?)

===========================================================================
Known Issues (MZ, 2010-11-07)

  KEEP IN MIND THAT TEXAS INSTRUMENTS NEVER RELEASED THE TI-99/8 AND THAT
  THERE ARE ONLY A FEW PROTOTYPES OF THE TI-99/8 AVAILABLE. ALL SOFTWARE
  MUST BE ASSUMED TO HAVE REMAINED IN A PRELIMINARY STATE.

- Extended Basic II does not start when a floppy controller is present. This is
  a problem of the prototypical XB II which we cannot solve. It seems as if only
  hexbus devices are properly supported, but we currently do not have an
  emulation for those. Thus you can currently only use cassette to load and
  save programs. You MUST turn off the floppy controller support in the
  configuration before you enter XB II. Other cartridges (like Editor/Assembler)
  seem to be not affected by this problem and can make use of the floppy
  controllers.
    Technical detail: The designers of XB II seem to have decided to put PABs
    (Peripheral access block; contains pointers to buffers, the file name, and
    the access modes) into CPU RAM instead of the traditional storage in VDP
    RAM. The existing peripheral cards are hard-coded to interpret the given
    pointer to the PAB as pointing to a VDP RAM address. That is, as soon as
    the card is found, control is passed to the DSR (device service routine),
    the file name will not be found, and control returns with an error. It seems
    as if XB II does not properly handle this situation and may lock up
    (sometimes it starts up, but file access is still not possible).

- Multiple cartridges are not shown in the startup screen; only one
  cartridge is presented. You have to manually select the cartridges with the
  dip switch.

- Emulation speed may still be incorrect.

- SAVE and OLD MINIMEM do not work properly in XB II. It seems as if the
  mapper shadows the NVRAM of the cartridge. You will lose the contents when
  you turn off the machine.
===========================================================================

*/

#include "emu.h"
#include "cpu/tms9900/tms9900.h"
#include "sound/sn76496.h"
//#include "video/tms9928a.h"
#include "sound/wave.h"
#include "machine/tms9901.h"
#include "imagedev/cassette.h"

#include "machine/ti99/tiboard.h"

#include "machine/ti99/videowrp.h"
#include "machine/ti99/speech8.h"

#include "machine/ti99/peribox.h"
#include "machine/ti99/crubus.h"
#include "machine/ti99/mapper8.h"
#include "machine/ti99/grom.h"
#include "machine/ti99/gromport.h"
#include "machine/ti99/mecmouse.h"


class ti99_8_state : public driver_device
{
public:
	ti99_8_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


/*
    Memory map - see description above
*/

static ADDRESS_MAP_START(ti99_8_memmap, AS_PROGRAM, 8)
	AM_RANGE(0x0000, 0xffff) AM_DEVREADWRITE("mapper", ti99_mapper8_r, ti99_mapper8_w )
ADDRESS_MAP_END


/*
    CRU map - see description above
*/

static ADDRESS_MAP_START(ti99_8_cru_map, AS_IO, 8)
	AM_RANGE(0x0000, 0x007f) AM_DEVREAD("tms9901", tms9901_cru_r)
	AM_RANGE(0x0000, 0x02ff) AM_DEVREAD("crubus", ti99_crubus_r )

	AM_RANGE(0x0000, 0x03ff) AM_DEVWRITE("tms9901", tms9901_cru_w)
	AM_RANGE(0x0000, 0x17ff) AM_DEVWRITE("crubus", ti99_crubus_w )
ADDRESS_MAP_END

/* ti99/8 : 54-key keyboard */
static INPUT_PORTS_START(ti99_8)
	PORT_START( "DISKCTRL" )
	PORT_CONFNAME( 0x07, 0x00, "Disk controller" )
		PORT_CONFSETTING(    0x00, DEF_STR( None ) )
		PORT_CONFSETTING(    0x01, "TI SD Floppy Controller" )
		PORT_CONFSETTING(    0x02, "SNUG BwG Controller" )
		PORT_CONFSETTING(    0x03, "Myarc HFDC" )

	PORT_START( "HDCTRL" )
	PORT_CONFNAME( 0x03, 0x00, "HD controller" )
		PORT_CONFSETTING(    0x00, DEF_STR( None ) )
		PORT_CONFSETTING(    0x01, "Nouspikel IDE Controller" )
//      PORT_CONFSETTING(    0x02, "WHTech SCSI Controller" )
	PORT_CONFNAME( 0x08, 0x00, "USB-SM card" )
		PORT_CONFSETTING(    0x00, DEF_STR( Off ) )
		PORT_CONFSETTING(    0x08, DEF_STR( On ) )

	PORT_START( "SERIAL" )
	PORT_CONFNAME( 0x03, 0x00, "Serial/Parallel interface" )
		PORT_CONFSETTING(    0x00, DEF_STR( None ) )
		PORT_CONFSETTING(    0x01, "TI RS-232 card" )

	PORT_START( "HCI" )
	PORT_CONFNAME( 0x01, 0x00, "Mouse support" )
		PORT_CONFSETTING(    0x00, DEF_STR( Off ) )
		PORT_CONFSETTING(    0x01, "Mechatronics Mouse" )

	PORT_START( "CARTSLOT" )
	PORT_DIPNAME( 0x0f, 0x00, "Cartridge slot" )
		PORT_DIPSETTING(    0x00, "Auto" )
		PORT_DIPSETTING(    0x01, "Slot 1" )
		PORT_DIPSETTING(    0x02, "Slot 2" )
		PORT_DIPSETTING(    0x03, "Slot 3" )
		PORT_DIPSETTING(    0x04, "Slot 4" )

	PORT_START( "BWGDIP1" )
	PORT_DIPNAME( 0x01, 0x00, "BwG step rate" ) PORT_CONDITION( "DISKCTRL", 0x07, PORTCOND_EQUALS, 0x02 )
		PORT_DIPSETTING( 0x00, "6 ms")
		PORT_DIPSETTING( 0x01, "20 ms")

	PORT_START( "BWGDIP2" )
	PORT_DIPNAME( 0x01, 0x00, "BwG date/time display" ) PORT_CONDITION( "DISKCTRL", 0x07, PORTCOND_EQUALS, 0x02 )
		PORT_DIPSETTING( 0x00, "Hide")
		PORT_DIPSETTING( 0x01, "Show")

	PORT_START( "BWGDIP34" )
	PORT_DIPNAME( 0x03, 0x00, "BwG drives" ) PORT_CONDITION( "DISKCTRL", 0x07, PORTCOND_EQUALS, 0x02 )
		PORT_DIPSETTING( 0x00, "DSK1 only")
		PORT_DIPSETTING( 0x01, "DSK1-DSK2")
		PORT_DIPSETTING( 0x02, "DSK1-DSK3")
		PORT_DIPSETTING( 0x03, "DSK1-DSK4")

	PORT_START( "HFDCDIP" )
	PORT_DIPNAME( 0xff, 0x55, "HFDC drive config" ) PORT_CONDITION( "DISKCTRL", 0x07, PORTCOND_EQUALS, 0x03 )
		PORT_DIPSETTING( 0x00, "40 track, 16 ms")
		PORT_DIPSETTING( 0xaa, "40 track, 8 ms")
		PORT_DIPSETTING( 0x55, "80 track, 2 ms")
		PORT_DIPSETTING( 0xff, "80 track HD, 2 ms")

	PORT_START( "DRVSPD" )
	PORT_CONFNAME( 0x01, 0x01, "Floppy and HD speed" ) PORT_CONDITION( "DISKCTRL", 0x07, PORTCOND_EQUALS, 0x03 )
		PORT_CONFSETTING( 0x00, "No delay")
		PORT_CONFSETTING( 0x01, "Realistic")

	// We do not want to show this setting; makes only sense for Geneve
	PORT_START( "MODE" )
	PORT_CONFNAME( 0x01, 0x00, "Ext. cards modification" ) PORT_CONDITION( "HFDCDIP", 0xff, PORTCOND_EQUALS, GM_NEVER )
		PORT_CONFSETTING(    0x00, "Standard" )
		PORT_CONFSETTING(    GENMOD, "GenMod" )

	/* 3 ports for mouse */
	PORT_START("MOUSEX") /* Mouse - X AXIS */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSEY") /* Mouse - Y AXIS */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSE0") /* Mouse - buttons */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button 1") PORT_PLAYER(1)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Mouse Button 2") PORT_PLAYER(1)

	/* 16 ports for keyboard and joystick */
	PORT_START("KEY0")    /* col 0 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ALPHA LOCK") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FCTN") PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LSHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY1")    /* col 1 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 ! DEL") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a A") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY2")    /* col 2 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 @ INS") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w W") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s S (LEFT)") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x X (DOWN)") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY3")    /* col 3 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e E (UP)") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d D (RIGHT)") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c C") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY4")    /* col 4 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $ CLEAR") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r R") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f F") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v V") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY5")    /* col 5 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 % BEGIN") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t T") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g G") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b B") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY6")    /* col 6 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 ^ PROC'D") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY7")    /* col 7 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 & AID") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY8")    /* col 8 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 * REDO") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY9")    /* col 9 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 ( BACK") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY10")    /* col 10 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY11")    /* col 11 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= + QUIT") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY12")    /* col 12 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(SPACE)") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY13")    /* col 13 */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('`') PORT_CHAR('~')
	PORT_BIT(0x07, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY14")    /* col 14: "wired handset 1" (= joystick 1) */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN/*, "(1DOWN)", CODE_NONE, OSD_JOY_DOWN, 0*/) PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT/*, "(1RIGHT)", CODE_NONE, OSD_JOY_RIGHT, 0*/) PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT/*, "(1LEFT)", CODE_NONE, OSD_JOY_LEFT, 0*/) PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP/*, "(1UP)", CODE_NONE, OSD_JOY_UP*/) PORT_PLAYER(1)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1/*, "(1FIRE)", CODE_NONE, OSD_JOY_FIRE, 0*/) PORT_PLAYER(1)

	PORT_START("KEY15")    /* col 15: "wired handset 2" (= joystick 2) */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN/*, "(2UP)", CODE_NONE, OSD_JOY2_UP, 0*/) PORT_PLAYER(2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT/*, "(2DOWN)", CODE_NONE, OSD_JOY2_DOWN, 0*/) PORT_PLAYER(2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT/*, "(2RIGHT)", CODE_NONE, OSD_JOY2_RIGHT, 0*/) PORT_PLAYER(2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP/*, "(2LEFT)", CODE_NONE, OSD_JOY2_LEFT, 0*/) PORT_PLAYER(2)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1/*, "(2FIRE)", CODE_NONE, OSD_JOY2_FIRE, 0*/) PORT_PLAYER(2)
INPUT_PORTS_END


/* F4 Character Displayer */
static const gfx_layout ti99_7_charlayout =
{
	8, 7,					/* 8 x 7 characters */
	96,					/* 96 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8 },
	8*7					/* every char takes 7 bytes */
};

static const gfx_layout ti99_8_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	64,					/* 64 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static const gfx_layout ti99_c_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	1,					/* 1 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static const gfx_layout ti99_24_charlayout =
{
	24, 24,					/* 24 x 24 characters */
	1,					/* 1 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 64, 65, 66, 67, 68, 69, 70, 71, 128, 129, 130, 131, 132, 133, 134, 135 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 24*8, 25*8, 26*8, 27*8, 28*8, 29*8, 30*8, 31*8, 48*8, 49*8, 50*8, 51*8, 52*8, 53*8, 54*8, 55*8 },
	24*24					/* every char takes 72 bytes */
};

static GFXDECODE_START( ti99_8 )
	GFXDECODE_ENTRY( region_grom, 0x050d, ti99_8_charlayout, 0, 8 )	// large
	GFXDECODE_ENTRY( region_grom, 0x070d, ti99_7_charlayout, 0, 8 ) // small
	GFXDECODE_ENTRY( region_grom, 0x09b4, ti99_24_charlayout, 0, 8 )// TI logo
	GFXDECODE_ENTRY( region_grom, 0x09fc, ti99_c_charlayout, 0, 8 )	// (c)
GFXDECODE_END


static const TMS9928a_interface tms9118_interface =
{
	TMS99x8A,
	0x4000,
	15, 15,
	tms9901_set_int2
};

static const TMS9928a_interface tms9129_interface =
{
	TMS9929A,
	0x4000,
	13, 13,
	tms9901_set_int2
};

static const struct tms9995reset_param ti99_8_processor_config =
{
	1,	/* enable automatic wait state generation */
		/* (in January 83 99/8 schematics sheet 9: the delay logic */
		/* seems to keep READY low for one cycle when RESET* is */
		/* asserted, but the timings are completely wrong this way) */
	0,	/* no IDLE callback */
	1	/* MP9537 mask */
};

/*
    Format:
    Name, mode, stop, mask, select, write, read8z function, write8 function

    Multiple devices may have the same select pattern; as in the real hardware,
    care must be taken that only one device actually responds. In the case of
    GROMs, each chip has an internal address counter and an ID, and the chip
    only responds when the ID and the most significant 3 bits match.
*/
static const mapper8_dev_config mapper_devices[] =
{
	// TI-99/8 mode
	{ SRAMNAME,			0, 1, 0xf800, 0xf000, 0x0000, NULL, 			NULL },
	{ "soundgen",   	0, 1, 0xfff0, 0xf800, 0x0000, NULL,         	sn76496_w },
	{ "video",			0, 1, 0xfffd, 0xf810, 0x0000, ti8_tms991x_rz,	ti8_tms991x_w },
	{ "speech", 		0, 1, 0xfff0, 0xf820, 0x0000, ti998spch_rz, 	ti998spch_w },
	{ "grom_0", 	    0, 0, 0xfff1, 0xf830, 0x0000, ti99grom_rz,		ti99grom_w },
	{ "grom_1", 		0, 0, 0xfff1, 0xf830, 0x0000, ti99grom_rz,		ti99grom_w },
	{ "grom_2",			0, 0, 0xfff1, 0xf830, 0x0000, ti99grom_rz,  	ti99grom_w },
	{ "gromport",   	0, 0, 0xfff1, 0xf830, 0x0400, gromportg_rz,		gromportg_w, },
//  { "grom_tts_0",     2, 0, 0xfff1, 0xf840, 0x0000, 0, 0, ti99grom_rz,    ti99grom_w },  // up to 6 GROMs; no good dumps known
//  { "grom_pcode1_0",  2, 0, 0xfff1, 0xf850, 0x0000, 0, 0, ti99grom_rz,    ti99grom_w },  // up to 6 GROMs; no good dumps known
//  { "grom_pcode2_0",  2, 0, 0xfff1, 0xf860, 0x0000, 0, 0, ti99grom_rz,    ti99grom_w },  // up to 6 GROMs; no good dumps known
	{ "mapper", 		0, 1, 0xfff0, 0xf870, 0x0000, NULL,			ti99_mapreg_w },

	// TI-99/4A mode
	// GROM: 1001 1000 0000 xxa0
	{ ROM0NAME,			1, 1, 0xe000, 0x0000, 0x0000, NULL, 			NULL },
	{ "grom_0",     	1, 0, 0xfff1, 0x9800, 0x0400, ti99grom_rz,  	ti99grom_w },
	{ "grom_1",     	1, 0, 0xfff1, 0x9800, 0x0400, ti99grom_rz,  	ti99grom_w },
	{ "grom_2",     	1, 0, 0xfff1, 0x9800, 0x0400, ti99grom_rz,  	ti99grom_w },
	{ "gromport",   	1, 0, 0xfff1, 0x9800, 0x0400, gromportg_rz,		gromportg_w, },

	{ "mapper", 		1, 1, 0xfff0, 0x8810, 0x0000, NULL,				ti99_mapreg_w },
	{ "soundgen",   	1, 1, 0xfff0, 0x8400, 0x0000, NULL,     		sn76496_w },
	{ "video",			1, 1, 0xfffd, 0x8800, 0x0400, ti8_tms991x_rz,	ti8_tms991x_w },
	{ "speech", 		1, 1, 0xfff0, 0x9000, 0x0400, ti998spch_rz, 	ti998spch_w },
	{ SRAMNAME,			1, 1, 0xf800, 0x8000, 0x0000, NULL,				NULL }, // write at the end; soundgen must be found earlier

	// Physical (need to pack this into here as well)
	{ DRAMNAME, 		3, 1, 0xff0000, 0x000000, 0x000000, NULL, NULL },
	{ "mapper",			3, 0, 0xffe000, 0xff4000, 0x000000, ti998dsr_rz, NULL },   // DSR
	{ "gromport",		3, 1, 0xffe000, 0xff6000, 0x000000, gromportr_rz, gromportr_w  },
	{ "gromport",		3, 1, 0xffe000, 0xff8000, 0x000000, gromportr_rz, gromportr_w  },
	{ ROM1NAME, 		3, 1, 0xffe000, 0xffa000, 0x000000, NULL, NULL },
	{ ROM1ANAME,		3, 1, 0xffe000, 0xffc000, 0x000000, NULL, NULL },
	{ INTSNAME, 		3, 1, 0xfffff0, 0xffe000, 0x000000, NULL, NULL },
	{ "peribox",		3, 1, 0x000000, 0x000000, 0x000000, ti99_peb_data_rz, ti99_peb_data_w },

	{ NULL, 0, 0, 0, 0, 0, NULL, NULL  }
};

static const cruconf_device cru_devices[] =
{
	{ "mapper", 	0xfff0, 0x2700, NULL, mapper8c_w },
	{ "gromport",	0xf800, 0x0800, gromportc_rz, gromportc_w },				// Check: Is this really limited?
	{ "peribox",	0x0000, 0x0000, ti99_peb_cru_rz, ti99_peb_cru_w },    // Peribox needs all addresses
	{ NULL, 		0, 0, NULL, NULL }
};

MACHINE_RESET( ti99_8 )
{
}

static MACHINE_CONFIG_START( ti99_8_60hz, ti99_8_state )
	/* basic machine hardware */
	/* TMS9995-MP9537 CPU @ 10.7 MHz */
	MCFG_CPU_ADD("maincpu", TMS9995, 10738635)
	MCFG_CPU_CONFIG(ti99_8_processor_config)
	MCFG_CPU_PROGRAM_MAP(ti99_8_memmap)
	MCFG_CPU_IO_MAP(ti99_8_cru_map)
	MCFG_CPU_VBLANK_INT("screen", ti99_vblank_interrupt)

	MCFG_MACHINE_RESET( ti99_8 )

	/* video hardware */
	MCFG_TI_TMS991x_ADD("video", tms9928a, 60, "screen", 2500, &tms9118_interface)

	MCFG_GFXDECODE(ti99_8)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_WAVE_ADD("wave", "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
	MCFG_SOUND_ADD("soundgen", SN76496, 3579545)	/* 3.579545 MHz */
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

	MCFG_TISPEECH_ADD("speech")

	MCFG_TI998_BOARD_ADD( "ti_board" )
	MCFG_MAPPER8_ADD( "mapper", mapper_devices )

	/* tms9901 */
	MCFG_TMS9901_ADD("tms9901", tms9901_wiring_ti99_8)

	MCFG_CRUBUS_ADD( "crubus", cru_devices)

	MCFG_GROM_ADD( "grom_0", 0, region_grom, 0x0000, 0x1800, console_ready )
	MCFG_GROM_ADD( "grom_1", 1, region_grom, 0x2000, 0x1800, console_ready )
	MCFG_GROM_ADD( "grom_2", 2, region_grom, 0x4000, 0x1800, console_ready )

	MCFG_PBOX8_ADD( "peribox", console_extint, console_notconnected, console_ready )
	MCFG_CASSETTE_ADD( "cassette", default_cassette_config )

	MCFG_TI99_GROMPORT_ADD( "gromport", console_ready )
	MCFG_MECMOUSE_ADD( "mecmouse" )
MACHINE_CONFIG_END


static MACHINE_CONFIG_START( ti99_8_50hz, ti99_8_state )
	/* basic machine hardware */
	/* TMS9995-MP9537 CPU @ 10.7 MHz */
	MCFG_CPU_ADD("maincpu", TMS9995, 10738635)
	MCFG_CPU_CONFIG(ti99_8_processor_config)
	MCFG_CPU_PROGRAM_MAP(ti99_8_memmap)
	MCFG_CPU_IO_MAP(ti99_8_cru_map)
	MCFG_CPU_VBLANK_INT("screen", ti99_vblank_interrupt)

	MCFG_MACHINE_RESET( ti99_8 )

	/* video hardware */
	MCFG_TI_TMS991x_ADD("video", tms9928a, 50, "screen", 2500, &tms9129_interface)

	MCFG_GFXDECODE(ti99_8)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_WAVE_ADD("wave", "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
	MCFG_SOUND_ADD("soundgen", SN76496, 3579545)	/* 3.579545 MHz */
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

	MCFG_TISPEECH_ADD("speech")

	MCFG_TI998_BOARD_ADD( "ti_board" )
	MCFG_MAPPER8_ADD( "mapper", mapper_devices )

	/* tms9901 */
	MCFG_TMS9901_ADD("tms9901", tms9901_wiring_ti99_8)

	MCFG_CRUBUS_ADD( "crubus", cru_devices)

	MCFG_GROM_ADD( "grom_0", 0, region_grom, 0x0000, 0x1800, console_ready )
	MCFG_GROM_ADD( "grom_1", 1, region_grom, 0x2000, 0x1800, console_ready )
	MCFG_GROM_ADD( "grom_2", 2, region_grom, 0x4000, 0x1800, console_ready )

	MCFG_PBOX8_ADD( "peribox", console_extint, console_notconnected, console_ready )
	MCFG_CASSETTE_ADD( "cassette", default_cassette_config )

	MCFG_TI99_GROMPORT_ADD( "gromport", console_ready )
	MCFG_MECMOUSE_ADD( "mecmouse" )
MACHINE_CONFIG_END

/*
    ROM loading
*/
ROM_START(ti99_8)
	/*CPU memory space*/
	ROM_REGION(0x8000,"maincpu",0)
	ROM_LOAD("998rom.bin", 0x0000, 0x8000, CRC(b7a06ffd) SHA1(17dc8529fa808172fc47089982efb0bf0548c80c))		/* system ROMs */

	/*GROM memory space*/
	ROM_REGION(0x10000, region_grom, 0)
	ROM_LOAD("998grom.bin", 0x0000, 0x6000, CRC(c63806bc) SHA1(cbfa8b04b4aefbbd9a713c54267ad4dd179c13a3))	/* system GROMs */

ROM_END

#define rom_ti99_8e rom_ti99_8

/*      YEAR    NAME        PARENT  COMPAT  MACHINE     INPUT   INIT      COMPANY                 FULLNAME */
COMP(	1983,	ti99_8,		0,		0,	ti99_8_60hz,ti99_8,	0,		"Texas Instruments",	"TI-99/8 Computer (US)" , 0)
COMP(	1983,	ti99_8e,	ti99_8,	0,	ti99_8_50hz,ti99_8,	0,		"Texas Instruments",	"TI-99/8 Computer (Europe)" , 0 )
