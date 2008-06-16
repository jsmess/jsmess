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
#include "includes/special.h"

static UINT8 specimx_color;
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

static READ8_HANDLER (specialist_8255_porta_r )
{
	if (input_port_read(machine, "LINE0")!=0xff) return 0xfe;
	if (input_port_read(machine, "LINE1")!=0xff) return 0xfd;
	if (input_port_read(machine, "LINE2")!=0xff) return 0xfb;
	if (input_port_read(machine, "LINE3")!=0xff) return 0xf7;
	if (input_port_read(machine, "LINE4")!=0xff) return 0xef;
	if (input_port_read(machine, "LINE5")!=0xff) return 0xdf;
	if (input_port_read(machine, "LINE6")!=0xff) return 0xbf;
	if (input_port_read(machine, "LINE7")!=0xff) return 0x7f;	
	return 0xff;
}

static READ8_HANDLER (specialist_8255_portb_r )
{
	
	int dat = 0;
	double level;	
	
  if ((specialist_8255_porta & 0x01)==0) dat ^= (input_port_read(machine, "LINE0") ^ 0xff);
  if ((specialist_8255_porta & 0x02)==0) dat ^= (input_port_read(machine, "LINE1") ^ 0xff);
  if ((specialist_8255_porta & 0x04)==0) dat ^= (input_port_read(machine, "LINE2") ^ 0xff);
  if ((specialist_8255_porta & 0x08)==0) dat ^= (input_port_read(machine, "LINE3") ^ 0xff);
  if ((specialist_8255_porta & 0x10)==0) dat ^= (input_port_read(machine, "LINE4") ^ 0xff);
  if ((specialist_8255_porta & 0x20)==0) dat ^= (input_port_read(machine, "LINE5") ^ 0xff);
  if ((specialist_8255_porta & 0x40)==0) dat ^= (input_port_read(machine, "LINE6") ^ 0xff);
  if ((specialist_8255_porta & 0x80)==0) dat ^= (input_port_read(machine, "LINE7") ^ 0xff);
  if ((specialist_8255_portc & 0x01)==0) dat ^= (input_port_read(machine, "LINE8") ^ 0xff);
  if ((specialist_8255_portc & 0x02)==0) dat ^= (input_port_read(machine, "LINE9") ^ 0xff);
  if ((specialist_8255_portc & 0x04)==0) dat ^= (input_port_read(machine, "LINE10") ^ 0xff);
  if ((specialist_8255_portc & 0x08)==0) dat ^= (input_port_read(machine, "LINE11") ^ 0xff);
  	
	dat = (dat  << 2) ^0xff;	
	if (input_port_read(machine, "LINE12")!=0xff) dat ^= 0x02;
		
	level = cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0));	 									 					
	if (level >=  0) { 
			dat ^= 0x01;
 	}		
	return dat & 0xff;
}

static READ8_HANDLER (specialist_8255_portc_r )
{
	if (input_port_read(machine, "LINE8")!=0xff) return 0x0e;
	if (input_port_read(machine, "LINE9")!=0xff) return 0x0d;
	if (input_port_read(machine, "LINE10")!=0xff) return 0x0b;
	if (input_port_read(machine, "LINE11")!=0xff) return 0x07;
	return 0x0f;
}

static WRITE8_HANDLER (specialist_8255_porta_w )
{	
	specialist_8255_porta = data;
}

static WRITE8_HANDLER (specialist_8255_portb_w )
{	
	specialist_8255_portb = data;
}
static WRITE8_HANDLER (specialist_8255_portc_w )
{		
	specialist_8255_portc = data;
	
	cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0),data & 0x80 ? 1 : -1);	

	DAC_data_w(0,data & 0x20); //beeper
	
}

const ppi8255_interface specialist_ppi8255_interface =
{
	specialist_8255_porta_r,
	specialist_8255_portb_r,
	specialist_8255_portc_r,
	specialist_8255_porta_w,
	specialist_8255_portb_w,
	specialist_8255_portc_w
};

static TIMER_CALLBACK( special_reset )
{
	memory_set_bank(1, 0);
}


MACHINE_RESET( special )
{
	timer_set(ATTOTIME_IN_USEC(10), NULL, 0, special_reset);
	memory_set_bank(1, 1);	
}

READ8_HANDLER( specialist_keyboard_r )
{	
	return ppi8255_r((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255" ), (offset & 3));
}

WRITE8_HANDLER( specialist_keyboard_w )
{	
	ppi8255_w((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255" ), (offset & 3) , data );
}


/*
	 Specialist MX
*/

static WRITE8_HANDLER( video_memory_w )
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

static void specimx_set_bank(running_machine *machine, int i,int data)
{
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xffbf, 0, 0, SMH_BANK3);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xffc0, 0xffdf, 0, 0, SMH_BANK4);
	memory_set_bankptr(4, mess_ram + 0xffc0);
	switch(i) {
		case 0 :			  
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x8fff, 0, 0, SMH_BANK1);
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xbfff, 0, 0, video_memory_w);
			
				memory_set_bankptr(1, mess_ram);
				memory_set_bankptr(2, mess_ram + 0x9000);
				memory_set_bankptr(3, mess_ram + 0xc000);				
				break;
		case 1 :
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x8fff, 0, 0, SMH_BANK1);
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xbfff, 0, 0, SMH_BANK2);
			
				memory_set_bankptr(1, mess_ram + 0x10000);
				memory_set_bankptr(2, mess_ram + 0x19000);
				memory_set_bankptr(3, mess_ram + 0x1c000);								
				break;
		case 2 :
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x8fff, 0, 0, SMH_UNMAP);
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xbfff, 0, 0, SMH_UNMAP);
			
				memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000);
				memory_set_bankptr(2, memory_region(REGION_CPU1) + 0x19000);
			  if (data & 0x80) {
					memory_set_bankptr(3, mess_ram + 0x1c000);					
				} else {
					memory_set_bankptr(3, mess_ram + 0xc000);					
				}
				break;
	}
}
WRITE8_HANDLER( specimx_select_bank )
{	
	specimx_set_bank(machine, offset, data);	
}

DRIVER_INIT(specimx)
{
	memset(mess_ram,0,128*1024);
}

const struct pit8253_config specimx_pit8253_intf =
{
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
	wd17xx_init(machine, WD_TYPE_1793, NULL , NULL);
	wd17xx_set_density (DEN_FM_HI);
}

static TIMER_CALLBACK( setup_pit8253_gates ) {
	device_config *pit8253 = (device_config*)device_list_find_by_tag( machine->config->devicelist, PIT8253, "pit8253" );

	pit8253_gate_w(pit8253, 0, 0);
	pit8253_gate_w(pit8253, 1, 0);
	pit8253_gate_w(pit8253, 2, 0);
}

MACHINE_RESET( specimx )
{
	specimx_set_bank(machine, 2,0x00); // Initiali load ROM disk
	specimx_color = 0x70;	
	wd17xx_reset(machine);
	wd17xx_set_side(0);
	timer_set( attotime_zero, NULL, 0, setup_pit8253_gates );
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

DEVICE_IMAGE_LOAD( specimx_floppy )
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


/*
	Erik
*/
static UINT8 RR_register;
static UINT8 RC_register;

static void erik_set_bank(running_machine *machine) {		
	UINT8 bank1 = (RR_register & 3);
	UINT8 bank2 = ((RR_register >> 2) & 3);
	UINT8 bank3 = ((RR_register >> 4) & 3);
	UINT8 bank4 = ((RR_register >> 6) & 3);
	
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_BANK1);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x8fff, 0, 0, SMH_BANK2);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xbfff, 0, 0, SMH_BANK3);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xefff, 0, 0, SMH_BANK4);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xf7ff, 0, 0, SMH_BANK5);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf800, 0xffff, 0, 0, SMH_BANK6);
	
	switch(bank1) {
		case 	1: 						
		case 	2:
		case 	3: 			
						memory_set_bankptr(1, mess_ram + 0x10000*(bank1-1));			 
						break;		
		case 	0: 
						memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
						memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000);	
						break;
	}
	switch(bank2) {
		case 	1: 						
		case 	2:
		case 	3: 			
						memory_set_bankptr(2, mess_ram + 0x10000*(bank2-1) + 0x4000);			 
						break;		
		case 	0: 
						memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x8fff, 0, 0, SMH_UNMAP);
						memory_set_bankptr(2, memory_region(REGION_CPU1) + 0x14000);	
						break;
	}
	switch(bank3) {
		case 	1: 												
		case 	2:
		case 	3: 			
						memory_set_bankptr(3, mess_ram + 0x10000*(bank3-1) + 0x9000);			 
						break;		
		case 	0: 
						memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xbfff, 0, 0, SMH_UNMAP);
						memory_set_bankptr(3, memory_region(REGION_CPU1) + 0x19000);	
						break;
	}
	switch(bank4) {
		case 	1: 						
		case 	2:
		case 	3: 			
						memory_set_bankptr(4, mess_ram + 0x10000*(bank4-1) + 0x0c000);			 
						memory_set_bankptr(5, mess_ram + 0x10000*(bank4-1) + 0x0f000);			 
						memory_set_bankptr(6, mess_ram + 0x10000*(bank4-1) + 0x0f800);			 
						break;		
		case 	0: 
						memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xefff, 0, 0, SMH_UNMAP);
						memory_set_bankptr(4, memory_region(REGION_CPU1) + 0x1c000);	
						memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xf7ff, 0, 0, SMH_UNMAP);
						memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xf7ff, 0, 0, SMH_NOP);
						memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf800, 0xffff, 0, 0, specialist_keyboard_w);
						memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf800, 0xffff, 0, 0, specialist_keyboard_r);
						break;
	}
}

DRIVER_INIT(erik)
{	
	memset(mess_ram,0,192*1024);	
	erik_color_1 = 0;
	erik_color_2 = 0;
	erik_background = 0;
}

MACHINE_START( erik )
{
	wd17xx_init(machine, WD_TYPE_1793, NULL , NULL);
}

MACHINE_RESET( erik )
{
	wd17xx_reset(machine);		
	
	RR_register = 0x00;	
	RC_register = 0x00;
	erik_set_bank(machine);				
}

READ8_HANDLER ( erik_rr_reg_r )
{
	return RR_register;
}
WRITE8_HANDLER( erik_rr_reg_w ) 
{
	RR_register = data;
	erik_set_bank(machine);
}

READ8_HANDLER ( erik_rc_reg_r ) 
{
	return RC_register;
}


WRITE8_HANDLER( erik_rc_reg_w ) 
{
	RC_register = data;
	erik_color_1 =  RC_register & 7;
	erik_color_2 =  (RC_register >> 3) & 7;
	erik_background = ((RC_register  >> 6 ) & 1) + ((RC_register  >> 7 ) & 1) * 4;
}

READ8_HANDLER ( erik_disk_reg_r ) {
	return 0xff;	
}

WRITE8_HANDLER( erik_disk_reg_w ) {
	wd17xx_set_side (data & 1);	
	wd17xx_set_drive((data >> 1) & 1);
	if((data >>2) & 1) {
		wd17xx_set_density (DEN_FM_LO);
	} else {
		wd17xx_set_density (DEN_FM_HI);
  }	
}
