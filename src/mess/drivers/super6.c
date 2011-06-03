/*

	ADC Super Six SBC
	
	Skeleton driver
	
*/

/*
	
	TODO:
	
	- software list
	- bankswitch
	- DMA
	- peripheral interfaces
	- baud select

*/

#include "includes/super6.h"


//**************************************************************************
//  MEMORY BANKING
//**************************************************************************

//-------------------------------------------------
//  bankswitch -
//-------------------------------------------------

void super6_state::bankswitch()
{
}


//-------------------------------------------------
//  s100_w - S-100 bus extended address A16-A23
//-------------------------------------------------

WRITE8_MEMBER( super6_state::s100_w )
{
	/*
		
		bit		description
		
		0		A16
		1		A17
		2		A18
		3		A19
		4		A20
		5		A21
		6		A22
		7		A23

	*/
	
	m_s100 = data;
}

//-------------------------------------------------
//  bank0_w - on-board memory control port #0
//-------------------------------------------------

WRITE8_MEMBER( super6_state::bank0_w )
{
	/*
		
		bit		description
		
		0		memory bank 0 (0000-3fff)
		1		memory bank 1 (4000-7fff)
		2		memory bank 2 (8000-bfff)
		3		memory bank 3 (c000-ffff)
		4		
		5		PROM enabled (0=enabled, 1=disabled)
		6		power on jump reset
		7		parity check enable

	*/
	
	m_bank0 = data;
	
	bankswitch();
}


//-------------------------------------------------
//  bank1_w - on-board memory control port #1
//-------------------------------------------------

WRITE8_MEMBER( super6_state::bank1_w )
{
	/*
		
		bit		description
		
		0		memory bank 4
		1		memory bank 5
		2		memory bank 6
		3		memory bank 7
		4		map select 0
		5		map select 1
		6		map select 2
		7		

	*/
	
	m_bank1 = data;
	
	bankswitch();
}



//**************************************************************************
//  PERIPHERALS
//**************************************************************************

//-------------------------------------------------
//  floppy_w - FDC synchronization/drive/density
//-------------------------------------------------

WRITE8_MEMBER( super6_state::fdc_w )
{
	/*
		
		bit		description
		
		0		disk drive select 0
		1		disk drive select 1
		2		head select (0=head 1, 1=head 2)
		3		disk density (0=single, 1=double)
		4		size select (0=8", 1=5.25")
		5		
		6		
		7		

	*/
	
	// disk drive select
	wd17xx_set_drive(m_fdc, data & 0x03);
	
	// head select
	wd17xx_set_side(m_fdc, !BIT(data, 2));
	
	// disk density
	wd17xx_dden_w(m_fdc, !BIT(data, 3));
}


//-------------------------------------------------
//  baud_w - baud rate
//-------------------------------------------------

WRITE8_MEMBER( super6_state::baud_w )
{
	/*
		
		bit		description
		
		0		
		1		
		2		
		3		
		4		
		5		
		6		
		7		

	*/
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( super6_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( super6_mem, AS_PROGRAM, 8, super6_state )
	AM_RANGE(0xf800, 0xffff) AM_ROM AM_REGION(Z80_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( super6_io )
//-------------------------------------------------

static ADDRESS_MAP_START( super6_io, AS_IO, 8, super6_state )
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE_LEGACY(Z80DART_TAG, z80dart_ba_cd_r, z80dart_ba_cd_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE_LEGACY(Z80PIO_TAG, z80pio_cd_ba_r, z80pio_cd_ba_w)
	AM_RANGE(0x08, 0x0c) AM_DEVREADWRITE_LEGACY(Z80CTC_TAG, z80ctc_r, z80ctc_w)
	AM_RANGE(0x0d, 0x0f) AM_DEVREADWRITE_LEGACY(WD2793_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x10, 0x10) AM_MIRROR(0x03) AM_DEVREADWRITE_LEGACY(Z80DMA_TAG, z80dma_r, z80dma_w)
	AM_RANGE(0x14, 0x14) AM_WRITE(fdc_w)
	AM_RANGE(0x15, 0x15) AM_READ_PORT("BAUD") AM_WRITE(s100_w)
	AM_RANGE(0x16, 0x16) AM_WRITE(bank0_w)
	AM_RANGE(0x17, 0x17) AM_WRITE(bank1_w)
	AM_RANGE(0x18, 0x18) AM_MIRROR(0x03) AM_WRITE(baud_w)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( super6 )
//-------------------------------------------------

static INPUT_PORTS_START( super6 )
	PORT_START("BAUD")
	PORT_DIPNAME( 0x01, 0x00, "Switch 1" ) PORT_DIPLOCATION("S1:1")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Switch 2" ) PORT_DIPLOCATION("S1:2")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Switch 3" ) PORT_DIPLOCATION("S1:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Switch 4" ) PORT_DIPLOCATION("S1:4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Switch 5" ) PORT_DIPLOCATION("S1:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Switch 6" ) PORT_DIPLOCATION("S1:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Switch 7" ) PORT_DIPLOCATION("S1:7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Double Sided Disk Drive" ) PORT_DIPLOCATION("S1:1")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  Z80CTC_INTERFACE( ctc_intf )
//-------------------------------------------------

static Z80CTC_INTERFACE( ctc_intf )
{
	0,
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
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

	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0)
};


//-------------------------------------------------
//  Z80DMA_INTERFACE( dma_intf )
//-------------------------------------------------

static Z80DMA_INTERFACE( dma_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_HALT),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  Z80PIO_INTERFACE( pio_intf )
//-------------------------------------------------

static Z80PIO_INTERFACE( pio_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  floppy_config super6_floppy_config
//-------------------------------------------------

static const floppy_config super6_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_5_25_DSHD,
    FLOPPY_OPTIONS_NAME(default),
    "floppy_5_25"
};


//-------------------------------------------------
//  wd17xx_interface fdc_intf
//-------------------------------------------------

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(Z80DMA_TAG, z80dma_device, rdy_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};


//-------------------------------------------------
//  Z80PIO_INTERFACE( pio_intf )
//-------------------------------------------------

static const z80_daisy_config super6_daisy_chain[] =
{
	{ NULL }
};


//-------------------------------------------------
//  GENERIC_TERMINAL_INTERFACE( terminal_intf )
//-------------------------------------------------

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_NULL
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( super6 )
//-------------------------------------------------

void super6_state::machine_start()
{
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( super6 )
//-------------------------------------------------

static MACHINE_CONFIG_START( super6, super6_state )
	// basic machine hardware
	MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_24MHz/4)
	MCFG_CPU_PROGRAM_MAP(super6_mem)
	MCFG_CPU_IO_MAP(super6_io)
	MCFG_CPU_CONFIG(super6_daisy_chain)

	// video hardware
	MCFG_FRAGMENT_ADD(generic_terminal)
	
	// devices
	MCFG_Z80CTC_ADD(Z80CTC_TAG, XTAL_24MHz/6, ctc_intf)
	MCFG_Z80DART_ADD(Z80DART_TAG, XTAL_24MHz/6, dart_intf)
	MCFG_Z80DMA_ADD(Z80DMA_TAG, XTAL_24MHz/6, dma_intf)
	MCFG_Z80PIO_ADD(Z80PIO_TAG, XTAL_24MHz/6, pio_intf)
	MCFG_WD2793_ADD(WD2793_TAG, fdc_intf)
	MCFG_FLOPPY_2_DRIVES_ADD(super6_floppy_config)
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("128K")

	// software list
	//MCFG_SOFTWARE_LIST_ADD("flop_list", "super6")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( super6 )
//-------------------------------------------------

ROM_START( super6 )
	ROM_REGION( 0x800, Z80_TAG, 0 )
	ROM_LOAD( "digitex monitor 1.2a 6oct1983.u29", 0x000, 0x800, CRC(a4c33ce4) SHA1(46dde43ea51d295f2b3202c2d0e1883bde1a8da7) )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT    INIT    COMPANY                          FULLNAME        FLAGS
COMP( 1983, super6,  0,      0,      super6,  super6,  0,      "Advanced Digital Corporation",	"Super Six",	GAME_NOT_WORKING | GAME_NO_SOUND_HW )
