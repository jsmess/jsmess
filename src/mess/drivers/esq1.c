/***************************************************************************

    drivers/esq1.c

    Ensoniq ESQ-1, ESQ-m, and SQ-80 Digital Wave Synthesizers
    Preliminary driver by R. Belmont

    Map for ESQ-1 and ESQ-m:
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
    $6878   -$80    Floppy (Motor/LED on - SQ-80 only)

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

#define KEYBOARD_HACK   (0)

class esq1_state : public driver_device
{
public:
	esq1_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
        m_duart(*this, "duart")
    { }

    required_device<device_t> m_duart;

};


static void esq1_doc_irq(device_t *device, int state)
{
}

static UINT8 esq1_adc_read(device_t *device)
{
	return 0x80;
}

static VIDEO_START( esq1 )
{
}

static SCREEN_UPDATE_RGB32( esq1 )
{
	return 0;
}

static MACHINE_RESET( esq1 )
{
	// set default OSROM banking
	esq1_state *state = machine.driver_data<esq1_state>();
	state->membank("osbank")->set_base(machine.root_device().memregion("osrom")->base() );
}

static ADDRESS_MAP_START( esq1_map, AS_PROGRAM, 8, esq1_state )
	AM_RANGE(0x0000, 0x1fff) AM_RAM					// OSRAM
	AM_RANGE(0x4000, 0x5fff) AM_RAM					// SEQRAM
	AM_RANGE(0x6000, 0x63ff) AM_DEVREADWRITE("es5503", es5503_device, read, write)
	AM_RANGE(0x6400, 0x640f) AM_DEVREADWRITE_LEGACY("duart", duart68681_r, duart68681_w)
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

static void duart_irq_handler(device_t *device, int state, UINT8 vector)
{
	cputag_set_input_line(device->machine(), "maincpu", 0, state);
};

static UINT8 duart_input(device_t *device)
{
	return 0;
}

static void duart_output(device_t *device, UINT8 data)
{
	int bank = ((data >> 1) & 0x7);
	esq1_state *state = device->machine().driver_data<esq1_state>();
//  printf("DP [%02x]: %d mlo %d mhi %d tape %d\n", data, data&1, (data>>4)&1, (data>>5)&1, (data>>6)&3);
//  printf("[%02x] bank %d => offset %x (PC=%x)\n", data, bank, bank * 0x1000, cpu_get_pc(device->machine().firstcpu));
	state->membank("osbank")->set_base(state->memregion("osrom")->base() + (bank * 0x1000) );
}

static void duart_tx(device_t *device, int channel, UINT8 data)
{
	if (channel == 1)
    {
        if (data >= 0x20 && data <= 0x7f)
        {
            printf("%c", data);
        }
        else
        {
            printf("[%02x]", data);
        }
    }
}

#if KEYBOARD_HACK
static INPUT_CHANGED( key_stroke )
{
	esq1_state *state = device.machine().driver_data<esq1_state>();

    if (oldval == 0 && newval == 1)
    {
        printf("key on %02x\n", (int)(FPTR)param);
        duart68681_rx_data(state->m_duart, 1, (UINT8)(FPTR)param);
    }
    else if (oldval == 1 && newval == 0)
    {
        printf("key off %02x\n", (int)(FPTR)param);
//        duart68681_rx_data(state->m_duart, 1, (UINT8)(FPTR)param-0x40);
    }
}
#endif

static const duart68681_config duart_config =
{
	duart_irq_handler,
	duart_tx,
	duart_input,
	duart_output,

	500000, 500000,	// IP3, IP4
	1000000, 1000000, // IP5, IP6
};

static MACHINE_CONFIG_START( esq1, esq1_state )
	MCFG_CPU_ADD("maincpu", M6809E, 4000000)	// how fast is it?
	MCFG_CPU_PROGRAM_MAP(esq1_map)

	MCFG_MACHINE_RESET( esq1 )

	MCFG_DUART68681_ADD("duart", 4000000, duart_config)

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_UPDATE_STATIC(esq1)
	MCFG_SCREEN_SIZE(320, 240)
	MCFG_SCREEN_VISIBLE_AREA(0, 319, 1, 239)

	MCFG_VIDEO_START(esq1)

	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MCFG_ES5503_ADD("es5503", 7000000, esq1_doc_irq, esq1_adc_read)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)
MACHINE_CONFIG_END

static INPUT_PORTS_START( esq1 )
    #if KEYBOARD_HACK
    PORT_START("KEY0")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) 			PORT_CHAR('a') PORT_CHAR('A') PORT_CHANGED(key_stroke, 0x80)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S') PORT_CHANGED(key_stroke, 0x81)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D') PORT_CHANGED(key_stroke, 0x82)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F') PORT_CHANGED(key_stroke, 0x83)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G') PORT_CHANGED(key_stroke, 0x84)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H') PORT_CHANGED(key_stroke, 0x85)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J') PORT_CHANGED(key_stroke, 0x86)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K') PORT_CHANGED(key_stroke, 0x87)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L') PORT_CHANGED(key_stroke, 0x88)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q') PORT_CHANGED(key_stroke, 0x89)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)             PORT_CHAR('w') PORT_CHAR('W') PORT_CHANGED(key_stroke, 0x8a)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('e') PORT_CHAR('E') PORT_CHANGED(key_stroke, 0x8b)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('r') PORT_CHAR('R') PORT_CHANGED(key_stroke, 0x8c)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('t') PORT_CHAR('T') PORT_CHANGED(key_stroke, 0x8d)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('y') PORT_CHAR('Y') PORT_CHANGED(key_stroke, 0x8e)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('u') PORT_CHAR('U') PORT_CHANGED(key_stroke, 0x8f)

	PORT_START("KEY1")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHANGED(key_stroke, 0x0c)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHANGED(key_stroke, 0x0d)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHANGED(key_stroke, 0x0e)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHANGED(key_stroke, 0x15)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHANGED(key_stroke, 0x0f)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHANGED(key_stroke, 0x10)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHANGED(key_stroke, 0x11)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHANGED(key_stroke, 0x12)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHANGED(key_stroke, 0x13)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) 			PORT_CHAR('0') PORT_CHANGED(key_stroke, 0x14)
    #endif
INPUT_PORTS_END

ROM_START( esq1 )
    ROM_REGION(0x10000, "osrom", 0)
    ROM_LOAD( "3p5lo.bin",    0x0000, 0x8000, CRC(ed001ad8) SHA1(14d1150bccdbc15d90567cf1812aacdb3b6ee882) )
    ROM_LOAD( "3p5hi.bin",    0x8000, 0x8000, CRC(332c572f) SHA1(ddb4f62807eb2ab29e5ac6b5d209d2ecc74cf806) )

    ROM_REGION(0x20000, "es5503", 0)
    ROM_LOAD( "esq1wavlo.bin", 0x0000, 0x8000, CRC(4d04ac87) SHA1(867b51229b0a82c886bf3b216aa8893748236d8b) )
    ROM_LOAD( "esq1wavhi.bin", 0x8000, 0x8000, CRC(94c554a3) SHA1(ed0318e5253637585559e8cf24c06d6115bd18f6) )
ROM_END

CONS( 1986, esq1, 0, 0, esq1, esq1, 0, "Ensoniq", "ESQ-1", GAME_NOT_WORKING )
