/***************************************************************************

        LLC driver by Miodrag Milanovic

        17/04/2009 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/llc.h"
#include "machine/ram.h"



static READ8_DEVICE_HANDLER (llc1_port2_b_r)
{
	llc_state *state = device->machine().driver_data<llc_state>();
	UINT8 retVal = 0;

	if (state->m_term_status)
	{
		retVal = state->m_term_status;
		state->m_term_status = 0;
	}
	else
		retVal = state->m_term_data;

	return retVal;
}

static READ8_DEVICE_HANDLER (llc1_port2_a_r)
{
	return 0;
}

static READ8_DEVICE_HANDLER (llc1_port1_a_r)
{
	llc_state *state = device->machine().driver_data<llc_state>();
	UINT8 data = 0;
	if (!BIT(state->m_porta, 4))
		data = state->ioport("X4")->read();
	if (!BIT(state->m_porta, 5))
		data = state->ioport("X5")->read();
	if (!BIT(state->m_porta, 6))
		data = state->ioport("X6")->read();
	if (data & 0xf0)
		data = (data >> 4) | 0x80;

	return data | (state->m_porta & 0x70);
}

static WRITE8_DEVICE_HANDLER (llc1_port1_a_w)
{
	llc_state *state = device->machine().driver_data<llc_state>();
	state->m_porta = data;
}

static WRITE8_DEVICE_HANDLER (llc1_port1_b_w)
{
	static UINT8 count = 0, digit = 0;

	if (data == 0)
	{
		digit = 0;
		count = 0;
	}
	else
		count++;

	if (count == 1)
		output_set_digit_value(digit, data & 0x7f);
	else
	if (count == 3)
	{
		count = 0;
		digit++;
	}
}

// timer 0 irq does digit display, and timer 3 irq does scan of the
// monitor keyboard. Currently the keyboard is being scanned too fast.
// No idea how the CTC is connected, so guessed.
Z80CTC_INTERFACE( llc1_ctc_intf )
{
	0,
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_DEVICE_LINE("z80ctc", z80ctc_trg1_w),
	DEVCB_DEVICE_LINE("z80ctc", z80ctc_trg3_w),
	DEVCB_NULL
};

Z80PIO_INTERFACE( llc1_z80pio1_intf )
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_HANDLER(llc1_port1_a_r),
	DEVCB_HANDLER(llc1_port1_a_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(llc1_port1_b_w),
	DEVCB_NULL
};

Z80PIO_INTERFACE( llc1_z80pio2_intf )
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_HANDLER(llc1_port2_a_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(llc1_port2_b_r),
	DEVCB_NULL,
	DEVCB_NULL
};

DRIVER_INIT(llc1)
{
}

MACHINE_RESET( llc1 )
{
}

MACHINE_START(llc1)
{
}

Z80CTC_INTERFACE( llc2_ctc_intf )
{
	0,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


/* Driver initialization */
DRIVER_INIT(llc2)
{
	llc_state *state = machine.driver_data<llc_state>();
	state->m_video_ram.set_target( machine.device<ram_device>(RAM_TAG)->pointer() + 0xc000,state->m_video_ram.bytes());
}

MACHINE_RESET( llc2 )
{
	llc_state *state = machine.driver_data<llc_state>();
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	space->unmap_write(0x0000, 0x3fff);
	state->membank("bank1")->set_base(machine.root_device().memregion("maincpu")->base());

	space->unmap_write(0x4000, 0x5fff);
	state->membank("bank2")->set_base(machine.root_device().memregion("maincpu")->base() + 0x4000);

	space->unmap_write(0x6000, 0xbfff);
	state->membank("bank3")->set_base(machine.root_device().memregion("maincpu")->base() + 0x6000);

	space->install_write_bank(0xc000, 0xffff, "bank4");
	state->membank("bank4")->set_base(machine.device<ram_device>(RAM_TAG)->pointer() + 0xc000);

}

WRITE8_MEMBER(llc_state::llc2_rom_disable_w)
{
	address_space *mem_space = machine().device("maincpu")->memory().space(AS_PROGRAM);
	UINT8 *ram = machine().device<ram_device>(RAM_TAG)->pointer();

	mem_space->install_write_bank(0x0000, 0xbfff, "bank1");
	membank("bank1")->set_base(ram);

	mem_space->install_write_bank(0x4000, 0x5fff, "bank2");
	membank("bank2")->set_base(ram + 0x4000);

	mem_space->install_write_bank(0x6000, 0xbfff, "bank3");
	membank("bank3")->set_base(ram + 0x6000);

	mem_space->install_write_bank(0xc000, 0xffff, "bank4");
	membank("bank4")->set_base(ram + 0xc000);

}

WRITE8_MEMBER(llc_state::llc2_basic_enable_w)
{

	address_space *mem_space = machine().device("maincpu")->memory().space(AS_PROGRAM);
	if (data & 0x02) {
		mem_space->unmap_write(0x4000, 0x5fff);
		membank("bank2")->set_base(machine().root_device().memregion("maincpu")->base() + 0x10000);
	} else {
		mem_space->install_write_bank(0x4000, 0x5fff, "bank2");
		membank("bank2")->set_base(machine().device<ram_device>(RAM_TAG)->pointer() + 0x4000);
	}

}

static READ8_DEVICE_HANDLER (llc2_port_b_r)
{
	return 0;
}

static UINT8 key_pos(UINT8 val)
{
	if (BIT(val,0)) return 1;
	if (BIT(val,1)) return 2;
	if (BIT(val,2)) return 3;
	if (BIT(val,3)) return 4;
	if (BIT(val,4)) return 5;
	if (BIT(val,5)) return 6;
	if (BIT(val,6)) return 7;
	if (BIT(val,7)) return 8;
	return 0;
}

static READ8_DEVICE_HANDLER (llc2_port_a_r)
{
	UINT8 *k7659 = device->machine().root_device().memregion("k7659")->base();
	UINT8 retVal = 0;
	UINT8 a1 = device->machine().root_device().ioport("A1")->read();
	UINT8 a2 = device->machine().root_device().ioport("A2")->read();
	UINT8 a3 = device->machine().root_device().ioport("A3")->read();
	UINT8 a4 = device->machine().root_device().ioport("A4")->read();
	UINT8 a5 = device->machine().root_device().ioport("A5")->read();
	UINT8 a6 = device->machine().root_device().ioport("A6")->read();
	UINT8 a7 = device->machine().root_device().ioport("A7")->read();
	UINT8 a8 = device->machine().root_device().ioport("A8")->read();
	UINT8 a9 = device->machine().root_device().ioport("A9")->read();
	UINT8 a10 = device->machine().root_device().ioport("A10")->read();
	UINT8 a11 = device->machine().root_device().ioport("A11")->read();
	UINT8 a12 = device->machine().root_device().ioport("A12")->read();
	UINT16 code = 0;
	if (a1!=0)
		code = 0x10 + key_pos(a1);
	else if (a2!=0)
		code = 0x20 + key_pos(a2);
	else if (a3!=0)
		code = 0x30 + key_pos(a3);
	else if (a4!=0)
		code = 0x40 + key_pos(a4);
	else if (a5!=0)
		code = 0x50 + key_pos(a5);
	else if (a6!=0)
		code = 0x60 + key_pos(a6);
	else if (a7!=0)
		code = 0x70 + key_pos(a7);
	else if (a9!=0)
		code = 0x80 + key_pos(a9);
	else if (a10!=0)
		code = 0x90 + key_pos(a10);
	else if (a11!=0)
		code = 0xA0 + key_pos(a11);

	if (code!=0)
	{
		if (BIT(a8,6) || BIT(a8,7))
			code |= 0x100;
		else
		if (BIT(a12,6))
			code |= 0x200;

		retVal = k7659[code] | 0x80;
	}
	return retVal;
}


Z80PIO_INTERFACE( llc2_z80pio_intf )
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_HANDLER(llc2_port_a_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(llc2_port_b_r),
	DEVCB_NULL,
	DEVCB_NULL
};

