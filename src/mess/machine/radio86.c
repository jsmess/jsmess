/***************************************************************************

        Radio-86RK machine driver by Miodrag Milanovic

        06/03/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/8255ppi.h"
#include "machine/8257dma.h"
#include "video/i8275.h"
#include "includes/radio86.h"

static int radio86_keyboard_mask;
UINT8 radio86_tape_value;

static UINT8* radio_ram_disk;

void radio86_init_keyboard() 
{
	radio86_keyboard_mask = 0;
	radio86_tape_value = 0x10;
}

/* Driver initialization */
DRIVER_INIT(radio86)
{
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = memory_region(machine, "main");
	memset(RAM,0x0000,0x1000); // make frist page empty by default
  	memory_configure_bank(1, 1, 2, RAM, 0x0000);
	memory_configure_bank(1, 0, 2, RAM, 0xf800);
	radio86_init_keyboard();
}

DRIVER_INIT(radioram)
{
	DRIVER_INIT_CALL(radio86);
	radio_ram_disk = auto_malloc(0x20000);
	memset(radio_ram_disk,0,0x20000);
}
static READ8_HANDLER (radio86_8255_portb_r2 )
{
	UINT8 key = 0xff;
	if ((radio86_keyboard_mask & 0x01)!=0) { key &= input_port_read(machine,"LINE0"); }
	if ((radio86_keyboard_mask & 0x02)!=0) { key &= input_port_read(machine,"LINE1"); }
	if ((radio86_keyboard_mask & 0x04)!=0) { key &= input_port_read(machine,"LINE2"); }
	if ((radio86_keyboard_mask & 0x08)!=0) { key &= input_port_read(machine,"LINE3"); }
	if ((radio86_keyboard_mask & 0x10)!=0) { key &= input_port_read(machine,"LINE4"); }
	if ((radio86_keyboard_mask & 0x20)!=0) { key &= input_port_read(machine,"LINE5"); }
	if ((radio86_keyboard_mask & 0x40)!=0) { key &= input_port_read(machine,"LINE6"); }
	if ((radio86_keyboard_mask & 0x80)!=0) { key &= input_port_read(machine,"LINE7"); }
	return key;
}

static READ8_HANDLER (radio86_8255_portc_r2 )
{
	double level = cassette_input(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette" ));	 									 					
	UINT8 dat = input_port_read(machine,"LINE8");
	if (level <  0) { 
		dat ^= radio86_tape_value;
 	}	
	return dat;	
}

static WRITE8_HANDLER (radio86_8255_porta_w2 )
{
	radio86_keyboard_mask = data ^ 0xff;
}

static WRITE8_HANDLER (radio86_8255_portc_w2 )
{
	cassette_output(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette" ),data & 0x01 ? 1 : -1);	
}


const ppi8255_interface radio86_ppi8255_interface_1 =
{
	NULL,
	radio86_8255_portb_r2,
	radio86_8255_portc_r2,
	radio86_8255_porta_w2,
	NULL,
	radio86_8255_portc_w2,
};

const ppi8255_interface mikrosha_ppi8255_interface_1 =
{
	radio86_8255_portb_r2,
	NULL,
	radio86_8255_portc_r2,
	NULL,
	radio86_8255_porta_w2,
	NULL,
};



static READ8_HANDLER (rk7007_8255_portc_r )
{
	double level = cassette_input(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette" ));	 									 					
	UINT8 key = 0xff;
	if ((radio86_keyboard_mask & 0x01)!=0) { key &= input_port_read(machine,"CLINE0"); }
	if ((radio86_keyboard_mask & 0x02)!=0) { key &= input_port_read(machine,"CLINE1"); }
	if ((radio86_keyboard_mask & 0x04)!=0) { key &= input_port_read(machine,"CLINE2"); }
	if ((radio86_keyboard_mask & 0x08)!=0) { key &= input_port_read(machine,"CLINE3"); }
	if ((radio86_keyboard_mask & 0x10)!=0) { key &= input_port_read(machine,"CLINE4"); }
	if ((radio86_keyboard_mask & 0x20)!=0) { key &= input_port_read(machine,"CLINE5"); }
	if ((radio86_keyboard_mask & 0x40)!=0) { key &= input_port_read(machine,"CLINE6"); }
	if ((radio86_keyboard_mask & 0x80)!=0) { key &= input_port_read(machine,"CLINE7"); }
	key &= 0xe0;
	if (level <  0) { 
		key ^= radio86_tape_value;
 	}	
	return key;	
}

const ppi8255_interface rk7007_ppi8255_interface =
{
	NULL,
	radio86_8255_portb_r2,
	rk7007_8255_portc_r,
	radio86_8255_porta_w2,
	NULL,
	radio86_8255_portc_w2,
};

static I8275_DMA_REQUEST(radio86_video_dma_request) {
	const device_config *dma8257 = device_list_find_by_tag(device->machine->config->devicelist, DMA8257, "dma8257");
	dma8257_drq_w(dma8257, 2, state);
}

READ8_DEVICE_HANDLER(radio86_dma_read_byte)
{
	UINT8 result;
	cpuintrf_push_context(0);
	result = program_read_byte(offset);
	cpuintrf_pop_context();
	return result;
}

WRITE8_DEVICE_HANDLER(radio86_write_video)
{
	i8275_dack_set_data((device_config*)device_list_find_by_tag( device->machine->config->devicelist, I8275, "i8275" ),data);
}

const dma8257_interface radio86_dma =
{
	0,
	XTAL_16MHz / 9,

	radio86_dma_read_byte,
	0,

	{ 0, 0, 0, 0 },
	{ 0, 0, radio86_write_video, 0 },
	{ 0, 0, 0, 0 }
};

static TIMER_CALLBACK( radio86_reset )	
{
	memory_set_bank(1, 0);
}

static UINT8 romdisk_lsb,romdisk_msb, disk_sel;

MACHINE_RESET( radio86 )
{
	timer_set(ATTOTIME_IN_USEC(10), NULL, 0, radio86_reset);
	memory_set_bank(1, 1);

	radio86_keyboard_mask = 0;
	disk_sel = 0;
}


WRITE8_HANDLER ( radio86_pagesel ) 
{
	disk_sel = data;
}

static READ8_HANDLER (radio86_romdisk_porta_r )
{
	UINT8 *romdisk = memory_region(machine, "main") + 0x10000;	
	if ((disk_sel & 0x0f) ==0) {
		return romdisk[romdisk_msb*256+romdisk_lsb];	
	} else {
		if (disk_sel==0xdf) {
			return radio_ram_disk[romdisk_msb*256+romdisk_lsb + 0x10000];
		} else {
			return radio_ram_disk[romdisk_msb*256+romdisk_lsb];
		}
	}
}

static WRITE8_HANDLER (radio86_romdisk_portb_w )
{	
	romdisk_lsb = data;
}

static WRITE8_HANDLER (radio86_romdisk_portc_w )
{		
	romdisk_msb = data;	
}

const ppi8255_interface radio86_ppi8255_interface_2 =
{
	radio86_romdisk_porta_r,
	NULL,
	NULL,
	NULL,
	radio86_romdisk_portb_w,
	radio86_romdisk_portc_w
};

static WRITE8_HANDLER (mikrosha_8255_font_page_w )
{
	mikrosha_font_page = (data  > 7) & 1;
}

const ppi8255_interface mikrosha_ppi8255_interface_2 =
{
	NULL,
	NULL,
	NULL,
	NULL,	
	mikrosha_8255_font_page_w,
	NULL
};

const i8275_interface radio86_i8275_interface = {
	"main",
	6,
	0,
	radio86_video_dma_request,
	NULL,
	radio86_display_pixels
};

const i8275_interface mikrosha_i8275_interface = {
	"main",
	6,
	0,
	radio86_video_dma_request,
	NULL,
	mikrosha_display_pixels
};

const i8275_interface apogee_i8275_interface = {
	"main",
	6,
	0,
	radio86_video_dma_request,
	NULL,
	apogee_display_pixels
};

const i8275_interface partner_i8275_interface = {
	"main",
	6,
	1,
	radio86_video_dma_request,
	NULL,
	partner_display_pixels
};
