/***************************************************************************
   
        Chromatics CGC 7900

        05/04/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"

static UINT16* chrom_ram;

static ADDRESS_MAP_START(cgc7900_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM AM_BASE(&chrom_ram) // 16 * Buffer card memory
	AM_RANGE(0x00800000, 0x0080ffff) AM_ROM AM_REGION("user1",0)	
	AM_RANGE(0x00a00000, 0x00bfffff) AM_RAM // Z mode screen memory
	AM_RANGE(0x00c00000, 0x00dfffff) AM_RAM // Image plane 0-15 
	AM_RANGE(0x00e00000, 0x00efffff) AM_RAM // Color status map in first 128K
	// I/O 
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( cgc7900 )
INPUT_PORTS_END


static MACHINE_RESET(cgc7900) 
{
	UINT8* user1 = memory_region(machine, "user1");

	memcpy((UINT8*)chrom_ram,user1,0x10000);

	devtag_get_device(machine, "maincpu")->reset();	
}

static VIDEO_START( cgc7900 )
{
}

static VIDEO_UPDATE( cgc7900 )
{
    return 0;
}

/* F4 Character Displayer */
static const gfx_layout cgc7900_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( cgc7900 )
	GFXDECODE_ENTRY( "gfx1", 0x0000, cgc7900_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_DRIVER_START( cgc7900 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", M68000, XTAL_8MHz)
    MDRV_CPU_PROGRAM_MAP(cgc7900_mem)

    MDRV_MACHINE_RESET(cgc7900)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_GFXDECODE(cgc7900)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(cgc7900)
    MDRV_VIDEO_UPDATE(cgc7900)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( cgc7900 )
    ROM_REGION( 0x10000, "user1", ROMREGION_ERASEFF )
	
	ROM_LOAD16_BYTE( "210274.bin", 0x0001, 0x1000, CRC(5fa8f368) SHA1(120dbcfedce0badd38bf5b23e1fbc99667eb286c))
	ROM_LOAD16_BYTE( "210275.bin", 0x0000, 0x1000, CRC(4d479457) SHA1(5fa96a1eadfd9ba493d28691286e2e001a489a19))
	ROM_LOAD16_BYTE( "210276.bin", 0x2001, 0x1000, CRC(c88c44ec) SHA1(f39d8a3cf7aaefd815b4426348965b076c1f2265))
	ROM_LOAD16_BYTE( "210277.bin", 0x2000, 0x1000, CRC(52ffe01f) SHA1(683aa71c2eb17b7ad639b888487db73d7684901a))
	ROM_LOAD16_BYTE( "210278.bin", 0x4001, 0x1000, CRC(7fe326db) SHA1(d39d05e008160fb8afa62e0d4cfb1d813f2296d4))
	ROM_LOAD16_BYTE( "210279.bin", 0x4000, 0x1000, CRC(6c12d81c) SHA1(efe7c20e567c02b9c5b66a2d18e035d5f5f56b28))
	ROM_LOAD16_BYTE( "210280.bin", 0x6001, 0x1000, CRC(70930d74) SHA1(a5ab1c0bd8bd829408961107e01598cd71a97fec))
	ROM_LOAD16_BYTE( "210281.bin", 0x6000, 0x1000, CRC(8080aa2a) SHA1(c018a23e6f2158e2d63723cade0a3ad737090921))
	ROM_LOAD16_BYTE( "210282.bin", 0x8001, 0x1000, CRC(8f5834cd) SHA1(3cd03c82aa85c30cbc8e954f5a9fc4e9034f913b))
	ROM_LOAD16_BYTE( "210283.bin", 0x8000, 0x1000, CRC(e593b133) SHA1(6c641df844706e0d990b5fd544e98594171080a1))
	ROM_LOAD16_BYTE( "210258.bin", 0xa001, 0x1000, CRC(ec5a1250) SHA1(ffef73d35f172ac610b35bdf729d51eb6f9346ba))
	ROM_LOAD16_BYTE( "210259.bin", 0xa000, 0x1000, CRC(61eb43c6) SHA1(baaae0b787798147da453aac1f815589ea4ed921))
	ROM_LOAD16_BYTE( "210244.bin", 0xc001, 0x1000, CRC(3b64e4cb) SHA1(71b28d160b101ea6165e602ff1c54272b7b30ece))
	ROM_LOAD16_BYTE( "210245.bin", 0xc000, 0x1000, CRC(0b6539ca) SHA1(d49e6d3307e5d634a412fd80b59492f31e29f7e0))		
	ROM_LOAD16_BYTE( "210290.bin", 0xe001, 0x1000, CRC(07065772) SHA1(620ea5d55021e5c38efc010722ddbd852cd49e39))
	ROM_LOAD16_BYTE( "210291.bin", 0xe000, 0x1000, CRC(d81b30da) SHA1(228f9b4e39d430ce4aaa81ea63f4516a51e6d001))
	ROM_REGION( 0x0800, "gfx1", ROMREGION_ERASEFF )
	ROM_LOAD( "normchar.bin", 0x0000, 0x0400, CRC(55eb7b87) SHA1(768cea80597e7396d9e26f8cd09e6b480a526fce))
	ROM_LOAD( "altchar.bin",  0x0400, 0x0400, CRC(be22b7e4) SHA1(83ef252c7fab33b4d3821a3049b89d044df35de8))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( ????, cgc7900,  0,       0, 	cgc7900, 	cgc7900, 	 0,   "Chromatics",   "CGC 7900",		GAME_NOT_WORKING | GAME_NO_SOUND)

