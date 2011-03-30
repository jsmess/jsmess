#include "includes/pc8401a.h"
#include "pc8500.lh"

/* PC-8401A */

static PALETTE_INIT( pc8401a )
{
	palette_set_color(machine, 0, MAKE_RGB(39, 108, 51));
	palette_set_color(machine, 1, MAKE_RGB(16, 37, 84));
}

void pc8401a_state::video_start()
{
	/* allocate video memory */
	m_video_ram = auto_alloc_array(m_machine, UINT8, PC8401A_LCD_VIDEORAM_SIZE);

	/* register for state saving */
	save_pointer(NAME(m_video_ram), PC8401A_LCD_VIDEORAM_SIZE);
}

bool pc8401a_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	return 0;
}

/* PC-8500 */

void pc8500_state::video_start()
{
	/* allocate video memory */
	m_video_ram = auto_alloc_array(m_machine, UINT8, PC8500_LCD_VIDEORAM_SIZE);

	/* register for state saving */
	save_pointer(NAME(m_video_ram), PC8500_LCD_VIDEORAM_SIZE);
}

bool pc8500_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	sed1330_update(m_lcdc, &bitmap, &cliprect);

	/*
	if (strcmp(screen.tag(), SCREEN_TAG) == 0)
	{
		sed1330_update(m_lcdc, &bitmap, &cliprect);
	}
	else
	{
		mc6845_update(m_crtc, &bitmap, &cliprect);
	}
	*/

	return 0;
}

/* SED1330 Interface */

READ8_MEMBER( pc8500_state::vd_r )
{
	return m_video_ram[offset & PC8500_LCD_VIDEORAM_MASK];
}

WRITE8_MEMBER( pc8500_state::vd_w )
{
	m_video_ram[offset & PC8500_LCD_VIDEORAM_MASK] = data;
}

static SED1330_INTERFACE( pc8500_sed1330_config )
{
	SCREEN_TAG,
	DEVCB_DRIVER_MEMBER(pc8500_state, vd_r),
	DEVCB_DRIVER_MEMBER(pc8500_state, vd_w)
};

/* MC6845 Interface */

static MC6845_UPDATE_ROW( pc8441a_update_row )
{
}

static const mc6845_interface pc8441a_mc6845_interface = 
{
	CRT_SCREEN_TAG,
	6,
	NULL,
	pc8441a_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

/* Machine Drivers */

MACHINE_CONFIG_FRAGMENT( pc8401a_video )
//  MCFG_DEFAULT_LAYOUT(layout_pc8401a)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(pc8401a)

	/* LCD */
	MCFG_SCREEN_ADD(SCREEN_TAG, LCD)
	MCFG_SCREEN_REFRESH_RATE(44)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(480, 128)
	MCFG_SCREEN_VISIBLE_AREA(0, 480-1, 0, 128-1)
MACHINE_CONFIG_END

MACHINE_CONFIG_FRAGMENT( pc8500_video )
	MCFG_DEFAULT_LAYOUT(layout_pc8500)

	MCFG_PALETTE_LENGTH(2+8)
	MCFG_PALETTE_INIT(pc8401a)

	/* LCD */
	MCFG_SCREEN_ADD(SCREEN_TAG, LCD)
	MCFG_SCREEN_REFRESH_RATE(44)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(480, 208)
	MCFG_SCREEN_VISIBLE_AREA(0, 480-1, 0, 200-1)
	
	MCFG_SED1330_ADD(SED1330_TAG, 0, pc8500_sed1330_config)

	/* PC-8441A CRT */
	MCFG_SCREEN_ADD(CRT_SCREEN_TAG, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(80*8, 24*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 80*8-1, 0, 24*8-1)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_REFRESH_RATE(50)
	
	MCFG_MC6845_ADD(MC6845_TAG, MC6845, 400000, pc8441a_mc6845_interface)
MACHINE_CONFIG_END
