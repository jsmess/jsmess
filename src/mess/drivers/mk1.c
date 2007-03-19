/******************************************************************************
 PeT mess@utanet.at 2000,2001
******************************************************************************/

#include "driver.h"

#include "cpu/f8/f8.h"
#include "cpu/f8/f3853.h"
#include "includes/mk1.h"

/*
chess champion mk i


Signetics 7916E C48091 82S210-1 COPYRIGHT
2 KB rom? 2716 kompatible?

4 11 segment displays (2. point up left, verticals in the mid )

2 x 2111 256x4 SRAM

MOSTEK MK 3853N 7915 Philippines ( static memory interface for f8)

MOSTEK MK 3850N-3 7917 Philipines (fairchild f8 cpu)

16 keys (4x4 matrix)

switch s/l (dark, light) pure hardware?
(power on switch)

speaker?

 */

#ifdef MAME_DEBUG
#define VERBOSE	1
#else
#define VERBOSE 0
#endif

static UINT8 mk1_f8[2];

static  READ8_HANDLER(mk1_f8_r)
{
    UINT8 data = mk1_f8[offset];

#if VERBOSE
	logerror ("f8 %.6f r %x %x\n", timer_get_time(), offset, data);
#endif

    if (offset==0)
	{
		if (data&1) data|=readinputport(1);
		if (data&2) data|=readinputport(2);
		if (data&4) data|=readinputport(3);
		if (data&8) data|=readinputport(4);
		if (data&0x10)
		{
			if (readinputport(1)&0x10) data|=1;
			if (readinputport(2)&0x10) data|=2;
			if (readinputport(3)&0x10) data|=4;
			if (readinputport(4)&0x10) data|=8;
		}
		if (data&0x20)
		{
			if (readinputport(1)&0x20) data|=1;
			if (readinputport(2)&0x20) data|=2;
			if (readinputport(3)&0x20) data|=4;
			if (readinputport(4)&0x20) data|=8;
		}
		if (data&0x40)
		{
			if (readinputport(1)&0x40) data|=1;
			if (readinputport(2)&0x40) data|=2;
			if (readinputport(3)&0x40) data|=4;
			if (readinputport(4)&0x40) data|=8;
		}
		if (data&0x80)
		{
			if (readinputport(1)&0x80) data|=1;
			if (readinputport(2)&0x80) data|=2;
			if (readinputport(3)&0x80) data|=4;
			if (readinputport(4)&0x80) data|=8;
		}
    }
    return data;
}

static WRITE8_HANDLER(mk1_f8_w)
{
	/* 0 is high and allows also input */
	mk1_f8[offset]=data;

#if VERBOSE
	logerror("f8 %.6f w %x %x\n", timer_get_time(), offset, data);
#endif

	if (!(mk1_f8[1]&1)) mk1_led[0]=mk1_f8[0];
	if (!(mk1_f8[1]&2)) mk1_led[1]=mk1_f8[0];
	if (!(mk1_f8[1]&4)) mk1_led[2]=mk1_f8[0];
	if (!(mk1_f8[1]&8)) mk1_led[3]=mk1_f8[0];
}

static ADDRESS_MAP_START( mk1_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x1800, 0x18ff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( mk1_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0, 0x1) AM_READWRITE( mk1_f8_r, mk1_f8_w )
	AM_RANGE(0xc, 0xf) AM_READWRITE( f3853_r, f3853_w )
ADDRESS_MAP_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BIT(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(name) PORT_CODE(keycode) PORT_CODE(r)

INPUT_PORTS_START( mk1 )
	PORT_START
	PORT_DIPNAME ( 0x01, 0x01, "Switch")
	PORT_DIPSETTING(  0, "L" )
	PORT_DIPSETTING(  1, "S" )
	PORT_START
	PORT_BIT ( 0x0f, 0x0,	 IPT_UNUSED )
	DIPS_HELPER( 0x80, "White A    King", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x40, "White B    Queen", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x20, "White C    Bishop", KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x10, "White D    PLAY", KEYCODE_D, CODE_NONE)
	PORT_START
	PORT_BIT ( 0x0f, 0x0,	 IPT_UNUSED )
	DIPS_HELPER( 0x80, "White E    Knight", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x40, "White F    Castle", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x20, "White G    Pawn", KEYCODE_G, CODE_NONE)
	DIPS_HELPER( 0x10, "White H    md", KEYCODE_H, CODE_NONE)
	PORT_START
	PORT_BIT ( 0x0f, 0x0,	 IPT_UNUSED )
	DIPS_HELPER( 0x80, "Black 1    King", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x40, "Black 2    Queen", KEYCODE_2, CODE_NONE)
	DIPS_HELPER( 0x20, "Black 3    Bishop", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x10, "Black 4    fp", KEYCODE_4, CODE_NONE)
	PORT_START
	PORT_BIT ( 0x0f, 0x0,	 IPT_UNUSED )
	DIPS_HELPER( 0x80, "Black 5    Knight", KEYCODE_5, CODE_NONE)
	DIPS_HELPER( 0x40, "Black 6    Castle", KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x20, "Black 7    Pawn", KEYCODE_7, CODE_NONE)
	DIPS_HELPER( 0x10, "Black 8    ep", KEYCODE_8, CODE_NONE)
INPUT_PORTS_END

static MACHINE_RESET( mk1 )
{
    f3853_reset();
}

static MACHINE_DRIVER_START( mk1 )
	/* basic machine hardware */
	MDRV_CPU_ADD(F8, 1000000)        /* MK3850 */
	MDRV_CPU_PROGRAM_MAP(mk1_mem, 0)
	MDRV_CPU_IO_MAP(mk1_io, 0)
	MDRV_SCREEN_REFRESH_RATE(LCD_FRAMES_PER_SECOND)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( mk1 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(626, 323)
	MDRV_SCREEN_VISIBLE_AREA(0, 626-1, 0, 323-1)
	MDRV_PALETTE_LENGTH(242 + 32768)
	MDRV_COLORTABLE_LENGTH(2)
	MDRV_PALETTE_INIT( mk1 )

	MDRV_VIDEO_START( mk1 )
	MDRV_VIDEO_UPDATE( mk1 )
MACHINE_DRIVER_END

ROM_START(mk1)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("82c210-1", 0x0000, 0x800, CRC(278f7bf3) SHA1(b384c95ba691d52dfdddd35987a71e9746a46170))
ROM_END

static void mk1_interrupt(UINT16 addr, bool level)
{
    cpunum_set_input_line_vector(0, 0, addr);
    cpunum_set_input_line(0, F8_INT_INTR, level);
}

static DRIVER_INIT( mk1 )
{
    F3853_CONFIG config;
	machine_config drv;

	construct_mk1(&drv);

    config.frequency = drv.cpu[0].cpu_clock;
    config.interrupt_request=mk1_interrupt;

    f3853_init(&config);
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

// seams to be developed by mostek (MK)
/*     YEAR   NAME  PARENT  COMPAT	MACHINE INPUT   INIT	CONFIG	COMPANY                 FULLNAME */
CONS( 1979,  mk1,  0, 		0,		mk1,	mk1,	mk1,	NULL,	"Computer Electronic",  "Chess Champion MK I", GAME_NOT_WORKING)

