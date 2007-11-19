/***************************************************************************
	zx.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

****************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "cpu/z80/z80.h"
#include "includes/zx.h"
#include "devices/cassette.h"

#define	DEBUG_ZX81_PORTS	1
#define DEBUG_ZX81_VSYNC	1

#define LOG_ZX81_IOR(_comment) { if (DEBUG_ZX81_PORTS) logerror("ZX81 IOR: %04x, Data: %02x, Scanline: %d (%s)\n", offset, data, video_screen_get_vpos(0), _comment); }
#define LOG_ZX81_IOW(_comment) { if (DEBUG_ZX81_PORTS) logerror("ZX81 IOW: %04x, Data: %02x, Scanline: %d (%s)\n", offset, data, video_screen_get_vpos(0), _comment); }
#define LOG_ZX81_VSYNC { if (DEBUG_ZX81_VSYNC) logerror("VSYNC starts in scanline: %d\n", video_screen_get_vpos(0)); }

static UINT8 zx_tape_bit = 0x80;

DRIVER_INIT ( zx )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4000 + mess_ram_size - 1, 0, 0x8000, MRA8_BANK1);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4000 + mess_ram_size - 1, 0, 0x8000, MWA8_BANK1);
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x4000);
}

static OPBASE_HANDLER ( zx_setopbase )
{
	if (address & 0x8000)
		return zx_ula_r(address, REGION_CPU1);
	return address;
}

static OPBASE_HANDLER ( pc8300_setopbase )
{
	if (address & 0x8000)
		return zx_ula_r(address, REGION_GFX1);
	return address;
}

static void common_init_machine(opbase_handler setopbase)
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
	if ((offset & 2) == 0)
	{
		timer_reset(ula_nmi, attotime_never);

		LOG_ZX81_IOW("ULA NMIs off");
	}
	else if ((offset & 1) == 0)
	{
		timer_adjust(ula_nmi, attotime_zero, 0, ATTOTIME_IN_CYCLES(207, 0));

		LOG_ZX81_IOW("ULA NMIs on");

		/* remove the IRQ */
		ula_irq_active = 0;
	}
	else
	{
		zx_ula_bkgnd(1);
		if (ula_frame_vsync == 2)
		{
			cpu_spinuntil_time(video_screen_get_time_until_pos(0, Machine->screen[0].height - 1, 0));
			ula_scanline_count = Machine->screen[0].height - 1;
			logerror ("S: %d B: %d\n", video_screen_get_vpos(0), video_screen_get_hpos(0));
		}

		LOG_ZX81_IOW("ULA IRQs on");
	}
}

READ8_HANDLER ( zx_io_r )
{
	int data = 0xff;

	if ((offset & 1) == 0)
	{
		int extra1 = readinputport(9);
		int extra2 = readinputport(10);

		ula_scancode_count = 0;
		if ((offset & 0x0100) == 0)
		{
			data &= readinputport(1);
			/* SHIFT for extra keys */
			if (extra1 != 0xff || extra2 != 0xff)
				data &= ~0x01;
		}
		if ((offset & 0x0200) == 0)
			data &= readinputport(2);
		if ((offset & 0x0400) == 0)
			data &= readinputport(3);
		if ((offset & 0x0800) == 0)
			data &= readinputport(4) & extra1;
		if ((offset & 0x1000) == 0)
			data &= readinputport(5) & extra2;
		if ((offset & 0x2000) == 0)
			data &= readinputport(6);
		if ((offset & 0x4000) == 0)
			data &= readinputport(7);
		if ((offset & 0x8000) == 0)
			data &= readinputport(8);
		if (Machine->screen[0].refresh > 55)
			data &= ~0x40;

		if (ula_irq_active)
		{
			zx_ula_bkgnd(0);
			ula_irq_active = 0;

			LOG_ZX81_IOR("ULA IRQs off");
		}
		else
		{
			if ((cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) < -0.75) && zx_tape_bit)
			{
				zx_tape_bit = 0x00;
				timer_set(ATTOTIME_IN_USEC(362), 0, zx_tape_pulse);
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
	int data = 0xff;

	if ((offset & 1) == 0)
	{
		int extra1 = readinputport(9);
		int extra2 = readinputport(10);

		ula_scancode_count = 0;
		if ((offset & 0x0100) == 0)
		{
			data &= readinputport(1) & extra1;
			/* SHIFT for extra keys */
			if (extra1 != 0xff || extra2 != 0xff)
				data &= ~0x01;
		}
		if ((offset & 0x0200) == 0)
			data &= readinputport(2) & extra2;
		if ((offset & 0x0400) == 0)
			data &= readinputport(3);
		if ((offset & 0x0800) == 0)
			data &= readinputport(4);
		if ((offset & 0x1000) == 0)
			data &= readinputport(5);
		if ((offset & 0x2000) == 0)
			data &= readinputport(6);
		if ((offset & 0x4000) == 0)
			data &= readinputport(7);
		if ((offset & 0x8000) == 0)
			data &= readinputport(8);
		if (Machine->screen[0].refresh > 55)
			data &= ~0x40;

		if (ula_irq_active)
		{
			zx_ula_bkgnd(0);
			ula_irq_active = 0;
			LOG_ZX81_IOR("ULA IRQs off");
		}
		else
		{
			if ((cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) < -0.75) && zx_tape_bit)
			{
				zx_tape_bit = 0x00;
				timer_set(ATTOTIME_IN_USEC(362), 0, zx_tape_pulse);
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
