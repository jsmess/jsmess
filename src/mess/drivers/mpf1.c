/************************************************\
* Multitech Micro Professor 1                    *
*                                                *
*     CPU: Z80 @ 1.79 MHz                        *
*     ROM: 4-kilobyte ROM monitor                *
*     RAM: 4 kilobytes                           *
*   Input: Hex keypad                            *
* Storage: Cassette tape                         *
*   Video: 6x 7-segment LED display              *
*   Sound: Speaker                               *
*                                                *
* TODO:                                          *
*    Cassette storage emulation                  *
*    Sound                                       *
*    I/O board                                   *
*    MONI/INTR key interrupts                    *
*    PIO/CTC + interrupt daisy chain             *
\************************************************/

/*

    Artwork picture downloaded from:

    http://members.lycos.co.uk/leeedavison/z80/mpf1/

*/

#include "driver.h"
#include "machine/i8255a.h"
#include "cpu/z80/z80.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "sound/dac.h"
#include "mpf1.lh"


#define VERBOSE_LEVEL ( 0 )

INLINE void ATTR_PRINTF(2,3) verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
//		logerror( "%08x: %s", cpu_get_pc(space->cpu), buf );
	}
}


static emu_timer *led_refresh_timer;
static INT8 lednum;
static int led_tone;
static int led_halt;
static INT8 keycol;
static UINT8 kbdlatch;


static TIMER_CALLBACK( check_halt_callback )
{
	// halt-LED; the red one, is turned on when the processor is halted
	// TODO: processor seems to halt, but restarts(?) at 0x0000 after a while -> fix
	led_halt = (UINT8) devtag_get_info_int(machine, "maincpu", CPUINFO_INT_REGISTER + Z80_HALT);
	set_led_status(1, led_tone);
}


/* Memory Maps */

static ADDRESS_MAP_START( mpf1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0x1fff) AM_RAM
	AM_RANGE(0x2000, 0x2fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mpf1p_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mpf1_io_map, ADDRESS_SPACE_IO, 8 )
    /* Appendix B.D from the MPF-I user's manual:
       (contains possible typing/printing errors so I've cited it literally)

    * D. Input/Output port addressing
    *
    *    U96 (74LS139) is an I/O port decoder.
    *
    *           IORQ    A7  A6   Selected I/O    Port Address
    *
    *            0      0   0       8255          00 - 03
    *
    *            0      0   1       CTC           40 - 43
    *
    *            0      1   0       PIO           80 - 83
    *
    *      Note; I/O port is not fully decoded, e.g. the 16 combinations
    *            00 - 03,  04 - 07, 08 - 0B,.....3C - 3F, all select the s
    *            8255.  The CTC & PIO are also selected by 16 different
    *            combinations.

       Where the text states ".... , all select the s ...." it probably means
       "... ., all select the same ....".

       So to assure that this "incompleteness" of the hardware does also exist in
       this simulator I've expanded the port assignments accordingly. I've also
       tested whether this is true for the actual hardware, and it is.
    */
	ADDRESS_MAP_GLOBAL_MASK(0xff)

	// The 16 I/O port combinations for the 8255 (P8255A-5, 8628LLP, (c) 1981 AMD)
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE("ppi8255", i8255a_r, i8255a_w) AM_MIRROR(0x3C)

//  TODO: create drivers to emulate the following two chips
//  The 16 I/O port combinations for the CTC (Zilog, Z0843004PSC, Z80 CTC, 8644)
//  AM_RANGE(0x40, 0x43) AM_WRITE(ctc_enable_w) AM_MIRROR(0x7C)

	/* The 16 I/O port combinations for the PIO (Zilog, Z0842004PSC, Z80 PIO, 8735) */
	AM_RANGE(0x80, 0x83) AM_DEVREADWRITE("z80pio", z80pio_r, z80pio_w) AM_MIRROR(0xBF)

ADDRESS_MAP_END

/* Input Ports */

#define KEY_UNUSED( bit ) \
	PORT_BIT( bit, IP_ACTIVE_LOW, IPT_UNUSED )

static INPUT_PORTS_START( mpf1 )
	PORT_START("LINE0")	// column 0
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 HL") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 HL'") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B I*IF") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F *PNC'") PORT_CODE(KEYCODE_F)
	KEY_UNUSED( 0x10 )
	KEY_UNUSED( 0x20 )
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START("LINE1")	// column 1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 DE") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 DE'") PORT_CODE(KEYCODE_6)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A SP") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E SZ*H'") PORT_CODE(KEYCODE_E)
	KEY_UNUSED( 0x10 )
	KEY_UNUSED( 0x20 )
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START("LINE2")	// column 2
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 BC") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 BC'") PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 IY") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D *PNC") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("STEP") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAPE RD") PORT_CODE(KEYCODE_F5)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START("LINE3")	// column 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 AF") PORT_CODE(KEYCODE_0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 AF'") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 IX") PORT_CODE(KEYCODE_8)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C SZ*H") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GO") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAPE WR") PORT_CODE(KEYCODE_F6)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START("LINE4")	// column 4
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CBR") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PC") PORT_CODE(KEYCODE_M)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("REG") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ADR") PORT_CODE(KEYCODE_STOP)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RELA") PORT_CODE(KEYCODE_RCONTROL)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START("LINE5")	// column 5
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SBR") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_J)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DATA") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INS") PORT_CODE(KEYCODE_COLON)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MOVE") PORT_CODE(KEYCODE_QUOTE)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START("LINE6")	// column 6
	KEY_UNUSED( 0x01 )
	KEY_UNUSED( 0x02 )
	KEY_UNUSED( 0x04 )
	KEY_UNUSED( 0x08 )
	KEY_UNUSED( 0x10 )
	KEY_UNUSED( 0x20 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("USER KEY") PORT_CODE(KEYCODE_U)
	KEY_UNUSED( 0x80 )

	PORT_START("EXTRA")	// interrupt keys
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MONI") PORT_CODE(KEYCODE_M)	// causes NMI ?
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INTR") PORT_CODE(KEYCODE_I)	// causes INT
INPUT_PORTS_END

static INPUT_PORTS_START( mpf1b )
	PORT_START("LINE0")	// column 0
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 /") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 >") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B STOP") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F LET") PORT_CODE(KEYCODE_F)
	KEY_UNUSED( 0x10 )
	KEY_UNUSED( 0x20 )
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START("LINE1")	// column 1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 *") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 <") PORT_CODE(KEYCODE_6)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A CALL") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E INPUT") PORT_CODE(KEYCODE_E)
	KEY_UNUSED( 0x10 )
	KEY_UNUSED( 0x20 )
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START("LINE2")	// column 2
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 -") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 =") PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 P") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D PRINT") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CONT") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LOAD") PORT_CODE(KEYCODE_F5)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START("LINE3")	// column 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 +") PORT_CODE(KEYCODE_0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 * *") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 M") PORT_CODE(KEYCODE_8)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C NEXT") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RUN") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SAVE") PORT_CODE(KEYCODE_F6)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START("LINE4")	// column 4
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("IF/pgup") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TO/down") PORT_CODE(KEYCODE_T) PORT_CODE(KEYCODE_DOWN)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("THEN/pgdn") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GOTO") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RET/~") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GOSUB") PORT_CODE(KEYCODE_O)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START("LINE5")	// column 5
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FOR/up") PORT_CODE(KEYCODE_H) PORT_CODE(KEYCODE_UP)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LIST") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NEW") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CLR/right") PORT_CODE(KEYCODE_INSERT) PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL/left") PORT_CODE(KEYCODE_DEL) PORT_CODE(KEYCODE_LEFT)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START("LINE6")	// column 6
	KEY_UNUSED( 0x01 )
	KEY_UNUSED( 0x02 )
	KEY_UNUSED( 0x04 )
	KEY_UNUSED( 0x08 )
	KEY_UNUSED( 0x10 )
	KEY_UNUSED( 0x20 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
	KEY_UNUSED( 0x80 )

	PORT_START("EXTRA")	// interrupt keys
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MONI") PORT_CODE(KEYCODE_M)	// causes NMI ?
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INTR") PORT_CODE(KEYCODE_I)	// causes INT
INPUT_PORTS_END

/* Z80 PIO Interface */

static void mpf1_pio_interrupt(const device_config *device, int state)
{
	logerror("pio irq state: %02x\n",state);
}

static const z80pio_interface mpf1_pio_intf =
{
	DEVCB_LINE(mpf1_pio_interrupt),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 CTC Interface */

/* PPI8255 Interface */

//  PA7 = tape EAR = INPUT, meant to be connected to the earphone jacket of a taperecorder
// ~PC7 = tape MIC = OUTPUT, meant to be connected to the microphone jacket of a taperecorder

static READ8_DEVICE_HANDLER( mpf1_porta_r )
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5" };

	UINT8 retval;
	for(keycol = 0; keycol < 6; keycol++)
	{
		if( kbdlatch & 0x01 ) break;
		kbdlatch >>= 1;
	}
	verboselog( 0, "Key column for kbd read: %02x\n", keycol );
	if( keycol != 6 )
	{
		retval = input_port_read(device->machine, keynames[keycol]);
	}
	else
	{
		retval = 0;
	}
	verboselog( 0, "PPI port A read: %02x\n", retval );
	return retval;
}


static TIMER_CALLBACK( led_refresh )
{
	if (BIT(lednum, 5)) output_set_digit_value(0, param);
	if (BIT(lednum, 4)) output_set_digit_value(1, param);
	if (BIT(lednum, 3)) output_set_digit_value(2, param);
	if (BIT(lednum, 2)) output_set_digit_value(3, param);
	if (BIT(lednum, 1)) output_set_digit_value(4, param);
	if (BIT(lednum, 0)) output_set_digit_value(5, param);
}

static WRITE8_DEVICE_HANDLER( mpf1_portb_w )
{
	/* swap bits around for the mame 7-segment emulation */
	UINT8 led_data = BITSWAP8(data, 6, 1, 2, 0, 7, 5, 4, 3);

	/* timer to update segments */
	timer_adjust_oneshot(led_refresh_timer, ATTOTIME_IN_USEC(70), led_data);

	verboselog(1, "PPI port B (LED Data) write: %02x\n", data);
}


static WRITE8_DEVICE_HANDLER( mpf1_portc_w )
{
	const device_config *dac_device = devtag_get_device(device->machine, "dac");

	kbdlatch = ~data;

	/* bits 0-5, led select */
	lednum = data & 0x3f;
	timer_adjust_oneshot(led_refresh_timer, attotime_never, 0);

	data = ~kbdlatch;

	// watchdog reset
	watchdog_reset(device->machine);

	// TONE led & speaker
	led_tone = (~data & 0x80) >> 7;
	set_led_status(0, led_tone);

	// speaker
	dac_data_w(dac_device, 0xFF * led_tone);

	verboselog( 1, "PPI port C (LED/Kbd Col select) write: %02x\n", data );
}



static I8255A_INTERFACE( ppi8255_intf )
{
	DEVCB_HANDLER(mpf1_porta_r),	/* Port A read */
	DEVCB_NULL,	/* Port B read */
	DEVCB_NULL,	/* Port C read */
	DEVCB_NULL,	/* Port A write */
	DEVCB_HANDLER(mpf1_portb_w),	/* Port B write */
	DEVCB_HANDLER(mpf1_portc_w),	/* Port C write */
};

/* Machine Initialization */

static TIMER_CALLBACK( irq0_callback )
{
	irq0_line_hold( cputag_get_cpu(machine, "maincpu") );
}

static MACHINE_START( mpf1 )
{
	led_refresh_timer = timer_alloc(machine, led_refresh, 0);
}

static MACHINE_RESET( mpf1 )
{
	lednum = 0;

	timer_pulse(machine,  ATTOTIME_IN_HZ(1), NULL, 0, check_halt_callback );
	timer_pulse(machine,  ATTOTIME_IN_HZ(60), NULL, 0, irq0_callback );
}

/* Machine Drivers */

static MACHINE_DRIVER_START( mpf1 )
	// basic machine hardware
	MDRV_CPU_ADD("maincpu", Z80, 3579500/2)	// 1.79 MHz
	MDRV_CPU_PROGRAM_MAP(mpf1_map)
	MDRV_CPU_IO_MAP(mpf1_io_map)

	MDRV_MACHINE_START( mpf1 )
	MDRV_MACHINE_RESET( mpf1 )

	MDRV_Z80PIO_ADD( "z80pio", mpf1_pio_intf )

	MDRV_I8255A_ADD( "ppi8255", ppi8255_intf )

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_DEFAULT_LAYOUT( layout_mpf1 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mpf1p )
	// basic machine hardware
	MDRV_CPU_ADD("maincpu", Z80, 2500000)
	MDRV_CPU_PROGRAM_MAP(mpf1p_map)
	MDRV_CPU_IO_MAP(mpf1_io_map)

	MDRV_MACHINE_RESET( mpf1 )

	MDRV_Z80PIO_ADD( "z80pio", mpf1_pio_intf )

	MDRV_I8255A_ADD( "ppi8255", ppi8255_intf )

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_DEFAULT_LAYOUT( layout_mpf1 )
MACHINE_DRIVER_END

/* ROMs */

ROM_START( mpf1 )
    ROM_REGION( 0x10000, "maincpu", 0 )
    ROM_LOAD( "mpf.u6", 0x0000, 0x1000, CRC(b60249ce) SHA1(78e0e8874d1497fabfdd6378266d041175e3797f) )
ROM_END

ROM_START( mpf1b )
    ROM_REGION( 0x10000, "maincpu", 0 )
    ROM_LOAD( "c55167.u6", 0x0000, 0x1000, CRC(28b06dac) SHA1(99cfbab739d71a914c39302d384d77bddc4b705b) )
    ROM_LOAD( "basic.u7",  0x2000, 0x1000, CRC(d276ed6b) SHA1(a45fb98961be5e5396988498c6ed589a35398dcf) )
ROM_END

ROM_START( mpf1p )
    ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "mpf1pmon.bin", 0x0000, 0x2000, CRC(91ace7d3) SHA1(22e3c16a81ac09f37741ad1b526a4456b2ba9493))
ROM_END
/* System Configuration */

static SYSTEM_CONFIG_START(mpf1)
	CONFIG_RAM_DEFAULT(4 * 1024)
SYSTEM_CONFIG_END

/* System Drivers */

COMP( 1979, mpf1,  0,    0, mpf1, mpf1,  0, mpf1, "Multitech", "Micro Professor 1" , 0)
COMP( 1979, mpf1b, mpf1, 0, mpf1, mpf1b, 0, mpf1, "Multitech", "Micro Professor 1B" , 0)
COMP( 1982, mpf1p, mpf1, 0, mpf1p,mpf1b, 0, mpf1, "Multitech", "Micro Professor 1 Plus" , GAME_NOT_WORKING)
