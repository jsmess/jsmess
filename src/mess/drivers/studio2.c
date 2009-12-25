/*

RCA Studio II ******* PRESS Q for a car racing game *********** X to accelerate, left & right arrow to steer.

PCB Layout
----------

1809746-S1-E

|----------------------------------------------------------------------------------------------------------|
|                                  |----------------------------|                                          |
|                                  |----------------------------|                          CA555   7805    |
|      SPKR                                     CN1                                                        |
|                                                                                                          |
|                                                                                           |-----------|  |
|                                                                                           | Output    |  |
|   ROM.4  ROM.3  ROM.2  ROM.1  CDP1822  CDP1822  CDP1822  CDP1822                          |TV Module  |  |
|                                                                                           |           |  |
|                                                                 CDP1802  TA10171V1        |           |  |
|                                                                                           |           |  |
|                                                                                           |           |  |
|                                                                                           |           |  |
|                                                                                           |-----------|  |
|                                                                                                          |
|                       CD4042  CD4001  CD4515                                                             |
|                                                                                                          |
|      CN2                                                                                       CN3       |
|----------------------------------------------------------------------------------------------------------|

Notes:
      All IC's shown.

      CDP1802 - RCA CDP1802CE Microprocessor
      TA10171V1 - RCA TA10171V1 NTSC Video Display Controller (VDC) (= RCA CDP1861)
      CDP1822 - RCA CDP1822NCE 256 x4 RAM (= Mitsubishi M58721P)
      ROM.x   - RCA CDP1831CE 512 x8 MASKROM. All ROMs are marked 'PROGRAM COPYRIGHT (C) RCA CORP. 1977'
      CD4001  - 4001 Quad 2-Input NOR Buffered B Series Gate (4000-series CMOS TTL logic IC)
      CD4042  - 4042 Quad Clocked D Latch (4000-series CMOS TTL logic IC)
      CD4515  - 4515 4-Bit Latched/4-to-16 Line Decoders (4000-series CMOS TTL logic IC)
      CA555   - CA555CG General Purpose Single Bipolar Timer (= NE555)
      7805    - Voltage regulator, input 10V-35V, output +5V
      SPKR    - Loudspeaker, 8 ohms, 0.3 W
      CN1     - ROM cartridge connector, 2x22 pins, 0.154" spacing
      CN2     - Player A keypad connector, 1x12 pins
      CN3     - Player B keypad connector, 1x12 pins
*/

/*

Toshiba Visicom Console (RCA Studio II clone)
Toshiba, 1978

PCB Layout                                            Many resistors/caps
----------                                7.5VDC    / transistors in this area
                                           \/   /---|-----/
|------------------------------------------||---|-------|   |-----|C|------|
|D235  POT       TC4011     CART_SLOT           |       |   | TV Modulator |
|HEATSINK                                       |       |   |              |
|           TC4515          TC4049 TC4011 74LS08|       |   | SW (VHF Ch1) |
|DIODES(x20)                       74LS74 74LS73|       |   |    (VHF Ch2) |
|-----------|                                 3.579545MHz   |              |
            |        CDP1802                    |  POT  |   |--------------|
            |TMM331                             |       |
            |               CDP1861   TC5012 TC5012     |
            |                     TC4021 TC4021 |       |
            |                          TC5012   \-----/ |
            |          2111      2111   2111   74LS74   |
            |TC4042    2111      2111   2111   TC4011   |
            |-------------------------------------------|
Notes: (all chips shown above)
      CDP1802 - RCA CDP1802 CPU (DIP40), clock 1.7897725MHz [3.579545/2]
      CDP1861 - RCA CDP1861 Video Controller (DIP24)
                VSync - 60.4533Hz   \ (measured on pin 6)
                HSync - 15.8387kHz  /
                Clock - 223.721562kHz [3.579545/16] (measured on pin 1)
      2111    - NEC D2111AL-4 256 bytes x4 SRAM (DIP18, x6). Total 1.5k
      C       - Composite Video Output to TV from TV Modulator
      TMM331  - Toshiba TMM331AP 2k x8 MASKROM (DIP24)
                Pinout (preliminary):
                           TMM331
                        |----\/----|
                     A7 |1       24| VCC
                     A8 |2       23| D0
                     A9 |3       22| D1
                    A10 |4       21| D2
                     A0 |5       20| D3
                     A1 |6       19| D4
                     A2 |7       18| D5
                     A3 |8       17| D6
                     A4 |9       16| D7
                     A5 |10      15| CE (LOW)
                     A6 |11      14| ? (unknown, leave NC?)
                    GND |12      13| OE (LOW)
                        |----------|

*/

/*

Mustang 9016 Telespiel Computer

PCB Layout
----------

|----------------------------------------------------------------------------------------------------------|
|7805                              |----------------------------|                          CD4069  MC14001 |
|                                  |----------------------------|                                          |
|                                               CN1                                                        |
|                                                                                                          |
|       ROM.IC13  ROM.IC14      CDP1822  CDP1822 CDP1822 CDP1822                            |-----------|  |
|                                                                                           | Output    |  |
|                                                                                           |TV Module? |  |
| ROM.IC12                                                        CDP1802  CDP1864          |           |  |
|                 CDP1822                                                                   |           |  |
|                          CD4019 CDP1858 CD4081 CD4069                                     |           |  |
|                                                                                           |           |  |
|                                                        CD4515                             |           |  |
|                                                                          1.750MHz         |-----------|  |
|                                                                                              4.3236MHz   |
|                                                                                                          |
|                                                                                                          |
|                                                                                                          |
|----------------------------------------------------------------------------------------------------------|

Notes:
      All IC's shown.

      CDP1802 - RCA CDP1802CE Microprocessor
      CDP1864 - RCA CDP1864CE PAL Video Display Controller (VDC)
      CDP1822 - RCA CDP1822NCE 256 x4 RAM (= Mitsubishi M58721P)
      ROM.ICx - RCA CDP1833 1k x8 MASKROM. All ROMs are marked 'PROGRAM COPYRIGHT (C) RCA CORP. 1978'
      CD4019  - 4019 Quad AND-OR Select Gate (4000-series CMOS TTL logic IC)
      CDP1858 - RCA CDP1858E Latch/Decoder - 4-bit
      CD4081  - 4081 Quad 2-Input AND Buffered B Series Gate (4000-series CMOS TTL logic IC)
      CD4069  - 4069 Hex Buffer, Inverter (4000-series CMOS TTL logic IC)
      CD4515  - 4515 4-Bit Latched/4-to-16 Line Decoders (4000-series CMOS TTL logic IC)
      7805    - Voltage regulator, input 10V-35V, output +5V
      CN1     - ROM cartridge connector, 2x22 pins, 0.154" spacing
*/

/*

    RCA Studio II games list

    Title                           Series                  Dumped
    ----------------------------------------------------------------------------
    Bowling                         built-in                yes
    Doodles                         built-in                yes
    Freeway                         built-in                yes
    Math                            built-in                yes
    Patterns                        built-in                yes
    Gunfighter/Moonship Battle      TV Arcade               no, but Guru has one
    Space War                       TV Arcade I             yes
    Fun with Numbers                TV Arcade II            no, but Guru has one
    Tennis/Squash                   TV Arcade III           yes
    Baseball                        TV Arcade IV            yes
    Speedway/Tag                    TV Arcade               yes
    Blackjack                       TV Casino I             yes
    Bingo                           TV Casino               no
    Math and Social Studies         TV School House I       no, but Guru has one
    Math Fun                        TV School House II      no, but Guru has one
    Biorhythm                       TV Mystic               no, but Guru has one


    MPT-02 games list

    ID      Title                   Series                  Dumped
    ----------------------------------------------------------------------------
    MG-201  Bingo                                           no
    MG-202  Concentration Match                             no, but Guru has one
    MG-203  Star Wars                                       no, but Guru has one
    MG-204  Math Fun                School House II         no, but Guru has one
    MG-205  Pinball                                         no, but Guru has one
    MG-206  Biorythm                                        no
    MG-207  Tennis/Squash                                   no
    MG-208  Fun with Numbers                                no
    MG-209  Computer Quiz           School House I          no
    MG-210  Baseball                                        no
    MG-211  Speedway/Tag                                    no
    MG-212  Spacewar Intercept                              no
    MG-213  Gun Fight/Moon Ship                             no

*/

/*

    TODO:

    - studio2 cpu clock from schematics
    - visicom alternate videoram @ 0x1300 ?
    - mpt02/mustang cdp1864 colors
    - discrete sound
    - Academy Apollo 80 (Germany)

*/

#include "driver.h"
#include "includes/studio2.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1861.h"
#include "sound/cdp1864.h"
#include "devices/cartslot.h"
#include "sound/beep.h"
#include "sound/discrete.h"

/* Read/Write Handlers */

static WRITE8_HANDLER( keylatch_w )
{
	studio2_state *state = space->machine->driver_data;

	state->keylatch = data & 0x0f;
}

static READ8_DEVICE_HANDLER( studio2_cdp1861_dispon_r )
{
	cdp1861_dispon_w(device, 1);
	cdp1861_dispon_w(device, 0);

	return 0xff;
}

static WRITE8_DEVICE_HANDLER( visicom_cdp1861_dispon_w )
{
	cdp1861_dispon_w(device, 1);
	cdp1861_dispon_w(device, 0);
}

/* Memory Maps */

static ADDRESS_MAP_START( studio2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x09ff) AM_MIRROR(0xf400) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( studio2_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVREAD(CDP1861_TAG, studio2_cdp1861_dispon_r)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( visicom_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x1000, 0x10ff) AM_MIRROR(0x200) AM_RAM
	AM_RANGE(0x1100, 0x11ff) AM_RAM
//  AM_RANGE(0x1200, 0x12ff) AM_RAM
	AM_RANGE(0x1300, 0x13ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( visicom_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVWRITE(CDP1861_TAG, visicom_cdp1861_dispon_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mpt02_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x09ff) AM_RAM
	AM_RANGE(0x0b00, 0x0b3f) AM_RAM AM_BASE_MEMBER(studio2_state, color_ram)
	AM_RANGE(0x0c00, 0x0dff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mpt02_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1864_TAG, cdp1864_dispon_r, cdp1864_step_bgcolor_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
	AM_RANGE(0x04, 0x04) AM_DEVREADWRITE(CDP1864_TAG, cdp1864_dispoff_r, cdp1864_tone_latch_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( studio2 )
	PORT_START("KEYPAD_L")
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 0") PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 1") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 2") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 3") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 4") PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 5") PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 6") PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 7") PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 8") PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 1/Left 9") PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_D)

	PORT_START("KEYPAD_R")
	PORT_BIT( 0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 1") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 2") PORT_CODE(KEYCODE_8_PAD) PORT_CODE(KEYCODE_UP)
	PORT_BIT( 0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 3") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 4") PORT_CODE(KEYCODE_4_PAD) PORT_CODE(KEYCODE_LEFT)
	PORT_BIT( 0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 6") PORT_CODE(KEYCODE_6_PAD) PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT( 0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 7") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 8") PORT_CODE(KEYCODE_2_PAD) PORT_CODE(KEYCODE_DOWN)
	PORT_BIT( 0x200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Player 2/Right 9") PORT_CODE(KEYCODE_3_PAD)
INPUT_PORTS_END

/* Video */

static WRITE_LINE_DEVICE_HANDLER( studio2_efx_w )
{
	studio2_state *driver_state = device->machine->driver_data;

	driver_state->cdp1861_efx = state;
}

static CDP1861_INTERFACE( studio2_cdp1861_intf )
{
	CDP1802_TAG,
	SCREEN_TAG,
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_INT),
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_DMAOUT),
	DEVCB_LINE(studio2_efx_w)
};

static VIDEO_UPDATE( studio2 )
{
	studio2_state *state = screen->machine->driver_data;

	cdp1861_update(state->cdp1861, bitmap, cliprect);

	return 0;
}

static WRITE_LINE_DEVICE_HANDLER( mpt02_efx_w )
{
	studio2_state *driver_state = device->machine->driver_data;

	driver_state->cdp1864_efx = state;
}

static CDP1864_INTERFACE( mpt02_cdp1864_intf )
{
	CDP1802_TAG,
	SCREEN_TAG,
	CDP1864_INTERLACED,
	DEVCB_LINE_VCC, // ?
	DEVCB_LINE_VCC, // ?
	DEVCB_LINE_VCC, // ?
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_INT),
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_DMAOUT),
	DEVCB_LINE(mpt02_efx_w),
	RES_K(2.2),	// unverified
	RES_K(1),	// unverified
	RES_K(5.1),	// unverified
	RES_K(4.7)	// unverified
};

static VIDEO_UPDATE( mpt02 )
{
	studio2_state *state = screen->machine->driver_data;

	cdp1864_update(state->cdp1864, bitmap, cliprect);

	return 0;
}

/* CDP1802 Configuration */

static CDP1802_MODE_READ( studio2_mode_r )
{
	studio2_state *state = device->machine->driver_data;

	return state->cdp1802_mode;
}

static CDP1802_EF_READ( studio2_ef_r )
{
	studio2_state *state = device->machine->driver_data;

	int ef = 0x0f;

	if (state->cdp1861_efx) ef -= EF1;

	if (input_port_read(device->machine, "KEYPAD_L") & (1 << state->keylatch)) ef -= EF3;
	if (input_port_read(device->machine, "KEYPAD_R") & (1 << state->keylatch)) ef -= EF4;

	return ef;
}

static WRITE_LINE_DEVICE_HANDLER( studio2_q_w )
{
	const device_config *speaker = devtag_get_device(device->machine, "beep");
	beep_set_state(speaker, state);
}

static CDP1802_INTERFACE( studio2_config )
{
	studio2_mode_r,
	studio2_ef_r,
	NULL,
	DEVCB_LINE(studio2_q_w),
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER(CDP1861_TAG, cdp1861_dma_w)
};

static CDP1802_EF_READ( mpt02_ef_r )
{
	studio2_state *state = device->machine->driver_data;

	int ef = 0x0f;

	if (state->cdp1864_efx) ef -= EF1;

	if (input_port_read(device->machine, "KEYPAD_L") & (1 << state->keylatch)) ef -= EF3;
	if (input_port_read(device->machine, "KEYPAD_R") & (1 << state->keylatch)) ef -= EF4;

	return ef;
}

static WRITE8_DEVICE_HANDLER( mpt02_dma_w )
{
	studio2_state *state = device->machine->driver_data;

	state->color = state->color_ram[offset / 4];

	cdp1864_con_w(state->cdp1864, 0); // HACK
	cdp1864_dma_w(state->cdp1864, offset, data);
}

static CDP1802_INTERFACE( mpt02_config )
{
	studio2_mode_r,
	mpt02_ef_r,
	NULL,
	DEVCB_LINE(studio2_q_w),
	DEVCB_NULL,
	DEVCB_HANDLER(mpt02_dma_w)
};

/* Machine Initialization */

static TIMER_CALLBACK( set_cpu_mode )
{
	studio2_state *state = machine->driver_data;

	state->cdp1802_mode = CDP1802_MODE_RUN;
}

static MACHINE_START( studio2 )
{
	studio2_state *state = machine->driver_data;

	/* find devices */
	state->cdp1861 = devtag_get_device(machine, CDP1861_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->cdp1802_mode);
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->cdp1861_efx);
}

static MACHINE_RESET( studio2 )
{
	studio2_state *state = machine->driver_data;

	/* reset CPU */
	state->cdp1802_mode = CDP1802_MODE_RESET;
	timer_set(machine, ATTOTIME_IN_MSEC(200), NULL, 0, set_cpu_mode);

	/* reset CDP1861 */
	device_reset(state->cdp1861);
}

static MACHINE_START( mpt02 )
{
	studio2_state *state = machine->driver_data;

	/* find devices */
	state->cdp1864 = devtag_get_device(machine, CDP1864_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->cdp1802_mode);
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->cdp1864_efx);
}

static MACHINE_RESET( mpt02 )
{
	studio2_state *state = machine->driver_data;

	/* reset CPU */
	state->cdp1802_mode = CDP1802_MODE_RESET;
	timer_set(machine, ATTOTIME_IN_MSEC(200), NULL, 0, set_cpu_mode);

	/* reset CDP1864 */
	device_reset(state->cdp1864);
}

static DEVICE_IMAGE_LOAD( studio2_cart )
{
	st2_header header;
	int block;

	/* check file size */
	int filesize = image_length(image);

	if (filesize <= ST2_BLOCK_SIZE) {
		logerror( "Error loading cartridge: Invalid ROM file: %s.\n", image_filename(image));
		return INIT_FAIL;
	}

	/* read ST2 header */
	if (image_fread(image, &header, ST2_BLOCK_SIZE) != ST2_BLOCK_SIZE) {
		logerror( "Error loading cartridge: Unable to read header from file: %s.\n", image_filename(image));
		return INIT_FAIL;
	}

	//logerror("ST2 Catalogue: %s\n", header.catalogue);
	//logerror("ST2 Title: %s\n", header.title);

	/* read ST2 cartridge into memory */
	for (block = 0; block < (header.blocks - 1); block++)
	{
		UINT16 offset = header.page[block] << 8;
		UINT8 *ptr = ((UINT8 *) memory_region(image->machine, CDP1802_TAG)) + offset;

		//logerror("ST2 Reading block %u to %04x\n", block, offset);

		if (image_fread(image, ptr, ST2_BLOCK_SIZE) != ST2_BLOCK_SIZE) {
			logerror( "Error loading cartridge: Unable to read contents from file: %s.\n", image_filename(image));
			return INIT_FAIL;
		}
	}

	return INIT_PASS;
}

static MACHINE_DRIVER_START( studio2_cartslot )
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("st2")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(studio2_cart)
MACHINE_DRIVER_END

/* Machine Drivers */

static MACHINE_DRIVER_START( studio2 )
	MDRV_DRIVER_DATA(studio2_state)

	// basic machine hardware
	MDRV_CPU_ADD(CDP1802_TAG, CDP1802, 3579545/2) // the real clock is derived from an oscillator circuit
	MDRV_CPU_PROGRAM_MAP(studio2_map)
	MDRV_CPU_IO_MAP(studio2_io_map)
	MDRV_CPU_CONFIG(studio2_config)

	MDRV_MACHINE_START(studio2)
	MDRV_MACHINE_RESET(studio2)

    // video hardware
	MDRV_CDP1861_SCREEN_ADD(SCREEN_TAG, 3579545/2)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(studio2)

	MDRV_CDP1861_ADD(CDP1861_TAG, 3579545/2, studio2_cdp1861_intf)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_IMPORT_FROM( studio2_cartslot )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( visicom )
	MDRV_DRIVER_DATA(studio2_state)

	// basic machine hardware
	MDRV_CPU_ADD(CDP1802_TAG, CDP1802, XTAL_3_579545MHz/2)
	MDRV_CPU_PROGRAM_MAP(visicom_map)
	MDRV_CPU_IO_MAP(visicom_io_map)
	MDRV_CPU_CONFIG(studio2_config)

	MDRV_MACHINE_START(studio2)
	MDRV_MACHINE_RESET(studio2)

    // video hardware
	MDRV_CDP1864_SCREEN_ADD(SCREEN_TAG, XTAL_3_579545MHz/2)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(studio2)

	MDRV_CDP1861_ADD(CDP1861_TAG, XTAL_3_579545MHz/2/8, studio2_cdp1861_intf)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_IMPORT_FROM( studio2_cartslot )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mpt02 )
	MDRV_DRIVER_DATA(studio2_state)

	// basic machine hardware
	MDRV_CPU_ADD(CDP1802_TAG, CDP1802, CDP1864_CLOCK)
	MDRV_CPU_PROGRAM_MAP(mpt02_map)
	MDRV_CPU_IO_MAP(mpt02_io_map)
	MDRV_CPU_CONFIG(mpt02_config)

	MDRV_MACHINE_START(mpt02)
	MDRV_MACHINE_RESET(mpt02)

    // video hardware
	MDRV_CDP1864_SCREEN_ADD(SCREEN_TAG, CDP1864_CLOCK)

	MDRV_PALETTE_LENGTH(8+8)
	MDRV_VIDEO_UPDATE(mpt02)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_CDP1864_ADD(CDP1864_TAG, CDP1864_CLOCK, mpt02_cdp1864_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_IMPORT_FROM( studio2_cartslot )
MACHINE_DRIVER_END

/* ROMs */

ROM_START( studio2 )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "84932", 0x0000, 0x0200, NO_DUMP )
	ROM_LOAD( "84933", 0x0200, 0x0200, NO_DUMP )
	ROM_LOAD( "85456", 0x0400, 0x0200, NO_DUMP )
	ROM_LOAD( "85457", 0x0600, 0x0200, NO_DUMP )
	ROM_LOAD( "studio2.rom", 0x0000, 0x0800, BAD_DUMP CRC(a494b339) SHA1(f2650dacc9daab06b9fdf0e7748e977b2907010c) )
ROM_END

ROM_START( visicom )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "visicom.q003", 0x0000, 0x0800, CRC(23d22074) SHA1(a0a8be23f70621a2bd8010b1134e8a0019075bf1) )
ROM_END

ROM_START( mtc9016 )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "86676.ic13",  0x0000, 0x0400, NO_DUMP )
	ROM_LOAD( "86677b.ic14", 0x0400, 0x0400, NO_DUMP )
	ROM_LOAD( "87201.ic12",  0x0a00, 0x0400, NO_DUMP )
ROM_END

ROM_START( shmc1200 )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "shmc1200.bin",  0x0000, 0x0800, NO_DUMP )
ROM_END

ROM_START( mpt02s )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "86676.ic13",  0x0000, 0x0400, CRC(a7d0dd3b) SHA1(e1881ab4d67a5d735dd2c8d7e924e41df6f2aeec) )
	ROM_LOAD( "86677b.ic14", 0x0400, 0x0400, CRC(82a2d29e) SHA1(37e02089d611db10bad070d89c8801de41521189) )
	ROM_LOAD( "87201.ic12",  0x0c00, 0x0400, CRC(8006a1e3) SHA1(b67612d98231485fce55d604915abd19b6d64eac) )
ROM_END

ROM_START( mpt02h )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
ROM_END

ROM_START( eti660 )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "eti660.bin", 0x0000, 0x0400, CRC(811dfa62) SHA1(c0c4951e02f873f15560bdc3f35cdf3f99653922) )
ROM_END

/* Driver Initialization */

static TIMER_CALLBACK( setup_beep )
{
	const device_config *speaker = devtag_get_device(machine, "beep");
	beep_set_state(speaker, 0);
	beep_set_frequency(speaker, 300);
}

static DRIVER_INIT( studio2 )
{
	timer_set(machine, attotime_zero, NULL, 0, setup_beep);
}

/* Game Drivers */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        COMPANY   FULLNAME */
CONS( 1977,	studio2,	0,		0,		studio2,	studio2,	studio2,	"RCA",		"Studio II", GAME_SUPPORTS_SAVE )
CONS( 1978, visicom,	studio2,0,		visicom,	studio2,	studio2,	"Toshiba",	"Visicom (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS | GAME_SUPPORTS_SAVE )
CONS( 1978,	mpt02s,		studio2,0,		mpt02,		studio2,	studio2,	"Soundic",	"MPT-02 Victory Home TV Programmer (Austria)", GAME_NOT_WORKING )
CONS( 1978,	mpt02h,		studio2,0,		mpt02,		studio2,	studio2,	"Hanimex",	"MPT-02 Jeu TV Programmable (France)", GAME_NOT_WORKING )
CONS( 1978,	mtc9016,	studio2,0,		mpt02,		studio2,	studio2,	"Mustang",	"9016 Telespiel Computer (Germany)", GAME_NOT_WORKING )
CONS( 1978, shmc1200,	studio2,0,		mpt02,		studio2,	studio2,	"Sheen",	"1200 Micro Computer (Australia)", GAME_NOT_WORKING )
CONS( 1978, eti660,		studio2,0,		mpt02,		studio2,	studio2,	"Electronics Today International",	"ETI-660 (Australia)", GAME_NOT_WORKING )
