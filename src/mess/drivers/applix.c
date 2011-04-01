/***************************************************************************

    Skeleton driver for Applix 1616 computer
 
    See for docs: http;//www.microbee-mspp.org.au
    You need to sign up and make an introductory thread.
    Then you will be granted permission to visit the repository.
 
	TODO: everything!

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "video/mc6845.h"


class applix_state : public driver_device
{
public:
	applix_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
	m_maincpu(*this, "maincpu"),
	m_crtc(*this, "crtc")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_crtc;
	DECLARE_WRITE16_MEMBER(applix_index_w);
	DECLARE_WRITE16_MEMBER(applix_register_w);
	UINT8* m_base;
};

WRITE16_MEMBER( applix_state::applix_index_w )
{
	mc6845_address_w( m_crtc, 0, data >> 8 );
}

WRITE16_MEMBER( applix_state::applix_register_w )
{
	mc6845_register_w( m_crtc, 0, data >> 8 );
}

static ADDRESS_MAP_START(applix_mem, AS_PROGRAM, 16, applix_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x47ffff) AM_RAM AM_BASE(m_base)
	AM_RANGE(0x500000, 0x5fffff) AM_ROM
	AM_RANGE(0x700180, 0x700181) AM_WRITE(applix_index_w)
	AM_RANGE(0x700182, 0x700183) AM_WRITE(applix_register_w)
	//600000, 6FFFFF  io ports and latches
	//700000, 7FFFFF  peripheral chips and devices
	//800000, FFC000  optional roms
	//FFFFC0, FFFFFF  disk controller board
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( applix )
INPUT_PORTS_END


static MACHINE_RESET(applix)
{
	applix_state *state = machine.driver_data<applix_state>();
	UINT8* RAM = machine.region("maincpu")->base();
	memcpy(state->m_base, RAM+0x500000, 16);
	machine.device("maincpu")->reset();
}

static VIDEO_START( applix )
{
}

static SCREEN_UPDATE( applix )
{
	applix_state *state = screen->machine().driver_data<applix_state>();
	mc6845_update(state->m_crtc, bitmap, cliprect);
	return 0;
}

MC6845_UPDATE_ROW( applix_update_row )
{
#if 0
	applix_state *state = device->machine().driver_data<applix_state>();
	UINT8 chr,gfx,fg,bg;
	UINT16 mem,x,col;
	UINT16 colourm = (state->m_08 & 0x0e) << 7;
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)			// for each character
	{
		UINT8 inv=0;
		mem = (ma + x) & 0x7ff;
		chr = state->m_videoram[mem];
		col = state->m_colorram[mem] | colourm;					// read a byte of colour

		/* get pattern of pixels for that character scanline */
		gfx = state->m_gfxram[(chr<<4) | ra] ^ inv;
		fg = (col & 0x001f) | 64;					// map to foreground palette
		bg = (col & 0x07e0) >> 5;					// and background palette

		/* Display a scanline of a character (8 pixels) */
		*p++ = ( gfx & 0x80 ) ? fg : bg;
		*p++ = ( gfx & 0x40 ) ? fg : bg;
		*p++ = ( gfx & 0x20 ) ? fg : bg;
		*p++ = ( gfx & 0x10 ) ? fg : bg;
		*p++ = ( gfx & 0x08 ) ? fg : bg;
		*p++ = ( gfx & 0x04 ) ? fg : bg;
		*p++ = ( gfx & 0x02 ) ? fg : bg;
		*p++ = ( gfx & 0x01 ) ? fg : bg;
	}
#endif
}

static const mc6845_interface applix_crtc = {
	"screen",			/* name of screen */
	8,			/* number of dots per character */
	NULL,
	applix_update_row,		/* handler to display a scanline */
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

static MACHINE_CONFIG_START( applix, applix_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, 7500000)
	MCFG_CPU_PROGRAM_MAP(applix_mem)

	MCFG_MACHINE_RESET(applix)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(applix)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(applix)
	MCFG_MC6845_ADD("crtc", MC6845, 1875000, applix_crtc)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( applix )
	ROM_REGION(0x1000000, "maincpu", 0)
	ROM_LOAD16_BYTE( "1616oshv.044", 0x500000, 0x10000, CRC(4a1a90d3) SHA1(4df504bbf6fc5dad76c29e9657bfa556500420a6) )
	ROM_LOAD16_BYTE( "1616oslv.044", 0x500001, 0x10000, CRC(ef619994) SHA1(ff16fe9e2c99a1ffc855baf89278a97a2a2e881a) )
	//ROM_LOAD16_BYTE( "1616oshv.045", 0x520000, 0x10000, CRC(9dfb3224) SHA1(5223833a357f90b147f25826c01713269fc1945f) )
	//ROM_LOAD16_BYTE( "1616oslv.045", 0x520001, 0x10000, CRC(951bd441) SHA1(e0a38c8d0d38d84955c1de3f6a7d56ce06b063f6) )
	//ROM_LOAD( "1616osv.045",  0x540000, 0x20000, CRC(b9f75432) SHA1(278964e2a02b1fe26ff34f09dc040e03c1d81a6d) )
	//ROM_LOAD( "1616ssdv.022", 0x560000, 0x08000, CRC(6d8e413a) SHA1(fc27d92c34f231345a387b06670f36f8c1705856) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1993, applix,	0,       0, 	applix,	applix,	 0, 	  "Applix Pty Ltd",   "Applix 1616", GAME_NOT_WORKING | GAME_NO_SOUND)
