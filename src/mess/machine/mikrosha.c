/***************************************************************************

        Mikrosha machine driver by Miodrag Milanovic

        06/03/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/8255ppi.h"
#include "machine/8257dma.h"
#include "video/i8275.h"
#include "includes/mikrosha.h"

static int mikrosha_keyboard_mask;


/* Driver initialization */
DRIVER_INIT(mikrosha)
{
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = memory_region(machine, REGION_CPU1);
	memset(RAM,0x0000,0x1000); // make frist page empty by default
  	memory_configure_bank(1, 1, 2, RAM, 0x0000);
	memory_configure_bank(1, 0, 2, RAM, 0xf800);
}

static READ8_HANDLER (mikrosha_8255_porta_r2 )
{
	UINT8 key = 0xff;
	if ((mikrosha_keyboard_mask & 0x01)!=0) { key &= input_port_read(machine,"LINE0"); }
	if ((mikrosha_keyboard_mask & 0x02)!=0) { key &= input_port_read(machine,"LINE1"); }
	if ((mikrosha_keyboard_mask & 0x04)!=0) { key &= input_port_read(machine,"LINE2"); }
	if ((mikrosha_keyboard_mask & 0x08)!=0) { key &= input_port_read(machine,"LINE3"); }
	if ((mikrosha_keyboard_mask & 0x10)!=0) { key &= input_port_read(machine,"LINE4"); }
	if ((mikrosha_keyboard_mask & 0x20)!=0) { key &= input_port_read(machine,"LINE5"); }
	if ((mikrosha_keyboard_mask & 0x40)!=0) { key &= input_port_read(machine,"LINE6"); }
	if ((mikrosha_keyboard_mask & 0x80)!=0) { key &= input_port_read(machine,"LINE7"); }
	return key;
}

static READ8_HANDLER (mikrosha_8255_portc_r2 )
{
	double level = cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0));
	UINT8 dat = input_port_read(machine,"LINE8");
	if (level <  0) {
		dat ^= 0x10;
 	}
	return dat;
}

static WRITE8_HANDLER (mikrosha_8255_portb_w2 )
{
	mikrosha_keyboard_mask = data ^ 0xff;
}

const ppi8255_interface mikrosha_ppi8255_interface_1 =
{
	mikrosha_8255_porta_r2,
	NULL,
	mikrosha_8255_portc_r2,
	NULL,
	mikrosha_8255_portb_w2,
	NULL,
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

static I8275_DMA_REQUEST(mikrosha_video_dma_request) {
	const device_config *dma8257 = device_list_find_by_tag(device->machine->config->devicelist, VIDEO_SCREEN, "dma8257");
	dma8257_drq_w(dma8257, 2, state);
}

const i8275_interface mikrosha_i8275_interface = {
	"main",
	6,
	0,
	mikrosha_video_dma_request,
	NULL,
	mikrosha_display_pixels
};

static READ8_HANDLER( mikrosha_dma_read_byte )
{
	UINT8 result;
	cpuintrf_push_context(0);
	result = program_read_byte(offset);
	cpuintrf_pop_context();
	return result;
}

static WRITE8_HANDLER( mikrosha_write_video )
{
	i8275_dack_set_data((device_config*)device_list_find_by_tag( machine->config->devicelist, I8275, "i8275" ),data);
}

const dma8257_interface mikrosha_dma =
{
	0,
	XTAL_16MHz / 9,

	mikrosha_dma_read_byte,
	0,

	{ 0, 0, 0, 0 },
	{ 0, 0, mikrosha_write_video, 0 },
	{ 0, 0, 0, 0 }
};

static TIMER_CALLBACK( mikrosha_reset )
{
	memory_set_bank(1, 0);
}

MACHINE_RESET( mikrosha )
{
	timer_set(ATTOTIME_IN_USEC(10), NULL, 0, mikrosha_reset);
	memory_set_bank(1, 1);

	mikrosha_keyboard_mask = 0;
}
