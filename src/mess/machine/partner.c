/***************************************************************************

        Partner driver by Miodrag Milanovic

        09/06/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/8255ppi.h"
#include "machine/8257dma.h"
#include "video/i8275.h"
#include "includes/radio86.h"
#include "includes/partner.h"

static UINT8 partner_mem_page;
static UINT8 partner_win_mem_page;

/* Driver initialization */
DRIVER_INIT(partner)
{
	memset(mess_ram,0,64*1024);
	radio86_tape_value = 0x80;
}


static void partner_bank_switch(running_machine *machine)
{
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07ff, 0, 0, SMH_BANK1);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0800, 0x7fff, 0, 0, SMH_BANK2);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, SMH_BANK3);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xbfff, 0, 0, SMH_BANK4);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc7ff, 0, 0, SMH_BANK5);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc800, 0xd7ff, 0, 0, SMH_BANK6);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xe7ff, 0, 0, SMH_BANK7);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xffff, 0, 0, SMH_BANK8);

	switch(	partner_mem_page ) {
		case 0 :
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(1, memory_region(machine, REGION_CPU1) + 0x10000);
				memory_set_bankptr(2, mess_ram + 0x0800);
				memory_set_bankptr(3, mess_ram + 0x8000);
				memory_set_bankptr(4, mess_ram + 0xa000);
				memory_set_bankptr(5, mess_ram + 0xc000);
				memory_set_bankptr(6, mess_ram + 0xc800);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(7, memory_region(machine, REGION_CPU1) + 0x10000);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xffff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(8, memory_region(machine, REGION_CPU1) + 0x10800);
				break;
		case 1 :
				memory_set_bankptr(1, mess_ram + 0x0000);
				memory_set_bankptr(2, mess_ram + 0x0800);
				memory_set_bankptr(3, mess_ram + 0x8000);
				memory_set_bankptr(4, mess_ram + 0xa000);
				memory_set_bankptr(5, mess_ram + 0xc000);
				memory_set_bankptr(6, mess_ram + 0xc800);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(7, memory_region(machine, REGION_CPU1) + 0x10000);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xffff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(8, memory_region(machine, REGION_CPU1) + 0x10800);
		case 2 :
				memory_set_bankptr(1, mess_ram + 0x0000);
				memory_set_bankptr(2, mess_ram + 0x0800);
				memory_set_bankptr(3, mess_ram + 0x8000);
				memory_set_bankptr(4, mess_ram + 0xa000);
				memory_set_bankptr(5, mess_ram + 0xc000);
				memory_set_bankptr(6, mess_ram + 0xc800);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(7, memory_region(machine, REGION_CPU1) + 0x10000); // BIOS

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xffff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(8, memory_region(machine, REGION_CPU1) + 0x10800); // BIOS + 0x800
				break;
		case 3 :
				memory_set_bankptr(1, mess_ram + 0x0000);
				memory_set_bankptr(2, mess_ram + 0x0800);
				memory_set_bankptr(3, mess_ram + 0x8000);
				memory_set_bankptr(4, mess_ram + 0xa000);
				memory_set_bankptr(5, mess_ram + 0xc000);
				memory_set_bankptr(6, mess_ram + 0xc800);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(7, memory_region(machine, REGION_CPU1) + 0x10000);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xffff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(8, memory_region(machine, REGION_CPU1) + 0x10800);
				break;
		case 4 :
				memory_set_bankptr(1, mess_ram + 0x0000);
				memory_set_bankptr(2, mess_ram + 0x0800);
				memory_set_bankptr(3, mess_ram + 0x8000);
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xbfff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(5, mess_ram + 0xc000);
				memory_set_bankptr(6, mess_ram + 0xc800);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(7, memory_region(machine, REGION_CPU1) + 0x10000);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xffff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(8, memory_region(machine, REGION_CPU1) + 0x10800);
				break;
		case 5 :
				memory_set_bankptr(1, mess_ram + 0x0000);
				memory_set_bankptr(2, mess_ram + 0x0800);
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, SMH_UNMAP);
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xbfff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(5, mess_ram + 0xc000);
				memory_set_bankptr(6, mess_ram + 0xc800);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(7, memory_region(machine, REGION_CPU1) + 0x10000);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xffff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(8, memory_region(machine, REGION_CPU1) + 0x10800);
				break;
		case 6 :
				memory_set_bankptr(1, mess_ram + 0x0000);
				memory_set_bankptr(2, mess_ram + 0x0800);
				memory_set_bankptr(3, mess_ram + 0x8000);
				memory_set_bankptr(4, mess_ram + 0xa000);
				memory_set_bankptr(5, mess_ram + 0xc000);
				memory_set_bankptr(6, mess_ram + 0xc800);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(7, memory_region(machine, REGION_CPU1) + 0x10000);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xffff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(8, memory_region(machine, REGION_CPU1) + 0x10800);
				break;
		case 7 :
				memory_set_bankptr(1, mess_ram + 0x0000);
				memory_set_bankptr(2, mess_ram + 0x0800);
				memory_set_bankptr(3, mess_ram + 0x8000);
				memory_set_bankptr(4, mess_ram + 0xa000);
				memory_set_bankptr(5, mess_ram + 0xc000);
				memory_set_bankptr(6, mess_ram + 0xc800);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(7, memory_region(machine, REGION_CPU1) + 0x10000);

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xffff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(8, memory_region(machine, REGION_CPU1) + 0x10800);
				break;
		case 8 :
				memory_set_bankptr(1, mess_ram + 0x0000);
				memory_set_bankptr(2, mess_ram + 0x0800);
				memory_set_bankptr(3, mess_ram + 0x8000);
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xbfff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(4, memory_region(machine, REGION_CPU1) + 0x12000); // BASIC
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(5, memory_region(machine, REGION_CPU1) + 0x10000); // BIOS
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc800, 0xcfff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(6, memory_region(machine, REGION_CPU1) + 0x10000); // BIOS

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(7, memory_region(machine, REGION_CPU1) + 0x10000); // BIOS

				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xffff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(8, memory_region(machine, REGION_CPU1) + 0x10800); // BIOS + 0x800
				break;

	}
}

WRITE8_HANDLER ( partner_win_memory_page_w )
{
	partner_win_mem_page = ~data;
	partner_bank_switch(machine);
	logerror("change win page : %02x\n",partner_win_mem_page);
}

WRITE8_HANDLER (partner_mem_page_w )
{
	partner_mem_page = (data >> 4) & 0x0f;
	partner_bank_switch(machine);
	logerror("change page : %02x [%02x]\n",data,partner_mem_page);
}

MACHINE_RESET( partner )
{
	partner_mem_page = 0;
	partner_bank_switch(machine);
}
