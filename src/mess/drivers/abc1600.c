/*
	
	Luxor ABC 1600

	Skeleton driver

*/

/*

	TODO:

	- video
	- keyboard
	- mouse
	- floppy
	- DMA
	- DART
	- CIO
	- hard disk (Xebec controller card w/ Z80A, 1K RAM, ROM "104521G", PROM "103911", Xebec 3198-0009, Xebec 3198-0045, 16MHz xtal, 20MHz xtal)
	- monitor portrait/landscape mode

*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"
#include "machine/abc99.h"
#include "machine/wd17xx.h"
#include "machine/z80dart.h"
#include "machine/z80dma.h"
#include "video/mc6845.h"
#include "includes/abc1600.h"



//**************************************************************************
//	ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( abc1600_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc1600_mem, ADDRESS_SPACE_PROGRAM, 8, abc1600_state )
	AM_RANGE(0x00000, 0x03fff) AM_ROM
	AM_RANGE(0x04000, 0x7efff) AM_RAM
	AM_RANGE(0x7f100, 0x7f100) AM_DEVWRITE_LEGACY(SY6845EA_TAG, mc6845_address_w)
	AM_RANGE(0x7f101, 0x7f101) AM_DEVREADWRITE_LEGACY(SY6845EA_TAG, mc6845_register_r, mc6845_register_w)
//	AM_RANGE(0x7f000, 0x7f000) AM_DEVREADWRITE_LEGACY(Z8410AB1_0_TAG, z80dma_r, z80dma_w)
//	AM_RANGE(0x7f000, 0x7f000) AM_DEVREADWRITE_LEGACY(Z8410AB1_1_TAG, z80dma_r, z80dma_w)
//	AM_RANGE(0x7f000, 0x7f000) AM_DEVREADWRITE_LEGACY(Z8410AB1_2_TAG, z80dma_r, z80dma_w)
//	AM_RANGE(0x7f000, 0x7f003) AM_DEVREADWRITE_LEGACY(Z80DART_TAG, z80dart_ba_cd_r, z80dart_ba_cd_w)
//	AM_RANGE(0x7f000, 0x7f003) AM_DEVREADWRITE_LEGACY(SAB1797_02P_TAG, wd17xx_r, wd17xx_w)
//	AM_RANGE(0x7f000, 0x7f003) AM_DEVREADWRITE(Z8536B1_TAG, z8536_r, z8536_w)
	AM_RANGE(0x80000, 0xfffff) AM_RAM
ADDRESS_MAP_END



//**************************************************************************
//	INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( abc1600 )
//-------------------------------------------------

static INPUT_PORTS_START( abc1600 )
	PORT_INCLUDE(abc99)
INPUT_PORTS_END



//**************************************************************************
//	VIDEO
//**************************************************************************

//-------------------------------------------------
//  MC6845_UPDATE_ROW( abc1600_update_row )
//-------------------------------------------------

static MC6845_UPDATE_ROW( abc1600_update_row )
{
}


//-------------------------------------------------
//  mc6845_interface crtc_intf
//-------------------------------------------------

static const mc6845_interface crtc_intf =
{
	SCREEN_TAG,
	8,
	NULL,
	abc1600_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};


//-------------------------------------------------
//  VIDEO_UPDATE( abc1600 )
//-------------------------------------------------

bool abc1600_state::video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	mc6845_update(m_crtc, &bitmap, &cliprect);

	return 0;
}



//**************************************************************************
//	DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  Z80DMA_INTERFACE( dma0_intf )
//-------------------------------------------------

static Z80DMA_INTERFACE( dma0_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
};


//-------------------------------------------------
//  Z80DMA_INTERFACE( dma1_intf )
//-------------------------------------------------

static Z80DMA_INTERFACE( dma1_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
};


//-------------------------------------------------
//  Z80DMA_INTERFACE( dma2_intf )
//-------------------------------------------------

static Z80DMA_INTERFACE( dma2_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
};


//-------------------------------------------------
//  Z80DART_INTERFACE( dart_intf )
//-------------------------------------------------

static Z80DART_INTERFACE( dart_intf )
{
	0, 0, 0, 0,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL
};


//-------------------------------------------------
//  wd17xx_interface fdc_intf
//-------------------------------------------------

static const floppy_config abc1600_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_5_25_DSQD,
    FLOPPY_OPTIONS_NAME(default),
    NULL
};

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{ FLOPPY_0, NULL, NULL, NULL }
};



//**************************************************************************
//	MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( abc1600 )
//-------------------------------------------------

void abc1600_state::machine_start()
{
}



//**************************************************************************
//	MACHINE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( abc1600 )
//-------------------------------------------------

static MACHINE_CONFIG_START( abc1600, abc1600_state )
	// basic machine hardware
	MDRV_CPU_ADD(MC68008P8_TAG, M68008, XTAL_64MHz/8)
	MDRV_CPU_PROGRAM_MAP(abc1600_mem)

	// video hardware
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) // not accurate
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(1024, 768)
    MDRV_SCREEN_VISIBLE_AREA(0, 1024-1, 0, 768-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)
	MDRV_MC6845_ADD(SY6845EA_TAG, MC6845, XTAL_64MHz, crtc_intf)

	// sound hardware

	// devices
	MDRV_Z80DMA_ADD(Z8410AB1_0_TAG, 4000000, dma0_intf)
	MDRV_Z80DMA_ADD(Z8410AB1_1_TAG, 4000000, dma1_intf)
	MDRV_Z80DMA_ADD(Z8410AB1_2_TAG, 4000000, dma2_intf)
	MDRV_Z80DART_ADD(Z8470AB1_TAG, 4000000, dart_intf)
	MDRV_WD179X_ADD(SAB1797_02P_TAG, fdc_intf)
	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, abc1600_floppy_config)
	MDRV_ABC99_ADD()

	// internal ram
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1M")
MACHINE_CONFIG_END



//**************************************************************************
//	ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( abc1600 )
//-------------------------------------------------

ROM_START( abc1600 )
	ROM_REGION( 0x4000, MC68008P8_TAG, 0 )
	ROM_LOAD( "boot 6490356-04.1f", 0x0000, 0x4000, CRC(9372f6f2) SHA1(86f0681f7ef8dd190b49eda5e781881582e0c2a4) )

	ROM_REGION( 0x2000, "wrmsk", 0 )
	ROM_LOAD( "wrmskl 6490362-01.1g", 0x0000, 0x1000, CRC(bc737538) SHA1(80e2c3757eb7f713018808d6e41ebef612425028) )
	ROM_LOAD( "wrmskh 6490363-01.1j", 0x1000, 0x1000, CRC(6b7c9f0b) SHA1(7155a993adcf08a5a8a2f22becf9fd66fda698be) )

	ROM_REGION( 0x200, "shinf", 0 )
	ROM_LOAD( "shinf 6490361-01.1f", 0x000, 0x200, CRC(20260f8f) SHA1(29bf49c64e7cc7592e88cde2768ac57c7ce5e085) )

	ROM_REGION( 0x40, "drmsk", 0 )
	ROM_LOAD( "drmskl 6490359-01.1k", 0x00, 0x20, CRC(6e71087c) SHA1(0acf67700d6227f4b315cf8fb0fb31c0e7fb9496) )
	ROM_LOAD( "drmskh 6490358-01.1k", 0x20, 0x20, CRC(a4a9a9dc) SHA1(d8575c0335d6021cbb5f7bcd298b41c35294a80a) )

	ROM_REGION( 0x71c, "plds", 0 )
	ROM_LOAD( "drmsk 6490360-01.1m", 0x000, 0x104, CRC(5f7143c1) SHA1(1129917845f8e505998b15288f02bf907487e4ac) ) // @ 1m,1n,1t,2t
	ROM_LOAD( "1020 6490349-01.8b",  0x104, 0x104, CRC(1fa065eb) SHA1(20a95940e39fa98e97e59ea1e548ac2e0c9a3444) )
	ROM_LOAD( "1021 6490350-01.5d",  0x208, 0x104, CRC(96f6f44b) SHA1(12d1cd153dcc99d1c4a6c834122f370d49723674) )
	ROM_LOAD( "1022 6490351-01.17e", 0x30c, 0x104, CRC(5dd00d43) SHA1(a3871f0d796bea9df8f25d41b3169dd4b8ef65ab) )
	ROM_LOAD( "1023 6490352-01.11e", 0x410, 0x104, CRC(a2f350ac) SHA1(77e08654a197080fa2111bc3031cd2c7699bf82b) )
	ROM_LOAD( "1024 6490353-01.12e", 0x514, 0x104, CRC(67f1328a) SHA1(b585495fe14a7ae2fbb29f722dca106d59325002) )
	ROM_LOAD( "1025 6490354-01.6e",  0x618, 0x104, CRC(9bda0468) SHA1(ad373995dcc18532274efad76fa80bd13c23df25) )

	ROM_REGION( 0x1000, Z8400A_TAG, 0 ) // Xebec hard disk controller card
	ROM_LOAD( "104521g", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x100, "103911", 0 ) // Xebec hard disk controller card
	ROM_LOAD( "103911", 0x000, 0x100, NO_DUMP ) // DM74S288N
ROM_END



//**************************************************************************
//	SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     INIT  COMPANY     FULLNAME     FLAGS
COMP( 1985, abc1600, 0,      0,      abc1600, abc1600, 0,    "Luxor", "ABC 1600", GAME_NOT_WORKING | GAME_NO_SOUND )
