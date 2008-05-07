/*

    telmac.c


    MESS Driver by Curt Coder


    Telmac 1800
    -----------
    (c) 10/1977 Telercas Oy, Finland

    CPU:        CDP1802     1.76 MHz
    RAM:        2 KB (4 KB)
    ROM:        512 B

	Video:		CDP1861

    Designer:   Osmo Kainulainen
    Keyboard:   OS-70 A (QWERTY, hexadecimal keypad)


    OSCOM 1000B
    -----------
    (c) 197? OSCOM Oy, Finland

    CPU:        CDP1802A    ? MHz

    Video:      OSM-200 (1 KB videoram, 10.2 MHz xtal)

    Enhanced Telmac 1800 with built-in CRT board (OSM-200).


    Telmac 2000
    -----------
    (c) 1980 Telercas Oy, Finland

    CPU:        CDP1802A    1.75 MHz
    RAM:        16 KB
    ROM:        512 B / 2 KB (TOOL-2000)

    Video:      CDP1864     1.75 MHz
    Color RAM:  512 B

    Colors:     8 fg, 4 bg
    Resolution: 64x192
    Sound:      frequency control, volume on/off
    Keyboard:   ASCII, KB-16, KB-64


    OSCOM Nano
    ----------
    (c) 1981 OSCOM Oy, Finland

    CPU:        CDP1802A    1.75 MHz
    ROM:        512 B / 1 KB
	RAM:		4 KB

	Video:		CDP1864		1.75 MHz

    Small form factor version of Telmac 1800. Combined text and graphics output.

*/

/*

	TODO:

	- tmc2000: fix display
	- tmc2000: add missing keys
	- tmc2000: TOOL-2000 rom banking

*/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1861.h"
#include "video/cdp1864.h"
#include "devices/cassette.h"
#include "sound/beep.h"
#include "rescap.h"

#define SCREEN_TAG "main"
#define CDP1861_TAG "cdp1861"
#define CDP1864_TAG "cdp1864"

extern VIDEO_START( osm200 );
extern VIDEO_UPDATE( osm200 );

static const device_config *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

/* Read/Write Handlers */

static int keylatch;

static WRITE8_HANDLER( tmc1800_keylatch_w )
{
	keylatch = data;
}

static WRITE8_HANDLER( tmc2000_keylatch_w )
{
	/*

		bit		description
		
		0		X0
		1		X1
		2		X2
		3		Y0
		4		Y1
		5		Y2
		6		EXP1
		7		EXP2

	*/

	keylatch = data & 0x3f;
}

static WRITE8_DEVICE_HANDLER( tmc2000_bankswitch_w )
{
	int bank = data & 0x01;

	// enable RAM

	memory_set_bank(1, 1);

	if (bank)
	{
		// enable Color RAM

		memory_install_readwrite8_handler(device->machine, 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x81ff, 0, 0x0e00, SMH_UNMAP, SMH_BANK2);
	}
	else
	{
		// enable ROM

		memory_install_readwrite8_handler(device->machine, 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x81ff, 0, 0x0e00, SMH_BANK2, SMH_UNMAP);
	}

	memory_set_bank(2, bank);

	cdp1864_tone_latch_w(device, 0, data);
}

static WRITE8_DEVICE_HANDLER( oscnano_bankswitch_w )
{
	memory_set_bank(1, 1);

	cdp1864_tone_latch_w(device, 0, data);
}

/* Memory Maps */

// Telmac 1800

static ADDRESS_MAP_START( tmc1800_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x8000, 0x81ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc1800_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1861, CDP1861_TAG, cdp1861_dispon_r, cdp1861_dispoff_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(tmc1800_keylatch_w)
ADDRESS_MAP_END

// OSCOM 1000B

static ADDRESS_MAP_START( osc1000b_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x8000, 0x81ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( osc1000b_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x02, 0x02) AM_WRITE(tmc1800_keylatch_w)
ADDRESS_MAP_END

// Telmac 2000

static ADDRESS_MAP_START( tmc2000_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc2000_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1864, CDP1864_TAG, cdp1864_dispon_r, cdp1864_step_bgcolor_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(tmc2000_keylatch_w)
	AM_RANGE(0x04, 0x04) AM_DEVREADWRITE(CDP1864, CDP1864_TAG, cdp1864_dispoff_r, tmc2000_bankswitch_w)
ADDRESS_MAP_END

// OSCOM Nano

static ADDRESS_MAP_START( oscnano_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK(1)
	AM_RANGE(0x8000, 0x83ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( oscnano_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1864, CDP1864_TAG, cdp1864_dispon_r, cdp1864_step_bgcolor_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(tmc1800_keylatch_w)
	AM_RANGE(0x04, 0x04) AM_DEVREADWRITE(CDP1864, CDP1864_TAG, cdp1864_dispoff_r, oscnano_bankswitch_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( tmc1800 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7')

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')

	PORT_START_TAG("RUN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Run/Reset") PORT_CODE(KEYCODE_R) PORT_TOGGLE
INPUT_PORTS_END

static INPUT_PORTS_START( tmc2000 )
	PORT_INCLUDE(tmc1800)

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')

	PORT_START_TAG("IN4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("IN5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("IN6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START_TAG("IN7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
INPUT_PORTS_END

static INPUT_PORTS_START( oscnano )
	PORT_INCLUDE(tmc1800)

	PORT_MODIFY("RUN")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Monitor") PORT_CODE(KEYCODE_M) PORT_TOGGLE
INPUT_PORTS_END

/* Video */

// Telmac 1800

static int cdp1861_efx;

static CDP1861_ON_INT_CHANGED( tmc1800_int_w )
{
	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_INT, level);
}

static CDP1861_ON_DMAO_CHANGED( tmc1800_dmao_w )
{
	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_DMAOUT, level);
}

static CDP1861_ON_EFX_CHANGED( tmc1800_efx_w )
{
	cdp1861_efx = level;
}

static const cdp1861_interface tmc1800_cdp1861_intf =
{
	SCREEN_TAG,
	XTAL_1_75MHz,
	tmc1800_int_w,
	tmc1800_dmao_w,
	tmc1800_efx_w
};

static VIDEO_UPDATE( tmc1800 )
{
	const device_config *cdp1861 = device_list_find_by_tag(screen->machine->config->devicelist, CDP1861, CDP1861_TAG);

	cdp1861_update(cdp1861, bitmap, cliprect);

	return 0;
}

// Telmac 2000

static int cdp1864_efx;

static CDP1864_ON_INT_CHANGED( tmc2000_int_w )
{
	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_INT, level);
}

static CDP1864_ON_DMAO_CHANGED( tmc2000_dmao_w )
{
	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_DMAOUT, level);
}

static CDP1864_ON_EFX_CHANGED( tmc2000_efx_w )
{
	cdp1864_efx = level;
}

static CDP1864_COLOR_RAM_READ( tmc2000_colorram_r )
{
	return (~colorram[addr & 0x1ff]) & 0x07;
}

static const cdp1864_interface tmc2000_cdp1864_intf =
{
	SCREEN_TAG,
	CDP1864_CLK_FREQ,
	tmc2000_int_w,
	tmc2000_dmao_w,
	tmc2000_efx_w,
	tmc2000_colorram_r,
	RES_K(2.2),	// unverified
	RES_K(1),	// unverified
	RES_K(5.1),	// unverified
	RES_K(4.7)	// unverified
};

static VIDEO_UPDATE( tmc2000 )
{
	const device_config *cdp1864 = device_list_find_by_tag(screen->machine->config->devicelist, CDP1864, CDP1864_TAG);

	cdp1864_update(cdp1864, bitmap, cliprect);

	return 0;
}

/* CDP1802 Interfaces */

// Telmac 1800

static CDP1802_MODE_READ( tmc1800_mode_r )
{
	if (input_port_read(machine, "RUN") & 0x01)
	{
		return CDP1802_MODE_RUN;
	}
	else
	{
		return CDP1802_MODE_RESET;
	}
}

static CDP1802_EF_READ( tmc1800_ef_r )
{
	UINT8 flags = 0x0f;
	char port[4];

	/*
        EF1     ?
        EF2     tape in
        EF3     keyboard
        EF4     ?
    */

	// tape in

	if (cassette_input(cassette_device_image()) > +1.0) flags -= EF2;

	// keyboard

	sprintf(port, "IN%d", keylatch / 8);
	if (~input_port_read(machine, port) & (1 << (keylatch % 8))) flags -= EF3;

	return flags;
}

static CDP1802_Q_WRITE( tmc1800_q_w )
{
	cassette_output(cassette_device_image(), level ? -1.0 : +1.0);
}

static CDP1802_DMA_WRITE( tmc1800_dma_w )
{
	const device_config *cdp1861 = device_list_find_by_tag(machine->config->devicelist, CDP1861, CDP1861_TAG);

	cdp1861_dma_w(cdp1861, data);
}

static const cdp1802_interface tmc1800_config =
{
	tmc1800_mode_r,
	tmc1800_ef_r,
	NULL,
	tmc1800_q_w,
	NULL,
	tmc1800_dma_w
};

static const cdp1802_interface osc1000b_config =
{
	tmc1800_mode_r,
	tmc1800_ef_r,
	NULL,
	tmc1800_q_w,
	NULL,
	NULL
};

// Telmac 2000

static CDP1802_MODE_READ( tmc2000_mode_r )
{
	if (input_port_read(machine, "RUN") & 0x01)
	{
		return CDP1802_MODE_RUN;
	}
	else
	{
		// enable ROM at 0x0000

		memory_set_bank(1, 0);

		return CDP1802_MODE_RESET;
	}
}

static CDP1802_EF_READ( tmc2000_ef_r )
{
	int flags = 0x0f;
	char port[4];

	/*
        EF1     CDP1864
        EF2     tape in
        EF3     keyboard
        EF4     ?
    */

	// CDP1864

	if (!cdp1864_efx) flags -= EF1;

	// tape in

	if (cassette_input(cassette_device_image()) > +1.0) flags -= EF2;

	// keyboard

	sprintf(port, "IN%d", keylatch / 8);
	if (~input_port_read(machine, port) & (1 << (keylatch % 8))) flags -= EF3;

	return flags;
}

static CDP1802_Q_WRITE( tmc2000_q_w )
{
	const device_config *cdp1864 = device_list_find_by_tag(machine->config->devicelist, CDP1864, CDP1864_TAG);

	// turn CDP1864 sound generator on/off

	cdp1864_aoe_w(cdp1864, level);

	// set Q led status

	set_led_status(1, level);

	// tape out

	cassette_output(cassette_device_image(), level ? -1.0 : +1.0);
}

static CDP1802_DMA_WRITE( tmc2000_dma_w )
{
	const device_config *cdp1864 = device_list_find_by_tag(machine->config->devicelist, CDP1864, CDP1864_TAG);

	cdp1864_dma_w(cdp1864, data);
}

static const cdp1802_interface tmc2000_config =
{
	tmc2000_mode_r,
	tmc2000_ef_r,
	NULL,
	tmc2000_q_w,
	NULL,
	tmc2000_dma_w
};

// OSCOM Nano

static CDP1802_MODE_READ( oscnano_mode_r )
{
	if (input_port_read(machine, "RUN") & 0x01)
	{
		return CDP1802_MODE_RUN;
	}
	else
	{
		// enable ROM at 0x0000

		memory_set_bank(1, 0);

		return CDP1802_MODE_RESET;
	}
}

static CDP1802_EF_READ( oscnano_ef_r )
{
	int flags = 0x0f;
	char port[4];

	/*
        EF1     CDP1864
        EF2     tape in
        EF3     keyboard
        EF4     monitor
    */

	// CDP1864

	if (cdp1864_efx) flags -= EF1;

	// tape in

	if (cassette_input(cassette_device_image()) > +1.0) flags -= EF2;

	// keyboard

	sprintf(port, "IN%d", keylatch / 8);
	if (~input_port_read(machine, port) & (1 << (keylatch % 8))) flags -= EF3;

	// monitor

	if (input_port_read(machine, "RUN") & 0x02) flags -= EF4;

	return flags;
}

static CDP1802_Q_WRITE( oscnano_q_w )
{
	const device_config *cdp1864 = device_list_find_by_tag(machine->config->devicelist, CDP1864, CDP1864_TAG);

	// turn CDP1864 sound generator on/off

	cdp1864_aoe_w(cdp1864, level);

	// set Q led status

	set_led_status(1, level);

	// tape out

	cassette_output(cassette_device_image(), level ? -1.0 : +1.0);
}

static CDP1802_DMA_WRITE( oscnano_dma_w )
{
	const device_config *cdp1864 = device_list_find_by_tag(machine->config->devicelist, CDP1864, CDP1864_TAG);

	cdp1864_dma_w(cdp1864, data);
}

static const cdp1802_interface oscnano_config =
{
	oscnano_mode_r,
	oscnano_ef_r,
	NULL,
	oscnano_q_w,
	NULL,
	oscnano_dma_w
};

/* Machine Initialization */

// Telmac 1800

static MACHINE_START( tmc1800 )
{
	state_save_register_global(keylatch);
}

static MACHINE_RESET( tmc1800 )
{
	const device_config *cdp1861 = device_list_find_by_tag(machine->config->devicelist, CDP1861, CDP1861_TAG);
	cdp1861->reset(cdp1861);

	cpunum_set_input_line(machine, 0, INPUT_LINE_RESET, PULSE_LINE);
}

// OSCOM 1000B

static MACHINE_RESET( osc1000b )
{
	cpunum_set_input_line(machine, 0, INPUT_LINE_RESET, PULSE_LINE);
}

// Telmac 2000

static MACHINE_START( tmc2000 )
{
	state_save_register_global(keylatch);

	// RAM banking

	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1) + 0x8000, 0);
	memory_configure_bank(1, 1, 1, &mess_ram, 0);

	switch (mess_ram_size)
	{
	case 16*1024:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_BANK1, SMH_BANK1);
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case 32*1024:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x7fff, 0, 0, SMH_BANK1, SMH_BANK1);
		break;
	}

	// ROM/ColorRAM banking

	colorram = auto_malloc(0x200);

	memory_configure_bank(2, 0, 1, memory_region(REGION_CPU1) + 0x8000, 0);
	memory_configure_bank(2, 1, 1, &colorram, 0);

	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x81ff, 0, 0x0e00, SMH_BANK2, SMH_UNMAP);
}

static MACHINE_RESET( tmc2000 )
{
	const device_config *cdp1864 = device_list_find_by_tag(machine->config->devicelist, CDP1864, CDP1864_TAG);
	cdp1864->reset(cdp1864);

	cpunum_set_input_line(machine, 0, INPUT_LINE_RESET, PULSE_LINE);

	memory_set_bank(1, 0);
	memory_set_bank(2, 0);
}

// OSCOM Nano

static MACHINE_START( oscnano )
{
	state_save_register_global(keylatch);

	// RAM banking

	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1) + 0x8000, 0);
	memory_configure_bank(1, 1, 1, &mess_ram, 0);
}

static MACHINE_RESET( oscnano )
{
	const device_config *cdp1864 = device_list_find_by_tag(machine->config->devicelist, CDP1864, CDP1864_TAG);
	cdp1864->reset(cdp1864);

	cpunum_set_input_line(machine, 0, INPUT_LINE_RESET, PULSE_LINE);

	memory_set_bank(1, 0);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( tmc1800 )
	// basic system hardware

	MDRV_CPU_ADD(CDP1802, XTAL_1_75MHz)
	MDRV_CPU_PROGRAM_MAP(tmc1800_map, 0)
	MDRV_CPU_IO_MAP(tmc1800_io_map, 0)
	MDRV_CPU_CONFIG(tmc1800_config)

	MDRV_MACHINE_START(tmc1800)
	MDRV_MACHINE_RESET(tmc1800)

	// video hardware

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_1_75MHz, CDP1861_SCREEN_WIDTH, CDP1861_HBLANK_END, CDP1861_HBLANK_START, CDP1861_TOTAL_SCANLINES, CDP1861_SCANLINE_VBLANK_END, CDP1861_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(tmc1800)

	MDRV_DEVICE_ADD(CDP1861_TAG, CDP1861)
	MDRV_DEVICE_CONFIG(tmc1800_cdp1861_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( osc1000b )
	// basic system hardware

	MDRV_CPU_ADD(CDP1802, XTAL_1_75MHz)
	MDRV_CPU_PROGRAM_MAP(osc1000b_map, 0)
	MDRV_CPU_IO_MAP(osc1000b_io_map, 0)
	MDRV_CPU_CONFIG(osc1000b_config)

	MDRV_MACHINE_START(tmc1800)
	MDRV_MACHINE_RESET(osc1000b)

	// video hardware

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 0, 199)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(osm200)
	MDRV_VIDEO_UPDATE(osm200)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tmc2000 )
	// basic system hardware

	MDRV_CPU_ADD(CDP1802, XTAL_1_75MHz)
	MDRV_CPU_PROGRAM_MAP(tmc2000_map, 0)
	MDRV_CPU_IO_MAP(tmc2000_io_map, 0)
	MDRV_CPU_CONFIG(tmc2000_config)

	MDRV_MACHINE_START(tmc2000)
	MDRV_MACHINE_RESET(tmc2000)

	// video hardware

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(CDP1864_CLK_FREQ, CDP1864_SCREEN_WIDTH, CDP1864_HBLANK_END, CDP1864_HBLANK_START, CDP1864_TOTAL_SCANLINES, CDP1864_SCANLINE_VBLANK_END, CDP1864_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(8)
	MDRV_VIDEO_UPDATE(tmc2000)

	MDRV_DEVICE_ADD(CDP1864_TAG, CDP1864)
	MDRV_DEVICE_CONFIG(tmc2000_cdp1864_intf)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( oscnano )
	// basic system hardware

	MDRV_CPU_ADD(CDP1802, XTAL_1_75MHz)
	MDRV_CPU_PROGRAM_MAP(oscnano_map, 0)
	MDRV_CPU_IO_MAP(oscnano_io_map, 0)
	MDRV_CPU_CONFIG(oscnano_config)

	MDRV_MACHINE_START(oscnano)
	MDRV_MACHINE_RESET(oscnano)

	// video hardware

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(CDP1864_CLK_FREQ, CDP1864_SCREEN_WIDTH, CDP1864_HBLANK_END, CDP1864_HBLANK_START, CDP1864_TOTAL_SCANLINES, CDP1864_SCANLINE_VBLANK_END, CDP1864_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(8)
	MDRV_VIDEO_UPDATE(tmc2000)

	MDRV_DEVICE_ADD(CDP1864_TAG, CDP1864)
	MDRV_DEVICE_CONFIG(tmc2000_cdp1864_intf)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( tmc1800 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "mmi6341-1.ic2", 0x8000, 0x0200, NO_DUMP ) // equivalent to 82S141
ROM_END

ROM_START( osc1000b )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "mmi6341-1.ic2", 0x8000, 0x0200, NO_DUMP ) // equivalent to 82S141

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mmi6349.5d",	0x0000, 0x0200, NO_DUMP ) // equivalent to 82S147
	ROM_LOAD( "mmi6349.5c",	0x0200, 0x0200, NO_DUMP ) // equivalent to 82S147
ROM_END

ROM_START( tmc2000 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "default",  "PROM N:o 200" )
	ROMX_LOAD( "200.m5",    0x8000, 0x0200, BAD_DUMP CRC(53BDDF1A) SHA1(3691741A6921A2E2333B697E80D4936BE41DCDA8), ROM_BIOS(1) ) // typed in from the manual
	ROM_SYSTEM_BIOS( 1, "prom202",  "PROM N:o 202" )
	ROMX_LOAD( "202.m5",    0x8000, 0x0200, NO_DUMP, ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "tool2000", "TOOL-2000" )
	ROMX_LOAD( "tool2000",	0x8000, 0x0800, NO_DUMP, ROM_BIOS(3) )
ROM_END

ROM_START( oscnano )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "mmi6349.ic", 0x8000, 0x0200, NO_DUMP ) // equivalent to 82S141
ROM_END

/* System Configuration */

static void tmc1800_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 1; break;
		case MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE:	info->i = CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( tmc1800 )
	CONFIG_RAM_DEFAULT	( 2 * 1024)
	CONFIG_RAM			( 4 * 1024)
	CONFIG_DEVICE(tmc1800_cassette_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( osc1000b )
	CONFIG_RAM_DEFAULT	( 2 * 1024)
	CONFIG_RAM			( 4 * 1024)
	CONFIG_DEVICE(tmc1800_cassette_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( tmc2000 )
	CONFIG_RAM_DEFAULT	(16 * 1024)
	CONFIG_RAM			(32 * 1024)
	CONFIG_DEVICE(tmc1800_cassette_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( oscnano )
	CONFIG_RAM_DEFAULT	(4 * 1024)
	CONFIG_DEVICE(tmc1800_cassette_getinfo)
SYSTEM_CONFIG_END

/* Driver Initialization */

static TIMER_CALLBACK(setup_beep)
{
	beep_set_state(0, 0);
	beep_set_frequency( 0, 0 );
}

static DRIVER_INIT( tmc1800 )
{
	timer_set(attotime_zero, NULL, 0, setup_beep);
}

/* System Drivers */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        CONFIG      COMPANY         FULLNAME        FLAGS */
COMP( 1977, tmc1800,    0,      0,      tmc1800,    tmc1800,    tmc1800,    tmc1800,    "Telercas Oy",  "Telmac 1800",  GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1977, osc1000b,   tmc1800,0,      osc1000b,   tmc1800,    tmc1800,    tmc1800,    "OSCOM Oy",		"OSCOM 1000B",  GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1980, oscnano,	0,		0,		oscnano,	oscnano,	tmc1800,	oscnano,	"OSCOM Oy",		"OSCOM Nano",  GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1980, tmc2000,    0,      0,      tmc2000,    tmc2000,    tmc1800,    tmc2000,    "Telercas Oy",  "Telmac 2000",  GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
