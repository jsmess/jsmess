/*
    Linus Akesson's "Craft"

    Skeleton driver by MooglyGuy
*/

#include "emu.h"
#include "cpu/avr8/avr8.h"
#include "sound/dac.h"

#define MASTER_CLOCK 20000000

/****************************************************\
* I/O devices                                        *
\****************************************************/

typedef struct
{
    UINT8 r[0x20];

    UINT8 res0[3];

    UINT8 pinb;
    UINT8 ddrb;
    UINT8 portb;
    UINT8 pinc;
    UINT8 ddrc;
    UINT8 portc;
    UINT8 pind;
    UINT8 ddrd;
    UINT8 portd;

    UINT8 res1[9];

    UINT8 tifr0;
    UINT8 tifr1;
    UINT8 tifr2;

    UINT8 res2[3];

    UINT8 pcifr;
    UINT8 eifr;
    UINT8 eimsk;
    UINT8 gpior0;
    UINT8 eecr;
    UINT8 eedr;
    UINT8 eearl;
    UINT8 eearh;
    UINT8 gtccr;
    UINT8 tccr0a;
    UINT8 tccr0b;
    UINT8 tcnt0;
    UINT8 ocr0a;
    UINT8 ocr0b;

    UINT8 res3;

    UINT8 gpior1;
    UINT8 gpior2;
    UINT8 spcr;
    UINT8 spsr;
    UINT8 spdr;

    UINT8 res4;

    UINT8 acsr;

    UINT8 res5[2];

    UINT8 smcr;
    UINT8 mcusr;
    UINT8 mcucr;

    UINT8 res6;

    UINT8 spmcsr;

    UINT8 res7[5];

    UINT16 sp;
    UINT8 sreg;

    UINT8 wdtcsr;
    UINT8 clkpr;

    UINT8 res8[2];

    UINT8 prr;

    UINT8 res9;

    UINT8 osccal;

    UINT8 res10;

    UINT8 pcicr;
    UINT8 eicra;

    UINT8 res11;

    UINT8 pcmsk0;
    UINT8 pcmsk1;
    UINT8 pcmsk2;
    UINT8 timsk0;
    UINT8 timsk1;
    UINT8 timsk2;

    UINT8 res12[7];

    UINT8 adcl;
    UINT8 adch;
    UINT8 adcsra;
    UINT8 adcsrb;
    UINT8 admux;

    UINT8 res13;

    UINT8 didr0;
    UINT8 didr1;
    UINT8 tccr1a;
    UINT8 tccr1b;
    UINT8 tccr1c;

    UINT8 res14;

    UINT16 tcnt1;
    UINT16 icr1;
    UINT16 ocr1a;
    UINT16 ocr1b;

    UINT8 res15[36];

    UINT8 tccr2a;
    UINT8 tccr2b;
    UINT8 tcnt2;
    UINT8 ocr2a;
    UINT8 ocr2b;

    UINT8 res16;

    UINT8 assr;

    UINT8 res17;

    UINT8 twbr;
    UINT8 twsr;
    UINT8 twar;
    UINT8 twdr;
    UINT8 twcr;
    UINT8 twamr;

    UINT8 res18[2];

    UINT8 ucsr0a;
    UINT8 ucsr0b;
    UINT8 ucsr0c;

    UINT8 res19;

    UINT8 ubrr0l;
    UINT8 ubrr0h;
    UINT8 udr0;

    UINT8 res20[57];

    UINT8 hidden_high;

    INT64 last_tcnt1_cycle;

    emu_timer *ocr1a_timer;
    emu_timer *ocr1b_timer;
} avr8_regs;


class craft_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, craft_state(machine)); }

	craft_state(running_machine &machine) { }

	avr8_regs regs;
	int blah;
};

#define AVR8_TIMSK1_OCIE1A  0x02
#define AVR8_TIMSK1_OCIE1B  0x04

static UINT64 avr8_get_timer_1_frequency(avr8_regs *device)
{
    UINT64 frequency = 0;
    switch(device->tccr1b & 0x07)
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
            printf( "device->tccr1b & 0x07 returned a value not in the range of 0 to 7.  This is\n" );
            printf( "impossible.  Please ensure that your walls are not bleeding and your bathroom\n" );
            printf( "has not turned into a psychokinetic vortex.\n" );
            break;
    }

    return frequency;
}

static void avr8_maybe_start_timer_1(avr8_regs *device)
{
    UINT64 frequency = avr8_get_timer_1_frequency(device);
    if(frequency == 0)
    {
        timer_adjust_oneshot(device->ocr1a_timer, attotime_never, 0);
        timer_adjust_oneshot(device->ocr1b_timer, attotime_never, 0);
    }
    else
    {
        UINT16 wrap_count_a = (UINT16)(((UINT32)device->ocr1a - (UINT32)device->tcnt1) & 0x0000ffff);
        UINT16 wrap_count_b = (UINT16)(((UINT32)device->ocr1b - (UINT32)device->tcnt1) & 0x0000ffff);
        attotime period_a = attotime_mul(ATTOTIME_IN_HZ(frequency), wrap_count_a);
        attotime period_b = attotime_mul(ATTOTIME_IN_HZ(frequency), wrap_count_b);
        if(device->timsk1 & AVR8_TIMSK1_OCIE1A)
        {
            timer_adjust_oneshot(device->ocr1a_timer, period_a, 0);
        }
        if(device->timsk1 & AVR8_TIMSK1_OCIE1B)
        {
            timer_adjust_oneshot(device->ocr1b_timer, period_b, 0);
        }
    }
}

static TIMER_CALLBACK( ocr1a_timer_compare )
{
    craft_state *state = (craft_state *)machine->driver_data;
    avr8_regs *regs = &state->regs;

    printf( "ocr1a_timer_compare %d\n", state->blah++ );
    avr8_maybe_start_timer_1(regs);

    // Inaccurate
    cpu_set_input_line(devtag_get_device(machine, "maincpu"), AVR8_INT_T1COMPA, ASSERT_LINE);
}

static TIMER_CALLBACK( ocr1b_timer_compare )
{
    craft_state *state = (craft_state *)machine->driver_data;
    avr8_regs *regs = &state->regs;

    printf( "ocr1b_timer_compare\n" );
    avr8_maybe_start_timer_1(regs);

    // Inaccurate
    cpu_set_input_line(devtag_get_device(machine, "maincpu"), AVR8_INT_T1COMPB, ASSERT_LINE);
}

static READ8_HANDLER( avr8_read )
{
    craft_state *state = (craft_state *)space->machine->driver_data;
    avr8_regs *regs = &state->regs;

    switch( offset )
    {
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
            return regs->r[offset];
        case 0x24:
            printf( "AVR8: R: regs->ddrb (%02x)\n", regs->ddrb );
            return regs->ddrb;
        case 0x25:
            printf( "AVR8: R: regs->portb (%02x)\n", regs->portb );
            return regs->portb;
        case 0x27:
            printf( "AVR8: R: regs->ddrc (%02x)\n", regs->ddrc );
            return regs->ddrc;
        case 0x2a:
            printf( "AVR8: R: regs->ddrd (%02x)\n", regs->ddrd );
            return regs->ddrd;
        case 0x4c:
            printf( "AVR8: R: regs->spcr (%02x)\n", regs->spcr );
            return regs->spcr;
        case 0x4d:
            printf( "AVR8: R: regs->spsr (%02x)\n", regs->spsr );
            return regs->spsr;
        case 0x5d:
            //printf( "AVR8: R: regs->spl (%02x)\n", regs->spl );
            return regs->sp & 0x00ff;
        case 0x5e:
            //printf( "AVR8: R: regs->sph (%02x)\n", regs->sph );
            return (regs->sp >> 8) & 0x00ff;
        case 0x5f:
            return regs->sreg;
        case 0x6f:
            printf( "AVR8: R: regs->timsk1 (%02x)\n", regs->timsk1 );
            return regs->timsk1;
        case 0x80:
            printf( "AVR8: R: regs->tccr1a (%02x)\n", regs->tccr1a );
            return regs->tccr1a;
        case 0x81:
            printf( "AVR8: R: regs->tccr1b (%02x)\n", regs->tccr1b );
            return regs->tccr1b;
        case 0x84:
        {
            //INT64 divisor = 1;
#if 0
            INT64 update = (INT64)(cpu_get_total_cycles(space->cpu)) - last_tcnt1_cycle;
            switch(device->tccr1b & 0x07)
            {
                case 0: // No clock source (Timer/Counter stopped).
                case 1: // clk / 1 (No prescaling)
                case 6: // External clock source on T1 pin.  Clock on falling edge.
                case 7: // External clock source on T1 pin.  Clock on rising edge.
                    divisor = 1;
                    break;

                case 2: // clk / 8 (From prescaler)
                    divisor = 8;
                    break;

                case 3: // clk / 64 (From prescaler)
                    divisor = 64;
                    break;

                case 4: // clk / 256 (From prescaler)
                    divisor = 256;
                    break;

                case 5: // clk / 1024 (From prescaler)
                    divisor = 1024;
                    break;

                default:
                    divisor = 1;
                    printf( "device->tccr1b & 0x07 returned a value not in the range of 0 to 7.  This is\n" );
                    printf( "impossible.  Please ensure that your walls are not bleeding and your bathroom\n" );
                    printf( "has not turned into a psychokinetic vortex.\n" );
                    break;
            }
            diff /= divisor;
            diff %= regs->ocr1a;
            printf( "AVR8: R: regs->tcnt1l read: %04x\n",  );
#endif
            break;
        }

        case 0x85:
            printf( "AVR8: R: regs->tcnt1h\n" );
            break;
        case 0x88:
            printf( "AVR8: R: regs->ocr1al (%02x)\n", regs->ocr1a & 0x00ff );
            return regs->ocr1a & 0x00ff;
        case 0x89:
            printf( "AVR8: R: regs->ocr1ah (%02x)\n", (regs->ocr1a >> 8) & 0x00ff );
            return (regs->ocr1a >> 8) & 0x00ff;
        case 0x8a:
            printf( "AVR8: R: regs->ocr1bl (%02x)\n", regs->ocr1b & 0x00ff );
            return regs->ocr1b & 0x00ff;
        case 0x8b:
            printf( "AVR8: R: regs->ocr1bh (%02x)\n", (regs->ocr1b >> 8) & 0x00ff );
            return (regs->ocr1b >> 8) & 0x00ff;
        case 0xb0:
            printf( "AVR8: R: regs->tccr2a (%02x)\n", regs->tccr2a );
            return regs->tccr2a;
        case 0xb1:
            printf( "AVR8: R: regs->tccr2b (%02x)\n", regs->tccr2b );
            return regs->tccr2b;
        default:
            printf( "AVR8: Unrecognized register read: %02x\n", offset );
    }
    return 0;
}

static WRITE8_HANDLER( avr8_write )
{
    craft_state *state = (craft_state *)space->machine->driver_data;
    avr8_regs *regs = &state->regs;

    switch( offset )
    {
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
            regs->r[offset] = data;
            break;
        case 0x24:
            printf( "AVR8: W: regs->ddrb = %02x\n", data );
            regs->ddrb = data;
            break;
        case 0x25:
            printf( "AVR8: W: regs->portb = %02x\n", data );
            regs->portb = data;
            break;
        case 0x27:
            printf( "AVR8: W: regs->ddrc = %02x\n", data );
            regs->ddrc = data;
            break;
        case 0x2a:
            printf( "AVR8: W: regs->ddrd = %02x\n", data );
            regs->ddrd = data;
            break;
        case 0x2b:
            dac_data_w(devtag_get_device(space->machine, "dac"), data);
            break;
        case 0x4c:
            printf( "AVR8: W: regs->spcr = %02x\n", data );
            regs->spcr = data;
            break;
        case 0x4d:
            printf( "AVR8: W: regs->spsr = %02x\n", data );
            regs->spsr = data;
            break;
        case 0x5d:
            //printf( "AVR8: W: regs->sp(L) = %02x\n", data );
            regs->sp = (regs->hidden_high << 8) | data;
            break;
        case 0x5e:
            //printf( "AVR8: W: regs->sp(H) = %02x\n", data );
            regs->hidden_high = data & 0x07;
            break;
        case 0x5f:
            regs->sreg = data;
            break;
        case 0x6f:
            printf( "AVR8: W: regs->timsk1 = %02x\n", data );
            regs->timsk1 = data;
            avr8_maybe_start_timer_1(regs);
            break;
        case 0x80:
            printf( "AVR8: W: regs->tccr1a = %02x\n", data );
            regs->tccr1a = data;
            break;
        case 0x81:
            printf( "AVR8: W: regs->tccr1b = %02x\n", data );
            regs->tccr1b = data;
            break;
        case 0x88:
            printf( "AVR8: W: regs->ocr1a(L) = %02x\n", data );
            regs->ocr1a = (regs->hidden_high << 8) | data;
            avr8_maybe_start_timer_1(regs);
            break;
        case 0x89:
            printf( "AVR8: W: regs->ocr1a(H) = %02x\n", data );
            regs->hidden_high = data;
            break;
        case 0x8a:
            printf( "AVR8: W: regs->ocr1b(L) = %02x\n", data );
            regs->ocr1b = (regs->hidden_high << 8) | data;
            avr8_maybe_start_timer_1(regs);
            break;
        case 0x8b:
            printf( "AVR8: W: regs->ocr1b(H) = %02x\n", data );
            regs->hidden_high = data;
            break;
        case 0xb0:
            printf( "AVR8: W: regs->tccr2a = %02x\n", data );
            regs->tccr2a = data;
            break;
        case 0xb1:
            printf( "AVR8: W: regs->tccr2b = %02x\n", data );
            regs->tccr2b = data;
            break;
        default:
            printf( "AVR8: Unrecognized register write: %02x = %02x\n", offset, data );
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
    avr8_regs *regs = &state->regs;

    regs->ocr1a_timer = timer_alloc(machine, ocr1a_timer_compare, 0);
    regs->ocr1b_timer = timer_alloc(machine, ocr1b_timer_compare, 0);
}

static MACHINE_RESET( craft )
{
    craft_state *state = (craft_state *)machine->driver_data;
    avr8_regs *regs = &state->regs;

    regs->timsk1 = 0;
    regs->tcnt1 = 0;
    regs->ocr1a = 0;
    regs->ocr1b = 0;
    regs->icr1 = 0;
    regs->last_tcnt1_cycle = 0;

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
