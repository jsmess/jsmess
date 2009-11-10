/***************************************************************************

        Interact Family Computer

        12/05/2009 Skeleton driver - Micko
	31/05/2009 Added notes and video - Robbbert

	This was made by Interact Company of Ann Arbor, Michigan. However,
	just after launch, the company collapsed. The liquidator, Protecto,
	sold some and MicroVideo sold the rest. MicroVideo continued to
	develop but went under 2 years later. Meanwhile, the French company
	Lambda Systems sold a clone called the Victor Lambda. But, like the
	Americans, Lambda Systems also collapsed. Another French company,
	Micronique, purchased all remaining stock and intellectual rights
	from Lambda Systems, Microvideo and Interact, and the computer becomes
	wholly French. The computer has a name change, becoming the Hector.
	This in turn gets upgraded (2HR, HRX, MX). The line is finally
	retired in about 1985.

        Hector 2HR+
        Victor
        Hector 2HR
        Hector HRX
        Hector MX40c
        Hector MX80c
        Hector 1
        Interact
             
        29/10/2009 Update skeleton to functional machine
                          by yo_fr       (jj.stac@aliceadsl.fr)
               
               => add Keyboard,
               => add color, 
               => add cassette,
               => add sn76477 sound and 1bit sound,
               => add joysticks (stick, pot, fire)
               => add BR/HR switching 
               => add bank switch for HRX
               => add device MX80c and bank switching for the ROM
    Importante note : the keyboard function add been piked from 
                      DChector project : http://dchector.free.fr/ made by DanielCoulom
                      (thank's Daniel)
    TODO : Add the cartridge function,
           Adjust the one shot and A/D timing (sn76477)

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"   //i8085/i8085.h"
#include "machine/pckeybrd.h"
#include "devices/cassette.h"
#include "sound/wave.h"      // for K7 sound
#include "sound/sn76477.h"   // for sn sound
#include "sound/discrete.h"  // for 1 Bit sound

#include "includes/hec2hrp.h"

static ADDRESS_MAP_START(interact_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
    // Hardward address mapping
//	AM_RANGE(0x0800,0x0808) AM_WRITE( hector_switch_bank_w)// Bank management not udsed in BR machine
	AM_RANGE(0x1000,0x1000) AM_WRITE( hector_color_a_w)  // Color c0/c1
	AM_RANGE(0x1800,0x1800) AM_WRITE( hector_color_b_w)  // Color c2/c3
	AM_RANGE(0x2000,0x2003) AM_WRITE( hector_sn_2000_w)  // Sound
	AM_RANGE(0x2800,0x2803) AM_WRITE( hector_sn_2800_w)  // Sound
	AM_RANGE(0x3000,0x3000) AM_READWRITE( hector_cassette_r, hector_sn_3000_w)// Write necessary
	AM_RANGE(0x3800,0x3807) AM_READ( hector_keyboard_r)  // Keyboard

    // Main ROM page 
	AM_RANGE(0x0000,0x3fff) AM_ROM  //BANK(2) 
 //   AM_RANGE(0x1000,0x3fff) AM_RAM

	// Vidéo br mapping
	AM_RANGE(0x4000,0x49ff) AM_RAM AM_BASE(&videoram)
	// continous RAM
    AM_RANGE(0x4A00,0xffff) AM_RAM

ADDRESS_MAP_END

/* Cassette definition */
const struct CassetteOptions interact_cassette_options = {
	1,		/* channels */
	16,		/* bits per sample */
	44100	/* sample frequency */
};

static const cassette_config interact_cassette_config =
{
	cassette_default_formats,
	&interact_cassette_options,
	CASSETTE_STOPPED | CASSETTE_MASK_SPEAKER
};

/* Discrete Sound */
static DISCRETE_SOUND_START( hec2hrp )
	DISCRETE_INPUT_LOGIC(NODE_01)
	DISCRETE_OUTPUT(NODE_01, 5000)
DISCRETE_SOUND_END

static MACHINE_RESET(interact)
{
    flag_hr=0;
    flag_clk =0;
	write_cassette=0;
}

static MACHINE_START(interact)
{
UINT8 i;
       
	// Start keyboard
    at_keyboard_init(machine, AT_KEYBOARD_TYPE_PC);
    at_keyboard_reset();
    at_keyboard_set_scan_code_set(2);
    at_keyboard_reset();

	// For keyboard
    keyboard_timer = timer_alloc(machine, Callback_keyboard, 0);
    timer_adjust_periodic(keyboard_timer, ATTOTIME_IN_MSEC(100), 0, ATTOTIME_IN_MSEC(20));//keyboard scan 25ms

    for(i = 0; i < 8; i++) touches[i] = 0xff;     //all key off

	// For Cassette synchro
    Cassette_timer = timer_alloc(machine, Callback_CK, 0);
    timer_adjust_periodic(Cassette_timer, ATTOTIME_IN_MSEC(100), 0, ATTOTIME_IN_USEC(64));// 11 * 6; 16 *4; 32 *2; 64 => real synchro scan speed for 15,624Khz
 
	// Sound sn76477
	Init_Value_SN76477_Hector();  //init R/C value
}

static VIDEO_START( interact )
{
    Init_Hector_Palette(machine);
    flag_hr=0;
}

static VIDEO_UPDATE( interact )
{
   	video_screen_set_visarea(screen, 0, 113, 0, 75);
    hector_hr( bitmap, videoram,  77, 32);
	return 0;
}

static MACHINE_DRIVER_START( interact )

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_1_75MHz)
	MDRV_CPU_PROGRAM_MAP(interact_mem)
    MDRV_CPU_PERIODIC_INT(irq0_line_hold,50) //  put on the 8080 irq in Hz

	MDRV_MACHINE_RESET(interact)
    MDRV_MACHINE_START(interact)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 79)
	MDRV_SCREEN_VISIBLE_AREA(0, 112, 0, 77)
	MDRV_PALETTE_LENGTH(256)				// 8 colours, but only 4 at a time
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(interact)
	MDRV_VIDEO_UPDATE(interact)
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
    MDRV_CASSETTE_ADD( "cassette", interact_cassette_config )

MACHINE_DRIVER_END

/* Input ports */
static INPUT_PORTS_START( interact )
	PORT_INCLUDE( at_keyboard )
INPUT_PORTS_END


static SYSTEM_CONFIG_START(interact)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( interact )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "interact.rom", 0x0000, 0x0800, CRC(1aa50444) SHA1(405806c97378abcf7c7b0d549430c78c7fc60ba2))
ROM_END

ROM_START( hector1 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hector1.rom",  0x0000, 0x1000, CRC(3be6628b) SHA1(1c106d6732bed743d8283d39e5b8248271f18c42))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG		 COMPANY   FULLNAME       FLAGS */
COMP(1979, interact,        0, 0,   interact, interact,     0,	interact,  	 "Interact",   "Interact Family Computer", GAME_IMPERFECT_SOUND)
COMP(????, hector1,  interact, 0, 	interact, interact,     0,  interact,  	 "Micronique", "Hector 1",	GAME_IMPERFECT_SOUND)
