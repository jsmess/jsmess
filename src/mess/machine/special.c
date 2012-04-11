/***************************************************************************

        Specialist machine driver by Miodrag Milanovic

        20/03/2008 Cassette support
        15/03/2008 Preliminary driver.

****************************************************************************/



#include "includes/special.h"


/* Driver initialization */
DRIVER_INIT(special)
{
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = machine.region("maincpu")->base();
	memset(RAM,0x0000,0x3000); // make first page empty by default
	memory_configure_bank(machine, "bank1", 1, 2, RAM, 0x0000);
	memory_configure_bank(machine, "bank1", 0, 2, RAM, 0xc000);
}

READ8_MEMBER( special_state::specialist_8255_porta_r )
{
	if (input_port_read(machine(), "LINE0")!=0xff) return 0xfe;
	if (input_port_read(machine(), "LINE1")!=0xff) return 0xfd;
	if (input_port_read(machine(), "LINE2")!=0xff) return 0xfb;
	if (input_port_read(machine(), "LINE3")!=0xff) return 0xf7;
	if (input_port_read(machine(), "LINE4")!=0xff) return 0xef;
	if (input_port_read(machine(), "LINE5")!=0xff) return 0xdf;
	if (input_port_read(machine(), "LINE6")!=0xff) return 0xbf;
	if (input_port_read(machine(), "LINE7")!=0xff) return 0x7f;
	return 0xff;
}

READ8_MEMBER( special_state::specialist_8255_portb_r )
{
	UINT8 dat = 0;
	double level;

	if ((m_specialist_8255_porta & 0x01)==0) dat ^= (input_port_read(machine(), "LINE0") ^ 0xff);
	if ((m_specialist_8255_porta & 0x02)==0) dat ^= (input_port_read(machine(), "LINE1") ^ 0xff);
	if ((m_specialist_8255_porta & 0x04)==0) dat ^= (input_port_read(machine(), "LINE2") ^ 0xff);
	if ((m_specialist_8255_porta & 0x08)==0) dat ^= (input_port_read(machine(), "LINE3") ^ 0xff);
	if ((m_specialist_8255_porta & 0x10)==0) dat ^= (input_port_read(machine(), "LINE4") ^ 0xff);
	if ((m_specialist_8255_porta & 0x20)==0) dat ^= (input_port_read(machine(), "LINE5") ^ 0xff);
	if ((m_specialist_8255_porta & 0x40)==0) dat ^= (input_port_read(machine(), "LINE6") ^ 0xff);
	if ((m_specialist_8255_porta & 0x80)==0) dat ^= (input_port_read(machine(), "LINE7") ^ 0xff);
	if ((m_specialist_8255_portc & 0x01)==0) dat ^= (input_port_read(machine(), "LINE8") ^ 0xff);
	if ((m_specialist_8255_portc & 0x02)==0) dat ^= (input_port_read(machine(), "LINE9") ^ 0xff);
	if ((m_specialist_8255_portc & 0x04)==0) dat ^= (input_port_read(machine(), "LINE10") ^ 0xff);
	if ((m_specialist_8255_portc & 0x08)==0) dat ^= (input_port_read(machine(), "LINE11") ^ 0xff);

	dat = (dat  << 2) ^0xff;
	if (input_port_read(machine(), "LINE12")!=0xff) dat ^= 0x02;

	level = m_cass->input();
	if (level >=  0)
			dat ^= 0x01;

	return dat;
}

READ8_MEMBER( special_state::specialist_8255_portc_r )
{
	if (input_port_read(machine(), "LINE8")!=0xff) return 0x0e;
	if (input_port_read(machine(), "LINE9")!=0xff) return 0x0d;
	if (input_port_read(machine(), "LINE10")!=0xff) return 0x0b;
	if (input_port_read(machine(), "LINE11")!=0xff) return 0x07;
	return 0x0f;
}

WRITE8_MEMBER( special_state::specialist_8255_porta_w )
{
	m_specialist_8255_porta = data;
}

WRITE8_MEMBER( special_state::specialist_8255_portb_w )
{
	m_specialist_8255_portb = data;
}

WRITE8_MEMBER( special_state::specialist_8255_portc_w )
{
	m_specialist_8255_portc = data;

	m_cass->output(BIT(data, 7) ? 1 : -1);

	dac_data_w(m_dac, BIT(data, 5)); //beeper
}

I8255_INTERFACE( specialist_ppi8255_interface )
{
	DEVCB_DRIVER_MEMBER(special_state, specialist_8255_porta_r),
	DEVCB_DRIVER_MEMBER(special_state, specialist_8255_porta_w),
	DEVCB_DRIVER_MEMBER(special_state, specialist_8255_portb_r),
	DEVCB_DRIVER_MEMBER(special_state, specialist_8255_portb_w),
	DEVCB_DRIVER_MEMBER(special_state, specialist_8255_portc_r),
	DEVCB_DRIVER_MEMBER(special_state, specialist_8255_portc_w)
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


/*
     Specialist MX
*/
WRITE8_MEMBER( special_state::video_memory_w )
{
	m_ram->pointer()[0x9000 + offset] = data;
	m_specimx_colorram[offset] = m_specimx_color;
}

WRITE8_MEMBER( special_state::specimx_video_color_w )
{
	m_specimx_color = data;
}

READ8_MEMBER( special_state::specimx_video_color_r )
{
	return m_specimx_color;
}

void special_state::specimx_set_bank(offs_t i, UINT8 data)
{
	address_space *space = m_maincpu->memory().space(AS_PROGRAM);
	UINT8 *ram = m_ram->pointer();

	space->install_write_bank(0xc000, 0xffbf, "bank3");
	space->install_write_bank(0xffc0, 0xffdf, "bank4");
	memory_set_bankptr(machine(), "bank4", ram + 0xffc0);
	switch(i)
	{
		case 0 :
			space->install_write_bank(0x0000, 0x8fff, "bank1");
			space->install_write_handler(0x9000, 0xbfff, write8_delegate(FUNC(special_state::video_memory_w), this));

			memory_set_bankptr(machine(), "bank1", ram);
			memory_set_bankptr(machine(), "bank2", ram + 0x9000);
			memory_set_bankptr(machine(), "bank3", ram + 0xc000);
			break;
		case 1 :
			space->install_write_bank(0x0000, 0x8fff, "bank1");
			space->install_write_bank(0x9000, 0xbfff, "bank2");

			memory_set_bankptr(machine(), "bank1", ram + 0x10000);
			memory_set_bankptr(machine(), "bank2", ram + 0x19000);
			memory_set_bankptr(machine(), "bank3", ram + 0x1c000);
			break;
		case 2 :
			space->unmap_write(0x0000, 0x8fff);
			space->unmap_write(0x9000, 0xbfff);

			memory_set_bankptr(machine(), "bank1", machine().region("maincpu")->base() + 0x10000);
			memory_set_bankptr(machine(), "bank2", machine().region("maincpu")->base() + 0x19000);

			if (data & 0x80)
				memory_set_bankptr(machine(), "bank3", ram + 0x1c000);
			else
				memory_set_bankptr(machine(), "bank3", ram + 0xc000);

			break;
	}
}

WRITE8_MEMBER( special_state::specimx_select_bank )
{
	specimx_set_bank(offset, data);
}

WRITE_LINE_MEMBER( special_state::specimx_pit8253_out0_changed )
{
	specimx_set_input( m_specimx_audio, 0, state );
}

WRITE_LINE_MEMBER( special_state::specimx_pit8253_out1_changed )
{
	specimx_set_input( m_specimx_audio, 1, state );
}

WRITE_LINE_MEMBER( special_state::specimx_pit8253_out2_changed )
{
	specimx_set_input( m_specimx_audio, 2, state );
}



const struct pit8253_config specimx_pit8253_intf =
{
	{
		{
			2000000,
			DEVCB_NULL,
			DEVCB_DRIVER_LINE_MEMBER(special_state, specimx_pit8253_out0_changed)
		},
		{
			2000000,
			DEVCB_NULL,
			DEVCB_DRIVER_LINE_MEMBER(special_state, specimx_pit8253_out1_changed)
		},
		{
			2000000,
			DEVCB_NULL,
			DEVCB_DRIVER_LINE_MEMBER(special_state, specimx_pit8253_out2_changed)
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
	state->specimx_set_bank(2, 0); // Initiali load ROM disk
	state->m_specimx_color = 0x70;
	machine.scheduler().timer_set(attotime::zero, FUNC(setup_pit8253_gates));
	device_t *fdc = machine.device("wd1793");
	wd17xx_set_pause_time(fdc,12);
	wd17xx_dden_w(fdc, 0);
}

READ8_MEMBER( special_state::specimx_disk_ctrl_r )
{
	return 0xff;
}

WRITE8_MEMBER( special_state::specimx_disk_ctrl_w )
{
	switch(offset)
	{
		case 2 :
				wd17xx_set_side(m_fdc, data & 1);
				break;
		case 3 :
				wd17xx_set_drive(m_fdc, data & 1);
				break;
	}
}

/*
    Erik
*/

void special_state::erik_set_bank()
{
	UINT8 bank1 = m_RR_register & 3;
	UINT8 bank2 = (m_RR_register >> 2) & 3;
	UINT8 bank3 = (m_RR_register >> 4) & 3;
	UINT8 bank4 = (m_RR_register >> 6) & 3;
	UINT8 *mem = machine().region("maincpu")->base();
	UINT8 *ram = m_ram->pointer();
	address_space *space = m_maincpu->memory().space(AS_PROGRAM);

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
			memory_set_bankptr(machine(), "bank1", ram + 0x10000*(bank1-1));
			break;
		case	0:
			space->unmap_write(0x0000, 0x3fff);
			memory_set_bankptr(machine(), "bank1", mem + 0x10000);
			break;
	}
	switch(bank2)
	{
		case	1:
		case	2:
		case	3:
			memory_set_bankptr(machine(), "bank2", ram + 0x10000*(bank2-1) + 0x4000);
			break;
		case	0:
			space->unmap_write(0x4000, 0x8fff);
			memory_set_bankptr(machine(), "bank2", mem + 0x14000);
			break;
	}
	switch(bank3)
	{
		case	1:
		case	2:
		case	3:
			memory_set_bankptr(machine(), "bank3", ram + 0x10000*(bank3-1) + 0x9000);
			break;
		case	0:
			space->unmap_write(0x9000, 0xbfff);
			memory_set_bankptr(machine(), "bank3", mem + 0x19000);
			break;
	}
	switch(bank4)
	{
		case	1:
		case	2:
		case	3:
			memory_set_bankptr(machine(), "bank4", ram + 0x10000*(bank4-1) + 0x0c000);
			memory_set_bankptr(machine(), "bank5", ram + 0x10000*(bank4-1) + 0x0f000);
			memory_set_bankptr(machine(), "bank6", ram + 0x10000*(bank4-1) + 0x0f800);
			break;
		case	0:
			space->unmap_write(0xc000, 0xefff);
			memory_set_bankptr(machine(), "bank4", mem + 0x1c000);
			space->unmap_write(0xf000, 0xf7ff);
			space->nop_read(0xf000, 0xf7ff);
			space->install_readwrite_handler(0xf800, 0xf803, 0, 0x7fc, read8_delegate(FUNC(i8255_device::read), (i8255_device*)m_ppi), write8_delegate(FUNC(i8255_device::write), (i8255_device*)m_ppi));
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
	state->erik_set_bank();
}

READ8_MEMBER( special_state::erik_rr_reg_r )
{
	return m_RR_register;
}

WRITE8_MEMBER( special_state::erik_rr_reg_w )
{
	m_RR_register = data;
	erik_set_bank();
}

READ8_MEMBER( special_state::erik_rc_reg_r )
{
	return m_RC_register;
}

WRITE8_MEMBER( special_state::erik_rc_reg_w )
{
	m_RC_register = data;
	m_erik_color_1 = m_RC_register & 7;
	m_erik_color_2 = (m_RC_register >> 3) & 7;
	m_erik_background = BIT(m_RC_register, 6) + BIT(m_RC_register, 7) * 4;
}

READ8_MEMBER( special_state::erik_disk_reg_r )
{
	return 0xff;
}

WRITE8_MEMBER( special_state::erik_disk_reg_w )
{
	wd17xx_set_side (m_fdc,data & 1);
	wd17xx_set_drive(m_fdc,(data >> 1) & 1);
	wd17xx_dden_w(m_fdc, BIT(data, 2));
	floppy_mon_w(floppy_get_device(machine(), BIT(data, 1)), 0);
	floppy_mon_w(floppy_get_device(machine(), BIT(data, 1) ^ 1), 1);
	floppy_drive_set_ready_state(floppy_get_device(machine(), BIT(data, 1)), 1, 1);
}
