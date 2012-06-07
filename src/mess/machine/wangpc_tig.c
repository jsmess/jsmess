/**********************************************************************

    Wang PC Text/Image/Graphics controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*
	
	TODO:

	- all

*/

#include "wangpc_tig.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define LOG 1

#define OPTION_ID_0			0x13
#define OPTION_ID_1			0x17

#define UPD7720_0_TAG		"upd7220_0"
#define UPD7720_1_TAG		"upd7220_1"
#define SCREEN_TAG			"screen"

#define DMA_GRAPHICS		BIT(m_option, 0)
#define DMA_DREQ1			BIT(m_option, 1)
#define DMA_DREQ2			BIT(m_option, 2)
#define DMA_DREQ3			BIT(m_option, 3)
#define DMA_ID				BIT(m_option, 4)



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type WANGPC_TIG = &device_creator<wangpc_tig_device>;


//-------------------------------------------------
//  UPD7220_INTERFACE( hgdc0_intf )
//-------------------------------------------------

static ADDRESS_MAP_START( upd7220_0_map, AS_0, 8, wangpc_tig_device )
	AM_RANGE(0x0000, 0x0fff) AM_MIRROR(0x1000) AM_RAM // frame buffer
	AM_RANGE(0x4000, 0x7fff) AM_RAM // font memory
ADDRESS_MAP_END

static UPD7220_DRAW_TEXT_LINE( hgdc_display_text)
{
}

static UPD7220_INTERFACE( hgdc0_intf )
{
	SCREEN_TAG,
	NULL,
	hgdc_display_text,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  UPD7220_INTERFACE( hgdc1_intf )
//-------------------------------------------------

static ADDRESS_MAP_START( upd7220_1_map, AS_0, 8, wangpc_tig_device )
	AM_RANGE(0x0000, 0xffff) AM_RAM // graphics memory
ADDRESS_MAP_END

static UPD7220_DISPLAY_PIXELS( hgdc_display_pixels )
{
}

static UPD7220_INTERFACE( hgdc1_intf )
{
	SCREEN_TAG,
	hgdc_display_pixels,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( wangpc_tig )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( wangpc_tig )
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_UPDATE_DEVICE(DEVICE_SELF, wangpc_tig_device, screen_update)
	MCFG_SCREEN_SIZE(80*10, 25*12)
	MCFG_SCREEN_VISIBLE_AREA(0, 80*10-1, 0, 25*12-1)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_REFRESH_RATE(60)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)

	MCFG_UPD7220_ADD(UPD7720_0_TAG, 4000000, hgdc0_intf, upd7220_0_map)
	MCFG_UPD7220_ADD(UPD7720_1_TAG, 4000000, hgdc1_intf, upd7220_1_map)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor wangpc_tig_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( wangpc_tig );
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  wangpc_tig_device - constructor
//-------------------------------------------------

wangpc_tig_device::wangpc_tig_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, WANGPC_TIG, "Wang PC TIG Controller", tag, owner, clock),
	device_wangpcbus_card_interface(mconfig, *this),
	m_hgdc0(*this, UPD7720_0_TAG),
	m_hgdc1(*this, UPD7720_1_TAG),
	m_option(0)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void wangpc_tig_device::device_start()
{
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void wangpc_tig_device::device_reset()
{
	m_option = 0;
}


//-------------------------------------------------
//  screen_update -
//-------------------------------------------------

UINT32 wangpc_tig_device::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_hgdc0->screen_update(screen, bitmap, cliprect);
	m_hgdc1->screen_update(screen, bitmap, cliprect);

	return 0;
}


//-------------------------------------------------
//  wangpcbus_iorc_r - I/O read
//-------------------------------------------------

UINT16 wangpc_tig_device::wangpcbus_iorc_r(address_space &space, offs_t offset, UINT16 mem_mask)
{
	UINT16 data = 0xffff;

	if (sad(offset))
	{
		switch (offset & 0x7f)
		{
		case 0x20/2:
		case 0x22/2:
			data = m_hgdc0->read(space, offset);
			break;

		case 0x24/2:
		case 0x26/2:
			data = m_hgdc1->read(space, offset);
			break;

		case 0xfe/2:
			data = 0xff00 | DMA_ID ? OPTION_ID_1 : OPTION_ID_0;
			break;
		}
	}

	return data;
}


//-------------------------------------------------
//  wangpcbus_aiowc_w - I/O write
//-------------------------------------------------

void wangpc_tig_device::wangpcbus_aiowc_w(address_space &space, offs_t offset, UINT16 mem_mask, UINT16 data)
{
	if (sad(offset) && ACCESSING_BITS_0_7)
	{
		switch (offset & 0x7f)
		{
		case 0x00/2: case 0x02/2: case 0x04/2: case 0x06/2: case 0x08/2: case 0x0a/2: case 0x0c/2: case 0x0e/2:
		case 0x10/2: case 0x12/2: case 0x14/2: case 0x16/2: case 0x18/2: case 0x1a/2: case 0x1c/2: case 0x1e/2:
			if (LOG) logerror("TIG attribute %u: %02x\n", offset, data & 0xff);

			m_attr[offset] = data & 0xff;
			break;

		case 0x20/2:
		case 0x22/2:
			m_hgdc0->write(space, offset, data);
			break;

		case 0x24/2:
		case 0x26/2:
			m_hgdc1->write(space, offset, data);
			break;

		case 0x28/2:
			if (LOG) logerror("TIG underline %02x\n", data & 0xff);

			m_underline = data & 0xff;
			break;

		case 0x2a/2:
			if (LOG) logerror("TIG option %02x\n", data & 0xff);
			
			m_option = data & 0xff;
			break;

		case 0xfc/2:
			device_reset();
			break;
		}
	}
}


//-------------------------------------------------
//  wangpcbus_dack_r - DMA read
//-------------------------------------------------

UINT8 wangpc_tig_device::wangpcbus_dack_r(address_space &space, int line)
{
	UINT8 data = 0;

	if (DMA_GRAPHICS)
	{
		data = m_hgdc1->dack_r(space, 0);
	}
	else
	{
		data = m_hgdc0->dack_r(space, 0);
	}

	return data;
}


//-------------------------------------------------
//  wangpcbus_dack_w - DMA write
//-------------------------------------------------

void wangpc_tig_device::wangpcbus_dack_w(address_space &space, int line, UINT8 data)
{
	if (DMA_GRAPHICS)
	{
		m_hgdc1->dack_w(space, 0, data);
	}
	else
	{
		m_hgdc0->dack_w(space, 0, data);
	}
}


//-------------------------------------------------
//  wangpcbus_have_dack - DMA acknowledge
//-------------------------------------------------

bool wangpc_tig_device::wangpcbus_have_dack(int line)
{
	return (line == 1 && DMA_DREQ1) || (line == 2 && DMA_DREQ2) || (line == 3 && DMA_DREQ3);
}
