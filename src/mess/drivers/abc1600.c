/*

    Luxor ABC 1600

    Skeleton driver

*/

/*

    TODO:

    - keyboard
        - "Bad command" on first enter after boot
    - floppy
        - internal floppy is really drive 2, but wd17xx.c doesn't like having NULL drives
    - BUS0I/0X/1/2
    - short/long reset (RSTBUT)
    - SCC interrupt
    - CIO
        - vectored interrupts with status
        - port A bit mode, OR-PEVM, interrupt when bit=1
        - port B bit mode, OR-PEVM, interrupt when bit=1
        - port C, open drain output bit PC1 (RTC, NVRAM)
        - counter 1 disabled
        - counter 2 enabled, TC=000d, IE=0, output to PB0
        - counter 3 enabled, TC=9c40, IE=1
    - hard disk (Xebec S1410)

*/

#include "includes/abc1600.h"



//**************************************************************************
//  CONSTANTS / MACROS
//**************************************************************************

#define LOG 0

// MAC
#define A0			BIT(offset, 0)
#define A1			BIT(offset, 1)
#define A2			BIT(offset, 2)
#define A7			BIT(offset, 7)
#define A8			BIT(offset, 8)
#define X11			BIT(offset, 11)
#define X12			BIT(offset, 12)
#define A17			BIT(offset, 17)
#define A18			BIT(offset, 18)
#define A19			BIT(offset, 19)

#define A10_A9_A8	((offset >> 8) & 0x07)
#define A2_A1_A0	(offset & 0x07)
#define A1_A2		((A1 << 1) | A2)
#define A2_A1		((offset >> 1) & 0x03)

#define FC0			BIT(fc, 0)
#define FC1			BIT(fc, 1)
#define FC2			BIT(fc, 2)

#define PAGE_WP		BIT(page_data, 14)
#define PAGE_NONX	BIT(page_data, 15)


// task register
#define BOOTE		BIT(m_task, 6)
#define MAGIC		BIT(m_task, 7)
#define READ_MAGIC	!MAGIC


// DMA map
enum
{
	DMAMAP_R2_LO = 0,
	DMAMAP_R2_HI,
	DMAMAP_R1_LO = 4,
	DMAMAP_R1_HI,
	DMAMAP_R0_LO,
	DMAMAP_R0_HI
};


// internal I/O registers
enum
{
	FLP = 0,
	CRT,
	DRT,
	DMA0,
	DMA1,
	DMA2,
	SCC,
	CIO
};


// internal I/O registers
enum
{
	IORD0 = 0
};


// internal I/O registers
enum
{
	IOWR0 = 0,
	IOWR1,
	IOWR2,
	FW,
	DMAMAP = 5,
	SPEC_CONTR_REG
};



//**************************************************************************
//  MEMORY ACCESS CONTROLLER
//**************************************************************************

//-------------------------------------------------
//  read_ram -
//-------------------------------------------------

UINT8 abc1600_state::read_ram(offs_t offset)
{
	UINT8 data = 0;

	if (offset < 0x100000)
	{
		// main RAM
		UINT8 *ram = ram_get_ptr(m_ram);
		data = ram[offset];
	}
	else if (offset < 0x180000)
	{
		// video RAM
		address_space *program = m_maincpu->memory().space(AS_PROGRAM);
		data = video_ram_r(*program, offset);
	}
	else
	{
		if (LOG) logerror("Unmapped read from virtual memory %06x\n", offset);
	}

	return data;
}


//-------------------------------------------------
//  write_ram -
//-------------------------------------------------

void abc1600_state::write_ram(offs_t offset, UINT8 data)
{
	if (offset < 0x100000)
	{
		// main RAM
		UINT8 *ram = ram_get_ptr(m_ram);
		ram[offset] = data;
	}
	else if (offset < 0x180000)
	{
		// video RAM
		address_space *program = m_maincpu->memory().space(AS_PROGRAM);
		video_ram_w(*program, offset, data);
	}
	else
	{
		if (LOG) logerror("Unmapped write to virtual memory %06x : %02x\n", offset, data);
	}
}


//-------------------------------------------------
//  read_io -
//-------------------------------------------------

UINT8 abc1600_state::read_io(offs_t offset)
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	UINT8 data = 0;

	if (X12)
	{
		if (X11)
		{
			switch (A10_A9_A8)
			{
			case IORD0:
				data = iord0_r(*program, offset);
				break;

			default:
				logerror("Unmapped read from virtual I/O %06x\n", offset);
			}
		}
		else
		{
			switch (A10_A9_A8)
			{
			case FLP:
				data = wd17xx_r(m_fdc, A2_A1);
				break;

			case CRT:
				if (A0)
					data = m_crtc->register_r(*program, offset);
				else
					data = m_crtc->status_r(*program, offset);
				break;

			case DRT:
				data = z80dart_ba_cd_r(m_dart, A2_A1 ^ 0x03);
				break;

			case DMA0:
				data = m_dma0->read();
				break;

			case DMA1:
				data = m_dma1->read();
				break;

			case DMA2:
				data = m_dma2->read();
				break;

			case SCC:
				data = scc8530_r(m_scc, A1_A2);
				break;

			case CIO:
				data = m_cio->read(*program, A2_A1);
				break;
			}
		}
	}
	else
	{
		// BUS0I, BUS0X, BUS1, BUS2
		//UINT8 cs = (m_cs7 << 7) | ((offset >> 5) & 0x3f);

		switch ((offset >> 1) & 0x07)
		{
		case 0: // INP
			break;

		case 1: // STAT
			break;

		case 2: // OPS
			break;
		}

		logerror("Unmapped read from virtual I/O %06x\n", offset);
	}

	return data;
}


//-------------------------------------------------
//  write_io -
//-------------------------------------------------

void abc1600_state::write_io(offs_t offset, UINT8 data)
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);

	if (X12)
	{
		if (X11)
		{
			switch (A10_A9_A8)
			{
			case IOWR0:
				iowr0_w(*program, offset, data);
				break;

			case IOWR1:
				iowr1_w(*program, offset, data);
				break;

			case IOWR2:
				iowr2_w(*program, offset, data);
				break;

			case FW:
				if (!A7)
				{
					if (A0)
						fw1_w(*program, offset, data);
					else
						fw0_w(*program, offset, data);
				}
				else
				{
					logerror("Unmapped write to virtual I/O %06x : %02x\n", offset, data);
				}
				break;

			case DMAMAP:
				dmamap_w(*program, offset, data);
				break;

			case SPEC_CONTR_REG:
				spec_contr_reg_w(*program, offset, data);
				break;

			default:
				logerror("Unmapped write to virtual I/O %06x : %02x\n", offset, data);
			}
		}
		else
		{
			switch (A10_A9_A8)
			{
			case FLP:
				wd17xx_w(m_fdc, A2_A1, data);
				break;

			case CRT:
				if (A0)
					m_crtc->register_w(*program, offset, data);
				else
					m_crtc->address_w(*program, offset, data);
				break;

			case DRT:
				z80dart_ba_cd_w(m_dart, A2_A1 ^ 0x03, data);
				break;

			case DMA0:
				m_dma0->write(data);
				break;

			case DMA1:
				m_dma1->write(data);
				break;

			case DMA2:
				m_dma2->write(data);
				break;

			case SCC:
				scc8530_w(m_scc, A1_A2, data);
				break;

			case CIO:
				m_cio->write(*program, A2_A1, data);
				break;
			}
		}
	}
	else
	{
		//UINT8 cs = (m_cs7 << 7) | ((offset >> 5) & 0x3f);

		switch ((offset >> 1) & 0x07)
		{
		case 0: // OUT
			break;

		case 2: // C1
			break;

		case 3: // C2
			break;

		case 4: // C3
			break;

		case 5: // C4
			break;
		}

		// BUS0I, BUS0X, BUS1, BUS2
		logerror("Unmapped write %02x to virtual I/O %06x\n", data, offset);
	}
}


//-------------------------------------------------
//  get_current_task -
//-------------------------------------------------

int abc1600_state::get_current_task(offs_t offset)
{
	int force_task0 = !(m_ifc2 | A19);
	int t0 = !(BIT(m_task, 0) | force_task0);
	int t1 = !(BIT(m_task, 1) | force_task0);
	int t2 = !(BIT(m_task, 2) | force_task0);
	int t3 = !(BIT(m_task, 3) | force_task0);

	return (t3 << 3) | (t2 << 2) | (t1 << 1) | t0;
}


//-------------------------------------------------
//  get_segment_address -
//-------------------------------------------------

offs_t abc1600_state::get_segment_address(offs_t offset)
{
	int sega19 = !(!(A8 | m_ifc2) | !A19);
	int task = get_current_task(offset);

	return (task << 5) | (sega19 << 4) | ((offset >> 15) & 0x0f);
}


//-------------------------------------------------
//  get_page_address -
//-------------------------------------------------

offs_t abc1600_state::get_page_address(offs_t offset, UINT8 segd)
{
	return ((segd & 0x3f) << 4) | ((offset >> 11) & 0x0f);
}


//-------------------------------------------------
//  translate_address -
//-------------------------------------------------

offs_t abc1600_state::translate_address(offs_t offset, int *nonx, int *wp)
{
	// segment
	offs_t sega = get_segment_address(offset);
	UINT8 segd = m_segment_ram[sega];

	// page
	offs_t pga = get_page_address(offset, segd);
	UINT16 page_data = m_page_ram[pga];

	offs_t virtual_offset = ((page_data & 0x3ff) << 11) | (offset & 0x7ff);

	if (PAGE_NONX)
	{
		logerror("Bus error %06x : %06x\n", offset, virtual_offset);
		m_maincpu->set_input_line(M68K_LINE_BUSERROR, ASSERT_LINE);
		m_maincpu->set_input_line(M68K_LINE_BUSERROR, CLEAR_LINE);
	}

	*nonx = PAGE_NONX;
	*wp = PAGE_WP;

	return virtual_offset;
}


//-------------------------------------------------
//  read_user_memory -
//-------------------------------------------------

UINT8 abc1600_state::read_user_memory(offs_t offset)
{
	int nonx = 0, wp = 0;
	offs_t virtual_offset = translate_address(offset, &nonx, &wp);
	UINT8 data = 0;

	if (virtual_offset < 0x1fe000)
	{
		data = read_ram(virtual_offset);
	}
	else
	{
		data = read_io(virtual_offset);
	}

	return data;
}


//-------------------------------------------------
//  write_user_memory -
//-------------------------------------------------

void abc1600_state::write_user_memory(offs_t offset, UINT8 data)
{
	int nonx = 0, wp = 0;
	offs_t virtual_offset = translate_address(offset, &nonx, &wp);

	if (nonx || !wp) return;

	if (virtual_offset < 0x1fe000)
	{
		write_ram(virtual_offset, data);
	}
	else
	{
		write_io(virtual_offset, data);
	}
}


//-------------------------------------------------
//  read_supervisor_memory -
//-------------------------------------------------

UINT8 abc1600_state::read_supervisor_memory(offs_t offset)
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	UINT8 data = 0;

	if (!A19)
	{
		data = read_user_memory(offset);
	}
	else
	{
		if (!A2 & !A1)
		{
			// _EP
			data = page_r(*program, offset);
		}
		else if (!A2 & A1 & A0)
		{
			// _ES
			data = segment_r(*program, offset);
		}
		else if (A2 & A1 & A0)
		{
			// _CAUSE
			data = cause_r(*program, offset);
		}
	}

	return data;
}


//-------------------------------------------------
//  write_supervisor_memory -
//-------------------------------------------------

void abc1600_state::write_supervisor_memory(offs_t offset, UINT8 data)
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);

	if (!A19)
	{
		write_user_memory(offset, data);
	}
	else
	{
		if (!A2 & !A1)
		{
			// _WEP
			page_w(*program, offset, data);
		}
		else if (!A2 & A1 & A0)
		{
			// _WES
			segment_w(*program, offset, data);
		}
		else if (A2 & !A1 & A0)
		{
			// W(C)
			task_w(*program, offset, data);
		}
	}
}


//-------------------------------------------------
//  get_fc -
//-------------------------------------------------

int abc1600_state::get_fc()
{
	UINT16 fc = m68k_get_fc(m_maincpu);

	m_ifc2 = !(!(MAGIC | FC0) | FC2);

	return fc;
}


//-------------------------------------------------
//  mac_r -
//-------------------------------------------------

READ8_MEMBER( abc1600_state::mac_r )
{
	int fc = get_fc();

	UINT8 data = 0;

	if (!BOOTE && !A19 && !A18 && !A17)
	{
		// _BOOTCE
		UINT8 *rom = machine().region(MC68008P8_TAG)->base();
		data = rom[offset & 0x3fff];
	}
	else if (!m_ifc2 && !FC1)
	{
		data = read_supervisor_memory(offset);
	}
	else
	{
		data = read_user_memory(offset);
	}

	return data;
}


//-------------------------------------------------
//  mac_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::mac_w )
{
	int fc = get_fc();

	if (!m_ifc2 && !FC1)
	{
		write_supervisor_memory(offset, data);
	}
	else
	{
		write_user_memory(offset, data);
	}
}


//-------------------------------------------------
//  cause_r -
//-------------------------------------------------

READ8_MEMBER( abc1600_state::cause_r )
{
	/*

        bit     description

        0       RSTBUT
        1       1
        2       DMAOK
        3       X16
        4       X17
        5       X18
        6       X19
        7       X20

    */

	UINT8 data = 0x02;

	// DMA status
	data |= m_cause;

	watchdog_reset(machine());

	return data;
}


//-------------------------------------------------
//  task_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::task_w )
{
	/*

        bit     description

        0       TASKD0* (inverted SEGA5)
        1       TASKD1* (inverted SEGA6)
        2       TASKD2* (inverted SEGA7)
        3       TASKD3* (inverted SEGA8)
        4
        5
        6       BOOTE*
        7       MAGIC*

    */

	m_task = data ^ 0xff;

	if (LOG) logerror("%s: %06x Task %u BOOTE %u MAGIC %u\n", machine().describe_context(), offset, get_current_task(offset), BOOTE, MAGIC);
}


//-------------------------------------------------
//  segment_r -
//-------------------------------------------------

READ8_MEMBER( abc1600_state::segment_r )
{
	/*

        bit     description

        0       SEGD0
        1       SEGD1
        2       SEGD2
        3       SEGD3
        4       SEGD4
        5       SEGD5
        6       SEGD6
        7       READ_MAGIC

    */

	offs_t sega = get_segment_address(offset);
	UINT8 segd = m_segment_ram[sega];

	return (READ_MAGIC << 7) | (segd & 0x7f);
}


//-------------------------------------------------
//  segment_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::segment_w )
{
	/*

        bit     description

        0       SEGD0
        1       SEGD1
        2       SEGD2
        3       SEGD3
        4       SEGD4
        5       SEGD5
        6       SEGD6
        7

    */

	offs_t sega = get_segment_address(offset);

	m_segment_ram[sega] = data & 0x7f;

	if (LOG) logerror("%s: %06x Task %u Segment %03x : %02x\n", machine().describe_context(), offset, get_current_task(offset), sega, data);
}


//-------------------------------------------------
//  page_r -
//-------------------------------------------------

READ8_MEMBER( abc1600_state::page_r )
{
	/*

        bit     description

        0       X11
        1       X12
        2       X13
        3       X14
        4       X15
        5       X16
        6       X17
        7       X18

        8       X19
        9       X20
        10
        11
        12
        13
        14      _WP
        15      NONX

    */

	// segment
	offs_t sega = get_segment_address(offset);
	UINT8 segd = m_segment_ram[sega];

	// page
	offs_t pga = get_page_address(offset, segd);
	UINT16 data = m_page_ram[pga];

	return A0 ? (data & 0xff) : (data >> 8);
}


//-------------------------------------------------
//  page_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::page_w )
{
	/*

        bit     description

        0       X11
        1       X12
        2       X13
        3       X14
        4       X15
        5       X16
        6       X17
        7       X18

        8       X19
        9       X20
        10
        11
        12
        13
        14      _WP
        15      NONX

    */

	// segment
	offs_t sega = get_segment_address(offset);
	UINT8 segd = m_segment_ram[sega];

	// page
	offs_t pga = get_page_address(offset, segd);

	if (A0)
	{
		m_page_ram[pga] = (m_page_ram[pga] & 0xff00) | data;
	}
	else
	{
		m_page_ram[pga] = (data << 8) | (m_page_ram[pga] & 0xff);
	}

	if (LOG) logerror("%s: %06x Task %u Segment %03x Page %03x : %02x -> %04x\n", machine().describe_context(), offset, get_current_task(offset), sega, pga, data, m_page_ram[pga]);
}



//**************************************************************************
//  DMA
//**************************************************************************

//-------------------------------------------------
//  update_drdy0 -
//-------------------------------------------------

inline void abc1600_state::update_drdy0()
{
	if (m_sysfs)
	{
		// floppy
		m_dma0->rdy_w(!wd17xx_drq_r(m_fdc));
	}
	else
	{
		// BUS0I/BUS0X
		m_dma0->rdy_w(1);
	}
}


//-------------------------------------------------
//  update_drdy1 -
//-------------------------------------------------

inline void abc1600_state::update_drdy1()
{
	if (m_sysscc)
	{
		// SCC
		m_dma1->rdy_w(1);
	}
	else
	{
		// BUS1
		m_dma1->rdy_w(1);
	}
}


//-------------------------------------------------
//  update_drdy2 -
//-------------------------------------------------

inline void abc1600_state::update_drdy2()
{
	// Winchester
	m_dma2->rdy_w(1);
}


//-------------------------------------------------
//  get_dma_address -
//-------------------------------------------------

inline offs_t abc1600_state::get_dma_address(int index, UINT16 offset)
{
	// A0 = DMA15, A1 = BA1, A2 = BA2
	UINT8 dmamap_addr = index | BIT(offset, 15);
	UINT8 dmamap = m_dmamap[dmamap_addr];

	m_cause = (dmamap & 0x1f) << 3;

	return ((dmamap & 0x1f) << 16) | offset;
}


//-------------------------------------------------
//  dma_mreq_r - DMA memory read
//-------------------------------------------------

inline UINT8 abc1600_state::dma_mreq_r(int index, UINT16 offset)
{
	offs_t virtual_offset = get_dma_address(index, offset);

	return read_ram(virtual_offset);
}


//-------------------------------------------------
//  dma_mreq_w - DMA memory write
//-------------------------------------------------

inline void abc1600_state::dma_mreq_w(int index, UINT16 offset, UINT8 data)
{
	offs_t virtual_offset = get_dma_address(index, offset);

	write_ram(virtual_offset, data);
}


//-------------------------------------------------
//  dma_iorq_r - DMA I/O read
//-------------------------------------------------

inline UINT8 abc1600_state::dma_iorq_r(int index, UINT16 offset)
{
	offs_t virtual_offset = get_dma_address(index, offset);

	return read_io(virtual_offset);
}


//-------------------------------------------------
//  dma_iorq_w - DMA I/O write
//-------------------------------------------------

inline void abc1600_state::dma_iorq_w(int index, UINT16 offset, UINT8 data)
{
	offs_t virtual_offset = get_dma_address(index, offset);

	write_io(virtual_offset, data);
}


//-------------------------------------------------
//  dmamap_w - DMA map write
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::dmamap_w )
{
	/*

        bit     description

        0       X16
        1       X17
        2       X18
        3       X19
        4       X20
        5
        6
        7       _R/W

    */

	if (LOG) logerror("DMAMAP %u %02x\n", offset & 7, data);

	m_dmamap[offset & 7] = data;
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

        bit     description

        0       SEL1
        1       SEL2
        2       SEL3
        3       MOTOR
        4       LC/PC
        5       LC/PC
        6
        7

    */

	if (LOG) logerror("FW0 %02x\n", data);

	// drive select
	if (BIT(data, 0)) wd17xx_set_drive(m_fdc, 0);
	if (BIT(data, 1)) wd17xx_set_drive(m_fdc, 1);
	if (BIT(data, 2)) wd17xx_set_drive(m_fdc, 2);

	// floppy motor
	floppy_mon_w(m_floppy, !BIT(data, 3));
}


//-------------------------------------------------
//  fw1_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::fw1_w )
{
	/*

        bit     description

        0       MR
        1       DDEN
        2       HLT
        3       MINI
        4       HLD
        5       P0
        6       P1
        7       P2

    */

	if (LOG) logerror("FW1 %02x\n", data);

	// FDC master reset
	wd17xx_mr_w(m_fdc, BIT(data, 0));

	// density select
	wd17xx_dden_w(m_fdc, BIT(data, 1));
}


//-------------------------------------------------
//  spec_contr_reg_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::spec_contr_reg_w )
{
	int state = BIT(data, 3);

	switch (data & 0x07)
	{
	case 0: // CS7
		m_cs7 = state;
		break;

	case 1:
		break;

	case 2: // _BTCE
		m_btce = state;
		break;

	case 3: // _ATCE
		m_atce = state;
		break;

	case 4: // PARTST
		m_partst = state;
		break;

	case 5: // _DMADIS
		m_dmadis = state;
		break;

	case 6: // SYSSCC
		m_sysscc = state;
		update_drdy1();
		break;

	case 7: // SYSFS
		m_sysfs = state;
		update_drdy0();
		break;
	}
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( abc1600_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc1600_mem, AS_PROGRAM, 8, abc1600_state )
	AM_RANGE(0x00000, 0xfffff) AM_READWRITE(mac_r, mac_w)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( abc1600 )
//-------------------------------------------------

static INPUT_PORTS_START( abc1600 )
	// inputs defined in machine/abc99.c
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  Z80DMA_INTERFACE( dma0_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( abc1600_state::dbrq_w )
{
	m_maincpu->set_input_line(INPUT_LINE_HALT, state & m_dmadis);
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

        bit     description

        PA0     BUS2
        PA1     BUS1
        PA2     BUS0X*2
        PA3     BUS0X*3
        PA4     BUS0X*4
        PA5     BUS0X*5
        PA6     BUS0X
        PA7     BUS0I

    */

	return 0;
}

READ8_MEMBER( abc1600_state::cio_pb_r )
{
	/*

        bit     description

        PB0
        PB1     POWERFAIL
        PB2
        PB3
        PB4     MINT
        PB5     _PREN-1
        PB6     _PREN-0
        PB7     FINT

    */

	UINT8 data = 0;

	// floppy interrupt
	data |= wd17xx_intrq_r(m_fdc) << 7;

	return data;
}

WRITE8_MEMBER( abc1600_state::cio_pb_w )
{
	/*

        bit     description

        PB0     PRBR
        PB1
        PB2
        PB3
        PB4
        PB5
        PB6
        PB7

    */

	// printer baudrate
	int prbr = BIT(data, 0);

	z80dart_txca_w(m_dart, prbr);
	z80dart_rxca_w(m_dart, prbr);
}

READ8_MEMBER( abc1600_state::cio_pc_r )
{
	/*

        bit     description

        PC0     1
        PC1     DATA IN
        PC2     1
        PC3     1

    */

	UINT8 data = 0x0d;

	// data in
	data |= (m_rtc->dio_r() | m_nvram->do_r()) << 1;

	return data;
}

WRITE8_MEMBER( abc1600_state::cio_pc_w )
{
	/*

        bit     description

        PC0     CLOCK
        PC1     DATA OUT
        PC2     RTC CS
        PC3     NVRAM CS

    */

	int clock = BIT(data, 0);
	int data_out = BIT(data, 1);
	int rtc_cs = BIT(data, 2);
	int nvram_cs = BIT(data, 3);

	if (LOG) logerror("CLK %u DATA %u RTC %u NVRAM %u\n", clock, data_out, rtc_cs, nvram_cs);

	m_rtc->cs_w(rtc_cs);
	m_rtc->dio_w(data_out);
	m_rtc->clk_w(clock);

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

static LEGACY_FLOPPY_OPTIONS_START( abc1600 )
	LEGACY_FLOPPY_OPTION(abc1600, "dsk", "Luxor ABC 1600", basicdsk_identify_default, basicdsk_construct_default, NULL,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
LEGACY_FLOPPY_OPTIONS_END

static const floppy_interface abc1600_floppy_interface =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_5_25_DSQD,
    LEGACY_FLOPPY_OPTIONS_NAME(abc1600),
    "floppy_5_25",
	NULL
};

WRITE_LINE_MEMBER( abc1600_state::drq_w )
{
	update_drdy0();
}

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(Z8536B1_TAG, z8536_device, pb7_w),
	DEVCB_DRIVER_LINE_MEMBER(abc1600_state, drq_w),
	{ FLOPPY_0, NULL, NULL, NULL } // TODO should be { NULL, NULL, FLOPPY_2, NULL }
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
//  IRQ_CALLBACK( abc1600_int_ack )
//-------------------------------------------------

static IRQ_CALLBACK( abc1600_int_ack )
{
	abc1600_state *state = device->machine().driver_data<abc1600_state>();
	UINT8 data = 0;

	switch (irqline)
	{
	case M68K_IRQ_2:
		data = state->m_cio->intack_r();
		break;
	}

	return data;
}


//-------------------------------------------------
//  MACHINE_START( abc1600 )
//-------------------------------------------------

void abc1600_state::machine_start()
{
	// interrupt callback
	device_set_irq_callback(m_maincpu, abc1600_int_ack);

	// fill segment RAM with non-zero values or no boot
	memset(m_segment_ram, 0xcd, 0x400);

	// state saving
	save_item(NAME(m_ifc2));
	save_item(NAME(m_task));
	save_item(NAME(m_segment_ram));
	save_item(NAME(m_page_ram));
	save_item(NAME(m_dmamap));
	save_item(NAME(m_dmadis));
	save_item(NAME(m_sysscc));
	save_item(NAME(m_sysfs));
	save_item(NAME(m_cause));
	save_item(NAME(m_partst));
	save_item(NAME(m_cs7));
	save_item(NAME(m_atce));
	save_item(NAME(m_btce));
}


//-------------------------------------------------
//  MACHINE_RESET( abc1600 )
//-------------------------------------------------

void abc1600_state::machine_reset()
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);

	// clear special control register
	for (int i = 0; i < 8; i++)
	{
		spec_contr_reg_w(*program, 0, i);
	}

	// clear floppy registers
	fw0_w(*program, 0, 0);
	fw1_w(*program, 0, 0);

	// clear task register
	m_task = 0;

	// disable display
	m_clocks_disabled = 1;
	m_endisp = 0;
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
	MCFG_WATCHDOG_TIME_INIT(attotime::from_msec(1600)) // XTAL_64MHz/8/10/20000/8/8

	// video hardware
	MCFG_FRAGMENT_ADD(abc1600_video)

	// devices
	MCFG_Z80DMA_ADD(Z8410AB1_0_TAG, XTAL_64MHz/16, dma0_intf)
	MCFG_Z80DMA_ADD(Z8410AB1_1_TAG, XTAL_64MHz/16, dma1_intf)
	MCFG_Z80DMA_ADD(Z8410AB1_2_TAG, XTAL_64MHz/16, dma2_intf)
	MCFG_Z80DART_ADD(Z8470AB1_TAG, XTAL_64MHz/16, dart_intf)
	MCFG_SCC8530_ADD(Z8530B1_TAG, XTAL_64MHz/16)
	MCFG_Z8536_ADD(Z8536B1_TAG, XTAL_64MHz/16, cio_intf)
	MCFG_NMC9306_ADD(NMC9306_TAG)
	MCFG_E0516_ADD(E050_C16PC_TAG, XTAL_32_768kHz)
	MCFG_FD1797_ADD(SAB1797_02P_TAG, fdc_intf)
	MCFG_LEGACY_FLOPPY_DRIVE_ADD(FLOPPY_0, abc1600_floppy_interface)
	MCFG_HARDDISK_ADD("harddisk0")
	MCFG_ABC99_ADD(abc99_intf)
	MCFG_S1410_ADD()

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1M")

	// software list
	MCFG_SOFTWARE_LIST_ADD("flop_list", "abc1600")
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
	ROM_LOAD( "drmskh 6490358-01.1l", 0x20, 0x20, CRC(a4a9a9dc) SHA1(d8575c0335d6021cbb5f7bcd298b41c35294a80a) )

	ROM_REGION( 0x71c, "plds", 0 )
	ROM_LOAD( "drmsk 6490360-01.1m", 0x000, 0x104, CRC(5f7143c1) SHA1(1129917845f8e505998b15288f02bf907487e4ac) ) // mover word mixer @ 1m,1n,1t,2t
	ROM_LOAD( "1020 6490349-01.8b",  0x104, 0x104, CRC(1fa065eb) SHA1(20a95940e39fa98e97e59ea1e548ac2e0c9a3444) ) // expansion bus strobes
	ROM_LOAD( "1021 6490350-01.5d",  0x208, 0x104, CRC(96f6f44b) SHA1(12d1cd153dcc99d1c4a6c834122f370d49723674) ) // interrupt encoder and ROM/RAM control
	ROM_LOAD( "1022 6490351-01.17e", 0x30c, 0x104, CRC(5dd00d43) SHA1(a3871f0d796bea9df8f25d41b3169dd4b8ef65ab) ) // MAC register address decoder
	ROM_LOAD( "1023 6490352-01.11e", 0x410, 0x104, CRC(a2f350ac) SHA1(77e08654a197080fa2111bc3031cd2c7699bf82b) ) // interrupt acknowledge
	ROM_LOAD( "1024 6490353-01.12e", 0x514, 0x104, CRC(67f1328a) SHA1(b585495fe14a7ae2fbb29f722dca106d59325002) ) // expansion bus timing and control
	ROM_LOAD( "1025 6490354-01.6e",  0x618, 0x104, CRC(9bda0468) SHA1(ad373995dcc18532274efad76fa80bd13c23df25) ) // DMA transfer
	//ROM_LOAD( "pal16r4.10c", 0x71c, 0x104, NO_DUMP ) // SCC read/write, mentioned in the preliminary service manual, but not present on the PCB
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     INIT  COMPANY     FULLNAME     FLAGS
COMP( 1985, abc1600, 0,      0,      abc1600, abc1600, 0,    "Luxor", "ABC 1600", GAME_NOT_WORKING )
