/*

[CBM systems which belong to this driver (info to be moved to sysinfo.dat soon)]
(most of the informations are taken from http://www.zimmers.net/cbmpics/ )


* VIC-1001 (1981, Japan)

  The first model released was the Japanese one. It featured support for the
Japanese katakana character.

CPU: MOS Technology 6502 (1.01 MHz)
RAM: 5 kilobytes (Expanded to 21k though an external 16k unit)
ROM: 20 kilobytes
Video: MOS Technology 6560 "VIC"(Text: 22 columns, 23 rows; Hires: 176x184
pixels bitmapped; 8 text colors, 16 background colors)
Sound: MOS Technology 6560 "VIC" (3 voices -square wave-, noise and volume)
Ports: 6522 VIA x2 (1 Joystick/Mouse port; CBM Serial port; 'Cartridge /
    Game / Expansion' port; CBM Monitor port; CBM 'USER' port; Power and
    reset switches; Power connector)
Keyboard: Full-sized QWERTY 66 key (8 programmable function keys; 2 sets of
    Keyboardable graphic characters; 2 key direction cursor-pad)


* VIC 20 (1981)

  This system was the first computer to sell more than one million units
worldwide. It was sold both in Europe and in the US. In Germany the
computer was renamed as VC 20 (apparently, it stands for 'VolksComputer'

CPU: MOS Technology 6502A (1.01 MHz)
RAM: 5 kilobytes (Expanded to 32k)
ROM: 20 kilobytes
Video: MOS Technology 6560 "VIC"(Text: 22 columns, 23 rows; Hires: 176x184
pixels bitmapped; 8 text colors, 16 background colors)
Sound: MOS Technology 6560 "VIC" (3 voices -square wave-, noise and volume)
Ports: 6522 VIA x2 (1 Joystick/Mouse port; CBM Serial port; 'Cartridge /
    Game / Expansion' port; CBM Monitor port; CBM 'USER' port; Power and
    reset switches; Power connector)
Keyboard: Full-sized QWERTY 66 key (8 programmable function keys; 2 sets of
    Keyboardable graphic characters; 2 key direction cursor-pad)


* VIC 21 (1983)

  It consists of a VIC 20 with built-in RAM expansion, to reach a RAM
  capability of 21 kilobytes.


* VIC 20CR

  CR stands for Cost Reduced, as it consisted of a board with only 2 (larger)
block of RAM instead of 8.

*******************************************************************************

    TODO:

    - C1540 is unreliable (VIA timing issues?)
    - clean up inputs
    - clean up VIA interface
    - access violation in mos6560.c
        * In the Chips (Japan, USA).60
        * K-Star Patrol (Europe).60
        * Seafox (Japan, USA).60
    - SHIFT LOCK
    - restore key
    - light pen
    - VIC21 (built in 21K ram)
    - new cart system (rpk)

*/

#include "emu.h"
#include "includes/vic20.h"
#include "includes/cbm.h"
#include "formats/cbm_snqk.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/cartslot.h"
#include "machine/ram.h"
#include "machine/6522via.h"
#include "machine/c1541.h"
#include "machine/cbmiec.h"
#include "machine/ieee488.h"
#include "machine/vic1112.h"
#include "machine/cbmipt.h"
#include "sound/mos6560.h"
#include "sound/dac.h"

/* Memory Maps */

static ADDRESS_MAP_START( vic20_mem, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
//  AM_RANGE(0x0400, 0x07ff) RAM1
//  AM_RANGE(0x0800, 0x0bff) RAM2
//  AM_RANGE(0x0c00, 0x0fff) RAM3
	AM_RANGE(0x1000, 0x1fff) AM_RAM
//  AM_RANGE(0x2000, 0x3fff) BLK1
//  AM_RANGE(0x4000, 0x5fff) BLK2
//  AM_RANGE(0x6000, 0x7fff) BLK3
	AM_RANGE(0x8000, 0x8fff) AM_ROM
	AM_RANGE(0x9000, 0x900f) AM_DEVREADWRITE(M6560_TAG, mos6560_port_r, mos6560_port_w)
	AM_RANGE(0x9110, 0x911f) AM_DEVREADWRITE_MODERN(M6522_0_TAG, via6522_device, read, write)
	AM_RANGE(0x9120, 0x912f) AM_DEVREADWRITE_MODERN(M6522_1_TAG, via6522_device, read, write)
	AM_RANGE(0x9400, 0x97ff) AM_RAM
//  AM_RANGE(0x9800, 0x9bff) I/O2
//  AM_RANGE(0x9c00, 0x9fff) I/O3
//  AM_RANGE(0xa000, 0xbfff) BLK5
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

#ifdef UNUSED_FUNCTION
static INPUT_PORTS_START( vic_lightpen_6560 )
	PORT_START( "LIGHTX" )
	PORT_BIT( 0xff, 0, IPT_PADDLE ) PORT_NAME("Lightpen X Axis") PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(MOS6560_MAME_XSIZE - 1)) PORT_CATEGORY(12) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH)

	PORT_START( "LIGHTY" )
	PORT_BIT( 0xff, 0, IPT_PADDLE ) PORT_NAME("Lightpen Y Axis") PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(MOS6560_MAME_YSIZE - 1)) PORT_CATEGORY(12) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH)
INPUT_PORTS_END


static INPUT_PORTS_START( vic_lightpen_6561 )
	PORT_START( "LIGHTX" )
	PORT_BIT( 0xff, 0, IPT_PADDLE ) PORT_NAME("Lightpen X Axis") PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(MOS6560_MAME_XSIZE - 1)) PORT_CATEGORY(12) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH)

	PORT_START( "LIGHTY" )
	PORT_BIT( 0x1ff, 0, IPT_PADDLE ) PORT_NAME("Lightpen Y Axis") PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(MOS6561_MAME_YSIZE - 1)) PORT_CATEGORY(12) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH)
INPUT_PORTS_END
#endif

static INPUT_PORTS_START( vic20 )
	PORT_INCLUDE( vic_keyboard )       /* ROW0 -> ROW7 */

	PORT_INCLUDE( vic_special )        /* SPECIAL */

	PORT_INCLUDE( vic_controls )       /* CTRLSEL, JOY, FAKE0, FAKE1, PADDLE0, PADDLE1 */
INPUT_PORTS_END


static INPUT_PORTS_START( vic1001 )
	PORT_INCLUDE( vic20 )

	PORT_MODIFY( "ROW0" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('\xA5')
INPUT_PORTS_END


static INPUT_PORTS_START( vic20s )
	PORT_INCLUDE( vic20 )

	PORT_MODIFY( "ROW0" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2)		PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)			PORT_CHAR('-')
	PORT_MODIFY( "ROW1" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)		PORT_CHAR('@')
	PORT_MODIFY( "ROW2" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE)			PORT_CHAR(0x00C4)
	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH)		PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON)			PORT_CHAR(0x00D6)
	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE)		PORT_CHAR(0x00C5)
	PORT_MODIFY( "ROW7" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)			PORT_CHAR('=')
INPUT_PORTS_END

/* VIA 0 Interface */

static READ8_DEVICE_HANDLER( via0_pa_r )
{
	/*

        bit     description

        PA0     SERIAL CLK IN
        PA1     SERIAL DATA IN
        PA2     JOY 0
        PA3     JOY 1
        PA4     JOY 2
        PA5     LITE PEN
        PA6     CASS SWITCH
        PA7     SERIAL ATN OUT

    */

	vic20_state *state = device->machine().driver_data<vic20_state>();
	UINT8 data = 0xfc;

	/* serial clock in */
	data |= cbm_iec_clk_r(state->m_iec);

	/* serial data in */
	data |= cbm_iec_data_r(state->m_iec) << 1;

	/* joystick */
	data &= ~(input_port_read(device->machine(), "JOY") & 0x3c);

	/* cassette switch */
	if ((cassette_get_state(state->m_cassette) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
		data &= ~0x40;
	else
		data |=  0x40;

	return data;
}

static WRITE8_DEVICE_HANDLER( via0_pa_w )
{
	/*

        bit     description

        PA0     SERIAL CLK IN
        PA1     SERIAL DATA IN
        PA2     JOY 0
        PA3     JOY 1
        PA4     JOY 2
        PA5     LITE PEN
        PA6     CASS SWITCH
        PA7     SERIAL ATN OUT

    */

	vic20_state *state = device->machine().driver_data<vic20_state>();

	/* serial attention out */
	cbm_iec_atn_w(state->m_iec, device, !BIT(data, 7));
}

static READ8_DEVICE_HANDLER( via0_pb_r )
{
	/*

        bit     description

        PB0     USER PORT C
        PB1     USER PORT D
        PB2     USER PORT E
        PB3     USER PORT F
        PB4     USER PORT H
        PB5     USER PORT J
        PB6     USER PORT K
        PB7     USER PORT L

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via0_pb_w )
{
	/*

        bit     description

        PB0     USER PORT C
        PB1     USER PORT D
        PB2     USER PORT E
        PB3     USER PORT F
        PB4     USER PORT H
        PB5     USER PORT J
        PB6     USER PORT K
        PB7     USER PORT L

    */
}

static WRITE8_DEVICE_HANDLER( via0_ca2_w )
{
	vic20_state *state = device->machine().driver_data<vic20_state>();

	if (!BIT(data, 0))
	{
		cassette_change_state(state->m_cassette, CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
		state->m_cassette_timer->enable(true);
	}
	else
	{
		cassette_change_state(state->m_cassette, CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
		state->m_cassette_timer->enable(false);
	}
}

static const via6522_interface vic20_via0_intf =
{
	DEVCB_HANDLER(via0_pa_r),
	DEVCB_HANDLER(via0_pb_r),
	DEVCB_NULL, // RESTORE
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(via0_pa_w),
	DEVCB_HANDLER(via0_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(via0_ca2_w), // CASS MOTOR
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE(M6502_TAG, INPUT_LINE_NMI)
};

/* VIA 1 Interface */

static READ8_DEVICE_HANDLER( via1_pa_r )
{
	/*

        bit     description

        PA0     ROW 0
        PA1     ROW 1
        PA2     ROW 2
        PA3     ROW 3
        PA4     ROW 4
        PA5     ROW 5
        PA6     ROW 6
        PA7     ROW 7

    */

	vic20_state *state = device->machine().driver_data<vic20_state>();
	UINT8 data = 0xff;

	if (!BIT(state->m_key_col, 0)) data &= input_port_read(device->machine(), "ROW0");
	if (!BIT(state->m_key_col, 1)) data &= input_port_read(device->machine(), "ROW1");
	if (!BIT(state->m_key_col, 2)) data &= input_port_read(device->machine(), "ROW2");
	if (!BIT(state->m_key_col, 3)) data &= input_port_read(device->machine(), "ROW3");
	if (!BIT(state->m_key_col, 4)) data &= input_port_read(device->machine(), "ROW4");
	if (!BIT(state->m_key_col, 5)) data &= input_port_read(device->machine(), "ROW5");
	if (!BIT(state->m_key_col, 6)) data &= input_port_read(device->machine(), "ROW6");
	if (!BIT(state->m_key_col, 7)) data &= input_port_read(device->machine(), "ROW7");

	return data;
}

static READ8_DEVICE_HANDLER( via1_pb_r )
{
	/*

        bit     description

        PB0     COL 0
        PB1     COL 1
        PB2     COL 2
        PB3     COL 3, CASS WRITE
        PB4     COL 4
        PB5     COL 5
        PB6     COL 6
        PB7     COL 7, JOY 3

    */

	UINT8 data = 0xff;

	/* joystick */
	data &= ~(input_port_read(device->machine(), "JOY") & 0x80);

	return data;
}

static WRITE8_DEVICE_HANDLER( via1_pb_w )
{
	/*

        bit     description

        PB0     COL 0
        PB1     COL 1
        PB2     COL 2
        PB3     COL 3, CASS WRITE
        PB4     COL 4
        PB5     COL 5
        PB6     COL 6
        PB7     COL 7, JOY 3

    */

	vic20_state *state = device->machine().driver_data<vic20_state>();

	/* cassette write */
	cassette_output(device->machine().device("cassette"), BIT(data, 3) ? -(0x5a9e >> 1) : +(0x5a9e >> 1));

	/* keyboard column */
	state->m_key_col = data;
}

static WRITE_LINE_DEVICE_HANDLER( via1_ca2_w )
{
	vic20_state *driver_state = device->machine().driver_data<vic20_state>();

	/* serial clock out */
	cbm_iec_clk_w(driver_state->m_iec, device, !state);
}

static WRITE_LINE_DEVICE_HANDLER( via1_cb2_w )
{
	vic20_state *driver_state = device->machine().driver_data<vic20_state>();

	/* serial data out */
	cbm_iec_data_w(driver_state->m_iec, device, !state);
}

static const via6522_interface vic20_via1_intf =
{
	DEVCB_HANDLER(via1_pa_r),
	DEVCB_HANDLER(via1_pb_r),
	DEVCB_NULL, /* CASS READ */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_HANDLER(via1_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(via1_ca2_w),
	DEVCB_LINE(via1_cb2_w),

	DEVCB_CPU_INPUT_LINE(M6502_TAG, M6502_IRQ_LINE)
};

/* Cassette Interface */

static TIMER_DEVICE_CALLBACK( cassette_tick )
{
	vic20_state *state = timer.machine().driver_data<vic20_state>();
	int data = (cassette_input(state->m_cassette) > +0.0) ? 1 : 0;

	state->m_via1->write_ca1(data);
}

/* IEC Serial Bus */

static CBM_IEC_DAISY( cbm_iec_daisy )
{
	{ M6522_0_TAG },
	{ M6522_1_TAG, DEVCB_DEVICE_LINE_MEMBER(M6522_1_TAG, via6522_device, write_cb1), },
	{ C1541_IEC(C1540_TAG) },
	{ NULL}
};

/* IEEE-488 Bus */

static IEEE488_DAISY( ieee488_daisy )
{
	{ VIC1112_IEEE488 },
	{ C2031_IEEE488(C2031_TAG) },
	{ NULL}
};

/* MOS6560 Interface */

#define VC20ADDR2MOS6560ADDR(a) (((a) > 0x8000) ? ((a) & 0x1fff) : ((a) | 0x2000))
#define MOS6560ADDR2VC20ADDR(a) (((a) > 0x2000) ? ((a) & 0x1fff) : ((a) | 0x8000))

static int vic20_dma_read_color( running_machine &machine, int offset )
{
	address_space *program = machine.device(M6502_TAG)->memory().space(AS_PROGRAM);

	return program->read_byte(0x9400 | (offset & 0x3ff));
}

static int vic20_dma_read( running_machine &machine, int offset )
{
	address_space *program = machine.device(M6502_TAG)->memory().space(AS_PROGRAM);

	return program->read_byte(MOS6560ADDR2VC20ADDR(offset));
}

static UINT8 vic20_lightx_cb( running_machine &machine )
{
	return (input_port_read_safe(machine, "LIGHTX", 0) & ~0x01);
}

static UINT8 vic20_lighty_cb( running_machine &machine )
{
	return (input_port_read_safe(machine, "LIGHTY", 0) & ~0x01);
}

static UINT8 vic20_lightbut_cb( running_machine &machine )
{
	return (((input_port_read(machine, "CTRLSEL") & 0xf0) == 0x20) && (input_port_read(machine, "JOY") & 0x40));
}

static UINT8 vic20_paddle0_cb( running_machine &machine )
{
	return input_port_read(machine, "PADDLE0");
}

static UINT8 vic20_paddle1_cb( running_machine &machine )
{
	return input_port_read(machine, "PADDLE1");
}

static const mos6560_interface vic20_6560_intf =
{
	"screen",	/* screen */
	MOS6560,
	vic20_lightx_cb, vic20_lighty_cb, vic20_lightbut_cb,	/* lightgun cb */
	vic20_paddle0_cb, vic20_paddle1_cb,		/* paddle cb */
	vic20_dma_read, vic20_dma_read_color	/* DMA */
};

static const mos6560_interface vic20_6561_intf =
{
	"screen",	/* screen */
	MOS6561,
	vic20_lightx_cb, vic20_lighty_cb, vic20_lightbut_cb,	/* lightgun cb */
	vic20_paddle0_cb, vic20_paddle1_cb,		/* paddle cb */
	vic20_dma_read, vic20_dma_read_color	/* DMA */
};


/* Machine Initialization */

static MACHINE_START( vic20 )
{
	vic20_state *state = machine.driver_data<vic20_state>();
	address_space *program = machine.device(M6502_TAG)->memory().space(AS_PROGRAM);

	/* find devices */
	state->m_via0 = machine.device<via6522_device>(M6522_0_TAG);
	state->m_via1 = machine.device<via6522_device>(M6522_1_TAG);
	state->m_iec = machine.device(IEC_TAG);
	state->m_cassette = machine.device(CASSETTE_TAG);
	state->m_cassette_timer = machine.device<timer_device>(TIMER_C1530_TAG);
	state->m_mos6560 = machine.device(M6560_TAG);

	/* set VIA clocks */
	state->m_via0->set_unscaled_clock(machine.device(M6502_TAG)->unscaled_clock());
	state->m_via1->set_unscaled_clock(machine.device(M6502_TAG)->unscaled_clock());

	/* memory expansions */
	switch (ram_get_size(machine.device(RAM_TAG)))
	{
	case 32*1024:
		program->install_ram(0x6000, 0x7fff, NULL);
		/* fallthru */
	case 24*1024:
		program->install_ram(0x4000, 0x5fff, NULL);
		/* fallthru */
	case 16*1024:
		program->install_ram(0x2000, 0x3fff, NULL);
		/* fallthru */
	case 8*1024:
		program->install_ram(0x0400, 0x0fff, NULL);
	}

	/* register for state saving */
	state->save_item(NAME(state->m_key_col));
}


/* Cartridge */

static DEVICE_IMAGE_LOAD( vic20_cart )
{
	address_space *program = image.device().machine().device(M6502_TAG)->memory().space(AS_PROGRAM);
	const char *filetype = image.filetype();
	UINT32 address = 0;
	UINT32 size;
	UINT8 *ptr = image.device().machine().region(M6502_TAG)->base();

	if (image.software_entry() == NULL)
	{
		size = image.length();

		if (!mame_stricmp(filetype, "20"))
			address = 0x2000;
		else if (!mame_stricmp(filetype, "40"))
			address = 0x4000;
		else if (!mame_stricmp(filetype, "60"))
			address = 0x6000;
		else if (!mame_stricmp(filetype, "70"))
			address = 0x7000;
		else if (!mame_stricmp(filetype, "a0"))
			address = 0xa000;
		else if (!mame_stricmp(filetype, "b0"))
			address = 0xb000;

		// special case for a 16K image containing two 8K files glued together
		if (size == 0x4000 && address != 0x4000)
		{
			image.fread(ptr + address, 0x2000);
			image.fread(ptr + 0xa000, 0x2000);

			program->install_rom(address, address + 0x1fff, ptr + address);
			program->install_rom(0xa000, 0xbfff, ptr + 0xa000);
		}
		else
		{
			image.fread(ptr + address, size);
			program->install_rom(address, (address + size) - 1, ptr + address);
		}
	}
	else
	{
		size = image.get_software_region_length("2000");
		if (size)
		{
			address = 0x2000;
			memcpy(ptr + address, image.get_software_region("2000"), size);
			program->install_rom(address, (address + size) - 1, ptr + address);
		}

		size = image.get_software_region_length("4000");
		if (size)
		{
			address = 0x4000;
			memcpy(ptr + address, image.get_software_region("4000"), size);
			program->install_rom(address, (address + size) - 1, ptr + address);
		}

		size = image.get_software_region_length("6000");
		if (size)
		{
			address = 0x6000;
			memcpy(ptr + address, image.get_software_region("6000"), size);
			program->install_rom(address, (address + size) - 1, ptr + address);
		}

		size = image.get_software_region_length("a000");
		if (size)
		{
			address = 0xa000;
			memcpy(ptr + address, image.get_software_region("a000"), size);
			program->install_rom(address, (address + size) - 1, ptr + address);
		}

		size = image.get_software_region_length("b000");
		if (size)
		{
			address = 0xb000;
			memcpy(ptr + address, image.get_software_region("b000"), size);
			program->install_rom(address, (address + size) - 1, ptr + address);
		}
	}

	return IMAGE_INIT_PASS;
}

/* Graphics Definitions and Video Emulation */

static const unsigned char mos6560_palette[] =
{
/* ripped from vice, a very excellent emulator */
/* black, white, red, cyan */
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0xf0, 0xf0,
/* purple, green, blue, yellow */
	0x60, 0x00, 0x60, 0x00, 0xa0, 0x00, 0x00, 0x00, 0xf0, 0xd0, 0xd0, 0x00,
/* orange, light orange, pink, light cyan, */
	0xc0, 0xa0, 0x00, 0xff, 0xa0, 0x00, 0xf0, 0x80, 0x80, 0x00, 0xff, 0xff,
/* light violett, light green, light blue, light yellow */
	0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0xa0, 0xff, 0xff, 0xff, 0x00
};

static PALETTE_INIT( vic20 )
{
	int i;

	for (i = 0; i < sizeof(mos6560_palette) / 3; i++)
	{
		palette_set_color_rgb(machine, i, mos6560_palette[i * 3], mos6560_palette[i * 3 + 1], mos6560_palette[i * 3 + 2]);
	}
}

static SCREEN_UPDATE( vic20 )
{
	vic20_state *state = screen->machine().driver_data<vic20_state>();
	mos6560_video_update(state->m_mos6560, bitmap, cliprect);
	return 0;
}

/* Machine Driver */

static INTERRUPT_GEN( vic20_raster_interrupt )
{
	vic20_state *state = device->machine().driver_data<vic20_state>();
	mos6560_raster_interrupt_gen(state->m_mos6560);
}

static MACHINE_CONFIG_START( vic20_common, vic20_state )

	MCFG_TIMER_ADD_PERIODIC(TIMER_C1530_TAG, cassette_tick, attotime::from_hz(44100))

	/* devices */
	MCFG_VIA6522_ADD(M6522_0_TAG, 0, vic20_via0_intf)
	MCFG_VIA6522_ADD(M6522_1_TAG, 0, vic20_via1_intf)

	MCFG_QUICKLOAD_ADD("quickload", cbm_vc20, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)
	MCFG_CASSETTE_ADD(CASSETTE_TAG, cbm_cassette_config )
	MCFG_CBM_IEC_ADD(IEC_TAG, cbm_iec_daisy)
	MCFG_C1540_ADD(C1540_TAG, IEC_TAG, 8)
/*
    MCFG_IEEE488_ADD(IEEE488_TAG, ieee488_daisy)
    MCFG_VIC1112_ADD(IEEE488_TAG)
    MCFG_C2031_ADD(C2031_TAG, IEEE488_TAG, 9)
*/
	MCFG_CARTSLOT_ADD("cart")
	MCFG_CARTSLOT_EXTENSION_LIST("20,40,60,70,a0,b0")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_INTERFACE("vic1001_cart")
	MCFG_CARTSLOT_LOAD(vic20_cart)

	/* software lists */
	MCFG_SOFTWARE_LIST_ADD("cart_list","vic1001_cart")

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("5K")
	MCFG_RAM_EXTRA_OPTIONS("8K,16K,24K,32K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( vic20_ntsc, vic20_common )

	/* basic machine hardware */
	MCFG_CPU_ADD(M6502_TAG, M6502, MOS6560_CLOCK)
	MCFG_CPU_PROGRAM_MAP(vic20_mem)
	MCFG_CPU_PERIODIC_INT(vic20_raster_interrupt, MOS656X_HRETRACERATE)

	MCFG_MACHINE_START(vic20)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(MOS6560_VRETRACERATE)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE((MOS6560_XSIZE + 7) & ~7, MOS6560_YSIZE)
	MCFG_SCREEN_VISIBLE_AREA(MOS6560_MAME_XPOS, MOS6560_MAME_XPOS + MOS6560_MAME_XSIZE - 1, MOS6560_MAME_YPOS, MOS6560_MAME_YPOS + MOS6560_MAME_YSIZE - 1)
	MCFG_SCREEN_UPDATE( vic20 )

	MCFG_PALETTE_LENGTH(16)
	MCFG_PALETTE_INIT( vic20 )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_MOS656X_ADD(M6560_TAG, vic20_6560_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( vic20_pal, vic20_common )

	/* basic machine hardware */
	MCFG_CPU_ADD(M6502_TAG, M6502, MOS6561_CLOCK)
	MCFG_CPU_PROGRAM_MAP(vic20_mem)
	MCFG_CPU_PERIODIC_INT(vic20_raster_interrupt, MOS656X_HRETRACERATE)

	MCFG_MACHINE_START(vic20)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(MOS6561_VRETRACERATE)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE((MOS6561_XSIZE + 7) & ~7, MOS6561_YSIZE)
	MCFG_SCREEN_VISIBLE_AREA(MOS6561_MAME_XPOS, MOS6561_MAME_XPOS + MOS6561_MAME_XSIZE - 1, MOS6561_MAME_YPOS, MOS6561_MAME_YPOS + MOS6561_MAME_YSIZE - 1)
	MCFG_SCREEN_UPDATE( vic20 )

	MCFG_PALETTE_LENGTH(16)
	MCFG_PALETTE_INIT( vic20 )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_MOS656X_ADD(M6560_TAG, vic20_6561_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END

/* ROMs */

ROM_START( vic1001 )
	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "901460-02", 0x8000, 0x1000, CRC(fcfd8a4b) SHA1(dae61ac03065aa2904af5c123ce821855898c555) )
	ROM_LOAD( "901486-01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "901486-02", 0xe000, 0x2000, CRC(336900d7) SHA1(c9ead45e6674d1042ca6199160e8583c23aeac22) )
ROM_END

ROM_START( vic20 )
	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "901460-03.ud7",  0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4) )
	ROM_LOAD( "901486-01.ue11", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_SYSTEM_BIOS( 0, "cbm", "Original" )
	ROMX_LOAD( "901486-06.ue12", 0xe000, 0x2000, CRC(e5e7c174) SHA1(06de7ec017a5e78bd6746d89c2ecebb646efeb19), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "jiffydos", "JiffyDOS" )
	ROMX_LOAD( "jiffydos vic-20 ntsc.ue12", 0xe000, 0x2000, CRC(683a757f) SHA1(83fb83e97b5a840311dbf7e1fe56fe828f41936d), ROM_BIOS(2) )
ROM_END

ROM_START( vic20p )
	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "901460-03.ud7",  0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4) )
	ROM_LOAD( "901486-01.ue11", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_SYSTEM_BIOS( 0, "cbm", "Original" )
	ROMX_LOAD( "901486-07.ue12", 0xe000, 0x2000, CRC(4be07cb4) SHA1(ce0137ed69f003a299f43538fa9eee27898e621e), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "jiffydos", "JiffyDOS" )
	ROMX_LOAD( "jiffydos vic-20 pal.ue12", 0xe000, 0x2000, CRC(705e7810) SHA1(5a03623a4b855531b8bffd756f701306f128be2d), ROM_BIOS(2) )
ROM_END

ROM_START( vic20s )
	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "nec22101.207",   0x8000, 0x1000, CRC(d808551d) SHA1(f403f0b0ce5922bd61bbd768bdd6f0b38e648c9f) )
	ROM_LOAD( "901486-01.ue11",	0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d) )
	ROM_LOAD( "nec22081.206",   0xe000, 0x2000, CRC(b2a60662) SHA1(cb3e2f6e661ea7f567977751846ce9ad524651a3) )
ROM_END

/* System Drivers */

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT        INIT        COMPANY                             FULLNAME                    FLAGS */
COMP( 1980, vic1001,    0,          0,      vic20_ntsc,  vic1001,    0,          "Commodore Business Machines",      "VIC-1001 (Japan)",         GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
COMP( 1981, vic20,      vic1001,    0,      vic20_ntsc,  vic20,      0,          "Commodore Business Machines",      "VIC-20 (NTSC)",            GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
COMP( 1981, vic20p,     vic1001,    0,      vic20_pal,   vic20,      0,          "Commodore Business Machines",      "VIC-20 / VC-20 (PAL)",     GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
COMP( 1981, vic20s,     vic1001,    0,      vic20_pal,   vic20s,     0,          "Commodore Business Machines",      "VIC-20 (Sweden/Finland)",  GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
