/*

    Luxor ABC 1600

    Skeleton driver

*/

/*

    TODO:

	- memory access controller
		- task register
		- cause register (parity error)
		- segment RAM
		- page RAM
		- bankswitching
		- DMA
	- video
		- CRTC
		- bitmap layer
		- monitor portrait/landscape mode
    - CIO (interrupt controller)
		- RTC
		- NVRAM
    - DART
		- keyboard
    - floppy
    - hard disk (Xebec S1410)

*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "imagedev/flopdrv.h"
#include "machine/ram.h"
#include "machine/8530scc.h"
#include "machine/abc99.h"
#include "machine/e0516.h"
#include "machine/nmc9306.h"
#include "machine/s1410.h"
#include "machine/wd17xx.h"
#include "machine/z80dart.h"
#include "machine/z80dma.h"
#include "machine/z8536.h"
#include "video/mc6845.h"
#include "includes/abc1600.h"



//**************************************************************************
//  MEMORY ACCESS CONTROLLER
//**************************************************************************

//-------------------------------------------------
//  bankswitch -
//-------------------------------------------------

void abc1600_state::bankswitch()
{
}


//-------------------------------------------------
//  cause_r -
//-------------------------------------------------

READ8_MEMBER( abc1600_state::cause_r )
{
	return 0;
}


//-------------------------------------------------
//  task_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::task_w )
{
	/*
	
		bit		description
		
		0		SEGA5
		1		SEGA6
		2		SEGA7
		3		SEGA8
		4		
		5		
		6		BOOTE
		7		READ_MAGIC
	
	*/

	m_task = data;

	bankswitch();
}


//-------------------------------------------------
//  segment_r -
//-------------------------------------------------

READ8_MEMBER( abc1600_state::segment_r )
{
	/*
	
		bit		description
		
		0		SEGD0
		1		SEGD1
		2		SEGD2
		3		SEGD3
		4		SEGD4
		5		SEGD5
		6		SEGD6
		7		READ_MAGIC
	
	*/

	UINT16 sega = ((m_task & 0x0f) << 4) | ((offset >> 15) & 0x0f);
	UINT8 segd = m_segment_ram[sega];

	return (m_task & 0x80) | (segd & 0x7f);
}


//-------------------------------------------------
//  segment_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::segment_w )
{
	/*
	
		bit		description
		
		0		SEGD0
		1		SEGD1
		2		SEGD2
		3		SEGD3
		4		SEGD4
		5		SEGD5
		6		SEGD6
		7		
	
	*/

	UINT16 sega = ((m_task & 0x0f) << 4) | ((offset >> 15) & 0x0f);
	m_segment_ram[sega] = data & 0x7f;

	bankswitch();
}


//-------------------------------------------------
//  page_r -
//-------------------------------------------------

READ8_MEMBER( abc1600_state::page_r )
{
	/*
	
		bit		description
		
		0		X11
		1		X12
		2		X13
		3		X14
		4		X15
		5		X16
		6		X17
		7		X18

		8		X19
		9		X20
		10
		11
		12
		13
		14		_WP
		15		NONX
	
	*/

	UINT16 sega = ((m_task & 0x0f) << 4) | ((offset >> 15) & 0x0f);
	UINT8 segd = m_segment_ram[sega];

	UINT16 pagea = (segd << 4) | ((offset >> 11) & 0x0f);
	UINT16 paged = m_page_ram[pagea];

	int a0 = BIT(offset, 0);

	return a0 ? (paged >> 8) : (paged & 0xff);
}


//-------------------------------------------------
//  page_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::page_w )
{
	/*
	
		bit		description
		
		0		X11
		1		X12
		2		X13
		3		X14
		4		X15
		5		X16
		6		X17
		7		X18

		8		X19
		9		X20
		10
		11
		12
		13
		14		_WP
		15		NONX
	
	*/

	UINT16 sega = ((m_task & 0x0f) << 4) | ((offset >> 15) & 0x0f);
	UINT8 segd = m_segment_ram[sega];

	UINT16 pagea = (segd << 4) | ((offset >> 11) & 0x0f);

	int a0 = BIT(offset, 0);

	if (a0)
	{
		m_page_ram[pagea] = (data << 8) | (m_page_ram[pagea] & 0xff);
	}
	else
	{
		m_page_ram[pagea] = (m_page_ram[pagea] & 0xff00) | data;
	}

	bankswitch();
}



//**************************************************************************
//  READ/WRITE HANDLERS
//**************************************************************************

//-------------------------------------------------
//  fw0_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::fw0_w )
{
	/*
	
		bit		description
		
		0		SEL1
		1		SEL2
		2		SEL3
		3		MOTOR
		4		LC/PC
		5		LC/PC
		6		
		7		
	
	*/
}


//-------------------------------------------------
//  fw1_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::fw1_w )
{
	/*
	
		bit		description
		
		0		MR
		1		DDEN
		2		HLT
		3		MINI
		4		HLD
		5		P0
		6		P1
		7		P2
	
	*/
}


//-------------------------------------------------
//  en_spec_contr_reg_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600_state::en_spec_contr_reg_w )
{
	/*
	
		bit		description
		
		0		CS7
		1		
		2		_BTCE
		3		_ATCE
		4		PARTST
		5		_DMADIS
		6		SYSSCC
		7		SYSFS
	
	*/
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( abc1600_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc1600_mem, ADDRESS_SPACE_PROGRAM, 8, abc1600_state )
	AM_RANGE(0x00000, 0x03fff) AM_ROM AM_REGION(MC68008P8_TAG, 0)
/*

	Supervisor Data map

	AM_RANGE(0x80000, 0x80001) AM_MIRROR(0x7f800) AM_MASK(0x7f800) AM_READWRITE(page_r, page_w)
	AM_RANGE(0x80003, 0x80003) AM_MIRROR(0x7f800) AM_MASK(0x7f800) AM_READWRITE(segment_r, segment_w)
	AM_RANGE(0x80004, 0x80004) AM_READ(cause_r)
	AM_RANGE(0x80007, 0x80007) AM_WRITE(task_w)


	Logical Address Map
	
	AM_RANGE(0x000000, 0x0fffff) AM_RAM
	AM_RANGE(0x100000, 0x17ffff) AM_RAM AM_MEMBER(m_video_ram)
	AM_RANGE(0x1ff000, 0x1ff007) AM_DEVREADWRITE_LEGACY(SAB1797_02P_TAG, wd17xx_r, wd17xx_w) // A2,A1
	AM_RANGE(0x1ff100, 0x1ff100) AM_DEVWRITE_LEGACY(SY6845E_TAG, mc6845_address_w)
	AM_RANGE(0x1ff101, 0x1ff101) AM_DEVREADWRITE_LEGACY(SY6845E_TAG, mc6845_register_r, mc6845_register_w)
	AM_RANGE(0x1ff200, 0x1ff207) AM_DEVREADWRITE_LEGACY(Z80DART_TAG, z80dart_ba_cd_r, z80dart_ba_cd_w) // A2,A1
	AM_RANGE(0x1ff600, 0x1ff607) AM_DEVREADWRITE(Z8530B1_TAG, scc8530_r, scc8530_w) // A2,A1
	AM_RANGE(0x1ff700, 0x1ff707) AM_DEVREADWRITE(Z8536B1_TAG, z8536_r, z8536_w) // A2,A1
	AM_RANGE(0x1ff800, 0x1ff800) AM_READ(iord0_w)
	AM_RANGE(0x1ff800, 0x1ff807) AM_WRITE(iowr0_w)
	AM_RANGE(0x1ff900, 0x1ff907) AM_WRITE(iowr1_w)
	AM_RANGE(0x1ffa00, 0x1ffa07) AM_WRITE(iowr2_w)
	AM_RANGE(0x1ffb00, 0x1ffb00) AM_WRITE(fw0_w)
	AM_RANGE(0x1ffb01, 0x1ffb01) AM_WRITE(fw1_w)
	AM_RANGE(0x1ffd00, 0x1ffd07) AM_WRITE(dmamap_w)
	AM_RANGE(0x1ffe00, 0x1ffe00) AM_WRITE(en_spec_contr_reg_w)

	*/

/*
	AM_RANGE(0x1ff000, 0x1ff000) AM_DEVREADWRITE_LEGACY(Z8410AB1_0_TAG, z80dma_r, z80dma_w)
	AM_RANGE(0x1ff000, 0x1ff000) AM_DEVREADWRITE_LEGACY(Z8410AB1_1_TAG, z80dma_r, z80dma_w)
	AM_RANGE(0x1ff000, 0x1ff000) AM_DEVREADWRITE_LEGACY(Z8410AB1_2_TAG, z80dma_r, z80dma_w)
	AM_RANGE(0x1ff000, 0x1ff003) AM_DEVREADWRITE(E050_C16PC_TAG, e050c16pc_r, e050c16pc_w)
*/
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( abc1600 )
//-------------------------------------------------

static INPUT_PORTS_START( abc1600 )
INPUT_PORTS_END



//**************************************************************************
//  VIDEO
//**************************************************************************

//-------------------------------------------------
//  mc6845_interface crtc_intf
//-------------------------------------------------

static MC6845_UPDATE_ROW( abc1600_update_row )
{
}

static MC6845_ON_UPDATE_ADDR_CHANGED( crtc_update )
{
}

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
	crtc_update
};


//-------------------------------------------------
//  SCREEN_UPDATE( abc1600 )
//-------------------------------------------------

void abc1600_state::video_start()
{
	// allocate video RAM
	m_video_ram = auto_alloc_array(machine, UINT8, 128*1024);
}


//-------------------------------------------------
//  SCREEN_UPDATE( abc1600 )
//-------------------------------------------------

bool abc1600_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	mc6845_update(m_crtc, &bitmap, &cliprect);

	return 0;
}



//**************************************************************************
//  DEVICE CONFIGURATION
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

	DEVCB_DEVICE_LINE_MEMBER(ABC99_TAG, abc99_device, txd_r),
	DEVCB_DEVICE_LINE_MEMBER(ABC99_TAG, abc99_device, rxd_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_CPU_INPUT_LINE(MC68008P8_TAG, M68K_IRQ_5) // shared with SCC
};

//-------------------------------------------------
//  Z8536_INTERFACE( cio_intf )
//-------------------------------------------------

READ8_MEMBER( abc1600_state::cio_pa_r )
{
	/*
		
		bit		description
		
		PA0		BUS2
		PA1		BUS1
		PA2		BUS0X*2
		PA3		BUS0X*3
		PA4		BUS0X*4
		PA5		BUS0X*5
		PA6		BUS0X
		PA7		BUS0I
		
	*/
	
	return 0;
}

READ8_MEMBER( abc1600_state::cio_pb_r )
{
	/*
		
		bit		description
		
		PB0		
		PB1		POWERFAIL
		PB2		
		PB3		
		PB4		MINT
		PB5		_PREN-1
		PB6		_PREN-0
		PB7		FINT
		
	*/

	return 0;
}

WRITE8_MEMBER( abc1600_state::cio_pb_w )
{
	/*
		
		bit		description
		
		PB0		PRBR
		PB1		
		PB2		
		PB3		
		PB4		
		PB5		
		PB6		
		PB7		
		
	*/
}

READ8_MEMBER( abc1600_state::cio_pc_r )
{
	/*
		
		bit		description
		
		PC0		
		PC1		DATA IN
		PC2		
		PC3		
		
	*/

	UINT8 data = 0;

	// data in	
	data |= (e0516_dio_r(m_rtc) | m_nvram->do_r()) << 1;
	
	return 0;
}

WRITE8_MEMBER( abc1600_state::cio_pc_w )
{
	/*
		
		bit		description
		
		PC0		CLOCK
		PC1		DATA OUT
		PC2		RTC CS
		PC3		NVRAM CS
		
	*/
	
	int clock = BIT(data, 0);
	int data_out = BIT(data, 1);
	int rtc_cs = BIT(data, 2);
	int nvram_cs = BIT(data, 3);
	
	e0516_cs_w(m_rtc, rtc_cs);
	e0516_dio_w(m_rtc, data_out);
	e0516_clk_w(m_rtc, clock);
	
	m_nvram->cs_w(nvram_cs);
	m_nvram->di_w(data_out);
	m_nvram->sk_w(clock);
}

static Z8536_INTERFACE( cio_intf )
{
	DEVCB_CPU_INPUT_LINE(MC68008P8_TAG, M68K_IRQ_2),
	DEVCB_DRIVER_MEMBER(abc1600_state, cio_pa_r),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(abc1600_state, cio_pb_r),
	DEVCB_DRIVER_MEMBER(abc1600_state, cio_pb_w),
	DEVCB_DRIVER_MEMBER(abc1600_state, cio_pc_r),
	DEVCB_DRIVER_MEMBER(abc1600_state, cio_pc_w)
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


//-------------------------------------------------
//  ABC99_INTERFACE( abc99_intf )
//-------------------------------------------------

static ABC99_INTERFACE( abc99_intf )
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(Z8470AB1_TAG, z80dart_rxtxcb_w),
	DEVCB_DEVICE_LINE(Z8470AB1_TAG, z80dart_dcdb_w)
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( abc1600 )
//-------------------------------------------------

void abc1600_state::machine_start()
{
}



//**************************************************************************
//  MACHINE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( abc1600 )
//-------------------------------------------------

static MACHINE_CONFIG_START( abc1600, abc1600_state )
	// basic machine hardware
	MCFG_CPU_ADD(MC68008P8_TAG, M68008, XTAL_64MHz/8)
	MCFG_CPU_PROGRAM_MAP(abc1600_mem)

	// video hardware
    MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) // not accurate
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(1024, 768)
    MCFG_SCREEN_VISIBLE_AREA(0, 1024-1, 0, 768-1)
    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)
	MCFG_MC6845_ADD(SY6845E_TAG, SY6845E, XTAL_64MHz, crtc_intf)

	// devices
	MCFG_Z80DMA_ADD(Z8410AB1_0_TAG, 4000000, dma0_intf)
	MCFG_Z80DMA_ADD(Z8410AB1_1_TAG, 4000000, dma1_intf)
	MCFG_Z80DMA_ADD(Z8410AB1_2_TAG, 4000000, dma2_intf)
	MCFG_Z80DART_ADD(Z8470AB1_TAG, XTAL_64MHz/16, dart_intf)
	MCFG_SCC8530_ADD(Z8530B1_TAG, XTAL_64MHz/16)
	MCFG_Z8536_ADD(Z8536B1_TAG, XTAL_64MHz/16, cio_intf)
	MCFG_NMC9306_ADD(NMC9306_TAG)
	MCFG_E0516_ADD(E050_C16PC_TAG, XTAL_32_768kHz)
	MCFG_WD179X_ADD(SAB1797_02P_TAG, fdc_intf)
	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, abc1600_floppy_config)
	MCFG_ABC99_ADD(abc99_intf)
	MCFG_S1410_ADD()

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1M")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
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
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     INIT  COMPANY     FULLNAME     FLAGS
COMP( 1985, abc1600, 0,      0,      abc1600, abc1600, 0,    "Luxor", "ABC 1600", GAME_NOT_WORKING | GAME_NO_SOUND )
