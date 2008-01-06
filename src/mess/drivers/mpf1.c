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
#include "machine/8255ppi.h"
#include "cpu/z80/z80.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "mscommon.h"
#include "sound/dac.h"

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
		logerror( "%08x: %s", activecpu_get_pc(), buf );
	}
}

static const char leddisplay[] =
{
	"         aaaaaaaaaaaaaaaaa   \r"
	"        aaaaaaaaaaaaaaaaaaa  \r"
	"        aaaaaaaaaaaaaaaaaa   \r"
	"      ff aaaaaaaaaaaaaaaa bb \r"
	"      fff                bbb \r"
	"     ffff               bbbb \r"
	"     ffff               bbbb \r"
	"     ffff              bbbbb \r"
	"     ffff              bbbb  \r"
	"    fffff              bbbb  \r"
	"    ffff               bbbb  \r"
	"    ffff               bbbb  \r"
	"    ffff               bbb   \r"
	"    ffff              bbbb   \r"
	"    ffff              bbbb   \r"
	"   fffff              bbbb   \r"
	"   ffff               bbbb   \r"
	"    ff ggggggggggggggg bb    \r"
	"      ggggggggggggggggg      \r"
	"      gggggggggggggggg       \r"
	"    e  gggggggggggggg cc     \r"
	"   eee               cccc    \r"
	"  eeee              ccccc    \r"
	"  eeee              ccccc    \r"
	"  eeee              ccccc    \r"
	" eeee               ccccc    \r"
	" eeee               cccc     \r"
	" eeee               cccc     \r"
	" eeee              ccccc     \r"
	" eeee              ccccc     \r"
	"eeeee              cccc      \r"
	"eeee               cccc      \r"
	"eeee               cccc  hh  \r"
	"eee                cccc hhhh \r"
	"ee dddddddddddddddd cc  hhhh \r"
	"  dddddddddddddddddd    hhhh \r"
	"  dddddddddddddddddd     hh  \r"
	"   dddddddddddddddd          \r"
}; // h is originally represented by p in original schematics

static const char radius_7_led[] = {
       "     11111\r"
       "   111111111\r"
       "  11111111111\r"
       " 1111111111111\r"
       "11111111111111\r"
       "111111111111111\r"
       "111111111111111\r"
       "111111111111111\r"
       "111111111111111\r"
       "111111111111111\r"
       "111111111111111\r"
       " 1111111111111\r"
       "  11111111111\r"
       "   111111111\r"
       "    111111\r"
};

static UINT32 leddigit[6];
static INT8 lednum;
static int led_tone;
static int led_halt;
static INT8 keycol;
static UINT8 kbdlatch;



/* video */

static PALETTE_INIT( mpf1 )
{
	palette_set_color_rgb(machine, 0, 0x00, 0x00, 0x00);
	palette_set_color_rgb(machine, 1, 0xff, 0x00, 0x00);
	palette_set_color_rgb(machine, 2, 0x00, 0xff, 0x00);
}



static VIDEO_START( mpf1 )
{
    videoram_size = 6 * 2 + 24;
    videoram = auto_malloc (videoram_size);
	video_start_generic(machine);
}



static VIDEO_UPDATE( mpf1 )
{
	int x;
	static UINT8 xpositions[] = { 20, 59, 97, 135, 185, 223 };

	//fillbitmap(bitmap, get_black_pen(), NULL);

	for(x = 0; x < 6; x++)
		draw_led(bitmap, leddisplay, leddigit[x], xpositions[x], 377);

	// tone-LED; the green one
	draw_led(bitmap, radius_7_led, led_tone * 2, 277, 375);

	// halt-LED; the red one, is turned on when the processor is halted
	// TODO: processor seems to halt, but restarts(?) at 0x0000 after a while -> fix
	led_halt = (UINT8) cpunum_get_info_int(0, CPUINFO_INT_REGISTER + Z80_HALT);
	draw_led(bitmap, radius_7_led, led_halt, 277, 394);
	return 0;
}

/* Memory Maps */

static ADDRESS_MAP_START( mpf1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0x1fff) AM_RAM
	AM_RANGE(0x2000, 0x2fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mpf1_io_map, ADDRESS_SPACE_IO, 8 )
    /* Appendix B.D from the MPF-I user's manual:
       (contains possible typing/printing errors so I've cited it litteraly)

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

       So to asure that this "incompleteness" of the hardware does also exist in
       this simulator I've expanded the port assignments accordingly. I've also
       tested wether this is true for the actual hardware, and it is.
    */
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )

	// The 16 I/O port combinations for the 8255 (P8255A-5, 8628LLP, (c) 1981 AMD)
	AM_RANGE(0x00, 0x03) AM_READWRITE(ppi8255_0_r, ppi8255_0_w) AM_MIRROR(0x3C)

//  TODO: create drivers to emulate the following two chips
//  The 16 I/O port combinations for the CTC (Zilog, Z0843004PSC, Z80 CTC, 8644)
//  AM_RANGE(0x40, 0x43) AM_WRITE(ctc_enable_w) AM_MIRROR(0x7C)

//  The 16 I/O port combinations for the PIO (Zilog, Z0842004PSC, Z80 PIO, 8735)
//  AM_RANGE(0x80, 0x83) AM_READWRITE(pio_r, pio_w) AM_MIRROR(0xBF)

ADDRESS_MAP_END

/* Input Ports */

#define KEY_UNUSED( bit ) \
	PORT_BIT( bit, IP_ACTIVE_LOW, IPT_UNUSED )

static INPUT_PORTS_START( mpf1 )
	PORT_START	// column 0
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 HL") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 HL'") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B I*IF") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F *PNC'") PORT_CODE(KEYCODE_F)
	KEY_UNUSED( 0x10 )
	KEY_UNUSED( 0x20 )
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START	// column 1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 DE") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 DE'") PORT_CODE(KEYCODE_6)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A SP") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E SZ*H'") PORT_CODE(KEYCODE_E)
	KEY_UNUSED( 0x10 )
	KEY_UNUSED( 0x20 )
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START	// column 2
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 BC") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 BC'") PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 IY") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D *PNC") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("STEP") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAPE RD") PORT_CODE(KEYCODE_F5)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START	// column 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 AF") PORT_CODE(KEYCODE_0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 AF'") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 IX") PORT_CODE(KEYCODE_8)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C SZ*H") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GO") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAPE WR") PORT_CODE(KEYCODE_F6)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START	// column 4
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CBR") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PC") PORT_CODE(KEYCODE_M)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("REG") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ADR") PORT_CODE(KEYCODE_STOP)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RELA") PORT_CODE(KEYCODE_RCONTROL)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START	// column 5
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SBR") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_J)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DATA") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INS") PORT_CODE(KEYCODE_COLON)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MOVE") PORT_CODE(KEYCODE_QUOTE)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START	// column 6
	KEY_UNUSED( 0x01 )
	KEY_UNUSED( 0x02 )
	KEY_UNUSED( 0x04 )
	KEY_UNUSED( 0x08 )
	KEY_UNUSED( 0x10 )
	KEY_UNUSED( 0x20 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("USER KEY") PORT_CODE(KEYCODE_U)
	KEY_UNUSED( 0x80 )

	PORT_START	// interrupt keys
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MONI") PORT_CODE(KEYCODE_M)	// causes NMI ?
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INTR") PORT_CODE(KEYCODE_I)	// causes INT
INPUT_PORTS_END

static INPUT_PORTS_START( mpf1b )
	PORT_START	// column 0
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 /") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 >") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B STOP") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F LET") PORT_CODE(KEYCODE_F)
	KEY_UNUSED( 0x10 )
	KEY_UNUSED( 0x20 )
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START	// column 1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 *") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 <") PORT_CODE(KEYCODE_6)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A CALL") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E INPUT") PORT_CODE(KEYCODE_E)
	KEY_UNUSED( 0x10 )
	KEY_UNUSED( 0x20 )
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START	// column 2
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 -") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 =") PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 P") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D PRINT") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CONT") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LOAD") PORT_CODE(KEYCODE_F5)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START	// column 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 +") PORT_CODE(KEYCODE_0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 * *") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 M") PORT_CODE(KEYCODE_8)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C NEXT") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RUN") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SAVE") PORT_CODE(KEYCODE_F6)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START	// column 4
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("IF/pgup") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TO/down") PORT_CODE(KEYCODE_T) PORT_CODE(KEYCODE_DOWN)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("THEN/pgdn") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GOTO") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RET/~") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GOSUB") PORT_CODE(KEYCODE_O)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START	// column 5
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FOR/up") PORT_CODE(KEYCODE_H) PORT_CODE(KEYCODE_UP)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LIST") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NEW") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CLR/right") PORT_CODE(KEYCODE_INSERT) PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL/left") PORT_CODE(KEYCODE_DEL) PORT_CODE(KEYCODE_LEFT)
	KEY_UNUSED( 0x40 )
	KEY_UNUSED( 0x80 )

	PORT_START	// column 6
	KEY_UNUSED( 0x01 )
	KEY_UNUSED( 0x02 )
	KEY_UNUSED( 0x04 )
	KEY_UNUSED( 0x08 )
	KEY_UNUSED( 0x10 )
	KEY_UNUSED( 0x20 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
	KEY_UNUSED( 0x80 )

	PORT_START	// interrupt keys
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MONI") PORT_CODE(KEYCODE_M)	// causes NMI ?
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INTR") PORT_CODE(KEYCODE_I)	// causes INT
INPUT_PORTS_END

/* Z80 PIO Interface */

static void mpf1_pio_interrupt( int state )
{
	logerror("pio irq state: %02x\n",state);
}

static const z80pio_interface pio_intf =
{
	mpf1_pio_interrupt,
	NULL,
	NULL
};

/* Z80 CTC Interface */

/* PPI8255 Interface */

//  PA7 = tape EAR = INPUT, meant to be connected to the earphone jacket of a taperecorder
// ~PC7 = tape MIC = OUTPUT, meant to be connected to the microphone jacket of a taperecorder

static  READ8_HANDLER( mpf1_porta_r )
{
	UINT8 retval;
	for(keycol = 0; keycol < 6; keycol++)
	{
		if( kbdlatch & 0x01 ) break;
		kbdlatch >>= 1;
	}
	verboselog( 0, "Key column for kbd read: %02x\n", keycol );
	if( keycol != 6 )
	{
		retval = readinputport(keycol);
	}
	else
	{
		retval = 0;
	}
	verboselog( 0, "PPI port A read: %02x\n", retval );
	return retval;
}



static  READ8_HANDLER( mpf1_portb_r )
{
	verboselog( 0, "PPI port B read\n" );
	return 0;
}



static  READ8_HANDLER( mpf1_portc_r )
{
	verboselog( 0, "PPI port C read\n" );
	return 0;
}



static WRITE8_HANDLER( mpf1_porta_w )
{
	verboselog( 0, "PPI port A write: %02x\n", data );
}



static WRITE8_HANDLER( mpf1_portb_w )
{
    //int i;
	/*  A   0x08
        B   0x10
        C   0x20
        D   0x80
        E   0x01
        F   0x04
        G   0x02
        H   0x40 (represented by P in original schematics) */

	/*  Original bit to leddisplay-segment assignment:
        (and as such what is written as output to this port)
        bit      7 6 5 4 3 2 1 0
        segment  d p c b a f g e

        The next statement converts this to the following assignment: (which is compatible with draw_led())
        bit      7 6 5 4 3 2 1 0
        segment  p g f e d c b a */
	if( data | ( activecpu_get_pc() != 0x63C ) )
	{
		data = ( (data & 0x08) >> 3 ) |
			   ( (data & 0x10) >> 3 ) |
		   	   ( (data & 0x20) >> 3 ) |
		       ( (data & 0x80) >> 4 ) |
		       ( (data & 0x01) << 4 ) |
		       ( (data & 0x04) << 3 ) |
		       ( (data & 0x02) << 5 ) |
		       ( (data & 0x40) << 1 );
		leddigit[lednum] = data;
	}
	verboselog( 1, "PPI port B (LED Data) write: %02x\n", data );
}



static WRITE8_HANDLER( mpf1_portc_w )
{
	kbdlatch = ~data;

	if (data & 0x3f)
	{
		for(lednum = 0; lednum < 6; lednum++)
		{
			if( data & 0x01 ) break;
			data >>= 1;
		}
		if( lednum == 6 ) lednum = 0;
		lednum = 5 - lednum;
	}

	data = ~kbdlatch;

	// watchdog reset
	watchdog_reset_w(0, ~data & 0x40);

	// TONE led & speaker
	led_tone = (~data & 0x80) >> 7;
	set_led_status(0, led_tone);

	// speaker
	DAC_data_w(0, 0xFF * led_tone);

	verboselog( 1, "PPI port C (LED/Kbd Col select) write: %02x\n", data );
}



static const ppi8255_interface ppi8255_intf =
{
	1, 					/* 1 chip */
	{ mpf1_porta_r },	/* Port A read */
	{ mpf1_portb_r },	/* Port B read */
	{ mpf1_portc_r },	/* Port C read */
	{ mpf1_porta_w },	/* Port A write */
	{ mpf1_portb_w },	/* Port B write */
	{ mpf1_portc_w },	/* Port C write */
};

/* Machine Initialization */

static MACHINE_RESET( mpf1 )
{
	// PIO
	z80pio_init(0, &pio_intf);
	z80pio_reset(0);

	// CTC
/*  z80ctc_init(&ctc_intf);
    z80ctc_reset(0);*/

	// 8255
	ppi8255_init(&ppi8255_intf);

	// leds
	for (lednum = 0; lednum < 6; lednum++)
		leddigit[lednum] = 0;

	lednum = 0;
}

/* Machine Drivers */

static MACHINE_DRIVER_START( mpf1 )
	// basic machine hardware
	MDRV_CPU_ADD(Z80, 3579500/2)	// 1.79 MHz
	MDRV_CPU_PROGRAM_MAP(mpf1_map, 0)
	MDRV_CPU_IO_MAP(mpf1_io_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET( mpf1 )

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(462, 661)
	MDRV_SCREEN_VISIBLE_AREA(0, 461, 0, 660)
	MDRV_PALETTE_LENGTH(3)

	MDRV_PALETTE_INIT( mpf1 )
	MDRV_VIDEO_START(mpf1)
	MDRV_VIDEO_UPDATE(mpf1)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

MACHINE_DRIVER_END

/* ROMs */

ROM_START( mpf1 )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "mpf.u6", 0x0000, 0x1000, CRC(b60249ce) SHA1(78e0e8874d1497fabfdd6378266d041175e3797f) )
ROM_END

ROM_START( mpf1b )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "c55167.u6", 0x0000, 0x1000, CRC(28b06dac) SHA1(99cfbab739d71a914c39302d384d77bddc4b705b) )
    ROM_LOAD( "basic.u7",  0x2000, 0x1000, CRC(d276ed6b) SHA1(a45fb98961be5e5396988498c6ed589a35398dcf) )
ROM_END

/* System Configuration */

SYSTEM_CONFIG_START(mpf1)
	CONFIG_RAM_DEFAULT(4 * 1024)
SYSTEM_CONFIG_END

/* System Drivers */

COMP( 1979, mpf1,  0,    0, mpf1, mpf1,  0, mpf1, "Multitech", "Micro Professor 1" , 0)
COMP( 1979, mpf1b, mpf1, 0, mpf1, mpf1b, 0, mpf1, "Multitech", "Micro Professor 1B" , 0)
