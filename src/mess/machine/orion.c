/***************************************************************************

        Orion machine driver by Miodrag Milanovic

        22/04/2008 Orion Pro added
        02/04/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/mc146818.h"
#include "machine/wd17xx.h"
#include "sound/speaker.h"
#include "sound/ay8910.h"
#include "includes/orion.h"
#include "includes/radio86.h"
#include "devices/messram.h"

#define SCREEN_WIDTH_384 48
#define SCREEN_WIDTH_480 60
#define SCREEN_WIDTH_512 64





static READ8_DEVICE_HANDLER (orion_romdisk_porta_r )
{
	orion_state *state = device->machine->driver_data<orion_state>();
	UINT8 *romdisk = device->machine->region("maincpu")->base() + 0x10000;
	return romdisk[state->romdisk_msb*256+state->romdisk_lsb];
}

static WRITE8_DEVICE_HANDLER (orion_romdisk_portb_w )
{
	orion_state *state = device->machine->driver_data<orion_state>();
	state->romdisk_lsb = data;
}

static WRITE8_DEVICE_HANDLER (orion_romdisk_portc_w )
{
	orion_state *state = device->machine->driver_data<orion_state>();
	state->romdisk_msb = data;
}

I8255A_INTERFACE( orion128_ppi8255_interface_1)
{
	DEVCB_HANDLER(orion_romdisk_porta_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(orion_romdisk_portb_w),
	DEVCB_HANDLER(orion_romdisk_portc_w)
};


MACHINE_START( orion128 )
{
	orion_state *state = machine->driver_data<orion_state>();
	state->video_mode_mask = 7;
}

READ8_HANDLER ( orion128_system_r )
{
	return i8255a_r(space->machine->device("ppi8255_2"), offset & 3);
}

WRITE8_HANDLER ( orion128_system_w )
{
	i8255a_w(space->machine->device("ppi8255_2"), offset & 3, data);
}

READ8_HANDLER ( orion128_romdisk_r )
{
	return i8255a_r(space->machine->device("ppi8255_1"), offset & 3);
}

WRITE8_HANDLER ( orion128_romdisk_w )
{
	i8255a_w(space->machine->device("ppi8255_1"), offset & 3, data);
}

static void orion_set_video_mode(running_machine *machine, int width)
{
		rectangle visarea;

		visarea.min_x = 0;
		visarea.min_y = 0;
		visarea.max_x = width-1;
		visarea.max_y = 255;
		machine->primary_screen->configure(width, 256, visarea, machine->primary_screen->frame_period().attoseconds);
}

WRITE8_HANDLER ( orion128_video_mode_w )
{
	orion_state *state = space->machine->driver_data<orion_state>();
	if ((data & 0x80)!=(state->orion128_video_mode & 0x80))
	{
		if ((data & 0x80)==0x80)
		{
			if (state->video_mode_mask == 31)
			{
				state->orion128_video_width = SCREEN_WIDTH_512;
				orion_set_video_mode(space->machine,512);
			}
			else
			{
				state->orion128_video_width = SCREEN_WIDTH_480;
				orion_set_video_mode(space->machine,480);
			}
		}
		else
		{
			state->orion128_video_width = SCREEN_WIDTH_384;
			orion_set_video_mode(space->machine,384);
		}
	}

	state->orion128_video_mode = data;
}

WRITE8_HANDLER ( orion128_video_page_w )
{
	orion_state *state = space->machine->driver_data<orion_state>();
	if (state->orion128_video_page != data)
	{
		if ((data & 0x80)!=(state->orion128_video_page & 0x80))
		{
			if ((data & 0x80)==0x80)
			{
				if (state->video_mode_mask == 31)
				{
					state->orion128_video_width = SCREEN_WIDTH_512;
					orion_set_video_mode(space->machine,512);
				}
				else
				{
					state->orion128_video_width = SCREEN_WIDTH_480;
					orion_set_video_mode(space->machine,480);
				}
			}
			else
			{
				state->orion128_video_width = SCREEN_WIDTH_384;
				orion_set_video_mode(space->machine,384);
			}
		}
	}
	state->orion128_video_page = data;
}


WRITE8_HANDLER ( orion128_memory_page_w )
{
	orion_state *state = space->machine->driver_data<orion_state>();
	if (data!=state->orion128_memory_page )
	{
		memory_set_bankptr(space->machine, "bank1", messram_get_ptr(space->machine->device("messram")) + (data & 3) * 0x10000);
		state->orion128_memory_page = (data & 3);
	}
}

MACHINE_RESET ( orion128 )
{
	orion_state *state = machine->driver_data<orion_state>();
	state->orion128_video_page = 0;
	state->orion128_video_mode = 0;
	state->orion128_memory_page = -1;
	memory_set_bankptr(machine, "bank1", machine->region("maincpu")->base() + 0xf800);
	memory_set_bankptr(machine, "bank2", messram_get_ptr(machine->device("messram")) + 0xf000);
	state->orion128_video_width = SCREEN_WIDTH_384;
	orion_set_video_mode(machine,384);
	radio86_init_keyboard(machine);
}

static WRITE8_HANDLER ( orion_disk_control_w )
{
	device_t *fdc = space->machine->device("wd1793");

	wd17xx_set_side(fdc,((data & 0x10) >> 4) ^ 1);
	wd17xx_set_drive(fdc,data & 3);
}

READ8_HANDLER ( orion128_floppy_r )
{
	device_t *fdc = space->machine->device("wd1793");

	switch(offset)
	{
		case 0x0	:
		case 0x10 : return wd17xx_status_r(fdc,0);
		case 0x1	:
		case 0x11 : return wd17xx_track_r(fdc,0);
		case 0x2  :
		case 0x12 : return wd17xx_sector_r(fdc,0);
		case 0x3  :
		case 0x13 : return wd17xx_data_r(fdc,0);
	}
	return 0xff;
}

WRITE8_HANDLER ( orion128_floppy_w )
{
	device_t *fdc = space->machine->device("wd1793");

	switch(offset)
	{
		case 0x0	:
		case 0x10 : wd17xx_command_w(fdc,0,data); break;
		case 0x1	:
		case 0x11 : wd17xx_track_w(fdc,0,data);break;
		case 0x2  :
		case 0x12 : wd17xx_sector_w(fdc,0,data);break;
		case 0x3  :
		case 0x13 : wd17xx_data_w(fdc,0,data);break;
		case 0x4  :
		case 0x14 :
		case 0x20 : orion_disk_control_w(space, offset, data);break;
	}
}
static READ8_HANDLER ( orionz80_floppy_rtc_r )
{
	if ((offset >= 0x60) && (offset <= 0x6f))
	{
		return space->machine->device<mc146818_device>("rtc")->read(*space,offset-0x60);
	}
	else
	{
		return orion128_floppy_r(space,offset);
	}
}

static WRITE8_HANDLER ( orionz80_floppy_rtc_w )
{
	if ((offset >= 0x60) && (offset <= 0x6f))
	{
		space->machine->device<mc146818_device>("rtc")->write(*space,offset-0x60,data);
	}
	else
	{
		orion128_floppy_w(space,offset,data);
	}
}


MACHINE_START( orionz80 )
{
	orion_state *state = machine->driver_data<orion_state>();
	state->video_mode_mask = 7;
}

WRITE8_HANDLER ( orionz80_sound_w )
{
	orion_state *state = space->machine->driver_data<orion_state>();
	device_t *speaker = space->machine->device("speaker");
	if (state->speaker == 0)
	{
		state->speaker = data;
	}
	else
	{
		state->speaker = 0 ;
	}
	speaker_level_w(speaker,state->speaker);

}

static WRITE8_HANDLER ( orionz80_sound_fe_w )
{
	device_t *speaker = space->machine->device("speaker");
	speaker_level_w(speaker,(data>>4) & 0x01);
}


static void orionz80_switch_bank(running_machine *machine)
{
	orion_state *state = machine->driver_data<orion_state>();
	UINT8 bank_select;
	UINT8 segment_select;
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	bank_select = (state->orionz80_dispatcher & 0x0c) >> 2;
	segment_select = state->orionz80_dispatcher & 0x03;

	memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank1");
	if ((state->orionz80_dispatcher & 0x80)==0)
	{ // dispatcher on
		memory_set_bankptr(machine, "bank1", messram_get_ptr(machine->device("messram")) + 0x10000 * bank_select + segment_select * 0x4000 );
	}
	else
	{ // dispatcher off
		memory_set_bankptr(machine, "bank1", messram_get_ptr(machine->device("messram")) + 0x10000 * state->orionz80_memory_page);
	}

	memory_set_bankptr(machine, "bank2", messram_get_ptr(machine->device("messram")) + 0x4000 + 0x10000 * state->orionz80_memory_page);

	if ((state->orionz80_dispatcher & 0x20) == 0)
	{
		memory_install_write8_handler(space, 0xf400, 0xf4ff, 0, 0, orion128_system_w);
		memory_install_write8_handler(space, 0xf500, 0xf5ff, 0, 0, orion128_romdisk_w);
		memory_install_write8_handler(space, 0xf700, 0xf7ff, 0, 0, orionz80_floppy_rtc_w);
		memory_install_read8_handler(space, 0xf400, 0xf4ff, 0, 0, orion128_system_r);
		memory_install_read8_handler(space, 0xf500, 0xf5ff, 0, 0, orion128_romdisk_r);
		memory_install_read8_handler(space, 0xf700, 0xf7ff, 0, 0, orionz80_floppy_rtc_r);

		memory_install_write8_handler(space, 0xf800, 0xf8ff, 0, 0, orion128_video_mode_w);
		memory_install_write8_handler(space, 0xf900, 0xf9ff, 0, 0, orionz80_memory_page_w);
		memory_install_write8_handler(space, 0xfa00, 0xfaff, 0, 0, orion128_video_page_w);
		memory_install_write8_handler(space, 0xfb00, 0xfbff, 0, 0, orionz80_dispatcher_w);
		memory_unmap_write(space, 0xfc00, 0xfeff, 0, 0);
		memory_install_write8_handler(space, 0xff00, 0xffff, 0, 0, orionz80_sound_w);

		memory_set_bankptr(machine, "bank3", messram_get_ptr(machine->device("messram")) + 0xf000);
		memory_set_bankptr(machine, "bank5", machine->region("maincpu")->base() + 0xf800);

	}
	else
	{
		/* if it is full memory access */
		memory_set_bankptr(machine, "bank3", messram_get_ptr(machine->device("messram")) + 0xf000 + 0x10000 * state->orionz80_memory_page);
		memory_set_bankptr(machine, "bank4", messram_get_ptr(machine->device("messram")) + 0xf400 + 0x10000 * state->orionz80_memory_page);
		memory_set_bankptr(machine, "bank5", messram_get_ptr(machine->device("messram")) + 0xf800 + 0x10000 * state->orionz80_memory_page);
	}
}

WRITE8_HANDLER ( orionz80_memory_page_w )
{
	orion_state *state = space->machine->driver_data<orion_state>();
	state->orionz80_memory_page = data & 7;
	orionz80_switch_bank(space->machine);
}

WRITE8_HANDLER ( orionz80_dispatcher_w )
{
	orion_state *state = space->machine->driver_data<orion_state>();
	state->orionz80_dispatcher = data;
	orionz80_switch_bank(space->machine);
}

MACHINE_RESET ( orionz80 )
{
	orion_state *state = machine->driver_data<orion_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
	memory_install_write_bank(space, 0x4000, 0xefff, 0, 0, "bank2");
	memory_install_write_bank(space, 0xf000, 0xf3ff, 0, 0, "bank3");

	memory_install_write8_handler(space, 0xf400, 0xf4ff, 0, 0, orion128_system_w);
	memory_install_write8_handler(space, 0xf500, 0xf5ff, 0, 0, orion128_romdisk_w);
	memory_install_write8_handler(space, 0xf700, 0xf7ff, 0, 0, orionz80_floppy_rtc_w);
	memory_install_read8_handler(space, 0xf400, 0xf4ff, 0, 0, orion128_system_r);
	memory_install_read8_handler(space, 0xf500, 0xf5ff, 0, 0, orion128_romdisk_r);
	memory_install_read8_handler(space, 0xf700, 0xf7ff, 0, 0, orionz80_floppy_rtc_r);

	memory_install_write8_handler(space, 0xf800, 0xf8ff, 0, 0, orion128_video_mode_w);
	memory_install_write8_handler(space, 0xf900, 0xf9ff, 0, 0, orionz80_memory_page_w);
	memory_install_write8_handler(space, 0xfa00, 0xfaff, 0, 0, orion128_video_page_w);
	memory_install_write8_handler(space, 0xfb00, 0xfbff, 0, 0, orionz80_dispatcher_w);
	memory_unmap_write(space, 0xfc00, 0xfeff, 0, 0);
	memory_install_write8_handler(space, 0xff00, 0xffff, 0, 0, orionz80_sound_w);


	memory_set_bankptr(machine, "bank1", machine->region("maincpu")->base() + 0xf800);
	memory_set_bankptr(machine, "bank2", messram_get_ptr(machine->device("messram")) + 0x4000);
	memory_set_bankptr(machine, "bank3", messram_get_ptr(machine->device("messram")) + 0xf000);
	memory_set_bankptr(machine, "bank5", machine->region("maincpu")->base() + 0xf800);


	state->orion128_video_page = 0;
	state->orion128_video_mode = 0;
	state->orionz80_memory_page = 0;
	state->orionz80_dispatcher = 0;
	state->speaker = 0;
	state->orion128_video_width = SCREEN_WIDTH_384;
	orion_set_video_mode(machine,384);
	radio86_init_keyboard(machine);
}

INTERRUPT_GEN( orionz80_interrupt )
{
	orion_state *state = device->machine->driver_data<orion_state>();
	if ((state->orionz80_dispatcher & 0x40)==0x40)
	{
		cpu_set_input_line(device, 0, HOLD_LINE);
	}
}

READ8_HANDLER ( orionz80_io_r )
{
	if (offset == 0xFFFD)
	{
		return ay8910_r (space->machine->device("ay8912"), 0);
	}
	return 0xff;
}

WRITE8_HANDLER ( orionz80_io_w )
{
	switch (offset & 0xff)
	{
		case 0xf8 : orion128_video_mode_w(space,0,data);break;
		case 0xf9 : orionz80_memory_page_w(space,0,data);break;
		case 0xfa : orion128_video_page_w(space,0,data);break;
		case 0xfb : orionz80_dispatcher_w(space,0,data);break;
		case 0xfe : orionz80_sound_fe_w(space,0,data);break;
		case 0xff : orionz80_sound_w(space,0,data);break;
	}
	switch(offset)
	{
		case 0xfffd : ay8910_address_w(space->machine->device("ay8912"), 0, data);
					  break;
		case 0xbffd :
		case 0xbefd : ay8910_data_w(space->machine->device("ay8912"), 0, data);
					  break;
	}
}





static WRITE8_HANDLER ( orionpro_memory_page_w );

static void orionpro_bank_switch(running_machine *machine)
{
	orion_state *state = machine->driver_data<orion_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int page = state->orionpro_page & 7; // we have only 8 pages
	int is128 = (state->orionpro_dispatcher & 0x80) ? 1 : 0;
	if (is128==1)
	{
		page = state->orionpro_128_page & 7;
	}
	memory_install_write_bank(space, 0x0000, 0x1fff, 0, 0, "bank1");
	memory_install_write_bank(space, 0x2000, 0x3fff, 0, 0, "bank2");
	memory_install_write_bank(space, 0x4000, 0x7fff, 0, 0, "bank3");
	memory_install_write_bank(space, 0x8000, 0xbfff, 0, 0, "bank4");
	memory_install_write_bank(space, 0xc000, 0xefff, 0, 0, "bank5");
	memory_install_write_bank(space, 0xf000, 0xf3ff, 0, 0, "bank6");
	memory_install_write_bank(space, 0xf400, 0xf7ff, 0, 0, "bank7");
	memory_install_write_bank(space, 0xf800, 0xffff, 0, 0, "bank8");


	if ((state->orionpro_dispatcher & 0x01)==0x00)
	{	// RAM0 segment disabled
		memory_set_bankptr(machine, "bank1", messram_get_ptr(machine->device("messram")) + 0x10000 * page);
		memory_set_bankptr(machine, "bank2", messram_get_ptr(machine->device("messram")) + 0x10000 * page + 0x2000);
	}
	else
	{
		memory_set_bankptr(machine, "bank1", messram_get_ptr(machine->device("messram")) + (state->orionpro_ram0_segment & 31) * 0x4000);
		memory_set_bankptr(machine, "bank2", messram_get_ptr(machine->device("messram")) + (state->orionpro_ram0_segment & 31) * 0x4000 + 0x2000);
	}
	if ((state->orionpro_dispatcher & 0x10)==0x10)
	{	// ROM1 enabled
		memory_unmap_write(space, 0x0000, 0x1fff, 0, 0);
		memory_set_bankptr(machine, "bank1", machine->region("maincpu")->base() + 0x20000);
	}
	if ((state->orionpro_dispatcher & 0x08)==0x08)
	{	// ROM2 enabled
		memory_unmap_write(space, 0x2000, 0x3fff, 0, 0);
		memory_set_bankptr(machine, "bank2", machine->region("maincpu")->base() + 0x22000 + (state->orionpro_rom2_segment & 7) * 0x2000);
	}

	if ((state->orionpro_dispatcher & 0x02)==0x00)
	{	// RAM1 segment disabled
		memory_set_bankptr(machine, "bank3", messram_get_ptr(machine->device("messram")) + 0x10000 * page + 0x4000);
	}
	else
	{
		memory_set_bankptr(machine, "bank3", messram_get_ptr(machine->device("messram")) + (state->orionpro_ram1_segment & 31) * 0x4000);
	}

	if ((state->orionpro_dispatcher & 0x04)==0x00)
	{	// RAM2 segment disabled
		memory_set_bankptr(machine, "bank4", messram_get_ptr(machine->device("messram")) + 0x10000 * page + 0x8000);
	}
	else
	{
		memory_set_bankptr(machine, "bank4", messram_get_ptr(machine->device("messram")) + (state->orionpro_ram2_segment & 31) * 0x4000);
	}

	memory_set_bankptr(machine, "bank5", messram_get_ptr(machine->device("messram")) + 0x10000 * page + 0xc000);

	if (is128)
	{
		memory_set_bankptr(machine, "bank6", messram_get_ptr(machine->device("messram")) + 0x10000 * 0 + 0xf000);

		memory_install_write8_handler(space, 0xf400, 0xf4ff, 0, 0, orion128_system_w);
		memory_install_write8_handler(space, 0xf500, 0xf5ff, 0, 0, orion128_romdisk_w);
		memory_unmap_write(space, 0xf600, 0xf6ff, 0, 0);
		memory_install_write8_handler(space, 0xf700, 0xf7ff, 0, 0, orion128_floppy_w);
		memory_install_read8_handler(space, 0xf400, 0xf4ff, 0, 0, orion128_system_r);
		memory_install_read8_handler(space, 0xf500, 0xf5ff, 0, 0, orion128_romdisk_r);
		memory_unmap_read(space, 0xf600, 0xf6ff, 0, 0);
		memory_install_read8_handler(space, 0xf700, 0xf7ff, 0, 0, orion128_floppy_r);

		memory_install_write8_handler(space, 0xf800, 0xf8ff, 0, 0, orion128_video_mode_w);
		memory_install_write8_handler(space, 0xf900, 0xf9ff, 0, 0, orionpro_memory_page_w);
		memory_install_write8_handler(space, 0xfa00, 0xfaff, 0, 0, orion128_video_page_w);
		memory_unmap_write(space, 0xfb00, 0xfeff, 0, 0);
		memory_install_write8_handler(space, 0xff00, 0xffff, 0, 0, orionz80_sound_w);


		memory_set_bankptr(machine, "bank8", messram_get_ptr(machine->device("messram")) + 0x10000 * 0 + 0xf800);
	}
	else
	{
		if ((state->orionpro_dispatcher & 0x40)==0x40)
		{	// FIX F000 enabled
			memory_set_bankptr(machine, "bank6", messram_get_ptr(machine->device("messram")) + 0x10000 * 0 + 0xf000);
			memory_set_bankptr(machine, "bank7", messram_get_ptr(machine->device("messram")) + 0x10000 * 0 + 0xf400);
			memory_set_bankptr(machine, "bank8", messram_get_ptr(machine->device("messram")) + 0x10000 * 0 + 0xf800);
		}
		else
		{
			memory_set_bankptr(machine, "bank6", messram_get_ptr(machine->device("messram")) + 0x10000 * page + 0xf000);
			memory_set_bankptr(machine, "bank7", messram_get_ptr(machine->device("messram")) + 0x10000 * page + 0xf400);
			memory_set_bankptr(machine, "bank8", messram_get_ptr(machine->device("messram")) + 0x10000 * page + 0xf800);
		}
	}
}

static WRITE8_HANDLER ( orionpro_memory_page_w )
{
	orion_state *state = space->machine->driver_data<orion_state>();
	state->orionpro_128_page = data;
	orionpro_bank_switch(space->machine);
}

MACHINE_RESET ( orionpro )
{
	orion_state *state = machine->driver_data<orion_state>();
	radio86_init_keyboard(machine);

	state->orion128_video_page = 0;
	state->orion128_video_mode = 0;
	state->orionpro_ram0_segment = 0;
	state->orionpro_ram1_segment = 0;
	state->orionpro_ram2_segment = 0;

	state->orionpro_page = 0;
	state->orionpro_128_page = 0;
	state->orionpro_rom2_segment = 0;

	state->orionpro_dispatcher = 0x50;
	orionpro_bank_switch(machine);

	state->speaker = 0;
	state->orion128_video_width = SCREEN_WIDTH_384;
	orion_set_video_mode(machine,384);

	state->video_mode_mask = 31;
	state->orionpro_pseudo_color = 0;
}

READ8_HANDLER ( orionpro_io_r )
{
	orion_state *state = space->machine->driver_data<orion_state>();
	device_t *fdc = space->machine->device("wd1793");

	switch (offset & 0xff)
	{
		case 0x00 : return 0x56;
		case 0x04 : return state->orionpro_ram0_segment;
		case 0x05 : return state->orionpro_ram1_segment;
		case 0x06 : return state->orionpro_ram2_segment;
		case 0x08 : return state->orionpro_page;
		case 0x09 : return state->orionpro_rom2_segment;
		case 0x0a : return state->orionpro_dispatcher;
		case 0x10 : return wd17xx_status_r(fdc,0);
		case 0x11 : return wd17xx_track_r(fdc,0);
		case 0x12 : return wd17xx_sector_r(fdc,0);
		case 0x13 : return wd17xx_data_r(fdc,0);
		case 0x18 :
		case 0x19 :
		case 0x1a :
		case 0x1b :
					return orion128_system_r(space,(offset & 0xff)-0x18);
		case 0x28 : return orion128_romdisk_r(space,0);
		case 0x29 : return orion128_romdisk_r(space,1);
		case 0x2a : return orion128_romdisk_r(space,2);
		case 0x2b : return orion128_romdisk_r(space,3);
	}
	if (offset == 0xFFFD)
	{
		return ay8910_r (space->machine->device("ay8912"), 0);
	}
	return 0xff;
}

WRITE8_HANDLER ( orionpro_io_w )
{
	orion_state *state = space->machine->driver_data<orion_state>();
	device_t *fdc = space->machine->device("wd1793");

	switch (offset & 0xff)
	{
		case 0x04 : state->orionpro_ram0_segment = data; orionpro_bank_switch(space->machine); break;
		case 0x05 : state->orionpro_ram1_segment = data; orionpro_bank_switch(space->machine); break;
		case 0x06 : state->orionpro_ram2_segment = data; orionpro_bank_switch(space->machine); break;
		case 0x08 : state->orionpro_page = data;		  orionpro_bank_switch(space->machine); break;
		case 0x09 : state->orionpro_rom2_segment = data; orionpro_bank_switch(space->machine); break;
		case 0x0a : state->orionpro_dispatcher = data;   orionpro_bank_switch(space->machine); break;
		case 0x10 : wd17xx_command_w(fdc,0,data); break;
		case 0x11 : wd17xx_track_w(fdc,0,data);break;
		case 0x12 : wd17xx_sector_w(fdc,0,data);break;
		case 0x13 : wd17xx_data_w(fdc,0,data);break;
		case 0x14 : orion_disk_control_w(space, 9, data);break;
		case 0x18 :
		case 0x19 :
		case 0x1a :
		case 0x1b :
					orion128_system_w(space,(offset & 0xff)-0x18,data); break;
		case 0x28 : orion128_romdisk_w(space,0,data); break;
		case 0x29 : orion128_romdisk_w(space,1,data); break;
		case 0x2a : orion128_romdisk_w(space,2,data); break;
		case 0x2b : orion128_romdisk_w(space,3,data); break;
		case 0xf8 : orion128_video_mode_w(space,0,data);break;
		case 0xf9 : state->orionpro_128_page = data;	  orionpro_bank_switch(space->machine); break;
		case 0xfa : orion128_video_page_w(space,0,data);break;
		case 0xfc : state->orionpro_pseudo_color = data;break;
		case 0xfe : orionz80_sound_fe_w(space,0,data);break;
		case 0xff : orionz80_sound_w(space,0,data);break;
	}
	switch(offset)
	{
		case 0xfffd : ay8910_address_w(space->machine->device("ay8912"), 0, data);
					  break;
		case 0xbffd :
		case 0xbefd : ay8910_data_w(space->machine->device("ay8912"), 0, data);
					  break;
	}
}
