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

#define ENABLE_VERBOSE_LOG (1)

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
		logerror( "%08x: %s", cpu_get_pc(machine->device("maincpu")), buf );
	}
}
#else
#define verboselog(x,y,z,...)
#endif

#define MASTER_CLOCK 20000000

/****************************************************\
* I/O devices                                        *
\****************************************************/

class craft_state : public driver_device
{
public:
	craft_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 regs[0x100];
	INT32 tcnt1_direction;

	emu_timer* ocr1a_timer;
	emu_timer* ocr1b_timer;
};

enum
{
	AVR8_REG_A = 0,
	AVR8_REG_B,
	AVR8_REG_C,
	AVR8_REG_D,
};

static const char avr8_reg_name[4] = { 'A', 'B', 'C', 'D' };

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
            fatalerror( "&state->tccr1b & 0x07 returned a value not in the range of 0 to 7." );
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

static UINT16 avr8_get_tc1_distance(craft_state *state, INT32 reg)
{
	UINT16 top = avr8_get_tc1_top(state);
	UINT16 ocr = (reg == Avr8::REGIDX_A) ? AVR8_OCR1A : AVR8_OCR1B;
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
        attotime period_a = attotime_mul(ATTOTIME_IN_HZ(frequency), avr8_get_tc1_distance(state, Avr8::REGIDX_A));
        attotime period_b = attotime_mul(ATTOTIME_IN_HZ(frequency), avr8_get_tc1_distance(state, Avr8::REGIDX_B));
		timer_adjust_oneshot(state->ocr1a_timer, period_a, 0);
		timer_adjust_oneshot(state->ocr1b_timer, period_b, 0);
    }
}
#endif

static TIMER_CALLBACK( ocr1a_timer_compare )
{
    //craft_state *state = machine->driver_data<craft_state>();
    //avr8_regs *regs = &state->regs;

	// TODO

    //avr8_maybe_start_timer_1(regs);
    //cpu_set_input_line(machine->device("maincpu"), AVR8_INT_T1COMPA, ASSERT_LINE);
}

static TIMER_CALLBACK( ocr1b_timer_compare )
{
    //craft_state *state = machine->driver_data<craft_state>();
    //avr8_regs *regs = &state->regs;

	// TODO

    //avr8_maybe_start_timer_1(regs);
    //cpu_set_input_line(machine->device("maincpu"), AVR8_INT_T1COMPB, ASSERT_LINE);
}

static READ8_HANDLER( avr8_read )
{
    craft_state *state = space->machine->driver_data<craft_state>();

	if(offset <= Avr8::REGIDX_R31)
	{
		return state->regs[offset];
	}

    switch( offset )
    {
		case Avr8::REGIDX_SPL:
		case Avr8::REGIDX_SPH:
		case Avr8::REGIDX_SREG:
			return state->regs[offset];

        default:
            verboselog(space->machine, 0, "AVR8: Unrecognized register read: %02x\n", offset );
    }

    return 0;
}

static UINT8 avr8_get_ddr(running_machine *machine, int reg)
{
    craft_state *state = machine->driver_data<craft_state>();

	switch(reg)
	{
		case AVR8_REG_B:
			return AVR8_DDRB;

		case AVR8_REG_C:
			return AVR8_DDRC;

		case AVR8_REG_D:
			return AVR8_DDRD;

		default:
			verboselog(machine, 0, "avr8_get_ddr: Unsupported register retrieval: %c\n", avr8_reg_name[reg]);
			break;
	}

	return 0;
}

static void avr8_change_ddr(running_machine *machine, int reg, UINT8 data)
{
    //craft_state *state = machine->driver_data<craft_state>();

	UINT8 oldddr = avr8_get_ddr(machine, reg);
	UINT8 newddr = data;
	UINT8 changed = newddr ^ oldddr;
	// TODO: When AVR8 is converted to emu/machine, this should be factored out to 8 single-bit callbacks per port
	if(changed)
	{
		// TODO
		verboselog(machine, 0, "avr8_change_port: DDR%c lines %02x changed\n", avr8_reg_name[reg], changed);
	}
}

static void avr8_change_port(running_machine *machine, int reg, UINT8 data)
{
    //craft_state *state = machine->driver_data<craft_state>();

	UINT8 oldport = avr8_get_ddr(machine, reg);
	UINT8 newport = data;
	UINT8 changed = newport ^ oldport;
	// TODO: When AVR8 is converted to emu/machine, this should be factored out to 8 single-bit callbacks per port
	if(changed)
	{
		// TODO
		verboselog(machine, 0, "avr8_change_port: PORT%c lines %02x changed\n", avr8_reg_name[reg], changed);
	}
}

namespace Avr8
{

enum
{
	INTIDX_SPI,
	INTIDX_ICF1,
	INTIDX_OCF1B,
	INTIDX_OCF1A,
	INTIDX_TOV1,

	INTIDX_COUNT,
};

class CInterruptCondition
{
	public:
		UINT8 m_intindex;
		UINT8 m_regindex;
		UINT8 m_regmask;
		UINT8 m_regshift;
		UINT8* mp_reg;
};

static const CInterruptCondition s_int_conditions[INTIDX_COUNT] =
{
	{ INTIDX_SPI,	REGIDX_SPSR,	AVR8_SPSR_SPIF_MASK,	AVR8_SPSR_SPIF_SHIFT,	NULL },
	{ INTIDX_ICF1,	REGIDX_TIFR1,	AVR8_TIFR1_ICF1_MASK,	AVR8_TIFR1_ICF1_SHIFT,	NULL },
	{ INTIDX_OCF1B,	REGIDX_TIFR1,	AVR8_TIFR1_OCF1B_MASK,	AVR8_TIFR1_OCF1B_SHIFT,	NULL },
	{ INTIDX_OCF1A,	REGIDX_TIFR1,	AVR8_TIFR1_OCF1A_MASK,	AVR8_TIFR1_OCF1A_SHIFT,	NULL },
	{ INTIDX_TOV1,	REGIDX_TIFR1,	AVR8_TIFR1_TOV1_MASK,	AVR8_TIFR1_TOV1_SHIFT,	NULL }
};

}

static void avr8_interrupt_update(running_machine *machine, int source)
{
	// TODO
	verboselog(machine, 0, "avr8_interrupt_update: TODO; source interrupt is %d\n", source);
}

/****************/
/* SPI Handling */
/****************/

static void avr8_enable_spi(running_machine *machine)
{
	// TODO
	verboselog(machine, 0, "avr8_enable_spi: TODO\n");
}

static void avr8_disable_spi(running_machine *machine)
{
	// TODO
	verboselog(machine, 0, "avr8_disable_spi: TODO\n");
}

static void avr8_spi_update_masterslave_select(running_machine *machine)
{
	// TODO
    craft_state *state = machine->driver_data<craft_state>();
	verboselog(machine, 0, "avr8_spi_update_masterslave_select: TODO; AVR is %s\n", AVR8_SPCR_MSTR ? "Master" : "Slave");
}

static void avr8_spi_update_clock_polarity(running_machine *machine)
{
	// TODO
    craft_state *state = machine->driver_data<craft_state>();
	verboselog(machine, 0, "avr8_spi_update_clock_polarity: TODO; SCK is Active-%s\n", AVR8_SPCR_CPOL ? "Low" : "High");
}

static void avr8_spi_update_clock_phase(running_machine *machine)
{
	// TODO
    craft_state *state = machine->driver_data<craft_state>();
	verboselog(machine, 0, "avr8_spi_update_clock_phase: TODO; Sampling edge is %s\n", AVR8_SPCR_CPHA ? "Trailing" : "Leading");
}

static const UINT8 avr8_spi_clock_divisor[8] = { 4, 16, 64, 128, 2, 8, 32, 64 };

static void avr8_spi_update_clock_rate(running_machine *machine)
{
	// TODO
    craft_state *state = machine->driver_data<craft_state>();
	verboselog(machine, 0, "avr8_spi_update_clock_rate: TODO; New clock rate should be f/%d\n", avr8_spi_clock_divisor[AVR8_SPCR_SPR] / (AVR8_SPSR_SPR2X ? 2 : 1));
}

static void avr8_change_spcr(running_machine *machine, UINT8 data)
{
    craft_state *state = machine->driver_data<craft_state>();

	UINT8 oldspcr = AVR8_SPCR;
	UINT8 newspcr = data;
	UINT8 changed = newspcr ^ oldspcr;
	UINT8 high_to_low = ~newspcr & oldspcr;
	UINT8 low_to_high = newspcr & ~oldspcr;

	AVR8_SPCR = data;

	if(changed & AVR8_SPCR_SPIE_MASK)
	{
		// Check for SPI interrupt condition
		avr8_interrupt_update(machine, Avr8::INTIDX_SPI);
	}

	if(low_to_high & AVR8_SPCR_SPE_MASK)
	{
		avr8_enable_spi(machine);
	}
	else if(high_to_low & AVR8_SPCR_SPE_MASK)
	{
		avr8_disable_spi(machine);
	}

	if(changed & AVR8_SPCR_MSTR_MASK)
	{
		avr8_spi_update_masterslave_select(machine);
	}

	if(changed & AVR8_SPCR_CPOL_MASK)
	{
		avr8_spi_update_clock_polarity(machine);
	}

	if(changed & AVR8_SPCR_CPHA_MASK)
	{
		avr8_spi_update_clock_phase(machine);
	}

	if(changed & AVR8_SPCR_SPR_MASK)
	{
		avr8_spi_update_clock_rate(machine);
	}
}

static void avr8_change_spsr(running_machine *machine, UINT8 data)
{
    craft_state *state = machine->driver_data<craft_state>();

	UINT8 oldspsr = AVR8_SPSR;
	UINT8 newspsr = data;
	UINT8 changed = newspsr ^ oldspsr;
	//UINT8 high_to_low = ~newspsr & oldspsr;
	//UINT8 low_to_high = newspsr & ~oldspsr;

	AVR8_SPSR &= ~1;
	AVR8_SPSR |= data & 1;

	if(changed & AVR8_SPSR_SPR2X_MASK)
	{
		avr8_spi_update_clock_rate(machine);
	}
}

/********************/
/* Timer 1 Handling */
/********************/

static void avr8_change_timsk1(running_machine *machine, UINT8 data)
{
    craft_state *state = machine->driver_data<craft_state>();

	UINT8 oldtimsk = AVR8_TIMSK1;
	UINT8 newtimsk = data;
	UINT8 changed = newtimsk ^ oldtimsk;

	if(changed & AVR8_TIMSK1_ICIE1_MASK)
	{
		// Check for Input Capture Interrupt interrupt condition
		avr8_interrupt_update(machine, Avr8::INTIDX_ICF1);
	}

	if(changed & AVR8_TIMSK1_OCIE1B_MASK)
	{
		// Check for Output Compare B Interrupt interrupt condition
		avr8_interrupt_update(machine, Avr8::INTIDX_OCF1B);
	}

	if(changed & AVR8_TIMSK1_OCIE1A_MASK)
	{
		// Check for Output Compare A Interrupt interrupt condition
		avr8_interrupt_update(machine, Avr8::INTIDX_OCF1A);
	}

	if(changed & AVR8_TIMSK1_TOIE1_MASK)
	{
		// Check for Output Compare A Interrupt interrupt condition
		avr8_interrupt_update(machine, Avr8::INTIDX_TOV1);
	}
}

static void avr8_update_timer1_compare_mode(running_machine *machine, int reg)
{
	// TODO
    craft_state *state = machine->driver_data<craft_state>();
    UINT8 mode = (reg == AVR8_REG_A) ? AVR8_TCCR1A_COM1A : AVR8_TCCR1A_COM1B;
    switch(mode)
    {
		case 0:
			verboselog(machine, 0, "avr8_update_timer1_compare_mode: TODO; OCR1%c disconnected, normal port operation\n", avr8_reg_name[reg]);
			break;

		case 1:
			switch(AVR8_WGM1)
			{
				case Avr8::WGM1_NORMAL:
				case Avr8::WGM1_CTC_OCR:
				case Avr8::WGM1_CTC_ICR:
				case Avr8::WGM1_FAST_PWM_8:
				case Avr8::WGM1_FAST_PWM_9:
				case Avr8::WGM1_FAST_PWM_10:
				case Avr8::WGM1_PWM_8_PC:
				case Avr8::WGM1_PWM_9_PC:
				case Avr8::WGM1_PWM_10_PC:
				case Avr8::WGM1_PWM_PFC_ICR:
				case Avr8::WGM1_PWM_PC_ICR:
					verboselog(machine, 0, "avr8_update_timer1_compare_mode: TODO; Toggle OC1%c on compare match\n", avr8_reg_name[reg]);
					break;

				case Avr8::WGM1_FAST_PWM_ICR:
				case Avr8::WGM1_FAST_PWM_OCR:
				case Avr8::WGM1_PWM_PFC_OCR:
				case Avr8::WGM1_PWM_PC_OCR:
					verboselog(machine, 0, "avr8_update_timer1_compare_mode: TODO; Toggle OC1A on compare match, OC1B disconnected\n");
					break;
			}
			break;

		case 2:
			switch(AVR8_WGM1)
			{
				case Avr8::WGM1_NORMAL:
				case Avr8::WGM1_CTC_OCR:
				case Avr8::WGM1_CTC_ICR:
					verboselog(machine, 0, "avr8_update_timer1_compare_mode: TODO; Clear OC1%c on compare match\n", avr8_reg_name[reg]);
					break;

				case Avr8::WGM1_PWM_8_PC:
				case Avr8::WGM1_PWM_9_PC:
				case Avr8::WGM1_PWM_10_PC:
				case Avr8::WGM1_PWM_PFC_ICR:
				case Avr8::WGM1_PWM_PC_ICR:
				case Avr8::WGM1_PWM_PFC_OCR:
				case Avr8::WGM1_PWM_PC_OCR:
					verboselog(machine, 0, "avr8_update_timer1_compare_mode: TODO; Clear OC1%c on match when up-counting, set OC1%c on match when down-counting\n", avr8_reg_name[reg]);
					break;

				case Avr8::WGM1_FAST_PWM_8:
				case Avr8::WGM1_FAST_PWM_9:
				case Avr8::WGM1_FAST_PWM_10:
				case Avr8::WGM1_FAST_PWM_ICR:
				case Avr8::WGM1_FAST_PWM_OCR:
					verboselog(machine, 0, "avr8_update_timer1_compare_mode: TODO; Clear OC1%c on compare match, set OC1%c at BOTTOM\n", avr8_reg_name[reg]);
					break;
			}
			break;

		case 3:
			switch(AVR8_WGM1)
			{
				case Avr8::WGM1_NORMAL:
				case Avr8::WGM1_CTC_OCR:
				case Avr8::WGM1_CTC_ICR:
					verboselog(machine, 0, "avr8_update_timer1_compare_mode: TODO; Set OC1%c on compare match\n", avr8_reg_name[reg]);
					break;

				case Avr8::WGM1_PWM_8_PC:
				case Avr8::WGM1_PWM_9_PC:
				case Avr8::WGM1_PWM_10_PC:
				case Avr8::WGM1_PWM_PFC_ICR:
				case Avr8::WGM1_PWM_PC_ICR:
				case Avr8::WGM1_PWM_PFC_OCR:
				case Avr8::WGM1_PWM_PC_OCR:
					verboselog(machine, 0, "avr8_update_timer1_compare_mode: TODO; Set OC1%c on match when up-counting, clear OC1%c on match when down-counting\n", avr8_reg_name[reg]);
					break;

				case Avr8::WGM1_FAST_PWM_8:
				case Avr8::WGM1_FAST_PWM_9:
				case Avr8::WGM1_FAST_PWM_10:
				case Avr8::WGM1_FAST_PWM_ICR:
				case Avr8::WGM1_FAST_PWM_OCR:
					verboselog(machine, 0, "avr8_update_timer1_compare_mode: TODO; Set OC1%c on compare match, clear OC1%c at BOTTOM\n", avr8_reg_name[reg]);
					break;
			}
			break;
	}
}

static void avr8_update_timer1_waveform_gen_mode(running_machine *machine)
{
	// TODO
	UINT16 top = 0;
    craft_state *state = machine->driver_data<craft_state>();
	verboselog(machine, 0, "avr8_update_timer1_waveform_gen_mode: TODO; WGM1 is %d\n", AVR8_WGM1 );
	switch(AVR8_WGM1)
	{
		case Avr8::WGM1_NORMAL:
			top = 0xffff;
			break;

		case Avr8::WGM1_PWM_8_PC:
		case Avr8::WGM1_FAST_PWM_8:
			top = 0x00ff;
			break;

		case Avr8::WGM1_PWM_9_PC:
		case Avr8::WGM1_FAST_PWM_9:
			top = 0x01ff;
			break;

		case Avr8::WGM1_PWM_10_PC:
		case Avr8::WGM1_FAST_PWM_10:
			top = 0x03ff;
			break;

		case Avr8::WGM1_PWM_PFC_ICR:
		case Avr8::WGM1_PWM_PC_ICR:
		case Avr8::WGM1_CTC_ICR:
		case Avr8::WGM1_FAST_PWM_ICR:
			top = AVR8_ICR1;
			break;

		case Avr8::WGM1_PWM_PFC_OCR:
		case Avr8::WGM1_PWM_PC_OCR:
		case Avr8::WGM1_CTC_OCR:
		case Avr8::WGM1_FAST_PWM_OCR:
			top = AVR8_OCR1A;
			break;

		default:
			verboselog(machine, 0, "avr8_update_timer1_waveform_gen_mode: Unsupported waveform generation type: %d\n", AVR8_WGM1);
			break;
	}
}

static void avr8_changed_tccr1a(running_machine *machine, UINT8 data)
{
    craft_state *state = machine->driver_data<craft_state>();

	UINT8 oldtccr = AVR8_TCCR1A;
	UINT8 newtccr = data;
	UINT8 changed = newtccr ^ oldtccr;

	if(changed & AVR8_TCCR1A_COM1A_MASK)
	{
		// TODO
		avr8_update_timer1_compare_mode(machine, AVR8_REG_A);
	}

	if(changed & AVR8_TCCR1A_COM1B_MASK)
	{
		// TODO
		avr8_update_timer1_compare_mode(machine, AVR8_REG_B);
	}

	if(changed & AVR8_TCCR1A_WGM1_10)
	{
		// TODO
		avr8_update_timer1_waveform_gen_mode(machine);
	}
}

static void avr8_update_timer1_input_noise_canceler(running_machine *machine)
{
}

static void avr8_update_timer1_input_edge_select(running_machine *machine)
{
	// TODO
    //craft_state *state = machine->driver_data<craft_state>();
	verboselog(machine, 0, "avr8_update_timer1_input_edge_select: TODO; Clocking edge is %s\n", "test");
}

static void avr8_update_timer1_clock_source(running_machine *machine)
{
}

static void avr8_changed_tccr1b(running_machine *machine, UINT8 data)
{
    craft_state *state = machine->driver_data<craft_state>();

	UINT8 oldtccr = AVR8_TCCR1B;
	UINT8 newtccr = data;
	UINT8 changed = newtccr ^ oldtccr;

	if(changed & AVR8_TCCR1B_ICNC1_MASK)
	{
		// TODO
		avr8_update_timer1_input_noise_canceler(machine);
	}

	if(changed & AVR8_TCCR1B_ICES1_MASK)
	{
		// TODO
		avr8_update_timer1_input_edge_select(machine);
	}

	if(changed & AVR8_TCCR1B_WGM1_32_MASK)
	{
		// TODO
		avr8_update_timer1_waveform_gen_mode(machine);
	}

	if(changed & AVR8_TCCR1B_CS_MASK)
	{
		// TODO
		avr8_update_timer1_clock_source(machine);
	}
}

static void avr8_update_ocr1(running_machine *machine, UINT16 newval, UINT8 reg)
{
    craft_state *state = machine->driver_data<craft_state>();
	UINT8 *p_reg_h = (reg == AVR8_REG_A) ? &AVR8_OCR1AH : &AVR8_OCR1BH;
	UINT8 *p_reg_l = (reg == AVR8_REG_A) ? &AVR8_OCR1AL : &AVR8_OCR1BL;
	*p_reg_h = (UINT8)(newval >> 8);
	*p_reg_l = (UINT8)newval;
	// TODO
	verboselog(machine, 0, "avr8_update_ocr1: TODO: new OCR1%c = %04x\n", avr8_reg_name[reg], newval);
}

static WRITE8_HANDLER( avr8_write )
{
    craft_state *state = space->machine->driver_data<craft_state>();

	if(offset <= Avr8::REGIDX_R31)
	{
		state->regs[offset] = data;
		return;
	}

    switch( offset )
    {
		case Avr8::REGIDX_OCR1BH:
			avr8_update_ocr1(space->machine, (AVR8_OCR1B & 0x00ff) | (data << 8), AVR8_REG_B);
			break;

		case Avr8::REGIDX_OCR1BL:
			avr8_update_ocr1(space->machine, (AVR8_OCR1B & 0xff00) | data, AVR8_REG_B);
			break;

		case Avr8::REGIDX_OCR1AH:
			avr8_update_ocr1(space->machine, (AVR8_OCR1A & 0x00ff) | (data << 8), AVR8_REG_A);
			break;

		case Avr8::REGIDX_OCR1AL:
			avr8_update_ocr1(space->machine, (AVR8_OCR1A & 0xff00) | data, AVR8_REG_A);
			break;

		case Avr8::REGIDX_TCCR1B:
			avr8_changed_tccr1b(space->machine, data);
			break;

		case Avr8::REGIDX_TCCR1A:
			avr8_changed_tccr1a(space->machine, data);
			break;

		case Avr8::REGIDX_TIMSK1:
			avr8_change_timsk1(space->machine, data);
			break;

		case Avr8::REGIDX_SPL:
		case Avr8::REGIDX_SPH:
		case Avr8::REGIDX_SREG:
			state->regs[offset] = data;
			break;

		case Avr8::REGIDX_SPSR:
			avr8_change_spsr(space->machine, data);
			break;

		case Avr8::REGIDX_SPCR:
			avr8_change_spcr(space->machine, data);
			break;

		case Avr8::REGIDX_DDRD:
			avr8_change_ddr(space->machine, AVR8_REG_D, data);
			break;

		case Avr8::REGIDX_DDRC:
			avr8_change_ddr(space->machine, AVR8_REG_C, data);
			break;

		case Avr8::REGIDX_PORTB:
			avr8_change_port(space->machine, AVR8_REG_B, data);
			break;

		case Avr8::REGIDX_DDRB:
			avr8_change_ddr(space->machine, AVR8_REG_B, data);
			break;

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
    craft_state *state = machine->driver_data<craft_state>();

    state->ocr1a_timer = timer_alloc(machine, ocr1a_timer_compare, 0);
    state->ocr1b_timer = timer_alloc(machine, ocr1b_timer_compare, 0);
}

static MACHINE_RESET( craft )
{
    craft_state *state = machine->driver_data<craft_state>();

    AVR8_TIMSK1 = 0;
    AVR8_OCR1AH = 0;
    AVR8_OCR1AL = 0;
    AVR8_OCR1BH = 0;
    AVR8_OCR1BL = 0;
    AVR8_ICR1H = 0;
    AVR8_ICR1L = 0;
    AVR8_TCNT1H = 0;
    AVR8_TCNT1L = 0;

    dac_data_w(machine->device("dac"), 0x00);
}

static MACHINE_CONFIG_START( craft, craft_state )

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
MACHINE_CONFIG_END

ROM_START( craft )
	ROM_REGION( 0x2000, "maincpu", 0 )  /* Main program store */
	ROM_LOAD( "craft.bin", 0x0000, 0x2000, CRC(2e6f9ad2) SHA1(75e495bf18395d74289ca7ee2649622fc4010457) )
	ROM_REGION( 0x200, "eeprom", 0 )  /* on-die eeprom */
	ROM_LOAD( "eeprom.raw", 0x0000, 0x0200, CRC(e18a2af9) SHA1(81fc6f2d391edfd3244870214fac37929af0ac0c) )
ROM_END

/*   YEAR  NAME      PARENT    COMPAT    MACHINE   INPUT     INIT      COMPANY          FULLNAME */
CONS(2008, craft,    0,        0,        craft,    craft,    craft,    "Linus Akesson", "Craft", GAME_NO_SOUND | GAME_NOT_WORKING)
