/***************************************************************************

        Ondra driver by Miodrag Milanovic

        08/09/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/ondra.h"


/* Driver initialization */
DRIVER_INIT(ondra)
{
	memset(mess_ram,0,64*1024);
}

static UINT8 ondra_video_enable;
static UINT8 ondra_bank1_status;
static UINT8 ondra_bank2_status;

static void ondra_update_banks(running_machine *machine)
{
	UINT8 *mem = memory_region(machine, "main");
	if (ondra_bank1_status==0) {
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0000, 0x07ff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0800, 0x0fff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x1000, 0x17ff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x1800, 0x1fff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x2000, 0x27ff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x2800, 0x2fff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x3000, 0x37ff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x3800, 0x3fff, 0, 0, SMH_UNMAP);

		memory_set_bankptr(machine, 1, mem + 0x010000);
		memory_set_bankptr(machine, 2, mem + 0x010000);
		memory_set_bankptr(machine, 3, mem + 0x010000);
		memory_set_bankptr(machine, 4, mem + 0x010000);
		memory_set_bankptr(machine, 5, mem + 0x010800);
		memory_set_bankptr(machine, 6, mem + 0x010800);
		memory_set_bankptr(machine, 7, mem + 0x010800);
		memory_set_bankptr(machine, 8, mem + 0x010800);
	} else {
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0000, 0x07ff, 0, 0, SMH_BANK1);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0800, 0x0fff, 0, 0, SMH_BANK2);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x1000, 0x17ff, 0, 0, SMH_BANK3);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x1800, 0x1fff, 0, 0, SMH_BANK4);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x2000, 0x27ff, 0, 0, SMH_BANK5);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x2800, 0x2fff, 0, 0, SMH_BANK6);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x3000, 0x37ff, 0, 0, SMH_BANK7);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x3800, 0x3fff, 0, 0, SMH_BANK8);

		memory_set_bankptr(machine, 1, mess_ram + 0x0000);
		memory_set_bankptr(machine, 2, mess_ram + 0x0800);
		memory_set_bankptr(machine, 3, mess_ram + 0x1000);
		memory_set_bankptr(machine, 4, mess_ram + 0x1800);
		memory_set_bankptr(machine, 5, mess_ram + 0x2000);
		memory_set_bankptr(machine, 6, mess_ram + 0x2800);
		memory_set_bankptr(machine, 7, mess_ram + 0x3000);
		memory_set_bankptr(machine, 8, mess_ram + 0x3800);
	}
	memory_set_bankptr(machine, 9, mess_ram + 0x4000);
	if (ondra_bank2_status==0) {
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xe000, 0xffff, 0, 0, SMH_BANK10);
		memory_install_read8_handler (cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xe000, 0xffff, 0, 0, SMH_BANK10);
		memory_set_bankptr(machine, 10, mess_ram + 0xe000);
	} else {
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xe000, 0xffff, 0, 0, SMH_UNMAP);
		memory_install_read8_handler (cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xe000, 0xffff, 0, 0, SMH_UNMAP);
	}
}

WRITE8_HANDLER( ondra_port_03_w )
{
	ondra_video_enable = data & 1;
	ondra_bank1_status = (data >> 1) & 1;
	ondra_bank2_status = (data >> 2) & 1;
	ondra_update_banks(space->machine);
}

WRITE8_HANDLER( ondra_port_09_w )
{

}

WRITE8_HANDLER( ondra_port_0a_w )
{
}


MACHINE_RESET( ondra )
{
	ondra_bank1_status = 0;
	ondra_bank2_status = 0;
	ondra_update_banks(machine);
}

MACHINE_START(ondra)
{
}
