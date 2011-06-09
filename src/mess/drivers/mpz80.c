/*

    Morrow MPZ80

    Skeleton driver

*/

/*

    TODO:

	- everything
	- AM9512 FPU

*/

#include "includes/mpz80.h"



//**************************************************************************
//  READ/WRITE HANDLERS
//**************************************************************************

//-------------------------------------------------
//  trap_addr_r - trap address register
//-------------------------------------------------

READ8_MEMBER( mpz80_state::trap_addr_r )
{
	/*
		
		bit		description
		
		0		DADDR 12
		1		DADDR 13
		2		DADDR 14
		3		DADDR 15
		4		I-ADDR 12
		5		I-ADDR 13
		6		I-ADDR 14
		7		I-ADDR 15
		
	*/

	return 0;
}


//-------------------------------------------------
//  status_r - trap status register
//-------------------------------------------------

READ8_MEMBER( mpz80_state::status_r )
{
	/*
		
		bit		description
		
		0		_TRAP VOID
		1		_IORQ
		2		_TRAP HALT
		3		_TRAP INT
		4		_TRAP STOP
		5		_TRAP AUX
		6		RIO
		7		_RD STB
		
	*/

	return 0;
}


//-------------------------------------------------
//  keyboard_r -
//-------------------------------------------------

READ8_MEMBER( mpz80_state::keyboard_r )
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

	return 0;
}


//-------------------------------------------------
//  switch_r - switch register
//-------------------------------------------------

READ8_MEMBER( mpz80_state::switch_r )
{
	/*
		
		bit		description
		
		0		_TRAP RESET
		1		INT PEND
		2		SW1-6
		3		SW1-5
		4		SW1-4
		5		SW1-3
		6		SW1-2
		7		SW1-1
		
	*/

	return 0;
}


//-------------------------------------------------
//  disp_seg_w -
//-------------------------------------------------

WRITE8_MEMBER( mpz80_state::disp_seg_w )
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


//-------------------------------------------------
//  disp_col_w -
//-------------------------------------------------

WRITE8_MEMBER( mpz80_state::disp_col_w )
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


//-------------------------------------------------
//  task_w - task register
//-------------------------------------------------

WRITE8_MEMBER( mpz80_state::task_w )
{
	/*
		
		bit		description
		
		0		T0
		1		T1
		2		T2
		3		T3
		4		T4
		5		T5
		6		T6
		7		T7
		
	*/
}


//-------------------------------------------------
//  mask_w - mask register
//-------------------------------------------------

WRITE8_MEMBER( mpz80_state::mask_w )
{
	/*
		
		bit		description
		
		0		_STOP ENBL
		1		AUX ENBL
		2		_TINT ENBL
		3		RUN ENBL
		4		_HALT ENBL
		5		SINT ENBL
		6		_IOENBL
		7		_ZIO MODE
		
	*/
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( mpz80_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( mpz80_mem, AS_PROGRAM, 8, mpz80_state )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x0400) AM_MIRROR(0x3fc) AM_READWRITE(trap_addr_r, disp_seg_w)
	AM_RANGE(0x0401, 0x0401) AM_MIRROR(0x3fc) AM_READWRITE(keyboard_r, disp_col_w)
	AM_RANGE(0x0402, 0x0402) AM_MIRROR(0x3fc) AM_READWRITE(switch_r, task_w)
	AM_RANGE(0x0403, 0x0403) AM_MIRROR(0x3fc) AM_READWRITE(status_r, mask_w)
	AM_RANGE(0x0800, 0x0bff) AM_ROM AM_REGION(Z80_TAG, 0)
//	AM_RANGE(0x0c00, 0x0c00) AM_MIRROR(0x3ff) AM_DEVREADWRITE(AM9512_TAG, am9512_device, read, write)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( mpz80_io )
//-------------------------------------------------

static ADDRESS_MAP_START( mpz80_io, AS_IO, 8, mpz80_state )
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( mpz80 )
//-------------------------------------------------

static INPUT_PORTS_START( mpz80 )
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  GENERIC_TERMINAL_INTERFACE( terminal_intf )
//-------------------------------------------------

static WRITE8_DEVICE_HANDLER( dummy_w )
{
	// handled in Z80DART_INTERFACE
}

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_HANDLER(dummy_w)
};


//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( mpz80 )
//-------------------------------------------------

void mpz80_state::machine_start()
{
}


//-------------------------------------------------
//  MACHINE_RESET( mpz80 )
//-------------------------------------------------

void mpz80_state::machine_reset()
{
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( mpz80 )
//-------------------------------------------------

static MACHINE_CONFIG_START( mpz80, mpz80_state )
	// basic machine hardware
	MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(mpz80_mem)
	MCFG_CPU_IO_MAP(mpz80_io)

	// video hardware
	MCFG_FRAGMENT_ADD( generic_terminal )

	// devices
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1K")

	// software list
	//MCFG_SOFTWARE_LIST_ADD("flop_list", "mpz80")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( mpz80 )
//-------------------------------------------------

ROM_START( mpz80 )
	ROM_REGION( 0x1000, Z80_TAG, 0 )
	ROM_DEFAULT_BIOS("447")
	ROM_SYSTEM_BIOS( 0, "373", "3.73" )
	ROMX_LOAD( "mpz80-mon-3.73_fb34.17c", 0x0000, 0x1000, CRC(0bbffaec) SHA1(005ba726fc071f06cb1c969d170960438a3fc1a8), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "375", "3.75" )
	ROMX_LOAD( "mpz80-mon-3.75_0706.17c", 0x0000, 0x1000, CRC(1118a592) SHA1(d70f94c09602cd0bdc4fbaeb14989e8cc1540960), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "447", "4.47" )
	ROMX_LOAD( "mpz80-mon-4.47_f4f6.17c", 0x0000, 0x1000, CRC(b99c5d7f) SHA1(11181432ee524c7e5a68ead0671fc945256f5d1b), ROM_BIOS(3) )

	ROM_REGION( 0x20, "proms", 0 )
	ROM_LOAD( "z80-2_15a_82s123.15a", 0x00, 0x20, CRC(8a84249d) SHA1(dfbc49c5944f110f48419fd893fa84f4f0e113b8) ) // this is actually the 6331 PROM?

	ROM_REGION( 0x20, "plds", 0 )
	ROM_LOAD( "pal16r4.5c", 0x00, 0x20, NO_DUMP )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT    INIT    COMPANY                          FULLNAME        FLAGS
COMP( 1980, mpz80,  0,      0,      mpz80,  mpz80,  0,      "Morrow",	"MPZ80",	GAME_NOT_WORKING | GAME_NO_SOUND_HW )
