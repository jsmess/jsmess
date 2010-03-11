/*\
* * Linus Akesson's "Craft"
* *
* * Skeleton driver by MooglyGuy
* * Partial rewrite by Harmony
\*/

#include "emu.h"
#include "cpu/avr8/avr8.h"
#include "machine/avr8.h"
#include "sound/dac.h"

#define VERBOSE_LEVEL	(0)

#define ENABLE_VERBOSE_LOG (0)

#if ENABLE_VERBOSE_LOG
INLINE void verboselog(running_machine *machine, int n_level, const char *s_fmt, ...)
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", cpu_get_pc(devtag_get_device(machine, "maincpu")), buf );
	}
}
#else
#define verboselog(x,y,z,...)
#endif

#define MASTER_CLOCK 20000000

/****************************************************\
* I/O devices                                        *
\****************************************************/

class craft_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, craft_state(machine)); }

	craft_state(running_machine &machine) { }

	UINT8 regs[0x100];
	INT32 tcnt1_direction;

	emu_timer* ocr1a_timer;
	emu_timer* ocr1b_timer;
};

#if 0
static UINT64 avr8_get_timer_1_frequency(craft_state *state)
{
    UINT64 frequency = 0;
    switch(AVR8_TCCR1B & 0x07)
    {
        case 0: // No clock source (Timer/Counter stopped).
        case 6: // External clock source on T1 pin.  Clock on falling edge.
        case 7: // External clock source on T1 pin.  Clock on rising edge.
            frequency = 0;
            break;

        case 1: // clk / 1 (No prescaling)
            frequency = MASTER_CLOCK;
            break;

        case 2: // clk / 8 (From prescaler)
            frequency = MASTER_CLOCK / 8;
            break;

        case 3: // clk / 64 (From prescaler)
            frequency = MASTER_CLOCK / 64;
            break;

        case 4: // clk / 256 (From prescaler)
            frequency = MASTER_CLOCK / 256;
            break;

        case 5: // clk / 1024 (From prescaler)
            frequency = MASTER_CLOCK / 1024;
            break;

        default:
        	frequency = 0;
            fatalerror( "&state->tccr1b & 0x07 returned a value not in the range of 0 to 7.\n" );
            break;
    }

    return frequency;
}

static UINT16 avr8_get_tc1_top(craft_state *state)
{
	switch(AVR8_WGM1)
	{
		case AVR8_WGM1_NORMAL:
			return 0xffff;

		case AVR8_WGM1_PWM_8_PC:
		case AVR8_WGM1_FAST_PWM_8:
			return 0x00ff;

		case AVR8_WGM1_PWM_9_PC:
		case AVR8_WGM1_FAST_PWM_9:
			return 0x01ff;

		case AVR8_WGM1_PWM_10_PC:
		case AVR8_WGM1_FAST_PWM_10:
			return 0x03ff;

		case AVR8_WGM1_CTC_ICR:
		case AVR8_WGM1_PWM_PFC_ICR:
		case AVR8_WGM1_PWM_PC_ICR:
		case AVR8_WGM1_FAST_PWM_ICR:
			return AVR8_ICR1;

		case AVR8_WGM1_CTC_OCR:
		case AVR8_WGM1_PWM_PFC_OCR:
		case AVR8_WGM1_PWM_PC_OCR:
		case AVR8_WGM1_FAST_PWM_OCR:
			return AVR8_OCR1A;

		case AVR8_WGM1_RESERVED:
			logerror("Attempting to retrieve TOP for a reserved waveform generation mode\n");
			return 0xffff;
	}
	return 0xffff;
}

enum
{
	AVR8_REG_A = 0,
	AVR8_REG_B,
};

static UINT16 avr8_get_tc1_distance(craft_state *state, INT32 reg)
{
	UINT16 top = avr8_get_tc1_top(state);
	UINT16 ocr = (reg == AVR8_REG_A) ? AVR8_OCR1A : AVR8_OCR1B;
	if(AVR8_TCNT1_DIR > 0)
	{
		if(AVR8_TCNT1 < ocr)
		{
			return ocr - AVR8_TCNT1;
		}
		else
		{
			return top - AVR8_TCNT1;
		}
	}
	else
	{
		if(AVR8_TCNT1 > ocr)
		{
			return AVR8_TCNT1 - ocr;
		}
		else
		{
			return ocr;
		}
	}
	return 0xffff;
}

static void avr8_maybe_start_timer_1(craft_state *state)
{
    UINT64 frequency = avr8_get_timer_1_frequency(state);
    if(frequency == 0)
    {
        timer_adjust_oneshot(state->ocr1a_timer, attotime_never, 0);
        timer_adjust_oneshot(state->ocr1b_timer, attotime_never, 0);
    }
    else
    {
        attotime period_a = attotime_mul(ATTOTIME_IN_HZ(frequency), avr8_get_tc1_distance(state, AVR8_REG_A));
        attotime period_b = attotime_mul(ATTOTIME_IN_HZ(frequency), avr8_get_tc1_distance(state, AVR8_REG_B));
		timer_adjust_oneshot(state->ocr1a_timer, period_a, 0);
		timer_adjust_oneshot(state->ocr1b_timer, period_b, 0);
    }
}
#endif

static TIMER_CALLBACK( ocr1a_timer_compare )
{
    //craft_state *state = (craft_state *)machine->driver_data;
    //avr8_regs *regs = &state->regs;

	// TODO

    //avr8_maybe_start_timer_1(regs);
    //cpu_set_input_line(devtag_get_device(machine, "maincpu"), AVR8_INT_T1COMPA, ASSERT_LINE);
}

static TIMER_CALLBACK( ocr1b_timer_compare )
{
    //craft_state *state = (craft_state *)machine->driver_data;
    //avr8_regs *regs = &state->regs;

	// TODO

    //avr8_maybe_start_timer_1(regs);
    //cpu_set_input_line(devtag_get_device(machine, "maincpu"), AVR8_INT_T1COMPB, ASSERT_LINE);
}

static READ8_HANDLER( avr8_read )
{
    craft_state *state = (craft_state *)space->machine->driver_data;

	if(offset <= AVR8_REG_R31)
	{
		return state->regs[offset];
	}

    switch( offset )
    {
        default:
            verboselog(space->machine, 0, "AVR8: Unrecognized register read: %02x\n", offset );
    }

    return 0;
}

static WRITE8_HANDLER( avr8_write )
{
    craft_state *state = (craft_state *)space->machine->driver_data;

	if(offset <= AVR8_REG_R31)
	{
		state->regs[offset] = data;
	}

    switch( offset )
    {
        default:
            verboselog(space->machine, 0, "AVR8: Unrecognized register write: %02x = %02x\n", offset, data );
    }
}

/****************************************************\
* Address maps                                       *
\****************************************************/

static ADDRESS_MAP_START( craft_prg_map, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x1fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( craft_io_map, ADDRESS_SPACE_IO, 8 )
    AM_RANGE(0x0000, 0x00ff) AM_READWRITE(avr8_read, avr8_write)
    AM_RANGE(0x0100, 0x04ff) AM_RAM
ADDRESS_MAP_END

/****************************************************\
* Input ports                                        *
\****************************************************/

static INPUT_PORTS_START( craft )
    PORT_START("MAIN")
        PORT_BIT(0xff, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END

/****************************************************\
* Video hardware                                     *
\****************************************************/

static VIDEO_UPDATE( craft )
{
    return 0;
}

/****************************************************\
* Machine definition                                 *
\****************************************************/

static DRIVER_INIT( craft )
{
    craft_state *state = (craft_state *)machine->driver_data;

    state->ocr1a_timer = timer_alloc(machine, ocr1a_timer_compare, 0);
    state->ocr1b_timer = timer_alloc(machine, ocr1b_timer_compare, 0);
}

static MACHINE_RESET( craft )
{
    craft_state *state = (craft_state *)machine->driver_data;

    AVR8_TIMSK1 = 0;
    AVR8_OCR1AH = 0;
    AVR8_OCR1AL = 0;
    AVR8_OCR1BH = 0;
    AVR8_OCR1BL = 0;
    AVR8_ICR1H = 0;
    AVR8_ICR1L = 0;
    AVR8_TCNT1H = 0;
    AVR8_TCNT1L = 0;

    dac_data_w(devtag_get_device(machine, "dac"), 0x00);
}

static MACHINE_DRIVER_START( craft )

    MDRV_DRIVER_DATA( craft_state )

    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", AVR8, MASTER_CLOCK)
    MDRV_CPU_PROGRAM_MAP(craft_prg_map)
    MDRV_CPU_IO_MAP(craft_io_map)

    MDRV_MACHINE_RESET(craft)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    //MDRV_SCREEN_RAW_PARAMS( MASTER_CLOCK, 634, 0, 633, 525, 0, 481 )
    MDRV_SCREEN_REFRESH_RATE(60.08)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(1395)) /* accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MDRV_SCREEN_SIZE(634, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 633, 0, 479)
    MDRV_PALETTE_LENGTH(0x1000)

    MDRV_VIDEO_START(generic_bitmapped)
    MDRV_VIDEO_UPDATE(craft)

    /* sound hardware */
    MDRV_SPEAKER_STANDARD_MONO("avr8")
    MDRV_SOUND_ADD("dac", DAC, 0)
    MDRV_SOUND_ROUTE(0, "avr8", 1.00)
MACHINE_DRIVER_END

ROM_START( craft )
	ROM_REGION( 0x2000, "maincpu", 0 )  /* Main program store */
	ROM_LOAD( "craft.bin", 0x0000, 0x2000, CRC(2e6f9ad2) SHA1(75e495bf18395d74289ca7ee2649622fc4010457) )
	ROM_REGION( 0x200, "eeprom", 0 )  /* on-die eeprom */
	ROM_LOAD( "eeprom.raw", 0x0000, 0x0200, CRC(e18a2af9) SHA1(81fc6f2d391edfd3244870214fac37929af0ac0c) )
ROM_END

/*   YEAR  NAME      PARENT    COMPAT    MACHINE   INPUT     INIT      COMPANY          FULLNAME */
CONS(2008, craft,    0,        0,        craft,    craft,    craft,    "Linus Akesson", "Craft", GAME_NO_SOUND | GAME_NOT_WORKING)