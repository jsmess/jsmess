/***************************************************************************
    microbee.c

    system driver
    Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

    Brett Selwood, Andrew Davies (technical assistance)

    Microbee Standard / Plus memory map

        0000-7FFF RAM
        8000-BFFF SYSTEM roms
        C000-DFFF Edasm or WBee (edasm.rom or wbeee12.rom, optional)
        E000-EFFF Telcom 1.2 (netrom.ic34; optional)
        F000-F7FF Video RAM
        F800-FFFF PCG RAM (graphics)

    Microbee IC memory map (preliminary)

        0000-7FFF RAM
        8000-BFFF SYSTEM roms (bas522a.rom, bas522b.rom)
        C000-DFFF Edasm or WBee (edasm.rom or wbeee12.rom, optional)
        E000-EFFF Telcom (optional)
        F000-F7FF Video RAM
        F800-FFFF PCG RAM (graphics), Colour RAM (banked)

    Microbee 56KB ROM memory map (preliminary)

        0000-DFFF RAM
        E000-EFFF ROM 56kb.rom CP/M bootstrap loader
        F000-F7FF Video RAM
        F800-FFFF PCG RAM (graphics), Colour RAM (banked)

    Commands to call up built-in roms (depends on the model):
    NET - Jump to E000, usually the Telcom communications program.
          This rom can be replaced with the Dreamdisk Chip-8 rom.
        Note that Telcom 3.21 is 8k, it uses a rombank switch
        (by reading port 0A) to swap between the two halves.

    EDASM - Jump to C000, usually the editor/Assembler package.
        Currently this works properly only on the Standard model,
        there appears to be some sort of core issue causing it to
        freeze on the other models.

    MENU - Do a rombank switch to bank 5 and jump to C000 to start the Shell

    PAK n - Do a rombank switch (write to port 0A) to bank "n" and jump to C000.

    These early colour computers have a PROM to create the foreground palette.

    TODO:
    - Printer is working, but with improper code. This needs to be fixed.
    - Other models to be added (64k, 128k, 256k, 512k, PPC85, Teleterm)
    - Roms for mbeepc to be checked (I think they are correct)
    - Diskette code to be checked and made working

    Notes about the printer:
    - When computer turned on, defaults to 1200 baud serial printer
    - Change it to parallel by entering OUTL#1
    - After you mount/create a printfile, you can LPRINT and LLIST.


***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "sound/wave.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "includes/mbee.h"

size_t mbee_size;

/********** NOTE !!! ***********************************************************
    The microbee uses lots of bankswitching and the memory maps are still
    being determined. Please don't merge memory maps !!
********************************************************************************/


static ADDRESS_MAP_START(mbee_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("bank1")
	AM_RANGE(0x1000, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0x7fff) AM_WRITENOP	/* Needed because quickload to here will crash MESS otherwise */
	AM_RANGE(0x8000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READ_BANK("bank2") AM_WRITE(mbee_videoram_w) AM_SIZE_GENERIC(videoram)
	AM_RANGE(0xf800, 0xffff) AM_READ_BANK("bank3") AM_WRITE(mbee_pcg_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbeeic_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("bank1")
	AM_RANGE(0x1000, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xdfff) AM_ROMBANK("bank4")
	AM_RANGE(0xe000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READ_BANK("bank2") AM_WRITE(mbee_videoram_w) AM_SIZE_GENERIC(videoram)
	AM_RANGE(0xf800, 0xffff) AM_READ_BANK("bank3") AM_WRITE(mbee_pcg_color_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbeepc_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("bank1")
	AM_RANGE(0x1000, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xdfff) AM_ROMBANK("bank4")
	AM_RANGE(0xe000, 0xefff) AM_ROMBANK("bank5")
	AM_RANGE(0xf000, 0xf7ff) AM_READ_BANK("bank2") AM_WRITE(mbee_videoram_w) AM_SIZE_GENERIC(videoram)
	AM_RANGE(0xf800, 0xffff) AM_READ_BANK("bank3") AM_WRITE(mbee_pcg_color_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbeepc85_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("bank1")
	AM_RANGE(0x1000, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xdfff) AM_ROMBANK("bank4")
	AM_RANGE(0xe000, 0xefff) AM_ROMBANK("bank5")
	AM_RANGE(0xf000, 0xf7ff) AM_READ_BANK("bank2") AM_WRITE(mbee_videoram_w) AM_SIZE_GENERIC(videoram)
	AM_RANGE(0xf800, 0xffff) AM_READ_BANK("bank3") AM_WRITE(mbee_pcg_color_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbeeppc_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("bank1")
	AM_RANGE(0x1000, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0x9fff) AM_ROMBANK("bank6")
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xdfff) AM_ROMBANK("bank4")
	AM_RANGE(0xe000, 0xefff) AM_ROMBANK("bank5")
	AM_RANGE(0xf000, 0xf7ff) AM_READ_BANK("bank2") AM_WRITE(mbee_videoram_w) AM_SIZE_GENERIC(videoram)
	AM_RANGE(0xf800, 0xffff) AM_READ_BANK("bank3") AM_WRITE(mbee_pcg_color_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbee56_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("bank1")
	AM_RANGE(0x1000, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READ_BANK("bank2") AM_WRITE(mbee_videoram_w) AM_SIZE_GENERIC(videoram)
	AM_RANGE(0xf800, 0xffff) AM_READ_BANK("bank3") AM_WRITE(mbee_pcg_color_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbee64_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("bank1")
	AM_RANGE(0x1000, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READ_BANK("bank2") AM_WRITE(mbee_videoram_w) AM_SIZE_GENERIC(videoram)
	AM_RANGE(0xf800, 0xffff) AM_READ_BANK("bank3") AM_WRITE(mbee_pcg_color_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START(mbee_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x03) AM_MIRROR(0x10) AM_DEVREADWRITE("z80pio", mbee_pio_r, mbee_pio_w)
	AM_RANGE(0x0b, 0x0b) AM_MIRROR(0x10) AM_READWRITE(mbee_video_bank_r, mbee_video_bank_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0x10) AM_READWRITE(m6545_status_r, m6545_index_w)
	AM_RANGE(0x0d, 0x0d) AM_MIRROR(0x10) AM_READWRITE(m6545_data_r, m6545_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbeeic_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x03) AM_MIRROR(0x10) AM_DEVREADWRITE("z80pio", mbee_pio_r, mbee_pio_w)
	AM_RANGE(0x08, 0x08) AM_MIRROR(0x10) AM_READWRITE(mbee_pcg_color_latch_r, mbee_pcg_color_latch_w)
	AM_RANGE(0x09, 0x09) AM_MIRROR(0x10) AM_NOP /* Listed as "Colour Wait Off" or "USART 2651" but doesn't appear in the schematics */
	AM_RANGE(0x0a, 0x0a) AM_MIRROR(0x10) AM_READWRITE(mbee_color_bank_r, mbee_color_bank_w)
	AM_RANGE(0x0b, 0x0b) AM_MIRROR(0x10) AM_READWRITE(mbee_video_bank_r, mbee_video_bank_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0x10) AM_READWRITE(m6545_status_r, m6545_index_w)
	AM_RANGE(0x0d, 0x0d) AM_MIRROR(0x10) AM_READWRITE(m6545_data_r, m6545_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbeepc_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0003) AM_MIRROR(0xff10) AM_DEVREADWRITE("z80pio", mbee_pio_r, mbee_pio_w)
	AM_RANGE(0x0008, 0x0008) AM_MIRROR(0xff10) AM_READWRITE(mbee_pcg_color_latch_r, mbee_pcg_color_latch_w)
	AM_RANGE(0x000a, 0x000a) AM_MIRROR(0xfe10) AM_READWRITE(mbee_color_bank_r, mbee_color_bank_w)
	AM_RANGE(0x000b, 0x000b) AM_MIRROR(0xff10) AM_READWRITE(mbee_video_bank_r, mbee_video_bank_w)
	AM_RANGE(0x000c, 0x000c) AM_MIRROR(0xff10) AM_READWRITE(m6545_status_r, m6545_index_w)
	AM_RANGE(0x000d, 0x000d) AM_MIRROR(0xff10) AM_READWRITE(m6545_data_r, m6545_data_w)
	AM_RANGE(0x010a, 0x010a) AM_MIRROR(0xfe10) AM_READWRITE(mbee_bank_netrom_r, mbee_color_bank_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbeepc85_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0003) AM_MIRROR(0xff10) AM_DEVREADWRITE("z80pio", mbee_pio_r, mbee_pio_w)
	AM_RANGE(0x0008, 0x0008) AM_MIRROR(0xff10) AM_READWRITE(mbee_pcg_color_latch_r, mbee_pcg_color_latch_w)
	AM_RANGE(0x000a, 0x000a) AM_MIRROR(0xfe10) AM_READWRITE(mbee_color_bank_r, mbee_color_bank_w)
	AM_RANGE(0x000b, 0x000b) AM_MIRROR(0xff10) AM_READWRITE(mbee_video_bank_r, mbee_video_bank_w)
	AM_RANGE(0x000c, 0x000c) AM_MIRROR(0xff10) AM_READWRITE(m6545_status_r, m6545_index_w)
	AM_RANGE(0x000d, 0x000d) AM_MIRROR(0xff10) AM_READWRITE(m6545_data_r, m6545_data_w)
	AM_RANGE(0x010a, 0x010a) AM_MIRROR(0xfe10) AM_READWRITE(mbee_bank_netrom_r, mbee_color_bank_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbeeppc_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0003) AM_MIRROR(0xff00) AM_DEVREADWRITE("z80pio", mbee_pio_r, mbee_pio_w)
	AM_RANGE(0x0008, 0x0008) AM_MIRROR(0xff00) AM_READWRITE(mbee_pcg_color_latch_r, mbee_pcg_color_latch_w)
	AM_RANGE(0x000a, 0x000a) AM_MIRROR(0xfe00) AM_READWRITE(mbee_color_bank_r, mbee_0a_w)
	AM_RANGE(0x000b, 0x000b) AM_MIRROR(0xff00) AM_READWRITE(mbee_video_bank_r, mbee_video_bank_w)
	AM_RANGE(0x000c, 0x000c) AM_MIRROR(0xff00) AM_READWRITE(m6545_status_r, m6545_index_w)
	AM_RANGE(0x000d, 0x000d) AM_MIRROR(0xff00) AM_READWRITE(m6545_data_r, m6545_data_w)
	AM_RANGE(0x001c, 0x001c) AM_MIRROR(0xff00) AM_WRITE(mbee_1c_w)
	AM_RANGE(0x010a, 0x010a) AM_MIRROR(0xfe00) AM_READWRITE(mbee_bank_netrom_r, mbee_0a_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbee56_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x03) AM_MIRROR(0x10) AM_DEVREADWRITE("z80pio", mbee_pio_r, mbee_pio_w)
	AM_RANGE(0x08, 0x08) AM_MIRROR(0x10) AM_READWRITE(mbee_pcg_color_latch_r, mbee_pcg_color_latch_w)
	AM_RANGE(0x0b, 0x0b) AM_MIRROR(0x10) AM_READWRITE(mbee_video_bank_r, mbee_video_bank_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0x10) AM_READWRITE(m6545_status_r, m6545_index_w)
	AM_RANGE(0x0d, 0x0d) AM_MIRROR(0x10) AM_READWRITE(m6545_data_r, m6545_data_w)
	AM_RANGE(0x44, 0x47) AM_DEVREADWRITE("wd179x", wd17xx_r, wd17xx_w)
	AM_RANGE(0x48, 0x48) AM_READWRITE(mbee_fdc_status_r, mbee_fdc_motor_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbee64_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x03) AM_MIRROR(0x10) AM_DEVREADWRITE("z80pio", mbee_pio_r, mbee_pio_w)
	AM_RANGE(0x08, 0x08) AM_MIRROR(0x10) AM_READWRITE(mbee_pcg_color_latch_r, mbee_pcg_color_latch_w)
	AM_RANGE(0x0b, 0x0b) AM_MIRROR(0x10) AM_READWRITE(mbee_video_bank_r, mbee_video_bank_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0x10) AM_READWRITE(m6545_status_r, m6545_index_w)
	AM_RANGE(0x0d, 0x0d) AM_MIRROR(0x10) AM_READWRITE(m6545_data_r, m6545_data_w)
	AM_RANGE(0x44, 0x47) AM_DEVREADWRITE("wd179x", wd17xx_r, wd17xx_w)
	AM_RANGE(0x48, 0x48) AM_READWRITE(mbee_fdc_status_r, mbee_fdc_motor_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( mbee )
	PORT_START("LINE0") /* IN0 KEY ROW 0 [000] */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("@") PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A') PORT_CHAR(0x01)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B') PORT_CHAR(0x02)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C') PORT_CHAR(0x03)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D') PORT_CHAR(0x04)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E') PORT_CHAR(0x05)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F') PORT_CHAR(0x06)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G') PORT_CHAR(0x07)

	PORT_START("LINE1") /* IN1 KEY ROW 1 [080] */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H') PORT_CHAR(0x08)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I') PORT_CHAR(0x09)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J') PORT_CHAR(0x0a)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K') PORT_CHAR(0x0b)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L') PORT_CHAR(0x0c)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M') PORT_CHAR(0x0d)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N') PORT_CHAR(0x0e)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O') PORT_CHAR(0x0f)

	PORT_START("LINE2") /* IN2 KEY ROW 2 [100] */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P') PORT_CHAR(0x10)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q') PORT_CHAR(0x11)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR(0x12)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S') PORT_CHAR(0x13)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR(0x14)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U') PORT_CHAR(0x15)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V') PORT_CHAR(0x16)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W') PORT_CHAR(0x17)

	PORT_START("LINE3") /* IN3 KEY ROW 3 [180] */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X') PORT_CHAR(0x18)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('u') PORT_CHAR('Y') PORT_CHAR(0x19)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z') PORT_CHAR(0x1a)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{') PORT_CHAR(0x1b)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|') PORT_CHAR(0x1c)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}') PORT_CHAR(0x1d)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("^") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('^') PORT_CHAR('~') PORT_CHAR(0x1e)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL) PORT_CHAR(8) PORT_CHAR(0x5f) PORT_CHAR(0x1f)	// port_char not working - hijacked

	PORT_START("LINE4") /* IN4 KEY ROW 4 [200] */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("1 !") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("3 #") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("4 $") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("5 %") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("6 &") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("7 '") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')

	PORT_START("LINE5") /* IN5 KEY ROW 5 [280] */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("8 (") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("9 )") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START("LINE6") /* IN6 KEY ROW 6 [300] */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Escape") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Linefeed") PORT_CODE(KEYCODE_HOME) PORT_CHAR(10)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Break") PORT_CODE(KEYCODE_END) PORT_CHAR(3)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("LINE7") /* IN7 KEY ROW 7 [380] */
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("EXTRA") /* IN8 extra keys */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("(Up)") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("(Down)") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("(Left)") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("(Right)") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("(Insert)") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* Enhanced options not available on real hardware */
	PORT_START("CONFIG")
	PORT_CONFNAME( 0x01, 0x01, "Autorun on Quickload")
	PORT_CONFSETTING(    0x00, DEF_STR(No))
	PORT_CONFSETTING(    0x01, DEF_STR(Yes))
	PORT_BIT( 0x6, 0x6, IPT_UNUSED )
//  PORT_CONFNAME( 0x08, 0x08, "Cassette Speaker")
//  PORT_CONFSETTING(    0x08, DEF_STR(On))
//  PORT_CONFSETTING(    0x00, DEF_STR(Off))
INPUT_PORTS_END

static const z80_daisy_chain mbee_daisy_chain[] =
{
	{ "z80pio" },
	{ NULL }
};

/**************************** F4 CHARACTER DISPLAYER */
static const gfx_layout mbee_charlayout =
{
	8,16,					/* 8 x 16 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8, 8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

/* This will show the 128 characters in the ROM + whatever happens to be in the PCG */
static GFXDECODE_START( mbee )
	GFXDECODE_ENTRY( "maincpu", 0x11000, mbee_charlayout, 0, 1 )
GFXDECODE_END

static GFXDECODE_START( mbeeic )
	GFXDECODE_ENTRY( "maincpu", 0x11000, mbee_charlayout, 0, 48 )
GFXDECODE_END

static FLOPPY_OPTIONS_START(mbee)
	FLOPPY_OPTION(ss80, "ss80", "SS80 disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([80])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(ds40, "ds40", "DS40 disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(ds80, "ds80", "DS80 disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(ds84, "ds84", "DS84 disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([84])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config mbee_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(mbee),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( mbee )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_12MHz / 6)         /* 2 MHz */
	MDRV_CPU_PROGRAM_MAP(mbee_mem)
	MDRV_CPU_IO_MAP(mbee_io)
	MDRV_CPU_CONFIG(mbee_daisy_chain)
	MDRV_CPU_VBLANK_INT("screen", mbee_interrupt)

	MDRV_MACHINE_RESET( mbee )

	MDRV_Z80PIO_ADD( "z80pio", mbee_z80pio_intf )

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(250)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 19*16)			/* need at least 17 lines for NET */
	MDRV_SCREEN_VISIBLE_AREA(0*8, 64*8-1, 0, 19*16-1)

	MDRV_GFXDECODE(mbee)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(mbee)
	MDRV_VIDEO_UPDATE(mbee)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* devices */
	MDRV_QUICKLOAD_ADD("quickload", mbee, "mwb,com", 2)
	MDRV_Z80BIN_QUICKLOAD_ADD("quickload2", mbee, 2)
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)
	MDRV_CASSETTE_ADD( "cassette", default_cassette_config )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mbeeic )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 3375000)         /* 3.37500 MHz */
	MDRV_CPU_PROGRAM_MAP(mbeeic_mem)
	MDRV_CPU_IO_MAP(mbeeic_io)
	MDRV_CPU_CONFIG(mbee_daisy_chain)
	MDRV_CPU_VBLANK_INT("screen", mbee_interrupt)

	MDRV_MACHINE_RESET( mbee )

	MDRV_Z80PIO_ADD( "z80pio", mbee_z80pio_intf )

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(250)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*8, 310)
	MDRV_SCREEN_VISIBLE_AREA(0, 80*8-1, 0, 19*16-1)

	MDRV_GFXDECODE(mbeeic)
	MDRV_PALETTE_LENGTH(96)
	MDRV_PALETTE_INIT(mbeeic)

	MDRV_VIDEO_START(mbeeic)
	MDRV_VIDEO_UPDATE(mbeeic)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* devices */
	MDRV_QUICKLOAD_ADD("quickload", mbee, "mwb,com", 2)
	MDRV_Z80BIN_QUICKLOAD_ADD("quickload2", mbee, 2)
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)
	MDRV_CASSETTE_ADD( "cassette", default_cassette_config )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mbeepc )
	MDRV_IMPORT_FROM( mbeeic )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP(mbeepc_mem)
	MDRV_CPU_IO_MAP(mbeepc_io)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mbeepc85 )
	MDRV_IMPORT_FROM( mbeeic )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP(mbeepc85_mem)
	MDRV_CPU_IO_MAP(mbeepc85_io)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mbeeppc )
	MDRV_IMPORT_FROM( mbeepc85 )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP(mbeeppc_mem)
	MDRV_CPU_IO_MAP(mbeeppc_io)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mbee56 )
	MDRV_IMPORT_FROM( mbeepc85 )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP(mbee56_mem)
	MDRV_CPU_IO_MAP(mbee56_io)
	MDRV_WD179X_ADD("wd179x", mbee_wd17xx_interface )
	MDRV_FLOPPY_2_DRIVES_ADD(mbee_floppy_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mbee64 )
	MDRV_IMPORT_FROM( mbeepc85 )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP(mbee64_mem)
	MDRV_CPU_IO_MAP(mbee64_io)
	MDRV_WD179X_ADD("wd179x", mbee_wd17xx_interface )
	MDRV_FLOPPY_2_DRIVES_ADD(mbee_floppy_config)
MACHINE_DRIVER_END

static DRIVER_INIT( mbee )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "bank1", 0, 2, &RAM[0x0000],  0x8000);
	memory_configure_bank(machine, "bank2", 0, 2, &RAM[0x11000], 0x4000);
	memory_configure_bank(machine, "bank3", 0, 2, &RAM[0x11800], 0x4000);
	memory_set_bank(machine, "bank2", 1);
	memory_set_bank(machine, "bank3", 0);
	mbee_size = 0x4000;
}

static DRIVER_INIT( mbeeic )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "bank1", 0, 2, &RAM[0x0000],  0x8000);
	memory_configure_bank(machine, "bank2", 0, 2, &RAM[0x11000], 0x4000);
	memory_configure_bank(machine, "bank3", 0, 2, &RAM[0x11800], 0x4000);
	memory_configure_bank(machine, "bank4", 0, 8, &RAM[0x20000], 0x2000);
	memory_set_bank(machine, "bank2", 1);
	memory_set_bank(machine, "bank3", 0);
	memory_set_bank(machine, "bank4", 0);
	mbee_size = 0x8000;
}

static DRIVER_INIT( mbeepc )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "bank1", 0, 2, &RAM[0x0000],  0x8000);
	memory_configure_bank(machine, "bank2", 0, 2, &RAM[0x11000], 0x4000);
	memory_configure_bank(machine, "bank3", 0, 2, &RAM[0x11800], 0x4000);
	memory_configure_bank(machine, "bank4", 0, 8, &RAM[0x20000], 0x2000);
	memory_configure_bank(machine, "bank5", 0, 2, &RAM[0x18000], 0x1000);
	memory_set_bank(machine, "bank2", 1);
	memory_set_bank(machine, "bank3", 0);
	memory_set_bank(machine, "bank4", 0);
	memory_set_bank(machine, "bank5", 0);
	mbee_size = 0x8000;
}

static DRIVER_INIT( mbeepc85 )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "bank1", 0, 2, &RAM[0x0000],  0x8000);
	memory_configure_bank(machine, "bank2", 0, 2, &RAM[0x11000], 0x4000);
	memory_configure_bank(machine, "bank3", 0, 2, &RAM[0x11800], 0x4000);
	memory_configure_bank(machine, "bank4", 0, 8, &RAM[0x20000], 0x2000);
	memory_configure_bank(machine, "bank5", 0, 2, &RAM[0x18000], 0x1000);
	memory_set_bank(machine, "bank2", 1);
	memory_set_bank(machine, "bank3", 0);
	memory_set_bank(machine, "bank4", 5);
	memory_set_bank(machine, "bank5", 0);
	mbee_size = 0x8000;
}

static DRIVER_INIT( mbeeppc )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "bank1", 0, 2, &RAM[0x0000],  0x30000);
	memory_configure_bank(machine, "bank2", 0, 2, &RAM[0x11000], 0x4000);
	memory_configure_bank(machine, "bank3", 0, 2, &RAM[0x11800], 0x4000);
	memory_configure_bank(machine, "bank4", 0, 8, &RAM[0x20000], 0x2000);
	memory_configure_bank(machine, "bank5", 0, 2, &RAM[0x18000], 0x1000);
	memory_configure_bank(machine, "bank6", 0, 2, &RAM[0x30000], 0x2000);
	memory_set_bank(machine, "bank2", 1);
	memory_set_bank(machine, "bank3", 0);
	memory_set_bank(machine, "bank4", 5);
	memory_set_bank(machine, "bank5", 0);
	memory_set_bank(machine, "bank6", 0);
	mbee_size = 0x8000;
}

static DRIVER_INIT( mbee56 )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "bank1", 0, 2, &RAM[0x0000],  0xe000);
	memory_configure_bank(machine, "bank2", 0, 2, &RAM[0x11000], 0x4000);
	memory_configure_bank(machine, "bank3", 0, 2, &RAM[0x11800], 0x4000);
	memory_set_bank(machine, "bank2", 1);
	memory_set_bank(machine, "bank3", 0);
	mbee_size = 0xe000;
}

static DRIVER_INIT( mbee64 )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "bank1", 0, 2, &RAM[0x0000],  0xe000);
	memory_configure_bank(machine, "bank2", 0, 2, &RAM[0x11000], 0x4000);
	memory_configure_bank(machine, "bank3", 0, 2, &RAM[0x11800], 0x4000);
	memory_set_bank(machine, "bank2", 1);
	memory_set_bank(machine, "bank3", 0);
	mbee_size = 0xe000;
}

ROM_START( mbee )
	ROM_REGION(0x18000,"maincpu",0)
	ROM_LOAD("bas510a.ic25",          0x8000,  0x1000, CRC(2ca47c36) SHA1(f36fd0afb3f1df26edc67919e78000b762b6cbcb) )
	ROM_LOAD("bas510b.ic27",          0x9000,  0x1000, CRC(a07a0c51) SHA1(dcbdd9df78b4b6b2972de2e4050dabb8ae9c3f5a) )
	ROM_LOAD("bas510c.ic28",          0xa000,  0x1000, CRC(906ac00f) SHA1(9b46458e5755e2c16cdb191a6a70df6de9fe0271) )
	ROM_LOAD("bas510d.ic30",          0xb000,  0x1000, CRC(61727323) SHA1(c0fea9fd0e25beb9faa7424db8efd07cf8d26c1b) )
	ROM_LOAD_OPTIONAL("edasma.ic31",  0xc000,  0x1000, CRC(120c3dea) SHA1(32c9bb6e54dd50d5218bb43cc921885a0307161d) )
	ROM_LOAD_OPTIONAL("edasmb.ic33",  0xd000,  0x1000, CRC(a23bf3c8) SHA1(73a57c2800a1c744b527d0440b170b8b03351753) )
	ROM_LOAD_OPTIONAL("telcom10.rom", 0xe000,  0x1000, CRC(cc9ac94d) SHA1(6804b5ff54d16f8e06180751d8681c44f351e0bb) )

/*  Optional Dreamcards Chip-8 V2.2 rom, take out the Telcom rom and insert this in its place
    ROM_LOAD_OPTIONAL("chip8_22.rom", 0xe000,  0x1000, CRC(11fbb547) SHA1(7bd9dc4b67b33b8e1be99beb6a0ddff25bdbd3f7) ) */

	ROM_LOAD("charrom.ic13",          0x11000, 0x0800, CRC(b149737b) SHA1(a3cd4f5d0d3c71137cd1f0f650db83333a2e3597) )
	ROM_RELOAD( 0x17000, 0x0800 )
	ROM_RELOAD( 0x17800, 0x0800 )

	ROM_REGION( 0x0020, "proms", 0 )
	ROM_LOAD_OPTIONAL( "82s123.ic16", 0x0000,  0x0020, CRC(4e779985) SHA1(cd2579cf65032c30b3fe7d6d07b89d4633687481) )	/* video switching prom, not needed for emulation purposes */
ROM_END

ROM_START( mbeeic )
	ROM_REGION(0x30000,"maincpu",0)
	ROM_LOAD("bas522a.rom",           0x8000,  0x2000, CRC(7896a696) SHA1(a158f7803296766160e1f258dfc46134735a9477) )
	ROM_LOAD("bas522b.rom",           0xa000,  0x2000, CRC(b21d9679) SHA1(332844433763331e9483409cd7da3f90ac58259d) )
	ROM_LOAD("charrom.bin",           0x11000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0) )
	ROM_RELOAD( 0x17000, 0x1000 )

/*  Telcom v1.1 was shipped with the first version of the IC model
    ROM_LOAD_OPTIONAL("telcom11.rom", 0xe000,  0x1000, CRC(15516499) SHA1(2d4953f994b66c5d3b1d457b8c92d9a0a69eb8b8) ) */
	ROM_LOAD_OPTIONAL("telcom12.rom", 0xe000,  0x1000, CRC(0231bda3) SHA1(be7b32499034f985cc8f7865f2bc2b78c485585c) )

	/* PAK option roms */
	ROM_LOAD_OPTIONAL("edasm.rom",    0x20000, 0x2000, CRC(1af1b3a9) SHA1(d035a997c2dbbb3918b3395a3a5a1076aa203ee5) )
	ROM_LOAD_OPTIONAL("wbee12.rom",   0x22000, 0x2000, CRC(0fc21cb5) SHA1(33b3995988fc51ddef1568e160dfe699867adbd5) ) // 4

	ROM_REGION( 0x0040, "proms", 0 )
	ROM_LOAD( "82s123.ic7",           0x0000,  0x0020, CRC(61b9c16c) SHA1(0ee72377831c21339360c376f7248861d476dc20) )
	ROM_LOAD_OPTIONAL( "82s123.ic16", 0x0020,  0x0020, CRC(4e779985) SHA1(cd2579cf65032c30b3fe7d6d07b89d4633687481) )	/* video switching prom, not needed for emulation purposes */
ROM_END

ROM_START( mbeepc )
	ROM_REGION(0x30000,"maincpu",0)
	ROM_LOAD("bas522a.rom",           0x8000,  0x2000, CRC(7896a696) SHA1(a158f7803296766160e1f258dfc46134735a9477) )
	ROM_LOAD("bas522b.rom",           0xa000,  0x2000, CRC(b21d9679) SHA1(332844433763331e9483409cd7da3f90ac58259d) )
	ROM_LOAD("charrom.bin",           0x11000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0) )
	ROM_RELOAD( 0x17000, 0x1000 )

	ROM_LOAD_OPTIONAL("telcom31.rom", 0x18000, 0x2000, CRC(5a904a29) SHA1(3120fb65ccefeb180ab80d8d35440c70dc8452c8) )

	/* PAK option roms */
	ROM_LOAD_OPTIONAL("mwbhelp.rom",  0x20000, 0x2000, CRC(d34fae54) SHA1(5ed30636f48e9d208ce2da367ba4425782a5bce3) ) // 1
	ROM_LOAD_OPTIONAL("wbee12.rom",   0x22000, 0x2000, CRC(0fc21cb5) SHA1(33b3995988fc51ddef1568e160dfe699867adbd5) ) // 4
	ROM_LOAD_OPTIONAL("edasm.rom",    0x24000, 0x2000, CRC(1af1b3a9) SHA1(d035a997c2dbbb3918b3395a3a5a1076aa203ee5) )

	ROM_REGION( 0x0040, "proms", 0 )
	ROM_LOAD( "82s123.ic7",           0x0000,  0x0020, CRC(61b9c16c) SHA1(0ee72377831c21339360c376f7248861d476dc20) )
	ROM_LOAD_OPTIONAL( "82s123.ic16", 0x0020,  0x0020, CRC(4e779985) SHA1(cd2579cf65032c30b3fe7d6d07b89d4633687481) )	/* video switching prom, not needed for emulation purposes */
ROM_END

ROM_START( mbeepc85 )
	ROM_REGION(0x30000,"maincpu", ROMREGION_ERASEFF )
	ROM_LOAD("bas525a.rom",           0x8000,  0x2000, CRC(a6e02afe) SHA1(0495308c7e1d84b5989a3af6d3b881f4580b2641) )
	ROM_LOAD("bas525b.rom",           0xa000,  0x2000, CRC(245dd36b) SHA1(dd288f3e6737627f50d3d2a49df3e57c423d3118) )
	ROM_LOAD("charrom.bin",           0x11000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0) )
	ROM_RELOAD( 0x17000, 0x1000 )

	ROM_LOAD_OPTIONAL("telco321.rom", 0x18000, 0x2000, CRC(36852a11) SHA1(c45b8d03629e86231c6b256a7435abd87d8872a4) )

	/* PAK option roms - Wordbee must be in slot 0 and Shell must be in slot 5. */
	ROM_LOAD("wbee13.rom",            0x20000, 0x2000, CRC(d7c58b7b) SHA1(5af1b8d21a0f21534ed1833ae919dbbc6ca973e2) ) // 0
	ROM_LOAD_OPTIONAL("cmdhelp.rom",  0x22000, 0x2000, CRC(a4f1fa90) SHA1(1456abc6ed0501a3b15a99b4302750843293ae5f) ) // 1
	ROM_LOAD_OPTIONAL("edasm.rom",    0x24000, 0x2000, CRC(1af1b3a9) SHA1(d035a997c2dbbb3918b3395a3a5a1076aa203ee5) ) // 2
	ROM_LOAD_OPTIONAL("forth.rom",    0x26000, 0x2000, CRC(c0795c2b) SHA1(8faa0a46fbbdb8a1019d706a40cd4431a5063f8c) ) // 3
	ROM_LOAD("shell.rom",             0x2a000, 0x2000, CRC(5a2c7cd6) SHA1(8edc086710cb558f2146d660eddc8a18ba6a141c) ) // 5
	ROM_LOAD_OPTIONAL("ozlogo.rom",   0x2c000, 0x2000, CRC(47c3ef69) SHA1(8274d27c323ca4a6cc9e7d24946ae9c0531c3112) ) // 6

	ROM_REGION( 0x0040, "proms", 0 )
	ROM_LOAD( "82s123.ic7",           0x0000,  0x0020, CRC(61b9c16c) SHA1(0ee72377831c21339360c376f7248861d476dc20) )
	ROM_LOAD_OPTIONAL( "82s123.ic16", 0x0020,  0x0020, CRC(4e779985) SHA1(cd2579cf65032c30b3fe7d6d07b89d4633687481) )	/* video switching prom, not needed for emulation purposes */
ROM_END

ROM_START( mbeeppc )
	ROM_REGION(0x40000,"maincpu", ROMREGION_ERASEFF )
	ROM_LOAD("bas529b.rom",           0xa000,  0x2000, CRC(a1bd986b) SHA1(5d79f210c9042db5aefc85a0bdf45210cb9e9899) )
	ROM_LOAD("charrom.bin",           0x11000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0) )
	ROM_RELOAD( 0x17000, 0x1000 )

	ROM_LOAD_OPTIONAL("telco321.rom", 0x18000, 0x2000, CRC(36852a11) SHA1(c45b8d03629e86231c6b256a7435abd87d8872a4) )

	/* PAK option roms - Wordbee must be in slot 0 and Shell must be in slot 5. */
	ROM_LOAD("wbee13.rom",            0x20000, 0x2000, CRC(d7c58b7b) SHA1(5af1b8d21a0f21534ed1833ae919dbbc6ca973e2) ) // 0
	ROM_LOAD_OPTIONAL("cmdhelp.rom",  0x22000, 0x2000, CRC(a4f1fa90) SHA1(1456abc6ed0501a3b15a99b4302750843293ae5f) ) // 1
	ROM_LOAD_OPTIONAL("busycalc.rom", 0x24000, 0x4000, CRC(f2897427) SHA1(b4c351bdac72d89589980be6d654f9b931bcba6b) ) // 2
	ROM_LOAD_OPTIONAL("graphics.rom", 0x26000, 0x4000, CRC(9e9d327c) SHA1(aebf60ed153004380b9f271f2212376910a6cef9) ) // 3
	ROM_LOAD_OPTIONAL("vtex235.rom",  0x28000, 0x2000, CRC(8c30ecb2) SHA1(cf068462d7def885bdb5d3a265851b88c727c0d7) ) // 4
	ROM_LOAD("ppcshell.rom",          0x2a000, 0x2000, CRC(1e793555) SHA1(ddeaa081ec4408e80e3fb192865d87daa035c701) ) // 5
	ROM_LOAD_OPTIONAL("ozlogo.rom",   0x2c000, 0x2000, CRC(47c3ef69) SHA1(8274d27c323ca4a6cc9e7d24946ae9c0531c3112) ) // 6
	ROM_LOAD("bas529a.rom",           0x30000, 0x4000, CRC(fe8242e1) SHA1(ff790edf4fcc7a134d451dbad7779157b07f6abf) )

	ROM_REGION( 0x0040, "proms", 0 )
	ROM_LOAD( "82s123.ic7",           0x0000,  0x0020, CRC(61b9c16c) SHA1(0ee72377831c21339360c376f7248861d476dc20) )
	ROM_LOAD_OPTIONAL( "82s123.ic16", 0x0020,  0x0020, CRC(4e779985) SHA1(cd2579cf65032c30b3fe7d6d07b89d4633687481) )	/* video switching prom, not needed for emulation purposes */
ROM_END

ROM_START( mbee56 )
	ROM_REGION(0x18000,"maincpu",0)
	ROM_LOAD("56kb.rom",              0xe000,  0x1000, CRC(28211224) SHA1(b6056339402a6b2677b0e6c57bd9b78a62d20e4f) )
	ROM_LOAD("charrom.bin",           0x11000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0) )
	ROM_RELOAD( 0x17000, 0x1000 )

	ROM_REGION( 0x0040, "proms", 0 )
	ROM_LOAD( "82s123.ic7",           0x0000,  0x0020, CRC(61b9c16c) SHA1(0ee72377831c21339360c376f7248861d476dc20) )
	ROM_LOAD_OPTIONAL( "82s123.ic16", 0x0020,  0x0020, CRC(4e779985) SHA1(cd2579cf65032c30b3fe7d6d07b89d4633687481) )	/* video switching prom, not needed for emulation purposes */
ROM_END

ROM_START( mbee64 )
	ROM_REGION(0x18000,"maincpu",0)
	ROM_LOAD("rom1.bin",              0xdf00,  0x2000, CRC(995c53db) SHA1(46e1a5cfd5795b8cf528bacf9dc79398ff7d64af) )
	ROM_LOAD("charrom.bin",           0x11000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0) )
	ROM_RELOAD( 0x17000, 0x1000 )

	ROM_REGION( 0x0040, "proms", 0 )
	ROM_LOAD( "82s123.ic7",           0x0000,  0x0020, CRC(61b9c16c) SHA1(0ee72377831c21339360c376f7248861d476dc20) )
	ROM_LOAD_OPTIONAL( "82s123.ic16", 0x0020,  0x0020, CRC(4e779985) SHA1(cd2579cf65032c30b3fe7d6d07b89d4633687481) )	/* video switching prom, not needed for emulation purposes */
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT       COMPANY         FULLNAME */
COMP( 1982, mbee,     0,	0,	mbee,     mbee,     mbee,     		"Applied Technology",  "Microbee 16 Standard" , 0)
COMP( 1982, mbeeic,   mbee,	0,	mbeeic,   mbee,     mbeeic,   		"Applied Technology",  "Microbee 32 IC" , 0)
COMP( 1982, mbeepc,   mbee,	0,	mbeepc,   mbee,     mbeepc,   		"Applied Technology",  "Microbee 32 Personal Communicator" , 0)
COMP( 1985, mbeepc85, mbee,	0,	mbeepc85, mbee,     mbeepc85, 		"Applied Technology",  "Microbee 32 PC85" , 0)
COMP( 1985, mbeeppc,  mbee,	0,	mbeeppc,  mbee,     mbeeppc,  		"Applied Technology",  "Microbee 32 Premium PC85" , GAME_NOT_WORKING)
COMP( 1986, mbee56,   mbee,	0,	mbee56,   mbee,     mbee56,   		"Applied Technology",  "Microbee 56k" , 0 )
COMP( 1986, mbee64,   mbee,	0,	mbee64,   mbee,     mbee64,   		"Applied Technology",  "Microbee 64k" , GAME_NOT_WORKING)

