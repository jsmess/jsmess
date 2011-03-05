#include "includes/abc1600.h"



//**************************************************************************
//  CONSTANTS / MACROS
//**************************************************************************

#define VIDEORAM_SIZE	128*1024

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
		break;
		
	case LDSX_LB:
		m_sx = (m_sx & 0xff00) | data;
		break;
		
	case LDSY_HB:
		m_sy = (data << 8) | (m_sy & 0xff);
		break;
		
	case LDSY_LB:
		m_sy = (m_sy & 0xff00) | data;
		break;
		
	case LDTX_HB:
		m_tx = (data << 8) | (m_tx & 0xff);
		break;
		
	case LDTX_LB:
		m_tx = (m_tx & 0xff00) | data;
		break;

	case LDTY_HB:
		m_ty = (data << 8) | (m_ty & 0xff);
		break;

	case LDTY_LB:
		m_ty = (m_ty & 0xff00) | data;
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
		break;
		
	case LDFX_LB:
		m_fx = (m_fx & 0xff00) | data;
		break;
		
	case LDFY_HB:
		m_fy = (data << 8) | (m_fy & 0xff);
		break;
		
	case LDFY_LB:
		m_fy = (m_fy & 0xff00) | data;
		break;
		
	case WRML:
		/*

			bit		description

			0		MOVE CYK CLK
			1		DISP CYK PRE FETCH / DISP CYC SEL
			2		DATA CLK
			3		_DISP MEM WE
			4		_CAS HB
			5		DTACK CLK
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
			1		DISP CYK PRE FETCH / DISP CYC SEL
			2		DATA CLK
			3		_DISP MEM WE
			4		_CAS HB
			5		DTACK CLK
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
	switch (data & 0x07)
	{
	case WRMASK_STROBE_HB:
		if (REPLACE)
		{
			// DATAPORT_HB
		}
		else
		{
			// WRPORT_HB
			m_wrm = (data << 8) | (m_wrm & 0xff);
		}
		break;
		
	case WRMASK_STROBE_LB:
		if (REPLACE)
		{
			// DATAPORT_LB
		}
		else
		{
			// WRPORT_LB
			m_wrm = (m_wrm & 0xff00) | data;
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

static MC6845_UPDATE_ROW( abc1600_update_row )
{
	// MA13-MA8, RA3-RA0, MA4-MA1, 0, 0
	//UINT16 dma = ((ma << 2) & 0xfc00) | (ra & 0x0f) << 6 | ((ma << 1) & 0x1c);
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
	8,
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

	// state saving
	save_pointer(NAME(m_video_ram), VIDEORAM_SIZE);
	save_item(NAME(m_endisp));
	save_item(NAME(m_clocks_disabled));
	save_item(NAME(m_vs));
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
	mc6845_update(m_crtc, &bitmap, &cliprect);
	
	for (int y = 0; y < 1024; y++)
	{
		UINT32 addr = y * 64;

		for (int sx = 0; sx < 48; sx++)
		{
			UINT16 data = m_video_ram[addr] << 8 | m_video_ram[addr + 1];

			for (int x = 0; x < 16; x++)
			{
				*BITMAP_ADDR16(&bitmap, y, (sx * 16) + x) = BIT(data, x);
			}

			addr += 2;
		}
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
    MCFG_PALETTE_INIT(black_and_white)
	MCFG_MC6845_ADD(SY6845E_TAG, SY6845E, XTAL_64MHz/32, crtc_intf)
MACHINE_CONFIG_END
