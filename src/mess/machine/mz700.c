/******************************************************************************
 *  Sharp MZ700
 *
 *  machine driver
 *
 *  Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Reference: http://sharpmz.computingmuseum.com
 *
 *****************************************************************************/

#include "emu.h"
#include "includes/mz700.h"
#include "cpu/z80/z80.h"
#include "machine/pit8253.h"
#include "machine/i8255a.h"
#include "machine/z80pio.h"
#include "machine/74145.h"
#include "machine/ctronics.h"
#include "sound/speaker.h"
#include "imagedev/cassette.h"
#include "machine/ram.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#define LOG(N,M,A,mac)	\
	do { \
		if(VERBOSE>=N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s",mac.time().as_double(), (const char*)M ); \
			logerror A; \
		} \
	} while (0)


static READ8_DEVICE_HANDLER ( pio_port_b_r );
static READ8_DEVICE_HANDLER ( pio_port_c_r );
static WRITE8_DEVICE_HANDLER ( pio_port_a_w );
static WRITE8_DEVICE_HANDLER ( pio_port_c_w );

I8255A_INTERFACE( mz700_ppi8255_interface )
{
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER("ls145", pio_port_b_r),
	DEVCB_HANDLER(pio_port_c_r),
	DEVCB_DEVICE_HANDLER("ls145", pio_port_a_w),
	DEVCB_NULL,
	DEVCB_HANDLER(pio_port_c_w)
};


static WRITE_LINE_DEVICE_HANDLER( pit_out0_changed );
static WRITE_LINE_DEVICE_HANDLER( pit_irq_2 );

const struct pit8253_config mz700_pit8253_config =
{
	{
		/* clockin             gate            callback */
		{ XTAL_17_73447MHz/20, DEVCB_NULL,     DEVCB_LINE(pit_out0_changed) },
		{	          15611.0, DEVCB_LINE_VCC, DEVCB_LINE(pit8253_clk2_w)   },
		{		            0, DEVCB_LINE_VCC, DEVCB_LINE(pit_irq_2)        },
	}
};

const struct pit8253_config mz800_pit8253_config =
{
	{
		/* clockin             gate            callback */
		{ XTAL_17_73447MHz/16, DEVCB_NULL,     DEVCB_LINE(pit_out0_changed) },
		{	          15611.0, DEVCB_LINE_VCC, DEVCB_LINE(pit8253_clk2_w)   },
		{		            0, DEVCB_LINE_VCC, DEVCB_LINE(pit_irq_2)        },
	}
};


/***************************************************************************
    INITIALIZATIoN
***************************************************************************/

DRIVER_INIT( mz700 )
{
	mz_state *state = machine.driver_data<mz_state>();
	mz_state *mz = machine.driver_data<mz_state>();
	mz->m_mz700 = TRUE;
	mz->m_mz700_mode = TRUE;

	state->m_videoram = auto_alloc_array(machine, UINT8, 0x800);
	mz->m_colorram = auto_alloc_array(machine, UINT8, 0x800);
}

DRIVER_INIT( mz800 )
{
	mz_state *state = machine.driver_data<mz_state>();
	mz_state *mz = machine.driver_data<mz_state>();
	mz->m_mz700 = FALSE;
	mz->m_mz700_mode = FALSE;

	/* video ram */
	state->m_videoram = auto_alloc_array(machine, UINT8, 0x4000);
	mz->m_colorram = state->m_videoram + 0x800;

	/* character generator ram */
	mz->m_cgram = auto_alloc_array(machine, UINT8, 0x1000);
}

MACHINE_START( mz700 )
{
	mz_state *mz = machine.driver_data<mz_state>();

	mz->m_pit = machine.device("pit8253");
	mz->m_ppi = machine.device("ppi8255");

	/* reset memory map to defaults */
	mz700_bank_4_w(machine.device("maincpu")->memory().space(AS_PROGRAM), 0, 0);
}


/***************************************************************************
    MMIO
***************************************************************************/

static READ8_HANDLER( mz700_e008_r )
{
	mz_state *mz = space->machine().driver_data<mz_state>();
	UINT8 data = 0;

	data |= mz->m_other_timer;
	data |= input_port_read(space->machine(), "JOY");
	data |= space->machine().primary_screen->hblank() << 7;

	LOG(1, "mz700_e008_r", ("%02X\n", data), space->machine());

	return data;
}

static WRITE8_HANDLER( mz700_e008_w )
{
	mz_state *mz = space->machine().driver_data<mz_state>();
	pit8253_gate0_w(mz->m_pit, BIT(data, 0));
}


/***************************************************************************
    BANK SWITCHING
***************************************************************************/

READ8_HANDLER( mz800_bank_0_r )
{
	mz_state *state = space->machine().driver_data<mz_state>();
	UINT8 *videoram = state->m_videoram;
	address_space *spc = space->machine().device("maincpu")->memory().space(AS_PROGRAM);
	mz_state *mz = space->machine().driver_data<mz_state>();

	/* switch in cgrom */
	spc->install_read_bank(0x1000, 0x1fff, "bank2");
	spc->nop_write(0x1000, 0x1fff);
	memory_set_bankptr(space->machine(), "bank2", space->machine().region("monitor")->base() + 0x1000);

	if (mz->m_mz700_mode)
	{
		/* cgram from 0xc000 to 0xcfff */
		spc->install_read_bank(0xc000, 0xcfff, "bank6");
		spc->install_legacy_write_handler(0xc000, 0xcfff, FUNC(mz800_cgram_w));
		memory_set_bankptr(space->machine(), "bank6", mz->m_cgram);
	}
	else
	{
		if (mz->m_hires_mode)
		{
			/* vram from 0x8000 to 0xbfff */
			spc->install_readwrite_bank(0x8000, 0xbfff, "bank4");
			memory_set_bankptr(space->machine(), "bank4", videoram);
		}
		else
		{
			/* vram from 0x8000 to 0x9fff */
			spc->install_readwrite_bank(0x8000, 0x9fff, "bank4");
			memory_set_bankptr(space->machine(), "bank4", videoram);

			/* ram from 0xa000 to 0xbfff */
			spc->install_readwrite_bank(0xa000, 0xbfff, "bank5");
			memory_set_bankptr(space->machine(), "bank5", ram_get_ptr(space->machine().device(RAM_TAG)) + 0xa000);
		}
	}

	return 0xff;
}

WRITE8_HANDLER( mz700_bank_0_w )
{
	address_space *spc = space->machine().device("maincpu")->memory().space(AS_PROGRAM);

	spc->install_readwrite_bank(0x0000, 0x0fff, "bank1");
	memory_set_bankptr(space->machine(), "bank1", ram_get_ptr(space->machine().device(RAM_TAG)));
}

WRITE8_HANDLER( mz800_bank_0_w )
{
	address_space *spc = space->machine().device("maincpu")->memory().space(AS_PROGRAM);

	spc->install_readwrite_bank(0x0000, 0x7fff, "bank1");
	memory_set_bankptr(space->machine(), "bank1", ram_get_ptr(space->machine().device(RAM_TAG)));
}

READ8_HANDLER( mz800_bank_1_r )
{
	address_space *spc = space->machine().device("maincpu")->memory().space(AS_PROGRAM);
	mz_state *mz = space->machine().driver_data<mz_state>();

	/* switch in ram from 0x1000 to 0x1fff */
	spc->install_readwrite_bank(0x1000, 0x1fff, "bank2");
	memory_set_bankptr(space->machine(), "bank2", ram_get_ptr(space->machine().device(RAM_TAG)) + 0x1000);

	if (mz->m_mz700_mode)
	{
		/* ram from 0xc000 to 0xcfff */
		spc->install_readwrite_bank(0xc000, 0xcfff, "bank6");
		memory_set_bankptr(space->machine(), "bank6", ram_get_ptr(space->machine().device(RAM_TAG)) + 0xc000);
	}
	else
	{
		/* ram from 0x8000 to 0xbfff */
		spc->install_readwrite_bank(0x8000, 0xbfff, "bank4");
		memory_set_bankptr(space->machine(), "bank4", ram_get_ptr(space->machine().device(RAM_TAG)) + 0x8000);
	}

	return 0xff;
}

WRITE8_HANDLER( mz700_bank_1_w )
{
	address_space *spc = space->machine().device("maincpu")->memory().space(AS_PROGRAM);
	mz_state *mz = space->machine().driver_data<mz_state>();

	if (mz->m_mz700_mode)
	{
		/* switch in ram when not locked */
		if (!mz->m_mz700_ram_lock)
		{
			spc->install_readwrite_bank(0xd000, 0xffff, "bank7");
			memory_set_bankptr(space->machine(), "bank7", ram_get_ptr(space->machine().device(RAM_TAG)) + 0xd000);
			mz->m_mz700_ram_vram = FALSE;
		}
	}
	else
	{
		/* switch in ram when not locked */
		if (!mz->m_mz800_ram_lock)
		{
			spc->install_readwrite_bank(0xe000, 0xffff, "bank8");
			memory_set_bankptr(space->machine(), "bank8", ram_get_ptr(space->machine().device(RAM_TAG)) + 0xe000);
			mz->m_mz800_ram_monitor = FALSE;
		}
	}
}

WRITE8_HANDLER( mz700_bank_2_w )
{
	address_space *spc = space->machine().device("maincpu")->memory().space(AS_PROGRAM);

	spc->install_read_bank(0x0000, 0x0fff, "bank1");
	spc->nop_write(0x0000, 0x0fff);
	memory_set_bankptr(space->machine(), "bank1", space->machine().region("monitor")->base());
}

WRITE8_HANDLER( mz700_bank_3_w )
{
	mz_state *state = space->machine().driver_data<mz_state>();
	UINT8 *videoram = state->m_videoram;
	address_space *spc = space->machine().device("maincpu")->memory().space(AS_PROGRAM);
	mz_state *mz = space->machine().driver_data<mz_state>();

	if (mz->m_mz700_mode)
	{
		if (!mz->m_mz700_ram_lock)
		{
			/* switch in videoram */
			spc->install_readwrite_bank(0xd000, 0xd7ff, "bank7");
			memory_set_bankptr(space->machine(), "bank7", videoram);

			/* switch in colorram */
			spc->install_readwrite_bank(0xd800, 0xdfff, "bank9");
			memory_set_bankptr(space->machine(), "bank9", mz->m_colorram);

			mz->m_mz700_ram_vram = TRUE;

			/* switch in memory mapped i/o devices */
			if (mz->m_mz700)
			{
				spc->install_legacy_readwrite_handler(*mz->m_ppi, 0xe000, 0xfff3, 0, 0x1ff0, FUNC(i8255a_r), FUNC(i8255a_w));
				spc->install_legacy_readwrite_handler(*mz->m_pit, 0xe004, 0xfff7, 0, 0x1ff0, FUNC(pit8253_r), FUNC(pit8253_w));
				spc->install_legacy_readwrite_handler(0xe008, 0xfff8, 0, 0x1ff0, FUNC(mz700_e008_r), FUNC(mz700_e008_w));
			}
			else
			{
				spc->install_legacy_readwrite_handler(*mz->m_ppi, 0xe000, 0xe003, FUNC(i8255a_r), FUNC(i8255a_w));
				spc->install_legacy_readwrite_handler(*mz->m_pit, 0xe004, 0xe007, FUNC(pit8253_r), FUNC(pit8253_w));
				spc->install_legacy_readwrite_handler(0xe008, 0xe008, FUNC(mz700_e008_r), FUNC(mz700_e008_w));
			}
		}
	}
	else
	{
		if (!mz->m_mz800_ram_lock)
		{
			/* switch in mz800 monitor rom if not locked */
			spc->install_read_bank(0xe000, 0xffff, "bank8");
			spc->nop_write(0xe000, 0xffff);
			memory_set_bankptr(space->machine(), "bank8", space->machine().region("monitor")->base() + 0x2000);
			mz->m_mz800_ram_monitor = TRUE;
		}
	}
}

WRITE8_HANDLER( mz700_bank_4_w )
{
	mz_state *state = space->machine().driver_data<mz_state>();
	UINT8 *videoram = state->m_videoram;
	address_space *spc = space->machine().device("maincpu")->memory().space(AS_PROGRAM);
	mz_state *mz = space->machine().driver_data<mz_state>();

	if (mz->m_mz700_mode)
	{
		mz->m_mz700_ram_lock = FALSE;		/* reset lock */
		mz700_bank_2_w(space, 0, 0);	/* switch in monitor rom */
		mz700_bank_3_w(space, 0, 0);	/* switch in videoram, colorram, and mmio */

		/* rest is ram is always ram in mz700 mode */
		spc->install_readwrite_bank(0x1000, 0xcfff, "bank2");
		memory_set_bankptr(space->machine(), "bank2", ram_get_ptr(space->machine().device(RAM_TAG)) + 0x1000);
	}
	else
	{
		/* monitor rom and cgrom */
		spc->install_read_bank(0x0000, 0x1fff, "bank1");
		spc->nop_write(0x0000, 0x1fff);
		memory_set_bankptr(space->machine(), "bank1", space->machine().region("monitor")->base());

		/* ram from 0x2000 to 0x7fff */
		spc->install_readwrite_bank(0x2000, 0x7fff, "bank3");
		memory_set_bankptr(space->machine(), "bank3", ram_get_ptr(space->machine().device(RAM_TAG)));

		if (mz->m_hires_mode)
		{
			/* vram from 0x8000 to 0xbfff */
			spc->install_readwrite_bank(0x8000, 0xbfff, "bank4");
			memory_set_bankptr(space->machine(), "bank4", videoram);
		}
		else
		{
			/* vram from 0x8000 to 0x9fff */
			spc->install_readwrite_bank(0x8000, 0x9fff, "bank4");
			memory_set_bankptr(space->machine(), "bank4", videoram);

			/* ram from 0xa000 to 0xbfff */
			spc->install_readwrite_bank(0xa000, 0xbfff, "bank5");
			memory_set_bankptr(space->machine(), "bank5", ram_get_ptr(space->machine().device(RAM_TAG)) + 0xa000);
		}

		/* ram from 0xc000 to 0xdfff */
		spc->install_readwrite_bank(0xc000, 0xdfff, "bank6");
		memory_set_bankptr(space->machine(), "bank6", ram_get_ptr(space->machine().device(RAM_TAG)) + 0xc000);

		/* mz800 monitor rom from 0xe000 to 0xffff */
		spc->install_read_bank(0xe000, 0xffff, "bank8");
		spc->nop_write(0xe000, 0xffff);
		memory_set_bankptr(space->machine(), "bank8", space->machine().region("monitor")->base() + 0x2000);
		mz->m_mz800_ram_monitor = TRUE;

		mz->m_mz800_ram_lock = FALSE; /* reset lock? */
	}
}

WRITE8_HANDLER( mz700_bank_5_w )
{
	address_space *spc = space->machine().device("maincpu")->memory().space(AS_PROGRAM);
	mz_state *mz = space->machine().driver_data<mz_state>();

	if (mz->m_mz700_mode)
	{
		/* prevent access from 0xd000 to 0xffff */
		mz->m_mz700_ram_lock = TRUE;
		spc->nop_readwrite(0xd000, 0xffff);
	}
	else
	{
		/* prevent access from 0xe000 to 0xffff */
		mz->m_mz800_ram_lock = TRUE;
		spc->nop_readwrite(0xe000, 0xffff);
	}
}

WRITE8_HANDLER( mz700_bank_6_w )
{
	mz_state *mz = space->machine().driver_data<mz_state>();

	if (mz->m_mz700_mode)
	{
		mz->m_mz700_ram_lock = FALSE;

		/* restore access */
		if (mz->m_mz700_ram_vram)
			mz700_bank_3_w(space, 0, 0);
		else
			mz700_bank_1_w(space, 0, 0);
	}
	else
	{
		mz->m_mz800_ram_lock = FALSE;

		/* restore access from 0xe000 to 0xffff */
		if (mz->m_mz800_ram_monitor)
			mz700_bank_3_w(space, 0, 0);
		else
			mz700_bank_1_w(space, 0, 0);
	}
}


/************************ PIT ************************************************/

/* Timer 0 is the clock for the speaker output */

static WRITE_LINE_DEVICE_HANDLER( pit_out0_changed )
{
	mz_state *drvstate = device->machine().driver_data<mz_state>();
	device_t *speaker = device->machine().device("speaker");
	if((drvstate->m_prev_state==0) && (state==1)) {
		drvstate->m_speaker_level ^= 1;
	}
	drvstate->m_prev_state = state;
	speaker_level_w( speaker, drvstate->m_speaker_level);
}

/* timer 2 is the AM/PM (12 hour) interrupt */
static WRITE_LINE_DEVICE_HANDLER( pit_irq_2 )
{
	mz_state *mz = device->machine().driver_data<mz_state>();

	if (mz->m_intmsk)
		cputag_set_input_line(device->machine(), "maincpu", 0, ASSERT_LINE);
}


/***************************************************************************
    8255 PPI
***************************************************************************/

static READ8_DEVICE_HANDLER( pio_port_b_r )
{
	int key_line = ttl74145_r(device, 0, 0);

	switch (key_line)
	{
	case 1 << 0: return input_port_read(device->machine(), "ROW0");
	case 1 << 1: return input_port_read(device->machine(), "ROW1");
	case 1 << 2: return input_port_read(device->machine(), "ROW2");
	case 1 << 3: return input_port_read(device->machine(), "ROW3");
	case 1 << 4: return input_port_read(device->machine(), "ROW4");
	case 1 << 5: return input_port_read(device->machine(), "ROW5");
	case 1 << 6: return input_port_read(device->machine(), "ROW6");
	case 1 << 7: return input_port_read(device->machine(), "ROW7");
	case 1 << 8: return input_port_read(device->machine(), "ROW8");
	case 1 << 9: return input_port_read(device->machine(), "ROW9");
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
	device_t *cas = device->machine().device("cassette");
	mz_state *mz = device->machine().driver_data<mz_state>();
	UINT8 data = 0;

	/* note: this is actually connected to Q output of the motor-control flip-flop (see below) */
	if ((cassette_get_state(cas) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
		data |= 0x10;

	if (cassette_input(cas) > 0.0038)
		data |= 0x20;       /* set the RDATA status */

	data |= mz->m_cursor_timer << 6;
	data |= device->machine().primary_screen->vblank() << 7;

	LOG(2,"mz700_pio_port_c_r",("%02X\n", data),device->machine());

	return data;
}


static WRITE8_DEVICE_HANDLER( pio_port_a_w )
{
	timer_device *timer = device->machine().device<timer_device>("cursor");

	LOG(2,"mz700_pio_port_a_w",("%02X\n", data),device->machine());

	/* the ls145 is connected to PA0-PA3 */
	ttl74145_w(device, 0, data & 0x07);

	/* ne556 reset is connected to PA7 */
	timer->enable(BIT(data, 7));
}


static WRITE8_DEVICE_HANDLER( pio_port_c_w )
{
    /*
     * bit 3 out    motor control (0 = on)
     * bit 2 out    INTMSK
     * bit 1 out    tape data (WDATA)
     * bit 0 out    unused
     */

//  UINT8 state = cassette_get_state(device->machine().device("cassette"));
//  UINT8 action = ((~pio_port_c_output & 8) & (data & 8));     /* detect low-to-high transition */

	/* The motor control circuit consists of a resistor, capacitor, invertor, nand-gate, and D flip-flop.
        The sense input from the cassette player goes low whenever play, rewind or fast-forward is pressed.
        This connects to most of the above components.
        The Q output enables the motor, and also connects to Bit 4 (input).
        Bit 3 outputs a string of pulses to the Clock pin, and therefore cannot be used to control
        the motor directly.
        For the moment, the user can use the UI to select play, stop, etc.
        If you load from the command-line or the software-picker, type in L <enter> immediately. */
#if 0
	cassette_change_state(
		device->machine().device("cassette"),
		((data & 0x08) && mz700_motor_on) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
		CASSETTE_MOTOR_DISABLED);

#endif

	LOG(2,"mz700_pio_port_c_w",("%02X\n", data),device->machine());

	cassette_output(device->machine().device("cassette"), (data & 0x02) ? +1.0 : -1.0);
}


/******************************************************************************
 *  Sharp MZ800
 *
 *
 ******************************************************************************/

//static UINT8 mz800_display_mode = 0;
//static UINT8 mz800_port_e8 = 0;


/***************************************************************************
    Z80 PIO
***************************************************************************/

static void mz800_z80pio_irq(device_t *device, int which)
{
	cputag_set_input_line(device->machine(), "maincpu", 0, which);
}

static READ8_DEVICE_HANDLER( mz800_z80pio_port_a_r )
{
	device_t *printer = device->machine().device("centronics");
	UINT8 result = 0;

	result |= centronics_busy_r(printer);
	result |= centronics_pe_r(printer) << 1;
	result |= device->machine().primary_screen->hblank() << 5;

	return result;
}

static WRITE8_DEVICE_HANDLER( mz800_z80pio_port_a_w )
{
	device_t *printer = device->machine().device("centronics");

	centronics_prime_w(printer, BIT(data, 6));
	centronics_strobe_w(printer, BIT(data, 7));
}

static WRITE8_DEVICE_HANDLER( mz800_printer_data_w )
{
	device_t *printer = device->machine().device("centronics");
	centronics_data_w(printer, 0, data);
}

const z80pio_interface mz800_z80pio_config =
{
	DEVCB_LINE(mz800_z80pio_irq),
	DEVCB_HANDLER(mz800_z80pio_port_a_r),
	DEVCB_HANDLER(mz800_z80pio_port_a_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(mz800_printer_data_w),
	DEVCB_NULL,
};




/* port CE */
READ8_HANDLER( mz800_crtc_r )
{
	UINT8 data = 0x00;
	LOG(1,"mz800_crtc_r",("%02X\n",data),space->machine());
    return data;
}


/* port EA */
 READ8_HANDLER( mz800_ramdisk_r )
{
	mz_state *state = space->machine().driver_data<mz_state>();
	UINT8 *mem = space->machine().region("user1")->base();
	UINT8 data = mem[state->m_mz800_ramaddr];
	LOG(2,"mz800_ramdisk_r",("[%04X] -> %02X\n", state->m_mz800_ramaddr, data),space->machine());
	if (state->m_mz800_ramaddr++ == 0)
		LOG(1,"mz800_ramdisk_r",("address wrap 0000\n"),space->machine());
    return data;
}

/* port CC */
WRITE8_HANDLER( mz800_write_format_w )
{
	LOG(1,"mz800_write_format_w",("%02X\n", data),space->machine());
}

/* port CD */
WRITE8_HANDLER( mz800_read_format_w )
{
	LOG(1,"mz800_read_format_w",("%02X\n", data),space->machine());
}

/* port CE
 * bit 3    1: MZ700 mode       0: MZ800 mode
 * bit 2    1: 640 horizontal   0: 320 horizontal
 * bit 1    1: 4bpp/2bpp        0: 2bpp/1bpp
 * bit 0    ???
 */
WRITE8_HANDLER( mz800_display_mode_w )
{
	mz_state *mz = space->machine().driver_data<mz_state>();

	mz->m_mz700_mode = BIT(data, 3);
	mz->m_hires_mode = BIT(data, 2);
	mz->m_screen = data & 0x03;

	/* change memory maps if we switched mode */
//  if (BIT(data, 3) != mz->m_mz700_mode)
//  {
//      logerror("mz800_display_mode_w: switching mode to %s\n", (BIT(data, 3) ? "mz700" : "mz800"));
//      mz->m_mz700_mode = BIT(data, 3);
//      mz700_bank_4_w(space->machine().device("maincpu")->memory().space(AS_PROGRAM), 0, 0);
//  }
}

/* port CF */
WRITE8_HANDLER( mz800_scroll_border_w )
{
	LOG(1,"mz800_scroll_border_w",("%02X\n", data),space->machine());
}

/* port EA */
WRITE8_HANDLER( mz800_ramdisk_w )
{
	mz_state *state = space->machine().driver_data<mz_state>();
	UINT8 *mem = space->machine().region("user1")->base();
	LOG(2,"mz800_ramdisk_w",("[%04X] <- %02X\n", state->m_mz800_ramaddr, data),space->machine());
	mem[state->m_mz800_ramaddr] = data;
	if (state->m_mz800_ramaddr++ == 0)
		LOG(1,"mz800_ramdisk_w",("address wrap 0000\n"),space->machine());
}

/* port EB */
WRITE8_HANDLER( mz800_ramaddr_w )
{
	mz_state *state = space->machine().driver_data<mz_state>();
	state->m_mz800_ramaddr = (cpu_get_reg(space->machine().device("maincpu"), Z80_BC) & 0xff00) | (data & 0xff);
	LOG(1,"mz800_ramaddr_w",("%04X\n", state->m_mz800_ramaddr),space->machine());
}

/* port F0 */
WRITE8_HANDLER( mz800_palette_w )
{
	mz_state *state = space->machine().driver_data<mz_state>();
	if (data & 0x40)
	{
        state->m_mz800_palette_bank = data & 3;
		LOG(1,"mz800_palette_w",("bank: %d\n", state->m_mz800_palette_bank),space->machine());
    }
	else
	{
		int idx = (data >> 4) & 3;
		int val = data & 15;
		LOG(1,"mz800_palette_w",("palette[%d] <- %d\n", idx, val),space->machine());
		state->m_mz800_palette[idx] = val;
	}
}
