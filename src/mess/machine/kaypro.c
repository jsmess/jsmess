
#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/ctronics.h"
#include "imagedev/snapquik.h"
#include "includes/kaypro.h"
#include "imagedev/flopdrv.h"





/***********************************************************

    PIO

    Port B is unused on both PIOs

************************************************************/

static void kaypro_interrupt(device_t *device, int state)
{
	cputag_set_input_line(device->machine(), "maincpu", 0, state);
}

static READ8_DEVICE_HANDLER( pio_system_r )
{
	kaypro_state *state = device->machine().driver_data<kaypro_state>();
	UINT8 data = 0;

	/* centronics busy */
	data |= centronics_not_busy_r(state->m_printer) << 3;

	/* PA7 is pulled high */
	data |= 0x80;

	return data;
}

static WRITE8_DEVICE_HANDLER( common_pio_system_w )
{
	kaypro_state *state = device->machine().driver_data<kaypro_state>();
/*  d7 bank select
    d6 disk drive motors - (0=on)
    d5 double-density enable (0=double density)
    d4 Centronics strobe
    d2 side select (1=side 1)
    d1 drive B
    d0 drive A */

	/* get address space */
	address_space *mem = device->machine().device("maincpu")->memory().space(AS_PROGRAM);

	if (data & 0x80)
	{
		mem->unmap_readwrite (0x0000, 0x3fff);
		mem->install_read_bank (0x0000, 0x0fff, "bank1");
		memory_set_bankptr(mem->machine(), "bank1", mem->machine().region("maincpu")->base());
		mem->install_legacy_readwrite_handler (0x3000, 0x3fff, FUNC(kaypro_videoram_r), FUNC(kaypro_videoram_w));
	}
	else
	{
		mem->unmap_readwrite(0x0000, 0x3fff);
		mem->install_read_bank (0x0000, 0x3fff, "bank2");
		mem->install_write_bank (0x0000, 0x3fff, "bank3");
		memory_set_bankptr(mem->machine(), "bank2", mem->machine().region("rambank")->base());
		memory_set_bankptr(mem->machine(), "bank3", mem->machine().region("rambank")->base());
	}

	wd17xx_dden_w(state->m_fdc, BIT(data, 5));

	centronics_strobe_w(state->m_printer, BIT(data, 4));

	if (data & 1)
		wd17xx_set_drive(state->m_fdc, 0);
	else
	if (data & 2)
		wd17xx_set_drive(state->m_fdc, 1);

	output_set_value("ledA",(data & 1) ? 1 : 0);		/* LEDs in artwork */
	output_set_value("ledB",(data & 2) ? 1 : 0);

	/* CLEAR_LINE means to turn motors on */
	floppy_mon_w(floppy_get_device(mem->machine(), 0), (data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
	floppy_mon_w(floppy_get_device(mem->machine(), 1), (data & 0x40) ? ASSERT_LINE : CLEAR_LINE);

	state->m_system_port = data;
}

static WRITE8_DEVICE_HANDLER( kayproii_pio_system_w )
{
	kaypro_state *state = device->machine().driver_data<kaypro_state>();
	common_pio_system_w(device, offset, data);

	/* side select */
	wd17xx_set_side(state->m_fdc, !BIT(data, 2));
}

static WRITE8_DEVICE_HANDLER( kaypro4_pio_system_w )
{
	kaypro_state *state = device->machine().driver_data<kaypro_state>();
	common_pio_system_w(device, offset, data);

	/* side select */
	wd17xx_set_side(state->m_fdc, BIT(data, 2));
}

const z80pio_interface kayproii_pio_g_intf =
{
	DEVCB_LINE(kaypro_interrupt),
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER("centronics", centronics_data_w),
	DEVCB_NULL,			/* portA ready active callback */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL			/* portB ready active callback */
};

const z80pio_interface kayproii_pio_s_intf =
{
	DEVCB_LINE(kaypro_interrupt),
	DEVCB_HANDLER(pio_system_r),	/* read printer status */
	DEVCB_HANDLER(kayproii_pio_system_w),	/* activate various internal devices */
	DEVCB_NULL,			/* portA ready active callback */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL			/* portB ready active callback */
};

const z80pio_interface kaypro4_pio_s_intf =
{
	DEVCB_LINE(kaypro_interrupt),
	DEVCB_HANDLER(pio_system_r),	/* read printer status */
	DEVCB_HANDLER(kaypro4_pio_system_w),	/* activate various internal devices */
	DEVCB_NULL,			/* portA ready active callback */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL			/* portB ready active callback */
};

/***********************************************************

    KAYPRO2X SYSTEM PORT

    The PIOs were replaced by a few standard 74xx chips

************************************************************/

READ8_HANDLER( kaypro2x_system_port_r )
{
	kaypro_state *state = space->machine().driver_data<kaypro_state>();
	UINT8 data = centronics_busy_r(state->m_printer) << 6;
	return (state->m_system_port & 0xbf) | data;
}

WRITE8_HANDLER( kaypro2x_system_port_w )
{
	kaypro_state *state = space->machine().driver_data<kaypro_state>();
/*  d7 bank select
    d6 alternate character set (write only)
    d5 double-density enable
    d4 disk drive motors (1=on)
    d3 Centronics strobe
    d2 side select (appears that 0=side 1?)
    d1 drive B
    d0 drive A */

	/* get address space */
	address_space *mem = space->machine().device("maincpu")->memory().space(AS_PROGRAM);

	if (data & 0x80)
	{
		mem->unmap_readwrite (0x0000, 0x3fff);
		mem->install_read_bank (0x0000, 0x1fff, "bank1");
		memory_set_bankptr(mem->machine(), "bank1", mem->machine().region("maincpu")->base());
	}
	else
	{
		mem->unmap_readwrite (0x0000, 0x3fff);
		mem->install_read_bank (0x0000, 0x3fff, "bank2");
		mem->install_write_bank (0x0000, 0x3fff, "bank3");
		memory_set_bankptr(mem->machine(), "bank2", mem->machine().region("rambank")->base());
		memory_set_bankptr(mem->machine(), "bank3", mem->machine().region("rambank")->base());
	}

	wd17xx_dden_w(state->m_fdc, BIT(data, 5));

	centronics_strobe_w(state->m_printer, BIT(data, 3));

	if (data & 1)
		wd17xx_set_drive(state->m_fdc, 0);
	else
	if (data & 2)
		wd17xx_set_drive(state->m_fdc, 1);

	wd17xx_set_side(state->m_fdc, (data & 4) ? 0 : 1);

	output_set_value("ledA",(data & 1) ? 1 : 0);		/* LEDs in artwork */
	output_set_value("ledB",(data & 2) ? 1 : 0);

	/* CLEAR_LINE means to turn motors on */
	floppy_mon_w(floppy_get_device(space->machine(), 0), (data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
	floppy_mon_w(floppy_get_device(space->machine(), 1), (data & 0x10) ? CLEAR_LINE : ASSERT_LINE);

	state->m_system_port = data;
}



/***********************************************************************

    SIO
    Baud rate setup commented out until MAME includes the feature.

    On Kaypro2x, Channel B on both SIOs is hardwired to 300 baud.

    Both devices on sio2 (printer and modem) are not emulated.

************************************************************************/

/* Set baud rate. bits 0..3 Rx and Tx are tied together. Baud Rate Generator is a AY-5-8116, SMC8116, etc.
    00h    50
    11h    75
    22h    110
    33h    134.5
    44h    150
    55h    300
    66h    600
    77h    1200
    88h    1800
    99h    2000
    AAh    2400
    BBh    3600
    CCh    4800
    DDh    7200
    EEh    9600
    FFh    19200 */

static const int baud_clock[]={ 800, 1200, 1760, 2152, 2400, 4800, 9600, 19200, 28800, 32000, 38400, 57600, 76800, 115200, 153600, 307200 };

WRITE8_HANDLER( kaypro_baud_a_w )	/* channel A - RS232C */
{
	//kaypro_state *state = space->machine().driver_data<kaypro_state>();
	data &= 0x0f;

//  z80sio_set_rx_clock( state->m_z80sio, baud_clock[data], 0);
//  z80sio_set_tx_clock( state->m_z80sio, baud_clock[data], 0);
}

WRITE8_HANDLER( kayproii_baud_b_w )	/* Channel B - Keyboard - only usable speed is 300 baud */
{
	//kaypro_state *state = space->machine().driver_data<kaypro_state>();
	data &= 0x0f;

//  z80sio_set_rx_clock( state->m_z80sio, baud_clock[data], 1);
//  z80sio_set_tx_clock( state->m_z80sio, baud_clock[data], 1);
}

WRITE8_HANDLER( kaypro2x_baud_a_w )	/* Channel A on 2nd SIO - Serial Printer */
{
	//kaypro_state *state = space->machine().driver_data<kaypro_state>();
	data &= 0x0f;

//  z80sio_set_rx_clock( state->m_kaypro2x_z80sio, baud_clock[data], 0);
//  z80sio_set_tx_clock( state->m_kaypro2x_z80sio, baud_clock[data], 0);
}

const z80sio_interface kaypro_sio_intf =
{
	kaypro_interrupt,	/* interrupt handler */
	0,			/* DTR changed handler */
	0,			/* RTS changed handler */
	0,			/* BREAK changed handler */
	0,			/* transmit handler - which channel is this for? */
	0			/* receive handler - which channel is this for? */
};

READ8_DEVICE_HANDLER( kaypro_sio_r )
{
	if (!offset)
		return z80sio_d_r(device, 0);
	else
	if (offset == 1)
//      return z80sio_d_r(device, 1);
		return kay_kbd_d_r(device->machine());
	else
	if (offset == 2)
		return z80sio_c_r(device, 0);
	else
//      return z80sio_c_r(device, 1);
		return kay_kbd_c_r(device->machine());
}

WRITE8_DEVICE_HANDLER( kaypro_sio_w )
{
	if (!offset)
		z80sio_d_w(device, 0, data);
	else
	if (offset == 1)
//      z80sio_d_w(device, 1, data);
		kay_kbd_d_w(device->machine(), data);
	else
	if (offset == 2)
		z80sio_c_w(device, 0, data);
	else
		z80sio_c_w(device, 1, data);
}


/*************************************************************************************

    Floppy DIsk

    If DRQ or IRQ is set, and cpu is halted, the NMI goes low.
    Since the HALT occurs last (and has no callback mechanism), we need to set
    a short delay, to give time for the processor to execute the HALT before NMI
    becomes active.

*************************************************************************************/

static TIMER_CALLBACK( kaypro_timer_callback )
{
	if (cpu_get_reg(machine.device("maincpu"), Z80_HALT))
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, ASSERT_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( kaypro_fdc_intrq_w )
{
	if (state)
		device->machine().scheduler().timer_set(attotime::from_usec(25), FUNC(kaypro_timer_callback));
	else
		cputag_set_input_line(device->machine(), "maincpu", INPUT_LINE_NMI, CLEAR_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( kaypro_fdc_drq_w )
{
	if (state)
		device->machine().scheduler().timer_set(attotime::from_usec(25), FUNC(kaypro_timer_callback));
	else
		cputag_set_input_line(device->machine(), "maincpu", INPUT_LINE_NMI, CLEAR_LINE);

}

const wd17xx_interface kaypro_wd1793_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(kaypro_fdc_intrq_w),
	DEVCB_LINE(kaypro_fdc_drq_w),
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};


/***********************************************************

    Machine

************************************************************/
MACHINE_START( kayproii )
{
	kaypro_state *state = machine.driver_data<kaypro_state>();
	state->m_kayproii_z80pio_g = machine.device("z80pio_g");
	state->m_kayproii_z80pio_s = machine.device("z80pio_s");
	state->m_z80sio = machine.device("z80sio");
	state->m_printer = machine.device("centronics");
	state->m_fdc = machine.device("wd1793");

	z80pio_astb_w(state->m_kayproii_z80pio_s, 0);
}

MACHINE_RESET( kayproii )
{
	MACHINE_RESET_CALL(kay_kbd);
}

MACHINE_START( kaypro2x )
{
	kaypro_state *state = machine.driver_data<kaypro_state>();
	state->m_z80sio = machine.device("z80sio");
	state->m_kaypro2x_z80sio = machine.device("z80sio_2x");
	state->m_printer = machine.device("centronics");
	state->m_fdc = machine.device("wd1793");
}

MACHINE_RESET( kaypro2x )
{
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	kaypro2x_system_port_w(space, 0, 0x80);
	MACHINE_RESET_CALL(kay_kbd);
}

/***********************************************************

    Quickload

    This loads a .COM file to address 0x100 then jumps
    there. Sometimes .COM has been renamed to .CPM to
    prevent windows going ballistic. These can be loaded
    as well.

************************************************************/

QUICKLOAD_LOAD( kayproii )
{
	kaypro_state *state = image.device().machine().driver_data<kaypro_state>();
	device_t *cpu = image.device().machine().device("maincpu");
	UINT8 *RAM = image.device().machine().region("rambank")->base();
	UINT16 i;
	UINT8 data;

	/* Load image to the TPA (Transient Program Area) */
	for (i = 0; i < quickload_size; i++)
	{
		if (image.fread( &data, 1) != 1) return IMAGE_INIT_FAIL;

		RAM[i+0x100] = data;
	}

//  if (input_port_read(image.device().machine(), "CONFIG") & 1)
	{
		common_pio_system_w(state->m_kayproii_z80pio_s, 0, state->m_system_port & 0x7f);	// switch TPA in
		RAM[0x80]=0;							// clear out command tail
		RAM[0x81]=0;
		cpu_set_reg(cpu, STATE_GENPC, 0x100);				// start program
	}

	return IMAGE_INIT_PASS;
}

QUICKLOAD_LOAD( kaypro2x )
{
	kaypro_state *state = image.device().machine().driver_data<kaypro_state>();
	address_space *space = image.device().machine().device("maincpu")->memory().space(AS_PROGRAM);
	device_t *cpu = image.device().machine().device("maincpu");
	UINT8 *RAM = image.device().machine().region("rambank")->base();
	UINT16 i;
	UINT8 data;

	for (i = 0; i < quickload_size; i++)
	{
		if (image.fread( &data, 1) != 1) return IMAGE_INIT_FAIL;

		RAM[i+0x100] = data;
	}

//  if (input_port_read(image.device().machine(), "CONFIG") & 1)
	{
		kaypro2x_system_port_w(space, 0, state->m_system_port & 0x7f);
		RAM[0x80]=0;
		RAM[0x81]=0;
		cpu_set_reg(cpu, STATE_GENPC, 0x100);
	}

	return IMAGE_INIT_PASS;
}
