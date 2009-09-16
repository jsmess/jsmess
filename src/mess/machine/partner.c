/***************************************************************************

        Partner driver by Miodrag Milanovic

        09/06/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/i8255a.h"
#include "machine/8257dma.h"
#include "machine/wd17xx.h"
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

static WD17XX_CALLBACK( partner_wd17xx_callback )
{
	const device_config *dma8257 = devtag_get_device(device->machine, "dma8257");
	switch(state)
	{
		case WD17XX_IRQ_CLR:
			break;
		case WD17XX_IRQ_SET:
			break;
		case WD17XX_DRQ_CLR:				
			break;
		case WD17XX_DRQ_SET:
			dma8257_drq_w(dma8257, 0, 1);
			break;
	}
}

const wd17xx_interface partner_wd17xx_interface = { partner_wd17xx_callback, NULL, {FLOPPY_0,FLOPPY_1,NULL,NULL} };

MACHINE_START(partner)
{
	const device_config *fdc = devtag_get_device(machine, "wd1793");
	wd17xx_set_density (fdc,DEN_MFM_HI);	
	wd17xx_set_pause_time(fdc,10);
}

static void partner_window_1(running_machine *machine, UINT8 bank, UINT16 offset,UINT8 *rom) 
{
	switch(partner_win_mem_page) {
	    case 2 : // FDD BIOS
				memory_set_bankptr(machine, bank, rom + 0x16000 + offset);
				break;
	    case 4 : // MCPG BIOS
				memory_set_bankptr(machine, bank, rom + 0x14000 + offset);
				break;
		default : // BIOS
				memory_set_bankptr(machine, bank, rom + 0x10000 + offset);
				break;
	}
}

static void partner_window_2(running_machine *machine, UINT8 bank, UINT16 offset,UINT8 *rom) 
{
	switch(partner_win_mem_page) {
	    case 4 : // MCPG FONT
				memory_set_bankptr(machine, bank, rom + 0x18000 + offset);
				break;
		default : // BIOS
				memory_set_bankptr(machine, bank, rom + 0x10000 + offset);
				break;
	}
}

static READ8_HANDLER ( partner_floppy_r ) {
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");
	
	if (offset<0x100) {
		switch(offset & 3) {
			case 0x00 : return wd17xx_status_r(fdc,0); 
			case 0x01 : return wd17xx_track_r(fdc,0);
			case 0x02 : return wd17xx_sector_r(fdc,0);
			default   :
						return wd17xx_data_r(fdc,0);
		}
	} else {
		return 0;
	}	
}

static WRITE8_HANDLER ( partner_floppy_w ) {
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");
	
	if (offset<0x100) {
		switch(offset & 3) {
			case 0x00 : wd17xx_command_w(fdc,0,data); break;
			case 0x01 : wd17xx_track_w(fdc,0,data);break;
			case 0x02 : wd17xx_sector_w(fdc,0,data);break;
			default   : wd17xx_data_w(fdc,0,data);break;
		}
	} else {
		if (((data >> 6) & 1)==1) {
			wd17xx_set_drive(fdc,0);
		}
		if (((data >> 3) & 1)==1) {
			wd17xx_set_drive(fdc,1);
		}		
		wd17xx_set_side(fdc,data >> 7);	 	
	}
}

static void partner_iomap_bank(running_machine *machine,UINT8 *rom)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	switch(partner_win_mem_page) {
		case 2 :
				// FDD
				memory_install_write8_handler(space, 0xdc00, 0xddff, 0, 0, partner_floppy_w);
				memory_install_read8_handler (space, 0xdc00, 0xddff, 0, 0, partner_floppy_r);
				break;
		case 4 : 
				// Timer
				break;
		default : // BIOS
				memory_set_bankptr(machine, 11, rom + 0x10000);
				break;
	}
}
static void partner_bank_switch(running_machine *machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *rom = memory_region(machine, "maincpu");
	
	memory_install_write8_handler(space, 0x0000, 0x07ff, 0, 0, SMH_BANK(1));
	memory_install_write8_handler(space, 0x0800, 0x3fff, 0, 0, SMH_BANK(2));
	memory_install_write8_handler(space, 0x4000, 0x5fff, 0, 0, SMH_BANK(3));
	memory_install_write8_handler(space, 0x6000, 0x7fff, 0, 0, SMH_BANK(4));
	memory_install_write8_handler(space, 0x8000, 0x9fff, 0, 0, SMH_BANK(5));
	memory_install_write8_handler(space, 0xa000, 0xb7ff, 0, 0, SMH_BANK(6));
	memory_install_write8_handler(space, 0xb800, 0xbfff, 0, 0, SMH_BANK(7));
	memory_install_write8_handler(space, 0xc000, 0xc7ff, 0, 0, SMH_BANK(8));
	memory_install_write8_handler(space, 0xc800, 0xcfff, 0, 0, SMH_BANK(9));
	memory_install_write8_handler(space, 0xd000, 0xd7ff, 0, 0, SMH_BANK(10));
	memory_install_write8_handler(space, 0xdc00, 0xddff, 0, 0, SMH_UNMAP);
	memory_install_read8_handler (space, 0xdc00, 0xddff, 0, 0, SMH_BANK(11));
	memory_install_write8_handler(space, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP);
	memory_install_write8_handler(space, 0xe800, 0xffff, 0, 0, SMH_UNMAP);

	// BANK 1 (0x0000 - 0x07ff)
	if (partner_mem_page==0) {
		memory_install_write8_handler(space, 0x0000, 0x07ff, 0, 0, SMH_UNMAP);
		memory_set_bankptr(machine, 1, rom + 0x10000);
	} else {
		if (partner_mem_page==7) {
			memory_set_bankptr(machine, 1, mess_ram + 0x8000);
		} else {
			memory_set_bankptr(machine, 1, mess_ram + 0x0000);
		}
	}

    // BANK 2 (0x0800 - 0x3fff)
	if (partner_mem_page==7) {
		memory_set_bankptr(machine, 2, mess_ram + 0x8800);
	} else {
		memory_set_bankptr(machine, 2, mess_ram + 0x0800);
	}

    // BANK 3 (0x4000 - 0x5fff)
	if (partner_mem_page==7) {
		memory_set_bankptr(machine, 3, mess_ram + 0xC000);
	} else {
		if (partner_mem_page==10) {
			//window 1
			memory_install_write8_handler(space, 0x4000, 0x5fff, 0, 0, SMH_UNMAP);
			partner_window_1(machine, 3, 0, rom);
		} else {
			memory_set_bankptr(machine, 3, mess_ram + 0x4000);
		}
	}

    // BANK 4 (0x6000 - 0x7fff)
	if (partner_mem_page==7) {
		memory_set_bankptr(machine, 4, mess_ram + 0xe000);
	} else {
		memory_set_bankptr(machine, 4, mess_ram + 0x6000);
	}

    // BANK 5 (0x8000 - 0x9fff)
	switch (partner_mem_page) {
		case 5:
		case 10: 
				//window 2
				memory_install_write8_handler(space, 0x8000, 0x9fff, 0, 0, SMH_UNMAP);
				partner_window_2(machine, 5, 0, rom);
				break;
		case 8:
		case 9: 
				//window 1
				memory_install_write8_handler(space, 0x8000, 0x9fff, 0, 0, SMH_UNMAP);
				partner_window_1(machine, 5, 0, rom);
				break;
		case 7: 
				memory_set_bankptr(machine, 5, mess_ram + 0x0000);
				break;
		default: 				
				memory_set_bankptr(machine, 5, mess_ram + 0x8000);
				break;
	}
    
    // BANK 6 (0xa000 - 0xb7ff)
	switch (partner_mem_page) {
		case 5:
		case 10: 
				//window 2
				memory_install_write8_handler(space, 0xa000, 0xb7ff, 0, 0, SMH_UNMAP);
				partner_window_2(machine, 6, 0, rom);
				break;
		case 6:
		case 8: 
				//BASIC
				memory_install_write8_handler(space, 0xa000, 0xb7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(machine, 6, rom + 0x12000); // BASIC				
				break;
		case 7: 
				memory_set_bankptr(machine, 6, mess_ram + 0x2000);
				break;
		default: 				
				memory_set_bankptr(machine, 6, mess_ram + 0xa000);
				break;
	}    

    // BANK 7 (0xb800 - 0xbfff)
	switch (partner_mem_page) {
		case 4:
		case 5:
		case 10: 
				//window 2
				memory_install_write8_handler(space, 0xb800, 0xbfff, 0, 0, SMH_UNMAP);
				partner_window_2(machine, 7, 0x1800, rom);
				break;
		case 6:
		case 8: 
				//BASIC
				memory_install_write8_handler(space, 0xb800, 0xbfff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(machine, 7, rom + 0x13800); // BASIC				
				break;
		case 7: 
				memory_set_bankptr(machine, 7, mess_ram + 0x3800);
				break;
		default: 				
				memory_set_bankptr(machine, 7, mess_ram + 0xb800);
				break;
	}    
	
    // BANK 8 (0xc000 - 0xc7ff)
	switch (partner_mem_page) {
		case 7:
				memory_set_bankptr(machine, 8, mess_ram + 0x4000);
				break;
		case 8:
		case 10:
				memory_install_write8_handler(space, 0xc000, 0xc7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(machine, 8, rom + 0x10000);
				break;
		default:
				memory_set_bankptr(machine, 8, mess_ram + 0xc000);
				break;			
	}

    // BANK 9 (0xc800 - 0xcfff)
	switch (partner_mem_page) {
		case 7:
				memory_set_bankptr(machine, 9, mess_ram + 0x4800);
				break;
		case 8:
		case 9:
				// window 2
				memory_install_write8_handler(space, 0xc800, 0xcfff, 0, 0, SMH_UNMAP);
				partner_window_2(machine, 9, 0, rom);
				break;
		case 10:
				memory_install_write8_handler(space, 0xc800, 0xcfff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(machine, 9, rom + 0x10800);
				break;
		default:
				memory_set_bankptr(machine, 9, mess_ram + 0xc800);
				break;			
	}

    // BANK 10 (0xd000 - 0xd7ff)
	switch (partner_mem_page) {
		case 7:
				memory_set_bankptr(machine, 10, mess_ram + 0x5000);
				break;
		case 8:
		case 9:
				// window 2
				memory_install_write8_handler(space, 0xd000, 0xd7ff, 0, 0, SMH_UNMAP);
				partner_window_2(machine, 10, 0x0800, rom);
				break;
		default:
				memory_set_bankptr(machine, 10, mess_ram + 0xd000);
				break;			
	}

	// BANK 11 (0xdc00 - 0xddff)
	partner_iomap_bank(machine,rom);
	
    // BANK 12 (0xe000 - 0xe7ff)
	if (partner_mem_page==1) {
		memory_set_bankptr(machine, 12, rom + 0x10000);
	} else {
		//window 1
		partner_window_1(machine, 12, 0, rom);
	}
	
    // BANK 13 (0xe800 - 0xffff)
	switch (partner_mem_page) {
		case 3:
		case 4:
		case 5:
				// window 1
				partner_window_1(machine, 13, 0x800, rom);
				break;
		default:		
				// BIOS		
				memory_set_bankptr(machine, 13, rom + 0x10800);
				break;
	}	
}

WRITE8_HANDLER ( partner_win_memory_page_w )
{
	partner_win_mem_page = ~data;
	partner_bank_switch(space->machine);
}

WRITE8_HANDLER (partner_mem_page_w )
{
	partner_mem_page = (data >> 4) & 0x0f;
	partner_bank_switch(space->machine);
}

static WRITE8_DEVICE_HANDLER(partner_dma_write_byte)
{
	memory_write_byte(cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM), offset, data);
}

static READ8_DEVICE_HANDLER ( partner_wd17xx_data_r )
{
	const device_config *fdc = devtag_get_device(device->machine, "wd1793");
	return wd17xx_data_r(fdc, offset);
}

static WRITE8_DEVICE_HANDLER ( partner_wd17xx_data_w )
{
	const device_config *fdc = devtag_get_device(device->machine, "wd1793");
	wd17xx_data_w(fdc, offset, data);
}

const dma8257_interface partner_dma =
{
	0,

	radio86_dma_read_byte,
	partner_dma_write_byte,

	{ partner_wd17xx_data_r,  0, 0, 0 },
	{ partner_wd17xx_data_w, 0, radio86_write_video, 0 },
	{ 0, 0, 0, 0 }
};


MACHINE_RESET( partner )
{
	partner_mem_page = 0;
	partner_win_mem_page = 0;
	partner_bank_switch(machine);
}
