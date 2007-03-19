/*
	Experimental Tomy Tutor driver

	This computer is known as Tomy Tutor in US, and as Grandstand Tutor in UK.
	It was initially released in Japan in 1982 or 1983 under the name of Pyuuta
	(Pi-yu-u-ta, with a Kanji for the "ta").  The Japanese versions are
	different from the English-language versions, as they have different ROMs
	with Japanese messages and support for the katakana syllabus.  There are at
	least 4 versions:
	* original Pyuuta (1982 or 1983) with title screens in Japanese but no
	  Basic
	* Pyuuta Jr. (1983?) which is a console with a simplified keyboard
	* Tomy/Grandstand Tutor (circa October 1983?) with title screens in English
	  and integrated Basic
	* Pyuuta Mk. 2 (1984?) with a better-looking keyboard and integrated Basic

	The Tomy Tutor features a TMS9995 CPU @10.7MHz (which includes a
	timer/counter and 256 bytes of 16-bit RAM), 48kb of ROM (32kb on early
	models that did not have the BASIC interpreter), a tms9918a/9929a VDP (or
	equivalent?) with 16kb of VRAM, and a sn76496 sound generator (or
	equivalent?).  There is a tape interface, a 56-key keyboard, an interface
	for two joysticks, a cartridge port and an extension port.  The OS design
	does not seem to be particularly expandable (I don't see any hook for
	additional DSRs), but there were prototypes for a parallel port (emulated)
	and a speech synthesizer unit (not emulated).


	The Tutor appears to be related to Texas Instruments' TI99 series.

	The general architecture is relatively close to the ti99/4(a): arguably,
	the Tutor does not include any GROM (it uses regular CPU ROMs for GPL
	code), and its memory map is quite different due to the fact it was
	designed with a tms9995 in mind (vs. a tms9985 for ti99/4), but, apart from
	that, it has a similar architecture with only 256 bytes of CPU RAM and 16kb
	of VRAM.

	While the OS is not derived directly from the TI99/4(a) OS, there are
	disturbing similarities: the Japanese title screen is virtually identical
	to the TI-99 title screen.  Moreover, the Tutor BASIC seems to be be
	derived from TI Extended BASIC, as both BASIC uses similar tokens and
	syntax, and are partially written in GPL (there is therefore a GPL
	interpreter in Tutor ROMs, although the Tutor GPL is incompatible with TI
	GPL, does not seem to be used by any program other than Tutor Basic, and it
	seems unlikely that the original Pyuuta had this GPL interpreter in ROMs).

	It appears that TI has sold the licence of the TI BASIC to Tomy, probably
	after it terminated its TI99 series.  It is not impossible that the entire
	Tutor concept is derived from a TI project under licence: this machine
	looks like a crossbreed of the TI99/2 and the TI99/4 (or /4a, /4b, /5), and
	it could either have been an early version of the TI99/2 project with a
	tms9918a/99289a VDP instead of the DMA video controller, or a "TI99/3" that
	would have closed the gap between the extremely low-end TI99/2 and the
	relatively mid-range TI99/5.


	Raphael Nabet, 2003


TODO :
	* debug the tape interface (Saved tapes sound OK, both Verify and Load
	  recognize the tape as a Tomy tape, but the data seems to be corrupted and
	  we end with a read error.)
	* guess which device is located at the >e600 base
	* find info about other Tutor variants


	Interrupts:

	Interrupt levels 1 (external interrupt 1) and 2 (error interrupt) do not
	seem to be used: triggering these seems to cause a soft reset.  XOPs are
	not used at all: the ROM area where these vectors should be defined is used
	by a ROM branch table.
*/

#include "driver.h"
#include "cpu/tms9900/tms9900.h"
#include "video/tms9928a.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "sound/sn76496.h"


/* mapper state */
static char cartridge_enable;

/* tape interface state */
static void tape_interrupt_handler(int dummy);

static char tape_interrupt_enable;
static mame_timer *tape_interrupt_timer;

/* parallel interface state */
static UINT8 printer_data;
static char printer_strobe;

enum
{
	basic_base = 0x8000,
	cartridge_base = 0xe000
};


static DRIVER_INIT(tutor)
{
	tape_interrupt_timer = timer_alloc(tape_interrupt_handler);

	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1) + basic_base, 0);
	memory_configure_bank(1, 1, 1, memory_region(REGION_CPU1) + cartridge_base, 0);
	memory_set_bank(1, 0);
}

static const TMS9928a_interface tms9929a_interface =
{
	TMS9929A,
	0x4000,
	0, 0,
	/*tms9901_set_int2*/NULL
};

static MACHINE_START(tutor)
{
	TMS9928A_configure(&tms9929a_interface);
	return 0;
}

static MACHINE_RESET(tutor)
{
	cartridge_enable = 0;

	tape_interrupt_enable = 0;

	printer_data = 0;
	printer_strobe = 0;
}

static void tutor_vblank_interrupt(void)
{
	/* No vblank interrupt? */
	TMS9928A_interrupt();
}

static mess_image *printer_fp(void)
{
	return image_from_devtype_and_index(IO_PARALLEL, 0);
}


/*
	Keyboard:

	Keyboard ports are located at CRU logical address >ec00->ec7e (CRU physical
	address >7600->763f).  There is one bit per key (bit >00 for keycode >00,
	bit >01 for keycode >01, etc.), each bit is set to one when the key is
	down.

	Joystick:

	Joystick ports seem to overlap keyboard ports, i.e. some CRU bits are
	mapped to both a keyboard key and a joystick switch.
*/

static READ8_HANDLER(read_keyboard)
{
	return readinputport(offset);
}

static int device_load_tutor_cart(mess_image *image)
{
	image_fread(image, memory_region(REGION_CPU1) + cartridge_base, 0x6000);
	return INIT_PASS;
}

static void device_unload_tutor_cart(mess_image *image)
{
	memset(memory_region(REGION_CPU1) + cartridge_base, 0, 0x6000);
}

/*
	Cartridge mapping:

	Cartridges share the >8000 address base with BASIC.  A write to @>e10c
	disables the BASIC ROM and enable the cartridge.  A write to @>e108
	disables the cartridge and enables the BASIC ROM.

	In order to be recognized by the system ROM, a cartridge should start with
	>55, >66 or >aa.  This may may correspond to three different ROM header
	versions (I am not sure).

	Cartridge may also define a boot ROM at base >0000 (see below).
*/

static  READ8_HANDLER(tutor_mapper_r)
{
	int reply;

	switch (offset)
	{
	case 0x10:
		/* return 0x42 if we have an cartridge with an alternate boot ROM */
		reply = 0;
		break;

	default:
		logerror("unknown port in %s %d\n", __FILE__, __LINE__);
		reply = 0;
		break;
	}

	return reply;
}

static WRITE8_HANDLER(tutor_mapper_w)
{
	switch (offset)
	{
	case 0x00:
		/* disable system ROM, enable alternate boot ROM in cartridge */
		break;

	case 0x08:
		/* disable cartridge ROM, enable BASIC ROM at base >8000 */
		cartridge_enable = 0;
		memory_set_bank(1, 0);
		break;

	case 0x0c:
		/* enable cartridge ROM, disable BASIC ROM at base >8000 */
		cartridge_enable = 1;
		memory_set_bank(1, 1);
		break;

	default:
		if (! (offset & 1))
			logerror("unknown port in %s %d\n", __FILE__, __LINE__);
		break;
	}
}

/*
	Cassette interface:

	The cassette interface uses several ports in the >e000 range.

	Writing to *CPU* address @>ee00 will set the tape output to 0.  Writing to
	*CPU* address @>ee20 will set the tape output to 1.

	Tape input level can be read from *CRU* logical address >ed00 (CRU physical
	address >7680).

	Writing to @>ee40 enables tape interrupts; writing to @>ee60 disables tape
	interrupts.  Tape interrupts are level-4 interrupt that occur when the tape
	input level is high(?).

	There are other output ports: @>ee80, @>eea0, @>eec0 & @>eee0.  I don't
	know their exact meaning.
*/

static void tape_interrupt_handler(int dummy)
{
	//assert(tape_interrupt_enable);
	cpunum_set_input_line(0, 1, (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.0) ? ASSERT_LINE : CLEAR_LINE);
}

/* CRU handler */
static  READ8_HANDLER(tutor_cassette_r)
{
	return (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.0) ? 1 : 0;
}

/* memory handler */
static WRITE8_HANDLER(tutor_cassette_w)
{
	if (offset & /*0x1f*/0x1e)
		logerror("unknown port in %s %d\n", __FILE__, __LINE__);

	if ((offset & 0x1f) == 0)
	{
		data = (offset & 0x20) != 0;

		switch ((offset >> 6) & 3)
		{
		case 0:
			/* data out */
			cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), (data) ? +1.0 : -1.0);
			break;
		case 1:
			/* interrupt control??? */
			//logerror("ignoring write of %d to cassette port 1\n", data);
			if (tape_interrupt_enable != ! data)
			{
				tape_interrupt_enable = ! data;
				if (tape_interrupt_enable)
					timer_adjust(tape_interrupt_timer, /*TIME_IN_HZ(44100)*/0., 0, TIME_IN_HZ(44100));
				else
				{
					timer_adjust(tape_interrupt_timer, TIME_NEVER, 0, 0.);
					cpunum_set_input_line(0, 1, CLEAR_LINE);
				}
			}
			break;
		case 2:
			/* ??? */
			logerror("ignoring write of %d to cassette port 2\n", data);
			break;
		case 3:
			/* ??? */
			logerror("ignoring write of %d to cassette port 3\n", data);
			break;
		}
	}
}

/* memory handlers */
static  READ8_HANDLER(tutor_printer_r)
{
	int reply;

	switch (offset)
	{
	case 0x20:
		/* busy */
		reply = printer_fp() ? 0xff : 0x00;
		break;

	default:
		if (! (offset & 1))
			logerror("unknown port in %s %d\n", __FILE__, __LINE__);
		reply = 0;
		break;
	}

	return reply;
}

static WRITE8_HANDLER(tutor_printer_w)
{
	switch (offset)
	{
	case 0x10:
		/* data */
		printer_data = data;
		break;

	case 0x40:
		/* strobe */
		if (data && ! printer_strobe)
		{
			/* strobe is asserted: output data */
			if (printer_fp())
				image_fwrite(printer_fp(), & printer_data, 1);
		}
		printer_strobe = data != 0;
		break;

	default:
		if (! (offset & 1))
			logerror("unknown port in %s %d\n", __FILE__, __LINE__);
		break;
	}
}

/*
	Memory map summary:

	@>0000-@>7fff: system ROM (can be paged out, see above).
	@>8000-@>bfff: basic ROM (can be paged out, see above).
	@>c000-@>dfff: free for future expansion? Used by 24kb cartridges?

	@>e000(r/w): VDP data
	@>e002(r/w): VDP register

	@>e100(w): enable cart and disable system ROM at base >0000??? (the system will only link to such a ROM if @>e110 is >42???)
	@>e108(w): disable cart and enable BASIC ROM at base >8000?
	@>e10c(w): enable cart and disable BASIC ROM at base >8000?
	@>e110(r): cartridges should return >42 if they have a ROM at base >0000 and they want the Tutor to boot from this ROM (with a blwp@>0000)???

	@>e200(w): sound write

	@>e600(r): handshake in from whatever device???
	@>e610(w): ???
	@>e620(w): ???
	@>e680(w): handshake out to this device???

	@>e810(w): parallel port data bus
	@>e820(r): parallel port busy input
	@>e840(w): parallel port strobe output

	@>ee00-@>eee0(w): tape interface (see above)

	@>f000-@>f0fb: tms9995 internal RAM 1
	@>fffa-@>fffb: tms9995 internal decrementer
	@>f000-@>f0fb: tms9995 internal RAM 2
*/

/*static WRITE8_HANDLER(test_w)
{
	switch (offset)
	{
	default:
		logerror("unmapped write %d %d\n", offset, data);
		break;
	}
}*/

static ADDRESS_MAP_START(tutor_memmap, ADDRESS_SPACE_PROGRAM, 8)

	AM_RANGE(0x0000, 0x7fff) AM_ROM	/*system ROM*/
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)	/*BASIC ROM & cartridge ROM*/
	AM_RANGE(0xc000, 0xdfff) AM_NOP	/*free for expansion, or cartridge ROM?*/

	AM_RANGE(0xe000, 0xe000) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)	/*VDP data*/
	AM_RANGE(0xe002, 0xe002) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)/*VDP status*/
	AM_RANGE(0xe100, 0xe1ff) AM_READWRITE(tutor_mapper_r, tutor_mapper_w)	/*cartridge mapper*/
	AM_RANGE(0xe200, 0xe200) AM_READWRITE(MRA8_NOP, SN76496_0_w)			/*sound chip*/
	AM_RANGE(0xe800, 0xe8ff) AM_READWRITE(tutor_printer_r, tutor_printer_w)	/*printer*/
	AM_RANGE(0xee00, 0xeeff) AM_READWRITE(MRA8_NOP, tutor_cassette_w)		/*cassette interface*/

	AM_RANGE(0xf000, 0xffff) AM_NOP	/*free for expansion (and internal processor RAM)*/

ADDRESS_MAP_END

/*
	CRU map summary:

	>1ee0->1efe: tms9995 flag register
	>1fda: tms9995 MID flag

	>ec00->ec7e(r): keyboard interface
	>ed00(r): tape input
*/

static ADDRESS_MAP_START(tutor_readcru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0xec0, 0xec7) AM_READ(read_keyboard)			/*keyboard interface*/
	AM_RANGE(0xed0, 0xed0) AM_READ(tutor_cassette_r)		/*cassette interface*/

ADDRESS_MAP_END

/*static ADDRESS_MAP_START(tutor_writecru, ADDRESS_SPACE_IO, 8)


ADDRESS_MAP_END*/



/* tutor keyboard: 56 keys */
INPUT_PORTS_START(tutor)

	PORT_START    /* col 0 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)

	PORT_START    /* col 1 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)

	PORT_START    /* col 2 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)

	PORT_START    /* col 3 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7 \\") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)

	PORT_START    /* col 4 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0 =") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("- |") PORT_CODE(KEYCODE_MINUS)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)

		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_PLAYER(1)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(1)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(1)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(1)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)

	PORT_START    /* col 5 */
		/* Unused? */
		PORT_BIT(0x03, IP_ACTIVE_HIGH, IPT_UNUSED)

		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\\ ^") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("_ @") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE)

		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(2)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_PLAYER(2)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(2)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(2)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(2)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)

	PORT_START    /* col 6 */
		/* Unused? */
		PORT_BIT(0x21, IP_ACTIVE_HIGH, IPT_UNUSED)

		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LOCK") PORT_CODE(KEYCODE_CAPSLOCK)
		/* only one shift key located on the left, but we support both for
		emulation to be friendlier */
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("MON") PORT_CODE(KEYCODE_F1)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RT") PORT_CODE(KEYCODE_ENTER)

		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("MOD") PORT_CODE(KEYCODE_F2)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(space)") PORT_CODE(KEYCODE_SPACE)

	PORT_START    /* col 7 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(left)") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(up)") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(down)") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(right)") PORT_CODE(KEYCODE_RIGHT)

		/* Unused? */
		PORT_BIT(0xf0, IP_ACTIVE_HIGH, IPT_UNUSED)


INPUT_PORTS_END


static struct tms9995reset_param tutor_processor_config =
{
#if 0
	REGION_CPU1,/* region for processor RAM */
	0xf000,     /* offset : this area is unused in our region, and matches the processor address */
	0xf0fc,		/* offset for the LOAD vector */
	1,          /* use fast IDLE */
#endif
	1,			/* enable automatic wait state generation */
	NULL		/* no IDLE callback */
};



static MACHINE_DRIVER_START(tutor)

	/* basic machine hardware */
	/* TMS9995 CPU @ 10.7 MHz */
	MDRV_CPU_ADD(TMS9995, 10700000)
	MDRV_CPU_CONFIG(tutor_processor_config)
	MDRV_CPU_PROGRAM_MAP(tutor_memmap, 0)
	MDRV_CPU_IO_MAP(tutor_readcru, /*tutor_writecru*/0)
	MDRV_CPU_VBLANK_INT(tutor_vblank_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_START( tutor )
	MDRV_MACHINE_RESET( tutor )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_IMPORT_FROM(tms9928a)

	/* sound */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76496, 3579545)	/* 3.579545 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END


/*
  ROM loading
*/
ROM_START(tutor)
	/*CPU memory space*/
	ROM_REGION(0x14000,REGION_CPU1,0)
	ROM_LOAD("tutor1.bin", 0x0000, 0x8000, CRC(702c38ba) SHA1(ce60607c3038895e31915d41bb5cf71cb8522d7a))      /* system ROM */
	ROM_LOAD("tutor2.bin", 0x8000, 0x4000, CRC(05f228f5) SHA1(46a14a45f6f9e2c30663a2b87ce60c42768a78d0))      /* BASIC ROM */
ROM_END

static void tutor_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_tutor_cart; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_tutor_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), ""); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void tutor_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void tutor_parallel_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* parallel */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_PARALLEL; break;
		case DEVINFO_INT_READABLE:						info->i = 0; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 1; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), ""); break;
	}
}

SYSTEM_CONFIG_START(tutor)

	/* cartridge port is not emulated */
	CONFIG_DEVICE(tutor_cartslot_getinfo)
	CONFIG_DEVICE(tutor_cassette_getinfo)
	CONFIG_DEVICE(tutor_parallel_getinfo)

SYSTEM_CONFIG_END

/*		YEAR	NAME	PARENT		COMPAT	MACHINE		INPUT	INIT	CONFIG		COMPANY		FULLNAME */
COMP(	1983?,	tutor,	0,			0,		tutor,		tutor,	tutor,	tutor,		"Tomy",		"Tomy Tutor" , 0)
