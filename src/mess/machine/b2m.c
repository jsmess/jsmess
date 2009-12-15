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
#include "includes/b2m.h"
#include "devices/messram.h"

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

	memory_install_write_bank(space, 0x0000, 0x27ff, 0, 0, "bank1");
	memory_install_write_bank(space, 0x2800, 0x2fff, 0, 0, "bank2");
	memory_install_write_bank(space, 0x3000, 0x6fff, 0, 0, "bank3");
	memory_install_write_bank(space, 0x7000, 0xdfff, 0, 0, "bank4");
	memory_install_write_bank(space, 0xe000, 0xffff, 0, 0, "bank5");

	rom = memory_region(machine, "maincpu");
	switch(bank) {
		case 0 :
		case 1 :
						memory_unmap_write(space, 0xe000, 0xffff, 0, 0);

						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2800);
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x3000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x7000);
						memory_set_bankptr(machine, "bank5", rom + 0x10000);
						break;
#if 0
		case 1 :
						memory_unmap_write(space, 0x3000, 0x6fff, 0, 0);
						memory_unmap_write(space, 0xe000, 0xffff, 0, 0);

						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2800);
						memory_set_bankptr(machine, "bank3", rom + 0x12000);
						memory_set_bankptr(machine, "bank4", rom + 0x16000);
						memory_set_bankptr(machine, "bank5", rom + 0x10000);
						break;
#endif
		case 2 :
						memory_unmap_write(space, 0x2800, 0x2fff, 0, 0);
						memory_unmap_write(space, 0xe000, 0xffff, 0, 0);

						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
						memory_install_read8_handler(space, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x10000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x7000);
						memory_set_bankptr(machine, "bank5", rom + 0x10000);
						break;
		case 3 :
						memory_unmap_write(space, 0x2800, 0x2fff, 0, 0);
						memory_unmap_write(space, 0xe000, 0xffff, 0, 0);

						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
						memory_install_read8_handler(space, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x14000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x7000);
						memory_set_bankptr(machine, "bank5", rom + 0x10000);
						break;
		case 4 :
						memory_unmap_write(space, 0x2800, 0x2fff, 0, 0);
						memory_unmap_write(space, 0xe000, 0xffff, 0, 0);

						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
						memory_install_read8_handler(space, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x18000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x7000);
						memory_set_bankptr(machine, "bank5", rom + 0x10000);

						break;
		case 5 :
						memory_unmap_write(space, 0x2800, 0x2fff, 0, 0);
						memory_unmap_write(space, 0xe000, 0xffff, 0, 0);

						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
						memory_install_read8_handler(space, 0x2800, 0x2fff, 0, 0, b2m_keyboard_r);
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x1c000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x7000);
						memory_set_bankptr(machine, "bank5", rom + 0x10000);

						break;
		case 6 :
						memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
						memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2800);
						memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x3000);
						memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x7000);
						memory_set_bankptr(machine, "bank5", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xe000);
						break;
		case 7 :
						memory_unmap_write(space, 0x0000, 0x27ff, 0, 0);
						memory_unmap_write(space, 0x2800, 0x2fff, 0, 0);
						memory_unmap_write(space, 0x3000, 0x6fff, 0, 0);
						memory_unmap_write(space, 0x7000, 0xdfff, 0, 0);
						memory_unmap_write(space, 0xe000, 0xffff, 0, 0);

						memory_set_bankptr(machine, "bank1", rom + 0x10000);
						memory_set_bankptr(machine, "bank2", rom + 0x10000);
						memory_set_bankptr(machine, "bank3", rom + 0x10000);
						memory_set_bankptr(machine, "bank4", rom + 0x10000);
						memory_set_bankptr(machine, "bank5", rom + 0x10000);
						break;
	}
}

static PIT8253_OUTPUT_CHANGED(bm2_pit_out0)
{
	b2m_state *st = device->machine->driver_data;
	pic8259_set_irq_line(st->pic,1,state);
}


static PIT8253_OUTPUT_CHANGED(bm2_pit_out1)
{
	b2m_state *st = device->machine->driver_data;
	speaker_level_w(st->speaker, state);

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
	b2m_state *state = device->machine->driver_data;
	state->b2m_8255_porta = data;
}
static WRITE8_DEVICE_HANDLER (b2m_8255_portb_w )
{
	b2m_state *state = device->machine->driver_data;
	state->b2m_video_scroll = data;
}

static WRITE8_DEVICE_HANDLER (b2m_8255_portc_w )
{
	b2m_state *state = device->machine->driver_data;

	state->b2m_8255_portc = data;
	b2m_set_bank(device->machine, state->b2m_8255_portc & 7);
	state->b2m_video_page = (state->b2m_8255_portc >> 7) & 1;
}

static READ8_DEVICE_HANDLER (b2m_8255_portb_r )
{
	b2m_state *state = device->machine->driver_data;
	return state->b2m_video_scroll;
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
	b2m_state *state = device->machine->driver_data;

	if (state->b2m_drive!=drive) {
		wd17xx_set_drive(state->fdc,drive);
		state->b2m_drive = drive;
	}
	if (state->b2m_side!=side) {
		wd17xx_set_side(state->fdc,side);
		state->b2m_side = side;
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
	b2m_state *state = device->machine->driver_data;

	UINT8 *romdisk = memory_region(device->machine, "maincpu") + 0x12000;
	return romdisk[state->b2m_romdisk_msb*256+state->b2m_romdisk_lsb];
}

static WRITE8_DEVICE_HANDLER (b2m_romdisk_portb_w )
{
	b2m_state *state = device->machine->driver_data;
	state->b2m_romdisk_lsb = data;
}

static WRITE8_DEVICE_HANDLER (b2m_romdisk_portc_w )
{
	b2m_state *state = device->machine->driver_data;
	state->b2m_romdisk_msb = data & 0x7f;
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

/* Driver initialization */
DRIVER_INIT(b2m)
{
	b2m_state *state = machine->driver_data;

	memset(messram_get_ptr(devtag_get_device(machine, "messram")),0,128*1024);
	state->vblank_state = 0;
}

WRITE8_HANDLER ( b2m_palette_w )
{
	b2m_state *state = space->machine->driver_data;

	UINT8 b = (3 - ((data >> 6) & 3)) * 0x55;
	UINT8 g = (3 - ((data >> 4) & 3)) * 0x55;
	UINT8 r = (3 - ((data >> 2) & 3)) * 0x55;

	UINT8 bw = (3 - (data & 3)) * 0x55;

	state->b2m_color[offset & 3] = data;

	if (input_port_read(space->machine,"MONITOR")==1) {
		palette_set_color_rgb(space->machine,offset, r, g, b);
	} else {
		palette_set_color_rgb(space->machine,offset, bw, bw, bw);
	}
}

READ8_HANDLER ( b2m_palette_r )
{
	b2m_state *state = space->machine->driver_data;
	return state->b2m_color[offset];
}

WRITE8_HANDLER ( b2m_localmachine_w )
{
	b2m_state *state = space->machine->driver_data;
	state->b2m_localmachine = data;
}

READ8_HANDLER ( b2m_localmachine_r )
{
	b2m_state *state = space->machine->driver_data;
	return state->b2m_localmachine;
}

static STATE_POSTLOAD( b2m_postload )
{
	b2m_state *state = machine->driver_data;
	b2m_set_bank(machine, state->b2m_8255_portc & 7);
}

MACHINE_START(b2m)
{
	b2m_state *state = machine->driver_data;
	state->pic = devtag_get_device(machine, "pic8259");
	state->fdc = devtag_get_device(machine, "wd1793");
	state->speaker = devtag_get_device(machine, "speaker");

	wd17xx_set_pause_time(state->fdc,10);

	/* register for state saving */
	state_save_register_global(machine, state->b2m_8255_porta);
	state_save_register_global(machine, state->b2m_video_scroll);
	state_save_register_global(machine, state->b2m_8255_portc);
	state_save_register_global(machine, state->b2m_video_page);
	state_save_register_global(machine, state->b2m_drive);
	state_save_register_global(machine, state->b2m_side);
	state_save_register_global(machine, state->b2m_romdisk_lsb);
	state_save_register_global(machine, state->b2m_romdisk_msb);
	state_save_register_global_pointer(machine, state->b2m_color, 4);
	state_save_register_global(machine, state->b2m_localmachine);
	state_save_register_global(machine, state->vblank_state);

	state_save_register_postload(machine, b2m_postload, NULL);
}

static IRQ_CALLBACK(b2m_irq_callback)
{
	b2m_state *state = device->machine->driver_data;
	return pic8259_acknowledge(state->pic);
}

const struct pic8259_interface b2m_pic8259_config = {
	b2m_pic_set_int_line
};

INTERRUPT_GEN (b2m_vblank_interrupt)
{
	b2m_state *state = device->machine->driver_data;
	state->vblank_state++;
	if (state->vblank_state>1) state->vblank_state=0;
	pic8259_set_irq_line(state->pic, 0, state->vblank_state);
}

MACHINE_RESET(b2m)
{
	b2m_state *state = machine->driver_data;
	state->b2m_side = 0;
	state->b2m_drive = 0;

	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), b2m_irq_callback);
	b2m_set_bank(machine, 7);
}
