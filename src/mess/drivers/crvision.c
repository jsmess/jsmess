/*
    This 1980s computer was manufactured by Vtech of Hong Kong.
    known as: creatiVision, Dick Smith Wizzard, Funvision, Hanimex Ramses, Laser2001 and possibly others.

    There is also a CreatiVision Mk 2, possibly also known as the Laser 500. This was a proper computer,
    containing a CreatiVision cartridge slot.

    NOTE:

    If you find that the keys R,D,F,G are not working while using the BASIC cartridge,
    please remap the Joystick 2 keys to somewhere else. By default they overlap those keys.

    TODO:

    - BASIC v1,v2 numbers are invisible
    - proper keyboard emulation, need keyboard schematics
    - memory expansion 16K/32K, can be chained
    - centronics control/status port
    - convert to use new cartridge system
    - non-working cartridges:
        * Deep Sea Adventure (6K)
        * Locomotive (10K)
        * Planet Defender (6K)
        * Tennis (Dick Smith 6K)
        * Tennis (Wimbledon 6K)
    - homebrew roms with graphics issues:
        * Christmas Demo 1.0
        * Titanic Frogger Demo 1.0
        * Titanic Frogger Demo 1.1

*/

#include "driver.h"
#include "includes/crvision.h"
#include "cpu/m6502/m6502.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "machine/ctronics.h"
#include "machine/6821pia.h"
#include "sound/sn76496.h"
#include "sound/wave.h"
#include "video/tms9928a.h"
#include "devices/messram.h"

/* Read/Write Handlers */

static READ8_DEVICE_HANDLER( centronics_status_r )
{
	UINT8 data = 0;

	data |= centronics_busy_r(device) << 7;

	return 0;
}

static WRITE8_DEVICE_HANDLER( centronics_ctrl_w )
{
	centronics_strobe_w(device, BIT(data, 4));
}

/* Memory Map */

static ADDRESS_MAP_START( crvision_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_MIRROR(0x0c00) AM_RAM
	AM_RANGE(0x1000, 0x1003) AM_MIRROR(0x0ffc) AM_DEVREADWRITE(PIA6821_TAG, pia6821_r, pia6821_w)
	AM_RANGE(0x2000, 0x2000) AM_MIRROR(0x0ffe) AM_READ(TMS9928A_vram_r)
	AM_RANGE(0x2001, 0x2001) AM_MIRROR(0x0ffe) AM_READ(TMS9928A_register_r)
	AM_RANGE(0x3000, 0x3000) AM_MIRROR(0x0ffe) AM_WRITE(TMS9928A_vram_w)
	AM_RANGE(0x3001, 0x3001) AM_MIRROR(0x0ffe) AM_WRITE(TMS9928A_register_w)
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(BANK_ROM2)
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(BANK_ROM1)
	AM_RANGE(0xc000, 0xc7ff) AM_MIRROR(0x3800) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( wizzard_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_MIRROR(0x0c00) AM_RAM
	AM_RANGE(0x1000, 0x1003) AM_MIRROR(0x0ffc) AM_DEVREADWRITE(PIA6821_TAG, pia6821_r, pia6821_w)
	AM_RANGE(0x2000, 0x2000) AM_MIRROR(0x0ffe) AM_READ(TMS9928A_vram_r)
	AM_RANGE(0x2001, 0x2001) AM_MIRROR(0x0ffe) AM_READ(TMS9928A_register_r)
	AM_RANGE(0x3000, 0x3000) AM_MIRROR(0x0ffe) AM_WRITE(TMS9928A_vram_w)
	AM_RANGE(0x3001, 0x3001) AM_MIRROR(0x0ffe) AM_WRITE(TMS9928A_register_w)
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(BANK_ROM2)
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(BANK_ROM1)
//  AM_RANGE(0xc000, 0xe7ff) AM_RAMBANK(3)
	AM_RANGE(0xe800, 0xe800) AM_DEVWRITE(CENTRONICS_TAG, centronics_data_w)
	AM_RANGE(0xe801, 0xe801) AM_DEVREADWRITE(CENTRONICS_TAG, centronics_status_r, centronics_ctrl_w)
//  AM_RANGE(0xf000, 0xf7ff) AM_RAMBANK(4)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

static INPUT_CHANGED( trigger_nmi )
{
	cputag_set_input_line(field->port->machine, M6502_TAG, INPUT_LINE_NMI, (input_port_read(field->port->machine, "NMI") ? CLEAR_LINE : ASSERT_LINE));
}

static INPUT_PORTS_START( crvision )
	// Player 1 Joystick

	PORT_START("PA0-0")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0xfd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-2")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0xf3, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-3")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0xf7, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-4")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-5")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0xdf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-6")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA0-7")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Button 2 / CNT'L") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)

	// Player 1 Keyboard

	PORT_START("PA1-0")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x81, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-1")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x83, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-2")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x87, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-3")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x8f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-4")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x9f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-5")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0xbf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-6")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA1-7")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Button 1 / SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)

	// Player 2 Joystick

	PORT_START("PA2-0")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA2-1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0xfd, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA2-2")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0xf3, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA2-3")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0xf7, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA2-4")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA2-5")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0xdf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA2-6")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA2-7")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P2 Button 2 / \xE2\x86\x92") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9) PORT_PLAYER(2)

	// Player 2 Keyboard

	PORT_START("PA3-0")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RET'N") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x81, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA3-1")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('@')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x83, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA3-2")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x87, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA3-3")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x8f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA3-4")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x9f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA3-5")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0xbf, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA3-6")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA3-7")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P2 Button 1 / - =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=') PORT_PLAYER(2)

	PORT_START("NMI")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START ) PORT_NAME("Reset") PORT_CODE(KEYCODE_F10) PORT_CHANGED(trigger_nmi, 0)
INPUT_PORTS_END

/* Machine Interface */

static INTERRUPT_GEN( crvision_int )
{
    TMS9928A_interrupt(device->machine);
}

static void crvision_vdp_interrupt(running_machine *machine, int state)
{
	cputag_set_input_line(machine, M6502_TAG, INPUT_LINE_IRQ0, state);
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

static WRITE8_DEVICE_HANDLER( crvision_pia_porta_w )
{
	/*
        Signal  Description

        PA0     Keyboard raster player 1 output (joystick)
        PA1     Keyboard raster player 1 output (hand keys)
        PA2     Keyboard raster player 2 output (joystick)
        PA3     Keyboard raster player 2 output (hand keys)
        PA4     ?
        PA5     ?
        PA6     Cassette motor
        PA7     Cassette data in/out
    */

	crvision_state *state = device->machine->driver_data;

	/* keyboard raster */
	state->keylatch = ~data & 0x0f;

	/* cassette motor */
	cassette_change_state(state->cassette, BIT(data, 6) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);

	/* cassette data output */
	cassette_output(state->cassette, BIT(data, 7) ? +1.0 : -1.0);
}

static UINT8 read_keyboard(running_machine *machine, int pa)
{
	int i;
	UINT8 value;
	static const char *const keynames[4][8] =
			{
				{ "PA0-0", "PA0-1", "PA0-2", "PA0-3", "PA0-4", "PA0-5", "PA0-6", "PA0-7" },
				{ "PA1-0", "PA1-1", "PA1-2", "PA1-3", "PA1-4", "PA1-5", "PA1-6", "PA1-7" },
				{ "PA2-0", "PA2-1", "PA2-2", "PA2-3", "PA2-4", "PA2-5", "PA2-6", "PA2-7" },
				{ "PA3-0", "PA3-1", "PA3-2", "PA3-3", "PA3-4", "PA3-5", "PA3-6", "PA3-7" }
			};

	for (i = 0; i < 8; i++)
	{
		value = input_port_read(machine, keynames[pa][i]);

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

static READ8_DEVICE_HANDLER( crvision_pia_porta_r )
{
	/*
        PA0     Keyboard raster player 1 output (joystick)
        PA1     Keyboard raster player 1 output (hand keys)
        PA2     Keyboard raster player 2 output (joystick)
        PA3     Keyboard raster player 2 output (hand keys)
        PA4     ?
        PA5     ?
        PA6     Cassette motor
        PA7     Cassette data in/out
    */

	crvision_state *state = device->machine->driver_data;

	UINT8 data = 0x7f;

	if (cassette_input(state->cassette) > -0.1469) data |= 0x80;

	return data;
}

static READ8_DEVICE_HANDLER( crvision_pia_portb_r )
{
	/*
        Signal  Description

        PB0     Keyboard input
        PB1     Keyboard input
        PB2     Keyboard input
        PB3     Keyboard input
        PB4     Keyboard input
        PB5     Keyboard input
        PB6     Keyboard input
        PB7     Keyboard input
    */

	crvision_state *state = device->machine->driver_data;

	UINT8 data = 0xff;

	if (BIT(state->keylatch, 0)) data &= read_keyboard(device->machine, 0);
	if (BIT(state->keylatch, 1)) data &= read_keyboard(device->machine, 1);
	if (BIT(state->keylatch, 2)) data &= read_keyboard(device->machine, 2);
	if (BIT(state->keylatch, 3)) data &= read_keyboard(device->machine, 3);

	return data;
}

static READ_LINE_DEVICE_HANDLER( crvision_pia_cb1_r )
{
	crvision_state *state = device->machine->driver_data;

	return sn76496_ready_r(state->sn76489, 0);
}

static WRITE_LINE_DEVICE_HANDLER( crvision_pia_cb2_w )
{
}

static WRITE8_DEVICE_HANDLER( crvision_pia_portb_w )
{
	/*
        Signal  Description

        PB0     SN76489 data output
        PB1     SN76489 data output
        PB2     SN76489 data output
        PB3     SN76489 data output
        PB4     SN76489 data output
        PB5     SN76489 data output
        PB6     SN76489 data output
        PB7     SN76489 data output
    */

	crvision_state *state = device->machine->driver_data;

	sn76496_w(state->sn76489, 0, data);
}

static const pia6821_interface crvision_pia_intf =
{
	DEVCB_HANDLER(crvision_pia_porta_r),	// input A
	DEVCB_HANDLER(crvision_pia_portb_r),	// input B
	DEVCB_LINE_VCC,							// input CA1 (+5V)
	DEVCB_LINE(crvision_pia_cb1_r),			// input CB1 (SN76489 pin READY )
	DEVCB_LINE_VCC,							// input CA2 (+5V)
	DEVCB_LINE_VCC,							// input CB2 (+5V)
	DEVCB_HANDLER(crvision_pia_porta_w),	// output A
	DEVCB_HANDLER(crvision_pia_portb_w),	// output B (SN76489 pins D0-D7)
	DEVCB_NULL,								// output CA2
	DEVCB_LINE(crvision_pia_cb2_w),			// output CB2 (SN76489 pin CE_)
	DEVCB_NULL,								// irq A
	DEVCB_NULL								// irq B
};

static MACHINE_START( creativision )
{
	crvision_state *state = machine->driver_data;

	/* find devices */
	state->sn76489 = devtag_get_device(machine, SN76489_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->keylatch);
}

static MACHINE_START( ntsc )
{
	MACHINE_START_CALL(creativision);

	TMS9928A_configure(&tms9918_intf);
}

static MACHINE_START( pal )
{
	MACHINE_START_CALL(creativision);

	TMS9928A_configure(&tms9929_intf);
}

static DEVICE_IMAGE_LOAD( crvision_cart )
{
	int size = image_length(image);
	running_machine *machine = image->machine;
	UINT8 *mem = memory_region(machine, M6502_TAG);
	const address_space *program = cputag_get_address_space(machine, M6502_TAG, ADDRESS_SPACE_PROGRAM);

	switch (size)
	{
	case 0x1000: // 4K
		image_fread(image, mem + 0x9000, 0x1000);
		memory_install_read8_handler(program, 0x8000, 0x9fff, 0, 0x2000, SMH_BANK(BANK_ROM1));
		break;

	case 0x1800: // 6K
		image_fread(image, mem + 0x9000, 0x1000);
		image_fread(image, mem + 0x8800, 0x0800);
		memory_install_read8_handler(program, 0x8000, 0x9fff, 0, 0x2000, SMH_BANK(BANK_ROM1));
		break;

	case 0x2000: // 8K
		image_fread(image, mem + 0x8000, 0x2000);
		memory_install_read8_handler(program, 0x8000, 0x9fff, 0, 0x2000, SMH_BANK(BANK_ROM1));
		break;

	case 0x2800: // 10K
		image_fread(image, mem + 0x8000, 0x2000);
		image_fread(image, mem + 0x5800, 0x0800);
		memory_install_read8_handler(program, 0x8000, 0x9fff, 0, 0x2000, SMH_BANK(BANK_ROM1));
		memory_install_read8_handler(program, 0x4000, 0x5fff, 0, 0x2000, SMH_BANK(BANK_ROM2));
		break;

	case 0x3000: // 12K
		image_fread(image, mem + 0x8000, 0x2000);
		image_fread(image, mem + 0x5000, 0x1000);
		memory_install_read8_handler(program, 0x8000, 0x9fff, 0, 0x2000, SMH_BANK(BANK_ROM1));
		memory_install_read8_handler(program, 0x4000, 0x5fff, 0, 0x2000, SMH_BANK(BANK_ROM2));
		break;

	case 0x4000: // 16K
		image_fread(image, mem + 0xa000, 0x2000);
		image_fread(image, mem + 0x8000, 0x2000);
		memory_install_read8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_BANK(BANK_ROM1));
		memory_install_read8_handler(program, 0x4000, 0x7fff, 0, 0, SMH_BANK(BANK_ROM2));
		break;

	case 0x4800: // 18K
		image_fread(image, mem + 0xa000, 0x2000);
		image_fread(image, mem + 0x8000, 0x2000);
		image_fread(image, mem + 0x4800, 0x0800);
		memory_install_read8_handler(program, 0x8000, 0x8fff, 0, 0, SMH_BANK(BANK_ROM1));
		memory_install_read8_handler(program, 0x4000, 0x4fff, 0, 0x3000, SMH_BANK(BANK_ROM2));
		break;

	default:
		return INIT_FAIL;
	}

	memory_configure_bank(machine, 1, 0, 1, mem + 0x8000, 0);
	memory_set_bank(machine, 1, 0);

	memory_configure_bank(machine, 2, 0, 1, mem + 0x4000, 0);
	memory_set_bank(machine, 2, 0);

	return INIT_PASS;
}

/* Machine Driver */

static const cassette_config crvision_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED
};

static MACHINE_DRIVER_START( creativision )
	MDRV_DRIVER_DATA(crvision_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502_TAG, M6502, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(crvision_map)
	MDRV_CPU_VBLANK_INT(SCREEN_TAG, crvision_int)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76489_TAG, SN76489, XTAL_2MHz)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_SOUND_WAVE_ADD("wave", CASSETTE_TAG)
	MDRV_SOUND_ROUTE(1, "mono", 0.25)

	/* peripheral hardware */
	MDRV_PIA6821_ADD(PIA6821_TAG, crvision_pia_intf)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin,rom")
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_LOAD(crvision_cart)

	/* cassette */
	MDRV_CASSETTE_ADD(CASSETTE_TAG, crvision_cassette_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ntsc )
	MDRV_IMPORT_FROM(creativision)

	MDRV_MACHINE_START(ntsc)

    /* video hardware */
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY(SCREEN_TAG)
	MDRV_SCREEN_REFRESH_RATE((float)XTAL_10_738635MHz/2/342/262)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pal )
	MDRV_IMPORT_FROM(creativision)

	MDRV_MACHINE_START(pal)

	/* video hardware */
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY(SCREEN_TAG)
	MDRV_SCREEN_REFRESH_RATE((float)XTAL_10_738635MHz/2/342/313)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wizzard )
	MDRV_IMPORT_FROM(pal)

	MDRV_CPU_MODIFY(M6502_TAG)
	MDRV_CPU_PROGRAM_MAP(wizzard_map)

	/* printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)
	
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1K")
	MDRV_RAM_EXTRA_OPTIONS("17K,33K") // 16K or 32K expansion
MACHINE_DRIVER_END

/* ROMs */

ROM_START( crvision )
    ROM_REGION( 0x10000, M6502_TAG, 0 )
    ROM_LOAD( "crvision.u20", 0xc000, 0x0800, CRC(c3c590c6) SHA1(5ac620c529e4965efb5560fe824854a44c983757) )
ROM_END

ROM_START( wizzard )
    ROM_REGION( 0x10000, M6502_TAG, 0 )
    ROM_LOAD( "wizzard.u20",  0xf800, 0x0800, CRC(c3c590c6) SHA1(5ac620c529e4965efb5560fe824854a44c983757) )
ROM_END

ROM_START( fnvision )
    ROM_REGION( 0x10000, M6502_TAG, 0 )
    ROM_LOAD( "funboot.rom",  0xc000, 0x0800, CRC(05602697) SHA1(c280b20c8074ba9abb4be4338b538361dfae517f) )
ROM_END

#define rom_crvisioj rom_crvision
#define rom_crvisio2 rom_crvision
#define rom_rameses rom_fnvision
#define rom_vz2000 rom_fnvision

/* System Drivers */

/*    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT       INIT    CONFIG      COMPANY                     FULLNAME */
CONS( 1982, crvision, 0,		0,		pal,		crvision,	0,		0,			"Video Technology",			"CreatiVision", 0 )
CONS( 1982, fnvision, crvision, 0,		pal,		crvision,	0,		0,			"Video Technology",			"FunVision", 0 )
CONS( 1982, crvisioj, crvision,	0,		ntsc,		crvision,	0,		0,			"Cheryco",					"CreatiVision (Japan)", 0 )
CONS( 1982, wizzard,  crvision, 0,		wizzard,	crvision,	0,		0,	"Dick Smith Electronics",	"Wizzard (Oceania)", 0 )
CONS( 1982, rameses,  crvision, 0,		pal,		crvision,	0,		0,			"Hanimex",					"Rameses (Oceania)", 0 )
CONS( 1983, vz2000,   crvision, 0,		pal,		crvision,	0,		0,			"Dick Smith Electronics",	"VZ 2000 (Oceania)", 0 )
CONS( 1983, crvisio2, crvision, 0,		pal,		crvision,	0,		0,			"Sanyo Video",				"CreatiVision MK-II (Germany)", 0 )
/*
COMP( 1983, lasr2001, 0,        0,      lasr2001,   lasr2001,   0,      lasr2001,   "Video Technology",         "Laser 2001", GAME_NOT_WORKING )
COMP( 1983, vz2001,   lasr2001, 0,      lasr2001,   lasr2001,   0,      lasr2001,   "Dick Smith Electronics",   "VZ 2001 (Oceania)", GAME_NOT_WORKING )
COMP( 1983, manager,  lasr2001, 0,      lasr2001,   lasr2001,   0,      lasr2001,   "Salora",                   "Manager (Finland)", GAME_NOT_WORKING )
*/
