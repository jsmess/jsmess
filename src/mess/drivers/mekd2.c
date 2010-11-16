/******************************************************************************
    Motorola Evaluation Kit 6800 D2
    MEK6800D2

    system driver

    Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

    memory map

    range       short   description
    0000-00ff   RAM     256 bytes RAM
    0100-01ff   RAM     optional 256 bytes RAM
    6000-63ff   PROM    optional PROM
    or
    6000-67ff   ROM     optional ROM
    8004-8007   PIA     expansion port
    8008-8009   ACIA    cassette interface
    8020-8023   PIA     keyboard interface
    a000-a07f   RAM     128 bytes RAM (JBUG scratch)
    c000-c3ff   PROM    optional PROM
    or
    c000-c7ff   ROM     optional ROM
    e000-e3ff   ROM     JBUG monitor program
    e400-ffff   -/-     mirrors of monitor rom


    TODO
    - Cassette (it is extremely complex)
    - Keyboard (it should work but doesn't)
    - Replace graphical displays with modern artwork

******************************************************************************/

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "sound/dac.h"
#include "machine/6821pia.h"
#include "machine/6850acia.h"
#include "devices/cartslot.h"
#include "includes/mekd2.h"

#define XTAL_MEKD2 1228800

static UINT8 mekd2_segment;
static UINT8 mekd2_digit;
static UINT8 mekd2_keydata;

/***********************************************************

    Trace hardware (what happens when N is pressed)

************************************************************/

static TIMER_CALLBACK( mekd2_trace )
{
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, ASSERT_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( mekd2_nmi_w )
{
	if (state)
		cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_NMI, CLEAR_LINE);
	else
		timer_set(device->machine, ATTOTIME_IN_USEC(18), NULL, 0, mekd2_trace);
}

/***********************************************************

    Keyboard

************************************************************/

static READ_LINE_DEVICE_HANDLER( mekd2_key40_r )
{
	return (mekd2_keydata & 0x40) ? 1 : 0;
}

static READ8_DEVICE_HANDLER( mekd2_key_r )
{
	char kbdrow[4];
	UINT8 i;
	mekd2_keydata = 0xff;

	for (i = 0; i < 6; i++)
		if (BIT(mekd2_digit, i))
		{
			sprintf(kbdrow,"X%d",i);
			mekd2_keydata &= input_port_read(device->machine, kbdrow);
		}

	if (mekd2_digit < 0x40)
		i = (mekd2_keydata & 1) ? 0x80 : 0;
	else
	if (mekd2_digit < 0x80)
		i = (mekd2_keydata & 2) ? 0x80 : 0;
	else
	if (mekd2_digit < 0xc0)
		i = (mekd2_keydata & 4) ? 0x80 : 0;
	else
		i = (mekd2_keydata & 8) ? 0x80 : 0;

	return i;
}

/***********************************************************

    LED display

************************************************************/

static WRITE8_DEVICE_HANDLER( mekd2_segment_w )
{
	mekd2_segment = ~data;
}

static WRITE8_DEVICE_HANDLER( mekd2_digit_w )
{
	UINT8 i;
	mekd2_state *state = device->machine->driver_data<mekd2_state>();
	UINT8 *videoram = state->videoram;

	for (i = 0; i < 6; i++)
		if (BIT(data, i))
		{
			videoram[10 - i * 2] = mekd2_segment;
			videoram[11 - i * 2] = 14;
		}

	mekd2_digit = data;
}

/***********************************************************

    Address Map

************************************************************/

static ADDRESS_MAP_START( mekd2_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x00ff) AM_RAM // user ram
	AM_RANGE(0x8004, 0x8007) AM_DEVREADWRITE("pia_u", pia6821_r, pia6821_w)
	AM_RANGE(0x8008, 0x8008) AM_DEVREADWRITE("acia", acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0x8009, 0x8009) AM_DEVREADWRITE("acia", acia6850_data_r, acia6850_data_w)
	AM_RANGE(0x8020, 0x8023) AM_DEVREADWRITE("pia_s", pia6821_r, pia6821_w)
	AM_RANGE(0xa000, 0xa07f) AM_RAM // system ram
	AM_RANGE(0xe000, 0xe3ff) AM_ROM AM_MIRROR(0x1c00)	/* JBUG ROM */
ADDRESS_MAP_END

/***********************************************************

    Keys

************************************************************/

/*

Enter the 4 digit address then the command key:
 
  - M : Examine and Change Memory
  - E : Escape (abort) operation (H key in our emulation)
  - R : Examine Registers
  - G : Begin execution at specified address
  - P : Punch data from memory to magnetic tape
  - L : Load memory from magnetic tape
  - N : Trace one instruction
  - V : Set (and remove) breakpoints

The keys are laid out as:

  P L N V

  7 8 9 A  M
  4 5 6 B  E
  1 2 3 C  R
  0 F E D  G

 */
static INPUT_PORTS_START( mekd2 )
	PORT_START("X0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E (hex)") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)

	PORT_START("X1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)

	PORT_START("X2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)

	PORT_START("X3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)

	PORT_START("X4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)

	PORT_START("X5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E (escape)") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
INPUT_PORTS_END

/***********************************************************

    GFX

************************************************************/

static const gfx_layout led_layout =
{
	18, 24, 	/* 16 x 24 LED 7segment displays */
	128,		/* 128 codes */
	1,			/* 1 bit per pixel */
	{ 0 },		/* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8, 9,10,11,12,13,14,15,
	 16,17 },
	{ 0*24, 1*24, 2*24, 3*24,
	  4*24, 5*24, 6*24, 7*24,
	  8*24, 9*24,10*24,11*24,
	 12*24,13*24,14*24,15*24,
	 16*24,17*24,18*24,19*24,
	 20*24,21*24,22*24,23*24,
	 24*24,25*24,26*24,27*24,
	 28*24,29*24,30*24,31*24 },
	24 * 24,	/* every LED code takes 32 times 18 (aligned 24) bit words */
};

static const gfx_layout key_layout =
{
	24, 18, 	/* 24 * 18 keyboard icons */
	24, 		/* 24  codes */
	2,			/* 2 bit per pixel */
	{ 0, 1 },	/* two bitplanes */
	{ 0*2, 1*2, 2*2, 3*2, 4*2, 5*2, 6*2, 7*2,
	  8*2, 9*2,10*2,11*2,12*2,13*2,14*2,15*2,
	 16*2,17*2,18*2,19*2,20*2,21*2,22*2,23*2 },
	{ 0*24*2, 1*24*2, 2*24*2, 3*24*2, 4*24*2, 5*24*2, 6*24*2, 7*24*2,
	  8*24*2, 9*24*2,10*24*2,11*24*2,12*24*2,13*24*2,14*24*2,15*24*2,
	 16*24*2,17*24*2 },
	18 * 24 * 2,	/* every icon takes 18 rows of 24 * 2 bits */
};

static GFXDECODE_START( mekd2 )
	GFXDECODE_ENTRY( "gfx1", 0, led_layout, 0, 16 )
	GFXDECODE_ENTRY( "gfx2", 0, key_layout, 16*2, 2 )
GFXDECODE_END

/***********************************************************

    Interfaces

************************************************************/

static const pia6821_interface mekd2_s_mc6821_intf =
{
	DEVCB_HANDLER(mekd2_key_r),				/* port A input */
	DEVCB_NULL,						/* port B input */
	DEVCB_NULL,						/* CA1 input */
	DEVCB_LINE(mekd2_key40_r),				/* CB1 input */
	DEVCB_NULL,						/* CA2 input */
	DEVCB_NULL,						/* CB2 input */
	DEVCB_HANDLER(mekd2_segment_w),				/* port A output */
	DEVCB_HANDLER(mekd2_digit_w),				/* port B output */
	DEVCB_LINE(mekd2_nmi_w),				/* CA2 output */
	DEVCB_NULL,						/* CB2 output */
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_NMI),	/* IRQA output */
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_NMI)		/* IRQB output */
};

static const pia6821_interface mekd2_u_mc6821_intf =
{
	DEVCB_NULL,						/* port A input */
	DEVCB_NULL,						/* port B input */
	DEVCB_NULL,						/* CA1 input */
	DEVCB_NULL,						/* CB1 input */
	DEVCB_NULL,						/* CA2 input */
	DEVCB_NULL,						/* CB2 input */
	DEVCB_NULL,						/* port A output */
	DEVCB_NULL,						/* port B output */
	DEVCB_NULL,						/* CA2 output */
	DEVCB_NULL,						/* CB2 output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6800_IRQ_LINE),	/* IRQA output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6800_IRQ_LINE)		/* IRQB output */
};

static ACIA6850_INTERFACE( mekd2_acia_intf )
{
	XTAL_MEKD2 / 256,	//connected to cassette circuit	/* tx clock 4800Hz */
	XTAL_MEKD2 / 256,	//connected to cassette circuit	/* rx clock varies, controlled by cassette circuit */
	DEVCB_NULL,//LINE(cass),//connected to cassette circuit	/* in rxd func */
	DEVCB_NULL,//LINE(cass),//connected to cassette circuit	/* out txd func */
	DEVCB_NULL,						/* in cts func */
	DEVCB_NULL,		//connected to cassette circuit	/* out rts func */
	DEVCB_NULL,						/* in dcd func */
	DEVCB_NULL						/* out irq func */
};

/***********************************************************

    Machine

************************************************************/

static MACHINE_CONFIG_START( mekd2, mekd2_state )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6800, XTAL_MEKD2 / 2)        /* 614.4 kHz */
	MDRV_CPU_PROGRAM_MAP(mekd2_mem)
	MDRV_QUANTUM_TIME(HZ(60))

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(600, 768)
	MDRV_SCREEN_VISIBLE_AREA(0, 600-1, 0, 768-1)
	MDRV_GFXDECODE( mekd2 )
	MDRV_PALETTE_LENGTH(40)
	MDRV_PALETTE_INIT( mekd2 )

	MDRV_VIDEO_START( mekd2 )
	MDRV_VIDEO_UPDATE( mekd2 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("d2")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(mekd2_cart)

	/* Devices */
	MDRV_PIA6821_ADD("pia_s", mekd2_s_mc6821_intf)
	MDRV_PIA6821_ADD("pia_u", mekd2_u_mc6821_intf)
	MDRV_ACIA6850_ADD("acia", mekd2_acia_intf)
MACHINE_CONFIG_END

/***********************************************************

    ROMS

************************************************************/

ROM_START(mekd2)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("jbug.rom", 0xe000, 0x0400, CRC(5ed08792) SHA1(b06e74652a4c4e67c4a12ddc191ffb8c07f3332e) )
	ROM_REGION(128 * 24 * 3,"gfx1",ROMREGION_ERASEFF)
		/* space filled with 7segement graphics by mekd2_init_driver */
	ROM_REGION( 24 * 18 * 3 * 2,"gfx2",ROMREGION_ERASEFF)
		/* space filled with key icons by mekd2_init_driver */
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME    PARENT  COMPAT  MACHINE   INPUT     INIT    COMPANY     FULLNAME */
CONS( 1977, mekd2,	0,		0,		mekd2,	  mekd2,	mekd2,	"Motorola",	"MEK6800D2" , GAME_NOT_WORKING )
