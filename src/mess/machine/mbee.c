/***************************************************************************

    microbee.c

    machine driver
    Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000


****************************************************************************/

#include "emu.h"
#include "machine/mc146818.h"
#include "devices/flopdrv.h"
#include "includes/mbee.h"


/***********************************************************

    PIO

************************************************************/

static WRITE8_DEVICE_HANDLER( pio_ardy )
{
	mbee_state *state = device->machine->driver_data<mbee_state>();
	/* devices need to be redeclared in this callback for some strange reason */
	state->printer = device->machine->device("centronics");
	centronics_strobe_w(state->printer, (data) ? 0 : 1);
}

static WRITE8_DEVICE_HANDLER( pio_port_a_w )
{
	mbee_state *state = device->machine->driver_data<mbee_state>();
	/* hardware strobe driven by PIO ARDY, bit 7..0 = data */
	z80pio_astb_w( state->z80pio, 1);	/* needed - otherwise nothing prints */
	centronics_data_w(state->printer, 0, data);
};

static WRITE8_DEVICE_HANDLER( pio_port_b_w )
{
	mbee_state *state = device->machine->driver_data<mbee_state>();
/*  PIO port B - d5..d2 not emulated
    d7 network interrupt (microbee network for classrooms)
    d6 speaker
    d5 rs232 output (1=mark)
    d4 rs232 input (0=mark)
    d3 rs232 CTS (0=clear to send)
    d2 rs232 clock or DTR
    d1 cass out and (on 256tc) keyboard irq
    d0 cass in */

	cassette_output(state->cassette, (data & 0x02) ? -1.0 : +1.0);

	speaker_level_w(state->speaker, (data & 0x40) ? 1 : 0);
};

static READ8_DEVICE_HANDLER( pio_port_b_r )
{
	mbee_state *state = device->machine->driver_data<mbee_state>();
	UINT8 data = 0;

	if (cassette_input(state->cassette) > 0.03) data |= 1;

	data |= state->clock_pulse;
	data |= state->mbee256_key_available;

	state->clock_pulse = 0;

	return data;
};

const z80pio_interface mbee_z80pio_intf =
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_HANDLER(pio_port_a_w),
	DEVCB_HANDLER(pio_ardy),
	DEVCB_HANDLER(pio_port_b_r),
	DEVCB_HANDLER(pio_port_b_w),
	DEVCB_NULL
};

/*************************************************************************************

    Floppy DIsk

    The callback is quite simple, no interrupts are used.
    If either IRQ or DRQ activate, they set bit 7 of inport 0x48.

*************************************************************************************/

static WRITE_LINE_DEVICE_HANDLER( mbee_fdc_intrq_w )
{
	mbee_state *drvstate = device->machine->driver_data<mbee_state>();
	drvstate->fdc_intrq = state ? 0x80 : 0;
}

static WRITE_LINE_DEVICE_HANDLER( mbee_fdc_drq_w )
{
	mbee_state *drvstate = device->machine->driver_data<mbee_state>();
	drvstate->fdc_drq = state ? 0x80 : 0;
}

const wd17xx_interface mbee_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(mbee_fdc_intrq_w),
	DEVCB_LINE(mbee_fdc_drq_w),
	{FLOPPY_0, FLOPPY_1, NULL, NULL }
};

READ8_HANDLER ( mbee_fdc_status_r )
{
	mbee_state *state = space->machine->driver_data<mbee_state>();
/*  d7 indicate if IRQ or DRQ is occuring (1=happening)
    d6..d0 not used */

	return 0x7f | state->fdc_intrq | state->fdc_drq;
}

WRITE8_HANDLER ( mbee_fdc_motor_w )
{
	mbee_state *state = space->machine->driver_data<mbee_state>();
/*  d7..d4 not used
    d3 density (1=MFM)
    d2 side (1=side 1)
    d1..d0 drive select (0 to 3) */

	wd17xx_set_drive(state->fdc, data & 3);
	wd17xx_set_side(state->fdc, (data & 4) ? 1 : 0);
	wd17xx_dden_w(state->fdc, !BIT(data, 3));
	/* no idea what turns the motors on & off, guessing it could be drive select 
	commented out because it prevents 128k and 256TC from booting up */
	//floppy_mon_w(floppy_get_device(space->machine, data & 3), CLEAR_LINE); // motor on
}

/***********************************************************

    256TC Keyboard

************************************************************/


static TIMER_CALLBACK( mbee256_kbd )
{
	mbee_state *state = machine->driver_data<mbee_state>();
    /* Keyboard scanner is a Mostek M3870 chip. Its speed of operation is determined by a 15k resistor on
    pin 2 (XTL2) and is therefore unknown. If a key change is detected (up or down), the /strobe
    line activates, sending a high to bit 1 of port 2 (one of the pio input lines). The next read of
    port 18 will clear this line, and read the key scancode. It will also signal the 3870 that the key
    data has been read, on pin 38 (/extint). The 3870 can cache up to 9 keys. With no rom dump
    available, the following is a guess.

    The 3870 (MK3870) 8-bit microcontroller is a single chip implementation of Fairchild F8 (Mostek 3850).
    It includes up to 4 KB of mask-programmable ROM, 64 bytes of scratchpad RAM and up to 64 bytes
    of executable RAM. The MCU also integrates 32-bit I/O and a programmable timer. */

	UINT8 i, j;
	UINT8 pressed[15];
	char kbdrow[6];


	/* see what is pressed */
	for (i = 0; i < 15; i++)
	{
		sprintf(kbdrow,"LINE%d",i);
		pressed[i] = (input_port_read(machine, kbdrow));
	}

	/* find what has changed */
	for (i = 0; i < 15; i++)
	{
		if (pressed[i] != state->mbee256_was_pressed[i])
		{
			/* get scankey value */
			for (j = 0; j < 8; j++)
			{
				if (BIT(pressed[i]^state->mbee256_was_pressed[i], j))
				{
					/* put it in the queue */
					state->mbee256_q[state->mbee256_q_pos] = (i << 3) | j | (BIT(pressed[i], j) ? 0x80 : 0);
					if (state->mbee256_q_pos < 19) state->mbee256_q_pos++;
				}
			}
			state->mbee256_was_pressed[i] = pressed[i];
		}
	}

	/* if anything queued, cause an interrupt */
	if (state->mbee256_q_pos)
		state->mbee256_key_available = 2; // set irq
}

READ8_HANDLER( mbee256_18_r )
{
	mbee_state *state = space->machine->driver_data<mbee_state>();
	UINT8 i, data = state->mbee256_q[0]; // get oldest key

	if (state->mbee256_q_pos)
	{
		state->mbee256_q_pos--;
		for (i = 0; i < state->mbee256_q_pos; i++) state->mbee256_q[i] = state->mbee256_q[i+1]; // ripple queue
	}

	state->mbee256_key_available = 0; // clear irq
	return data;
}


/***********************************************************

    256TC Change CPU speed

************************************************************/

READ8_HANDLER( mbee256_speed_low_r )
{
	cputag_set_clock(space->machine, "maincpu", 3375000);
	return 0xff;
}

READ8_HANDLER( mbee256_speed_high_r )
{
	cputag_set_clock(space->machine, "maincpu", 6750000);
	return 0xff;
}



/***********************************************************

    256TC Real Time Clock

************************************************************/

WRITE8_HANDLER( mbee_04_w )	// address
{
	mbee_state *state = space->machine->driver_data<mbee_state>();
	state->rtc->write(*space, 0, data);
}

WRITE8_HANDLER( mbee_06_w )	// write
{
	mbee_state *state = space->machine->driver_data<mbee_state>();
	state->rtc->write(*space, 1, data);
}

READ8_HANDLER( mbee_07_r )	// read
{
	mbee_state *state = space->machine->driver_data<mbee_state>();
	return state->rtc->read(*space, 1);
}

static TIMER_CALLBACK( mbee_rtc_irq )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	address_space *mem = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	mc146818_device *rtc = machine->device<mc146818_device>("rtc");
	UINT8 data = rtc->read(*mem, 12);
	if (data) state->clock_pulse = 0x80;
}


/***********************************************************

    256TC Memory Banking

    Bits 0, 1 and 5 select which bank goes into 0000-7FFF.
    Bit 2 disables ROM, replacing it with RAM.
    Bit 3 disables Video, replacing it with RAM.
    Bit 4 switches the video circuits between F000-FFFF and
          8000-8FFF.

    In case of a clash, video overrides ROM which overrides RAM.

************************************************************/

WRITE8_HANDLER( mbee256_50_w )
{
	address_space *mem = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	// primary low banks
	memory_set_bank(space->machine, "boot", (data & 3) | ((data & 0x20) >> 3));
	memory_set_bank(space->machine, "bank1", (data & 3) | ((data & 0x20) >> 3));

	// 9000-EFFF
	memory_set_bank(space->machine, "bank9", (data & 4) ? 1 : 0);

	// 8000-8FFF, F000-FFFF
	memory_unmap_readwrite (mem, 0x8000, 0x87ff, 0, 0);
	memory_unmap_readwrite (mem, 0x8800, 0x8fff, 0, 0);
	memory_unmap_readwrite (mem, 0xf000, 0xf7ff, 0, 0);
	memory_unmap_readwrite (mem, 0xf800, 0xffff, 0, 0);

	switch (data & 0x1c)
	{
		case 0x00:
			memory_install_read_bank (mem, 0x8000, 0x87ff, 0, 0, "bank8l");
			memory_install_read_bank (mem, 0x8800, 0x8fff, 0, 0, "bank8h");
			memory_install_readwrite8_handler (mem, 0xf000, 0xf7ff, 0, 0, mbeeppc_low_r, mbeeppc_low_w);
			memory_install_readwrite8_handler (mem, 0xf800, 0xffff, 0, 0, mbeeppc_high_r, mbeeppc_high_w);
			memory_set_bank(space->machine, "bank8l", 0); // rom
			memory_set_bank(space->machine, "bank8h", 0); // rom
			break;
		case 0x04:
			memory_install_read_bank (mem, 0x8000, 0x87ff, 0, 0, "bank8l");
			memory_install_read_bank (mem, 0x8800, 0x8fff, 0, 0, "bank8h");
			memory_install_readwrite8_handler (mem, 0xf000, 0xf7ff, 0, 0, mbeeppc_low_r, mbeeppc_low_w);
			memory_install_readwrite8_handler (mem, 0xf800, 0xffff, 0, 0, mbeeppc_high_r, mbeeppc_high_w);
			memory_set_bank(space->machine, "bank8l", 1); // ram
			memory_set_bank(space->machine, "bank8h", 1); // ram
			break;
		case 0x08:
		case 0x18:
			memory_install_read_bank (mem, 0x8000, 0x87ff, 0, 0, "bank8l");
			memory_install_read_bank (mem, 0x8800, 0x8fff, 0, 0, "bank8h");
			memory_install_read_bank (mem, 0xf000, 0xf7ff, 0, 0, "bankfl");
			memory_install_read_bank (mem, 0xf800, 0xffff, 0, 0, "bankfh");
			memory_set_bank(space->machine, "bank8l", 0); // rom
			memory_set_bank(space->machine, "bank8h", 0); // rom
			memory_set_bank(space->machine, "bankfl", 0); // ram
			memory_set_bank(space->machine, "bankfh", 0); // ram
			break;
		case 0x0c:
		case 0x1c:
			memory_install_read_bank (mem, 0x8000, 0x87ff, 0, 0, "bank8l");
			memory_install_read_bank (mem, 0x8800, 0x8fff, 0, 0, "bank8h");
			memory_install_read_bank (mem, 0xf000, 0xf7ff, 0, 0, "bankfl");
			memory_install_read_bank (mem, 0xf800, 0xffff, 0, 0, "bankfh");
			memory_set_bank(space->machine, "bank8l", 1); // ram
			memory_set_bank(space->machine, "bank8h", 1); // ram
			memory_set_bank(space->machine, "bankfl", 0); // ram
			memory_set_bank(space->machine, "bankfh", 0); // ram
			break;
		case 0x10:
		case 0x14:
			memory_install_readwrite8_handler (mem, 0x8000, 0x87ff, 0, 0, mbeeppc_low_r, mbeeppc_low_w);
			memory_install_readwrite8_handler (mem, 0x8800, 0x8fff, 0, 0, mbeeppc_high_r, mbeeppc_high_w);
			memory_install_read_bank (mem, 0xf000, 0xf7ff, 0, 0, "bankfl");
			memory_install_read_bank (mem, 0xf800, 0xffff, 0, 0, "bankfh");
			memory_set_bank(space->machine, "bankfl", 0); // ram
			memory_set_bank(space->machine, "bankfh", 0); // ram
			break;
	}
}

/***********************************************************

    128k Memory Banking

    The only difference to the 256TC is that bit 5 switches
    between rom2 and rom3. Since neither of these is dumped,
    this bit is not emulated. If it was, this scheme is used:

    Low - rom2 occupies C000-FFFF
    High - ram = C000-DFFF, rom3 = E000-FFFF.

************************************************************/

WRITE8_HANDLER( mbee128_50_w )
{
	address_space *mem = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	// primary low banks
	memory_set_bank(space->machine, "boot", (data & 3));
	memory_set_bank(space->machine, "bank1", (data & 3));

	// 9000-EFFF
	memory_set_bank(space->machine, "bank9", (data & 4) ? 1 : 0);

	// 8000-8FFF, F000-FFFF
	memory_unmap_readwrite (mem, 0x8000, 0x87ff, 0, 0);
	memory_unmap_readwrite (mem, 0x8800, 0x8fff, 0, 0);
	memory_unmap_readwrite (mem, 0xf000, 0xf7ff, 0, 0);
	memory_unmap_readwrite (mem, 0xf800, 0xffff, 0, 0);

	switch (data & 0x1c)
	{
		case 0x00:
			memory_install_read_bank (mem, 0x8000, 0x87ff, 0, 0, "bank8l");
			memory_install_read_bank (mem, 0x8800, 0x8fff, 0, 0, "bank8h");
			memory_install_readwrite8_handler (mem, 0xf000, 0xf7ff, 0, 0, mbeeppc_low_r, mbeeppc_low_w);
			memory_install_readwrite8_handler (mem, 0xf800, 0xffff, 0, 0, mbeeppc_high_r, mbeeppc_high_w);
			memory_set_bank(space->machine, "bank8l", 0); // rom
			memory_set_bank(space->machine, "bank8h", 0); // rom
			break;
		case 0x04:
			memory_install_read_bank (mem, 0x8000, 0x87ff, 0, 0, "bank8l");
			memory_install_read_bank (mem, 0x8800, 0x8fff, 0, 0, "bank8h");
			memory_install_readwrite8_handler (mem, 0xf000, 0xf7ff, 0, 0, mbeeppc_low_r, mbeeppc_low_w);
			memory_install_readwrite8_handler (mem, 0xf800, 0xffff, 0, 0, mbeeppc_high_r, mbeeppc_high_w);
			memory_set_bank(space->machine, "bank8l", 1); // ram
			memory_set_bank(space->machine, "bank8h", 1); // ram
			break;
		case 0x08:
		case 0x18:
			memory_install_read_bank (mem, 0x8000, 0x87ff, 0, 0, "bank8l");
			memory_install_read_bank (mem, 0x8800, 0x8fff, 0, 0, "bank8h");
			memory_install_read_bank (mem, 0xf000, 0xf7ff, 0, 0, "bankfl");
			memory_install_read_bank (mem, 0xf800, 0xffff, 0, 0, "bankfh");
			memory_set_bank(space->machine, "bank8l", 0); // rom
			memory_set_bank(space->machine, "bank8h", 0); // rom
			memory_set_bank(space->machine, "bankfl", 0); // ram
			memory_set_bank(space->machine, "bankfh", 0); // ram
			break;
		case 0x0c:
		case 0x1c:
			memory_install_read_bank (mem, 0x8000, 0x87ff, 0, 0, "bank8l");
			memory_install_read_bank (mem, 0x8800, 0x8fff, 0, 0, "bank8h");
			memory_install_read_bank (mem, 0xf000, 0xf7ff, 0, 0, "bankfl");
			memory_install_read_bank (mem, 0xf800, 0xffff, 0, 0, "bankfh");
			memory_set_bank(space->machine, "bank8l", 1); // ram
			memory_set_bank(space->machine, "bank8h", 1); // ram
			memory_set_bank(space->machine, "bankfl", 0); // ram
			memory_set_bank(space->machine, "bankfh", 0); // ram
			break;
		case 0x10:
		case 0x14:
			memory_install_readwrite8_handler (mem, 0x8000, 0x87ff, 0, 0, mbeeppc_low_r, mbeeppc_low_w);
			memory_install_readwrite8_handler (mem, 0x8800, 0x8fff, 0, 0, mbeeppc_high_r, mbeeppc_high_w);
			memory_install_read_bank (mem, 0xf000, 0xf7ff, 0, 0, "bankfl");
			memory_install_read_bank (mem, 0xf800, 0xffff, 0, 0, "bankfh");
			memory_set_bank(space->machine, "bankfl", 0); // ram
			memory_set_bank(space->machine, "bankfh", 0); // ram
			break;
	}
}


/***********************************************************

    64k Memory Banking

    Bit 2 disables ROM, replacing it with RAM.

    Due to lack of documentation, it is not possible to know
    if other bits are used.

************************************************************/

WRITE8_HANDLER( mbee64_50_w )
{
	if (data & 4)
	{
		memory_set_bank(space->machine, "boot", 0);
		memory_set_bank(space->machine, "bankl", 0);
		memory_set_bank(space->machine, "bankh", 0);
	}
	else
	{
		memory_set_bank(space->machine, "bankl", 1);
		memory_set_bank(space->machine, "bankh", 1);
	}
}


/***********************************************************

    ROM Banking on older models

    Set A to 0 or 1 then read the port to switch between the
    halves of the Telcom ROM.

    Output the PAK number to choose an optional PAK ROM.

    The bios will support 256 PAKs, although normally only
    8 are available in hardware. Each PAK is normally a 4K
    ROM. If 8K ROMs are used, the 2nd half becomes PAK+8,
    thus 16 PAKs in total. This is used in the PC85 models.

************************************************************/

READ8_HANDLER ( mbeeic_0a_r )
{
	mbee_state *state = space->machine->driver_data<mbee_state>();
	return state->_0a;
}

WRITE8_HANDLER ( mbeeic_0a_w )
{
	mbee_state *state = space->machine->driver_data<mbee_state>();
	state->_0a = data;
	memory_set_bank(space->machine, "pak", data & 15);
}

READ8_HANDLER ( mbeepc_telcom_low_r )
{
	mbee_state *state = space->machine->driver_data<mbee_state>();
/* Read of port 0A - set Telcom rom to first half */
	memory_set_bank(space->machine, "telcom", 0);
	return state->_0a;
}

READ8_HANDLER ( mbeepc_telcom_high_r )
{
	mbee_state *state = space->machine->driver_data<mbee_state>();
/* Read of port 10A - set Telcom rom to 2nd half */
	memory_set_bank(space->machine, "telcom", 1);
	return state->_0a;
}


/***********************************************************

    Machine

************************************************************/

/*
  On reset or power on, a circuit forces rom 8000-8FFF to appear at 0000-0FFF, while ram is disabled.
  It gets set back to normal on the first attempt to write to memory. (/WR line goes active).
*/


/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( mbee_reset )
{
	memory_set_bank(machine, "boot", 0);
}

static void machine_reset_common(running_machine *machine)
{
	mbee_state *state = machine->driver_data<mbee_state>();
	state->z80pio = machine->device("z80pio");
	state->speaker = machine->device("speaker");
	state->cassette = machine->device("cassette");
	state->printer = machine->device("centronics");
}

static void machine_reset_common_disk(running_machine *machine)
{
	mbee_state *state = machine->driver_data<mbee_state>();
	machine_reset_common(machine);
	state->fdc = machine->device("fdc");
	/* These values need to be fine tuned or the fdc repaired */
	wd17xx_set_pause_time(state->fdc, 45);       /* default is 40 usec if not set */
	wd17xx_set_complete_command_delay(state->fdc, 50);   /* default is 12 usec if not set */
}

MACHINE_RESET( mbee )
{
	machine_reset_common(machine);
	memory_set_bank(machine, "boot", 1);
	timer_set(machine, ATTOTIME_IN_USEC(4), NULL, 0, mbee_reset);
}

MACHINE_RESET( mbee56 )
{
	machine_reset_common_disk(machine);
	memory_set_bank(machine, "boot", 1);
	timer_set(machine, ATTOTIME_IN_USEC(4), NULL, 0, mbee_reset);
}

MACHINE_RESET( mbee64 )
{
	machine_reset_common_disk(machine);
	memory_set_bank(machine, "boot", 1);
	memory_set_bank(machine, "bankl", 1);
	memory_set_bank(machine, "bankh", 1);
}

MACHINE_RESET( mbee128 )
{
	address_space *mem = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	machine_reset_common_disk(machine);
	mbee128_50_w(mem,0,0); // set banks to default
	memory_set_bank(machine, "boot", 4); // boot time
}

MACHINE_RESET( mbee256 )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	UINT8 i;
	address_space *mem = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	machine_reset_common_disk(machine);
	state->rtc = machine->device<mc146818_device>("rtc");
	for (i = 0; i < 15; i++) state->mbee256_was_pressed[i] = 0;
	state->mbee256_q_pos = 0;
	mbee256_50_w(mem,0,0); // set banks to default
	memory_set_bank(machine, "boot", 8); // boot time
	timer_set(machine, ATTOTIME_IN_USEC(4), NULL, 0, mbee_reset);
}

MACHINE_RESET( mbeett )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	UINT8 i;
	state->rtc = machine->device<mc146818_device>("rtc");
	for (i = 0; i < 15; i++) state->mbee256_was_pressed[i] = 0;
	state->mbee256_q_pos = 0;
	machine_reset_common(machine);
	memory_set_bank(machine, "boot", 1);
	timer_set(machine, ATTOTIME_IN_USEC(4), NULL, 0, mbee_reset);
}

INTERRUPT_GEN( mbee_interrupt )
{
// Due to the uncertainly and hackage here, this is commented out for now - Robbbert - 05-Oct-2010
#if 0
	mbee_state *state = device->machine->driver_data<mbee_state>();

	//address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	/* The printer status connects to the pio ASTB pin, and the printer changing to not
        busy should signal an interrupt routine at B61C, (next line) but this doesn't work.
        The line below does what the interrupt should be doing. */
	/* But it would break any program loaded to that area of memory, such as CP/M programs */

	//z80pio_astb_w( state->z80pio, centronics_busy_r(state->printer));	/* signal int when not busy (L->H) */
	//space->write_byte(0x109, centronics_busy_r(state->printer));


	/* once per frame, pulse the PIO B bit 7 - it is in the schematic as an option,
	but need to find out what it does */
	state->clock_pulse = 0x80;
	irq0_line_hold(device);

#endif
}

DRIVER_INIT( mbee )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0x8000);
	state->size = 0x4000;
}

DRIVER_INIT( mbeeic )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0x8000);

	RAM = memory_region(machine, "pakrom");
	memory_configure_bank(machine, "pak", 0, 16, &RAM[0x0000], 0x2000);

	memory_set_bank(machine, "pak", 0);
	state->size = 0x8000;
}

DRIVER_INIT( mbeepc )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0x8000);

	RAM = memory_region(machine, "telcomrom");
	memory_configure_bank(machine, "telcom", 0, 2, &RAM[0x0000], 0x1000);

	RAM = memory_region(machine, "pakrom");
	memory_configure_bank(machine, "pak", 0, 16, &RAM[0x0000], 0x2000);

	memory_set_bank(machine, "pak", 0);
	memory_set_bank(machine, "telcom", 0);
	state->size = 0x8000;
}

DRIVER_INIT( mbeepc85 )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0x8000);

	RAM = memory_region(machine, "telcomrom");
	memory_configure_bank(machine, "telcom", 0, 2, &RAM[0x0000], 0x1000);

	RAM = memory_region(machine, "pakrom");
	memory_configure_bank(machine, "pak", 0, 16, &RAM[0x0000], 0x2000);

	memory_set_bank(machine, "pak", 5);
	memory_set_bank(machine, "telcom", 0);
	state->size = 0x8000;
}

DRIVER_INIT( mbeeppc )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 1, &RAM[0x0000], 0x0000);

	RAM = memory_region(machine, "basicrom");
	memory_configure_bank(machine, "basic", 0, 2, &RAM[0x0000], 0x2000);
	memory_configure_bank(machine, "boot", 1, 1, &RAM[0x0000], 0x0000);

	RAM = memory_region(machine, "telcomrom");
	memory_configure_bank(machine, "telcom", 0, 2, &RAM[0x0000], 0x1000);

	RAM = memory_region(machine, "pakrom");
	memory_configure_bank(machine, "pak", 0, 16, &RAM[0x0000], 0x2000);

	memory_set_bank(machine, "pak", 5);
	memory_set_bank(machine, "telcom", 0);
	memory_set_bank(machine, "basic", 0);
	state->size = 0x8000;
}

DRIVER_INIT( mbee56 )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0xe000);
	state->size = 0xe000;
}

DRIVER_INIT( mbee64 )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 1, &RAM[0x0000], 0x0000);
	memory_configure_bank(machine, "bankl", 0, 1, &RAM[0x1000], 0x0000);
	memory_configure_bank(machine, "bankl", 1, 1, &RAM[0x9000], 0x0000);
	memory_configure_bank(machine, "bankh", 0, 1, &RAM[0x8000], 0x0000);

	RAM = memory_region(machine, "bootrom");
	memory_configure_bank(machine, "bankh", 1, 1, &RAM[0x0000], 0x0000);
	memory_configure_bank(machine, "boot", 1, 1, &RAM[0x0000], 0x0000);

	state->size = 0xf000;
}

DRIVER_INIT( mbee128 )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 4, &RAM[0x0000], 0x8000); // standard banks 0000
	memory_configure_bank(machine, "bank1", 0, 4, &RAM[0x1000], 0x8000); // standard banks 1000
	memory_configure_bank(machine, "bank8l", 1, 1, &RAM[0x0000], 0x0000); // shadow ram
	memory_configure_bank(machine, "bank8h", 1, 1, &RAM[0x0800], 0x0000); // shadow ram
	memory_configure_bank(machine, "bank9", 1, 1, &RAM[0x1000], 0x0000); // shadow ram
	memory_configure_bank(machine, "bankfl", 0, 1, &RAM[0xf000], 0x0000); // shadow ram
	memory_configure_bank(machine, "bankfh", 0, 1, &RAM[0xf800], 0x0000); // shadow ram

	RAM = memory_region(machine, "bootrom");
	memory_configure_bank(machine, "bank9", 0, 1, &RAM[0x1000], 0x0000); // rom
	memory_configure_bank(machine, "boot", 4, 1, &RAM[0x0000], 0x0000); // rom at boot for 4usec
	memory_configure_bank(machine, "bank8l", 0, 1, &RAM[0x0000], 0x0000); // rom
	memory_configure_bank(machine, "bank8h", 0, 1, &RAM[0x0800], 0x0000); // rom

	state->size = 0x8000;
}

DRIVER_INIT( mbee256 )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 8, &RAM[0x0000], 0x8000); // standard banks 0000
	memory_configure_bank(machine, "bank1", 0, 8, &RAM[0x1000], 0x8000); // standard banks 1000
	memory_configure_bank(machine, "bank8l", 1, 1, &RAM[0x0000], 0x0000); // shadow ram
	memory_configure_bank(machine, "bank8h", 1, 1, &RAM[0x0800], 0x0000); // shadow ram
	memory_configure_bank(machine, "bank9", 1, 1, &RAM[0x1000], 0x0000); // shadow ram
	memory_configure_bank(machine, "bankfl", 0, 1, &RAM[0xf000], 0x0000); // shadow ram
	memory_configure_bank(machine, "bankfh", 0, 1, &RAM[0xf800], 0x0000); // shadow ram

	RAM = memory_region(machine, "bootrom");
	memory_configure_bank(machine, "bank9", 0, 1, &RAM[0x1000], 0x0000); // rom
	memory_configure_bank(machine, "boot", 8, 1, &RAM[0x0000], 0x0000); // rom at boot for 4usec
	memory_configure_bank(machine, "bank8l", 0, 1, &RAM[0x0000], 0x0000); // rom
	memory_configure_bank(machine, "bank8h", 0, 1, &RAM[0x0800], 0x0000); // rom

	timer_pulse(machine, ATTOTIME_IN_HZ(1),NULL,0,mbee_rtc_irq);	/* timer for rtc */
	timer_pulse(machine, ATTOTIME_IN_HZ(25),NULL,0,mbee256_kbd);	/* timer for kbd */

	state->size = 0x8000;
}

DRIVER_INIT( mbeett )
{
	mbee_state *state = machine->driver_data<mbee_state>();
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0x8000);

	RAM = memory_region(machine, "telcomrom");
	memory_configure_bank(machine, "telcom", 0, 2, &RAM[0x0000], 0x1000);

	RAM = memory_region(machine, "pakrom");
	memory_configure_bank(machine, "pak", 0, 16, &RAM[0x0000], 0x2000);

	memory_set_bank(machine, "pak", 5);
	memory_set_bank(machine, "telcom", 0);

	timer_pulse(machine, ATTOTIME_IN_HZ(1),NULL,0,mbee_rtc_irq);	/* timer for rtc */
	timer_pulse(machine, ATTOTIME_IN_HZ(25),NULL,0,mbee256_kbd);	/* timer for kbd */

	state->size = 0x8000;
}


/***********************************************************

    Quickload

    These load the standard BIN format, as well
    as COM and MWB files.

************************************************************/

Z80BIN_EXECUTE( mbee )
{
	running_device *cpu = machine->device("maincpu");
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	space->write_word(0xa6, execute_address);			/* fix the EXEC command */

	if (autorun)
	{
		space->write_word(0xa2, execute_address);		/* fix warm-start vector to get around some copy-protections */
		cpu_set_reg(cpu, STATE_GENPC, execute_address);
	}
	else
	{
		space->write_word(0xa2, 0x8517);
	}
}

QUICKLOAD_LOAD( mbee )
{
	mbee_state *state = image.device().machine->driver_data<mbee_state>();
	running_device *cpu = image.device().machine->device("maincpu");
	address_space *space = cputag_get_address_space(image.device().machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT16 i, j;
	UINT8 data, sw = input_port_read(image.device().machine, "CONFIG") & 1;	/* reading the dipswitch: 1 = autorun */

	if (!mame_stricmp(image.filetype(), "mwb"))
	{
		/* mwb files - standard basic files */
		for (i = 0; i < quickload_size; i++)
		{
			j = 0x8c0 + i;

			if (image.fread(&data, 1) != 1)
			{
				image.message("Unexpected EOF");
				return IMAGE_INIT_FAIL;
			}

			if ((j < state->size) || (j > 0xefff))
				space->write_byte(j, data);
			else
			{
				image.message("Not enough memory in this microbee");
				return IMAGE_INIT_FAIL;
			}
		}

		if (sw)
		{
			space->write_word(0xa2,0x801e);	/* fix warm-start vector to get around some copy-protections */
			cpu_set_reg(cpu, STATE_GENPC, 0x801e);
		}
		else
			space->write_word(0xa2,0x8517);
	}
	else if (!mame_stricmp(image.filetype(), "com"))
	{
		/* com files - most com files are just machine-language games with a wrapper and don't need cp/m to be present */
		for (i = 0; i < quickload_size; i++)
		{
			j = 0x100 + i;

			if (image.fread(&data, 1) != 1)
			{
				image.message("Unexpected EOF");
				return IMAGE_INIT_FAIL;
			}

			if ((j < state->size) || (j > 0xefff))
				space->write_byte(j, data);
			else
			{
				image.message("Not enough memory in this microbee");
				return IMAGE_INIT_FAIL;
			}
		}

		if (sw) cpu_set_reg(cpu, STATE_GENPC, 0x100);
	}

	return IMAGE_INIT_PASS;
}
