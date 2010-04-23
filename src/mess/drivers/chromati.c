/***************************************************************************
   
        Chromatics CGC 7900

        05/04/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "includes/cgc7900.h"
#include "cpu/m68000/m68000.h"
#include "formats/basicdsk.h"
#include "devices/messram.h"
#include "machine/msm8251.h"
#include "sound/ay8910.h"

static UINT16* chrom_ram;

static ADDRESS_MAP_START(cgc7900_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x1fffff) AM_RAM AM_BASE(&chrom_ram) // 16 * Buffer card memory
	AM_RANGE(0x800000, 0x80ffff) AM_ROM AM_REGION(M68000_TAG, 0)	
	AM_RANGE(0xa00000, 0xbfffff) AM_RAM AM_BASE_MEMBER(cgc7900_state, z_ram)
	AM_RANGE(0xc00000, 0xdfffff) AM_RAM AM_BASE_MEMBER(cgc7900_state, plane_ram)
//	AM_RANGE(0xe00000, 0xe1ffff) AM_WRITE(color_status_w)
//	AM_RANGE(0xe20000, 0xe23fff) Raster Processor
	AM_RANGE(0xe30000, 0xe303ff) AM_RAM AM_BASE_MEMBER(cgc7900_state, clut_ram)
	AM_RANGE(0xe38000, 0xe3bfff) AM_RAM AM_BASE_MEMBER(cgc7900_state, overlay_ram)
//	AM_RANGE(0xe40000, 0xe40001) Bitmap roll counter
//	AM_RANGE(0xe40002, 0xe40003) X Pan
//	AM_RANGE(0xe40004, 0xe40005) Y Pan
//	AM_RANGE(0xe40006, 0xe40007) X Zoom
//	AM_RANGE(0xe40008, 0xe40009) Y Zoom
//	AM_RANGE(0xe4000a, 0xe4000f) Raster Processor
//	AM_RANGE(0xe40010, 0xe40011) Blink Select
//	AM_RANGE(0xe40012, 0xe40013) Plane Select
//	AM_RANGE(0xe40014, 0xe40015) Plane Video Switch
//	AM_RANGE(0xe40016, 0xe40017) AM_WRITE(color_status_fg_w)
//	AM_RANGE(0xe40018, 0xe40019) AM_WRITE(color_status_bg_w)
//	AM_RANGE(0xe4001a, 0xe4001b) Overlay Roll Counter
//	AM_RANGE(0xefc440, 0xefc441) HVG Load X
//	AM_RANGE(0xefc442, 0xefc443) HVG Load Y
//	AM_RANGE(0xefc444, 0xefc445) HVG Load dX
//	AM_RANGE(0xefc446, 0xefc447) HVG Load dY
//	AM_RANGE(0xefc448, 0xefc449) HVG Load Pixel Color
//	AM_RANGE(0xefc44a, 0xefc44b) HVG Load Trip
	AM_RANGE(0xff8000, 0xff8001) AM_DEVREADWRITE8(INS8251_0_TAG, msm8251_data_r, msm8251_data_w, 0x00ff)
	AM_RANGE(0xff8002, 0xff8003) AM_DEVREADWRITE8(INS8251_0_TAG, msm8251_status_r, msm8251_control_w, 0x00ff)
	AM_RANGE(0xff8040, 0xff8041) AM_DEVREADWRITE8(INS8251_1_TAG, msm8251_data_r, msm8251_data_w, 0x00ff)
	AM_RANGE(0xff8042, 0xff8043) AM_DEVREADWRITE8(INS8251_1_TAG, msm8251_status_r, msm8251_control_w, 0x00ff)
//	AM_RANGE(0xff8080, 0xff8081) Keyboard
//	AM_RANGE(0xff80c6, 0xff80c7) Joystick X axis
//	AM_RANGE(0xff80ca, 0xff80cb) Joystick Y axis
//	AM_RANGE(0xff80cc, 0xff80cd) Joystick Z axis
//	AM_RANGE(0xff8100, 0xff8101) Disk data port
//	AM_RANGE(0xff8120, 0xff8121) Disk control/status port
//	AM_RANGE(0xff8141, 0xff8141) Bezel switches
//	AM_RANGE(0xff8181, 0xff8181) Baud rate generator
//	AM_RANGE(0xff81c0, 0xff81ff) AM_DEVREADWRITE8(MM58167_TAG, mm58167_r, mm58167_w, 0x00ff)
//	AM_RANGE(0xff8200, 0xff8201) Interrupt Mask
//	AM_RANGE(0xff8240, 0xff8241) Light Pen enable
//	AM_RANGE(0xff8242, 0xff8243) Light Pen X value
//	AM_RANGE(0xff8244, 0xff8245) Light Pen Y value
//	AM_RANGE(0xff8246, 0xff8247) Buffer memory parity check
//	AM_RANGE(0xff8248, 0xff8249) Buffer memory parity set/reset
//	AM_RANGE(0xff824a, 0xff824b) Sync information
	AM_RANGE(0xff83c0, 0xff83c1) AM_DEVWRITE8(AY8910_TAG, ay8910_address_w, 0x00ff)
	AM_RANGE(0xff83c2, 0xff83c3) AM_DEVREAD8(AY8910_TAG, ay8910_r, 0x00ff)
	AM_RANGE(0xff83c4, 0xff83c5) AM_DEVWRITE8(AY8910_TAG, ay8910_data_w, 0x00ff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( cgc7900 )
INPUT_PORTS_END


static MACHINE_RESET(cgc7900) 
{
	UINT8* user1 = memory_region(machine, M68000_TAG);

	memcpy((UINT8*)chrom_ram,user1,0x10000);

	devtag_get_device(machine, M68000_TAG)->reset();	
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

static msm8251_interface rs232_intf =
{
	NULL,
	NULL,
	NULL
};

static msm8251_interface rs449_intf =
{
	NULL,
	NULL,
	NULL
};

static const ay8910_interface ay8910_intf =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static MACHINE_DRIVER_START( cgc7900 )
	MDRV_DRIVER_DATA(cgc7900_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(M68000_TAG, M68000, XTAL_8MHz)
    MDRV_CPU_PROGRAM_MAP(cgc7900_mem)

    MDRV_MACHINE_RESET(cgc7900)
	
    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(1024, 768)
    MDRV_SCREEN_VISIBLE_AREA(0, 1024-1, 0, 768-1)
	MDRV_GFXDECODE(cgc7900)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(cgc7900)
    MDRV_VIDEO_UPDATE(cgc7900)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910_TAG, AY8910, 3579545/2)
	MDRV_SOUND_CONFIG(ay8910_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_MSM8251_ADD(INS8251_0_TAG, rs232_intf)
	MDRV_MSM8251_ADD(INS8251_1_TAG, rs449_intf)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( cgc7900 )
    ROM_REGION( 0x10000, M68000_TAG, 0 )
	ROM_LOAD16_BYTE( "210274 800k even a3ee term 1.4.ue24", 0x0001, 0x1000, CRC(5fa8f368) SHA1(120dbcfedce0badd38bf5b23e1fbc99667eb286c) )
	ROM_LOAD16_BYTE( "210275 800k odd bbb3 term 1.4.uf24", 0x0000, 0x1000, CRC(4d479457) SHA1(5fa96a1eadfd9ba493d28691286e2e001a489a19) )
	ROM_LOAD16_BYTE( "210276 802k even 0c22 term 1.4.ue22", 0x2001, 0x1000, CRC(c88c44ec) SHA1(f39d8a3cf7aaefd815b4426348965b076c1f2265) )
	ROM_LOAD16_BYTE( "210277 802k odd b58c term 1.4.uf22", 0x2000, 0x1000, CRC(52ffe01f) SHA1(683aa71c2eb17b7ad639b888487db73d7684901a) )
	ROM_LOAD16_BYTE( "210278 804k even eaf4 term 1.4.ue21", 0x4001, 0x1000, CRC(7fe326db) SHA1(d39d05e008160fb8afa62e0d4cfb1d813f2296d4) )
	ROM_LOAD16_BYTE( "210279 804k odd 3f6d term 1.4.uf21", 0x4000, 0x1000, CRC(6c12d81c) SHA1(efe7c20e567c02b9c5b66a2d18e035d5f5f56b28) )
	ROM_LOAD16_BYTE( "210280 806k even 679d term 1.4.ue20", 0x6001, 0x1000, CRC(70930d74) SHA1(a5ab1c0bd8bd829408961107e01598cd71a97fec) )
	ROM_LOAD16_BYTE( "210281 1.4 806k odd sum 611e.uf20", 0x6000, 0x1000, CRC(8080aa2a) SHA1(c018a23e6f2158e2d63723cade0a3ad737090921) )
	ROM_LOAD16_BYTE( "su7700 210282 808k even opmod term 1.4 sum 2550.ue18", 0x8001, 0x1000, CRC(8f5834cd) SHA1(3cd03c82aa85c30cbc8e954f5a9fc4e9034f913b) )
	ROM_LOAD16_BYTE( "su7700 210283 808k odd opmod term 1.4 sum faca.uf18", 0x8000, 0x1000, CRC(e593b133) SHA1(6c641df844706e0d990b5fd544e98594171080a1) )
	ROM_LOAD16_BYTE( "210258 80ak even b2f6 mon 1.3.ue15", 0xa001, 0x1000, CRC(ec5a1250) SHA1(ffef73d35f172ac610b35bdf729d51eb6f9346ba) )
	ROM_LOAD16_BYTE( "210259 80ak odd cd66 mon 1.3.uf15", 0xa000, 0x1000, CRC(61eb43c6) SHA1(baaae0b787798147da453aac1f815589ea4ed921) )
	ROM_LOAD16_BYTE( "210244 80c even ce40 dos 1.6b.ue13", 0xc001, 0x1000, CRC(3b64e4cb) SHA1(71b28d160b101ea6165e602ff1c54272b7b30ece) )
	ROM_LOAD16_BYTE( "210245 80c odd 1ac3 dos 1.6b.uf13", 0xc000, 0x1000, CRC(0b6539ca) SHA1(d49e6d3307e5d634a412fd80b59492f31e29f7e0) )		
	ROM_LOAD16_BYTE( "210290 idris even rel3 sum 0cce.ue12", 0xe001, 0x1000, CRC(07065772) SHA1(620ea5d55021e5c38efc010722ddbd852cd49e39) )
	ROM_LOAD16_BYTE( "210291 idris odd rel3 sum 5d11.uf12", 0xe000, 0x1000, CRC(d81b30da) SHA1(228f9b4e39d430ce4aaa81ea63f4516a51e6d001) )
	
	ROM_REGION( 0x800, "gfx1", 0 )
	ROM_LOAD( "norm chrset 4b40.ua13", 0x0000, 0x0400, CRC(55eb7b87) SHA1(768cea80597e7396d9e26f8cd09e6b480a526fce) )
	ROM_LOAD( "alt 46a7.ua14",  0x0400, 0x0400, CRC(be22b7e4) SHA1(83ef252c7fab33b4d3821a3049b89d044df35de8) )

	ROM_REGION( 0x20, "proms", 0 )
	ROM_LOAD( "rp0a.ub8", 0x0000, 0x0020, NO_DUMP )
	ROM_LOAD( "rp1a.ub7", 0x0000, 0x0020, NO_DUMP )
	ROM_LOAD( "rp2a.ub9", 0x0000, 0x0020, NO_DUMP )
	ROM_LOAD( "ha-5.ub1", 0x0000, 0x0020, NO_DUMP )
	ROM_LOAD( "03c0.ua16", 0x0000, 0x0020, NO_DUMP )
	ROM_LOAD( "03c0.ua11", 0x0000, 0x0020, NO_DUMP )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1980, cgc7900,  0,       0, 	cgc7900, 	cgc7900, 	 0,   "Chromatics",   "CGC 7900",		GAME_NOT_WORKING)
