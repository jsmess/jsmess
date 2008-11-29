/***************************************************************************

        Vector06c driver by Miodrag Milanovic

        10/07/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/8255ppi.h"
#include "includes/vector06.h"

static int vector06_keyboard_mask;
static UINT8 vector_color_index;

/* Driver initialization */
DRIVER_INIT(vector06)
{
	memset(mess_ram,0,64*1024);
}

static READ8_DEVICE_HANDLER (vector06_8255_portb_r )
{
	UINT8 key = 0xff;
	if ((vector06_keyboard_mask & 0x01)!=0) { key &= input_port_read(device->machine,"LINE0"); }
	if ((vector06_keyboard_mask & 0x02)!=0) { key &= input_port_read(device->machine,"LINE1"); }
	if ((vector06_keyboard_mask & 0x04)!=0) { key &= input_port_read(device->machine,"LINE2"); }
	if ((vector06_keyboard_mask & 0x08)!=0) { key &= input_port_read(device->machine,"LINE3"); }
	if ((vector06_keyboard_mask & 0x10)!=0) { key &= input_port_read(device->machine,"LINE4"); }
	if ((vector06_keyboard_mask & 0x20)!=0) { key &= input_port_read(device->machine,"LINE5"); }
	if ((vector06_keyboard_mask & 0x40)!=0) { key &= input_port_read(device->machine,"LINE6"); }
	if ((vector06_keyboard_mask & 0x80)!=0) { key &= input_port_read(device->machine,"LINE7"); }
	return key;
}

static READ8_DEVICE_HANDLER (vector06_8255_portc_r )
{
	return input_port_read(device->machine, "LINE8");
}

static WRITE8_DEVICE_HANDLER (vector06_8255_porta_w )
{
	vector06_keyboard_mask = data ^ 0xff;
}

static WRITE8_DEVICE_HANDLER (vector06_8255_portb_w )
{
	vector_color_index = data;
}

WRITE8_HANDLER(vector06_color_set)
{
	UINT8 r = (data & 7) << 4;
	UINT8 g = ((data >> 3) & 7) << 4;
	UINT8 b = ((data >>6) & 3) << 5;
	palette_set_color( space->machine, vector_color_index, MAKE_RGB(r,g,b) );
}

const ppi8255_interface vector06_ppi8255_interface =
{
	NULL,
	vector06_8255_portb_r,
	vector06_8255_portc_r,
	vector06_8255_porta_w,
	vector06_8255_portb_w,
	NULL
};

READ8_HANDLER(vector_8255_1_r) {
	return ppi8255_r((device_config*)device_list_find_by_tag( space->machine->config->devicelist, PPI8255, "ppi8255" ), (offset ^ 0x03));
}

WRITE8_HANDLER(vector_8255_1_w) {
	ppi8255_w((device_config*)device_list_find_by_tag( space->machine->config->devicelist, PPI8255, "ppi8255" ), (offset ^0x03) , data );

}


INTERRUPT_GEN( vector06_interrupt )
{
	cpu_set_input_line(device, 0, ASSERT_LINE);
}

static IRQ_CALLBACK (  vector06_irq_callback )
{
	// Interupt is RST 7
	return 0xcd0038;
}

MACHINE_RESET( vector06 )
{
	const address_space *space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	
	cpu_set_irq_callback(machine->cpu[0], vector06_irq_callback);
	memory_install_read8_handler (space, 0x0000, 0x7fff, 0, 0, SMH_BANK1);
	memory_install_write8_handler(space, 0x0000, 0x7fff, 0, 0, SMH_BANK2);
	memory_install_read8_handler (space, 0x8000, 0xffff, 0, 0, SMH_BANK3);
	memory_install_write8_handler(space, 0x8000, 0xffff, 0, 0, SMH_BANK4);

	memory_set_bankptr(machine, 1, memory_region(machine, "main") + 0x10000);
	memory_set_bankptr(machine, 2, mess_ram + 0x0000);
	memory_set_bankptr(machine, 3, mess_ram + 0x8000);
	memory_set_bankptr(machine, 4, mess_ram + 0x8000);

	vector06_keyboard_mask = 0;
}
