/***************************************************************************
    zx.c

    machine driver
    Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/zx.h"
#include "devices/cassette.h"
#include "sound/speaker.h"
#include "devices/messram.h"

#define video_screen_get_refresh(screen)	(((screen_config *)(screen)->inline_config)->refresh)

#define	DEBUG_ZX81_PORTS	1
#define DEBUG_ZX81_VSYNC	1

#define LOG_ZX81_IOR(_comment) do { if (DEBUG_ZX81_PORTS) logerror("ZX81 IOR: %04x, Data: %02x, Scanline: %d (%s)\n", offset, data, space->machine->primary_screen->vpos(), _comment); } while (0)
#define LOG_ZX81_IOW(_comment) do { if (DEBUG_ZX81_PORTS) logerror("ZX81 IOW: %04x, Data: %02x, Scanline: %d (%s)\n", offset, data, space->machine->primary_screen->vpos(), _comment); } while (0)
#define LOG_ZX81_VSYNC do { if (DEBUG_ZX81_VSYNC) logerror("VSYNC starts in scanline: %d\n", space->machine->primary_screen->vpos()); } while (0)

static UINT8 zx_tape_bit = 0x80;

static WRITE8_HANDLER( zx_ram_w )
{
	UINT8 *RAM = memory_region(space->machine, "maincpu");
	RAM[offset + 0x4000] = data;

	if (data & 0x40)
	{
		space->write_byte(offset | 0xc000, data);
		RAM[offset | 0xc000] = data;
	}
	else
	{
		space->write_byte(offset | 0xc000, 0);
		RAM[offset | 0xc000] = 0;
	}
}

/* I know this looks really pointless... but it has to be here */
READ8_HANDLER( zx_ram_r )
{
	UINT8 *RAM = memory_region(space->machine, "maincpu");
	return RAM[offset | 0xc000];
}

DRIVER_INIT ( zx )
{
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	memory_install_read_bank(space, 0x4000, 0x4000 + messram_get_size(machine->device("messram")) - 1, 0, 0, "bank1");
	memory_install_write8_handler(space, 0x4000, 0x4000 + messram_get_size(machine->device("messram")) - 1, 0, 0, zx_ram_w);
	memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu") + 0x4000);
}

DIRECT_UPDATE_HANDLER ( zx_setdirect )
{
	if (address & 0xc000)
		zx_ula_r(machine, address, "maincpu", 0);
	return address;
}

DIRECT_UPDATE_HANDLER ( pc8300_setdirect )
{
	if (address & 0xc000)
		zx_ula_r(machine, address, "gfx1", 0);
	return address;
}

DIRECT_UPDATE_HANDLER ( pow3000_setdirect )
{
	if (address & 0xc000)
		zx_ula_r(machine, address, "gfx1", 1);
	return address;
}

MACHINE_RESET ( zx80 )
{
	cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM)->set_direct_update_handler(direct_update_delegate_create_static(zx_setdirect, *machine));	
	zx_tape_bit = 0x80;
}

MACHINE_RESET ( pow3000 )
{
	cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM)->set_direct_update_handler(direct_update_delegate_create_static(pow3000_setdirect, *machine));
	zx_tape_bit = 0x80;
}

MACHINE_RESET ( pc8300 )
{
	cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM)->set_direct_update_handler(direct_update_delegate_create_static(pc8300_setdirect, *machine));
	zx_tape_bit = 0x80;
}

static TIMER_CALLBACK(zx_tape_pulse)
{
	zx_tape_bit = 0x80;
}

READ8_HANDLER ( zx80_io_r )
{
/* port FE = read keyboard, NTSC/PAL diode, and cass bit; turn off HSYNC-generator/cass-out
    The upper 8 bits are used to select a keyboard scan line

    The diode doesn't make any visual difference, but it's in the schematic, and used by the code. */

	UINT8 data = 0xff;
	UINT8 offs = offset & 0xff;

	if (offs == 0xfe)
	{
		if ((offset & 0x0100) == 0)
			data &= input_port_read(space->machine, "ROW0");
		if ((offset & 0x0200) == 0)
			data &= input_port_read(space->machine, "ROW1");
		if ((offset & 0x0400) == 0)
			data &= input_port_read(space->machine, "ROW2");
		if ((offset & 0x0800) == 0)
			data &= input_port_read(space->machine, "ROW3");
		if ((offset & 0x1000) == 0)
			data &= input_port_read(space->machine, "ROW4");
		if ((offset & 0x2000) == 0)
			data &= input_port_read(space->machine, "ROW5");
		if ((offset & 0x4000) == 0)
			data &= input_port_read(space->machine, "ROW6");
		if ((offset & 0x8000) == 0)
			data &= input_port_read(space->machine, "ROW7");

		if (!input_port_read(space->machine, "CONFIG"))
			data &= ~0x40;

		cassette_output(space->machine->device("cassette"), +0.75);

		if (ula_irq_active)
		{
			zx_ula_bkgnd(space->machine, 0);
			ula_irq_active = 0;

//          LOG_ZX81_IOR("ULA IRQs off");
		}
//      else
//      {
			if ((cassette_input(space->machine->device("cassette")) < -0.75) && zx_tape_bit)
			{
				zx_tape_bit = 0x00;
				timer_set(space->machine, ATTOTIME_IN_USEC(362), NULL, 0, zx_tape_pulse);
			}

			data &= ~zx_tape_bit;

//          LOG_ZX81_IOR("Tape");
//      }
		if (ula_frame_vsync == 3)
		{
			ula_frame_vsync = 2;
//          LOG_ZX81_VSYNC;
		}
	}
	else
		LOG_ZX81_IOR("Unmapped port");

	return data;
}

READ8_HANDLER ( zx81_io_r )
{
/* port FB = read printer status, not emulated
    FE = read keyboard, NTSC/PAL diode, and cass bit; turn off HSYNC-generator/cass-out
    The upper 8 bits are used to select a keyboard scan line */

	UINT8 data = 0xff;
	UINT8 offs = offset & 0xff;

	if (offs == 0xfe)
	{
		if ((offset & 0x0100) == 0)
			data &= input_port_read(space->machine, "ROW0");
		if ((offset & 0x0200) == 0)
			data &= input_port_read(space->machine, "ROW1");
		if ((offset & 0x0400) == 0)
			data &= input_port_read(space->machine, "ROW2");
		if ((offset & 0x0800) == 0)
			data &= input_port_read(space->machine, "ROW3");
		if ((offset & 0x1000) == 0)
			data &= input_port_read(space->machine, "ROW4");
		if ((offset & 0x2000) == 0)
			data &= input_port_read(space->machine, "ROW5");
		if ((offset & 0x4000) == 0)
			data &= input_port_read(space->machine, "ROW6");
		if ((offset & 0x8000) == 0)
			data &= input_port_read(space->machine, "ROW7");

		if (!input_port_read(space->machine, "CONFIG"))
			data &= ~0x40;

		cassette_output(space->machine->device("cassette"), +0.75);

		if (ula_irq_active)
		{
			zx_ula_bkgnd(space->machine, 0);
			ula_irq_active = 0;

//          LOG_ZX81_IOR("ULA IRQs off");
		}
		else
		{
			if ((cassette_input(space->machine->device("cassette")) < -0.75) && zx_tape_bit)
			{
				zx_tape_bit = 0x00;
				timer_set(space->machine, ATTOTIME_IN_USEC(362), NULL, 0, zx_tape_pulse);
			}

			data &= ~zx_tape_bit;

//          LOG_ZX81_IOR("Tape");
		}
		if (ula_frame_vsync == 3)
		{
			ula_frame_vsync = 2;
//          LOG_ZX81_VSYNC;
		}
	}
	else
		LOG_ZX81_IOR("Unmapped port");

	return data;
}

READ8_HANDLER ( pc8300_io_r )
{
/* port F5 = sound
    F6 = unknown
    FB = read printer status, not emulated
    FE = read keyboard and cass bit; turn off HSYNC-generator/cass-out
    The upper 8 bits are used to select a keyboard scan line.
    No TV diode */

	UINT8 data = 0xff;
	UINT8 offs = offset & 0xff;
	static UINT8 speaker_state = 0;
	running_device *speaker = space->machine->device("speaker");

	if (offs == 0xf5)
	{
		speaker_state ^= 1;
		speaker_level_w(speaker, speaker_state);
	}
	else
	if (offs == 0xfe)
	{
		if ((offset & 0x0100) == 0)
			data &= input_port_read(space->machine, "ROW0");
		if ((offset & 0x0200) == 0)
			data &= input_port_read(space->machine, "ROW1");
		if ((offset & 0x0400) == 0)
			data &= input_port_read(space->machine, "ROW2");
		if ((offset & 0x0800) == 0)
			data &= input_port_read(space->machine, "ROW3");
		if ((offset & 0x1000) == 0)
			data &= input_port_read(space->machine, "ROW4");
		if ((offset & 0x2000) == 0)
			data &= input_port_read(space->machine, "ROW5");
		if ((offset & 0x4000) == 0)
			data &= input_port_read(space->machine, "ROW6");
		if ((offset & 0x8000) == 0)
			data &= input_port_read(space->machine, "ROW7");

		cassette_output(space->machine->device("cassette"), +0.75);

		if (ula_irq_active)
		{
			zx_ula_bkgnd(space->machine, 0);
			ula_irq_active = 0;

//          LOG_ZX81_IOR("ULA IRQs off");
		}
		else
		{
			if ((cassette_input(space->machine->device("cassette")) < -0.75) && zx_tape_bit)
			{
				zx_tape_bit = 0x00;
				timer_set(space->machine, ATTOTIME_IN_USEC(362), NULL, 0, zx_tape_pulse);
			}

			data &= ~zx_tape_bit;

//          LOG_ZX81_IOR("Tape");
		}
		if (ula_frame_vsync == 3)
		{
			ula_frame_vsync = 2;
//          LOG_ZX81_VSYNC;
		}
	}
	else
		LOG_ZX81_IOR("Unmapped port");

	return data;
}

READ8_HANDLER ( pow3000_io_r )
{
/* port 7E = read NTSC/PAL diode
    F5 = sound
    F6 = unknown
    FB = read printer status, not emulated
    FE = read keyboard and cass bit; turn off HSYNC-generator/cass-out
    The upper 8 bits are used to select a keyboard scan line */

	UINT8 data = 0xff;
	UINT8 offs = offset & 0xff;
	static UINT8 speaker_state = 0;
	running_device *speaker = space->machine->device("speaker");

	if (offs == 0x7e)
	{
		data = (input_port_read(space->machine, "CONFIG"));
	}
	else
	if (offs == 0xf5)
	{
		speaker_state ^= 1;
		speaker_level_w(speaker, speaker_state);
	}
	else
	if (offs == 0xfe)
	{
		if ((offset & 0x0100) == 0)
			data &= input_port_read(space->machine, "ROW0");
		if ((offset & 0x0200) == 0)
			data &= input_port_read(space->machine, "ROW1");
		if ((offset & 0x0400) == 0)
			data &= input_port_read(space->machine, "ROW2");
		if ((offset & 0x0800) == 0)
			data &= input_port_read(space->machine, "ROW3");
		if ((offset & 0x1000) == 0)
			data &= input_port_read(space->machine, "ROW4");
		if ((offset & 0x2000) == 0)
			data &= input_port_read(space->machine, "ROW5");
		if ((offset & 0x4000) == 0)
			data &= input_port_read(space->machine, "ROW6");
		if ((offset & 0x8000) == 0)
			data &= input_port_read(space->machine, "ROW7");

		cassette_output(space->machine->device("cassette"), +0.75);

		if (ula_irq_active)
		{
			zx_ula_bkgnd(space->machine, 0);
			ula_irq_active = 0;
			LOG_ZX81_IOR("ULA IRQs off");
		}
		else
		{
			if ((cassette_input(space->machine->device("cassette")) < -0.75) && zx_tape_bit)
			{
				zx_tape_bit = 0x00;
				timer_set(space->machine, ATTOTIME_IN_USEC(362), NULL, 0, zx_tape_pulse);
			}

			data &= ~zx_tape_bit;

//          LOG_ZX81_IOR("Tape");
		}
		if (ula_frame_vsync == 3)
		{
			ula_frame_vsync = 2;
//          LOG_ZX81_VSYNC;
		}
	}
	else
		LOG_ZX81_IOR("Unknown port");

	return data;
}

WRITE8_HANDLER( zx80_io_w )
{
/* port FF = write HSYNC and cass data */

	UINT8 offs = offset & 0xff;

	if (offs == 0xff)
		cassette_output(space->machine->device("cassette"), -0.75);
	else
		LOG_ZX81_IOR("Unmapped port");
}

WRITE8_HANDLER ( zx81_io_w )
{
/* port F5 = unknown, pc8300/pow3000/lambda only
    F6 = unknown, pc8300/pow3000/lambda only
    FB = write data to printer, not emulated
    FD = turn off NMI generator
    FE = turn on NMI generator
    FF = write HSYNC and cass data */

	screen_device *screen = screen_first(*space->machine);
	int height = screen->height();
	UINT8 offs = offset & 0xff;

	if (offs == 0xfd)
	{
		timer_reset(ula_nmi, attotime_never);

		LOG_ZX81_IOW("ULA NMIs off");
	}
	else
	if (offs == 0xfe)
	{
		timer_adjust_periodic(ula_nmi, attotime_zero, 0, space->machine->device<cpu_device>("maincpu")->cycles_to_attotime(207));

		LOG_ZX81_IOW("ULA NMIs on");

		/* remove the IRQ */
		ula_irq_active = 0;
	}
	else
	if (offs == 0xff)
	{
		cassette_output(space->machine->device("cassette"), -0.75);
		zx_ula_bkgnd(space->machine, 1);
		if (ula_frame_vsync == 2)
		{
			cpu_spinuntil_time(space->cpu,space->machine->primary_screen->time_until_pos(height - 1, 0));
			ula_scanline_count = height - 1;
			logerror ("S: %d B: %d\n", space->machine->primary_screen->vpos(), space->machine->primary_screen->hpos());
		}

		LOG_ZX81_IOW("ULA IRQs on");
	}
	else
		LOG_ZX81_IOR("Unmapped port");
}

