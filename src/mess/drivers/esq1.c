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
    - Actual 2681 DUART emulation
    - VFD display
    - Keyboard
    - Analog filters and VCA on the back end of the 5503
    - SQ-80 support (additional banking, FDC)

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/es5503.h"

static UINT8 uart_regs[0x10];
static UINT8 uart_outputport;

static void esq1_doc_irq(const device_config *device, int state)
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
	es5503_set_base(devtag_get_device(machine, "es5503"), memory_region(machine, "ensoniq"));

	// set default OSROM banking
	memory_set_bankptr(machine, "bank1", memory_region(machine, "osrom") );
}

static void recalc_osrom_bank(running_machine *machine)
{
	int bank = ((uart_outputport >> 1) & 0x7) ^ 7;

//  printf("bank %d => offset %x (PC=%x)\n", bank, bank * 0x1000, cpu_get_pc(machine->cpu[0]));
	memory_set_bankptr(machine, "bank1", memory_region(machine, "osrom") + (bank * 0x1000) );
}

static READ8_HANDLER( uart_r )
{
//  printf("Read UART @ %x\n", offset);

	switch (offset)
	{
		case 5:	// interrupt status
			return 0x08;	// timer/counter expire
			break;
	}

	return 0;
}

static WRITE8_HANDLER( uart_w )
{
	uart_regs[offset] = data;

//  printf("%x to UART %x\n", data, offset);

	switch (offset)
	{
		case 0xb:
//          printf("%02x to 6500\n", data);
			break;

		case 0xe:	// set output port bits
			uart_outputport |= data;
			recalc_osrom_bank(space->machine);
			break;

		case 0xf:	// reset output port bits
			uart_outputport &= ~data;
			recalc_osrom_bank(space->machine);
			break;
	}
}

static ADDRESS_MAP_START( esq1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM					// OSRAM
	AM_RANGE(0x4000, 0x5fff) AM_RAM					// SEQRAM
	AM_RANGE(0x6000, 0x63ff) AM_DEVREADWRITE("es5503", es5503_r, es5503_w)
	AM_RANGE(0x6400, 0x640f) AM_READWRITE( uart_r, uart_w )
	AM_RANGE(0x7000, 0x7fff) AM_ROMBANK("bank1")
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("osrom", 0x8000)	// OS "high" ROM is always mapped here
ADDRESS_MAP_END

static INTERRUPT_GEN( esq1_interrupt )
{
	cputag_set_input_line(device->machine, "maincpu", 0, HOLD_LINE);
}

static MACHINE_DRIVER_START( esq1 )
	MDRV_CPU_ADD("maincpu", M6809E, 4000000)	// how fast is it?
	MDRV_CPU_PROGRAM_MAP(esq1_map)
	MDRV_CPU_VBLANK_INT("screen", esq1_interrupt)

	MDRV_MACHINE_RESET( esq1 )

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
MACHINE_DRIVER_END

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

CONS( 1987, esq1, 0, 0, esq1, esq1, 0, 0, "Ensoniq", "ESQ-1", GAME_NOT_WORKING )

/*

7 to UART 6                counter/timer upper
d0 to UART 7               counter/timer lower
60 to UART 4               aux control (C/T timer mode using external clock)
20 to UART a               command register B
30 to UART a               command register B
10 to UART a               command register B
13 to UART 8               mode register B => 8 bits no parity
f to UART 8                mode register B => force odd parity, 8 bits (?)
ee to UART 9               clock select register B
5 to UART a                command register B (Enable Tx/Enable Rx)
e7 to UART b               Tx holding register B (send E7 to display)
0 to UART d                output port config (all bits are output ports)
ab to UART 5               interrupt mask register (input port change, RxRdyB, counter ready, RxReadyA, TxReadyA)

*/

