/*
    Amiga 1200 / CD32

    Preliminary MAME driver by Mariusz Wojcieszek
    CD-ROM controller by Ernesto Corvi
    Borrowed by incog for MESS


    Several of the games have Audio tracks, therefore the CRC / SHA1 information you get when
    reading your own CDs may not match confirmed dumps.  There is currently no 100% accurate
    way to rip the audio data with full sub-track and offset information.

    CD32 Hardware Specs (from Wikipedia, http://en.wikipedia.org/wiki/Amiga_CD32)
     * Main Processor: Motorola 68EC020 at 14.3 MHz
     * System Memory: 2 MB Chip RAM
     * 1 MB ROM with Kickstart ROM 3.1 and integrated cdfs.filesystem
     * 1KB of FlashROM for game saves
     * Graphics/Chipset: AGA Chipset
     * Akiko chip, which handles CD-ROM and can do Chunky to Planar conversion
     * Proprietary (MKE) CD-ROM drive at 2x speed
     * Expansion socket for MPEG cartridge, as well as 3rd party devices such as the SX-1 and SX32 expansion packs.
     * 4 8-bit audio channels (2 for left, 2 for right)
     * Gamepad, Serial port, 2 Gameports, Interfaces for keyboard

    2009-05 Fabio Priuli:
    Amiga 1200 support is just sketched (I basically took cd32 and removed Akiko). I connected
    the floppy drive in the same way as in amiga.c but it seems to be not working, since I
    tried to load WB3.1 with no success. However, this problem may be due to anything: maybe
    the floppy code must be connected elsewhere, or the .adf image is broken, or I made some
    stupid mistake in the CIA interfaces.
    Later, it could be wise to re-factor this source and merge the non-AGA code with
    mess/drivers/amiga.c
*/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "machine/6526cia.h"

#include "machine/amigafdc.h"
#include "machine/amigakbd.h"

#include "sound/cdda.h"
#include "includes/amiga.h"
#include "includes/cubocd32.h"
#include "devices/chd_cd.h"


#define A1200PAL_XTAL_X1  XTAL_28_37516MHz
#define A1200PAL_XTAL_X2  XTAL_4_433619MHz
#define CD32PAL_XTAL_X1   XTAL_28_37516MHz
#define CD32PAL_XTAL_X2   XTAL_4_433619MHz
#define CD32PAL_XTAL_X3   XTAL_16_9344MHz


static void handle_cd32_joystick_cia(UINT8 pra, UINT8 dra);

static WRITE32_HANDLER( aga_overlay_w )
{
	if (ACCESSING_BITS_16_23)
	{
		data = (data >> 16) & 1;

		/* switch banks as appropriate */
		memory_set_bank(space->machine, "bank1", data & 1);

		/* swap the write handlers between ROM and bank 1 based on the bit */
		if ((data & 1) == 0)
			/* overlay disabled, map RAM on 0x000000 */
			memory_install_write_bank(space, 0x000000, 0x1fffff, 0, 0, "bank1");
		else
			/* overlay enabled, map Amiga system ROM on 0x000000 */
			memory_unmap_write(space, 0x000000, 0x1fffff, 0, 0);
	}
}

/*************************************
 *
 *  CIA-A port A access:
 *
 *  PA7 = game port 1, pin 6 (fire)
 *  PA6 = game port 0, pin 6 (fire)
 *  PA5 = /RDY (disk ready)
 *  PA4 = /TK0 (disk track 00)
 *  PA3 = /WPRO (disk write protect)
 *  PA2 = /CHNG (disk change)
 *  PA1 = /LED (LED, 0=bright / audio filter control)
 *  PA0 = MUTE
 *
 *************************************/

static WRITE8_DEVICE_HANDLER( cd32_cia_0_porta_w )
{
	/* bit 1 = cd audio mute */
	const device_config *cdda = devtag_get_device(device->machine, "cdda");

	if (cdda != NULL)
		sound_set_output_gain(cdda, 0, BIT(data, 0) ? 0.0 : 1.0 );

	/* bit 2 = Power Led on Amiga */
	set_led_status(device->machine, 0, !BIT(data, 1));

	handle_cd32_joystick_cia(data, cia_r(device, 2));
}

/*************************************
 *
 *  CIA-A port B access:
 *
 *  PB7 = parallel data 7
 *  PB6 = parallel data 6
 *  PB5 = parallel data 5
 *  PB4 = parallel data 4
 *  PB3 = parallel data 3
 *  PB2 = parallel data 2
 *  PB1 = parallel data 1
 *  PB0 = parallel data 0
 *
 *************************************/

static READ8_DEVICE_HANDLER( cd32_cia_0_portb_r )
{
	/* parallel port */
	logerror("%s:CIA0_portb_r\n", cpuexec_describe_context(device->machine));
	return 0xff;
}

static WRITE8_DEVICE_HANDLER( cd32_cia_0_portb_w )
{
	/* parallel port */
	logerror("%s:CIA0_portb_w(%02x)\n", cpuexec_describe_context(device->machine), data);
}

static ADDRESS_MAP_START( a1200_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x1fffff) AM_RAMBANK("bank1") AM_BASE(&amiga_chip_ram32) AM_SIZE(&amiga_chip_ram_size)
	AM_RANGE(0xbfa000, 0xbfa003) AM_WRITE(aga_overlay_w)
	AM_RANGE(0xbfd000, 0xbfefff) AM_READWRITE16(amiga_cia_r, amiga_cia_w, 0xffffffff)
	AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE16(amiga_custom_r, amiga_custom_w, 0xffffffff) AM_BASE((UINT32**)&amiga_custom_regs)
	AM_RANGE(0xe80000, 0xe8ffff) AM_READWRITE16(amiga_autoconfig_r, amiga_autoconfig_w, 0xffffffff)
	AM_RANGE(0xf80000, 0xffffff) AM_ROM AM_REGION("user1", 0)	/* Kickstart */
ADDRESS_MAP_END

static ADDRESS_MAP_START( cd32_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x1fffff) AM_RAMBANK("bank1") AM_BASE(&amiga_chip_ram32) AM_SIZE(&amiga_chip_ram_size)
	AM_RANGE(0xb80000, 0xb8003f) AM_READWRITE(amiga_akiko32_r, amiga_akiko32_w)
	AM_RANGE(0xbfa000, 0xbfa003) AM_WRITE(aga_overlay_w)
	AM_RANGE(0xbfd000, 0xbfefff) AM_READWRITE16(amiga_cia_r, amiga_cia_w, 0xffffffff)
	AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE16(amiga_custom_r, amiga_custom_w, 0xffffffff) AM_BASE((UINT32**)&amiga_custom_regs)
	AM_RANGE(0xe00000, 0xe7ffff) AM_ROM AM_REGION("user1", 0x80000)	/* CD32 Extended ROM */
	AM_RANGE(0xa00000, 0xf7ffff) AM_NOP
	AM_RANGE(0xf80000, 0xffffff) AM_ROM AM_REGION("user1", 0x0)		/* Kickstart */
ADDRESS_MAP_END

//int cd32_input_port_val = 0;
//int cd32_input_select = 0;
static UINT16 potgo_value = 0;
static int cd32_shifter[2];

static void cd32_potgo_w(running_machine *machine, UINT16 data)
{
	int i;

	potgo_value = potgo_value & 0x5500;
	potgo_value |= data & 0xaa00;

    for (i = 0; i < 8; i += 2)
	{
		UINT16 dir = 0x0200 << i;
		if (data & dir)
		{
			UINT16 d = 0x0100 << i;
			potgo_value &= ~d;
			potgo_value |= data & d;
		}
    }
    for (i = 0; i < 2; i++)
	{
	    UINT16 p5dir = 0x0200 << (i * 4); /* output enable P5 */
	    UINT16 p5dat = 0x0100 << (i * 4); /* data P5 */
	    if ((potgo_value & p5dir) && (potgo_value & p5dat))
		cd32_shifter[i] = 8;
    }

}

static void handle_cd32_joystick_cia(UINT8 pra, UINT8 dra)
{
    static int oldstate[2];
    int i;

    for (i = 0; i < 2; i++)
	{
		UINT8 but = 0x40 << i;
		UINT16 p5dir = 0x0200 << (i * 4); /* output enable P5 */
		UINT16 p5dat = 0x0100 << (i * 4); /* data P5 */
		if (!(potgo_value & p5dir) || !(potgo_value & p5dat))
		{
			if ((dra & but) && (pra & but) != oldstate[i])
			{
				if (!(pra & but))
				{
					cd32_shifter[i]--;
					if (cd32_shifter[i] < 0)
						cd32_shifter[i] = 0;
				}
			}
		}
		oldstate[i] = pra & but;
    }
}

static UINT16 handle_joystick_potgor (running_machine *machine, UINT16 potgor)
{
    int i;

    for (i = 0; i < 2; i++)
	{
		UINT16 p9dir = 0x0800 << (i * 4); /* output enable P9 */
		UINT16 p9dat = 0x0400 << (i * 4); /* data P9 */
		UINT16 p5dir = 0x0200 << (i * 4); /* output enable P5 */
		UINT16 p5dat = 0x0100 << (i * 4); /* data P5 */

	    /* p5 is floating in input-mode */
	    potgor &= ~p5dat;
	    potgor |= potgo_value & p5dat;
	    if (!(potgo_value & p9dir))
			potgor |= p9dat;
	    /* P5 output and 1 -> shift register is kept reset (Blue button) */
	    if ((potgo_value & p5dir) && (potgo_value & p5dat))
			cd32_shifter[i] = 8;
	    /* shift at 1 == return one, >1 = return button states */
	    if (cd32_shifter[i] == 0)
			potgor &= ~p9dat; /* shift at zero == return zero */
		if (i == 0)
			if (cd32_shifter[i] >= 2 && (input_port_read(machine, "IN0") & (1 << (cd32_shifter[i] - 2))))
				potgor &= ~p9dat;
    }
    return potgor;
}

static CUSTOM_INPUT(cd32_input)
{
	return handle_joystick_potgor(field->port->machine, potgo_value) >> 10;
}


static INPUT_PORTS_START( a1200 )
	PORT_START("CIA0PORTA")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START("CIA0PORTB")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOY0DAT")
	PORT_BIT( 0x0303, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(amiga_joystick_convert, "P1JOY")
	PORT_BIT( 0xfcfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("JOY1DAT")
	PORT_BIT( 0x0303, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(amiga_joystick_convert, "P2JOY")
	PORT_BIT( 0xfcfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("POTGO")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0xaaff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("P1JOY")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)

	PORT_START("P2JOY")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)

	PORT_START("P0MOUSEX")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1)

	PORT_START("P0MOUSEY")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1)

	PORT_START("P1MOUSEX")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(2)

	PORT_START("P1MOUSEY")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(2)

	PORT_INCLUDE( amiga_us_keyboard )
INPUT_PORTS_END

static INPUT_PORTS_START( cd32 )
	PORT_START("CIA0PORTA")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL

	PORT_START("CIA0PORTB")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START("JOY0DAT")
	PORT_BIT( 0x0303, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(amiga_joystick_convert, "P1JOY")
	PORT_BIT( 0xfcfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("JOY1DAT")
	PORT_BIT( 0x0303, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(amiga_joystick_convert, "P2JOY")
	PORT_BIT( 0xfcfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("POTGO")
	PORT_BIT( 0x4400, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM(cd32_input, 0)
	PORT_BIT( 0xaaff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("P1JOY")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)

	PORT_START("P2JOY")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)

	PORT_START("COINS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START("DIPSW1")
	PORT_DIPNAME( 0x01, 0x01, "DSW1 1" )
	PORT_DIPSETTING(    0x01, "Reset" )
	PORT_DIPSETTING(    0x00, "Set" )
	PORT_DIPNAME( 0x02, 0x02, "DSW1 2" )
	PORT_DIPSETTING(    0x02, "Reset" )
	PORT_DIPSETTING(    0x00, "Set" )
	PORT_DIPNAME( 0x04, 0x04, "DSW1 3" )
	PORT_DIPSETTING(    0x04, "Reset" )
	PORT_DIPSETTING(    0x00, "Set" )
	PORT_DIPNAME( 0x08, 0x08, "DSW1 4" )
	PORT_DIPSETTING(    0x08, "Reset" )
	PORT_DIPSETTING(    0x00, "Set" )
	PORT_DIPNAME( 0x10, 0x10, "DSW1 5" )
	PORT_DIPSETTING(    0x10, "Reset" )
	PORT_DIPSETTING(    0x00, "Set" )
	PORT_DIPNAME( 0x20, 0x20, "DSW1 6" )
	PORT_DIPSETTING(    0x20, "Reset" )
	PORT_DIPSETTING(    0x00, "Set" )
	PORT_SERVICE( 0x40, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x80, 0x80, "DSW1 8" )
	PORT_DIPSETTING(    0x80, "Reset" )
	PORT_DIPSETTING(    0x00, "Set" )

	PORT_START("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON5 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON6 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON7 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_UNUSED )

INPUT_PORTS_END

/*************************************
 *
 *  Sound definitions
 *
 *************************************/

static READ8_DEVICE_HANDLER( a1200_cia_0_portA_r )
{
	UINT8 ret = input_port_read(device->machine, "CIA0PORTA") & 0xc0;	/* Gameport 1 and 0 buttons */
	ret |= amiga_fdc_status_r();
	return ret;
}


static const cia6526_interface a1200_cia_0_intf =
{
	DEVCB_LINE(amiga_cia_0_irq),									/* irq_func */
	DEVCB_NULL,	/* pc_func */
	0,													/* tod_clock */
	{
		{ DEVCB_HANDLER(a1200_cia_0_portA_r), DEVCB_HANDLER(cd32_cia_0_porta_w) },		/* port A */
		{ DEVCB_HANDLER(cd32_cia_0_portb_r), DEVCB_HANDLER(cd32_cia_0_portb_w) }		/* port B */
	}
};

static const cia6526_interface cd32_cia_0_intf =
{
	DEVCB_LINE(amiga_cia_0_irq),									/* irq_func */
	DEVCB_NULL,	/* pc_func */
	0,													/* tod_clock */
	{
		{ DEVCB_INPUT_PORT("CIA0PORTA"), DEVCB_HANDLER(cd32_cia_0_porta_w) },		/* port A */
		{ DEVCB_HANDLER(cd32_cia_0_portb_r), DEVCB_HANDLER(cd32_cia_0_portb_w) }		/* port B */
	}
};

static const cia6526_interface a1200_cia_1_intf =
{
	DEVCB_LINE(amiga_cia_1_irq),									/* irq_func */
	DEVCB_NULL,	/* pc_func */
	0,													/* tod_clock */
	{
		{ DEVCB_NULL, DEVCB_NULL },									/* port A */
		{ DEVCB_NULL, DEVCB_HANDLER(amiga_fdc_control_w) }			/* port B */
	}
};

static const cia6526_interface cd32_cia_1_intf =
{
	DEVCB_LINE(amiga_cia_1_irq),									/* irq_func */
	DEVCB_NULL,	/* pc_func */
	0,													/* tod_clock */
	{
		{ DEVCB_NULL, DEVCB_NULL },									/* port A */
		{ DEVCB_NULL, DEVCB_NULL }									/* port B */
	}
};

static MACHINE_DRIVER_START( a1200n )

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M68EC020, AMIGA_68EC020_NTSC_CLOCK) /* 14.3 Mhz */
	MDRV_CPU_PROGRAM_MAP(a1200_map)

	MDRV_MACHINE_RESET(amiga)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(59.997)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(512*2, 312)
	MDRV_SCREEN_VISIBLE_AREA((129-8-8)*2, (449+8-1+8)*2, 44-8, 300+8-1)

	MDRV_VIDEO_START(amiga_aga)
	MDRV_VIDEO_UPDATE(amiga_aga)

	/* sound hardware */
    MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

    MDRV_SOUND_ADD("amiga", AMIGA, XTAL_28_63636MHz/8)
    MDRV_SOUND_ROUTE(0, "lspeaker", 0.25)
    MDRV_SOUND_ROUTE(1, "rspeaker", 0.25)
    MDRV_SOUND_ROUTE(2, "rspeaker", 0.25)
    MDRV_SOUND_ROUTE(3, "lspeaker", 0.25)

	/* cia */
	MDRV_CIA8520_ADD("cia_0", AMIGA_68EC020_NTSC_CLOCK / 10, a1200_cia_0_intf)
	MDRV_CIA8520_ADD("cia_1", AMIGA_68EC020_NTSC_CLOCK / 10, a1200_cia_1_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( a1200p )
	MDRV_IMPORT_FROM(a1200n)

	/* adjust for PAL specs */
	MDRV_CPU_REPLACE("maincpu", M68EC020, A1200PAL_XTAL_X1/2) /* 14.18758 MHz */

	/* video hardware */
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE(50)

	/* sound hardware */
	MDRV_SOUND_MODIFY("amiga")
	MDRV_SOUND_CLOCK(A1200PAL_XTAL_X1/8) /* 3.546895 MHz */

	/* cia */
	MDRV_DEVICE_MODIFY("cia_0")
	MDRV_DEVICE_CLOCK(A1200PAL_XTAL_X1/20)
	MDRV_DEVICE_MODIFY("cia_1")
	MDRV_DEVICE_CLOCK(A1200PAL_XTAL_X1/20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cd32 )

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M68EC020, AMIGA_68EC020_NTSC_CLOCK) /* 14.3 Mhz */
	MDRV_CPU_PROGRAM_MAP(cd32_map)

	MDRV_MACHINE_RESET(amiga)
	MDRV_NVRAM_HANDLER(cd32)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(59.997)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(512*2, 312)
	MDRV_SCREEN_VISIBLE_AREA((129-8-8)*2, (449+8-1+8)*2, 44-8, 300+8-1)

	MDRV_VIDEO_START(amiga_aga)
	MDRV_VIDEO_UPDATE(amiga_aga)

	/* sound hardware */
    MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

    MDRV_SOUND_ADD("amiga", AMIGA, XTAL_28_63636MHz/8)
    MDRV_SOUND_ROUTE(0, "lspeaker", 0.25)
    MDRV_SOUND_ROUTE(1, "rspeaker", 0.25)
    MDRV_SOUND_ROUTE(2, "rspeaker", 0.25)
    MDRV_SOUND_ROUTE(3, "lspeaker", 0.25)

    MDRV_SOUND_ADD( "cdda", CDDA, 0 )
	MDRV_SOUND_ROUTE( 0, "lspeaker", 0.50 )
	MDRV_SOUND_ROUTE( 1, "rspeaker", 0.50 )

	/* cia */
	MDRV_CIA8520_ADD("cia_0", AMIGA_68EC020_PAL_CLOCK / 10, cd32_cia_0_intf)
	MDRV_CIA8520_ADD("cia_1", AMIGA_68EC020_PAL_CLOCK / 10, cd32_cia_1_intf)
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( a1200n )
	ROM_REGION32_BE(0x080000, "user1", 0)
	ROM_DEFAULT_BIOS("kick31")
	ROM_SYSTEM_BIOS(0, "kick30", "Kickstart 3.0 (39.106)")
	ROMX_LOAD("391523-01.u6a", 0x000000, 0x040000, CRC(c742a412) SHA1(999eb81c65dfd07a71ee19315d99c7eb858ab186), ROM_GROUPWORD | ROM_REVERSE | ROM_SKIP(2) | ROM_BIOS(1))
	ROMX_LOAD("391524-01.u6b", 0x000002, 0x040000, CRC(d55c6ec6) SHA1(3341108d3a402882b5ef9d3b242cbf3c8ab1a3e9), ROM_GROUPWORD | ROM_REVERSE | ROM_SKIP(2) | ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "kick31", "Kickstart 3.1 (40.068)")
	ROMX_LOAD("391773-01.u6a", 0x000000, 0x040000, CRC(08dbf275) SHA1(b8800f5f909298109ea69690b1b8523fa22ddb37), ROM_GROUPWORD | ROM_REVERSE | ROM_SKIP(2) | ROM_BIOS(2))	// ROM_LOAD32_WORD_SWAP!
	ROMX_LOAD("391774-01.u6b", 0x000002, 0x040000, CRC(16c07bf8) SHA1(90e331be1970b0e53f53a9b0390b51b59b3869c2), ROM_GROUPWORD | ROM_REVERSE | ROM_SKIP(2) | ROM_BIOS(2))
ROM_END

#define rom_a1200p    rom_a1200n

ROM_START( cd32 )
	ROM_REGION32_BE(0x100000, "user1",0)
	ROM_LOAD("391640-03.u6a", 0x000000, 0x100000, CRC(d3837ae4) SHA1(06807db3181637455f4d46582d9972afec8956d9))
ROM_END


/***************************************************************************************************/

// these come directly from amiga.c

static UINT16 a1200_read_dskbytr(running_machine *machine)
{
	return amiga_fdc_get_byte();
}

static void a1200_write_dsklen(running_machine *machine, UINT16 data)
{
	if ( data & 0x8000 )
	{
		if ( CUSTOM_REG(REG_DSKLEN) & 0x8000 )
			amiga_fdc_setup_dma(machine);
	}
}


static DRIVER_INIT( a1200 )
{
	static const amiga_machine_interface cubocd32_intf =
	{
		AGA_CHIP_RAM_MASK,
		NULL, NULL,			/* joy0dat_r & joy1dat_r */
		cd32_potgo_w,		/* potgo_w */
		a1200_read_dskbytr, a1200_write_dsklen, /* dskbytr_r & dsklen_w */
		NULL,				/* serdat_w */
		NULL,				/* scanline0_callback */
		NULL,				/* reset_callback */
		NULL,				/* nmi_callback */
		FLAGS_AGA_CHIPSET	/* flags */
	};

	/* configure our Amiga setup */
	amiga_machine_config(machine, &cubocd32_intf);

	/* set up memory */
	memory_configure_bank(machine, "bank1", 0, 1, amiga_chip_ram32, 0);
	memory_configure_bank(machine, "bank1", 1, 1, memory_region(machine, "user1"), 0);

	/* initialize keyboard */
	amigakbd_init(machine);
}

static DRIVER_INIT( cd32 )
{
	static const amiga_machine_interface cubocd32_intf =
	{
		AGA_CHIP_RAM_MASK,
		NULL, NULL, 		/* joy0dat_r & joy1dat_r */
		cd32_potgo_w,		/* potgo_w */
		NULL, NULL,			/* dskbytr_r & dsklen_w */
		NULL,				/* serdat_w */
		NULL, 				/* scanline0_callback */
		NULL,				/* reset_callback */
		NULL,				/* nmi_callback */
		FLAGS_AGA_CHIPSET	/* flags */
	};

	/* configure our Amiga setup */
	amiga_machine_config(machine, &cubocd32_intf);

	/* set up memory */
	memory_configure_bank(machine, "bank1", 0, 1, amiga_chip_ram32, 0);
	memory_configure_bank(machine, "bank1", 1, 1, memory_region(machine, "user1"), 0);

	/* intialize akiko */
	amiga_akiko_init(machine);
}

/***************************************************************************
  System config
***************************************************************************/

static SYSTEM_CONFIG_START(a1200)
	CONFIG_DEVICE(amiga_floppy_getinfo)
SYSTEM_CONFIG_END


/***************************************************************************************************/

/*    YEAR  NAME     PARENT   COMPAT  MACHINE INPUT   INIT    CONFIG  COMPANY       FULLNAME */
COMP( 1992, a1200n,  0,       0,      a1200n, a1200,  a1200,  a1200,  "Commodore",  "Amiga 1200 (NTSC)" , GAME_NOT_WORKING )
COMP( 1992, a1200p,  a1200n,  0,      a1200p, a1200,  a1200,  a1200,  "Commodore",  "Amiga 1200 (PAL)" , GAME_NOT_WORKING )
CONS( 1993, cd32,    0,       0,      cd32,   cd32,   cd32,   0,      "Commodore",  "Amiga CD32 (NTSC)" , GAME_NOT_WORKING )
