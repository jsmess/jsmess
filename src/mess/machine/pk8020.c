/***************************************************************************

        PK-8020 driver by Miodrag Milanovic

        18/07/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "includes/pk8020.h"


/* Driver initialization */
DRIVER_INIT(pk8020)
{
	memset(mess_ram,0,128*1024);
}


MACHINE_RESET( pk8020 )
{
	const address_space *space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);	
	
	memory_install_read8_handler (space, 0x0000, 0x1fff, 0, 0, SMH_BANK1);
	memory_install_write8_handler(space, 0x0000, 0x7fff, 0, 0, SMH_UNMAP);
	memory_install_read8_handler (space, 0x2000, 0x37ff, 0, 0, SMH_BANK2);
	memory_install_write8_handler(space, 0x2000, 0x37ff, 0, 0, SMH_UNMAP);


	memory_install_read8_handler (space, 0x4000, 0x5fff, 0, 0, SMH_BANK7);
	memory_install_read8_handler (space, 0x6000, 0x7fff, 0, 0, SMH_BANK8);
	memory_install_read8_handler (space, 0x8000, 0xbeff, 0, 0, SMH_BANK9);
	memory_install_read8_handler (space, 0xbf00, 0xbfff, 0, 0, SMH_BANK10);
	memory_install_read8_handler (space, 0xc000, 0xf7ff, 0, 0, SMH_BANK11);
	memory_install_read8_handler (space, 0xf800, 0xf9ff, 0, 0, SMH_BANK12);
	memory_install_read8_handler (space, 0xfa00, 0xfaff, 0, 0, SMH_BANK13);
	memory_install_read8_handler (space, 0xfb00, 0xfbff, 0, 0, SMH_BANK14);
	memory_install_read8_handler (space, 0xfc00, 0xfdff, 0, 0, SMH_BANK15);
	memory_install_read8_handler (space, 0xfe00, 0xfeff, 0, 0, SMH_BANK16);
	memory_install_read8_handler (space, 0xff00, 0xffff, 0, 0, SMH_BANK17);

	memory_install_write8_handler(space, 0x4000, 0x5fff, 0, 0, SMH_BANK7);
	memory_install_write8_handler(space, 0x6000, 0x7fff, 0, 0, SMH_BANK8);
	memory_install_write8_handler(space, 0x8000, 0xbeff, 0, 0, SMH_BANK9);
	memory_install_write8_handler(space, 0xbf00, 0xbfff, 0, 0, SMH_BANK10);
	memory_install_write8_handler(space, 0xc000, 0xf7ff, 0, 0, SMH_BANK11);
	memory_install_write8_handler(space, 0xf800, 0xf9ff, 0, 0, SMH_BANK12);
	memory_install_write8_handler(space, 0xfa00, 0xfaff, 0, 0, SMH_BANK13);
	memory_install_write8_handler(space, 0xfb00, 0xfbff, 0, 0, SMH_BANK14);
	memory_install_write8_handler(space, 0xfc00, 0xfdff, 0, 0, SMH_BANK15);
	memory_install_write8_handler(space, 0xfe00, 0xfeff, 0, 0, SMH_BANK16);
	memory_install_write8_handler(space, 0xff00, 0xffff, 0, 0, SMH_BANK17);

	memory_set_bankptr(machine, 1, memory_region(machine,"main") + 0x10000);
	memory_set_bankptr(machine, 2, memory_region(machine,"main") + 0x12000);

	memory_set_bankptr(machine, 7, mess_ram + 0x4000);
	memory_set_bankptr(machine, 8, mess_ram + 0x6000);
	memory_set_bankptr(machine, 9, mess_ram + 0x8000);
	memory_set_bankptr(machine, 10, mess_ram + 0xbf00);
	memory_set_bankptr(machine, 11, mess_ram + 0xc000);
	memory_set_bankptr(machine, 12, mess_ram + 0xf800);
	memory_set_bankptr(machine, 13, mess_ram + 0xfa00);
	memory_set_bankptr(machine, 14, mess_ram + 0xfb00);
	memory_set_bankptr(machine, 15, mess_ram + 0xfc00);
	memory_set_bankptr(machine, 16, mess_ram + 0xfe00);
	memory_set_bankptr(machine, 17, mess_ram + 0xff00);
}
