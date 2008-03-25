/***************************************************************************

		Specialist machine driver by Miodrag Milanovic

		20/03/2008 Cassette support
		15/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "sound/dac.h"
#include "devices/cassette.h"
#include "devices/basicdsk.h"
#include "machine/8255ppi.h"
#include "machine/pit8253.h"
#include "machine/wd17xx.h"

UINT8 specimx_color;
UINT8 *specimx_colorram;

static int specialist_8255_porta;
static int specialist_8255_portb;
static int specialist_8255_portc;

/* Driver initialization */
DRIVER_INIT(special)
{
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = memory_region(REGION_CPU1);	
	memset(RAM,0x0000,0x3000); // make frist page empty by default
	memory_configure_bank(1, 1, 2, RAM, 0x0000);
	memory_configure_bank(1, 0, 2, RAM, 0xc000);	
}

READ8_HANDLER (specialist_8255_porta_r )
{
	if (readinputport(0)!=0xff) return 0xfe;
	if (readinputport(1)!=0xff) return 0xfd;
	if (readinputport(2)!=0xff) return 0xfb;
	if (readinputport(3)!=0xff) return 0xf7;
	if (readinputport(4)!=0xff) return 0xef;
	if (readinputport(5)!=0xff) return 0xdf;
	if (readinputport(6)!=0xff) return 0xbf;
	if (readinputport(7)!=0xff) return 0x7f;	
	return 0xff;
}

READ8_HANDLER (specialist_8255_portb_r )
{
	
	int dat = 0;
	double level;	
	
    if ((specialist_8255_porta & 0x01)==0) dat ^= (readinputport(0) ^ 0xff);
	if ((specialist_8255_porta & 0x02)==0) dat ^= (readinputport(1) ^ 0xff);
    if ((specialist_8255_porta & 0x04)==0) dat ^= (readinputport(2) ^ 0xff);
    if ((specialist_8255_porta & 0x08)==0) dat ^= (readinputport(3) ^ 0xff);
	if ((specialist_8255_porta & 0x10)==0) dat ^= (readinputport(4) ^ 0xff);
	if ((specialist_8255_porta & 0x20)==0) dat ^= (readinputport(5) ^ 0xff);
    if ((specialist_8255_porta & 0x40)==0) dat ^= (readinputport(6) ^ 0xff);
    if ((specialist_8255_porta & 0x80)==0) dat ^= (readinputport(7) ^ 0xff);
	if ((specialist_8255_portc & 0x01)==0) dat ^= (readinputport(8) ^ 0xff);
	if ((specialist_8255_portc & 0x02)==0) dat ^= (readinputport(9) ^ 0xff);
    if ((specialist_8255_portc & 0x04)==0) dat ^= (readinputport(10) ^ 0xff);
    if ((specialist_8255_portc & 0x08)==0) dat ^= (readinputport(11) ^ 0xff);
  	
	dat = (dat  << 2) ^0xff;	
	if (readinputport(12)!=0xff) dat ^= 0x02;
		
	level = cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0));	 									 					
	if (level >=  0) { 
			dat ^= 0x01;
 	}		
	return dat & 0xff;
}

READ8_HANDLER (specialist_8255_portc_r )
{
	if (readinputport(8)!=0xff) return 0x0e;
	if (readinputport(9)!=0xff) return 0x0d;
	if (readinputport(10)!=0xff) return 0x0b;
	if (readinputport(11)!=0xff) return 0x07;
	return 0x0f;
}

WRITE8_HANDLER (specialist_8255_porta_w )
{	
	specialist_8255_porta = data;
}

WRITE8_HANDLER (specialist_8255_portb_w )
{	
	specialist_8255_portb = data;
}
WRITE8_HANDLER (specialist_8255_portc_w )
{		
	specialist_8255_portc = data;
	
	cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0),data & 0x80 ? 1 : -1);	

	DAC_data_w(0,data & 0x20); //beeper
	
}

static const ppi8255_interface specialist_ppi8255_interface =
{
	1,
	{specialist_8255_porta_r},
	{specialist_8255_portb_r},
	{specialist_8255_portc_r},
	{specialist_8255_porta_w},
	{specialist_8255_portb_w},
	{specialist_8255_portc_w}
};

static TIMER_CALLBACK( special_reset )
{
	memory_set_bank(1, 0);
}


MACHINE_RESET( special )
{
	timer_set(ATTOTIME_IN_USEC(10), NULL, 0, special_reset);
	memory_set_bank(1, 1);	

	ppi8255_init(&specialist_ppi8255_interface);
}

READ8_HANDLER( specialist_keyboard_r )
{	
	return ppi8255_0_r(machine, (offset & 3));	
}

WRITE8_HANDLER( specialist_keyboard_w )
{
	ppi8255_0_w(machine, (offset & 3) , data );
}


/*
	 Specialist MX
*/

WRITE8_HANDLER( video_memory_w )
{
	mess_ram[0x9000 + offset] = data;
	specimx_colorram[offset]  = specimx_color;
}

WRITE8_HANDLER (specimx_video_color_w )
{		
	specimx_color = data;
}

READ8_HANDLER (specimx_video_color_r )
{
	return specimx_color;
}


void specimx_set_bank(int i) {	
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xffbf, 0, 0, SMH_BANK3);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xffc0, 0xffdf, 0, 0, SMH_BANK4);
	memory_set_bankptr(4, mess_ram + 0xffc0);
	switch(i) {
		case 0 :
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x8fff, 0, 0, SMH_BANK1);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xbfff, 0, 0, video_memory_w);
			
				memory_set_bankptr(1, mess_ram);
				memory_set_bankptr(2, mess_ram + 0x9000);
				memory_set_bankptr(3, mess_ram + 0xc000);				
				break;
		case 1 :
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x8fff, 0, 0, SMH_BANK1);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xbfff, 0, 0, SMH_BANK2);
			
				memory_set_bankptr(1, mess_ram + 0x10000);
				memory_set_bankptr(2, mess_ram + 0x19000);
				memory_set_bankptr(3, mess_ram + 0x1c000);								
				break;
		case 2 :
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x8fff, 0, 0, SMH_UNMAP);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xbfff, 0, 0, SMH_UNMAP);
			
				memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000);
				memory_set_bankptr(2, memory_region(REGION_CPU1) + 0x19000);
				memory_set_bankptr(3, mess_ram + 0xc000);
				break;
	}
}
WRITE8_HANDLER( specimx_select_bank )
{	
	specimx_set_bank(offset);	
}

void specimx_fdc_callback(running_machine *machine, wd17xx_state_t event, void *param)
{
	switch (event)
	{
		case WD17XX_IRQ_CLR:
		case WD17XX_IRQ_SET:
		case WD17XX_DRQ_CLR:
		case WD17XX_DRQ_SET:
			/* do nothing */
			break;
	}
}

DRIVER_INIT(specimx)
{
	memset(mess_ram,0,256*1024);
}

static const struct pit8253_config specimx_pit8253_intf =
{
	TYPE8253,
	{
		{
			2000000,
			NULL,
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
			NULL
		}
	}
};

MACHINE_START( specimx )
{
	wd17xx_init(machine, WD_TYPE_1793, specimx_fdc_callback, NULL);
	wd17xx_set_density (DEN_FM_HI);
	pit8253_init(1, &specimx_pit8253_intf);
}

MACHINE_RESET( specimx )
{
	ppi8255_init(&specialist_ppi8255_interface);
	specimx_set_bank(2); // Initiali load ROM disk
	specimx_color = 0x70;	
	wd17xx_reset();
	wd17xx_set_side(0);
	pit8253_reset(0);
	pit8253_0_gate_w(machine, 0, 0);
	pit8253_0_gate_w(machine, 1, 0);
	pit8253_0_gate_w(machine, 2, 0);
}

READ8_HANDLER ( specimx_disk_data_r )
{
 switch(offset) {  		
	case 0 :
			return wd17xx_status_r(machine, offset);
	case 1 : 
			return wd17xx_track_r(machine, offset);
    case 2 : 
			return wd17xx_sector_r(machine, offset);
	case 3 : 
			return wd17xx_data_r(machine, offset);
  }  
  return 0;
}
WRITE8_HANDLER( specimx_disk_data_w )
{		
  switch(offset) {  		
		case 0 : 
			wd17xx_command_w(machine, offset, data);break;
		case 1 : 
			wd17xx_track_w (machine, offset, data);break;
		case 2 : 
			wd17xx_sector_w (machine, offset, data);break;
		case 3 : 
			wd17xx_data_w (machine, offset, data);break;
  }  
}

READ8_HANDLER ( specimx_disk_ctrl_r )
{
  return 0xff;
}

WRITE8_HANDLER( specimx_disk_ctrl_w )
{	

	switch(offset) {  				
		case 2 :						
		 		wd17xx_set_side(data & 1);							
				break;			
		case 3 :				
		 		wd17xx_set_drive(data & 1);				
		 		break;
			
  }  
}

static unsigned long specimx_calcoffset(UINT8 t, UINT8 h, UINT8 s,
	UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT8 first_sector_id, UINT16 offset_track_zero)
{
	unsigned long o;	
    o = (t * 1024 * 5 * 2) + (h * 1024 * 5) + 1024 * (s-1);
	return o;
}

DEVICE_LOAD( specimx_floppy )
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

	basicdsk_set_geometry (image, 80, 2, 6, 1024, 1, 0, FALSE);
	basicdsk_set_calcoffset(image, specimx_calcoffset);
	return INIT_PASS;
}

WRITE8_HANDLER( specimx_sound_w)
{
	
		pit8253_0_w(machine,offset,data);		
}
