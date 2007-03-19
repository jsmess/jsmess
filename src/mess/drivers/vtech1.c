/******************************************************************************

Video Technology Laser 110-310 computers:

    Video Technology Laser 110
      Sanyo Laser 110
    Video Technology Laser 200
      Salora Fellow
      Texet TX-8000
      Video Technology VZ-200
    Video Technology Laser 210
      Dick Smith Electronics VZ-200
      Sanyo Laser 210
    Video Technology Laser 310
      Dick Smith Electronics VZ-300

System driver:

    Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999
      - everything
      
    Dirk Best <duke@redump.de>, May 2004
      - clean up
      - fixed parent/clone relationsips and memory size for the Laser 200
      - fixed loading of the DOS ROM
      - added BASIC V2.1
      - added SHA1 checksums

	Dirk Best <duke@redump.de>, March 2006
	  - 64KB memory expansion (banked)
	  - cartridge support

Thanks go to:

    - Guy Thomason
    - Jason Oakley
    - Bushy Maunder
    - and anybody else on the vzemu list :)
    - Davide Moretti for the detailed description of the colors
    - Leslie Milburn

Memory maps:

    Laser 110/200
        0000-1FFF 8K ROM 0
        2000-3FFF 8K ROM 1
        4000-5FFF 8K DOS ROM or other cartridges (optional)
        6000-67FF 2K reserved for rom cartridges
        6800-6FFF 2K memory mapped I/O
                    R: keyboard
                    W: cassette I/O, speaker, VDP control
        7000-77FF 2K video RAM
        7800-7FFF 2K internal user RAM
        8800-C7FF 16K memory expansion
        8000-BFFF 64K memory expansion, first bank
        C000-FFFF 64K memory expansion, other banks
        
    Laser 210
        0000-1FFF 8K ROM 0
        2000-3FFF 8K ROM 1
        4000-5FFF 8K DOS ROM or other cartridges (optional)
        6000-67FF 2K reserved for rom cartridges
        6800-6FFF 2K memory mapped I/O
                    R: keyboard
                    W: cassette I/O, speaker, VDP control
        7000-77FF 2K video RAM
        7800-8FFF 6K internal user RAM (3x2K: U2, U3, U4)
        9000-CFFF 16K memory expansion
        8000-BFFF 64K memory expansion, first bank
        C000-FFFF 64K memory expansion, other banks
    
    Laser 310
        0000-3FFF 16K ROM
        4000-5FFF 8K DOS ROM or other cartridges (optional)
        6000-67FF 2K reserved for rom cartridges
        6800-6FFF 2K memory mapped I/O
                    R: keyboard
                    W: cassette I/O, speaker, VDP control
        7000-77FF 2K video RAM
        7800-B7FF 16K internal user RAM
        B800-F7FF 16K memory expansion
        8000-BFFF 64K memory expansion, first bank
        C000-FFFF 64K memory expansion, other banks

Todo:

    - Figure out which machines were shipped with which ROM version
      where not known (currently only a guess)
    - Lightpen support
    - External keyboard? (and maybe the other strange I/O stuff which is
      commented out at the moment...)

Notes:

    - The only known dumped cartridge is the DOS ROM:
      CRC(b6ed6084) SHA1(59d1cbcfa6c5e1906a32704fbf0d9670f0d1fd8b)


******************************************************************************/


#include "driver.h"
#include "video/generic.h"
#include "video/m6847.h"
#include "includes/vtech1.h"
#include "devices/cartslot.h"
#include "devices/snapquik.h"
#include "devices/cassette.h"
#include "devices/printer.h"
#include "devices/z80bin.h"
#include "formats/vt_cas.h"
#include "sound/speaker.h"
#include "inputx.h"


/******************************************************************************
 Address Maps
******************************************************************************/

/* Note: Expansion memory is dynamically mapped in machine/vtech1.c */

static ADDRESS_MAP_START(laser110_mem, ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE(0x0000, 0x3fff) AM_ROM	/* basic rom */
    AM_RANGE(0x4000, 0x5fff) AM_ROM	/* dos rom or other catridges */
    AM_RANGE(0x6000, 0x67ff) AM_ROM	/* reserved for cartridges */
    AM_RANGE(0x6800, 0x6fff) AM_READWRITE(vtech1_keyboard_r, vtech1_latch_w)
    AM_RANGE(0x7000, 0x77ff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size) /* (6847) */
    AM_RANGE(0x7800, 0x7fff) AM_RAMBANK(1) /* 2KB user ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START(laser210_mem, ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE(0x0000, 0x3fff) AM_ROM	/* basic rom */
    AM_RANGE(0x4000, 0x5fff) AM_ROM	/* dos rom or other catridges */
    AM_RANGE(0x6000, 0x67ff) AM_ROM	/* reserved for cartridges */
    AM_RANGE(0x6800, 0x6fff) AM_READWRITE(vtech1_keyboard_r, vtech1_latch_w)
    AM_RANGE(0x7000, 0x77ff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size) /* U7 (6847) */
    AM_RANGE(0x7800, 0x8fff) AM_RAMBANK(1) /* 6KB user ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START(laser310_mem, ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE(0x0000, 0x3fff) AM_ROM	/* basic rom */
    AM_RANGE(0x4000, 0x5fff) AM_ROM	/* dos rom or other catridges */
    AM_RANGE(0x6000, 0x67ff) AM_ROM	/* reserved for cartridges */
    AM_RANGE(0x6800, 0x6fff) AM_READWRITE(vtech1_keyboard_r, vtech1_latch_w)
    AM_RANGE(0x7000, 0x77ff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size) /* (6847) */
    AM_RANGE(0x7800, 0xb7ff) AM_RAMBANK(1) /* 16KB user ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START(vtech1_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x0f) AM_READWRITE(vtech1_printer_r, vtech1_printer_w)
	AM_RANGE(0x10, 0x1f) AM_READWRITE(vtech1_fdc_r, vtech1_fdc_w)
	AM_RANGE(0x20, 0x2f) AM_READ(vtech1_joystick_r)
	AM_RANGE(0x30, 0x3f) AM_READWRITE(vtech1_serial_r, vtech1_serial_w)
	AM_RANGE(0x40, 0x4f) AM_READ(vtech1_lightpen_r)
	AM_RANGE(0x50, 0x5f) AM_NOP /* Real time clock (proposed) */
	AM_RANGE(0x60, 0x6f) AM_NOP /* External keyboard */
	AM_RANGE(0x70, 0x7f) AM_WRITE(vtech1_memory_bank_w)
	AM_RANGE(0x80, 0xff) AM_NOP
//	AM_RANGE(0xc9, 0xca) AM_NOP /* Eprom programmer */
//	AM_RANGE(0xd8, 0xe7) AM_NOP /* Auto boot at 8000-9fff (proposed) */
//	AM_RANGE(0xe8, 0xf7) AM_NOP /* RDOS at 6000-67ff (proposed) */
//	AM_RANGE(0xf0, 0xf3) AM_NOP /* 24 bit I/O interface */
//	AM_RANGE(0xf8, 0xff) AM_NOP /* RAM Disk at 4000-5fff (proposed) */
//	AM_RANGE(0xe0, 0xff) AM_NOP /* D. Newcombes Rom Board */
ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/

INPUT_PORTS_START(vtech1)
	PORT_START_TAG("keyboard_0")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R       RETURN  LEFT$")   PORT_CODE(KEYCODE_R)     PORT_CHAR('R')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q       FOR     CHR$")    PORT_CODE(KEYCODE_Q)     PORT_CHAR('Q')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E       NEXT    LEN(")    PORT_CODE(KEYCODE_E)     PORT_CHAR('E')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W       TO      VAL(")    PORT_CODE(KEYCODE_W)     PORT_CHAR('W')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T       THEN    MID$")    PORT_CODE(KEYCODE_T)     PORT_CHAR('T')
	
	PORT_START_TAG("keyboard_1")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F       GOSUB   RND(")    PORT_CODE(KEYCODE_F)     PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A       MODE(   ASC(")    PORT_CODE(KEYCODE_A)     PORT_CHAR('A')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D       DIM     RESTORE") PORT_CODE(KEYCODE_D)     PORT_CHAR('D')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL")                    PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S       STEP    STR$(")   PORT_CODE(KEYCODE_S)     PORT_CHAR('S')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G       GOTO    STOP")    PORT_CODE(KEYCODE_G)     PORT_CHAR('G')
	
	PORT_START_TAG("keyboard_2")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V       LPRINT  USR")     PORT_CODE(KEYCODE_V)     PORT_CHAR('V')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z       PEEK(   INP")     PORT_CODE(KEYCODE_Z)     PORT_CHAR('Z')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C       CONT    COPY")    PORT_CODE(KEYCODE_C)     PORT_CHAR('C')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT")                   PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X       POKE    OUT")     PORT_CODE(KEYCODE_X)     PORT_CHAR('X')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B       LLIST   SOUND")   PORT_CODE(KEYCODE_B)     PORT_CHAR('B')
	
	PORT_START_TAG("keyboard_3")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4  $    VERIFY  ATN(")    PORT_CODE(KEYCODE_4)     PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1  !    CSAVE   SIN(")    PORT_CODE(KEYCODE_1)     PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3  #    CRUN    TAN(")    PORT_CODE(KEYCODE_3)     PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2  \"    CLOAD   COS(")   PORT_CODE(KEYCODE_2)     PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5  %    LIST    LOG(")    PORT_CODE(KEYCODE_5)     PORT_CHAR('5') PORT_CHAR('%')
	
	PORT_START_TAG("keyboard_4")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M  \\    \xE2\x86\x90")   PORT_CODE(KEYCODE_M)     PORT_CHAR('M') PORT_CHAR('\\') PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE   \xE2\x86\x93")    PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ') PORT_CHAR('~')  PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",  <    \xE2\x86\x92")    PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')  PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".  >    \xE2\x86\x91")    PORT_CODE(KEYCODE_STOP)  PORT_CHAR('.') PORT_CHAR('>')  PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N  ^    COLOR   USING")   PORT_CODE(KEYCODE_N)     PORT_CHAR('N') PORT_CHAR('^')
	
	PORT_START_TAG("keyboard_5")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7  '    END     SGN(")    PORT_CODE(KEYCODE_7)     PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0  @    DATA    INT(")    PORT_CODE(KEYCODE_0)     PORT_CHAR('0') PORT_CHAR('@')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8  (    NEW     SQR(")    PORT_CODE(KEYCODE_8)     PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-  =    [Break]")         PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')  PORT_CHAR(UCHAR_MAMEKEY(CANCEL))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9  )    READ    ABS(")    PORT_CODE(KEYCODE_9)     PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6  &    RUN     EXP(")    PORT_CODE(KEYCODE_6)     PORT_CHAR('6') PORT_CHAR('&')
	
	PORT_START_TAG("keyboard_6")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U       IF      INKEY$")  PORT_CODE(KEYCODE_U)     PORT_CHAR('U')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P  ]    PRINT   NOT")     PORT_CODE(KEYCODE_P)     PORT_CHAR('P') PORT_CHAR(']')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I       INPUT   AND")     PORT_CODE(KEYCODE_I)     PORT_CHAR('I')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN  [Function]")      PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O  [    LET     OR")      PORT_CODE(KEYCODE_O)     PORT_CHAR('O') PORT_CHAR('[')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y       ELSE    RIGHT$(") PORT_CODE(KEYCODE_Y)     PORT_CHAR('Y')
	
	PORT_START_TAG("keyboard_7")
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J       REM     RESET")   PORT_CODE(KEYCODE_J)     PORT_CHAR('J')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";  +    [Rubout]")        PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')  PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K  /    TAB(    POINT")   PORT_CODE(KEYCODE_K)     PORT_CHAR('K') PORT_CHAR('/')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":  *    [Inverse]")       PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')  PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L  ?    [Insert]")        PORT_CODE(KEYCODE_L)     PORT_CHAR('L') PORT_CHAR('?')  PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H       CLS     SET")     PORT_CODE(KEYCODE_H)     PORT_CHAR('H')
	
	PORT_START_TAG("joystick_0")
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1)        PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(1)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(1)
	
	PORT_START_TAG("joystick_0_arm")
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON2)        PORT_PLAYER(1)
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
	
	PORT_START_TAG("joystick_1")
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1)        PORT_PLAYER(2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(2)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(2)
	
	PORT_START_TAG("joystick_1_arm")
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON2)        PORT_PLAYER(2)
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


/******************************************************************************
 Audio Initialisation
******************************************************************************/

static INT16 speaker_levels[] = {-32768, 0, 32767, 0};

static struct Speaker_interface speaker_interface =
{
	4,
	speaker_levels
};


/******************************************************************************
 Machine Drivers
******************************************************************************/

static MACHINE_DRIVER_START(laser110)
    /* basic machine hardware */
    MDRV_CPU_ADD_TAG("main", Z80, VTECH1_CLK)  /* 3.57950 Mhz */
    MDRV_CPU_PROGRAM_MAP(laser110_mem, 0)
    MDRV_CPU_IO_MAP(vtech1_io, 0)
    MDRV_CPU_VBLANK_INT(vtech1_interrupt,1)
	MDRV_SCREEN_REFRESH_RATE(M6847_PAL_FRAMES_PER_SECOND)
    MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START(laser110)

    /* video hardware */
	MDRV_VIDEO_START(vtech1m)
	MDRV_VIDEO_UPDATE(m6847)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 25+192+26)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 1, 239)

    /* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_CONFIG(speaker_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(laser200)
    MDRV_IMPORT_FROM(laser110)

	MDRV_VIDEO_START(vtech1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(laser210)
    MDRV_IMPORT_FROM(laser200)
    MDRV_CPU_MODIFY("main")
    MDRV_CPU_PROGRAM_MAP(laser210_mem, 0)

    MDRV_MACHINE_START(laser210)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(laser310)
    MDRV_IMPORT_FROM( laser200 )
    MDRV_CPU_REPLACE( "main", Z80, VZ300_XTAL1_CLK/5)  /* 3.546894 Mhz */
    MDRV_CPU_PROGRAM_MAP(laser310_mem, 0)

    MDRV_MACHINE_START(laser310)
MACHINE_DRIVER_END


/******************************************************************************
 ROM Definitions
******************************************************************************/

ROM_START(laser110)
    ROM_REGION(0x6800, REGION_CPU1, 0)
    ROM_LOAD("vtechv12.u09",   0x0000, 0x2000, CRC(99412d43) SHA1(6aed8872a0818be8e1b08ecdfd92acbe57a3c96d))
    ROM_LOAD("vtechv12.u10",   0x2000, 0x2000, CRC(e4c24e8b) SHA1(9d8fb3d24f3d4175b485cf081a2d5b98158ab2fb))
    ROM_CART_LOAD(0, "rom\0",  0x4000, 0x27ff, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

/* The VZ-200 sold in Germany and the Netherlands came with BASIC V1.1, which
   is currently not dumped. */
ROM_START(vz200de)
    ROM_REGION(0x6800, REGION_CPU1, 0)
    ROM_LOAD("vtechv11.u09",   0x0000, 0x2000, NO_DUMP)
    ROM_LOAD("vtechv11.u10",   0x2000, 0x2000, NO_DUMP)
    ROM_CART_LOAD(0, "rom\0",  0x4000, 0x27ff, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

#define rom_las110de    rom_laser110
#define rom_laser200    rom_laser110
#define rom_fellow      rom_laser110

/* It's possible that the Texet TX8000 came with BASIC V1.0, but this
   needs to be verified */
#define rom_tx8000      rom_laser110

ROM_START(laser210)
    ROM_REGION(0x6800, REGION_CPU1, 0)
    ROM_LOAD("vtechv20.u09",   0x0000, 0x2000, CRC(cc854fe9) SHA1(6e66a309b8e6dc4f5b0b44e1ba5f680467353d66))
    ROM_LOAD("vtechv20.u10",   0x2000, 0x2000, CRC(7060f91a) SHA1(8f3c8f24f97ebb98f3c88d4e4ba1f91ffd563440))
    ROM_CART_LOAD(0, "rom\0",  0x4000, 0x27ff, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

#define rom_las210de    rom_laser210
#define rom_vz200       rom_laser210

SYSTEM_BIOS_START(laser310)
	SYSTEM_BIOS_ADD(0, "basic20", "BASIC V2.0")
	SYSTEM_BIOS_ADD(1, "basic21", "BASIC V2.1 (hack)")
SYSTEM_BIOS_END

ROM_START(laser310)
    ROM_REGION(0x6800, REGION_CPU1, 0)
    ROMX_LOAD("vtechv20.u12", 0x0000, 0x4000, CRC(613de12c) SHA1(f216c266bc09b0dbdbad720796e5ea9bc7d91e53), ROM_BIOS(1))
    ROMX_LOAD("vtechv21.u12", 0x0000, 0x4000, CRC(f7df980f) SHA1(5ba14a7a2eedca331b033901080fa5d205e245ea), ROM_BIOS(2))
    ROM_CART_LOAD(0, "rom\0", 0x4000, 0x27ff, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

#define rom_vz300       rom_laser310


/******************************************************************************
 System Config
******************************************************************************/

static void vtech1_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

static void vtech1_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_CASSETTE_FORMATS:				info->p = (void *) vtech1_cassette_formats; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void vtech1_snapshot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* snapshot */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "vz"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_SNAPSHOT_LOAD:					info->f = (genf *) snapshot_load_vtech1; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_SNAPSHOT_DELAY:				info->d = 0.5; break;

		default:										snapshot_device_getinfo(devclass, state, info); break;
	}
}

static void vtech1_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_FLOPPY; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 1; break;
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_vtech1_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;
	}
}

/* Memory expansions available for the Laser/VZ computers:

   - a 16kb expansion without banking
   - a 64kb expansion where the first bank is fixed and the other 3 are
     banked in as needed
   - a banked memory expansion similar to the 64kb one, that the user could
     fill themselves with memory up to 4MB total.

   They are externally connected devices. The 16kb extension is different
   between Laser 110/210/310 computers, though it could be relativly
   easily modified to work on another model. */

SYSTEM_CONFIG_START(vtech1)
    CONFIG_DEVICE(vtech1_printer_getinfo)
    CONFIG_DEVICE(cartslot_device_getinfo)
    CONFIG_DEVICE(vtech1_cassette_getinfo)
    CONFIG_DEVICE(vtech1_snapshot_getinfo)
    CONFIG_DEVICE(vtech1_floppy_getinfo)
	CONFIG_DEVICE(z80bin_quickload_getinfo)
	CONFIG_RAM_DEFAULT (66 * 1024)   /* with 64K memory expansion */
	CONFIG_RAM         (4098 * 1024) /* with 4MB memory expansion */
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(laser110)
	CONFIG_IMPORT_FROM (vtech1)
	CONFIG_RAM         ( 2 * 1024)   /* standard */
	CONFIG_RAM         (18 * 1024)   /* with 16K memory expansion */
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(laser210)
	CONFIG_IMPORT_FROM (vtech1)
	CONFIG_RAM         ( 6 * 1024)   /* standard */
	CONFIG_RAM         (22 * 1024)   /* with 16K memory expansion */
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(laser310)
	CONFIG_IMPORT_FROM (vtech1)
	CONFIG_RAM         (16 * 1024)   /* standard */
	CONFIG_RAM         (32 * 1024)   /* with 16K memory expansion */
SYSTEM_CONFIG_END


/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME      PARENT    BIOS COMPAT  MACHINE   INPUT   INIT  CONFIG    COMPANY                   FULLNAME                          FLAGS */
COMP( 1983, laser110,        0,           0, laser110, vtech1, NULL, laser110, "Video Technology",       "Laser 110"                     , 0)
COMP( 1983, las110de, laser110,           0, laser110, vtech1, NULL, laser110, "Sanyo",                  "Laser 110 (Germany)"           , 0)

COMP( 1983, laser200,        0,           0, laser200, vtech1, NULL, laser110, "Video Technology",       "Laser 200"                     , 0)
COMP( 1983, vz200de,  laser200,           0, laser200, vtech1, NULL, laser110, "Video Technology",       "VZ-200 (Germany & Netherlands)", 0)
COMP( 1983, fellow,   laser200,           0, laser200, vtech1, NULL, laser110, "Salora",                 "Fellow (Finland)"              , 0)
COMP( 1983, tx8000,   laser200,           0, laser200, vtech1, NULL, laser110, "Texet",                  "TX-8000 (UK)"                  , 0)

COMP( 1984, laser210,        0,           0, laser210, vtech1, NULL, laser210, "Video Technology",       "Laser 210"                     , 0)
COMP( 1984, vz200,    laser210,           0, laser210, vtech1, NULL, laser210, "Dick Smith Electronics", "VZ-200 (Oceania)"              , 0)
COMP( 1984, las210de, laser210,           0, laser210, vtech1, NULL, laser210, "Sanyo",                  "Laser 210 (Germany)"           , 0)

COMPB(1984, laser310,        0, laser310, 0, laser310, vtech1, NULL, laser310, "Video Technology",       "Laser 310"                     , 0)
COMPB(1984, vz300,    laser310, laser310, 0, laser310, vtech1, NULL, laser310, "Dick Smith Electronics", "VZ-300 (Oceania)"              , 0)
