/***************************************************************************
  MPU4 highly preliminary driver by J.Wallace, and Anonymous.

  03-08-2007: J Wallace: Removed audio filter for now, since sound is more accurate without them.
                         Connect 4 now has the right sound.
  03-07-2007: J Wallace: Several major changes, including input relabelling, and system timer improvements.
     06-2007: Atari Ace, many cleanups and optimizations of I/O routines
  09-06-2007: J Wallace: Fixed 50Hz detection circuit.
  09-04-2007: J Wallace: Corrected a lot of out of date info in this revision history.
                         Revisionism, if you will.
  17-02-2007: J Wallace: Added Deal 'Em - still needs some work.
  10-02-2007: J Wallace: Improved input timing.
              It appears Connect 4 uses a different sound mapping to regular Barcrest games.
  30-01-2007: J Wallace: Characteriser rewritten to run the 'extra' data needed by some games.
  24-01-2007: J Wallace: With thanks to Canonman and HIGHWAYMAN/System 80, I was able to confirm a seemingly
              ghastly misuse of a PIA is actually on the real hardware. This fixes the meters.
  30-12-2006: J Wallace: Fixed init routines, state saving is theoretically supported.
  23-09-2006: Converted 7Segment code to cleaner version, but have yet to add new 16k EPROM, as
              the original image was purported to come from a Barcrest BBS service, and is probably
              more 'official'.
  07-09-2006: It appears that the video firmware is intended to be a 16k EPROM, not 64k as dumped.
              In fact, the current dump is just the code repeated 4 times, when nothing earlier than
              0xC000 is ever called. Presumably, the ROM is loaded into C000, and the remainder of the
              'ROM' space is in fact the extra 32k of RAM apparently needed to hold the video board data.
              For now, we'll continue to use the old dump, as I am unsure as to how to map this in MAME.
              Changed Turn Over's name, it's either 12 Pound or 20 Pound Turn Over, depending on settings.

              At this stage, I cannot see how to progress any further with the emulation, so I have transferred
              a non-AWP version of this driver to MAME for further study.

  06-09-2006: Connect 4 added - VFD is currently reversed, although it looks to me like
              that's the correct behaviour, and that my 'correction' for Old Timer was wrong.
              AY sound does work, but is horrendous - presumably there's a filter on the
              sound that I haven't spotted. Trackball is apparently connected to AUX1, via
              Schmitt triggers - I wish I had a clue what this meant (El Condor).

  05-09-2006: And the award for most bone-headed bug goes to.. me! the 6840 handler wasn't
              resetting the clocks after timeout, so the wave frequencies were way off.
              Machines now hang on reset due to a lack of reel support.
              CHR decoding now included, but still requires knowledge of the data table
              location - this should be present in the ROMs somewhere.

  11-08-2006: It appears that the PIA IRQ's are not connected after all - but even
              disabling these won't get around the PTM issues.
              It also looks like the video card CHR is just a word-based version
              of the original, byte-based chip, meaning that should be fairly simple
              to emulate too, when the time comes.

  08-07-2006: Revised mapping of peripherals, and found method of calculating CHR
              values - although their use is still unknown.

  11-05-2006: El Condor, working from schematics and photos at present
  28-04-2006: El Condor
  20-05-2004: Re-Animator

  See http://www.mameworld.net/agemame/techinfo/mpu4.php for Information.

--- Board Setup ---

The MPU4 BOARD is the driver board, originally designed to run Fruit Machines made by the Barcrest Group, but later
licensed to other firms as a general purpose unit (even some old Photo-Me booths used the unit).

This original board uses a ~1.72 Mhz 6809B CPU, and a number of PIA6821 chips for multiplexing inputs and the like.

A 6840PTM is used for internal timing, one of it's functions is to act with an AY8913 chip as a crude analogue sound device.
(Data is transmitted through a PIA, with a square wave from the PTM being used as the alarm sound generator)

A MPU4 GAME CARD (cartridge) plugs into the MPU4 board containing the game, and a protection PAL (the 'characteriser').
This PAL, as well as protecting the games, also controlled some of the lamp address matrix for many games, and acted as
an anti-tampering device which helped to prevent the hacking of certain titles in a manner which broke UK gaming laws.

One of the advantages of the hardware setup was that the developer could change the nature of the game card
up to a point, adding extra lamp support, different amounts of RAM, and (in many cases) an OKI MSM6376 or Yamaha synth chip
and related PIA and PTM for improved audio (This was eventually made the only way to generate sound in MOD4 of the hardware,
when the AY8913 was removed from the main board)

For the Barcrest MPU4 Video system, the cartridge contains the MPU4 video bios in the usual ROM space (occupying 16k), an interface
interface card to connect an additional Video board, and a 6850 serial IO to communicate with said board.
This version of the game card does not have the OKI chip, or the characteriser.

The VIDEO BOARD is driven by a 10mhz 68000 processor, and contains a 6840PTM, 6850 serial IO (the other end of the
communications), an SAA1099 for stereo sound and SCN2674 gfx chip.

The VIDEO CARTRIDGE plugs into the video board, and contains the program roms for the video based game. Like the MPU4 game
card, in some cases an extra OKI sound chip is added to the video board's game card,as well as extra RAM.
There is a protection chip similar to and replacing the MPU4 Characteriser, which is often fed question data to descramble
(unknown how it works). In non-question cases, however, the protection chip works near identically to the original.

No video card schematics ever left the PCB factory, but some decent scans of the board have been made,
now also available for review.

Additional: 68k HALT line is tied to the reset circuit of the MPU4.

Emulating the standalone MPU4 is a priority, unless this works, the video won't run.
The copy of this driver held by AGEMAME is currently set up to run two UK standalone
games (Celebration Club and Connect 4), as well as a title using one of the later 'mods' (Old Timer,
a Dutch machine).
Everything here is preliminary...  the boards are quite fussy with regards their self tests
and the timing may have to be perfect for them to function correctly.  (as the comms are
timer driven, the video is capable of various raster effects etc.)

Datasheets are available for the main components, The AGEMAME site mirrors a few of the harder-to-find ones.

--- Preliminary MPU4 Memorymap  ---

(NV) indicates an item which is not present on the video version, which has a Comms card instead.

   hex     |r/w| D D D D D D D D |
 location  |   | 7 6 5 4 3 2 1 0 | function
-----------+---+-----------------+--------------------------------------------------------------------------
 0000-07FF |R/W| D D D D D D D D | 2k RAM
-----------+---+-----------------+--------------------------------------------------------------------------
 0800      |R/W|                 | Characteriser (Security PAL) (NV)
-----------+---+-----------------+--------------------------------------------------------------------------
 0850 ?    | W | ??????????????? | page latch (NV)
-----------+---+-----------------+--------------------------------------------------------------------------
 0880      |R/W| D D D D D D D D | PIA6821 on soundboard (Oki MSM6376@16MHz,(NV)
           |   |                 | port A = ??
           |   |                 | port B (882)
           |   |                 |        b7 = 0 if OKI busy
           |   |                 |             1 if OKI ready
           |   |                 |        b6 = ??
           |   |                 |        b5 = volume control clock
           |   |                 |        b4 = volume control direction (0= up, 1 = down)
           |   |                 |        b3 = ??
           |   |                 |        b2 = ??
           |   |                 |        b1 = ??
           |   |                 |        b0 = ??
-----------+---+-----------------+--------------------------------------------------------------------------
 08C0      |   |                 | MC6840 on sound board(NV?)
-----------+---+-----------------+--------------------------------------------------------------------------
 0900-     |R/W| D D D D D D D D | MC6840 PTM IC2


  Clock1 <--------------------------------------
     |                                          |
     V                                          |
  Output1 ---> Clock2                           |
                                                |
               Output2 --+-> Clock3             |
                         |                      |
                         |   Output3 ---> 'to audio amp' ??
                         |
                         +--------> CA1 IC3 (

IRQ line connected to CPU

-----------+---+-----------------+--------------------------------------------------------------------------
 0A00-0A03 |R/W| D D D D D D D D | PIA6821 IC3 port A Lamp Drives 1,2,3,4,6,7,8,9 (sic)(IC14)
           |   |                 |
           |   |                 |          CA1 <= output2 from PTM6840 (IC2)
           |   |                 |          CA2 => alpha data
           |   |                 |
           |   |                 |          port B Lamp Drives 10,11,12,13,14,15,16,17 (sic)(IC13)
           |   |                 |
           |   |                 |          CB2 => alpha reset (clock on Dutch systems)
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0B00-0B03 |R/W| D D D D D D D D | PIA6821 IC4 port A = data for 7seg leds (pins 10 - 17, via IC32)
           |   |                 |
           |   |                 |             CA1 INPUT, 50 Hz input (used to generate IRQ)
           |   |                 |             CA2 OUTPUT, connected to pin2 74LS138 CE for multiplexer
           |   |                 |                        (B on LED strobe multiplexer)
           |   |                 |             IRQA connected to IRQ of CPU
           |   |                 |             port B
           |   |                 |                    PB7 = INPUT, serial port Receive data (Rx)
           |   |                 |                    PB6 = INPUT, reel A sensor
           |   |                 |                    PB5 = INPUT, reel B sensor
           |   |                 |                    PB4 = INPUT, reel C sensor
           |   |                 |                    PB3 = INPUT, reel D sensor
           |   |                 |                    PB2 = INPUT, Connected to CA1 (50Hz signal)
           |   |                 |                    PB1 = INPUT, undercurrent sense
           |   |                 |                    PB0 = INPUT, overcurrent  sense
           |   |                 |
           |   |                 |             CB1 INPUT,  used to generate IRQ on edge of serial input line
           |   |                 |             CB2 OUTPUT, enable signal for reel optics
           |   |                 |             IRQB connected to IRQ of CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0C00-0C03 |R/W| D D D D D D D D | PIA6821 IC5 port A
           |   |                 |
           |   |                 |                    PA0-PA7, INPUT AUX1 connector
           |   |                 |
           |   |                 |             CA2  OUTPUT, serial port Transmit line
           |   |                 |             CA1  not connected
           |   |                 |             IRQA connected to IRQ of CPU
           |   |                 |
           |   |                 |             port B
           |   |                 |
           |   |                 |                    PB0-PB7 INPUT, AUX2 connector
           |   |                 |
           |   |                 |             CB1  INPUT,  connected to PB7 (Aux2 connector pin 4)
           |   |                 |
           |   |                 |             CB2  OUTPUT, AY8913 chip select line
           |   |                 |             IRQB connected to IRQ of CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0D00-0D03 |R/W| D D D D D D D D | PIA6821 IC6
           |   |                 |
           |   |                 |  port A
           |   |                 |
           |   |                 |        PA0 - PA7 (INPUT/OUTPUT) data port AY8913 sound chip
           |   |                 |
           |   |                 |        CA1 INPUT,  not connected
           |   |                 |        CA2 OUTPUT, BC1 pin AY8913 sound chip
           |   |                 |        IRQA , connected to IRQ CPU
           |   |                 |
           |   |                 |  port B
           |   |                 |
           |   |                 |        PB0-PB3 OUTPUT, reel A
           |   |                 |        PB4-PB7 OUTPUT, reel B
           |   |                 |
           |   |                 |        CB1 INPUT,  not connected
           |   |                 |        CB2 OUTPUT, B01R pin AY8913 sound chip
           |   |                 |        IRQB , connected to IRQ CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0E00-0E03 |R/W| D D D D D D D D | PIA6821 IC7
           |   |                 |
           |   |                 |  port A
           |   |                 |
           |   |                 |        PA0-PA3 OUTPUT, reel C
           |   |                 |        PA4-PA7 OUTPUT, reel D
           |   |                 |        CA1     INPUT,  not connected
           |   |                 |        CA2     OUTPUT, A on LED strobe multiplexer
           |   |                 |        IRQA , connected to IRQ CPU
           |   |                 |
           |   |                 |  port B
           |   |                 |
           |   |                 |        PB0-PB6 OUTPUT mech meter 1-7 or reel E + F
           |   |                 |        PB7     Voltage drop sensor
           |   |                 |        CB1     INPUT, not connected
           |   |                 |        CB2     OUTPUT,mech meter 8
           |   |                 |        IRQB , connected to IRQ CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0F00-0F03 |R/W| D D D D D D D D | PIA6821 IC8
           |   |                 |
           |   |                 | port A
           |   |                 |
           |   |                 |        PA0-PA7 INPUT  multiplexed inputs data
           |   |                 |
           |   |                 |        CA1     INPUT, not connected
           |   |                 |        CA2    OUTPUT, C on LED strobe multiplexer
           |   |                 |        IRQA           connected to IRQ CPU
           |   |                 |
           |   |                 | port B
           |   |                 |
           |   |                 |        PB0-PB7 OUTPUT  triacs outputs connector PL6
           |   |                 |        used for slides / hoppers
           |   |                 |
           |   |                 |        CB1     INPUT, not connected
           |   |                 |        CB2    OUTPUT, pin1 alpha display PL7 (clock signal)
           |   |                 |        IRQB           connected to IRQ CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 1000-FFFF | R | D D D D D D D D | ROM (can be bank switched by 0x850 in 8 banks of 64 k ) (NV)
-----------+---+-----------------+--------------------------------------------------------------------------

 TODO: - Input timing is completely wrong, need more info.
       - Get MPU4 board working properly, so that video layer will operate.
       - Confirm that MC6850 emulation is sufficient.
       - MPU4 Master clock value taken from schematic, but 68k value is not.
       - On IRQ, inhibit line goes low.
*****************************************************************************************/

#include "driver.h"
#include "machine/6821pia.h"
#include "machine/6840ptm.h"

// MPU4
#include "ui.h"
#include "timer.h"
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"
#include "sound/flt_rc.h"
#include "sound/2413intf.h"
#include "machine/steppers.h"	// stepper motor
#include "machine/roc10937.h"	// vfd
#include "machine/mmtr.h"

// Video
#include "cpu/m68000/m68000.h"
#include "machine/6850acia.h"
#include "sound/saa1099.h"

//Deal 'Em
#include "video/crtc6845.h"

#ifdef MAME_DEBUG
#define LOG(x)
#define LOG_CHR(x)
#define LOG_IC3(x)
#define LOG_IC8(x)
#define LOGSTUFF(x)
#else
#define LOG(x)
#define LOG_CHR(x)
#define LOG_IC3(x)
#define LOG_IC8(x)
#define LOGSTUFF(x)
#endif

#include "mpu4.lh"
#include "connect4.lh"
#define MPU4_MASTER_CLOCK (6880000)
#define VIDEO_MASTER_CLOCK (10000000)
//#include "video/awpvid.h"     //Fruit Machines Only
// local vars /////////////////////////////////////////////////////////////
static int mod_number;
static int mmtr_data;
static int alpha_data_line;
static int alpha_clock;
static int ay8913_address;
static int serial_data;
static int signal_50hz;
static int ic4_input_b;
static int IC23G1;
static int IC23G2A;
static int IC23G2B;
static int IC23GC;
static int IC23GB;
static int IC23GA;
static int prot_col;
static int lamp_col;
static int ic24_active;
static int led_extend;
static mame_timer *ic24_timer;
static mame_timer *freq_timer;
static TIMER_CALLBACK( ic24_timeout );
static TIMER_CALLBACK( freq_timeout );

static int	input_strobe;	  // IC23 74LS138 A = CA2 IC7, B = CA2 IC4, C = CA2 IC8
static UINT8  lamp_strobe,lamp_strobe2;
static UINT8  lamp_data;
static UINT8  aydata;

const UINT8 MPU4_chr_lut[72];
UINT8 MPU4_chr_data[72];
static UINT8 led_segs[8];
static UINT8 Lamps[128];		// 128 multiplexed lamps
//static UINT8 Lampscopy[128];
								// 32  multiplexed inputs - but a further 8 possible per AUX.
								// Two connectors 'orange' (sampled every 8ms) and 'black' (sampled every 16ms)
								// Each connector carries two banks of eight inputs and two enable signals
static int optic_pattern;

// Video

static UINT8 m6840_irq_state;
static UINT8 m6850_irq_state;
static UINT8 scn2674_irq_state;
static int serial_card_connected;

/* UART source/sinks */
static UINT8 m68k_m6809_line;
static UINT8 m6809_m68k_line;

static int mpu4_gfx_index;
static UINT16 * mpu4_vid_vidram;
static UINT8 * mpu4_vid_vidram_is_dirty;
static UINT16 * mpu4_vid_mainram;

static UINT8 scn2674_IR[16];
static UINT8 scn2675_IR_pointer;
static UINT8 scn2674_screen1_l;
static UINT8 scn2674_screen1_h;
static UINT8 scn2674_cursor_l;
static UINT8 scn2674_cursor_h;
static UINT8 scn2674_screen2_l;
static UINT8 scn2674_screen2_h;

// End of Video
/*
LED Segments related to pins (5 is not connected):
Unlike the controllers emulated in the layout code, each
segment of an MPU4 LED can be set individually, even
being used as individual lamps. However, we can get away
with settings like this in the majority of cases.
   _9_
  |   |
  3   8
  |   |
   _2_
  |   |
  4   7
  |_ _|
    6  1

8 display enables (pins 10 - 17)
*/

static void mpu4_draw_led(UINT8 id, UINT8 value)
{
	output_set_digit_value(id,value);
}

static void update_lamps(void)
{
	int i;

	for (i=0; i<8; i++)
	{
		Lamps[(8*input_strobe)+i]    = (lamp_strobe  & (1 << i)) != 0;
		Lamps[(8*input_strobe)+i+64] = (lamp_strobe2 & (1 << i)) != 0;
	}

	if (led_extend)
	{
		//Connect 4 uses 'programmable' displays, built from lights.
		UINT8 pled_segs[2] = {0,0};

		static const int lamps1[8] = { 106, 107, 108, 109, 104, 105, 110, 133 };
		static const int lamps2[8] = { 114, 115, 116, 117, 112, 113, 118, 119 };

		for (i=0; i<8; i++)
		{
			if (output_get_lamp_value(lamps1[i])) pled_segs[0] |= (1 << i);
			if (output_get_lamp_value(lamps2[i])) pled_segs[1] |= (1 << i);
		}

		mpu4_draw_led(8, pled_segs[0]);
		mpu4_draw_led(9, pled_segs[1]);
	}
}

static void draw_lamps(void)
{
	int i;

	for (i=0; i<8; i++)
	{
		output_set_lamp_value((8*input_strobe)+i, (Lamps[(8*input_strobe)+i]));
		output_set_lamp_value((8*input_strobe)+i+64, (Lamps[(8*input_strobe)+i+64]));
	}
}

// palette initialisation /////////////////////////////////////////////////

PALETTE_INIT( mpu4 )
{
	int i;
	static const rgb_t color[16] =
	{
		MAKE_RGB(0x00,0x00,0x00), MAKE_RGB(0x00,0x00,0xFF), MAKE_RGB(0x00,0xFF,0x00), MAKE_RGB(0x00,0xFF,0xFF),
		MAKE_RGB(0xFF,0x00,0x00), MAKE_RGB(0xFF,0x00,0xFF), MAKE_RGB(0xFF,0xFF,0x00), MAKE_RGB(0xFF,0xFF,0xFF),
		MAKE_RGB(0x80,0x80,0x80), MAKE_RGB(0x00,0x00,0x80), MAKE_RGB(0x00,0x80,0x00), MAKE_RGB(0x00,0x80,0x80),
		MAKE_RGB(0x80,0x00,0x00), MAKE_RGB(0x80,0x00,0x80), MAKE_RGB(0x80,0x80,0x00), MAKE_RGB(0x80,0x80,0x80)
	};

	for (i=0; i<16; i++)
		palette_set_color(machine, i, color[i]);
}

///////////////////////////////////////////////////////////////////////////
// called if board is reset ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static void mpu4_stepper_reset(void)
{
	int pattern = 0,i;
	for ( i = 0; i < 6; i++)
	{
		Stepper_reset_position(i);
		if ( Stepper_optic_state(i) ) pattern |= 1<<i;
	}
	optic_pattern = pattern;
}

static MACHINE_RESET( mpu4 )
{
	ROC10937_reset(0);	// reset display1

	mpu4_stepper_reset();

	lamp_strobe    = 0;
	lamp_strobe2   = 0;
	lamp_data      = 0;

	IC23GC    = 0;
	IC23GB    = 0;
	IC23GA    = 0;
	IC23G1    = 1;
	IC23G2A   = 0;
	IC23G2B   = 0;

	prot_col  = 0;

// init rom bank ////////////////////////////////////////////////////////

	{
		UINT8 *rom = memory_region(REGION_CPU1);

		memory_configure_bank(1, 0, 8, &rom[0x01000], 0x10000);

		memory_set_bank(1,0);//?
	}

}

///////////////////////////////////////////////////////////////////////////

static MACHINE_RESET( mpu4_vid )
{
	ROC10937_reset(0);	// reset display1

//  cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
	mpu4_stepper_reset();

	lamp_strobe    = 0;
	lamp_strobe2   = 0;
	lamp_data      = 0;

	IC23GC    = 0;
	IC23GB    = 0;
	IC23GA    = 0;
	IC23G1    = 1;
	IC23G2A   = 0;
	IC23G2B   = 0;

	prot_col  = 0;
}

///////////////////////////////////////////////////////////////////////////

static void cpu0_irq(int state)
{
	if (!serial_card_connected)
	{
		cpunum_set_input_line(0, M6809_IRQ_LINE, state?ASSERT_LINE:CLEAR_LINE);
		LOG(("6809 int%d \n",state));
	}
	else
	{
		cpunum_set_input_line(0, M6809_FIRQ_LINE, state?ASSERT_LINE:CLEAR_LINE);
		LOG(("6809 fint%d \n",state));
	}
}

///////////////////////////////////////////////////////////////////////////
static WRITE8_HANDLER( bankswitch_w )
{
	memory_set_bank(1,data & 0x07);
}

// IC2 6840 PTM handler ///////////////////////////////////////////////////////

static WRITE8_HANDLER( ic2_o1_callback )
{
	ptm6840_set_c2(0,data);

	// copy output value to IC2 c2
	// this output is the clock for timer2,
	// the output from timer2 is the input clock for timer3
	// the output from timer3 is used as a square wave for the alarm output
	// and as an external clock source for timer 1!

}

static WRITE8_HANDLER( ic2_o2_callback )
{
	pia_set_input_ca1(0, data); // copy output value to IC3 ca1
	ptm6840_set_c3(   0, data); // copy output value to IC2 c3
}

static WRITE8_HANDLER( ic2_o3_callback )
{
	ptm6840_set_c1(   0, data); // copy output value to IC2 c1

/*  if (serial_card_connected)
    {
        m6809_m68k_line=data;
    }*/
}

static const ptm6840_interface ptm_ic2_intf =
{
	MPU4_MASTER_CLOCK/4,
	{ 0,0,0 },
	{ ic2_o1_callback, ic2_o2_callback, ic2_o3_callback },
	cpu0_irq
};

/***************************************************************************
    6821 PIA handlers
***************************************************************************/

static WRITE8_HANDLER( pia_ic3_porta_w )
{
	LOG_IC3(("%04x IC3 PIA Port A Set to %2x (lamp strobes 1 - 9)\n", activecpu_get_previouspc(),data));

	lamp_strobe = data;
	update_lamps();
}

static WRITE8_HANDLER( pia_ic3_portb_w )
{
	LOG_IC3(("%04x IC3 PIA Port B Set to %2x  (lamp strobes 10 - 17)\n", activecpu_get_previouspc(),data));

	lamp_strobe2 = data;
	update_lamps();
}

static WRITE8_HANDLER( pia_ic3_ca2_w )
{
	LOG_IC3(("%04x IC3 PIA Write CA2 (alpha data), %02X\n", activecpu_get_previouspc(),data&0xFF));

	alpha_data_line = data;
	ROC10937_draw_16seg(0);
}

static WRITE8_HANDLER( pia_ic3_cb2_w )
{
	LOG_IC3(("%04x IC3 PIA Write CB (alpha reset), %02X\n",activecpu_get_previouspc(),data&0xff));

	if ( data ) ROC10937_reset(0);
	ROC10937_draw_16seg(0);
}

// IC3, lamp data lines + alpha numeric display

static const pia6821_interface pia_ic3_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia_ic3_porta_w, pia_ic3_portb_w, pia_ic3_ca2_w, pia_ic3_cb2_w,
	/*irqs   : A/B             */ cpu0_irq, cpu0_irq
};

/*---------------------------------------
   IC23 emulation
  ---------------------------------------
IC23 is a 74LS138 1-of-8 Decoder

It is used as a multiplexer for the LEDs, lamp selects and inputs.

*/

static void ic23_update(void)
{
	if (!IC23G2A)
	{
		if (!IC23G2B)
		{
			if (IC23G1)
			{
				if ( IC23GA ) input_strobe |= 0x01;
				else          input_strobe &= ~0x01;

				if ( IC23GB ) input_strobe |= 0x02;
				else          input_strobe &= ~0x02;

				if ( IC23GC ) input_strobe |= 0x04;
				else          input_strobe &= ~0x04;
			}
		}
	}
	else
	if ((IC23G2A)||(IC23G2B))
	{
		input_strobe = 0x00;
	}
}
/*---------------------------------------
   IC24 emulation
  ---------------------------------------
IC24 is a 74LS122 pulse generator

CLEAR and B2 are tied high and A1 and A2 tied low, meaning any pulse on B1 will give a low pulse on the output pin.
*/

static void ic24_output(int data)
{
	IC23G2A = data;
	ic23_update();
}

static void ic24_setup(void)
{
	if (IC23GA)
	{
		double duration = TIME_OF_74LS123((220*1000),(0.1*0.000001));
		if (!ic24_active)
		{
			ic24_output(0);
			mame_timer_adjust(ic24_timer, double_to_mame_time(duration), 0, time_zero);
			ic24_active = 1;
		}
		draw_lamps();
	}
}

static TIMER_CALLBACK( ic24_timeout )
{
	ic24_active = 0;
	ic24_output(1);
}

static WRITE8_HANDLER( pia_ic4_porta_w )
{
	led_segs[input_strobe] = data;
	mpu4_draw_led(input_strobe, led_segs[input_strobe]);
}

static READ8_HANDLER( pia_ic4_portb_r )
{
	if ( serial_data )
	{
		ic4_input_b |=  0x80;
		pia_set_input_cb1(1, 1);
	}
	else
	{
		ic4_input_b &= ~0x80;
		pia_set_input_cb1(1, 0);
	}

	if ( optic_pattern & 0x01 ) ic4_input_b |=  0x40; // reel A tab
	else                        ic4_input_b &= ~0x40;

	if ( optic_pattern & 0x02 ) ic4_input_b |=  0x20; // reel B tab
	else                        ic4_input_b &= ~0x20;

	if ( optic_pattern & 0x04 ) ic4_input_b |=  0x10; // reel C tab
	else                        ic4_input_b &= ~0x10;

	if ( optic_pattern & 0x08 ) ic4_input_b |=  0x08; // reel D tab
	else                        ic4_input_b &= ~0x08;

	if ( signal_50hz ) 			ic4_input_b |=  0x04; // 50 Hz
	else   	                    ic4_input_b &= ~0x04;

	// if ( lamp_overcurrent  ) ic4_input_b |= 0x02;
	// if ( lamp_undercurrent ) ic4_input_b |= 0x01;

	LOG_IC3(("%04x IC4 PIA Read of Port B %x\n",activecpu_get_previouspc(),ic4_input_b));
	return ic4_input_b;
}

static WRITE8_HANDLER( pia_ic4_ca2_w )
{
	LOG_IC3(("%04x IC4 PIA Write CA (input MUX strobe /LED B), %02X\n", activecpu_get_previouspc(),data&0xFF));

	IC23GB = data;
	ic23_update();
}

// IC4, 7 seg leds, 50Hz timer reel sensors, current sensors

static const pia6821_interface pia_ic4_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, pia_ic4_portb_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia_ic4_porta_w, 0, pia_ic4_ca2_w, 0,
	/*irqs   : A/B             */ cpu0_irq, cpu0_irq
};

static READ8_HANDLER( pia_ic5_porta_r )
{
	LOG(("%04x IC5 PIA Read of Port A (AUX1)\n",activecpu_get_previouspc()));
	return readinputportbytag("AUX1");
}

static READ8_HANDLER( pia_ic5_portb_r )
{
	LOG(("%04x IC5 PIA Read of Port B (coin input AUX2)\n",activecpu_get_previouspc()));
	coin_lockout_w(0, (pia_get_output_b(2) & 0x01) );
	coin_lockout_w(1, (pia_get_output_b(2) & 0x02) );
	coin_lockout_w(2, (pia_get_output_b(2) & 0x04) );
	coin_lockout_w(3, (pia_get_output_b(2) & 0x08) );
	return readinputportbytag("AUX2");
}

static WRITE8_HANDLER( pia_ic5_ca2_w )
{
	LOG(("%04x IC5 PIA Write CA2 (Serial Tx) %2x\n",activecpu_get_previouspc(),data));
	serial_data = data;
}

/* ---------------------------------------
   AY Chip sound function selection -
   ---------------------------------------
The databus of the AY sound chip is connected to IC6 Port A.
Data is read from/written to the AY chip through this port.

If this sounds familiar, Amstrad did something very similar with their home computers.

The PSG function, defined by the BC1,BC2 and BDIR signals, is controlled by CA2 and CB2 of IC6.

PSG function selection:
-----------------------
BDIR = IC6 CB2 and BC1 = IC6 CA2

Pin            | PSG Function
BDIR BC1       |
0    0         | Inactive
0    1         | Read from selected PSG register. When function is set, the PSG will make the register data available to Port A.
1    0         | Write to selected PSG register. When set, the PSG will take the data at Port A and write it into the selected PSG register.
1    1         | Select PSG register. When set, the PSG will take the data at Port A and select a register.
*/
/* PSG function selected */

static void update_ay(void)
{
	if (!pia_get_output_cb2(2))
	{
		switch (ay8913_address)
		{
  			case 0x00:
			{
				/* Inactive */
				break;
		    }
		  	case 0x01:
			{	/* CA2 = 1 CB2 = 0? : Read from selected PSG register and make the register data available to Port A */
				LOG(("AY8913 address = %d \n",pia_get_output_a(3)&0x0f));
				break;
		  	}
		  	case 0x02:
			{/* CA2 = 0 CB2 = 1? : Write to selected PSG register and write data to Port A */
	  			AY8910_write_port_0_w(0, pia_get_output_a(3));
				LOG(("AY Chip Write \n"));
				break;
	  		}
		  	case 0x03:
			{/* CA2 = 1 CB2 = 1? : The register will now be selected and the user can read from or write to it.  The register will remain selected until another is chosen.*/
	  			AY8910_control_port_0_w(0, pia_get_output_a(3));
				LOG(("AY Chip Select \n"));
				break;
	  		}
			default:
			{
				LOG(("AY Chip error \n"));
			}
		}
	}

}

static WRITE8_HANDLER( pia_ic5_cb2_w )
{
    update_ay();
}

static const pia6821_interface pia_ic5_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ pia_ic5_porta_r, pia_ic5_portb_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, pia_ic5_ca2_w,  pia_ic5_cb2_w,
	/*irqs   : A/B             */ cpu0_irq, cpu0_irq
};

static WRITE8_HANDLER( pia_ic6_portb_w )
{
	LOG(("%04x IC6 PIA Port B Set to %2x (Reel A and B)\n", activecpu_get_previouspc(),data));

	Stepper_update(0, (data >> 4) & 0x0F );
	Stepper_update(1, data        & 0x0F );

	if ( pia_get_output_cb2(1))
	{
		if ( Stepper_optic_state(0) ) optic_pattern |=  0x01;
		else                          optic_pattern &= ~0x01;
		if ( Stepper_optic_state(1) ) optic_pattern |=  0x02;
		else                          optic_pattern &= ~0x02;
	}
}

static WRITE8_HANDLER( pia_ic6_porta_w )
{
	LOG(("%04x IC6 PIA Write A %2x\n", activecpu_get_previouspc(),data));
	if (mod_number <4)
	{
	  	aydata = data;
	    update_ay();
	}
}

static WRITE8_HANDLER( pia_ic6_ca2_w )
{
	LOG(("%04x IC6 PIA write CA2 %2x (AY8913 BC1)\n", activecpu_get_previouspc(),data));
	if (mod_number <4)
	{
		if ( data ) ay8913_address |=  0x01;
		else        ay8913_address &= ~0x01;
		update_ay();
	}
}

static WRITE8_HANDLER( pia_ic6_cb2_w )
{
	LOG(("%04x IC6 PIA write CB2 %2x (AY8913 BCDIR)\n", activecpu_get_previouspc(),data));
	if (mod_number <4)
	{
		if ( data ) ay8913_address |=  0x02;
		else        ay8913_address &= ~0x02;
		update_ay();
	}
}

static const pia6821_interface pia_ic6_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia_ic6_porta_w, pia_ic6_portb_w, pia_ic6_ca2_w, pia_ic6_cb2_w,
	/*irqs   : A/B             */ cpu0_irq, cpu0_irq
};

static WRITE8_HANDLER( pia_ic7_porta_w )
{
	LOG(("%04x IC7 PIA Port A Set to %2x (Reel C and D)\n", activecpu_get_previouspc(),data));

	Stepper_update(2, (data >> 4) & 0x0F );
	Stepper_update(3, data        & 0x0F );

	if ( pia_get_output_cb2(1))
	{
		if ( Stepper_optic_state(2) ) optic_pattern |=  0x04;
		else                          optic_pattern &= ~0x04;
		if ( Stepper_optic_state(3) ) optic_pattern |=  0x08;
		else                          optic_pattern &= ~0x08;
	}
}

static WRITE8_HANDLER( pia_ic7_portb_w )
{
	int i;
	long cycles  = MAME_TIME_TO_CYCLES(0, mame_timer_get_time() );

// The meters are connected to a voltage drop sensor, where current
// flowing through them also passes through pin B7, meaning that when
// any meter is activated, pin B7 goes high.
// As for why they connected this to an output port rather than using
// CB1, no idea.
// This appears to have confounded the schematic drawer, who has assumed that
// all eight meters are driven from this port, giving the 8 line driver chip
// 9 connections in total.

	mmtr_data = data;
	if (mmtr_data)
	{
		pia_set_input_b(4,mmtr_data|0x80);
		for (i=0; i<8; i++)
		if ( mmtr_data & (1 << i) )	Mechmtr_update(i, cycles, mmtr_data & (1 << i) );
	}
	else
	{
		pia_set_input_b(4,mmtr_data&~0x80);
	}

	LOG(("%04x IC7 PIA Port B Set to %2x (Meters, Reel E and F)\n", activecpu_get_previouspc(),data));
}

static WRITE8_HANDLER( pia_ic7_ca2_w )
{
	LOG(("%04x IC7 PIA write CA2 %2x (input strobe bit 0 / LED A)\n", activecpu_get_previouspc(),data));

	IC23GA = data;
	ic24_setup();
	ic23_update();
}

static WRITE8_HANDLER( pia_ic7_cb2_w )
{
// The eighth meter is connected here, because the voltage sensor
// is on PB7.
	long cycles  = MAME_TIME_TO_CYCLES(0, mame_timer_get_time() );
	if (data)
	{
		pia_set_input_b(4,mmtr_data|0x80);
		Mechmtr_update(7, cycles, data );
	}
	LOG(("%04x IC7 PIA write CB2 %2x \n", activecpu_get_previouspc(),data));
}

static const pia6821_interface pia_ic7_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia_ic7_porta_w, pia_ic7_portb_w, pia_ic7_ca2_w, pia_ic7_cb2_w,
	/*irqs   : A/B             */ cpu0_irq, cpu0_irq
};

static READ8_HANDLER( pia_ic8_porta_r )
{
	static const UINT8 ports[8] = { 0, 1, 2, 3, 0, 1, 4, 5 };
	int input_read = ports[input_strobe];

	LOG_IC8(("%04x IC8 PIA Read of Port A (MUX input data)\n",activecpu_get_previouspc()));
// The orange inputs are polled twice as often as the black ones, for reasons of efficiency.
// This is achieved via connecting every input line to an AND gate, thus allowing two strobes
// to represent each orange input bank (strobes are active low).
	pia_set_input_cb1(2, (readinputportbytag("AUX2") & 0x80));
	return readinputport(input_read);
}

static WRITE8_HANDLER( pia_ic8_portb_w )
{
	int i;
	LOG_IC8(("%04x IC8 PIA Port B Set to %2x (OUTPUT PORT, TRIACS)\n", activecpu_get_previouspc(),data));
	for (i=0; i<8; i++)
		if ( data & (1 << i) )		output_set_indexed_value("triac", i, data & (1 << i));
}

static WRITE8_HANDLER( pia_ic8_ca2_w )
{
	LOG_IC8(("%04x IC8 PIA write CA2 (input_strobe bit 2 / LED C) %02X\n", activecpu_get_previouspc(), data & 0xFF));

	IC23GC = data;
	ic23_update();
}

static WRITE8_HANDLER( pia_ic8_cb2_w )
{
	LOG_IC8(("%04x IC8 PIA write CB2 (alpha clock) %02X\n", activecpu_get_previouspc(), data & 0xFF));

	if ( !alpha_clock && (data) )
	{
		ROC10937_shift_data(0, alpha_data_line&0x01?0:1);
	}
	alpha_clock = data;
	ROC10937_draw_16seg(0);
}

static const pia6821_interface pia_ic8_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ pia_ic8_porta_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, pia_ic8_portb_w, pia_ic8_ca2_w, pia_ic8_cb2_w,
	/*irqs   : A/B             */ cpu0_irq, cpu0_irq
};


// Video

/*************************************
 *
 *  Interrupt system
 *
 *************************************/

/* The interrupt system consists of a 74148 priority encoder
   with the following interrupt priorites.  A lower number
   indicates a lower priority:

    7 - Game Card
    6 - Game Card
    5 - Game Card
    4 - Game Card
    3 - 2674 AVDC
    2 - 6850 ACIA
    1 - 6840 PTM
    0 - Unused
*/

static void update_mpu68_interrupts(void)
{
	int newstate = 0;

	if (m6840_irq_state)//1
		newstate = 1;
	if (m6850_irq_state)//2
		newstate = 2;
	if (scn2674_irq_state)//3
		newstate = 3;

	/* set the new state of the IRQ lines */
	if (newstate)
	{
		LOG(("68k IRQ, %x\n", newstate));
		cpunum_set_input_line(1, newstate, ASSERT_LINE);
	}
	else
	{
		LOG(("68k IRQ Clear, %x\n", newstate));
		cpunum_set_input_line(1, 7, CLEAR_LINE);
	}
//  m6840_irq_state = 0;
//  m6850_irq_state = 0;
//  scn2674_irq_state = 0;

}

// Communications ////////////////////////////////////////////////////////
/* Clock values are currently unknown, and are derived from the 68k board.
For now, it's fixed to the frequency set by the PTM on initialisation.*/

static void cpu1_acia_irq(int state)
{
	cpunum_set_input_line(0, M6809_IRQ_LINE, state?ASSERT_LINE:CLEAR_LINE);
	m6850_irq_state = state;
	update_mpu68_interrupts();
	LOG(("acia irq \n"));
}

static struct acia6850_interface m6809_acia_if =
{
	MPU4_MASTER_CLOCK/44,//This isn't a real division, this is just to work around
	MPU4_MASTER_CLOCK/44,//the ACIA code's hard coding
	&m68k_m6809_line,
	&m6809_m68k_line,
	cpu1_acia_irq
};

static struct acia6850_interface m68k_acia_if =
{
	MPU4_MASTER_CLOCK/44,
	MPU4_MASTER_CLOCK/44,
	&m6809_m68k_line,
	&m68k_m6809_line,
	0
};

static void cpu1_irq(int state)
{
	LOG(("68k IRQ set, %x\n", state));
	m6840_irq_state = state;
	update_mpu68_interrupts();
}

static WRITE8_HANDLER( vid_o1_callback )
{
	ptm6840_set_c2(   1, data); // copy output value to c2
}

static WRITE8_HANDLER( vid_o2_callback )
{
	ptm6840_set_c3(   1, data); // copy output value to c3
	m6809_m68k_line=data;
	m68k_m6809_line=data;
}

static WRITE8_HANDLER( vid_o3_callback )
{
	ptm6840_set_c1(   1, data); // copy output value to c1
}

//Again, this PTM clock might be wrong, it needs tracing from the PCB
static const ptm6840_interface ptm_vid_intf =
{
	VIDEO_MASTER_CLOCK/10,
	{ MPU4_MASTER_CLOCK/4,0,0 },
	{ vid_o1_callback, vid_o2_callback, vid_o3_callback },
	cpu1_irq
};

// SCN2674 AVDC emulation

/* the chip is actually more complex than this.. character aren't limited to 8 rows high... but I
 don't *think* the MPU4 stuff needs otherwise.. yet .. */

static const gfx_layout mpu4_vid_char_8x8_layout =
{
	8,8,
	0x1000, // 0x1000 tiles (128k of GFX Ram, 0x20 bytes per tile)
	4,
	{ 0,8,16,24 },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32},
	8*32
};

/* double height */
static const gfx_layout mpu4_vid_char_8x16_layout =
{
	8,16,
	0x1000, // 0x1000 tiles (128k of GFX Ram, 0x20 bytes per tile)
	4,
	{ 0,8,16,24 },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*32, 0*32, 1*32, 1*32, 2*32, 2*32, 3*32, 3*32, 4*32, 4*32, 5*32, 5*32, 6*32, 6*32, 7*32, 7*32},
	8*32
};

/* double width */
static const gfx_layout mpu4_vid_char_16x8_layout =
{
	16,8,
	0x1000, // 0x1000 tiles (128k of GFX Ram, 0x20 bytes per tile)
	4,
	{ 0,8,16,24 },
	{ 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32},
	8*32
};

/* double height & width */
static const gfx_layout mpu4_vid_char_16x16_layout =
{
	16,16,
	0x1000, // 0x1000 tiles (128k of GFX Ram, 0x20 bytes per tile)
	4,
	{ 0,8,16,24 },
	{ 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 },
	{ 0*32, 0*32, 1*32, 1*32, 2*32, 2*32, 3*32, 3*32, 4*32, 4*32, 5*32, 5*32, 6*32, 6*32, 7*32, 7*32},
	8*32
};

///////////////////////////////////////////////////////////////////////////

static UINT8 IR0_scn2674_double_ht_wd;
static UINT8 IR0_scn2674_scanline_per_char_row;
static UINT8 IR0_scn2674_sync_select;
static UINT8 IR0_scn2674_buffer_mode_select;
static UINT8 IR1_scn2674_interlace_enable;
static UINT8 IR1_scn2674_equalizing_constant;
static UINT8 IR2_scn2674_row_table;
static UINT8 IR2_scn2674_horz_sync_width;
static UINT8 IR2_scn2674_horz_back_porch;
static UINT8 IR3_scn2674_vert_front_porch;
static UINT8 IR3_scn2674_vert_back_porch;
static UINT8 IR4_scn2674_rows_per_screen;
static UINT8 IR4_scn2674_character_blink_rate;
static UINT8 IR5_scn2674_character_per_row;
static UINT8 IR8_scn2674_display_buffer_first_address_LSB;
static UINT8 IR9_scn2674_display_buffer_first_address_MSB;
static UINT8 IR9_scn2674_display_buffer_last_address;
static UINT8 IR10_scn2674_display_pointer_address_lower;
static UINT8 IR11_scn2674_display_pointer_address_upper;
static UINT8 IR12_scn2674_scroll_start;
static UINT8 IR12_scn2674_split_register_1;
static UINT8 IR13_scn2674_scroll_end;
static UINT8 IR13_scn2674_split_register_2;

VIDEO_UPDATE( mpu4_vid )
{
	int i;

	int x,y,count = 0;

	fillbitmap(bitmap,machine->pens[0],cliprect);

	for (i=0; i < 0x1000; i++)
		if (mpu4_vid_vidram_is_dirty[i]==1)
		{
			decodechar(machine->gfx[mpu4_gfx_index+0], i, (UINT8 *)mpu4_vid_vidram, &mpu4_vid_char_8x8_layout);
			decodechar(machine->gfx[mpu4_gfx_index+1], i, (UINT8 *)mpu4_vid_vidram, &mpu4_vid_char_8x16_layout);
			decodechar(machine->gfx[mpu4_gfx_index+2], i, (UINT8 *)mpu4_vid_vidram, &mpu4_vid_char_16x8_layout);
			decodechar(machine->gfx[mpu4_gfx_index+3], i, (UINT8 *)mpu4_vid_vidram, &mpu4_vid_char_16x16_layout);
			mpu4_vid_vidram_is_dirty[i]=0;
		}


	/* this is in main ram.. i think it must transfer it out of here??? */
	// count = 0x0018b6/2; // crmaze
	// count = 0x004950/2; // turnover

	/* we're in row table mode...thats why */

	for(y=0;y<=IR4_scn2674_rows_per_screen;y++)
	{
		int screen2_base = (scn2674_screen2_h<<8)|(scn2674_screen2_l);

		UINT16 rowbase = (mpu4_vid_mainram[1+screen2_base+(y*2)]<<8)|mpu4_vid_mainram[screen2_base+(y*2)];
		int dbl_size;
		int gfxregion = 0;

		dbl_size = (rowbase & 0xc000)>>14;  // ONLY if double size is enabled.. otherwise it can address more chars given more RAM

		if (dbl_size&2) gfxregion = 1;


		for(x=0;x<=IR5_scn2674_character_per_row;x++)
		{
			UINT16 tiledat;
			UINT16 colattr;

			tiledat = mpu4_vid_mainram[(rowbase+x)&0x7fff];
			colattr = tiledat >>12;
			tiledat &= 0x0fff;

			drawgfx(bitmap,machine->gfx[gfxregion],tiledat,colattr,0,0,x*8,y*8,cliprect,TRANSPARENCY_NONE,0);

			count++;
		}

		if (dbl_size&2) y++; // skip a row?

	}

	popmessage("%02x %02x %02x %02x %02x %02x",scn2674_screen1_l,scn2674_screen1_h,scn2674_cursor_l, scn2674_cursor_h,scn2674_screen2_l,scn2674_screen2_h);

	for (i=0; i<8; i++)
		mpu4_draw_led(i, led_segs[i]);

	return 0;
}

READ16_HANDLER( mpu4_vid_vidram_r )
{
	return mpu4_vid_vidram[offset];
}

WRITE16_HANDLER( mpu4_vid_vidram_w )
{
	COMBINE_DATA(&mpu4_vid_vidram[offset]);
	offset <<= 1;
	mpu4_vid_vidram_is_dirty[offset/0x20]=1;
}

/*
SCN2674 - Advanced Video Display Controller (AVDC)  (Video Chip)

15 Initialization Registers (8-bit each)

-- fill me in later ---

IR0  ---- ----
IR1  ---- ----
IR2  ---- ----
IR3  ---- ----
IR4  ---- ----
IR5  ---- ----
IR6  ---- ----
IR7  ---- ----
IR8  ---- ----
IR9  ---- ----
IR10 ---- ----
IR11 ---- ----
IR12 ---- ----
IR13 ---- ----
IR14 ---- ----

*/

static void scn2674_write_init_regs(UINT8 data)
{
	LOGSTUFF(("scn2674_write_init_regs %02x %02x\n",scn2675_IR_pointer,data));

	scn2674_IR[scn2675_IR_pointer]=data;


	switch ( scn2675_IR_pointer) // display some debug info, set mame specific variables
	{
		case 0:
			IR0_scn2674_double_ht_wd = (data & 0x80)>>7;
			IR0_scn2674_scanline_per_char_row = (data & 0x78)>>3;
			IR0_scn2674_sync_select = (data&0x04)>>2;
			IR0_scn2674_buffer_mode_select = (data&0x03);

		   	LOGSTUFF(("IR0 - Double Ht Wd %02x\n",IR0_scn2674_double_ht_wd));
		   	LOGSTUFF(("IR0 - Scanlines per Character Row %02x\n",IR0_scn2674_scanline_per_char_row));
		   	LOGSTUFF(("IR0 - Sync Select %02x\n",IR0_scn2674_sync_select));
		   	LOGSTUFF(("IR0 - Buffer Mode Select %02x\n",IR0_scn2674_buffer_mode_select));
			break;

		case 1:
			IR1_scn2674_interlace_enable = (data&0x80)>>7;
			IR1_scn2674_equalizing_constant = (data&0x7f);

		   	LOGSTUFF(("IR1 - Interlace Enable %02x\n",IR1_scn2674_interlace_enable));
			LOGSTUFF(("IR1 - Equalizing Constant %02x\n",IR1_scn2674_equalizing_constant));
			break;

		case 2:
			IR2_scn2674_row_table = (data&0x80)>>7;
			IR2_scn2674_horz_sync_width = (data&0x78)>>3;
			IR2_scn2674_horz_back_porch = (data&0x07);

		   	LOGSTUFF(("IR2 - Row Table %02x\n",IR2_scn2674_row_table));
			LOGSTUFF(("IR2 - Horizontal Sync Width %02x\n",IR2_scn2674_horz_sync_width));
			LOGSTUFF(("IR2 - Horizontal Back Porch %02x\n",IR2_scn2674_horz_back_porch));
			break;

		case 3:
			IR3_scn2674_vert_front_porch = (data&0xe0)>>5;
			IR3_scn2674_vert_back_porch = (data&0x1f)>>0;

		   	LOGSTUFF(("IR3 - Vertical Front Porch %02x\n",IR3_scn2674_vert_front_porch));
		   	LOGSTUFF(("IR3 - Vertical Back Porch %02x\n",IR3_scn2674_vert_back_porch));
			break;

		case 4:
		   	IR4_scn2674_rows_per_screen = data&0x7f;
			IR4_scn2674_character_blink_rate = (data & 0x80)>>7;

		   	LOGSTUFF(("IR4 - Rows Per Screen %02x\n",IR4_scn2674_rows_per_screen));
		   	LOGSTUFF(("IR4 - Character Blink Rate %02x\n",IR4_scn2674_character_blink_rate));
			break;

		case 5:
		   /* IR5 - Active Characters Per Row
             cccc cccc
             c = Characters Per Row */
		   	IR5_scn2674_character_per_row = data;
		   	LOGSTUFF(("IR5 - Active Characters Per Row %02x\n",IR5_scn2674_character_per_row));
			break;

		case 6:
			break;

		case 7:
			break;

		case 8:
			IR8_scn2674_display_buffer_first_address_LSB = data;
		   	LOGSTUFF(("IR8 - Display Buffer First Address LSB %02x\n",IR8_scn2674_display_buffer_first_address_LSB));
			break;

		case 9:
			IR9_scn2674_display_buffer_first_address_MSB = data & 0x0f;
			IR9_scn2674_display_buffer_last_address = (data & 0xf0)>>4;
		   	LOGSTUFF(("IR9 - Display Buffer First Address MSB %02x\n",IR9_scn2674_display_buffer_first_address_MSB));
		   	LOGSTUFF(("IR9 - Display Buffer Last Address %02x\n",IR9_scn2674_display_buffer_last_address));
			break;

		case 10:
			IR10_scn2674_display_pointer_address_lower = data;
		   	LOGSTUFF(("IR10 - Display Pointer Address Lower %02x\n",IR10_scn2674_display_pointer_address_lower));
			break;

		case 11:
			IR11_scn2674_display_pointer_address_upper= data&0x3f;
		   	LOGSTUFF(("IR11 - Display Pointer Address Lower %02x\n",IR11_scn2674_display_pointer_address_upper));
			break;

		case 12:
			IR12_scn2674_scroll_start = (data & 0x80)>>7;
			IR12_scn2674_split_register_1 = (data & 0x7f);
		   	LOGSTUFF(("IR12 - Scroll Start %02x\n",IR12_scn2674_scroll_start));
		   	LOGSTUFF(("IR12 - Split Register 1 %02x\n",IR12_scn2674_split_register_1));
			break;

		case 13:
			IR13_scn2674_scroll_end = (data & 0x80)>>7;
			IR13_scn2674_split_register_2 = (data & 0x7f);
		   	LOGSTUFF(("IR13 - Scroll End %02x\n",IR13_scn2674_scroll_end));
		   	LOGSTUFF(("IR13 - Split Register 2 %02x\n",IR13_scn2674_split_register_2));
			break;

		case 14:
			break;

		case 15: // not valid!
			break;


	}

	scn2675_IR_pointer++;
	if (scn2675_IR_pointer>14)scn2675_IR_pointer=14;
}

static UINT8 scn2674_irq_register = 0;
static UINT8 scn2674_status_register = 0;
static UINT8 scn2674_irq_mask = 0;
static UINT8 scn2674_gfx_enabled;
static UINT8 scn2674_display_enabled;
static UINT8 scn2674_cursor_enabled;

static void scn2674_write_command(UINT8 data)
{
	UINT8 oprand;

	LOGSTUFF(("scn2674_write_command %02x\n",data));

	if (data==0x00)
	{
		// master reset
		LOGSTUFF(("master reset %02x\n",data));
	}

	if ((data&0xf0)==0x10)
	{
		// set IR pointer
		LOGSTUFF(("set IR pointer %02x\n",data));

		oprand = data & 0x0f;
		scn2675_IR_pointer=oprand;

	}

	/*** ANY COMBINATION OF THESE ARE POSSIBLE */

	if ((data&0xe3)==0x22)
	{
		// Disable GFX
		LOGSTUFF(("disable GFX %02x\n",data));
		scn2674_gfx_enabled = 0;
	}

	if ((data&0xe3)==0x23)
	{
		// Enable GFX
		LOGSTUFF(("enable GFX %02x\n",data));
		scn2674_gfx_enabled = 1;
	}

	if ((data&0xe9)==0x28)
	{
		// Display off
		oprand = data & 0x04;

		scn2674_display_enabled = 0;

		if (oprand)
			LOGSTUFF(("display OFF - float DADD bus %02x\n",data));
		else
			LOGSTUFF(("display OFF - no float DADD bus %02x\n",data));
	}

	if ((data&0xe9)==0x29)
	{
		// Display on
		oprand = data & 0x04;

		scn2674_display_enabled = 1;

		if (oprand)
			LOGSTUFF(("display ON - next field %02x\n",data));
		else
			LOGSTUFF(("display ON - next scanline %02x\n",data));

	}

	if ((data&0xf1)==0x30)
	{
		// Cursor Off
		LOGSTUFF(("cursor off %02x\n",data));
		scn2674_cursor_enabled = 0;
	}

	if ((data&0xf1)==0x31)
	{
		// Cursor On
		LOGSTUFF(("cursor on %02x\n",data));
		scn2674_cursor_enabled = 1;
	}

	/*** END ***/

	if ((data&0xe0)==0x40)
	{
		// Reset Interrupt / Status bit
		oprand = data & 0x1f;
		LOGSTUFF(("reset interrupt / status bit %02x\n",data));

		LOGSTUFF(("Split 2   IRQ: %d Reset\n",(data>>0)&1));
		LOGSTUFF(("Ready     IRQ: %d Reset\n",(data>>1)&1));
		LOGSTUFF(("Split 1   IRQ: %d Reset\n",(data>>2)&1));
		LOGSTUFF(("Line Zero IRQ: %d Reset\n",(data>>3)&1));
		LOGSTUFF(("V-Blank   IRQ: %d Reset\n",(data>>4)&1));

		scn2674_irq_register &= ((data & 0x1f)^0x1f);
		scn2674_status_register &= ((data & 0x1f)^0x1f);

		if(data&0x10) //cpunum_set_input_line(1, 3, ASSERT_LINE); // maybe .. or maybe it just changes the register
		{
			scn2674_irq_state = 1;
			update_mpu68_interrupts();
		}
		else
		scn2674_irq_state = 0;
		update_mpu68_interrupts();
	}
	if ((data&0xe0)==0x80)
	{
		// Disable Interrupt
		oprand = data & 0x1f;
		LOGSTUFF(("disable interrupt %02x\n",data));
		LOGSTUFF(("Split 2   IRQ: %d Disabled\n",(data>>0)&1));
		LOGSTUFF(("Ready     IRQ: %d Disabled\n",(data>>1)&1));
		LOGSTUFF(("Split 1   IRQ: %d Disabled\n",(data>>2)&1));
		LOGSTUFF(("Line Zero IRQ: %d Disabled\n",(data>>3)&1));
		LOGSTUFF(("V-Blank   IRQ: %d Disabled\n",(data>>4)&1));

//      scn2674_irq_mask &= ((data & 0x1f)^0x1f); // disables.. doesn't enable?

		scn2674_irq_mask &= ~(data & 0x1f);
	}

	if ((data&0xe0)==0x60)
	{
		// Enable Interrupt
		LOGSTUFF(("enable interrupt %02x\n",data));
		LOGSTUFF(("Split 2   IRQ: %d Enabled\n",(data>>0)&1));
		LOGSTUFF(("Ready     IRQ: %d Enabled\n",(data>>1)&1));
		LOGSTUFF(("Split 1   IRQ: %d Enabled\n",(data>>2)&1));
		LOGSTUFF(("Line Zero IRQ: %d Enabled\n",(data>>3)&1));
		LOGSTUFF(("V-Blank   IRQ: %d Enabled\n",(data>>4)&1));

		scn2674_irq_mask |= (data & 0x1f);  // enables .. doesn't disable?

	}

	/**** Delayed Commands ****/

	if (data == 0xa4)
	{
		// read at pointer address
		LOGSTUFF(("read at pointer address %02x\n",data));
	}

	if (data == 0xa2)
	{
		// write at pointer address
		LOGSTUFF(("write at pointer address %02x\n",data));
	}

	if (data == 0xa9)
	{
		// increase cursor address
		LOGSTUFF(("increase cursor address %02x\n",data));
	}

	if (data == 0xac)
	{
		// read at cursor address
		LOGSTUFF(("read at cursor address %02x\n",data));
	}

	if (data == 0xaa)
	{
		// write at cursor address
		LOGSTUFF(("write at cursor address %02x\n",data));
	}

	if (data == 0xad)
	{
		// read at cursor address + incrememnt
		LOGSTUFF(("read at cursor address+increment %02x\n",data));
	}

	if (data == 0xab)
	{
		// write at cursor address + incrememnt
		LOGSTUFF(("write at cursor address+increment %02x\n",data));
	}

	if (data == 0xbb)
	{
		// write from cursor address to pointer address
		LOGSTUFF(("write from cursor address to pointer address %02x\n",data));
	}

	if (data == 0xbd)
	{
		// read from cursor address to pointer address
		LOGSTUFF(("read from cursor address to pointer address %02x\n",data));
	}

}

READ16_HANDLER( mpu4_vid_scn2674_r )
{
	/*
    Offset:  Purpose
     0       Interrupt Register
     1       Status Register
     2       Screen Start 1 Lower Register
     3       Screen Start 1 Upper Register
     4       Cursor Address Lower Register
     5       Cursor Address Upper Register
     6       Screen Start 2 Lower Register
     7       Screen Start 2 Upper Register
    */

	switch (offset)
	{

		/*  Status / Irq Register

            --RV ZSRs

            -- = ALWAYS 0
            R  = RDFLG (Status Register Only)
            V  = Vblank
            Z  = Line Zero
            S  = Split 1
            R  = Ready
            s  = Split 2
        */
		case 0:
			LOGSTUFF(("Read Irq Register %06x\n",activecpu_get_pc()));
			return scn2674_irq_register;

		case 1:
			LOGSTUFF(("Read Status Register %06x\n",activecpu_get_pc()));
			return scn2674_status_register;

		case 2: LOGSTUFF(("Read Screen1_l Register %06x\n",activecpu_get_pc()));return scn2674_screen1_l;
		case 3: LOGSTUFF(("Read Screen1_h Register %06x\n",activecpu_get_pc()));return scn2674_screen1_h;
		case 4: LOGSTUFF(("Read Cursor_l Register %06x\n",activecpu_get_pc()));return scn2674_cursor_l;
		case 5: LOGSTUFF(("Read Cursor_h Register %06x\n",activecpu_get_pc()));return scn2674_cursor_h;
		case 6:	LOGSTUFF(("Read Screen2_l Register %06x\n",activecpu_get_pc()));return scn2674_screen2_l;
		case 7: LOGSTUFF(("Read Screen2_h Register %06x\n",activecpu_get_pc()));return scn2674_screen2_h;
	}

	return 0xffff;
}

WRITE16_HANDLER( mpu4_vid_scn2674_w )
{
	/*
    Offset:  Purpose
     0       Initialization Registers
     1       Command Register
     2       Screen Start 1 Lower Register
     3       Screen Start 1 Upper Register
     4       Cursor Address Lower Register
     5       Cursor Address Upper Register
     6       Screen Start 2 Lower Register
     7       Screen Start 2 Upper Register
    */

	data &=0x00ff; // it's an 8-bit chip on a 16-bit board, feel the cheapness.

	switch (offset)
	{
		case 0:
			scn2674_write_init_regs(data);
			break;

		case 1:
			scn2674_write_command(data);
			break;

		case 2: scn2674_screen1_l = data; break;
		case 3: scn2674_screen1_h = data; break;
		case 4: scn2674_cursor_l  = data; break;
		case 5: scn2674_cursor_h  = data; break;
		case 6:	scn2674_screen2_l = data; break;
		case 7: scn2674_screen2_h = data; break;
	}
}

READ16_HANDLER( mpu4_vid_unmap_r )
{
	//logerror("Data %s Offset %s", data, offset);
	return mame_rand(Machine);
}

WRITE16_HANDLER( mpu4_vid_unmap_w )
{
	LOG(("WData %d WOffset %d", data, offset));
}

VIDEO_START( mpu4_vid )
{
	/* if anything uses tile sizes other than 8x8 we can't really do it this way.. we'll have to draw tiles by hand.
      maybe we will anyway, but for now we don't need to */

	mpu4_vid_vidram = auto_malloc (0x20000);
	mpu4_vid_vidram_is_dirty = auto_malloc(0x1000);

	memset(mpu4_vid_vidram_is_dirty,1,0x1000);
	memset(mpu4_vid_vidram,0,0x20000);

 	/* find first empty slot to decode gfx */
	for (mpu4_gfx_index = 0; mpu4_gfx_index < MAX_GFX_ELEMENTS; mpu4_gfx_index++)
		if (machine->gfx[mpu4_gfx_index] == 0)
			break;

	assert(mpu4_gfx_index != MAX_GFX_ELEMENTS);

	/* create the char set (gfx will then be updated dynamically from RAM) */
	machine->gfx[mpu4_gfx_index+0] = allocgfx(&mpu4_vid_char_8x8_layout);
	machine->gfx[mpu4_gfx_index+1] = allocgfx(&mpu4_vid_char_8x16_layout);
	machine->gfx[mpu4_gfx_index+2] = allocgfx(&mpu4_vid_char_16x8_layout);
	machine->gfx[mpu4_gfx_index+3] = allocgfx(&mpu4_vid_char_16x16_layout);

	/* set the color information */
	machine->gfx[mpu4_gfx_index+0]->total_colors = machine->drv->total_colors / 16;
	machine->gfx[mpu4_gfx_index+1]->total_colors = machine->drv->total_colors / 16;
	machine->gfx[mpu4_gfx_index+2]->total_colors = machine->drv->total_colors / 16;
	machine->gfx[mpu4_gfx_index+3]->total_colors = machine->drv->total_colors / 16;

	scn2675_IR_pointer = 0;
}

/* palette support is very preliminary */
static UINT8 ef9369_palette[32];
static UINT8 ef9369_counter;

static WRITE16_HANDLER( ef9369_data_w )
{
	int color;
	UINT16 coldat;

	data &=0x00ff;

	ef9369_palette[ef9369_counter] = data;

	color = ef9369_counter/2;
	coldat = (ef9369_palette[color*2+1]<<8)|ef9369_palette[color*2];
	palette_set_color_rgb(Machine,color,pal4bit(coldat >> 0),pal4bit(coldat >> 8),pal4bit(coldat >> 4));

	ef9369_counter++;
	if (ef9369_counter>31) ef9369_counter = 31;
}

static WRITE16_HANDLER( ef9369_address_w )
{
	data &=0x00ff;

	ef9369_counter = data*2; // = 0?

	if (ef9369_counter>30) ef9369_counter = 30;

}

// input ports for MPU4 board ////////////////////////////////////////

INPUT_PORTS_START( mpu4 )
	PORT_START_TAG("ORANGE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("00")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("01")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("02")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("03")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("04")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("05")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("06")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("07")

	PORT_START_TAG("ORANGE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("08")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("09")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("10")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("11")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("12")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("13")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("14")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("15")

	PORT_START_TAG("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("16")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("17")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("18")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("19")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("20")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Test Switch")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Door Switch?") PORT_TOGGLE

	PORT_START_TAG("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("24")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("25")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("26")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("27")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("28")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("29")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("30")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("31")

	PORT_START_TAG("DIL1")
	PORT_DIPNAME( 0x01, 0x00, "DIL101" ) PORT_DIPLOCATION("DIL1:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL102" ) PORT_DIPLOCATION("DIL1:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL103" ) PORT_DIPLOCATION("DIL1:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL104" ) PORT_DIPLOCATION("DIL1:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL105" ) PORT_DIPLOCATION("DIL1:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL106" ) PORT_DIPLOCATION("DIL1:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL107" ) PORT_DIPLOCATION("DIL1:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "DIL108" ) PORT_DIPLOCATION("DIL1:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START_TAG("DIL2")
	PORT_DIPNAME( 0x01, 0x00, "DIL201" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL202" ) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL203" ) PORT_DIPLOCATION("DIL2:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL204" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL205" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL206" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL207" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "DIL208" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START_TAG("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("0")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("2")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("3")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("4")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("5")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("6")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("7")

	PORT_START_TAG("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")PORT_IMPULSE(5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")PORT_IMPULSE(5)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")PORT_IMPULSE(5)

INPUT_PORTS_END

INPUT_PORTS_START( connect4 )
	PORT_START_TAG("ORANGE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("00")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("01")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("02")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("03")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("04")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("05")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("06")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("07")

	PORT_START_TAG("ORANGE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("08")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("09")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("10")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("11")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("12")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("13")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("14")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("15")

	PORT_START_TAG("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("16")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("17")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("18")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("19")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("20")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Door Switch?") PORT_TOGGLE

	PORT_START_TAG("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Select")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("25")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_START2) PORT_NAME("Pass")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Play")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("28")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("29")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("30")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Drop")

	PORT_START_TAG("DIL1")
	PORT_DIPNAME( 0x01, 0x00, "DIL101" ) PORT_DIPLOCATION("DIL1:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL102" ) PORT_DIPLOCATION("DIL1:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL103" ) PORT_DIPLOCATION("DIL1:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL104" ) PORT_DIPLOCATION("DIL1:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL105" ) PORT_DIPLOCATION("DIL1:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL106" ) PORT_DIPLOCATION("DIL1:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL107" ) PORT_DIPLOCATION("DIL1:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "Invert Alpha?" ) PORT_DIPLOCATION("DIL1:08")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes  ) )

	PORT_START_TAG("DIL2")
	PORT_DIPNAME( 0x01, 0x00, "DIL201" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL202" ) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL203" ) PORT_DIPLOCATION("DIL2:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL204" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL205" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL206" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL207" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "DIL208" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START_TAG("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("0")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("2")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("3")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("4")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("5")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("6")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("7")

	PORT_START_TAG("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")PORT_IMPULSE(5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")PORT_IMPULSE(5)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")PORT_IMPULSE(5)

INPUT_PORTS_END

INPUT_PORTS_START( crmaze )
	PORT_START_TAG("ORANGE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("00")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("01")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("02")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("03")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("04")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("05")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("06")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("07")

	PORT_START_TAG("ORANGE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("08")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("09")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("10")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("11")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("12")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("13")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("14")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("200p?")

	PORT_START_TAG("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("16")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("17")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("18")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("19")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("20")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Door Switch?") PORT_TOGGLE

	PORT_START_TAG("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Right Yellow")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Right Red")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("26")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Left Yellow")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Left Red")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Getout Yellow")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Getout Red")//Labelled Escape on cabinet
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("100p Service?")PORT_IMPULSE(100)

	PORT_START_TAG("DIL1")
	PORT_DIPNAME( 0x01, 0x00, "DIL101" ) PORT_DIPLOCATION("DIL1:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL102" ) PORT_DIPLOCATION("DIL1:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL103" ) PORT_DIPLOCATION("DIL1:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL104" ) PORT_DIPLOCATION("DIL1:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL105" ) PORT_DIPLOCATION("DIL1:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL106" ) PORT_DIPLOCATION("DIL1:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL107" ) PORT_DIPLOCATION("DIL1:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "DIL108" ) PORT_DIPLOCATION("DIL1:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START_TAG("DIL2")
	PORT_DIPNAME( 0x01, 0x00, "DIL201" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL202" ) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL203" ) PORT_DIPLOCATION("DIL2:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL204" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL205" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL206" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL207" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "DIL208" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START_TAG("AUX1")//Presumed to be trackball, but only one phase available?
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("0")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("2")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("3")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("4")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("5")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("6")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("7")

	PORT_START_TAG("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")PORT_IMPULSE(5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")PORT_IMPULSE(5)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")PORT_IMPULSE(5)

INPUT_PORTS_END

INPUT_PORTS_START( dealem )
	PORT_START_TAG("ORANGE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("00")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("01")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("02")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("03")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("04")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("05")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("06")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("07")

	PORT_START_TAG("ORANGE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("08")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("09")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("10")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("11")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("12")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("13")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("14")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("15")

	PORT_START_TAG("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("16")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("17")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("18")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("19")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("20")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Test Switch")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Door Switch?") PORT_TOGGLE

	PORT_START_TAG("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("24")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("25")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("26")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("27")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("28")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("29")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("30")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("31")

	PORT_START_TAG("DIL1")
	PORT_DIPNAME( 0x0f, 0x00, "Cabinet Set Up Mode" ) PORT_DIPLOCATION("DIL1:01,02,03,04")
	PORT_DIPSETTING(    0x00, "Stop The Clock" )
	PORT_DIPSETTING(    0x01, "Hit the Top" )
	PORT_DIPSETTING(    0x02, "Way In" )
	PORT_DIPSETTING(    0x03, "Smash and Grab" )
	PORT_DIPSETTING(    0x04, "Ready Steady Go-1" )
	PORT_DIPSETTING(    0x05, "Ready Steady Go-2" )
	PORT_DIPSETTING(    0x06, "Top Gears-1" )
	PORT_DIPSETTING(    0x07, "Top Gears-2" )
	PORT_DIPSETTING(    0x08, "Nifty Fifty" )
	PORT_DIPSETTING(    0x09, "Super Tubes" )
	PORT_DIPNAME( 0x70, 0x00, "Target Payout Percentage" ) PORT_DIPLOCATION("DIL1:05,06,07")
	PORT_DIPSETTING(    0x00, "72%" )
	PORT_DIPSETTING(    0x10, "74%" )
	PORT_DIPSETTING(    0x20, "76%" )
	PORT_DIPSETTING(    0x30, "78%" )
	PORT_DIPSETTING(    0x40, "80%" )
	PORT_DIPSETTING(    0x50, "82%" )
	PORT_DIPSETTING(    0x60, "84%" )
	PORT_DIPSETTING(    0x70, "86%" )
	PORT_DIPNAME( 0x80, 0x00, "Display Switch Settings on Monitor" ) PORT_DIPLOCATION("DIL1:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START_TAG("DIL2")
	PORT_DIPNAME( 0x01, 0x00, "Payout Limit" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, "GBP 2.00 (All Cash)")
	PORT_DIPSETTING(    0x01, "GBP 4.00 (Token)")
	PORT_DIPNAME( 0x02, 0x00, "10p Payout Priority" ) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "Clear Credits and bank at power on?" ) PORT_DIPLOCATION("DIL2:03")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes  ) )
	PORT_DIPNAME( 0x08, 0x00, "50p Payout Solenoid fitted?" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes  ) )
	PORT_DIPNAME( 0x10, 0x00, "GBP 1.00 Payout Solenoid fitted?" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes  ) )
	PORT_DIPNAME( 0x20, 0x00, "Coin alarm active?" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes  ) )
	PORT_DIPNAME( 0x40, 0x00, "Stake" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, "10p 1 Game" )
	PORT_DIPSETTING(    0x40, "10p 2 Games" )
	PORT_DIPNAME( 0x80, 0x00, "DIL208" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START_TAG("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("0")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("2")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("3")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("4")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("5")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("6")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("7")

	PORT_START_TAG("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")PORT_IMPULSE(5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")PORT_IMPULSE(5)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")PORT_IMPULSE(5)

INPUT_PORTS_END

/*

IRQ  1  MC68A40 PTM   // On VDU board
IRQ  2  MC68A50 ACIA  // On MPU4 cart?
IRQ  3  SCN2674 AVDC  // On VDU board
IRQ 4-7 Program Card

*/

INTERRUPT_GEN(mpu4_vid_irq)
{
//  if (cpu_getiloops()&1)
//      cpunum_set_input_line(1, 1, ASSERT_LINE);
//  else
//  LOGSTUFF(("scn2674_irq_mask %02x",scn2674_irq_mask));

	if (cpu_getiloops()==0) // vbl
	{
	//  if (scn2674_display_enabled) // ?
		{
			if (scn2674_irq_mask&0x10)
			{
				LOGSTUFF(("vblank irq\n"));
				scn2674_irq_state = 1;
				update_mpu68_interrupts();

				scn2674_irq_register |= 0x10;
				scn2674_irq_state = 0;
			}
		}
		scn2674_status_register |= 0x10;

	}
}

static void mpu4_config_common(void)
{
	pia_config(0,&pia_ic3_intf);
	pia_config(1,&pia_ic4_intf);
	pia_config(2,&pia_ic5_intf);
	pia_config(3,&pia_ic6_intf);
	pia_config(4,&pia_ic7_intf);
	pia_config(5,&pia_ic8_intf);

	freq_timer = mame_timer_alloc(freq_timeout);
	ic24_timer = mame_timer_alloc(ic24_timeout);
	// setup ptm ////////////////////////////////////////////////////////////
	ptm6840_config(0, &ptm_ic2_intf );
}

// machine start (called only once) /////////////////////////////////////

MACHINE_START( mpu4_vid )
{
	mpu4_config_common();
	pia_reset();

	ptm6840_config(1, &ptm_vid_intf );

// setup comms///////////////////////////////////////////////////////////
	serial_card_connected=1;
	acia6850_config(0, &m6809_acia_if);
	acia6850_config(1, &m68k_acia_if);

// setup 8 mechanical meters ////////////////////////////////////////////
	Mechmtr_init(8);

// setup 4 default 96 half step reels ///////////////////////////////////

	Stepper_init(0, BARCREST_48STEP_REEL);
	Stepper_init(1, BARCREST_48STEP_REEL);
	Stepper_init(2, BARCREST_48STEP_REEL);
	Stepper_init(3, BARCREST_48STEP_REEL);

// setup the standard oki MSC1937 display ///////////////////////////////

	ROC10937_init(0, MSC1937,0);   // ?
}

static MACHINE_START( mpu4mod2 )
{
	mpu4_config_common();
	pia_reset();

	serial_card_connected=0;
	mod_number=2;

// setup 8 mechanical meters ////////////////////////////////////////////
	Mechmtr_init(8);

// setup 4 default 96 half step reels ///////////////////////////////////
	Stepper_init(0, BARCREST_48STEP_REEL);
	Stepper_init(1, BARCREST_48STEP_REEL);
	Stepper_init(2, BARCREST_48STEP_REEL);
	Stepper_init(3, BARCREST_48STEP_REEL);

// setup the standard oki MSC1937 display ///////////////////////////////
	ROC10937_init(0, MSC1937,0);
}

/*
Characteriser (CHR)

I haven't been able to work out all the ways of finding the CHR data, but there must be a flag in the ROMs
somewhere to pick it out. As built, the CHR is a PAL which holds an internal data table that is inaccessible to
anything other than the CPU. However, the programmers decided to best use this protection device in read/write/compare
cycles, storing almost the entire 'hidden' data table in the ROMs in plain sight. Only later rebuilds by BwB
avoided this 'feature' of the development kit, and as such, only low level access can defeat their protection.

This information has been used to generate the CHR tables loaded by the programs.

For most Barcrest games, the following method was used:

To calculate the values necessary to program the CHR, we must first find the version string, taking
the example of an AWP called Club Celebration, this starts at 0xff28 and terminates at 0xff2f.
0xff2f then represents the CHR address.
For some reason, the tables always seem to start and end with '00 00'.

From that point on, every word represents a call and response pair, until we have generated 8 8 byte rows of data.

The call values appear to be fixed, so it would be possible to make a switch statement, but I believe the emulated
behaviour to be more likely.

The initial 'PALTEST' routine as found in the Barcrest programs simply writes the first 'call' to the CHR space,
to read back the 'response'. There is no attempt to alter the order or anything else, just
a simple runthrough of the entire data table. The only 'catch' in this is to note that the CHR chip always scans
through the table starting at the last accessed data value - there are duplications within the table to catch out
the unwary.

However, a final 8 byte row, that controls the lamp matrix is not tested - to date, no-one outside of Barcrest knows
how this is generated, and currently trial and error is the only sensible method. It is noted that the default,
of all 00, is sometimes the correct answer, particularly in non-Barcrest use of the CHR chip, though when used normally,
there are again fixed call values.

Despite the potential to radically overhaul the design, the 68k version of the chip appears to just be a
16-bit version of the previous design, with some endian-swapping necessary. It is unclear, however, if it has any capacity
to affect lamp matrices in the same way as before, as no software seen makes any request for the 'lamp' row.
It has been left in the table, as it clearly exists, but is unused.
The 'quiz' games on the board did use an address-scrambling PAL for encryption, and the very last mod had a characteriser capable
of scrambling the ROM address lines.
*/

static WRITE8_HANDLER( characteriser_w )
{
	UINT8 x;
	int call=data;
		for ( x = 0; x < 64; x++ )
		{
			LOG_CHR(("Characteriser %02X:",MPU4_chr_data[x]));
		}

	if (offset == 0)
	{
		for ( x = prot_col; x < 64; x++ )
		{
			if	(MPU4_chr_lut[(x)] == call)
			{
				prot_col = x;
				LOG_CHR(("Characteriser find column %02X\n",prot_col));
				LOG_CHR(("Characteriser find data %02X\n",MPU4_chr_data[prot_col]));
				break;
			}
		}
			if (prot_col > 63)
			{
				prot_col = 0;
			}

	}
	else if (offset == 2)
	{
		LOG_CHR(("Characteriser write 2 data %02X\n",data));
		for ( x = lamp_col; x < 16; x++ )
		{
			if	(MPU4_chr_lut[(64+x)] == call)
			{
				lamp_col = x;
				LOG_CHR(("Characteriser find column %02X\n",lamp_col));
				break;
			}
			if (lamp_col > 7)
			{
				lamp_col = 0;
			}
		}
	}
}

static READ8_HANDLER( characteriser_r )
{
	LOG_CHR(("Characteriser read offset %02X \n",offset));
	if (offset == 0)
	{
		LOG_CHR(("Characteriser read data %02X \n",MPU4_chr_data[prot_col]));
		return MPU4_chr_data[prot_col];
	}
	if (offset == 3)
	{
		LOG_CHR(("Characteriser read data %02X \n",MPU4_chr_data[lamp_col+64]));
		return MPU4_chr_data[lamp_col+64];
	}
	return 0;
}

static WRITE16_HANDLER( characteriser16_w )
{
	int x;
	int call=data;
	LOG_CHR(("Characteriser write offset %02X data %02X\n",offset,data));
	for ( x = prot_col; x < 64; x++ )
	{
		if	(MPU4_chr_lut[(x)] == call)
		{
			prot_col = x;
			LOG_CHR(("Characteriser find column %02X\n",prot_col));
			break;
		}
			if (prot_col > 63)
			prot_col = 0;
	}
}

static READ16_HANDLER( characteriser16_r )
{
	LOG_CHR(("Characteriser read offset %02X \n",offset));
	LOG_CHR(("Characteriser read data %02X \n",MPU4_chr_data[prot_col]));
	return MPU4_chr_data[prot_col];
}

// generate a 50 Hz signal (based on an RC time) //////////////////////////

static TIMER_CALLBACK( freq_timeout )
{
	signal_50hz = 0;
	pia_set_input_ca1(1,0);	// signal is connected to IC4 CA1
}

static INTERRUPT_GEN( gen_50hz )
{
	signal_50hz = 1;

	pia_set_input_ca1(1,1);	// signal is connected to IC4 CA1

	mame_timer_adjust(freq_timer, MAME_TIME_IN_HZ(100), 0, time_zero);
}

static ADDRESS_MAP_START( mpu4_68k_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x7fffff) AM_ROM

//  AM_RANGE(0x600000, 0x63ffff) AM_RAM? In expanded games (mating)

	AM_RANGE(0x800000, 0x80ffff) AM_RAM AM_BASE(&mpu4_vid_mainram) // mainram / char address ram?

	/* what is here, the sound chip? Assume so */
	AM_RANGE(0x900000, 0x900001) AM_WRITE(saa1099_control_port_0_lsb_w)
	AM_RANGE(0x900002, 0x900003) AM_WRITE(saa1099_write_port_0_lsb_w)

	/* the palette chip */
	AM_RANGE(0xa00000, 0xa00001) AM_WRITE(ef9369_data_w)
	AM_RANGE(0xa00002, 0xa00003) AM_WRITE(ef9369_address_w)
	AM_RANGE(0xa00004, 0xa0000f) //AM_READWRITE(mpu4_vid_unmap_r, mpu4_vid_unmap_w)

	AM_RANGE(0xb00000, 0xb0000f) AM_READWRITE(mpu4_vid_scn2674_r, mpu4_vid_scn2674_w)

	AM_RANGE(0xc00000, 0xc1ffff) AM_READWRITE(mpu4_vid_vidram_r, mpu4_vid_vidram_w)

	/* comms with the MPU4 */
    AM_RANGE(0xff8000, 0xff8001) AM_READWRITE(acia6850_1_stat_lsb_r, acia6850_1_ctrl_lsb_w)
    AM_RANGE(0xff8002, 0xff8003) AM_READWRITE(acia6850_1_data_lsb_r, acia6850_1_data_lsb_w)

	AM_RANGE(0xff9000, 0xff900f) AM_READ(  ptm6840_1_lsb_r)
	AM_RANGE(0xff9000, 0xff900f) AM_WRITE( ptm6840_1_lsb_w)

	/* characterizer */
	AM_RANGE(0xffd000, 0xffd00f) AM_READWRITE(characteriser16_r, characteriser16_w) // Word-based version of old CHR???
ADDRESS_MAP_END

/* TODO: Fix up MPU4 map*/
static ADDRESS_MAP_START( mpu4_6809_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)

	AM_RANGE(0x0800, 0x0800) AM_READWRITE(acia6850_0_stat_r, acia6850_0_ctrl_w)
	AM_RANGE(0x0801, 0x0801) AM_READWRITE(acia6850_0_data_r, acia6850_0_data_w)

	//AM_RANGE(0x0880, 0x0880) AM_READ(uart1stat_r)     // Could be a UART datalogger is here.
	//AM_RANGE(0x0880, 0x0880) AM_WRITE(uart1ctrl_w)    // Or a PIA?
	//AM_RANGE(0x0881, 0x0881) AM_READ(uart1data_r)
	//AM_RANGE(0x0881, 0x0881) AM_WRITE(uart1data_w)
	AM_RANGE(0x0880, 0x0881) AM_NOP

	AM_RANGE(0x0900, 0x0907) AM_READ( ptm6840_0_r)  	// 6840PTM IC2
	AM_RANGE(0x0900, 0x0907) AM_WRITE(ptm6840_0_w)

	AM_RANGE(0x0A00, 0x0A03) AM_WRITE(pia_0_w)			// PIA6821 IC3
	AM_RANGE(0x0A00, 0x0A03) AM_READ( pia_0_r)

	AM_RANGE(0x0B00, 0x0B03) AM_WRITE(pia_1_w)			// PIA6821 IC4
	AM_RANGE(0x0B00, 0x0B03) AM_READ( pia_1_r)

	AM_RANGE(0x0C00, 0x0C03) AM_WRITE(pia_2_w)			// PIA6821 IC5
	AM_RANGE(0x0C00, 0x0C03) AM_READ( pia_2_r)

	AM_RANGE(0x0D00, 0x0D03) AM_WRITE(pia_3_w)			// PIA6821 IC6
	AM_RANGE(0x0D00, 0x0D03) AM_READ( pia_3_r)

	AM_RANGE(0x0E00, 0x0E03) AM_WRITE(pia_4_w)			// PIA6821 IC7
	AM_RANGE(0x0E00, 0x0E03) AM_READ( pia_4_r)

	AM_RANGE(0x0F00, 0x0F03) AM_WRITE(pia_5_w)			// PIA6821 IC8
	AM_RANGE(0x0F00, 0x0F03) AM_READ( pia_5_r)

	AM_RANGE(0x4000, 0x40ff) AM_RAM // it actually runs code from here...

	AM_RANGE(0xBE00, 0xBFFF) AM_RAM //00 written on startup
	AM_RANGE(0xC000, 0xFFFF) AM_ROM	AM_REGION(REGION_CPU1,0)  // 64k EPROM on board, only this region read
ADDRESS_MAP_END

static ADDRESS_MAP_START( mod2_memmap, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)

	AM_RANGE(0x0800, 0x0810) AM_WRITE(characteriser_w)
	AM_RANGE(0x0800, 0x0810) AM_READ( characteriser_r)

	AM_RANGE(0x0850, 0x0850) AM_WRITE(bankswitch_w)	// write bank (rom page select)

//  AM_RANGE(0x08E0, 0x08E7) AM_READ( 68681_duart_r)
//  AM_RANGE(0x08E0, 0x08E7) AM_WRITE( 68681_duart_w)

	AM_RANGE(0x0900, 0x0907) AM_READ( ptm6840_0_r)  // 6840PTM
	AM_RANGE(0x0900, 0x0907) AM_WRITE(ptm6840_0_w)

	AM_RANGE(0x0A00, 0x0A03) AM_WRITE(pia_0_w)		// PIA6821 IC3
	AM_RANGE(0x0A00, 0x0A03) AM_READ( pia_0_r)

	AM_RANGE(0x0B00, 0x0B03) AM_WRITE(pia_1_w)		// PIA6821 IC4
	AM_RANGE(0x0B00, 0x0B03) AM_READ( pia_1_r)

	AM_RANGE(0x0C00, 0x0C03) AM_WRITE(pia_2_w)		// PIA6821 IC5
	AM_RANGE(0x0C00, 0x0C03) AM_READ( pia_2_r)

	AM_RANGE(0x0D00, 0x0D03) AM_WRITE(pia_3_w)		// PIA6821 IC6
	AM_RANGE(0x0D00, 0x0D03) AM_READ( pia_3_r)

	AM_RANGE(0x0E00, 0x0E03) AM_WRITE(pia_4_w)		// PIA6821 IC7
	AM_RANGE(0x0E00, 0x0E03) AM_READ( pia_4_r)

	AM_RANGE(0x0F00, 0x0F03) AM_WRITE(pia_5_w)		// PIA6821 IC8
	AM_RANGE(0x0F00, 0x0F03) AM_READ( pia_5_r)

	AM_RANGE(0x1000, 0xffff) AM_READ(MRA8_BANK1)	// 64k  paged ROM (4 pages)
ADDRESS_MAP_END

//Deal 'Em was designed as an enhanced gamecard, to fit into an existing MPU4 cabinet
//It's an unoffical addon, and does all its work through the existing 6809 CPU

static const gfx_layout dealemcharlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_decode dealemgfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &dealemcharlayout, 0, 32 },
	{ -1 }
};

UINT8 *dealem_videoram,*dealem_charram;


tilemap *dealem_tilemap;


/***************************************************************************

  Convert the color PROMs into a more useable format.

  The palette PROM is connected to the RGB output this way:

  Red:      1K      Bit 0
            470R
            220R

  Green:    1K      Bit 3
            470R
            220R

  Blue:     470R
            220R    Bit 7

***************************************************************************/

PALETTE_INIT( dealem )
{
	int i;
	for (i = 0;i < memory_region_length(REGION_PROMS);i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = BIT(*color_prom,0);
		bit1 = BIT(*color_prom,1);
		bit2 = BIT(*color_prom,2);
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = BIT(*color_prom,3);
		bit1 = BIT(*color_prom,4);
		bit2 = BIT(*color_prom,5);
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = BIT(*color_prom,6);
		bit2 = BIT(*color_prom,7);
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(machine,i,MAKE_RGB(r,g,b));
		color_prom++;
	}

}

static TILE_GET_INFO( get_bg_tile_info )
{
	int tileno, colour;

	tileno = dealem_videoram[tile_index];
	colour = dealem_videoram[tile_index];

	SET_TILE_INFO(0,tileno,colour,0);
}

VIDEO_START(dealem)
{
	dealem_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_TYPE_PEN, 8, 8,32,32);
}

WRITE8_HANDLER( dealem_videoram_w )
{
	dealem_videoram[offset] = data;
	tilemap_mark_tile_dirty(dealem_tilemap, offset);
}

// this is wrong, the PAL handles the colour selection
WRITE8_HANDLER( dealem_colorram_w )
{
	colorram[offset] = data;
	tilemap_mark_tile_dirty(dealem_tilemap, offset);
}

VIDEO_UPDATE(dealem)
{
	tilemap_draw(bitmap, &machine->screen[0].visarea, dealem_tilemap, 0, 0);
	return 0;
}

static WRITE8_HANDLER( dealem_pal_w )
{
	switch (data)
	{
		default:
		{
			logerror("Deal 'em PAL write %d",data);
			break;
		}
	}
}

static ADDRESS_MAP_START( dealem_memmap, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)

	AM_RANGE(0x0800, 0x0800) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x0801, 0x0801) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)

//  AM_RANGE(0x0850, 0x0850) AM_WRITE(bankswitch_w) // write bank (rom page select)

	AM_RANGE(0x0880, 0x0880) AM_READ(MRA8_RAM) AM_WRITE(dealem_colorram_w) AM_BASE(&colorram) //AM_WRITE(dealem_pal_w)//

//  AM_RANGE(0x08E0, 0x08E7) AM_READ( 68681_duart_r)
//  AM_RANGE(0x08E0, 0x08E7) AM_WRITE( 68681_duart_w)

	AM_RANGE(0x0900, 0x0907) AM_READ( ptm6840_0_r)  // 6840PTM
	AM_RANGE(0x0900, 0x0907) AM_WRITE(ptm6840_0_w)

	AM_RANGE(0x0A00, 0x0A03) AM_WRITE(pia_0_w)		// PIA6821 IC3
	AM_RANGE(0x0A00, 0x0A03) AM_READ( pia_0_r)

	AM_RANGE(0x0B00, 0x0B03) AM_WRITE(pia_1_w)		// PIA6821 IC4
	AM_RANGE(0x0B00, 0x0B03) AM_READ( pia_1_r)

	AM_RANGE(0x0C00, 0x0C03) AM_WRITE(pia_2_w)		// PIA6821 IC5
	AM_RANGE(0x0C00, 0x0C03) AM_READ( pia_2_r)

	AM_RANGE(0x0D00, 0x0D03) AM_WRITE(pia_3_w)		// PIA6821 IC6
	AM_RANGE(0x0D00, 0x0D03) AM_READ( pia_3_r)

	AM_RANGE(0x0E00, 0x0E03) AM_WRITE(pia_4_w)		// PIA6821 IC7
	AM_RANGE(0x0E00, 0x0E03) AM_READ( pia_4_r)

	AM_RANGE(0x0F00, 0x0F03) AM_WRITE(pia_5_w)		// PIA6821 IC8
	AM_RANGE(0x0F00, 0x0F03) AM_READ( pia_5_r)

	AM_RANGE(0x1000, 0x3000) AM_WRITE(dealem_videoram_w) AM_BASE(&dealem_videoram)
	AM_RANGE(0xBE00, 0xFFFF) AM_ROM	AM_WRITENOP// 64k  paged ROM (4 pages)

ADDRESS_MAP_END

static MACHINE_DRIVER_START( mpu4_vid )

	MDRV_CPU_ADD_TAG("main", M6809, MPU4_MASTER_CLOCK/4 )
	MDRV_CPU_PROGRAM_MAP(mpu4_6809_map,0)
	MDRV_CPU_PERIODIC_INT(gen_50hz, 50 )

	MDRV_CPU_ADD_TAG("video", M68000, VIDEO_MASTER_CLOCK )
	MDRV_CPU_PROGRAM_MAP(mpu4_68k_map,0)
	MDRV_CPU_VBLANK_INT(mpu4_vid_irq,1)

	MDRV_NVRAM_HANDLER(generic_0fill)				// confirm

	MDRV_INTERLEAVE(16)
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 63*8-1, 0*8, 37*8-1)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_MACHINE_START(mpu4_vid)
	MDRV_MACHINE_RESET(mpu4_vid)
	MDRV_VIDEO_START (mpu4_vid)
	MDRV_VIDEO_UPDATE(mpu4_vid)

	MDRV_PALETTE_LENGTH(16)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("AY8913",AY8913, MPU4_MASTER_CLOCK/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")// Present on all video cards
	MDRV_SOUND_ADD(SAA1099, 8000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.00)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.00)

MACHINE_DRIVER_END

// machine driver for barcrest mpu4 board /////////////////////////////////

static MACHINE_DRIVER_START( mpu4mod2 )

	MDRV_MACHINE_START(mpu4mod2)
	MDRV_MACHINE_RESET(mpu4)
	MDRV_CPU_ADD_TAG("main", M6809, MPU4_MASTER_CLOCK/4)
	MDRV_CPU_PROGRAM_MAP(mod2_memmap,0)

	MDRV_CPU_PERIODIC_INT(gen_50hz, 50)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("AY8913",AY8913, MPU4_MASTER_CLOCK/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_NVRAM_HANDLER(generic_0fill)					// load/save nv RAM
	MDRV_DEFAULT_LAYOUT(layout_mpu4)
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_SIZE(288, 34)
	MDRV_SCREEN_VISIBLE_AREA(0, 288-1, 0, 34-1)
	MDRV_SCREEN_REFRESH_RATE(50)

	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(16)
	MDRV_PALETTE_INIT(mpu4)
MACHINE_DRIVER_END

// machine driver for zenitone dealem board /////////////////////////////////

static MACHINE_DRIVER_START( dealem )

	MDRV_MACHINE_START(mpu4mod2)							// main mpu4 board initialisation
	MDRV_MACHINE_RESET(mpu4_vid)
	MDRV_CPU_ADD_TAG("main", M6809, MPU4_MASTER_CLOCK/4)	// 6809 CPU
	MDRV_CPU_PROGRAM_MAP(dealem_memmap,0)					// setup read and write memorymap

	MDRV_CPU_PERIODIC_INT(gen_50hz, 50)	// generate 50 hz signal

	MDRV_SCREEN_REFRESH_RATE(56)//Measured accurately from the flip-flop
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("AY8913",AY8913, MPU4_MASTER_CLOCK/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_NVRAM_HANDLER(generic_0fill)					// load/save nv RAM
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((54+1)*8, (32+1)*8)                  /* Taken from MC6845 init, registers 00 & 04. Normally programmed with (value-1) */
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 31*8-1)    /* Taken from MC6845 init, registers 01 & 06 */
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_VIDEO_START( dealem)
	MDRV_GFXDECODE(dealemgfxdecodeinfo)
	MDRV_VIDEO_UPDATE(dealem)

	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(32)
	MDRV_PALETTE_INIT(dealem)

MACHINE_DRIVER_END

	const UINT8 MPU4_chr_lut[72]= {	0x00,0x1A,0x04,0x10,0x18,0x0F,0x13,0x1B,
									0x03,0x07,0x17,0x1D,0x36,0x35,0x2B,0x28,
									0x39,0x21,0x22,0x25,0x2C,0x29,0x31,0x34,
									0x0A,0x1F,0x06,0x0E,0x1C,0x12,0x1E,0x0D,
									0x14,0x0A,0x19,0x15,0x06,0x0F,0x08,0x1B,
									0x1E,0x04,0x01,0x0C,0x18,0x1A,0x11,0x0B,
									0x03,0x17,0x10,0x1D,0x0E,0x07,0x12,0x09,
									0x0D,0x1F,0x16,0x05,0x13,0x1C,0x02,0x00,
									0x00,0x01,0x04,0x09,0x10,0x19,0x24,0x31};

DRIVER_INIT (crmaze)
{
	int x;
	static const UINT8 chr_table[72]={0x00,0x84,0x94,0x3C,0xEC,0x5C,0xEC,0x50,
							 		  0x2C,0x68,0x60,0xAC,0x74,0x00,0xAC,0x58,
									  0xEC,0x7C,0xEC,0x58,0xE0,0x90,0x18,0xEC,
									  0x54,0x28,0x68,0x44,0x84,0xB4,0x10,0x20,
									  0x84,0xBC,0xE8,0x70,0x24,0x84,0xB8,0xE0,
									  0x94,0x14,0x2C,0x64,0x8C,0x50,0x28,0x4C,
									  0x6C,0x60,0xA0,0xBC,0xCC,0x78,0xE8,0x50,
									  0x20,0xAC,0x74,0x04,0xA4,0x94,0x3C,0x00,
									  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

	for (x=0; x < 72; x++)
	{
		MPU4_chr_data[(x)] = chr_table[(x)];
	}
}

DRIVER_INIT (mating)
{
	int x;
	static const UINT8 chr_table[72]={0x00,0x18,0xC8,0xA4,0x0C,0x80,0x0C,0x90,
									  0x34,0x30,0x00,0x58,0xC8,0x84,0x4C,0xA0,
									  0x4C,0xC0,0x3C,0xC8,0xA4,0x4C,0x80,0x0C,
									  0x80,0x0C,0xE0,0x1C,0x88,0xA4,0x0C,0xA0,
									  0x0C,0x80,0x4C,0xA0,0x3C,0x98,0xEC,0x84,
									  0x0C,0xC0,0x1C,0xA8,0x84,0x0C,0xA0,0x5C,
									  0xE8,0xA4,0x0C,0xD0,0x04,0x38,0xA8,0xC4,
									  0x2C,0x90,0x44,0x18,0xE8,0x84,0x3C,0x00,
									  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

	for (x=0; x < 72; x++)
	{
		MPU4_chr_data[(x)] = chr_table[(x)];
	}
}

DRIVER_INIT (connect4)
{
	led_extend=1;
}

/*
   Barcrest released two different games called "The Crystal Maze".
   One is a non-video AWP, and uses only the MPU4 card, and the other SWP is the one we're interested in running
   Some of the dumps available seem to confuse the two, due to an early database not distinguishing
   between MPU4 and MPU4Video, as the latter had not been emulated at all at that stage. */

#define VID_BIOS \
	ROM_LOAD("vid.p1",  0x00000, 0x10000,  CRC(e996bc18) SHA1(49798165640627eb31024319353da04380787b10))

ROM_START( bctvidbs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	VID_BIOS
ROM_END

ROM_START( crmaze ) // this set runs in MFME, so should be OK */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "cm3.p1",  0x000000, 0x80000,  CRC(2d2edee5) SHA1(0281ec97aaaaf4c7969340bd5995ac1541dbad54) )
	ROM_LOAD16_BYTE( "cm3.p2",  0x000001, 0x80000,  CRC(c223d7b9) SHA1(da9d730716a30d0e93f2a02c1efa7f19457ae010) )
	ROM_LOAD16_BYTE( "cm3.p3",  0x100000, 0x80000,  CRC(2959c77b) SHA1(8de533bfad48ad19a635dddcafa2a0825133b4de) )
	ROM_LOAD16_BYTE( "cm3.p4",  0x100001, 0x80000,  CRC(b7873e9a) SHA1(a71fac883e02d5f49aee0a20f92dbdb00640ce8d) )
	ROM_LOAD16_BYTE( "cm3.p5",  0x200000, 0x80000,  CRC(c8375070) SHA1(da2ba6591d8765f896c40d6526da8e945d02a182) )
	ROM_LOAD16_BYTE( "cm3.p6",  0x200001, 0x80000,  CRC(1ea36938) SHA1(43f62935b21232d23f662e1e124663267edb1283) )
	ROM_LOAD16_BYTE( "cm3.p7",  0x300000, 0x80000,  CRC(9de3802e) SHA1(ec792f115a0708d68046ba0beb314b7e1f1eb422) )
	ROM_LOAD16_BYTE( "cm3.p8",  0x300001, 0x80000,  CRC(1e6e60b0) SHA1(5e71714747073dd89852a84585642388ee440325) )
	ROM_LOAD16_BYTE( "cm3.p9",  0x400000, 0x80000,  CRC(bfba55a7) SHA1(22eb9b1f9fe83d3b424fd521b68e2976a1940df9) )
	ROM_LOAD16_BYTE( "cm3.pa",  0x400001, 0x80000,  CRC(07edda81) SHA1(e94525be03f30e407051992925bb0d693f3d809b) )
ROM_END

ROM_START( crmazea )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "am1z.p1",  0x000000, 0x80000,  CRC(d2fdda6d) SHA1(96c27dedc3cf1dd478e6169200844d418a276f14) )
	ROM_LOAD16_BYTE( "am1z.p2",  0x000001, 0x80000,  CRC(1637170f) SHA1(fd17a0e7794f01bf4ad7a16b185f87cb060c70ab) )
	ROM_LOAD16_BYTE( "am1g.p1",  0x100000, 0x80000,  CRC(e8cf8203) SHA1(e9f42e5c18b97807f51284ad2416346578ed73c4) )
	ROM_LOAD16_BYTE( "am1g.p2",  0x100001, 0x80000,  CRC(7b036151) SHA1(7b0040c296059b1e1798ddedf0ecb4582d67ee70) )
	ROM_LOAD16_BYTE( "am1g.p3",  0x200000, 0x80000,  CRC(48f17b20) SHA1(711c46fcfd86ded8ff7da883188d70560d20e42f) )
	ROM_LOAD16_BYTE( "am1g.p4",  0x200001, 0x80000,  CRC(2b3d9a97) SHA1(7468fffd90d840d245a70475b42308f1e48c5017) )
	ROM_LOAD16_BYTE( "am1g.p5",  0x300000, 0x80000,  CRC(68286bb1) SHA1(c307e3ad1e0fd92314216c8e554aafa949559452) )
	ROM_LOAD16_BYTE( "am1g.p6",  0x300001, 0x80000,  CRC(a6b498ad) SHA1(117e1a4ec7e2d3c7d530c5a56cbc1d24b0ddc747) )
	ROM_LOAD16_BYTE( "am1g.p7",  0x400000, 0x80000,  CRC(15882699) SHA1(b29a331e51a37554323b91330a7c2b62b33a943a) )
	ROM_LOAD16_BYTE( "am1g.p8",  0x400001, 0x80000,  CRC(6f0f855b) SHA1(ab411d1af0f88049a6c435bafd4b1fa63f5519b1) )
ROM_END

ROM_START( crmazeb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "am2z.p1",  0x000000, 0x80000,  CRC(f27e02f0) SHA1(8637904c201ece4f08ad63b4fd6d06a860fa762f) )
	ROM_LOAD16_BYTE( "am2z.p2",  0x000001, 0x80000,  CRC(4d24f482) SHA1(9e3687db9d0233e56999017f3ed59ec543bce303) )
	ROM_LOAD16_BYTE( "am2g.p1",  0x100000, 0x80000,  CRC(115402db) SHA1(250f2eded1b88a1abf82febb009eadbb90936f8a) )
	ROM_LOAD16_BYTE( "am2g.p2",  0x100001, 0x80000,  CRC(5d804fbb) SHA1(8dc02eb9329f9c29d4bcc9a0315ae96085625d3e) )
	ROM_LOAD16_BYTE( "am2g.p3",  0x200000, 0x80000,  CRC(5ead0c06) SHA1(35d9aefc60e2c391e32f8119a6dc44434d91c09e) )
	ROM_LOAD16_BYTE( "am2g.p4",  0x200001, 0x80000,  CRC(de4fb542) SHA1(4bf8f8f6850fd819d91827d3c474bd488e61e5ac) )
	ROM_LOAD16_BYTE( "am2g.p5",  0x300000, 0x80000,  CRC(80b01ce2) SHA1(4a3a4bcff4bd9affd1a5eeca5781b6af05bbcc16) )
	ROM_LOAD16_BYTE( "am2g.p6",  0x300001, 0x80000,  CRC(3e134ecc) SHA1(1f8cdce62e693eb07c4620b64cc467339c0563de) )
	ROM_LOAD16_BYTE( "am2g.p7",  0x400000, 0x80000,  CRC(6eb36f1d) SHA1(08b9ec184d64bdbdfa61d3e991a3647e74a7756f) )
	ROM_LOAD16_BYTE( "am2g.p8",  0x400001, 0x80000,  CRC(dda353ef) SHA1(56a5b43f0b0bd9dbf348946a5758ebe63eadb8cf) )
ROM_END

ROM_START( turnover )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "tos.p1",  0x000000, 0x010000,  CRC(4044dbeb) SHA1(3aa553055c56f14564b1e33e1c1975337e639f70) )
	ROM_LOAD16_BYTE( "to.p2",   0x000001, 0x010000,  CRC(4bc4659a) SHA1(0e282134c4fe4e8c1cc7b16957903179e23c7abc) )
	ROM_LOAD16_BYTE( "to.p3",   0x020000, 0x010000,  CRC(273c7c14) SHA1(71feb555a05a0ff1ec674505cab72d93c9fbdf65) )
	ROM_LOAD16_BYTE( "to.p4",   0x020001, 0x010000,  CRC(83d29546) SHA1(cef90455b9d8a92424fe1aa10f20fd075d0e3091) )
	ROM_LOAD16_BYTE( "to.p5",   0x040000, 0x010000,  CRC(dceac511) SHA1(7a6d65464e23d832943f771c4cf580aabc6f0e44) )
	ROM_LOAD16_BYTE( "to.p6",   0x040001, 0x010000,  CRC(54c6afb7) SHA1(b724b87b6f4e47d220310b38c97be2fa73dcd617) )
	ROM_LOAD16_BYTE( "to.p7",   0x060000, 0x010000,  CRC(acf19542) SHA1(ad46ffb3c2c078a8e3712eff27aa61f0d1a7c059) )
	ROM_LOAD16_BYTE( "to.p8",   0x060001, 0x010000,  CRC(a5ca385d) SHA1(8df26a33ea7f5b577761c6f9d2fa4eaed74661f8) )
	ROM_LOAD16_BYTE( "to.p9",   0x080000, 0x010000,  CRC(6e85fde3) SHA1(14868d58829e13987e66f52e1899c4385987a87b) )
	ROM_LOAD16_BYTE( "to.p10",  0x080001, 0x010000,  CRC(fadd11a2) SHA1(2b2fbb0769ef6035688d495464f3ea3bc8c7c660) )
	ROM_LOAD16_BYTE( "to.p11",  0x0a0000, 0x010000,  CRC(2d72a61a) SHA1(ce455ab6fea452f96a3ad365178e0e5a0b437867) )
	ROM_LOAD16_BYTE( "to.p12",  0x0a0001, 0x010000,  CRC(a14eedb6) SHA1(219b887a334ff28a88ed2e50f0caff4b510cd549) )
	ROM_LOAD16_BYTE( "to.p13",  0x0c0000, 0x010000,  CRC(3f66ef6b) SHA1(60be6d3f8da1f3084db15ac1bb2470e55c0271de) )
	ROM_LOAD16_BYTE( "to.p14",  0x0c0001, 0x010000,  CRC(127ba65d) SHA1(e34dcd19efd31dc712daac940277bb17694ea61a) )
	ROM_LOAD16_BYTE( "to.p15",  0x0e0000, 0x010000,  CRC(ad787e31) SHA1(314ba312adfc71e4b3b2d52355ec692c192b74eb) )
	ROM_LOAD16_BYTE( "to.p16",  0x0e0001, 0x010000,  CRC(e635c942) SHA1(08f8b5fdb738647bc0b49938da05533be42a2d60) )
ROM_END

ROM_START( skiltrek )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "st.p1",  0x000000, 0x010000,  CRC(d9de47a5) SHA1(625bf40780203293fc34cd8cea8278b4b4a52a75) )
	ROM_LOAD16_BYTE( "st.p2",  0x000001, 0x010000,  CRC(b62575c2) SHA1(06d75e8a364750663d329650720021279e195236) )
	ROM_LOAD16_BYTE( "st.p3",  0x020000, 0x010000,  CRC(9506da76) SHA1(6ef28ab8ec1af455be8ecfab20243f0823dca7c1) )
	ROM_LOAD16_BYTE( "st.p4",  0x020001, 0x010000,  CRC(6ab447bc) SHA1(d01c209dbf4d19a6a7f878fa54ff1cb51e7dcba5) )
	ROM_LOAD16_BYTE( "st.q1",  0x040000, 0x010000,  CRC(4faca475) SHA1(69b498c543600b8e37ab0ed1863ba57845648f3c) )
	ROM_LOAD16_BYTE( "st.q2",  0x040001, 0x010000,  CRC(9f2c5938) SHA1(85527c4c0b7a1e66576d56607d89750fab082580) )
	ROM_LOAD16_BYTE( "st.q3",  0x060000, 0x010000,  CRC(6b6cb194) SHA1(aeac5dcc0827c17e758e3e821ae8a78a3a16ddce) )
	ROM_LOAD16_BYTE( "st.q4",  0x060001, 0x010000,  CRC(ec57bc17) SHA1(d9f522739dbb190fb941ca654299bbedbb8fb703) )
	ROM_LOAD16_BYTE( "st.q5",  0x080000, 0x010000,  CRC(7740a88b) SHA1(d9a683d3e0d6c1b4b59520f90f825124b7a61168) )
	ROM_LOAD16_BYTE( "st.q6",  0x080001, 0x010000,  CRC(95e97796) SHA1(f1a8de0ad02aca31f79a4fe8ba5044546163e3c4) )
	ROM_LOAD16_BYTE( "st.q7",  0x0a0000, 0x010000,  CRC(f3b8fe7f) SHA1(52d5be3f8cab419103f4727d0fb9d30f34c8f651) )
	ROM_LOAD16_BYTE( "st.q8",  0x0a0001, 0x010000,  CRC(b85e75a2) SHA1(b7b03b090c0ec6d92e9a25abb7fec0507356bdfc) )
	ROM_LOAD16_BYTE( "st.q9",  0x0c0000, 0x010000,  CRC(835f6001) SHA1(2cd9084c102d74bcb578c8ea22bbc9ea58f0ceab) )
	ROM_LOAD16_BYTE( "st.qa",  0x0c0001, 0x010000,  CRC(3fc62a0e) SHA1(0628de4b962d3fcca3757cd4e89b3005c9bfd218) )
ROM_END

ROM_START( mating )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "matd.p1",  0x000000, 0x080000,  CRC(e660909f) SHA1(0acd990264fd7faf1f91a796d2438e8c2c7b83d1) )
	ROM_LOAD16_BYTE( "matd.p2",  0x000001, 0x080000,  CRC(a4c7e9b4) SHA1(30148c0257181bb88159e02d2b7cd79995ee84a7) )
	ROM_LOAD16_BYTE( "matg.p3",  0x100000, 0x080000,  CRC(571f4e8e) SHA1(51babacb5d9fb1cc9e1e56a3b2a355597d04f178) )
	ROM_LOAD16_BYTE( "matg.p4",  0x100001, 0x080000,  CRC(52d8657b) SHA1(e44e1db13c4abd4fedcd72df9dce1df594f74e44) )
	ROM_LOAD16_BYTE( "matg.p5",  0x200000, 0x080000,  CRC(9f0c9552) SHA1(8b1197f20853e18841a8f64fd5ff58cdd0bd1dbd) )
	ROM_LOAD16_BYTE( "matg.p6",  0x200001, 0x080000,  CRC(59f2b6a8) SHA1(4921cf1fc4c3bc50d2598b63726f61f68b41658c) )
	ROM_LOAD16_BYTE( "matg.p7",  0x300000, 0x080000,  CRC(64c0031a) SHA1(a519addd5d8f4696967ec84c163d28cb81ff9f32) )
	ROM_LOAD16_BYTE( "matg.p8",  0x300001, 0x080000,  CRC(22370dae) SHA1(72b1686b458750b5ee9dfe5599c308329d2c79d5) )
	ROM_LOAD16_BYTE( "matq.p9",  0x400000, 0x040000,  CRC(2d42e982) SHA1(80e476d5d65662059daa93a2fd383aecb74903c1) )
	ROM_LOAD16_BYTE( "matq.p10", 0x400001, 0x040000,  CRC(90364c3c) SHA1(6a4d2a3dd2cf9040887503888e6f38341578ad64) )

	/* Mating Game has an extra OKI sound chip */
	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "matsnd.p1",  0x000000, 0x080000,  CRC(f16df9e3) SHA1(fd9b82d73e18e635a9ea4aabd8c0b4aa2c8c6fdb) )
	ROM_LOAD( "matsnd.p2",  0x080000, 0x080000,  CRC(0c041621) SHA1(9156bf17ef6652968d9fbdc0b2bde64d3a67459c) )
	ROM_LOAD( "matsnd.p3",  0x100000, 0x080000,  CRC(c7435af9) SHA1(bd6080afaaaecca0d65e6d4125b46849aa4d1f33) )
	ROM_LOAD( "matsnd.p4",  0x180000, 0x080000,  CRC(d7e65c5b) SHA1(5575fb9f948158f2e94c986bf4bca9c9ee66a489) )
ROM_END

ROM_START( matinga )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "mats.p1",  0x000000, 0x080000,  CRC(ebc7ea7e) SHA1(11015489a803ba5c8dbdafd632424bbd6080aece) ) // 3 bytes changed from above set...
	ROM_LOAD16_BYTE( "mats.p2",  0x000001, 0x080000,  CRC(a4c7e9b4) SHA1(30148c0257181bb88159e02d2b7cd79995ee84a7) ) // Just enough to disable the data logger
	ROM_LOAD16_BYTE( "matg.p3",  0x100000, 0x080000,  CRC(571f4e8e) SHA1(51babacb5d9fb1cc9e1e56a3b2a355597d04f178) )
	ROM_LOAD16_BYTE( "matg.p4",  0x100001, 0x080000,  CRC(52d8657b) SHA1(e44e1db13c4abd4fedcd72df9dce1df594f74e44) )
	ROM_LOAD16_BYTE( "matg.p5",  0x200000, 0x080000,  CRC(9f0c9552) SHA1(8b1197f20853e18841a8f64fd5ff58cdd0bd1dbd) )
	ROM_LOAD16_BYTE( "matg.p6",  0x200001, 0x080000,  CRC(59f2b6a8) SHA1(4921cf1fc4c3bc50d2598b63726f61f68b41658c) )
	ROM_LOAD16_BYTE( "matg.p7",  0x300000, 0x080000,  CRC(64c0031a) SHA1(a519addd5d8f4696967ec84c163d28cb81ff9f32) )
	ROM_LOAD16_BYTE( "matg.p8",  0x300001, 0x080000,  CRC(22370dae) SHA1(72b1686b458750b5ee9dfe5599c308329d2c79d5) )
	ROM_LOAD16_BYTE( "matq.p9",  0x400000, 0x040000,  CRC(2d42e982) SHA1(80e476d5d65662059daa93a2fd383aecb74903c1) )
	ROM_LOAD16_BYTE( "matq.p10", 0x400001, 0x040000,  CRC(90364c3c) SHA1(6a4d2a3dd2cf9040887503888e6f38341578ad64) )

	/* Mating Game has an extra OKI sound chip */
	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "matsnd.p1",  0x000000, 0x080000,  CRC(f16df9e3) SHA1(fd9b82d73e18e635a9ea4aabd8c0b4aa2c8c6fdb) )
	ROM_LOAD( "matsnd.p2",  0x080000, 0x080000,  CRC(0c041621) SHA1(9156bf17ef6652968d9fbdc0b2bde64d3a67459c) )
	ROM_LOAD( "matsnd.p3",  0x100000, 0x080000,  CRC(c7435af9) SHA1(bd6080afaaaecca0d65e6d4125b46849aa4d1f33) )
	ROM_LOAD( "matsnd.p4",  0x180000, 0x080000,  CRC(d7e65c5b) SHA1(5575fb9f948158f2e94c986bf4bca9c9ee66a489) )
ROM_END

ROM_START( connect4 )
	ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_ERASE00  )
	ROM_LOAD( "connect4.p2",  0x8000, 0x4000,  CRC(6090633c) )
	ROM_LOAD( "connect4.p1",  0xC000, 0x4000,  CRC(b1af50c0) )
ROM_END

ROM_START( dealem )
	ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_ERASE00  )
	ROM_LOAD( "zenndlem.u6",	0x8000, 0x8000,  CRC(571e5c05) SHA1(89b4c331407a04eae34bb187b036791e0a671533) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "zenndlem.u24",	0x0000, 0x10000, CRC(3a1950c4) SHA1(7138346d4e8b3cffbd9751b4d7ebd367b9ad8da9) )    /* text layer */

	ROM_REGION( 0x020, REGION_PROMS, 0 )
	ROM_LOAD( "zenndlem.u22",		0x000, 0x020, CRC(29988304) SHA1(42f61b8f9e1ee96b65db3b70833eb2f6e7a6ae0a) )

	ROM_REGION( 0x200, REGION_PLDS, 0 )
	ROM_LOAD( "zenndlem.u10",		0x000, 0x104, CRC(e3103c05) SHA1(91b7be75c5fb37025039ab54b484e46a033969b5) )
ROM_END

GAMEL(1989?,connect4,0,       mpu4mod2, connect4, connect4, 0,   "Dolbeck Systems", "Connect 4",														GAME_IMPERFECT_GRAPHICS,layout_connect4 )
GAME( 199?, bctvidbs,0,       mpu4mod2, mpu4,     0,	 ROT0,   "Barcrest", 		"MPU4 Video Firmware",												GAME_IS_BIOS_ROOT )

//Deal 'Em was a conversion kit designed to make early MPU4 machines into video games by replacing the top glass
//and reel assembly with this kit and a supplied monitor.
//The real Deal 'Em ran on Summit Coin hardware, and was made by someone else.
//A further different release was made in 2000, running on the Barcrest MPU4 Video, rather than this one.
GAME( 1987, dealem,	 0,		  dealem,	dealem,   0,	 ROT0,   "Zenitone", 		"Deal 'Em (MPU4 Conversion Kit)",									GAME_NOT_WORKING|GAME_IMPERFECT_GRAPHICS|GAME_WRONG_COLORS )

GAME( 1994?,crmaze,  bctvidbs,mpu4_vid, crmaze,   crmaze,ROT0,   "Barcrest", 		"The Crystal Maze: Team Challenge (SWP)",							GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1992?,crmazea, crmaze,  mpu4_vid, crmaze,   crmaze,ROT0,   "Barcrest", 		"The Crystal Maze (AMLD version SWP)",								GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1993?,crmazeb, crmaze,  mpu4_vid, crmaze,   0,     ROT0,   "Barcrest", 		"The Crystal Maze - Now Featuring Ocean Zone (AMLD Version SWP)",	GAME_NOT_WORKING|GAME_NO_SOUND ) // unprotected?
GAME( 1990, turnover,bctvidbs,mpu4_vid, mpu4,     0,     ROT0,   "Barcrest", 		"Turnover",															GAME_NOT_WORKING|GAME_NO_SOUND ) // unprotected?
GAME( 1992, skiltrek,bctvidbs,mpu4_vid, mpu4,     0,     ROT0,   "Barcrest", 		"Skill Trek",														GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 199?, mating,  bctvidbs,mpu4_vid, mpu4,     mating,ROT0,   "Barcrest", 		"The Mating Game (Datapak)",										GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 199?, matinga, mating,  mpu4_vid, mpu4,     mating,ROT0,   "Barcrest", 		"The Mating Game (Standard)",										GAME_NOT_WORKING|GAME_NO_SOUND )
