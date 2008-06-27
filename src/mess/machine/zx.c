/***************************************************************************
	zx.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/zx.h"
#include "devices/cassette.h"

#define video_screen_get_refresh(screen)	(((screen_config *)(screen)->inline_config)->refresh)

#define	DEBUG_ZX81_PORTS	1
#define DEBUG_ZX81_VSYNC	1

#define LOG_ZX81_IOR(_comment) { if (DEBUG_ZX81_PORTS) logerror("ZX81 IOR: %04x, Data: %02x, Scanline: %d (%s)\n", offset, data, video_screen_get_vpos(machine->primary_screen), _comment); }
#define LOG_ZX81_IOW(_comment) { if (DEBUG_ZX81_PORTS) logerror("ZX81 IOW: %04x, Data: %02x, Scanline: %d (%s)\n", offset, data, video_screen_get_vpos(machine->primary_screen), _comment); }
#define LOG_ZX81_VSYNC { if (DEBUG_ZX81_VSYNC) logerror("VSYNC starts in scanline: %d\n", video_screen_get_vpos(machine->primary_screen)); }

static UINT8 zx_tape_bit = 0x80;

static WRITE8_HANDLER( zx_ram_w )
{
	UINT8 *rom = memory_region(machine, REGION_CPU1);

	rom[offset | 0x4000] = data;

	if ((offset > 0x29) && (offset < 0x300))
	{
		if (data & 0x40)
			program_write_byte (offset | 0xc000, data);
		else
			program_write_byte (offset | 0xc000, 0);
	}
}

DRIVER_INIT ( zx )
{
	memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4000 + mess_ram_size - 1, 0, 0, SMH_BANK1);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4000 + mess_ram_size - 1, 0, 0, zx_ram_w);
	memory_set_bankptr(1, memory_region(machine, REGION_CPU1) + 0x4000);
}

#if 0
static OPBASE_HANDLER ( zx_setopbase )
{
	if ((address > 0xc029) && (address < 0xc300))
	{
		int byte = program_read_byte( address & 0x7fff );

		if (byte & 0x40)
			program_write_byte( address, byte );
		else
			program_write_byte( address, 0 );

		zx_ula_r(machine, address, REGION_CPU1);
	}
	return address;
}
#endif

static OPBASE_HANDLER ( zx_setopbase )
{
	if (address & 0x8000)
		zx_ula_r(machine, address, REGION_CPU1);
	return address;
}

static OPBASE_HANDLER ( pc8300_setopbase )
{
	if (address & 0x8000)
		zx_ula_r(machine, address, REGION_GFX1);
	return address;
}

static void common_init_machine(opbase_handler_func setopbase)
{
	memory_set_opbase_handler(0, setopbase);
	zx_tape_bit = 0x80;
}

MACHINE_RESET ( zx80 )
{
	common_init_machine(zx_setopbase);
}

MACHINE_RESET ( zx81 )
{
	common_init_machine(zx_setopbase);
}

MACHINE_RESET ( pc8300 )
{
	common_init_machine(pc8300_setopbase);
}

static TIMER_CALLBACK(zx_tape_pulse)
{
	zx_tape_bit = 0x80;
}

WRITE8_HANDLER ( zx_io_w )
{
	const device_config *screen = video_screen_first(machine->config);
	int height = video_screen_get_height(screen);

	if ((offset & 2) == 0)
	{
		timer_reset(ula_nmi, attotime_never);

		LOG_ZX81_IOW("ULA NMIs off");
	}
	else if ((offset & 1) == 0)
	{
		timer_adjust_periodic(ula_nmi, attotime_zero, 0, ATTOTIME_IN_CYCLES(207, 0));

		LOG_ZX81_IOW("ULA NMIs on");

		/* remove the IRQ */
		ula_irq_active = 0;
	}
	else
	{
		zx_ula_bkgnd(machine, 1);
		if (ula_frame_vsync == 2)
		{
			cpu_spinuntil_time(video_screen_get_time_until_pos(machine->primary_screen, height - 1, 0));
			ula_scanline_count = height - 1;
			logerror ("S: %d B: %d\n", video_screen_get_vpos(machine->primary_screen), video_screen_get_hpos(machine->primary_screen));
		}

		LOG_ZX81_IOW("ULA IRQs on");
	}
}

READ8_HANDLER ( zx_io_r )
{
	const device_config *screen = video_screen_first(machine->config);
	int data = 0xff;

	if ((offset & 1) == 0)
	{
		int extra1 = input_port_read(machine, "SPC0");
		int extra2 = input_port_read(machine, "SPC1");

//		ula_scancode_count = 0;
		if ((offset & 0x0100) == 0)
		{
			data &= input_port_read(machine, "ROW0");
			/* SHIFT for extra keys */
			if (extra1 != 0xff || extra2 != 0xff)
				data &= ~0x01;
		}
		if ((offset & 0x0200) == 0)
			data &= input_port_read(machine, "ROW1");
		if ((offset & 0x0400) == 0)
			data &= input_port_read(machine, "ROW2");
		if ((offset & 0x0800) == 0)
			data &= input_port_read(machine, "ROW3") & extra1;
		if ((offset & 0x1000) == 0)
			data &= input_port_read(machine, "ROW4") & extra2;
		if ((offset & 0x2000) == 0)
			data &= input_port_read(machine, "ROW5");
		if ((offset & 0x4000) == 0)
			data &= input_port_read(machine, "ROW6");
		if ((offset & 0x8000) == 0)
			data &= input_port_read(machine, "ROW7");
		if (video_screen_get_refresh(screen) > 55)
			data &= ~0x40;

		if (ula_irq_active)
		{
			zx_ula_bkgnd(machine, 0);
			ula_irq_active = 0;

			LOG_ZX81_IOR("ULA IRQs off");
		}
		else
		{
			if ((cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) < -0.75) && zx_tape_bit)
			{
				zx_tape_bit = 0x00;
				timer_set(ATTOTIME_IN_USEC(362), NULL, 0, zx_tape_pulse);
			}

			data &= ~zx_tape_bit;

			LOG_ZX81_IOR("Tape");
		}
		if (ula_frame_vsync == 3)
		{
			ula_frame_vsync = 2;
			LOG_ZX81_VSYNC;
		}
	}
	else
		LOG_ZX81_IOR("Unmapped port");

	return data;
}

READ8_HANDLER ( pow3000_io_r )
{
	const device_config *screen = video_screen_first(machine->config);
	int data = 0xff;

	if ((offset & 1) == 0)
	{
		int extra1 = input_port_read(machine, "SPC0");
		int extra2 = input_port_read(machine, "SPC1");

//		ula_scancode_count = 0;
		if ((offset & 0x0100) == 0)
		{
			data &= input_port_read(machine, "ROW0") & extra1;
			/* SHIFT for extra keys */
			if (extra1 != 0xff || extra2 != 0xff)
				data &= ~0x01;
		}
		if ((offset & 0x0200) == 0)
			data &= input_port_read(machine, "ROW1") & extra2;
		if ((offset & 0x0400) == 0)
			data &= input_port_read(machine, "ROW2");
		if ((offset & 0x0800) == 0)
			data &= input_port_read(machine, "ROW3");
		if ((offset & 0x1000) == 0)
			data &= input_port_read(machine, "ROW4");
		if ((offset & 0x2000) == 0)
			data &= input_port_read(machine, "ROW5");
		if ((offset & 0x4000) == 0)
			data &= input_port_read(machine, "ROW6");
		if ((offset & 0x8000) == 0)
			data &= input_port_read(machine, "ROW7");
		if (video_screen_get_refresh(screen) > 55)
			data &= ~0x40;

		if (ula_irq_active)
		{
			zx_ula_bkgnd(machine, 0);
			ula_irq_active = 0;
			LOG_ZX81_IOR("ULA IRQs off");
		}
		else
		{
			if ((cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) < -0.75) && zx_tape_bit)
			{
				zx_tape_bit = 0x00;
				timer_set(ATTOTIME_IN_USEC(362), NULL, 0, zx_tape_pulse);
			}

			data &= ~zx_tape_bit;

			LOG_ZX81_IOR("Tape");
		}
		if (ula_frame_vsync == 3)
		{
			ula_frame_vsync = 2;
			LOG_ZX81_VSYNC;
		}
	}
	else
		LOG_ZX81_IOR("Unknown port");

	return data;
}
