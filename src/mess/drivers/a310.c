/******************************************************************************
 * 
 *	Acorn Archimedes 310
 *
 *	Skeleton: Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *	Enhanced: R. Belmont, June 2007
 *
 *      Memory map (from http://b-em.bbcmicro.com/arculator/archdocs.txt)
 *
 *	0000000 - 1FFFFFF - logical RAM (32 meg)
 *	2000000 - 2FFFFFF - physical RAM (supervisor only - max 16MB - requires quad MEMCs)
 *	3000000 - 33FFFFF - IOC (IO controllers - supervisor only)
 *	3310000 - FDC - WD1772
 *	33A0000 - Econet - 6854
 *	33B0000 - Serial - 6551
 *	3240000 - 33FFFFF - internal expansion cards
 *	32D0000 - hard disc controller (not IDE) - HD63463
 *	3350010 - printer
 *	3350018 - latch A
 *	3350040 - latch B
 *	3270000 - external expansion cards
 *
 *	3400000 - 3FFFFFF - ROM (read - 12 meg - Arthur and RiscOS 2 512k, RiscOS 3 2MB)
 *	3400000 - 37FFFFF - Low ROM  (4 meg, I think this is expansion ROMs)
 *	3800000 - 3FFFFFF - High ROM (main OS ROM)
 *
 *	3400000 - 35FFFFF - VICD10 (write - supervisor only)
 *	3600000 - 3FFFFFF - MEMC (write - supervisor only)
 *
 *****************************************************************************/

#include "driver.h"
#include "machine/wd17xx.h"
#include "video/generic.h"
#include "devices/basicdsk.h"
#include "image.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

static int page_sizes[4] = { 4096, 8192, 16384, 32768 };

static UINT32 *a310_mainram, *a310_physmem;
static UINT32 a310_pagesize;
UINT32 a310_vidregs[256];

VIDEO_START( a310 )
{
}

VIDEO_UPDATE( a310 )
{
	return 0;
}

static MACHINE_RESET( a310 )
{
	// copy startup vectors to where the ARM expects them
	memcpy(a310_mainram, memory_region(REGION_CPU1), 0x30);
}

static void a310_wd177x_callback(wd17xx_state_t event, void *param)
{
}

static MACHINE_START( a310 )
{
	wd17xx_init(WD_TYPE_177X, a310_wd177x_callback, NULL);
}

static READ32_HANDLER(ioc_r)
{
	return 0;
}

static WRITE32_HANDLER(ioc_w)
{
	logerror("IOC: W %x @ %x (mask %08x)\n", data, offset, mem_mask);
}

static READ32_HANDLER(vidc_r)
{
	return 0;
}

static WRITE32_HANDLER(vidc_w)
{
	logerror("VIDC: %x to register %x\n", data & 0xffffff, data>>24);

	a310_vidregs[data>>24] = data & 0xffffff;
}

static READ32_HANDLER(memc_r)
{
	return 0;
}

static WRITE32_HANDLER(memc_w)
{
	// is it a register?
	if ((data & 0x0fe00000) == 0x03600000)
	{
		switch ((data >> 17) & 7)
		{
			case 7:	/* Control */
				a310_pagesize = ((data>>2) & 3);

				logerror("MEMC: %x to Control (page size %d)\n", data & 0x1ffc, page_sizes[a310_pagesize]);
				break;

			default:
				logerror("MEMC: %x to Unk reg %d\n", data&0x1ffff, (data >> 17) & 7);
				break;
		}
	}
	else
	{
		logerror("MEMC non-reg: W %x @ %x (mask %08x)\n", data, offset, mem_mask);
	}	
}

/*
	  22 2222 1111 1111 1100 0000 0000
          54 3210 9876 5432 1098 7654 3210
4k  page: 11 1LLL LLLL LLLL LLAA MPPP PPPP
8k  page: 11 1LLL LLLL LLLM LLAA MPPP PPPP
16k page: 11 1LLL LLLL LLxM LLAA MPPP PPPP
32k page: 11 1LLL LLLL LxxM LLAA MPPP PPPP
	   3   8    2   9    0    f    f

L - logical page
P - physical page
A - access permissions
M - MEMC number (for machines with multiple MEMCs)

The logical page is encoded with bits 11+10 being the most significant bits
(in that order), and the rest being bit 22 down.

The physical page is encoded differently depending on the page size :

4k  page:   bits 6-0 being bits 6-0
8k  page:   bits 6-1 being bits 5-0, bit 0 being bit 6
16k page:   bits 6-2 being bits 4-0, bits 1-0 being bits 6-5
32k page:   bits 6-3 being bits 4-0, bit 0 being bit 4, bit 2 being bit 5, bit
            1 being bit 6




*/
static WRITE32_HANDLER(memc_page_w)
{
	UINT32 log, phys, memc, perms;

//	logerror("MEMC_PAGE: W %x @ %x (mask %08x)\n", data, offset, mem_mask);

	perms = (data & 0x300)>>8;
	log = phys = memc = 0;

	switch (a310_pagesize)
	{
		case 0:
			phys = data & 0x7f;
			log = (data & 0xc00)>>10;
			log <<= 23;
			log |= (data & 0x7ff000);
			memc = (data & 0x80) ? 1 : 0;
			break;

		case 1:
			phys = ((data & 0x7f) >> 1) | (data & 1) ? 0x40 : 0; 
			log = (data & 0xc00)>>10;
			log <<= 23;
			log |= (data & 0x7fe000);
			memc = ((data & 0x80) ? 1 : 0) | ((data & 0x1000) ? 2 : 0);
			break;

		case 2:
			phys = ((data & 0x7f) >> 2) | ((data & 3) << 5);
			log = (data & 0xc00)>>10;
			log <<= 23;
			log |= (data & 0x7fc000);
			memc = ((data & 0x80) ? 1 : 0) | ((data & 0x1000) ? 2 : 0);
			break;

		case 3:
			phys = ((data & 0x7f) >> 3) | (data & 1)<<4 | (data & 2) << 5 | (data & 4)<<3;
			log = (data & 0xc00)>>10;
			log <<= 23;
			log |= (data & 0x7f8000);
			memc = ((data & 0x80) ? 1 : 0) | ((data & 0x1000) ? 2 : 0);
			break;
	}

	logerror("MEMC_PAGE(%d): W %08x: log %x to phys %x, MEMC %d, perms %d\n", a310_pagesize, data, log, phys, memc, perms);
}

static ADDRESS_MAP_START( a310_mem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000000ff) AM_RAM	AM_BASE(&a310_mainram) /* main RAM, we bank this out after boot */
	AM_RANGE(0x02000000, 0x023fffff) AM_RAM AM_BASE(&a310_physmem) /* physical RAM - 4 MB for now */
	AM_RANGE(0x03000000, 0x033fffff) AM_READWRITE(ioc_r, ioc_w)
	AM_RANGE(0x03400000, 0x035fffff) AM_READWRITE(vidc_r, vidc_w)
	AM_RANGE(0x03600000, 0x037fffff) AM_READWRITE(memc_r, memc_w)
	AM_RANGE(0x03800000, 0x03ffffff) AM_ROM AM_REGION(REGION_CPU1, 0) AM_WRITE(memc_page_w)
ADDRESS_MAP_END


INPUT_PORTS_START( a310 )
	PORT_START /* DIP switches */
	PORT_BIT(0xfd, 0xfd, IPT_UNUSED)

	PORT_START /* KEY ROW 0 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("1  !") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("2  \"") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("3  #") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("4  $") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("5  %") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("6  &") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("7  '") PORT_CODE(KEYCODE_7)

	PORT_START /* KEY ROW 1 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("8  *") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("9  (") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("0  )") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("-  _") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("=  +") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("`  ~") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("BACK SPACE") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)

	PORT_START /* KEY ROW 2 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("q  Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("w  W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("e  E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("r  R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("t  T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("y  Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("u  U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("i  I") PORT_CODE(KEYCODE_I)

	PORT_START /* KEY ROW 3 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("o  O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("p  P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("[  {") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("]  }") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE

	PORT_START /* KEY ROW 4 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("a  A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("s  S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("d  D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("f  F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("g  G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("h  H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("j  J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("k  K") PORT_CODE(KEYCODE_K)

	PORT_START /* KEY ROW 5 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("l  L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME(";  :") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("'  \"") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("\\  |") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("SHIFT (L)") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("z  Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("x  X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("c  C") PORT_CODE(KEYCODE_C)

	PORT_START /* KEY ROW 6 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("v  V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("b  B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("n  N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("m  M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME(",  <") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME(".  >") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("/  ?") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("SHIFT (R)") PORT_CODE(KEYCODE_RSHIFT)

	PORT_START /* KEY ROW 7 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("LINE FEED")
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("- (KP)") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME(", (KP)") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("ENTER (KP)") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME(". (KP)") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("0 (KP)") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("1 (KP)") PORT_CODE(KEYCODE_1_PAD)

	PORT_START /* KEY ROW 8 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("2 (KP)") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("3 (KP)") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("4 (KP)") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("5 (KP)") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("6 (KP)") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("7 (KP)") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("8 (KP)") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("9 (KP)") PORT_CODE(KEYCODE_9_PAD)

	PORT_START /* VIA #1 PORT A */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1					 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2					 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_4WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_4WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_4WAY

	PORT_START /* tape control */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("TAPE STOP") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("TAPE PLAY") PORT_CODE(KEYCODE_F6)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("TAPE REW") PORT_CODE(KEYCODE_F7)
	PORT_BIT (0xf8, 0x80, IPT_UNUSED)
INPUT_PORTS_END


static MACHINE_DRIVER_START( a310 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", ARM, 8000000)        /* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(a310_mem, 0)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET( a310 )
	MDRV_MACHINE_START( a310 )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 16*16)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8 - 1, 0*16, 16*16 - 1)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(a310)
	MDRV_VIDEO_UPDATE(a310)
MACHINE_DRIVER_END


ROM_START(a310)
	ROM_REGION(0x800000, REGION_CPU1, 0)
	ROM_LOAD("ic24.rom", 0x000000, 0x80000, CRC(c1adde84) SHA1(12d060e0401dd0523d44453f947bdc55dd2c3240))
	ROM_LOAD("ic25.rom", 0x080000, 0x80000, CRC(15d89664) SHA1(78f5d0e6f1e8ee603317807f53ff8fe65a3b3518))
	ROM_LOAD("ic26.rom", 0x100000, 0x80000, CRC(a81ceb7c) SHA1(46b870876bc1f68f242726415f0c49fef7be0c72))
	ROM_LOAD("ic27.rom", 0x180000, 0x80000, CRC(707b0c6c) SHA1(345199a33fed23996374b9db8170a52ab63f0380))

	ROM_REGION(0x00800, REGION_GFX1, 0)
ROM_END

/*    YEAR  NAME  PARENT COMPAT	 MACHINE  INPUT	 INIT  CONFIG  COMPANY	FULLNAME */
COMP( 1988, a310, 0,     0,      a310,    a310,  0,    NULL,   "Acorn", "Archimedes 310", GAME_NOT_WORKING)



