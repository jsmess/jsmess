/*

	TODO:

	- proper emulation of the monstrous keyboard

*/

#include "driver.h"
#include "inputx.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "machine/6821pia.h"
#include "sound/sn76496.h"
#include "video/tms9928a.h"

/* Memory Map */

static ADDRESS_MAP_START( crvision_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM AM_MIRROR(0x0c00)
	AM_RANGE(0x1000, 0x1003) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x2000, 0x2000) AM_READ(TMS9928A_vram_r)
	AM_RANGE(0x2001, 0x2001) AM_READ(TMS9928A_register_r)
	AM_RANGE(0x3000, 0x3000) AM_WRITE(TMS9928A_vram_w)
	AM_RANGE(0x3001, 0x3001) AM_WRITE(TMS9928A_register_w)
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(2)
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xc7ff) AM_ROM AM_MIRROR(0x3800)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( crvision )
	
	// Player 1 Joystick

	PORT_START_TAG("PA0-0")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA0-1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0xfd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA0-2")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0xf3, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA0-3")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0xf7, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA0-4")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA0-5")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0xdf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA0-6")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA0-7")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Button 2 / CNT'L") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)

	// Player 1 Keyboard

	PORT_START_TAG("PA1-0")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("<-") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x81, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA1-1")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x83, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA1-2")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x87, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA1-3")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x8f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA1-4")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x9f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA1-5")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0xbf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA1-6")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA1-7")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Button 1 / SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)

	// Player 2 Joystick

	PORT_START_TAG("PA2-0")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA2-1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0xfd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA2-2")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0xf3, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA2-3")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0xf7, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA2-4")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA2-5")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0xdf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA2-6")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA2-7")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P2 Button 2 / ->") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9) PORT_PLAYER(2)

	// Player 2 Keyboard

	PORT_START_TAG("PA3-0")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RET'N") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x81, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA3-1")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('@')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x83, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA3-2")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x87, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA3-3")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x8f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA3-4")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x9f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA3-5")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0xbf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA3-6")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PA3-7")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P2 Button 1 / - =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=') PORT_PLAYER(2)
INPUT_PORTS_END

/* Machine Interface */

static INTERRUPT_GEN( crvision_int )
{
    TMS9928A_interrupt();
}

static void crvision_vdp_interrupt(int state)
{
	cpunum_set_input_line(0, INPUT_LINE_IRQ0, state);
}

static const TMS9928a_interface tms9918_intf =
{
	TMS99x8,
	0x4000,
	0, 0,
	crvision_vdp_interrupt
};

static const TMS9928a_interface tms9929_intf =
{
	TMS9929,
	0x4000,
	0, 0,
	crvision_vdp_interrupt
};

static int keylatch;

static WRITE8_HANDLER( pia_porta_w )
{
	/*
		Signal	Description

		PA0		Keyboard raster player 1 output
		PA1		Keyboard raster player 1 output
		PA2		Keyboard raster player 2 output
		PA3		Keyboard raster player 2 output
		PA4		?
		PA5		?
		PA6		?
		PA7		?
	*/

	keylatch = ~data & 0x0f;
}

static UINT8 read_keyboard(int pa)
{
	int i;
	UINT8 value;
	char portname[10];
	
	for (i = 0; i < 8; i++)
	{
		sprintf(portname, "PA%u-%u", pa, i);
		value = readinputportbytag(portname);
		
		if (value != 0xff)
		{
			if (value == 0xff - (1 << i))
				return value;
			else
				return value - (1 << i);
		}
	}

	return 0xff;
}

static READ8_HANDLER( pia_porta_r )
{
	/*
		PA0		Keyboard raster player 1 output
		PA1		Keyboard raster player 1 output
		PA2		Keyboard raster player 2 output
		PA3		Keyboard raster player 2 output
		PA4		?
		PA5		?
		PA6		?
		PA7		?
	*/

	return 0xff;
}

static READ8_HANDLER( pia_portb_r )
{
	/*
		Signal	Description

		PB0		Keyboard input
		PB1		Keyboard input
		PB2		Keyboard input
		PB3		Keyboard input
		PB4		Keyboard input
		PB5		Keyboard input
		PB6		Keyboard input
		PB7		Keyboard input
	*/

	if (keylatch & 0x01)
	{
		return read_keyboard(0);
	}
	else if (keylatch & 0x02)
	{
		return read_keyboard(1);
	}
	else if (keylatch & 0x04)
	{
		return read_keyboard(2);
	}
	else if (keylatch & 0x08)
	{
		return read_keyboard(3);
	}

	return 0xff;
}

static READ8_HANDLER( pia_ca1_r )
{
	return 1;
}

static READ8_HANDLER( pia_ca2_r )
{
	return 1;
}

static int sn76489_ready;

static READ8_HANDLER( pia_cb1_r )
{
	return sn76489_ready;
}

static TIMER_CALLBACK(sn76489_set_ready)
{
	sn76489_ready = 1;
}

static WRITE8_HANDLER( pia_portb_w )
{
	/*
		Signal	Description

		PB0		SN76489 data output
		PB1		SN76489 data output
		PB2		SN76489 data output
		PB3		SN76489 data output
		PB4		SN76489 data output
		PB5		SN76489 data output
		PB6		SN76489 data output
		PB7		SN76489 data output
	*/

	SN76496_0_w(0, data);
	
	sn76489_ready = 0;

	// wait 32 cycles of 2 MHz to synchronize CPU and SN76489
	mame_timer_set(MAME_TIME_IN_USEC(16), 0, sn76489_set_ready); 
}

static WRITE8_HANDLER( pia_cb2_w )
{
	sn76489_ready = data & 0x01;
}

static const pia6821_interface crvision_pia_intf =
{
	pia_porta_r,	// input A
	pia_portb_r,	// input B
	pia_ca1_r,		// input CA1 (+5V)
	pia_cb1_r,		// input CB1 (SN76489 pin READY )
	pia_ca2_r,		// input CA2 (+5V)
	0,				// input CB2
	pia_porta_w,	// output A
	pia_portb_w,	// output B (SN76489 pins D0-D7)
	0,				// output CA2
	pia_cb2_w,		// output CB2 (SN76489 pin CE_)
	0,				// irq A
	0				// irq B
};

static MACHINE_START( crvision )
{
	state_save_register_global(keylatch);
	state_save_register_global(sn76489_ready);

	TMS9928A_configure(&tms9918_intf);
	pia_config(0, &crvision_pia_intf);
}

static MACHINE_START( fnvision )
{
	state_save_register_global(keylatch);
	state_save_register_global(sn76489_ready);

	TMS9928A_configure(&tms9929_intf);
	pia_config(0, &crvision_pia_intf);
}

static MACHINE_RESET( crvision )
{
	pia_reset();
}

/* Machine Driver */

static MACHINE_DRIVER_START( crvision )
	// basic machine hardware
	MDRV_CPU_ADD(M6502, 2000000)
	MDRV_CPU_PROGRAM_MAP(crvision_map, 0)
	MDRV_CPU_VBLANK_INT(crvision_int, 1)
	MDRV_SCREEN_REFRESH_RATE(10738635.0/2/342/262)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START( crvision )
	MDRV_MACHINE_RESET( crvision )

    // video hardware
	MDRV_IMPORT_FROM(tms9928a)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76489, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fnvision )
	MDRV_IMPORT_FROM( crvision )
	MDRV_MACHINE_START( fnvision )
	MDRV_SCREEN_REFRESH_RATE(10738635.0/2/342/313)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( crvision )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "crvision.rom", 0xc000, 0x0800, CRC(c3c590c6) SHA1(5ac620c529e4965efb5560fe824854a44c983757) )
ROM_END

ROM_START( fnvision )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "funboot.rom", 0xc000, 0x0800, CRC(05602697) SHA1(c280b20c8074ba9abb4be4338b538361dfae517f) )
ROM_END

/* System Configuration */

static DEVICE_LOAD( crvision_cart )
{
	int size = image_length(image);

	switch (size)
	{
	case 0x1000: // 4K
		image_fread(image, memory_region(REGION_CPU1) + 0x9000, 0x1000);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0x2000, MRA8_BANK1);
		break;

	case 0x1800: // 6K
		image_fread(image, memory_region(REGION_CPU1) + 0x9000, 0x1000);
		image_fread(image, memory_region(REGION_CPU1) + 0x8800, 0x0800);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0x2000, MRA8_BANK1);
		break;

	case 0x2000: // 8K
		image_fread(image, memory_region(REGION_CPU1) + 0x8000, 0x2000);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0x2000, MRA8_BANK1);
		break;

	case 0x2800: // 10K
		image_fread(image, memory_region(REGION_CPU1) + 0x8000, 0x2000);
		image_fread(image, memory_region(REGION_CPU1) + 0x5800, 0x0800);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0x2000, MRA8_BANK1);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0x2000, MRA8_BANK2);
		break;

	case 0x3000: // 12K
		image_fread(image, memory_region(REGION_CPU1) + 0x8000, 0x2000);
		image_fread(image, memory_region(REGION_CPU1) + 0x5000, 0x1000);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0x2000, MRA8_BANK1);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0x2000, MRA8_BANK2);
		break;

	case 0x4000: // 16K
		image_fread(image, memory_region(REGION_CPU1) + 0xa000, 0x2000);
		image_fread(image, memory_region(REGION_CPU1) + 0x8000, 0x2000);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_BANK1);
		break;

	case 0x4800: // 18K
		image_fread(image, memory_region(REGION_CPU1) + 0xa000, 0x2000);
		image_fread(image, memory_region(REGION_CPU1) + 0x8000, 0x2000);
		image_fread(image, memory_region(REGION_CPU1) + 0x4800, 0x0800);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, 0, MRA8_BANK1);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4fff, 0, 0x3000, MRA8_BANK2);
		break;

	default:
		return INIT_FAIL;
	}

	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1) + 0x8000, 0);
	memory_set_bank(1, 0);

	memory_configure_bank(2, 0, 1, memory_region(REGION_CPU1) + 0x4000, 0);
	memory_set_bank(2, 0);

	return INIT_PASS;
}

static void crvision_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_crvision_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "rom"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void crvision_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( crvision )
	CONFIG_DEVICE(crvision_cartslot_getinfo)
	CONFIG_DEVICE(crvision_cassette_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( fnvision )
	CONFIG_DEVICE(crvision_cartslot_getinfo)
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR	NAME	  PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG      COMPANY				FULLNAME */
COMP( 1981, crvision, 0,		0,		crvision,	crvision,	0,		crvision,	"Video Technology", "CreatiVision (NTSC)", GAME_SUPPORTS_SAVE )
CONS( 1983, fnvision, crvision, 0,		fnvision,	crvision,	0,		fnvision,	"Video Technology", "FunVision Computer Video Games System (PAL)", GAME_SUPPORTS_SAVE )
