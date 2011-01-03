/***************************************************************************
   
        Micro Craft Dimension 68000

        28/12/2011 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"

class dim68k_state : public driver_device
{
public:
	dim68k_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }
		
	UINT16* ram;
	UINT8 *FNT;
};

static ADDRESS_MAP_START(dim68k_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x00feffff) AM_RAM AM_BASE_MEMBER(dim68k_state, ram) // 16MB RAM / ROM at boot	
	AM_RANGE(0x00ff0000, 0x00ff1fff) AM_ROM AM_REGION("user1",0)
	AM_RANGE(0x00ff2000, 0x00ff7fff) AM_RAM // Video RAM
	AM_RANGE(0x00ff8000, 0x00ffffff) AM_RAM // I/O Region
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( dim68k )
INPUT_PORTS_END


static MACHINE_RESET(dim68k) 
{
	dim68k_state *state = machine->driver_data<dim68k_state>();
	UINT8* user1 = machine->region("user1")->base();

	memcpy((UINT8*)state->ram,user1,0x20000);

	machine->device("maincpu")->reset();	
}

static VIDEO_START( dim68k )
{
	dim68k_state *state = machine->driver_data<dim68k_state>();
	state->FNT = machine->region("chargen")->base();
}

// Please note: This is NOT how the real video works. It is a sample, until the driver is rewritten.
static VIDEO_UPDATE( dim68k )
{
	dim68k_state *state = screen->machine->driver_data<dim68k_state>();
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0x100,x,dchr;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma+40; x++)
			{
				dchr = state->ram[x]; // reads 2 characters
				chr = dchr>>8;

				gfx = state->FNT[(chr<<4) | ra] ^ ((chr & 0x80) ? 0xff : 0);

				*p++ = ( gfx & 0x80 ) ? 1 : 0;
				*p++ = ( gfx & 0x40 ) ? 1 : 0;
				*p++ = ( gfx & 0x20 ) ? 1 : 0;
				*p++ = ( gfx & 0x10 ) ? 1 : 0;
				*p++ = ( gfx & 0x08 ) ? 1 : 0;
				*p++ = ( gfx & 0x04 ) ? 1 : 0;
				*p++ = ( gfx & 0x02 ) ? 1 : 0;
				*p++ = ( gfx & 0x01 ) ? 1 : 0;

				chr = dchr;

				gfx = state->FNT[(chr<<4) | ra] ^ ((chr & 0x80) ? 0xff : 0);

				*p++ = ( gfx & 0x80 ) ? 1 : 0;
				*p++ = ( gfx & 0x40 ) ? 1 : 0;
				*p++ = ( gfx & 0x20 ) ? 1 : 0;
				*p++ = ( gfx & 0x10 ) ? 1 : 0;
				*p++ = ( gfx & 0x08 ) ? 1 : 0;
				*p++ = ( gfx & 0x04 ) ? 1 : 0;
				*p++ = ( gfx & 0x02 ) ? 1 : 0;
				*p++ = ( gfx & 0x01 ) ? 1 : 0;
			}
		}
		ma+=40;
	}
	return 0;
}

/* F4 Character Displayer */
static const gfx_layout dim68k_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( dim68k )
	GFXDECODE_ENTRY( "chargen", 0x0000, dim68k_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( dim68k, dim68k_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, XTAL_10MHz)
	MCFG_CPU_PROGRAM_MAP(dim68k_mem)	

	MCFG_MACHINE_RESET(dim68k)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 250-1)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)
	MCFG_GFXDECODE(dim68k)

	MCFG_VIDEO_START(dim68k)
	MCFG_VIDEO_UPDATE(dim68k)
MACHINE_CONFIG_END

/*
68000 

MC101A	82S100
MC102B	82S100
MC103E	2732A
MC104	2732A	label "4050" underneath
MC105	2732A	char gen

6512

MC106	82LS135	U24
MC107	82LS135	U20
MC108	82S137	U23
MC109	82S131	U16
MC110	82LS135	U35

Z80

MC111	82S123	U11

8086

MC112	82LS135	U18
MC113	82S153	U16
*/
/* ROM definition */
ROM_START( dim68k )
	ROM_REGION( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "mc103e.bin", 0x0000, 0x1000, CRC(4730c902) SHA1(5c4bb79ad22def721a22eb63dd05e0391c8082be))
	ROM_LOAD16_BYTE( "mc104.bin",  0x0001, 0x1000, CRC(14b04575) SHA1(43e15d9ebe1c9c1bf1bcfc1be3899a49e6748200))

	ROM_REGION( 0x10000, "cop6512", ROMREGION_ERASEFF )
	ROM_LOAD( "mc106.bin", 0x0000, 0x0100, CRC(11530d8a) SHA1(e3eae266535383bcaee2d84d7bed6052d40e4e4a))
	ROM_LOAD( "mc107.bin", 0x0000, 0x0100, CRC(966db11b) SHA1(3c3105ac842602d8e01b0f924152fd672a85f00c))
	ROM_LOAD( "mc108.bin", 0x0000, 0x0400, CRC(687f9b0a) SHA1(ed9f1265b25f89f6d3cf8cd0a7b0fb73cb129f9f))
	ROM_LOAD( "mc109.bin", 0x0000, 0x0200, CRC(4a857f98) SHA1(9f2bbc2171fc49f65aa798c9cd7799a26afd2ddf))
	ROM_LOAD( "mc110.bin", 0x0000, 0x0100, CRC(e207b457) SHA1(a8987ba3d1bbdb3d8b3b11cec90c532ff09e762e))

	ROM_REGION( 0x10000, "copz80", ROMREGION_ERASEFF )
	ROM_LOAD( "mc111.bin", 0x0000, 0x0020, CRC(6a380057) SHA1(6522a7b3e0af9db14a6ed04d4eec3ee6e44c2dab))

	ROM_REGION( 0x10000, "cop8086", ROMREGION_ERASEFF )
	ROM_LOAD( "mc112.bin", 0x0000, 0x0100, CRC(dfd4cdbb) SHA1(a7831d415943fa86c417066807038bccbabb2573))
	ROM_LOAD( "mc113.bin", 0x0000, 0x00ef, CRC(594bdf05) SHA1(36db911a27d930e023fa12683e86e9eecfffdba6))
  
	ROM_REGION( 0x1000, "chargen", ROMREGION_ERASEFF )   
	ROM_LOAD( "mc105e.bin", 0x0000, 0x1000, CRC(7a09daa8) SHA1(844bfa579293d7c3442fcbfa21bda75fff930394))

	ROM_REGION( 0x1000, "mb", ROMREGION_ERASEFF )	// mainboard unknown
	ROM_LOAD( "mc102.bin", 0x0000, 0x00fa, CRC(38e2abac) SHA1(0d7e730b46fc162764c69c51dea3bbe8337b1a7d))
	ROM_LOAD( "mc101.bin", 0x0000, 0x00fa, CRC(caffb3a0) SHA1(36f5140b306565794c4d856e0c20589b8f2a37f4))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1984, dim68k,  0,       0, 	dim68k, 	dim68k, 	 0,  	   "Micro Craft",   "Dimension 68000",		GAME_NOT_WORKING | GAME_NO_SOUND)

