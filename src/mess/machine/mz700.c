/******************************************************************************
 *	Sharp MZ700
 *
 *	machine driver
 *
 *	Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Reference: http://sharpmz.computingmuseum.com
 *
 *****************************************************************************/

/* Core includes */
#include "driver.h"

/* Components */
#include "cpu/z80/z80.h"
#include "machine/pit8253.h"
#include "machine/8255ppi.h"
#include "sound/speaker.h"
#include "machine/74145.h"

#include "includes/mz700.h"

/* Devices */
#include "devices/cassette.h"


#ifndef VERBOSE
#define VERBOSE 0
#endif

#define LOG(N,M,A,mac)	\
	do { \
		if(VERBOSE>=N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s",attotime_to_double(timer_get_time(mac)), (const char*)M ); \
			logerror A; \
		} \
	} while (0)


static READ8_DEVICE_HANDLER ( pio_port_b_r );
static READ8_DEVICE_HANDLER ( pio_port_c_r );
static WRITE8_DEVICE_HANDLER ( pio_port_a_w );
static WRITE8_DEVICE_HANDLER ( pio_port_c_w );

const ppi8255_interface mz700_ppi8255_interface =
{
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER(TTL74145, "ls145", pio_port_b_r),
	DEVCB_HANDLER(pio_port_c_r),
	DEVCB_DEVICE_HANDLER(TTL74145, "ls145", pio_port_a_w),
	DEVCB_NULL,
	DEVCB_HANDLER(pio_port_c_w)
};


static int pio_port_c_output;

static PIT8253_OUTPUT_CHANGED( pit_out0_changed );
static PIT8253_OUTPUT_CHANGED( pit_out1_changed );
static PIT8253_OUTPUT_CHANGED( pit_irq_2 );

const struct pit8253_config mz700_pit8253_config =
{
	{
		/* clockin	  irq callback	  */
		{ 1108800.0,  pit_out0_changed },
		{	15611.0,  pit_out1_changed },
		{		  0,  pit_irq_2        },
	}
};


DRIVER_INIT( mz700 )
{
	videoram_size = 0x5000;
	videoram = auto_malloc(videoram_size);
	colorram = auto_malloc(0x800);

	mz700_bank_w(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 4, 0);
}


/************************ PIT ************************************************/

/* Timer 0 is the clock for the speaker output */
static PIT8253_OUTPUT_CHANGED( pit_out0_changed )
{
	const device_config *speaker = devtag_get_device(device->machine, SOUND, "speaker");
	speaker_level_w( speaker, state ? 1 : 0 );
}


/* Timer 1 is the clock for timer 2 clock input */
static PIT8253_OUTPUT_CHANGED( pit_out1_changed )
{
	pit8253_set_clock_signal( device, 2, state );
}


/* timer 2 is the AM/PM (12 hour) interrupt */
static PIT8253_OUTPUT_CHANGED( pit_irq_2 )
{
	/* INTMSK: interrupt enabled? */
    if (pio_port_c_output & 0x04)
		cpu_set_input_line(device->machine->cpu[0], 0, HOLD_LINE);
}

/************************ PIO ************************************************/

static READ8_DEVICE_HANDLER( pio_port_b_r )
{
	int key_line = ttl74145_r(device, 0, 0);

	switch (key_line)
	{
	case 1 << 0: return input_port_read(device->machine, "ROW0");
	case 1 << 1: return input_port_read(device->machine, "ROW1");
	case 1 << 2: return input_port_read(device->machine, "ROW2");
	case 1 << 3: return input_port_read(device->machine, "ROW3");
	case 1 << 4: return input_port_read(device->machine, "ROW4");
	case 1 << 5: return input_port_read(device->machine, "ROW5");
	case 1 << 6: return input_port_read(device->machine, "ROW6");
	case 1 << 7: return input_port_read(device->machine, "ROW7");
	case 1 << 8: return input_port_read(device->machine, "ROW8");
	case 1 << 9: return input_port_read(device->machine, "ROW9");
	}

	/* should never reach this */
    return 0xff;
}

/*
 * bit 7 in     vertical blank
 * bit 6 in     NE556 output
 * bit 5 in     tape data (RDATA)
 * bit 4 in     motor (1 = on)
 */
static READ8_DEVICE_HANDLER( pio_port_c_r )
{
	const device_config *cas = devtag_get_device(device->machine, CASSETTE, "cassette");
	mz700_state *mz700 = device->machine->driver_data;
	UINT8 data = pio_port_c_output & 0x0f;

	/* note: this is actually connected to Q output of the motor-control flip-flop (see below) */
	if ((cassette_get_state(cas) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
		data |= 0x10;

	if (cassette_input(cas) > 0.0038)
		data |= 0x20;       /* set the RDATA status */

	data |= mz700->cursor_timer << 6;
	data |= video_screen_get_vblank(device->machine->primary_screen) << 7;

	LOG(2,"mz700_pio_port_c_r",("%02X\n", data),device->machine);

	return data;
}


static WRITE8_DEVICE_HANDLER( pio_port_a_w )
{
	const device_config *timer = devtag_get_device(device->machine, TIMER, "cursor");

	LOG(2,"mz700_pio_port_a_w",("%02X\n", data),device->machine);

	/* the ls145 is connected to PA0-PA3 */
	ttl74145_w(device, 0, data & 0x07);

	/* ne556 reset is connected to PA7 */
	timer_device_enable(timer, BIT(data, 7));
}


static WRITE8_DEVICE_HANDLER( pio_port_c_w )
{
    /*
     * bit 3 out    motor control (0 = on)
     * bit 2 out    INTMSK
     * bit 1 out    tape data (WDATA)
     * bit 0 out    unused
     */

//	UINT8 state = cassette_get_state(devtag_get_device(device->machine, CASSETTE, "cassette"));
//	UINT8 action = ((~pio_port_c_output & 8) & (data & 8));		/* detect low-to-high transition */

	/* The motor control circuit consists of a resistor, capacitor, invertor, nand-gate, and D flip-flop.
		The sense input from the cassette player goes low whenever play, rewind or fast-forward is pressed.
		This connects to most of the above components.
		The Q output enables the motor, and also connects to Bit 4 (input).
		Bit 3 outputs a string of pulses to the Clock pin, and therefore cannot be used to control
		the motor directly.
		For the moment, the user can use the UI to select play, stop, etc.
		If you load from the command-line or the software-picker, type in L <enter> immediately.

	cassette_change_state(
		devtag_get_device(device->machine, CASSETTE, "cassette"),
		((data & 0x08) && mz700_motor_on) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
		CASSETTE_MOTOR_DISABLED);

	*/

	LOG(2,"mz700_pio_port_c_w",("%02X\n", data),device->machine);

	cassette_output(devtag_get_device(device->machine, CASSETTE, "cassette"), (data & 0x02) ? +1.0 : -1.0);

	pio_port_c_output = data;
}

/************************ MMIO ***********************************************/

READ8_HANDLER( mz700_e008_r )
{
	mz700_state *mz700 = space->machine->driver_data;
	UINT8 data = 0;

	data |= mz700->other_timer;
	data |= input_port_read(space->machine, "JOY");	/* get joystick ports */
	data |= video_screen_get_hblank(space->machine->primary_screen);

	LOG(1, "mz700_e008_r", ("%02X\n", data), space->machine);

	return data;
}


/************************ BANK ***********************************************/

/* BANK1 0000-0FFF */
static void bank1_RAM(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 1, &mem[0x00000]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0000, 0x0fff, 0, 0, SMH_BANK1);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0000, 0x0fff, 0, 0, SMH_BANK1);
}

static void bank1_ROM(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 1, &mem[0x10000]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0000, 0x0fff, 0, 0, SMH_BANK1);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0000, 0x0fff, 0, 0, SMH_UNMAP);
}


/* BANK2 1000-1FFF */
static void bank2_RAM(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 2, &mem[0x01000]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x1000, 0x1fff, 0, 0, SMH_BANK2);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x1000, 0x1fff, 0, 0, SMH_BANK2);
}

static void bank2_ROM(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 2, &mem[0x11000]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x1000, 0x1fff, 0, 0, SMH_BANK2);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x1000, 0x1fff, 0, 0, SMH_UNMAP);
}


/* BANK3 8000-9FFF */
static void bank3_RAM(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 3, &mem[0x08000]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x8000, 0x9fff, 0, 0, SMH_BANK3);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x8000, 0x9fff, 0, 0, SMH_BANK3);
}

static void bank3_VID(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 3, videoram);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x8000, 0x9fff, 0, 0, SMH_BANK3);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x8000, 0x9fff, 0, 0, SMH_BANK3);
}


/* BANK4 A000-BFFF */
static void bank4_RAM(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 4, &mem[0x0a000]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xA000, 0xBFFF, 0, 0, SMH_BANK4);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xA000, 0xBFFF, 0, 0, SMH_BANK4);
}

static void bank4_VID(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 4, videoram + 0x2000);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xA000, 0xBFFF, 0, 0, SMH_BANK4);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xA000, 0xBFFF, 0, 0, SMH_BANK4);
}


/* BANK7 C000-CFFF */
static void bank5_RAM(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 5, &mem[0x0c000]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xC000, 0xCFFF, 0, 0, SMH_BANK5);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xC000, 0xCFFF, 0, 0, SMH_BANK5);
}


/* BANK6 D000-D7FF */
static void bank6_NOP(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 6, &mem[0x0d000]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xD000, 0xD7FF, 0, 0, SMH_NOP);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xD000, 0xD7FF, 0, 0, SMH_NOP);
}

static void bank6_RAM(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 6, &mem[0x0d000]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xD000, 0xD7FF, 0, 0, SMH_BANK6);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xD000, 0xD7FF, 0, 0, SMH_BANK6);
}

static void bank6_VIO(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 6, videoram);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xD000, 0xD7FF, 0, 0, SMH_BANK6);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xD000, 0xD7FF, 0, 0, SMH_BANK6);
}


/* BANK9 D800-DFFF */
static void bank7_NOP(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 7, &mem[0x0d800]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xD800, 0xDFFF, 0, 0, SMH_NOP);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xD800, 0xDFFF, 0, 0, SMH_NOP);
}

static void bank7_RAM(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 7, &mem[0x0d800]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xD800, 0xDFFF, 0, 0, SMH_BANK7);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xD800, 0xDFFF, 0, 0, SMH_BANK7);
}

static void bank7_VIO(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 7, colorram);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xD800, 0xDFFF, 0, 0, SMH_BANK7);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xD800, 0xDFFF, 0, 0, SMH_BANK7);
}


/* BANK8 E000-FFFF */
static void bank8_NOP(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 8, &mem[0x0e000]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xE000, 0xFFFF, 0, 0, SMH_NOP);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xE000, 0xFFFF, 0, 0, SMH_NOP);

}

static void bank8_RAM(running_machine *machine, UINT8 *mem)
{
	memory_set_bankptr(machine, 8, &mem[0x0e000]);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xE000, 0xFFFF, 0, 0, SMH_BANK8);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xE000, 0xFFFF, 0, 0, SMH_BANK8);
}

static void bank8_VIO(const address_space *space, UINT8 *mem)
{
	const device_config *pit = devtag_get_device(space->machine, PIT8253, "pit8253");
	const device_config *ppi = devtag_get_device(space->machine, PPI8255, "ppi8255");

	memory_set_bankptr(space->machine, 8, &mem[0x16000]);
	memory_install_readwrite8_device_handler(space, ppi, 0xe000, 0xfff3, 0, 0x1ff0, ppi8255_r, ppi8255_w);
	memory_install_readwrite8_device_handler(space, pit, 0xe004, 0xfff7, 0, 0x1ff0, pit8253_r, pit8253_w);
	memory_install_read8_handler(space, 0xe008, 0xfff8, 0, 0x1ff0, mz700_e008_r);
	memory_install_write8_device_handler(space, pit, 0xe008, 0xfff8, 0, 0x1ff0, pit8253_gate_w);
}


WRITE8_HANDLER ( mz700_bank_w )
{
    static int mz700_locked = 0;
	static int vio_mode = 0;
	static int vio_lock = 0;
	UINT8 *mem = memory_region(space->machine, "maincpu");

    switch (offset)
	{
	case 0: /* 0000-0FFF RAM */
		LOG(1,"mz700_bank_w",("0: 0000-0FFF RAM\n"),space->machine);
		bank1_RAM(space->machine, mem);
		mz700_locked = 0;
        break;

	case 1: /* D000-FFFF RAM */
		LOG(1,"mz700_bank_w",("1: D000-FFFF RAM\n"),space->machine);
		bank6_RAM(space->machine, mem);
		bank7_RAM(space->machine, mem);
		bank8_RAM(space->machine, mem);
        mz700_locked = 0;
		vio_mode = 1;
        break;

	case 2: /* 0000-0FFF ROM */
		LOG(1,"mz700_bank_w",("2: 0000-0FFF ROM\n"),space->machine);
		bank1_ROM(space->machine, mem);
		mz700_locked = 0;
        break;

	case 3: /* D000-FFFF videoram, memory mapped io */
		LOG(1,"mz700_bank_w",("3: D000-FFFF videoram, memory mapped io\n"),space->machine);
		bank6_VIO(space->machine, mem);
		bank7_VIO(space->machine, mem);
		bank8_VIO(space, mem);
		mz700_locked = 0;
		vio_mode = 3;
        break;

	case 4: /* 0000-0FFF ROM	D000-FFFF videoram, memory mapped io */
		LOG(1,"mz700_bank_w",("4: 0000-0FFF ROM; D000-FFFF videoram, memory mapped io\n"),space->machine);
		bank1_ROM(space->machine, mem);
		bank6_VIO(space->machine, mem);
		bank7_VIO(space->machine, mem);
		bank8_VIO(space, mem);
        mz700_locked = 0;
		vio_mode = 3;
        break;

	case 5: /* 0000-0FFF no chg D000-FFFF locked */
		LOG(1,"mz700_bank_w",("5: D000-FFFF locked\n"),space->machine);
		if (mz700_locked == 0)
		{
			vio_lock = vio_mode;
			mz700_locked = 1;
			bank6_NOP(space->machine, mem);
			bank7_NOP(space->machine, mem);
			bank8_NOP(space->machine, mem);
		}
        break;

	case 6: /* 0000-0FFF no chg D000-FFFF unlocked */
		LOG(1,"mz700_bank_w",("6: D000-FFFF unlocked\n"),space->machine);
		if (mz700_locked == 1)
			mz700_bank_w(space, vio_lock, 0); /* old config for D000-DFFF */
        break;
    }
}

/******************************************************************************
 *	Sharp MZ800
 *
 *
 ******************************************************************************/

static UINT16 mz800_ramaddr = 0;
static UINT8 mz800_display_mode = 0;
static UINT8 mz800_port_e8 = 0;
static UINT8 mz800_palette[4];
static UINT8 mz800_palette_bank;

/* port CE */
READ8_HANDLER( mz800_crtc_r )
{
	UINT8 data = 0x00;
	LOG(1,"mz800_crtc_r",("%02X\n",data),space->machine);
    return data;
}

/* port E0 - E9 */
READ8_HANDLER( mz800_bank_r )
{
	UINT8 *mem = memory_region(space->machine, "maincpu");
    UINT8 data = 0xff;

    switch (offset)
    {
	case 0: /* 1000-1FFF PCG ROM */
		LOG(1,"mz800_bank_r",("0: 1000-1FFF PCG ROM"),space->machine);
        bank2_ROM(space->machine, mem);
		if ((mz800_display_mode & 0x08) == 0)
		{
			if (VERBOSE>=1) logerror("; 8000-9FFF videoram");
            bank3_VID(space->machine, mem);
			if (mz800_display_mode & 0x04)
			{
				if (VERBOSE>=1) logerror("; A000-BFFF videoram");
                /* 640x480 mode so A000-BFFF is videoram too */
				bank4_VID(space->machine, mem);
            }
			else
			{
				if (VERBOSE>=1) logerror("; A000-BFFF RAM");
				bank4_RAM(space->machine, mem);
            }
		}
		else
		{
			if (VERBOSE>=1) logerror("; C000-CFFF PCG RAM");
            /* make C000-CFFF PCG RAM */
			bank5_RAM(space->machine, mem);
        }
		if (VERBOSE>=1) logerror("\n");
        break;

    case 1: /* make 1000-1FFF and C000-CFFF RAM */
		LOG(1,"mz800_bank_r",("1: 1000-1FFF RAM"),space->machine);
        bank2_RAM(space->machine, mem);
		if ((mz800_display_mode & 0x08) == 0)
		{
			if (VERBOSE>=1) logerror("; 8000-9FFF RAM; A000-BFFF RAM");
            /* make 8000-BFFF RAM */
            bank3_RAM(space->machine, mem);
			bank4_RAM(space->machine, mem);
		}
		else
		{
			if (VERBOSE>=1) logerror("; C000-CFFF RAM");
            /* make C000-CFFF RAM */
			bank5_RAM(space->machine, mem);
        }
		if (VERBOSE>=1) logerror("\n");
        break;

    case 8: /* get MZ700 enable bit 7 ? */
		data = mz800_port_e8;
		break;
    }
	return data;
}

/* port EA */
 READ8_HANDLER( mz800_ramdisk_r )
{
	UINT8 *mem = memory_region(space->machine, "user1");
	UINT8 data = mem[mz800_ramaddr];
	LOG(2,"mz800_ramdisk_r",("[%04X] -> %02X\n", mz800_ramaddr, data),space->machine);
	if (mz800_ramaddr++ == 0)
		LOG(1,"mz800_ramdisk_r",("address wrap 0000\n"),space->machine);
    return data;
}

/* port CC */
WRITE8_HANDLER( mz800_write_format_w )
{
	LOG(1,"mz800_write_format_w",("%02X\n", data),space->machine);
}

/* port CD */
WRITE8_HANDLER( mz800_read_format_w )
{
	LOG(1,"mz800_read_format_w",("%02X\n", data),space->machine);
}

/* port CE
 * bit 3	1: MZ700 mode		0: MZ800 mode
 * bit 2	1: 640 horizontal	0: 320 horizontal
 * bit 1	1: 4bpp/2bpp		0: 2bpp/1bpp
 * bit 0	???
 */
WRITE8_HANDLER( mz800_display_mode_w )
{
	UINT8 *mem = memory_region(space->machine, "maincpu");
	LOG(1,"mz800_display_mode_w",("%02X\n", data),space->machine);
    mz800_display_mode = data;
	if ((mz800_display_mode & 0x08) == 0)
	{
		bank8_RAM(space->machine, mem);
	}
}

/* port CF */
WRITE8_HANDLER( mz800_scroll_border_w )
{
	LOG(1,"mz800_scroll_border_w",("%02X\n", data),space->machine);
}

/* port E0-E9 */
WRITE8_HANDLER ( mz800_bank_w )
{
    static int mz800_locked = 0;
    static int vio_mode = 0;
    static int vio_lock = 0;
    UINT8 *mem = memory_region(space->machine, "maincpu");

    switch (offset)
    {
    case 0: /* 0000-0FFF RAM */
		LOG(1,"mz800_bank_w",("0: 0000-0FFF RAM\n"),space->machine);
        bank1_RAM(space->machine, mem);
        mz800_locked = 0;
        break;

    case 1: /* D000-FFFF RAM */
		LOG(1,"mz800_bank_w",("1: D000-FFFF RAM\n"),space->machine);
		bank6_RAM(space->machine, mem);
		bank7_RAM(space->machine, mem);
		bank8_RAM(space->machine, mem);
        mz800_locked = 0;
        vio_mode = 1;
        break;

    case 2: /* 0000-0FFF ROM */
		LOG(1,"mz800_bank_w",("2: 0000-0FFF ROM\n"),space->machine);
        bank1_ROM(space->machine, mem);
        mz800_locked = 0;
        break;

    case 3: /* D000-FFFF videoram, memory mapped io */
		LOG(1,"mz800_bank_w",("3: D000-FFFF videoram, memory mapped io\n"),space->machine);
		bank6_VIO(space->machine, mem);
		bank7_VIO(space->machine, mem);
		bank8_VIO(space, mem);
        mz800_locked = 0;
        vio_mode = 3;
        break;

    case 4: /* 0000-0FFF ROM    D000-FFFF videoram, memory mapped io */
		LOG(1,"mz800_bank_w",("4: 0000-0FFF ROM; D000-FFFF videoram, memory mapped io\n"),space->machine);
        bank1_ROM(space->machine, mem);
		bank6_VIO(space->machine, mem);
		bank7_VIO(space->machine, mem);
		bank8_VIO(space, mem);
        mz800_locked = 0;
        vio_mode = 3;
        break;

    case 5: /* 0000-0FFF no chg D000-FFFF locked */
		LOG(1,"mz800_bank_w",("5: D000-FFFF locked\n"),space->machine);
        if (mz800_locked == 0)
        {
            vio_lock = vio_mode;
            mz800_locked = 1;
			bank6_NOP(space->machine, mem);
			bank7_NOP(space->machine, mem);
			bank8_NOP(space->machine, mem);
        }
        break;

    case 6: /* 0000-0FFF no chg D000-FFFF unlocked */
		LOG(1,"mz800_bank_w",("6: D000-FFFF unlocked\n"),space->machine);
        if (mz800_locked == 1)
            mz800_bank_w(space, vio_lock, 0); /* old config for D000-DFFF */
        break;

	case 8: /* set MZ700 enable bit 7 ? */
		mz800_port_e8 = data;
		if (mz800_port_e8 & 0x80)
		{
			bank6_VIO(space->machine, mem);
			bank7_VIO(space->machine, mem);
			bank8_VIO(space, mem);
		}
		else
		{
			bank6_RAM(space->machine, mem);
			bank7_RAM(space->machine, mem);
			bank8_RAM(space->machine, mem);
        }
        break;
    }
}

/* port EA */
WRITE8_HANDLER( mz800_ramdisk_w )
{
	UINT8 *mem = memory_region(space->machine, "user1");
	LOG(2,"mz800_ramdisk_w",("[%04X] <- %02X\n", mz800_ramaddr, data),space->machine);
	mem[mz800_ramaddr] = data;
	if (mz800_ramaddr++ == 0)
		LOG(1,"mz800_ramdisk_w",("address wrap 0000\n"),space->machine);
}

/* port EB */
WRITE8_HANDLER( mz800_ramaddr_w )
{
	mz800_ramaddr = (cpu_get_reg(space->machine->cpu[0], Z80_BC) & 0xff00) | (data & 0xff);
	LOG(1,"mz800_ramaddr_w",("%04X\n", mz800_ramaddr),space->machine);
}

/* port F0 */
WRITE8_HANDLER( mz800_palette_w )
{
	if (data & 0x40)
	{
        mz800_palette_bank = data & 3;
		LOG(1,"mz800_palette_w",("bank: %d\n", mz800_palette_bank),space->machine);
    }
	else
	{
		int idx = (data >> 4) & 3;
		int val = data & 15;
		LOG(1,"mz800_palette_w",("palette[%d] <- %d\n", idx, val),space->machine);
		mz800_palette[idx] = val;
	}
}


DRIVER_INIT( mz800 )
{
	UINT8 *mem = memory_region(machine, "maincpu");
	const address_space *space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);

	videoram_size = 0x5000;
	videoram = auto_malloc(videoram_size);
	colorram = auto_malloc(0x800);

    mem[0x10001] = 0x4a;
	mem[0x10002] = 0x00;
    memcpy(&mem[0x00000], &mem[0x10000], 0x02000);

    mem = memory_region(machine, "user1");
	memset(&mem[0x00000], 0xff, 0x10000);

    mz800_display_mode_w(space, 0, 0x08);   /* set MZ700 mode */
	mz800_bank_r(space, 1);
	mz800_bank_w(space, 4, 0);
}
