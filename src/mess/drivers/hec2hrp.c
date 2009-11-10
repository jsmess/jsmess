/***************************************************************************

        Hector 2HR+
        Victor
        Hector 2HR
        Hector HRX
        Hector MX40c
        Hector MX80c
        
        12/05/2009 Skeleton driver - Micko
	    31/06/2009 Video - Robbbert

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
//
// Joystick 0 :
// clavier numerique :
//                (UP)5
//  (left)1                     (right)3
//               (down)2
//
// Fire <+>
// Pot =>  UP/DOWN (arrows)
//
// Joystick 1 :
// clavier numrique :
//                (UP)/
//  (left)7                     (right)9
//               (down)8
//
// Fire <-> or <²>
// Pot => INS /SUPPR
// Cassete : wav file (1 way, 16 bits, 44100hz)

#include "driver.h"
#include "machine/pckeybrd.h"
#include "devices/cassette.h"
#include "sound/wave.h"      // for K7 sound
#include "sound/sn76477.h"   // for sn sound
#include "sound/discrete.h"  // for 1 Bit sound
#include "cpu/z80/z80.h"

#include "includes/hec2hrp.h"


// Rom page (MX machine)
UINT8 rom_p1[0x04000]; // Basic 3x page
UINT8 rom_p2[0x04000]; // Assemblex / Monitrix page

// Ram video BR :
UINT8 videoramhector[0x04000];
// Status for screen definition
UINT8 flag_hr=1;
UINT8 flag_80c=0;
// Status for CPU clock
UINT8 flag_clk=0;  // 0 5MHz (HR machine) - 1  1.75MHz (BR machine)

///////////////////////////////////////////////////////////////////////////////
static ADDRESS_MAP_START(hec2hrp_mem, ADDRESS_SPACE_PROGRAM, 8)
///////////////////////////////////////////////////////////////////////////////
	ADDRESS_MAP_UNMAP_HIGH

    // Hardward address mapping
	AM_RANGE(0x0800,0x0808) AM_WRITE( hector_switch_bank_w)// Bank management
	AM_RANGE(0x1000,0x1000) AM_WRITE( hector_color_a_w)  // Color c0/c1
	AM_RANGE(0x1800,0x1800) AM_WRITE( hector_color_b_w)  // Color c2/c3
	AM_RANGE(0x2000,0x2003) AM_WRITE( hector_sn_2000_w)  // Sound
	AM_RANGE(0x2800,0x2803) AM_WRITE( hector_sn_2800_w)  // Sound
	AM_RANGE(0x3000,0x3000) AM_READWRITE( hector_cassette_r, hector_sn_3000_w)// Write necessary
	AM_RANGE(0x3800,0x3807) AM_READ( hector_keyboard_r)  // Keyboard

    // Main ROM page 
	AM_RANGE(0x0000,0x3fff) AM_ROMBANK(2) 

	// Vidéo br mapping
	AM_RANGE(0x4000,0x49ff) AM_RAM AM_BASE(&videoram)
	// continous RAM
    AM_RANGE(0x4A00,0xbfff) AM_RAM
	// from 0xC000 to 0xFFFF => Bank Ram for vidéo and data !
	AM_RANGE(0xc000,0xffff) AM_RAMBANK(1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hec2hrp_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
    ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hec2mx40_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
    ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x000,0x0ff) AM_READWRITE( hector_mx_io_port_r, hector_mx40_io_port_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START( hec2mx80_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
    ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x000,0x0ff) AM_READWRITE( hector_mx_io_port_r, hector_mx80_io_port_w )
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( hec2hrp )
	PORT_INCLUDE( at_keyboard )
INPUT_PORTS_END

///////////////////////////////////////////////////////////////////////////////
static MACHINE_START( hec2hrp )
///////////////////////////////////////////////////////////////////////////////
{    
UINT8 i;

UINT8 *RAM = memory_region(machine, "maincpu"); // pointer to mess ram
UINT8 *ROM1 = memory_region(machine, "page1"); // pointer to rom page 1
UINT8 *ROM2 = memory_region(machine, "page2"); // pointer to rom page 2

    // Memory install for bank switching
	memory_configure_bank(machine, 1, HECTOR_BANK_PROG , 1, &RAM[0xc000]  , 0); // Mess ram
	memory_configure_bank(machine, 1, HECTOR_BANK_VIDEO, 1, videoramhector, 0); // Video ram

    // Set bank HECTOR_BANK_PROG as basic bank
	memory_set_bank(machine, 1, HECTOR_BANK_PROG);

//////////////////////////////////////////////////////////SPECIFIQUE MX /////////////////
	memory_configure_bank(machine, 2, HECTORMX_BANK_PAGE0 , 1, &RAM[0x0000]  , 0); // Mess ram
	memory_configure_bank(machine, 2, HECTORMX_BANK_PAGE1 , 1, &ROM1[0x0000] , 0); // Rom page 1
	memory_configure_bank(machine, 2, HECTORMX_BANK_PAGE2 , 1, &ROM2[0x0000] , 0); // Rom page 2
	memory_set_bank(machine, 2, HECTORMX_BANK_PAGE0);
//////////////////////////////////////////////////////////SPECIFIQUE MX /////////////////
	// For keyboard
    keyboard_timer = timer_alloc(machine, Callback_keyboard, 0);
    timer_adjust_periodic(keyboard_timer, ATTOTIME_IN_MSEC(100), 0, ATTOTIME_IN_MSEC(20));//keyboard scan 25ms

    // Start keyboard
    at_keyboard_init(machine, AT_KEYBOARD_TYPE_PC);
    at_keyboard_reset();
    at_keyboard_set_scan_code_set(2);
    at_keyboard_reset();

    for(i = 0; i < 8; i++) touches[i] = 0xff;     //all key off

	// For Cassette synchro
    Cassette_timer = timer_alloc(machine, Callback_CK, 0);
    timer_adjust_periodic(Cassette_timer, ATTOTIME_IN_MSEC(100), 0, ATTOTIME_IN_USEC(64));// 11 * 6; 16 *4; 32 *2; 64 => real synchro scan speed for 15,624Khz
 
	// Sound sn76477
	Init_Value_SN76477_Hector();  //init R/C value
}

static MACHINE_RESET(hec2hrp)
{
	memory_set_bank(machine, 1, HECTOR_BANK_PROG);
    memory_set_bank(machine, 2, HECTORMX_BANK_PAGE0);
    flag_hr=1;
    flag_clk =0;
	write_cassette=0;
}

/* Cassette definition */
const struct CassetteOptions hector_cassette_options = {
	1,		/* channels */
	16,		/* bits per sample */
	44100	/* sample frequency */
};

static const cassette_config hector_cassette_config =
{
	cassette_default_formats,
	&hector_cassette_options,
	CASSETTE_STOPPED | CASSETTE_MASK_SPEAKER
};

/* Discrete Sound */
static DISCRETE_SOUND_START( hec2hrp )
	DISCRETE_INPUT_LOGIC(NODE_01)
	DISCRETE_OUTPUT(NODE_01, 5000)
DISCRETE_SOUND_END

///////////////////////////////////////////////////////////////////////////////
static VIDEO_START( hec2hrp )
///////////////////////////////////////////////////////////////////////////////
{
    Init_Hector_Palette(machine);
}

static VIDEO_UPDATE( hec2hrp )
{
    if (flag_hr==1)
        {    
           if (flag_80c==0)
           {
              video_screen_set_visarea(screen, 0, 243, 0, 227);
              hector_hr( bitmap , &videoramhector[0], 227, 64);
           }
           else
           {
               video_screen_set_visarea(screen, 0, 243*2, 0, 227);
               hector_80c( bitmap , &videoramhector[0], 227, 64);
           }
        }
	else
        {
           	video_screen_set_visarea(screen, 0, 113, 0, 75);
            hector_hr( bitmap, videoram,  77, 32);
        }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
static MACHINE_DRIVER_START( hec2hr )
///////////////////////////////////////////////////////////////////////////////
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_5MHz)
	MDRV_CPU_PROGRAM_MAP(hec2hrp_mem)
	MDRV_CPU_IO_MAP(hec2hrp_io)
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
    MDRV_PALETTE_LENGTH(256)
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

MACHINE_DRIVER_END

///////////////////////////////////////////////////////////////////////////////
 MACHINE_DRIVER_START( hec2hrp )
///////////////////////////////////////////////////////////////////////////////
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_5MHz)
	MDRV_CPU_PROGRAM_MAP(hec2hrp_mem)
	MDRV_CPU_IO_MAP(hec2hrp_io)
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
    MDRV_PALETTE_LENGTH(256)
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

MACHINE_DRIVER_END

///////////////////////////////////////////////////////////////////////////////
static MACHINE_DRIVER_START( hec2mx40 )
///////////////////////////////////////////////////////////////////////////////
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_5MHz)
	MDRV_CPU_PROGRAM_MAP(hec2hrp_mem)
	MDRV_CPU_IO_MAP(hec2mx40_io)
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
    MDRV_PALETTE_LENGTH(256)
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

MACHINE_DRIVER_END

///////////////////////////////////////////////////////////////////////////////
static MACHINE_DRIVER_START( hec2mx80 )
///////////////////////////////////////////////////////////////////////////////
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
    MDRV_PALETTE_LENGTH(256)
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

MACHINE_DRIVER_END

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
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP(1983, hec2hrp,  0,       0,     hec2hrp, 	hec2hrp, 0,  	  0,  	 "Micronique",   "Hector 2HR+",	GAME_IMPERFECT_SOUND)
COMP(????, victor,   hec2hrp, 0, 	 hec2hrp,   hec2hrp, 0,       0,  	 "Micronique",   "Victor",	GAME_IMPERFECT_SOUND)
COMP(1983, hec2hr ,  hec2hrp, 0,     hec2hr, 	hec2hrp, 0,  	  0,  	 "Micronique",   "Hector 2HR",	GAME_IMPERFECT_SOUND)
COMP(1984, hec2hrx,  hec2hrp, 0,     hec2hrp, 	hec2hrp, 0,  	  0,  	 "Micronique",   "Hector HRX",	GAME_IMPERFECT_SOUND)
COMP(1985, hec2mx80, hec2hrp, 0,    hec2mx80, 	hec2hrp, 0,  	  0,  	 "Micronique",   "Hector MX 80c" ,	GAME_IMPERFECT_SOUND)
COMP(1985, hec2mx40, hec2hrp, 0,    hec2mx40, 	hec2hrp, 0,  	  0,  	 "Micronique",   "Hector MX 40c" ,	GAME_IMPERFECT_SOUND)
