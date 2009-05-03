/******************************************************************************************

	Kyocera Kyotronics 85 (and similar laptop computers)
	
	2009/04 Very Preliminary Driver (video emulation courtesy of very old code by 
	        Hamish Coleman)

	Comments about bit usage from Tech References and Virtual T source.

	Supported systems:
	  - Kyocera Kyotronic 85
	  - Olivetti M10 (slightly diff hw, the whole bios is shifted by 1 word)
	  - NEC PC-8201A (slightly diff hw)
	  - Tandy Model 100
	  - Tandy Model 102 (slightly diff hw)
	  - Tandy Model 200 (diff video & rtc)
	
	To Do:
	  - Basically everything:
	    * rtc (which also controls input reads)
		* inputs (keyboards are different)
		* sound
		* cassettes
		* optional ROM & RAM
		* additional hardware
		* what are the differences between available BIOSes for trsm100 and olivm10?
		* Tandy Model 200 support (video emulation completely different, but also 
		  memory map & IO have to be adapted)
		* PIO8155 might be made an independent device
		* clean up driver and split machine & video parts

	  - Find dumps of systems which could easily be added:
		* Olivetti M10 US (diff BIOS than the European version, it might be the alt BIOS)
		* NEC PC-8201 (original Japanese version of PC-8201A)

	  - Investigate other similar machines:
		* NEC PC-8300 (similar hardware?)
		* Tandy Model 600 (possibly different?)

******************************************************************************************/

/* Core includes */
#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "machine/upd1990a.h"

static UINT8 rom_page, ram_page;
static UINT8 port_a, port_b, port_c;
static UINT8 timer_lsb, timer_msb;
static UINT8 io_E8, io_A1, io_90, io_D0;

int pc8201_lcd_bank = 0;

void lcd_bank_update( UINT16 bank )
{
	switch ( bank ) 
	{
			case 0x000:	pc8201_lcd_bank = 0; break;
			case 0x001:	pc8201_lcd_bank = 1; break;
			case 0x002:	pc8201_lcd_bank = 2; break;
			case 0x004:	pc8201_lcd_bank = 3; break;
			case 0x008:	pc8201_lcd_bank = 4; break;
			case 0x010:	pc8201_lcd_bank = 5; break;
			case 0x020:	pc8201_lcd_bank = 6; break;
			case 0x040:	pc8201_lcd_bank = 7; break;
			case 0x080:	pc8201_lcd_bank = 8; break;
			case 0x100:	pc8201_lcd_bank = 9; break;
			case 0x200:	pc8201_lcd_bank = 10; break;
			default: /*logerror("pc8201: bank update with unk bank = %d\n", bank);*/ break;
	}

//	logerror("pc8201: _lcd_bank = %i\n",pc8201_lcd_bank);
}

/*  WRITE HANDLERS  */

// 0x90 PC-8201 only
static WRITE8_HANDLER( pc8201_io90_write )
{
	if ((data & 0x10) != (io_90 & 0x10))
	{
		const device_config *rtc = devtag_get_device(space->machine, "rtc");
		upd1990a_w(rtc, offset, port_a);
		logerror("RTC write: %d\n", port_a);
	}

	/* printer */
//	if ((data & 0x20) != (io_90 & 0x20))
//		WRITE port_a;

	io_90 = data;		// io_90 & 0xc0 is read in 0xa0!
}

// 0x90 T200 only
//static WRITE8_HANDLER( trs200_clock_mode_write )
//{
//}

// 0xA0
static WRITE8_HANDLER( modem_control_port_write )
{
}

// 0xA1 PC-8201 only
static WRITE8_HANDLER( pc8201_bank_write )
{
	UINT8 *rom = memory_region(space->machine, "maincpu");
	const address_space *space_program = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	
	if ((data & 0x03) != rom_page)
	{
		UINT8 read_only = 0;
		UINT8 optional_rom = 0;
		rom_page = data & 0x03;
		
		/* Test for a change to the ROM bank (0x0000 - 0x7FFF) */
		switch (rom_page)
		{
			case 0:
				memory_set_bankptr(space_program->machine, 1, rom + 0x10000);
				read_only = 1;
				break;
			case 1:
				optional_rom = 1;	/* Not implemented yet */
				break;
			case 2:
			case 3:
				memory_set_bankptr(space_program->machine, 1, mess_ram + (rom_page - 1) * 0x8000);
				break;
		}

		if (!optional_rom)
			memory_install_readwrite8_handler(space_program, 0x0000, 0x7fff, 0, 0, SMH_BANK1, read_only ? SMH_UNMAP : SMH_BANK1);
	}

	if (((data >> 2) & 0x03) != ram_page)
	{
		ram_page = (data >> 2) & 0x03;

		/* Test for a change to the RAM bank (0x8000 - 0xFFFF) */
		/* According to the PC-8200 Tech Notes, only 0,2,3 are admissible values here */
		switch (ram_page)
		{
			case 0:
			case 1:		/* However, we treat 1 as 0*/
				if (ram_page == 1)
					logerror("PC-8201 Illegal RAM Bank value\n");

				memory_set_bankptr(space_program->machine, 2, mess_ram);
				break;
			case 2:
			case 3:
				memory_set_bankptr(space_program->machine, 2, mess_ram + (ram_page - 1) * 0x8000);
				break;
		}
	}
			
	io_A1 = data & 0x0f;
}

// 0xB8
static WRITE8_HANDLER( pio8155_status_write )
{
// enable-disable
			/*
			Bit:
			    0 - Direction of Port A (0-input, 1-output)
			    1 - Direction of Port B (0-input, 1-output)
			2 & 3 - Port C definition (00 - All input, 11 - All output, 
				01 - Alt 3, 10 - Alt 4 (see Intel technical sheets 
				for more information))
			    4 - Enable Port A interrupt (1 - enable)
			    5 - Enable Port B interrupt (1 - enable)
			6 & 7 - Timer mode (00 - No effect on counter, 01 - Stop 
				counter immediately, 10 - Stop counter after TC, 11 
				- Start counter) */
}

// 0xB9
static WRITE8_HANDLER( pio8155_portA_write )
{
	port_a = data;
	lcd_bank_update( port_a | ((port_b & 3) << 8) );

			/*
			The first 5 bits of this port is used to control the 1990 real time clock chip.  
			The configuration of these five bits are:
			Bit:
			    0 -  C0
			    1 -  C1
			    2 -  C2
			    3 -  Clock
			    4 -  Serial data into clock chip */
	if ((data & 0x08) != (port_a & 0x08))
	{
		const device_config *rtc = devtag_get_device(space->machine, "rtc");
		upd1990a_clk_w(rtc, offset, data);
		logerror("CLK write %d\n", data);
	}
}

// 0xBA
static WRITE8_HANDLER( pio8155_portB_write )
{
			/*
			Bit:
			    0 - Column 9 select line for keyboard.  This line is
				also used for the CS-28 line of the LCD.
			    1 - CS 29 line of LCD
			    2 - Beep toggle (1-Data from bit 5, 0-Data from 8155 
				timer)
			    3 - Serial toggle (1-Modem, 0-RS232)
			    4 - Software on/off switch for computer
			    5 - Data to beeper if bit 2 set.  Set if bit 2 low.
			    6 - DTR (not) line for RS232
			    7 - RTS (not) line for RS232 */
	port_b = data;
	lcd_bank_update( port_a | ((port_b & 3) << 8) );
}

// 0xBB
static WRITE8_HANDLER( pio8155_portC_write )
{
	port_c = data;
}

// 0xBC
static WRITE8_HANDLER( pio8155_timer_lsb_write )
{
	timer_lsb = data;
}

// 0xBD
static WRITE8_HANDLER( pio8155_timer_msb_write )
{
	UINT8 timer_counter = 0;

	timer_msb = data;
	timer_counter = (timer_lsb | (timer_msb & 0x3f) << 8);
	
	// set serial baud
}


// 0xC0->0xCF
// serial write byet

// 0xC0->0xCE T200 only
// serial write byte

// 0xCF T200 only
// set serial through UART
 
// 0xD0... T200 only
static WRITE8_HANDLER( trs200_bank_write )	
{
	UINT8 *rom = memory_region(space->machine, "maincpu");
	const address_space *space_program = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	
	if ((data & 0x03) != rom_page)
	{
		rom_page = data & 0x03;
		
		/* Test for a change to the ROM bank (0x0000 - 0x7FFF) */
		switch (rom_page)
		{
			case 0:
				memory_set_bankptr(space_program->machine, 1, rom + 0x10000);
				memory_install_readwrite8_handler(space_program, 0x0000, 0x7fff, 0, 0, SMH_BANK1, SMH_UNMAP);
				memory_install_readwrite8_handler(space_program, 0x8000, 0x9fff, 0, 0, SMH_BANK2, SMH_UNMAP);
				break;
			case 1:
				memory_set_bankptr(space_program->machine, 1, rom + 0x1a000);	/* Multiplan */
				memory_install_readwrite8_handler(space_program, 0x0000, 0x7fff, 0, 0, SMH_BANK1, SMH_UNMAP);
				memory_install_readwrite8_handler(space_program, 0x8000, 0x9fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
				break;
			case 2: /* Optional ROM, not implemented yet */
				break;
			case 3:
				logerror("Model 200 Illegal ROM Bank value\n");				
				break;
		}
	}

	if (((data >> 2) & 0x03) != ram_page)
	{
		ram_page = (data >> 2) & 0x03;

		/* Test for a change to the RAM bank (0x8000 - 0xFFFF) */
		/* According to the PC-8200 Tech Notes, only 0,2,3 are admissible values here */
		switch (ram_page)
		{
			case 0:
			case 1:		/* However, we treat 1 as 0*/
				if (ram_page == 1)
					logerror("Model 200 Illegal RAM Bank value\n");

				memory_set_bankptr(space_program->machine, 2, mess_ram);
				break;
			case 2:
			case 3:
				memory_set_bankptr(space_program->machine, 2, mess_ram + (ram_page - 1) * 0x8000);
				break;
		}
	}
			
	io_D0 = data;
}

// 0xD0... all systems but T200 - set serial
				/*
				Bits:
					0 - Stop Bits (1-1.5, 0-2)
					1 - Parity (1-even, 0-odd)
					2 - Parity Enable (1-no parity, 0-parity enabled)
					3 - Data length (00-5 bits, 10-6 bits, 01-7 bits, 11-8 
					bits)
					4 - Data length (see bit 3) */


// 0xE0... all systems but T200
			/*
			Bits:
			    0 - ROM select (0-Standard ROM, 1-Option ROM)
			    1 - STROBE (not) signal to printer
			    2 - STROBE for Clock chip (1990)
			    3 - Remote plug control signal */
static WRITE8_HANDLER( ioE8_write )
{
	/* Check for Clock Chip strobe */
	if ((data & 0x04) != (io_E8 & 0x04))
	{
		const device_config *rtc = devtag_get_device(space->machine, "rtc");
		upd1990a_w(rtc, offset, port_a);
		logerror("RTC write: %d\n", port_a);
	}

	/* printer */
//	if ((data & 0x02) != (io_E8 & 0x02))
//		WRITE port_a;

	io_E8 = data;
}


// 0xE0... T200 only
//static WRITE8_HANDLER( trs200_ioE8_write )
//{
	// T200 Printer port STROBE is on Bit 0, not Bit 1
//	if ((data & 0x01) != (io_E8 & 0x01))
//		WRITE port_a;

//	io_E8 = data;
//}


UINT8 pc8201_lcd[11][256];
UINT8 pc8201_lcd_addr[10];

// 0xF0...
static WRITE8_HANDLER( lcd_command_write )
{
	pc8201_lcd_addr[pc8201_lcd_bank] = data;
	/* logerror("pc8201: LCDCM = 0x%02x\n",data); */
}

// 0xFF
static WRITE8_HANDLER( lcd_data_write )
{
	UINT8 addr = pc8201_lcd_addr[pc8201_lcd_bank]++;
	pc8201_lcd[pc8201_lcd_bank][addr] = data;
	/* logerror("pc8201: LCD [%i][%02x] = 0x%02x\n",pc8201_lcd_bank,addr,data); */
}

static WRITE8_HANDLER( unk_write )
{
	logerror("pc8201 IO port w: %04x %02x\n", offset, data);
}


/*  READ HANDLERS  */

// 0x82
static READ8_HANDLER( io82_read )
{
	return 0xa2;	/* FIXME */
}

// 0x90... T200 only
//static READ8_HANDLER( trs200_clock_mode_read )
//{
//}

// 0xA0 all systems but PC8201
// Modem control port
static READ8_HANDLER( modem_control_port_read )
{
	return 0;	// FIXME
}

// 0xA0 PC8201 only
static READ8_HANDLER( pc8201_ioa0_read )
{
	return (io_A1 & 0x3f) | (io_90 & 0xc0);
}

// 0xA1 PC-8201 only
static READ8_HANDLER( pc8201_bank_read )
{
	return io_A1;
}

// 0xB8
static READ8_HANDLER( pio8155_status_read )
{
			/*
			Bit:
			    0 - Port A interrupt request
			    1 - Port A buffer full/empty (input/output)
			    2 - Port A interrupt enabled
			    3 - Port B interrupt request
			    4 - Port B buffer full/empty (input/output)
			    5 - Port B interrupt enabled
			    6 - Timer interrupt (status of TC pin)
			    7 - Not used */

	/* Force TC high */
	return 0x89;
}

// 0xB9
static READ8_HANDLER( pio8155_portA_read )
{
	return port_a;
}

// 0xBA
static READ8_HANDLER( pio8155_portB_read )
{
	return port_b;
}

// 0xBB
static READ8_HANDLER( pio8155_portC_read )
{
	const device_config *rtc = devtag_get_device(space->machine, "rtc");
	port_c = (port_c & ~0x01) | upd1990a_data_out_r(rtc, offset);

	return port_c;
			/*
			Bits:
			    0 - Serial data input from clock chip
			    1 - Busy (not) signal from printer
			    2 - Busy signal from printer
			    3 - Data from BCR
			    4 - CTS (not) line from RS232
			    5 - DSR (not) line from RS232
			    6-7 - Not avaiable on 8155 */
}

// 0xBC
// 0xBD
static READ8_HANDLER( pio8155_timer_read )
{
	logerror("kyocera timer r: %04x\n", offset);
	return 0;
}

// 0xC0->0xCF all systems but T200
// serial read byte

// 0xC0->0xCE T200 only
// serial read byte

// 0xCF T200 only
// serial flag

// 0xD0->0xDF T200 only
static READ8_HANDLER( trs200_bank_read )
{
	return io_D0;
}
				/*
				Bits:
					0 - Data on telephone line (used to detect carrier)
					1 - Overrun error from UART
					2 - Framing error from UART
					3 - Parity error from UART
					4 - Transmit buffer empty from UART
					5 - Ring line on modem connector
					6 - Not used
					7 - Low Power signal from power supply (LPS not) */


// 0xD8?!?!
static READ8_HANDLER( uart_read )
{
	return 0xff;	/* FIXME */
}

// 0xE0...
static READ8_HANDLER( keyboard_read )
{
	UINT8 data = 0xff, col;
	UINT16 strobe = (port_b << 8) | port_a;
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4",	"KEY5", "KEY6", "KEY7", "KEY8" };

	for (col = 0; col < 9; col++) 
	{
		if ((strobe & (1 << col)) == 0) 
			data &= input_port_read(space->machine, keynames[col]);
	}

	logerror("CALL to %d and RETURN %d\n", offset, data);
	return data;
}

// 0xF0...
static READ8_HANDLER( lcd_status_read )
{
	/* logerror("pc8201: LCDST = 0\n"); */
	return 0;	/* currently we are always ready */
}

// 0xFF
static READ8_HANDLER( lcd_data_read )
{
	UINT8 addr = pc8201_lcd_addr[pc8201_lcd_bank]++;
	return pc8201_lcd[pc8201_lcd_bank][addr];
}

static READ8_HANDLER( unk_read )
{
	logerror("pc8201 IO port r: %04x\n", offset);
	return 0;
}

static ADDRESS_MAP_START( kyo85_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( npc8201_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( trs200_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(1)
	AM_RANGE(0x8000, 0x9fff) AM_RAMBANK(2)
	AM_RANGE(0xa000, 0xffff) AM_RAMBANK(3)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kyo85_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x81) AM_READWRITE( unk_read, unk_write )
	AM_RANGE(0x82, 0x82) AM_READ( io82_read )
	AM_RANGE(0x83, 0x9f) AM_READWRITE( unk_read, unk_write )
	AM_RANGE(0xa0, 0xa0) AM_READWRITE( modem_control_port_read, modem_control_port_write )
	AM_RANGE(0xb9, 0xb9) AM_READWRITE( pio8155_portA_read, pio8155_portA_write )
	AM_RANGE(0xba, 0xba) AM_READWRITE( pio8155_portB_read, pio8155_portB_write )
	AM_RANGE(0xbb, 0xbb) AM_READWRITE( pio8155_portC_read, pio8155_portC_write )
	AM_RANGE(0xbc, 0xbd) AM_READ( pio8155_timer_read )			// currently, this only logs read attempts
	AM_RANGE(0xbc, 0xbc) AM_WRITE( pio8155_timer_lsb_write )	// these should set serial comms but it's not yet implemented
	AM_RANGE(0xbd, 0xbd) AM_WRITE( pio8155_timer_msb_write )
	AM_RANGE(0xd8, 0xd8) AM_READ( uart_read )
	AM_RANGE(0xe0, 0xe8) AM_READ( keyboard_read )
	AM_RANGE(0xe0, 0xe8) AM_READWRITE( keyboard_read, ioE8_write )
	AM_RANGE(0xfe, 0xfe) AM_READWRITE( lcd_status_read, lcd_command_write ) /* LCD status / command */
	AM_RANGE(0xff, 0xff) AM_READWRITE( lcd_data_read, lcd_data_write )		/* LCD data */
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8201_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x82, 0x82) AM_READ( io82_read )
	AM_RANGE(0x90, 0x90) AM_WRITE( pc8201_io90_write )
	AM_RANGE(0xa0, 0xa0) AM_READWRITE( pc8201_ioa0_read, modem_control_port_write )
	AM_RANGE(0xa1, 0xa1) AM_READWRITE( pc8201_bank_read, pc8201_bank_write )
	AM_RANGE(0xb8, 0xb8) AM_READWRITE( pio8155_status_read, pio8155_status_write )
	AM_RANGE(0xb9, 0xb9) AM_READWRITE( pio8155_portA_read, pio8155_portA_write )
	AM_RANGE(0xba, 0xba) AM_READWRITE( pio8155_portB_read, pio8155_portB_write )
	AM_RANGE(0xbb, 0xbb) AM_READWRITE( pio8155_portC_read, pio8155_portC_write )
	AM_RANGE(0xbc, 0xbd) AM_READ( pio8155_timer_read )			// currently, this only logs read attempts
	AM_RANGE(0xbc, 0xbc) AM_WRITE( pio8155_timer_lsb_write )	// these should set serial comms but it's not yet implemented
	AM_RANGE(0xbd, 0xbd) AM_WRITE( pio8155_timer_msb_write )
	AM_RANGE(0xd8, 0xd8) AM_READ( uart_read )
	AM_RANGE(0xe0, 0xe8) AM_READ( keyboard_read )
//	AM_RANGE(0xe0, 0xe8) AM_READWRITE( keyboard_read, ioE8_write )
	AM_RANGE(0xfe, 0xfe) AM_READWRITE( lcd_status_read, lcd_command_write ) /* LCD status / command */
	AM_RANGE(0xff, 0xff) AM_READWRITE( lcd_data_read, lcd_data_write )		/* LCD data */
ADDRESS_MAP_END

static ADDRESS_MAP_START( trs200_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(0x00, 0x81) AM_READWRITE( unk_read, unk_write )
	AM_RANGE(0x82, 0x82) AM_READ( io82_read )
//	AM_RANGE(0x83, 0x8f) AM_READWRITE( unk_read, unk_write )
//	AM_RANGE(0x90, 0x90) AM_WRITE( trs200_clock_mode_write )
//	AM_RANGE(0x91, 0x9f) AM_READWRITE( unk_read, unk_write )
	AM_RANGE(0xa0, 0xa0) AM_READWRITE( modem_control_port_read, modem_control_port_write )
	AM_RANGE(0xb9, 0xb9) AM_READWRITE( pio8155_portA_read, pio8155_portA_write )
	AM_RANGE(0xba, 0xba) AM_READWRITE( pio8155_portB_read, pio8155_portB_write )
	AM_RANGE(0xbb, 0xbb) AM_READWRITE( pio8155_portC_read, pio8155_portC_write )
	AM_RANGE(0xbc, 0xbd) AM_READ( pio8155_timer_read )			// currently, this only logs read attempts
	AM_RANGE(0xbc, 0xbc) AM_WRITE( pio8155_timer_lsb_write )	// these should set serial comms but it's not yet implemented
	AM_RANGE(0xbd, 0xbd) AM_WRITE( pio8155_timer_msb_write )
	AM_RANGE(0xd0, 0xdf) AM_READWRITE( trs200_bank_read, trs200_bank_write )
	AM_RANGE(0xe0, 0xe8) AM_READ( keyboard_read )
//	AM_RANGE(0xe0, 0xe8) AM_READWRITE( keyboard_read, ioE8_write )
	AM_RANGE(0xfe, 0xfe) AM_READWRITE( lcd_status_read, lcd_command_write ) /* LCD status / command */
	AM_RANGE(0xff, 0xff) AM_READWRITE( lcd_data_read, lcd_data_write )		/* LCD data */
ADDRESS_MAP_END


// pasted from PX-4. to be fixed!!
static PALETTE_INIT( kyo85 )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static VIDEO_UPDATE( kyo85 )
{
	int x,y;
	int bank;
	int row,col;
	int addr;
	int yoff;
	unsigned char b;

	int pen1,pen0;

	pen0 = screen->machine->pens[0];
	pen1 = screen->machine->pens[1];

	for (y=0;y<64;y+=8) 
	{
		for (x=0;x<240;x++) 
		{
			bank = x/50;
			row = x%50;
			col = y/8;
			if (col>3) 
			{
				col -= 4;
				bank+= 5;
			}

		addr = col*64 + row;

		b = pc8201_lcd[bank+1][addr];

		/*logerror("pc8201: vid [%i][%02x] = 0x%02x (%i,%i)\n",bank,addr,b,x,y); */

	for (yoff = 0; yoff < 8; yoff++)
	{
		*BITMAP_ADDR16(bitmap, y+yoff, x ) = ((b & (1 << yoff)) == (1 << yoff)) ? pen0 : pen1;
	}
	
		}
	}

	return 0;
}


/**************************************************************************

	Keyboard

***************************************************************************/

/*

"Model 100" - must be changed for pc8201 and others
Data
(@ E0)	7 | L  A  I  ?  *  -> Ent f8   Brk
	6 | M  S  U  >  & <-  Prt f7
	5 | N  D  Y  <  ^  Up Lbl f6   Cap
	4 | B  F  T  "  % Dwn Pst f5   Num
	3 | V  G  R  :  $    Esc f4   Cde
	2 | C  H  E  ]  #  -  Tab f3   Gph
	1 | X  J  W  P  @  )  Del f2   Ctl
	0 | Z  K  Q  O  !  (  Spc f1   Sft
--------------------------------------------------
Strobe      0  1  2  3  4  5  6  7  |  0  Strobe
(from				    |     (from B2)
 B1)
*/
static INPUT_PORTS_START( kyo85 )
	PORT_START("KEY0")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)

	PORT_START("KEY1")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)

	PORT_START("KEY2")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)

	PORT_START("KEY3")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("?") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(">") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("<") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\"") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)

	PORT_START("KEY4")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("*") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("&") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("%") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("$") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("#") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("!") PORT_CODE(KEYCODE_1)

	PORT_START("KEY5")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LEFT") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("UP") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DOWN") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(")") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(") PORT_CODE(KEYCODE_9)

	PORT_START("KEY6")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENT") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PRT")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LBL")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PST") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) 

	PORT_START("KEY7")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F8")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F7")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F6")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)

	PORT_START("KEY8")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("BRK")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CAP") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CDE")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("GPH") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CTL") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SFT") PORT_CODE(KEYCODE_LSHIFT)
INPUT_PORTS_END


static MACHINE_START( kyo85 )
{
}

static MACHINE_RESET( kyo85 )
{
	rom_page = -1;
	ram_page = -1;
	port_a = 0;
	port_b = 0;
	port_c = 0;
	timer_lsb = 0;
	timer_msb = 0;
	io_E8 = 0;
	io_D0 = 0;
	io_A1 = 0;
	io_90 = 0;
}


static MACHINE_DRIVER_START( kyo85 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", 8085A, 2400000)
	MDRV_CPU_PROGRAM_MAP(kyo85_mem, 0)
	MDRV_CPU_IO_MAP(kyo85_io, 0)

	MDRV_MACHINE_START( kyo85 )
	MDRV_MACHINE_RESET( kyo85 )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(240, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 240-1, 0, 64-1)

//	MDRV_DEFAULT_LAYOUT(layout_kyo85)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(kyo85)

	MDRV_VIDEO_UPDATE(kyo85)

	/* sound hardware */
//	MDRV_SPEAKER_STANDARD_MONO("mono")
//	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
//	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_UPD1990A_ADD("rtc")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( npc8201 )
	MDRV_IMPORT_FROM(kyo85)

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(npc8201_mem, 0)
	MDRV_CPU_IO_MAP(pc8201_io, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( trs200 )
	MDRV_IMPORT_FROM(kyo85)

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(trs200_mem, 0)
	MDRV_CPU_IO_MAP(trs200_io, 0)

	/* devices */
	MDRV_UPD1990A_REMOVE("rtc")
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(trsm100)
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_SYSTEM_BIOS(0, "default", "Model 100")
	ROMX_LOAD( "m100rom.bin",  0x0000, 0x8000, CRC(730a3611) SHA1(094dbc4ac5a4ea5cdf51a1ac581a40a9622bb25d), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "alt", "Model 100 (alt)")
	ROMX_LOAD( "m100arom.bin", 0x0000, 0x8000, CRC(75ac39b7) SHA1(824e730ae45babf886023151504fcd6191bbed10), ROM_BIOS(2) )
ROM_END

ROM_START(olivm10)
	ROM_REGION( 0x8010, "maincpu", 0 )
	ROM_SYSTEM_BIOS(0, "default", "M10")
	ROMX_LOAD( "m10rom.bin", 0x0000, 0x8010, CRC(0be02b58) SHA1(56f2087a658efd0323663d15afcd4f5f27c68664), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "alt", "M10 (alt)")
	ROMX_LOAD( "m10arom.bin", 0x0000, 0x8010, CRC(afd5a43d) SHA1(b8362f7f248692de13128a47c6c116172e38da10), ROM_BIOS(2) )
ROM_END

ROM_START(kyo85)
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "kc85rom.bin", 0x0000, 0x8000, CRC(8a9ddd6b) SHA1(9d18cb525580c9e071e23bc3c472380aa46356c0) )
ROM_END

ROM_START(trsm102)
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "m102rom.bin", 0x0000, 0x8000, CRC(0e4ff73a) SHA1(d91f4f412fb78c131ccd710e8158642de47355e2) )
ROM_END

ROM_START(npc8201)
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "pc8201rom.bin", 0x10000, 0x8000, CRC(4c534662) SHA1(758fefbba251513e7f9d86f7e9016ad8817188d8) )
ROM_END

ROM_START(trsm200)
	ROM_REGION( 0x22000, "maincpu", 0 )
	ROM_LOAD( "t200rom.bin", 0x10000, 0x12000, CRC(e3358b38) SHA1(35d4e6a5fb8fc584419f57ec12b423f6021c0991) )
ROM_END

static DRIVER_INIT( npc8201 )
{
	UINT8 *rom = memory_region(machine, "maincpu");

	memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, SMH_BANK1, SMH_UNMAP);
	memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0xffff, 0, 0, SMH_BANK2, SMH_BANK2);

	memory_set_bankptr(machine, 1, rom + 0x10000);
	memory_set_bankptr(machine, 2, mess_ram);
}

static DRIVER_INIT( trs200 )
{
	UINT8 *rom = memory_region(machine, "maincpu");

	memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, 0x7fff, 0, 0, SMH_BANK1, SMH_UNMAP);
	memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0x9fff, 0, 0, SMH_BANK2, SMH_UNMAP);
	memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xa000, 0xffff, 0, 0, SMH_BANK3, SMH_BANK3);

	memory_set_bankptr(machine, 1, rom + 0x10000);
	memory_set_bankptr(machine, 2, rom + 0x18000);
	memory_set_bankptr(machine, 3, mess_ram);
}

static SYSTEM_CONFIG_START(npc8201)
	CONFIG_RAM_DEFAULT(96 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(trs200)
	CONFIG_RAM_DEFAULT(72 * 1024)
SYSTEM_CONFIG_END


/*    YEAR  NAME      PARENT   COMPAT  MACHINE     INPUT    INIT      CONFIG       COMPANY  FULLNAME */
COMP( 1983, kyo85,    0,       0,      kyo85,      kyo85,   0,        0,          "Kyocera",            "Kyotronic 85", GAME_NOT_WORKING )
COMP( 1983, olivm10,  0,       0,      kyo85,      kyo85,   0,        0,          "Olivetti",           "M10",       GAME_NOT_WORKING )
COMP( 1983, npc8201,  0,       0,      npc8201,    kyo85,   npc8201,  npc8201,    "NEC",                "PC-8201A", GAME_NOT_WORKING )
COMP( 1983, trsm100,  0,       0,      kyo85,      kyo85,   0,        0,          "Tandy Radio Shack",  "TRS-80 Model 100", GAME_NOT_WORKING )
COMP( 1984, trsm200,  0,       0,      trs200,     kyo85,   trs200,   trs200,     "Tandy Radio Shack",  "TRS-80 Model 200", GAME_NOT_WORKING )
COMP( 1986, trsm102,  0,       0,      kyo85,      kyo85,   0,        0,          "Tandy Radio Shack",  "TRS-80 Model 102", GAME_NOT_WORKING )
