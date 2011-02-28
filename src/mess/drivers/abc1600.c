/*

    Luxor ABC 1600

    Skeleton driver

*/

/*

    TODO:

	- memory access controller
		- task register
		- cause register (parity error)
		- segment RAM
		- page RAM
		- bankswitching
		- DMA
	- video
		- CRTC
		- bitmap layer
		- "mover" (blitter)
		- monitor portrait/landscape mode
    - CIO (interrupt controller)
		- RTC
		- NVRAM
    - DART
		- keyboard
    - floppy
    - hard disk (Xebec S1410)

*/

#include "includes/abc1600.h"



//**************************************************************************
//  CONSTANTS / MACROS
//**************************************************************************

// MAC
#define SEGMENT_ADDRESS(_segment) \
	(((m_task & 0x0f) << 5) | _segment)

#define SEGMENT_DATA(_segment) \
	m_segment_ram[SEGMENT_ADDRESS(_segment)]

#define PAGE_ADDRESS(_segment, _page) \
	((SEGMENT_DATA(_segment) << 4) | _page)

#define PAGE_DATA(_segment, _page) \
	m_page_ram[PAGE_ADDRESS(_segment, _page)]

#define PAGE_SIZE	0x800

#define PAGE_WP		0x4000
#define PAGE_NONX	0x8000


// DMA
#define A0 BIT(addr, 0)
#define A7 BIT(addr, 7)
#define A2_A1_A0 (addr & 0x07)
#define A1_A2 ((BIT(addr, 1) << 1) | BIT(addr, 2))
#define A2_A1 ((addr >> 1) & 0x03)


// DMA map
enum
{
	DMAMAP_R2_LO = 0,
	DMAMAP_R2_HI,
	DMAMAP_R1_LO,
	DMAMAP_R1_HI,
	DMAMAP_R0_LO,
	DMAMAP_R0_HI
};



//**************************************************************************
//  MEMORY ACCESS CONTROLLER
//**************************************************************************

//-------------------------------------------------
//  install_memory_page -
//-------------------------------------------------

void abc1600_state::install_memory_page(offs_t start, offs_t virtual_address, bool wp, bool nonx)
{
	address_space *program = cpu_get_address_space(m_maincpu, ADDRESS_SPACE_PROGRAM);
	offs_t end = start + PAGE_SIZE - 1;

	logerror("MAC physical %06x: virtual %06x %s %s\n", start, virtual_address, wp ? "WP":"", nonx ? "NONX":"");

	if (nonx)
	{
		// TODO read/write should raise Bus Error
		memory_unmap_readwrite(program, start, end, 0, 0);
	}
	else
	{
		if (virtual_address < 0x100000)
		{
			// main RAM
			UINT8 *ram = ram_get_ptr(m_ram);
			offs_t offset = virtual_address;
		
			if (wp)
				memory_install_rom(program, start, end, 0, 0, ram + offset);
			else
				memory_install_ram(program, start, end, 0, 0, ram + offset);
		}
		else if (virtual_address < 0x180000)
		{
			// video RAM
			offs_t offset = virtual_address - 0x100000;

			if (wp)
				memory_install_rom(program, start, end, 0, 0, m_video_ram + offset);
			else
				memory_install_ram(program, start, end, 0, 0, m_video_ram + offset);
		}
		else if (virtual_address >= 0x1ff000 && virtual_address < 0x1ff800)
		{
			// TODO I/O
			memory_unmap_readwrite(program, start, end, 0, 0);
		}
		else if (virtual_address >= 0x1ff800 && virtual_address < 0x200000)
		{
			// TODO I/O
			memory_unmap_readwrite(program, start, end, 0, 0);
		}
		else
		{
			// unmapped
			memory_unmap_readwrite(program, start, end, 0, 0);
		}
	}
}


//-------------------------------------------------
//  bankswitch -
//-------------------------------------------------

void abc1600_state::bankswitch()
{
	offs_t physical_address = 0;

	for (int segment = 0; segment < 32; segment++)
	{
		for (int page = 0; page < 32; page++)
		{
			UINT16 page_data = PAGE_DATA(segment, page);
			offs_t virtual_address = (page_data & 0x3ff) << 11;
			bool wp = ((page_data & PAGE_WP) == 0);
			bool nonx = ((page_data & PAGE_NONX) == PAGE_NONX);

			install_memory_page(physical_address, virtual_address, wp, nonx);

			physical_address += PAGE_SIZE;
		}
	}
}


//-------------------------------------------------
//  cause_r -
//-------------------------------------------------

READ8_MEMBER( abc1600_state::cause_r )
{
	return 0;
}


//-------------------------------------------------
//  task_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::task_w )
{
	/*
	
		bit		description
		
		0		SEGA5
		1		SEGA6
		2		SEGA7
		3		SEGA8
		4		
		5		
		6		BOOTE
		7		READ_MAGIC
	
	*/

	m_task = data;

	bankswitch();
}


//-------------------------------------------------
//  segment_r -
//-------------------------------------------------

READ8_MEMBER( abc1600_state::segment_r )
{
	/*
	
		bit		description
		
		0		SEGD0
		1		SEGD1
		2		SEGD2
		3		SEGD3
		4		SEGD4
		5		SEGD5
		6		SEGD6
		7		READ_MAGIC
	
	*/

	int segment = (offset >> 15) & 0x1f;
	UINT8 data = SEGMENT_DATA(segment);

	return (m_task & 0x80) | (data & 0x7f);
}


//-------------------------------------------------
//  segment_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::segment_w )
{
	/*
	
		bit		description
		
		0		SEGD0
		1		SEGD1
		2		SEGD2
		3		SEGD3
		4		SEGD4
		5		SEGD5
		6		SEGD6
		7		
	
	*/

	int segment = (offset >> 15) & 0x1f;
	SEGMENT_DATA(segment) = data & 0x7f;

	bankswitch();
}


//-------------------------------------------------
//  page_r -
//-------------------------------------------------

READ8_MEMBER( abc1600_state::page_r )
{
	/*
	
		bit		description
		
		0		X11
		1		X12
		2		X13
		3		X14
		4		X15
		5		X16
		6		X17
		7		X18

		8		X19
		9		X20
		10
		11
		12
		13
		14		_WP
		15		NONX
	
	*/

	int segment = (offset >> 15) & 0x1f;
	int page = (offset >> 11) & 0x0f;
	UINT16 data = PAGE_DATA(segment, page);

	int a0 = BIT(offset, 0);

	return a0 ? (data >> 8) : (data & 0xff);
}


//-------------------------------------------------
//  page_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::page_w )
{
	/*
	
		bit		description
		
		0		X11
		1		X12
		2		X13
		3		X14
		4		X15
		5		X16
		6		X17
		7		X18

		8		X19
		9		X20
		10
		11
		12
		13
		14		_WP
		15		NONX
	
	*/

	int segment = (offset >> 15) & 0x1f;
	int page = (offset >> 11) & 0x0f;
	int a0 = BIT(offset, 0);

	if (a0)
	{
		PAGE_DATA(segment, page) = (data << 8) | (PAGE_DATA(segment, page) & 0xff);
	}
	else
	{
		PAGE_DATA(segment, page) = (PAGE_DATA(segment, page) & 0xff00) | data;
	}

	bankswitch();
}


//**************************************************************************
//  DMA
//**************************************************************************

//-------------------------------------------------
//  get_dma_address - 
//-------------------------------------------------

inline offs_t abc1600_state::get_dma_address(int index, UINT16 offset)
{
	// A0 = DMA15, A1 = BA1, A2 = BA2
	UINT8 dmamap_addr = (index << 1) | BIT(offset, 15);
	UINT8 dmamap = m_dmamap[dmamap_addr & 0x07];

	return ((dmamap & 0x1f) << 16) | offset;
}


//-------------------------------------------------
//  dma_mreq_r - DMA memory read
//-------------------------------------------------

inline UINT8 abc1600_state::dma_mreq_r(int index, UINT16 offset)
{
	offs_t addr = get_dma_address(index, offset);
	UINT8 data = 0;
	
	if (addr < 0x100000)
	{
		UINT8 *ram = ram_get_ptr(m_ram);
		data = ram[addr];
	}
	else if (addr <= 0x180000)
	{
		data = m_video_ram[addr - 0x100000];
	}
	
	return data;
}


//-------------------------------------------------
//  dma_mreq_w - DMA memory write
//-------------------------------------------------

inline void abc1600_state::dma_mreq_w(int index, UINT16 offset, UINT8 data)
{
	offs_t addr = get_dma_address(index, offset);
	
	if (addr < 0x100000)
	{
		UINT8 *ram = ram_get_ptr(m_ram);
		ram[addr] = data;
	}
	else if (addr < 0x180000)
	{
		m_video_ram[addr - 0x100000] = data;
	}
}


//-------------------------------------------------
//  dma_iorq_r - DMA I/O read
//-------------------------------------------------

inline UINT8 abc1600_state::dma_iorq_r(int index, UINT16 offset)
{
	address_space *space = cpu_get_address_space(m_maincpu, ADDRESS_SPACE_PROGRAM);
	offs_t addr = get_dma_address(index, offset);
	UINT8 data = 0;
	
	if (addr >= 0x1ff000 && addr < 0x1ff100)
	{
		data = wd17xx_r(m_fdc, A2_A1);
	}
	else if (addr >= 0x1ff100 && addr < 0x1ff200)
	{
		if (A0) data = mc6845_register_r(m_crtc, 0);
	}
	else if (addr >= 0x1ff200 && addr < 0x1ff300)
	{
		data = z80dart_ba_cd_r(m_dart, A2_A1);
	}
	else if (addr >= 0x1ff300 && addr < 0x1ff400)
	{
		data = m_dma0->read();
	}
	else if (addr >= 0x1ff400 && addr < 0x1ff500)
	{
		data = m_dma1->read();
	}
	else if (addr >= 0x1ff500 && addr < 0x1ff600)
	{
		data = m_dma2->read();
	}
	else if (addr >= 0x1ff600 && addr < 0x1ff700)
	{
		data = scc8530_r(m_scc, A1_A2);
	}
	else if (addr >= 0x1ff700 && addr < 0x1ff800)
	{
		data = m_cio->read(*space, A2_A1);
	}
	else if (addr >= 0x1ff800 && addr < 0x1ff900)
	{
		data = iord0_r(*space, offset);
	}

	return 0;
}


//-------------------------------------------------
//  dma_iorq_w - DMA I/O write
//-------------------------------------------------

inline void abc1600_state::dma_iorq_w(int index, UINT16 offset, UINT8 data)
{
	address_space *space = cpu_get_address_space(m_maincpu, ADDRESS_SPACE_PROGRAM);
	offs_t addr = get_dma_address(index, offset);

	if (addr >= 0x1ff000 && addr < 0x1ff100)
	{
		wd17xx_w(m_fdc, A2_A1, data);
	}
	else if (addr >= 0x1ff100 && addr < 0x1ff200)
	{
		if (A0)
			mc6845_register_w(m_crtc, 0, data);
		else
			mc6845_address_w(m_crtc, 0, data);
	}
	else if (addr >= 0x1ff200 && addr < 0x1ff300)
	{
		z80dart_ba_cd_w(m_dart, A2_A1, data);
	}
	else if (addr >= 0x1ff300 && addr < 0x1ff400)
	{
		m_dma0->write(data);
	}
	else if (addr >= 0x1ff400 && addr < 0x1ff500)
	{
		m_dma1->write(data);
	}
	else if (addr >= 0x1ff500 && addr < 0x1ff600)
	{
		m_dma2->write(data);
	}
	else if (addr >= 0x1ff600 && addr < 0x1ff700)
	{
		scc8530_w(m_scc, A1_A2, data);
	}
	else if (addr >= 0x1ff700 && addr < 0x1ff800)
	{
		m_cio->write(*space, A2_A1, data);
	}
	else if (addr >= 0x1ff800 && addr < 0x1ff900)
	{
		iowr0_w(*space, A2_A1_A0, data);
	}
	else if (addr >= 0x1ff900 && addr < 0x1ffa00)
	{
		iowr1_w(*space, A2_A1_A0, data);
	}
	else if (addr >= 0x1ffa00 && addr < 0x1ffb00)
	{
		iowr2_w(*space, A2_A1_A0, data);
	}
	else if (addr >= 0x1ffb00 && addr < 0x1ffc00)
	{
		if (!A7)
		{
			if (A0)
				fw1_w(*space, 0, data);
			else
				fw0_w(*space, 0, data);
		}
	}
	else if (addr >= 0x1ffd00 && addr < 0x1ffe00)
	{
		dmamap_w(*space, A2_A1_A0, data);
	}
	else if (addr >= 0x1ffe00 && addr < 0x1fff00)
	{
		en_spec_contr_reg_w(*space, 0, data);
	}
}


//-------------------------------------------------
//  dmamap_w - DMA map write
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::dmamap_w )
{
	/*
	
		bit		description
		
		0		X16
		1		X17
		2		X18
		3		X19
		4		X20
		5		
		6		
		7		_R/W
	
	*/
	
	m_dmamap[offset] = data;
}



//**************************************************************************
//  READ/WRITE HANDLERS
//**************************************************************************

//-------------------------------------------------
//  fw0_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::fw0_w )
{
	/*
	
		bit		description
		
		0		SEL1
		1		SEL2
		2		SEL3
		3		MOTOR
		4		LC/PC
		5		LC/PC
		6		
		7		
	
	*/

	// drive select
	if (BIT(data, 0)) wd17xx_set_drive(m_fdc, 0);
//	if (BIT(data, 1)) wd17xx_set_drive(m_fdc, 1);
//	if (BIT(data, 2)) wd17xx_set_drive(m_fdc, 2);
	
	// floppy motor
	floppy_mon_w(m_floppy, !BIT(data, 3));
}


//-------------------------------------------------
//  fw1_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::fw1_w )
{
	/*
	
		bit		description
		
		0		MR
		1		DDEN
		2		HLT
		3		MINI
		4		HLD
		5		P0
		6		P1
		7		P2
	
	*/
	
	// FDC master reset
	wd17xx_mr_w(m_fdc, BIT(data, 0));
	
	// density select
	wd17xx_dden_w(m_fdc, BIT(data, 1));
}


//-------------------------------------------------
//  en_spec_contr_reg_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::en_spec_contr_reg_w )
{
	int state = BIT(data, 3);
	
	switch (data & 0x07)
	{
	case 0: // CS7
		break;
	
	case 1:
		break;
	
	case 2: // _BTCE
		break;
	
	case 3: // _ATCE
		break;
	
	case 4: // PARTST
		break;
	
	case 5: // _DMADIS
		m_dmadis = state;
		break;
	
	case 6: // SYSSCC
		m_sysscc = state;
		break;
	
	case 7: // SYSFS
		m_sysfs = state;
		if (m_sysfs) z80dma_rdy_w(m_dma0, wd17xx_drq_r(m_fdc));
		break;
	}
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( abc1600_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc1600_mem, ADDRESS_SPACE_PROGRAM, 8, abc1600_state )
	AM_RANGE(0x00000, 0x03fff) AM_ROM AM_REGION(MC68008P8_TAG, 0)
/*

	Supervisor Data map

	AM_RANGE(0x80000, 0x80001) AM_MIRROR(0x7f800) AM_MASK(0x7f800) AM_READWRITE(page_r, page_w)
	AM_RANGE(0x80003, 0x80003) AM_MIRROR(0x7f800) AM_MASK(0x7f800) AM_READWRITE(segment_r, segment_w)
	AM_RANGE(0x80004, 0x80004) AM_READ(cause_r)
	AM_RANGE(0x80007, 0x80007) AM_WRITE(task_w)


	Logical Address Map
	
	AM_RANGE(0x000000, 0x0fffff) AM_RAM
	AM_RANGE(0x100000, 0x17ffff) AM_RAM AM_MEMBER(m_video_ram)
	AM_RANGE(0x1ff000, 0x1ff007) AM_DEVREADWRITE_LEGACY(SAB1797_02P_TAG, wd17xx_r, wd17xx_w) // A2,A1
	AM_RANGE(0x1ff100, 0x1ff100) AM_DEVWRITE_LEGACY(SY6845E_TAG, mc6845_address_w)
	AM_RANGE(0x1ff101, 0x1ff101) AM_DEVREADWRITE_LEGACY(SY6845E_TAG, mc6845_register_r, mc6845_register_w)
	AM_RANGE(0x1ff200, 0x1ff207) AM_DEVREADWRITE_LEGACY(Z80DART_TAG, z80dart_ba_cd_r, z80dart_ba_cd_w) // A2,A1
	AM_RANGE(0x1ff600, 0x1ff607) AM_DEVREADWRITE(Z8530B1_TAG, scc8530_r, scc8530_w) // A2,A1
	AM_RANGE(0x1ff700, 0x1ff707) AM_DEVREADWRITE(Z8536B1_TAG, z8536_r, z8536_w) // A2,A1
	AM_RANGE(0x1ff800, 0x1ff800) AM_READ(iord0_w)
	AM_RANGE(0x1ff800, 0x1ff807) AM_WRITE(iowr0_w)
	AM_RANGE(0x1ff900, 0x1ff907) AM_WRITE(iowr1_w)
	AM_RANGE(0x1ffa00, 0x1ffa07) AM_WRITE(iowr2_w)
	AM_RANGE(0x1ffb00, 0x1ffb00) AM_WRITE(fw0_w)
	AM_RANGE(0x1ffb01, 0x1ffb01) AM_WRITE(fw1_w)
	AM_RANGE(0x1ffd00, 0x1ffd07) AM_WRITE(dmamap_w)
	AM_RANGE(0x1ffe00, 0x1ffe00) AM_WRITE(en_spec_contr_reg_w)

	*/

/*
	AM_RANGE(0x1ff000, 0x1ff000) AM_DEVREADWRITE_LEGACY(Z8410AB1_0_TAG, z80dma_r, z80dma_w)
	AM_RANGE(0x1ff000, 0x1ff000) AM_DEVREADWRITE_LEGACY(Z8410AB1_1_TAG, z80dma_r, z80dma_w)
	AM_RANGE(0x1ff000, 0x1ff000) AM_DEVREADWRITE_LEGACY(Z8410AB1_2_TAG, z80dma_r, z80dma_w)
	AM_RANGE(0x1ff000, 0x1ff003) AM_DEVREADWRITE(E050_C16PC_TAG, e050c16pc_r, e050c16pc_w)
*/
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( abc1600 )
//-------------------------------------------------

static INPUT_PORTS_START( abc1600 )
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  Z80DMA_INTERFACE( dma0_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( abc1600_state::dbrq_w )
{
	if (!m_dmadis)
	{
		// TODO
	}
}

READ8_MEMBER( abc1600_state::dma0_mreq_r )
{
	return dma_mreq_r(DMAMAP_R0_LO, offset);
}

WRITE8_MEMBER( abc1600_state::dma0_mreq_w )
{
	dma_mreq_w(DMAMAP_R0_LO, offset, data);
}

READ8_MEMBER( abc1600_state::dma0_iorq_r )
{
	return dma_iorq_r(DMAMAP_R0_LO, offset);
}

WRITE8_MEMBER( abc1600_state::dma0_iorq_w )
{
	dma_iorq_w(DMAMAP_R0_LO, offset, data);
}

static Z80DMA_INTERFACE( dma0_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(abc1600_state, dbrq_w),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(Z8410AB1_1_TAG, z80dma_bai_w),
	DEVCB_DRIVER_MEMBER(abc1600_state, dma0_mreq_r),
	DEVCB_DRIVER_MEMBER(abc1600_state, dma0_mreq_w),
	DEVCB_DRIVER_MEMBER(abc1600_state, dma0_iorq_r),
	DEVCB_DRIVER_MEMBER(abc1600_state, dma0_iorq_w)
};


//-------------------------------------------------
//  Z80DMA_INTERFACE( dma1_intf )
//-------------------------------------------------

READ8_MEMBER( abc1600_state::dma1_mreq_r )
{
	return dma_mreq_r(DMAMAP_R1_LO, offset);
}

WRITE8_MEMBER( abc1600_state::dma1_mreq_w )
{
	dma_mreq_w(DMAMAP_R1_LO, offset, data);
}

READ8_MEMBER( abc1600_state::dma1_iorq_r )
{
	return dma_iorq_r(DMAMAP_R1_LO, offset);
}

WRITE8_MEMBER( abc1600_state::dma1_iorq_w )
{
	dma_iorq_w(DMAMAP_R1_LO, offset, data);
}

static Z80DMA_INTERFACE( dma1_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(abc1600_state, dbrq_w),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(Z8410AB1_2_TAG, z80dma_bai_w),
	DEVCB_DRIVER_MEMBER(abc1600_state, dma1_mreq_r),
	DEVCB_DRIVER_MEMBER(abc1600_state, dma1_mreq_w),
	DEVCB_DRIVER_MEMBER(abc1600_state, dma1_iorq_r),
	DEVCB_DRIVER_MEMBER(abc1600_state, dma1_iorq_w)
};


//-------------------------------------------------
//  Z80DMA_INTERFACE( dma2_intf )
//-------------------------------------------------

READ8_MEMBER( abc1600_state::dma2_mreq_r )
{
	return dma_mreq_r(DMAMAP_R2_LO, offset);
}

WRITE8_MEMBER( abc1600_state::dma2_mreq_w )
{
	dma_mreq_w(DMAMAP_R2_LO, offset, data);
}

READ8_MEMBER( abc1600_state::dma2_iorq_r )
{
	return dma_iorq_r(DMAMAP_R2_LO, offset);
}

WRITE8_MEMBER( abc1600_state::dma2_iorq_w )
{
	dma_iorq_w(DMAMAP_R2_LO, offset, data);
}

static Z80DMA_INTERFACE( dma2_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(abc1600_state, dbrq_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(abc1600_state, dma2_mreq_r),
	DEVCB_DRIVER_MEMBER(abc1600_state, dma2_mreq_w),
	DEVCB_DRIVER_MEMBER(abc1600_state, dma2_iorq_r),
	DEVCB_DRIVER_MEMBER(abc1600_state, dma2_iorq_w)
};


//-------------------------------------------------
//  Z80DART_INTERFACE( dart_intf )
//-------------------------------------------------

static Z80DART_INTERFACE( dart_intf )
{
	0, 0, 0, 0,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DEVICE_LINE_MEMBER(ABC99_TAG, abc99_device, txd_r),
	DEVCB_DEVICE_LINE_MEMBER(ABC99_TAG, abc99_device, rxd_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_CPU_INPUT_LINE(MC68008P8_TAG, M68K_IRQ_5) // shared with SCC
};

//-------------------------------------------------
//  Z8536_INTERFACE( cio_intf )
//-------------------------------------------------

READ8_MEMBER( abc1600_state::cio_pa_r )
{
	/*
		
		bit		description
		
		PA0		BUS2
		PA1		BUS1
		PA2		BUS0X*2
		PA3		BUS0X*3
		PA4		BUS0X*4
		PA5		BUS0X*5
		PA6		BUS0X
		PA7		BUS0I
		
	*/
	
	return 0;
}

READ8_MEMBER( abc1600_state::cio_pb_r )
{
	/*
		
		bit		description
		
		PB0		
		PB1		POWERFAIL
		PB2		
		PB3		
		PB4		MINT
		PB5		_PREN-1
		PB6		_PREN-0
		PB7		FINT
		
	*/
	
	UINT8 data = 0;
	
	// floppy interrupt
	data |= wd17xx_intrq_r(m_fdc) << 7;

	return data;
}

WRITE8_MEMBER( abc1600_state::cio_pb_w )
{
	/*
		
		bit		description
		
		PB0		PRBR
		PB1		
		PB2		
		PB3		
		PB4		
		PB5		
		PB6		
		PB7		
		
	*/
}

READ8_MEMBER( abc1600_state::cio_pc_r )
{
	/*
		
		bit		description
		
		PC0		
		PC1		DATA IN
		PC2		
		PC3		
		
	*/

	UINT8 data = 0;

	// data in	
	data |= (e0516_dio_r(m_rtc) | m_nvram->do_r()) << 1;
	
	return 0;
}

WRITE8_MEMBER( abc1600_state::cio_pc_w )
{
	/*
		
		bit		description
		
		PC0		CLOCK
		PC1		DATA OUT
		PC2		RTC CS
		PC3		NVRAM CS
		
	*/
	
	int clock = BIT(data, 0);
	int data_out = BIT(data, 1);
	int rtc_cs = BIT(data, 2);
	int nvram_cs = BIT(data, 3);
	
	e0516_cs_w(m_rtc, rtc_cs);
	e0516_dio_w(m_rtc, data_out);
	e0516_clk_w(m_rtc, clock);
	
	m_nvram->cs_w(nvram_cs);
	m_nvram->di_w(data_out);
	m_nvram->sk_w(clock);
}

static Z8536_INTERFACE( cio_intf )
{
	DEVCB_CPU_INPUT_LINE(MC68008P8_TAG, M68K_IRQ_2),
	DEVCB_DRIVER_MEMBER(abc1600_state, cio_pa_r),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(abc1600_state, cio_pb_r),
	DEVCB_DRIVER_MEMBER(abc1600_state, cio_pb_w),
	DEVCB_DRIVER_MEMBER(abc1600_state, cio_pc_r),
	DEVCB_DRIVER_MEMBER(abc1600_state, cio_pc_w)
};


//-------------------------------------------------
//  wd17xx_interface fdc_intf
//-------------------------------------------------

static const floppy_config abc1600_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_5_25_DSQD,
    FLOPPY_OPTIONS_NAME(default),
    NULL
};

WRITE_LINE_MEMBER( abc1600_state::drq_w )
{
	if (m_sysfs) z80dma_rdy_w(m_dma0, state);
}

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,// DEVCB_DEVICE_LINE_MEMBER(Z8536B1_TAG, z8536_device, pb7_w), 
	DEVCB_DRIVER_LINE_MEMBER(abc1600_state, drq_w),
	{ FLOPPY_0, NULL, NULL, NULL }
};


//-------------------------------------------------
//  ABC99_INTERFACE( abc99_intf )
//-------------------------------------------------

static ABC99_INTERFACE( abc99_intf )
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(Z8470AB1_TAG, z80dart_rxtxcb_w),
	DEVCB_DEVICE_LINE(Z8470AB1_TAG, z80dart_dcdb_w)
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( abc1600 )
//-------------------------------------------------

void abc1600_state::machine_start()
{
}



//**************************************************************************
//  MACHINE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( abc1600 )
//-------------------------------------------------

static MACHINE_CONFIG_START( abc1600, abc1600_state )
	// basic machine hardware
	MCFG_CPU_ADD(MC68008P8_TAG, M68008, XTAL_64MHz/8)
	MCFG_CPU_PROGRAM_MAP(abc1600_mem)

	// video hardware
	MCFG_FRAGMENT_ADD(abc1600_video)

	// devices
	MCFG_Z80DMA_ADD(Z8410AB1_0_TAG, 4000000, dma0_intf)
	MCFG_Z80DMA_ADD(Z8410AB1_1_TAG, 4000000, dma1_intf)
	MCFG_Z80DMA_ADD(Z8410AB1_2_TAG, 4000000, dma2_intf)
	MCFG_Z80DART_ADD(Z8470AB1_TAG, XTAL_64MHz/16, dart_intf)
	MCFG_SCC8530_ADD(Z8530B1_TAG, XTAL_64MHz/16)
	MCFG_Z8536_ADD(Z8536B1_TAG, XTAL_64MHz/16, cio_intf)
	MCFG_NMC9306_ADD(NMC9306_TAG)
	MCFG_E0516_ADD(E050_C16PC_TAG, XTAL_32_768kHz)
	MCFG_WD179X_ADD(SAB1797_02P_TAG, fdc_intf)
	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, abc1600_floppy_config)
	MCFG_ABC99_ADD(abc99_intf)
	MCFG_S1410_ADD()

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1M")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( abc1600 )
//-------------------------------------------------

ROM_START( abc1600 )
	ROM_REGION( 0x4000, MC68008P8_TAG, 0 )
	ROM_LOAD( "boot 6490356-04.1f", 0x0000, 0x4000, CRC(9372f6f2) SHA1(86f0681f7ef8dd190b49eda5e781881582e0c2a4) )

	ROM_REGION( 0x2000, "wrmsk", 0 )
	ROM_LOAD( "wrmskl 6490362-01.1g", 0x0000, 0x1000, CRC(bc737538) SHA1(80e2c3757eb7f713018808d6e41ebef612425028) )
	ROM_LOAD( "wrmskh 6490363-01.1j", 0x1000, 0x1000, CRC(6b7c9f0b) SHA1(7155a993adcf08a5a8a2f22becf9fd66fda698be) )

	ROM_REGION( 0x200, "shinf", 0 )
	ROM_LOAD( "shinf 6490361-01.1f", 0x000, 0x200, CRC(20260f8f) SHA1(29bf49c64e7cc7592e88cde2768ac57c7ce5e085) )

	ROM_REGION( 0x40, "drmsk", 0 )
	ROM_LOAD( "drmskl 6490359-01.1k", 0x00, 0x20, CRC(6e71087c) SHA1(0acf67700d6227f4b315cf8fb0fb31c0e7fb9496) )
	ROM_LOAD( "drmskh 6490358-01.1k", 0x20, 0x20, CRC(a4a9a9dc) SHA1(d8575c0335d6021cbb5f7bcd298b41c35294a80a) )

	ROM_REGION( 0x71c, "plds", 0 )
	ROM_LOAD( "drmsk 6490360-01.1m", 0x000, 0x104, CRC(5f7143c1) SHA1(1129917845f8e505998b15288f02bf907487e4ac) ) // @ 1m,1n,1t,2t
	ROM_LOAD( "1020 6490349-01.8b",  0x104, 0x104, CRC(1fa065eb) SHA1(20a95940e39fa98e97e59ea1e548ac2e0c9a3444) )
	ROM_LOAD( "1021 6490350-01.5d",  0x208, 0x104, CRC(96f6f44b) SHA1(12d1cd153dcc99d1c4a6c834122f370d49723674) )
	ROM_LOAD( "1022 6490351-01.17e", 0x30c, 0x104, CRC(5dd00d43) SHA1(a3871f0d796bea9df8f25d41b3169dd4b8ef65ab) )
	ROM_LOAD( "1023 6490352-01.11e", 0x410, 0x104, CRC(a2f350ac) SHA1(77e08654a197080fa2111bc3031cd2c7699bf82b) )
	ROM_LOAD( "1024 6490353-01.12e", 0x514, 0x104, CRC(67f1328a) SHA1(b585495fe14a7ae2fbb29f722dca106d59325002) )
	ROM_LOAD( "1025 6490354-01.6e",  0x618, 0x104, CRC(9bda0468) SHA1(ad373995dcc18532274efad76fa80bd13c23df25) )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     INIT  COMPANY     FULLNAME     FLAGS
COMP( 1985, abc1600, 0,      0,      abc1600, abc1600, 0,    "Luxor", "ABC 1600", GAME_NOT_WORKING | GAME_NO_SOUND )
