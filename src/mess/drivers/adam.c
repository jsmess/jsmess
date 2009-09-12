/***************************************************************************

Full schematics of anything Adam at http://drushel.cwru.edu/schematics/

  adam.c

  Driver file to handle emulation of the ColecoAdam.

  Marat Fayzullin (ColEm source)
  Marcel de Kogel (AdamEm source)
  Mike Balfour
  Ben Bruscella
  Sean Young
  Jose Moya


The Coleco ADAM is a Z80-based micro with all peripheral devices
attached to an internal serial serial bus (ADAMnet) managed by 6801
microcontrollers (processor, internal RAM, internal ROM, serial port).Each
device had its own 6801, and the ADAMnet was managed by a "master" 6801 on
the ADAM motherboard.  Each device was allotted a block of 21 bytes in Z80
address space; device control was accomplished by poking function codes into
the first byte of each device control block (hereafter DCB) after setup of
other DCB locations with such things as:  buffer address in Z80 space, # of
bytes to transfer, block # to access if it was a block device like a tape
or disk drive, etc.  The master 6801 would interpret this data, and pass
along internal ADAMnet requests to the desired peripheral, which would then
send/receive its data and return the status of the operation.  The status
codes were left in the same byte of the DCB as the function request, and
certain bits of the status byte would reflect done/working on it/error/
not present, and error codes were left in another DCB byte for things like
CRC error, write protected disk, missing block, etc.

ADAM's ROM operating system, EOS (Elementary OS), was constructed
similar to CP/M in that it provided both a filesystem (like BDOS) and raw
device interface (BIOS).  At the file level, sequential files could be
created, opened, read, written, appended, closed, and deleted.  Forward-
only random access was implemented (you could not move the R/W pointer
backward, except clear to the beginning!), and all files had to be
contiguous on disk/tape.  Directories could be initialized or searched
for a matching filename (no wildcards allowed).  At the device level,
individual devices could be read/written by block (for disks/tapes) or
character-by-character (for printer, keyboard, and a prototype serial
board which was never released).  Devices could be checked for their
ADAMnet status, and reset if necessary.  There was no function provided
to do low-level formatting of disks/tapes.

 At system startup, the EOS was loaded from ROM into the highest
8K of RAM, a function call made to initialize the ADAMnet, and then
any disks or tapes were checked for a boot medium; if found, block 0 of
the medium was loaded in, and a jump made to the start of the boot code.
The boot block would take over loading in the rest of the program.  If no
boot media were found, a jump would be made to a ROM word processor (called
SmartWriter).

 Coleco designed the ADAMnet to have up to 15 devices attached.
Before they went bankrupt, Coleco had released a 64K memory expander and
a 300-baud internal modem, but surprisingly neither of these was an
ADAMnet device.  Disassembly of the RAMdisk drivers in ADAM CP/M 2.2, and
of the ADAMlink terminal program revealed that these were simple port I/O
devices, banks of XRAM being accessed by a special memory switch port not
documented as part of the EOS.  The modem did not even use the interrupt
capabilities of the Z80--it was simply polled.  A combination serial/
parallel interface, each port of which *was* an ADAMnet device, reached the
prototype stage, as did a 5MB hard disk, but neither was ever released to
the public.  (One prototype serial/parallel board is still in existence,
but the microcontroller ROMs have not yet been succcessfully read.)  So
when Coleco finally bailed out of the computer business, a maximum ADAM
system consisted of a daisy wheel printer, a keyboard, 2 tape drives, and
2 disk drives (all ADAMnet devices), a 64K expander and a 300-baud modem
(which were not ADAMnet devices).

 Third-party vendors reverse-engineered the modem (which had a
2651 UART at its heart) and made a popular serial interface board.  It was
not an ADAMnet device, however, because nobody knew how to make a new ADAMnet
device (no design specs were ever released), and the 6801 microcontrollers
had unreadable mask ROMs.  Disk drives, however, were easily upgraded from
160K to as high as 1MB because, for some unknown reason, the disk controller
boards used a separate microprocessor and *socketed* EPROM (which was
promptly disassembled and reworked).  Hard drives were cobbled together from
a Kaypro-designed board and accessed as standard I/O port devices.  A parallel
interface card was similarly set up at its own I/O port.

  Devices (15 max):
    Device 0 = Master 6801 ADAMnet controller (uses the adam_pcb as DCB)
    Device 1 = Keyboard
    Device 2 = ADAM printer
    Device 3 = Copywriter (projected)
    Device 4 = Disk drive 1
    Device 5 = Disk drive 2
    Device 6 = Disk drive 3 (third party)
    Device 7 = Disk drive 4 (third party)
    Device 8 = Tape drive 1
    Device 9 = Tape drive 3 (projected)
    Device 10 = Unused
    Device 11 = Non-ADAMlink modem
    Device 12 = Hi-resolution monitor
    Device 13 = ADAM parallel interface (never released)
    Device 14 = ADAM serial interface (never released)
    Device 15 = Gateway
    Device 24 = Tape drive 2 (share DCB with Tape1)
    Device 25 = Tape drive 4 (projected, may have share DCB with Tape3)
    Device 26 = Expansion RAM disk drive (third party ID, not used by Coleco)

  Terminology:
    EOS = Elementary Operating System
    DCB = Device Control Block Table (21bytes each DCB, DCB+16=dev#, DCB+0=Status Byte) (0xFD7C)

           0     Status byte
         1-2     Buffer start address (lobyte, hibyte)
         3-4     Buffer length (lobyte, hibyte)
         5-8     Block number accessed (loword, hiword in lobyte, hibyte format)
           9     High nibble of device number
        10-15    Always zero (unknown purpose)
          16     Low nibble of device number
        17-18    Maximum block length (lobyte, hibyte)
          19     Device type (0 for block device, 1 for character device)
          20     Node type

        - Writing to byte0 requests the following operations:
            1     Return current status
            2     Soft reset
            3     Write
            4     Read


    FCB = File Control Block Table (32bytes, 2 max each application) (0xFCB0)
    OCB = Overlay Control Block Table
    adam_pcb = Processor Control Block Table, 4bytes (adam_pcb+3 = Number of valid DCBs) (0xFEC0 relocatable), current adam_pcb=[0xFD70]
            adam_pcb+0 = Status, 0=Request Status of Z80 -> must return 0x81..0x82 to sync Master 6801 clk with Z80 clk
            adam_pcb+1,adam_pcb+2 = address of adam_pcb start
            adam_pcb+3 = device #

            - Writing to byte0:
                1   Synchronize the Z80 clock (should return 0x81)
                2   Synchronize the Master 6801 clock (should return 0x82)
                3   Relocate adam_pcb

            - Status values:
                0x80 -> Success
                0x81 -> Z80 clock in sync
                0x82 -> Master 6801 clock in sync
                0x83 -> adam_pcb relocated
                0x9B -> Time Out

    DEV_ID = Device id


    The ColecoAdam I/O map is contolled by the MIOC (Memory Input Output Controller):

            20-3F (W) = Adamnet Writes
            20-3F (R) = Adamnet Reads

            42-42 (W) = Expansion RAM page selection, only useful if expansion greater than 64k

            40-40 (W) = Printer Data Out
            40-40 (R) = Printer (Returns 0x41)

            5E-5E (RW)= Modem Data I/O
            5F-5F (RW)= Modem Data Control Status

            60-7F (W) = Set Memory configuration
            60-7F (R) = Read Memory configuration

            80-9F (W) = Set both controllers to keypad mode
            80-9F (R) = Not Connected

            A0-BF (W) = Video Chip (TMS9928A), A0=0 -> Write Register 0 , A0=1 -> Write Register 1
            A0-BF (R) = Video Chip (TMS9928A), A0=0 -> Read Register 0 , A0=1 -> Read Register 1

            C0-DF (W) = Set both controllers to joystick mode
            C0-DF (R) = Not Connected

            E0-FF (W) = Sound Chip (SN76489A)
            E0-FF (R) = Read Controller data, A1=0 -> read controller 1, A1=1 -> read controller 2

TO DO:
    - Improve Keyboard "Simulation" (No ROM dumps available for the keyboard MC6801 AdamNet MCU)
    - Add Tape "Simulation" (No ROM dumps available for the Tape MC6801 AdamNet MCU)
    - Add Disc "Simulation" (No ROM dumps available for the Disc MC6801 AdamNet MCU)
    - Add Full ColecoAdam emulation (every MC6801) if ROM dumps become available.
        Do you have the ROM dumps?... let us know.

***************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/sn76496.h"
#include "video/tms9928a.h"
#include "includes/adam.h"
#include "cpu/m6800/m6800.h"
#include "devices/cartslot.h"
#include "devices/flopdrv.h"
#include "formats/adam_dsk.h"

static ADDRESS_MAP_START( adam_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x01fff) AM_READWRITE( SMH_BANK(1), SMH_BANK(6) )
	AM_RANGE(0x02000, 0x03fff) AM_READWRITE( SMH_BANK(2), SMH_BANK(7) )
	AM_RANGE(0x04000, 0x05fff) AM_READWRITE( SMH_BANK(3), SMH_BANK(8) )
	AM_RANGE(0x06000, 0x07fff) AM_READWRITE( SMH_BANK(4), SMH_BANK(9) )
	AM_RANGE(0x08000, 0x0ffff) AM_READWRITE( SMH_BANK(5), adam_common_writes_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START ( adam_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x20, 0x3F) AM_READWRITE( adamnet_r, adamnet_w )
	AM_RANGE(0x60, 0x7F) AM_READWRITE( adam_memory_map_controller_r, adam_memory_map_controller_w )
	AM_RANGE(0x80, 0x9F) AM_WRITE( adam_paddle_toggle_off )
	AM_RANGE(0xA0, 0xBF) AM_READWRITE( adam_video_r, adam_video_w )
	AM_RANGE(0xC0, 0xDF) AM_WRITE( adam_paddle_toggle_on )
	AM_RANGE(0xE0, 0xFF) AM_READ( adam_paddle_r )
	AM_RANGE(0xE0, 0xFF) AM_DEVWRITE( "sn76489a", sn76496_w )
ADDRESS_MAP_END

#ifdef UNUSED_FUNCTION
/*
I do now know the real memory map of the Master 6801...
and the 6801 ASM code is a replacement coded for this driver.
*/
static ADDRESS_MAP_START( master6801_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0100, 0x3fff) AM_ROM /* Replacement Master ROM code */
	AM_RANGE( 0x4000, 0xffff) AM_READWRITE( master6801_ram_r, master6801_ram_w ) /* RAM Memory shared with Z80 not banked*/
ADDRESS_MAP_END
#endif

static INPUT_PORTS_START( adam )
    PORT_START( "controller1_keypad1" )  /* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("P1 Keypad 0") PORT_CODE(KEYCODE_0)         PORT_PLAYER(1)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("P1 Keypad 1") PORT_CODE(KEYCODE_1)         PORT_PLAYER(1)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("P1 Keypad 2") PORT_CODE(KEYCODE_2)         PORT_PLAYER(1)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("P1 Keypad 3") PORT_CODE(KEYCODE_3)         PORT_PLAYER(1)
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_NAME("P1 Keypad 4") PORT_CODE(KEYCODE_4)         PORT_PLAYER(1)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON10) PORT_NAME("P1 Keypad 5") PORT_CODE(KEYCODE_5)         PORT_PLAYER(1)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON11) PORT_NAME("P1 Keypad 6") PORT_CODE(KEYCODE_6)         PORT_PLAYER(1)
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON12) PORT_NAME("P1 Keypad 7") PORT_CODE(KEYCODE_7)         PORT_PLAYER(1)

    PORT_START( "controller1_keypad2" )  /* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON13) PORT_NAME("P1 Keypad 8") PORT_CODE(KEYCODE_8)         PORT_PLAYER(1)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON14) PORT_NAME("P1 Keypad 9") PORT_CODE(KEYCODE_9)         PORT_PLAYER(1)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON15) PORT_NAME("P1 Keypad #") PORT_CODE(KEYCODE_MINUS)     PORT_PLAYER(1)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON16) PORT_NAME("P1 Keypad *") PORT_CODE(KEYCODE_EQUALS)    PORT_PLAYER(1)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Button 2 (SAC Red Button)")             PORT_PLAYER(1)
    PORT_BIT( 0xB0, IP_ACTIVE_LOW, IPT_UNUSED )

    PORT_START( "controller1_joystick" )  /* IN2 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )                                                   PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )                                                PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )                                                 PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )                                                 PORT_PLAYER(1) PORT_8WAY
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Button 1 (SAC Yellow Button)")          PORT_PLAYER(1)
    PORT_BIT( 0xB0, IP_ACTIVE_LOW, IPT_UNUSED )

    PORT_START( "controller2_keypad1" )  /* IN3 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("P2 Keypad 0") PORT_CODE(KEYCODE_0_PAD)     PORT_PLAYER(2)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("P2 Keypad 1") PORT_CODE(KEYCODE_1_PAD)     PORT_PLAYER(2)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("P2 Keypad 2") PORT_CODE(KEYCODE_2_PAD)     PORT_PLAYER(2)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("P2 Keypad 3") PORT_CODE(KEYCODE_3_PAD)     PORT_PLAYER(2)
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_NAME("P2 Keypad 4") PORT_CODE(KEYCODE_4_PAD)     PORT_PLAYER(2)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON10) PORT_NAME("P2 Keypad 5") PORT_CODE(KEYCODE_5_PAD)     PORT_PLAYER(2)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON11) PORT_NAME("P2 Keypad 6") PORT_CODE(KEYCODE_6_PAD)     PORT_PLAYER(2)
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON12) PORT_NAME("P2 Keypad 7") PORT_CODE(KEYCODE_7_PAD)     PORT_PLAYER(2)

    PORT_START( "controller2_keypad2" )  /* IN4 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON13) PORT_NAME("P2 Keypad 8") PORT_CODE(KEYCODE_8_PAD)     PORT_PLAYER(2)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON14) PORT_NAME("P2 Keypad 9") PORT_CODE(KEYCODE_9_PAD)     PORT_PLAYER(2)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON15) PORT_NAME("P2 Keypad #") PORT_CODE(KEYCODE_MINUS_PAD) PORT_PLAYER(2)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON16) PORT_NAME("P2 Keypad *") PORT_CODE(KEYCODE_PLUS_PAD)  PORT_PLAYER(2)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2)  PORT_NAME("P2 Button 2 (SAC Red Button)")             PORT_PLAYER(2)
    PORT_BIT( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START( "controller2_joystick" )  /* IN5 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)                                                    PORT_PLAYER(2) PORT_8WAY
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)                                                 PORT_PLAYER(2) PORT_8WAY
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)                                                  PORT_PLAYER(2) PORT_8WAY
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)                                                  PORT_PLAYER(2) PORT_8WAY
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("P2 Button 1 (SAC Yellow Button)")           PORT_PLAYER(2)
    PORT_BIT( 0xB0, IP_ACTIVE_LOW, IPT_UNUSED )

    PORT_START( "controllers" ) /* IN6 */
    PORT_CONFNAME(0x0F, 0x00, "Controllers")
    PORT_CONFSETTING(0x00, DEF_STR( None ) )
    PORT_CONFSETTING(0x01, "Driving Controller" )
    PORT_CONFSETTING(0x02, "Roller Controller" )
    PORT_CONFSETTING(0x04, "Super Action Controllers" )
    PORT_CONFSETTING(0x08, "Standard Controllers" )

    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_NAME("P1 SAC Blue Button")   PORT_CODE(KEYCODE_Z) PORT_PLAYER(1)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_NAME("P1 SAC Purple Button") PORT_CODE(KEYCODE_X) PORT_PLAYER(1)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_NAME("P2 SAC Blue Button")   PORT_CODE(KEYCODE_Q) PORT_PLAYER(2)
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_NAME("P2 SAC Purple Button") PORT_CODE(KEYCODE_W) PORT_PLAYER(2)

    PORT_START( "controller1_analog" ) /* IN7, to emulate Extra Controls (Driving Controller, SAC P1 slider, Roller Controller X Axis) */
    PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_X) \
              PORT_SENSITIVITY(20) \
              PORT_KEYDELTA(10) \
              PORT_CODE_DEC(KEYCODE_J) \
              PORT_CODE_INC(KEYCODE_L) \
              PORT_PLAYER(1) \
              PORT_RESET

    PORT_START( "controller2_analog" ) /* IN8, to emulate Extra Controls (SAC P2 slider, Roller Controller Y Axis) */
    PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_X) \
              PORT_SENSITIVITY(20) \
              PORT_KEYDELTA(10) \
              PORT_CODE_DEC(KEYCODE_I) \
              PORT_CODE_INC(KEYCODE_K) \
              PORT_PLAYER(2) \
              PORT_RESET

/* Keyboard with 75 Keys - TODO: Discover if the keys are actually organized this way on real hardware */

	PORT_START("keyboard_1") /* IN9 0*/
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Escape/WP")    PORT_CODE(KEYCODE_ESC)        PORT_CHAR(27)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_A)          PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_B)          PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_C)          PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_D)          PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_E)          PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_F)          PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_G)          PORT_CHAR('g') PORT_CHAR('G')

	PORT_START("keyboard_2") /* IN10 1*/
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_H)          PORT_CHAR('h') PORT_CHAR('H') PORT_CHAR(8)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_I)          PORT_CHAR('i') PORT_CHAR('I') PORT_CHAR(9)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_J)          PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_K)          PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_L)          PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m M Return")   PORT_CODE(KEYCODE_M)          PORT_CHAR('m') PORT_CHAR('M') PORT_CHAR(13)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_N)          PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_O)          PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("keyboard_3") /* IN11 2*/
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_P)          PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_Q)          PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_R)          PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_S)          PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_T)          PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_U)          PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_V)          PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_W)          PORT_CHAR('w') PORT_CHAR('W')

	PORT_START("keyboard_4") /* IN12 3*/
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_X)          PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_Y)          PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_Z)          PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_TILDE)      PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_BACKSPACE)  PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_MINUS)      PORT_CHAR('-') PORT_CHAR('`')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_COLON)      PORT_CHAR(';') PORT_CHAR(':')

	PORT_START("keyboard_5") /* IN13 4*/
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_0)          PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_1)          PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_2)          PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_3)          PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_4)          PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_5)          PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_6)          PORT_CHAR('6') PORT_CHAR('_')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_7)          PORT_CHAR('7') PORT_CHAR('&')

	PORT_START("keyboard_6") /* IN14 5*/
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_8)          PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_9)          PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_QUOTE)      PORT_CHAR('\'') PORT_CHAR('\"')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_EQUALS)     PORT_CHAR('+') PORT_CHAR('=')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_COMMA)      PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_STOP)       PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_SLASH)      PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR('[') PORT_CHAR('{')

	PORT_START("keyboard_7") /* IN15 6*/
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x85\xA0") PORT_CODE(KEYCODE_F3)         PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x85\xA1") PORT_CODE(KEYCODE_F4)         PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x85\xA2") PORT_CODE(KEYCODE_F5)         PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x85\xA3") PORT_CODE(KEYCODE_F6)         PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x85\xA4") PORT_CODE(KEYCODE_F7)         PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x85\xA5") PORT_CODE(KEYCODE_F8)         PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space")        PORT_CODE(KEYCODE_SPACE)      PORT_CHAR(32)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return")       PORT_CODE(KEYCODE_ENTER)      PORT_CHAR(13)

	PORT_START("keyboard_8") /* IN16 7*/
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Wild Card")    PORT_CODE(KEYCODE_F1)         PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Undo")         PORT_CODE(KEYCODE_F9)         PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Move/Copy")    PORT_CODE(KEYCODE_INSERT)     PORT_CHAR(UCHAR_MAMEKEY(PGUP))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Store/Get")    PORT_CODE(KEYCODE_HOME)       PORT_CHAR(UCHAR_MAMEKEY(PGDN))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Insert")       PORT_CODE(KEYCODE_DEL)        PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Print")        PORT_CODE(KEYCODE_END)        PORT_CHAR(UCHAR_MAMEKEY(PRTSCR))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Clear")        PORT_CODE(KEYCODE_PGUP)       PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Delete")       PORT_CODE(KEYCODE_PGDN)       PORT_CHAR(UCHAR_MAMEKEY(DEL))

	PORT_START("keyboard_9") /* IN17 8*/
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Home")         PORT_CODE(KEYCODE_5_PAD)      PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x87\xA7") PORT_CODE(KEYCODE_8_PAD)      PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x87\xA9") PORT_CODE(KEYCODE_2_PAD)      PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x87\xA6") PORT_CODE(KEYCODE_4_PAD)      PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x87\xA8") PORT_CODE(KEYCODE_6_PAD)      PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab")          PORT_CODE(KEYCODE_TAB)        PORT_CHAR(UCHAR_MAMEKEY(TAB))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left Shift")   PORT_CODE(KEYCODE_LSHIFT)     PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right Shift")  PORT_CODE(KEYCODE_RSHIFT)     PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("keyboard_10") /* IN18 9*/
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Control")      PORT_CODE(KEYCODE_CAPSLOCK)   PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backspace")    PORT_CODE(KEYCODE_F10)        PORT_CHAR(8)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Lock")         PORT_CODE(KEYCODE_LCONTROL)   PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK)) PORT_TOGGLE
	PORT_BIT(0xF8, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END

/***************************************************************************

  The interrupts come from the vdp. The vdp (tms9928a) interrupt can go up
  and down; the Coleco only uses nmi interrupts (which is just a pulse). They
  are edge-triggered: as soon as the vdp interrupt line goes up, an interrupt
  is generated. Nothing happens when the line stays up or goes down.

  To emulate this correctly, we set a callback in the tms9928a (they
  can occur mid-frame). At every frame we call the TMS9928A_interrupt
  because the vdp needs to know when the end-of-frame occurs, but we don't
  return an interrupt.

***************************************************************************/

static INTERRUPT_GEN( adam_interrupt )
{
    TMS9928A_interrupt(device->machine);
    adam_explore_keyboard(device->machine);
}

static void adam_vdp_interrupt (running_machine *machine, int state)
{
	static int last_state = 0;

    /* only if it goes up */
	if (state && !last_state)
    {
        cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);
    }
	last_state = state;
}

static TIMER_CALLBACK(adam_paddle_callback)
{
	int port7 = input_port_read(machine, "controller1_analog");
	int port8 = input_port_read(machine, "controller2_analog");

	if (port7 == 0)
		adam_joy_stat[0] = 0;
	else if (port7 & 0x08)
		adam_joy_stat[0] = -1;
	else
		adam_joy_stat[0] = 1;

	if (port8 == 0)
		adam_joy_stat[1] = 0;
	else if (port8 & 0x08)
		adam_joy_stat[1] = -1;
	else
		adam_joy_stat[1] = 1;

	if (adam_joy_stat[0] || adam_joy_stat[1])
		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
}

void adam_set_memory_banks(running_machine *machine)
{
/*
Lineal virtual memory map:

0x00000, 0x07fff -> Lower Internal RAM
0x08000, 0x0ffff -> Upper Internal RAM
0x10000, 0x17fff -> Lower Expansion RAM
0x18000, 0x1ffff -> Upper Expansion RAM
0x20000, 0x27fff -> SmartWriter ROM
0x28000, 0x2ffff -> Cartridge
0x30000, 0x31fff -> OS7 ROM (ColecoVision ROM)
0x32000, 0x39fff -> EOS ROM
0x3A000, 0x41fff -> Used to Write Protect ROMs
*/
	UINT8 *BankBase;
	BankBase = &memory_region(machine, "maincpu")[0x00000];

	switch (adam_lower_memory)
	{
		case 0: /* SmartWriter ROM */
			if (adam_net_data & 0x02)
			{
				/* Read */
				memory_set_bankptr(machine, 1, BankBase+0x32000); /* No data here */
				memory_set_bankptr(machine, 2, BankBase+0x34000); /* No data here */
				memory_set_bankptr(machine, 3, BankBase+0x36000); /* No data here */
				memory_set_bankptr(machine, 4, BankBase+0x38000); /* EOS ROM */

				/* Write */
				memory_set_bankptr(machine, 6, BankBase+0x3A000); /* Write protecting ROM */
				memory_set_bankptr(machine, 7, BankBase+0x3A000); /* Write protecting ROM */
				memory_set_bankptr(machine, 8, BankBase+0x3A000); /* Write protecting ROM */
				memory_set_bankptr(machine, 9, BankBase+0x3A000); /* Write protecting ROM */
			}
			else
			{
				/* Read */
				memory_set_bankptr(machine, 1, BankBase+0x20000); /* SmartWriter ROM */
				memory_set_bankptr(machine, 2, BankBase+0x22000);
				memory_set_bankptr(machine, 3, BankBase+0x24000);
				memory_set_bankptr(machine, 4, BankBase+0x26000);

				/* Write */
				memory_set_bankptr(machine, 6, BankBase+0x3A000); /* Write protecting ROM */
				memory_set_bankptr(machine, 7, BankBase+0x3A000); /* Write protecting ROM */
				memory_set_bankptr(machine, 8, BankBase+0x3A000); /* Write protecting ROM */
				memory_set_bankptr(machine, 9, BankBase+0x3A000); /* Write protecting ROM */
			}
			break;
		case 1: /* Internal RAM */
			/* Read */
			memory_set_bankptr(machine, 1, BankBase);
			memory_set_bankptr(machine, 2, BankBase+0x02000);
			memory_set_bankptr(machine, 3, BankBase+0x04000);
			memory_set_bankptr(machine, 4, BankBase+0x06000);

			/* Write */
			memory_set_bankptr(machine, 6, BankBase);
			memory_set_bankptr(machine, 7, BankBase+0x02000);
			memory_set_bankptr(machine, 8, BankBase+0x04000);
			memory_set_bankptr(machine, 9, BankBase+0x06000);
			break;
		case 2: /* RAM Expansion */
			/* Read */
			memory_set_bankptr(machine, 1, BankBase+0x10000);
			memory_set_bankptr(machine, 2, BankBase+0x12000);
			memory_set_bankptr(machine, 3, BankBase+0x14000);
			memory_set_bankptr(machine, 4, BankBase+0x16000);

			/* Write */
			memory_set_bankptr(machine, 6, BankBase+0x10000);
			memory_set_bankptr(machine, 7, BankBase+0x12000);
			memory_set_bankptr(machine, 8, BankBase+0x14000);
			memory_set_bankptr(machine, 9, BankBase+0x16000);
			break;
		case 3: /* OS7 ROM (8k) + Internal RAM (24k) */
			/* Read */
			memory_set_bankptr(machine, 1, BankBase+0x30000);
			memory_set_bankptr(machine, 2, BankBase+0x02000);
			memory_set_bankptr(machine, 3, BankBase+0x04000);
			memory_set_bankptr(machine, 4, BankBase+0x06000);

			/* Write */
			memory_set_bankptr(machine, 6, BankBase+0x3A000); /* Write protecting ROM */
			memory_set_bankptr(machine, 7, BankBase+0x02000);
			memory_set_bankptr(machine, 8, BankBase+0x04000);
			memory_set_bankptr(machine, 9, BankBase+0x06000);
	}

	switch (adam_upper_memory)
	{
		case 0: /* Internal RAM */
			/* Read */
			memory_set_bankptr(machine, 5, BankBase+0x08000);
			/*memory_set_bankptr(machine, 10, BankBase+0x08000);*/
			break;
		case 1: /* ROM Expansion */
			break;
		case 2: /* RAM Expansion */
			/* Read */
			memory_set_bankptr(machine, 5, BankBase+0x18000);
			/*memory_set_bankptr(machine, 10, BankBase+0x18000);*/
			break;
		case 3: /* Cartridge ROM */
			/* Read */
			memory_set_bankptr(machine, 5, BankBase+0x28000);
			/*memory_set_bankptr(machine, 10, BankBase+0x3A000); *//* Write protecting ROM */
			break;
	}
}

void adam_reset_pcb(running_machine *machine)
{
    int i;
    memory_region(machine, "maincpu")[adam_pcb] = 0x01;

	for (i = 0; i < 15; i++)
		memory_region(machine, "maincpu")[(adam_pcb+4+i*21)&0xFFFF]=i+1;
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS99x8A,
	0x4000,
	0, 0,
	adam_vdp_interrupt
};

static MACHINE_START( adam )
{
	TMS9928A_configure(&tms9928a_interface);
}

static MACHINE_RESET( adam )
{
	const device_config *img;

	img = devtag_get_device(machine, "cart");

	if (image_exists(img))
	{
		/* ColecoVision Mode Reset (Cartridge Mounted) */
		adam_lower_memory = 3; /* OS7 + 24k RAM */
		adam_upper_memory = 3; /* Cartridge ROM */
	}
	else
	{
		/* Adam Mode Reset */
		adam_lower_memory = 0; /* SmartWriter ROM/EOS */
		adam_upper_memory = 0; /* Internal RAM */
	}

	adam_net_data = 0;
	adam_set_memory_banks(machine);
	adam_pcb=0xFEC0;
	adam_clear_keyboard_buffer();

	memset(&memory_region(machine, "maincpu")[0x0000], 0xFF, 0x20000); /* Initializing RAM */
	timer_pulse(machine, ATTOTIME_IN_MSEC(20), NULL, 0, adam_paddle_callback);
}

static MACHINE_DRIVER_START( adam )
	/* Machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 3579545)       /* 3.579545 MHz */
	MDRV_CPU_PROGRAM_MAP(adam_mem)
	MDRV_CPU_IO_MAP(adam_io)

    /* Master M6801 AdamNet controller */
	//MDRV_CPU_ADD("adamnet", M6800, 4000000)       /* 4.0 MHz */
	//MDRV_CPU_PROGRAM_MAP(master6801_mem, 0)

	MDRV_CPU_VBLANK_INT("screen", adam_interrupt)

	MDRV_MACHINE_START( adam )
	MDRV_MACHINE_RESET( adam )

    /* video hardware */
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(50)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sn76489a", SN76489A, 3579545)	/* 3.579545 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	
	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,col,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*
os7.rom CRC(3AA93EF3)
eos.rom CRC(05A37A34)
wp.rom  CRC(58D86A2A)
*/
/*
Total Memory Size: 64k Internal RAM, 64k Expansion RAM, 32k SmartWriter ROM, 8k OS7, 32k Cartridge, 32k Extra

Lineal virtual memory map:

0x00000, 0x07fff -> Lower Internal RAM
0x08000, 0x0ffff -> Upper Internal RAM
0x10000, 0x17fff -> Lower Expansion RAM
0x18000, 0x1ffff -> Upper Expansion RAM
0x20000, 0x27fff -> SmartWriter ROM
0x28000, 0x2ffff -> Cartridge
0x30000, 0x31fff -> OS7 ROM (ColecoVision ROM)
0x32000, 0x39fff -> EOS ROM
0x3A000, 0x41fff -> Used to Write Protect ROMs
0x42000, 0x47fff -> Low unused EOS ROM
*/
ROM_START (adam)
	ROM_REGION( 0x42000, "maincpu", 0)
	ROM_LOAD ("wp.rom", 0x20000, 0x8000, CRC(58d86a2a) SHA1(d4aec4efe1431e56fe52d83baf9118542c525255))
	ROM_LOAD ("os7.rom", 0x30000, 0x2000, CRC(3aa93ef3) SHA1(45bedc4cbdeac66c7df59e9e599195c778d86a92))
	ROM_LOAD ("eos.rom", 0x38000, 0x2000, CRC(05a37a34) SHA1(ad3c20ef444f10af7ae8eb75c81e500d9b1bba3d))

	ROM_CART_LOAD("cart", 0x28000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)

	//ROM_REGION( 0x10000, "cpu1", 0)
	//ROM_LOAD ("master68.rom", 0x0100, 0x0E4, CRC(619a47b8)) /* Replacement 6801 Master ROM */
ROM_END

static void adam_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_adam; break;

		default:										floppy_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(adam)
	CONFIG_DEVICE(adam_floppy_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    CONFIG  COMPANY FULLNAME */
COMP( 1982, adam,   0,		coleco,	adam,   adam,   0,		adam,	"Coleco", "Adam" , 0)

