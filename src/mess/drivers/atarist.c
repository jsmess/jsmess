#include "emu.h"
#include "includes/atarist.h"
#include "cpu/m68000/m68000.h"
#include "cpu/m6800/m6800.h"
#include "audio/lmc1992.h"
#include "formats/atarist_dsk.h"
#include "devices/cartslot.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"
#include "machine/6850acia.h"
#include "machine/68901mfp.h"
#include "machine/8530scc.h"
#include "machine/ctronics.h"
#include "machine/rescap.h"
#include "machine/rp5c15.h"
#include "machine/rs232.h"
#include "machine/wd17xx.h"
#include "sound/ay8910.h"
#include "video/atarist.h"

/*

    TODO:

    - fix floppy interface
    - fix mouse
    - MSA disk image support
    - UK keyboard layout for the special keys
    - accurate screen timing
    - floppy DMA transfer timer
    - memory shadow for boot memory check
    - STe DMA sound and LMC1992 Microwire mixer
    - Mega ST/STe MC68881 FPU
    - MIDI interface
    - Mega STe 16KB cache
    - Mega STe LAN

*/

/* Floppy Disk Controller */

static void atarist_fdc_dma_transfer(running_machine *machine)
{
	address_space *program = cputag_get_address_space(machine, M68000_TAG, ADDRESS_SPACE_PROGRAM);
	atarist_state *state = machine->driver_data<atarist_state>();

	if ((state->fdc_mode & ATARIST_FLOPPY_MODE_DMA_DISABLE) == 0)
	{
		while (state->fdc_sectors > 0)
		{
			if (state->fdc_mode & ATARIST_FLOPPY_MODE_WRITE)
			{
				UINT8 data = program->read_byte(state->fdc_dmabase);

				wd17xx_data_w(state->wd1772, 0, data);
			}
			else
			{
				UINT8 data = wd17xx_data_r(state->wd1772, 0);

				program->write_byte(state->fdc_dmabase, data);
			}

			state->fdc_dmabase++;
			state->fdc_dmabytes--;

			if (state->fdc_dmabytes == 0)
			{
				state->fdc_sectors--;

				if (state->fdc_sectors == 0)
				{
					state->fdc_status &= ~ATARIST_FLOPPY_STATUS_SECTOR_COUNT_ZERO;
				}
				else
				{
					state->fdc_dmabytes = ATARIST_FLOPPY_BYTES_PER_SECTOR;
				}
			}
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( atarist_fdc_intrq_w )
{
	atarist_state *driver_state = device->machine->driver_data<atarist_state>();

	driver_state->fdc_irq = state;
}

static WRITE_LINE_DEVICE_HANDLER( atarist_fdc_drq_w )
{
	atarist_state *driver_state = device->machine->driver_data<atarist_state>();

	if (state)
	{
		driver_state->fdc_status |= ATARIST_FLOPPY_STATUS_FDC_DATA_REQUEST;
		atarist_fdc_dma_transfer(device->machine);
	}
	else
	{
		driver_state->fdc_status &= ~ATARIST_FLOPPY_STATUS_FDC_DATA_REQUEST;
	}
}

static const wd17xx_interface fdc_intf =
{
	DEVCB_LINE_GND,
	DEVCB_LINE(atarist_fdc_intrq_w),
	DEVCB_LINE(atarist_fdc_drq_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

static const wd17xx_interface stbook_fdc_intf =
{
	DEVCB_NULL,
	DEVCB_LINE(atarist_fdc_intrq_w),
	DEVCB_LINE(atarist_fdc_drq_w),
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

static READ16_HANDLER( atarist_fdc_data_r )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	if (state->fdc_mode & ATARIST_FLOPPY_MODE_SECTOR_COUNT)
	{
		return state->fdc_sectors;
	}
	else
	{
		if (state->fdc_mode & ATARIST_FLOPPY_MODE_HDC)
		{
			// HDC not implemented
			state->fdc_status &= ~ATARIST_FLOPPY_STATUS_DMA_ERROR;

			return 0;
		}
		else
		{
			return wd17xx_r(state->wd1772, (state->fdc_mode & ATARIST_FLOPPY_MODE_ADDRESS_MASK) >> 1);
		}
	}
}

static WRITE16_HANDLER( atarist_fdc_data_w )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	if (state->fdc_mode & ATARIST_FLOPPY_MODE_SECTOR_COUNT)
	{
		if (data == 0)
		{
			state->fdc_status &= ~ATARIST_FLOPPY_STATUS_SECTOR_COUNT_ZERO;
		}
		else
		{
			state->fdc_status |= ATARIST_FLOPPY_STATUS_SECTOR_COUNT_ZERO;
		}

		state->fdc_sectors = data;
	}
	else
	{
		if (state->fdc_mode & ATARIST_FLOPPY_MODE_HDC)
		{
			// HDC not implemented
			state->fdc_status &= ~ATARIST_FLOPPY_STATUS_DMA_ERROR;
		}
		else
		{
			wd17xx_w(state->wd1772, (state->fdc_mode & ATARIST_FLOPPY_MODE_ADDRESS_MASK) >> 1, data);
		}
	}
}

static READ16_HANDLER( atarist_fdc_dma_status_r )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	return state->fdc_status;
}

static WRITE16_HANDLER( atarist_fdc_dma_mode_w )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	if ((data & ATARIST_FLOPPY_MODE_WRITE) != (state->fdc_mode & ATARIST_FLOPPY_MODE_WRITE))
	{
		state->fdc_status = 0;
		state->fdc_sectors = 0;
	}

	state->fdc_mode = data;
}

static READ16_HANDLER( atarist_fdc_dma_base_r )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	switch (offset)
	{
	case 0:
		return (state->fdc_dmabase >> 16) & 0xff;
	case 1:
		return (state->fdc_dmabase >> 8) & 0xff;
	case 2:
		return state->fdc_dmabase & 0xff;
	}

	return 0;
}

static WRITE16_HANDLER( atarist_fdc_dma_base_w )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	switch (offset)
	{
	case 0:
		state->fdc_dmabase = (state->fdc_dmabase & 0x00ffff) | ((data & 0xff) << 16);
		break;
	case 1:
		state->fdc_dmabase = (state->fdc_dmabase & 0x0000ff) | ((data & 0xff) << 8);
		break;
	case 2:
		state->fdc_dmabase = data & 0xff;
		break;
	}

	state->fdc_dmabytes = ATARIST_FLOPPY_BYTES_PER_SECTOR;
}

/* MMU */

static READ16_HANDLER( atarist_mmu_r )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	return state->mmu;
}

static WRITE16_HANDLER( atarist_mmu_w )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	state->mmu = data & 0xff;
}

/* IKBD */

static const int IKBD_MOUSE_XYA[3][4] = { { 0, 0, 0, 0 }, { 1, 1, 0, 0 }, { 0, 1, 1, 0 } };
static const int IKBD_MOUSE_XYB[3][4] = { { 0, 0, 0, 0 }, { 0, 1, 1, 0 }, { 1, 1, 0, 0 } };

static READ8_HANDLER( ikbd_port1_r )
{
	/*

        bit     description

        0       Keyboard column input
        1       Keyboard column input
        2       Keyboard column input
        3       Keyboard column input
        4       Keyboard column input
        5       Keyboard column input
        6       Keyboard column input
        7       Keyboard column input

    */

	atarist_state *state = space->machine->driver_data<atarist_state>();

	return state->ikbd_keylatch;
}

static READ8_HANDLER( ikbd_port2_r )
{
	/*

        bit     description

        0       JOY 1-5
        1       JOY 0-6
        2       JOY 1-6
        3       SD FROM CPU
        4       SD TO CPU

    */

	atarist_state *state = space->machine->driver_data<atarist_state>();

	return (state->ikbd_tx << 3) | (input_port_read_safe(space->machine, "IKBD_JOY1", 0xff) & 0x06);
}

static WRITE8_HANDLER( ikbd_port2_w )
{
	/*

        bit     description

        0       JOY 1-5
        1       JOY 0-6
        2       JOY 1-6
        3       SD FROM CPU
        4       SD TO CPU

    */

	atarist_state *state = space->machine->driver_data<atarist_state>();

	state->ikbd_rx = (data & 0x10) >> 4;
}

static WRITE8_HANDLER( ikbd_port3_w )
{
	/*

        bit     description

        0       CAPS LOCK LED
        1       Keyboard row select
        2       Keyboard row select
        3       Keyboard row select
        4       Keyboard row select
        5       Keyboard row select
        6       Keyboard row select
        7       Keyboard row select

    */

	atarist_state *state = space->machine->driver_data<atarist_state>();

	set_led_status(space->machine, 1, data & 0x01);

	if (~data & 0x02) state->ikbd_keylatch = input_port_read(space->machine, "P31");
	if (~data & 0x04) state->ikbd_keylatch = input_port_read(space->machine, "P32");
	if (~data & 0x08) state->ikbd_keylatch = input_port_read(space->machine, "P33");
	if (~data & 0x10) state->ikbd_keylatch = input_port_read(space->machine, "P34");
	if (~data & 0x20) state->ikbd_keylatch = input_port_read(space->machine, "P35");
	if (~data & 0x40) state->ikbd_keylatch = input_port_read(space->machine, "P36");
	if (~data & 0x80) state->ikbd_keylatch = input_port_read(space->machine, "P37");
}

static READ8_HANDLER( ikbd_port4_r )
{
	/*

        bit     description

        0       JOY 0-1 or mouse XB
        1       JOY 0-2 or mouse XA
        2       JOY 0-3 or mouse YA
        3       JOY 0-4 or mouse YB
        4       JOY 1-1
        5       JOY 1-2
        6       JOY 1-3
        7       JOY 1-4

    */

	atarist_state *state = space->machine->driver_data<atarist_state>();

	if (input_port_read(space->machine, "config") & 0x01)
	{
		/*

                Right   Left        Up      Down

            XA  1100    0110    YA  1100    0110
            XB  0110    1100    YB  0110    1100

        */

		UINT8 data = input_port_read_safe(space->machine, "IKBD_JOY0", 0xff) & 0xf0;
		UINT8 x = input_port_read_safe(space->machine, "IKBD_MOUSEX", 0x00);
		UINT8 y = input_port_read_safe(space->machine, "IKBD_MOUSEY", 0x00);

		if (x == state->ikbd_mouse_x)
		{
			state->ikbd_mouse_px = IKBD_MOUSE_PHASE_STATIC;
		}
		else if (x > state->ikbd_mouse_x)
		{
			state->ikbd_mouse_px = IKBD_MOUSE_PHASE_POSITIVE;
		}
		else if (x < state->ikbd_mouse_x)
		{
			state->ikbd_mouse_px = IKBD_MOUSE_PHASE_NEGATIVE;
		}

		if (y == state->ikbd_mouse_y)
		{
			state->ikbd_mouse_py = IKBD_MOUSE_PHASE_STATIC;
		}
		else if (y > state->ikbd_mouse_y)
		{
			state->ikbd_mouse_py = IKBD_MOUSE_PHASE_POSITIVE;
		}
		else if (y < state->ikbd_mouse_y)
		{
			state->ikbd_mouse_py = IKBD_MOUSE_PHASE_NEGATIVE;
		}

		data |= IKBD_MOUSE_XYB[state->ikbd_mouse_px][state->ikbd_mouse_pc];		 // XB
		data |= IKBD_MOUSE_XYA[state->ikbd_mouse_px][state->ikbd_mouse_pc] << 1; // XA
		data |= IKBD_MOUSE_XYA[state->ikbd_mouse_py][state->ikbd_mouse_pc] << 2; // YA
		data |= IKBD_MOUSE_XYB[state->ikbd_mouse_py][state->ikbd_mouse_pc] << 3; // YB

		state->ikbd_mouse_pc++;

		if (state->ikbd_mouse_pc == 4)
		{
			state->ikbd_mouse_pc = 0;
		}

		state->ikbd_mouse_x = x;
		state->ikbd_mouse_y = y;

		return data;
	}
	else
	{
		return input_port_read_safe(space->machine, "IKBD_JOY0", 0xff);
	}
}

static WRITE8_HANDLER( ikbd_port4_w )
{
	/*

        bit     description

        0       Keyboard row select
        1       Keyboard row select
        2       Keyboard row select
        3       Keyboard row select
        4       Keyboard row select
        5       Keyboard row select
        6       Keyboard row select
        7       Keyboard row select

    */

	atarist_state *state = space->machine->driver_data<atarist_state>();

	if (~data & 0x01) state->ikbd_keylatch = input_port_read(space->machine, "P40");
	if (~data & 0x02) state->ikbd_keylatch = input_port_read(space->machine, "P41");
	if (~data & 0x04) state->ikbd_keylatch = input_port_read(space->machine, "P42");
	if (~data & 0x08) state->ikbd_keylatch = input_port_read(space->machine, "P43");
	if (~data & 0x10) state->ikbd_keylatch = input_port_read(space->machine, "P44");
	if (~data & 0x20) state->ikbd_keylatch = input_port_read(space->machine, "P45");
	if (~data & 0x40) state->ikbd_keylatch = input_port_read(space->machine, "P46");
	if (~data & 0x80) state->ikbd_keylatch = input_port_read(space->machine, "P47");
}

/* DMA Sound */

static const int DMASOUND_RATE[] = { Y2/640/8, Y2/640/4, Y2/640/2, Y2/640 };

static void atariste_dmasound_set_state(running_machine *machine, int level)
{
	atarist_state *state = machine->driver_data<atarist_state>();

	state->dmasnd_active = level;
	mc68901_tai_w(state->mc68901, level);

	if (level == 0)
	{
		state->dmasnd_baselatch = state->dmasnd_base;
		state->dmasnd_endlatch = state->dmasnd_end;
	}
	else
	{
		state->dmasnd_cntr = state->dmasnd_baselatch;
	}
}

static TIMER_CALLBACK( atariste_dmasound_tick )
{
	atarist_state *state = machine->driver_data<atarist_state>();

	if (state->dmasnd_samples == 0)
	{
		int i;
		UINT8 *RAM = messram_get_ptr(machine->device("messram"));

		for (i = 0; i < 8; i++)
		{
			state->dmasnd_fifo[i] = RAM[state->dmasnd_cntr];
			state->dmasnd_cntr++;
			state->dmasnd_samples++;

			if (state->dmasnd_cntr == state->dmasnd_endlatch)
			{
				atariste_dmasound_set_state(machine, 0);
				break;
			}
		}
	}

	if (state->dmasnd_ctrl & 0x80)
	{
//      logerror("DMA sound left  %i\n", state->dmasnd_fifo[7 - state->dmasnd_samples]);
		state->dmasnd_samples--;

//      logerror("DMA sound right %i\n", state->dmasnd_fifo[7 - state->dmasnd_samples]);
		state->dmasnd_samples--;
	}
	else
	{
//      logerror("DMA sound mono %i\n", state->dmasnd_fifo[7 - state->dmasnd_samples]);
		state->dmasnd_samples--;
	}

	if ((state->dmasnd_samples == 0) && (state->dmasnd_active == 0))
	{
		if ((state->dmasnd_ctrl & 0x03) == 0x03)
		{
			atariste_dmasound_set_state(machine, 1);
		}
		else
		{
			timer_enable(state->dmasound_timer, 0);
		}
	}
}

static READ16_HANDLER( atariste_sound_dma_control_r )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	return state->dmasnd_ctrl;
}

static READ16_HANDLER( atariste_sound_dma_base_r )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	switch (offset)
	{
	case 0x00:
		return (state->dmasnd_base >> 16) & 0x3f;
	case 0x01:
		return (state->dmasnd_base >> 8) & 0xff;
	case 0x02:
		return state->dmasnd_base & 0xff;
	}

	return 0;
}

static READ16_HANDLER( atariste_sound_dma_counter_r )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	switch (offset)
	{
	case 0x00:
		return (state->dmasnd_cntr >> 16) & 0x3f;
	case 0x01:
		return (state->dmasnd_cntr >> 8) & 0xff;
	case 0x02:
		return state->dmasnd_cntr & 0xff;
	}

	return 0;
}

static READ16_HANDLER( atariste_sound_dma_end_r )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	switch (offset)
	{
	case 0x00:
		return (state->dmasnd_end >> 16) & 0x3f;
	case 0x01:
		return (state->dmasnd_end >> 8) & 0xff;
	case 0x02:
		return state->dmasnd_end & 0xff;
	}

	return 0;
}

static READ16_HANDLER( atariste_sound_mode_r )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	return state->dmasnd_mode;
}

static WRITE16_HANDLER( atariste_sound_dma_control_w )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	state->dmasnd_ctrl = data & 0x03;

	if (state->dmasnd_ctrl & 0x01)
	{
		if (!state->dmasnd_active)
		{
			atariste_dmasound_set_state(space->machine, 1);
			timer_adjust_periodic(state->dmasound_timer, attotime_zero, 0, ATTOTIME_IN_HZ(DMASOUND_RATE[state->dmasnd_mode & 0x03]));
		}
	}
	else
	{
		atariste_dmasound_set_state(space->machine, 0);
		timer_enable(state->dmasound_timer, 0);
	}
}

static WRITE16_HANDLER( atariste_sound_dma_base_w )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	switch (offset)
	{
	case 0x00:
		state->dmasnd_base = (data << 16) & 0x3f0000;
		break;
	case 0x01:
		state->dmasnd_base = (state->dmasnd_base & 0x3f00fe) | (data & 0xff) << 8;
		break;
	case 0x02:
		state->dmasnd_base = (state->dmasnd_base & 0x3fff00) | (data & 0xfe);
		break;
	}

	if (!state->dmasnd_active)
	{
		state->dmasnd_baselatch = state->dmasnd_base;
	}
}

static WRITE16_HANDLER( atariste_sound_dma_end_w )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	switch (offset)
	{
	case 0x00:
		state->dmasnd_end = (data << 16) & 0x3f0000;
		break;
	case 0x01:
		state->dmasnd_end = (state->dmasnd_end & 0x3f00fe) | (data & 0xff) << 8;
		break;
	case 0x02:
		state->dmasnd_end = (state->dmasnd_end & 0x3fff00) | (data & 0xfe);
		break;
	}

	if (!state->dmasnd_active)
	{
		state->dmasnd_endlatch = state->dmasnd_end;
	}
}

static WRITE16_HANDLER( atariste_sound_mode_w )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	state->dmasnd_mode = data & 0x8f;
}

/* Microwire */

static void atariste_microwire_shift(running_machine *machine)
{
	atarist_state *state = machine->driver_data<atarist_state>();

	if (BIT(state->mw_mask, 15))
	{
		lmc1992_data_w(state->lmc1992, BIT(state->mw_data, 15));
		lmc1992_clock_w(state->lmc1992, 1);
		lmc1992_clock_w(state->lmc1992, 0);
	}

	// rotate mask and data left

	state->mw_mask = (state->mw_mask << 1) | BIT(state->mw_mask, 15);
	state->mw_data = (state->mw_data << 1) | BIT(state->mw_data, 15);
	state->mw_shift++;
}

static TIMER_CALLBACK( atariste_microwire_tick )
{
	atarist_state *state = machine->driver_data<atarist_state>();

	switch (state->mw_shift)
	{
	case 0:
		lmc1992_enable_w(state->lmc1992, 0);
		atariste_microwire_shift(machine);
		break;

	default:
		atariste_microwire_shift(machine);
		break;

	case 15:
		atariste_microwire_shift(machine);
		lmc1992_enable_w(state->lmc1992, 1);
		state->mw_shift = 0;
		timer_enable(state->microwire_timer, 0);
		break;
	}
}

static READ16_HANDLER( atariste_microwire_data_r )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	return state->mw_data;
}

static WRITE16_HANDLER( atariste_microwire_data_w )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	if (!timer_enabled(state->microwire_timer))
	{
		state->mw_data = data;
		timer_adjust_periodic(state->microwire_timer, attotime_zero, 0, ATTOTIME_IN_USEC(2));
	}
}

static READ16_HANDLER( atariste_microwire_mask_r )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	return state->mw_mask;
}

static WRITE16_HANDLER( atariste_microwire_mask_w )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	if (!timer_enabled(state->microwire_timer))
	{
		state->mw_mask = data;
	}
}

/* Mega STe Cache */

static READ16_HANDLER( megaste_cache_r )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	return state->megaste_cache;
}

static WRITE16_HANDLER( megaste_cache_w )
{
	atarist_state *state = space->machine->driver_data<atarist_state>();

	state->megaste_cache = data;

	cputag_set_clock(space->machine, M68000_TAG, BIT(data, 0) ? Y2/2 : Y2/4);
}

/* ST Book */

static READ16_HANDLER( stbook_config_r )
{
	/*

        bit     description

        0       _POWER_SWITCH
        1       _TOP_CLOSED
        2       _RTC_ALARM
        3       _SOURCE_DEAD
        4       _SOURCE_LOW
        5       _MODEM_WAKE
        6       (reserved)
        7       _EXPANSION_WAKE
        8       (reserved)
        9       (reserved)
        10      (reserved)
        11      (reserved)
        12      (reserved)
        13      SELF TEST
        14      LOW SPEED FLOPPY
        15      DMA AVAILABLE

    */

	return (input_port_read(space->machine, "SW400") << 8) | 0xff;
}

static WRITE16_HANDLER( stbook_lcd_control_w )
{
	/*

        bit     description

        0       Shadow Chip OFF
        1       _SHIFTER OFF
        2       POWEROFF
        3       _22ON
        4       RS-232_OFF
        5       (reserved)
        6       (reserved)
        7       MTR_PWR_ON

    */
}

/* Memory Maps */

static ADDRESS_MAP_START( ikbd_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_internal_registers_r, hd63701_internal_registers_w)
	AM_RANGE(0x0080, 0x00ff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_ROM AM_REGION(HD6301_TAG, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ikbd_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(HD63701_PORT1, HD63701_PORT1) AM_READ(ikbd_port1_r)
	AM_RANGE(HD63701_PORT2, HD63701_PORT2) AM_READWRITE(ikbd_port2_r, ikbd_port2_w)
	AM_RANGE(HD63701_PORT3, HD63701_PORT3) AM_WRITE(ikbd_port3_w)
	AM_RANGE(HD63701_PORT4, HD63701_PORT4) AM_READWRITE(ikbd_port4_r, ikbd_port4_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( st_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x000007) AM_ROM AM_REGION(M68000_TAG, 0)
	AM_RANGE(0x000008, 0x1fffff) AM_RAM
	AM_RANGE(0x200000, 0x3fffff) AM_RAM
//	AM_RANGE(0xfa0000, 0xfbffff) AM_ROM // cartridge
	AM_RANGE(0xfc0000, 0xfeffff) AM_ROM AM_REGION(M68000_TAG, 0)
	AM_RANGE(0xff8000, 0xff8001) AM_READWRITE(atarist_mmu_r, atarist_mmu_w)
	AM_RANGE(0xff8200, 0xff8203) AM_READWRITE(atarist_shifter_base_r, atarist_shifter_base_w)
	AM_RANGE(0xff8204, 0xff8209) AM_READ(atarist_shifter_counter_r)
	AM_RANGE(0xff820a, 0xff820b) AM_READWRITE8(atarist_shifter_sync_r, atarist_shifter_sync_w, 0xff00)
	AM_RANGE(0xff8240, 0xff825f) AM_READWRITE(atarist_shifter_palette_r, atarist_shifter_palette_w)
	AM_RANGE(0xff8260, 0xff8261) AM_READWRITE8(atarist_shifter_mode_r, atarist_shifter_mode_w, 0xff00)
	AM_RANGE(0xff8604, 0xff8605) AM_READWRITE(atarist_fdc_data_r, atarist_fdc_data_w)
	AM_RANGE(0xff8606, 0xff8607) AM_READWRITE(atarist_fdc_dma_status_r, atarist_fdc_dma_mode_w)
	AM_RANGE(0xff8608, 0xff860d) AM_READWRITE(atarist_fdc_dma_base_r, atarist_fdc_dma_base_w)
	AM_RANGE(0xff8800, 0xff8801) AM_DEVREADWRITE8(YM2149_TAG, ay8910_r, ay8910_data_w, 0xff00)
	AM_RANGE(0xff8802, 0xff8803) AM_DEVWRITE8(YM2149_TAG, ay8910_data_w, 0xff00)
	AM_RANGE(0xfffa00, 0xfffa2f) AM_DEVREADWRITE8(MC68901_TAG, mc68901_register_r, mc68901_register_w, 0xff)
	AM_RANGE(0xfffc00, 0xfffc01) AM_DEVREADWRITE8(MC6850_0_TAG, acia6850_stat_r, acia6850_ctrl_w, 0xff00)
	AM_RANGE(0xfffc02, 0xfffc03) AM_DEVREADWRITE8(MC6850_0_TAG, acia6850_data_r, acia6850_data_w, 0xff00)
	AM_RANGE(0xfffc04, 0xfffc05) AM_DEVREADWRITE8(MC6850_1_TAG, acia6850_stat_r, acia6850_ctrl_w, 0xff00)
	AM_RANGE(0xfffc06, 0xfffc07) AM_DEVREADWRITE8(MC6850_1_TAG, acia6850_data_r, acia6850_data_w, 0xff00)
ADDRESS_MAP_END

static ADDRESS_MAP_START( megast_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x000007) AM_ROM AM_REGION(M68000_TAG, 0)
	AM_RANGE(0x000008, 0x1fffff) AM_RAM
	AM_RANGE(0x200000, 0x3fffff) AM_RAM
//	AM_RANGE(0xfa0000, 0xfbffff) AM_ROM // cartridge
	AM_RANGE(0xfc0000, 0xfeffff) AM_ROM AM_REGION(M68000_TAG, 0)
	AM_RANGE(0xff7f30, 0xff7f31) AM_READWRITE(atarist_blitter_dst_inc_y_r, atarist_blitter_dst_inc_y_w) // for TOS 1.02
	AM_RANGE(0xff8000, 0xff8007) AM_READWRITE(atarist_mmu_r, atarist_mmu_w)
	AM_RANGE(0xff8200, 0xff8203) AM_READWRITE(atarist_shifter_base_r, atarist_shifter_base_w)
	AM_RANGE(0xff8204, 0xff8209) AM_READ(atarist_shifter_counter_r)
	AM_RANGE(0xff820a, 0xff820b) AM_READWRITE8(atarist_shifter_sync_r, atarist_shifter_sync_w, 0xff00)
	AM_RANGE(0xff8240, 0xff825f) AM_READWRITE(atarist_shifter_palette_r, atarist_shifter_palette_w)
	AM_RANGE(0xff8260, 0xff8261) AM_READWRITE8(atarist_shifter_mode_r, atarist_shifter_mode_w, 0xff00)
	AM_RANGE(0xff8604, 0xff8605) AM_READWRITE(atarist_fdc_data_r, atarist_fdc_data_w)
	AM_RANGE(0xff8606, 0xff8607) AM_READWRITE(atarist_fdc_dma_status_r, atarist_fdc_dma_mode_w)
	AM_RANGE(0xff8608, 0xff860d) AM_READWRITE(atarist_fdc_dma_base_r, atarist_fdc_dma_base_w)
	AM_RANGE(0xff8800, 0xff8801) AM_DEVREADWRITE8(YM2149_TAG, ay8910_r, ay8910_data_w, 0xff00)
	AM_RANGE(0xff8802, 0xff8803) AM_DEVWRITE8(YM2149_TAG, ay8910_data_w, 0xff00)
	AM_RANGE(0xff8a00, 0xff8a1f) AM_READWRITE(atarist_blitter_halftone_r, atarist_blitter_halftone_w)
	AM_RANGE(0xff8a20, 0xff8a21) AM_READWRITE(atarist_blitter_src_inc_x_r, atarist_blitter_src_inc_x_w)
	AM_RANGE(0xff8a22, 0xff8a23) AM_READWRITE(atarist_blitter_src_inc_y_r, atarist_blitter_src_inc_y_w)
	AM_RANGE(0xff8a24, 0xff8a27) AM_READWRITE(atarist_blitter_src_r, atarist_blitter_src_w)
	AM_RANGE(0xff8a28, 0xff8a2d) AM_READWRITE(atarist_blitter_end_mask_r, atarist_blitter_end_mask_w)
	AM_RANGE(0xff8a2e, 0xff8a2f) AM_READWRITE(atarist_blitter_dst_inc_x_r, atarist_blitter_dst_inc_x_w)
	AM_RANGE(0xff8a30, 0xff8a31) AM_READWRITE(atarist_blitter_dst_inc_y_r, atarist_blitter_dst_inc_y_w)
	AM_RANGE(0xff8a32, 0xff8a35) AM_READWRITE(atarist_blitter_dst_r, atarist_blitter_dst_w)
	AM_RANGE(0xff8a36, 0xff8a37) AM_READWRITE(atarist_blitter_count_x_r, atarist_blitter_count_x_w)
	AM_RANGE(0xff8a38, 0xff8a39) AM_READWRITE(atarist_blitter_count_y_r, atarist_blitter_count_y_w)
	AM_RANGE(0xff8a3a, 0xff8a3b) AM_READWRITE(atarist_blitter_op_r, atarist_blitter_op_w)
	AM_RANGE(0xff8a3c, 0xff8a3d) AM_READWRITE(atarist_blitter_ctrl_r, atarist_blitter_ctrl_w)
	AM_RANGE(0xfffa00, 0xfffa3f) AM_DEVREADWRITE8(MC68901_TAG, mc68901_register_r, mc68901_register_w, 0xff)
//  AM_RANGE(0xfffa40, 0xfffa57) AM_READWRITE(megast_fpu_r, megast_fpu_w)
	AM_RANGE(0xfffc00, 0xfffc01) AM_DEVREADWRITE8(MC6850_0_TAG, acia6850_stat_r, acia6850_ctrl_w, 0xff00)
	AM_RANGE(0xfffc02, 0xfffc03) AM_DEVREADWRITE8(MC6850_0_TAG, acia6850_data_r, acia6850_data_w, 0xff00)
	AM_RANGE(0xfffc04, 0xfffc05) AM_DEVREADWRITE8(MC6850_1_TAG, acia6850_stat_r, acia6850_ctrl_w, 0xff00)
	AM_RANGE(0xfffc06, 0xfffc07) AM_DEVREADWRITE8(MC6850_1_TAG, acia6850_data_r, acia6850_data_w, 0xff00)
	AM_RANGE(0xfffc20, 0xfffc3f) AM_DEVREADWRITE(RP5C15_TAG, rp5c15_r, rp5c15_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ste_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x000007) AM_ROM AM_REGION(M68000_TAG, 0)
	AM_RANGE(0x000008, 0x1fffff) AM_RAM
	AM_RANGE(0x200000, 0x3fffff) AM_RAM
	AM_RANGE(0xe00000, 0xe3ffff) AM_ROM AM_REGION(M68000_TAG, 0)
//	AM_RANGE(0xfa0000, 0xfbffff) AM_ROM // cartridge
//	AM_RANGE(0xfc0000, 0xfeffff) AM_ROM
	AM_RANGE(0xff8000, 0xff8001) AM_READWRITE(atarist_mmu_r, atarist_mmu_w)
	AM_RANGE(0xff8200, 0xff8203) AM_READWRITE(atarist_shifter_base_r, atarist_shifter_base_w)
	AM_RANGE(0xff8204, 0xff8209) AM_READWRITE(atariste_shifter_counter_r, atariste_shifter_counter_w)
	AM_RANGE(0xff820a, 0xff820b) AM_READWRITE8(atarist_shifter_sync_r, atarist_shifter_sync_w, 0xff00)
	AM_RANGE(0xff820c, 0xff820d) AM_READWRITE(atariste_shifter_base_low_r, atariste_shifter_base_low_w)
	AM_RANGE(0xff820e, 0xff820f) AM_READWRITE(atariste_shifter_lineofs_r, atariste_shifter_lineofs_w)
	AM_RANGE(0xff8240, 0xff825f) AM_READWRITE(atarist_shifter_palette_r, atariste_shifter_palette_w)
	AM_RANGE(0xff8260, 0xff8261) AM_READWRITE8(atarist_shifter_mode_r, atarist_shifter_mode_w, 0xff00)
	AM_RANGE(0xff8264, 0xff8265) AM_READWRITE(atariste_shifter_pixelofs_r, atariste_shifter_pixelofs_w)
	AM_RANGE(0xff8604, 0xff8605) AM_READWRITE(atarist_fdc_data_r, atarist_fdc_data_w)
	AM_RANGE(0xff8606, 0xff8607) AM_READWRITE(atarist_fdc_dma_status_r, atarist_fdc_dma_mode_w)
	AM_RANGE(0xff8608, 0xff860d) AM_READWRITE(atarist_fdc_dma_base_r, atarist_fdc_dma_base_w)
	AM_RANGE(0xff8800, 0xff8801) AM_DEVREADWRITE8(YM2149_TAG, ay8910_r, ay8910_data_w, 0xff00)
	AM_RANGE(0xff8802, 0xff8803) AM_DEVWRITE8(YM2149_TAG, ay8910_data_w, 0xff00)
	AM_RANGE(0xff8900, 0xff8901) AM_READWRITE(atariste_sound_dma_control_r, atariste_sound_dma_control_w)
	AM_RANGE(0xff8902, 0xff8907) AM_READWRITE(atariste_sound_dma_base_r, atariste_sound_dma_base_w)
	AM_RANGE(0xff8908, 0xff890d) AM_READ(atariste_sound_dma_counter_r)
	AM_RANGE(0xff890e, 0xff8913) AM_READWRITE(atariste_sound_dma_end_r, atariste_sound_dma_end_w)
	AM_RANGE(0xff8920, 0xff8921) AM_READWRITE(atariste_sound_mode_r, atariste_sound_mode_w)
	AM_RANGE(0xff8922, 0xff8923) AM_READWRITE(atariste_microwire_data_r, atariste_microwire_data_w)
	AM_RANGE(0xff8924, 0xff8925) AM_READWRITE(atariste_microwire_mask_r, atariste_microwire_mask_w)
	AM_RANGE(0xff8a00, 0xff8a1f) AM_READWRITE(atarist_blitter_halftone_r, atarist_blitter_halftone_w)
	AM_RANGE(0xff8a20, 0xff8a21) AM_READWRITE(atarist_blitter_src_inc_x_r, atarist_blitter_src_inc_x_w)
	AM_RANGE(0xff8a22, 0xff8a23) AM_READWRITE(atarist_blitter_src_inc_y_r, atarist_blitter_src_inc_y_w)
	AM_RANGE(0xff8a24, 0xff8a27) AM_READWRITE(atarist_blitter_src_r, atarist_blitter_src_w)
	AM_RANGE(0xff8a28, 0xff8a2d) AM_READWRITE(atarist_blitter_end_mask_r, atarist_blitter_end_mask_w)
	AM_RANGE(0xff8a2e, 0xff8a2f) AM_READWRITE(atarist_blitter_dst_inc_x_r, atarist_blitter_dst_inc_x_w)
	AM_RANGE(0xff8a30, 0xff8a31) AM_READWRITE(atarist_blitter_dst_inc_y_r, atarist_blitter_dst_inc_y_w)
	AM_RANGE(0xff8a32, 0xff8a35) AM_READWRITE(atarist_blitter_dst_r, atarist_blitter_dst_w)
	AM_RANGE(0xff8a36, 0xff8a37) AM_READWRITE(atarist_blitter_count_x_r, atarist_blitter_count_x_w)
	AM_RANGE(0xff8a38, 0xff8a39) AM_READWRITE(atarist_blitter_count_y_r, atarist_blitter_count_y_w)
	AM_RANGE(0xff8a3a, 0xff8a3b) AM_READWRITE(atarist_blitter_op_r, atarist_blitter_op_w)
	AM_RANGE(0xff8a3c, 0xff8a3d) AM_READWRITE(atarist_blitter_ctrl_r, atarist_blitter_ctrl_w)
	AM_RANGE(0xff9200, 0xff9201) AM_READ_PORT("JOY0")
	AM_RANGE(0xff9202, 0xff9203) AM_READ_PORT("JOY1")
	AM_RANGE(0xff9210, 0xff9211) AM_READ_PORT("PADDLE0X")
	AM_RANGE(0xff9212, 0xff9213) AM_READ_PORT("PADDLE0Y")
	AM_RANGE(0xff9214, 0xff9215) AM_READ_PORT("PADDLE1X")
	AM_RANGE(0xff9216, 0xff9217) AM_READ_PORT("PADDLE1Y")
	AM_RANGE(0xff9220, 0xff9221) AM_READ_PORT("GUNX")
	AM_RANGE(0xff9222, 0xff9223) AM_READ_PORT("GUNY")
	AM_RANGE(0xfffa00, 0xfffa2f) AM_DEVREADWRITE8(MC68901_TAG, mc68901_register_r, mc68901_register_w, 0xff)
	AM_RANGE(0xfffc00, 0xfffc01) AM_DEVREADWRITE8(MC6850_0_TAG, acia6850_stat_r, acia6850_ctrl_w, 0xff00)
	AM_RANGE(0xfffc02, 0xfffc03) AM_DEVREADWRITE8(MC6850_0_TAG, acia6850_data_r, acia6850_data_w, 0xff00)
	AM_RANGE(0xfffc04, 0xfffc05) AM_DEVREADWRITE8(MC6850_1_TAG, acia6850_stat_r, acia6850_ctrl_w, 0xff00)
	AM_RANGE(0xfffc06, 0xfffc07) AM_DEVREADWRITE8(MC6850_1_TAG, acia6850_data_r, acia6850_data_w, 0xff00)
ADDRESS_MAP_END

static ADDRESS_MAP_START( megaste_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x000007) AM_ROM AM_REGION(M68000_TAG, 0)
	AM_RANGE(0x000008, 0x1fffff) AM_RAM
	AM_RANGE(0x200000, 0x3fffff) AM_RAM
	AM_RANGE(0xe00000, 0xe3ffff) AM_ROM AM_REGION(M68000_TAG, 0)
//	AM_RANGE(0xfa0000, 0xfbffff) AM_ROM // cartridge
//	AM_RANGE(0xfc0000, 0xfeffff) AM_ROM
	AM_RANGE(0xff8000, 0xff8007) AM_READWRITE(atarist_mmu_r, atarist_mmu_w)
	AM_RANGE(0xff8200, 0xff8203) AM_READWRITE(atarist_shifter_base_r, atarist_shifter_base_w)
	AM_RANGE(0xff8204, 0xff8209) AM_READWRITE(atariste_shifter_counter_r, atariste_shifter_counter_w)
	AM_RANGE(0xff820a, 0xff820b) AM_READWRITE8(atarist_shifter_sync_r, atarist_shifter_sync_w, 0xff00)
	AM_RANGE(0xff820c, 0xff820d) AM_READWRITE(atariste_shifter_base_low_r, atariste_shifter_base_low_w)
	AM_RANGE(0xff820e, 0xff820f) AM_READWRITE(atariste_shifter_lineofs_r, atariste_shifter_lineofs_w)
	AM_RANGE(0xff8240, 0xff825f) AM_READWRITE(atarist_shifter_palette_r, atariste_shifter_palette_w)
	AM_RANGE(0xff8260, 0xff8261) AM_READWRITE8(atarist_shifter_mode_r, atarist_shifter_mode_w, 0xff00)
	AM_RANGE(0xff8264, 0xff8265) AM_READWRITE(atariste_shifter_pixelofs_r, atariste_shifter_pixelofs_w)
	AM_RANGE(0xff8604, 0xff8605) AM_READWRITE(atarist_fdc_data_r, atarist_fdc_data_w)
	AM_RANGE(0xff8606, 0xff8607) AM_READWRITE(atarist_fdc_dma_status_r, atarist_fdc_dma_mode_w)
	AM_RANGE(0xff8608, 0xff860d) AM_READWRITE(atarist_fdc_dma_base_r, atarist_fdc_dma_base_w)
	AM_RANGE(0xff8800, 0xff8801) AM_DEVREADWRITE8(YM2149_TAG, ay8910_r, ay8910_data_w, 0xff00)
	AM_RANGE(0xff8802, 0xff8803) AM_DEVWRITE8(YM2149_TAG, ay8910_data_w, 0xff00)
	AM_RANGE(0xff8900, 0xff8901) AM_READWRITE(atariste_sound_dma_control_r, atariste_sound_dma_control_w)
	AM_RANGE(0xff8902, 0xff8907) AM_READWRITE(atariste_sound_dma_base_r, atariste_sound_dma_base_w)
	AM_RANGE(0xff8908, 0xff890d) AM_READ(atariste_sound_dma_counter_r)
	AM_RANGE(0xff890e, 0xff8913) AM_READWRITE(atariste_sound_dma_end_r, atariste_sound_dma_end_w)
	AM_RANGE(0xff8920, 0xff8921) AM_READWRITE(atariste_sound_mode_r, atariste_sound_mode_w)
	AM_RANGE(0xff8922, 0xff8923) AM_READWRITE(atariste_microwire_data_r, atariste_microwire_data_w)
	AM_RANGE(0xff8924, 0xff8925) AM_READWRITE(atariste_microwire_mask_r, atariste_microwire_mask_w)
	AM_RANGE(0xff8a00, 0xff8a1f) AM_READWRITE(atarist_blitter_halftone_r, atarist_blitter_halftone_w)
	AM_RANGE(0xff8a20, 0xff8a21) AM_READWRITE(atarist_blitter_src_inc_x_r, atarist_blitter_src_inc_x_w)
	AM_RANGE(0xff8a22, 0xff8a23) AM_READWRITE(atarist_blitter_src_inc_y_r, atarist_blitter_src_inc_y_w)
	AM_RANGE(0xff8a24, 0xff8a27) AM_READWRITE(atarist_blitter_src_r, atarist_blitter_src_w)
	AM_RANGE(0xff8a28, 0xff8a2d) AM_READWRITE(atarist_blitter_end_mask_r, atarist_blitter_end_mask_w)
	AM_RANGE(0xff8a2e, 0xff8a2f) AM_READWRITE(atarist_blitter_dst_inc_x_r, atarist_blitter_dst_inc_x_w)
	AM_RANGE(0xff8a30, 0xff8a31) AM_READWRITE(atarist_blitter_dst_inc_y_r, atarist_blitter_dst_inc_y_w)
	AM_RANGE(0xff8a32, 0xff8a35) AM_READWRITE(atarist_blitter_dst_r, atarist_blitter_dst_w)
	AM_RANGE(0xff8a36, 0xff8a37) AM_READWRITE(atarist_blitter_count_x_r, atarist_blitter_count_x_w)
	AM_RANGE(0xff8a38, 0xff8a39) AM_READWRITE(atarist_blitter_count_y_r, atarist_blitter_count_y_w)
	AM_RANGE(0xff8a3a, 0xff8a3b) AM_READWRITE(atarist_blitter_op_r, atarist_blitter_op_w)
	AM_RANGE(0xff8a3c, 0xff8a3d) AM_READWRITE(atarist_blitter_ctrl_r, atarist_blitter_ctrl_w)
	AM_RANGE(0xff8e20, 0xff8e21) AM_READWRITE(megaste_cache_r, megaste_cache_w)
	AM_RANGE(0xfffa00, 0xfffa3f) AM_DEVREADWRITE8(MC68901_TAG, mc68901_register_r, mc68901_register_w, 0xff)
//  AM_RANGE(0xfffa40, 0xfffa5f) AM_READWRITE(megast_fpu_r, megast_fpu_w)
	AM_RANGE(0xff8c80, 0xff8c87) AM_DEVREADWRITE8("scc", scc8530_r, scc8530_w, 0xff00)
	AM_RANGE(0xfffc00, 0xfffc01) AM_DEVREADWRITE8(MC6850_0_TAG, acia6850_stat_r, acia6850_ctrl_w, 0xff00)
	AM_RANGE(0xfffc02, 0xfffc03) AM_DEVREADWRITE8(MC6850_0_TAG, acia6850_data_r, acia6850_data_w, 0xff00)
	AM_RANGE(0xfffc04, 0xfffc05) AM_DEVREADWRITE8(MC6850_1_TAG, acia6850_stat_r, acia6850_ctrl_w, 0xff00)
	AM_RANGE(0xfffc06, 0xfffc07) AM_DEVREADWRITE8(MC6850_1_TAG, acia6850_data_r, acia6850_data_w, 0xff00)
	AM_RANGE(0xfffc20, 0xfffc3f) AM_DEVREADWRITE(RP5C15_TAG, rp5c15_r, rp5c15_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( stbook_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x000007) AM_ROM AM_REGION(M68000_TAG, 0)
	AM_RANGE(0x000008, 0x1fffff) AM_RAM
	AM_RANGE(0x200000, 0x3fffff) AM_RAM
//	AM_RANGE(0xd40000, 0xd7ffff) AM_ROM
	AM_RANGE(0xe00000, 0xe3ffff) AM_ROM AM_REGION(M68000_TAG, 0)
//	AM_RANGE(0xe80000, 0xebffff) AM_ROM
//  AM_RANGE(0xf00000, 0xf1ffff) AM_READWRITE(stbook_ide_r, stbook_ide_w)
//	AM_RANGE(0xfa0000, 0xfbffff) AM_ROM // cartridge
//	AM_RANGE(0xfc0000, 0xfeffff) AM_ROM
/*  AM_RANGE(0xff8000, 0xff8001) AM_READWRITE(stbook_mmu_r, stbook_mmu_w)
    AM_RANGE(0xff8200, 0xff8203) AM_READWRITE(stbook_shifter_base_r, stbook_shifter_base_w)
    AM_RANGE(0xff8204, 0xff8209) AM_READWRITE(stbook_shifter_counter_r, stbook_shifter_counter_w)
    AM_RANGE(0xff820a, 0xff820b) AM_READWRITE8(stbook_shifter_sync_r, stbook_shifter_sync_w, 0xff00)
    AM_RANGE(0xff820c, 0xff820d) AM_READWRITE(stbook_shifter_base_low_r, stbook_shifter_base_low_w)
    AM_RANGE(0xff820e, 0xff820f) AM_READWRITE(stbook_shifter_lineofs_r, stbook_shifter_lineofs_w)
    AM_RANGE(0xff8240, 0xff8241) AM_READWRITE(stbook_shifter_palette_r, stbook_shifter_palette_w)
    AM_RANGE(0xff8260, 0xff8261) AM_READWRITE8(stbook_shifter_mode_r, stbook_shifter_mode_w, 0xff00)
    AM_RANGE(0xff8264, 0xff8265) AM_READWRITE(stbook_shifter_pixelofs_r, stbook_shifter_pixelofs_w)*/
	AM_RANGE(0xff827e, 0xff827f) AM_WRITE(stbook_lcd_control_w)
	AM_RANGE(0xff8604, 0xff8605) AM_READWRITE(atarist_fdc_data_r, atarist_fdc_data_w)
	AM_RANGE(0xff8606, 0xff8607) AM_READWRITE(atarist_fdc_dma_status_r, atarist_fdc_dma_mode_w)
	AM_RANGE(0xff8608, 0xff860d) AM_READWRITE(atarist_fdc_dma_base_r, atarist_fdc_dma_base_w)
	AM_RANGE(0xff8800, 0xff8801) AM_DEVREADWRITE8(YM3439_TAG, ay8910_r, ay8910_data_w, 0xff00)
	AM_RANGE(0xff8802, 0xff8803) AM_DEVWRITE8(YM3439_TAG, ay8910_data_w, 0xff00)
	AM_RANGE(0xff8900, 0xff8901) AM_READWRITE(atariste_sound_dma_control_r, atariste_sound_dma_control_w)
	AM_RANGE(0xff8902, 0xff8907) AM_READWRITE(atariste_sound_dma_base_r, atariste_sound_dma_base_w)
	AM_RANGE(0xff8908, 0xff890d) AM_READ(atariste_sound_dma_counter_r)
	AM_RANGE(0xff890e, 0xff8913) AM_READWRITE(atariste_sound_dma_end_r, atariste_sound_dma_end_w)
	AM_RANGE(0xff8920, 0xff8921) AM_READWRITE(atariste_sound_mode_r, atariste_sound_mode_w)
	AM_RANGE(0xff8922, 0xff8923) AM_READWRITE(atariste_microwire_data_r, atariste_microwire_data_w)
	AM_RANGE(0xff8924, 0xff8925) AM_READWRITE(atariste_microwire_mask_r, atariste_microwire_mask_w)
	AM_RANGE(0xff8a00, 0xff8a1f) AM_READWRITE(atarist_blitter_halftone_r, atarist_blitter_halftone_w)
	AM_RANGE(0xff8a20, 0xff8a21) AM_READWRITE(atarist_blitter_src_inc_x_r, atarist_blitter_src_inc_x_w)
	AM_RANGE(0xff8a22, 0xff8a23) AM_READWRITE(atarist_blitter_src_inc_y_r, atarist_blitter_src_inc_y_w)
	AM_RANGE(0xff8a24, 0xff8a27) AM_READWRITE(atarist_blitter_src_r, atarist_blitter_src_w)
	AM_RANGE(0xff8a28, 0xff8a2d) AM_READWRITE(atarist_blitter_end_mask_r, atarist_blitter_end_mask_w)
	AM_RANGE(0xff8a2e, 0xff8a2f) AM_READWRITE(atarist_blitter_dst_inc_x_r, atarist_blitter_dst_inc_x_w)
	AM_RANGE(0xff8a30, 0xff8a31) AM_READWRITE(atarist_blitter_dst_inc_y_r, atarist_blitter_dst_inc_y_w)
	AM_RANGE(0xff8a32, 0xff8a35) AM_READWRITE(atarist_blitter_dst_r, atarist_blitter_dst_w)
	AM_RANGE(0xff8a36, 0xff8a37) AM_READWRITE(atarist_blitter_count_x_r, atarist_blitter_count_x_w)
	AM_RANGE(0xff8a38, 0xff8a39) AM_READWRITE(atarist_blitter_count_y_r, atarist_blitter_count_y_w)
	AM_RANGE(0xff8a3a, 0xff8a3b) AM_READWRITE(atarist_blitter_op_r, atarist_blitter_op_w)
	AM_RANGE(0xff8a3c, 0xff8a3d) AM_READWRITE(atarist_blitter_ctrl_r, atarist_blitter_ctrl_w)
	AM_RANGE(0xff9200, 0xff9201) AM_READ(stbook_config_r)
/*  AM_RANGE(0xff9202, 0xff9203) AM_READWRITE(stbook_lcd_contrast_r, stbook_lcd_contrast_w)
    AM_RANGE(0xff9210, 0xff9211) AM_READWRITE(stbook_power_r, stbook_power_w)
    AM_RANGE(0xff9214, 0xff9215) AM_READWRITE(stbook_reference_r, stbook_reference_w)*/
	AM_RANGE(0xfffa00, 0xfffa2f) AM_DEVREADWRITE8(MC68901_TAG, mc68901_register_r, mc68901_register_w, 0xff)
	AM_RANGE(0xfffc00, 0xfffc01) AM_DEVREADWRITE8(MC6850_0_TAG, acia6850_stat_r, acia6850_ctrl_w, 0xff00)
	AM_RANGE(0xfffc02, 0xfffc03) AM_DEVREADWRITE8(MC6850_0_TAG, acia6850_data_r, acia6850_data_w, 0xff00)
	AM_RANGE(0xfffc04, 0xfffc05) AM_DEVREADWRITE8(MC6850_1_TAG, acia6850_stat_r, acia6850_ctrl_w, 0xff00)
	AM_RANGE(0xfffc06, 0xfffc07) AM_DEVREADWRITE8(MC6850_1_TAG, acia6850_data_r, acia6850_data_w, 0xff00)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( ikbd )
	PORT_START("P31")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Control") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0xef, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("P32")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1) PORT_NAME("F1")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0xde, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("P33")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2) PORT_NAME("F2")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(DEF_STR( Alternate )) PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0xbe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("P34")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3) PORT_NAME("F3")
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("P35")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F4) PORT_NAME("F4")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')

	PORT_START("P36")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F5) PORT_NAME("F5")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START("P37")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F6) PORT_NAME("F6")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')

	PORT_START("P40")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F7) PORT_NAME("F7")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')

	PORT_START("P41")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F8) PORT_NAME("F8")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START("P42")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F9) PORT_NAME("F9")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("P43")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F10) PORT_NAME("F10")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00B4) PORT_CHAR('`')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))

	PORT_START("P44")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Help") PORT_CODE(KEYCODE_F11)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Insert") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')

	PORT_START("P45")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Undo") PORT_CODE(KEYCODE_F12)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Clr Home") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD)

	PORT_START("P46")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad (")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad )")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_DEL_PAD)

	PORT_START("P47")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad /") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad *") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad -") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad +") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad Enter") PORT_CODE(KEYCODE_ENTER_PAD)
INPUT_PORTS_END

static INPUT_PORTS_START( st )
	PORT_START("config")
	PORT_CONFNAME( 0x01, 0x00, "Input Port 0 Device")
	PORT_CONFSETTING( 0x00, "Mouse" )
	PORT_CONFSETTING( 0x01, DEF_STR( Joystick ) )
	PORT_CONFNAME( 0x80, 0x80, "Monitor")
	PORT_CONFSETTING( 0x00, "Monochrome" )
	PORT_CONFSETTING( 0x80, "Color" )

	PORT_INCLUDE( ikbd )

	PORT_START("IKBD_JOY0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY // XB
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY // XA
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY // YA
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY // YB
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY

	PORT_START("IKBD_JOY1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START("IKBD_MOUSEX")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1)

	PORT_START("IKBD_MOUSEY")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1)
INPUT_PORTS_END

static INPUT_PORTS_START( ste )
	PORT_START("config")
	PORT_CONFNAME( 0x01, 0x00, "Input Port 0 Device")
	PORT_CONFSETTING( 0x00, "Mouse" )
	PORT_CONFSETTING( 0x01, DEF_STR( Joystick ) )
	PORT_CONFNAME( 0x80, 0x80, "Monitor")
	PORT_CONFSETTING( 0x00, "Monochrome" )
	PORT_CONFSETTING( 0x80, "Color" )

	PORT_INCLUDE( ikbd )

	PORT_START("JOY0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOY1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(3) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(4) PORT_8WAY
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(4) PORT_8WAY

	PORT_START("PADDLE0X")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START("PADDLE0Y")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE_V ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START("PADDLE1X")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(2)

	PORT_START("PADDLE1Y")
	PORT_BIT( 0xff, 0x00, IPT_PADDLE_V ) PORT_SENSITIVITY(30) PORT_KEYDELTA(15) PORT_PLAYER(2)

	PORT_START("GUNX") // should be 10-bit
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START("GUNY") // should be 10-bit
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

static INPUT_PORTS_START( stbook )
	PORT_START("SW400")
	PORT_DIPNAME( 0x80, 0x80, "DMA sound hardware")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, "WD1772 FDC")
	PORT_DIPSETTING( 0x40, "Low Speed (8 MHz)" )
	PORT_DIPSETTING( 0x00, "High Speed (16 MHz)" )
	PORT_DIPNAME( 0x20, 0x00, "Bypass Self Test")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Yes ) )
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( tt030 )
INPUT_PORTS_END

static INPUT_PORTS_START( falcon )
INPUT_PORTS_END

/* Sound Interface */

static WRITE8_DEVICE_HANDLER( ym2149_port_a_w )
{
	atarist_state *state = device->machine->driver_data<atarist_state>();

	wd17xx_set_side(state->wd1772, BIT(data, 0) ? 0 : 1);

	if (!BIT(data, 1))
	{
		wd17xx_set_drive(state->wd1772, 0);
	}

	if (!BIT(data, 2))
	{
		wd17xx_set_drive(state->wd1772, 1);
	}

	rs232_rts_w(state->rs232, BIT(data, 3));
	rs232_dtr_w(state->rs232, BIT(data, 4));

	centronics_strobe_w(state->centronics, BIT(data, 5));

	// 0x40 = General Purpose Output
	// 0x80 = Reserved
}

static const ay8910_interface psg_intf =
{
	AY8910_SINGLE_OUTPUT,
	{ RES_K(1) },
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(ym2149_port_a_w),
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w)
};

/* Machine Drivers */

static READ_LINE_DEVICE_HANDLER( ikbd_rx )
{
	atarist_state *driver_state = device->machine->driver_data<atarist_state>();

	return driver_state->ikbd_rx;
}

static WRITE_LINE_DEVICE_HANDLER( ikbd_tx )
{
	atarist_state *driver_state = device->machine->driver_data<atarist_state>();

	driver_state->ikbd_tx = state;
}

static READ_LINE_DEVICE_HANDLER( midi_rx )
{
	atarist_state *driver_state = device->machine->driver_data<atarist_state>();

	return driver_state->midi_rx;
}

static WRITE_LINE_DEVICE_HANDLER( midi_tx )
{
	atarist_state *driver_state = device->machine->driver_data<atarist_state>();

	driver_state->midi_tx = state;
}

static WRITE_LINE_DEVICE_HANDLER( acia_interrupt )
{
	atarist_state *driver_state = device->machine->driver_data<atarist_state>();

	driver_state->acia_irq = state;
}

static ACIA6850_INTERFACE( acia_ikbd_intf )
{
	Y2/64,
	Y2/64,
	DEVCB_LINE(ikbd_rx),
	DEVCB_LINE(ikbd_tx),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(acia_interrupt)
};

static ACIA6850_INTERFACE( acia_midi_intf )
{
	Y2/64,
	Y2/64,
	DEVCB_LINE(midi_rx),
	DEVCB_LINE(midi_tx),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(acia_interrupt)
};

static IRQ_CALLBACK( atarist_int_ack )
{
	atarist_state *state = device->machine->driver_data<atarist_state>();

	if (irqline == M68K_IRQ_6)
	{
		return mc68901_get_vector(state->mc68901);
	}

	return M68K_INT_ACK_AUTOVECTOR;
}

static READ8_DEVICE_HANDLER( mfp_gpio_r )
{
	/*

        bit     description

        0       Centronics BUSY
        1       RS232 DCD
        2       RS232 CTS
        3       Blitter done
        4       Keyboard/MIDI
        5       FDC
        6       RS232 RI
        7       Monochrome monitor detect

    */

	atarist_state *state = device->machine->driver_data<atarist_state>();

	UINT8 data = centronics_busy_r(state->centronics);

	mc68901_tai_w(device, data & 0x01);

	data |= rs232_dcd_r(state->rs232) << 1;
	data |= rs232_cts_r(state->rs232) << 2;
	data |= (state->acia_irq << 4);
	data |= (state->fdc_irq << 5);
	data |= rs232_ri_r(state->rs232) << 6;
	data |= (input_port_read(device->machine, "config") & 0x80);

	return data;
}

static WRITE8_DEVICE_HANDLER( mfp_tdo_w )
{
	mc68901_rx_clock_w(device, data);
	mc68901_tx_clock_w(device, data);
}

static WRITE_LINE_DEVICE_HANDLER( mfp_so_w )
{
	atarist_state *driver_state = device->machine->driver_data<atarist_state>();

	rs232_td_w(driver_state->rs232, device, state);
}

static MC68901_INTERFACE( mfp_intf )
{
	Y1,													/* timer clock */
	0,													/* receive clock */
	0,													/* transmit clock */
	DEVCB_DEVICE_HANDLER(MC68901_TAG, mfp_gpio_r),		/* GPIO read */
	DEVCB_NULL,											/* GPIO write */
	DEVCB_DEVICE_LINE(RS232_TAG, rs232_rd_r),			/* serial input */
	DEVCB_LINE(mfp_so_w),								/* serial output */
	DEVCB_NULL,											/* TAO */
	DEVCB_NULL,											/* TBO */
	DEVCB_NULL,											/* TCO */
	DEVCB_DEVICE_HANDLER(MC68901_TAG, mfp_tdo_w),		/* TDO */
	DEVCB_CPU_INPUT_LINE(M68000_TAG, M68K_IRQ_6)		/* interrupt */
};

static void atarist_configure_memory(running_machine *machine)
{
	address_space *program = cputag_get_address_space(machine, M68000_TAG, ADDRESS_SPACE_PROGRAM);
	UINT8 *RAM = messram_get_ptr(machine->device("messram"));

	switch (messram_get_size(machine->device("messram")))
	{
	case 256 * 1024:
		memory_install_ram(program, 0x000008, 0x03ffff, 0, 0, RAM + 8);
		memory_unmap_readwrite(program, 0x040000, 0x3fffff, 0, 0);
		break;
	case 512 * 1024:
		memory_install_ram(program, 0x000008, 0x07ffff, 0, 0, RAM + 8);
		memory_unmap_readwrite(program, 0x080000, 0x3fffff, 0, 0);
		break;
	case 1024 * 1024:
		memory_install_ram(program, 0x000008, 0x0fffff, 0, 0, RAM + 8);
		memory_unmap_readwrite(program, 0x100000, 0x3fffff, 0, 0);
		break;
	case 2048 * 1024:
		memory_install_ram(program, 0x000008, 0x1fffff, 0, 0, RAM + 8);
		memory_unmap_readwrite(program, 0x200000, 0x3fffff, 0, 0);
		break;
	case 4096 * 1024:
		memory_install_ram(program, 0x000008, 0x3fffff, 0, 0, RAM + 8);
		break;
	}
}

static void atarist_state_save(running_machine *machine)
{
	atarist_state *state = machine->driver_data<atarist_state>();

	state->fdc_status |= ATARIST_FLOPPY_STATUS_DMA_ERROR;

	state_save_register_global(machine, state->mmu);
	state_save_register_global(machine, state->fdc_dmabase);
	state_save_register_global(machine, state->fdc_status);
	state_save_register_global(machine, state->fdc_mode);
	state_save_register_global(machine, state->fdc_sectors);
	state_save_register_global(machine, state->fdc_dmabytes);
	state_save_register_global(machine, state->fdc_irq);
	state_save_register_global(machine, state->ikbd_keylatch);
	state_save_register_global(machine, state->ikbd_mouse_x);
	state_save_register_global(machine, state->ikbd_mouse_y);
	state_save_register_global(machine, state->ikbd_mouse_px);
	state_save_register_global(machine, state->ikbd_mouse_py);
	state_save_register_global(machine, state->ikbd_mouse_pc);
	state_save_register_global(machine, state->ikbd_rx);
	state_save_register_global(machine, state->ikbd_tx);
	state_save_register_global(machine, state->midi_rx);
	state_save_register_global(machine, state->midi_tx);
	state_save_register_global(machine, state->acia_irq);
}

static MACHINE_START( atarist )
{
	atarist_state *state = machine->driver_data<atarist_state>();

	/* configure RAM banking */
	atarist_configure_memory(machine);

	/* set CPU interrupt callback */
	cpu_set_irq_callback(machine->device(M68000_TAG), atarist_int_ack);

	/* find devices */
	state->mc68901 = machine->device(MC68901_TAG);
	state->wd1772 = machine->device(WD1772_TAG);
	state->centronics = machine->device(CENTRONICS_TAG);
	state->rs232 = machine->device(RS232_TAG);

	/* register for state saving */
	atarist_state_save(machine);
}

static const struct rp5c15_interface rtc_intf =
{
	NULL
};

static MACHINE_START( megast )
{
	MACHINE_START_CALL(atarist);
}

static READ8_DEVICE_HANDLER( atariste_mfp_gpio_r )
{
	/*

        bit     description

        0       Centronics BUSY
        1       RS232 DCD
        2       RS232 CTS
        3       Blitter done
        4       Keyboard/MIDI
        5       FDC
        6       RS232 RI
        7       Monochrome monitor detect / DMA sound active

    */

	atarist_state *state = device->machine->driver_data<atarist_state>();

	UINT8 data = centronics_busy_r(state->centronics);

	data |= rs232_dcd_r(state->rs232) << 1;
	data |= rs232_cts_r(state->rs232) << 2;
	data |= (state->acia_irq << 4);
	data |= (state->fdc_irq << 5);
	data |= rs232_ri_r(state->rs232) << 6;
	data |= (input_port_read(device->machine, "config") & 0x80) ^ (state->dmasnd_active << 7);

	return data;
}

static MC68901_INTERFACE( atariste_mfp_intf )
{
	Y1,													/* timer clock */
	0,													/* receive clock */
	0,													/* transmit clock */
	DEVCB_DEVICE_HANDLER(MC68901_TAG, atariste_mfp_gpio_r),	/* GPIO read */
	DEVCB_NULL,											/* GPIO write */
	DEVCB_DEVICE_LINE(RS232_TAG, rs232_rd_r),			/* serial input */
	DEVCB_LINE(mfp_so_w),								/* serial output */
	DEVCB_NULL,											/* TAO */
	DEVCB_NULL,											/* TBO */
	DEVCB_NULL,											/* TCO */
	DEVCB_DEVICE_HANDLER(MC68901_TAG, mfp_tdo_w),	/* TDO */
	DEVCB_CPU_INPUT_LINE(M68000_TAG, M68K_IRQ_6)		/* interrupt */
};

static void atariste_state_save(running_machine *machine)
{
	atarist_state *state = machine->driver_data<atarist_state>();

	atarist_state_save(machine);

	state_save_register_global(machine, state->dmasnd_base);
	state_save_register_global(machine, state->dmasnd_end);
	state_save_register_global(machine, state->dmasnd_cntr);
	state_save_register_global(machine, state->dmasnd_baselatch);
	state_save_register_global(machine, state->dmasnd_endlatch);
	state_save_register_global(machine, state->dmasnd_ctrl);
	state_save_register_global(machine, state->dmasnd_mode);
	state_save_register_global_array(machine, state->dmasnd_fifo);
	state_save_register_global(machine, state->dmasnd_samples);
	state_save_register_global(machine, state->dmasnd_active);
	state_save_register_global(machine, state->mw_data);
	state_save_register_global(machine, state->mw_mask);
	state_save_register_global(machine, state->mw_shift);
}

static MACHINE_START( atariste )
{
	atarist_state *state = machine->driver_data<atarist_state>();

	/* configure RAM banking */
	atarist_configure_memory(machine);

	/* set CPU interrupt callback */
	cpu_set_irq_callback(machine->device(M68000_TAG), atarist_int_ack);

	/* allocate timers */
	state->dmasound_timer = timer_alloc(machine, atariste_dmasound_tick, NULL);
	state->microwire_timer = timer_alloc(machine, atariste_microwire_tick, NULL);

	/* find devices */
	state->mc68901 = machine->device(MC68901_TAG);
	state->wd1772 = machine->device(WD1772_TAG);
	state->lmc1992 = machine->device(LMC1992_TAG);
	state->centronics = machine->device(CENTRONICS_TAG);
	state->rs232 = machine->device(RS232_TAG);

	/* register for state saving */
	atariste_state_save(machine);
}

static MACHINE_START( megaste )
{
	atarist_state *state = machine->driver_data<atarist_state>();

	MACHINE_START_CALL(atariste);

	state_save_register_global(machine, state->megaste_cache);
}

static void stbook_configure_memory(running_machine *machine)
{
	address_space *program = cputag_get_address_space(machine, M68000_TAG, ADDRESS_SPACE_PROGRAM);
	UINT8 *RAM = messram_get_ptr(machine->device("messram"));

	switch (messram_get_size(machine->device("messram")))
	{
	case 1024 * 1024:
		memory_install_ram(program, 0x000008, 0x07ffff, 0, 0x080000, RAM + 8);
		memory_unmap_readwrite(program, 0x100000, 0x3fffff, 0, 0);
		break;
	case 4096 * 1024:
		memory_install_ram(program, 0x000008, 0x3fffff, 0, 0, RAM + 8);
		break;
	}

}

static WRITE8_DEVICE_HANDLER( stbook_ym2149_port_a_w )
{
	atarist_state *state = device->machine->driver_data<atarist_state>();

	wd17xx_set_side(state->wd1772, (data & 0x01) ? 0 : 1);

	if (!(data & 0x02))
	{
		wd17xx_set_drive(state->wd1772, 0);
	}

	if (!(data & 0x04))
	{
		wd17xx_set_drive(state->wd1772, 1);
	}

	rs232_rts_w(state->rs232, BIT(data, 3));
	rs232_dtr_w(state->rs232, BIT(data, 4));

	centronics_strobe_w(state->centronics, BIT(data, 5));

	// 0x40 = IDE RESET

	wd17xx_dden_w(state->wd1772, BIT(data, 7));
}

static const ay8910_interface stbook_psg_intf =
{
	AY8910_SINGLE_OUTPUT,
	{ RES_K(1) },
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(stbook_ym2149_port_a_w),
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w)
};

static ACIA6850_INTERFACE( stbook_acia_ikbd_intf )
{
	U517/2/16, // 500kHz
	U517/2/2, // 1MHZ
	DEVCB_LINE(ikbd_rx),
	DEVCB_LINE(ikbd_tx),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(acia_interrupt)
};

static READ8_DEVICE_HANDLER( stbook_mfp_gpio_r )
{
	/*

        bit     description

        0       Centronics BUSY
        1       RS232 DCD
        2       RS232 CTS
        3       Blitter done
        4       Keyboard/MIDI
        5       FDC
        6       RS232 RI
        7       POWER ALARMS

    */

	atarist_state *state = device->machine->driver_data<atarist_state>();

	UINT8 data = centronics_busy_r(state->centronics);

	data |= rs232_dcd_r(state->rs232) << 1;
	data |= rs232_cts_r(state->rs232) << 2;
	data |= (state->acia_irq << 4);
	data |= (state->fdc_irq << 5);
	data |= rs232_ri_r(state->rs232) << 6;

	return data;
}

static MC68901_INTERFACE( stbook_mfp_intf )
{
	Y1,													/* timer clock */
	0,													/* receive clock */
	0,													/* transmit clock */
	DEVCB_DEVICE_HANDLER(MC68901_TAG, stbook_mfp_gpio_r),	/* GPIO read */
	DEVCB_NULL,											/* GPIO write */
	DEVCB_DEVICE_LINE(RS232_TAG, rs232_rd_r),			/* serial input */
	DEVCB_LINE(mfp_so_w),								/* serial output */
	DEVCB_NULL,											/* TAO */
	DEVCB_NULL,											/* TBO */
	DEVCB_NULL,											/* TCO */
	DEVCB_DEVICE_HANDLER(MC68901_TAG, mfp_tdo_w),		/* TDO */
	DEVCB_CPU_INPUT_LINE(M68000_TAG, M68K_IRQ_6)		/* interrupt */
};

static MACHINE_START( stbook )
{
	atarist_state *state = machine->driver_data<atarist_state>();

	/* configure RAM banking */
	stbook_configure_memory(machine);

	/* set CPU interrupt callback */
	cpu_set_irq_callback(machine->device(M68000_TAG), atarist_int_ack);

	/* find devices */
	state->mc68901 = machine->device(MC68901_TAG);
	state->wd1772 = machine->device(WD1772_TAG);
	state->centronics = machine->device(CENTRONICS_TAG);
	state->rs232 = machine->device(RS232_TAG);

	/* register for state saving */
	atariste_state_save(machine);
}

static DEVICE_IMAGE_LOAD( atarist_cart )
{
	return IMAGE_INIT_FAIL;
}

static const floppy_config atarist_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_3_5_DSDD,
	FLOPPY_OPTIONS_NAME(atarist),
	NULL
};

static const floppy_config megaste_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_3_5_DSHD,
	FLOPPY_OPTIONS_NAME(atarist),
	NULL
};

static MACHINE_CONFIG_FRAGMENT( atarist_cartslot )
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("stc")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(atarist_cart)
MACHINE_CONFIG_END

static RS232_INTERFACE( rs232_intf )
{
	{ MC68901_TAG },
	{ NULL }
};

static MACHINE_CONFIG_START( st, atarist_state )
	// basic machine hardware
	MDRV_CPU_ADD(M68000_TAG, M68000, Y2/4)
	MDRV_CPU_PROGRAM_MAP(st_map)

	MDRV_CPU_ADD(HD6301_TAG, HD63701, XTAL_4MHz)  /* HD6301 */
	MDRV_CPU_PROGRAM_MAP(ikbd_map)
	MDRV_CPU_IO_MAP(ikbd_io_map)

	MDRV_MACHINE_START(atarist)

	// device hardware
	MDRV_MC68901_ADD(MC68901_TAG, Y2/8, mfp_intf)
	MDRV_SCC8530_ADD("scc", Y2/4)
	MDRV_RS232_ADD(RS232_TAG, rs232_intf)

	// video hardware
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_PALETTE_LENGTH(16)
	MDRV_VIDEO_START( atarist )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_SCREEN_RAW_PARAMS(Y2/4, ATARIST_HTOT_PAL, ATARIST_HBEND_PAL, ATARIST_HBSTART_PAL, ATARIST_VTOT_PAL, ATARIST_VBEND_PAL, ATARIST_VBSTART_PAL)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2149_TAG, YM2149, Y2/16)
	MDRV_SOUND_CONFIG(psg_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)

	/* acia */
	MDRV_ACIA6850_ADD(MC6850_0_TAG, acia_ikbd_intf)
	MDRV_ACIA6850_ADD(MC6850_1_TAG, acia_midi_intf)

	MDRV_WD1772_ADD(WD1772_TAG, fdc_intf )

	MDRV_FLOPPY_2_DRIVES_ADD(atarist_floppy_config)

	MDRV_FRAGMENT_ADD(atarist_cartslot)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1024K")  // 1040ST
	MDRV_RAM_EXTRA_OPTIONS("512K,256K") // 520ST, 260ST
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( megast, st )
	MDRV_CPU_MODIFY(M68000_TAG)
	MDRV_CPU_PROGRAM_MAP(megast_map)
	MDRV_RP5C15_ADD(RP5C15_TAG, rtc_intf)

	MDRV_MACHINE_START(megast)

	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("4M")  //  Mega ST 4
	MDRV_RAM_EXTRA_OPTIONS("2M,1M") //  Mega ST 2 ,Mega ST 1
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( ste, atarist_state )
	// basic machine hardware
	MDRV_CPU_ADD(M68000_TAG, M68000, Y2/4)
	MDRV_CPU_PROGRAM_MAP(ste_map)

	MDRV_CPU_ADD(HD6301_TAG, HD63701, XTAL_4MHz)  /* HD6301 */
	MDRV_CPU_PROGRAM_MAP(ikbd_map)
	MDRV_CPU_IO_MAP(ikbd_io_map)

	MDRV_MACHINE_START(atariste)

	// device hardware
	MDRV_MC68901_ADD(MC68901_TAG, Y2/8, atariste_mfp_intf)
	MDRV_SCC8530_ADD("scc", Y2/4)
	MDRV_RS232_ADD(RS232_TAG, rs232_intf)

	// video hardware
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_PALETTE_LENGTH(512)
	MDRV_VIDEO_START( atarist )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_SCREEN_RAW_PARAMS(Y2/4, ATARIST_HTOT_PAL, ATARIST_HBEND_PAL, ATARIST_HBSTART_PAL, ATARIST_VTOT_PAL, ATARIST_VBEND_PAL, ATARIST_VBSTART_PAL)

	// sound hardware
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MDRV_SOUND_ADD(YM2149_TAG, YM2149, Y2/16)
	MDRV_SOUND_CONFIG(psg_intf)
	MDRV_SOUND_ROUTE(0, "lspeaker", 0.50)
	MDRV_SOUND_ROUTE(0, "rspeaker", 0.50)
/*
    MDRV_SOUND_ADD("custom", CUSTOM, 0) // DAC
    MDRV_SOUND_ROUTE(0, "rspeaker", 0.50)
    MDRV_SOUND_ROUTE(1, "lspeaker", 0.50)
*/
	MDRV_LMC1992_ADD(LMC1992_TAG /* ,atariste_lmc1992_intf */)

	/* printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)

	/* acia */
	MDRV_ACIA6850_ADD(MC6850_0_TAG, acia_ikbd_intf)
	MDRV_ACIA6850_ADD(MC6850_1_TAG, acia_midi_intf)

	MDRV_WD1772_ADD(WD1772_TAG, fdc_intf )

	MDRV_FLOPPY_2_DRIVES_ADD(atarist_floppy_config)

	MDRV_FRAGMENT_ADD(atarist_cartslot)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1024K")  // 1040STe
	MDRV_RAM_EXTRA_OPTIONS("512K") //  520STe
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( megaste, ste )
	MDRV_CPU_MODIFY(M68000_TAG)
	MDRV_CPU_PROGRAM_MAP(megaste_map)
	MDRV_RP5C15_ADD(RP5C15_TAG, rtc_intf)

	MDRV_MACHINE_START(megaste)

	MDRV_FLOPPY_2_DRIVES_MODIFY(megaste_floppy_config)

	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("4M")  //  Mega STe 4
	MDRV_RAM_EXTRA_OPTIONS("2M,1M") //  Mega STe 2 ,Mega STe 1
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( stbook, atarist_state )
	// basic machine hardware
	MDRV_CPU_ADD(M68000_TAG, M68000, U517/2)
	MDRV_CPU_PROGRAM_MAP(stbook_map)

	//MDRV_CPU_ADD(COP888_TAG, COP888, Y700)

	MDRV_MACHINE_START(stbook)

	// device hardware
	MDRV_MC68901_ADD(MC68901_TAG, U517/8, stbook_mfp_intf)
	MDRV_RP5C15_ADD(RP5C15_TAG, rtc_intf)
	MDRV_SCC8530_ADD("scc", U517/2)
	MDRV_RS232_ADD(RS232_TAG, rs232_intf)

	// video hardware
	MDRV_SCREEN_ADD(SCREEN_TAG, LCD)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 399)
	MDRV_PALETTE_LENGTH(2)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)
	MDRV_PALETTE_INIT(black_and_white)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM3439_TAG, YM3439, U517/8)
	MDRV_SOUND_CONFIG(stbook_psg_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)

	/* acia */
	MDRV_ACIA6850_ADD(MC6850_0_TAG, stbook_acia_ikbd_intf)
	MDRV_ACIA6850_ADD(MC6850_1_TAG, acia_midi_intf)

	MDRV_WD1772_ADD(WD1772_TAG, stbook_fdc_intf )

	MDRV_FLOPPY_2_DRIVES_ADD(megaste_floppy_config)

	MDRV_FRAGMENT_ADD(atarist_cartslot)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("4M")
	MDRV_RAM_EXTRA_OPTIONS("1M")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( tt030, st )
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( falcon, st )
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( falcon40, st )
MACHINE_CONFIG_END

/* ROMs */

ROM_START( st )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos099", "TOS 0.99 (Disk TOS)" )
	ROMX_LOAD( "tos099.bin", 0x00000, 0x04000, CRC(cee3c664) SHA1(80c10b31b63b906395151204ec0a4984c8cb98d6), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos100", "TOS 1.0 (ROM TOS)" )
	ROMX_LOAD( "tos100.bin", 0x00000, 0x30000, BAD_DUMP CRC(d331af30) SHA1(7bcc2311d122f451bd03c9763ade5a119b2f90da), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102.bin", 0x00000, 0x30000, BAD_DUMP CRC(d3c32283) SHA1(735793fdba07fe8d5295caa03484f6ef3de931f5), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104.bin", 0x00000, 0x30000, BAD_DUMP CRC(90f4fbff) SHA1(2487f330b0895e5d88d580d4ecb24061125e88ad), ROM_BIOS(4) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( st_uk )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos100", "TOS 1.0 (ROM TOS)" )
	ROMX_LOAD( "tos100uk.bin", 0x00000, 0x30000, BAD_DUMP CRC(1a586c64) SHA1(9a6e4c88533a9eaa4d55cdc040e47443e0226eb2), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102uk.bin", 0x00000, 0x30000, BAD_DUMP CRC(3b5cd0c5) SHA1(87900a40a890fdf03bd08be6c60cc645855cbce5), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104uk.bin", 0x00000, 0x30000, BAD_DUMP CRC(a50d1d43) SHA1(9526ef63b9cb1d2a7109e278547ae78a5c1db6c6), ROM_BIOS(3) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( st_de )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos100", "TOS 1.0 (ROM TOS)" )
	ROMX_LOAD( "tos100de.bin", 0x00000, 0x30000, BAD_DUMP CRC(16e3e979) SHA1(663d9c87cfb44ae8ada855fe9ed3cccafaa7a4ce), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102de.bin", 0x00000, 0x30000, BAD_DUMP CRC(36a0058e) SHA1(cad5d2902e875d8bf0a14dc5b5b8080b30254148), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104de.bin", 0x00000, 0x30000, BAD_DUMP CRC(62b82b42) SHA1(5313733f91b083c6265d93674cb9d0b7efd02da8), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "tos10x", "TOS 1.0?" )
	ROMX_LOAD( "st 7c1 a4.u4", 0x00000, 0x08000, CRC(867fdd7e) SHA1(320d12acf510301e6e9ab2e3cf3ee60b0334baa0), ROM_SKIP(1) | ROM_BIOS(4) )
	ROMX_LOAD( "st 7c1 a9.u7", 0x00001, 0x08000, CRC(30e8f982) SHA1(253f26ff64b202b2681ab68ffc9954125120baea), ROM_SKIP(1) | ROM_BIOS(4) )
	ROMX_LOAD( "st 7c1 b0.u3", 0x10000, 0x08000, CRC(b91337ed) SHA1(21a338f9bbd87bce4a12d38048e03a361f58d33e), ROM_SKIP(1) | ROM_BIOS(4) )
	ROMX_LOAD( "st 7a4 a6.u6", 0x10001, 0x08000, CRC(969d7bbe) SHA1(72b998c1f25211c2a96c81a038d71b6a390585c2), ROM_SKIP(1) | ROM_BIOS(4) )
	ROMX_LOAD( "st 7c1 a2.u2", 0x20000, 0x08000, CRC(d0513329) SHA1(49855a3585e2f75b2af932dd4414ed64e6d9501f), ROM_SKIP(1) | ROM_BIOS(4) )
	ROMX_LOAD( "st 7c1 b1.u5", 0x20001, 0x08000, CRC(c115cbc8) SHA1(2b52b81a1a4e0818d63f98ee4b25c30e2eba61cb), ROM_SKIP(1) | ROM_BIOS(4) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( st_fr )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos100", "TOS 1.0 (ROM TOS)" )
	ROMX_LOAD( "tos100fr.bin", 0x00000, 0x30000, BAD_DUMP CRC(2b7f2117) SHA1(ecb00a2e351a6205089a281b4ce6e08959953704), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102fr.bin", 0x00000, 0x30000, BAD_DUMP CRC(8688fce6) SHA1(f5a79aac0a4e812ca77b6ac51d58d98726f331fe), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104fr.bin", 0x00000, 0x30000, BAD_DUMP CRC(a305a404) SHA1(20dba880344b810cf63cec5066797c5a971db870), ROM_BIOS(3) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( st_es )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104es.bin", 0x00000, 0x30000, BAD_DUMP CRC(f4e8ecd2) SHA1(df63f8ac09125d0877b55d5ba1282779b7f99c16), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( st_nl )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104nl.bin", 0x00000, 0x30000, BAD_DUMP CRC(bb4370d4) SHA1(6de7c96b2d2e5c68778f4bce3eaf85a4e121f166), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( st_se )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102se.bin", 0x00000, 0x30000, BAD_DUMP CRC(673fd0c2) SHA1(433de547e09576743ae9ffc43d43f2279782e127), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104se.bin", 0x00000, 0x30000, BAD_DUMP CRC(80ecfdce) SHA1(b7ad34d5cdfbe86ea74ae79eca11dce421a7bbfd), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( st_sg )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102sg.bin", 0x00000, 0x30000, BAD_DUMP CRC(5fe16c66) SHA1(45acb2fc4b1b13bd806c751aebd66c8304fc79bc), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104sg.bin", 0x00000, 0x30000, BAD_DUMP CRC(e58f0bdf) SHA1(aa40bf7203f02b2251b9e4850a1a73ff1c7da106), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megast )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102.bin", 0x00000, 0x30000, BAD_DUMP CRC(d3c32283) SHA1(735793fdba07fe8d5295caa03484f6ef3de931f5), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104.bin", 0x00000, 0x30000, BAD_DUMP CRC(90f4fbff) SHA1(2487f330b0895e5d88d580d4ecb24061125e88ad), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megast_uk )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102uk.bin", 0x00000, 0x30000, BAD_DUMP CRC(3b5cd0c5) SHA1(87900a40a890fdf03bd08be6c60cc645855cbce5), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104uk.bin", 0x00000, 0x30000, BAD_DUMP CRC(a50d1d43) SHA1(9526ef63b9cb1d2a7109e278547ae78a5c1db6c6), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megast_de )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102de.bin", 0x00000, 0x30000, BAD_DUMP CRC(36a0058e) SHA1(cad5d2902e875d8bf0a14dc5b5b8080b30254148), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104de.bin", 0x00000, 0x30000, BAD_DUMP CRC(62b82b42) SHA1(5313733f91b083c6265d93674cb9d0b7efd02da8), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megast_fr )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102fr.bin", 0x00000, 0x30000, BAD_DUMP CRC(8688fce6) SHA1(f5a79aac0a4e812ca77b6ac51d58d98726f331fe), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104fr.bin", 0x00000, 0x30000, BAD_DUMP CRC(a305a404) SHA1(20dba880344b810cf63cec5066797c5a971db870), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megast_se )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102se.bin", 0x00000, 0x30000, BAD_DUMP CRC(673fd0c2) SHA1(433de547e09576743ae9ffc43d43f2279782e127), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104se.bin", 0x00000, 0x30000, BAD_DUMP CRC(80ecfdce) SHA1(b7ad34d5cdfbe86ea74ae79eca11dce421a7bbfd), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megast_sg )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos104")
	ROM_SYSTEM_BIOS( 0, "tos102", "TOS 1.02 (MEGA TOS)" )
	ROMX_LOAD( "tos102sg.bin", 0x00000, 0x30000, BAD_DUMP CRC(5fe16c66) SHA1(45acb2fc4b1b13bd806c751aebd66c8304fc79bc), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104sg.bin", 0x00000, 0x30000, BAD_DUMP CRC(e58f0bdf) SHA1(aa40bf7203f02b2251b9e4850a1a73ff1c7da106), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( stacy )
	ROM_REGION16_BE( 0x30000, M68000_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "tos104", "TOS 1.04 (Rainbow TOS)" )
	ROMX_LOAD( "tos104.bin", 0x00000, 0x30000, BAD_DUMP CRC(a50d1d43) SHA1(9526ef63b9cb1d2a7109e278547ae78a5c1db6c6), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( ste )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos206")
	ROM_SYSTEM_BIOS( 0, "tos106", "TOS 1.06 (STE TOS, Revision 1)" )
	ROMX_LOAD( "tos106.bin", 0x00000, 0x40000, BAD_DUMP CRC(a2e25337) SHA1(6a850810a92fdb1e64d005a06ea4079f51c97145), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos162", "TOS 1.62 (STE TOS, Revision 2)" )
	ROMX_LOAD( "tos162.bin", 0x00000, 0x40000, BAD_DUMP CRC(1c1a4eba) SHA1(42b875f542e5b728905d819c83c31a095a6a1904), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos206", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206.bin", 0x00000, 0x40000, BAD_DUMP CRC(3f2f840f) SHA1(ee58768bdfc602c9b14942ce5481e97dd24e7c83), ROM_BIOS(3) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( ste_uk )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos206")
	ROM_SYSTEM_BIOS( 0, "tos106", "TOS 1.06 (STE TOS, Revision 1)" )
	ROMX_LOAD( "tos106uk.bin", 0x00000, 0x40000, BAD_DUMP CRC(d72fea29) SHA1(06f9ea322e74b682df0396acfaee8cb4d9c90cad), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos162", "TOS 1.62 (STE TOS, Revision 2)" )
	ROMX_LOAD( "tos162uk.bin", 0x00000, 0x40000, BAD_DUMP CRC(d1c6f2fa) SHA1(70db24a7c252392755849f78940a41bfaebace71), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos206", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206uk.bin", 0x00000, 0x40000, BAD_DUMP CRC(08538e39) SHA1(2400ea95f547d6ea754a99d05d8530c03f8b28e3), ROM_BIOS(3) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( ste_de )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos206")
	ROM_SYSTEM_BIOS( 0, "tos106", "TOS 1.06 (STE TOS, Revision 1)" )
	ROMX_LOAD( "tos106de.bin", 0x00000, 0x40000, BAD_DUMP CRC(7c67c5c9) SHA1(3b8cf5ffa41b252eb67f8824f94608fa4005d6dd), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos162", "TOS 1.62 (STE TOS, Revision 2)" )
	ROMX_LOAD( "tos162de.bin", 0x00000, 0x40000, BAD_DUMP CRC(2cdeb5e5) SHA1(10d9f61705048ee3dcbec67df741bed49b922149), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos206", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206de.bin", 0x00000, 0x40000, BAD_DUMP CRC(143cd2ab) SHA1(d1da866560734289c4305f1028c36291d331d417), ROM_BIOS(3) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( ste_es )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos106")
	ROM_SYSTEM_BIOS( 0, "tos106", "TOS 1.06 (STE TOS, Revision 1)" )
	ROMX_LOAD( "tos106es.bin", 0x00000, 0x40000, BAD_DUMP CRC(5cd2a540) SHA1(3a18f342c8288c0bc1879b7a209c73d5d57f7e81), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( ste_fr )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos206")
	ROM_SYSTEM_BIOS( 0, "tos106", "TOS 1.06 (STE TOS, Revision 1)" )
	ROMX_LOAD( "tos106fr.bin", 0x00000, 0x40000, BAD_DUMP CRC(b6e58a46) SHA1(7d7e3cef435caa2fd7733a3fbc6930cb9ea7bcbc), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos162", "TOS 1.62 (STE TOS, Revision 2)" )
	ROMX_LOAD( "tos162fr.bin", 0x00000, 0x40000, BAD_DUMP CRC(0ab003be) SHA1(041e134da613f718fca8bd47cd7733076e8d7588), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos206", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206fr.bin", 0x00000, 0x40000, BAD_DUMP CRC(e3a99ca7) SHA1(387da431e6e3dd2e0c4643207e67d06cf33618c3), ROM_BIOS(3) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( ste_it )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos106")
	ROM_SYSTEM_BIOS( 0, "tos106", "TOS 1.06 (STE TOS, Revision 1)" )
	ROMX_LOAD( "tos106it.bin", 0x00000, 0x40000, BAD_DUMP CRC(d3a55216) SHA1(28dc74e5e0fa56b685bbe15f9837f52684fee9fd), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( ste_se )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos206")
	ROM_SYSTEM_BIOS( 0, "tos162", "TOS 1.62 (STE TOS, Revision 2)" )
	ROMX_LOAD( "tos162se.bin", 0x00000, 0x40000, BAD_DUMP CRC(90f124b1) SHA1(6e5454e861dbf4c46ce5020fc566c31202087b88), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos206", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206se.bin", 0x00000, 0x40000, BAD_DUMP CRC(be61906d) SHA1(ebdf5a4cf08471cd315a91683fcb24e0f029d451), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( ste_sg )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos206")
	ROM_SYSTEM_BIOS( 0, "tos206", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206sg.bin", 0x00000, 0x40000, BAD_DUMP CRC(8c4fe57d) SHA1(c7a9ae3162f020dcac0c2a46cf0c033f91b98644), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megaste )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos206")
	ROM_SYSTEM_BIOS( 0, "tos205", "TOS 2.05 (Mega STE TOS)" )
	ROMX_LOAD( "tos205.bin", 0x00000, 0x40000, BAD_DUMP CRC(d8845f8d) SHA1(e069c14863819635bea33074b90c22e5bd99f1bd), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos206", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206.bin", 0x00000, 0x40000, BAD_DUMP CRC(3f2f840f) SHA1(ee58768bdfc602c9b14942ce5481e97dd24e7c83), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megaste_uk )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos206")
	ROM_SYSTEM_BIOS( 0, "tos202", "TOS 2.02 (Mega STE TOS)" )
	ROMX_LOAD( "tos202uk.bin", 0x00000, 0x40000, NO_DUMP, ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos205", "TOS 2.05 (Mega STE TOS)" )
	ROMX_LOAD( "tos205uk.bin", 0x00000, 0x40000, NO_DUMP, ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos206", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206uk.bin", 0x00000, 0x40000, BAD_DUMP CRC(08538e39) SHA1(2400ea95f547d6ea754a99d05d8530c03f8b28e3), ROM_BIOS(3) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megaste_fr )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos206")
	ROM_SYSTEM_BIOS( 0, "tos205", "TOS 2.05 (Mega STE TOS)" )
	ROMX_LOAD( "tos205fr.bin", 0x00000, 0x40000, BAD_DUMP CRC(27b83d2f) SHA1(83963b0feb0d119b2ca6f51e483e8c20e6ab79e1), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos206", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206fr.bin", 0x00000, 0x40000, BAD_DUMP CRC(e3a99ca7) SHA1(387da431e6e3dd2e0c4643207e67d06cf33618c3), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megaste_de )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos206")
	ROM_SYSTEM_BIOS( 0, "tos205", "TOS 2.05 (Mega STE TOS)" )
	ROMX_LOAD( "tos205de.bin", 0x00000, 0x40000, BAD_DUMP CRC(518b24e6) SHA1(084e083422f8fd9ac7a2490f19b81809c52b91b4), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos206", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206de.bin", 0x00000, 0x40000, BAD_DUMP CRC(143cd2ab) SHA1(d1da866560734289c4305f1028c36291d331d417), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megaste_es )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos205")
	ROM_SYSTEM_BIOS( 0, "tos205", "TOS 2.05 (Mega STE TOS)" )
	ROMX_LOAD( "tos205es.bin", 0x00000, 0x40000, BAD_DUMP CRC(2a426206) SHA1(317715ad8de718b5acc7e27ecf1eb833c2017c91), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megaste_it )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos205")
	ROM_SYSTEM_BIOS( 0, "tos205", "TOS 2.05 (Mega STE TOS)" )
	ROMX_LOAD( "tos205it.bin", 0x00000, 0x40000, BAD_DUMP CRC(b28bf5a1) SHA1(8e0581b442384af69345738849cf440d72f6e6ab), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( megaste_se )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos206")
	ROM_SYSTEM_BIOS( 0, "tos205", "TOS 2.05 (Mega STE TOS)" )
	ROMX_LOAD( "tos205se.bin", 0x00000, 0x40000, BAD_DUMP CRC(6d49ccbe) SHA1(c065b1a9a2e42e5e373333e99be829028902acaa), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos206", "TOS 2.06 (ST/STE TOS)" )
	ROMX_LOAD( "tos206se.bin", 0x00000, 0x40000, BAD_DUMP CRC(be61906d) SHA1(ebdf5a4cf08471cd315a91683fcb24e0f029d451), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( stbook )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "tos208", "TOS 2.08" )
	ROMX_LOAD( "tos208.bin", 0x00000, 0x40000, NO_DUMP, ROM_BIOS(1) )

	ROM_REGION( 0x1000, COP888_TAG, 0 )
	ROM_LOAD( "cop888c0.u703", 0x0000, 0x1000, NO_DUMP )
ROM_END

ROM_START( stpad )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "tos205", "TOS 2.05" )
	ROMX_LOAD( "tos205.bin", 0x00000, 0x40000, NO_DUMP, ROM_BIOS(1) )
ROM_END

ROM_START( tt030 )
	ROM_REGION32_BE( 0x80000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos306")
	ROM_SYSTEM_BIOS( 0, "tos306", "TOS 3.06 (TT TOS)" )
	ROMX_LOAD( "tos306.bin", 0x00000, 0x80000, BAD_DUMP CRC(e65adbd7) SHA1(b15948786278e1f2abc4effbb6d40786620acbe8), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( tt030_uk )
	ROM_REGION32_BE( 0x80000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos306")
	ROM_SYSTEM_BIOS( 0, "tos306", "TOS 3.06 (TT TOS)" )
	ROMX_LOAD( "tos306.bin", 0x00000, 0x80000, BAD_DUMP CRC(75dda215) SHA1(6325bdfd83f1b4d3afddb2b470a19428ca79478b), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( tt030_de )
	ROM_REGION32_BE( 0x80000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos306")
	ROM_SYSTEM_BIOS( 0, "tos306", "TOS 3.06 (TT TOS)" )
	ROMX_LOAD( "tos306.bin", 0x00000, 0x80000, BAD_DUMP CRC(4fcbb59d) SHA1(80af04499d1c3b8551fc4d72142ff02c2182e64a), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( tt030_fr )
	ROM_REGION32_BE( 0x80000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos306")
	ROM_SYSTEM_BIOS( 0, "tos306", "TOS 3.06 (TT TOS)" )
	ROMX_LOAD( "tos306.bin", 0x00000, 0x80000, BAD_DUMP CRC(1945511c) SHA1(6bb19874e1e97dba17215d4f84b992c224a81b95), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( tt030_pl )
	ROM_REGION32_BE( 0x80000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos306")
	ROM_SYSTEM_BIOS( 0, "tos306", "TOS 3.06 (TT TOS)" )
	ROMX_LOAD( "tos306.bin", 0x00000, 0x80000, BAD_DUMP CRC(4f2404bc) SHA1(d122b8ceb202b52754ff0d442b1c81f8b4de3436), ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( fx1 )
	ROM_REGION16_BE( 0x40000, M68000_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "tos207", "TOS 2.07" )
	ROMX_LOAD( "tos207.bin", 0x00000, 0x40000, NO_DUMP, ROM_BIOS(1) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( falcon )
	ROM_REGION32_BE( 0x80000, M68000_TAG, 0 )
	ROM_DEFAULT_BIOS("tos404")
	ROM_SYSTEM_BIOS( 0, "tos400", "TOS 4.00" )
	ROMX_LOAD( "tos400.bin", 0x00000, 0x7ffff, BAD_DUMP CRC(1fbc5396) SHA1(d74d09f11a0bf37a86ccb50c6e7f91aac4d4b11b), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "tos401", "TOS 4.01" )
	ROMX_LOAD( "tos401.bin", 0x00000, 0x80000, NO_DUMP, ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tos402", "TOS 4.02" )
	ROMX_LOAD( "tos402.bin", 0x00000, 0x80000, BAD_DUMP CRC(63f82f23) SHA1(75de588f6bbc630fa9c814f738195da23b972cc6), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "tos404", "TOS 4.04" )
	ROMX_LOAD( "tos404.bin", 0x00000, 0x80000, BAD_DUMP CRC(028b561d) SHA1(27dcdb31b0951af99023b2fb8c370d8447ba6ebc), ROM_BIOS(4) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

ROM_START( falcon40 )
	ROM_REGION32_BE( 0x80000, M68000_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "tos492", "TOS 4.92" )
	ROMX_LOAD( "tos492.bin", 0x00000, 0x7d314, BAD_DUMP CRC(bc8e497f) SHA1(747a38042844a6b632dcd9a76d8525fccb5eb892), ROM_BIOS(2) )

	ROM_REGION( 0x1000, HD6301_TAG, 0 )
	ROM_LOAD( "keyboard.u1", 0x0000, 0x1000, CRC(0296915d) SHA1(1102f20d38f333234041c13687d82528b7cde2e1) )
ROM_END

/* System Drivers */

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT       INIT     COMPANY    FULLNAME                FLAGS */
COMP( 1985,	st,			0,			0,		st,			st,			0,		"Atari",	"ST (USA)",				GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1985,	st_uk,		st,			0,		st,			st,			0,		"Atari",	"ST (UK)",				GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1985, st_de,		st,			0,		st,			st,			0,		"Atari",	"ST (Germany)",			GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1985, st_es,		st,			0,		st,			st,			0,		"Atari",	"ST (Spain)",			GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1985, st_fr,		st,			0,		st,			st,			0,		"Atari",	"ST (France)",			GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1985, st_nl,		st,			0,		st,			st,			0,		"Atari",	"ST (Netherlands)",		GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1985, st_se,		st,			0,		st,			st,			0,		"Atari",	"ST (Sweden)",			GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1985, st_sg,		st,			0,		st,			st,			0,		"Atari",	"ST (Switzerland)",		GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1987, megast,		st,			0,		megast,		st,			0,		"Atari",	"MEGA ST (USA)",		GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1987, megast_uk,	st,			0,		megast,		st,			0,		"Atari",	"MEGA ST (UK)",			GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1987, megast_de,	st,			0,		megast,		st,			0,		"Atari",	"MEGA ST (Germany)",	GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1987, megast_fr,	st,			0,		megast,		st,			0,		"Atari",	"MEGA ST (France)",		GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1987, megast_se,	st,			0,		megast,		st,			0,		"Atari",	"MEGA ST (Sweden)",		GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1987, megast_sg,	st,			0,		megast,		st,			0,		"Atari",	"MEGA ST (Switzerland)",GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1989, ste,		0,			0,		ste,		ste,		0,		"Atari",	"STE (USA)",			GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1989, ste_uk,		ste,		0,		ste,		ste,		0,		"Atari",	"STE (UK)",				GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1989, ste_de,		ste,		0,		ste,		ste,		0,		"Atari",	"STE (Germany)",		GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1989, ste_es,		ste,		0,		ste,		ste,		0,		"Atari",	"STE (Spain)",			GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1989, ste_fr,		ste,		0,		ste,		ste,		0,		"Atari",	"STE (France)",			GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1989, ste_it,		ste,		0,		ste,		ste,		0,		"Atari",	"STE (Italy)",			GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1989, ste_se,		ste,		0,		ste,		ste,		0,		"Atari",	"STE (Sweden)",			GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1989, ste_sg,		ste,		0,		ste,		ste,		0,		"Atari",	"STE (Switzerland)",	GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1990, stbook,		ste,		0,		stbook,		stbook,		0,		"Atari",	"STBook",				GAME_NOT_WORKING )
COMP( 1990,	tt030,		0,			0,		tt030,		tt030,		0,		"Atari",	"TT030 (USA)",			GAME_NOT_WORKING )
COMP( 1990,	tt030_uk,	tt030,		0,		tt030,		tt030,		0,		"Atari",	"TT030 (UK)",			GAME_NOT_WORKING )
COMP( 1990,	tt030_de,	tt030,		0,		tt030,		tt030,		0,		"Atari",	"TT030 (Germany)",		GAME_NOT_WORKING )
COMP( 1990,	tt030_fr,	tt030,		0,		tt030,		tt030,		0,		"Atari",	"TT030 (France)",		GAME_NOT_WORKING )
COMP( 1990,	tt030_pl,	tt030,		0,		tt030,		tt030,		0,		"Atari",	"TT030 (Poland)",		GAME_NOT_WORKING )
COMP( 1991, megaste,	ste,		0,		megaste,	st,			0,		"Atari",	"MEGA STE (USA)",		GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1991, megaste_uk,	ste,		0,		megaste,	st,			0,		"Atari",	"MEGA STE (UK)",		GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1991, megaste_de,	ste,		0,		megaste,	st,			0,		"Atari",	"MEGA STE (Germany)",	GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1991, megaste_es,	ste,		0,		megaste,	st,			0,		"Atari",	"MEGA STE (Spain)",		GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1991, megaste_fr,	ste,		0,		megaste,	st,			0,		"Atari",	"MEGA STE (France)",	GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1991, megaste_it,	ste,		0,		megaste,	st,			0,		"Atari",	"MEGA STE (Italy)",		GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1991, megaste_se,	ste,		0,		megaste,	st,			0,		"Atari",	"MEGA STE (Sweden)",	GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1992, falcon,		0,			0,		falcon,		falcon,		0,		"Atari",	"Falcon030",			GAME_NOT_WORKING )
COMP( 1992, falcon40,	falcon,		0,		falcon40,	falcon,		0,		"Atari",	"Falcon040 (prototype)",GAME_NOT_WORKING )
/*
COMP( 1989, stacy,    st,  0,      stacy,    stacy,    0,     "Atari", "Stacy", GAME_NOT_WORKING )
COMP( 1991, stpad,    ste, 0,      stpad,    stpad,    0,     "Atari", "STPad (prototype)", GAME_NOT_WORKING )
COMP( 1992, fx1,      0,        0,      falcon,   falcon,   0,     "Atari", "FX-1 (prototype)", GAME_NOT_WORKING )
*/
