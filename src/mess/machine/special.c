/***************************************************************************

        Specialist machine driver by Miodrag Milanovic

        20/03/2008 Cassette support
        15/03/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "sound/dac.h"
#include "imagedev/cassette.h"
#include "machine/i8255a.h"
#include "machine/pit8253.h"
#include "machine/wd17xx.h"
#include "includes/special.h"
#include "machine/ram.h"
#include "imagedev/flopdrv.h"



/* Driver initialization */
DRIVER_INIT(special)
{
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = machine.region("maincpu")->base();
	memset(RAM,0x0000,0x3000); // make frist page empty by default
	memory_configure_bank(machine, "bank1", 1, 2, RAM, 0x0000);
	memory_configure_bank(machine, "bank1", 0, 2, RAM, 0xc000);
}

static READ8_DEVICE_HANDLER (specialist_8255_porta_r )
{
	if (input_port_read(device->machine(), "LINE0")!=0xff) return 0xfe;
	if (input_port_read(device->machine(), "LINE1")!=0xff) return 0xfd;
	if (input_port_read(device->machine(), "LINE2")!=0xff) return 0xfb;
	if (input_port_read(device->machine(), "LINE3")!=0xff) return 0xf7;
	if (input_port_read(device->machine(), "LINE4")!=0xff) return 0xef;
	if (input_port_read(device->machine(), "LINE5")!=0xff) return 0xdf;
	if (input_port_read(device->machine(), "LINE6")!=0xff) return 0xbf;
	if (input_port_read(device->machine(), "LINE7")!=0xff) return 0x7f;
	return 0xff;
}

static READ8_DEVICE_HANDLER (specialist_8255_portb_r )
{
	special_state *state = device->machine().driver_data<special_state>();

	int dat = 0;
	double level;

	if ((state->m_specialist_8255_porta & 0x01)==0) dat ^= (input_port_read(device->machine(), "LINE0") ^ 0xff);
	if ((state->m_specialist_8255_porta & 0x02)==0) dat ^= (input_port_read(device->machine(), "LINE1") ^ 0xff);
	if ((state->m_specialist_8255_porta & 0x04)==0) dat ^= (input_port_read(device->machine(), "LINE2") ^ 0xff);
	if ((state->m_specialist_8255_porta & 0x08)==0) dat ^= (input_port_read(device->machine(), "LINE3") ^ 0xff);
	if ((state->m_specialist_8255_porta & 0x10)==0) dat ^= (input_port_read(device->machine(), "LINE4") ^ 0xff);
	if ((state->m_specialist_8255_porta & 0x20)==0) dat ^= (input_port_read(device->machine(), "LINE5") ^ 0xff);
	if ((state->m_specialist_8255_porta & 0x40)==0) dat ^= (input_port_read(device->machine(), "LINE6") ^ 0xff);
	if ((state->m_specialist_8255_porta & 0x80)==0) dat ^= (input_port_read(device->machine(), "LINE7") ^ 0xff);
	if ((state->m_specialist_8255_portc & 0x01)==0) dat ^= (input_port_read(device->machine(), "LINE8") ^ 0xff);
	if ((state->m_specialist_8255_portc & 0x02)==0) dat ^= (input_port_read(device->machine(), "LINE9") ^ 0xff);
	if ((state->m_specialist_8255_portc & 0x04)==0) dat ^= (input_port_read(device->machine(), "LINE10") ^ 0xff);
	if ((state->m_specialist_8255_portc & 0x08)==0) dat ^= (input_port_read(device->machine(), "LINE11") ^ 0xff);

	dat = (dat  << 2) ^0xff;
	if (input_port_read(device->machine(), "LINE12")!=0xff) dat ^= 0x02;

	level = cassette_input(device->machine().device("cassette"));
	if (level >=  0)
	{
			dat ^= 0x01;
	}
	return dat & 0xff;
}

static READ8_DEVICE_HANDLER (specialist_8255_portc_r )
{
	if (input_port_read(device->machine(), "LINE8")!=0xff) return 0x0e;
	if (input_port_read(device->machine(), "LINE9")!=0xff) return 0x0d;
	if (input_port_read(device->machine(), "LINE10")!=0xff) return 0x0b;
	if (input_port_read(device->machine(), "LINE11")!=0xff) return 0x07;
	return 0x0f;
}

static WRITE8_DEVICE_HANDLER (specialist_8255_porta_w )
{
	special_state *state = device->machine().driver_data<special_state>();
	state->m_specialist_8255_porta = data;
}

static WRITE8_DEVICE_HANDLER (specialist_8255_portb_w )
{
	special_state *state = device->machine().driver_data<special_state>();
	state->m_specialist_8255_portb = data;
}
static WRITE8_DEVICE_HANDLER (specialist_8255_portc_w )
{
	special_state *state = device->machine().driver_data<special_state>();
	device_t *dac_device = device->machine().device("dac");

	state->m_specialist_8255_portc = data;

	cassette_output(device->machine().device("cassette"),data & 0x80 ? 1 : -1);

	dac_data_w(dac_device, data & 0x20); //beeper

}

I8255A_INTERFACE( specialist_ppi8255_interface )
{
	DEVCB_HANDLER(specialist_8255_porta_r),
	DEVCB_HANDLER(specialist_8255_portb_r),
	DEVCB_HANDLER(specialist_8255_portc_r),
	DEVCB_HANDLER(specialist_8255_porta_w),
	DEVCB_HANDLER(specialist_8255_portb_w),
	DEVCB_HANDLER(specialist_8255_portc_w)
};

static TIMER_CALLBACK( special_reset )
{
	memory_set_bank(machine, "bank1", 0);
}


MACHINE_RESET( special )
{
	machine.scheduler().timer_set(attotime::from_usec(10), FUNC(special_reset));
	memory_set_bank(machine, "bank1", 1);
}

READ8_HANDLER( specialist_keyboard_r )
{
	return i8255a_r(space->machine().device("ppi8255"), (offset & 3));
}

WRITE8_HANDLER( specialist_keyboard_w )
{
	i8255a_w(space->machine().device("ppi8255"), (offset & 3) , data );
}


/*
     Specialist MX
*/

static WRITE8_HANDLER( video_memory_w )
{
	special_state *state = space->machine().driver_data<special_state>();
	ram_get_ptr(space->machine().device(RAM_TAG))[0x9000 + offset] = data;
	state->m_specimx_colorram[offset]  = state->m_specimx_color;
}

WRITE8_HANDLER (specimx_video_color_w )
{
	special_state *state = space->machine().driver_data<special_state>();
	state->m_specimx_color = data;
}

READ8_HANDLER (specimx_video_color_r )
{
	special_state *state = space->machine().driver_data<special_state>();
	return state->m_specimx_color;
}

static void specimx_set_bank(running_machine &machine, int i,int data)
{
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	UINT8 *ram = ram_get_ptr(machine.device(RAM_TAG));

	space->install_write_bank(0xc000, 0xffbf, "bank3");
	space->install_write_bank(0xffc0, 0xffdf, "bank4");
	memory_set_bankptr(machine, "bank4", ram + 0xffc0);
	switch(i)
	{
		case 0 :
				space->install_write_bank(0x0000, 0x8fff, "bank1");
				space->install_legacy_write_handler(0x9000, 0xbfff, FUNC(video_memory_w));

				memory_set_bankptr(machine, "bank1", ram);
				memory_set_bankptr(machine, "bank2", ram + 0x9000);
				memory_set_bankptr(machine, "bank3", ram + 0xc000);
				break;
		case 1 :
				space->install_write_bank(0x0000, 0x8fff, "bank1");
				space->install_write_bank(0x9000, 0xbfff, "bank2");

				memory_set_bankptr(machine, "bank1", ram + 0x10000);
				memory_set_bankptr(machine, "bank2", ram + 0x19000);
				memory_set_bankptr(machine, "bank3", ram + 0x1c000);
				break;
		case 2 :
				space->unmap_write(0x0000, 0x8fff);
				space->unmap_write(0x9000, 0xbfff);

				memory_set_bankptr(machine, "bank1", machine.region("maincpu")->base() + 0x10000);
				memory_set_bankptr(machine, "bank2", machine.region("maincpu")->base() + 0x19000);
				if (data & 0x80)
				{
					memory_set_bankptr(machine, "bank3", ram + 0x1c000);
				}
				else
				{
					memory_set_bankptr(machine, "bank3", ram + 0xc000);
				}
				break;
	}
}
WRITE8_HANDLER( specimx_select_bank )
{
	specimx_set_bank(space->machine(), offset, data);
}

static WRITE_LINE_DEVICE_HANDLER( specimx_pit8253_out0_changed )
{
	special_state *drvstate = device->machine().driver_data<special_state>();
	specimx_set_input( drvstate->m_specimx_audio, 0, state );
}



static WRITE_LINE_DEVICE_HANDLER(specimx_pit8253_out1_changed)
{
	special_state *drvstate = device->machine().driver_data<special_state>();
	specimx_set_input( drvstate->m_specimx_audio, 1, state );
}



static WRITE_LINE_DEVICE_HANDLER(specimx_pit8253_out2_changed)
{
	special_state *drvstate = device->machine().driver_data<special_state>();
	specimx_set_input( drvstate->m_specimx_audio, 2, state );
}



const struct pit8253_config specimx_pit8253_intf =
{
	{
		{
			2000000,
			DEVCB_NULL,
			DEVCB_LINE(specimx_pit8253_out0_changed)
		},
		{
			2000000,
			DEVCB_NULL,
			DEVCB_LINE(specimx_pit8253_out1_changed)
		},
		{
			2000000,
			DEVCB_NULL,
			DEVCB_LINE(specimx_pit8253_out2_changed)
		}
	}
};

MACHINE_START( specimx )
{
	special_state *state = machine.driver_data<special_state>();
	state->m_specimx_audio = machine.device("custom");
}

static TIMER_CALLBACK( setup_pit8253_gates )
{
	device_t *pit8253 = machine.device("pit8253");

	pit8253_gate0_w(pit8253, 0);
	pit8253_gate1_w(pit8253, 0);
	pit8253_gate2_w(pit8253, 0);
}

MACHINE_RESET( specimx )
{
	special_state *state = machine.driver_data<special_state>();
	specimx_set_bank(machine, 2,0x00); // Initiali load ROM disk
	state->m_specimx_color = 0x70;
	machine.scheduler().timer_set(attotime::zero, FUNC(setup_pit8253_gates));
	device_t *fdc = machine.device("wd1793");
	wd17xx_set_pause_time(fdc,12);
	wd17xx_dden_w(fdc, 0);
}

READ8_HANDLER ( specimx_disk_ctrl_r )
{
	return 0xff;
}

WRITE8_HANDLER( specimx_disk_ctrl_w )
{
	device_t *fdc = space->machine().device("wd1793");

	switch(offset)
	{
		case 2 :
				wd17xx_set_side(fdc,data & 1);
				break;
		case 3 :
				wd17xx_set_drive(fdc,data & 1);
				break;

	}
}

/*
    Erik
*/

static void erik_set_bank(running_machine &machine)
{
	special_state *state = machine.driver_data<special_state>();
	UINT8 bank1 = (state->m_RR_register & 3);
	UINT8 bank2 = ((state->m_RR_register >> 2) & 3);
	UINT8 bank3 = ((state->m_RR_register >> 4) & 3);
	UINT8 bank4 = ((state->m_RR_register >> 6) & 3);
	UINT8 *mem = machine.region("maincpu")->base();
	UINT8 *ram = ram_get_ptr(machine.device(RAM_TAG));
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	space->install_write_bank(0x0000, 0x3fff, "bank1");
	space->install_write_bank(0x4000, 0x8fff, "bank2");
	space->install_write_bank(0x9000, 0xbfff, "bank3");
	space->install_write_bank(0xc000, 0xefff, "bank4");
	space->install_write_bank(0xf000, 0xf7ff, "bank5");
	space->install_write_bank(0xf800, 0xffff, "bank6");

	switch(bank1)
	{
		case	1:
		case	2:
		case	3:
						memory_set_bankptr(machine, "bank1", ram + 0x10000*(bank1-1));
						break;
		case	0:
						space->unmap_write(0x0000, 0x3fff);
						memory_set_bankptr(machine, "bank1", mem + 0x10000);
						break;
	}
	switch(bank2)
	{
		case	1:
		case	2:
		case	3:
						memory_set_bankptr(machine, "bank2", ram + 0x10000*(bank2-1) + 0x4000);
						break;
		case	0:
						space->unmap_write(0x4000, 0x8fff);
						memory_set_bankptr(machine, "bank2", mem + 0x14000);
						break;
	}
	switch(bank3)
	{
		case	1:
		case	2:
		case	3:
						memory_set_bankptr(machine, "bank3", ram + 0x10000*(bank3-1) + 0x9000);
						break;
		case	0:
						space->unmap_write(0x9000, 0xbfff);
						memory_set_bankptr(machine, "bank3", mem + 0x19000);
						break;
	}
	switch(bank4)
	{
		case	1:
		case	2:
		case	3:
						memory_set_bankptr(machine, "bank4", ram + 0x10000*(bank4-1) + 0x0c000);
						memory_set_bankptr(machine, "bank5", ram + 0x10000*(bank4-1) + 0x0f000);
						memory_set_bankptr(machine, "bank6", ram + 0x10000*(bank4-1) + 0x0f800);
						break;
		case	0:
						space->unmap_write(0xc000, 0xefff);
						memory_set_bankptr(machine, "bank4", mem + 0x1c000);
						space->unmap_write(0xf000, 0xf7ff);
						space->nop_read(0xf000, 0xf7ff);
						space->install_legacy_write_handler(0xf800, 0xffff, FUNC(specialist_keyboard_w));
						space->install_legacy_read_handler(0xf800, 0xffff, FUNC(specialist_keyboard_r));
						break;
	}
}

DRIVER_INIT( erik )
{
	special_state *state = machine.driver_data<special_state>();
	state->m_erik_color_1 = 0;
	state->m_erik_color_2 = 0;
	state->m_erik_background = 0;
}

MACHINE_RESET( erik )
{
	special_state *state = machine.driver_data<special_state>();
	state->m_RR_register = 0x00;
	state->m_RC_register = 0x00;
	erik_set_bank(machine);
}

READ8_HANDLER ( erik_rr_reg_r )
{
	special_state *state = space->machine().driver_data<special_state>();
	return state->m_RR_register;
}
WRITE8_HANDLER( erik_rr_reg_w )
{
	special_state *state = space->machine().driver_data<special_state>();
	state->m_RR_register = data;
	erik_set_bank(space->machine());
}

READ8_HANDLER ( erik_rc_reg_r )
{
	special_state *state = space->machine().driver_data<special_state>();
	return state->m_RC_register;
}


WRITE8_HANDLER( erik_rc_reg_w )
{
	special_state *state = space->machine().driver_data<special_state>();
	state->m_RC_register = data;
	state->m_erik_color_1 =  state->m_RC_register & 7;
	state->m_erik_color_2 =  (state->m_RC_register >> 3) & 7;
	state->m_erik_background = ((state->m_RC_register  >> 6 ) & 1) + ((state->m_RC_register  >> 7 ) & 1) * 4;
}

READ8_HANDLER ( erik_disk_reg_r )
{
	return 0xff;
}

WRITE8_HANDLER( erik_disk_reg_w )
{
	device_t *fdc = space->machine().device("wd1793");

	wd17xx_set_side (fdc,data & 1);
	wd17xx_set_drive(fdc,(data >> 1) & 1);
	wd17xx_dden_w(fdc, BIT(data, 2));
	floppy_mon_w(floppy_get_device(space->machine(), (data >> 1) & 1), 0);
	floppy_mon_w(floppy_get_device(space->machine(), ((data >> 1) & 1) ? 0 : 1), 1);
	floppy_drive_set_ready_state(floppy_get_device(space->machine(), (data >> 1) & 1), 1, 1);

}
