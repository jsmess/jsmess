/***************************************************************************

    esq5505.c - Ensoniq ES5505 + ES5510 based synthesizers

    Ensoniq VFX, EPS-16 Plus, and SQ-1 (VFX-SD, SD-1, SQ-1 Plus, SQ-2,
    and KS-32 are known to also be this architecture).

    The Taito sound system in taito_en.c is directly derived from the SQ-1.

    Driver by R. Belmont

    00 - harsh piano
    01 - bells/chimes
    04 - organ
    13 - transwave brass
    17 - slow string
    18 - plucked string
    19 - layered strings
    20 - marimba
    26 - Hammond
    29 - slow organ
    38 - nylon guitar
    41 - layered strings
    51/52 - RE-style slow strings
    53 - RE-style fast strings
    69 - orch hit
    70 - organ
    73 - orch hit with strings
    74 - bell tone
    80 - drums

***************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "sound/es5506.h"
#include "machine/68681.h"

#define KEYBOARD_HACK   (0)

#if KEYBOARD_HACK
static int program = 1;
#endif

class esq5505_state : public driver_device
{
public:
	esq5505_state(const machine_config &mconfig, device_type type, const char *tag)
	: driver_device(mconfig, type, tag),
        m_maincpu(*this, "maincpu"),
        m_duart(*this, "duart")
    { }

    required_device<device_t> m_maincpu;
    required_device<device_t> m_duart;

    virtual void machine_reset();

    DECLARE_READ16_MEMBER(es5510_dsp_r);
    DECLARE_WRITE16_MEMBER(es5510_dsp_w);
    DECLARE_READ16_MEMBER(mc68681_r);
    DECLARE_WRITE16_MEMBER(mc68681_w);

private:
    UINT16  es5510_dsp_ram[0x200];
    UINT32  es5510_gpr[0xc0];
    UINT32  es5510_dram[1<<24];
    UINT32  es5510_dol_latch;
    UINT32  es5510_dil_latch;
    UINT32  es5510_dadr_latch;
    UINT32  es5510_gpr_latch;
    UINT8   es5510_ram_sel;
};


static VIDEO_START( esq5505 )
{
}

static SCREEN_UPDATE_RGB32( esq5505 )
{
#if KEYBOARD_HACK
    if( screen.machine().input().code_pressed_once( KEYCODE_Q ) )
    {
        program--;
        if (program < 0)
        {
            program = 0;
        }
        printf("program to %d\n", program);
    }
	if( screen.machine().input().code_pressed_once( KEYCODE_W ) )
    {
        program++;
        if (program > 127)
        {
            program = 127;
        }
        printf("program to %d\n", program);
    }
#endif
	return 0;
}

void esq5505_state::machine_reset()
{
	UINT8 *ROM = machine().root_device().memregion("osrom")->base();
	UINT8 *RAM = (UINT8 *)machine().root_device().memshare("osram")->ptr();

    memcpy(RAM, ROM, 256);

    // pick up the new vectors
    m_maincpu->reset();
}

READ16_MEMBER(esq5505_state::es5510_dsp_r)
{
//  logerror("%06x: DSP read offset %04x (data is %04x)\n",cpu_get_pc(&space->device()),offset,es5510_dsp_ram[offset]);

	switch(offset)
	{
		case 0x09: return (es5510_dil_latch >> 16) & 0xff;
		case 0x0a: return (es5510_dil_latch >> 8) & 0xff;
		case 0x0b: return (es5510_dil_latch >> 0) & 0xff; //TODO: docs says that this always returns 0
	}

	if (offset==0x12) return 0;

	if (offset==0x16) return 0x27;

	return es5510_dsp_ram[offset];
}

WRITE16_MEMBER(esq5505_state::es5510_dsp_w)
{
	UINT8 *snd_mem = (UINT8 *)space.machine().root_device().memregion("waverom")->base();

	COMBINE_DATA(&es5510_dsp_ram[offset]);

	switch (offset) {
		case 0x00: es5510_gpr_latch=(es5510_gpr_latch&0x00ffff)|((data&0xff)<<16); break;
		case 0x01: es5510_gpr_latch=(es5510_gpr_latch&0xff00ff)|((data&0xff)<< 8); break;
		case 0x02: es5510_gpr_latch=(es5510_gpr_latch&0xffff00)|((data&0xff)<< 0); break;

		/* 0x03 to 0x08 INSTR Register */
		/* 0x09 to 0x0b DIL Register (r/o) */

		case 0x0c: es5510_dol_latch=(es5510_dol_latch&0x00ffff)|((data&0xff)<<16); break;
		case 0x0d: es5510_dol_latch=(es5510_dol_latch&0xff00ff)|((data&0xff)<< 8); break;
		case 0x0e: es5510_dol_latch=(es5510_dol_latch&0xffff00)|((data&0xff)<< 0); break; //TODO: docs says that this always returns 0xff

		case 0x0f:
			es5510_dadr_latch=(es5510_dadr_latch&0x00ffff)|((data&0xff)<<16);
			if(es5510_ram_sel)
				es5510_dil_latch = es5510_dram[es5510_dadr_latch];
			else
				es5510_dram[es5510_dadr_latch] = es5510_dol_latch;
			break;

		case 0x10: es5510_dadr_latch=(es5510_dadr_latch&0xff00ff)|((data&0xff)<< 8); break;
		case 0x11: es5510_dadr_latch=(es5510_dadr_latch&0xffff00)|((data&0xff)<< 0); break;

		/* 0x12 Host Control */

		case 0x14: es5510_ram_sel = data & 0x80; /* bit 6 is i/o select, everything else is undefined */break;

		/* 0x16 Program Counter (test purpose, r/o?) */
		/* 0x17 Internal Refresh counter (test purpose) */
		/* 0x18 Host Serial Control */
		/* 0x1f Halt enable (w) / Frame Counter (r) */

		case 0x80: /* Read select - GPR + INSTR */
	//      logerror("ES5510:  Read GPR/INSTR %06x (%06x)\n",data,es5510_gpr[data]);

			/* Check if a GPR is selected */
			if (data<0xc0) {
				//es_tmp=0;
				es5510_gpr_latch=es5510_gpr[data];
			}// else es_tmp=1;
			break;

		case 0xa0: /* Write select - GPR */
	//      logerror("ES5510:  Write GPR %06x %06x (0x%04x:=0x%06x\n",data,es5510_gpr_latch,data,snd_mem[es5510_gpr_latch>>8]);
			if (data<0xc0)
				es5510_gpr[data]=snd_mem[es5510_gpr_latch>>8];
			break;

		case 0xc0: /* Write select - INSTR */
	//      logerror("ES5510:  Write INSTR %06x %06x\n",data,es5510_gpr_latch);
			break;

		case 0xe0: /* Write select - GPR + INSTR */
	//      logerror("ES5510:  Write GPR/INSTR %06x %06x\n",data,es5510_gpr_latch);
			break;
	}
}

static ADDRESS_MAP_START( vfx_map, AS_PROGRAM, 16, esq5505_state )
	AM_RANGE(0x000000, 0x03ffff) AM_RAM AM_SHARE("osram")
	AM_RANGE(0x200000, 0x20001f) AM_DEVREADWRITE_LEGACY("ensoniq", es5505_r, es5505_w)
	AM_RANGE(0x260000, 0x2601ff) AM_READWRITE(es5510_dsp_r, es5510_dsp_w)
    AM_RANGE(0x280000, 0x28001f) AM_DEVREADWRITE8_LEGACY("duart", duart68681_r, duart68681_w, 0x00ff)
    AM_RANGE(0xc00000, 0xc1ffff) AM_ROM AM_REGION("osrom", 0)
    AM_RANGE(0xfc0000, 0xffffff) AM_RAM AM_SHARE("osram")
ADDRESS_MAP_END

static ADDRESS_MAP_START( eps16_map, AS_PROGRAM, 16, esq5505_state )
	AM_RANGE(0x000000, 0x03ffff) AM_RAM AM_SHARE("osram")
	AM_RANGE(0x200000, 0x20001f) AM_DEVREADWRITE_LEGACY("ensoniq", es5505_r, es5505_w)
    AM_RANGE(0x280000, 0x28001f) AM_DEVREADWRITE8_LEGACY("duart", duart68681_r, duart68681_w, 0x00ff)
    AM_RANGE(0x580000, 0x77ffff) AM_RAM         // sample RAM (2 MB max represented here)
    AM_RANGE(0xc00000, 0xc0ffff) AM_ROM AM_REGION("osrom", 0)
ADDRESS_MAP_END

static void duart_irq_handler(device_t *device, int state, UINT8 vector)
{
    esq5505_state *esq5505 = device->machine().driver_data<esq5505_state>();

//    printf("\nDUART IRQ: state %d vector %d\n", state, vector);
    if (state == ASSERT_LINE)
    {
        device_set_input_line_vector(esq5505->m_maincpu, 1, vector);
        device_set_input_line(esq5505->m_maincpu, 1, ASSERT_LINE);
    }
    else
    {
        device_set_input_line(esq5505->m_maincpu, 1, CLEAR_LINE);
    }
};

static UINT8 duart_input(device_t *device)
{
	return 0;
}

static void duart_output(device_t *device, UINT8 data)
{
//    printf("DUART output: %02x\n", data);
}

static void duart_tx(device_t *device, int channel, UINT8 data)
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

static const duart68681_config duart_config =
{
	duart_irq_handler,
	duart_tx,
	duart_input,
	duart_output,

	500000, 500000,	// IP3, IP4
	1000000, 1000000, // IP5, IP6
};

static const es5505_interface es5505_config =
{
	"waverom",	/* Bank 0 */
	"waverom",	/* Bank 1 */
	NULL        /* irq */
};

static MACHINE_CONFIG_START( vfx, esq5505_state )
	MCFG_CPU_ADD("maincpu", M68000, 16000000)
	MCFG_CPU_PROGRAM_MAP(vfx_map)

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_UPDATE_STATIC(esq5505)
	MCFG_SCREEN_SIZE(320, 240)
	MCFG_SCREEN_VISIBLE_AREA(0, 319, 1, 239)

	MCFG_VIDEO_START(esq5505)

	MCFG_DUART68681_ADD("duart", 2000000, duart_config)

	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MCFG_SOUND_ADD("ensoniq", ES5505, 30476100/2)
	MCFG_SOUND_CONFIG(es5505_config)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED(eps16, vfx)
	MCFG_CPU_MODIFY( "maincpu" )
	MCFG_CPU_PROGRAM_MAP(eps16_map)
MACHINE_CONFIG_END

#if KEYBOARD_HACK
static INPUT_CHANGED( key_stroke )
{
	esq5505_state *state = device.machine().driver_data<esq5505_state>();

    // send a MIDI Note On
    if (oldval == 0 && newval == 1)
    {
        if ((UINT8)(FPTR)param == 0x40)
        {
            duart68681_rx_data(state->m_duart, 0, (UINT8)(FPTR)0xc0);   // program change
            duart68681_rx_data(state->m_duart, 0, program);             // program
        }
        else
        {
            duart68681_rx_data(state->m_duart, 0, (UINT8)(FPTR)0x90);   // note on
            duart68681_rx_data(state->m_duart, 0, (UINT8)(FPTR)param);
            duart68681_rx_data(state->m_duart, 0, (UINT8)(FPTR)0x7f);
        }
    }
    else if (oldval == 1 && newval == 0)
    {
        if ((UINT8)(FPTR)param != 0x40)
        {
            duart68681_rx_data(state->m_duart, 0, (UINT8)(FPTR)0x80);   // note off
            duart68681_rx_data(state->m_duart, 0, (UINT8)(FPTR)param);
            duart68681_rx_data(state->m_duart, 0, (UINT8)(FPTR)0x7f);
        }
    }
}
#endif

static INPUT_PORTS_START( vfx )
#if KEYBOARD_HACK
	PORT_START("KEY0")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A') PORT_CHANGED(key_stroke, 0x40)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S') PORT_CHANGED(key_stroke, 0x41)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D') PORT_CHANGED(key_stroke, 0x42)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F') PORT_CHANGED(key_stroke, 0x43)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G') PORT_CHANGED(key_stroke, 0x44)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H') PORT_CHANGED(key_stroke, 0x45)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J') PORT_CHANGED(key_stroke, 0x46)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K') PORT_CHANGED(key_stroke, 0x47)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L') PORT_CHANGED(key_stroke, 0x48)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q') PORT_CHANGED(key_stroke, 0x49)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W') PORT_CHANGED(key_stroke, 0x4a)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('e') PORT_CHAR('E') PORT_CHANGED(key_stroke, 0x4b)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('r') PORT_CHAR('R') PORT_CHANGED(key_stroke, 0x4c)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('t') PORT_CHAR('T') PORT_CHANGED(key_stroke, 0x4d)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('y') PORT_CHAR('Y') PORT_CHANGED(key_stroke, 0x4e)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('u') PORT_CHAR('U') PORT_CHANGED(key_stroke, 0x4f)
#endif
INPUT_PORTS_END

ROM_START( vfx )
    ROM_REGION(0x20000, "osrom", 0)
    ROM_LOAD16_BYTE( "vfx210b-low.bin",  0x000000, 0x010000, CRC(c51b19cd) SHA1(2a125b92ffa02ae9d7fb88118d525491d785e87e) )
    ROM_LOAD16_BYTE( "vfx210b-high.bin", 0x000001, 0x010000, CRC(59853be8) SHA1(8e07f69d53f80885d15f624e0b912aeaf3212ee4) )

    ROM_REGION(0x200000, "waverom", ROMREGION_ERASE00)
    ROM_LOAD( "vfx-waves-1.bin",  0x000000, 0x080000, NO_DUMP )
    ROM_LOAD( "vfx-waves-2.bin",  0x100000, 0x080000, NO_DUMP )
ROM_END

ROM_START( sq1 )
    ROM_REGION(0x20000, "osrom", 0)
    ROM_LOAD16_BYTE( "esq5505lo.bin",    0x000000, 0x010000, CRC(b004cf05) SHA1(567b0dae2e35b06e39da108f9c041fd9bc38fa35) )
    ROM_LOAD16_BYTE( "esq5505up.bin",    0x000001, 0x010000, CRC(2e927873) SHA1(06a948cb71fa254b23f4b9236f29035d10778da1) )

    ROM_REGION(0x200000, "waverom", 0)
    ROM_LOAD16_BYTE( "sq1-u25.bin",  0x000001, 0x080000, CRC(26312451) SHA1(9f947a11592fd8420fc581914bf16e7ade75390c) )
    ROM_LOAD16_BYTE( "sq1-u26.bin",  0x100001, 0x080000, CRC(2edaa9dc) SHA1(72fead505c4f44e5736ff7d545d72dfa37d613e2) )
ROM_END

ROM_START( eps16 )
    ROM_REGION(0x10000, "osrom", 0)
    ROM_LOAD16_BYTE( "eps-l.bin",    0x000000, 0x008000, CRC(382beac1) SHA1(110e31edb03fcf7bbde3e17423b21929e5b32db2) )
    ROM_LOAD16_BYTE( "eps-h.bin",    0x000001, 0x008000, CRC(d8747420) SHA1(460597751386eb5f08465699b61381c4acd78065) )

    ROM_REGION(0x200000, "waverom", ROMREGION_ERASE00)      // did the EPS-16 have ROM sounds or is it a pure sampler?
ROM_END

CONS( 1989, vfx,   0, 0, vfx,   vfx, 0, "Ensoniq", "VFX", GAME_NOT_WORKING )
CONS( 1990, sq1,   0, 0, vfx,   vfx, 0, "Ensoniq", "SQ-1", GAME_NOT_WORKING )
CONS( 1990, eps16, 0, 0, eps16, vfx, 0, "Ensoniq", "EPS-16 Plus", GAME_NOT_WORKING )

