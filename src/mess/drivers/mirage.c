/***************************************************************************

    drivers/mirage.c

    Ensoniq Mirage Sampler
    Preliminary driver by R. Belmont

    Map for Mirage:
    0000-7fff: 32k window on 128k of sample RAM
    8000-bfff: main RAM
    c000-dfff: optional expansion RAM
    e100-e101: 6850 UART (for MIDI)
    e200-e2ff: 6522 VIA
    e408-e40f: filter cut-off frequency
    e410-e417: filter resonance
    e418-e41f: multiplexer address pre-set
    e800-e803: WD1770 FDC
    ec00-ecef: ES5503 "DOC" sound chip
    f000-ffff: boot ROM

LED patterns

     80
    _____
   |     | 40
04 | 02  |
    _____
   |     | 20
08 |     |
    _____
     10

PORT A: 111xyzzz


***************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "machine/6850acia.h"
#include "machine/6522via.h"
#include "machine/wd17xx.h"
#include "imagedev/flopdrv.h"
#include "formats/basicdsk.h"
#include "sound/es5503.h"

class mirage_state : public driver_device
{
public:
    mirage_state(const machine_config &mconfig, device_type type, const char *tag)
    : driver_device(mconfig, type, tag) { }

	virtual void machine_reset();

    int last_sndram_bank;
};


static void mirage_doc_irq(device_t *device, int state)
{
}

static READ8_DEVICE_HANDLER( mirage_adc_read )
{
	return 0x80;
}

static const es5503_interface mirage_es5503_interface =
{
	mirage_doc_irq,
	mirage_adc_read,
	NULL
};

static VIDEO_START( mirage )
{
}

static SCREEN_UPDATE( mirage )
{
	return 0;
}

void mirage_state::machine_reset()
{
	es5503_set_base(machine().device("es5503"), machine().region("ensoniq")->base());

	last_sndram_bank = 0;
	memory_set_bankptr(machine(), "sndbank", machine().region("ensoniq")->base() );
}

static ADDRESS_MAP_START( mirage_map, AS_PROGRAM, 8, mirage_state )
	AM_RANGE(0x0000, 0x7fff) AM_ROMBANK("sndbank")	// 32k window on 128k of wave RAM
	AM_RANGE(0x8000, 0xbfff) AM_RAM			// main RAM
	AM_RANGE(0xe100, 0xe100) AM_DEVREADWRITE_LEGACY("acia6850", acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0xe101, 0xe101) AM_DEVREADWRITE_LEGACY("acia6850", acia6850_data_r, acia6850_data_w)
	AM_RANGE(0xe200, 0xe2ff) AM_DEVREADWRITE("via6522", via6522_device, read, write)
	AM_RANGE(0xe800, 0xe800) AM_DEVREADWRITE_LEGACY("wd177x", wd17xx_status_r,wd17xx_command_w)
	AM_RANGE(0xe801, 0xe801) AM_DEVREADWRITE_LEGACY("wd177x", wd17xx_track_r,wd17xx_track_w)
	AM_RANGE(0xe802, 0xe802) AM_DEVREADWRITE_LEGACY("wd177x", wd17xx_sector_r,wd17xx_sector_w)
	AM_RANGE(0xe803, 0xe803) AM_DEVREADWRITE_LEGACY("wd177x", wd17xx_data_r,wd17xx_data_w)
//  AM_RANGE(0xec00, 0xecef) AM_DEVREADWRITE_LEGACY("es5503", es5503_r, es5503_w)
	AM_RANGE(0xf000, 0xffff) AM_ROM AM_REGION("osrom", 0)
ADDRESS_MAP_END

// port A: front panel
static WRITE8_DEVICE_HANDLER( mirage_via_write_porta )
{
//  printf("PORT A: %02x\n", data);
}

// port B:
//  bit 7: OUT UART clock
//  bit 4: OUT disk enable (motor on?)
//  bit 3: OUT sample/play
//  bit 2: OUT mic line/in
//  bit 1: OUT upper/lower bank (64k halves)
//  bit 0: OUT bank 0/bank 1 (32k halves)

static WRITE8_DEVICE_HANDLER( mirage_via_write_portb )
{
	int bank = 0;
    mirage_state *mirage = device->machine().driver_data<mirage_state>();

	// handle sound RAM bank switching
	bank = (data & 2) ? (64*1024) : 0;
	bank += (data & 1) ? (32*1024) : 0;
	if (bank != mirage->last_sndram_bank)
	{
		mirage->last_sndram_bank = bank;
		memory_set_bankptr(device->machine(), "sndbank", device->machine().region("ensoniq")->base() + bank);
	}
}

// port A: front panel
static READ8_DEVICE_HANDLER( mirage_via_read_porta )
{
	return 0;
}

// port B:
//  bit 6: IN FDC disk loaded
//  bit 5: IN 5503 sync (?)
static READ8_DEVICE_HANDLER( mirage_via_read_portb )
{
	return 0x60;
}

// external sync pulse
static READ8_DEVICE_HANDLER( mirage_via_read_ca1 )
{
	return 0;
}

// keyscan
static READ8_DEVICE_HANDLER( mirage_via_read_cb1 )
{
	return 0;
}

// keyscan
static READ8_DEVICE_HANDLER( mirage_via_read_ca2 )
{
	return 0;
}


// keyscan
static READ8_DEVICE_HANDLER( mirage_via_read_cb2 )
{
	return 0;
}

const via6522_interface mirage_via =
{
	DEVCB_HANDLER(mirage_via_read_porta),
	DEVCB_HANDLER(mirage_via_read_portb),
	DEVCB_HANDLER(mirage_via_read_ca1),
	DEVCB_HANDLER(mirage_via_read_cb1),
	DEVCB_HANDLER(mirage_via_read_ca2),
	DEVCB_HANDLER(mirage_via_read_cb2),
	DEVCB_HANDLER(mirage_via_write_porta),
	DEVCB_HANDLER(mirage_via_write_portb),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE("maincpu", M6809_IRQ_LINE)
};

static ACIA6850_INTERFACE( mirage_acia6850_interface )
{
	0,				// tx clock
	0,				// rx clock
	DEVCB_NULL,			// rx in
	DEVCB_NULL,			// rx out
	DEVCB_NULL,			// cts in
	DEVCB_NULL,			// rts out
	DEVCB_NULL,			// dcd in
	DEVCB_CPU_INPUT_LINE("maincpu", M6809_FIRQ_LINE)
};

static FLOPPY_OPTIONS_START(mirage)
	FLOPPY_OPTION(mirage, "img", "Mirage disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([80])
		SECTORS([5])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([0]))
FLOPPY_OPTIONS_END

static const floppy_config mirage_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_3_5_DSDD,
	FLOPPY_OPTIONS_NAME(mirage),
	NULL
};

const wd17xx_interface mirage_wd17xx_interface =
{
	DEVCB_NULL,			// dden
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_NMI),	// intrq
	DEVCB_NULL,			// drq
	{FLOPPY_0, NULL, NULL, NULL}
};

static MACHINE_CONFIG_START( mirage, mirage_state )
	MCFG_CPU_ADD("maincpu", M6809E, 4000000)
	MCFG_CPU_PROGRAM_MAP(mirage_map)

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_VIDEO_START(mirage)
	MCFG_SCREEN_UPDATE(mirage)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MCFG_SCREEN_SIZE(320, 240)
	MCFG_SCREEN_VISIBLE_AREA(0, 319, 1, 239)

	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MCFG_SOUND_ADD("es5503", ES5503, 7000000)
	MCFG_SOUND_CONFIG(mirage_es5503_interface)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)

	MCFG_VIA6522_ADD("via6522", 1000000, mirage_via)

	MCFG_ACIA6850_ADD("acia6850", mirage_acia6850_interface)

	MCFG_WD177X_ADD("wd177x", mirage_wd17xx_interface )
	MCFG_FLOPPY_2_DRIVES_ADD(mirage_floppy_config)
MACHINE_CONFIG_END

static INPUT_PORTS_START( mirage )
INPUT_PORTS_END

ROM_START( mirage )
	ROM_REGION(0x1000, "osrom", 0)
	ROM_LOAD( "mirage.bin",   0x0000, 0x1000, CRC(9fc7553c) SHA1(ec6ea5613eeafd21d8f3a7431a35a6ff16eed56d) )

	ROM_REGION(0x20000, "ensoniq", ROMREGION_ERASE)
ROM_END

CONS( 1984, mirage, 0, 0, mirage, mirage, 0, "Ensoniq", "Mirage", GAME_NOT_WORKING )

