/***************************************************************************

        Hector 2HR+
        Victor
        Hector 2HR
        Hector HRX
        Hector MX40c
        Hector MX80c

    These machines can load and run cassettes for the interact / hector1.
    hec2hr - press 2 then 1
    hec2hrp - press 2 then 1
    hec2hrx - press 5 then 1
    hec2mx40 - press 5 then 1
    hec2mx80 - not compatible
    victor - press R then L

        12/05/2009 Skeleton driver - Micko
       31/06/2009 Video - Robbbert

        29/10/2009 Update skeleton to functional machine
                          by yo_fr       (jj.stac@aliceadsl.fr)

               => add Keyboard,
               => add color,
               => add cassette(24/12/09 add the *.K7 and *.FOR format for the file and castools),
               => add sn76477 sound and 1bit sound,
               => add joysticks (stick, pot, fire)
               => add BR/HR switching
               => add bank switch for HRX
               => add device MX80c and bank switching for the ROM
       03/01/2010 Update and clean prog  by yo_fr       (jj.stac@aliceadsl.fr)
               => add the port mapping for keyboard

      don't forget to keep some information about these machine see DChector project : http://dchector.free.fr/ made by DanielCoulom
      (and thanks to Daniel!)

    TODO : Add the cartridge function,
           Add diskette support,
           Adjust the one shot and A/D timing (sn76477)
****************************************************************************/
/* Joystick 1 :
 clavier numerique :
                (UP)5
  (left)1                     (right)3
               (down)2

 Fire <+>
 Pot =>  home/end */

 /* Joystick 0 :
 arrows
                (UP)^
  (left)<-                     (right)->
               (down)v

 Fire <0> on numpad
 Pot => INS /SUPPR
 Cassette : wav file (1 way, 16 bits, 44100hz)
             K7 file  (For data and games)
           FOR file (for forth screen data)
*/

#include "emu.h"

#include "devices/cassette.h"
#include "formats/hect_tap.h"
#include "devices/printer.h"
#include "sound/sn76477.h"   /* for sn sound*/
#include "sound/wave.h"      /* for K7 sound*/
#include "sound/discrete.h"  /* for 1 Bit sound*/
#include "machine/upd765.h"	/* for floppy disc controller */
#include "devices/flopdrv.h" 
#include "formats/basicdsk.h"
#include "cpu/z80/z80.h"

#include "includes/hec2hrp.h"

/*****************************************************************************/
static ADDRESS_MAP_START(hecdisk2_mem, ADDRESS_SPACE_PROGRAM, 8)
/*****************************************************************************/
	ADDRESS_MAP_UNMAP_HIGH

    /* Hardward address mapping*/
	AM_RANGE( 0x0000, 0x0fff ) AM_ROM AM_REGION("disk2cpu", 0)				/* rom memory ->passage en RAM pour ecriture en zone 0x0066 */
	AM_RANGE( 0x0000, 0xffff ) AM_READWRITE( hector_disk2_r, hector_disk2_w)/* ram memory */

ADDRESS_MAP_END

static ADDRESS_MAP_START( hecdisk2_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x030,0x03f) AM_READWRITE( hector_disk2_io30_port_r, hector_disk2_io30_port_w )
	AM_RANGE(0x040,0x04f) AM_READWRITE( hector_disk2_io40_port_r, hector_disk2_io40_port_w )
	AM_RANGE(0x050,0x05f) AM_READWRITE( hector_disk2_io50_port_r, hector_disk2_io50_port_w )
	AM_RANGE(0x060,0x060) AM_READWRITE( hector_disk2_io60_port_r, hector_disk2_io60_port_w )
	AM_RANGE(0x061,0x061) AM_READWRITE( hector_disk2_io61_port_r, hector_disk2_io61_port_w )
	AM_RANGE(0x070,0x07f) AM_READWRITE( hector_disk2_io70_port_r, hector_disk2_io70_port_w )
ADDRESS_MAP_END
/*****************************************************************************/
static ADDRESS_MAP_START(hec2hrp_mem, ADDRESS_SPACE_PROGRAM, 8)
/*****************************************************************************/
	ADDRESS_MAP_UNMAP_HIGH

	/* Hardware address mapping*/
	AM_RANGE(0x0800,0x0808) AM_WRITE( hector_switch_bank_w)/* Bank management*/
	AM_RANGE(0x1000,0x1000) AM_WRITE( hector_color_a_w)  /* Color c0/c1*/
	AM_RANGE(0x1800,0x1800) AM_WRITE( hector_color_b_w)  /* Color c2/c3*/
	AM_RANGE(0x2000,0x2003) AM_WRITE( hector_sn_2000_w)  /* Sound*/
	AM_RANGE(0x2800,0x2803) AM_WRITE( hector_sn_2800_w)  /* Sound*/
	AM_RANGE(0x3000,0x3000) AM_READWRITE( hector_cassette_r, hector_sn_3000_w)/* Write necessary*/
	AM_RANGE(0x3800,0x3807) AM_READWRITE( hector_keyboard_r, hector_keyboard_w)  /* Keyboard*/

	/* Main ROM page*/
	AM_RANGE(0x0000,0x3fff) AM_ROMBANK("bank2")

	/* Video br mapping*/
	AM_RANGE(0x4000,0x49ff) AM_RAM AM_BASE_MEMBER(hec2hrp_state, videoram)
	/* continous RAM*/
	AM_RANGE(0x4A00,0xbfff) AM_RAM
	/* from 0xC000 to 0xFFFF => Bank Ram for video and data !*/
	AM_RANGE(0xc000,0xffff) AM_RAMBANK("bank1")
ADDRESS_MAP_END

static ADDRESS_MAP_START( hec2hrp_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hec2hrx_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
    ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x0f0,0x0ff) AM_READWRITE( hector_io_8255_r, hector_io_8255_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START( hec2mx40_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x000,0x0ef) AM_WRITE(     hector_mx40_io_port_w )
	AM_RANGE(0x0f0,0x0f3) AM_READWRITE( hector_io_8255_r, hector_io_8255_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START( hec2mx80_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_GLOBAL_MASK(0xff)// sure ?
	AM_RANGE(0x000,0x0ef) AM_WRITE(     hector_mx80_io_port_w )

	AM_RANGE(0x000,0x0ff) AM_READWRITE( hector_io_8255_r, hector_io_8255_w )
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( hec2hrp )
	/* keyboard input */
	PORT_START("KEY0") /* [0] - port 3000 @ 0 */
	    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\\') PORT_CHAR('|')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space")			PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return")			PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab")			PORT_CODE(KEYCODE_TAB)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("<--")			PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps Lock")      PORT_CODE(KEYCODE_CAPSLOCK)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl")			PORT_CODE(KEYCODE_LCONTROL)   PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift")			PORT_CODE(KEYCODE_LSHIFT)     PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_START("KEY1") /* [1] - port 3000 @ 1 */    /* touches => 2  1  0  /  .  -  ,  +     */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 \"")			PORT_CODE(KEYCODE_2)	PORT_CHAR('2')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 >")			PORT_CODE(KEYCODE_1)	PORT_CHAR('1')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 <")			PORT_CODE(KEYCODE_0)	PORT_CHAR('0')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)	    PORT_CHAR('.') PORT_CHAR('>')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)	PORT_CHAR(',') PORT_CHAR('<')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)	PORT_CHAR('-') PORT_CHAR('_')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)	PORT_CHAR('=') PORT_CHAR('+')

	PORT_START("KEY2") /* [1] - port 3000 @ 2 */     /* touches => .. 9  8  7  6  5  4  3  */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 )")			PORT_CODE(KEYCODE_9)	PORT_CHAR('9')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (")			PORT_CODE(KEYCODE_8)	PORT_CHAR('8')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 :")			PORT_CODE(KEYCODE_7)	PORT_CHAR('7')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 !")			PORT_CODE(KEYCODE_6)	PORT_CHAR('6')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %")			PORT_CODE(KEYCODE_5)	PORT_CHAR('5')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $")			PORT_CODE(KEYCODE_4)	PORT_CHAR('4')
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 /")			PORT_CODE(KEYCODE_3)	PORT_CHAR('3')
	PORT_START("KEY3") /* [1] - port 3000 @ 3 */    /* touches =>  B  A  ..  ? .. =   ..  ;       */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)	 PORT_CHAR('B')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A")  		 PORT_CODE(KEYCODE_Q)	PORT_CHAR('Q')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[') PORT_CHAR('{')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)	   PORT_CHAR('/') PORT_CHAR('?')
	PORT_START("KEY4") /* [1] - port 3000 @ 4 */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)			PORT_CHAR('J')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)			PORT_CHAR('I')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)			PORT_CHAR('H')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)			PORT_CHAR('G')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)			PORT_CHAR('F')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)			PORT_CHAR('E')
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)			PORT_CHAR('C')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)			PORT_CHAR('D')

	PORT_START("KEY5") /* [1] - port 3000 @ 5 */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)			PORT_CHAR('R')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q")         PORT_CODE(KEYCODE_A)			PORT_CHAR('A')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)			PORT_CHAR('P')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)			PORT_CHAR('O')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)			PORT_CHAR('N')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)			PORT_CHAR('L')
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)			PORT_CHAR('K')

	PORT_START("KEY6") /* [1] - port 3000 @ 6 */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z")          PORT_CODE(KEYCODE_W)			PORT_CHAR('W')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)			PORT_CHAR('Y')
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X")          PORT_CODE(KEYCODE_X)			PORT_CHAR('X')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W")          PORT_CODE(KEYCODE_Z)			PORT_CHAR('Z')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)			PORT_CHAR('V')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)			PORT_CHAR('U')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)			PORT_CHAR('T')
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)			PORT_CHAR('S')

	PORT_START("KEY7") /* [1] - port 3000 @ 7  JOYSTICK */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Joy(0) LEFT")		PORT_CODE(KEYCODE_LEFT)		PORT_CHAR(UCHAR_MAMEKEY(LEFT))
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Joy(0) RIGHT")		PORT_CODE(KEYCODE_RIGHT)	PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Joy(0) UP")			PORT_CODE(KEYCODE_UP)		PORT_CHAR(UCHAR_MAMEKEY(UP))
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Joy(0) DOWN")		PORT_CODE(KEYCODE_DOWN)		PORT_CHAR(UCHAR_MAMEKEY(DOWN))
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Joy(1) LEFT")		PORT_CODE(KEYCODE_1_PAD)	 // Joy(1) on numpad
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Joy(1) RIGHT")		PORT_CODE(KEYCODE_3_PAD)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Joy(1) UP")			PORT_CODE(KEYCODE_5_PAD)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Joy(1) DOWN")		PORT_CODE(KEYCODE_2_PAD)

	PORT_START("KEY8") /* [1] - port 3000 @ 8  not for the real machine, but to emulate the analog signal of the joystick */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RESET")				PORT_CODE(KEYCODE_ESC)		PORT_CHAR(27)
		PORT_BIT(0x02, IP_ACTIVE_LOW,  IPT_KEYBOARD) PORT_NAME("Joy(0) FIRE")		PORT_CODE(KEYCODE_0_PAD)
		PORT_BIT(0x04, IP_ACTIVE_LOW,  IPT_KEYBOARD) PORT_NAME("Joy(1) FIRE")		PORT_CODE(KEYCODE_PLUS_PAD)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Pot(0)+") PORT_CODE(KEYCODE_INSERT)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Pot(0)-") PORT_CODE(KEYCODE_DEL)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Pot(1)+") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Pot(1)-") PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END))
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

/*****************************************************************************/
static MACHINE_START( hec2hrp )
/*****************************************************************************/
{
	UINT8 *RAM = memory_region(machine, "maincpu"); // pointer to mess ram
	UINT8 *ROM1 = memory_region(machine, "page1"); // pointer to rom page 1
	UINT8 *ROM2 = memory_region(machine, "page2"); // pointer to rom page 2
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_RESET, PULSE_LINE);
//    cputag_set_input_line(machine, "disk2cpu", INPUT_LINE_IRQ0, CLEAR_LINE); //INT / NMI...

    // Memory install for bank switching
	memory_configure_bank(machine, "bank1", HECTOR_BANK_PROG , 1, &RAM[0xc000]  , 0); // Mess ram
	memory_configure_bank(machine, "bank1", HECTOR_BANK_VIDEO, 1, hector_videoram, 0); // Video ram

    // Set bank HECTOR_BANK_PROG as basic bank
	memory_set_bank(machine, "bank1", HECTOR_BANK_PROG);

/******************************************************SPECIFIQUE MX ***************************/
	memory_configure_bank(machine, "bank2", HECTORMX_BANK_PAGE0 , 1, &RAM[0x0000]  , 0); // Mess ram
	memory_configure_bank(machine, "bank2", HECTORMX_BANK_PAGE1 , 1, &ROM1[0x0000] , 0); // Rom page 1
	memory_configure_bank(machine, "bank2", HECTORMX_BANK_PAGE2 , 1, &ROM2[0x0000] , 0); // Rom page 2
	memory_set_bank(machine, "bank2", HECTORMX_BANK_PAGE0);
/******************************************************SPECIFIQUE MX ***************************/
	hector_init(machine);
}

/*****************************************************************************/
static MACHINE_START( hec2hrx )
/*****************************************************************************/
{
	UINT8 *RAM = memory_region(machine, "maincpu"); // pointer to mess ram
	UINT8 *RAMD2 = memory_region(machine, "disk2cpu"); // pointer to mess ram
	UINT8 *ROM1 = memory_region(machine, "page1"); // pointer to rom page 1
	UINT8 *ROM2 = memory_region(machine, "page2"); // pointer to rom page 2
	UINT32 index=0;
//Patch rom possible !
//RAMD2[0xff6b] = 0x0ff; // force verbose mode hector !

//Initialisation ROM
	while (index <0x01001) 
	{	
		hector_Disk2memory[index] = RAMD2[index];
		index++;
	}
	cputag_set_input_line(machine, "disk2cpu", INPUT_LINE_RESET, PULSE_LINE);
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_RESET, PULSE_LINE);
//    cputag_set_input_line(machine, "disk2cpu", INPUT_LINE_IRQ0, CLEAR_LINE); //INT / NMI...

    // Memory install for bank switching
	memory_configure_bank(machine, "bank1", HECTOR_BANK_PROG , 1, &RAM[0xc000]  , 0); // Mess ram
	memory_configure_bank(machine, "bank1", HECTOR_BANK_VIDEO, 1, hector_videoram, 0); // Video ram

    // Set bank HECTOR_BANK_PROG as basic bank
	memory_set_bank(machine, "bank1", HECTOR_BANK_PROG);

/******************************************************SPECIFIQUE MX ***************************/
	memory_configure_bank(machine, "bank2", HECTORMX_BANK_PAGE0 , 1, &RAM[0x0000]  , 0); // Mess ram
	memory_configure_bank(machine, "bank2", HECTORMX_BANK_PAGE1 , 1, &ROM1[0x0000] , 0); // Rom page 1
	memory_configure_bank(machine, "bank2", HECTORMX_BANK_PAGE2 , 1, &ROM2[0x0000] , 0); // Rom page 2
	memory_set_bank(machine, "bank2", HECTORMX_BANK_PAGE0);
/******************************************************SPECIFIQUE MX ***************************/
	hector_init(machine);
	hector_disk2_init(machine); // Init of the Disk II !
}

static MACHINE_RESET(hec2hrp)
{
	memory_set_bank(machine, "bank1", HECTOR_BANK_PROG);
	memory_set_bank(machine, "bank2", HECTORMX_BANK_PAGE0);
	hec2hrp_reset(machine, 1);
}

static MACHINE_RESET(hec2hrx)
{
	memory_set_bank(machine, "bank1", HECTOR_BANK_PROG);
	memory_set_bank(machine, "bank2", HECTORMX_BANK_PAGE0);
	hec2hrx_reset(machine, 1);
}

/* Cassette definition */
static const struct CassetteOptions hector_cassette_options = {
	1,		/* channels */
	16,		/* bits per sample */
	44100	/* sample frequency */
};

static const cassette_config hector_cassette_config =
{
	hector_cassette_formats,
	&hector_cassette_options,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MASK_SPEAKER),
	NULL
};

/* Discrete Sound */
static DISCRETE_SOUND_START( hec2hrp )
	DISCRETE_INPUT_LOGIC(NODE_01)
	DISCRETE_OUTPUT(NODE_01, 5000)
DISCRETE_SOUND_END

/******************************************************************************/
static MACHINE_CONFIG_START( hec2hr, hec2hrp_state )
/******************************************************************************/
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_5MHz)
	MDRV_CPU_PROGRAM_MAP(hec2hrp_mem)
	MDRV_CPU_IO_MAP(hec2hrp_io)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,50) /*  put on the Z80 irq in Hz*/
	MDRV_MACHINE_RESET(hec2hrp)
	MDRV_MACHINE_START(hec2hrp)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(400)) /* 2500 not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 230)
	MDRV_SCREEN_VISIBLE_AREA(0, 243, 0, 227)
	MDRV_PALETTE_LENGTH(16)
	MDRV_VIDEO_START(hec2hrp)
	MDRV_VIDEO_UPDATE(hec2hrp)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(0, "mono", 0.1)

	MDRV_SOUND_ADD("sn76477", SN76477, 0)
	MDRV_SOUND_CONFIG(hector_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.1)

	MDRV_SOUND_ADD("discrete", DISCRETE, 0) /* Son 1bit*/
	MDRV_SOUND_CONFIG_DISCRETE( hec2hrp )
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

    /* Gestion cassette*/
	MDRV_CASSETTE_ADD( "cassette", hector_cassette_config )

	/* printer */
	MDRV_PRINTER_ADD("printer")

MACHINE_CONFIG_END

/*****************************************************************************/
static MACHINE_CONFIG_START( hec2hrp, hec2hrp_state )
/*****************************************************************************/
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_5MHz)
	MDRV_CPU_PROGRAM_MAP(hec2hrp_mem)
	MDRV_CPU_IO_MAP(hec2hrp_io)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,50) /*  put on the Z80 irq in Hz*/
	MDRV_MACHINE_RESET(hec2hrp)
	MDRV_MACHINE_START(hec2hrp)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(400)) /* 2500 not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 230)
	MDRV_SCREEN_VISIBLE_AREA(0, 243, 0, 227)
	MDRV_PALETTE_LENGTH(16)
	MDRV_VIDEO_START(hec2hrp)
	MDRV_VIDEO_UPDATE(hec2hrp)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(0, "mono", 0.1)// Sound level for cassette, as it is in mono => output channel=0

	MDRV_SOUND_ADD("sn76477", SN76477, 0)
	MDRV_SOUND_CONFIG(hector_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.1)

	MDRV_SOUND_ADD("discrete", DISCRETE, 0) // Son 1bit
	MDRV_SOUND_CONFIG_DISCRETE( hec2hrp )
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

   /* Gestion cassette*/
	MDRV_CASSETTE_ADD( "cassette", hector_cassette_config )

	/* printer */
	MDRV_PRINTER_ADD("printer")

MACHINE_CONFIG_END

/*****************************************************************************/
static MACHINE_CONFIG_START( hec2mx40, hec2hrp_state )
/*****************************************************************************/
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_5MHz)
	MDRV_CPU_PROGRAM_MAP(hec2hrp_mem)
	MDRV_CPU_IO_MAP(hec2mx40_io)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,50) //  put on the Z80 irq in Hz
	MDRV_MACHINE_RESET(hec2hrx)
	MDRV_MACHINE_START(hec2hrx)

	/* Disk II unit */
	MDRV_CPU_ADD("disk2cpu",Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(hecdisk2_mem)
	MDRV_CPU_IO_MAP(hecdisk2_io)
//	MDRV_CPU_PERIODIC_INT(irq0_line_hold,50) //  put on the Z80 irq in Hz
	MDRV_UPD765A_ADD("upd765", hector_disk2_upd765_interface)
	MDRV_FLOPPY_2_DRIVES_ADD(hector_disk2_floppy_config)
	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(400)) /* 2500 not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 230)
	MDRV_SCREEN_VISIBLE_AREA(0, 243, 0, 227)
	MDRV_PALETTE_LENGTH(16)
	MDRV_VIDEO_START(hec2hrp)
	MDRV_VIDEO_UPDATE(hec2hrp)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(0, "mono", 0.1)// Sound level for cassette, as it is in mono => output channel=0

	MDRV_SOUND_ADD("sn76477", SN76477, 0)
	MDRV_SOUND_CONFIG(hector_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.1)

	MDRV_SOUND_ADD("discrete", DISCRETE, 0) // Son 1bit
	MDRV_SOUND_CONFIG_DISCRETE( hec2hrp )
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

    /* Gestion cassette*/
	MDRV_CASSETTE_ADD( "cassette", hector_cassette_config )

	/* printer */
	MDRV_PRINTER_ADD("printer")

MACHINE_CONFIG_END
/*****************************************************************************/
static MACHINE_CONFIG_START( hec2hrx, hec2hrp_state )
/*****************************************************************************/
/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_5MHz)
	MDRV_CPU_PROGRAM_MAP(hec2hrp_mem)
	MDRV_CPU_IO_MAP(hec2hrx_io)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,50) //  put on the Z80 irq in Hz
	MDRV_MACHINE_RESET(hec2hrx)
	MDRV_MACHINE_START(hec2hrx)

	/* Disk II unit */
	MDRV_CPU_ADD("disk2cpu",Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(hecdisk2_mem)
	MDRV_CPU_IO_MAP(hecdisk2_io)
	MDRV_UPD765A_ADD("upd765", hector_disk2_upd765_interface)
	MDRV_FLOPPY_2_DRIVES_ADD(hector_disk2_floppy_config)


	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(400)) /* 2500 not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 230)
	MDRV_SCREEN_VISIBLE_AREA(0, 243, 0, 227)
    MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(hec2hrp)
	MDRV_VIDEO_UPDATE(hec2hrp)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(0, "mono", 0.1)// Sound level for cassette, as it is in mono => output channel=0

	MDRV_SOUND_ADD("sn76477", SN76477, 0)
	MDRV_SOUND_CONFIG(hector_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.1)

	MDRV_SOUND_ADD("discrete", DISCRETE, 0) // Son 1bit
	MDRV_SOUND_CONFIG_DISCRETE( hec2hrp )
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

    // Gestion cassette
    MDRV_CASSETTE_ADD( "cassette", hector_cassette_config )

	/* printer */
	MDRV_PRINTER_ADD("printer")

MACHINE_CONFIG_END
/*****************************************************************************/
static MACHINE_CONFIG_START( hec2mx80, hec2hrp_state )
/*****************************************************************************/
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_5MHz)
	MDRV_CPU_PROGRAM_MAP(hec2hrp_mem)
	MDRV_CPU_IO_MAP(hec2mx80_io)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,50) //  put on the Z80 irq in Hz
	MDRV_MACHINE_RESET(hec2hrp)
	MDRV_MACHINE_START(hec2hrp)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(400)) /* 2500 not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 230)
	MDRV_SCREEN_VISIBLE_AREA(0, 243, 0, 227)
	MDRV_PALETTE_LENGTH(16)
	MDRV_VIDEO_START(hec2hrp)
	MDRV_VIDEO_UPDATE(hec2hrp)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(0, "mono", 0.1)// Sound level for cassette, as it is in mono => output channel=0

	MDRV_SOUND_ADD("sn76477", SN76477, 0)
	MDRV_SOUND_CONFIG(hector_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.1)

	MDRV_SOUND_ADD("discrete", DISCRETE, 0) // Son 1bit
	MDRV_SOUND_CONFIG_DISCRETE( hec2hrp )
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

   /* Gestion cassette*/
	MDRV_CASSETTE_ADD( "cassette", hector_cassette_config )

	/* printer */
	MDRV_PRINTER_ADD("printer")

MACHINE_CONFIG_END

/* ROM definition */
ROM_START( hec2hr )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "2hr.bin", 0x0000, 0x1000, CRC(84b9e672) SHA1(8c8b089166122eee565addaed10f84c5ce6d849b))
	ROM_REGION( 0x4000, "page1", ROMREGION_ERASEFF )
	ROM_REGION( 0x4000, "page2", ROMREGION_ERASEFF )
ROM_END

ROM_START( victor )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "victor.rom",  0x0000, 0x1000, CRC(d1e9508f) SHA1(d0f1bdcd39917FAFC8859223AB38EEE2A7DC85FF))
	ROM_REGION( 0x4000, "page1", ROMREGION_ERASEFF )
	ROM_REGION( 0x4000, "page2", ROMREGION_ERASEFF )
ROM_END

ROM_START( hec2hrp )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hector2hrp.rom", 0x0000, 0x4000, CRC(983f52e4) SHA1(71695941d689827356042ee52ffe55ce7e6b8ecd))
	ROM_REGION( 0x4000, "page1", ROMREGION_ERASEFF )
	ROM_REGION( 0x4000, "page2", ROMREGION_ERASEFF )
ROM_END

ROM_START( hec2hrx )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hector2hrx.rom", 0x0000, 0x4000, CRC(f047c521) SHA1(744336b2acc76acd7c245b562bdc96dca155b066))
	ROM_REGION( 0x4000, "page1", ROMREGION_ERASEFF )
	ROM_REGION( 0x4000, "page2", ROMREGION_ERASEFF )
	ROM_REGION( 0x10000, "disk2cpu", ROMREGION_ERASEFF )
	ROM_LOAD( "d800k.bin" , 0x0000,0x1000, CRC(831bd584) SHA1(9782ee58f570042608d9d568b2c3fc4c6d87d8b9))
ROM_END

ROM_START( hec2mx80 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mx80c_page0.rom" , 0x0000,0x4000, CRC(A75945CF) SHA1(542391E482271BE0997B069CF13C8B5DAE28FEEC))
	ROM_REGION( 0x4000, "page1", ROMREGION_ERASEFF )
	ROM_LOAD( "mx80c_page1.rom", 0x0000, 0x4000, CRC(4615f57c) SHA1(5de291bf3ae0320915133b99f1a088cb56c41658))
	ROM_REGION( 0x4000, "page2", ROMREGION_ERASEFF )
	ROM_LOAD( "mx80c_page2.rom" , 0x0000,0x4000, CRC(2D5D975E) SHA1(48307132E0F3FAD0262859BB8142D108F694A436))
ROM_END

ROM_START( hec2mx40 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mx40c_page0.rom" , 0x0000,0x4000, CRC(9bb5566d) SHA1(0c8c2e396ec8eb995d2b621abe06b6968ca5d0aa))
	ROM_REGION( 0x4000, "page1", ROMREGION_ERASEFF )
	ROM_LOAD( "mx40c_page1.rom", 0x0000, 0x4000, CRC(192a76fa) SHA1(062aa6df0b554b85774d4b5edeea8496a4baca35))
	ROM_REGION( 0x4000, "page2", ROMREGION_ERASEFF )
	ROM_LOAD( "mx40c_page2.rom" , 0x0000,0x4000, CRC(ef1b2654) SHA1(66624ea040cb7ede4720ad2eca0738d0d3bad89a))
	ROM_REGION( 0x10000, "disk2cpu", ROMREGION_ERASEFF )
//	ROM_REGION( 0x10000, "rom_disk2", ROMREGION_ERASEFF ) // Marche avec cela sauf ecr en ram !!!!
//	ROM_LOAD( "d360k.bin" , 0x0000,0x4000, CRC(2454eacb) SHA1(dc0d5a7d5891a7e422d9d142a2419527bb15dfd5))
	ROM_LOAD( "d800k.bin" , 0x0000,0x1000, CRC(831bd584) SHA1(9782ee58f570042608d9d568b2c3fc4c6d87d8b9))
//	ROM_LOAD( "d200k.bin" , 0x0000,0x4000, CRC(e2801377) SHA1(0926df5b417ecd8013e35c71b76780c5a25c1cbf))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP(1983, hec2hrp,  0, interact,    hec2hrp,	hec2hrp, 0, 	 "Micronique",   "Hector 2HR+",	GAME_IMPERFECT_SOUND)
COMP(1980, victor,   hec2hrp, 0,     hec2hrp,   hec2hrp, 0,      "Micronique",   "Victor",	GAME_IMPERFECT_SOUND)
COMP(1983, hec2hr ,  hec2hrp, 0,     hec2hr,	hec2hrp, 0, 	 "Micronique",   "Hector 2HR",	GAME_IMPERFECT_SOUND)
COMP(1984, hec2hrx,  hec2hrp, 0,     hec2hrx,	hec2hrp, 0, 	 "Micronique",   "Hector HRX",	GAME_IMPERFECT_SOUND)
COMP(1985, hec2mx80, hec2hrp, 0,     hec2mx80,	hec2hrp, 0, 	 "Micronique",   "Hector MX 80c" ,	GAME_IMPERFECT_SOUND)
COMP(1985, hec2mx40, hec2hrp, 0,     hec2mx40,	hec2hrp, 0, 	 "Micronique",   "Hector MX 40c" ,	GAME_IMPERFECT_SOUND)
