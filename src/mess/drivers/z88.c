/******************************************************************************

        z88.c

        z88 Notepad computer

        system driver

        TODO:
        - speaker controlled by constant tone or txd
        - Facility to load games
        - expansion interface

        Kevin Thacker [MESS driver]

*******************************************************************************/
#define ADDRESS_MAP_MODERN

#include "includes/z88.h"


/* Assumption:

all banks can access the same memory blocks in the same way.
bank 0 is special. If a bit is set in the com register,
the lower 8k is replaced with the rom. Bank 0 has been split
into 2 8k chunks, and all other banks into 16k chunks.
I wanted to handle all banks in the code below, and this
explains why the extra checks are done


    bank 0      0x0000-0x3FFF
    bank 1      0x4000-0x7FFF
    bank 2      0x8000-0xBFFF
    bank 3      0xC000-0xFFFF

    pages 0x00 - 0x1f   internal ROM
    pages 0x20 - 0x3f   512KB internal RAM
    pages 0x40 - 0x7f   Slot 1
    pages 0x80 - 0xbf   Slot 2
    pages 0xc0 - 0xff   Slot 3

*/

void z88_state::bankswitch_update(int bank, UINT16 page, int rams)
{
	char bank_tag[6];
	sprintf(bank_tag, "bank%d", bank + 2);

	// bank 0 is always even
	if (bank == 0)	page &= 0xfe;

	if (page < 0x40)	// internal memory
	{
		// install read bank
		m_maincpu->memory().space(AS_PROGRAM)->install_read_bank(bank<<14, (bank<<14) + 0x3fff, bank_tag);

		// install write bank
		if (page > 0x1f)
		{
			 // 512KB internal RAM
			m_maincpu->memory().space(AS_PROGRAM)->install_write_bank(bank<<14, (bank<<14) + 0x3fff, bank_tag);
		}
		else
		{
			// internal ROM
			m_maincpu->memory().space(AS_PROGRAM)->unmap_write(bank<<14, (bank<<14) + 0x3fff);
		}

		// set the bank
		memory_set_bank(machine(), bank_tag, page);
	}
	else
	{
		// TODO: expansion slots
		m_maincpu->memory().space(AS_PROGRAM)->unmap_readwrite(bank<<14, (bank<<14) + 0x3fff);
	}


	// override setting for lower 8k of bank 0
	if (bank == 0)
	{
		m_maincpu->memory().space(AS_PROGRAM)->install_read_bank(0, 0x1fff, "bank1");

		// enable RAM
		if (rams)
			m_maincpu->memory().space(AS_PROGRAM)->install_write_bank(0, 0x1fff, "bank1");
		else
			m_maincpu->memory().space(AS_PROGRAM)->unmap_write(0, 0x1fff);

		memory_set_bank(machine(), "bank1", rams & 1);
	}
}


static ADDRESS_MAP_START(z88_mem, AS_PROGRAM, 8, z88_state )
	AM_RANGE(0x0000, 0x1fff) AM_READWRITE_BANK("bank1")
	AM_RANGE(0x2000, 0x3fff) AM_READWRITE_BANK("bank2")
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE_BANK("bank3")
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE_BANK("bank4")
	AM_RANGE(0xc000, 0xffff) AM_READWRITE_BANK("bank5")
ADDRESS_MAP_END

static ADDRESS_MAP_START( z88_io, AS_IO, 8, z88_state )
	AM_RANGE(0x0000, 0xffff)	AM_DEVREADWRITE("blink", upd65031_device, read, write)
ADDRESS_MAP_END



/*
-------------------------------------------------------------------------
         | D7     D6      D5      D4      D3      D2      D1      D0
-------------------------------------------------------------------------
A15 (#7) | RSH    SQR     ESC     INDEX   CAPS    .       /       ??
A14 (#6) | HELP   LSH     TAB     DIA     MENU    ,       ;       '
A13 (#5) | [      SPACE   1       Q       A       Z       L       0
A12 (#4) | ]      LFT     2       W       S       X       M       P
A11 (#3) | -      RGT     3       E       D       C       K       9
A10 (#2) | =      DWN     4       R       F       V       J       O
A9  (#1) | \      UP      5       T       G       B       U       I
A8  (#0) | DEL    ENTER   6       Y       H       N       7       8
-------------------------------------------------------------------------

2008-05 FP:
Small note about natural keyboard: currently,
- "Square" is mapped to 'F1'
- "Diamond" is mapped to 'Left Control'
- "Index" is mapped to 'F2'
- "Menu" is mapped to 'F3'
- "Help" is mapped to 'F4'
*/

static INPUT_PORTS_START( z88 )
	PORT_START("LINE0")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)			PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)			PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)			PORT_CHAR('8') PORT_CHAR('*')

	PORT_START("LINE1")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)			PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)			PORT_CHAR('i') PORT_CHAR('I')

	PORT_START("LINE2")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)		PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)			PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("LINE3")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)		PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)			PORT_CHAR('9') PORT_CHAR('(')

	PORT_START("LINE4")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)		PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)			PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)			PORT_CHAR('p') PORT_CHAR('P')

	PORT_START("LINE5")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)			PORT_CHAR('0') PORT_CHAR(')')

	PORT_START("LINE6")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Help") PORT_CODE(KEYCODE_F4)				PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift (Left)") PORT_CODE(KEYCODE_LSHIFT)	PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)									PORT_CHAR('\t')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Diamond") PORT_CODE(KEYCODE_LCONTROL)		PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Menu") PORT_CODE(KEYCODE_F3)				PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)								PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)								PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)								PORT_CHAR('\'') PORT_CHAR('"')

	PORT_START("LINE7")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift (Right)") PORT_CODE(KEYCODE_RSHIFT)	PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Square") PORT_CODE(KEYCODE_F1)				PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC)								PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Index") PORT_CODE(KEYCODE_F2)				PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK)		PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)								PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)								PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)							PORT_CHAR('\xA3') PORT_CHAR('~')
INPUT_PORTS_END

void z88_state::machine_start()
{
	// configure the memory banks
	memory_configure_bank(machine(), "bank1", 0, 1, machine().region("maincpu")->base(), 0);
	memory_configure_bank(machine(), "bank1", 1, 1, machine().region("maincpu")->base() + (32<<14), 0);
	memory_configure_bank(machine(), "bank2", 0, 64, machine().region("maincpu")->base(), 0x4000);
	memory_configure_bank(machine(), "bank3", 0, 64, machine().region("maincpu")->base(), 0x4000);
	memory_configure_bank(machine(), "bank4", 0, 64, machine().region("maincpu")->base(), 0x4000);
	memory_configure_bank(machine(), "bank5", 0, 64, machine().region("maincpu")->base(), 0x4000);
}

READ8_MEMBER(z88_state::kb_r)
{
	UINT8 data = 0xff;

	if (!(offset & 0x80))
		data &= input_port_read(machine(), "LINE7");

	if (!(offset & 0x40))
		data &= input_port_read(machine(), "LINE6");

	if (!(offset & 0x20))
		data &= input_port_read(machine(), "LINE5");

	if (!(offset & 0x10))
		data &= input_port_read(machine(), "LINE4");

	if (!(offset & 0x08))
		data &= input_port_read(machine(), "LINE3");

	if (!(offset & 0x04))
		data &= input_port_read(machine(), "LINE2");

	if (!(offset & 0x02))
		data &= input_port_read(machine(), "LINE1");

	if (!(offset & 0x01))
		data &= input_port_read(machine(), "LINE0");

	return data;
}

static UPD65031_MEMORY_UPDATE(z88_bankswitch_update)
{
	z88_state *state = device.machine().driver_data<z88_state>();
	state->bankswitch_update(bank, page, rams);
}

static UPD65031_SCREEN_UPDATE(z88_screen_update)
{
	z88_state *state = device.machine().driver_data<z88_state>();
	state->lcd_update(bitmap, sbf, hires0, hires1, lores0, lores1, flash);
}

static UPD65031_INTERFACE( z88_blink_intf )
{
	z88_screen_update,										// callback for update the LCD
	z88_bankswitch_update,									// callback for update the bankswitch
	DEVCB_DRIVER_MEMBER(z88_state, kb_r),       			// kb read input
	DEVCB_CPU_INPUT_LINE("maincpu", 0),         			// INT line out
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_NMI),        // NMI line out
	DEVCB_NULL												// Speaker line out
};


static MACHINE_CONFIG_START( z88, z88_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_9_8304MHz/3)	// divided by 3 through the uPD65031
	MCFG_CPU_PROGRAM_MAP(z88_mem)
	MCFG_CPU_IO_MAP(z88_io)
	//MCFG_QUANTUM_TIME(attotime::from_hz(60))

	/* video hardware */
	MCFG_SCREEN_ADD("screen", LCD)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(Z88_SCREEN_WIDTH, Z88_SCREEN_HEIGHT)
	MCFG_SCREEN_VISIBLE_AREA(0, (Z88_SCREEN_WIDTH - 1), 0, (Z88_SCREEN_HEIGHT - 1))
	MCFG_SCREEN_UPDATE_DEVICE("blink", upd65031_device, screen_update)

	MCFG_PALETTE_LENGTH(Z88_NUM_COLOURS)
	MCFG_PALETTE_INIT( z88 )

	MCFG_DEFAULT_LAYOUT(layout_lcd)

	MCFG_UPD65031_ADD("blink", XTAL_9_8304MHz, z88_blink_intf)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(z88)
	ROM_REGION(0x100000, "maincpu", ROMREGION_ERASE00)
	ROM_DEFAULT_BIOS("v40uk")
	ROM_SYSTEM_BIOS( 0, "v220uk", "Version 2.20 UK")
	ROMX_LOAD("z88v220.rom", 0x00000, 0x20000, CRC(0ae7d0fc) SHA1(5d89e8d98d2cc0acb8cd42dbfca601b7bd09c51e), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v250sw", "Version 2.50 Swedish")
	ROMX_LOAD("z88v250sw.rom", 0x00000, 0x20000, CRC(dad01338) SHA1(3825eee346b692b16215a500250cc0c76d2d8f0b), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "v260nr", "Version 2.60 Norwegian")
	ROMX_LOAD("z88v260nr.rom", 0x00000, 0x20000, CRC(293f35c8) SHA1(b68b8f5bb1f69fe7a24897933b1464dc79e96d80), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "v30uk", "Version 3.0 UK")
	ROMX_LOAD("z88v300.rom" ,  0x00000, 0x20000, CRC(802cb9aa) SHA1(ceb688025b79454cf229cae4dbd0449df2747f79), ROM_BIOS(4) )
	ROM_SYSTEM_BIOS( 4, "v313he", "Version 3.13 Swiss")
	ROMX_LOAD("z88v313he.rom", 0x00000, 0x20000, CRC(a56d732c) SHA1(c2276a12d457f01a8fd2e2ac238aff2b5c3559d8), ROM_BIOS(5) )
	ROM_SYSTEM_BIOS( 5, "v317tk", "Version 3.17 Turkish")
	ROMX_LOAD("z88v317tk.rom", 0x00000, 0x20000, CRC(9468d677) SHA1(8d76e94f43846c736bf257d15d531c2df1e20fae), ROM_BIOS(6) )
	ROM_SYSTEM_BIOS( 6, "v318de", "Version 3.18 German")
	ROMX_LOAD("z88v318de.rom", 0x00000, 0x20000, CRC(d7eaf937) SHA1(5acbfa324e2a6582ffd1af5f2e28086318d2ed27), ROM_BIOS(7) )
	ROM_SYSTEM_BIOS( 7, "v319es", "Version 3.19 Spanish")
	ROMX_LOAD("z88v319es.rom", 0x00000, 0x20000, CRC(7a08af73) SHA1(a99a7581f47a032e1ec3b4f534c06f00f67647df), ROM_BIOS(8) )
	ROM_SYSTEM_BIOS( 8, "v321dk", "Version 3.21 Danish")
	ROMX_LOAD("z88v321dk.rom", 0x00000, 0x20000, CRC(baa80408) SHA1(7b0d44af2688d0fe47667e0424860aafa0948dae), ROM_BIOS(9) )
	ROM_SYSTEM_BIOS( 9, "v323it", "Version 3.23 Italian")
	ROMX_LOAD("z88v323it.rom", 0x00000, 0x20000, CRC(13f54308) SHA1(29bda04ae803f2dff6357d81b3894db669d12dbf), ROM_BIOS(10) )
	ROM_SYSTEM_BIOS( 10, "v326fr", "Version 3.26 French")
	ROMX_LOAD("z88v326fr.rom", 0x00000, 0x20000, CRC(218fbb72) SHA1(6e4c590f40f5b14d66e6559807f538fb5fa91474), ROM_BIOS(11) )
	ROM_SYSTEM_BIOS( 11, "v40uk", "Version 4.0 UK")
	ROMX_LOAD("z88v400.rom",   0x00000, 0x20000, CRC(1356d440) SHA1(23c63ceced72d0a9031cba08d2ebc72ca336921d), ROM_BIOS(12) )
	ROM_SYSTEM_BIOS( 12, "v401fi", "Version 4.01 Finnish")
	ROMX_LOAD("z88v401fi.rom", 0x00000, 0x20000, CRC(ecd7f3f6) SHA1(bf8d3e083f1959e5a0d7e9c8d2ad0c14abd46381), ROM_BIOS(13) )
ROM_END

/*    YEAR     NAME    PARENT  COMPAT  MACHINE     INPUT       INIT     COMPANY         FULLNAME */
COMP( 1988,    z88,    0,      0,      z88,        z88,        0, "Cambridge Computers", "Z88", GAME_NOT_WORKING)
