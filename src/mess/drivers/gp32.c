/**************************************************************************
 *
 * gp32.c - Game Park GP32
 * Skeleton by R. Belmont
 *
 * CPU: Samsung S3C2400X01 SoC
 * S3C2400X01 consists of:
 *    ARM920T CPU core + MMU
 *    LCD controller
 *    DMA controller
 *    Interrupt controller
 *    USB controller
 *    and more.
 *
 **************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "cpu/arm7/arm7.h"

static UINT32 s3c240x_vidregs[0x400/4];

static UINT32 *s3c240x_ram;

static VIDEO_UPDATE( gp32 )
{
	if (s3c240x_vidregs[0] & 1)	// display enabled?
	{
		int x, y;
		UINT8 *vram;
		UINT32 vramaddr = s3c240x_vidregs[5];

		// get offset
		vramaddr = (vramaddr & 0x1fffff);
		vramaddr |= (s3c240x_vidregs[5]&0xffe00000)<<1;

		vram = (UINT8 *)&s3c240x_ram[(vramaddr-0x0c000000)/4];

		for (y = 0; y < 240; y++)
		{
			UINT16 *scanline = BITMAP_ADDR16(bitmap, y, 0);

			for (x = 0; x < 320; x++)
			{
				*scanline++ = vram[(160*y)+x];
			}
		}
	}

	return 0;
}

static READ32_HANDLER(s3c240x_vidregs_r)
{
	// make sure line counter is going
	s3c240x_vidregs[0] &= 0x3ffff;
	s3c240x_vidregs[0] |= (240-video_screen_get_vpos(space->machine->primary_screen))<<18;

	return s3c240x_vidregs[offset];
}

static WRITE32_HANDLER(s3c240x_vidregs_w)
{
	COMBINE_DATA(&s3c240x_vidregs[offset]);
}

static WRITE32_HANDLER(s3c240x_palette_w)
{
	if (mem_mask != 0xffffffff)
	{
		printf("s3c240x_palette_w: unknown mask %08x\n", mem_mask);
	}

	palette_set_color_rgb(space->machine, offset, ((data>>11)&0x1f)<<3, ((data>>5)&0x3f)<<2, (data&0x1f)<<3);
}

static UINT32 s3c240x_irq_regs[0x18/4];
static READ32_HANDLER(s3c240x_irq_r)
{
	return s3c240x_irq_regs[offset];
}

static WRITE32_HANDLER(s3c240x_irq_w)
{
//  printf("%08x to interrupt controller @ 0x144000%02x\n", data, offset<<2);

	COMBINE_DATA(&s3c240x_irq_regs[offset]);
}

#if 0
static const char *timer_reg_names[] =
{
	"Timer config 0",
	"Timer config 1",
	"Timer control",
	"Timer count buffer 0",
	"Timer compare buffer 0",
	"Timer count observation 0",
	"Timer count buffer 1",
	"Timer compare buffer 1",
	"Timer count observation 1",
	"Timer count buffer 2",
	"Timer compare buffer 2",
	"Timer count observation 2",
	"Timer count buffer 3",
	"Timer compare buffer 3",
	"Timer count observation 3",
	"Timer count buffer 4",
	"Timer compare buffer 4",
	"Timer count observation 4",
};
#endif

static emu_timer *s3c240x_timers[5];
static UINT32 s3c240x_timer_regs[0x44/4];
static READ32_HANDLER(s3c240x_timer_r)
{
	return s3c240x_timer_regs[offset];
}

static void s3c240x_timer_recalc(running_machine *machine, int timer, UINT32 ctrl_bits, UINT32 count_reg)
{
	if (ctrl_bits & 1)	// timer start?
	{
//      printf("starting timer %d\n", timer);
	}
	else	// stopping is easy
	{
//      printf("stopping timer %d\n", timer);
		timer_adjust_oneshot(s3c240x_timers[timer], attotime_never, 0);
	}
}

static WRITE32_HANDLER(s3c240x_timer_w)
{
	UINT32 old_value = s3c240x_timer_regs[offset];

//  printf("%08x to timer @ 0x151000%02x (%s)\n", data, offset<<2, timer_reg_names[offset]);

	COMBINE_DATA(&s3c240x_timer_regs[offset]);

	if (offset == 2)	// TCON
	{
		if ((data & 1) != (old_value & 1))	// timer 0 status change
		{
			s3c240x_timer_recalc(space->machine, 0, data&0xf, 0xc);
		}

		if ((data & 0x100) != (old_value & 0x100))	// timer 1 status change
		{
			s3c240x_timer_recalc(space->machine, 1, (data>>8)&0xf, 0x18);
		}

		if ((data & 0x1000) != (old_value & 0x1000))	// timer 2 status change
		{
			s3c240x_timer_recalc(space->machine, 2, (data>>12)&0xf, 0x24);
		}

		if ((data & 0x10000) != (old_value & 0x10000))	// timer 3 status change
		{
			s3c240x_timer_recalc(space->machine, 3, (data>>16)&0xf, 0x30);
		}

		if ((data & 0x100000) != (old_value & 0x100000))	// timer 4 status change
		{
			s3c240x_timer_recalc(space->machine, 4, (data>>20)&0xf, 0x3c);
		}
	}
}

static TIMER_CALLBACK( s3c240x_timer_exp )
{
	int ch = param;

	timer_adjust_oneshot(s3c240x_timers[ch], attotime_never, 0);
}

static void s3c240x_machine_start(running_machine *machine)
{
	s3c240x_timers[0] = timer_alloc(machine, s3c240x_timer_exp, (void *)(FPTR)0);
	s3c240x_timers[1] = timer_alloc(machine, s3c240x_timer_exp, (void *)(FPTR)1);
	s3c240x_timers[2] = timer_alloc(machine, s3c240x_timer_exp, (void *)(FPTR)2);
	s3c240x_timers[3] = timer_alloc(machine, s3c240x_timer_exp, (void *)(FPTR)3);
	s3c240x_timers[4] = timer_alloc(machine, s3c240x_timer_exp, (void *)(FPTR)4);
}

static UINT32 s3c240x_gpio[0x60/4];

static READ32_HANDLER( s3c240x_gpio_r )
{
//  printf("read GPIO @ 0x156000%02x (PC %x)\n", offset*4, cpu_get_pc(space->cpu));

	return s3c240x_gpio[offset];
}

static WRITE32_HANDLER( s3c240x_gpio_w )
{
	COMBINE_DATA(&s3c240x_gpio[offset]);

//  printf("%08x to GPIO @ 0x156000%02x (PC %x)\n", data, offset*4, cpu_get_pc(space->cpu));

//  if (offset == 0x30/4) printf("[%08x] %s %s\n", data, (data & 0x200) ? "SCL" : "scl", (data & 0x400) ? "SDL" : "sdl");
}

static ADDRESS_MAP_START( gp32_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_ROM
	AM_RANGE(0x0c000000, 0x0c7fffff) AM_RAM AM_BASE(&s3c240x_ram)
	AM_RANGE(0x14400000, 0x14400017) AM_READWRITE(s3c240x_irq_r, s3c240x_irq_w)
	AM_RANGE(0x14a00000, 0x14a003ff) AM_READWRITE(s3c240x_vidregs_r, s3c240x_vidregs_w)
	AM_RANGE(0x14a00400, 0x14a007ff) AM_RAM AM_BASE_GENERIC(paletteram) AM_WRITE(s3c240x_palette_w)
	AM_RANGE(0x15100000, 0x15100043) AM_READWRITE(s3c240x_timer_r, s3c240x_timer_w)
	AM_RANGE(0x15600000, 0x1560005b) AM_READWRITE(s3c240x_gpio_r, s3c240x_gpio_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( gp32 )
INPUT_PORTS_END

static MACHINE_START( gp32 )
{
	s3c240x_machine_start(machine);
}

static MACHINE_DRIVER_START( gp32 )
	MDRV_CPU_ADD("maincpu", ARM9, 40000000)
	MDRV_CPU_PROGRAM_MAP(gp32_map)

	MDRV_PALETTE_LENGTH(32768)

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 0, 239)

	MDRV_VIDEO_UPDATE(gp32)

	MDRV_MACHINE_START(gp32)

	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
MACHINE_DRIVER_END

ROM_START(gp32)
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "gp32157e.bin", 0x000000, 0x080000, CRC(b1e35643) SHA1(1566bc2a27980602e9eb501cf8b2d62939bfd1e5) )
ROM_END

CONS(2001, gp32, 0, 0, gp32, gp32, 0, 0, "Game Park", "GP32", GAME_NOT_WORKING|GAME_NO_SOUND)

