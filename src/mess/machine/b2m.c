/***************************************************************************

		Bashkiria-2M machine driver by Miodrag Milanovic

		28/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/i8255a.h"
#include "machine/pit8253.h"
#include "machine/wd17xx.h"
#include "machine/pic8259.h"
#include "machine/msm8251.h"
#include "streams.h"
#include "includes/b2m.h"

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

static int	b2m_sound_input;
static sound_stream *mixer_channel;


static READ8_HANDLER (b2m_keyboard_r )
{		
	UINT8 key = 0x00;
	if (offset < 0x100) {
		if ((offset & 0x01)!=0) { key |= input_port_read(space->machine,"LINE0"); }
		if ((offset & 0x02)!=0) { key |= input_port_read(space->machine,"LINE1"); }
		if ((offset & 0x04)!=0) { key |= input_port_read(space->machine,"LINE2"); }
		if ((offset & 0x08)!=0) { key |= input_port_read(space->machine,"LINE3"); }
		if ((offset & 0x10)!=0) { key |= input_port_read(space->machine,"LINE4"); }
		if ((offset & 0x20)!=0) { key |= input_port_read(space->machine,"LINE5"); }
		if ((offset & 0x40)!=0) { key |= input_port_read(space->machine,"LINE6"); }
		if ((offset & 0x80)!=0) { key |= input_port_read(space->machine,"LINE7"); }					
	} else {
		if ((offset & 0x01)!=0) { key |= input_port_read(space->machine,"LINE8"); }
		if ((offset & 0x02)!=0) { key |= input_port_read(space->machine,"LINE9"); }
		if ((offset & 0x04)!=0) { key |= input_port_read(space->machine,"LINE10"); }
	}
	return key;
}


static void b2m_set_bank(running_machine *machine,int bank) 
{
	UINT8 *rom;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	
	memory_install_write8_handler(space, 0x0000, 0x27ff, 0, 0, SMH_BANK(1));
	memory_install_write8_handler(space, 0x2800, 0x2fff, 0, 0, SMH_BANK(2));
	memory_install_write8_handler(space, 0x3000, 0x6fff, 0, 0, SMH_BANK(3));
	memory_install_write8_handler(space, 0x7000, 0xdfff, 0, 0, SMH_BANK(4));
	memory_install_write8_handler(space, 0xe000, 0xffff, 0, 0, SMH_BANK(5));

	rom = memory_region(machine, "maincpu");
	switch(bank) {
		case 0 :
		case 1 :
						memory_install_write8_handler(space, 0xe000, 0xffff, 0, 0, SMH_UNMAP);
						
						memory_set_bankptr(machine, 1, mess_ram);
						memory_set_bankptr(machine, 2, mess_ram + 0x2800);
						memory_set_bankptr(machine, 3, mess_ram + 0x3000);
						memory_set_bankptr(machine, 4, mess_ram + 0x7000);
						memory_set_bankptr(machine, 5, rom + 0x10000);						
						break;
#if 0
		case 1 :
						memory_install_write8_handler(space, 0x3000, 0x6fff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(space, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(machine, 1, mess_ram);
						memory_set_bankptr(machine, 2, mess_ram + 0x2800);
						memory_set_bankptr(machine, 3, rom + 0x12000);
						memory_set_bankptr(machine, 4, rom + 0x16000);
						memory_set_bankptr(machine, 5, rom + 0x10000);						
						break;
#endif
		case 2 :
						memory_install_write8_handler(space, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(space, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(machine, 1, mess_ram);
						memory_install_read8_handler(space, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(machine, 3, mess_ram + 0x10000);
						memory_set_bankptr(machine, 4, mess_ram + 0x7000);
						memory_set_bankptr(machine, 5, rom + 0x10000);						
						break;
		case 3 :
						memory_install_write8_handler(space, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(space, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(machine, 1, mess_ram);
						memory_install_read8_handler(space, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(machine, 3, mess_ram + 0x14000);
						memory_set_bankptr(machine, 4, mess_ram + 0x7000);
						memory_set_bankptr(machine, 5, rom + 0x10000);						
						break;
		case 4 :
						memory_install_write8_handler(space, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(space, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(machine, 1, mess_ram);
						memory_install_read8_handler(space, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(machine, 3, mess_ram + 0x18000);
						memory_set_bankptr(machine, 4, mess_ram + 0x7000);
						memory_set_bankptr(machine, 5, rom + 0x10000);						
					
						break;
		case 5 :
						memory_install_write8_handler(space, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);			
						memory_install_write8_handler(space, 0xe000, 0xffff, 0, 0, SMH_UNMAP);

						memory_set_bankptr(machine, 1, mess_ram);
						memory_install_read8_handler(space, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);			
						memory_set_bankptr(machine, 3, mess_ram + 0x1c000);
						memory_set_bankptr(machine, 4, mess_ram + 0x7000);
						memory_set_bankptr(machine, 5, rom + 0x10000);						
					
						break;
		case 6 :
						memory_set_bankptr(machine, 1, mess_ram);
						memory_set_bankptr(machine, 2, mess_ram + 0x2800);
						memory_set_bankptr(machine, 3, mess_ram + 0x3000);
						memory_set_bankptr(machine, 4, mess_ram + 0x7000);
						memory_set_bankptr(machine, 5, mess_ram + 0xe000);					
						break;
		case 7 :
						memory_install_write8_handler(space, 0x0000, 0x27ff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(space, 0x2800, 0x2fff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(space, 0x3000, 0x6fff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(space, 0x7000, 0xdfff, 0, 0, SMH_UNMAP);
						memory_install_write8_handler(space, 0xe000, 0xffff, 0, 0, SMH_UNMAP);
						
						memory_set_bankptr(machine, 1, rom + 0x10000);	
						memory_set_bankptr(machine, 2, rom + 0x10000);	
						memory_set_bankptr(machine, 3, rom + 0x10000);	
						memory_set_bankptr(machine, 4, rom + 0x10000);	
						memory_set_bankptr(machine, 5, rom + 0x10000);						
						break;
	}
}

static PIT8253_OUTPUT_CHANGED(bm2_pit_out0)
{
	pic8259_set_irq_line(devtag_get_device(device->machine, "pic8259"),1,state);		
}


static PIT8253_OUTPUT_CHANGED(bm2_pit_out1)
{
	if (mixer_channel!=NULL) {
		stream_update( mixer_channel );
	}
	b2m_sound_input = state;
}

static PIT8253_OUTPUT_CHANGED(bm2_pit_out2)
{
	pit8253_set_clock_signal( device, 0, state );
}


const struct pit8253_config b2m_pit8253_intf =
{
	{
		{
			0,
			bm2_pit_out0
		},
		{
			2000000,
			bm2_pit_out1
		},
		{
			2000000,
			bm2_pit_out2
		}
	}
};

static WRITE8_DEVICE_HANDLER (b2m_8255_porta_w )
{	
	b2m_8255_porta = data;
}
static WRITE8_DEVICE_HANDLER (b2m_8255_portb_w )
{	
	b2m_video_scroll = data;
}

static WRITE8_DEVICE_HANDLER (b2m_8255_portc_w )
{	
	b2m_8255_portc = data;
	b2m_set_bank(device->machine, b2m_8255_portc & 7);
	b2m_video_page = (b2m_8255_portc >> 7) & 1;
}

static READ8_DEVICE_HANDLER (b2m_8255_portb_r )
{
	return b2m_video_scroll;
}

I8255A_INTERFACE( b2m_ppi8255_interface_1 )
{
	DEVCB_NULL,
	DEVCB_HANDLER(b2m_8255_portb_r),
	DEVCB_NULL,
	DEVCB_HANDLER(b2m_8255_porta_w),
	DEVCB_HANDLER(b2m_8255_portb_w),
	DEVCB_HANDLER(b2m_8255_portc_w)
};



static WRITE8_DEVICE_HANDLER (b2m_ext_8255_portc_w )
{		
	UINT8 drive = ((data >> 1) & 1) ^ 1;
	UINT8 side  = (data  & 1) ^ 1;	
	const device_config *fdc = devtag_get_device(device->machine, "wd1793");
	
	if (b2m_drive!=drive) {
		wd17xx_set_drive(fdc,drive);
		b2m_drive = drive;
	}
	if (b2m_side!=side) {
		wd17xx_set_side(fdc,side);
		b2m_side = side;
	}
}

I8255A_INTERFACE( b2m_ppi8255_interface_2 )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(b2m_ext_8255_portc_w)
};

static READ8_DEVICE_HANDLER (b2m_romdisk_porta_r )
{
	UINT8 *romdisk = memory_region(device->machine, "maincpu") + 0x12000;		
	return romdisk[b2m_romdisk_msb*256+b2m_romdisk_lsb];	
}

static WRITE8_DEVICE_HANDLER (b2m_romdisk_portb_w )
{	
	b2m_romdisk_lsb = data;
}

static WRITE8_DEVICE_HANDLER (b2m_romdisk_portc_w )
{			
	b2m_romdisk_msb = data & 0x7f;	
}

I8255A_INTERFACE( b2m_ppi8255_interface_3 )
{
	DEVCB_HANDLER(b2m_romdisk_porta_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(b2m_romdisk_portb_w),
	DEVCB_HANDLER(b2m_romdisk_portc_w)
};

static PIC8259_SET_INT_LINE( b2m_pic_set_int_line )
{		
	cputag_set_input_line(device->machine, "maincpu", 0, interrupt ?  HOLD_LINE : CLEAR_LINE);  
} 
static UINT8 vblank_state = 0;


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
	
	if (input_port_read(space->machine,"MONITOR")==1) {
		palette_set_color_rgb(space->machine,offset, r, g, b);
	} else {
		palette_set_color_rgb(space->machine,offset, bw, bw, bw);
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

MACHINE_START(b2m)
{
	wd17xx_set_pause_time(devtag_get_device(machine, "wd1793"),10);
}

static IRQ_CALLBACK(b2m_irq_callback)
{	
	return pic8259_acknowledge(devtag_get_device(device->machine, "pic8259"));
} 


const struct pic8259_interface b2m_pic8259_config = {
	b2m_pic_set_int_line
};

INTERRUPT_GEN (b2m_vblank_interrupt)
{	
	vblank_state++;
	if (vblank_state>1) vblank_state=0;
	pic8259_set_irq_line(devtag_get_device(device->machine, "pic8259"), 0, vblank_state);		
}

MACHINE_RESET(b2m)
{	
	b2m_sound_input = 0;
		
	b2m_side = 0;
	b2m_drive = 0;

	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), b2m_irq_callback);
	b2m_set_bank(machine, 7);
}

static STREAM_UPDATE( b2m_sh_update )
{
	INT16 channel_1_signal;

	stream_sample_t *sample_left = outputs[0];

	channel_1_signal = b2m_sound_input ? 3000 : -3000;

	while (samples--)
	{
		*sample_left = channel_1_signal;		
		sample_left++;
	}
}

static DEVICE_START(b2m_sound)
{
	b2m_sound_input = 0;
	mixer_channel = stream_create(device, 0, 1, device->machine->sample_rate, 0, b2m_sh_update);
}


DEVICE_GET_INFO( b2m_sound )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(b2m_sound);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "B2M Custom");				break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
	}
}
