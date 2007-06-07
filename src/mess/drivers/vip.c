#include "driver.h"
#include "image.h"
#include "inputx.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cassette.h"
#include "sound/beep.h"
#include "video/cdp1861.h"

extern int cdp1861_efx;

/* Read/Write Handlers */

static UINT8 keylatch;

static WRITE8_HANDLER( keylatch_w )
{
	keylatch = data & 0x0f;
}

static WRITE8_HANDLER( bankswitch_w )
{
	memory_set_bank(1, 0);
}

/* Memory Maps */

static ADDRESS_MAP_START( vip_map, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x03ff) AM_RAMBANK(1)
	AM_RANGE(0x0400, 0x0fff) AM_RAM
	AM_RANGE(0x8000, 0x83ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vip_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READWRITE(cdp1861_dispon_r, cdp1861_dispoff_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(bankswitch_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( vip )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0 MW") PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A MR") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B TR") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F TW") PORT_CODE(KEYCODE_F)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Run/Reset") PORT_CODE(KEYCODE_R) PORT_TOGGLE
INPUT_PORTS_END

/* CDP1802 Configuration */

static UINT8 vip_mode_r(void)
{
	if (readinputport(1) & 0x01)
	{
		return CDP1802_MODE_RUN;
	}
	else
	{
		return CDP1802_MODE_RESET;
	}
}

static UINT8 vip_ef_r(void)
{
	int ef = 0x0f;

	if (cdp1861_efx) ef -= EF1;
	// EF2 = tape (high when tone read)
	if (readinputport(0) & (1 << keylatch)) ef -= EF3;

	return ef;
}

static void vip_q_w(int level)
{
	beep_set_state(0, level);
	// PWR Q TAPE
	set_led_status(1, level);
}

static CDP1802_CONFIG vip_config = 
{
	vip_mode_r,
	vip_ef_r,
	NULL,
	vip_q_w,
	NULL,
	cdp1861_dma_w
};

/* Machine Initialization */

static MACHINE_START( vip )
{
	state_save_register_global(keylatch);

	memory_configure_bank(1, 0, 2, memory_region(REGION_CPU1), 0x8000);
}

static MACHINE_RESET( vip )
{
	machine_reset_cdp1861(machine);
	memory_set_bank(1, 1);
	cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( vip )

	// basic machine hardware

	MDRV_CPU_ADD(CDP1802, 3579545/2)
	MDRV_CPU_PROGRAM_MAP(vip_map, 0)
	MDRV_CPU_IO_MAP(vip_io_map, 0)
	MDRV_CPU_CONFIG(vip_config)

	MDRV_MACHINE_START(vip)
	MDRV_MACHINE_RESET(vip)

    // video hardware

	MDRV_SCREEN_ADD("main", 0)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(3579545/2, CDP1861_SCREEN_WIDTH, CDP1861_HBLANK_END, CDP1861_HBLANK_START, CDP1861_TOTAL_SCANLINES, CDP1861_SCANLINE_VBLANK_END, CDP1861_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(cdp1861)
	MDRV_VIDEO_UPDATE(cdp1861)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( vip )
	ROM_REGION( 0x10000,REGION_CPU1, 0 )
	ROM_LOAD( "monitor.rom", 0x8000, 0x0200, CRC(5be0a51f) SHA1(40266e6d13e3340607f8b3dcc4e91d7584287c06) )
	ROM_LOAD( "chip8.rom",	 0x8200, 0x0200, CRC(3e0f50f0) SHA1(4a9759035f99d125859cdf6ad71b8728637dcf4f) )
ROM_END

/* System Configuration */

static void vip_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( vip )
	CONFIG_RAM_DEFAULT	( 2 * 1024)
	CONFIG_RAM			( 4 * 1024)
	CONFIG_RAM			(32 * 1024)
	CONFIG_DEVICE(vip_cassette_getinfo)
SYSTEM_CONFIG_END

/* Driver Initialization */

static void setup_beep(int dummy)
{
	beep_set_state( 0, 0 );
	beep_set_frequency( 0, 300 );
}

static DRIVER_INIT( vip )
{
	// enable power led
	set_led_status(0, 1);

	memory_region(REGION_CPU1)[0x8022] = 0x3e; //bn3, default monitor

	timer_set(0.0, 0, setup_beep);
}

/* System Drivers */

//    YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT		CONFIG      COMPANY   FULLNAME
COMP( 1977,	vip,		0,		0,		vip,		vip,		vip,		vip,	"RCA",		"Cosmac VIP (VP-111)", GAME_SUPPORTS_SAVE )
