/*

    TODO:

	- CDP1862 colorram
    - pcb layout guru-style readme
    - artwork for leds
    - VP-550/551 Super Sound Board (VP-550 2 channel, VP-551 4 channel sound)
	- VP-580 Expansion Keyboard (connects to VP-585 or VP-590)
	- VP-585 Expansion Keyboard Interface (2 keypad connectors for VP-580)
    - VP-590 Color Board (CDP1862, 2 keypad connectors for VP-580)
    - VP-595 Simple Sound Board (CDP1863?)
    - VP-601/611 ASCII Keyboard (VP-601 58 keys, VP611 58 keys + 16 keys numerical keypad)
    - VP-700 Expanded Tiny Basic Board (4 KB ROM expansion)

	- VIP Blockout

		1. This game is programmed in color and has sound effects.  It can be used with the
		VP590 Color Board and VP595 Simple Sound Board, or it will run on a standard VIP
		without color and sound enhancement.

		2. This game requires a minimum of one VIP expansion keyboard (VP580) or two expansion
		keyboards for exciting "Dual action".  Expansion keyboards plug directly into the
		VP590 Color graphics board or into the VP585 expansion keyboard interface.

*/

#include "driver.h"
#include "includes/vip.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "sound/beep.h"
#include "sound/discrete.h"
#include "video/cdp1861.h"
#include "video/cdp1862.h"
#include "machine/rescap.h"

static QUICKLOAD_LOAD( vip );
static MACHINE_RESET( vip );

/* Cassette Image */

static const device_config *cassette_device_image(running_machine *machine)
{
	return device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette" );
}

/* Discrete Sound */

static const discrete_555_desc vip_ca555_a =
{
	DISC_555_OUT_SQW | DISC_555_OUT_DC,
	5,		// B+ voltage of 555
	DEFAULT_555_VALUES
};

DISCRETE_SOUND_START( vip )
	DISCRETE_INPUT_LOGIC(NODE_01)
	DISCRETE_555_ASTABLE_CV(NODE_02, NODE_01, 470, RES_M(1), CAP_P(470), NODE_01, &vip_ca555_a)
	DISCRETE_OUTPUT(NODE_02, 5000)
DISCRETE_SOUND_END

/* Read/Write Handlers */

static WRITE8_HANDLER( keylatch_w )
{
	vip_state *state = machine->driver_data;

	state->keylatch = data & 0x0f;
}

static WRITE8_HANDLER( bankswitch_w )
{
	/* enable RAM */

	memory_set_bank(1, VIP_BANK_RAM);

	switch (mess_ram_size)
	{
	case 1 * 1024:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x03ff, 0, 0x7c00, SMH_BANK1, SMH_BANK1);
		break;

	case 2 * 1024:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07ff, 0, 0x7800, SMH_BANK1, SMH_BANK1);
		break;

	case 4 * 1024:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x0fff, 0, 0x7000, SMH_BANK1, SMH_BANK1);
		break;

	case 32 * 1024:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x7fff, 0, 0, SMH_BANK1, SMH_BANK1);
		break;
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( vip_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( vip_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1861, CDP1861_TAG, cdp1861_dispon_r, cdp1861_dispoff_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(bankswitch_w)
//	AM_RANGE(0x05, 0x05) AM_DEVWRITE(CDP1862, CDP1862_TAG, cdp1862_bkg_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( vip )
	PORT_START("KEYPAD1")
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

	PORT_START("RUN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Run/Reset") PORT_CODE(KEYCODE_R) PORT_TOGGLE

	PORT_START("KEYBOARD")
	PORT_CONFNAME( 0x07, 0x00, "Keyboard")
	PORT_CONFSETTING( 0x00, "Standard" )
	PORT_CONFSETTING( 0x01, "VP-580 Expansion Keyboard" )
	PORT_CONFSETTING( 0x02, "2x VP-580 Expansion Keyboard" )
	PORT_CONFSETTING( 0x03, "VP-601 ASCII Keyboard" )
	PORT_CONFSETTING( 0x04, "VP-611 ASCII Keyboard" )
	
	PORT_START("VIDEO")
	PORT_CONFNAME( 0x01, 0x00, "Video")
	PORT_CONFSETTING( 0x00, "Standard" )
	PORT_CONFSETTING( 0x01, "VP-590 Color Board" )

	PORT_START("SOUND")
	PORT_CONFNAME( 0x03, 0x00, "Sound")
	PORT_CONFSETTING( 0x00, DEF_STR( None ) )
	PORT_CONFSETTING( 0x01, "VP-595 Simple Sound Board" )
	PORT_CONFSETTING( 0x02, "VP-550 Super Sound Board" )
	PORT_CONFSETTING( 0x03, "VP-551 Super Sound Board" )
INPUT_PORTS_END

/* Video */

static CDP1861_ON_INT_CHANGED( vip_int_w )
{
	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_INT, level);
}

static CDP1861_ON_DMAO_CHANGED( vip_dmao_w )
{
	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_DMAOUT, level);
}

static CDP1861_ON_EFX_CHANGED( vip_efx_w )
{
	vip_state *state = device->machine->driver_data;

	state->cdp1861_efx = level;
}

static CDP1861_INTERFACE( vip_cdp1861_intf )
{
	SCREEN_TAG,
	XTAL_3_52128MHz,
	vip_int_w,
	vip_dmao_w,
	vip_efx_w
};

static CDP1862_INTERFACE( vip_cdp1862_intf )
{
	SCREEN_TAG,
	CPD1862_CLOCK,
	RES_K(1.21), // ???
	RES_K(2.05), // ???
	RES_K(2.26), // ???
	RES_K(3.92)	 // ???
};

static VIDEO_UPDATE( vip )
{
	vip_state *state = screen->machine->driver_data;

	switch (input_port_read(screen->machine, "VIDEO"))
	{
	case VIP_VIDEO_CDP1861:
		{
			cdp1861_update(state->cdp1861, bitmap, cliprect);
		}
		break;

	case VIP_VIDEO_CDP1862:
		{
			cdp1862_update(state->cdp1862, bitmap, cliprect);
		}
		break;
	}

	return 0;
}

/* CDP1802 Configuration */

static CDP1802_MODE_READ( vip_mode_r )
{
	vip_state *state = machine->driver_data;

	if (input_port_read(machine, "RUN") & 0x01)
	{
		if (state->reset)
		{
			state->reset = 0;
		}

		return CDP1802_MODE_RUN;
	}
	else
	{
		if (!state->reset)
		{
			MACHINE_RESET_CALL(vip);

			state->reset = 1;
		}

		return CDP1802_MODE_RESET;
	}
}

static CDP1802_EF_READ( vip_ef_r )
{
	vip_state *state = machine->driver_data;

	UINT8 flags = 0x0f;

	/*
        EF1     CDP1861
        EF2     tape in
        EF3     keyboard
        EF4     extended keyboard
    */

	/* CDP1861 */
	if (state->cdp1861_efx) flags -= EF1;

	/* tape input */
	if (cassette_input(cassette_device_image(machine)) < 0) flags -= EF2;

	/* keyboard */
	if (input_port_read(machine, "KEYPAD1") & (1 << state->keylatch)) flags -= EF3;

	/* extended keyboard */
//	if (input_port_read(machine, "KEYPAD2") & (1 << state->keylatch)) flags -= EF4;

	return flags;
}

static CDP1802_Q_WRITE( vip_q_w )
{
	/* sound output */
	discrete_sound_w(machine, NODE_01, level);

	/* Q led */
	set_led_status(1, level);

	/* tape output */
	cassette_output(cassette_device_image(machine), level ? 1.0 : -1.0);
}

static CDP1802_DMA_WRITE( vip_dma_w )
{
	vip_state *state = machine->driver_data;

	switch (input_port_read(machine, "VIDEO"))
	{
	case VIP_VIDEO_CDP1861:
		{
			cdp1861_dma_w(state->cdp1861, data);
		}
		break;

	case VIP_VIDEO_CDP1862:
		{
			cdp1862_dma_w(state->cdp1862, data, ASSERT_LINE, 1, 1, 1);
		}
		break;
	}
}

static CDP1802_INTERFACE( vip_config )
{
	vip_mode_r,
	vip_ef_r,
	NULL,
	vip_q_w,
	NULL,
	vip_dma_w
};

/* Machine Initialization */

static MACHINE_START( vip )
{
	vip_state *state = machine->driver_data;

	UINT8 *ram = memory_region(machine, CDP1802_TAG);
	UINT16 addr;

	/* RAM banking */

	memory_configure_bank(1, 0, 2, memory_region(machine, CDP1802_TAG), 0x8000);

	/* ROM banking */

	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x81ff, 0, 0x7e00, SMH_BANK2, SMH_UNMAP);
	memory_configure_bank(2, 0, 1, memory_region(machine, CDP1802_TAG) + 0x8000, 0);
	memory_set_bank(2, 0);

	/* randomize RAM contents */

	for (addr = 0; addr < mess_ram_size; addr++)
	{
		ram[addr] = mame_rand(machine) & 0xff;
	}

	/* allocate color RAM */
	
	state->colorram = auto_malloc(VP590_COLOR_RAM_SIZE);

	/* enable power LED */

	set_led_status(0, 1);

	/* look up devices */

	state->cdp1861 = device_list_find_by_tag(machine->config->devicelist, CDP1861, CDP1861_TAG);
	state->cdp1862 = device_list_find_by_tag(machine->config->devicelist, CDP1862, CDP1862_TAG);

	/* register for state saving */

	state_save_register_global_pointer(state->colorram, VP590_COLOR_RAM_SIZE);

	state_save_register_global(state->cdp1861_efx);
	state_save_register_global(state->keylatch);
	state_save_register_global(state->reset);
}

static MACHINE_RESET( vip )
{
	vip_state *state = machine->driver_data;

	/* reset video chips */

	state->cdp1861->reset(state->cdp1861);
	state->cdp1862->reset(state->cdp1862);

	/* map CDP1862 memory */

	switch (input_port_read(machine, "VIDEO"))
	{
	case VIP_VIDEO_CDP1861:
		memory_install_write8_device_handler(state->cdp1862, 0, ADDRESS_SPACE_IO, 0x05, 0x05, 0, 0, SMH_UNMAP);
		break;

	case VIP_VIDEO_CDP1862:
		memory_install_write8_device_handler(state->cdp1862, 0, ADDRESS_SPACE_IO, 0x05, 0x05, 0, 0, cdp1862_bkg_w);
		break;
	}

	/* enable ROM mirror at 0x0000 */

	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x01ff, 0, 0x7e00, SMH_BANK1, SMH_UNMAP);
	memory_set_bank(1, VIP_BANK_ROM);
}

/* Machine Drivers */

static const cassette_config vip_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED
};

static MACHINE_DRIVER_START( vip )
	MDRV_DRIVER_DATA(vip_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(CDP1802_TAG, CDP1802, XTAL_3_52128MHz/2)
	MDRV_CPU_PROGRAM_MAP(vip_map, 0)
	MDRV_CPU_IO_MAP(vip_io_map, 0)
	MDRV_CPU_CONFIG(vip_config)

	MDRV_MACHINE_START(vip)
	MDRV_MACHINE_RESET(vip)

    /* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_3_52128MHz/2, CDP1861_SCREEN_WIDTH, CDP1861_HBLANK_END, CDP1861_HBLANK_START, CDP1861_TOTAL_SCANLINES, CDP1861_SCANLINE_VBLANK_END, CDP1861_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(vip)

	MDRV_DEVICE_ADD(CDP1861_TAG, CDP1861)
	MDRV_DEVICE_CONFIG(vip_cdp1861_intf)

	MDRV_DEVICE_ADD(CDP1862_TAG, CDP1862)
	MDRV_DEVICE_CONFIG(vip_cdp1862_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(vip)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	/* devices */
	MDRV_QUICKLOAD_ADD(vip, "bin,c8,c8x", 0)

	MDRV_CASSETTE_ADD( "cassette", vip_cassette_config )
MACHINE_DRIVER_END

/* ROMs */

ROM_START( vip )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "cdpr566.u10", 0x8000, 0x0200, CRC(5be0a51f) SHA1(40266e6d13e3340607f8b3dcc4e91d7584287c06) )
	ROM_SYSTEM_BIOS( 0, "vp711", "VP-711" )
	ROMX_LOAD( "vp711.bin",	 0x9000, 0x1000, NO_DUMP, ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "vp700", "VP-700 Tiny BASIC" )
	ROMX_LOAD( "vp700.bin",	 0x9000, 0x1000, NO_DUMP, ROM_BIOS(2) )

	ROM_REGION( 0x200, "chip8", 0 )
	ROM_LOAD( "chip8.bin", 0x0000, 0x0200, CRC(438ec5d5) SHA1(8aa634c239004ff041c9adbf9144bd315ab5fc77) )

	ROM_REGION( 0x300, "chip8x", 0 )
	ROM_LOAD( "chip8x.bin", 0x0000, 0x0300, CRC(79c5f6f8) SHA1(ed438747b577399f6ccbf20fe14156f768842898) )
ROM_END

ROM_START( vp111 )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "cdpr566.u10", 0x8000, 0x0200, CRC(5be0a51f) SHA1(40266e6d13e3340607f8b3dcc4e91d7584287c06) )
	ROM_SYSTEM_BIOS( 0, "vp111", "VP-111" )
	ROM_SYSTEM_BIOS( 1, "vp700", "VP-700 Tiny BASIC" )
	ROMX_LOAD( "vp700.bin",	 0x9000, 0x1000, NO_DUMP, ROM_BIOS(2) )

	ROM_REGION( 0x200, "chip8", 0 )
	ROM_LOAD( "chip8.bin", 0x000, 0x0200, CRC(438ec5d5) SHA1(8aa634c239004ff041c9adbf9144bd315ab5fc77) )

	ROM_REGION( 0x300, "chip8x", 0 )
	ROM_LOAD( "chip8x.bin", 0x0000, 0x0300, CRC(79c5f6f8) SHA1(ed438747b577399f6ccbf20fe14156f768842898) )
ROM_END

/* System Configuration */

static QUICKLOAD_LOAD( vip )
{
	UINT8 *ptr = memory_region(image->machine, CDP1802_TAG);
	UINT8 *chip8_ptr = NULL;
	int chip8_size = 0;
	int size = image_length(image);

	if (strcmp(image_filetype(image), "c8") == 0)
	{
		/* CHIP-8 program */
		chip8_ptr = memory_region(image->machine, "chip8");
		chip8_size = memory_region_length(image->machine, "chip8");
	}
	else if (strcmp(image_filetype(image), "c8x") == 0)
	{
		/* CHIP-8X program */
		chip8_ptr = memory_region(image->machine, "chip8x");
		chip8_size = memory_region_length(image->machine, "chip8x");
	}

	if ((size + chip8_size) > mess_ram_size)
	{
		return INIT_FAIL;
	}

	if (chip8_size > 0)
	{
		/* copy CHIP-8 interpreter to RAM */
		memcpy(ptr, chip8_ptr, chip8_size);
	}

	/* load image to RAM */
	image_fread(image, ptr + chip8_size, size);

	return INIT_PASS;
}

static SYSTEM_CONFIG_START( vp711 )
	CONFIG_RAM_DEFAULT	( 4 * 1024)
	CONFIG_RAM			(32 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( vp111 )
	CONFIG_RAM_DEFAULT	( 1 * 1024)
	CONFIG_RAM			( 2 * 1024)
	CONFIG_RAM			( 4 * 1024)
SYSTEM_CONFIG_END

/* System Drivers */

//	  YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT   INIT    CONFIG  COMPANY FULLNAME
COMP( 1977, vip,	0,		0,		vip,		vip,	0,		vp711,	"RCA",	"Cosmac VIP (VP-711)", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
COMP( 1977, vp111,	vip,	0,		vip,		vip,	0,		vp111,	"RCA",	"Cosmac VIP (VP-111)", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
