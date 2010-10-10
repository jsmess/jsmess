/***************************************************************************

    microbee.c

    machine driver
    Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000


****************************************************************************/

#include "emu.h"
#include "machine/mc146818.h"
#include "devices/flopdrv.h"
#include "includes/mbee.h"

static size_t mbee_size;
static UINT8 mbee_clock_pulse;
static UINT8 fdc_status = 0;
static running_device *mbee_fdc;
static mc146818_device *mbee_rtc;
static running_device *mbee_z80pio;
static running_device *mbee_speaker;
static running_device *mbee_cassette;
static running_device *mbee_printer;

/***********************************************************

    PIO

************************************************************/

static void mbee_pio_interrupt(running_device *device, int state)
{
	cputag_set_input_line(device->machine, "maincpu", 0, state );
}

static WRITE8_DEVICE_HANDLER( pio_ardy )
{
	/* devices need to be redeclared in this callback for some strange reason */
	mbee_printer = device->machine->device("centronics");
	centronics_strobe_w(mbee_printer, (data) ? 0 : 1);
}

static WRITE8_DEVICE_HANDLER( pio_port_a_w )
{
	/* hardware strobe driven by PIO ARDY, bit 7..0 = data */
	z80pio_astb_w( mbee_z80pio, 1);	/* needed - otherwise nothing prints */
	centronics_data_w(mbee_printer, 0, data);
};

static WRITE8_DEVICE_HANDLER( pio_port_b_w )
{
/*  PIO port B - d5..d2 not emulated
    d7 network interrupt (microbee network for classrooms)
    d6 speaker
    d5 rs232 output (1=mark)
    d4 rs232 input (0=mark)
    d3 rs232 CTS (0=clear to send)
    d2 rs232 clock or DTR
    d1 cass out and (on 256tc) keyboard irq
    d0 cass in */

	cassette_output(mbee_cassette, (data & 0x02) ? -1.0 : +1.0);

	speaker_level_w(mbee_speaker, (data & 0x40) ? 1 : 0);
};

static READ8_DEVICE_HANDLER( pio_port_b_r )
{
	UINT8 data = z80pio_pb_r(mbee_z80pio,0) & 0x7e;

	if (cassette_input(mbee_cassette) > 0.03)
		data |= 0x01;

	data |= mbee_clock_pulse;

	mbee_clock_pulse = 0;

	return data;
};

const z80pio_interface mbee_z80pio_intf =
{
	DEVCB_LINE(mbee_pio_interrupt),	/* callback when change interrupt status */
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
	if (state)
		fdc_status |= 0x80;
	else
		fdc_status &= 0x7f;
}

static WRITE_LINE_DEVICE_HANDLER( mbee_fdc_drq_w )
{
	if (state)
		fdc_status |= 0x80;
	else
		fdc_status &= 0x7f;
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
/*  d7 indicate if IRQ or DRQ is occuring (1=happening)
    d6..d0 not used */

	return fdc_status;
}

WRITE8_HANDLER ( mbee_fdc_motor_w )
{
/*  d7..d4 not used
    d3 density (1=MFM)
    d2 side (1=side 1)
    d1..d0 drive select (0 to 3) */

	wd17xx_set_drive(mbee_fdc, data & 3);
	wd17xx_set_side(mbee_fdc, (data & 4) ? 1 : 0);
	wd17xx_dden_w(mbee_fdc, !BIT(data, 3));
}

/***********************************************************

    Keyboard of the 256TC

************************************************************/

static UINT8 mbee256_was_pressed[15] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
static UINT8 mbee256_q[20];
static UINT8 mbee256_q_pos = 0;

static TIMER_CALLBACK( mbee256_kbd )
{
    /* Keyboard scanner is a '3870' chip. It is not clocked, but is triggered by each read of port 18.
    When a key is detected, it sets bit 1 of port 2 (one of the pio input lines). The next read of
    port 18 will clear this line. The 3870 can store up to 9 keys, and has separate scan codes for
    keys being pressed and being released. With no data sheet available, the following is a guess. */

	UINT8 i, j, scancode;
	UINT8 pressed[15] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
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
		if (pressed[i] != mbee256_was_pressed[i])
		{
			/* get scankey value */
			for (j = 0; j < 8; j++)
			{
				if (BIT(pressed[i]^mbee256_was_pressed[i], j))
				{
					scancode = (i << 3) | j | (BIT(pressed[i], j) ? 0x80 : 0);
					/* put it in the queue */
					mbee256_q[mbee256_q_pos] = scancode;
					if (mbee256_q_pos < 20) mbee256_q_pos++;
				}
			}
		}
		mbee256_was_pressed[i] = pressed[i];
	}

    /* if anything queued, cause an interrupt */
	if (mbee256_q_pos)
	{
		i = z80pio_pb_r(mbee_z80pio,0);
		z80pio_pb_w(mbee_z80pio, 0, i | 2);
	}
}

READ8_HANDLER( mbee256_18_r )
{
	UINT8 i, ret = 0;
	if (mbee256_q_pos)
	{
		mbee256_q_pos--;
		ret = mbee256_q[0]; // get oldest key
		for (i = 0; i < mbee256_q_pos; i++) mbee256_q[i] = mbee256_q[i+1]; // ripple queue
	}

	z80pio_pb_w(mbee_z80pio, 0, z80pio_pb_r(mbee_z80pio,0) & 0xfd); // clear irq

    /* time delay of 0.01uf cap and 1.5k resistor */
	timer_set(space->machine, ATTOTIME_IN_USEC(15), NULL, 0, mbee256_kbd);

    /* get next char and return it */
	
	return ret;
}


/***********************************************************

    Real Time Clock option

************************************************************/

WRITE8_HANDLER( mbee_04_w )	// address
{
	mbee_rtc->write(*space, 0, data);
}

WRITE8_HANDLER( mbee_06_w )	// write
{
	mbee_rtc->write(*space, 1, data);
}

READ8_HANDLER( mbee_07_r )	// read
{
	return mbee_rtc->read(*space, 1);
}

static TIMER_CALLBACK( mbee_rtc_irq )
{
	address_space *mem = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	mc146818_device *rtc = machine->device<mc146818_device>("rtc");
	UINT8 data = rtc->read(*mem, 12);
	if (data) mbee_clock_pulse = 0x80;
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

MACHINE_RESET( mbee )
{
	timer_set(machine, ATTOTIME_IN_USEC(4), NULL, 0, mbee_reset);
	memory_set_bank(machine, "boot", 1);
	mbee_z80pio = machine->device("z80pio");
	mbee_speaker = machine->device("speaker");
	mbee_cassette = machine->device("cassette");
	mbee_printer = machine->device("centronics");
	mbee_fdc = machine->device("wd179x");
	//mbee_rtc = machine->device<mc146818_device>("rtc");
	//wd17xx_set_pause_time(mbee_fdc, 45);       /* default is 40 usec if not set */
	//wd17xx_set_complete_command_delay(mbee_fdc, 50);   /* default is 12 usec if not set */
}

MACHINE_RESET( mbee64 )
{
	memory_set_bank(machine, "boot", 1);
	memory_set_bank(machine, "bankl", 1);
	memory_set_bank(machine, "bankh", 1);
	mbee_z80pio = machine->device("z80pio");
	mbee_speaker = machine->device("speaker");
	mbee_cassette = machine->device("cassette");
	mbee_printer = machine->device("centronics");
	mbee_fdc = machine->device("wd179x");
	//mbee_rtc = machine->device<mc146818_device>("rtc");
}

MACHINE_RESET( mbee256 )
{
	UINT8 i;
	timer_set(machine, ATTOTIME_IN_USEC(4), NULL, 0, mbee_reset);
	memory_set_bank(machine, "boot", 1);
	memory_set_bank(machine, "bankl", 1);
	memory_set_bank(machine, "bankh", 1);
	mbee_z80pio = machine->device("z80pio");
	mbee_speaker = machine->device("speaker");
	mbee_cassette = machine->device("cassette");
	mbee_printer = machine->device("centronics");
	mbee_fdc = machine->device("wd179x");
	mbee_rtc = machine->device<mc146818_device>("rtc");
	for (i = 0; i < 15; i++) mbee256_was_pressed[i] = 0;
	mbee256_q_pos = 0;
}

INTERRUPT_GEN( mbee_interrupt )
{
// Due to the uncertainly and hackage here, this is commented out for now - Robbbert - 05-Oct-2010
#if 0

	address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	/* once per frame, pulse the PIO B bit 7 - it is in the schematic as an option,
	but need to find out what it does */
	mbee_clock_pulse = 0x80;

	/* The printer status connects to the pio ASTB pin, and the printer changing to not
        busy should signal an interrupt routine at B61C, (next line) but this doesn't work.
        The line below does what the interrupt should be doing. */
	/* But it would break any program loaded to that area of memory, such as CP/M programs */

	z80pio_astb_w( mbee_z80pio, centronics_busy_r(mbee_printer));	/* signal int when not busy (L->H) */

	space->write_byte(0x109, centronics_busy_r(mbee_printer));
#endif
}

DRIVER_INIT( mbee )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000],  0x8000);
	mbee_size = 0x4000;
}

DRIVER_INIT( mbeeic )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000],  0x8000);

	RAM = memory_region(machine, "pakrom");
	memory_configure_bank(machine, "pak", 0, 16, &RAM[0x0000], 0x2000);

	memory_set_bank(machine, "pak", 0);
	mbee_size = 0x8000;

	timer_pulse(machine, ATTOTIME_IN_HZ(1),NULL,0,mbee_rtc_irq);	/* timer for rtc */
}

DRIVER_INIT( mbeepc )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000],  0x8000);

	RAM = memory_region(machine, "telcomrom");
	memory_configure_bank(machine, "telcom", 0, 2, &RAM[0x0000], 0x1000);

	RAM = memory_region(machine, "pakrom");
	memory_configure_bank(machine, "pak", 0, 16, &RAM[0x0000], 0x2000);

	memory_set_bank(machine, "pak", 0);
	memory_set_bank(machine, "telcom", 0);
	mbee_size = 0x8000;

	timer_pulse(machine, ATTOTIME_IN_HZ(1),NULL,0,mbee_rtc_irq);	/* timer for rtc */
}

DRIVER_INIT( mbeepc85 )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000],  0x8000);

	RAM = memory_region(machine, "telcomrom");
	memory_configure_bank(machine, "telcom", 0, 2, &RAM[0x0000], 0x1000);

	RAM = memory_region(machine, "pakrom");
	memory_configure_bank(machine, "pak", 0, 16, &RAM[0x0000], 0x2000);

	memory_set_bank(machine, "pak", 5);
	memory_set_bank(machine, "telcom", 0);
	mbee_size = 0x8000;

	timer_pulse(machine, ATTOTIME_IN_HZ(1),NULL,0,mbee_rtc_irq);	/* timer for rtc */
}

DRIVER_INIT( mbeeppc )
{
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
	mbee_size = 0x8000;

	timer_pulse(machine, ATTOTIME_IN_HZ(1),NULL,0,mbee_rtc_irq);	/* timer for rtc */
}

DRIVER_INIT( mbee56 )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000],  0xe000);
	mbee_size = 0xe000;

	timer_pulse(machine, ATTOTIME_IN_HZ(1),NULL,0,mbee_rtc_irq);	/* timer for rtc */
}

DRIVER_INIT( mbee64 )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, "boot", 0, 1, &RAM[0x0000],  0x0000);
	memory_configure_bank(machine, "bankl", 0, 1, &RAM[0x1000],  0x0000);
	memory_configure_bank(machine, "bankl", 1, 1, &RAM[0x9000],  0x0000);
	memory_configure_bank(machine, "bankh", 0, 1, &RAM[0x8000],  0x0000);

	RAM = memory_region(machine, "bootrom");
	memory_configure_bank(machine, "bankh", 1, 1, &RAM[0x0000], 0x0000);
	memory_configure_bank(machine, "boot", 1, 1, &RAM[0x0000], 0x0000);

	mbee_size = 0xf000;

	timer_pulse(machine, ATTOTIME_IN_HZ(1),NULL,0,mbee_rtc_irq);	/* timer for rtc */
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

			if ((j < mbee_size) || (j > 0xefff))
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

			if ((j < mbee_size) || (j > 0xefff))
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
