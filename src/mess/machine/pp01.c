/***************************************************************************

        PP-01 driver by Miodrag Milanovic

        08/09/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "includes/pp01.h"


/* Driver initialization */
DRIVER_INIT(pp01)
{
	memset(mess_ram,0,64*1024);
}


MACHINE_RESET( pp01 )
{
	UINT8 *mem = memory_region(machine, "main");
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);

	memory_set_bankptr(1, mem + 0x010000);
	memory_set_bankptr(2, mess_ram + 0x4000);
	memory_set_bankptr(3, mess_ram + 0x8000);
	memory_set_bankptr(4, mem + 0x010000);

}

MACHINE_START(pp01)
{
}

