/***************************************************************************

    Poly/Proteus (New Zealand)

    10/07/2011 Skeleton driver.

    http://www.cs.otago.ac.nz/homepages/andrew/poly/Poly.htm

    It uses a 6809 for all main functions. There is a Z80 for CP/M, but all
    of the roms are 6809 code.

    ToDo:
    - Everything!

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "cpu/z80/z80.h"
#include "video/saa5050.h"
#include "machine/terminal.h"


class poly_state : public driver_device
{
public:
	poly_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu")
	{ }

	required_device<cpu_device> m_maincpu;
	DECLARE_WRITE8_MEMBER(kbd_put);
};


static ADDRESS_MAP_START(poly_mem, AS_PROGRAM, 8, poly_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0x9fff) AM_RAM
	AM_RANGE(0xa000,0xcfff) AM_ROM
	AM_RANGE(0xd000,0xe7ff) AM_RAM
	AM_RANGE(0xe800,0xebff) AM_DEVREADWRITE_LEGACY("saa5050", saa5050_videoram_r, saa5050_videoram_w)
	AM_RANGE(0xec00,0xefff) AM_RAM
	AM_RANGE(0xf000,0xffff) AM_ROM
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( poly )
INPUT_PORTS_END


static MACHINE_RESET( poly )
{
}

static SCREEN_UPDATE( poly )
{
	device_t *saa5050 = screen->machine().device("saa5050");

	saa5050_update(saa5050, bitmap, NULL);
	return 0;
}


static const saa5050_interface poly_saa5050_intf =
{
	"screen",
	0,	/* starting gfxnum */
	40, 25, 40,  /* x, y, size */
	0	/* rev y order */
};

// temporary hack
WRITE8_MEMBER( poly_state::kbd_put )
{
	address_space *mem = m_maincpu->memory().space(AS_PROGRAM);
	mem->write_byte(0xebec, data);
	mem->write_byte(0xebd0, 1); // any non-zero here
}

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_DRIVER_MEMBER(poly_state, kbd_put)
};

static MACHINE_CONFIG_START( poly, poly_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M6809E, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(poly_mem)

	MCFG_MACHINE_RESET(poly)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(SAA5050_VBLANK))
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(40 * 12, 24 * 20)
	MCFG_SCREEN_VISIBLE_AREA(0, 40 * 12 - 1, 0, 24 * 20 - 1)
	MCFG_SCREEN_UPDATE(poly)

	MCFG_GFXDECODE(saa5050)
	MCFG_PALETTE_LENGTH(128)
	MCFG_PALETTE_INIT(saa5050)

	MCFG_SAA5050_ADD("saa5050", poly_saa5050_intf)

	// temporary hack
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( poly1 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "v3bas1.bin", 0xa000, 0x1000, CRC(2c5276cb) SHA1(897cb9c2456ddb0f316a8c3b8aa56706056cc1dd) )
	// next 3 roms are at the wrong location
	ROM_LOAD( "v3bas2.bin", 0xb000, 0x1000, CRC(30f99447) SHA1(a26170113a968ccd8df7db1b0f256a2198054037) )
	ROM_LOAD( "v3bas3.bin", 0xc000, 0x1000, CRC(89ea5b27) SHA1(e37a61d3dd78fb40bc43c70af9714ce3f75fd895) )
	ROM_LOAD( "v3bas4.bin", 0x9000, 0x1000, CRC(c16c1209) SHA1(42f3b0bce32aafab14bc0500fb13bd456730875c) )
	// boot rom
	ROM_LOAD( "plrt16v3e9.bin", 0xf000, 0x1000, CRC(453c10a0) SHA1(edfbc3d83710539c01093e89fe1b47dfe1e68acd) )

	/* This is SAA5050 came with poly emulator, identical to p2000t */
	ROM_REGION(0x01000, "gfx1",0)
	ROM_LOAD("p2000.chr", 0x0140, 0x08c0, CRC(78c17e3e) SHA1(4e1c59dc484505de1dc0b1ba7e5f70a54b0d4ccc))
ROM_END

/* Driver */

/*    YEAR   NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1981,  poly1,  0,      0,       poly,      poly,    0,      "Polycorp",  "Poly-1 Educational Computer", GAME_NOT_WORKING | GAME_NO_SOUND )
