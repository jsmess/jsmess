/***************************************************************************
   
        Mitsubishi Multi 8

        13/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(multi8_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7FFF) AM_ROM
	AM_RANGE(0x8000, 0xFFFF) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( multi8_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	/* This is all a guess... ports used in the bootup process:
	in 00 = ascii code from the keyboard
	in 01 = is a key pressed? (bit 1) also bits 6 and 7 are used
	out 10 = unknown
	out 18, 19 = audio channel
	in 1A = unknown
	out 1C, 1D = possibly a mc6845
	out 25, 26, 27, 2A, 2B, 2C, 2D, 30, 78 = unknown
	in 28 = unknown (bit 5 is a status line) */
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( multi8 )
INPUT_PORTS_END


static MACHINE_RESET(multi8) 
{	
}

static VIDEO_START( multi8 )
{
}

static VIDEO_UPDATE( multi8 )
{
    return 0;
}

/* F4 Character Displayer */
static const gfx_layout multi8_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( multi8 )
	GFXDECODE_ENTRY( "chargen", 0x0000, multi8_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_DRIVER_START( multi8 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(multi8_mem)
    MDRV_CPU_IO_MAP(multi8_io)	

    MDRV_MACHINE_RESET(multi8)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)
	MDRV_GFXDECODE(multi8)

    MDRV_VIDEO_START(multi8)
    MDRV_VIDEO_UPDATE(multi8)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( multi8 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basic.rom", 0x0000, 0x8000, CRC(2131a3a8) SHA1(0f5a565ecfbf79237badbf9011dcb374abe74a57))
	ROM_REGION( 0x0800, "chargen", 0 )
	ROM_LOAD( "font.rom",  0x0000, 0x0800, CRC(08f9ed0e) SHA1(57480510fb30af1372df5a44b23066ca61c6f0d9))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1983, multi8,  0,       0, 	multi8, 	multi8, 	 0,   "Mitsubishi",   "Multi 8",		GAME_NOT_WORKING | GAME_NO_SOUND)

