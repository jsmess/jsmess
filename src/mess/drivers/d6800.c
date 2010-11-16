/*

	The Dream 6800 is a CHIP-8 computer roughly modelled on the Cosmac VIP.
	It was decribed in Electronics Australia magazine in 4 articles starting
	in May 1979. It has 1k of ROM and 1k of RAM. The video consists of 64x32
	pixels.	The keyboard is a hexcode 4x4 matrix, plus a Function key.
	
	Function keys:
	FN 0 - Modify memory - firstly enter a 4-digit address, then 2-digit data
	                the address will increment by itself, enter the next byte.
	                FN by itself will step to the next address.

	FN 1 - Tape load

	FN 2 - Tape save

	FN 3 - Run. Enter the 4-digit go address, then it starts executing.


	Information and programs can be found at http://chip8.com/?page=78



    TODO:
	- It doesn't boot up
	- Cassette
	- CPU should freeze while screen is being drawn
	- Confirm if screen is being drawn the right way
	- Confirm keyboard configuration
*/

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "sound/beep.h"
#include "devices/cassette.h"
#include "sound/wave.h"
#include "sound/speaker.h"
#include "machine/6821pia.h"
#include "machine/rescap.h"

static UINT8 d6800_keylatch;
static UINT8 d6800_keydown;
static UINT8 d6800_screen;
static UINT8 d6800_rtc;
static running_device *d6800_cassette;
static running_device *d6800_speaker;

class d6800_state : public driver_device
{
public:
	d6800_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 dummy;
};


/* Memory Maps */

static ADDRESS_MAP_START( d6800_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_REGION("maincpu", 0x0000)
	AM_RANGE(0x8010, 0x8013) AM_MIRROR(0x3cec) AM_DEVREADWRITE("pia", pia6821_r, pia6821_w)
	AM_RANGE(0xc000, 0xc3ff) AM_MIRROR(0x3c00) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( d6800 )
	PORT_START("LINE0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')

	PORT_START("LINE1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')

	PORT_START("LINE2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')

	PORT_START("LINE3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("FN") PORT_CODE(KEYCODE_LSHIFT)
INPUT_PORTS_END

/* Video */

static VIDEO_UPDATE( d6800 )
{
	UINT8 x,y,gfx=0;
	UINT8 *RAM = memory_region(screen->machine, "maincpu");

	for (y = 0; y < 32; y++)
	{
		UINT16 *p = BITMAP_ADDR16(bitmap, y, 0);

		for (x = 0; x < 8; x++)
		{
			if (d6800_screen)
				gfx = RAM[0x100 | x | (y<<3)];

			*p++ = ( gfx & 0x80 ) ? 1 : 0;
			*p++ = ( gfx & 0x40 ) ? 1 : 0;
			*p++ = ( gfx & 0x20 ) ? 1 : 0;
			*p++ = ( gfx & 0x10 ) ? 1 : 0;
			*p++ = ( gfx & 0x08 ) ? 1 : 0;
			*p++ = ( gfx & 0x04 ) ? 1 : 0;
			*p++ = ( gfx & 0x02 ) ? 1 : 0;
			*p++ = ( gfx & 0x01 ) ? 1 : 0;
		}
	}
	return 0;
}

/* PIA6821 Interface */

static INTERRUPT_GEN( d6800_interrupt )
{
	d6800_rtc = 1;
}

static READ_LINE_DEVICE_HANDLER( d6800_rtc_pulse )
{
	UINT8 res = d6800_rtc;
	d6800_rtc = 0;
	return res;
}

static READ_LINE_DEVICE_HANDLER( d6800_keydown_r )
{
	return d6800_keydown;
}

static READ_LINE_DEVICE_HANDLER( d6800_fn_key_r )
{
	return input_port_read(device->machine, "SPECIAL");
}

static WRITE_LINE_DEVICE_HANDLER( d6800_screen_w )
{
	d6800_screen = state;
}

static READ8_DEVICE_HANDLER( d6800_cassette_r )
{
	/*
	Cassette circuit consists of a 741 op-amp, a 74121 oneshot, and a 74LS74.
	When a pulse arrives, the oneshot is set. After a preset time, it triggers
	and the 74LS74 compares this pulse to the output of the 741. Therefore it
	knows if the tone is 1200 or 2400 Hz. Input to PIA is bit 7.
	*/

	return 0;
}

static WRITE8_DEVICE_HANDLER( d6800_cassette_w )
{
	/*
	Cassette circuit consists of a 566 and a transistor. The 556 runs at 2400
	or 1200 Hz depending on the state of the transistor. This is controlled by
	bit 0 of the PIA. Bit 6 drives the speaker.
	*/

	speaker_level_w(d6800_speaker, (data & 0x40) ? 0 : 1);
}

static READ8_DEVICE_HANDLER( d6800_keyboard_r )
{
	UINT8 data = 0x0f;

	if (!BIT(d6800_keylatch, 4)) data &= input_port_read(device->machine, "LINE0");
	if (!BIT(d6800_keylatch, 5)) data &= input_port_read(device->machine, "LINE1");
	if (!BIT(d6800_keylatch, 6)) data &= input_port_read(device->machine, "LINE2");
	if (!BIT(d6800_keylatch, 7)) data &= input_port_read(device->machine, "LINE3");

	d6800_keydown = (data==0x0f) ? 0 : 1;

	return d6800_keylatch | data;
}

static WRITE8_DEVICE_HANDLER( d6800_keyboard_w )
{
	/*

        bit     description

        PA0     keyboard column 0
        PA1     keyboard column 1
        PA2     keyboard column 2
        PA3     keyboard column 3
        PA4     keyboard row 0
        PA5     keyboard row 1
        PA6     keyboard row 2
        PA7     keyboard row 3

    */


	d6800_keylatch = data & 0xf0;
}

static const pia6821_interface d6800_mc6821_intf =
{
	DEVCB_HANDLER(d6800_keyboard_r),			/* port A input */
	DEVCB_HANDLER(d6800_cassette_r),			/* port B input */
	DEVCB_LINE(d6800_keydown_r),				/* CA1 input */
	DEVCB_LINE(d6800_rtc_pulse),				/* CB1 input */
	DEVCB_LINE(d6800_fn_key_r),				/* CA2 input */
	DEVCB_NULL,						/* CB2 input */
	DEVCB_HANDLER(d6800_keyboard_w),			/* port A output */
	DEVCB_HANDLER(d6800_cassette_w),			/* port B output */
	DEVCB_NULL,						/* CA2 output */
	DEVCB_LINE(d6800_screen_w),				/* CB2 output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6800_IRQ_LINE),	/* IRQA output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6800_IRQ_LINE)		/* IRQB output */
};

/* Machine Initialization */

static MACHINE_START( d6800 )
{
	d6800_speaker = machine->device("speaker");
	d6800_cassette = machine->device("cassette");
}

static MACHINE_RESET( d6800 )
{
}

/* Machine Drivers */

static const cassette_config d6800_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};

static MACHINE_CONFIG_START( d6800, d6800_state )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",M6800, XTAL_4MHz/4)
	MDRV_CPU_PROGRAM_MAP(d6800_map)
	MDRV_CPU_VBLANK_INT("screen", d6800_interrupt)

	MDRV_MACHINE_START(d6800)
	MDRV_MACHINE_RESET(d6800)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64, 32)
	MDRV_SCREEN_VISIBLE_AREA(0, 63, 0, 31)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(d6800)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* devices */
	MDRV_PIA6821_ADD("pia", d6800_mc6821_intf)
	MDRV_CASSETTE_ADD("cassette", d6800_cassette_config)
MACHINE_CONFIG_END

/* ROMs */

ROM_START( d6800 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "d6800.bin", 0xc000, 0x0400, CRC(3f97ca2e) SHA1(60f26e57a058262b30befceceab4363a5d65d877) )
ROM_END

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        COMPANY                             FULLNAME                FLAGS */
COMP( 1979, d6800,	0,	0,	d6800,	    d6800,	0,	"Electronics Australia",	"Dream 6800",	GAME_NOT_WORKING | GAME_NO_SOUND )
