/*

    Morrow MPZ80 "Decision"

    Skeleton driver

*/

/*

    TODO:

	- trap logic
	- memory management
		- task RAM
		- mapping RAM
		- attribute RAM
	- front panel LEDs
	- keyboard
	- I/O
		- Mult I/O
		- Wunderbus I/O (8259A PIC, 3x 8250 ACE, uPD1990C RTC)
	- floppy
		- DJ/DMA controller (Z80, 1K RAM, 2/4K ROM, TTL floppy control logic)
	- hard disk
		- HDCA controller (Shugart SA4000/Fujitsu M2301B/Winchester M2320B)
		- HD/DMA controller (Seagate ST-506/Shugart SA1000)
	- AM9512 FPU

*/

#include "includes/mpz80.h"


//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

enum
{
	NO_ACCESS = 0,
	READ_ONLY,
	EXECUTE_ONLY,
	FULL_ACCESS
};

#define R10	0x04



//**************************************************************************
//  MEMORY MANAGEMENT
//**************************************************************************

//-------------------------------------------------
//  get_address - 
//-------------------------------------------------

inline UINT32 mpz80_state::get_address(UINT16 offset)
{
	UINT16 map_addr = ((m_task & 0x0f) << 5) | (offset >> 11);
	UINT8 map = m_map_ram[map_addr];
	//UINT8 attr = m_map_ram[map_addr + 1];

	//logerror("task %02x map %02x attr %02x address %06x\n", m_task, map, attr, addr);
	
	// T7 T6 T5 T4 M7 M6 M5 M4 M3 M2 M1 M0 A11 A10 A9 A8 A7 A6 A5 A4 A3 A2 A1 A0
	return ((m_task & 0xf0) << 16) | (map << 12) | (offset & 0xfff);
}


//-------------------------------------------------
//  mmu_r - 
//-------------------------------------------------

READ8_MEMBER( mpz80_state::mmu_r )
{
	UINT32 addr = get_address(offset);
	UINT8 data = 0;
		
	if (addr < 0x1000)
	{
		if (addr < 0x400)
		{
			UINT8 *ram = ram_get_ptr(m_ram);
			data = ram[addr & 0x3ff];
		}
		else if (addr == 0x400)
		{
			data = trap_addr_r(space, 0);
		}
		else if (addr == 0x401)
		{
			data = keyboard_r(space, 0);
		}
		else if (addr == 0x402)
		{
			data = switch_r(space, 0);
		}
		else if (addr == 0x403)
		{
			data = status_r(space, 0);
		}
		else if (addr >= 0x600 && addr < 0x800)
		{
			// TODO this might change the map RAM contents
		}
		else if (addr < 0xc00)
		{
			UINT8 *rom = machine().region(Z80_TAG)->base();
			data = rom[addr & 0x7ff];
		}
		else
		{
			logerror("Unmapped LOCAL read at %06x\n", addr);
		}
	}
	else
	{
		logerror("Unmapped RAM read at %06x\n", addr);
	}
	
	return data;
}


//-------------------------------------------------
//  mmu_w - 
//-------------------------------------------------

WRITE8_MEMBER( mpz80_state::mmu_w )
{
	UINT32 addr = get_address(offset);

	if (addr < 0x1000)
	{
		if (addr < 0x400)
		{
			UINT8 *ram = ram_get_ptr(m_ram);
			ram[addr & 0x3ff] = data;
		}
		else if (addr == 0x400)
		{
			disp_seg_w(space, 0, data);
		}
		else if (addr == 0x401)
		{
			disp_col_w(space, 0, data);
		}
		else if (addr == 0x402)
		{
			task_w(space, 0, data);
		}
		else if (addr == 0x403)
		{
			mask_w(space, 0, data);
		}
		else if (addr >= 0x600 && addr < 0x800)
		{
			m_map_ram[addr - 0x600] = data;
		}
		else
		{
			logerror("Unmapped LOCAL read at %06x\n", addr);
		}
	}
	else
	{
		logerror("Unmapped RAM read at %06x\n", addr);
	}
}


//-------------------------------------------------
//  mmu_io_r - 
//-------------------------------------------------

READ8_MEMBER( mpz80_state::mmu_io_r )
{
	return 0;
}


//-------------------------------------------------
//  mmu_io_w - 
//-------------------------------------------------

WRITE8_MEMBER( mpz80_state::mmu_io_w )
{
}


//-------------------------------------------------
//  trap_addr_r - trap address register
//-------------------------------------------------

READ8_MEMBER( mpz80_state::trap_addr_r )
{
	/*
		
		bit		description
		
		0		DADDR 12
		1		DADDR 13
		2		DADDR 14
		3		DADDR 15
		4		I-ADDR 12
		5		I-ADDR 13
		6		I-ADDR 14
		7		I-ADDR 15
		
	*/

	return 0;
}


//-------------------------------------------------
//  status_r - trap status register
//-------------------------------------------------

READ8_MEMBER( mpz80_state::status_r )
{
	/*
		
		bit		description
		
		0		_TRAP VOID
		1		_IORQ
		2		_TRAP HALT
		3		_TRAP INT
		4		_TRAP STOP
		5		_TRAP AUX
		6		R10
		7		_RD STB
		
	*/

	return 0;
}


//-------------------------------------------------
//  task_w - task register
//-------------------------------------------------

WRITE8_MEMBER( mpz80_state::task_w )
{
	/*
		
		bit		description
		
		0		T0, A16
		1		T1, A17
		2		T2, A18
		3		T3, A19
		4		T4, S-100 A20
		5		T5, S-100 A21
		6		T6, S-100 A22
		7		T7, S-100 A23
		
	*/
	
	m_task = data;
}


//-------------------------------------------------
//  mask_w - mask register
//-------------------------------------------------

WRITE8_MEMBER( mpz80_state::mask_w )
{
	/*
		
		bit		description
		
		0		_STOP ENBL
		1		AUX ENBL
		2		_TINT ENBL
		3		RUN ENBL
		4		_HALT ENBL
		5		SINT ENBL
		6		_IOENBL
		7		_ZIO MODE
		
	*/
	
	m_mask = data;
}



//**************************************************************************
//  FRONT PANEL
//**************************************************************************

//-------------------------------------------------
//  keyboard_r - front panel keyboard
//-------------------------------------------------

READ8_MEMBER( mpz80_state::keyboard_r )
{
	/*
		
		bit		description
		
		0		
		1		
		2		
		3		
		4		
		5		
		6		
		7		
		
	*/

	return 0;
}


//-------------------------------------------------
//  switch_r - switch register
//-------------------------------------------------

READ8_MEMBER( mpz80_state::switch_r )
{
	/*
		
		bit		description
		
		0		_TRAP RESET
		1		INT PEND
		2		16C S6
		3		16C S5
		4		16C S4
		5		16C S3
		6		16C S2
		7		16C S1
		
	*/

	UINT8 data = 0;
	
	// switches
	data |= input_port_read(machine(), "16C") & 0xfc;
	
	return data;
}


//-------------------------------------------------
//  disp_seg_w - front panel segment
//-------------------------------------------------

WRITE8_MEMBER( mpz80_state::disp_seg_w )
{
	/*
		
		bit		description
		
		0		
		1		
		2		
		3		
		4		
		5		
		6		
		7		
		
	*/
}


//-------------------------------------------------
//  disp_col_w - front panel column
//-------------------------------------------------

WRITE8_MEMBER( mpz80_state::disp_col_w )
{
	/*
		
		bit		description
		
		0		
		1		
		2		
		3		
		4		
		5		
		6		
		7		
		
	*/
}



//**************************************************************************
//  WUNDERBUS I/O
//**************************************************************************

//-------------------------------------------------
//  wunderbus_r - 
//-------------------------------------------------

READ8_MEMBER( mpz80_state::wunderbus_r )
{
	UINT8 data = 0;

	if (offset < 7)
	{
		switch (m_wb_group)
		{
		case 0:
			switch (offset)
			{
			case 0: // DAISY 0 IN (STATUS)
				/*
				
					bit		description
					
					0		End of Ribbon
					1		Paper Out
					2		Cover Open
					3		Paper Feed Ready
					4		Carriage Ready
					5		Print Wheel Ready
					6		Check
					7		Printer Ready
					
				*/
				break;
				
			case 1: // Switch/Parallel port flags
				/*
				
					bit		description
					
					0		FLAG1
					1		FLAG2
					2		10A S6
					3		10A S5
					4		10A S4
					5		10A S3
					6		10A S2
					7		10A S1
					
				*/
				
				data = BITSWAP8(input_port_read(machine(), "10A"),0,1,2,3,4,5,6,7) & 0xfc;
				break;
				
			case 2: // R.T. Clock IN/RESET CLK. Int.
				/*
				
					bit		description
					
					0		1990 Data Out
					1		1990 TP
					2		
					3		
					4		
					5		
					6		
					7		
					
				*/
				
				data |= m_rtc->data_out_r();
				data |= m_rtc->tp_r() << 1;

				// reset clock interrupt
				m_rtc_tp = 0;
				pic8259_ir7_w(m_pic, m_rtc_tp);
				break;
				
			case 3: // Parallel data IN
				break;
				
			case 4: // 8259 0 register
			case 5: // 8259 1 register
				data = pic8259_r(m_pic, offset & 0x01);
				break;
				
			case 6: // not used
				break;
			}
			break;
			
		case 1:
			data = ins8250_r(m_ace1, offset);
			break;
			
		case 2:
			data = ins8250_r(m_ace2, offset);
			break;
			
		case 3:
			data = ins8250_r(m_ace3, offset);
			break;
		}
	}
	
	return data;
}


//-------------------------------------------------
//  wunderbus_w - 
//-------------------------------------------------

WRITE8_MEMBER( mpz80_state::wunderbus_w )
{
	if (offset == 7)
	{
		m_wb_group = data;
	}
	else
	{
		switch (m_wb_group)
		{
		case 0:
			switch (offset)
			{
			case 0: // DAISY 0 OUT
				/*
				
					bit		description
					
					0		Data Bit 9
					1		Data Bit 10
					2		Data Bit 11
					3		Data Bit 12
					4		Paper Feed Strobe
					5		Carriage Strobe
					6		Print Wheel Strobe
					7		Restore
					
				*/
				break;
				
			case 1: // DAISY 1 OUT
				/*
				
					bit		description
					
					0		Data Bit 1
					1		Data Bit 2
					2		Data Bit 3
					3		Data Bit 4
					4		Data Bit 5
					5		Data Bit 6
					6		Data Bit 7
					7		Data Bit 8
					
				*/
				break;
				
			case 2: // R.T. Clock OUT
				/*
				
					bit		description
					
					0		1990 Data In
					1		1990 Clk
					2		1990 C0
					3		1990 C1
					4		1990 C2
					5		1990 STB
					6		Ribbon Lift
					7		Select
					
				*/
				
				m_rtc->data_in_w(BIT(data, 0));
				m_rtc->clk_w(BIT(data, 0));
				m_rtc->c0_w(BIT(data, 0));
				m_rtc->c1_w(BIT(data, 0));
				m_rtc->c2_w(BIT(data, 0));
				m_rtc->stb_w(BIT(data, 0));
				break;
				
			case 3: // Par. data OUT
				break;
				
			case 4: // 8259 0 register
			case 5: // 8259 1 register
				pic8259_w(m_pic, offset & 0x01, data);
				break;
				
			case 6: // Par. port cntrl.
				/*
				
					bit		description
					
					0		POE
					1		_RST1
					2		_RST2
					3		_ATTN1
					4		_ATTN2
					5		
					6		
					7		
					
				*/
				break;
			}
			break;
			
		case 1:
			ins8250_w(m_ace1, offset, data);
			break;
			
		case 2:
			ins8250_w(m_ace2, offset, data);
			break;
			
		case 3:
			ins8250_w(m_ace3, offset, data);
			break;
		}
	}
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( mpz80_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( mpz80_mem, AS_PROGRAM, 8, mpz80_state )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(mmu_r, mmu_w)
/*
	Task 0 Segment 0 map:
	
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x0400) AM_READWRITE(trap_addr_r, disp_seg_w)
	AM_RANGE(0x0401, 0x0401) AM_READWRITE(keyboard_r, disp_col_w)
	AM_RANGE(0x0402, 0x0402) AM_READWRITE(switch_r, task_w)
	AM_RANGE(0x0403, 0x0403) AM_READWRITE(status_r, mask_w)
	AM_RANGE(0x0600, 0x07ff) AM_RAM AM_BASE(m_map_ram)
	AM_RANGE(0x0800, 0x0bff) AM_ROM AM_REGION(Z80_TAG, 0)
	AM_RANGE(0x0c00, 0x0c00) AM_DEVREADWRITE(AM9512_TAG, am9512_device, read, write)
*/
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( mpz80_io )
//-------------------------------------------------

static ADDRESS_MAP_START( mpz80_io, AS_IO, 8, mpz80_state )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(mmu_io_r, mmu_io_w)
//	AM_RANGE(0x48, 0x4f) AM_READWRITE(wunderbus_r, wunderbus_w)
//	AM_RANGE(0x80, 0x80) HD/DMA
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( wunderbus )
//-------------------------------------------------

static INPUT_PORTS_START( wunderbus )
	PORT_START("7C")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("7C:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x3e, 0x24, "BASE Port Address" ) PORT_DIPLOCATION("7C:2,3,4,5,6")
	PORT_DIPSETTING(    0x00, "00H" )
	// ...
	PORT_DIPSETTING(    0x24, "48H" )
	// ...
	PORT_DIPSETTING(    0x3e, "F8H" )
	PORT_DIPNAME( 0x40, 0x40, "FLAG2 Polarity" ) PORT_DIPLOCATION("7C:7")
	PORT_DIPSETTING(    0x40, "Negative" )
	PORT_DIPSETTING(    0x00, "Positive" )
	PORT_DIPNAME( 0x80, 0x80, "FLAG1 Polarity" ) PORT_DIPLOCATION("7C:8")
	PORT_DIPSETTING(    0x80, "Negative" )
	PORT_DIPSETTING(    0x00, "Positive" )

	PORT_START("10A")
	PORT_DIPNAME( 0x07, 0x00, "Baud Rate" ) PORT_DIPLOCATION("10A:1,2,3")
	PORT_DIPSETTING(    0x00, "Automatic" )
	PORT_DIPSETTING(    0x01, "19200" )
	PORT_DIPSETTING(    0x02, "9600" )
	PORT_DIPSETTING(    0x03, "4800" )
	PORT_DIPSETTING(    0x04, "2400" )
	PORT_DIPSETTING(    0x05, "1200" )
	PORT_DIPSETTING(    0x06, "300" )
	PORT_DIPSETTING(    0x07, "110" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("10A:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) ) PORT_DIPLOCATION("10A:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("10A:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) ) PORT_DIPLOCATION("10A:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) ) PORT_DIPLOCATION("10A:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


//-------------------------------------------------
//  INPUT_PORTS( mpz80 )
//-------------------------------------------------

static INPUT_PORTS_START( mpz80 )
	PORT_START("16C")
	PORT_DIPNAME( 0xf8, 0xf8, "Boot Address" ) PORT_DIPLOCATION("16C:1,2,3,4,5")
	PORT_DIPSETTING(    0xf8, "F800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xf0, "F000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xe8, "E800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xe0, "E000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xd8, "D800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xd0, "D000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xc8, "C800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xc0, "C000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xb8, "B800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xb0, "B000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xa8, "A800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xa0, "A000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x98, "9800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x90, "9000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x88, "8800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x80, "8000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x78, "7800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x70, "7000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x68, "6800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x60, "6000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x58, "5800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x50, "5000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x48, "4800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x40, "4000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x38, "3800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x30, "3000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x28, "2800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x20, "2000H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x18, "1800H" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x10, "Boot DJ/DMA" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x08, "Boot HD/DMA" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x00, "Boot HDCA" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x00, "Read Registers" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x10, "Write Registers" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x20, "Write Map RAMs" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x30, "Write R/W RAMs" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x40, "R/W FPP" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x50, "R/W S-100 Bus" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x60, "R/W S-100 Bus" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x70, "Read Switches" ) PORT_CONDITION("12C", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPNAME( 0x04, 0x00, "Power Up" ) PORT_DIPLOCATION("16C:6")
	PORT_DIPSETTING(    0x04, "Boot Address" )
	PORT_DIPSETTING(    0x00, "Monitor" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) ) PORT_DIPLOCATION("16C:7")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x00, "S-100 MWRITE" ) PORT_DIPLOCATION("16C:8")
	PORT_DIPSETTING(    0x01, "Disabled" )
	PORT_DIPSETTING(    0x00, "Enabled" )

	PORT_START("12C")
	PORT_DIPNAME( 0x02, 0x02, "Operation Mode" )
	PORT_DIPSETTING(    0x02, "Monitor" )
	PORT_DIPSETTING(    0x00, "Diagnostic" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	
	PORT_INCLUDE( wunderbus )
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//	pic8259_interface pic_intf
//-------------------------------------------------

/*

	bit		description
	
	IR0		S-100 V0
	IR1		S-100 V1
	IR2		S-100 V2
	IR3		Serial Device 1
	IR4		Serial Device 2
	IR5		Serial Device 3
	IR6		Daisy PWR line
	IR7		RT Clock TP line

*/

static struct pic8259_interface pic_intf =
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),
	DEVCB_LINE_VCC,
	DEVCB_NULL
};


//-------------------------------------------------
//	ins8250_interface ace1_intf
//-------------------------------------------------

static INS8250_INTERRUPT( ace1_interrupt )
{
	mpz80_state *driver_state = device->machine().driver_data<mpz80_state>();

	pic8259_ir3_w(driver_state->m_pic, state);
}

static INS8250_TRANSMIT( ace1_transmit )
{
	mpz80_state *state = device->machine().driver_data<mpz80_state>();
	
	terminal_write(state->m_terminal, 0, data);
}

static ins8250_interface ace1_intf =
{
	XTAL_18_432MHz/10,
	ace1_interrupt,
	ace1_transmit,
	NULL,
	NULL
};


//-------------------------------------------------
//	ins8250_interface ace2_intf
//-------------------------------------------------

static INS8250_INTERRUPT( ace2_interrupt )
{
	mpz80_state *driver_state = device->machine().driver_data<mpz80_state>();

	pic8259_ir4_w(driver_state->m_pic, state);
}

static ins8250_interface ace2_intf =
{
	XTAL_18_432MHz/10,
	ace2_interrupt,
	NULL,
	NULL,
	NULL
};


//-------------------------------------------------
//	ins8250_interface ace3_intf
//-------------------------------------------------

static INS8250_INTERRUPT( ace3_interrupt )
{
	mpz80_state *driver_state = device->machine().driver_data<mpz80_state>();

	pic8259_ir5_w(driver_state->m_pic, state);
}

static ins8250_interface ace3_intf =
{
	XTAL_18_432MHz/10,
	ace3_interrupt,
	NULL,
	NULL,
	NULL
};


//-------------------------------------------------
//	UPD1990A_INTERFACE( rtc_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( mpz80_state::rtc_tp_w )
{
	if (state)
	{
		m_rtc_tp = state;
		pic8259_ir7_w(m_pic, m_rtc_tp);
	}
}

static UPD1990A_INTERFACE( rtc_intf )
{
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(mpz80_state, rtc_tp_w)
};


//-------------------------------------------------
//  floppy_config super6_floppy_config
//-------------------------------------------------

static const floppy_config mpz80_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_8_DSDD,
    FLOPPY_OPTIONS_NAME(default),
    "floppy_8"
};


//-------------------------------------------------
//  GENERIC_TERMINAL_INTERFACE( terminal_intf )
//-------------------------------------------------

WRITE8_MEMBER( mpz80_state::terminal_w )
{
	ins8250_receive(m_ace1, data);
}

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_DRIVER_MEMBER(mpz80_state, terminal_w)
};


//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( mpz80 )
//-------------------------------------------------

void mpz80_state::machine_start()
{
	m_map_ram = auto_alloc_array_clear(machine(), UINT8, 0x200);
}


//-------------------------------------------------
//  MACHINE_RESET( mpz80 )
//-------------------------------------------------

void mpz80_state::machine_reset()
{
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( mpz80 )
//-------------------------------------------------

static MACHINE_CONFIG_START( mpz80, mpz80_state )
	// basic machine hardware
	MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(mpz80_mem)
	MCFG_CPU_IO_MAP(mpz80_io)

	// video hardware
	MCFG_FRAGMENT_ADD( generic_terminal )

	// devices
	MCFG_PIC8259_ADD(I8259A_TAG, pic_intf)
	MCFG_INS8250_ADD(INS8250_1_TAG, ace1_intf)
	MCFG_INS8250_ADD(INS8250_2_TAG, ace2_intf)
	MCFG_INS8250_ADD(INS8250_3_TAG, ace3_intf)
	MCFG_UPD1990A_ADD(UPD1990C_TAG, XTAL_32_768kHz, rtc_intf)
	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, mpz80_floppy_config)
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1K")

	// software list
	MCFG_SOFTWARE_LIST_ADD("flop_list", "mpz80")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( mpz80 )
//-------------------------------------------------

ROM_START( mpz80 )
	ROM_REGION( 0x1000, Z80_TAG, 0 )
	ROM_DEFAULT_BIOS("447")
	ROM_SYSTEM_BIOS( 0, "373", "3.73" )
	ROMX_LOAD( "mpz80-mon-3.73_fb34.17c", 0x0000, 0x1000, CRC(0bbffaec) SHA1(005ba726fc071f06cb1c969d170960438a3fc1a8), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "375", "3.75" )
	ROMX_LOAD( "mpz80-mon-3.75_0706.17c", 0x0000, 0x1000, CRC(1118a592) SHA1(d70f94c09602cd0bdc4fbaeb14989e8cc1540960), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "447", "4.47" )
	ROMX_LOAD( "mpz80-mon-4.47_f4f6.17c", 0x0000, 0x1000, CRC(b99c5d7f) SHA1(11181432ee524c7e5a68ead0671fc945256f5d1b), ROM_BIOS(3) )

	ROM_REGION( 0x20, "proms", 0 )
	ROM_LOAD( "z80-2_15a_82s123.15a", 0x00, 0x20, CRC(8a84249d) SHA1(dfbc49c5944f110f48419fd893fa84f4f0e113b8) ) // this is actually the 6331 PROM?

	ROM_REGION( 0x20, "plds", 0 )
	ROM_LOAD( "pal16r4.5c", 0x00, 0x20, NO_DUMP )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT    INIT    COMPANY                          FULLNAME        FLAGS
COMP( 1980, mpz80,  0,      0,      mpz80,  mpz80,  0,      "Morrow Designs",	"MPZ80",	GAME_NOT_WORKING | GAME_NO_SOUND_HW )
