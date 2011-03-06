/*

    TODO:

	- position image based on vsync/hsync
	- FRAME POL
	- landscape/portrait mode
	- mover

*/

#include "includes/abc1600.h"



//**************************************************************************
//  CONSTANTS / MACROS
//**************************************************************************

#define VIDEORAM_SIZE	512*1024

// flag register
#define L_P			BIT(m_flag, 0)
#define BLANK		BIT(m_flag, 1)
#define PIX_POL		BIT(m_flag, 2)
#define FRAME_POL	BIT(m_flag, 3)
#define HOLD_FY		BIT(m_flag, 4)
#define HOLD_FX		BIT(m_flag, 5)
#define COMP_MOVE	BIT(m_flag, 6)
#define REPLACE		BIT(m_flag, 7)


// IOWR0 registers
enum
{
	LDSX_HB = 0,
	LDSX_LB,
	LDSY_HB,
	LDSY_LB,
	LDTX_HB,
	LDTX_LB,
	LDTY_HB,
	LDTY_LB
};


// IOWR1 registers
enum
{
	LDFX_HB = 0,
	LDFX_LB,
	LDFY_HB,
	LDFY_LB,
	WRML = 5,
	WRDL = 7
};


// IOWR2 registers
enum
{
	WRMASK_STROBE_HB = 0,
	WRMASK_STROBE_LB,
	ENABLE_CLOCKS,
	FLAG_STROBE,
	ENDISP
};



//**************************************************************************
//  READ/WRITE HANDLERS
//**************************************************************************

//-------------------------------------------------
//  video_ram_r -
//-------------------------------------------------

READ8_MEMBER( abc1600_state::video_ram_r )
{
	UINT32 addr = offset & 0x7ffff;
	UINT8 data = m_video_ram[addr];

	return data;
}


//-------------------------------------------------
//  video_ram_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::video_ram_w )
{
	UINT32 addr = offset & 0x7ffff;

	if (offset & 0x01)
	{
		if (REPLACE)
		{
			// WRPORT_LB
			m_wrm = (m_wrm & 0xff00) | data;
			logerror("WRM LB %04x\n", m_wrm);
		}
		else
		{
			// DATAPORT_LB
			m_gmdi = (m_gmdi & 0xff00) | data;
			logerror("GMDI LB %04x\n", m_gmdi);
		}

		UINT8 gmdo = m_video_ram[addr];
		UINT8 gmdi = m_gmdi & 0xff;
		UINT8 mask = 0xff;//m_wrm & 0xff;

		m_video_ram[addr] = (gmdi & mask) | (gmdo & (mask ^ 0xff));

		logerror("Video RAM write LB to %05x : %02x\n", addr, m_video_ram[addr]);
	}
	else
	{
		if (REPLACE)
		{
			// WRPORT_HB
			m_wrm = (data << 8) | (m_wrm & 0xff);
			logerror("WRM HB %04x\n", m_wrm);
		}
		else
		{
			// DATAPORT_HB
			m_gmdi = (data << 8) | (m_gmdi & 0xff);
			logerror("GMDI HB %04x\n", m_gmdi);
		}

		UINT8 gmdo = m_video_ram[addr];
		UINT8 gmdi = m_gmdi >> 8;
		UINT8 mask = 0xff;//m_wrm >> 8;

		m_video_ram[addr] = (gmdi & mask) | (gmdo & (mask ^ 0xff));

		logerror("Video RAM write HB to %05x : %02x\n", addr, m_video_ram[addr]);
	}
}


//-------------------------------------------------
//  iord0_r -
//-------------------------------------------------

READ8_MEMBER( abc1600_state::iord0_r )
{
	/*

		bit		description

		0		0
		1		SCREENPOS
		2		
		3		
		4		
		5		
		6		VSYNC
		7		BUSY

	*/

	UINT8 data = 0;

	// vertical sync
	data |= m_vs << 6;

	return data;
}


//-------------------------------------------------
//  iowr0_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::iowr0_w )
{
	switch (offset & 0x07)
	{
	case LDSX_HB:
		m_sx = (data << 8) | (m_sx & 0xff);
		logerror("SX HB %04x\n", m_sx);
		break;
		
	case LDSX_LB:
		m_sx = (m_sx & 0xff00) | data;
		logerror("SX LB %04x\n", m_sx);
		break;
		
	case LDSY_HB:
		m_sy = (data << 8) | (m_sy & 0xff);
		logerror("SY HB %04x\n", m_sy);
		break;
		
	case LDSY_LB:
		m_sy = (m_sy & 0xff00) | data;
		logerror("SY LB %04x\n", m_sy);
		break;
		
	case LDTX_HB:
		m_tx = (data << 8) | (m_tx & 0xff);
		logerror("TX HB %04x\n", m_tx);
		break;
		
	case LDTX_LB:
		m_tx = (m_tx & 0xff00) | data;
		logerror("TX LB %04x\n", m_tx);
		break;

	case LDTY_HB:
		m_ty = (data << 8) | (m_ty & 0xff);
		logerror("TY HB %04x\n", m_ty);
		break;

	case LDTY_LB:
		m_ty = (m_ty & 0xff00) | data;
		logerror("TY LB %04x\n", m_ty);
		break;
	}
}


//-------------------------------------------------
//  iowr1_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::iowr1_w )
{
	switch (offset & 0x07)
	{
	case LDFX_HB:
		m_fx = (data << 8) | (m_fx & 0xff);
		logerror("FX HB %04x\n", m_fx);
		break;
		
	case LDFX_LB:
		m_fx = (m_fx & 0xff00) | data;
		logerror("FX LB %04x\n", m_fx);
		break;
		
	case LDFY_HB:
		m_fy = (data << 8) | (m_fy & 0xff);
		logerror("FY HB %04x\n", m_fy);
		break;
		
	case LDFY_LB:
		m_fy = (m_fy & 0xff00) | data;
		logerror("FY LB %04x\n", m_fy);
		break;
		
	case WRML:
		/*

			bit		description

			0		MOVE CYK CLK
			1		DISP CYC SEL / DISP CYK PRE FETCH (+1 PIXCLK)
			2		DATA CLK
			3		_DISP MEM WE
			4		_CAS HB
			5		DTACK CLK / BLANK TEST (+2 PIXCLK)
			6		DISPREC CLK
			7		_RAS HB

		*/

		logerror("MS %u : %02x\n", (offset >> 4) & 0x0f, data);

		if (!m_clocks_disabled)
		{
			m_ms[(offset >> 4) & 0x0f] = data;
		}
		break;
		
	case WRDL:
		/*

			bit		description

			0		MOVE CYK CLK
			1		DISP CYC SEL / DISP CYK PRE FETCH (+1 PIXCLK)
			2		DATA CLK
			3		_DISP MEM WE
			4		_CAS HB
			5		DTACK CLK / BLANK TEST (+2 PIXCLK)
			6		DISPREC CLK
			7		_RAS HB

		*/

		logerror("WS %u : %02x\n", (offset >> 4) & 0x0f, data);

		if (!m_clocks_disabled)
		{
			m_ds[(offset >> 4) & 0x0f] = data;
		}
		break;
	}
}


//-------------------------------------------------
//  iowr2_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::iowr2_w )
{
	switch (offset & 0x07)
	{
	case WRMASK_STROBE_HB:
		if (REPLACE)
		{
			// DATAPORT_HB
			m_gmdi = (data << 8) | (m_gmdi & 0xff);
			logerror("GMDI HB %04x\n", m_gmdi);
		}
		else
		{
			// WRPORT_HB
			m_wrm = (data << 8) | (m_wrm & 0xff);
			logerror("WRM HB %04x\n", m_gmdi);
		}
		break;
		
	case WRMASK_STROBE_LB:
		if (REPLACE)
		{
			// DATAPORT_LB
			m_gmdi = (m_gmdi & 0xff00) | data;
			logerror("GMDI LB %04x\n", m_gmdi);
		}
		else
		{
			// WRPORT_LB
			m_wrm = (m_wrm & 0xff00) | data;
			logerror("WRM LB %04x\n", m_gmdi);
		}
		break;

	case ENABLE_CLOCKS:
		logerror("ENABLE CLOCKS\n");
		m_clocks_disabled = 0;
		break;
		
	case FLAG_STROBE:
		/*
		
			bit		description
			
			0		L/_P FLAG
			1		BLANK FLAG
			2		PIX POL
			3		FRAME POL
			4		HOLD FY
			5		HOLD FX
			6		COMP MOVE FLAG
			7		REPLACE/SET & RESET
		
		*/

		m_flag = data;
		logerror("FLAG %02x\n", m_flag);
		break;
		
	case ENDISP:
		logerror("ENDISP\n");
		m_endisp = 1;
		break;
	}
}



//**************************************************************************
//  VIDEO
//**************************************************************************

//-------------------------------------------------
//  mc6845_interface crtc_intf
//-------------------------------------------------

inline UINT16 abc1600_state::get_crtca(UINT16 ma, UINT8 ra, UINT8 column)
{
	/*
		
		bit			description

		CRTCA0		0
		CRTCA1		0
		CRTCA2		CC1/MA1
		CRTCA3		CC2/MA2
		CRTCA4		CC3/MA3
		CRTCA5		CC4/MA4
		CRTCA6		RA0
		CRTCA7		RA1
		CRTCA8		RA2
		CRTCA9		RA3
		CRTCA10		CR0/MA8
		CRTCA11		CR1/MA9
		CRTCA12		CR2/MA10
		CRTCA13		CR3/MA11
		CRTCA14		CR4/MA12
		CRTCA15		CR5/MA13

	*/

	UINT8 cc = (ma & 0xff) + column;
	UINT8 cr = ma >> 8;

	return (cr << 10) | ((ra & 0x0f) << 6) | ((cc << 1) & 0x3c);
}

void abc1600_state::crtc_update_row(device_t *device, bitmap_t *bitmap, const rectangle *cliprect, UINT16 ma, UINT8 ra, UINT16 y, UINT8 x_count, INT8 cursor_x, void *param)
{
	int x = 0;

	for (int column = 0; column < x_count; column += 2)
	{
		UINT16 dma = get_crtca(ma, ra, column);

		// data is read out of video RAM in nibble mode by strobing CAS 4 times
		for (int cas = 0; cas < 4; cas++)
		{
			UINT8 data = m_video_ram[dma + cas];
			
			for (int bit = 0; bit < 8; bit++)
			{
				int color = (BIT(data, 7) ^ PIX_POL) & !BLANK;

				*BITMAP_ADDR16(bitmap, y, x++) = color;
			
				data <<= 1;
			}
		}
	}
}

static MC6845_UPDATE_ROW( abc1600_update_row )
{
	abc1600_state *state = device->machine->driver_data<abc1600_state>();
	state->crtc_update_row(device, bitmap, cliprect, ma, ra, y, x_count, cursor_x, param);
}

WRITE_LINE_MEMBER( abc1600_state::vs_w )
{
	m_vs = state;
}

static MC6845_ON_UPDATE_ADDR_CHANGED( crtc_update )
{
}

static const mc6845_interface crtc_intf =
{
	SCREEN_TAG,
	32,
	NULL,
	abc1600_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(abc1600_state, vs_w),
	crtc_update
};


//-------------------------------------------------
//  VIDEO_START( abc1600 )
//-------------------------------------------------

void abc1600_state::video_start()
{
	// allocate video RAM
	m_video_ram = auto_alloc_array(machine, UINT8, VIDEORAM_SIZE);
	memset(m_video_ram, 0, VIDEORAM_SIZE);

	// state saving
	save_pointer(NAME(m_video_ram), VIDEORAM_SIZE);
	save_item(NAME(m_endisp));
	save_item(NAME(m_clocks_disabled));
	save_item(NAME(m_vs));
	save_item(NAME(m_gmdi));
	save_item(NAME(m_wrm));
	save_item(NAME(m_ms));
	save_item(NAME(m_ds));
	save_item(NAME(m_flag));
	save_item(NAME(m_sx));
	save_item(NAME(m_sy));
	save_item(NAME(m_tx));
	save_item(NAME(m_ty));
	save_item(NAME(m_fx));
	save_item(NAME(m_fy));
}


//-------------------------------------------------
//  SCREEN_UPDATE( abc1600 )
//-------------------------------------------------

bool abc1600_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	if (m_endisp)
	{
		mc6845_update(m_crtc, &bitmap, &cliprect);
	}
	else
	{
		bitmap_fill(&bitmap, &cliprect, get_black_pen(machine));
	}

	return 0;
}


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( abc1600_video )
//-------------------------------------------------

MACHINE_CONFIG_FRAGMENT( abc1600_video )
    MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
    MCFG_SCREEN_REFRESH_RATE(60)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) // not accurate
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(768, 1024)
    MCFG_SCREEN_VISIBLE_AREA(0, 768-1, 0, 1024-1)
    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(monochrome_green)
	MCFG_MC6845_ADD(SY6845E_TAG, SY6845E, XTAL_64MHz/32, crtc_intf)
MACHINE_CONFIG_END
