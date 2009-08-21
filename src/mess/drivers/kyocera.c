/******************************************************************************************

	Kyocera Kyotronics 85 (and similar laptop computers)
	
	2009/04 Very Preliminary Driver (video emulation courtesy of very old code by 
	        Hamish Coleman)

	Comments about bit usage from Tech References and Virtual T source.

	Supported systems:
	  - Kyosei Kyotronic 85
	  - Olivetti M10 (slightly diff hw, BIOS is shifted by 2 words)
	  - NEC PC-8201A (slightly diff hw)
	  - TRS-80 Model 100
	  - Tandy Model 102 (slightly diff hw)
	  - Tandy Model 200 (diff video & rtc)
	
	To Do:
	  - Find dumps of systems which could easily be added:
		* Olivetti M10 Modem (US) (diff BIOS than the European version)
		* NEC PC-8201 (original Japanese version of PC-8201A)
		* NEC PC-8300 (similar hardware to PC-8201)
		* NEC PC-8300 w/BradyWriter II ROMs

******************************************************************************************/

/*

	TODO:

	-	--- memory leak warning ---
		allocation #004834, 256 bytes (src/lib/util/astring.c:127)
		a total of 256 bytes were not free()'d
	- un-Y2K-hack tandy200
	- keyboard is unresponsive for couple of seconds after boot
	- soft power on/off
	- IM6042 UART
	- pc8201 48K RAM option
	- pc8201 NEC PC-8241A video interface (TMS9918, 16K videoRAM, 8K ROM)
	- pc8201 128K ROM cartridge
	- pc8201 NEC PC-8233 floppy controller
	- pc8201 NEC floppy disc drives (PC-8031-1W, PC-8031-2W, PC-80S31)
	- trsm100 Tandy Portable Disk Drive (TPDD: 100k 3½", TPDD2: 200k 3½") (undumped HD63A01V1 MCU + full custom uPD65002, serial comms via the missing IM6042, not going to happen anytime soon)
	- trsm100 Chipmunk disk drive (384k 3½") (full custom logic, not going to happen)
	- trsm100 RS232/modem select
	- tandy200 UART8251
	- tandy200 RTC alarm
	- tandy200 TCM5089 sound
	- international keyboard option ROMs

*/

/*

						  * PC-8201/8300 HARDWARE PORT DEFINITIONS *

				-Port-
	Name       Hex  Dec   Notes
	--------   ---  ---   -----------------------------------------
	A8255      070  112   Video interface port A (8255)
	B8255      071  113   Video interface port B (8255)
	C8255      072  114   Video interface port C (8255)
	CW8255     073  115   Video interface command/mode port (8255)
	ROMAH      080  128   128k ROM select and MSBIT of address
	ROMAL      084  132   Lowest 8 bits of extended ROM address
	ROMAM      088  136   Middle eight bits of 128k ROM address
	ROMRD      08C  140   Read data port for 128k ROM

*/

#include "driver.h"
#include "includes/kyocera.h"
#include "cpu/i8085/i8085.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "machine/ctronics.h"
#include "machine/upd1990a.h"
#include "machine/i8155.h"
#include "machine/rp5c01a.h"
#include "machine/msm8251.h"
#include "video/hd61830.h"
#include "sound/speaker.h"

static const device_config *cassette_device_image(running_machine *machine)
{
	return devtag_get_device(machine, CASSETTE_TAG);
}

/* Read/Write Handlers */

static READ8_HANDLER( pc8201_bank_r )
{
	/*
		
		bit		signal		description

		0		LADR1		select address 0 to 7fff
		1		LADR2		select address 0 to 7fff
		2		HADR1		select address 8000 to ffff
		3		HADR2		select address 8000 to ffff
		4		
		5		
		6		SELB		serial interface status bit 1
		7		SELA		serial interface status bit 0

	*/

	kc85_state *state = space->machine->driver_data;

	return (state->iosel << 5) | state->bank;
}

static void pc8201_bankswitch(running_machine *machine, UINT8 data)
{
	kc85_state *state = machine->driver_data;

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
	/*
		
		bit		signal		description

		0		LADR1		select address 0 to 7fff
		1		LADR2		select address 0 to 7fff
		2		HADR1		select address 8000 to ffff
		3		HADR2		select address 8000 to ffff
		4		
		5		
		6		
		7		

	*/

	pc8201_bankswitch(space->machine, data & 0x0f);
}

static WRITE8_HANDLER( pc8201_ctrl_w )
{
	/*
		
		bit		signal		description

		0		
		1		
		2
		3		REMOTE		cassette motor
		4		TSTB		RTC strobe
		5		PSTB		printer strobe
		6		SELB		serial interface select bit 1
		7		SELA		serial interface select bit 0

	*/

	kc85_state *state = space->machine->driver_data;
	
	/* cassette motor */
	cassette_change_state(state->cassette, BIT(data, 3) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);

	/* RTC strobe */
	upd1990a_stb_w(state->upd1990a, BIT(data, 4));

	/* printer strobe */
	centronics_strobe_w(state->centronics, BIT(data, 5));

	/* serial interface select */
	state->iosel = data >> 5;
}

static WRITE8_HANDLER( uart_ctrl_w )
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
	kc85_state *state = space->machine->driver_data;

	im6402_sbs_w(state->im6402, BIT(data, 0));
	im6402_epe_w(state->im6402, BIT(data, 1));
	im6402_pi_w(state->im6402, BIT(data, 2));
	im6402_cls1_w(state->im6402, BIT(data, 3));
	im6402_cls2_w(state->im6402, BIT(data, 4));
*/
}

static READ8_HANDLER( uart_status_r )
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
	kc85_state *state = space->machine->driver_data;

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

static READ8_HANDLER( pc8201_uart_status_r )
{
	/*

		bit		signal		description

		0		_DCD/_RD	data carrier detect / ring detect
		1		OE			overrun error
		2		FE			framing error
		3		PE			parity error
		4		TBRE		transmit buffer register empty
		5		RP			
		6		+5V			
		7		_LPS		low power signal

	*/
/*
	kc85_state *state = space->machine->driver_data;

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
/*
	kc85_state *state = space->machine->driver_data;

	mc14412_en_w(state->mc14412, BIT(data, 1));
*/
}

static WRITE8_HANDLER( kc85_ctrl_w )
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

	kc85_state *state = space->machine->driver_data;

	/* ROM bank selection */
	memory_set_bank(space->machine, 1, BIT(data, 0));

	/* printer strobe */
	centronics_strobe_w(state->centronics, BIT(data, 1));

	/* RTC strobe */
	upd1990a_stb_w(state->upd1990a, BIT(data, 2));

	/* cassette motor */
	cassette_change_state(state->cassette, BIT(data, 3) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
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
	kc85_state *state = space->machine->driver_data;

	return read_keyboard(space->machine, state->keylatch);
}

static READ8_HANDLER( tandy200_bank_r )
{
	tandy200_state *state = space->machine->driver_data;

	return state->bank;
}

static WRITE8_HANDLER( tandy200_bank_w )
{
	tandy200_state *state = space->machine->driver_data;

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

static READ8_HANDLER( tandy200_stbk_r )
{
	tandy200_state *state = space->machine->driver_data;

	return read_keyboard(space->machine, state->keylatch);
}

static WRITE8_HANDLER( tandy200_stbk_w )
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

	tandy200_state *state = space->machine->driver_data;

	/* printer strobe */
	centronics_strobe_w(state->centronics, BIT(data, 0));

	/* cassette motor */
	cassette_change_state(state->cassette, BIT(data, 1) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
}

/* Memory Maps */

static ADDRESS_MAP_START( kc85_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8201_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tandy200_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8000, 0x9fff) AM_ROM
	AM_RANGE(0xa000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kc85_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
//	AM_RANGE(0x70, 0x70) AM_MIRROR(0x0f) optional RAM unit
//	AM_RANGE(0x80, 0x80) AM_MIRROR(0x0f) optional I/O controller unit
//	AM_RANGE(0x90, 0x90) AM_MIRROR(0x0f) optional answering telephone unit
//	AM_RANGE(0xa0, 0xa0) AM_MIRROR(0x0f) optional modem
	AM_RANGE(0xb0, 0xb7) AM_MIRROR(0x08) AM_DEVREADWRITE(I8155_TAG, i8155_r, i8155_w)
//	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0x0f) AM_DEVREADWRITE(IM6402_TAG, im6402_data_r, im6402_data_w)
	AM_RANGE(0xd0, 0xd0) AM_MIRROR(0x0f) AM_READWRITE(uart_status_r, uart_ctrl_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x0f) AM_READWRITE(keyboard_r, kc85_ctrl_w)
	AM_RANGE(0xf0, 0xf0) AM_MIRROR(0x0e) AM_READWRITE(kc85_lcd_status_r, kc85_lcd_command_w)
	AM_RANGE(0xf1, 0xf1) AM_MIRROR(0x0e) AM_READWRITE(kc85_lcd_data_r, kc85_lcd_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( trsm100_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
//	AM_RANGE(0x70, 0x70) AM_MIRROR(0x0f) optional RAM unit
//	AM_RANGE(0x80, 0x80) AM_MIRROR(0x0f) optional I/O controller unit
//	AM_RANGE(0x90, 0x90) AM_MIRROR(0x0f) optional answering telephone unit
	AM_RANGE(0xa0, 0xa0) AM_MIRROR(0x0f) AM_WRITE(modem_w)
	AM_RANGE(0xb0, 0xb7) AM_MIRROR(0x08) AM_DEVREADWRITE(I8155_TAG, i8155_r, i8155_w)
//	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0x0f) AM_DEVREADWRITE(IM6402_TAG, im6402_data_r, im6402_data_w)
	AM_RANGE(0xd0, 0xd0) AM_MIRROR(0x0f) AM_READWRITE(uart_status_r, uart_ctrl_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x0f) AM_READWRITE(keyboard_r, kc85_ctrl_w)
	AM_RANGE(0xf0, 0xf0) AM_MIRROR(0x0e) AM_READWRITE(kc85_lcd_status_r, kc85_lcd_command_w)
	AM_RANGE(0xf1, 0xf1) AM_MIRROR(0x0e) AM_READWRITE(kc85_lcd_data_r, kc85_lcd_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8201_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
//	AM_RANGE(0x70, 0x70) AM_MIRROR(0x0f) optional video interface 8255
//	AM_RANGE(0x80, 0x80) AM_MIRROR(0x0f) optional 128K ROM cartridge
	AM_RANGE(0x90, 0x90) AM_MIRROR(0x0f) AM_WRITE(pc8201_ctrl_w)
	AM_RANGE(0xa0, 0xa0) AM_MIRROR(0x0f) AM_READWRITE(pc8201_bank_r, pc8201_bank_w)
	AM_RANGE(0xb0, 0xb7) AM_MIRROR(0x08) AM_DEVREADWRITE(I8155_TAG, i8155_r, i8155_w )
//	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0x0f) AM_DEVREADWRITE(IM6402_TAG, im6402_data_r, im6402_data_w)
	AM_RANGE(0xd0, 0xd0) AM_MIRROR(0x0f) AM_READWRITE(pc8201_uart_status_r, uart_ctrl_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x0f) AM_READ(keyboard_r)
	AM_RANGE(0xf0, 0xf0) AM_MIRROR(0x0e) AM_READWRITE(kc85_lcd_status_r, kc85_lcd_command_w)
	AM_RANGE(0xf1, 0xf1) AM_MIRROR(0x0e) AM_READWRITE(kc85_lcd_data_r, kc85_lcd_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tandy200_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x90, 0x9f) AM_DEVREADWRITE(RP5C01A_TAG, rp5c01a_r, rp5c01a_w)
//	AM_RANGE(0xa0, 0xa0) AM_MIRROR(0x0f) AM_DEVWRITE(TCM5089_TAG, tcm5089_w)
	AM_RANGE(0xb0, 0xb7) AM_MIRROR(0x08) AM_DEVREADWRITE(I8155_TAG, i8155_r, i8155_w)
	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0x0e) AM_DEVREADWRITE(MSM8251_TAG, msm8251_data_r, msm8251_data_w)
	AM_RANGE(0xc1, 0xc1) AM_MIRROR(0x0e) AM_DEVREADWRITE(MSM8251_TAG, msm8251_status_r, msm8251_control_w)
	AM_RANGE(0xd0, 0xd0) AM_MIRROR(0x0f) AM_READWRITE(tandy200_bank_r, tandy200_bank_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x0f) AM_READWRITE(tandy200_stbk_r, tandy200_stbk_w)
	AM_RANGE(0xf0, 0xf0) AM_MIRROR(0x0e) AM_DEVREADWRITE(HD61830_TAG, hd61830_data_r, hd61830_data_w)
	AM_RANGE(0xf1, 0xf1) AM_MIRROR(0x0e) AM_DEVREADWRITE(HD61830_TAG, hd61830_status_r, hd61830_control_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( kc85 )
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
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("NUM") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CODE") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
INPUT_PORTS_END

static INPUT_PORTS_START( pc8201a )
	PORT_INCLUDE( kc85 )

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
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PAST INS") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
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
	
	PORT_MODIFY("KEY7")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("STOP") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("f.5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("f.4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("f.3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("f.2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("f.1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))

	PORT_MODIFY("KEY8")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
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
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("NUM") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PRINT") PORT_CODE(KEYCODE_F11) PORT_CHAR(UCHAR_MAMEKEY(F11))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LABEL") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10))
INPUT_PORTS_END

/* uPD1990A Interface */

static WRITE_LINE_DEVICE_HANDLER( kc85_upd1990a_data_w )
{
	kc85_state *driver_state = device->machine->driver_data;

	driver_state->upd1990a_data = state;
}

static UPD1990A_INTERFACE( kc85_upd1990a_intf )
{
	DEVCB_LINE(kc85_upd1990a_data_w),
	DEVCB_CPU_INPUT_LINE(I8085_TAG, I8085_RST75_LINE)
};

/* RP5C01A Interface */

static RP5C01A_INTERFACE( tandy200_rp5c01a_intf )
{
	DEVCB_NULL								/* alarm */
};

/* 8155 Interface */

static READ8_DEVICE_HANDLER( kc85_8155_port_c_r )
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

	kc85_state *state = device->machine->driver_data;

	UINT8 data = 0;

	data |= state->upd1990a_data;
	data |= centronics_not_busy_r(state->centronics) << 1;
	data |= centronics_busy_r(state->centronics) << 2;

	return data;
}

static WRITE8_DEVICE_HANDLER( kc85_8155_port_a_w )
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

	kc85_state *state = device->machine->driver_data;

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

static WRITE8_DEVICE_HANDLER( kc85_8155_port_b_w )
{
	/*

		bit		signal		description

		0					LCD chip select 8, key scan 8
		1					LCD chip select 9
		2					beeper data output
		3		_RS232		modem select (0=RS232, 1=modem)
		4		PCS			soft power off
		5					beeper data input select (0=data from 8155 TO, 1=data from PB2)
		6		_DTR		RS232 data terminal ready output
		7		_RTS		RS232 request to send output

	*/

	kc85_state *state = device->machine->driver_data;

	/* keyboard */
	state->keylatch = (BIT(data, 0) << 8) | (state->keylatch & 0xff);

	/* LCD */
	state->lcd_cs2[8] = BIT(data, 0);
	state->lcd_cs2[9] = BIT(data, 1);

	/* beeper */
	state->buzzer = BIT(data, 2);
	state->bell = BIT(data, 5);

	if (state->buzzer) speaker_level_w(state->speaker, state->bell);
}

static WRITE_LINE_DEVICE_HANDLER( kc85_8155_to_w )
{
	kc85_state *driver_state = device->machine->driver_data;

	if (!driver_state->buzzer && driver_state->bell)
	{
		speaker_level_w(driver_state->speaker, state);
	}
}

static I8155_INTERFACE( kc85_8155_intf )
{
	DEVCB_NULL,								/* port A read */
	DEVCB_NULL,								/* port B read */
	DEVCB_HANDLER(kc85_8155_port_c_r),		/* port C read */
	DEVCB_HANDLER(kc85_8155_port_a_w),		/* port A write */
	DEVCB_HANDLER(kc85_8155_port_b_w),		/* port B write */
	DEVCB_NULL,								/* port C write */
	DEVCB_LINE(kc85_8155_to_w)				/* timer output */
};

static READ8_DEVICE_HANDLER( pc8201_8155_port_c_r )
{
	/*
	
		bit		signal		description

		0		CDI			clock data input
		1		SLCT		_BUSY signal from printer
		2		BUSY		BUSY signal from printer
		3		BCR			bar code reader data input
		4		_CTS		RS232 clear to send input
		5		_DSR		RS232 DSR input

	*/

	kc85_state *state = device->machine->driver_data;

	UINT8 data = 0;

	data |= state->upd1990a_data;
	data |= centronics_not_busy_r(state->centronics) << 1;
	data |= centronics_busy_r(state->centronics) << 2;

	return data;
}

static WRITE8_DEVICE_HANDLER( pc8201_8155_port_b_w )
{
	/*

		bit		signal		description

		0					LCD chip select 8, key scan 8
		1					LCD chip select 9
		2		_MC			melody control output
		3		DCD/_RD		RS232 DCD/_RD select
		4		APO			auto power off output
		5		BELL		buzzer output (0=ring, 1=not ring)
		6		_DTR		RS232 data terminal ready output
		7		_RTS		RS232 request to send output

	*/

	kc85_state *state = device->machine->driver_data;

	/* keyboard */
	state->keylatch = (BIT(data, 0) << 8) | (state->keylatch & 0xff);

	/* LCD */
	state->lcd_cs2[8] = BIT(data, 0);
	state->lcd_cs2[9] = BIT(data, 1);

	/* beeper */
	state->buzzer = BIT(data, 2);
	state->bell = BIT(data, 5);

	if (state->buzzer) speaker_level_w(state->speaker, state->bell);
}

static I8155_INTERFACE( pc8201_8155_intf )
{
	DEVCB_NULL,								/* port A read */
	DEVCB_NULL,								/* port B read */
	DEVCB_HANDLER(pc8201_8155_port_c_r),	/* port C read */
	DEVCB_HANDLER(kc85_8155_port_a_w),		/* port A write */
	DEVCB_HANDLER(pc8201_8155_port_b_w),	/* port B write */
	DEVCB_NULL,								/* port C write */
	DEVCB_LINE(kc85_8155_to_w)				/* timer output */
};

static READ8_DEVICE_HANDLER( tandy200_8155_port_c_r )
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

	tandy200_state *state = device->machine->driver_data;

	UINT8 data = 0x01;

	data |= centronics_not_busy_r(state->centronics) << 1;
	data |= centronics_busy_r(state->centronics) << 2;

	return data;
}

static WRITE8_DEVICE_HANDLER( tandy200_8155_port_a_w )
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

	tandy200_state *state = device->machine->driver_data;

	centronics_data_w(state->centronics, 0, data);

	state->keylatch = (state->keylatch & 0x100) | data;
}

static WRITE8_DEVICE_HANDLER( tandy200_8155_port_b_w )
{
	/*

		bit		signal		description

		0					key scan 8
		1		ORIG/ANS	(1=ORIG, 0=ANS)
		2		_BUZZER		(0=data from 8155 TO, 1=data from PB2)
		3		_RS232C		(1=modem, 0=RS-232)
		4		PCS			power cut signal
		5		BELL		buzzer data output
		6		MEN			modem enable output
		7		CALL		connects and disconnects the phone line

	*/

	tandy200_state *state = device->machine->driver_data;

	/* keyboard */
	state->keylatch = (BIT(data, 0) << 8) | (state->keylatch & 0xff);

	/* beeper */
	state->buzzer = BIT(data, 2);
	state->bell = BIT(data, 5);

	if (state->buzzer) speaker_level_w(state->speaker, state->bell);
}

static WRITE_LINE_DEVICE_HANDLER( tandy200_8155_to_w )
{
	tandy200_state *driver_state = device->machine->driver_data;

	if (!driver_state->buzzer && driver_state->bell)
	{
		speaker_level_w(driver_state->speaker, state);
	}
}

static I8155_INTERFACE( tandy200_8155_intf )
{
	DEVCB_NULL,								/* port A read */
	DEVCB_NULL,								/* port B read */
	DEVCB_HANDLER(tandy200_8155_port_c_r),	/* port C read */
	DEVCB_HANDLER(tandy200_8155_port_a_w),	/* port A write */
	DEVCB_HANDLER(tandy200_8155_port_b_w),	/* port B write */
	DEVCB_NULL,								/* port C write */
	DEVCB_LINE(tandy200_8155_to_w)			/* timer output */
};

/* MSM8251 Interface */

static msm8251_interface tandy200_msm8251_interface = {
	NULL,
	NULL,
	NULL
};

/* Machine Drivers */

static MACHINE_START( kc85 )
{
	kc85_state *state = machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, I8085_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);
	state->speaker = devtag_get_device(machine, "speaker");
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

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
	state_save_register_global(machine, state->bank);
	state_save_register_global(machine, state->upd1990a_data);
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->buzzer);
	state_save_register_global(machine, state->bell);
}

static MACHINE_START( pc8201 )
{
	kc85_state *state = machine->driver_data;

	/* find devices */
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);
	state->speaker = devtag_get_device(machine, "speaker");
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* initialize RTC */
	upd1990a_cs_w(state->upd1990a, 1);
	upd1990a_oe_w(state->upd1990a, 1);

	/* configure ROM banking */
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, I8085_TAG), 0);
	memory_configure_bank(machine, 1, 1, 1, memory_region(machine, "option"), 0);
	memory_configure_bank(machine, 1, 2, 2, mess_ram + 0x8000, 0x8000);
	memory_set_bank(machine, 1, 0);

	/* configure RAM banking */
	memory_configure_bank(machine, 2, 0, 1, mess_ram, 0);
	memory_configure_bank(machine, 2, 2, 2, mess_ram + 0x8000, 0x8000);
	memory_set_bank(machine, 2, 0);

	pc8201_bankswitch(machine, 0);

	/* register for state saving */
	state_save_register_global(machine, state->bank);
	state_save_register_global(machine, state->upd1990a_data);
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->buzzer);
	state_save_register_global(machine, state->bell);
	state_save_register_global(machine, state->iosel);
}

static MACHINE_START( trsm100 )
{
	kc85_state *state = machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, I8085_TAG, ADDRESS_SPACE_PROGRAM);
	
	/* find devices */
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);
	state->speaker = devtag_get_device(machine, "speaker");
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

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
	state_save_register_global(machine, state->bank);
	state_save_register_global(machine, state->upd1990a_data);
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->buzzer);
	state_save_register_global(machine, state->bell);
}

static MACHINE_START( tandy200 )
{
	tandy200_state *state = machine->driver_data;

	/* find devices */
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);
	state->speaker = devtag_get_device(machine, "speaker");
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* configure ROM banking */
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, I8085_TAG), 0);
	memory_configure_bank(machine, 1, 1, 1, memory_region(machine, I8085_TAG) + 0x10000, 0);
	memory_configure_bank(machine, 1, 2, 1, memory_region(machine, "option"), 0);
	memory_set_bank(machine, 1, 0);

	/* configure RAM banking */
	memory_configure_bank(machine, 2, 0, 3, mess_ram, 0x6000);
	memory_set_bank(machine, 2, 0);

	/* register for state saving */
	state_save_register_global(machine, state->bank);
	state_save_register_global(machine, state->tp);
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->buzzer);
	state_save_register_global(machine, state->bell);
}

static const cassette_config kc85_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED
};

static WRITE_LINE_DEVICE_HANDLER( kc85_sod_w )
{
	cassette_output(cassette_device_image(device->machine), state ? +1.0 : -1.0);
}

static READ_LINE_DEVICE_HANDLER( kc85_sid_r )
{
	return cassette_input(cassette_device_image(device->machine)) > 0.0;
}

static I8085_CONFIG( kc85_i8085_config )
{
	DEVCB_NULL,				/* STATUS changed callback */
	DEVCB_NULL,				/* INTE changed callback */
	DEVCB_LINE(kc85_sid_r),	/* SID changed callback (8085A only) */
	DEVCB_LINE(kc85_sod_w)	/* SOD changed callback (8085A only) */
};

static TIMER_DEVICE_CALLBACK( tandy200_tp_tick )
{
	tandy200_state *state = timer->machine->driver_data;

	cputag_set_input_line(timer->machine, I8085_TAG, I8085_RST75_LINE, state->tp);

	state->tp = !state->tp;
}

static MACHINE_DRIVER_START( kc85 )
	MDRV_DRIVER_DATA(kc85_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(I8085_TAG, 8085A, XTAL_4_9152MHz)
	MDRV_CPU_PROGRAM_MAP(kc85_mem)
	MDRV_CPU_IO_MAP(kc85_io)
	MDRV_CPU_CONFIG(kc85_i8085_config)

	MDRV_MACHINE_START( kc85 )

	/* video hardware */
	MDRV_IMPORT_FROM(kc85_video)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_I8155_ADD(I8155_TAG, XTAL_4_9152MHz/2, kc85_8155_intf)
	MDRV_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, kc85_upd1990a_intf)

	/* printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)

	/* cassette */
	MDRV_CASSETTE_ADD("cassette", kc85_cassette_config)

	/* option ROM cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc8201 )
	MDRV_DRIVER_DATA(kc85_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(I8085_TAG, 8085A, XTAL_4_9152MHz)
	MDRV_CPU_PROGRAM_MAP(pc8201_mem)
	MDRV_CPU_IO_MAP(pc8201_io)
	MDRV_CPU_CONFIG(kc85_i8085_config)

	MDRV_MACHINE_START(pc8201)

	/* video hardware */
	MDRV_IMPORT_FROM(kc85_video)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_I8155_ADD(I8155_TAG, XTAL_4_9152MHz/2, pc8201_8155_intf)
	MDRV_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, kc85_upd1990a_intf)

	/* printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)

	/* cassette */
	MDRV_CASSETTE_ADD("cassette", kc85_cassette_config)

	/* option ROM cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( trsm100 )
	MDRV_IMPORT_FROM(kc85)

	/* basic machine hardware */
	MDRV_CPU_MODIFY(I8085_TAG)
	MDRV_CPU_IO_MAP(trsm100_io)

	MDRV_MACHINE_START(trsm100)

	/* devices */
//	MDRV_MC14412_ADD(MC14412_TAG, XTAL_1MHz)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tandy200 )
	MDRV_DRIVER_DATA(tandy200_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(I8085_TAG, 8085A, XTAL_4_9152MHz)
	MDRV_CPU_PROGRAM_MAP(tandy200_mem)
	MDRV_CPU_IO_MAP(tandy200_io)
	MDRV_CPU_CONFIG(kc85_i8085_config)

	MDRV_MACHINE_START( tandy200 )

	/* video hardware */
	MDRV_IMPORT_FROM(tandy200_video)

	/* TP timer */
	MDRV_TIMER_ADD_PERIODIC("tp", tandy200_tp_tick, HZ(XTAL_4_9152MHz/2/8192))

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
//	MDRV_TCM5089_ADD(TCM5089_TAG, XTAL_3_579545MHz)

	/* devices */
	MDRV_I8155_ADD(I8155_TAG, XTAL_4_9152MHz/2, tandy200_8155_intf)
	MDRV_RP5C01A_ADD(RP5C01A_TAG, XTAL_32_768kHz, tandy200_rp5c01a_intf)
	MDRV_MSM8251_ADD(MSM8251_TAG, /*XTAL_4_9152MHz/2,*/ tandy200_msm8251_interface)
//	MDRV_MC14412_ADD(MC14412_TAG, XTAL_1MHz)

	/* printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)

	/* cassette */
	MDRV_CASSETTE_ADD("cassette", kc85_cassette_config)

	/* option ROM cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

/* ROMs */

ROM_START( kc85 )
	ROM_REGION( 0x8000, I8085_TAG, 0 )
	ROM_LOAD( "kc85rom.bin", 0x0000, 0x8000, CRC(8a9ddd6b) SHA1(9d18cb525580c9e071e23bc3c472380aa46356c0) )

	ROM_REGION( 0x8000, "option", ROMREGION_ERASEFF )
	ROM_CART_LOAD("cart", 0x0000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( npc8201a )
	ROM_REGION( 0x10000, I8085_TAG, 0 )
	ROM_LOAD( "pc8201rom.rom0", 0x0000, 0x8000, CRC(30555035) SHA1(96f33ff235db3028bf5296052acedbc94437c596) )

	ROM_REGION( 0x8000, "option", ROMREGION_ERASEFF )
	ROM_CART_LOAD("cart", 0x0000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( trsm100 )
	/*
		Board Code	ROM type			ROM Code			Comment
		-------------------------------------------------------------------
		PLX110CH1X	custom				LH535618			early North America
		PLX110EH1X	27C256 compatible	3256C07-3J1/11US	late North America
		PLX120CH1X	27C256 compatible	3256C05-3E1/11EP	European/Italian
	*/
	ROM_REGION( 0x8000, I8085_TAG, 0 )
	ROM_LOAD( "m100rom.m12",  0x0000, 0x8000, CRC(730a3611) SHA1(094dbc4ac5a4ea5cdf51a1ac581a40a9622bb25d) )

	ROM_REGION( 0x8000, "option", ROMREGION_ERASEFF )
	ROM_CART_LOAD("cart", 0x0000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( olivm10 )
	// 3256C02-4B3/I		Italian
	ROM_REGION( 0x8010, I8085_TAG, 0 )
	ROM_LOAD( "m10rom.m12", 0x0000, 0x8000, CRC(f0e8447a) SHA1(d58867276213116a79f7074109b7d7ce02e8a3af) )

	ROM_REGION( 0x8000, "option", ROMREGION_ERASEFF )
	ROM_CART_LOAD("cart", 0x0000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( tandy102 )
	ROM_REGION( 0x8000, I8085_TAG, 0 )
	ROM_LOAD( "m102rom.m12", 0x0000, 0x8000, CRC(08e9f89c) SHA1(b6ede7735a361c80419f4c9c0e36e7d480c36d11) )

	ROM_REGION( 0x8000, "option", ROMREGION_ERASEFF )
	ROM_CART_LOAD("cart", 0x0000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( tandy200 )
	ROM_REGION( 0x18000, I8085_TAG, 0 )
	ROM_LOAD( "rom #1-1.m15", 0x00000, 0x8000, NO_DUMP )
	ROM_LOAD( "rom #1-2.m13", 0x08000, 0x2000, NO_DUMP )
	ROM_LOAD( "rom #2.m14",	  0x10000, 0x8000, NO_DUMP )
	ROM_LOAD( "t200rom.bin", 0x0000, 0xa000, BAD_DUMP CRC(e3358b38) SHA1(35d4e6a5fb8fc584419f57ec12b423f6021c0991) ) /* Y2K hacked */
	ROM_CONTINUE(			0x10000, 0x8000 )

	ROM_REGION( 0x8000, "option", ROMREGION_ERASEFF )
	ROM_CART_LOAD("cart", 0x0000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

/* System Configurations */

static SYSTEM_CONFIG_START( kc85 )
	CONFIG_RAM_DEFAULT	(16 * 1024)
	CONFIG_RAM			(32 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( pc8201 )
	CONFIG_RAM_DEFAULT	(16 * 1024)
	CONFIG_RAM			(32 * 1024)
//	CONFIG_RAM			(48 * 1024)
	CONFIG_RAM			(64 * 1024)
	CONFIG_RAM			(96 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( trsm100 )
	CONFIG_RAM_DEFAULT	( 8 * 1024)
	CONFIG_RAM			(16 * 1024)
	CONFIG_RAM			(24 * 1024)
	CONFIG_RAM			(32 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( tandy102 )
	CONFIG_RAM_DEFAULT	(24 * 1024)
	CONFIG_RAM			(32 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( tandy200 )
	CONFIG_RAM_DEFAULT	(24 * 1024)
	CONFIG_RAM			(48 * 1024)
	CONFIG_RAM			(72 * 1024)
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR  NAME		PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY					FULLNAME */
COMP( 1983,	kc85,		0,		0,		kc85,		kc85,		0,		kc85,		"Kyosei",				"Kyotronic 85 (Japan)", 0 )
COMP( 1983, olivm10,	kc85,	0,		kc85,		olivm10,	0,		kc85,		"Olivetti",				"M-10", 0 )
//COMP( 1983, olivm10m,	kc85,	0,		kc85,		olivm10,	0,		kc85,		"Olivetti",				"M-10 Modem (US)", 0 )
COMP( 1983, trsm100,	0,		0,		trsm100,	kc85,		0,		trsm100,	"Tandy Radio Shack",	"TRS-80 Model 100", 0 )
COMP( 1986, tandy102,	trsm100,0,		trsm100,	kc85,		0,		tandy102,	"Tandy Radio Shack",	"Tandy 102", 0 )
//COMP( 1983, npc8201,	0,		0,		pc8201,		pc8201a,	0,		pc8201,		"NEC",					"PC-8201 (Japan)", 0 )
COMP( 1983, npc8201a,	0,		0,		pc8201,		pc8201a,	0,		pc8201,		"NEC",					"PC-8201A", 0 )
//COMP( 1987, npc8300,	npc8201,0,		pc8300,		pc8300,		0,		pc8300,		"NEC",					"PC-8300", 0 )
COMP( 1984, tandy200,	0,		0,		tandy200,	kc85,		0,		tandy200,	"Tandy Radio Shack",	"Tandy 200", 0 )
