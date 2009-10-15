/***************************************************************************

    Bondwell 12/14

    12/05/2009 Skeleton driver.

    - Z80A CPU 4MHz
    - 64KB RAM (BW 12), 128KB RAM (BW 14)
    - 4KB ROM System
    - NEC765A Floppy controller
    - 2 x 5.25" Floppy drives 48 tpi SSDD (BW 12), DSDD (BW 14)
    - MC6845 Video controller
    - 2KB RAM Video buffer
    - 4KB ROM Character set
    - Z80SIO Serial interface
    - MC6821 Parallel interface
    - I8253 Counter-timer
    - MC1408 8-bit DAC sound
    - KB3600 PRO (AY-5-3600 PRO) Keyboard controller

    http://www.eld.leidenuniv.nl/~moene/Home/sitemap/
    http://www.baltissen.org/newhtm/schemas.htm

****************************************************************************/

#include "driver.h"
#include "includes/bw12.h"
#include "cpu/z80/z80.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "machine/6821pia.h"
#include "machine/ctronics.h"
#include "machine/nec765.h"
#include "machine/pit8253.h"
#include "machine/rescap.h"
#include "machine/z80sio.h"
#include "machine/kb3600.h"
#include "video/mc6845.h"
#include "sound/dac.h"
#include "devices/messram.h"
/*

    TODO:

    - Osborne 1 DD disk format
    - floppy motor off timer

*/

INLINE const device_config *get_floppy_image(running_machine *machine, int drive)
{
	return floppy_get_device(machine, drive);
}

static void bw12_bankswitch(running_machine *machine)
{
	bw12_state *state = machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	switch (state->bank)
	{
	case 0: /* ROM */
		memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_UNMAP);
		break;

	case 1: /* BK0 */
		memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		break;

	case 2: /* BK1 */
	case 3: /* BK2 */
		if (messram_get_size(devtag_get_device(machine, "messram")) > 64*1024)
		{
			memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		}
		else
		{
			memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		}
		break;
	}

	memory_set_bank(machine, 1, state->bank);
}

static TIMER_CALLBACK( floppy_motor_off_tick )
{
	bw12_state *state = machine->driver_data;

	floppy_drive_set_motor_state(get_floppy_image(machine, 0), 0);
	floppy_drive_set_motor_state(get_floppy_image(machine, 1), 0);
	floppy_drive_set_ready_state(get_floppy_image(machine, 0), 0, 0);
	floppy_drive_set_ready_state(get_floppy_image(machine, 1), 0, 0);

	state->motor_on = 0;
}

static void bw12_set_floppy_motor_off_timer(running_machine *machine)
{
	bw12_state *state = machine->driver_data;

	if (state->motor0 || state->motor1)
	{
		state->motor_on = 1;
		timer_enable(state->floppy_motor_off_timer, 0);
	}
	else
	{
		/* trigger floppy motor off NE556 timer */
		/*

            R18 = RES_K(100)
            C11 = CAP_U(4.7)

        */
		timer_adjust_oneshot(state->floppy_motor_off_timer, attotime_zero, 0);
	}
}

static void ls259_w(running_machine *machine, int address, int data)
{
	bw12_state *state = machine->driver_data;

	switch (address)
	{
	case 0: /* LS138 A0 */
		state->bank = (state->bank & 0x02) | data;
		bw12_bankswitch(machine);
		break;

	case 1: /* LS138 A1 */
		state->bank = (data << 1) | (state->bank & 0x01);
		bw12_bankswitch(machine);
		break;

	case 2: /* not connected */
		break;

	case 3: /* _INIT */
		break;

	case 4: /* CAP LOCK */
		output_set_led_value(0, data);
		break;

	case 5: /* MOTOR 0 */
		state->motor0 = data;

		if (data)
		{
			floppy_drive_set_motor_state(get_floppy_image(machine, 0), 1);
			floppy_drive_set_ready_state(get_floppy_image(machine, 0), 1, 0);
		}

		bw12_set_floppy_motor_off_timer(machine);
		break;

	case 6: /* MOTOR 1 */
		state->motor1 = data;

		if (data)
		{
			floppy_drive_set_motor_state(get_floppy_image(machine, 1), 1);
			floppy_drive_set_ready_state(get_floppy_image(machine, 1), 1, 0);
		}

		bw12_set_floppy_motor_off_timer(machine);
		break;

	case 7: /* FDC TC */
		nec765_tc_w(state->nec765, data);
		break;
	}
}

static WRITE8_HANDLER( bw12_ls259_w )
{
	int d = BIT(offset, 0);
	int a = (offset >> 1) & 0x07;

	ls259_w(space->machine, a, d);
}

static READ8_HANDLER( bw12_ls259_r )
{
	bw12_ls259_w(space, offset, 0);

	return 0;
}

/* Memory Maps */

static ADDRESS_MAP_START( bw12_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(1)
	AM_RANGE(0x8000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xffff) AM_RAM AM_BASE_MEMBER(bw12_state, video_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bw12_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x0f) AM_READWRITE(bw12_ls259_r, bw12_ls259_w)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x0e) AM_DEVWRITE(MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x11, 0x11) AM_MIRROR(0x0e) AM_DEVREADWRITE(MC6845_TAG, mc6845_register_r, mc6845_register_w)
	AM_RANGE(0x20, 0x20) AM_MIRROR(0x0e) AM_DEVREAD(NEC765_TAG, nec765_status_r)
	AM_RANGE(0x21, 0x21) AM_MIRROR(0x0e) AM_DEVREADWRITE(NEC765_TAG, nec765_data_r, nec765_data_w)
	AM_RANGE(0x30, 0x33) AM_MIRROR(0x0c) AM_DEVREADWRITE(PIA6821_TAG, pia6821_r, pia6821_w)
	AM_RANGE(0x40, 0x40) AM_MIRROR(0x0c) AM_DEVREADWRITE(Z80SIO_TAG, z80sio_d_r, z80sio_d_w)
	AM_RANGE(0x41, 0x41) AM_MIRROR(0x0c) AM_DEVREADWRITE(Z80SIO_TAG, z80sio_c_r, z80sio_c_w)
	AM_RANGE(0x42, 0x42) AM_MIRROR(0x0c) AM_DEVREADWRITE(Z80SIO_TAG, z80sio_d_r, z80sio_d_w)
	AM_RANGE(0x43, 0x43) AM_MIRROR(0x0c) AM_DEVREADWRITE(Z80SIO_TAG, z80sio_c_r, z80sio_c_w)
	AM_RANGE(0x50, 0x50) AM_MIRROR(0x0f) AM_DEVWRITE(MC1408_TAG, dac_w)
	AM_RANGE(0x60, 0x63) AM_MIRROR(0x0c) AM_DEVREADWRITE(PIT8253_TAG, pit8253_r, pit8253_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( bw12 )
	/*

      KB3600 PRO2 Keyboard matrix

          | Y0  | Y1  | Y2  | Y3  | Y4  | Y5  | Y6  | Y7  | Y8  | Y9  |
          |     |     |     |     |     |     |     |     |     |     |
      ----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----|
      X0  |  7  |  8  |  9  |  0  |  1  |  2  |  3  |  4  |  5  |  6  |
      ----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----|
      X1  |  U  |  I  |  O  |  P  |  Q  |  W  |  E  |  R  |  T  |  Y  |
      ----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----|
      X2  | F15 | F16 | RET | N.  | SP  | LOCK| F11 | F12 | F13 | F14 |
      ----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----|
      X3  | F7  | F8  | F9  | F10 | F1  | F2  | F3  | F4  | F5  | F6  |
      ----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----|
      X4  | LEFT|RIGHT| N3  | BS  |  @  |     |  -  |  ]  | UP  | DOWN|
      ----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----|
      X5  | N9  | CL  | N2  | LF  | DEL | HT  |ARROW|  [  | N7  | N8  |
      ----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----|
      X6  |  M  |  ,  |  .  |  /  |  Z  |  X  |  C  |  V  |  B  |  N  |
      ----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----|
      X7  |  J  |  K  |  L  |  ;  |  A  |  S  |  D  |  F  |  G  |  H  |
      ----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----|
      X8  | N6  |  -  | N1  | N0  | ESC |     |  :  | NRET| N4  | N5  |
      ----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----|

    */

	PORT_START("X0")
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('|')
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')

	PORT_START("X1")
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')

	PORT_START("X2")
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F15")
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F16")
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F11") PORT_CODE(KEYCODE_F11) PORT_CHAR(UCHAR_MAMEKEY(F11))
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F12") PORT_CODE(KEYCODE_F12) PORT_CHAR(UCHAR_MAMEKEY(F12))
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F13")
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F14")

	PORT_START("X3")
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F7") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F8") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F9") PORT_CODE(KEYCODE_F9) PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F10") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10))
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F6") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))

	PORT_START("X4")
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("BS") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('@') PORT_CHAR('\\')
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))

	PORT_START("X5")
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CL")
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LF")
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("HT")
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD)

	PORT_START("X6")
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')

	PORT_START("X7")
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')

	PORT_START("X8")
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Keypad RET") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD)

	PORT_START("MODIFIERS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
INPUT_PORTS_END

/* Video */

static MC6845_UPDATE_ROW( bw12_update_row )
{
	bw12_state *state = device->machine->driver_data;

	int column, bit;

	for (column = 0; column < x_count; column++)
	{
		UINT8 code = state->video_ram[((ma + column) & BW12_VIDEORAM_MASK)];
		UINT16 addr = code << 4 | (ra & 0x0f);
		UINT8 data = state->char_rom[addr & BW12_CHARROM_MASK];

		if (column == cursor_x)
		{
			data = 0xff;
		}

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;
			int color = BIT(data, 7);

			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}
	}
}

static const mc6845_interface bw12_mc6845_interface =
{
	SCREEN_TAG,
	8,
	NULL,
	bw12_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

static VIDEO_START( bw12 )
{
	bw12_state *state = machine->driver_data;

	/* find devices */
	state->mc6845 = devtag_get_device(machine, MC6845_TAG);

	/* find memory regions */
	state->char_rom = memory_region(machine, "chargen");
}

static VIDEO_UPDATE( bw12 )
{
	bw12_state *state = screen->machine->driver_data;

	mc6845_update(state->mc6845, bitmap, cliprect);

	return 0;
}

/* NEC765 Interface */

static WRITE_LINE_DEVICE_HANDLER( bw12_nec765_interrupt )
{
	bw12_state *driver_state = device->machine->driver_data;

	driver_state->fdcint = state;
}

static NEC765_GET_IMAGE( bw12_nec765_get_image )
{
	switch (floppy_index)
	{
	case 1: /* drive A */
		return get_floppy_image(device->machine, 0);

	case 2: /* drive B */
		return get_floppy_image(device->machine, 1);

	default:
		return NULL;
	}
}

static const struct nec765_interface bw12_nec765_interface =
{
	DEVCB_LINE(bw12_nec765_interrupt),		/* interrupt */
	NULL,						/* DMA request */
	bw12_nec765_get_image,		/* image lookup */
	NEC765_RDY_PIN_CONNECTED,	/* ready pin */
	{FLOPPY_0,FLOPPY_1, NULL, NULL}
};

/* PIA6821 Interface */

static WRITE_LINE_DEVICE_HANDLER( bw12_interrupt )
{
	cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_IRQ0, state);
}

static READ8_DEVICE_HANDLER( bw12_pia6821_pa_r )
{
	/*

        bit     description

        PA0     Input from Centronics BUSY status
        PA1     Input from Centronics ERROR status
        PA2     Input from Centronics PAPER OUT status
        PA3     Input from FDC MOTOR
        PA4     Input from PIT OUT2
        PA5     Input from keyboard strobe
        PA6     Input from keyboard serial data
        PA7     Input from FDC interrupt

    */

	bw12_state *state = device->machine->driver_data;

	UINT8 data = 0;

	data |= centronics_busy_r(state->centronics);
	data |= (centronics_fault_r(state->centronics) << 1);
	data |= (centronics_pe_r(state->centronics) << 2);
	data |= (state->motor_on << 3);
	data |= (state->pit_out2 << 4);
	data |= (state->key_stb << 5);
	data |= (state->key_sin << 6);
	data |= (state->fdcint << 7);

	return data;
}

static READ_LINE_DEVICE_HANDLER( bw12_pia6821_ca1_r )
{
	bw12_state *state = device->machine->driver_data;

	return centronics_ack_r(state->centronics);
}

static WRITE_LINE_DEVICE_HANDLER( bw12_pia6821_ca2_w )
{
	bw12_state *driver_state = device->machine->driver_data;

	centronics_strobe_w(driver_state->centronics, state);
}

static READ_LINE_DEVICE_HANDLER( bw12_pia6821_cb1_r )
{
	bw12_state *state = device->machine->driver_data;

	return state->key_stb;
}

static WRITE_LINE_DEVICE_HANDLER( bw12_pia6821_cb2_w )
{
	bw12_state *driver_state = device->machine->driver_data;

	if (state)
	{
		/* keyboard shift clock */
		driver_state->key_shift++;

		if (driver_state->key_shift < 10)
		{
			driver_state->key_sin = driver_state->key_data[driver_state->key_shift];
		}
	}
}

static const pia6821_interface bw12_pia_config =
{
	DEVCB_HANDLER(bw12_pia6821_pa_r),	/* port A input */
	DEVCB_NULL,							/* port B input */
	DEVCB_LINE(bw12_pia6821_ca1_r),		/* CA1 input */
	DEVCB_LINE(bw12_pia6821_cb1_r),		/* CB1 input */
	DEVCB_NULL,							/* CA2 input */
	DEVCB_NULL,							/* CB2 input */
	DEVCB_NULL, 						/* port A output */
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w),	/* port B output */
	DEVCB_LINE(bw12_pia6821_ca2_w),		/* CA2 output */
	DEVCB_LINE(bw12_pia6821_cb2_w),		/* CB2 output */
	DEVCB_LINE(bw12_interrupt),			/* IRQA output */
	DEVCB_LINE(bw12_interrupt)			/* IRQB output */
};

/* Centronics Interface */

static const centronics_interface bw12_centronics_intf =
{
	0,													/* is IBM PC? */
	DEVCB_DEVICE_HANDLER(PIA6821_TAG, pia6821_ca1_w),	/* ACK output */
	DEVCB_NULL,											/* BUSY output */
	DEVCB_NULL											/* NOT BUSY output */
};

/* Z80-SIO/0 Interface */

static const z80sio_interface bw12_z80sio_intf =
{
	bw12_interrupt,	/* interrupt handler */
	0,				/* DTR changed handler */
	0,				/* RTS changed handler */
	0,				/* BREAK changed handler */
	0,				/* transmit handler - which channel is this for? */
	0				/* receive handler - which channel is this for? */
};

/* PIT8253 Interface */

static WRITE_LINE_DEVICE_HANDLER( bw12_pit8253_out0_w )
{
	/* Z80-SIO TxCA, RxCA */
}

static WRITE_LINE_DEVICE_HANDLER( bw12_pit8253_out1_w )
{
	/* Z80-SIO RxTxCB */
}

static WRITE_LINE_DEVICE_HANDLER( bw12_pit8253_out2_w )
{
	bw12_state *driver_state = device->machine->driver_data;

	/* PIA6821 PA4 */
	driver_state->pit_out2 = state;
}

static const struct pit8253_config bw12_pit8253_intf =
{
	{
		{
			XTAL_1_8432MHz,
			bw12_pit8253_out0_w,
		},
		{
			XTAL_1_8432MHz,
			bw12_pit8253_out1_w,
		},
		{
			XTAL_1_8432MHz,
			bw12_pit8253_out2_w,
		}
	}
};

/* AY-5-3600-PRO-002 Interface */

static AY3600_Y_READ( bw2_ay3600_y_r )
{
	UINT16 data = 0;

	switch (x)
	{
	case 0: data = input_port_read(device->machine, "X0"); break;
	case 1: data = input_port_read(device->machine, "X1"); break;
	case 2: data = input_port_read(device->machine, "X2"); break;
	case 3: data = input_port_read(device->machine, "X3"); break;
	case 4: data = input_port_read(device->machine, "X4"); break;
	case 5: data = input_port_read(device->machine, "X5"); break;
	case 6: data = input_port_read(device->machine, "X6"); break;
	case 7: data = input_port_read(device->machine, "X7"); break;
	case 8: data = input_port_read(device->machine, "X8"); break;
	}

	return data;
}

static READ_LINE_DEVICE_HANDLER( bw2_ay3600_shift_r )
{
	return BIT(input_port_read(device->machine, "MODIFIERS"), 0);
}

static READ_LINE_DEVICE_HANDLER( bw2_ay3600_control_r )
{
	return BIT(input_port_read(device->machine, "MODIFIERS"), 1);
}

static WRITE_LINE_DEVICE_HANDLER( bw2_ay3600_data_ready_w )
{
	bw12_state *driver_state = device->machine->driver_data;

	driver_state->key_stb = state;

	pia6821_cb1_w(driver_state->pia6821, 0, state);

	if (state)
	{
		UINT16 data = ay3600_b_r(device);

		driver_state->key_data[0] = BIT(data, 6);
		driver_state->key_data[1] = BIT(data, 3);
		driver_state->key_data[2] = BIT(data, 1);
		driver_state->key_data[3] = BIT(data, 0);
		driver_state->key_data[4] = BIT(data, 2);
		driver_state->key_data[5] = BIT(data, 4);
		driver_state->key_data[6] = BIT(data, 5);
		driver_state->key_data[7] = BIT(data, 7);
		driver_state->key_data[8] = BIT(data, 8);

		driver_state->key_shift = 0;
		driver_state->key_sin = driver_state->key_data[driver_state->key_shift];
	}
}

static AY3600_INTERFACE( bw12_ay3600_intf )
{
	bw2_ay3600_y_r,							/* Y input */
	DEVCB_LINE(bw2_ay3600_shift_r),			/* shift */
	DEVCB_LINE(bw2_ay3600_control_r),		/* control */
	DEVCB_LINE(bw2_ay3600_data_ready_w),	/* data ready */
	DEVCB_NULL								/* any key down */
};

/* Machine Initialization */

static MACHINE_START( bw12 )
{
	bw12_state *state = machine->driver_data;

	/* find devices */
	state->pia6821 = devtag_get_device(machine, PIA6821_TAG);
	state->nec765 = devtag_get_device(machine, NEC765_TAG);
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);

	/* allocate floppy motor off timer */
	state->floppy_motor_off_timer = timer_alloc(machine, floppy_motor_off_tick, NULL);

	/* setup memory banking */
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, Z80_TAG), 0);
	memory_configure_bank(machine, 1, 1, 1, messram_get_ptr(devtag_get_device(machine, "messram")), 0);
	memory_configure_bank(machine, 1, 2, 2, messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000, 0x8000);

	/* register for state saving */
	state_save_register_global(machine, state->bank);
	state_save_register_global(machine, state->pit_out2);
	state_save_register_global_array(machine, state->key_data);
	state_save_register_global(machine, state->key_sin);
	state_save_register_global(machine, state->key_stb);
	state_save_register_global(machine, state->key_shift);
	state_save_register_global(machine, state->fdcint);
	state_save_register_global(machine, state->motor_on);
	state_save_register_global(machine, state->motor0);
	state_save_register_global(machine, state->motor1);
}

static MACHINE_RESET( bw12 )
{
	int i;

	for (i = 0; i < 8; i++)
	{
		ls259_w(machine, i, 0);
	}
}

static FLOPPY_OPTIONS_START(bw12)
	FLOPPY_OPTION(bw12, "dsk", "180KB BW 12 SSDD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
	FLOPPY_OPTION(bw12, "dsk", "360KB BW 14 DSDD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
	FLOPPY_OPTION(bw12, "dsk", "SVI-328 SSDD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([17])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
	FLOPPY_OPTION(bw12, "dsk", "SVI-328 DSDD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([17])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
	FLOPPY_OPTION(bw12, "dsk", "Kaypro II SSDD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([0]))
FLOPPY_OPTIONS_END

static const floppy_config bw12_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(bw12),
	DO_NOT_KEEP_GEOMETRY
};

/* Machine Driver */
static MACHINE_DRIVER_START( bw12 )
	MDRV_DRIVER_DATA(bw12_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_16MHz/4)
    MDRV_CPU_PROGRAM_MAP(bw12_mem)
    MDRV_CPU_IO_MAP(bw12_io)

    MDRV_MACHINE_START(bw12)
    MDRV_MACHINE_RESET(bw12)

    /* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(monochrome_amber)

    MDRV_VIDEO_START(bw12)
    MDRV_VIDEO_UPDATE(bw12)

	MDRV_MC6845_ADD(MC6845_TAG, MC6845, XTAL_16MHz/8, bw12_mc6845_interface)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(MC1408_TAG, DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_NEC765A_ADD(NEC765_TAG, bw12_nec765_interface)
	MDRV_PIA6821_ADD(PIA6821_TAG, bw12_pia_config)
	MDRV_Z80SIO_ADD(Z80SIO_TAG, XTAL_16MHz/4, bw12_z80sio_intf) /* Z80-SIO/0 */
	MDRV_PIT8253_ADD(PIT8253_TAG, bw12_pit8253_intf)
	MDRV_AY3600PRO002_ADD(AY3600_TAG, bw12_ay3600_intf)

	/* printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, bw12_centronics_intf)

	MDRV_FLOPPY_2_DRIVES_ADD(bw12_floppy_config)
	
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bw14 )
	MDRV_IMPORT_FROM(bw12)
	
	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( bw12 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "bw14boot.ic41", 0x0000, 0x1000, CRC(782fe341) SHA1(eefe5ad6b1ef77a1caf0af743b74de5fa1c4c19d) )

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "bw14char.ic1",  0x0000, 0x1000, CRC(f9dd68b5) SHA1(50132b759a6d84c22c387c39c0f57535cd380411) )
ROM_END

#define rom_bw14 rom_bw12

/* System Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    CONFIG  COMPANY                             FULLNAME        FLAGS */
COMP( 1984,	bw12,	0,		0,		bw12, 	bw12,	0,		0,	"Bondwell International Limited",   "Bondwell 12",	GAME_SUPPORTS_SAVE )
COMP( 1984,	bw14,	bw12,	0,		bw14,	bw12,	0,		0,	"Bondwell International Limited",   "Bondwell 14",	GAME_SUPPORTS_SAVE )
