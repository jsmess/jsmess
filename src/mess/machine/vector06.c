/***************************************************************************

        Vector06c driver by Miodrag Milanovic

        10/07/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/i8255a.h"
#include "machine/wd17xx.h"
#include "devices/cassette.h"
#include "includes/vector06.h"
#include "devices/messram.h"

UINT8 vector06_keyboard_mask;
UINT8 vector06_color_index;
UINT8 vector06_video_mode;

/* Driver initialization */
DRIVER_INIT(vector06)
{
	memset(messram_get_ptr(devtag_get_device(machine, "messram")),0,64*1024);
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

static void vector06_set_video_mode(running_machine *machine, int width) {
	rectangle visarea;

	visarea.min_x = 0;
	visarea.min_y = 0;
	visarea.max_x = width+64-1;
	visarea.max_y = 256+64-1;
	video_screen_configure(machine->primary_screen, width+64, 256+64, &visarea, video_screen_get_frame_period(machine->primary_screen).attoseconds);
}

static WRITE8_DEVICE_HANDLER (vector06_8255_portb_w )
{
	vector06_color_index = data & 0x0f;
	if ((data & 0x10) != vector06_video_mode)
	{
		vector06_video_mode = data & 0x10;
		vector06_set_video_mode(device->machine,(vector06_video_mode==0x10) ? 512 : 256);
	}
}

WRITE8_HANDLER(vector06_color_set)
{
	UINT8 r = (data & 7) << 5;
	UINT8 g = ((data >> 3) & 7) << 5;
	UINT8 b = ((data >>6) & 3) << 6;
	palette_set_color( space->machine, vector06_color_index, MAKE_RGB(r,g,b) );
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

I8255A_INTERFACE( vector06_ppi8255_2_interface )
{
	DEVCB_NULL,
	DEVCB_HANDLER(vector06_romdisk_portb_r),
	DEVCB_NULL,
	DEVCB_HANDLER(vector06_romdisk_porta_w),
	DEVCB_NULL,
	DEVCB_HANDLER(vector06_romdisk_portc_w)
};


I8255A_INTERFACE( vector06_ppi8255_interface )
{
	DEVCB_NULL,
	DEVCB_HANDLER(vector06_8255_portb_r),
	DEVCB_HANDLER(vector06_8255_portc_r),
	DEVCB_HANDLER(vector06_8255_porta_w),
	DEVCB_HANDLER(vector06_8255_portb_w),
	DEVCB_NULL
};

READ8_HANDLER(vector06_8255_1_r) {
	return i8255a_r(devtag_get_device(space->machine, "ppi8255"), (offset ^ 0x03));
}

WRITE8_HANDLER(vector06_8255_1_w) {
	i8255a_w(devtag_get_device(space->machine, "ppi8255"), (offset ^0x03) , data );

}

READ8_HANDLER(vector06_8255_2_r) {
	return i8255a_r(devtag_get_device(space->machine, "ppi8255_2"), (offset ^ 0x03));
}

WRITE8_HANDLER(vector06_8255_2_w) {
	i8255a_w(devtag_get_device(space->machine, "ppi8255_2"), (offset ^0x03) , data );

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
		memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu") + 0x10000);
		devtag_get_device(machine, "maincpu")->reset();
	}
	if ((val & 2)==2) {
		memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
		devtag_get_device(machine, "maincpu")->reset();
	}
}

WRITE8_HANDLER(vector06_disc_w)
{
	running_device *fdc = devtag_get_device(space->machine, "wd1793");
	wd17xx_set_side (fdc,((data & 4) >> 2) ^ 1);
 	wd17xx_set_drive(fdc,data & 1);
}

MACHINE_START( vector06 )
{
	timer_pulse(machine, ATTOTIME_IN_HZ(50), NULL, 0, reset_check_callback);
}

MACHINE_RESET( vector06 )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	cpu_set_irq_callback(devtag_get_device(machine, "maincpu"), vector06_irq_callback);
	memory_install_read_bank (space, 0x0000, 0x7fff, 0, 0, "bank1");
	memory_install_write_bank(space, 0x0000, 0x7fff, 0, 0, "bank2");
	memory_install_read_bank (space, 0x8000, 0xffff, 0, 0, "bank3");
	memory_install_write_bank(space, 0x8000, 0xffff, 0, 0, "bank4");

	memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu") + 0x10000);
	memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x0000);
	memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x8000);
	memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x8000);

	vector06_keyboard_mask = 0;
	vector06_color_index = 0;
	vector06_video_mode = 0;
}
