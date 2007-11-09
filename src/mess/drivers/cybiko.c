/*

	Cybiko Wireless Inter-tainment System

	(c) 2001-2007 Tim Schuerewegen

	Cybiko Classic (V1)
	Cybiko Classic (V2)
	Cybiko Xtreme

*/

#include "includes/cybiko.h"

#include "driver.h"
#include "cpu/h8s2xxx/h8s2xxx.h"
#include "video/hd66421.h"
#include "inputx.h"
#include "cybiko.lh"

//	+------------------------------------------------------+
//	| Cybiko Classic (CY6411)                         | V2 |
//	+------------------------------------------------------+
//	| - CYBIKO | CY-OS 1.1.7 | 6432241M04FA | 0028R JAPAN  |
//	| - SST 39VF020 | 90-4C-WH | 0012175-D                 |
//	| - ATMEL 0027 | AT90S2313-4SC                         |
//	| - ATMEL | AT45DB041A | TC | 0027                     |
//	| - RF2915 | RFMD0028 | 0F540BT                        |
//	| - EliteMT | LP62S2048X-70LLT | 0026B H4A27HA         |
//	| - MP02AB | LMX2315 | TMD                             |
//	+------------------------------------------------------+

//	+------------------------------------------------------+
//	| Cybiko Xtreme (CY44802)                              |
//	+------------------------------------------------------+
//	| - CYBIKO | CYBOOT 1.5A | HD6432323G03F | 0131 JAPAN  |
//	| - SST 39VF400A | 70-4C-EK                            |
//	| - ATMEL 0033 | AT90S2313-4SC                         |
//	| - SAMSUNG 129 | K4F171612D-TL60                      |
//	| - 2E16AB | USBN9604-28M | NSC00A1                    |
//	+------------------------------------------------------+

//	+------------------------------------------------------+
//	| Cybiko MP3 Player (CY65P10)                          |
//	+------------------------------------------------------+
//	| - H8S/2246 | 0G1 | HD6472246FA20 | JAPAN             |
//	| - MICRONAS | DAC3550A C2 | 0394 22 HM U | 089472.000 |
//	| - 2E08AJ | USBN9603-28M | NSC99A1                    |
//	+------------------------------------------------------+

///////////////////////////
// ADDRESS MAP - PROGRAM //
///////////////////////////

#define AM_RANGE_SL( start, length) \
	AM_RANGE( start, start + length - 1)

// 512 kbyte ram + no memory mapped flash
ADDRESS_MAP_START( cybikov1_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE_SL( 0x000000, 0x008000) AM_ROM
//	AM_RANGE_SL( 0x200000, 0x080000) AM_RAM AM_MIRROR( 0x180000)
	AM_RANGE_SL( 0x600000, 0x000002) AM_READWRITE( cybiko_lcd_r, cybiko_lcd_w)
	AM_RANGE_SL( 0xE00000, 0x008000) AM_READ( cybiko_key_r)
ADDRESS_MAP_END

//	+-------------------------------------+
//	| Cybiko Classic (V2) - Memory Map    |
//	+-------------------------------------+
//	| 000000 - 007FFF | rom               |
//	| 008000 - 00FFFF | 17 51 17 51 ..    |
//	| 010000 - 0FFFFF | flash mirror      |
//	| 100000 - 13FFFF | flash             |
//	| 140000 - 1FFFFF | flash mirror      |
//	| 200000 - 23FFFF | ram               |
//	| 240000 - 3FFFFF | ram mirror        |
//	| 400000 - 5FFFFF | FF FF FF FF ..    |
//	| 600000 - 600001 | lcd               |
//	| 600002 - DFFFFF | FF FF FF FF ..    |
//	| E00000 - FFDBFF | keyboard          |
//	| FFDC00 - FFFFFF | onchip ram & regs |
//	+-------------------------------------+

// 256 kbyte ram + 256 kbyte memory mapped flash
ADDRESS_MAP_START( cybikov2_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE_SL( 0x000000, 0x008000) AM_ROM
	AM_RANGE_SL( 0x100000, 0x040000) AM_READ( MRA16_BANK2) AM_MIRROR( 0x0C0000)
//	AM_RANGE_SL( 0x200000, 0x040000) AM_RAM AM_MIRROR( 0x1C0000)
	AM_RANGE_SL( 0x600000, 0x000002) AM_READWRITE( cybiko_lcd_r, cybiko_lcd_w)
	AM_RANGE_SL( 0xE00000, 0x1FDC00) AM_READ( cybiko_key_r)
ADDRESS_MAP_END

// 2048? kbyte ram + 512 kbyte memory mapped flash
ADDRESS_MAP_START( cybikoxt_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE_SL( 0x000000, 0x008000) AM_ROM
	AM_RANGE_SL( 0x100000, 0x000002) AM_READWRITE( cybiko_lcd_r, cybiko_lcd_w)
	AM_RANGE_SL( 0x200000, 0x000004) AM_WRITE( cybiko_unk1_w)
//	AM_RANGE_SL( 0x400000, 0x200000) AM_RAM
	AM_RANGE_SL( 0x600000, 0x080000) AM_READ( MRA16_BANK2)
	AM_RANGE_SL( 0x7FF800, 0x000800) AM_READ( cybiko_unk2_r)
	AM_RANGE_SL( 0xE00000, 0x1FDC00) AM_READ( cybiko_key_r)
ADDRESS_MAP_END

//////////////////////
// ADDRESS MAP - IO //
//////////////////////

ADDRESS_MAP_START( cybikov1_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( 0xFFFE40, 0xFFFFFF) AM_READWRITE( cybikov1_io_reg_r, cybikov1_io_reg_w)
ADDRESS_MAP_END

ADDRESS_MAP_START( cybikov2_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( 0xFFFE40, 0xFFFFFF) AM_READWRITE( cybikov2_io_reg_r, cybikov2_io_reg_w)
ADDRESS_MAP_END

ADDRESS_MAP_START( cybikoxt_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( 0xFFFE40, 0xFFFFFF) AM_READWRITE( cybikoxt_io_reg_r, cybikoxt_io_reg_w)
ADDRESS_MAP_END

/////////////////
// INPUT PORTS //
/////////////////

#define CYBIKO_PORT_BIT_K( mask)  \
	PORT_BIT( mask, IP_ACTIVE_LOW, IPT_KEYBOARD)

#define CYBIKO_PORT_BIT_U( mask)  \
	PORT_BIT( mask, IP_ACTIVE_LOW, IPT_UNUSED)

static INPUT_PORTS_START( cybiko)
	PORT_START // A1
	CYBIKO_PORT_BIT_K( 0x01) PORT_NAME( "F7"    ) PORT_CODE( KEYCODE_F7        ) PORT_CHAR( UCHAR_MAMEKEY( F7    ))
	CYBIKO_PORT_BIT_K( 0x02) PORT_NAME( "Esc"   ) PORT_CODE( KEYCODE_ESC       ) PORT_CHAR( UCHAR_MAMEKEY( ESC   ))
	CYBIKO_PORT_BIT_K( 0x04) PORT_NAME( "Del"   ) PORT_CODE( KEYCODE_DEL       ) PORT_CHAR( UCHAR_MAMEKEY( DEL   ))
	CYBIKO_PORT_BIT_K( 0x08) PORT_NAME( "Left"  ) PORT_CODE( KEYCODE_LEFT      ) PORT_CHAR( UCHAR_MAMEKEY( LEFT  ))
	CYBIKO_PORT_BIT_K( 0x10) PORT_NAME( "Q"     ) PORT_CODE( KEYCODE_Q         ) PORT_CHAR( 'Q'                   )
	CYBIKO_PORT_BIT_K( 0x20) PORT_NAME( "A"     ) PORT_CODE( KEYCODE_A         ) PORT_CHAR( 'A'                   )
	CYBIKO_PORT_BIT_K( 0x40) PORT_NAME( "`"     )                                PORT_CHAR( '`'                   )
	CYBIKO_PORT_BIT_K( 0x80) PORT_NAME( "Shift" ) PORT_CODE( KEYCODE_LSHIFT    ) PORT_CHAR( UCHAR_SHIFT_1         )
	PORT_START // A2
	CYBIKO_PORT_BIT_K( 0x01) PORT_NAME( "F6"    ) PORT_CODE( KEYCODE_F6        ) PORT_CHAR( UCHAR_MAMEKEY( F6    ))
	CYBIKO_PORT_BIT_K( 0x02) PORT_NAME( "Up"    ) PORT_CODE( KEYCODE_UP        ) PORT_CHAR( UCHAR_MAMEKEY( UP    ))
	CYBIKO_PORT_BIT_K( 0x04) PORT_NAME( "Ins"   ) PORT_CODE( KEYCODE_INSERT    ) PORT_CHAR( UCHAR_MAMEKEY( INSERT))
	CYBIKO_PORT_BIT_K( 0x08) PORT_NAME( "2 @"   ) PORT_CODE( KEYCODE_2         ) PORT_CHAR( '2'                   ) PORT_CHAR( '@')
	CYBIKO_PORT_BIT_K( 0x10) PORT_NAME( "W"     ) PORT_CODE( KEYCODE_W         ) PORT_CHAR( 'W'                   )
	CYBIKO_PORT_BIT_K( 0x20) PORT_NAME( "S"     ) PORT_CODE( KEYCODE_S         ) PORT_CHAR( 'S'                   )
	CYBIKO_PORT_BIT_K( 0x40) PORT_NAME( "Z"     ) PORT_CODE( KEYCODE_Z         ) PORT_CHAR( 'Z'                   )
	CYBIKO_PORT_BIT_K( 0x80) PORT_NAME( "Fn"    ) PORT_CODE( KEYCODE_LCONTROL  ) PORT_CHAR( UCHAR_SHIFT_2         )
	PORT_START // A3
	CYBIKO_PORT_BIT_K( 0x01) PORT_NAME( "F5"    ) PORT_CODE( KEYCODE_F5        ) PORT_CHAR( UCHAR_MAMEKEY( F5    ))
	CYBIKO_PORT_BIT_K( 0x02) PORT_NAME( "F3"    ) PORT_CODE( KEYCODE_F3        ) PORT_CHAR( UCHAR_MAMEKEY( F3    ))
	CYBIKO_PORT_BIT_K( 0x04) PORT_NAME( "Space" ) PORT_CODE( KEYCODE_SPACE     ) PORT_CHAR( ' '                   )
	CYBIKO_PORT_BIT_K( 0x08) PORT_NAME( "3 #"   ) PORT_CODE( KEYCODE_3         ) PORT_CHAR( '3'                   ) PORT_CHAR( '#')
	CYBIKO_PORT_BIT_K( 0x10) PORT_NAME( "E"     ) PORT_CODE( KEYCODE_E         ) PORT_CHAR( 'E'                   )
	CYBIKO_PORT_BIT_K( 0x20) PORT_NAME( "D"     ) PORT_CODE( KEYCODE_D         ) PORT_CHAR( 'D'                   )
	CYBIKO_PORT_BIT_K( 0x40) PORT_NAME( "X"     ) PORT_CODE( KEYCODE_X         ) PORT_CHAR( 'X'                   )
	CYBIKO_PORT_BIT_K( 0x80) PORT_NAME( "Help"  ) PORT_CODE( KEYCODE_END       ) PORT_CHAR( UCHAR_MAMEKEY( END   ))
	PORT_START // A4
	CYBIKO_PORT_BIT_K( 0x01) PORT_NAME( "F4"    ) PORT_CODE( KEYCODE_F4        ) PORT_CHAR( UCHAR_MAMEKEY( F4    ))
	CYBIKO_PORT_BIT_K( 0x02) PORT_NAME( "1 !"   ) PORT_CODE( KEYCODE_1         ) PORT_CHAR( '1'                   ) PORT_CHAR( '!')
	CYBIKO_PORT_BIT_K( 0x04) PORT_NAME( "Tab"   ) PORT_CODE( KEYCODE_TAB       ) PORT_CHAR( 9                     )
	CYBIKO_PORT_BIT_K( 0x08) PORT_NAME( "4 $"   ) PORT_CODE( KEYCODE_4         ) PORT_CHAR( '4'                   ) PORT_CHAR( '$')
	CYBIKO_PORT_BIT_K( 0x10) PORT_NAME( "R"     ) PORT_CODE( KEYCODE_R         ) PORT_CHAR( 'R'                   )
	CYBIKO_PORT_BIT_K( 0x20) PORT_NAME( "F"     ) PORT_CODE( KEYCODE_F         ) PORT_CHAR( 'F'                   )
	CYBIKO_PORT_BIT_K( 0x40) PORT_NAME( "C"     ) PORT_CODE( KEYCODE_C         ) PORT_CHAR( 'C'                   )
	CYBIKO_PORT_BIT_K( 0x80) PORT_NAME( "[ {"   ) PORT_CODE( KEYCODE_OPENBRACE ) PORT_CHAR( '['                   )
	PORT_START // A5
	CYBIKO_PORT_BIT_K( 0x01) PORT_NAME( "Right" ) PORT_CODE( KEYCODE_RIGHT     ) PORT_CHAR( UCHAR_MAMEKEY( RIGHT ))
	CYBIKO_PORT_BIT_K( 0x02) PORT_NAME( "Down"  ) PORT_CODE( KEYCODE_DOWN      ) PORT_CHAR( UCHAR_MAMEKEY( DOWN  ))
	CYBIKO_PORT_BIT_K( 0x04) PORT_NAME( "Select") PORT_CODE( KEYCODE_HOME      ) PORT_CHAR( UCHAR_MAMEKEY( HOME  ))
	CYBIKO_PORT_BIT_K( 0x08) PORT_NAME( "5 %"   ) PORT_CODE( KEYCODE_5         ) PORT_CHAR( '5'                   ) PORT_CHAR( '%')
	CYBIKO_PORT_BIT_K( 0x10) PORT_NAME( "T"     ) PORT_CODE( KEYCODE_T         ) PORT_CHAR( 'T'                   )
	CYBIKO_PORT_BIT_K( 0x20) PORT_NAME( "G"     ) PORT_CODE( KEYCODE_G         ) PORT_CHAR( 'G'                   )
	CYBIKO_PORT_BIT_K( 0x40) PORT_NAME( "V"     ) PORT_CODE( KEYCODE_V         ) PORT_CHAR( 'V'                   )
	CYBIKO_PORT_BIT_K( 0x80) PORT_NAME( "] }"   ) PORT_CODE( KEYCODE_CLOSEBRACE) PORT_CHAR( ']'                   )
	PORT_START // A6
	CYBIKO_PORT_BIT_K( 0x01) PORT_NAME( "F2"    ) PORT_CODE( KEYCODE_F2        ) PORT_CHAR( UCHAR_MAMEKEY( F2    ))
	CYBIKO_PORT_BIT_K( 0x02) PORT_NAME( "; :"   ) PORT_CODE( KEYCODE_COLON     ) PORT_CHAR( ';'                   ) PORT_CHAR( ':')
	CYBIKO_PORT_BIT_K( 0x04) PORT_NAME( "Enter" ) PORT_CODE( KEYCODE_ENTER     ) PORT_CHAR( 13                    )
	CYBIKO_PORT_BIT_K( 0x08) PORT_NAME( "6 ^"   ) PORT_CODE( KEYCODE_6         ) PORT_CHAR( '6'                   )
	CYBIKO_PORT_BIT_K( 0x10) PORT_NAME( "Y"     ) PORT_CODE( KEYCODE_Y         ) PORT_CHAR( 'Y'                   )
	CYBIKO_PORT_BIT_K( 0x20) PORT_NAME( "H"     ) PORT_CODE( KEYCODE_H         ) PORT_CHAR( 'H'                   )
	CYBIKO_PORT_BIT_K( 0x40) PORT_NAME( "B"     ) PORT_CODE( KEYCODE_B         ) PORT_CHAR( 'B'                   )
	CYBIKO_PORT_BIT_K( 0x80) PORT_NAME( "\\ |"  ) PORT_CODE( KEYCODE_BACKSLASH ) PORT_CHAR( '\\'                  ) PORT_CHAR( '|')
	PORT_START // A7
	CYBIKO_PORT_BIT_K( 0x01) PORT_NAME( "F1"    ) PORT_CODE( KEYCODE_F1        ) PORT_CHAR( UCHAR_MAMEKEY( F1    ))
	CYBIKO_PORT_BIT_K( 0x02) PORT_NAME( "/ ?"   ) PORT_CODE( KEYCODE_SLASH     ) PORT_CHAR( '/'                   ) PORT_CHAR( '?')
	CYBIKO_PORT_BIT_K( 0x04) PORT_NAME( "BkSp"  ) PORT_CODE( KEYCODE_BACKSPACE ) PORT_CHAR( 8                     )
	CYBIKO_PORT_BIT_K( 0x08) PORT_NAME( "7 &"   ) PORT_CODE( KEYCODE_7         ) PORT_CHAR( '7'                   ) PORT_CHAR( '&')
	CYBIKO_PORT_BIT_K( 0x10) PORT_NAME( "U"     ) PORT_CODE( KEYCODE_U         ) PORT_CHAR( 'U'                   )
	CYBIKO_PORT_BIT_K( 0x20) PORT_NAME( "J"     ) PORT_CODE( KEYCODE_J         ) PORT_CHAR( 'J'                   )
	CYBIKO_PORT_BIT_K( 0x40) PORT_NAME( "N"     ) PORT_CODE( KEYCODE_N         ) PORT_CHAR( 'N'                   )
	CYBIKO_PORT_BIT_U( 0x80)
	PORT_START // A8
	CYBIKO_PORT_BIT_K( 0x01) PORT_NAME( "- _"   ) PORT_CODE( KEYCODE_MINUS     ) PORT_CHAR( '-'                   ) PORT_CHAR( '_')
	CYBIKO_PORT_BIT_K( 0x02) PORT_NAME( ". >"   )                                PORT_CHAR( '.'                   ) PORT_CHAR( '>')
	CYBIKO_PORT_BIT_K( 0x04) PORT_NAME( "0 )"   ) PORT_CODE( KEYCODE_0         ) PORT_CHAR( '0'                   ) PORT_CHAR( ')')
	CYBIKO_PORT_BIT_K( 0x08) PORT_NAME( "8 *"   ) PORT_CODE( KEYCODE_8         ) PORT_CHAR( '8'                   ) PORT_CHAR( '*')
	CYBIKO_PORT_BIT_K( 0x10) PORT_NAME( "I"     ) PORT_CODE( KEYCODE_I         ) PORT_CHAR( 'I'                   )
	CYBIKO_PORT_BIT_K( 0x20) PORT_NAME( "K"     ) PORT_CODE( KEYCODE_K         ) PORT_CHAR( 'K'                   )
	CYBIKO_PORT_BIT_K( 0x40) PORT_NAME( "M"     ) PORT_CODE( KEYCODE_M         ) PORT_CHAR( 'M'                   )
	CYBIKO_PORT_BIT_U( 0x80)
	PORT_START // A9
	CYBIKO_PORT_BIT_K( 0x01) PORT_NAME( "' \""  ) PORT_CODE( KEYCODE_QUOTE     ) PORT_CHAR( '\''                  ) PORT_CHAR( '"')
	CYBIKO_PORT_BIT_K( 0x02) PORT_NAME( "= +"   ) PORT_CODE( KEYCODE_EQUALS    ) PORT_CHAR( '='                   ) PORT_CHAR( '+')
	CYBIKO_PORT_BIT_K( 0x04) PORT_NAME( "9 ("   ) PORT_CODE( KEYCODE_9         ) PORT_CHAR( '9'                   ) PORT_CHAR( '(')
	CYBIKO_PORT_BIT_K( 0x08) PORT_NAME( "P"     ) PORT_CODE( KEYCODE_P         ) PORT_CHAR( 'P'                   )
	CYBIKO_PORT_BIT_K( 0x10) PORT_NAME( "O"     ) PORT_CODE( KEYCODE_O         ) PORT_CHAR( 'O'                   )
	CYBIKO_PORT_BIT_K( 0x20) PORT_NAME( "L"     ) PORT_CODE( KEYCODE_L         ) PORT_CHAR( 'L'                   )
	CYBIKO_PORT_BIT_K( 0x40) PORT_NAME( ", <"   ) PORT_CODE( KEYCODE_COMMA     ) PORT_CHAR( ','                   ) PORT_CHAR( '<')
	CYBIKO_PORT_BIT_U( 0x80)
INPUT_PORTS_END

/////////
// ROM //
/////////

ROM_START( cybikov1 )
	ROM_REGION( 0x8000, REGION_CPU1, 0)
	ROM_LOAD( "cyrom112.bin", 0, 0x8000, CRC(9E1F1A0F) SHA1(6FC08DE6B2C67D884EC78F748E4A4BAD27EE8045))
ROM_END

ROM_START( cybikov2 )
	ROM_REGION( 0x8000, REGION_CPU1, 0)
	ROM_LOAD( "cyrom117.bin", 0, 0x8000, CRC(268DA7BF) SHA1(135EAF9E3905E69582AABD9B06BC4DE0A66780D5))
ROM_END

ROM_START( cybikoxt )
	ROM_REGION( 0x8000, REGION_CPU1, 0)
	ROM_LOAD( "cyrom150.bin", 0, 0x8000, CRC(18B9B21F) SHA1(28868D6174EB198A6CEC6C3C70B6E494517229B9))
ROM_END

///////////////////
// SYSTEM CONFIG //
///////////////////

SYSTEM_CONFIG_START( cybikov1 )
	CONFIG_RAM_DEFAULT( 512 * 1024)
	CONFIG_RAM( 1024 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( cybikov2 )
	CONFIG_RAM_DEFAULT( 256 * 1024)
	CONFIG_RAM(  512 * 1024)
	CONFIG_RAM( 1024 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( cybikoxt )
	CONFIG_RAM_DEFAULT( 2048 * 1024)
SYSTEM_CONFIG_END

////////////////////
// MACHINE DRIVER //
////////////////////

MACHINE_DRIVER_START( cybikov1 )
	// cpu
	MDRV_CPU_ADD_TAG( "main", H8S2241, 11059200)
	MDRV_CPU_PROGRAM_MAP( cybikov1_mem, 0)
	MDRV_CPU_IO_MAP( cybikov1_io, 0)
	// screen
	MDRV_SCREEN_REFRESH_RATE( 60)
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( HD66421_WIDTH, HD66421_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA( 0, HD66421_WIDTH - 1, 0, HD66421_HEIGHT - 1)
	// video
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER)
	MDRV_PALETTE_LENGTH( 4)
	MDRV_COLORTABLE_LENGTH( 4)
	MDRV_PALETTE_INIT( hd66421)
	MDRV_VIDEO_START( hd66421)
	MDRV_VIDEO_UPDATE( hd66421)
	MDRV_DEFAULT_LAYOUT( layout_cybiko)
	// sound
	MDRV_SPEAKER_STANDARD_MONO( "mono")
	MDRV_SOUND_ADD_TAG( "speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00)
	// machine
	MDRV_MACHINE_START( cybikov1)
	MDRV_MACHINE_RESET( cybikov1)
	// non-volatile ram
//	MDRV_NVRAM_HANDLER( cybikov1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cybikov2 )
	// import
	MDRV_IMPORT_FROM( cybikov1)
	// cpu
	MDRV_CPU_REPLACE( "main", H8S2246, 11059200)
	MDRV_CPU_PROGRAM_MAP( cybikov2_mem, 0)
	MDRV_CPU_IO_MAP( cybikov2_io, 0)
	// machine
	MDRV_MACHINE_START( cybikov2)
	MDRV_MACHINE_RESET( cybikov2)
	// non-volatile ram
//	MDRV_NVRAM_HANDLER( cybikov2)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cybikoxt )
	// import
	MDRV_IMPORT_FROM( cybikov1)
	// cpu
	MDRV_CPU_REPLACE( "main", H8S2323, 18432000)
	MDRV_CPU_PROGRAM_MAP( cybikoxt_mem, 0)
	MDRV_CPU_IO_MAP( cybikoxt_io, 0)
	// sound
	MDRV_SPEAKER_REMOVE( "mono")
	MDRV_SOUND_REMOVE( "speaker")
	// machine
	MDRV_MACHINE_START( cybikoxt)
	MDRV_MACHINE_RESET( cybikoxt)
	// non-volatile ram
//	MDRV_NVRAM_HANDLER( cybikoxt)
MACHINE_DRIVER_END

//////////////
// COMPUTER //
//////////////

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT        CONFIG      COMPANY         FULLNAME                FLAGS */
COMP( 2000, cybikov1,   0,          0,      cybikov1,   cybiko, cybikov1,   cybikov1,   "Cybiko, Inc.", "Cybiko Classic (V1)",  GAME_IMPERFECT_SOUND )
COMP( 2000, cybikov2,   cybikov1,   0,      cybikov2,   cybiko, cybikov2,   cybikov2,   "Cybiko, Inc.", "Cybiko Classic (V2)",  GAME_IMPERFECT_SOUND )
COMP( 2001, cybikoxt,   cybikov1,   0,      cybikoxt,   cybiko, cybikoxt,   cybikoxt,   "Cybiko, Inc.", "Cybiko Xtreme",        GAME_NO_SOUND | GAME_NOT_WORKING )
