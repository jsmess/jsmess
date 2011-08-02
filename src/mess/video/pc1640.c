#include "includes/pc1512.h"



//**************************************************************************
//  CONSTANTS
//**************************************************************************

#define LOG 0



//**************************************************************************
//  PALETTE
//**************************************************************************

//-------------------------------------------------
//  PALETTE_INIT( pc1640 )
//-------------------------------------------------

static PALETTE_INIT( pc1640 )
{
	palette_set_color_rgb(machine,  0, 0x00, 0x00, 0x00);
	palette_set_color_rgb(machine,  1, 0x00, 0x00, 0xaa);
	palette_set_color_rgb(machine,  2, 0x00, 0xaa, 0x00);
	palette_set_color_rgb(machine,  3, 0x00, 0xaa, 0xaa);
	palette_set_color_rgb(machine,  4, 0xaa, 0x00, 0x00);
	palette_set_color_rgb(machine,  5, 0xaa, 0x00, 0xaa);
	palette_set_color_rgb(machine,  6, 0xaa, 0x55, 0x00);
	palette_set_color_rgb(machine,  7, 0xaa, 0xaa, 0xaa);
	palette_set_color_rgb(machine,  8, 0x55, 0x55, 0x55);
	palette_set_color_rgb(machine,  9, 0x55, 0x55, 0xff);
	palette_set_color_rgb(machine, 10, 0x55, 0xff, 0x55);
	palette_set_color_rgb(machine, 11, 0x55, 0xff, 0xff);
	palette_set_color_rgb(machine, 12, 0xff, 0x55, 0x55);
	palette_set_color_rgb(machine, 13, 0xff, 0x55, 0xff);
	palette_set_color_rgb(machine, 14, 0xff, 0xff, 0x55);
	palette_set_color_rgb(machine, 15, 0xff, 0xff, 0xff);
}



//**************************************************************************
//  VIDEO RAM ACCESS
//**************************************************************************

//-------------------------------------------------
//  video_ram_r -
//-------------------------------------------------

READ8_MEMBER( pc1640_state::video_ram_r )
{
	return 0;
}


//-------------------------------------------------
//  video_ram_w -
//-------------------------------------------------

WRITE8_MEMBER( pc1640_state::video_ram_w )
{
}


//-------------------------------------------------
//  mc6845_interface crtc_intf
//-------------------------------------------------

static MC6845_UPDATE_ROW( pc1640_update_row )
{
}

static const mc6845_interface crtc_intf =
{
	SCREEN_TAG,
	8,
	NULL,
	pc1640_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};


//-------------------------------------------------
//  VIDEO_START( pc1640 )
//-------------------------------------------------

void pc1640_state::video_start()
{
	// allocate memory
	m_video_ram = auto_alloc_array(machine(), UINT8, 0x20000);
}


//-------------------------------------------------
//  SCREEN_UPDATE( pc1640 )
//-------------------------------------------------

bool pc1640_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	return false;
}


//-------------------------------------------------
//  MACHINE_CONFIG( pc1640 )
//-------------------------------------------------

MACHINE_CONFIG_FRAGMENT( pc1640_video )
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(80*8, 24*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 80*8-1, 0, 24*8-1)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_REFRESH_RATE(50)
	
	MCFG_PALETTE_LENGTH(16)
	MCFG_PALETTE_INIT(pc1640)
	
	MCFG_MC6845_ADD(AMS40041_TAG, AMS40041, XTAL_28_63636MHz/32, crtc_intf)
MACHINE_CONFIG_END
