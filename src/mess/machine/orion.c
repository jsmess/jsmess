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

static UINT8 romdisk_lsb,romdisk_msb;
UINT8 orion128_video_mode;
UINT8 orion128_video_page;
static UINT8 orion128_memory_page;

UINT8 orion128_video_width;

static UINT8 orionz80_memory_page;
static UINT8 orionz80_dispatcher;

UINT8 orion_video_mode_mask;

static READ8_DEVICE_HANDLER (orion_romdisk_porta_r )
{
	UINT8 *romdisk = memory_region(device->machine, "maincpu") + 0x10000;
	return romdisk[romdisk_msb*256+romdisk_lsb];
}

static WRITE8_DEVICE_HANDLER (orion_romdisk_portb_w )
{
	romdisk_lsb = data;
}

static WRITE8_DEVICE_HANDLER (orion_romdisk_portc_w )
{
	romdisk_msb = data;
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

/* Driver initialization */
DRIVER_INIT( orion128 )
{
	memset(messram_get_ptr(devtag_get_device(machine, "messram")),0,256*1024);
}


MACHINE_START( orion128 )
{
	const device_config *fdc = devtag_get_device(machine, "wd1793");

	wd17xx_set_density (fdc,DEN_FM_HI);
	orion_video_mode_mask = 7;
}

READ8_HANDLER ( orion128_system_r )
{
	return i8255a_r(devtag_get_device(space->machine, "ppi8255_2"), offset & 3);
}

WRITE8_HANDLER ( orion128_system_w )
{
	i8255a_w(devtag_get_device(space->machine, "ppi8255_2"), offset & 3, data);
}

READ8_HANDLER ( orion128_romdisk_r )
{
	return i8255a_r(devtag_get_device(space->machine, "ppi8255_1"), offset & 3);
}

WRITE8_HANDLER ( orion128_romdisk_w )
{
	i8255a_w(devtag_get_device(space->machine, "ppi8255_1"), offset & 3, data);
}

static void orion_set_video_mode(running_machine *machine, int width)
{
		rectangle visarea;

		visarea.min_x = 0;
		visarea.min_y = 0;
		visarea.max_x = width-1;
		visarea.max_y = 255;
		video_screen_configure(machine->primary_screen, width, 256, &visarea, video_screen_get_frame_period(machine->primary_screen).attoseconds);
}

WRITE8_HANDLER ( orion128_video_mode_w )
{
	if ((data & 0x80)!=(orion128_video_mode & 0x80))
	{
		if ((data & 0x80)==0x80)
		{
			if (orion_video_mode_mask == 31)
			{
				orion128_video_width = SCREEN_WIDTH_512;
				orion_set_video_mode(space->machine,512);
			}
			else
			{
				orion128_video_width = SCREEN_WIDTH_480;
				orion_set_video_mode(space->machine,480);
			}
		}
		else
		{
			orion128_video_width = SCREEN_WIDTH_384;
			orion_set_video_mode(space->machine,384);
		}
	}

	orion128_video_mode = data;
}

WRITE8_HANDLER ( orion128_video_page_w )
{
	if (orion128_video_page != data)
	{
		if ((data & 0x80)!=(orion128_video_page & 0x80))
		{
			if ((data & 0x80)==0x80)
			{
				if (orion_video_mode_mask == 31)
				{
					orion128_video_width = SCREEN_WIDTH_512;
					orion_set_video_mode(space->machine,512);
				}
				else
				{
					orion128_video_width = SCREEN_WIDTH_480;
					orion_set_video_mode(space->machine,480);
				}
			}
			else
			{
				orion128_video_width = SCREEN_WIDTH_384;
				orion_set_video_mode(space->machine,384);
			}
		}
	}
	orion128_video_page = data;
}


WRITE8_HANDLER ( orion128_memory_page_w )
{
	if (data!=orion128_memory_page )
	{
		memory_set_bankptr(space->machine, "bank1", messram_get_ptr(devtag_get_device(space->machine, "messram")) + (data & 3) * 0x10000);
		orion128_memory_page = (data & 3);
	}
}

MACHINE_RESET ( orion128 )
{
	orion128_video_page = 0;
	orion128_video_mode = 0;
	orion128_memory_page = -1;
	memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu") + 0xf800);
	memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf000);
	orion128_video_width = SCREEN_WIDTH_384;
	orion_set_video_mode(machine,384);
	radio86_init_keyboard();
}

static WRITE8_HANDLER ( orion_disk_control_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");

	wd17xx_set_side(fdc,((data & 0x10) >> 4) ^ 1);
 	wd17xx_set_drive(fdc,data & 3);
}

READ8_HANDLER ( orion128_floppy_r )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");

	switch(offset)
	{
		case 0x0	:
		case 0x10 : return wd17xx_status_r(fdc,0);
		case 0x1 	:
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
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");

	switch(offset)
	{
		case 0x0	:
		case 0x10 : wd17xx_command_w(fdc,0,data); break;
		case 0x1 	:
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
		return mc146818_port_r(space,offset-0x60);
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
		mc146818_port_w(space,offset-0x60,data);
	}
	else
	{
		orion128_floppy_w(space,offset,data);
	}
}


DRIVER_INIT( orionz80 )
{
	memset(messram_get_ptr(devtag_get_device(machine, "messram")),0,512*1024);
}


MACHINE_START( orionz80 )
{
	const device_config *fdc = devtag_get_device(machine, "wd1793");

	wd17xx_set_density (fdc,DEN_FM_HI);
	mc146818_init(machine, MC146818_IGNORE_CENTURY);
	orion_video_mode_mask = 7;
}

static UINT8 orion_speaker;
WRITE8_HANDLER ( orionz80_sound_w )
{
	const device_config *speaker = devtag_get_device(space->machine, "speaker");
	if (orion_speaker == 0)
	{
		orion_speaker = data;
	}
	else
	{
		orion_speaker = 0 ;
	}
	speaker_level_w(speaker,orion_speaker);

}

static WRITE8_HANDLER ( orionz80_sound_fe_w )
{
	const device_config *speaker = devtag_get_device(space->machine, "speaker");
	speaker_level_w(speaker,(data>>4) & 0x01);
}


static void orionz80_switch_bank(running_machine *machine)
{
	UINT8 bank_select;
	UINT8 segment_select;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	bank_select = (orionz80_dispatcher & 0x0c) >> 2;
	segment_select = orionz80_dispatcher & 0x03;

	memory_install_write_bank(space, 0x0000, 0x3fff, 0, 0, "bank1");
	if ((orionz80_dispatcher & 0x80)==0)
	{ // dispatcher on
		memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * bank_select + segment_select * 0x4000 );
	}
	else
	{ // dispatcher off
		memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * orionz80_memory_page);
	}

	memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000 + 0x10000 * orionz80_memory_page);

	if ((orionz80_dispatcher & 0x20) == 0)
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

		memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf000);
		memory_set_bankptr(machine, "bank5", memory_region(machine, "maincpu") + 0xf800);

	}
	else
	{
		/* if it is full memory access */
		memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf000 + 0x10000 * orionz80_memory_page);
		memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf400 + 0x10000 * orionz80_memory_page);
		memory_set_bankptr(machine, "bank5", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf800 + 0x10000 * orionz80_memory_page);
	}
}

WRITE8_HANDLER ( orionz80_memory_page_w )
{
	orionz80_memory_page = data & 7;
	orionz80_switch_bank(space->machine);
}

WRITE8_HANDLER ( orionz80_dispatcher_w )
{
	orionz80_dispatcher = data;
	orionz80_switch_bank(space->machine);
}

MACHINE_RESET ( orionz80 )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

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


	memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu") + 0xf800);
	memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);
	memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xf000);
	memory_set_bankptr(machine, "bank5", memory_region(machine, "maincpu") + 0xf800);


	orion128_video_page = 0;
	orion128_video_mode = 0;
	orionz80_memory_page = 0;
	orionz80_dispatcher = 0;
	orion_speaker = 0;
	orion128_video_width = SCREEN_WIDTH_384;
	orion_set_video_mode(machine,384);
	radio86_init_keyboard();
}

INTERRUPT_GEN( orionz80_interrupt )
{
	if ((orionz80_dispatcher & 0x40)==0x40)
	{
		cpu_set_input_line(device, 0, HOLD_LINE);
	}
}

READ8_HANDLER ( orionz80_io_r )
{
	if (offset == 0xFFFD)
	{
		return ay8910_r (devtag_get_device(space->machine, "ay8912"), 0);
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
		case 0xfffd : ay8910_address_w(devtag_get_device(space->machine, "ay8912"), 0, data);
					  break;
		case 0xbffd :
		case 0xbefd : ay8910_data_w(devtag_get_device(space->machine, "ay8912"), 0, data);
		 			  break;
	}
}

static UINT8 orionpro_ram0_segment;
static UINT8 orionpro_ram1_segment;
static UINT8 orionpro_ram2_segment;

static UINT8 orionpro_page;
static UINT8 orionpro_128_page;
static UINT8 orionpro_rom2_segment;

static UINT8 orionpro_dispatcher;
UINT8 orionpro_pseudo_color;

DRIVER_INIT( orionpro )
{
	memset(messram_get_ptr(devtag_get_device(machine, "messram")),0,512*1024);
}


MACHINE_START( orionpro )
{
	const device_config *fdc = devtag_get_device(machine, "wd1793");
	wd17xx_set_density (fdc,DEN_FM_HI);
}

static WRITE8_HANDLER ( orionpro_memory_page_w );

static void orionpro_bank_switch(running_machine *machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int page = orionpro_page & 7; // we have only 8 pages
	int is128 = (orionpro_dispatcher & 0x80) ? 1 : 0;
	if (is128==1)
	{
		page = orionpro_128_page & 7;
	}
	memory_install_write_bank(space, 0x0000, 0x1fff, 0, 0, "bank1");
	memory_install_write_bank(space, 0x2000, 0x3fff, 0, 0, "bank2");
	memory_install_write_bank(space, 0x4000, 0x7fff, 0, 0, "bank3");
	memory_install_write_bank(space, 0x8000, 0xbfff, 0, 0, "bank4");
	memory_install_write_bank(space, 0xc000, 0xefff, 0, 0, "bank5");
	memory_install_write_bank(space, 0xf000, 0xf3ff, 0, 0, "bank6");
	memory_install_write_bank(space, 0xf400, 0xf7ff, 0, 0, "bank7");
	memory_install_write_bank(space, 0xf800, 0xffff, 0, 0, "bank8");


	if ((orionpro_dispatcher & 0x01)==0x00)
	{	// RAM0 segment disabled
		memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * page);
		memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * page + 0x2000);
	}
	else
	{
		memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")) + (orionpro_ram0_segment & 31) * 0x4000);
		memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + (orionpro_ram0_segment & 31) * 0x4000 + 0x2000);
	}
	if ((orionpro_dispatcher & 0x10)==0x10)
	{	// ROM1 enabled
		memory_unmap_write(space, 0x0000, 0x1fff, 0, 0);
		memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu") + 0x20000);
	}
	if ((orionpro_dispatcher & 0x08)==0x08)
	{	// ROM2 enabled
		memory_unmap_write(space, 0x2000, 0x3fff, 0, 0);
		memory_set_bankptr(machine, "bank2", memory_region(machine, "maincpu") + 0x22000 + (orionpro_rom2_segment & 7) * 0x2000);
	}

	if ((orionpro_dispatcher & 0x02)==0x00)
	{	// RAM1 segment disabled
		memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * page + 0x4000);
	}
	else
	{
		memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + (orionpro_ram1_segment & 31) * 0x4000);
	}

	if ((orionpro_dispatcher & 0x04)==0x00)
	{	// RAM2 segment disabled
		memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * page + 0x8000);
	}
	else
	{
		memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + (orionpro_ram2_segment & 31) * 0x4000);
	}

	memory_set_bankptr(machine, "bank5", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * page + 0xc000);

	if (is128)
	{
		memory_set_bankptr(machine, "bank6", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * 0 + 0xf000);

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


		memory_set_bankptr(machine, "bank8", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * 0 + 0xf800);
	}
	else
	{
		if ((orionpro_dispatcher & 0x40)==0x40)
		{	// FIX F000 enabled
			memory_set_bankptr(machine, "bank6", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * 0 + 0xf000);
			memory_set_bankptr(machine, "bank7", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * 0 + 0xf400);
			memory_set_bankptr(machine, "bank8", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * 0 + 0xf800);
		}
		else
		{
			memory_set_bankptr(machine, "bank6", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * page + 0xf000);
			memory_set_bankptr(machine, "bank7", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * page + 0xf400);
			memory_set_bankptr(machine, "bank8", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000 * page + 0xf800);
		}
	}
}

static WRITE8_HANDLER ( orionpro_memory_page_w )
{
	orionpro_128_page = data;
	orionpro_bank_switch(space->machine);
}

MACHINE_RESET ( orionpro )
{
	radio86_init_keyboard();

	orion128_video_page = 0;
	orion128_video_mode = 0;
	orionpro_ram0_segment = 0;
	orionpro_ram1_segment = 0;
	orionpro_ram2_segment = 0;

	orionpro_page = 0;
	orionpro_128_page = 0;
	orionpro_rom2_segment = 0;

	orionpro_dispatcher = 0x50;
	orionpro_bank_switch(machine);

	orion_speaker = 0;
	orion128_video_width = SCREEN_WIDTH_384;
	orion_set_video_mode(machine,384);

	orion_video_mode_mask = 31;
	orionpro_pseudo_color = 0;
}

READ8_HANDLER ( orionpro_io_r )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");

	switch (offset & 0xff)
	{
		case 0x00 : return 0x56;
		case 0x04 : return orionpro_ram0_segment;
		case 0x05 : return orionpro_ram1_segment;
		case 0x06 : return orionpro_ram2_segment;
		case 0x08 : return orionpro_page;
		case 0x09 : return orionpro_rom2_segment;
		case 0x0a : return orionpro_dispatcher;
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
		return ay8910_r (devtag_get_device(space->machine, "ay8912"), 0);
	}
	return 0xff;
}

WRITE8_HANDLER ( orionpro_io_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");

	switch (offset & 0xff)
	{
		case 0x04 : orionpro_ram0_segment = data; orionpro_bank_switch(space->machine); break;
		case 0x05 : orionpro_ram1_segment = data; orionpro_bank_switch(space->machine); break;
		case 0x06 : orionpro_ram2_segment = data; orionpro_bank_switch(space->machine); break;
		case 0x08 : orionpro_page = data; 		  orionpro_bank_switch(space->machine); break;
		case 0x09 : orionpro_rom2_segment = data; orionpro_bank_switch(space->machine); break;
		case 0x0a : orionpro_dispatcher = data;   orionpro_bank_switch(space->machine); break;
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
		case 0xf9 : orionpro_128_page = data;	  orionpro_bank_switch(space->machine); break;
		case 0xfa : orion128_video_page_w(space,0,data);break;
		case 0xfc : orionpro_pseudo_color = data;break;
		case 0xfe : orionz80_sound_fe_w(space,0,data);break;
		case 0xff : orionz80_sound_w(space,0,data);break;
	}
	switch(offset)
	{
		case 0xfffd : ay8910_address_w(devtag_get_device(space->machine, "ay8912"), 0, data);
					  break;
		case 0xbffd :
		case 0xbefd : ay8910_data_w(devtag_get_device(space->machine, "ay8912"), 0, data);
		 			  break;
	}
}
