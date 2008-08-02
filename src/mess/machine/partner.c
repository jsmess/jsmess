/***************************************************************************

        Partner driver by Miodrag Milanovic

        09/06/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "devices/basicdsk.h"
#include "machine/8255ppi.h"
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

static void partner_wd17xx_callback(running_machine *machine, wd17xx_state_t state, void *param)
{
	const device_config *dma8257 = device_list_find_by_tag(machine->config->devicelist, DMA8257, "dma8257");
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
MACHINE_START(partner)
{
	wd17xx_init(machine, WD_TYPE_1793, partner_wd17xx_callback , NULL);
	wd17xx_set_density (DEN_MFM_HI);	
	wd17xx_set_pause_time(10);
}

DEVICE_IMAGE_LOAD( partner_floppy )
{
	int size;

	if (! image_has_been_created(image))
		{
		size = image_length(image);

		switch (size)
			{
			case 800*1024:
				break;
			default:
				return INIT_FAIL;
			}
		}
	else
		return INIT_FAIL;

	if (device_load_basicdsk_floppy (image) != INIT_PASS)
		return INIT_FAIL;

	basicdsk_set_geometry (image, 80, 2, 5, 1024, 0, 0, FALSE);
	return INIT_PASS;
}

static void partner_window_1(running_machine *machine, UINT8 bank, UINT16 offset,UINT8 *rom) 
{
	switch(partner_win_mem_page) {
	    case 2 : // FDD BIOS
				memory_set_bankptr(bank, rom + 0x16000 + offset);
				break;
	    case 4 : // MCPG BIOS
				memory_set_bankptr(bank, rom + 0x14000 + offset);
				break;
		default : // BIOS
				memory_set_bankptr(bank, rom + 0x10000 + offset);
				break;
	}
}

static void partner_window_2(running_machine *machine, UINT8 bank, UINT16 offset,UINT8 *rom) 
{
	switch(partner_win_mem_page) {
	    case 4 : // MCPG FONT
				memory_set_bankptr(bank, rom + 0x18000 + offset);
				break;
		default : // BIOS
				memory_set_bankptr(bank, rom + 0x10000 + offset);
				break;
	}
}

READ8_HANDLER ( partner_floppy_r ) {
	if (offset<0x100) {
		switch(offset & 3) {
			case 0x00 : return wd17xx_status_r(machine,0); 
			case 0x01 : return wd17xx_track_r(machine,0);
			case 0x02 : return wd17xx_sector_r(machine,0);
			default   :
						return wd17xx_data_r(machine,0);
		}
	} else {
		return 0;
	}	
}

WRITE8_HANDLER ( partner_floppy_w ) {
	if (offset<0x100) {
		switch(offset & 3) {
			case 0x00 : wd17xx_command_w(machine,0,data); break;
			case 0x01 : wd17xx_track_w(machine,0,data);break;
			case 0x02 : wd17xx_sector_w(machine,0,data);break;
			default   : wd17xx_data_w(machine,0,data);break;
		}
	} else {
		if (((data >> 6) & 1)==1) {
			wd17xx_set_drive(0);
		}
		if (((data >> 3) & 1)==1) {
			wd17xx_set_drive(1);
		}		
		wd17xx_set_side(data >> 7);	 	
	}
}

static void partner_iomap_bank(running_machine *machine,UINT8 *rom)
{
	switch(partner_win_mem_page) {
		case 2 :
				// FDD
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xdc00, 0xddff, 0, 0, partner_floppy_w);
				memory_install_read8_handler (machine, 0, ADDRESS_SPACE_PROGRAM, 0xdc00, 0xddff, 0, 0, partner_floppy_r);
				break;
		case 4 : 
				// Timer
				break;
		default : // BIOS
				memory_set_bankptr(11, rom + 0x10000);
				break;
	}
}
static void partner_bank_switch(running_machine *machine)
{
	UINT8 *rom = memory_region(machine, "|");
	
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07ff, 0, 0, SMH_BANK1);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0800, 0x3fff, 0, 0, SMH_BANK2);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, SMH_BANK3);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, SMH_BANK4);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, SMH_BANK5);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xb7ff, 0, 0, SMH_BANK6);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb800, 0xbfff, 0, 0, SMH_BANK7);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc7ff, 0, 0, SMH_BANK8);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc800, 0xcfff, 0, 0, SMH_BANK9);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xd7ff, 0, 0, SMH_BANK10);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xdc00, 0xddff, 0, 0, SMH_UNMAP);
	memory_install_read8_handler (machine, 0, ADDRESS_SPACE_PROGRAM, 0xdc00, 0xddff, 0, 0, SMH_BANK11);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xe7ff, 0, 0, SMH_UNMAP);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xffff, 0, 0, SMH_UNMAP);

	// BANK 1 (0x0000 - 0x07ff)
	if (partner_mem_page==0) {
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07ff, 0, 0, SMH_UNMAP);
		memory_set_bankptr(1, rom + 0x10000);
	} else {
		if (partner_mem_page==7) {
			memory_set_bankptr(1, mess_ram + 0x8000);
		} else {
			memory_set_bankptr(1, mess_ram + 0x0000);
		}
	}

    // BANK 2 (0x0800 - 0x3fff)
	if (partner_mem_page==7) {
		memory_set_bankptr(2, mess_ram + 0x8800);
	} else {
		memory_set_bankptr(2, mess_ram + 0x0800);
	}

    // BANK 3 (0x4000 - 0x5fff)
	if (partner_mem_page==7) {
		memory_set_bankptr(3, mess_ram + 0xC000);
	} else {
		if (partner_mem_page==10) {
			//window 1
			memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, SMH_UNMAP);
			partner_window_1(machine, 3, 0, rom);
		} else {
			memory_set_bankptr(3, mess_ram + 0x4000);
		}
	}

    // BANK 4 (0x6000 - 0x7fff)
	if (partner_mem_page==7) {
		memory_set_bankptr(4, mess_ram + 0xe000);
	} else {
		memory_set_bankptr(4, mess_ram + 0x6000);
	}

    // BANK 5 (0x8000 - 0x9fff)
	switch (partner_mem_page) {
		case 5:
		case 10: 
				//window 2
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, SMH_UNMAP);
				partner_window_2(machine, 5, 0, rom);
				break;
		case 8:
		case 9: 
				//window 1
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, SMH_UNMAP);
				partner_window_1(machine, 5, 0, rom);
				break;
		case 7: 
				memory_set_bankptr(5, mess_ram + 0x0000);
				break;
		default: 				
				memory_set_bankptr(5, mess_ram + 0x8000);
				break;
	}
    
    // BANK 6 (0xa000 - 0xb7ff)
	switch (partner_mem_page) {
		case 5:
		case 10: 
				//window 2
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xb7ff, 0, 0, SMH_UNMAP);
				partner_window_2(machine, 6, 0, rom);
				break;
		case 6:
		case 8: 
				//BASIC
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xb7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(6, rom + 0x12000); // BASIC				
				break;
		case 7: 
				memory_set_bankptr(6, mess_ram + 0x2000);
				break;
		default: 				
				memory_set_bankptr(6, mess_ram + 0xa000);
				break;
	}    

    // BANK 7 (0xb800 - 0xbfff)
	switch (partner_mem_page) {
		case 4:
		case 5:
		case 10: 
				//window 2
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb800, 0xbfff, 0, 0, SMH_UNMAP);
				partner_window_2(machine, 7, 0x1800, rom);
				break;
		case 6:
		case 8: 
				//BASIC
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb800, 0xbfff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(7, rom + 0x13800); // BASIC				
				break;
		case 7: 
				memory_set_bankptr(7, mess_ram + 0x3800);
				break;
		default: 				
				memory_set_bankptr(7, mess_ram + 0xb800);
				break;
	}    
	
    // BANK 8 (0xc000 - 0xc7ff)
	switch (partner_mem_page) {
		case 7:
				memory_set_bankptr(8, mess_ram + 0x4000);
				break;
		case 8:
		case 10:
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc7ff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(8, rom + 0x10000);
				break;
		default:
				memory_set_bankptr(8, mess_ram + 0xc000);
				break;			
	}

    // BANK 9 (0xc800 - 0xcfff)
	switch (partner_mem_page) {
		case 7:
				memory_set_bankptr(9, mess_ram + 0x4800);
				break;
		case 8:
		case 9:
				// window 2
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc800, 0xcfff, 0, 0, SMH_UNMAP);
				partner_window_2(machine, 9, 0, rom);
				break;
		case 10:
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc800, 0xcfff, 0, 0, SMH_UNMAP);
				memory_set_bankptr(9, rom + 0x10800);
				break;
		default:
				memory_set_bankptr(9, mess_ram + 0xc800);
				break;			
	}

    // BANK 10 (0xd000 - 0xd7ff)
	switch (partner_mem_page) {
		case 7:
				memory_set_bankptr(10, mess_ram + 0x5000);
				break;
		case 8:
		case 9:
				// window 2
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xd7ff, 0, 0, SMH_UNMAP);
				partner_window_2(machine, 10, 0x0800, rom);
				break;
		default:
				memory_set_bankptr(10, mess_ram + 0xd000);
				break;			
	}

	// BANK 11 (0xdc00 - 0xddff)
	partner_iomap_bank(machine,rom);
	
    // BANK 12 (0xe000 - 0xe7ff)
	if (partner_mem_page==1) {
		memory_set_bankptr(12, rom + 0x10000);
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
				memory_set_bankptr(13, rom + 0x10800);
				break;
	}	
}

WRITE8_HANDLER ( partner_win_memory_page_w )
{
	partner_win_mem_page = ~data;
	partner_bank_switch(machine);
}

WRITE8_HANDLER (partner_mem_page_w )
{
	partner_mem_page = (data >> 4) & 0x0f;
	partner_bank_switch(machine);
}

WRITE8_HANDLER(partner_dma_write_byte)
{
	cpuintrf_push_context(0);
	program_write_byte(offset,data);
	cpuintrf_pop_context();
}

const dma8257_interface partner_dma =
{
	0,
	XTAL_16MHz / 9,

	radio86_dma_read_byte,
	partner_dma_write_byte,

	{ wd17xx_data_r,  0, 0, 0 },
	{ wd17xx_data_w, 0, radio86_write_video, 0 },
	{ 0, 0, 0, 0 }
};

/*
const dma8257_interface partner_dma =
{
	0,
	XTAL_16MHz / 9,

	radio86_dma_read_byte,
	partner_dma_write_byte,

	{ wd17xx_data_r,  0, 0, 0 },
	{ wd17xx_data_w, 0, radio86_write_video, 0 },
	{ 0, 0, 0, 0 }
};
*/


MACHINE_RESET( partner )
{
	partner_mem_page = 0;
	partner_win_mem_page = 0;
	partner_bank_switch(machine);
	wd17xx_reset(machine);
}
