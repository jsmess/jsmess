/***************************************************************************

    drivers/esq1.c

    Ensoniq ESQ-1 and SQ-80 Digital Wave Synthesizers
    Preliminary driver by R. Belmont

    Map for ESQ-1:
    0000-1fff: OS RAM
    2000-3fff: Cartridge
    4000-5fff: SEQRAM
    6000-63ff: ES5503 DOC
    6400-67ff: MC2681 DUART
    6800-6fff: AD7524 (CV_MUX)
    7000-7fff: OS ROM low (banked)
    8000-ffff: OS ROM high (fixed)

    Map for SQ-80:
    0000-1fff: OS RAM
    2000-3fff: Cartridge
    4000-5fff: DOSRAM or SEQRAM (banked)
    6000-63ff: ES5503 DOC
    6400-67ff: MC2681 DUART
    6800-6bff: AD7524 (CV_MUX)
    6c00-6dff: Mapper (bit 0 only - determines DOSRAM or SEQRAM at 4000)
    6e00-6fff: WD1772 FDC (not present on ESQ1)
    7000-7fff: OS ROM low (banked)
    8000-ffff: OS ROM high (fixed)

    CV_MUX area:
    write to        output goes to
    $68f8   $00     D/A converter
    $68f0   -$08    Filter Frequency (FF)
    $68e8   -$10    Filter Resonance (Q)
    $68d8   -$20    Final DCA (ENV4)
    $68b8   -$40    Panning (PAN)
    $6878   -$80    Floppy (Motor/LED on)

If SEQRAM is mapped at 4000, DUART port 2 determines the 32KB "master bank" and ports 0 and 1
determine which of the 4 8KB "sub banks" is visible.

Output ports 3 to 1 determine the 4kB page which should be shown at $7000 to $7fff.

IRQ sources are the DUART and the DRQ line from the FDC (SQ-80 only).
NMI is from the IRQ line on the FDC (again, SQ-80 only).

TODO:
    - VFD display
    - Keyboard
    - Analog filters and VCA on the back end of the 5503
    - SQ-80 support (additional banking, FDC)

***************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "sound/es5503.h"
#include "machine/68681.h"

static void esq1_doc_irq(running_device *device, int state)
{
}

static READ8_DEVICE_HANDLER( esq1_adc_read )
{
	return 0x80;
}

static const es5503_interface esq1_es5503_interface =
{
	esq1_doc_irq,
	esq1_adc_read,
	NULL
};

static VIDEO_START( esq1 )
{
}

static VIDEO_UPDATE( esq1 )
{
	return 0;
}

static MACHINE_RESET( esq1 )
{
	es5503_set_base(machine->device("es5503"), memory_region(machine, "ensoniq"));

	// set default OSROM banking
	memory_set_bankptr(machine, "osbank", memory_region(machine, "osrom") );
}

static ADDRESS_MAP_START( esq1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM					// OSRAM
	AM_RANGE(0x4000, 0x5fff) AM_RAM					// SEQRAM
	AM_RANGE(0x6000, 0x63ff) AM_DEVREADWRITE("es5503", es5503_r, es5503_w)
	AM_RANGE(0x6400, 0x640f) AM_DEVREADWRITE("duart", duart68681_r, duart68681_w)
	AM_RANGE(0x7000, 0x7fff) AM_ROMBANK("osbank")
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("osrom", 0x8000)	// OS "high" ROM is always mapped here
ADDRESS_MAP_END

// from the schematics:
//
// DUART channel A is MIDI
// channel B is to the keyboard/display
// IP0 = tape in
// IP1 = sequencer expansion cartridge inserted
// IP2 = patch cartridge inserted
// IP3 & 4 are 0.5 MHz, IP 5 & 6 are 1 MHz (note 0.5 MHz / 16 = MIDI baud rate)
//
// OP0 = to display processor
// OP1/2/3 = bank select 0, 1, and 2
// OP4 = metronome low
// OP5 = metronome hi
// OP6/7 = tape out

static void duart_irq_handler(running_device *device, UINT8 vector)
{
	cputag_set_input_line(device->machine, "maincpu", 0, HOLD_LINE);
};

static UINT8 duart_input(running_device *device)
{
	return 0;
}

static void duart_output(running_device *device, UINT8 data)
{
	int bank = ((data >> 1) & 0x7);

//  printf("DP [%02x]: %d mlo %d mhi %d tape %d\n", data, data&1, (data>>4)&1, (data>>5)&1, (data>>6)&3);
//  printf("[%02x] bank %d => offset %x (PC=%x)\n", data, bank, bank * 0x1000, cpu_get_pc(device->machine->firstcpu));
	memory_set_bankptr(device->machine, "osbank", memory_region(device->machine, "osrom") + (bank * 0x1000) );
}

static void duart_tx(running_device *device, int channel, UINT8 data)
{
	if ((data != 0) && (channel == 1)) printf("[%d]: %02x %c\n", channel, data, data);
}

static const duart68681_config duart_config =
{
	duart_irq_handler,
	duart_tx,
	duart_input,
	duart_output,

	500000, 500000,	// IP3, IP4
	1000000, 1000000, // IP5, IP6
};

static MACHINE_CONFIG_START( esq1, driver_data_t )
	MDRV_CPU_ADD("maincpu", M6809E, 4000000)	// how fast is it?
	MDRV_CPU_PROGRAM_MAP(esq1_map)

	MDRV_MACHINE_RESET( esq1 )

	MDRV_DUART68681_ADD("duart", 40000000, duart_config)

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_VIDEO_START(esq1)
	MDRV_VIDEO_UPDATE(esq1)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 1, 239)

	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MDRV_SOUND_ADD("es5503", ES5503, 7000000)
	MDRV_SOUND_CONFIG(esq1_es5503_interface)
	MDRV_SOUND_ROUTE(0, "lspeaker", 1.0)
	MDRV_SOUND_ROUTE(1, "rspeaker", 1.0)
MACHINE_CONFIG_END

static INPUT_PORTS_START( esq1 )
INPUT_PORTS_END

ROM_START( esq1 )
	ROM_REGION(0x10000, "osrom", 0)
        ROM_LOAD( "3p5lo.bin",    0x0000, 0x8000, CRC(ed001ad8) SHA1(14d1150bccdbc15d90567cf1812aacdb3b6ee882) )
        ROM_LOAD( "3p5hi.bin",    0x8000, 0x8000, CRC(332c572f) SHA1(ddb4f62807eb2ab29e5ac6b5d209d2ecc74cf806) )

	ROM_REGION(0x20000, "ensoniq", 0)
        ROM_LOAD( "esq1wavlo.bin", 0x0000, 0x8000, CRC(4d04ac87) SHA1(867b51229b0a82c886bf3b216aa8893748236d8b) )
        ROM_LOAD( "esq1wavhi.bin", 0x8000, 0x8000, CRC(94c554a3) SHA1(ed0318e5253637585559e8cf24c06d6115bd18f6) )
ROM_END

CONS( 1987, esq1, 0, 0, esq1, esq1, 0, "Ensoniq", "ESQ-1", GAME_NOT_WORKING )
