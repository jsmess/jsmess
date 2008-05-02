/***************************************************************************

		Bashkiria-2M machine driver by Miodrag Milanovic

		28/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "deprecat.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "devices/basicdsk.h"
#include "machine/8255ppi.h"
#include "machine/pit8253.h"
#include "machine/wd17xx.h"
#include "machine/pic8259.h"

READ8_HANDLER (b2m_keyboard_r )
{		
	UINT8 key = 0x00;
	if (offset < 0x100) {
		if ((offset & 0x01)!=0) { key |= input_port_read_indexed(machine,0); }
		if ((offset & 0x02)!=0) { key |= input_port_read_indexed(machine,1); }
		if ((offset & 0x04)!=0) { key |= input_port_read_indexed(machine,2); }
		if ((offset & 0x08)!=0) { key |= input_port_read_indexed(machine,3); }
		if ((offset & 0x10)!=0) { key |= input_port_read_indexed(machine,4); }
		if ((offset & 0x20)!=0) { key |= input_port_read_indexed(machine,5); }
		if ((offset & 0x40)!=0) { key |= input_port_read_indexed(machine,6); }
		if ((offset & 0x80)!=0) { key |= input_port_read_indexed(machine,7); }					
	} else {
		if ((offset & 0x01)!=0) { key |= input_port_read_indexed(machine,8); }
		if ((offset & 0x02)!=0) { key |= input_port_read_indexed(machine,9); }
		if ((offset & 0x04)!=0) { key |= input_port_read_indexed(machine,10); }
	}
	return key;
}


void b2m_set_bank(running_machine *machine,int bank) 
{
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x27ff, 0, 0, SMH_BANK1);
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_BANK2);
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x6fff, 0, 0, SMH_BANK3);
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x7000, 0xdfff, 0, 0, SMH_BANK4);
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_BANK5);

	switch(bank) {
		case 0 :
		case 1 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);
						
						memory_set_bankptr(1, mess_ram);
						memory_set_bankptr(2, mess_ram + 0x2800);
						memory_set_bankptr(3, mess_ram + 0x3000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
						break;
/*		case 1 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x6fff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_set_bankptr(2, mess_ram + 0x2800);
						memory_set_bankptr(3, memory_region(REGION_CPU1) + 0x12000);
						memory_set_bankptr(4, memory_region(REGION_CPU1) + 0x16000);
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
						break;*/
		case 2 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_install_read8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(3, mess_ram + 0x10000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
						break;
		case 3 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_install_read8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(3, mess_ram + 0x14000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
						break;
		case 4 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_install_read8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(3, mess_ram + 0x18000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
					
						break;
		case 5 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_install_read8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(3, mess_ram + 0x1c000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
					
						break;
		case 6 :
						memory_set_bankptr(1, mess_ram);
						memory_set_bankptr(2, mess_ram + 0x2800);
						memory_set_bankptr(3, mess_ram + 0x3000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, mess_ram + 0xe000);					
						break;
		case 7 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x27ff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x6fff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x7000, 0xdfff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);
						
						memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000);	
						memory_set_bankptr(2, memory_region(REGION_CPU1) + 0x10000);	
						memory_set_bankptr(3, memory_region(REGION_CPU1) + 0x10000);	
						memory_set_bankptr(4, memory_region(REGION_CPU1) + 0x10000);	
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
						break;
	}
}
static PIT8253_FREQUENCY_CHANGED(bm2_pit_clk)
{
	pit8253_set_clockin((device_config*)device_list_find_by_tag( Machine->config->devicelist, PIT8253, "pit8253"), 0, frequency);
}

static PIT8253_OUTPUT_CHANGED(bm2_pit_irq)
{
	pic8259_set_irq_line((device_config*)device_list_find_by_tag( Machine->config->devicelist, PIC8259, "pic8259"),1,state);	
}

INTERRUPT_GEN (b2m_vblank_interrupt)
{
	pic8259_set_irq_line((device_config*)device_list_find_by_tag( Machine->config->devicelist, PIC8259, "pic8259"),0,1);		
}
const struct pit8253_config b2m_pit8253_intf =
{
	{
		{
			2000000,
			bm2_pit_irq,
			NULL
		},
		{
			2000000,
			NULL,
			NULL
		},
		{
			2000000,
			NULL,
			bm2_pit_clk
		}
	}
};

static int b2m_8255_porta;
UINT16 b2m_video_scroll;
static int b2m_8255_portc;

UINT8 b2m_video_page;


WRITE8_HANDLER (b2m_8255_porta_w )
{	
	b2m_8255_porta = data;
}
WRITE8_HANDLER (b2m_8255_portb_w )
{	
	b2m_video_scroll = data;
}

WRITE8_HANDLER (b2m_8255_portc_w )
{	
	b2m_8255_portc = data;
	b2m_set_bank(machine,b2m_8255_portc & 7);
	b2m_video_page = (b2m_8255_portc >> 7) & 1;
}

READ8_HANDLER (b2m_8255_porta_r )
{
	return 0xff;
}
READ8_HANDLER (b2m_8255_portb_r )
{
	return b2m_video_scroll;
}
READ8_HANDLER (b2m_8255_portc_r )
{
	return 0xff;
}
const ppi8255_interface b2m_ppi8255_interface_1 =
{
	b2m_8255_porta_r,
	b2m_8255_portb_r,
	b2m_8255_portc_r,
	b2m_8255_porta_w,
	b2m_8255_portb_w,
	b2m_8255_portc_w
};

UINT8 b2m_drive;
UINT8 b2m_side;


UINT8 b2m_8255_portc_e;
WRITE8_HANDLER (b2m_ext_8255_porta_w )
{	
}
WRITE8_HANDLER (b2m_ext_8255_portb_w )
{	
}

WRITE8_HANDLER (b2m_ext_8255_portc_w )
{		
	UINT8 drive = (data >> 1) & 1;
	UINT8 side  = (data  & 1) ^ 1;
	
	if (b2m_drive!=drive) {
		wd17xx_set_drive(drive);
		b2m_drive = drive;
	}
	if (b2m_side!=side) {
		wd17xx_set_side(side);
		b2m_side = side;
	}
}

READ8_HANDLER (b2m_ext_8255_porta_r )
{
	return 0xff;
}
READ8_HANDLER (b2m_ext_8255_portb_r )
{
	return 0xff;
}
READ8_HANDLER (b2m_ext_8255_portc_r )
{
	return 0xff;
}

const ppi8255_interface b2m_ppi8255_interface_2 =
{
	b2m_ext_8255_porta_r,
	b2m_ext_8255_portb_r,
	b2m_ext_8255_portc_r,
	b2m_ext_8255_porta_w,
	b2m_ext_8255_portb_w,
	b2m_ext_8255_portc_w
};



UINT8 b2m_romdisk_lsb;
UINT8 b2m_romdisk_msb;

READ8_HANDLER (b2m_romdisk_porta_r )
{
	UINT8 *romdisk = memory_region(REGION_CPU1) + 0x12000;		
	return romdisk[b2m_romdisk_msb*256+b2m_romdisk_lsb-0x8000];	
}

WRITE8_HANDLER (b2m_romdisk_portb_w )
{	
	b2m_romdisk_lsb = data;
}

WRITE8_HANDLER (b2m_romdisk_portc_w )
{		
	b2m_romdisk_msb = data;	
}

const ppi8255_interface b2m_ppi8255_interface_3 =
{
	b2m_romdisk_porta_r,
	NULL,
	NULL,
	NULL,
	b2m_romdisk_portb_w,
	b2m_romdisk_portc_w
};


READ8_HANDLER( b2m_8255_2_r ) 
{
	return ppi8255_r((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_3" ),offset);
}
WRITE8_HANDLER( b2m_8255_2_w ) 
{
	ppi8255_w((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_3" ),offset,data);
}

READ8_HANDLER( b2m_8255_1_r ) 
{
	return ppi8255_r((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_2" ),offset);
}
WRITE8_HANDLER( b2m_8255_1_w ) 
{
	ppi8255_w((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_2" ),offset,data);
}

READ8_HANDLER( b2m_8255_0_r ) 
{
	return ppi8255_r((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_1" ),offset);
}
WRITE8_HANDLER( b2m_8255_0_w ) 
{
	ppi8255_w((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_1" ),offset,data);
}

static PIC8259_SET_INT_LINE( b2m_pic_set_int_line )
{		
	cpunum_set_input_line(Machine, 0, 0,interrupt ?  HOLD_LINE : CLEAR_LINE);  
} 


/* Driver initialization */
DRIVER_INIT(b2m)
{
	memset(mess_ram,0,128*1024);	
}

MACHINE_START(b2m)
{
	wd17xx_init(machine, WD_TYPE_1793, NULL , NULL);	
}

IRQ_CALLBACK(b2m_irq_callback)
{	
	return pic8259_acknowledge((device_config*)device_list_find_by_tag( Machine->config->devicelist, PIC8259, "pic8259"));	
} 


const struct pic8259_interface b2m_pic8259_config = {
	b2m_pic_set_int_line
};


MACHINE_RESET(b2m)
{
	wd17xx_reset();
	
	b2m_side = 0;
	b2m_drive = 0;
	wd17xx_set_side(0);
	wd17xx_set_drive(0);

	cpunum_set_irq_callback(0, b2m_irq_callback);
	b2m_set_bank(machine,7);
}

DEVICE_IMAGE_LOAD( b2m_floppy )
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

	basicdsk_set_geometry (image, 80, 2, 5, 1024, 1, 0, FALSE);	
	return INIT_PASS;
}
