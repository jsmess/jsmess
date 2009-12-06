/***************************************************************************
   
        Sage II

		For memory map look at :
			htpp://www.thebattles.net/sage/img/SDT.pdf  (pages 14-)
			
		
        06/12/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "machine/msm8251.h"
#include "machine/terminal.h"

static UINT16* sage2_ram;

static ADDRESS_MAP_START(sage2_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0007ffff) AM_RAM AM_BASE(&sage2_ram) // 512 KB RAM / ROM at boot
	AM_RANGE(0x00fe0000, 0x00feffff) AM_ROM AM_REGION("user1",0)	
//	AM_RANGE(0x00ffc070, 0x00ffc071 ) AM_DEVREADWRITE8("uart", msm8251_data_r,msm8251_data_w, 0xffff)
//	AM_RANGE(0x00ffc072, 0x00ffc073 ) AM_DEVREADWRITE8("uart", msm8251_status_r,msm8251_control_w, 0xffff)	
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( sage2 )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(sage2) 
{	
	UINT8* user1 = memory_region(machine, "user1");

	memcpy((UINT8*)sage2_ram,user1,0x2000);

	device_reset(cputag_get_cpu(machine, "maincpu"));
}

static WRITE8_DEVICE_HANDLER( sage2_kbd_put )
{
}

static GENERIC_TERMINAL_INTERFACE( sage2_terminal_intf )
{
	DEVCB_HANDLER(sage2_kbd_put)
};

static MACHINE_DRIVER_START( sage2 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M68000, XTAL_8MHz)
    MDRV_CPU_PROGRAM_MAP(sage2_mem)    

    MDRV_MACHINE_RESET(sage2)
	
    /* video hardware */
    MDRV_IMPORT_FROM( generic_terminal )	
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,sage2_terminal_intf)	
	
	/* uart */
	MDRV_MSM8251_ADD("uart", default_msm8251_interface)	
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( sage2 )
    ROM_REGION( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "sage2.u18", 0x0000, 0x1000, CRC(ca9b312d) SHA1(99436a6d166aa5280c3b2d28355c4d20528fe48c))
	ROM_LOAD16_BYTE( "sage2.u17", 0x0001, 0x1000, CRC(27e25045) SHA1(041cd9d4617473d089f31f18cbb375046c3b61bb))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   				FULLNAME       FLAGS */
COMP( 1982, sage2,  0,       0, 	sage2, 		sage2, 	 0,  	0,  	  "Sage Technology",   "Sage II",		GAME_NOT_WORKING)

