/***************************************************************************

Emulation file for Keytronic KB3270/PC keyboard.

This keyboard supports both the PC/XT and the PC/AT keyboard protocols. The
desired protocol can be selected by a switch on the keybaord.

SW1 is connected to Port 1 of a 8031.
  P1.0 - On  - Standard PC and XT
         Off - Enhanced XT, AT and PS/2 Models.
  P1.1 - On  - IRMA Emulation
         Off - Native Scan Code Set
  P1.2 - On  - Enhanced 101 Scan Code Set
         Off - Native Scan Code Set
  P1.3 - On  - Disable E0
         Off - Enable E0
  P1.4 - On  - Int'l Code Tables
         Off - U.S. Code Tables
  P1.5 - Off - Not Used
  P1.6 - Off - Not Used
  P1.7 - On  - Key click
         Off - No key click

TODO:
- Verify P1 boot up behavior. Right now we ignore the signals when
  P1 != 0xFF. (Code at 0x0FCF)

Keyboard matrix.

Z5 = XR 22-00950-001
Z6 = SN74LS132N
Z7 = XR 22-00950-001
Z8 = SN74LS374N
Z11 = XR 22-908-03B


              Z11 pin 8
              |                Z11 pin 7
              |                |        Z11 pin 6
              |                |        |        Z11 pin 5
              |                |        |        |        Z11 pin 4
  |           |                |        |        |        |
  F1 -------- F2 ------------- F3 ----- F4 ----- F5 ----- F6 -------- J2 pin 2 - Z5 pin 19
  |           |                |        |        |        |
  F7 -------- F8 ------------- LShift - < ------ Z ------ X --------- J2 pin 1 - Z5 pin 18
  |           |                |        |        |        |
  F9 -------- F10 ------------ LCtrl -- LAlt --- Space -- RAlt ------ J1 pin 9 - Z5 pin 17
  |           |                |        |        |        |
  1 --------- ` -------------- Q ------ TAB ---- A ------ Caps Lock - J1 pin 8 - Z5 pin 16
  |           |                |        |        |        |
  2 --------- 3 -------------- W ------ E ------ S ------ D --------- J1 pin 7 - Z5 pin 15
  |           |                |        |        |        |
  5 --------- 4 -------------- T ------ R ------ G ------ F --------- J2 pin 3 - Z7 pin 20
  |           |                |        |        |        |
  N --------- M -------------- B ------ V ------ C ------ , -------------------- Z7 pin 19
  |           |                |        |        |        |
  6 --------- 7 -------------- Y ------ U ------ H ------ J -------------------- Z7 pin 18
  |           |                |        |        |        |
  9 --------- 8 -------------- O ------ I ------ L ------ K -------------------- Z7 pin 17
  |           |                |        |        |        |
  Pad 2 ----- Pad 1 ---------- Unused - Unused - Down --- Enter ---------------- Z7 pin 16
  |           |                |        |        |        |
  |           / -------------- Rshift - Rshift - Left --- . --------- J9 pin 8 - Z7 pin 15
  |           |                |        |        |        |
  0 --------- - -------------- P ------ [ ------ ; ------ ' --------- J9 pin 7 - Z7 pin 14
  |           |                |        |        |        |
  Backspace - = -------------- Return - \ ------ Return - ] --------- J9 pin 6 - Z7 pin 13
  |           |                |        |        |        |
  Backspace - PA1/Dup -------- |<-- --- /a\ ---- Pad + -- Pad + ----- J9 pin 5 - Z7 pin 12
  |           |                |        |        |        |
  Esc ------- NumLock -------- Pad 7 -- Pad 8 -- Pad 4 -- Pad 5 ---------------- Can't finish trace, goes under SW1
  |           |                |        |        |        |
  SysReq ---- ScrLk ---------- -->| --- Pad 9 -- Pad - -- Pad 6 ---------------- Can't finish trace, goes under Z8 between pins 5 and 6
  |           |                |        |        |        |
  Pad 3 ----- Pad 0 ---------- Pad 0 -- Pad . -- Unused - Up ------------------- Z7 pin 7
  |           |                |        |        |        |
  PrtSc * --- PA2/Field Mark - Right -- /a ----- Center - Unused --------------- Can't finish trace, SW1 blocks view
  |           |                |        |        |        |

The 'Return', 'Backspace, 'Pad 0', 'Right Shift', 'Pad +'  keys on the keyboard
are attached to two switches. The keys appear twice in the keyboard matrix.

***************************************************************************/

#include "driver.h"
#include "machine/kb_keytro.h"
#include "cpu/i8051/i8051.h"

static struct {
	UINT8					p1;
	UINT8					p1_data;
	UINT8					p2;
	UINT8					p3;
	UINT8					clock_signal;
	UINT8					data_signal;
	write8_machine_func		clock_callback;
	write8_machine_func		data_callback;
	int						cpunum;
} kb_keytronic;


/***************************************************************************

  Input port declaration

***************************************************************************/
INPUT_PORTS_START( kb_keytronic )
	PORT_START_TAG( "kb_keytronic_0b" )
PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG( "kb_keytronic_0e" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG( "kb_keytronic_0f" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_5)			PORT_CHAR('5')						/* 06 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_4)			PORT_CHAR('4')						/* 05 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_T)			PORT_CHAR('T')						/* 14 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_R)			PORT_CHAR('R')						/* 13 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_G)			PORT_CHAR('G')						/* 22 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F)			PORT_CHAR('F')						/* 21 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_RIGHT)		PORT_NAME("Cursor Right (MF2?)")	/* 6a */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F7)			PORT_CHAR(UCHAR_MAMEKEY(F7))		/* 41 */

	PORT_START_TAG( "kb_keytronic_30" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F12)			PORT_CHAR(UCHAR_MAMEKEY(F12))		/* 58 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_M)			PORT_CHAR('M')						/* 32 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_B)			PORT_CHAR('B')						/* 30 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 5b */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 5c */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 5d */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_END)			PORT_NAME("End (MF2?)")				/* 6b */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F8)			PORT_CHAR(UCHAR_MAMEKEY(F8))		/* 42 */

	PORT_START_TAG( "kb_keytronic_31" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_ASTERISK)		PORT_NAME("KP *")					/* 37 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_7)			PORT_CHAR('7')						/* 08 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_LSHIFT)		PORT_CHAR(UCHAR_MAMEKEY(LSHIFT))	/* 2a */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 70 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_Z)			PORT_CHAR('Z')						/* 2c */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_J)			PORT_CHAR('J')						/* 24 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_DOWN)			PORT_NAME("Cursor Down (MF2?)")		/* 6c */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F9)			PORT_CHAR(UCHAR_MAMEKEY(F9))		/* 43 */

	PORT_START_TAG( "kb_keytronic_32" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_9)			PORT_CHAR('9')						/* 0a */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_LCONTROL)		PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))	/* 1d */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 71 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_LALT)			PORT_NAME("Alt")					/* 38 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_SPACE)		PORT_NAME("Space")					/* 39 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_LALT)			PORT_NAME("Alt")					/* 38 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_LEFT)			PORT_NAME("Cursor Left (MF2?)")		/* 69 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F6)			PORT_CHAR(UCHAR_MAMEKEY(F6))		/* 40 */

	PORT_START_TAG( "kb_keytronic_33" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_1)			PORT_CHAR('1')						/* 02 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_TILDE)		PORT_CHAR('`')						/* 29 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_Q)			PORT_CHAR('Q')						/* 10 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_TAB)			PORT_CHAR(9)						/* 0f */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_A)			PORT_CHAR('A')						/* 1e */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_CAPSLOCK)		PORT_NAME("Caps")					/* 3a */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_PGUP)			PORT_NAME("Page Up (MF2?)")			/* 68 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F5)			PORT_CHAR(UCHAR_MAMEKEY(F5))		/* 3f */

	PORT_START_TAG( "kb_keytronic_34" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_2)			PORT_CHAR('2')						/* 02 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_3)			PORT_CHAR('3')						/* 03 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_W)			PORT_CHAR('W')						/* 11 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_E)			PORT_CHAR('E')						/* 12 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_S)			PORT_CHAR('S')						/* 1f */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_D)			PORT_CHAR('D')						/* 20 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_UP)			PORT_NAME("Cursor Up (MF2?)")		/* 67 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F4)			PORT_CHAR(UCHAR_MAMEKEY(F4))		/* 3e */

	PORT_START_TAG( "kb_keytronic_35" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_0)			PORT_CHAR('0')						/* 0b */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-')						/* 0c */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_P)			PORT_CHAR('P')						/* 19 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[')						/* 1a */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';')						/* 27 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR('\'')						/* 28 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_HOME)			PORT_NAME("Home (MF2?)")			/* 66 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F3)			PORT_CHAR(UCHAR_MAMEKEY(F3))		/* 3d */

	PORT_START_TAG( "kb_keytronic_36" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(8)						/* 0e */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('=')						/* 0d */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)						/* 1c */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\\')						/* 2b */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']')						/* 1b */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_PAUSE)		PORT_NAME("Pause (MF2?)")			/* 65 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F2)			PORT_CHAR(UCHAR_MAMEKEY(F2))		/* 3c */

	PORT_START_TAG( "kb_keytronic_37" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 7b */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 7e */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 7a */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_PLUS_PAD)		PORT_NAME("KP +")					/* 4e */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_RALT)			PORT_NAME("AltGr (MF2?)")			/* 64 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F1)			PORT_CHAR(UCHAR_MAMEKEY(F1))		/* 3b */

	PORT_START_TAG( "kb_keytronic_38" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 54 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_SCRLOCK)		PORT_NAME("ScrLock")				/* 46 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 7c */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_9_PAD)		PORT_NAME("KP 9")					/* 49 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_MINUS_PAD)	PORT_NAME("KP -")					/* 4a */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_6_PAD)		PORT_NAME("KP 6")					/* 4d */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG( "kb_keytronic_39" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_ESC)			PORT_NAME("Esc")					/* 01 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_NUMLOCK)		PORT_NAME("NumLock")				/* 45 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_7_PAD)		PORT_NAME("KP 7")					/* 47 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_8_PAD)		PORT_NAME("KP 8")					/* 48 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_4_PAD)		PORT_NAME("KP 4")					/* 4b */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_5_PAD)		PORT_NAME("KP 5")					/* 4c */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 76 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_PRTSCR)		PORT_NAME("PrtScr (MF2?)")			/* 63 */

	PORT_START_TAG( "kb_keytronic_3a" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_DEL)			PORT_NAME("Delete (MF2?)")			/* 6f */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 7f */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 7d */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 79 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 77 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_INSERT)		PORT_NAME("Insert (MF2?)")			/* 6e */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_SLASH_PAD)	PORT_NAME("KP / (MF2?)")			/* 62 */

	PORT_START_TAG( "kb_keytronic_3b" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_3_PAD)		PORT_NAME("KP 3")					/* 51 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_0_PAD)		PORT_NAME("KP 0")					/* 52 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_DEL_PAD)		PORT_NAME("KP .")					/* 53 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	/* 78 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_PGDN)			PORT_NAME("Page Down (MF2?)")		/* 6d */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F10)			PORT_CHAR(UCHAR_MAMEKEY(F10))		/* 44 */

INPUT_PORTS_END


/* Write handler which is called when the clock signal may have changed */
WRITE8_HANDLER( kb_keytronic_set_clock_signal )
{
	kb_keytronic.clock_signal = data;
	cpunum_set_input_line( machine, kb_keytronic.cpunum, I8051_INT0_LINE, data ? HOLD_LINE : CLEAR_LINE );
}


/* Write handler which is called when the data signal may have changed */
WRITE8_HANDLER( kb_keytronic_set_data_signal )
{
	kb_keytronic.data_signal = data;
//printf("kb_keytronic_set_data_signal(): data = %d\n", data);
	cpunum_set_input_line( machine, kb_keytronic.cpunum, I8051_T0_LINE, data ? HOLD_LINE : CLEAR_LINE );
}


void kb_keytronic_set_host_interface( running_machine *machine, write8_machine_func clock_cb, write8_machine_func data_cb )
{
	kb_keytronic.clock_callback = clock_cb;
	kb_keytronic.data_callback = data_cb;
	kb_keytronic.p3 = 0xff;
	kb_keytronic.cpunum = mame_find_cpu_index( machine, KEYTRONIC_KB3270PC_CPU );
}


static READ8_HANDLER( kb_keytronic_p1_r )
{
	UINT8 data = kb_keytronic.p1_data;

	return data;
}


static WRITE8_HANDLER( kb_keytronic_p1_w )
{
	logerror("kb_keytronic_p1_w(): write %02x\n", data );

	switch( data )
	{
	case 0x0b:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_0b" );
		kb_keytronic.p1_data = 0xFF;
		break;
	case 0x0e:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_0e" );
		kb_keytronic.p1_data = 0xFE;
		break;
	case 0x0f:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_0f" );
		break;
	case 0x30:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_30" );
		break;
	case 0x31:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_31" );
		break;
	case 0x32:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_32" );
		break;
	case 0x33:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_33" );
		break;
	case 0x34:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_34" );
		break;
	case 0x35:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_35" );
		break;
	case 0x36:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_36" );
		break;
	case 0x37:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_37" );
		break;
	case 0x38:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_38" );
		break;
	case 0x39:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_39" );
		break;
	case 0x3a:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_3a" );
		break;
	case 0x3b:
		kb_keytronic.p1_data = input_port_read( machine, "kb_keytronic_3b" );
		break;
	}
	kb_keytronic.p1 = data;
}


static READ8_HANDLER( kb_keytronic_p2_r )
{
	UINT8 data = kb_keytronic.p2;

	return data;
}


static WRITE8_HANDLER( kb_keytronic_p2_w )
{
	logerror("kb_keytronic_p2_w(): write %02x\n", data );

	kb_keytronic.p2 = data;
}


static READ8_HANDLER( kb_keytronic_p3_r )
{
	UINT8 data = kb_keytronic.p3;

	data &= ~ 0x14;

	/* -INT0 signal */
	data |= ( kb_keytronic.clock_signal ? 0x04 : 0x00 );

	/* T0 signal */
	data |= ( kb_keytronic.data_signal ? 0x00 : 0x10 );

//printf("kb_keytronic_p3_r(): %02x\n", data);
	return data;
}


static WRITE8_HANDLER( kb_keytronic_p3_w )
{
	logerror("kb_keytronic_p3_w(): write %02x\n", data );
}


static READ8_HANDLER( kb_keytronic_data_r )
{
	logerror("kb_keytronic_data_r(): read from %04x\n", offset );

	if ( kb_keytronic.p1 == 0xFF )
	{
		kb_keytronic.data_signal = ( offset & 0x0100 ) ? 1 : 0;
		kb_keytronic.clock_signal = ( offset & 0x0200 ) ? 1 : 0;

		kb_keytronic.data_callback( machine, 0, kb_keytronic.data_signal );
		kb_keytronic.clock_callback( machine, 0, kb_keytronic.clock_signal );

//printf("data = %d, clock = %d\n", kb_keytronic.data_signal, kb_keytronic.clock_signal );
	}
	return 0xFF;
}


static WRITE8_HANDLER( kb_keytronic_data_w )
{
	if ( kb_keytronic.p1 == 0xFF )
	{
		kb_keytronic.data_signal = ( offset & 0x0100 ) ? 1 : 0;
		kb_keytronic.clock_signal = ( offset & 0x0200 ) ? 1 : 0;

		kb_keytronic.data_callback( machine, 0, kb_keytronic.data_signal );
		kb_keytronic.clock_callback( machine, 0, kb_keytronic.clock_signal );
	
//printf("data = %d, clock = %d\n", kb_keytronic.data_signal, kb_keytronic.clock_signal );
	}
}


static ADDRESS_MAP_START( kb_keytronic_program, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x0FFF )	AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( kb_keytronic_data, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE( 0x0000, 0xFFFF ) AM_READWRITE( kb_keytronic_data_r, kb_keytronic_data_w )
ADDRESS_MAP_END


static ADDRESS_MAP_START( kb_keytronic_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( 0x01, 0x01 )	AM_READWRITE( kb_keytronic_p1_r, kb_keytronic_p1_w )
	AM_RANGE( 0x02, 0x02 )	AM_READWRITE( kb_keytronic_p2_r, kb_keytronic_p2_w )
	AM_RANGE( 0x03, 0x03 )	AM_READWRITE( kb_keytronic_p3_r, kb_keytronic_p3_w )
ADDRESS_MAP_END


MACHINE_DRIVER_START( kb_keytronic )
	MDRV_CPU_ADD( KEYTRONIC_KB3270PC_CPU, I8051, 11060250 )
	MDRV_CPU_PROGRAM_MAP( kb_keytronic_program, 0 )
	MDRV_CPU_DATA_MAP( kb_keytronic_data, 0 )
	MDRV_CPU_IO_MAP( kb_keytronic_io, 0 )
MACHINE_DRIVER_END


