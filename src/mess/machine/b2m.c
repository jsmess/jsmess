/***************************************************************************

		Bashkiria-2M machine driver by Miodrag Milanovic

		28/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "deprecat.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "devices/basicdsk.h"
#include "machine/8255ppi.h"
#include "machine/pit8253.h"
#include "machine/wd17xx.h"
#include "machine/pic8259.h"
#include "machine/msm8251.h"
#include "sound/custom.h"
#include "streams.h"

static int b2m_8255_porta;
UINT16 b2m_video_scroll;
static int b2m_8255_portc;

UINT8 b2m_video_page;
static UINT8 b2m_drive;
static UINT8 b2m_side;

static UINT8 b2m_romdisk_lsb;
static UINT8 b2m_romdisk_msb;

static UINT8 b2m_color[4];
static UINT8 b2m_localmachine;

READ8_HANDLER (b2m_keyboard_r )
{		
	UINT8 key = 0x00;
	if (offset < 0x100) {
		if ((offset & 0x01)!=0) { key |= input_port_read(machine,"LINE0"); }
		if ((offset & 0x02)!=0) { key |= input_port_read(machine,"LINE1"); }
		if ((offset & 0x04)!=0) { key |= input_port_read(machine,"LINE2"); }
		if ((offset & 0x08)!=0) { key |= input_port_read(machine,"LINE3"); }
		if ((offset & 0x10)!=0) { key |= input_port_read(machine,"LINE4"); }
		if ((offset & 0x20)!=0) { key |= input_port_read(machine,"LINE5"); }
		if ((offset & 0x40)!=0) { key |= input_port_read(machine,"LINE6"); }
		if ((offset & 0x80)!=0) { key |= input_port_read(machine,"LINE7"); }					
	} else {
		if ((offset & 0x01)!=0) { key |= input_port_read(machine,"LINE8"); }
		if ((offset & 0x02)!=0) { key |= input_port_read(machine,"LINE9"); }
		if ((offset & 0x04)!=0) { key |= input_port_read(machine,"LINE10"); }
	}
	return key;
}


void b2m_set_bank(running_machine *machine,int bank) 
{
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x27ff, 0, 0, SMH_BANK1);
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_BANK2);
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x6fff, 0, 0, SMH_BANK3);
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x7000, 0xdfff, 0, 0, SMH_BANK4);
	memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_BANK5);

	switch(bank) {
		case 0 :
		case 1 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);
						
						memory_set_bankptr(1, mess_ram);
						memory_set_bankptr(2, mess_ram + 0x2800);
						memory_set_bankptr(3, mess_ram + 0x3000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
						break;
/*		case 1 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x6fff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_set_bankptr(2, mess_ram + 0x2800);
						memory_set_bankptr(3, memory_region(REGION_CPU1) + 0x12000);
						memory_set_bankptr(4, memory_region(REGION_CPU1) + 0x16000);
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
						break;*/
		case 2 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_install_read8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(3, mess_ram + 0x10000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
						break;
		case 3 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_install_read8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(3, mess_ram + 0x14000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
						break;
		case 4 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_install_read8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(3, mess_ram + 0x18000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
					
						break;
		case 5 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(1, mess_ram);
						memory_install_read8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(3, mess_ram + 0x1c000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
					
						break;
		case 6 :
						memory_set_bankptr(1, mess_ram);
						memory_set_bankptr(2, mess_ram + 0x2800);
						memory_set_bankptr(3, mess_ram + 0x3000);
						memory_set_bankptr(4, mess_ram + 0x7000);
						memory_set_bankptr(5, mess_ram + 0xe000);					
						break;
		case 7 :
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x27ff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x6fff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0x7000, 0xdfff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(machine,0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_UNMAP);
						
						memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000);	
						memory_set_bankptr(2, memory_region(REGION_CPU1) + 0x10000);	
						memory_set_bankptr(3, memory_region(REGION_CPU1) + 0x10000);	
						memory_set_bankptr(4, memory_region(REGION_CPU1) + 0x10000);	
						memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x10000);						
						break;
	}
}
static PIT8253_FREQUENCY_CHANGED(bm2_pit_clk)
{
	pit8253_set_clockin((device_config*)device_list_find_by_tag( device->machine->config->devicelist, PIT8253, "pit8253"), 0, frequency);
}

static PIT8253_OUTPUT_CHANGED(bm2_pit_irq)
{
	pic8259_set_irq_line((device_config*)device_list_find_by_tag( device->machine->config->devicelist, PIC8259, "pic8259"),1,state);	
}


const struct pit8253_config b2m_pit8253_intf =
{
	{
		{
			2000000,
			bm2_pit_irq,
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
			bm2_pit_clk
		}
	}
};

WRITE8_HANDLER (b2m_8255_porta_w )
{	
	b2m_8255_porta = data;
}
WRITE8_HANDLER (b2m_8255_portb_w )
{	
	b2m_video_scroll = data;
}

WRITE8_HANDLER (b2m_8255_portc_w )
{	
	b2m_8255_portc = data;
	b2m_set_bank(machine,b2m_8255_portc & 7);
	b2m_video_page = (b2m_8255_portc >> 7) & 1;
}

READ8_HANDLER (b2m_8255_porta_r )
{
	return 0xff;
}
READ8_HANDLER (b2m_8255_portb_r )
{
	return b2m_video_scroll;
}
READ8_HANDLER (b2m_8255_portc_r )
{
	return 0xff;
}
const ppi8255_interface b2m_ppi8255_interface_1 =
{
	b2m_8255_porta_r,
	b2m_8255_portb_r,
	b2m_8255_portc_r,
	b2m_8255_porta_w,
	b2m_8255_portb_w,
	b2m_8255_portc_w
};


WRITE8_HANDLER (b2m_ext_8255_porta_w )
{	
}
WRITE8_HANDLER (b2m_ext_8255_portb_w )
{	
}

WRITE8_HANDLER (b2m_ext_8255_portc_w )
{		
	UINT8 drive = ((data >> 1) & 1) ^ 1;
	UINT8 side  = (data  & 1) ^ 1;
	
	if (b2m_drive!=drive) {
		wd17xx_set_drive(drive);
		b2m_drive = drive;
	}
	if (b2m_side!=side) {
		wd17xx_set_side(side);
		b2m_side = side;
	}
}

READ8_HANDLER (b2m_ext_8255_porta_r )
{
	return 0xff;
}
READ8_HANDLER (b2m_ext_8255_portb_r )
{
	return 0xff;
}
READ8_HANDLER (b2m_ext_8255_portc_r )
{
	return 0xff;
}

const ppi8255_interface b2m_ppi8255_interface_2 =
{
	b2m_ext_8255_porta_r,
	b2m_ext_8255_portb_r,
	b2m_ext_8255_portc_r,
	b2m_ext_8255_porta_w,
	b2m_ext_8255_portb_w,
	b2m_ext_8255_portc_w
};

READ8_HANDLER (b2m_romdisk_porta_r )
{
	UINT8 *romdisk = memory_region(REGION_CPU1) + 0x12000;		
	return romdisk[b2m_romdisk_msb*256+b2m_romdisk_lsb];	
}

WRITE8_HANDLER (b2m_romdisk_portb_w )
{	
	b2m_romdisk_lsb = data;
}

WRITE8_HANDLER (b2m_romdisk_portc_w )
{			
	b2m_romdisk_msb = data & 0x7f;	
}

const ppi8255_interface b2m_ppi8255_interface_3 =
{
	b2m_romdisk_porta_r,
	NULL,
	NULL,
	NULL,
	b2m_romdisk_portb_w,
	b2m_romdisk_portc_w
};


READ8_HANDLER( b2m_8255_2_r ) 
{
	return ppi8255_r((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_3" ),offset);
}
WRITE8_HANDLER( b2m_8255_2_w ) 
{
	ppi8255_w((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_3" ),offset,data);
}

READ8_HANDLER( b2m_8255_1_r ) 
{
	return ppi8255_r((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_2" ),offset);
}
WRITE8_HANDLER( b2m_8255_1_w ) 
{
	ppi8255_w((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_2" ),offset,data);
}

READ8_HANDLER( b2m_8255_0_r ) 
{
	return ppi8255_r((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_1" ),offset);
}
WRITE8_HANDLER( b2m_8255_0_w ) 
{
	ppi8255_w((device_config*)device_list_find_by_tag( machine->config->devicelist, PPI8255, "ppi8255_1" ),offset,data);
}

static PIC8259_SET_INT_LINE( b2m_pic_set_int_line )
{		
	cpunum_set_input_line(device->machine, 0, 0,interrupt ?  HOLD_LINE : CLEAR_LINE);  
} 
UINT8 vblank_state = 0;


/* Driver initialization */
DRIVER_INIT(b2m)
{
	memset(mess_ram,0,128*1024);	
	vblank_state = 0;
}

WRITE8_HANDLER ( b2m_palette_w ) 
{
	UINT8 b = (3 - ((data >> 6) & 3)) * 0x55;
	UINT8 g = (3 - ((data >> 4) & 3)) * 0x55;
	UINT8 r = (3 - ((data >> 2) & 3)) * 0x55;
	
	UINT8 bw = (3 - (data & 3)) * 0x55;
	
	b2m_color[offset] = data;		
	
	if (input_port_read(machine,"MONITOR")==1) {
		palette_set_color_rgb(machine,offset, r, g, b);
	} else {
		palette_set_color_rgb(machine,offset, bw, bw, bw);
	}
}

READ8_HANDLER ( b2m_palette_r )
{
	return b2m_color[offset];
}

WRITE8_HANDLER ( b2m_localmachine_w ) 
{
	b2m_localmachine = data;
}

READ8_HANDLER ( b2m_localmachine_r ) 
{
	return b2m_localmachine;
}

static const struct msm8251_interface b2m_msm8251_interface =
{
	NULL,
	NULL,
	NULL
};

MACHINE_START(b2m)
{
	wd17xx_init(machine, WD_TYPE_1793, NULL , NULL);	
	wd17xx_set_pause_time(10);
	msm8251_init(&b2m_msm8251_interface);
}

IRQ_CALLBACK(b2m_irq_callback)
{	
	return pic8259_acknowledge((device_config*)device_list_find_by_tag( machine->config->devicelist, PIC8259, "pic8259"));	
} 


const struct pic8259_interface b2m_pic8259_config = {
	b2m_pic_set_int_line
};

INTERRUPT_GEN (b2m_vblank_interrupt)
{	
	//vblank_state++;
	//if (vblank_state>1) vblank_state=0;
	//pic8259_set_irq_line((device_config*)device_list_find_by_tag( machine->config->devicelist, PIC8259, "pic8259"),0,vblank_state);		
}
static TIMER_CALLBACK (b2m_callback)
{	
	vblank_state++;
	if (vblank_state>1) vblank_state=0;
	pic8259_set_irq_line((device_config*)device_list_find_by_tag( machine->config->devicelist, PIC8259, "pic8259"),0,vblank_state);		
}

MACHINE_RESET(b2m)
{
	wd17xx_reset();
	msm8251_reset();
	
	b2m_side = 0;
	b2m_drive = 0;
	wd17xx_set_side(0);
	wd17xx_set_drive(0);

	cpunum_set_irq_callback(0, b2m_irq_callback);
	b2m_set_bank(machine,7);
		
	timer_pulse(ATTOTIME_IN_HZ(1), NULL, 0, b2m_callback);
}

DEVICE_IMAGE_LOAD( b2m_floppy )
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

static sound_stream *mixer_channel;
static void *b2m_sh_start(int clock, const struct CustomSound_interface *config);
static void b2m_sh_update(void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length);

const struct CustomSound_interface b2m_sound_interface =
{
	b2m_sh_start,
	NULL,
	NULL
};

static void *b2m_sh_start(int clock, const struct CustomSound_interface *config)
{
	mixer_channel = stream_create(0, 2, Machine->sample_rate, 0, b2m_sh_update);
	return (void *) ~0;
}

static void b2m_sh_update(void *param,stream_sample_t **inputs, stream_sample_t **buffer,int length)
{
	device_config *pit8253 = (device_config*)device_list_find_by_tag( Machine->config->devicelist, PIT8253, "pit8253" );
	INT16 channel_1_signal;
	static int channel_1_incr = 0;
	int channel_1_baseclock;

	int rate = Machine->sample_rate / 2;

	stream_sample_t *sample_left = buffer[0];
	stream_sample_t *sample_right = buffer[1];

	channel_1_baseclock = pit8253_get_frequency(pit8253, 1);

	channel_1_signal = pit8253_get_output (pit8253,1) ? 3000 : -3000;


	while (length--)
	{
		*sample_left = 0;
		*sample_right = 0;

		/* music channel 1 */

		*sample_left = channel_1_signal;
		channel_1_incr -= channel_1_baseclock;
		while( channel_1_incr < 0 )
		{
			channel_1_incr += rate;
			channel_1_signal = -channel_1_signal;
		}

		
		sample_left++;
		sample_right++;
	}
}

void b2m_sh_change_clock(double clock)
{
	stream_update(mixer_channel);
}
