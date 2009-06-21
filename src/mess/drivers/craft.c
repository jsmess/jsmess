/*
    Linus Akesson's "Craft"

    Skeleton driver by MooglyGuy
*/

#include "driver.h"
#include "cpu/avr8/avr8.h"

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

    UINT8 spl;
    UINT8 sph;
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

    UINT8 tcnt1l;
    UINT8 tcnt1h;
    UINT8 icr1l;
    UINT8 icr1h;
    UINT8 ocr1al;
    UINT8 ocr1ah;
    UINT8 ocr1bl;
    UINT8 ocr1bh;

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
} avr8_regs;

avr8_regs regs;

static READ8_HANDLER( avr8_read )
{
    switch( offset )
    {
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
            return regs.r[offset];
        case 0x24:
            printf( "AVR8: R: regs.ddrb (%02x)\n", regs.ddrb );
            return regs.ddrb;
        case 0x25:
            printf( "AVR8: R: regs.portb (%02x)\n", regs.portb );
            return regs.portb;
        case 0x27:
            printf( "AVR8: R: regs.ddrc (%02x)\n", regs.ddrc );
            return regs.ddrc;
        case 0x2a:
            printf( "AVR8: R: regs.ddrd (%02x)\n", regs.ddrd );
            return regs.ddrd;
        case 0x4c:
            printf( "AVR8: R: regs.spcr (%02x)\n", regs.spcr );
            return regs.spcr;
        case 0x4d:
            printf( "AVR8: R: regs.spsr (%02x)\n", regs.spsr );
            return regs.spsr;
        case 0x5d:
            //printf( "AVR8: R: regs.spl (%02x)\n", regs.spl );
            return regs.spl;
        case 0x5e:
            //printf( "AVR8: R: regs.sph (%02x)\n", regs.sph );
            return regs.sph;
        case 0x5f:
            return regs.sreg;
        case 0x6f:
            printf( "AVR8: R: regs.timsk1 (%02x)\n", regs.timsk1 );
            return regs.timsk1;
        case 0x80:
            printf( "AVR8: R: regs.tccr1a (%02x)\n", regs.tccr1a );
            return regs.tccr1a;
        case 0x81:
            printf( "AVR8: R: regs.tccr1b (%02x)\n", regs.tccr1b );
            return regs.tccr1b;
        case 0x88:
            printf( "AVR8: R: regs.ocr1al (%02x)\n", regs.ocr1al );
            return regs.ocr1al;
        case 0x89:
            printf( "AVR8: R: regs.ocr1ah (%02x)\n", regs.ocr1ah );
            return regs.ocr1ah;
        case 0x8a:
            printf( "AVR8: R: regs.ocr1bl (%02x)\n", regs.ocr1bl );
            return regs.ocr1bl;
        case 0x8b:
            printf( "AVR8: R: regs.ocr1bh (%02x)\n", regs.ocr1bh );
            return regs.ocr1bh;
        case 0xb0:
            printf( "AVR8: R: regs.tccr2a (%02x)\n", regs.tccr2a );
            return regs.tccr2a;
        case 0xb1:
            printf( "AVR8: R: regs.tccr2b (%02x)\n", regs.tccr2b );
            return regs.tccr2b;
        default:
            printf( "AVR8: Unrecognized register read: %02x\n", offset );
    }
    return 0;
}

static WRITE8_HANDLER( avr8_write )
{
    switch( offset )
    {
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
            regs.r[offset] = data;
            break;
        case 0x24:
            printf( "AVR8: W: regs.ddrb = %02x\n", data );
            regs.ddrb = data;
            break;
        case 0x25:
            printf( "AVR8: W: regs.portb = %02x\n", data );
            regs.portb = data;
            break;
        case 0x27:
            printf( "AVR8: W: regs.ddrc = %02x\n", data );
            regs.ddrc = data;
            break;
        case 0x2a:
            printf( "AVR8: W: regs.ddrd = %02x\n", data );
            regs.ddrd = data;
            break;
        case 0x4c:
            printf( "AVR8: W: regs.spcr = %02x\n", data );
            regs.spcr = data;
            break;
        case 0x4d:
            printf( "AVR8: W: regs.spsr = %02x\n", data );
            regs.spsr = data;
            break;
        case 0x5d:
            //printf( "AVR8: W: regs.spl = %02x\n", data );
            regs.spl = data;
            break;
        case 0x5e:
            //printf( "AVR8: W: regs.sph = %02x\n", data );
            regs.sph = data & 0x07;
            break;
        case 0x5f:
            regs.sreg = data;
            break;
        case 0x6f:
            printf( "AVR8: W: regs.timsk1 = %02x\n", data );
            regs.timsk1 = data;
            break;
        case 0x80:
            printf( "AVR8: W: regs.tccr1a = %02x\n", data );
            regs.tccr1a = data;
            break;
        case 0x81:
            printf( "AVR8: W: regs.tccr1b = %02x\n", data );
            regs.tccr1b = data;
            break;
        case 0x88:
            printf( "AVR8: W: regs.ocr1al = %02x\n", data );
            regs.ocr1al = data;
            break;
        case 0x89:
            printf( "AVR8: W: regs.ocr1ah = %02x\n", data );
            regs.ocr1ah = data;
            break;
        case 0x8a:
            printf( "AVR8: W: regs.ocr1bl = %02x\n", data );
            regs.ocr1bl = data;
            break;
        case 0x8b:
            printf( "AVR8: W: regs.ocr1bh = %02x\n", data );
            regs.ocr1bh = data;
            break;
        case 0xb0:
            printf( "AVR8: W: regs.tccr2a = %02x\n", data );
            regs.tccr2a = data;
            break;
        case 0xb1:
            printf( "AVR8: W: regs.tccr2b = %02x\n", data );
            regs.tccr2b = data;
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

static MACHINE_RESET( craft )
{
}

static MACHINE_DRIVER_START( craft )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", AVR8, 20000000)
    MDRV_CPU_PROGRAM_MAP(craft_prg_map)
    MDRV_CPU_IO_MAP(craft_io_map)

    MDRV_MACHINE_RESET(craft)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 479)
    MDRV_PALETTE_LENGTH(0x1000)

    MDRV_VIDEO_START(generic_bitmapped)
    MDRV_VIDEO_UPDATE(craft)
MACHINE_DRIVER_END

ROM_START( craft )
	ROM_REGION( 0x2000, "maincpu", 0 )  /* Main program store */
	//ROM_LOAD( "craft.bin", 0x0000, 0x2000, CRC(12345678) SHA1(1234567812345678123456781234567812345678) )
	/* This rom "eeprom.raw" was found on the author's site - feel free to make corrections */
	ROM_LOAD( "craft.bin", 0x0000, 0x0200, CRC(e18a2af9) SHA1(81fc6f2d391edfd3244870214fac37929af0ac0c) )
ROM_END

/*   YEAR  NAME      PARENT    COMPAT    MACHINE   INPUT     INIT      CONFIG    COMPANY          FULLNAME */
CONS(2008, craft,    0,        0,        craft,    craft,    0,        0,        "Linus Akesson", "Craft", GAME_NO_SOUND | GAME_NOT_WORKING)
