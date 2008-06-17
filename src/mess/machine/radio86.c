/***************************************************************************

        Radio-86RK machine driver by Miodrag Milanovic

        06/03/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "deprecat.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/8255ppi.h"
#include "machine/8257dma.h"
#include "video/i8275.h"
#include "includes/radio86.h"

static int radio86_keyboard_mask;


/* Driver initialization */
DRIVER_INIT(radio86)
{
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = memory_region(REGION_CPU1);
	memset(RAM,0x0000,0x1000); // make frist page empty by default
  	memory_configure_bank(1, 1, 2, RAM, 0x0000);
	memory_configure_bank(1, 0, 2, RAM, 0xf000);
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
	double level = cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0));	 									 					
	UINT8 dat = input_port_read(machine,"LINE8");
	if (level <  0) { 
		dat ^= 0x10;
 	}	
	return dat;	
}

static WRITE8_HANDLER (radio86_8255_porta_w2 )
{
	radio86_keyboard_mask = data ^ 0xff;
}

const ppi8255_interface radio86_ppi8255_interface_1 =
{
	NULL,
	radio86_8255_portb_r2,
	radio86_8255_portc_r2,
	radio86_8255_porta_w2,
	NULL,
	NULL,
};

static I8275_DMA_REQUEST(radio86_video_dma_request) {
	dma8257_drq_write(0, 2, state);
}

const i8275_interface radio86_i8275_interface = {
	"main",
	6,
	0,
	radio86_video_dma_request,
	NULL,
	radio86_display_pixels
};

static UINT8 radio86_dma_read_byte(int channel, offs_t offset)
{
	UINT8 result;
	cpuintrf_push_context(0);
	result = program_read_byte(offset);
	cpuintrf_pop_context();
	return result;
}

static void radio86_write_video(UINT8 data)
{
	i8275_dack_set_data((device_config*)device_list_find_by_tag( Machine->config->devicelist, I8275, "i8275" ),data);
}

static const struct dma8257_interface radio86_dma =
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

MACHINE_RESET( radio86 )
{
	timer_set(ATTOTIME_IN_USEC(10), NULL, 0, radio86_reset);
	memory_set_bank(1, 1);

	radio86_keyboard_mask = 0;
	dma8257_init(1);
	dma8257_config(0, &radio86_dma);
	dma8257_reset();
}
