/***************************************************************************

        Indiana University 68030 board

        08/12/2009 Skeleton driver.

        If 6001F7 is held at 40, then '>' will appear on screen.
        System often reads/writes 6003D4/5, might be a cut-down 6845,
        as it only uses registers C,D,E,F.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m68000/m68000.h"


class indiana_state : public driver_device
{
public:
	indiana_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT8 *m_p_chargen;
	UINT8 *m_p_videoram;
};


static ADDRESS_MAP_START(indiana_mem, AS_PROGRAM, 32, indiana_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0000ffff) AM_MIRROR(0x7f800000) AM_ROM AM_REGION("user1",0) // 64Kb of EPROM
	AM_RANGE(0x00100000, 0x00107fff) AM_MIRROR(0x7f8f8000) AM_RAM // SRAM 32Kb of SRAM
	AM_RANGE(0x00200000, 0x002fffff) AM_MIRROR(0x7f800000) AM_RAM // MFP
	AM_RANGE(0x00400000, 0x004fffff) AM_MIRROR(0x7f800000) AM_RAM // 16 bit PC IO
	AM_RANGE(0x00500000, 0x005fffff) AM_MIRROR(0x7f800000) AM_RAM // 16 bit PC MEM
	AM_RANGE(0x00600000, 0x006fffff) AM_MIRROR(0x7f800000) AM_RAM // 8 bit PC IO
	AM_RANGE(0x00700000, 0x007b7fff) AM_MIRROR(0x7f800000) AM_RAM // 8 bit PC MEM
	AM_RANGE(0x007b8000, 0x007fffff) AM_MIRROR(0x7f800000) AM_RAM AM_BASE(m_p_videoram) // 8 bit PC MEM
	AM_RANGE(0x80000000, 0x803fffff) AM_MIRROR(0x7fc00000) AM_RAM // 4 MB RAM
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( indiana )
INPUT_PORTS_END


static MACHINE_RESET(indiana)
{
}

static VIDEO_START( indiana )
{
	indiana_state *state = machine.driver_data<indiana_state>();
	static const UINT16 chargens[3] = { 0x6710, 0x671c, 0x66a4 };
	state->m_p_chargen = machine.region("user1")->base()+chargens[rom_system_bios(machine)-1];
}

static SCREEN_UPDATE_IND16( indiana )
{
	indiana_state *state = screen.machine().driver_data<indiana_state>();
	UINT8 y,ra,chr,gfx,fg;
	UINT16 sy=0,ma=0,x;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 16; ra++)
		{
			UINT16 *p = &bitmap.pix16(sy++);

			for (x = ma; x < ma + 160; x+=2)
			{
				chr = state->m_p_videoram[(x^2)+1];
				fg = state->m_p_videoram[x^2] & 7;

				gfx = state->m_p_chargen[(chr<<4) | (ra ^ 3) ];

				*p++ = BIT(gfx, 7) ? fg : 0;
				*p++ = BIT(gfx, 6) ? fg : 0;
				*p++ = BIT(gfx, 5) ? fg : 0;
				*p++ = BIT(gfx, 4) ? fg : 0;
				*p++ = BIT(gfx, 3) ? fg : 0;
				*p++ = BIT(gfx, 2) ? fg : 0;
				*p++ = BIT(gfx, 1) ? fg : 0;
				*p++ = BIT(gfx, 0) ? fg : 0;
			}
		}
		ma+=160;
	}

	return 0;
}

/* F4 Character Displayer */
static const gfx_layout indiana_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	128,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 3*8, 2*8, 1*8, 0*8, 7*8, 6*8, 5*8, 4*8, 11*8, 10*8, 9*8, 8*8, 15*8, 14*8, 13*8, 12*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( indiana )
	GFXDECODE_ENTRY( "user1", 0x6710, indiana_charlayout, 0, 4 ) // offset is for -bios 0
GFXDECODE_END

static MACHINE_CONFIG_START( indiana, indiana_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",M68030, XTAL_16MHz)
	MCFG_CPU_PROGRAM_MAP(indiana_mem)

	MCFG_MACHINE_RESET(indiana)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 400)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 400-1)
	MCFG_VIDEO_START(indiana)
	MCFG_SCREEN_UPDATE_STATIC(indiana)
	MCFG_GFXDECODE(indiana)
	MCFG_PALETTE_LENGTH(8)
	//MCFG_PALETTE_INIT(black_and_white)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( indiana )
	ROM_REGION32_BE( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v9", "ver 0.9" )
	ROMX_LOAD( "prom0_9.bin", 0x0000, 0x10000, CRC(746ad75e) SHA1(7d5c123c8568b1e02ab683e8f3188d0fef78d740), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v8", "ver 0.8" )
	ROMX_LOAD( "prom0_8.bin", 0x0000, 0x10000, CRC(9d8dafee) SHA1(c824e5fe6eec08f51ef287c651a5034fe3c8b718), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v7", "ver 0.7" )
	ROMX_LOAD( "prom0_7.bin", 0x0000, 0x10000, CRC(d6a3b6bc) SHA1(01d8cee989ab29646d9d3f8b7262b10055653d41), ROM_BIOS(3))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                  FULLNAME                               FLAGS */
COMP( 1993, indiana,  0,       0,    indiana,   indiana,  0,  "Indiana University", "Indiana University 68030 board", GAME_NOT_WORKING | GAME_NO_SOUND)
