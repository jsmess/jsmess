/***************************************************************************

        Elwro 800 Junior

        Driver by Mariusz Wojcieszek

		ToDo:
		- map keys with Polish characters
		- add reading of computer number (read of FE port when bit 4 of port F7 is set)
		- printing (LPRINT from ZX Basic) hangs waiting for bit 0 of DD register (port C of PPI8255),
		  may be a bug in handshake implementation of PPI (both PPI implementations are affected)
		- 8251 DTR and DTS signals are connected (with some additional logic) to NMI of Z80, this
		  is not emulated
		- 8251 is used for JUNET network (a network of Elwro 800 Junior computers, allows sharing
		  floppy disc drives and printers) - network is not emulated
		- when RAM is mapped at 0x0000 - 0x1fff (in CP/J mode), reading a location 66 with /M1=0
		  (effectively reading NMI vector) is hardwired to return 0xDF (RST #18) - this is not emulated
		  (note that in CP/J mode address 66 is used for FCB)

****************************************************************************/

#include "driver.h"
#include "includes/spectrum.h"

/* Components */
#include "cpu/z80/z80.h"
#include "machine/nec765.h"	/* for floppy disc controller */
#include "machine/i8255a.h"
#include "sound/speaker.h"
#include "sound/wave.h"
#include "machine/ctronics.h"
#include "machine/msm8251.h"

/* Devices */
#include "devices/dsk.h"		/* for CPCEMU style disk images */
#include "devices/cassette.h"
#include "formats/tzx_cas.h"

/*************************************
 *
 *  NEC765/Floppy drive
 *
 *************************************/

static const struct nec765_interface elwro800jr_nec765_interface =
{
	DEVCB_NULL,
	NULL,
	NULL,
	NEC765_RDY_PIN_CONNECTED
};

static WRITE8_HANDLER(elwro800jr_fdc_control_w)
{
	const device_config *fdc = devtag_get_device(space->machine, "nec765");

	floppy_drive_set_motor_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 0), (data & 0x01));
	floppy_drive_set_motor_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 1), (data & 0x02));
	floppy_drive_set_ready_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 0), 1,1);
	floppy_drive_set_ready_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 1), 1,1);

	nec765_tc_w(fdc, data & 0x04);

	nec765_reset_w(fdc, !(data & 0x08));
}

/*************************************
 *
 *  I/O port F7: memory mapping
 *
 *************************************/

static void elwro800jr_mmu_w(running_machine *machine, UINT8 data)
{
	UINT8 *prom = memory_region(machine, "proms") + 0x200;
	UINT8 cs;
	UINT8 ls175;
	
	ls175 = BITSWAP8(data, 7, 6, 5, 4, 4, 5, 7, 6) & 0x0f;

	cs = prom[((0x0000 >> 10) | (ls175 << 6)) & 0x1ff];
	if (!BIT(cs,0))
	{
		// rom BAS0
		memory_set_bankptr(machine, 1, memory_region(machine, "maincpu") + 0x0000); /* BAS0 ROM */
		memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, 0x1fff, 0, 0, SMH_NOP);
	}
	else if (!BIT(cs,4))
	{
		// rom BOOT
		memory_set_bankptr(machine, 1, memory_region(machine, "maincpu") + 0x4000); /* BOOT ROM */
		memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, 0x1fff, 0, 0, SMH_NOP);
	}
	else
	{
		// RAM
		memory_set_bankptr(machine, 1, mess_ram);
		memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, 0x1fff, 0, 0, SMH_BANK(1));
	}

	cs = prom[((0x2000 >> 10) | (ls175 << 6)) & 0x1ff];
	if (!BIT(cs,1))
	{
		memory_set_bankptr(machine, 2, memory_region(machine, "maincpu") + 0x2000);	/* BAS1 ROM */
		memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x2000, 0x3fff, 0, 0, SMH_NOP);
	}
	else
	{
		memory_set_bankptr(machine, 2, mess_ram + 0x2000); /* RAM */
		memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x2000, 0x3fff, 0, 0, SMH_BANK(2));
	}

	if (BIT(ls175,2))
	{
		// relok
		spectrum_screen_location = mess_ram + 0xe000;
	}
	else
	{
		spectrum_screen_location = mess_ram + 0x4000;
	}

	if (BIT(ls175,3))
	{
		logerror("Reading network number\n");
	}
}

/*************************************
 *
 *  8255: joystick and Centronics printer connections
 *
 *************************************/

static READ8_DEVICE_HANDLER(i8255_port_c_r)
{
	const device_config *printer = devtag_get_device(device->machine, "centronics");
	return (centronics_ack_r(printer) << 2);
}

static WRITE8_DEVICE_HANDLER(i8255_port_c_w)
{
	const device_config *printer = devtag_get_device(device->machine, "centronics");
	centronics_strobe_w(printer, (data >> 7) & 0x01);
}

static I8255A_INTERFACE(elwro800jr_ppi8255_interface)
{
	DEVCB_INPUT_PORT("JOY"),
	DEVCB_DEVICE_HANDLER("centronics", centronics_data_r),
	DEVCB_HANDLER(i8255_port_c_r),
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER("centronics", centronics_data_w),
	DEVCB_HANDLER(i8255_port_c_w)
};

static const centronics_interface elwro800jr_centronics_interface =
{
	FALSE,
	DEVCB_DEVICE_LINE("ppi8255", i8255a_pc2_w),
	DEVCB_NULL,
	DEVCB_NULL
};

/*************************************
 *
 *  I/O reads and writes
 *
 *  I/O accesses are decoded by prom which uses 8 low address lines (A0-A8) as input
 *  and outputs chip select signals for system components. Standard adresses are:
 *
 *  0x1F: 8255 port A (joystick)
 *  0xBE: 8251 data (Junet network)
 *  0xBF: 8251 control/status (Junet network)
 *  0xDC: 8255 control
 *  0xDD: 8255 port C (centronics 7-strobe, 2-ack)
 *  0xDE: 8255 port B (centronics data)
 *  0xDF: 8255 port A (joystick)
 *  0xEE: FDC 765A status
 *  0xEF: FDC 765A command and data
 *  0xF1: FDC control (motor on/off)
 *  0xF7: memory banking
 *  0xFE (write): border color, speaker and tape (as in Spectrum)
 *  0x??FE, 0x??7F, 0x??7B (read): keyboard reading
 *************************************/

static READ8_HANDLER(elwro800jr_io_r)
{
	UINT8 *prom = memory_region(space->machine, "proms");
	UINT8 cs = prom[offset & 0x1ff];

	if (!BIT(cs,0))
	{
		// CFE
		int mask = 0x8000;
		int data = 0xff;
		int i;
		char port_name[6] = "LINE0";

		for (i = 0; i < 9; mask >>= 1, i++)
		{
			if (!(offset & mask))
			{
				if (i == 8)
				{
					port_name[4] = '8';
				}
				else
				{
					port_name[4] = '0' + (7 - i);
				}
				data &= (input_port_read(space->machine, port_name));
			}
		}

		if ((offset & 0xff) == 0xfb)
		{
			data &= input_port_read(space->machine, "LINE9");
		}

		/* cassette input from wav */
		if (cassette_input(devtag_get_device(space->machine, "cassette")) > 0.0038 )
		{
			data &= ~0x40;
		}

		return data;
	}
	else if (!BIT(cs,1))
	{
		// CF7
	}
	else if (!BIT(cs,2))
	{
		// CS55
		const device_config *ppi = devtag_get_device(space->machine, "ppi8255");
		return i8255a_r(ppi, (offset & 0x03) ^ 0x03);
		//return ppi8255_r(ppi, (offset & 0x03) ^ 0x03);
	}
	else if (!BIT(cs,3))
	{
		// CSFDC
		const device_config *fdc = devtag_get_device(space->machine, "nec765");
		if (offset & 1)
		{
			return nec765_data_r(fdc,0);
		}
		else
		{
			return nec765_status_r(fdc,0);
		}
	}
	else if (!BIT(cs,4))
	{
		// CS51
		const device_config *usart = devtag_get_device(space->machine, "msm8251");
		if (offset & 1)
		{
			return msm8251_status_r(usart, 0);
		}
		else
		{
			return msm8251_data_r(usart, 0);
		}
	}
	else if (!BIT(cs,5))
	{
		// CF1
	}
	else
	{
		logerror("Unmapped I/O read: %04x\n", offset);
	}
	return 0x00;
}

static WRITE8_HANDLER(elwro800jr_io_w)
{
	UINT8 *prom = memory_region(space->machine, "proms");
	UINT8 cs = prom[offset & 0x1ff];

	if (!BIT(cs,0))
	{
		// CFE
		spectrum_port_fe_w(space, 0, data);
	}
	else if (!BIT(cs,1))
	{
		// CF7
		elwro800jr_mmu_w(space->machine, data);
	}
	else if (!BIT(cs,2))
	{
		// CS55
		const device_config *ppi = devtag_get_device(space->machine, "ppi8255");
		i8255a_w(ppi, (offset & 0x03) ^ 0x03, data);
		//ppi8255_w(ppi, (offset & 0x03) ^ 0x03, data);
	}
	else if (!BIT(cs,3))
	{
		// CSFDC
		const device_config *fdc = devtag_get_device(space->machine, "nec765");
		if (offset & 1)
		{
			nec765_data_w(fdc, 0, data);
		}
	}
	else if (!BIT(cs,4))
	{
		// CS51
		const device_config *usart = devtag_get_device(space->machine, "msm8251");
		if (offset & 1)
		{
			msm8251_control_w(usart, 0, data);
		}
		else
		{
			msm8251_data_w(usart, 0, data);
		}
	}
	else if (!BIT(cs,5))
	{
		// CF1
		elwro800jr_fdc_control_w(space, 0, data);
	}
	else
	{
		logerror("Unmapped I/O write: %04x %02x\n", offset, data);
	}
}

/*************************************
 *
 *  Memory maps
 *
 *************************************/

static ADDRESS_MAP_START(elwro800_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x1fff) AM_RAMBANK(1)
	AM_RANGE(0x2000, 0x3fff) AM_RAMBANK(2)
	AM_RANGE(0x4000, 0xffff) AM_RAMBANK(3)
ADDRESS_MAP_END

static ADDRESS_MAP_START(elwro800_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(elwro800jr_io_r, elwro800jr_io_w)
ADDRESS_MAP_END

/*************************************
 *
 *  Input ports
 *
 *************************************/

static INPUT_PORTS_START( elwro800 )
	/* PORT_NAME =  KEY Mode    CAPS Mode    SYMBOL Mode   EXT Mode   EXT+Shift Mode   BASIC Mode  */
	PORT_START("LINE0") /* 0xFEFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z    Z    :      LN       BEEP   COPY") PORT_CODE(KEYCODE_Z)		PORT_CHAR('z') PORT_CHAR('Z') PORT_CHAR(':')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x    X    \xC2\xA3   EXP      INK    CLEAR") PORT_CODE(KEYCODE_X)	PORT_CHAR('x') PORT_CHAR('X') PORT_CHAR('\xA3')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c    C    ?      LPRINT   PAPER  CONT") PORT_CODE(KEYCODE_C)		PORT_CHAR('c') PORT_CHAR('C') PORT_CHAR('?')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v    V    /      LLIST    FLASH  CLS") PORT_CODE(KEYCODE_V)		PORT_CHAR('v') PORT_CHAR('V') PORT_CHAR('/')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":    *") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";    +") PORT_CODE(KEYCODE_COLON)

	PORT_START("LINE1") /* 0xFDFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a    A    STOP   READ      ~     NEW") PORT_CODE(KEYCODE_A)		PORT_CHAR('a') PORT_CHAR('A')// PORT_CHAR('~')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s    S    NOT    RESTORE   |     SAVE") PORT_CODE(KEYCODE_S)		PORT_CHAR('s') PORT_CHAR('S')// PORT_CHAR('|')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d    D    STEP   DATA      \\    DIM") PORT_CODE(KEYCODE_D)		PORT_CHAR('d') PORT_CHAR('D')// PORT_CHAR('\\')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f    F    TO     SGN       {     FOR") PORT_CODE(KEYCODE_F)		PORT_CHAR('f') PORT_CHAR('F')// PORT_CHAR('{')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g    G    THEN   ABS       }     GOTO") PORT_CODE(KEYCODE_G)		PORT_CHAR('g') PORT_CHAR('G')// PORT_CHAR('}')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-    =") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[    {") PORT_CODE(KEYCODE_OPENBRACE)

	PORT_START("LINE2") /* 0xFBFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q    Q    <=     SIN      ASN      PLOT") PORT_CODE(KEYCODE_Q)	PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w    W    <>     COS      ACS      DRAW") PORT_CODE(KEYCODE_W)	PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e    E    >=     TAN      ATN      REM") PORT_CODE(KEYCODE_E)	PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r    R    <      INT      VERIFY   RUN") PORT_CODE(KEYCODE_R)	PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR('<')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t    T    >      RND      MERGE    RAND") PORT_CODE(KEYCODE_T)	PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR('>')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".    >") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",    <") PORT_CODE(KEYCODE_COMMA)

	PORT_START("LINE3") /* 0xF7FE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1   EDIT       !    BLUE     DEF FN") PORT_CODE(KEYCODE_1)	PORT_CHAR('1') PORT_CHAR('\xD7') PORT_CHAR('!')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2   CAPS LOCK  @    RED      FN") PORT_CODE(KEYCODE_2)		PORT_CHAR('2') PORT_CHAR('\xD7') PORT_CHAR('@')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3   TRUE VID   #    MAGENTA  LINE") PORT_CODE(KEYCODE_3)		PORT_CHAR('3') PORT_CHAR('\xD7') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4   INV VID    $    GREEN    OPEN#") PORT_CODE(KEYCODE_4)	PORT_CHAR('4') PORT_CHAR('\xD7') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5   Left       %    CYAN     CLOSE#") PORT_CODE(KEYCODE_5)	PORT_CHAR('5') PORT_CHAR(UCHAR_MAMEKEY(LEFT)) PORT_CHAR('%')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/   ?") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@   \\") PORT_CODE(KEYCODE_BACKSLASH)

	PORT_START("LINE4") /* 0xEFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0   DEL        _    BLACK    FORMAT") PORT_CODE(KEYCODE_0)	PORT_CHAR('0') PORT_CHAR(8) PORT_CHAR('_')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9   GRAPH      )             POINT") PORT_CODE(KEYCODE_9)	PORT_CHAR('9') PORT_CHAR('\xD7') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8   Right      (             CAT") PORT_CODE(KEYCODE_8)		PORT_CHAR('8') PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_CHAR('(')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7   Up         '    WHITE    ERASE") PORT_CODE(KEYCODE_7)	PORT_CHAR('7') PORT_CHAR(UCHAR_MAMEKEY(UP)) PORT_CHAR('\'')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6   Down       &    YELLOW   CP/J") PORT_CODE(KEYCODE_6)		PORT_CHAR('6') PORT_CHAR(UCHAR_MAMEKEY(DOWN)) PORT_CHAR('&')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]   }") PORT_CODE(KEYCODE_CLOSEBRACE)

	PORT_START("LINE5") /* 0xDFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("p    P    \"     TAB      (c)    PRINT") PORT_CODE(KEYCODE_P)	PORT_CHAR('p') PORT_CHAR('P') PORT_CHAR('"')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("o    O    ;      PEEK     OUT    POKE") PORT_CODE(KEYCODE_O)		PORT_CHAR('o') PORT_CHAR('O') PORT_CHAR(';')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("i    I    AT     CODE     IN     INPUT") PORT_CODE(KEYCODE_I)	PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u    U    OR     CHR$     ]      IF") PORT_CODE(KEYCODE_U)		PORT_CHAR('u') PORT_CHAR('U')// PORT_CHAR(']')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("y    Y    AND    STR$     [      RETURN") PORT_CODE(KEYCODE_Y)	PORT_CHAR('y') PORT_CHAR('Y')// PORT_CHAR('[')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BS") PORT_CODE(KEYCODE_BACKSPACE)

	PORT_START("LINE6") /* 0xBFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("l    L    =      USR      ATTR     LET") PORT_CODE(KEYCODE_L)	PORT_CHAR('l') PORT_CHAR('L') PORT_CHAR('=')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("k    K    +      LEN      SCREEN$  LIST") PORT_CODE(KEYCODE_K)	PORT_CHAR('k') PORT_CHAR('K') PORT_CHAR('+')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("j    J    -      VAL      VAL$     LOAD") PORT_CODE(KEYCODE_J)	PORT_CHAR('j') PORT_CHAR('J') PORT_CHAR('-')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h    H    ^      SQR      CIRCLE   GOSUB") PORT_CODE(KEYCODE_H)	PORT_CHAR('h') PORT_CHAR('H') PORT_CHAR('^')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT LOCK") PORT_CODE(KEYCODE_CAPSLOCK)

	PORT_START("LINE7") /* 0x7FFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SYMBOL SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m    M    .      PI       INVERSE  PAUSE") PORT_CODE(KEYCODE_M)	PORT_CHAR('m') PORT_CHAR('M') PORT_CHAR('.')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n    N    ,      INKEY$   OVER     NEXT") PORT_CODE(KEYCODE_N)	PORT_CHAR('n') PORT_CHAR('N') PORT_CHAR(',')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b    B    *      BIN      BRIGHT   BORDER") PORT_CODE(KEYCODE_B)	PORT_CHAR('b') PORT_CHAR('B') PORT_CHAR('*')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^    -") PORT_CODE(KEYCODE_TILDE)

	PORT_START("LINE8")
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNKNOWN)

	PORT_START("LINE9")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DIR") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor Down") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor Up") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor Right") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor Left") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN)

	PORT_START("JOY")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P1 Joystick Right") PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P1 Joystick Left") PORT_CODE(JOYCODE_X_LEFT_SWITCH)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P1 Joystick Down") PORT_CODE(JOYCODE_Y_DOWN_SWITCH)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P1 Joystick Up") PORT_CODE(JOYCODE_Y_UP_SWITCH)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P1 Joystick Fire") PORT_CODE(JOYCODE_BUTTON1)
INPUT_PORTS_END

/*************************************
 *
 *  Machine
 *
 *************************************/

static MACHINE_RESET(elwro800)
{
	memset(mess_ram, 0, 64*1024);

	memory_set_bankptr(machine, 3, mess_ram + 0x4000);

	// this is a reset of ls175 in mmu
	elwro800jr_mmu_w(machine, 0);
}

static const cassette_config elwro800jr_cassette_config =
{
	tzx_cassette_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_ENABLED
};

static INTERRUPT_GEN( elwro800jr_interrupt )
{
	cpu_set_input_line(device, 0, HOLD_LINE);
}

static MACHINE_DRIVER_START( elwro800 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, 3500000)	/* 3.5 MHz */
    MDRV_CPU_PROGRAM_MAP(elwro800_mem)
    MDRV_CPU_IO_MAP(elwro800_io)
	MDRV_CPU_VBLANK_INT("screen", elwro800jr_interrupt)

    MDRV_MACHINE_RESET(elwro800)

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50.08)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(SPEC_SCREEN_WIDTH, SPEC_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT( spectrum )

	MDRV_VIDEO_START( spectrum )
	MDRV_VIDEO_UPDATE( spectrum )
	MDRV_VIDEO_EOF( spectrum )

	MDRV_NEC765A_ADD("nec765", elwro800jr_nec765_interface)
    MDRV_I8255A_ADD( "ppi8255", elwro800jr_ppi8255_interface)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", elwro800jr_centronics_interface)

	MDRV_MSM8251_ADD("msm8251", default_msm8251_interface)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_CASSETTE_ADD( "cassette", elwro800jr_cassette_config )

MACHINE_DRIVER_END

static void elwro800jr_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		default:										legacydsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(elwro800)
	CONFIG_RAM_DEFAULT(64 * 1024)
	CONFIG_DEVICE(elwro800jr_floppy_getinfo)
SYSTEM_CONFIG_END

/*************************************
 *
 *  ROM definition
 *
 *************************************/

ROM_START( elwro800 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bas04.epr", 0x0000, 0x2000, CRC(6ab16f36) SHA1(49a19b279f311279c7fed3d2b3f207732d674c26) )
	ROM_LOAD( "bas14.epr", 0x2000, 0x2000, CRC(a743eb80) SHA1(3a300550838535b4adfe6d05c05fe0b39c47df16) )
	ROM_LOAD( "bootv.epr", 0x4000, 0x2000, CRC(de5fa37d) SHA1(4f203efe53524d84f69459c54b1a0296faa83fd9) )

	ROM_REGION(0x0400, "proms", 0 )
	ROM_LOAD( "junior_io_prom.bin", 0x0000, 0x0200,  CRC(c6a777c4) SHA1(41debc1b4c3bd4eef7e0e572327c759e0399a49c))
	ROM_LOAD( "junior_mem_prom.bin", 0x0200, 0x0200, CRC(0f745f42) SHA1(360ec23887fb6d7e19ee85d2bb30d9fa57f4936e))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1986, elwro800,  0,       0, 	elwro800, 	elwro800, 	 0,  	  elwro800,  	 "Elwro",   "800 Junior",		0)
