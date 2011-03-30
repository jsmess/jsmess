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

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "includes/vip.h"
#include "cpu/cosmac/cosmac.h"
#include "imagedev/cassette.h"
#include "imagedev/snapquik.h"
#include "audio/vp550.h"
#include "audio/vp595.h"
#include "sound/cdp1863.h"
#include "sound/discrete.h"
#include "video/cdp1861.h"
#include "video/cdp1862.h"
#include "machine/rescap.h"
#include "machine/ram.h"

static QUICKLOAD_LOAD( vip );

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

WRITE8_MEMBER( vip_state::keylatch_w )
{
	m_keylatch = data & 0x0f;
}

WRITE8_MEMBER( vip_state::bankswitch_w )
{
	/* enable RAM */
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	UINT8 *ram = ram_get_ptr(m_ram);

	switch (ram_get_size(m_ram))
	{
	case 1 * 1024:
		program->install_ram(0x0000, 0x03ff, 0, 0x7c00, ram);
		break;

	case 2 * 1024:
		program->install_ram(0x0000, 0x07ff, 0, 0x7800, ram);
		break;

	case 4 * 1024:
		program->install_ram(0x0000, 0x0fff, 0, 0x7000, ram);
		break;

	case 32 * 1024:
		program->install_ram(0x0000, 0x7fff, ram);
		break;
	}
}

READ8_MEMBER( vip_state::dispon_r )
{
	cdp1861_dispon_w(m_vdc, 1);
	cdp1861_dispon_w(m_vdc, 0);

	return 0xff;
}

WRITE8_MEMBER( vip_state::dispoff_w )
{
	cdp1861_dispoff_w(m_vdc, 1);
	cdp1861_dispoff_w(m_vdc, 0);
}

/* Memory Maps */

static ADDRESS_MAP_START( vip_map, AS_PROGRAM, 8, vip_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x01ff) AM_MIRROR(0xfe00) AM_ROM AM_REGION(CDP1802_TAG, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( vip_io_map, AS_IO, 8, vip_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x01, 0x01) AM_READWRITE(dispon_r, dispoff_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
//  AM_RANGE(0x03, 0x03) AM_DEVWRITE_LEGACY(CDP1863_TAG, cdp1863_str_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(bankswitch_w)
//  AM_RANGE(0x05, 0x05) AM_DEVWRITE_LEGACY(CDP1862_TAG, cdp1862_bkg_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_CHANGED( reset_w )
{
	vip_state *state = field->port->machine().driver_data<vip_state>();

	if (oldval && !newval)
	{
		state->machine_reset();
	}
}

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
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("Run/Reset") PORT_CODE(KEYCODE_R) PORT_TOGGLE PORT_CHANGED(reset_w, 0)

	PORT_START("KEYBOARD")
	PORT_CONFNAME( 0x07, KEYBOARD_KEYPAD, "Keyboard")
	PORT_CONFSETTING( KEYBOARD_KEYPAD, "Standard" )
	PORT_CONFSETTING( KEYBOARD_VP580, "VP-580 Expansion Keyboard" )
	PORT_CONFSETTING( KEYBOARD_DUAL_VP580, "2x VP-580 Expansion Keyboard" )
	PORT_CONFSETTING( KEYBOARD_VP601, "VP-601 ASCII Keyboard" )
	PORT_CONFSETTING( KEYBOARD_VP611, "VP-611 ASCII/Numeric Keyboard" )

	PORT_START("VIDEO")
	PORT_CONFNAME( 0x01, VIDEO_CDP1861, "Video")
	PORT_CONFSETTING( VIDEO_CDP1861, "Standard" )
	PORT_CONFSETTING( VIDEO_CDP1862, "VP-590 Color Board" )

	PORT_START("SOUND")
	PORT_CONFNAME( 0x03, SOUND_SPEAKER, "Sound")
	PORT_CONFSETTING( SOUND_SPEAKER, "Standard" )
	PORT_CONFSETTING( SOUND_VP595, "VP-595 Simple Sound Board" )
	PORT_CONFSETTING( SOUND_VP550, "VP-550 Super Sound Board" )
	PORT_CONFSETTING( SOUND_VP551, "VP-551 Super Sound Board" )
INPUT_PORTS_END

/* Video */

static CDP1861_INTERFACE( vip_cdp1861_intf )
{
	CDP1802_TAG,
	SCREEN_TAG,
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, COSMAC_INPUT_LINE_INT),
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, COSMAC_INPUT_LINE_DMAOUT),
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, COSMAC_INPUT_LINE_EF1)
};

static READ_LINE_DEVICE_HANDLER( rd_r )
{
	vip_state *state = device->machine().driver_data<vip_state>();

	return BIT(state->m_color, 1);
}

static READ_LINE_DEVICE_HANDLER( bd_r )
{
	vip_state *state = device->machine().driver_data<vip_state>();

	return BIT(state->m_color, 2);
}

static READ_LINE_DEVICE_HANDLER( gd_r )
{
	vip_state *state = device->machine().driver_data<vip_state>();

	return BIT(state->m_color, 3);
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

WRITE8_MEMBER( vip_state::vip_colorram_w )
{
	UINT8 mask = 0xff;

	m_a12 = (offset & 0x1000) ? 1 : 0;

	if (!m_a12)
	{
		// mask out A4 and A3
		mask = 0xe7;
	}

	// write to CDP1822
	m_colorram[offset & mask] = data << 1;

	cdp1862_con_w(m_cgc, 0);
}

bool vip_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	switch (input_port_read(m_machine, "VIDEO"))
	{
	case VIDEO_CDP1861:
		cdp1861_update(m_vdc, &bitmap, &cliprect);
		break;

	case VIDEO_CDP1862:
		cdp1862_update(m_cgc, &bitmap, &cliprect);
		break;
	}

	return 0;
}

/* CDP1802 Configuration */

static READ_LINE_DEVICE_HANDLER( clear_r )
{
	return BIT(input_port_read(device->machine(), "RUN"), 0);
}

static READ_LINE_DEVICE_HANDLER( ef2_r )
{
	set_led_status(device->machine(), LED_TAPE, (cassette_input(device) > 0));

	return cassette_input(device) < 0;
}

static READ_LINE_DEVICE_HANDLER( ef3_r )
{
	vip_state *state = device->machine().driver_data<vip_state>();

	return BIT(input_port_read(device->machine(), "KEYPAD"), state->m_keylatch);
}

static READ_LINE_DEVICE_HANDLER( ef4_r )
{
	vip_state *state = device->machine().driver_data<vip_state>();

	switch (input_port_read(device->machine(), "KEYBOARD"))
	{
	case KEYBOARD_VP580:
		return BIT(input_port_read(device->machine(), "VP-580"), state->m_keylatch);
	}

	return 0;
}

static COSMAC_SC_WRITE( vip_sc_w )
{
	switch (input_port_read(device->machine(), "SOUND"))
	{
	case SOUND_VP550:
		vp550_sc1_w(device, BIT(sc, 1)); // device = CPU since the handler calls device_set_input_line on it!
		break;

	case SOUND_VP551:
		vp550_sc1_w(device, BIT(sc, 1)); // device = CPU since the handler calls device_set_input_line on it!
		break;
	}
}

static WRITE_LINE_DEVICE_HANDLER( vip_q_w )
{
	vip_state *driver_state = device->machine().driver_data<vip_state>();

	// sound output
	switch (input_port_read(device->machine(), "SOUND"))
	{
	case SOUND_SPEAKER:
		discrete_sound_w(driver_state->m_beeper, NODE_01, state);
		vp595_q_w(driver_state->m_vp595, 0);
		vp550_q_w(driver_state->m_vp550, 0);
		break;

	case SOUND_VP595:
		discrete_sound_w(driver_state->m_beeper, NODE_01, 0);
		vp595_q_w(driver_state->m_vp595, state);
		vp550_q_w(driver_state->m_vp550, 0);
		break;

	case SOUND_VP550:
	case SOUND_VP551:
		discrete_sound_w(driver_state->m_beeper, NODE_01, 0);
		vp595_q_w(driver_state->m_vp595, 0);
		vp550_q_w(driver_state->m_vp550, state);
		break;
	}

	// Q led
	set_led_status(device->machine(), LED_Q, state);

	// tape output
	cassette_output(driver_state->m_cassette, state ? 1.0 : -1.0);
}

static WRITE8_DEVICE_HANDLER( vip_dma_w )
{
	vip_state *state = device->machine().driver_data<vip_state>();

	switch (input_port_read(device->machine(), "VIDEO"))
	{
	case VIDEO_CDP1861:
		cdp1861_dma_w(state->m_vdc, offset, data);
		break;

	case VIDEO_CDP1862:
		{
			UINT8 mask = 0xff;

			if (!state->m_a12)
			{
				// mask out A4 and A3
				mask = 0xe7;
			}

			state->m_color = state->m_colorram[offset & mask];

			cdp1862_dma_w(state->m_cgc, offset, data);
		}
		break;
	}
}

static COSMAC_INTERFACE( cosmac_intf )
{
	DEVCB_LINE_VCC,
	DEVCB_LINE(clear_r),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(CASSETTE_TAG, ef2_r),
	DEVCB_LINE(ef3_r),
	DEVCB_LINE(ef4_r),
	DEVCB_LINE(vip_q_w),
	DEVCB_NULL,
	DEVCB_HANDLER(vip_dma_w),
	vip_sc_w,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Machine Initialization */

void vip_state::machine_start()
{
	UINT8 *ram = ram_get_ptr(m_ram);

	/* randomize RAM contents */
	for (UINT16 addr = 0; addr < ram_get_size(m_ram); addr++)
	{
		ram[addr] = m_machine.rand() & 0xff;
	}

	/* allocate color RAM */
	m_colorram = auto_alloc_array(m_machine, UINT8, VP590_COLOR_RAM_SIZE);

	/* enable power LED */
	set_led_status(m_machine, LED_POWER, 1);

	/* reset sound */
	discrete_sound_w(m_beeper, NODE_01, 0);
	vp595_q_w(m_vp595, 0);
	vp550_q_w(m_vp550, 0);
	vp550_q_w(m_vp551, 0);

	/* register for state saving */
	state_save_register_global_pointer(m_machine, m_colorram, VP590_COLOR_RAM_SIZE);
	state_save_register_global(m_machine, m_color);
	state_save_register_global(m_machine, m_keylatch);
}

void vip_state::machine_reset()
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	address_space *io = m_maincpu->memory().space(AS_IO);

	/* reset auxiliary chips */
	m_vdc->reset();
	m_cgc->reset();

	/* reset devices */
	m_vp595->reset();
	m_vp550->reset();
	m_vp551->reset();

	/* configure video */
	switch (input_port_read(m_machine, "VIDEO"))
	{
	case VIDEO_CDP1861:
		io->unmap_write(0x05, 0x05);
		program->unmap_write(0xc000, 0xdfff);
		break;

	case VIDEO_CDP1862:
		io->install_legacy_write_handler(*m_cgc, 0x05, 0x05, FUNC(cdp1862_bkg_w));
//      program->install_legacy_write_handler(0xc000, 0xdfff, FUNC(vip_colorram_w));
		break;
	}

	/* configure audio */
	switch (input_port_read(m_machine, "SOUND"))
	{
	case SOUND_SPEAKER:
		vp595_install_write_handlers(m_vp595, io, 0);
		vp550_install_write_handlers(m_vp550, program, 0);
		vp551_install_write_handlers(m_vp551, program, 0);
		break;

	case SOUND_VP595:
		vp550_install_write_handlers(m_vp550, program, 0);
		vp551_install_write_handlers(m_vp551, program, 0);
		vp595_install_write_handlers(m_vp595, io, 1);
		break;

	case SOUND_VP550:
		vp595_install_write_handlers(m_vp595, io, 0);
		vp551_install_write_handlers(m_vp551, program, 0);
		vp550_install_write_handlers(m_vp550, program, 1);
		break;

	case SOUND_VP551:
		vp595_install_write_handlers(m_vp595, io, 0);
		vp550_install_write_handlers(m_vp550, program, 0);
		vp551_install_write_handlers(m_vp551, program, 1);
		break;
	}

	/* enable ROM all thru address space */
	program->install_rom(0x0000, 0x01ff, 0, 0xfe00, m_machine.region(CDP1802_TAG)->base());
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
	MCFG_CPU_ADD(CDP1802_TAG, COSMAC, XTAL_3_52128MHz/2)
	MCFG_CPU_PROGRAM_MAP(vip_map)
	MCFG_CPU_IO_MAP(vip_io_map)
	MCFG_CPU_CONFIG(cosmac_intf)

    /* video hardware */
	MCFG_CDP1861_SCREEN_ADD(SCREEN_TAG, XTAL_3_52128MHz/2)

	MCFG_PALETTE_LENGTH(16)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_CDP1861_ADD(CDP1861_TAG, XTAL_3_52128MHz, vip_cdp1861_intf)
	MCFG_CDP1862_ADD(CDP1862_TAG, CPD1862_CLOCK, vip_cdp1862_intf)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD(DISCRETE_TAG, DISCRETE, 0)
	MCFG_SOUND_CONFIG_DISCRETE(vip)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	MCFG_VP595_ADD
	MCFG_VP550_ADD(XTAL_3_52128MHz/2)
	MCFG_VP551_ADD(XTAL_3_52128MHz/2)

	/* devices */
	MCFG_QUICKLOAD_ADD("quickload", vip, "bin,c8,c8x", 0)
	MCFG_CASSETTE_ADD("cassette", vip_cassette_config)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("2K")
	MCFG_RAM_EXTRA_OPTIONS("4K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( vp111, vip )
	/* internal ram */
	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1K")
	MCFG_RAM_EXTRA_OPTIONS("2K,4K")
MACHINE_CONFIG_END

/* ROMs */

ROM_START( vip )
	ROM_REGION( 0x200, CDP1802_TAG, 0 )
	ROM_LOAD( "cdpr566.u10", 0x0000, 0x0200, CRC(5be0a51f) SHA1(40266e6d13e3340607f8b3dcc4e91d7584287c06) )

	ROM_REGION( 0x1000, "vp700", 0)
	ROM_LOAD( "vp700.bin", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x200, "chip8", 0 )
	ROM_LOAD( "chip8.bin", 0x0000, 0x0200, CRC(438ec5d5) SHA1(8aa634c239004ff041c9adbf9144bd315ab5fc77) )

	ROM_REGION( 0x300, "chip8x", 0 )
	ROM_LOAD( "chip8x.bin", 0x0000, 0x0300, CRC(79c5f6f8) SHA1(ed438747b577399f6ccbf20fe14156f768842898) )
ROM_END

#define rom_vp111 rom_vip

/* System Configuration */

static QUICKLOAD_LOAD( vip )
{
	UINT8 *ptr = image.device().machine().region(CDP1802_TAG)->base();
	UINT8 *chip8_ptr = NULL;
	int chip8_size = 0;
	int size = image.length();

	if (strcmp(image.filetype(), "c8") == 0)
	{
		/* CHIP-8 program */
		chip8_ptr = image.device().machine().region("chip8")->base();
		chip8_size = image.device().machine().region("chip8")->bytes();
	}
	else if (strcmp(image.filename(), "c8x") == 0)
	{
		/* CHIP-8X program */
		chip8_ptr = image.device().machine().region("chip8x")->base();
		chip8_size = image.device().machine().region("chip8x")->bytes();
	}

	if ((size + chip8_size) > ram_get_size(image.device().machine().device(RAM_TAG)))
	{
		return IMAGE_INIT_FAIL;
	}

	if (chip8_size > 0)
	{
		/* copy CHIP-8 interpreter to RAM */
		memcpy(ptr, chip8_ptr, chip8_size);
	}

	/* load image to RAM */
	image.fread(ptr + chip8_size, size);

	return IMAGE_INIT_PASS;
}

/* System Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT   INIT    COMPANY FULLNAME                FLAGS */
COMP( 1977, vip,	0,		0,		vip,		vip,	0,		"RCA",	"Cosmac VIP (VP-711)",	GAME_SUPPORTS_SAVE | GAME_IMPERFECT_COLORS )
COMP( 1977, vp111,	vip,	0,		vp111,		vip,	0,		"RCA",	"Cosmac VIP (VP-111)",	GAME_SUPPORTS_SAVE | GAME_IMPERFECT_COLORS )
