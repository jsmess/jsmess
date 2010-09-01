/*

RCA COSMAC VIP

PCB Layout
----------

|-------------------------------------------------------------------------------|
|   |---------------CN1---------------|     |---------------CN2---------------| |
|CN6                                                                            |
|   |---------|                 |---------|             4050        4050        |
|   | CDPR566 |                 | CDP1861 |                                 CN3 |
|   |---------|                 |---------|                                     |
|                                                                           CN4 |
| 7805                                      |---------|      |--------|         |
|       2114                    3.521280MHz |  4508   |      |  4508  |     CN5 |
|                                           |---------|      |--------|         |
|       2114                                                 |--------|         |
|                               7474  7400  4049  4051  4028 |  4515  | CA3401  |
|       2114                                                 |--------|         |
|               |-------------|                                                 |
|       2114    |   CDP1802   |                                                 |
|               |-------------|             LED1                                |
|       2114                                                                    |
|                                           LED2                                |
|       2114    4556    4042                                                    |
|                                           LED3                                |
|  555  2114    4011    4013                                                    |
|                                           SW1                                 |
|       2114                                                                    |
|                                                                               |
|-------------------------------------------------------------------------------|

Notes:
    All IC's shown.

    CDPR566 - Programmed CDP1832 512 x 8-Bit Static ROM
    2114    - 2114 4096 Bit (1024x4) NMOS Static RAM
    CDP1802 - RCA CDP1802 CMOS 8-Bit Microprocessor
    CDP1861 - RCA CDP1861 Video Display Controller
    CA3401  - Quad Single-Supply Op-Amp
    CN1     - expansion interface connector
    CN2     - parallel I/O interface connector
    CN3     - video connector
    CN4     - tape in connector
    CN5     - tape out connector
    CN6     - power connector
    LED1    - power led
    LED2    - Q led
    LED3    - tape led
    SW1     - Run/Reset switch

*/

/*

    TODO:

    - second VP580 keypad

    - VP-585 Expansion Keyboard Interface (2 keypad connectors for VP-580)
    - VP-551 Super Sound Board (4 channel sound)
    - VP-601/611 ASCII Keyboard (VP-601 58 keys, VP611 58 keys + 16 keys numerical keypad)
    - VP-700 Expanded Tiny Basic Board (4 KB ROM expansion)

*/

/*

    VP-711 COSMAC MicroComputer $199
    (CDP18S711) Features RCA COSMAC microprocessor. 2K RAM, expandable to
    32K (4K on-board). Built-in cassette interface and video interface.
    16 key hexidecimal keypad. ROM operating system. CHIP-8 language and
    machine language. Tone generator and speaker. 8-bit input port, 8-bit
    output port, and full system expansion connector. Power supply and 3
    manuals (VP-311, VP-320, MPM201 B) included. Completely assembled.

    VP-44 VP-711 RAM On-Board Expansion Kit $36
    Four type 2114 RAM IC's for expanding VP-711 on-board memory
    to 4K bytes.

    VP-111 MicroComputer $99
    RCA COSMAC microprocessor. 1 K RAM expandable to 32K (4K On-
    board). Built-in cassette interface and video interface. 16 key
    Hexidecimal keypad. ROM operating system. CHIP-8 language and Machine
    language. Tone generator. Assembled - user must install Cables
    (supplied) and furnish 5 volt power supply and speaker.

    VP-114 VP-111 Expansion Kit $76
    Includes I/O ports, system expansion connector and additional
    3K of RAM. Expands VP-111 to VP-711 capability.

    VP-155 VP-111 Cover $12
    Attractive protective plastic cover for VP-111.

    VP-3301 Interactive Data Terminal Available Approx. 6 Months
    Microprocessor Based Computer Terminal with keyboard, video
    Interface and color graphics - includes full resident and user
    Definable character font, switch selectable configuration, cursor
    Control, reverse video and many other features.

    VP-590 Color Board $69
    Displays VP-711 output in color! Program control of four
    Background colors and eight foreground colors. CHIP-8X language
    Adds color commands. Includes two sockets for VP-580 Expansion
    Keyboards.

    VP-595 Simple Sound Board $30
    Provides 256 different frequencies in place of VP-711 single-
    tone Output. Use with VP-590 Color Board for simultaneous color and
    Sound. Great for simple music or sound effects! Includes speaker.

    VP-550 Super Sound Board $49
    Turn your VP-711 into a music synthesizer! Two independent
    sound Channels. Frequency, duration and amplitude envelope (voice) of
    Each channel under program control. On-board tempo control. Provision
    for multi-track recording or slaving VP-711's. Output drives audio
    preamp. Does not permit simultaneous video display.

    VP-551 Super Sound 4-Channel Expander Package $74
    VP-551 provides four (4) independent sound channels with
    frequency duration and amplitude envelope for each channel. Package
    includes modified VP-550 super sound board, VP-576 two board
    expander, data cassette with 4-channel PIN-8 program, and instruction
    manual. Requires 4K RAM system and your VP-550 Super Sound Board.

    VP-570 Memory Expansion Board $95
    Plug-in 4K static RAM memory. Jumper locates RAM in any 4K
    block in first 32K of VP-711 memory space.

    VP-580 Auxiliary Keyboard $20
    Adds two-player interactive game capability to VP-711 16-key
    keypad with cable. Connects to sockets on VP-590 Color Board or VP-
    585 Keyboard Interface.

    VP-585 Keyboard Interface Board $15
    Interfaces two VP-580 Expansion Keyboards directly to the VP-
    711. Not required when VP-590 Color Board is used.

    VP-560 EPROM Board $34
    Interfaces two Intel 2716 EPROMs to VP-711. Places EPROMs any-
    where in memory space. Can also re-allocate on-board RAM in memory
    space.

    VP-565 EPROM Programmer Board $99
    Programs Intel 2716 EPROMs with VP-711. Complete with software
    to program, copy, and verify. On-board generation of all programming
    voltages.

    VP-575 Expansion Board $59
    Plug-in board with 4 buffered and one unbuffered socket.
    Permits use of up to 5 Accessory Boards in VP-711 Expansion Socket.

    VP-576 Two-Board Expander $20
    Plug-in board for VP-711 I/O or Expansion Socket permits use
    of two Accessory Boards in either location.

    VP-601* ASCII Keyboard. 7-Bit Parallel Output $69
    Fully encoded, 128-character ASCII alphanumeric keyboard. 58
    light touch keys (2 user defined). Selectable "Upper-Case-Only".

    VP-606* ASCII Keyboard - Serial Output $99
    Same as VP-601. EIA RS232C compatible, 20 mA current loop and
    TTL outputs. Six selectable baud rates. Available mid-1980.

    VP-611* ASCII/Numeric Keyboard. 7-Bit Parallel Output $89
    ASCII Keyboard identical to VP-601 plus 16 key numeric entry
    keyboard for easier entry of numbers.

    VP-616* ASCII/Numeric Keyboard - Serial Output $119
    Same as VP-611. EIA RS232C compatible, 20 mA current loop and
    TTL outputs. Six selectable baud rates. Available mid-1980.

    VP-620 Cable: ASCII Keyboards to VP-711 $20
    Flat ribbon cable, 24" length, for connecting VP-601 or VP-
    611 and VP-711. Includes matching connector on both ends.

    VP-623 Cable: Parallel Output ASCII Keyboards $20
    Flat ribbon cable, 36" length with mating connector for VP-
    601 or VP-611 Keyboards. Other end is unterminated.

    VP-626 Connector: Serial Output ASCII Keyboards & Terminal $7
    25 pin solderable male "D" connector mates to VP-606, VP-616
    or VP-3301.

    TC1210 9" Video Monitor $195
    Ideal, low-cost monochrome monitor for displaying the video
    output from your microcomputer or terminal.

    TC1217 17" Video Monitor $480
    A really BIG monochrome monitor for use with your
    microcomputer or terminal 148 sq. in. pictures.

    VP-700 Tiny BASIC ROM Board $39
    Run Tiny BASIC on your VP-711! All BASIC code stored in ROM.
    Requires separate ASCII keyboard.

    VP-701 Floating Point BASIC for VP-711 $49
    16K bytes on cassette tape, includes floating point and
    integer math, string capability and color graphics. More than 70
    commands and statements. Available mid-1980.

    VP-710 Game Manual $10
    More exciting games for your VP-711! Includes Blackjack,
    Biorythm, Pinball, Bowling and 10 others.

    VP-720 Game Manual II More exciting games. Available mid-1980. $15

    VP-311 VP-711 Instruction Manual (Included with VP-711) $5

    VP-320 VP-711 User Guide Manual (Included with VP-711) $5

    MPM-201B CDP1802 User Manual (Included with VP-711) $5

    * Quantities of 15 or more available less case and speaker (Assembled
    keypad and circut board only). Price on request.

*/

#include "emu.h"
#include "includes/vip.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "audio/vp550.h"
#include "audio/vp595.h"
#include "sound/cdp1863.h"
#include "sound/discrete.h"
#include "video/cdp1861.h"
#include "video/cdp1862.h"
#include "machine/rescap.h"
#include "devices/messram.h"

static QUICKLOAD_LOAD( vip );
static MACHINE_RESET( vip );

/* Sound */

static const discrete_555_desc vip_ca555_a =
{
	DISC_555_OUT_SQW | DISC_555_OUT_DC,
	5,		// B+ voltage of 555
	DEFAULT_555_VALUES
};

static DISCRETE_SOUND_START( vip )
	DISCRETE_INPUT_LOGIC(NODE_01)
	DISCRETE_555_ASTABLE_CV(NODE_02, NODE_01, 470, RES_M(1), CAP_P(470), NODE_01, &vip_ca555_a)
	DISCRETE_OUTPUT(NODE_02, 5000)
DISCRETE_SOUND_END

/* Read/Write Handlers */

static WRITE8_HANDLER( keylatch_w )
{
	vip_state *state = space->machine->driver_data<vip_state>();

	state->keylatch = data & 0x0f;
}

static WRITE8_HANDLER( bankswitch_w )
{
	/* enable RAM */

	address_space *program = cputag_get_address_space(space->machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM);

	memory_set_bank(space->machine, "bank1", VIP_BANK_RAM);

	switch (messram_get_size(space->machine->device("messram")))
	{
	case 1 * 1024:
		memory_install_readwrite_bank(program, 0x0000, 0x03ff, 0, 0x7c00, "bank1");
		break;

	case 2 * 1024:
		memory_install_readwrite_bank(program, 0x0000, 0x07ff, 0, 0x7800, "bank1");
		break;

	case 4 * 1024:
		memory_install_readwrite_bank(program, 0x0000, 0x0fff, 0, 0x7000, "bank1");
		break;

	case 32 * 1024:
		memory_install_readwrite_bank(program, 0x0000, 0x7fff, 0, 0, "bank1");
		break;
	}
}

static READ8_DEVICE_HANDLER( vip_cdp1861_dispon_r )
{
	cdp1861_dispon_w(device, 1);
	cdp1861_dispon_w(device, 0);

	return 0xff;
}

static WRITE8_DEVICE_HANDLER( vip_cdp1861_dispoff_w )
{
	cdp1861_dispoff_w(device, 1);
	cdp1861_dispoff_w(device, 0);
}

/* Memory Maps */

static ADDRESS_MAP_START( vip_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE(0x0000, 0x7fff) AM_RAMBANK("bank1")
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK("bank2")
ADDRESS_MAP_END

static ADDRESS_MAP_START( vip_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1861_TAG, vip_cdp1861_dispon_r, vip_cdp1861_dispoff_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
//  AM_RANGE(0x03, 0x03) AM_DEVWRITE(CDP1863_TAG, cdp1863_str_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(bankswitch_w)
//  AM_RANGE(0x05, 0x05) AM_DEVWRITE(CDP1862_TAG, cdp1862_bkg_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( vip )
	PORT_START("KEYPAD")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("0 MW") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("A MR") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("B TR") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("F TW") PORT_CODE(KEYCODE_F) PORT_CHAR('F')

	PORT_START("VP-580")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 A") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 B") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 C") PORT_CODE(KEYCODE_I)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 D") PORT_CODE(KEYCODE_J)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 E") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("VP-580 F") PORT_CODE(KEYCODE_L)

	PORT_START("RUN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("Run/Reset") PORT_CODE(KEYCODE_R) PORT_TOGGLE

	PORT_START("KEYBOARD")
	PORT_CONFNAME( 0x07, 0x00, "Keyboard")
	PORT_CONFSETTING( 0x00, "Standard" )
	PORT_CONFSETTING( 0x01, "VP-580 Expansion Keyboard" )
	PORT_CONFSETTING( 0x02, "2x VP-580 Expansion Keyboard" )
	PORT_CONFSETTING( 0x03, "VP-601 ASCII Keyboard" )
	PORT_CONFSETTING( 0x04, "VP-611 ASCII/Numeric Keyboard" )

	PORT_START("VIDEO")
	PORT_CONFNAME( 0x01, 0x00, "Video")
	PORT_CONFSETTING( 0x00, "Standard" )
	PORT_CONFSETTING( 0x01, "VP-590 Color Board" )

	PORT_START("SOUND")
	PORT_CONFNAME( 0x03, 0x00, "Sound")
	PORT_CONFSETTING( 0x00, "Standard" )
	PORT_CONFSETTING( 0x01, "VP-595 Simple Sound Board" )
	PORT_CONFSETTING( 0x02, "VP-550 Super Sound Board" )
	PORT_CONFSETTING( 0x03, "VP-551 Super Sound Board" )
INPUT_PORTS_END

/* Video */

static WRITE_LINE_DEVICE_HANDLER( vip_efx_w )
{
	vip_state *driver_state = device->machine->driver_data<vip_state>();

	driver_state->cdp1861_efx = state;
}

static CDP1861_INTERFACE( vip_cdp1861_intf )
{
	CDP1802_TAG,
	SCREEN_TAG,
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_INT),
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, CDP1802_INPUT_LINE_DMAOUT),
	DEVCB_LINE(vip_efx_w)
};

static READ_LINE_DEVICE_HANDLER( rd_r )
{
	vip_state *state = device->machine->driver_data<vip_state>();

	return BIT(state->color, 1);
}

static READ_LINE_DEVICE_HANDLER( bd_r )
{
	vip_state *state = device->machine->driver_data<vip_state>();

	return BIT(state->color, 2);
}

static READ_LINE_DEVICE_HANDLER( gd_r )
{
	vip_state *state = device->machine->driver_data<vip_state>();

	return BIT(state->color, 3);
}

static CDP1862_INTERFACE( vip_cdp1862_intf )
{
	SCREEN_TAG,
	DEVCB_LINE(rd_r),
	DEVCB_LINE(bd_r),
	DEVCB_LINE(gd_r),
	510,			/* R3 */
	360,			/* R4 */
	RES_K(1),		/* R5 */
	RES_K(1.5),		/* R6 */
	RES_K(3.9),		/* R7 */
	RES_K(10),		/* R8 */
	RES_K(2),		/* R9 */
	RES_K(3.3)		/* R10 */
};

static WRITE8_HANDLER( vip_colorram_w )
{
	vip_state *state = space->machine->driver_data<vip_state>();
	UINT8 mask = 0xff;

	state->a12 = (offset & 0x1000) ? 1 : 0;

	if (!state->a12)
	{
		/* mask out A4 and A3 */
		mask = 0xe7;
	}

	/* write to CDP1822 */
	state->colorram[offset & mask] = data << 1;

	cdp1862_con_w(state->cdp1862, 0);
}

static VIDEO_UPDATE( vip )
{
	vip_state *state = screen->machine->driver_data<vip_state>();

	switch (input_port_read(screen->machine, "VIDEO"))
	{
	case VIP_VIDEO_CDP1861:
		cdp1861_update(state->cdp1861, bitmap, cliprect);
		break;

	case VIP_VIDEO_CDP1862:
		cdp1862_update(state->cdp1862, bitmap, cliprect);
		break;
	}

	return 0;
}

/* CDP1802 Configuration */

static CDP1802_MODE_READ( vip_mode_r )
{
	vip_state *state = device->machine->driver_data<vip_state>();

	if (input_port_read(device->machine, "RUN") & 0x01)
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
			running_machine *machine = device->machine;
			MACHINE_RESET_CALL(vip);

			state->reset = 1;
		}

		return CDP1802_MODE_RESET;
	}
}

static CDP1802_EF_READ( vip_ef_r )
{
	vip_state *state = device->machine->driver_data<vip_state>();

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
	if (cassette_input(state->cassette) < 0) flags -= EF2;
	set_led_status(device->machine, VIP_LED_TAPE, (cassette_input(state->cassette) > 0));

	/* keyboard */
	if (input_port_read(device->machine, "KEYPAD") & (1 << state->keylatch)) flags -= EF3;

	/* extended keyboard */
	switch (input_port_read(device->machine, "KEYBOARD"))
	{
	case VIP_KEYBOARD_VP580:
		if (input_port_read(device->machine, "VP-580") & (1 << state->keylatch)) flags -= EF4;
		break;
	}

	return flags;
}

static CDP1802_SC_WRITE( vip_sc_w )
{
//  vip_state *driver_state = device->machine->driver_data;

	switch (input_port_read(device->machine, "SOUND"))
	{
	case VIP_SOUND_VP550:
	case VIP_SOUND_VP551:
		vp550_sc1_w(device, sc1);
		break;
	}
}

static WRITE_LINE_DEVICE_HANDLER( vip_q_w )
{
	vip_state *driver_state = device->machine->driver_data<vip_state>();

	/* sound output */

	switch (input_port_read(device->machine, "SOUND"))
	{
	case VIP_SOUND_SPEAKER:
		discrete_sound_w(driver_state->beeper, NODE_01, state);
		vp595_q_w(driver_state->vp595, 0);
		vp550_q_w(driver_state->vp550, 0);
		break;

	case VIP_SOUND_VP595:
		discrete_sound_w(driver_state->beeper, NODE_01, 0);
		vp595_q_w(driver_state->vp595, state);
		vp550_q_w(driver_state->vp550, 0);
		break;

	case VIP_SOUND_VP550:
	case VIP_SOUND_VP551:
		discrete_sound_w(driver_state->beeper, NODE_01, 0);
		vp595_q_w(driver_state->vp595, 0);
		vp550_q_w(driver_state->vp550, state);
		break;
	}

	/* Q led */
	set_led_status(device->machine, VIP_LED_Q, state);

	/* tape output */
	cassette_output(driver_state->cassette, state ? 1.0 : -1.0);
}

static WRITE8_DEVICE_HANDLER( vip_dma_w )
{
	vip_state *state = device->machine->driver_data<vip_state>();

	switch (input_port_read(device->machine, "VIDEO"))
	{
	case VIP_VIDEO_CDP1861:
		cdp1861_dma_w(state->cdp1861, offset, data);
		break;

	case VIP_VIDEO_CDP1862:
		{
			UINT8 mask = 0xff;

			if (!state->a12)
			{
				/* mask out A4 and A3 */
				mask = 0xe7;
			}

			state->color = state->colorram[offset & mask];

			cdp1862_dma_w(state->cdp1862, offset, data);
		}
		break;
	}
}

static CDP1802_INTERFACE( vip_config )
{
	vip_mode_r,
	vip_ef_r,
	vip_sc_w,
	DEVCB_LINE(vip_q_w),
	DEVCB_NULL,
	DEVCB_HANDLER(vip_dma_w)
};

/* Machine Initialization */

static MACHINE_START( vip )
{
	vip_state *state = machine->driver_data<vip_state>();

	UINT8 *ram = memory_region(machine, CDP1802_TAG);
	UINT16 addr;

	/* RAM banking */
	memory_configure_bank(machine, "bank1", 0, 2, memory_region(machine, CDP1802_TAG), 0x8000);

	/* ROM banking */
	memory_install_read_bank(cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x8000, 0x81ff, 0, 0x7e00, "bank2");
	memory_unmap_write(cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM), 0x8000, 0x81ff, 0, 0x7e00);
	memory_configure_bank(machine, "bank2", 0, 1, memory_region(machine, CDP1802_TAG) + 0x8000, 0);
	memory_set_bank(machine, "bank2", 0);

	/* randomize RAM contents */
	for (addr = 0; addr < messram_get_size(machine->device("messram")); addr++)
	{
		ram[addr] = mame_rand(machine) & 0xff;
	}

	/* allocate color RAM */
	state->colorram = auto_alloc_array(machine, UINT8, VP590_COLOR_RAM_SIZE);

	/* enable power LED */
	set_led_status(machine, VIP_LED_POWER, 1);

	/* look up devices */
	state->cdp1861 = machine->device(CDP1861_TAG);
	state->cdp1862 = machine->device(CDP1862_TAG);
	state->cassette = machine->device(CASSETTE_TAG);
	state->beeper = machine->device(DISCRETE_TAG);
	state->vp595 = machine->device(VP595_TAG);
	state->vp550 = machine->device(VP550_TAG);
	state->vp551 = machine->device(VP551_TAG);

	/* reset sound */
	discrete_sound_w(state->beeper, NODE_01, 0);
	vp595_q_w(state->vp595, 0);
	vp550_q_w(state->vp550, 0);
	vp550_q_w(state->vp551, 0);

	/* register for state saving */
	state_save_register_global(machine, state->reset);
	state_save_register_global_pointer(machine, state->colorram, VP590_COLOR_RAM_SIZE);
	state_save_register_global(machine, state->cdp1861_efx);
	state_save_register_global(machine, state->color);
	state_save_register_global(machine, state->keylatch);
}

static MACHINE_RESET( vip )
{
	vip_state *state = machine->driver_data<vip_state>();

	address_space *program = cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM);
	address_space *io = cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_IO);

	/* reset auxiliary chips */
	state->cdp1861->reset();
	state->cdp1862->reset();

	/* reset devices */
	state->vp595->reset();
	state->vp550->reset();
	state->vp551->reset();

	/* configure video */
	switch (input_port_read(machine, "VIDEO"))
	{
	case VIP_VIDEO_CDP1861:
		memory_unmap_write(io, 0x05, 0x05, 0, 0);
		memory_unmap_write(program, 0xc000, 0xdfff, 0, 0);
		break;

	case VIP_VIDEO_CDP1862:
		memory_install_write8_device_handler(io, state->cdp1862, 0x05, 0x05, 0, 0, cdp1862_bkg_w);
		memory_install_write8_handler(program, 0xc000, 0xdfff, 0, 0, vip_colorram_w);
		break;
	}

	/* configure audio */
	switch (input_port_read(machine, "SOUND"))
	{
	case VIP_SOUND_SPEAKER:
		vp595_install_write_handlers(state->vp595, io, 0);
		vp550_install_write_handlers(state->vp550, program, 0);
		vp551_install_write_handlers(state->vp551, program, 0);
		break;

	case VIP_SOUND_VP595:
		vp595_install_write_handlers(state->vp595, io, 1);
		vp550_install_write_handlers(state->vp550, program, 0);
		vp551_install_write_handlers(state->vp551, program, 0);
		break;

	case VIP_SOUND_VP550:
		vp595_install_write_handlers(state->vp595, io, 0);
		vp550_install_write_handlers(state->vp550, program, 1);
		vp551_install_write_handlers(state->vp551, program, 0);
		break;

	case VIP_SOUND_VP551:
		vp595_install_write_handlers(state->vp595, io, 0);
		vp550_install_write_handlers(state->vp550, program, 0);
		vp551_install_write_handlers(state->vp551, program, 1);
		break;
	}

	/* enable ROM mirror at 0x0000 */
	memory_install_read_bank(program, 0x0000, 0x01ff, 0, 0x7e00, "bank1");
	memory_unmap_write(program, 0x0000, 0x01ff, 0, 0x7e00);
	memory_set_bank(machine, "bank1", VIP_BANK_ROM);
}

/* Machine Drivers */

static const cassette_config vip_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};

static MACHINE_CONFIG_START( vip, vip_state )

	/* basic machine hardware */
	MDRV_CPU_ADD(CDP1802_TAG, CDP1802, XTAL_3_52128MHz/2)
	MDRV_CPU_PROGRAM_MAP(vip_map)
	MDRV_CPU_IO_MAP(vip_io_map)
	MDRV_CPU_CONFIG(vip_config)

	MDRV_MACHINE_START(vip)
	MDRV_MACHINE_RESET(vip)

    /* video hardware */
	MDRV_CDP1861_SCREEN_ADD(SCREEN_TAG, XTAL_3_52128MHz/2)

	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(vip)

	MDRV_CDP1861_ADD(CDP1861_TAG, XTAL_3_52128MHz, vip_cdp1861_intf)
	MDRV_CDP1862_ADD(CDP1862_TAG, CPD1862_CLOCK, vip_cdp1862_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DISCRETE_TAG, DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(vip)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	MDRV_VP595_ADD
	MDRV_VP550_ADD(XTAL_3_52128MHz/2)
	MDRV_VP551_ADD(XTAL_3_52128MHz/2)

	/* devices */
	MDRV_QUICKLOAD_ADD("quickload", vip, "bin,c8,c8x", 0)
	MDRV_CASSETTE_ADD("cassette", vip_cassette_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("2K")
	MDRV_RAM_EXTRA_OPTIONS("4K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( vp111, vip )

	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("1K")
	MDRV_RAM_EXTRA_OPTIONS("2K,4K")
MACHINE_CONFIG_END

/* ROMs */

ROM_START( vip )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_LOAD( "cdpr566.u10", 0x8000, 0x0200, CRC(5be0a51f) SHA1(40266e6d13e3340607f8b3dcc4e91d7584287c06) )
	ROM_SYSTEM_BIOS( 0, "monitor", "Monitor" )
	ROM_SYSTEM_BIOS( 1, "vp700", "VP-700 Tiny BASIC" )
	ROMX_LOAD( "vp700.bin",	 0x9000, 0x1000, NO_DUMP, ROM_BIOS(2) )

	ROM_REGION( 0x200, "chip8", 0 )
	ROM_LOAD( "chip8.bin", 0x0000, 0x0200, CRC(438ec5d5) SHA1(8aa634c239004ff041c9adbf9144bd315ab5fc77) )

	ROM_REGION( 0x300, "chip8x", 0 )
	ROM_LOAD( "chip8x.bin", 0x0000, 0x0300, CRC(79c5f6f8) SHA1(ed438747b577399f6ccbf20fe14156f768842898) )
ROM_END

#define rom_vp111 rom_vip

/* System Configuration */

static QUICKLOAD_LOAD( vip )
{
	UINT8 *ptr = memory_region(image.device().machine, CDP1802_TAG);
	UINT8 *chip8_ptr = NULL;
	int chip8_size = 0;
	int size = image.length();

	if (strcmp(image.filetype(), "c8") == 0)
	{
		/* CHIP-8 program */
		chip8_ptr = memory_region(image.device().machine, "chip8");
		chip8_size = memory_region_length(image.device().machine, "chip8");
	}
	else if (strcmp(image.filename(), "c8x") == 0)
	{
		/* CHIP-8X program */
		chip8_ptr = memory_region(image.device().machine, "chip8x");
		chip8_size = memory_region_length(image.device().machine, "chip8x");
	}

	if ((size + chip8_size) > messram_get_size(image.device().machine->device("messram")))
	{
		return IMAGE_INIT_FAIL;
	}

	if (chip8_size > 0)
	{
		/* copy CHIP-8 interpreter to RAM */
		memcpy(ptr, chip8_ptr, chip8_size);
	}

	/* load image to RAM */
	image.fread( ptr + chip8_size, size);

	return IMAGE_INIT_PASS;
}

/* System Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT   INIT    COMPANY FULLNAME                FLAGS */
COMP( 1977, vip,	0,		0,		vip,		vip,	0,		"RCA",	"Cosmac VIP (VP-711)",	GAME_SUPPORTS_SAVE | GAME_IMPERFECT_COLORS )
COMP( 1977, vp111,	vip,	0,		vp111,		vip,	0,		"RCA",	"Cosmac VIP (VP-111)",	GAME_SUPPORTS_SAVE | GAME_IMPERFECT_COLORS )
