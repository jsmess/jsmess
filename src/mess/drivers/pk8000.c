/***************************************************************************
   
        PK-8000

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "machine/8255ppi.h"

static void pk8000_set_bank(running_machine *machine,UINT8 data) 
{ 
	UINT8 *rom = memory_region(machine, "maincpu");
	UINT8 block1 = data & 3;
	UINT8 block2 = (data >> 2) & 3;
	UINT8 block3 = (data >> 4) & 3;
	UINT8 block4 = (data >> 6) & 3;
	
	switch(block1) {
		case 0:
				memory_set_bankptr(machine, 1, rom + 0x10000);
				memory_set_bankptr(machine, 5, mess_ram);
				break;
		case 1: break;
		case 2: break;
		case 3:				
				memory_set_bankptr(machine, 1, mess_ram);
				memory_set_bankptr(machine, 5, mess_ram);
				break;
	}
	
	switch(block2) {
		case 0:				
				memory_set_bankptr(machine, 2, rom + 0x14000);
				memory_set_bankptr(machine, 6, mess_ram + 0x4000);
				break;
		case 1: break;
		case 2: break;
		case 3:
				memory_set_bankptr(machine, 2, mess_ram + 0x4000);
				memory_set_bankptr(machine, 6, mess_ram + 0x4000);
				break;
	}
	switch(block3) {
		case 0:
				memory_set_bankptr(machine, 3, rom + 0x18000);
				memory_set_bankptr(machine, 7, mess_ram + 0x8000);
				break;
		case 1: break;
		case 2: break;
		case 3:
				memory_set_bankptr(machine, 3, mess_ram + 0x8000);
				memory_set_bankptr(machine, 7, mess_ram + 0x8000);
				break;
	}
	switch(block4) {
		case 0:
				memory_set_bankptr(machine, 4, rom + 0x1c000);
				memory_set_bankptr(machine, 8, mess_ram + 0xc000);
				break;
		case 1: break;
		case 2: break;
		case 3:
				memory_set_bankptr(machine, 4, mess_ram + 0xc000);
				memory_set_bankptr(machine, 8, mess_ram + 0xc000);
				break;
	}						
}
static WRITE8_DEVICE_HANDLER(pk8000_80_porta_w)
{
	pk8000_set_bank(device->machine,data);
}

const ppi8255_interface pk8000_ppi8255_interface_1 =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(pk8000_80_porta_w),
	DEVCB_NULL,
	DEVCB_NULL
};

static ADDRESS_MAP_START(pk8000_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x3fff ) AM_READWRITE(SMH_BANK(1), SMH_BANK(5))
	AM_RANGE( 0x4000, 0x7fff ) AM_READWRITE(SMH_BANK(2), SMH_BANK(6))
	AM_RANGE( 0x8000, 0xbfff ) AM_READWRITE(SMH_BANK(3), SMH_BANK(7))
	AM_RANGE( 0xc000, 0xffff ) AM_READWRITE(SMH_BANK(4), SMH_BANK(8))
ADDRESS_MAP_END

static ADDRESS_MAP_START( pk8000_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x80, 0x83) AM_DEVREADWRITE("ppi8255_1", ppi8255_r, ppi8255_w)
	//AM_RANGE(0x84, 0x87) AM_DEVREADWRITE("ppi8255_2", ppi8255_r, ppi8255_w)
	//AM_RANGE(0x88, 0x8b) AM_DEVREADWRITE("ppi8255_3", ppi8255_r, ppi8255_w)
	//AM_RANGE(0x8c, 0x8f) AM_DEVREADWRITE("ppi8255_4", ppi8255_r, ppi8255_w)
ADDRESS_MAP_END

/*	 Input ports */
static INPUT_PORTS_START( pk8000 )
INPUT_PORTS_END


static MACHINE_RESET(pk8000) 
{
	pk8000_set_bank(machine,0);	
}

static VIDEO_START( pk8000 )
{
}

static VIDEO_UPDATE( pk8000 )
{
    return 0;
}

static MACHINE_DRIVER_START( pk8000 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",8080, 1780000)
    MDRV_CPU_PROGRAM_MAP(pk8000_mem)
    MDRV_CPU_IO_MAP(pk8000_io)	

    MDRV_MACHINE_RESET(pk8000)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(pk8000)
    MDRV_VIDEO_UPDATE(pk8000)
    
    MDRV_PPI8255_ADD( "ppi8255_1", pk8000_ppi8255_interface_1 )
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(pk8000)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( vesta )
  ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "vesta.rom", 0x10000, 0x4000, CRC(fbf7e2cc) SHA1(4bc5873066124bd926c3c6aa2fd1a062c87af339))
ROM_END

ROM_START( hobby )
  ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "hobby.rom", 0x10000, 0x4000, CRC(a25b4b2c) SHA1(0d86e6e4be8d1aa389bfa9dd79e3604a356729f7))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1987, hobby,  0,   		 0, 	pk8000, 	pk8000, 	 0,  	  pk8000,  	 "BP EVM",   "PK8000 Sura/Hobby",		GAME_NOT_WORKING)
COMP( 1988, vesta,  hobby,       0, 	pk8000, 	pk8000, 	 0,  	  pk8000,  	 "BP EVM",   "PK8000 Vesta",		GAME_NOT_WORKING)

