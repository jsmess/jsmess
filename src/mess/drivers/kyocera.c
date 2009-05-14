/******************************************************************************************

	Kyocera Kyotronics 85 (and similar laptop computers)
	
	2009/04 Very Preliminary Driver (video emulation courtesy of very old code by 
	        Hamish Coleman)

	Comments about bit usage from Tech References and Virtual T source.

	Supported systems:
	  - Kyocera Kyotronic 85
	  - Olivetti M10 (slightly diff hw, BIOS is shifted by 2 words)
	  - NEC PC-8201A (slightly diff hw)
	  - Tandy Model 100
	  - Tandy Model 102 (slightly diff hw)
	  - Tandy Model 200 (diff video & rtc)
	
	To Do:
	  - Find dumps of systems which could easily be added:
		* Olivetti M10 US (diff BIOS than the European version, it might be the alt BIOS)
		* NEC PC-8201 (original Japanese version of PC-8201A)

	  - Investigate other similar machines:
		* NEC PC-8300 (similar hardware?)
		* Tandy Model 600 (possibly different?)

******************************************************************************************/

/*

	TODO:

	- discrete sound
	- pc8201 memory banking
	- pc8201 I/O control registers
	- NEC PC-8233 floppy controller
	- NEC floppy disc drives (PC-8031-1W, PC-8031-2W, PC-80S31)
	- NEC PC-8240 video monitor adapter
	- Tandy Portable Disk Drive (TPDD: 100k 3½", TPDD2: 200k 3½")
	- Chipmunk disk drive (384k 3½")
	- soft power on/off
	- RS232/modem select
	- trsm200 RTC alarm
	- trsm200 TCM5089 sound
	- IM6042 UART

*/

#include "driver.h"
#include "includes/kyocera.h"
#include "cpu/i8085/i8085.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "machine/ctronics.h"
#include "machine/upd1990a.h"
#include "machine/8155pio.h"
#include "machine/rp5c01a.h"
#include "video/hd61830.h"

static const device_config *cassette_device_image(running_machine *machine)
{
	return devtag_get_device(machine, "cassette");
}

/* Read/Write Handlers */

static UINT8 sio;

static READ8_HANDLER( pc8201_bank_r )
{
	kyocera_state *state = space->machine->driver_data;

	return (sio & 0xc0) | state->bank;
}

static void pc8201_bankswitch(running_machine *machine, UINT8 data)
{
	kyocera_state *state = machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, I8085_TAG, ADDRESS_SPACE_PROGRAM);

	int rom_bank = data & 0x03;
	int ram_bank = (data >> 2) & 0x03;

	state->bank = data & 0x0f;

	if (rom_bank > 1)
	{
		/* RAM */
		memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
	}
	else
	{
		/* ROM */
		memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_UNMAP);
	}

	memory_set_bank(machine, 1, rom_bank);

	switch (ram_bank)
	{
	case 0:
		if (mess_ram_size > 16 * 1024)
		{
			memory_install_readwrite8_handler(program, 0x8000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		}
		else
		{
			memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
			memory_install_readwrite8_handler(program, 0xc000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		}
		break;

	case 1:
		memory_install_readwrite8_handler(program, 0x8000, 0xffff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case 2:
		if (mess_ram_size > 32 * 1024)
			memory_install_readwrite8_handler(program, 0x8000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		else
			memory_install_readwrite8_handler(program, 0x8000, 0xffff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case 3:
		if (mess_ram_size > 64 * 1024)
			memory_install_readwrite8_handler(program, 0x8000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		else
			memory_install_readwrite8_handler(program, 0x8000, 0xffff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;
	}

	memory_set_bank(machine, 2, ram_bank);
}

static WRITE8_HANDLER( pc8201_bank_w )
{
	pc8201_bankswitch(space->machine, data);
}

static WRITE8_HANDLER( pc8201_90_w )
{
	/*
		
		bit		signal		description

		0		
		1		
		2
		3
		4		STB			RTC strobe
		5		
		6
		7

	*/

	kyocera_state *state = space->machine->driver_data;
	
	sio = data;

	upd1990a_stb_w(state->upd1990a, BIT(data, 4));
}

static WRITE8_HANDLER( pc8201_e0_w )
{
	/*
		
		bit		signal		description

		0		
		1		
		2
		3
		4
		5		
		6
		7

	*/

//	kyocera_state *state = space->machine->driver_data;
}

static WRITE8_HANDLER( im6402_ctrl_w )
{
	/*

		bit		signal		description

		0		SBS			stop bit select
		1		EPE			even parity enable
		2		PI			parity inhibit
		3		CLS1		character length select bit 1
		4		CLS2		character length select bit 2
		5		
		6		
		7		

	*/
/*
	kyocera_state *state = space->machine->driver_data;

	im6402_sbs_w(state->im6402, BIT(data, 0));
	im6402_epe_w(state->im6402, BIT(data, 0));
	im6402_pi_w(state->im6402, BIT(data, 0));
	im6402_cls1_w(state->im6402, BIT(data, 0));
	im6402_cls2_w(state->im6402, BIT(data, 0));
*/
}

static READ8_HANDLER( im6402_status_r )
{
	/*

		bit		signal		description

		0		CD			carrier detect
		1		OE			overrun error
		2		FE			framing error
		3		PE			parity error
		4		TBRE		transmit buffer register empty
		5		RP			
		6		+5V			
		7		_LPS		low power sensor

	*/
/*
	kyocera_state *state = space->machine->driver_data;

	UINT8 data = 0;

	int cd = im6402_cd_r(state->im6402);
	int oe = im6402_oe_r(state->im6402);
	int fe = im6402_fe_r(state->im6402);
	int pe = im6402_pe_r(state->im6402);
	int tbre = im6402_tbre_r(state->im6402);

	data = (tbre << 4) | (pe << 3) | (fe << 2) | (oe << 1) | cd;

	return data;
*/

	return 0xf0;
}

static WRITE8_HANDLER( modem_w )
{
	/*

		bit		signal		description

		0					telephone line signal selection relay output		
		1		EN			MC14412 enable output
		2		
		3		
		4		
		5		
		6		
		7		

	*/
}

static WRITE8_HANDLER( e0_w )
{
	/*

		bit		signal		description

		0		_STROM		ROM selection (0=standard, 1=option)
		1		_STROBE		printer strobe output
		2		STB			RTC strobe output
		3		_REMOTE		cassette motor
		4		
		5		
		6		
		7		

	*/

	kyocera_state *state = space->machine->driver_data;

	/* ROM bank selection */
	memory_set_bank(space->machine, 1, BIT(data, 0));

	/* printer strobe */
	centronics_strobe_w(state->centronics, BIT(data, 1));

	/* RTC strobe */
	upd1990a_stb_w(state->upd1990a, BIT(data, 2));

	/* cassette motor */
	cassette_change_state(cassette_device_image(space->machine), BIT(data, 3) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
}

static UINT8 read_keyboard(running_machine *machine, UINT16 keylatch)
{
	UINT8 data = 0xff;

	if (!BIT(keylatch, 0)) data &= input_port_read(machine, "KEY0");
	if (!BIT(keylatch, 1)) data &= input_port_read(machine, "KEY1");
	if (!BIT(keylatch, 2)) data &= input_port_read(machine, "KEY2");
	if (!BIT(keylatch, 3)) data &= input_port_read(machine, "KEY3");
	if (!BIT(keylatch, 4)) data &= input_port_read(machine, "KEY4");
	if (!BIT(keylatch, 5)) data &= input_port_read(machine, "KEY5");
	if (!BIT(keylatch, 6)) data &= input_port_read(machine, "KEY6");
	if (!BIT(keylatch, 7)) data &= input_port_read(machine, "KEY7");
	if (!BIT(keylatch, 8)) data &= input_port_read(machine, "KEY8");

	return data;
}

static READ8_HANDLER( keyboard_r )
{
	kyocera_state *state = space->machine->driver_data;

	return read_keyboard(space->machine, state->keylatch);
}

static READ8_HANDLER( trsm200_bank_r )
{
	trsm200_state *state = space->machine->driver_data;

	return state->bank;
}

static WRITE8_HANDLER( trsm200_bank_w )
{
	trsm200_state *state = space->machine->driver_data;

	const address_space *program = cputag_get_address_space(space->machine, I8085_TAG, ADDRESS_SPACE_PROGRAM);

	int rom_bank = data & 0x03;
	int ram_bank = (data >> 2) & 0x03;

	state->bank = data & 0x0f;

	if (rom_bank == 3)
	{
		/* invalid ROM bank */
		memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
	}
	else
	{
		memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_UNMAP);
		memory_set_bank(space->machine, 1, rom_bank);
	}

	if (mess_ram_size < ((ram_bank + 1) * 24 * 1024))
	{
		/* invalid RAM bank */
		memory_install_readwrite8_handler(program, 0xa000, 0xffff, 0, 0, SMH_UNMAP, SMH_UNMAP);
	}
	else
	{
		memory_install_readwrite8_handler(program, 0xa000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		memory_set_bank(space->machine, 2, ram_bank);
	}
}

static READ8_HANDLER( trsm200_stbk_r )
{
	trsm200_state *state = space->machine->driver_data;

	return read_keyboard(space->machine, state->keylatch);
}

static WRITE8_HANDLER( trsm200_stbk_w )
{
	/*
		
		bit		signal	description

		0		_PSTB	printer strobe output
		1		REMOTE	cassette motor
		2
		3
		4
		5
		6
		7

	*/

	trsm200_state *state = space->machine->driver_data;

	/* printer strobe */
	centronics_strobe_w(state->centronics, BIT(data, 0));

	/* cassette motor */
	cassette_change_state(cassette_device_image(space->machine), BIT(data, 1) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
}

/* Memory Maps */

static ADDRESS_MAP_START( kyo85_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8201_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( trsm200_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8000, 0x9fff) AM_ROM
	AM_RANGE(0xa000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kyo85_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
//	AM_RANGE(0x70, 0x70) AM_MIRROR(0x0f) optional RAM unit
//	AM_RANGE(0x80, 0x80) AM_MIRROR(0x0f) optional I/O controller unit
//	AM_RANGE(0x90, 0x90) AM_MIRROR(0x0f) optional answering telephone unit
	AM_RANGE(0xa0, 0xa0) AM_MIRROR(0x0f) AM_WRITE(modem_w)
	AM_RANGE(0xb0, 0xb7) AM_MIRROR(0x08) AM_DEVREADWRITE(PIO8155_TAG, pio8155_r, pio8155_w)
//	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0x0f) AM_DEVREADWRITE(IM6402_TAG, im6402_data_r, im6402_data_w)
	AM_RANGE(0xd0, 0xd0) AM_MIRROR(0x0f) AM_READWRITE(im6402_status_r, im6402_ctrl_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x0f) AM_READWRITE(keyboard_r, e0_w)
	AM_RANGE(0xf0, 0xf0) AM_MIRROR(0x0e) AM_READWRITE(kyo85_lcd_status_r, kyo85_lcd_command_w)
	AM_RANGE(0xf1, 0xf1) AM_MIRROR(0x0e) AM_READWRITE(kyo85_lcd_data_r, kyo85_lcd_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8201_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
//	AM_RANGE(0x70, 0x70) AM_MIRROR(0x0f) optional RAM unit
//	AM_RANGE(0x80, 0x80) AM_MIRROR(0x0f) optional I/O controller unit
	AM_RANGE(0x90, 0x90) AM_WRITE(pc8201_90_w)
	AM_RANGE(0xa0, 0xa0) AM_MIRROR(0x0f) AM_READWRITE(pc8201_bank_r, pc8201_bank_w)
	AM_RANGE(0xb0, 0xb7) AM_MIRROR(0x08) AM_DEVREADWRITE( PIO8155_TAG, pio8155_r, pio8155_w )
//	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0x0f) AM_DEVREADWRITE(IM6402_TAG, im6402_data_r, im6402_data_w)
	AM_RANGE(0xd0, 0xd0) AM_MIRROR(0x0f) AM_READWRITE(im6402_status_r, im6402_ctrl_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x0f) AM_READWRITE(keyboard_r, pc8201_e0_w)
	AM_RANGE(0xf0, 0xf0) AM_MIRROR(0x0e) AM_READWRITE(kyo85_lcd_status_r, kyo85_lcd_command_w)
	AM_RANGE(0xf1, 0xf1) AM_MIRROR(0x0e) AM_READWRITE(kyo85_lcd_data_r, kyo85_lcd_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( trsm200_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x90, 0x9f) AM_DEVREADWRITE(RP5C01A_TAG, rp5c01a_r, rp5c01a_w)
//	AM_RANGE(0xa0, 0xa0) AM_MIRROR(0x0f) AM_DEVWRITE(TCM5089_TAG, tcm5089_w)
	AM_RANGE(0xb0, 0xb7) AM_MIRROR(0x08) AM_DEVREADWRITE(PIO8155_TAG, pio8155_r, pio8155_w)
//	AM_RANGE(0xc0, 0xc1) AM_MIRROR(0x0e) AM_DEVREADWRITE(UART8251_TAG, uart8251_r, uart8251_w)
	AM_RANGE(0xd0, 0xd0) AM_MIRROR(0x0f) AM_READWRITE(trsm200_bank_r, trsm200_bank_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x0f) AM_READWRITE(trsm200_stbk_r, trsm200_stbk_w)
	AM_RANGE(0xf0, 0xf0) AM_MIRROR(0x0e) AM_DEVREADWRITE(HD61830_TAG, hd61830_data_r, hd61830_data_w)
	AM_RANGE(0xf1, 0xf1) AM_MIRROR(0x0e) AM_DEVREADWRITE(HD61830_TAG, hd61830_status_r, hd61830_control_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( kyo85 )
	PORT_START("KEY0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')

	PORT_START("KEY1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')

	PORT_START("KEY2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')

	PORT_START("KEY3")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR(']')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("KEY4")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')

	PORT_START("KEY5")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')

	PORT_START("KEY6")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PRINT") PORT_CODE(KEYCODE_F11) PORT_CHAR(UCHAR_MAMEKEY(F11))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LABEL") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PASTE") PORT_CODE(KEYCODE_F9) PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92|") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL BKSP") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("KEY7")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))

	PORT_START("KEY8")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PAUSE BREAK") PORT_CODE(KEYCODE_F12) PORT_CHAR(UCHAR_MAMEKEY(F12))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("NUM") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CODE") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
INPUT_PORTS_END

static INPUT_PORTS_START( pc8201a )
	PORT_INCLUDE( kyo85 )

	PORT_MODIFY("KEY3")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@') PORT_CHAR('^')

	PORT_MODIFY("KEY4")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')

	PORT_MODIFY("KEY5")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RALT) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('_')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')

	PORT_MODIFY("KEY6")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92|") PORT_CODE(KEYCODE_TAB)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL BKSP") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
INPUT_PORTS_END

static INPUT_PORTS_START( olivm10 )
	PORT_START("KEY0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')

	PORT_START("KEY1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('_')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')

	PORT_START("KEY2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')

	PORT_START("KEY3")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')

	PORT_START("KEY4")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')

	PORT_START("KEY5")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')

	PORT_START("KEY6")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PASTE") PORT_CODE(KEYCODE_F9) PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92|") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL BS") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))

	PORT_START("KEY7")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F8") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F7") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F6") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))

	PORT_START("KEY8")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PAUSE BREAK") PORT_CODE(KEYCODE_F12) PORT_CHAR(UCHAR_MAMEKEY(F12))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("NUM") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PRINT") PORT_CODE(KEYCODE_F11) PORT_CHAR(UCHAR_MAMEKEY(F11))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LABEL") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10))
INPUT_PORTS_END

/* uPD1990A Interface */

static WRITE_LINE_DEVICE_HANDLER( kyocera_upd1990a_data_w )
{
	kyocera_state *driver_state = device->machine->driver_data;

	driver_state->upd1990a_data = state;
}

static UPD1990A_INTERFACE( kyocera_upd1990a_intf )
{
	DEVCB_LINE(kyocera_upd1990a_data_w),
	DEVCB_CPU_INPUT_LINE(I8085_TAG, I8085_RST75_LINE)
};

/* RP5C01A Interface */

static RP5C01A_INTERFACE( trsm200_rp5c01a_intf )
{
	DEVCB_NULL								/* alarm */
};

/* 8155 Interface */

static READ8_DEVICE_HANDLER( kyocera_8155_port_c_r )
{
	/*
	
		bit		description

		0		serial data input from clock chip
		1		_BUSY signal from printer
		2		BUSY signal from printer
		3		data from BCR
		4		_CTS from RS232
		5		_DSR from RS232

	*/

	kyocera_state *state = device->machine->driver_data;

	UINT8 data = 0;

	data |= state->upd1990a_data;
	data |= centronics_not_busy_r(state->centronics) << 1;
	data |= centronics_busy_r(state->centronics) << 2;

	return data;
}

static WRITE8_DEVICE_HANDLER( kyocera_8155_port_a_w )
{
	/*

		bit		description

		0		LCD chip select 0, key scan 0, RTC C0
		1		LCD chip select 1, key scan 1, RTC C1
		2		LCD chip select 2, key scan 2, RTC C2
		3		LCD chip select 3, key scan 3, RTC CLK
		4		LCD chip select 4, key scan 4, RTC DATA IN
		5		LCD chip select 5, key scan 5
		6		LCD chip select 6, key scan 6
		7		LCD chip select 7, key scan 7

	*/

	kyocera_state *state = device->machine->driver_data;

	/* keyboard */
	state->keylatch = (state->keylatch & 0x100) | data;

	/* LCD */
	state->lcd_cs2[0] = BIT(data, 0);
	state->lcd_cs2[1] = BIT(data, 1);
	state->lcd_cs2[2] = BIT(data, 2);
	state->lcd_cs2[3] = BIT(data, 3);
	state->lcd_cs2[4] = BIT(data, 4);
	state->lcd_cs2[5] = BIT(data, 5);
	state->lcd_cs2[6] = BIT(data, 6);
	state->lcd_cs2[7] = BIT(data, 7);

	/* RTC */
	upd1990a_c0_w(state->upd1990a, BIT(data, 0));
	upd1990a_c1_w(state->upd1990a, BIT(data, 1));
	upd1990a_c2_w(state->upd1990a, BIT(data, 2));
	upd1990a_clk_w(state->upd1990a, BIT(data, 3));
	upd1990a_data_w(state->upd1990a, BIT(data, 4));
}

static WRITE8_DEVICE_HANDLER( kyocera_8155_port_b_w )
{
	/*

		bit		signal		description

		0					LCD chip select 8, key scan 8
		1					LCD chip select 9
		2					beeper data input select (0=data from 8155 TO, 1=data from PB5)
		3		_RS232		modem select (0=RS232, 1=modem)
		4		PCS			soft power off
		5					beeper data output
		6		_DTR		RS232 data terminal ready output
		7		_RTS		RS232 request to send output

	*/

	kyocera_state *state = device->machine->driver_data;

	/* keyboard */
	state->keylatch = (BIT(data, 0) << 8) | (state->keylatch & 0xff);

	/* LCD */
	state->lcd_cs2[8] = BIT(data, 0);
	state->lcd_cs2[9] = BIT(data, 1);
}

static PIO8155_INTERFACE( kyocera_8155_intf )
{
	DEVCB_NULL,								/* port A read */
	DEVCB_NULL,								/* port B read */
	DEVCB_HANDLER(kyocera_8155_port_c_r),	/* port C read */
	DEVCB_HANDLER(kyocera_8155_port_a_w),	/* port A write */
	DEVCB_HANDLER(kyocera_8155_port_b_w),	/* port B write */
	DEVCB_NULL,								/* port C write */
	DEVCB_NULL								/* timer output */
};

static READ8_DEVICE_HANDLER( trsm200_8155_port_c_r )
{
	/*
	
		bit		signal	description

		0		_LPS	low power sense input
		1		_BUSY	not busy input
		2		BUSY	busy input
		3		BCR		bar code reader data input
		4		CD		carrier detect input
		5		CDBD	carrier detect break down input

	*/

	trsm200_state *state = device->machine->driver_data;

	UINT8 data = 0x01;

	data |= centronics_not_busy_r(state->centronics) << 1;
	data |= centronics_busy_r(state->centronics) << 2;

	return data;
}

static WRITE8_DEVICE_HANDLER( trsm200_8155_port_a_w )
{
	/*

		bit		description

		0		print data 0, key scan 0
		1		print data 1, key scan 1
		2		print data 2, key scan 2
		3		print data 3, key scan 3
		4		print data 4, key scan 4
		5		print data 5, key scan 5
		6		print data 6, key scan 6
		7		print data 7, key scan 7

	*/

	trsm200_state *state = device->machine->driver_data;

	centronics_data_w(state->centronics, 0, data);

	state->keylatch = (state->keylatch & 0x100) | data;
}

static WRITE8_DEVICE_HANDLER( trsm200_8155_port_b_w )
{
	/*

		bit		signal		description

		0					key scan 8
		1		ORIG/ANS	(1=ORIG, 0=ANS)
		2		_BUZZER		
		3		_RS232C		(1=modem, 0=RS-232)
		4		PCS			power cut signal
		5		BELL		
		6		MEN			modem enable output
		7		CALL		connects and disconnects the phone line

	*/

	trsm200_state *state = device->machine->driver_data;

	state->keylatch = (BIT(data, 0) << 8) | (state->keylatch & 0xff);
}

static PIO8155_INTERFACE( trsm200_8155_intf )
{
	DEVCB_NULL,								/* port A read */
	DEVCB_NULL,								/* port B read */
	DEVCB_HANDLER(trsm200_8155_port_c_r),	/* port C read */
	DEVCB_HANDLER(trsm200_8155_port_a_w),	/* port A write */
	DEVCB_HANDLER(trsm200_8155_port_b_w),	/* port B write */
	DEVCB_NULL,								/* port C write */
	DEVCB_NULL								/* timer output */
};

/* Machine Drivers */

static MACHINE_START( kyo85 )
{
	kyocera_state *state = machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, I8085_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);
	state->centronics = devtag_get_device(machine, "centronics");

	/* initialize RTC */
	upd1990a_cs_w(state->upd1990a, 1);
	upd1990a_oe_w(state->upd1990a, 1);

	/* configure ROM banking */
	memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_UNMAP);
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, I8085_TAG), 0);
	memory_configure_bank(machine, 1, 1, 1, memory_region(machine, "option"), 0);
	memory_set_bank(machine, 1, 0);

	/* configure RAM banking */
	switch (mess_ram_size)
	{
	case 16 * 1024:
		memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		memory_install_readwrite8_handler(program, 0xc000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		break;

	case 32 * 1024:
		memory_install_readwrite8_handler(program, 0x8000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		break;
	}

	memory_configure_bank(machine, 2, 0, 1, mess_ram, 0);
	memory_set_bank(machine, 2, 0);

	/* register for state saving */
}

static MACHINE_START( pc8201 )
{
	kyocera_state *state = machine->driver_data;

	/* find devices */
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);
	state->centronics = devtag_get_device(machine, "centronics");

	/* initialize RTC */
	upd1990a_cs_w(state->upd1990a, 1);
	upd1990a_oe_w(state->upd1990a, 1);

	/* configure ROM banking */
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, I8085_TAG), 0);
	memory_configure_bank(machine, 1, 1, 1, memory_region(machine, "ext"), 0);
	memory_configure_bank(machine, 1, 2, 2, mess_ram + 0x8000, 0x8000);
	memory_set_bank(machine, 1, 0);

	/* configure RAM banking */
	memory_configure_bank(machine, 2, 0, 1, mess_ram, 0);
	memory_configure_bank(machine, 2, 2, 2, mess_ram + 0x8000, 0x8000);
	memory_set_bank(machine, 2, 0);

	pc8201_bankswitch(machine, 0);

	/* register for state saving */
}

static MACHINE_START( trsm100 )
{
	kyocera_state *state = machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, I8085_TAG, ADDRESS_SPACE_PROGRAM);
	
	/* find devices */
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);
	state->centronics = devtag_get_device(machine, "centronics");

	/* initialize RTC */
	upd1990a_cs_w(state->upd1990a, 1);
	upd1990a_oe_w(state->upd1990a, 1);

	/* configure ROM banking */
	memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_UNMAP);
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, I8085_TAG), 0);
	memory_configure_bank(machine, 1, 1, 1, memory_region(machine, "option"), 0);
	memory_set_bank(machine, 1, 0);

	/* configure RAM banking */
	switch (mess_ram_size)
	{
	case 8 * 1024:
		memory_install_readwrite8_handler(program, 0x8000, 0xcfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		memory_install_readwrite8_handler(program, 0xe000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		break;

	case 16 * 1024:
		memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		memory_install_readwrite8_handler(program, 0xc000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		break;

	case 24 * 1024:
		memory_install_readwrite8_handler(program, 0x8000, 0x9fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		memory_install_readwrite8_handler(program, 0xa000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		break;

	case 32 * 1024:
		memory_install_readwrite8_handler(program, 0x8000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		break;
	}

	memory_configure_bank(machine, 2, 0, 1, mess_ram, 0);
	memory_set_bank(machine, 2, 0);

	/* register for state saving */
}

static MACHINE_START( trsm200 )
{
	trsm200_state *state = machine->driver_data;

	/* find devices */
	state->centronics = devtag_get_device(machine, "centronics");

	/* configure ROM banking */
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, I8085_TAG), 0);
	memory_configure_bank(machine, 1, 1, 1, memory_region(machine, I8085_TAG) + 0x10000, 0);
	memory_configure_bank(machine, 1, 2, 1, memory_region(machine, "option"), 0);
	memory_set_bank(machine, 1, 0);

	/* configure RAM banking */
	memory_configure_bank(machine, 2, 0, 3, mess_ram, 0x6000);
	memory_set_bank(machine, 2, 0);

	/* register for state saving */
}

static const cassette_config kyocera_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED
};

static void kyocera_sod_w(const device_config *device, int state)
{
	cassette_output(cassette_device_image(device->machine), state ? +1.0 : -1.0);
}

static int kyocera_sid_r(const device_config *device)
{
	return cassette_input(cassette_device_image(device->machine)) > 0.0;
}

static const i8085_config kyocera_i8085_config =
{
	NULL,					/* INTE changed callback */
	NULL,					/* STATUS changed callback */
	kyocera_sod_w,			/* SOD changed callback (8085A only) */
	kyocera_sid_r			/* SID changed callback (8085A only) */
};

static TIMER_DEVICE_CALLBACK( trsm200_tp_tick )
{
	trsm200_state *state = timer->machine->driver_data;

	cputag_set_input_line(timer->machine, I8085_TAG, I8085_RST75_LINE, state->tp);

	state->tp = !state->tp;
}

static MACHINE_DRIVER_START( kyo85 )
	MDRV_DRIVER_DATA(kyocera_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(I8085_TAG, 8085A, 2400000)
	MDRV_CPU_PROGRAM_MAP(kyo85_mem, 0)
	MDRV_CPU_IO_MAP(kyo85_io, 0)
	MDRV_CPU_CONFIG(kyocera_i8085_config)

	MDRV_MACHINE_START( kyo85 )

	/* video hardware */
	MDRV_IMPORT_FROM(kyo85_video)

	/* sound hardware */
//	MDRV_SPEAKER_STANDARD_MONO("mono")
//	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
//	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_PIO8155_ADD(PIO8155_TAG, 2400000, kyocera_8155_intf)
	MDRV_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, kyocera_upd1990a_intf)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)

	/* cassette */
	MDRV_CASSETTE_ADD("cassette", kyocera_cassette_config)

	/* option ROM cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc8201 )
	MDRV_DRIVER_DATA(kyocera_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(I8085_TAG, 8085A, 2400000)
	MDRV_CPU_PROGRAM_MAP(pc8201_mem, 0)
	MDRV_CPU_IO_MAP(pc8201_io, 0)
	MDRV_CPU_CONFIG(kyocera_i8085_config)

	MDRV_MACHINE_START(pc8201)

	/* video hardware */
	MDRV_IMPORT_FROM(kyo85_video)

	/* sound hardware */
//	MDRV_SPEAKER_STANDARD_MONO("mono")
//	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
//	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_PIO8155_ADD(PIO8155_TAG, 2400000, kyocera_8155_intf)
	MDRV_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, kyocera_upd1990a_intf)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)

	/* cassette */
	MDRV_CASSETTE_ADD("cassette", kyocera_cassette_config)

	/* option ROM cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( trsm100 )
	MDRV_IMPORT_FROM(kyo85)

	MDRV_MACHINE_START(trsm100)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( trsm200 )
	MDRV_DRIVER_DATA(trsm200_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(I8085_TAG, 8085A, XTAL_2_4576MHz)
	MDRV_CPU_PROGRAM_MAP(trsm200_mem, 0)
	MDRV_CPU_IO_MAP(trsm200_io, 0)
	MDRV_CPU_CONFIG(kyocera_i8085_config)

	MDRV_MACHINE_START( trsm200 )

	/* video hardware */
	MDRV_IMPORT_FROM(trsm200_video)

	/* TP timer */
	MDRV_TIMER_ADD_PERIODIC("tp", trsm200_tp_tick, HZ((XTAL_2_4576MHz/8192)*2))

	/* sound hardware */
//	MDRV_TCM5089_ADD(TCM5089_TAG, XTAL_3_579545MHz)

//	MDRV_SPEAKER_STANDARD_MONO("mono")
//	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
//	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_PIO8155_ADD(PIO8155_TAG, XTAL_2_4576MHz, trsm200_8155_intf)
	MDRV_RP5C01A_ADD(RP5C01A_TAG, XTAL_32_768kHz, trsm200_rp5c01a_intf)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)

	/* cassette */
	MDRV_CASSETTE_ADD("cassette", kyocera_cassette_config)

	/* option ROM cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

/* ROMs */

ROM_START( kyo85 )
	ROM_REGION( 0x8000, I8085_TAG, 0 )
	ROM_LOAD( "kc85rom.bin", 0x0000, 0x8000, CRC(8a9ddd6b) SHA1(9d18cb525580c9e071e23bc3c472380aa46356c0) )

	ROM_REGION( 0x8000, "option", ROMREGION_ERASEFF )
	ROM_CART_LOAD("cart", 0x0000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( pc8201 )
	ROM_REGION( 0x10000, I8085_TAG, 0 )
	ROM_LOAD( "pc8201rom.bin", 0x0000, 0x8000, BAD_DUMP CRC(4c534662) SHA1(758fefbba251513e7f9d86f7e9016ad8817188d8) ) /* Y2K hacked */

	ROM_REGION( 0x8000, "ext", ROMREGION_ERASEFF )
	ROM_CART_LOAD("cart", 0x0000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( trsm100 )
	ROM_REGION( 0x8000, I8085_TAG, 0 )
	ROM_LOAD( "m100rom.bin",  0x0000, 0x8000, CRC(730a3611) SHA1(094dbc4ac5a4ea5cdf51a1ac581a40a9622bb25d) )

	ROM_REGION( 0x8000, "option", ROMREGION_ERASEFF )
	ROM_CART_LOAD("cart", 0x0000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( olivm10 )
	ROM_REGION( 0x8010, I8085_TAG, 0 )
	ROM_LOAD( "m10rom.bin", 0x0000, 0x8010, CRC(0be02b58) SHA1(56f2087a658efd0323663d15afcd4f5f27c68664) )

	ROM_REGION( 0x8000, "option", ROMREGION_ERASEFF )
	ROM_CART_LOAD("cart", 0x0000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( trsm102 )
	ROM_REGION( 0x8000, I8085_TAG, 0 )
	ROM_LOAD( "m102rom.bin", 0x0000, 0x8000, BAD_DUMP CRC(0e4ff73a) SHA1(d91f4f412fb78c131ccd710e8158642de47355e2) ) /* Y2K hacked */

	ROM_REGION( 0x8000, "option", ROMREGION_ERASEFF )
	ROM_CART_LOAD("cart", 0x0000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( trsm200 )
	ROM_REGION( 0x18000, I8085_TAG, 0 )
	ROM_LOAD( "t200rom.bin", 0x0000, 0xa000, BAD_DUMP CRC(e3358b38) SHA1(35d4e6a5fb8fc584419f57ec12b423f6021c0991) ) /* Y2K hacked */
	ROM_CONTINUE(			0x10000, 0x8000 )

	ROM_REGION( 0x8000, "option", ROMREGION_ERASEFF )
	ROM_CART_LOAD("cart", 0x0000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

/* System Configurations */

static SYSTEM_CONFIG_START( kyo85 )
	CONFIG_RAM_DEFAULT	(16 * 1024)
	CONFIG_RAM			(32 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( pc8201 )
	CONFIG_RAM_DEFAULT	(16 * 1024)
	CONFIG_RAM			(32 * 1024)
	CONFIG_RAM			(64 * 1024)
	CONFIG_RAM			(96 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( trsm100 )
	CONFIG_RAM_DEFAULT	( 8 * 1024)
	CONFIG_RAM			(16 * 1024)
	CONFIG_RAM			(24 * 1024)
	CONFIG_RAM			(32 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( trsm102 )
	CONFIG_RAM_DEFAULT	(24 * 1024)
	CONFIG_RAM			(32 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( trsm200 )
	CONFIG_RAM_DEFAULT	(24 * 1024)
	CONFIG_RAM			(48 * 1024)
	CONFIG_RAM			(72 * 1024)
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR  NAME      PARENT   COMPAT  MACHINE     INPUT    INIT      CONFIG       COMPANY  FULLNAME */
COMP( 1983, kyo85,    0,       0,      kyo85,      kyo85,   0,        kyo85,      "Kyocera",            "Kyotronic 85", GAME_IMPERFECT_SOUND )
COMP( 1983, olivm10,  0,       0,      kyo85,      olivm10, 0,        kyo85,      "Olivetti",           "M10",       GAME_IMPERFECT_SOUND )
COMP( 1983, pc8201,   0,       0,      pc8201,     pc8201a, 0,		  pc8201,     "NEC",                "PC-8201A", GAME_NOT_WORKING )
COMP( 1983, trsm100,  0,       0,      trsm100,    kyo85,   0,        trsm100,    "Tandy Radio Shack",  "TRS-80 Model 100", GAME_IMPERFECT_SOUND )
COMP( 1984, trsm200,  0,       0,      trsm200,    kyo85,   0,		  trsm200,    "Tandy Radio Shack",  "TRS-80 Model 200", GAME_IMPERFECT_SOUND )
COMP( 1986, trsm102,  0,       0,      trsm100,    kyo85,   0,        trsm102,    "Tandy Radio Shack",  "TRS-80 Model 102", GAME_IMPERFECT_SOUND )
