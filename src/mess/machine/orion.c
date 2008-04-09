/***************************************************************************

		Orion machine driver by Miodrag Milanovic

		02/04/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "devices/basicdsk.h"
#include "machine/mc146818.h"
#include "machine/8255ppi.h"
#include "machine/wd17xx.h"
#include "sound/speaker.h"
#include "sound/ay8910.h"

#define SCREEN_WIDTH_384 48
#define SCREEN_WIDTH_480 60
#define SCREEN_WIDTH_512 64

UINT8 romdisk_lsb,romdisk_msb;
UINT8 orion_keyboard_line;
UINT8 orion128_video_mode;
UINT8 orion128_video_page;
UINT8 orion128_memory_page;

UINT8 orion128_video_width;

UINT8 orionz80_memory_page;
UINT8 orionz80_dispatcher;

READ8_HANDLER (orion_romdisk_porta_r )
{
	UINT8 *romdisk = memory_region(REGION_CPU1) + 0x10000;		
	switch(input_port_read_indexed(machine, 9)) {
	  case 3 : return romdisk[romdisk_msb*256+romdisk_lsb];	
	  case 1 : return romdisk[romdisk_msb*256+romdisk_lsb+0x10000];		  	
	}
	return 0xff;	
}

WRITE8_HANDLER (orion_romdisk_portb_w )
{	
	romdisk_lsb = data;
}

WRITE8_HANDLER (orion_romdisk_portc_w )
{		
	romdisk_msb = data;	
}

READ8_HANDLER (orion_keyboard_portb_r )
{		
	return input_port_read_indexed(machine, orion_keyboard_line);
}

READ8_HANDLER (orion_keyboard_portc_r )
{
	return input_port_read_indexed(machine, 8);		
}

WRITE8_HANDLER (orion_keyboard_porta_w )
{	
	switch (data ^ 0xff) {
	  	case 0x01 : orion_keyboard_line = 0;break;
	  	case 0x02 : orion_keyboard_line = 1;break;
	  	case 0x04 : orion_keyboard_line = 2;break;
	  	case 0x08 : orion_keyboard_line = 3;break;
	  	case 0x10 : orion_keyboard_line = 4;break;
	  	case 0x20 : orion_keyboard_line = 5;break;
	  	case 0x40 : orion_keyboard_line = 6;break;
	  	case 0x80 : orion_keyboard_line = 7;break;
	}	
}

static const ppi8255_interface orion128_ppi8255_interface =
{
	2,
	{orion_romdisk_porta_r, NULL,									},
	{NULL,									orion_keyboard_portb_r},
	{NULL,									orion_keyboard_portc_r},
	{NULL,									orion_keyboard_porta_w},
	{orion_romdisk_portb_w, NULL,									},
	{orion_romdisk_portc_w, NULL,									}
};

/* Driver initialization */
DRIVER_INIT( orion128 )
{
	memset(mess_ram,0,256*1024);
}


MACHINE_START( orion128 )
{
	wd17xx_init(machine, WD_TYPE_1793, NULL , NULL);
	wd17xx_set_density (DEN_FM_HI);	
}

READ8_HANDLER ( orion128_system_r ) 
{
	return ppi8255_1_r(machine, offset & 3);
}

WRITE8_HANDLER ( orion128_system_w ) 
{
	ppi8255_1_w(machine, offset & 3, data);	
}

READ8_HANDLER ( orion128_romdisk_r ) 
{
	return ppi8255_0_r(machine, offset & 3);	
}

WRITE8_HANDLER ( orion128_romdisk_w ) 
{	
	ppi8255_0_w(machine, offset & 3, data);	
}

void orion_set_video_mode(running_machine *machine, int width) {
		rectangle visarea;
		
		visarea.min_x = 0;
		visarea.min_y = 0;
		visarea.max_x = width-1;
		visarea.max_y = 255;				
		video_screen_configure(machine->primary_screen, width, 256, &visarea, video_screen_get_frame_period(machine->primary_screen).attoseconds);	
}

WRITE8_HANDLER ( orion128_video_mode_w )
{			
	if ((data & 0x80)!=(orion128_video_mode & 0x80)) {
		if ((data & 0x80)==0x80) {
			orion128_video_width = SCREEN_WIDTH_480;		
			orion_set_video_mode(machine,480);		
		} else {
			orion128_video_width = SCREEN_WIDTH_384;
			orion_set_video_mode(machine,384);
		}
	}				
	
	orion128_video_mode = data;
}

WRITE8_HANDLER ( orion128_video_page_w )
{	
	if (orion128_video_page != data) {
		if ((data & 0x80)!=(orion128_video_page & 0x80)) {
			if ((data & 0x80)==0x80) {
				orion128_video_width = SCREEN_WIDTH_480;		
				orion_set_video_mode(machine,480);		
			} else {
				orion128_video_width = SCREEN_WIDTH_384;
				orion_set_video_mode(machine,384);
			}
		}				
	}
	orion128_video_page = data;
}


WRITE8_HANDLER ( orion128_memory_page_w )
{				
	if (data!=orion128_memory_page ) {
		memory_set_bankptr(1, mess_ram + (data & 3) * 0x10000);
		orion128_memory_page = (data & 3);
	}
}

MACHINE_RESET ( orion128 ) 
{		
	wd17xx_reset();
	ppi8255_init(&orion128_ppi8255_interface);
	orion_keyboard_line = 0;
	orion128_video_page = 0;
	orion128_video_mode = 0;
	orion128_memory_page = -1;
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0xf800);
	memory_set_bankptr(2, mess_ram + 0xf000);
	orion128_video_width = SCREEN_WIDTH_384;
	orion_set_video_mode(machine,384);
}


DEVICE_IMAGE_LOAD( orion_floppy )
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

WRITE8_HANDLER ( orion_disk_control_w )
{
	wd17xx_set_side(((data & 0x10) >> 4) ^ 1);
 	wd17xx_set_drive(data & 3);				
}

READ8_HANDLER ( orion128_floppy_r )
{	
	switch(offset) {
		case 0x0	:
		case 0x10 : return wd17xx_status_r(machine,0);
		case 0x1 	:
		case 0x11 : return wd17xx_track_r(machine,0);
		case 0x2  :
		case 0x12 : return wd17xx_sector_r(machine,0);
		case 0x3  :
		case 0x13 : return wd17xx_data_r(machine,0);
	}	
	return 0xff;
}

WRITE8_HANDLER ( orion128_floppy_w )
{		
	switch(offset) {
		case 0x0	:
		case 0x10 : wd17xx_command_w(machine,0,data); break;
		case 0x1 	:
		case 0x11 : wd17xx_track_w(machine,0,data);break;
		case 0x2  :
		case 0x12 : wd17xx_sector_w(machine,0,data);break;
		case 0x3  :
		case 0x13 : wd17xx_data_w(machine,0,data);break;
		case 0x4  : 
		case 0x14 : 
		case 0x20 : orion_disk_control_w(machine, offset, data);break;
	}
}
READ8_HANDLER ( orionz80_floppy_rtc_r )
{	
	if ((offset >= 0x60) && (offset <= 0x6f)) {
		return mc146818_port_r(machine,offset-0x60);
	} else { 
		return orion128_floppy_r(machine,offset);
	}	
}

WRITE8_HANDLER ( orionz80_floppy_rtc_w )
{		
	if ((offset >= 0x60) && (offset <= 0x6f)) {
		return mc146818_port_w(machine,offset-0x60,data);
	} else { 
		return orion128_floppy_w(machine,offset,data);
	}	
}


DRIVER_INIT( orionz80 )
{
	memset(mess_ram,0,512*1024);	
}


MACHINE_START( orionz80 )
{
	wd17xx_init(machine, WD_TYPE_1793, NULL , NULL);
	wd17xx_set_density (DEN_FM_HI);
	mc146818_init(MC146818_IGNORE_CENTURY);
}

UINT8 orion_speaker;
WRITE8_HANDLER ( orionz80_sound_w )
{	
	if (orion_speaker==0) {
		orion_speaker = data;		
	} else {
		orion_speaker = 0 ;	
	}
	speaker_level_w(0,orion_speaker);
		
}

WRITE8_HANDLER ( orionz80_sound_fe_w )
{	
	speaker_level_w(0,(data>>4) & 0x01);
}


WRITE8_HANDLER ( orionz80_memory_page_w );
WRITE8_HANDLER ( orionz80_dispatcher_w );

void orionz80_switch_bank(void)
{
	UINT8 bank_select;
	UINT8 segment_select;
	
	bank_select = (orionz80_dispatcher & 0x0c) >> 2;
	segment_select = orionz80_dispatcher & 0x03;
	
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_BANK1);
	if ((orionz80_dispatcher & 0x80)==0) { // dispatcher on
		memory_set_bankptr(1, mess_ram + 0x10000 * bank_select + segment_select * 0x4000 );		
	} else { // dispatcher off
		memory_set_bankptr(1, mess_ram + 0x10000 * orionz80_memory_page);		
	}
		
	memory_set_bankptr(2, mess_ram + 0x4000 + 0x10000 * orionz80_memory_page);		
	
	if ((orionz80_dispatcher & 0x20) == 0) {
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf400, 0xf4ff, 0, 0, orion128_system_w);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf500, 0xf5ff, 0, 0, orion128_romdisk_w);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf700, 0xf7ff, 0, 0, orionz80_floppy_rtc_w);	
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf400, 0xf4ff, 0, 0, orion128_system_r);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf500, 0xf5ff, 0, 0, orion128_romdisk_r);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf700, 0xf7ff, 0, 0, orionz80_floppy_rtc_r);	
		
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf800, 0xf8ff, 0, 0, orion128_video_mode_w);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf900, 0xf9ff, 0, 0, orionz80_memory_page_w);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfa00, 0xfaff, 0, 0, orion128_video_page_w);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfb00, 0xfbff, 0, 0, orionz80_dispatcher_w);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfc00, 0xfeff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xff00, 0xffff, 0, 0, orionz80_sound_w);
		
		memory_set_bankptr(3, mess_ram + 0xf000);				
		memory_set_bankptr(5, memory_region(REGION_CPU1) + 0xf800);		
		
	} else {
		/* if it is full memory access */
		memory_set_bankptr(3, mess_ram + 0xf000 + 0x10000 * orionz80_memory_page);		
		memory_set_bankptr(4, mess_ram + 0xf400 + 0x10000 * orionz80_memory_page);		
		memory_set_bankptr(5, mess_ram + 0xf800 + 0x10000 * orionz80_memory_page);		
	}		
}

WRITE8_HANDLER ( orionz80_memory_page_w )
{	
	orionz80_memory_page = data & 7;
	orionz80_switch_bank();
}

WRITE8_HANDLER ( orionz80_dispatcher_w )
{	
	orionz80_dispatcher = data;
	orionz80_switch_bank();
}

MACHINE_RESET ( orionz80 ) 
{	
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0xefff, 0, 0, SMH_BANK2);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xf3ff, 0, 0, SMH_BANK3);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf400, 0xf4ff, 0, 0, orion128_system_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf500, 0xf5ff, 0, 0, orion128_romdisk_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf700, 0xf7ff, 0, 0, orionz80_floppy_rtc_w);	
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf400, 0xf4ff, 0, 0, orion128_system_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf500, 0xf5ff, 0, 0, orion128_romdisk_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf700, 0xf7ff, 0, 0, orionz80_floppy_rtc_r);	
	
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf800, 0xf8ff, 0, 0, orion128_video_mode_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf900, 0xf9ff, 0, 0, orionz80_memory_page_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfa00, 0xfaff, 0, 0, orion128_video_page_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfb00, 0xfbff, 0, 0, orionz80_dispatcher_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfc00, 0xfeff, 0, 0, SMH_UNMAP);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xff00, 0xffff, 0, 0, orionz80_sound_w);
	
	
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0xf800);		
	memory_set_bankptr(2, mess_ram + 0x4000);		
	memory_set_bankptr(3, mess_ram + 0xf000);		
	memory_set_bankptr(5, memory_region(REGION_CPU1) + 0xf800);		
	
	wd17xx_reset();
	ppi8255_init(&orion128_ppi8255_interface);
	orion_keyboard_line = 0;
	orion128_video_page = 0;
	orion128_video_mode = 0;
	orionz80_memory_page = 0;
	orionz80_dispatcher = 0;
	orion_speaker = 0;
	orion128_video_width = SCREEN_WIDTH_384;
	orion_set_video_mode(machine,384);
}

INTERRUPT_GEN( orionz80_interrupt ) 
{
	if ((orionz80_dispatcher & 0x40)==0x40) {
		cpunum_set_input_line(machine, 0, 0, HOLD_LINE);			
	}	
}

READ8_HANDLER ( orionz80_io_r ) {
	if (offset == 0xFFFD) {
		return AY8910_read_port_0_r (machine, 0);
	}
	return 0xff;
}

WRITE8_HANDLER ( orionz80_io_w ) {
	switch (offset & 0xff) {
		case 0xf8 : orion128_video_mode_w(machine,0,data);break;
		case 0xf9 : orionz80_memory_page_w(machine,0,data);break;
		case 0xfa : orion128_video_page_w(machine,0,data);break;
		case 0xfb : orionz80_dispatcher_w(machine,0,data);break;
		case 0xfe : orionz80_sound_fe_w(machine,0,data);break;
		case 0xff : orionz80_sound_w(machine,0,data);break;
	}
	switch(offset) {
		case 0xfffd : AY8910_control_port_0_w(machine, 0, data);
					  break;
		case 0xbffd :
		case 0xbefd : AY8910_write_port_0_w(machine, 0, data);
		 			  break;		
	}
}
      
