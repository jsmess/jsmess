/***************************************************************************

        Vector06c driver by Miodrag Milanovic

        10/07/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "machine/8255ppi.h"
#include "machine/wd17xx.h"
#include "devices/cassette.h"
#include "devices/basicdsk.h"
#include "includes/vector06.h"

UINT8 vector06_keyboard_mask;
UINT8 vector_color_index;
UINT8 vector_video_mode;

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
	double level = cassette_input(devtag_get_device(device->machine, "cassette"));
	UINT8 retVal = input_port_read(device->machine, "LINE8");
	if (level >  0) { 
		retVal |= 0x10; 
 	}
	return retVal;
}

static WRITE8_DEVICE_HANDLER (vector06_8255_porta_w )
{
	vector06_keyboard_mask = data ^ 0xff;
}

static void vector_set_video_mode(running_machine *machine, int width) {
	rectangle visarea;
	
	visarea.min_x = 0;
	visarea.min_y = 0;
	visarea.max_x = width+64-1;
	visarea.max_y = 256+64-1;
	video_screen_configure(machine->primary_screen, width+64, 256+64, &visarea, video_screen_get_frame_period(machine->primary_screen).attoseconds);	
}

static WRITE8_DEVICE_HANDLER (vector06_8255_portb_w )
{
	vector_color_index = data & 0x0f;
	if ((data & 0x10) != vector_video_mode) 
	{
		vector_video_mode = data & 0x10;
		vector_set_video_mode(device->machine,(vector_video_mode==0x10) ? 512 : 256);
	}	
}

WRITE8_HANDLER(vector06_color_set)
{
	UINT8 r = (data & 7) << 5;
	UINT8 g = ((data >> 3) & 7) << 5;
	UINT8 b = ((data >>6) & 3) << 6;
	palette_set_color( space->machine, vector_color_index, MAKE_RGB(r,g,b) );
}

static UINT8 romdisk_msb;
static UINT8 romdisk_lsb;

static READ8_DEVICE_HANDLER (vector06_romdisk_portb_r )
{
	UINT8 *romdisk = memory_region(device->machine, "maincpu") + 0x18000;		
	UINT16 addr = (romdisk_msb*256+romdisk_lsb) & 0x7fff;
	return romdisk[addr];	
}

static WRITE8_DEVICE_HANDLER (vector06_romdisk_porta_w )
{	
	romdisk_lsb = data;
}

static WRITE8_DEVICE_HANDLER (vector06_romdisk_portc_w )
{		
	romdisk_msb = data;	
}

const ppi8255_interface vector06_ppi8255_2_interface =
{
	DEVCB_NULL,
	DEVCB_HANDLER(vector06_romdisk_portb_r),
	DEVCB_NULL,
	DEVCB_HANDLER(vector06_romdisk_porta_w),
	DEVCB_NULL,
	DEVCB_HANDLER(vector06_romdisk_portc_w)
};


const ppi8255_interface vector06_ppi8255_interface =
{
	DEVCB_NULL,
	DEVCB_HANDLER(vector06_8255_portb_r),
	DEVCB_HANDLER(vector06_8255_portc_r),
	DEVCB_HANDLER(vector06_8255_porta_w),
	DEVCB_HANDLER(vector06_8255_portb_w),
	DEVCB_NULL
};

READ8_HANDLER(vector_8255_1_r) {
	return ppi8255_r(devtag_get_device(space->machine, "ppi8255"), (offset ^ 0x03));
}

WRITE8_HANDLER(vector_8255_1_w) {
	ppi8255_w(devtag_get_device(space->machine, "ppi8255"), (offset ^0x03) , data );

}

READ8_HANDLER(vector_8255_2_r) {
	return ppi8255_r(devtag_get_device(space->machine, "ppi8255_2"), (offset ^ 0x03));
}

WRITE8_HANDLER(vector_8255_2_w) {
	ppi8255_w(devtag_get_device(space->machine, "ppi8255_2"), (offset ^0x03) , data );

}

static UINT8 vblank_state = 0;
INTERRUPT_GEN( vector06_interrupt )
{
	vblank_state++;
	if (vblank_state>1) vblank_state=0;
	cpu_set_input_line(device,0,vblank_state ? HOLD_LINE : CLEAR_LINE);		
	
}

static IRQ_CALLBACK (  vector06_irq_callback )
{
	// Interupt is RST 7
	return 0xff;
}

static TIMER_CALLBACK(reset_check_callback)
{
	UINT8 val = input_port_read(machine, "RESET");
	if ((val & 1)==1) {
		memory_set_bankptr(machine, 1, memory_region(machine, "maincpu") + 0x10000);
		device_reset(cputag_get_cpu(machine, "maincpu"));
	}
	if ((val & 2)==2) {
		memory_set_bankptr(machine, 1, mess_ram + 0x0000);
		device_reset(cputag_get_cpu(machine, "maincpu"));
	}
}

WRITE8_HANDLER(vector_disc_w)
{
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");
	wd17xx_set_side (fdc,((data & 4) >> 2) ^ 1);
 	wd17xx_set_drive(fdc,data & 1);					
}

DEVICE_IMAGE_LOAD( vector_floppy )
{
	int size;

	if (! image_has_been_created(image))
		{
		size = image_length(image);

		switch (size)
			{
			case 820*1024:
				break;
			default:
				return INIT_FAIL;
			}
		}
	else
		return INIT_FAIL;

	if (device_load_basicdsk_floppy (image) != INIT_PASS)
		return INIT_FAIL;

	basicdsk_set_geometry (image, 82, 2, 5, 1024, 1, 0, FALSE);	
	return INIT_PASS;
}


MACHINE_START( vector06 )
{
	const device_config *fdc = devtag_get_device(machine, "wd1793");
	wd17xx_set_density (fdc, DEN_FM_HI);
}

MACHINE_RESET( vector06 )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	
	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), vector06_irq_callback);
	memory_install_read8_handler (space, 0x0000, 0x7fff, 0, 0, SMH_BANK(1));
	memory_install_write8_handler(space, 0x0000, 0x7fff, 0, 0, SMH_BANK(2));
	memory_install_read8_handler (space, 0x8000, 0xffff, 0, 0, SMH_BANK(3));
	memory_install_write8_handler(space, 0x8000, 0xffff, 0, 0, SMH_BANK(4));

	memory_set_bankptr(machine, 1, memory_region(machine, "maincpu") + 0x10000);
	memory_set_bankptr(machine, 2, mess_ram + 0x0000);
	memory_set_bankptr(machine, 3, mess_ram + 0x8000);
	memory_set_bankptr(machine, 4, mess_ram + 0x8000);

	vector06_keyboard_mask = 0;
	vector_color_index = 0;
	vector_video_mode = 0;
	timer_pulse(machine, ATTOTIME_IN_HZ(50), NULL, 0, reset_check_callback);
}
