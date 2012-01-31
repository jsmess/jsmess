/**********************************************************************

    Wang PC Low-Resolution Video Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "wangpc_lores.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define OPTION_ID		0x10

#define MC6845_TAG		"mc6845"
#define SCREEN_TAG		"screen"

#define RAM_SIZE		0x8000



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type WANGPC_LORES_VIDEO = &device_creator<wangpc_lores_video_device>;


//-------------------------------------------------
//  mc6845_interface crtc_intf
//-------------------------------------------------

void wangpc_lores_video_device::crtc_update_row(mc6845_device *device, bitmap_rgb32 &bitmap, const rectangle &cliprect, UINT16 ma, UINT8 ra, UINT16 y, UINT8 x_count, INT8 cursor_x, void *param)
{
/*  const rgb_t *palette = palette_entry_list_raw(bitmap.palette());
    for (int column = 0; column < x_count; column++)
    {
        UINT8 code = m_video_ram[((ma + column) & 0x7ff)];
        UINT16 addr = (code << 3) | (ra & 0x07);
        UINT8 data = m_char_rom[addr & 0x7ff];

        if (column == cursor_x)
        {
            data = 0xff;
        }

        for (int bit = 0; bit < 8; bit++)
        {
            int x = (column * 8) + bit;
            int color = BIT(data, 7) ? 7 : 0;

            bitmap.pix32(y, x) = palette[color];

            data <<= 1;
        }
    }*/
}

static MC6845_UPDATE_ROW( wangpc_lores_video_update_row )
{
	wangpc_lores_video_device *lores = downcast<wangpc_lores_video_device *>(device->owner());
	lores->crtc_update_row(device,bitmap,cliprect,ma,ra,y,x_count,cursor_x,param);
}

WRITE_LINE_MEMBER( wangpc_lores_video_device::vsync_w )
{
	if (BIT(m_option, 3) && state)
	{
		m_irq3 = ASSERT_LINE;
		m_bus->irq3_w(m_irq3);
	}
}

static const mc6845_interface crtc_intf =
{
	SCREEN_TAG,
	8,
	NULL,
	wangpc_lores_video_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, wangpc_lores_video_device, vsync_w),
	NULL
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( wangpc_lores_video )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( wangpc_lores_video )
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_UPDATE_DEVICE(MC6845_TAG, mc6845_device, screen_update)
	MCFG_SCREEN_SIZE(80*8, 25*9)
	MCFG_SCREEN_VISIBLE_AREA(0, 80*8-1, 0, 25*9-1)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_REFRESH_RATE(60)

	MCFG_MC6845_ADD(MC6845_TAG, MC6845, XTAL_14_31818MHz, crtc_intf)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor wangpc_lores_video_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( wangpc_lores_video );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  wangpc_lores_video_device - constructor
//-------------------------------------------------

wangpc_lores_video_device::wangpc_lores_video_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, WANGPC_LORES_VIDEO, "Wang PC Low Resolution Video Card", tag, owner, clock),
	device_wangpcbus_card_interface(mconfig, *this),
	m_crtc(*this, MC6845_TAG),
	m_option(0),
	m_irq3(CLEAR_LINE)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void wangpc_lores_video_device::device_start()
{
	m_slot = dynamic_cast<wangpcbus_slot_device *>(owner());

	m_video_ram = auto_alloc_array(machine(), UINT16, RAM_SIZE);

	// state saving
	save_pointer(NAME(m_video_ram), RAM_SIZE);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void wangpc_lores_video_device::device_reset()
{
}


//-------------------------------------------------
//  wangpcbus_mrdc_r - memory read
//-------------------------------------------------

UINT16 wangpc_lores_video_device::wangpcbus_mrdc_r(address_space &space, offs_t offset, UINT16 mem_mask)
{
	UINT16 data = 0xffff;

	if (BIT(m_option, 0) && (offset >= 0xe0000/2) && (offset < 0xf0000/2))
	{
		data = m_video_ram[offset];
	}

	return data;
}


//-------------------------------------------------
//  wangpcbus_amwc_w - memory write
//-------------------------------------------------

void wangpc_lores_video_device::wangpcbus_amwc_w(address_space &space, offs_t offset, UINT16 mem_mask, UINT16 data)
{
	if (BIT(m_option, 0) && (offset >= 0xe0000/2) && (offset < 0xf0000/2))
	{
		m_video_ram[offset] = data;
	}
}


//-------------------------------------------------
//  wangpcbus_iorc_r - I/O read
//-------------------------------------------------

UINT16 wangpc_lores_video_device::wangpcbus_iorc_r(address_space &space, offs_t offset, UINT16 mem_mask)
{
	UINT16 data = 0xffff;

	if (sad(offset))
	{
		logerror("Lores read %04x %04x\n", offset << 1, mem_mask);

		switch (offset & 0x7f)
		{
		case 0x02/2:
			data = 0xff00 | m_crtc->register_r(space, 0);
			break;

		case 0x30/2:
			data = 0xffe3;
			data |= m_crtc->de_r() << 2;
			data |= m_crtc->vsync_r() << 3;
			data |= m_crtc->hsync_r() << 4;
			break;

		case 0xfe/2:
			data = 0xff00 | (m_irq3 << 7) | OPTION_ID;
			break;
		}
	}

	return data;
}


//-------------------------------------------------
//  wangpcbus_aiowc_w - I/O write
//-------------------------------------------------

void wangpc_lores_video_device::wangpcbus_aiowc_w(address_space &space, offs_t offset, UINT16 data, UINT16 mem_mask)
{
	if (sad(offset))
	{
		logerror("Lores write %04x %04x\n", offset << 1, mem_mask);

		switch (offset & 0x7f)
		{
		case 0x00/2:
			if (ACCESSING_BITS_0_7)
				m_crtc->address_w(space, 0, data & 0xff);
			break;

		case 0x02/2:
			if (ACCESSING_BITS_0_7)
				m_crtc->register_w(space, 0, data & 0xff);
			break;

		case 0x10/2:
			if (ACCESSING_BITS_0_7)
				m_option = data & 0xff;
			break;

		case 0x20/2:
			m_scroll = data;
			break;

		case 0x70/2:
			m_irq3 = CLEAR_LINE;
			m_bus->irq3_w(m_irq3);
			break;
		}
	}
}
