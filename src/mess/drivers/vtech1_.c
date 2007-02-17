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
        4000-5FFF 8K DOS ROM (optional)
        6000-67FF 2K reserved for rom cartridges
        6800-6FFF 2K memory mapped I/O
                    R: keyboard
                    W: cassette I/O, speaker, VDP control
        7000-77FF 2K video RAM
        7800-7FFF 2K internal user RAM
        8000-87FF 2K ?
        8800-C7FF 16K memory expansion
        C800-FFFF 64K memory expansion
        
    Laser 210
        0000-1FFF 8K ROM 0
        2000-3FFF 8K ROM 1
        4000-5FFF 8K DOS ROM (optional)
        6000-67FF 2K reserved for rom cartridges
        6800-6FFF 2K memory mapped I/O
                    R: keyboard
                    W: cassette I/O, speaker, VDP control
        7000-77FF 2K video RAM
        7800-8FFF 6K internal user RAM (3x2K: U2, U3, U4)
        9000-CFFF 16K memory expansion
        D000-FFFF 64K memory expansion
    
    Laser 310
        0000-3FFF 16K ROM
        4000-5FFF 8K DOS ROM (optional)
        6000-67FF 2K reserved for rom cartridges
        6800-6FFF 2K memory mapped I/O
                    R: keyboard
                    W: cassette I/O, speaker, VDP control
        7000-77FF 2K video RAM
        7800-B7FF 16K internal user RAM
        B800-F7FF 16K memory expansion
        F800-FFFF 64K memory expansion

Todo:

    - Figure out which machines were shipped with which ROM version
      (currently only a guess)
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

static ADDRESS_MAP_START(laser110_mem, ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE(0x0000, 0x3fff) AM_ROM	/* basic rom */
    AM_RANGE(0x4000, 0x5fff) AM_ROM	/* dos rom or other catridges */
    AM_RANGE(0x6000, 0x67ff) AM_ROM	/* reserved for cartridges */
    AM_RANGE(0x6800, 0x6fff) AM_READWRITE(vtech1_keyboard_r, vtech1_latch_w)
    AM_RANGE(0x7000, 0x77ff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size) /* (6847) */
    AM_RANGE(0x7800, 0x7fff) AM_RAM /* 2KB user ram */
    AM_RANGE(0x8000, 0x87ff) AM_NOP
//  AM_RANGE(0x8800, 0xc7ff) AM_RAM	/* 16KB/64KB memory expansion */
//  AM_RANGE(0xc800, 0xffff) AM_NOP	/* dynamically mapped */
ADDRESS_MAP_END

static ADDRESS_MAP_START(laser210_mem, ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE(0x0000, 0x3fff) AM_ROM	/* basic rom */
    AM_RANGE(0x4000, 0x5fff) AM_ROM	/* dos rom or other catridges */
    AM_RANGE(0x6000, 0x67ff) AM_ROM	/* reserved for cartridges */
    AM_RANGE(0x6800, 0x6fff) AM_READWRITE(vtech1_keyboard_r, vtech1_latch_w)
    AM_RANGE(0x7000, 0x77ff) AM_READWRITE(videoram_r, videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size) /* U7 (6847) */
    AM_RANGE(0x7800, 0x8fff) AM_RAM	/* 6KB user ram */
//  AM_RANGE(0x9000, 0xcfff) AM_RAM	/* 16KB/64KB memory expansion */
//  AM_RANGE(0xd000, 0xffff) AM_NOP	/* dynamically mapped */
ADDRESS_MAP_END

static ADDRESS_MAP_START(laser310_mem, ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE(0x0000, 0x3fff) AM_ROM	/* basic rom */
    AM_RANGE(0x4000, 0x5fff) AM_ROM	/* dos rom or other catridges */
    AM_RANGE(0x6000, 0x67ff) AM_ROM	/* reserved for cartridges */
    AM_RANGE(0x6800, 0x6fff) AM_READWRITE(vtech1_keyboard_r, vtech1_latch_w)
    AM_RANGE(0x7000, 0x77ff) AM_READWRITE(videoram_r, videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size) /* (6847) */
    AM_RANGE(0x7800, 0xb7ff) AM_RAM	/* 16KB user ram */
//  AM_RANGE(0xb800, 0xf7ff) AM_RAM	/* 16KB/64KB memory expansion */
//  AM_RANGE(0xf800, 0xffff) AM_NOP	/* dynamically mapped */
ADDRESS_MAP_END

static ADDRESS_MAP_START(vtech1_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x0f) AM_READWRITE(vtech1_printer_r, vtech1_printer_w    )
	AM_RANGE(0x10, 0x1f) AM_READWRITE(vtech1_fdc_r,     vtech1_fdc_w        )
	AM_RANGE(0x20, 0x2f) AM_READ(     vtech1_joystick_r                     )
	AM_RANGE(0x30, 0x3f) AM_READWRITE(vtech1_serial_r,  vtech1_serial_w     )
	AM_RANGE(0x40, 0x4f) AM_READ(     vtech1_lightpen_r                     )
	AM_RANGE(0x50, 0x5f) AM_NOP /* Real time clock (proposed) */
	AM_RANGE(0x60, 0x6f) AM_NOP /* External keyboard */
	AM_RANGE(0x70, 0x7f) AM_WRITE(                      vtech1_memory_bank_w)
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
	PORT_START /* IN0 */
	PORT_CONFNAME(0x10, 0x10, "Autorun on Quickload")
	PORT_CONFSETTING(   0x00, DEF_STR(No))
	PORT_CONFSETTING(   0x10, DEF_STR(Yes))
	
	PORT_START /* IN1 KEY ROW 0 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R       RETURN  LEFT$") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q       FOR     CHR$") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E       NEXT    LEN(") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W       TO      VAL(") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T       THEN    MID$") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	
	PORT_START /* IN2 KEY ROW 1 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F       GOSUB   RND(") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A       MODE(   ASC(") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D       DIM     RESTORE") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L-CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R-CTRL") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S       STEP    STR$(") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G       GOTO    STOP") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	
	PORT_START /* IN3 KEY ROW 2 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V       LPRINT  USR") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z       PEEK(   INP") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C       CONT    COPY") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L-SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R-SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X       POKE    OUT") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B       LLIST   SOUND") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	
	PORT_START /* IN4 KEY ROW 3 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4  $    VERIFY  ATN(") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1  !    CSAVE   SIN(") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3  #    CRUN    TAN(") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2  \"    CLOAD   COS(") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5  %    LIST    LOG(") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	
	PORT_START /* IN5 KEY ROW 4 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M  \\    [Left]") PORT_CODE(KEYCODE_M) PORT_CHAR('M') PORT_CHAR('\\') PORT_CHAR(8)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE   [Down]") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ') PORT_CHAR(10)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",  <    [Right]") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<') PORT_CHAR(9)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".  >    [Up]") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>') PORT_CHAR(27)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N  ^    COLOR   USING") PORT_CODE(KEYCODE_N) PORT_CHAR('N') PORT_CHAR('^')
	
	PORT_START /* IN6 KEY ROW 5 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7  '    END     SGN(") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0  @    DATA    INT(") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('@')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8  (    NEW     SQR(") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-  =    [Break]") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=') PORT_CHAR(3)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9  )    READ    ABS(") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6  &    RUN     EXP(") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	
	PORT_START /* IN7 KEY ROW 6 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U       IF      INKEY$") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P  ]    PRINT   NOT") PORT_CODE(KEYCODE_P) PORT_CHAR('P') PORT_CHAR(']')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I       INPUT   AND") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN  [Function]") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O  [    LET     OR") PORT_CODE(KEYCODE_O) PORT_CHAR('O') PORT_CHAR('[')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y       ELSE    RIGHT$(") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	
	PORT_START /* IN8 KEY ROW 7 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J       REM     RESET") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";  +    [Rubout]") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+') PORT_CHAR('_')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K  /    TAB(    POINT") PORT_CODE(KEYCODE_K) PORT_CHAR('K') PORT_CHAR('/')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":  *    [Inverse]") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L  ?    [Insert]") PORT_CODE(KEYCODE_L) PORT_CHAR('L') PORT_CHAR('?') PORT_CHAR(21)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H       CLS     SET") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	
	PORT_START /* IN9 EXTRA KEYS */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Inverse)") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Rubout)") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Cursor left)") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Cursor down)") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Cursor right)") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Backspace)") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Cursor up)") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Insert)") PORT_CODE(KEYCODE_INSERT)
	
	PORT_START /* IN10 JOYSTICK #1 */
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x10, IP_ACTIVE_LOW, 0| IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, 0| IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, 0| IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, 0| IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(0x01, IP_ACTIVE_LOW, 0| IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	
	PORT_START /* IN11 JOYSTICK #1 'Arm' */
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x10, IP_ACTIVE_LOW, 0| IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	
	PORT_START /* IN12 JOYSTICK #2 */
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x10, IP_ACTIVE_LOW, 0| IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, 0| IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, 0| IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, 0| IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(0x01, IP_ACTIVE_LOW, 0| IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	
	PORT_START /* IN13 JOYSTICK #2 'Arm' */
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x10, IP_ACTIVE_LOW, 0| IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/******************************************************************************
 Palette Initialisation
******************************************************************************/

/* note - Juergen's colors do not match the colors in the m6847 code */
static unsigned char vt_palette[] =
{
      0,   0,   0,    /* black (block graphics) */
      0, 224,   0,    /* green */
    208, 255,   0,    /* yellow (greenish) */
      0,   0, 255,    /* blue */
    255,   0,   0,    /* red */
    224, 224, 144,    /* buff */
      0, 255, 160,    /* cyan (greenish) */
    255,   0, 255,    /* magenta */
    240, 112,   0,    /* orange */
      0,  64,   0,    /* dark green (alphanumeric characters) */
      0, 224,  24,    /* bright green (alphanumeric characters) */
     64,  16,   0,    /* dark orange (alphanumeric characters) */
    255, 196,  24,    /* bright orange (alphanumeric characters) */
};

/* monochrome */
static PALETTE_INIT(monochrome)
{
    int i;
    for (i = 0; i < sizeof(vt_palette)/(sizeof(unsigned char)*3); i++) {
        int mono;
        mono = (int)(vt_palette[i*3+0] * 0.299 + vt_palette[i*3+1] * 0.587 + vt_palette[i*3+2] * 0.114);
        palette_set_color(i, mono, mono, mono);
    }
}

/* color */
static PALETTE_INIT(color)
{
    palette_set_colors(0, vt_palette, sizeof(vt_palette) / (sizeof(vt_palette[0]) * 3));
}


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
    MDRV_CPU_ADD_TAG("main", Z80, 3579500)        /* 3.57950 Mhz */
    MDRV_CPU_PROGRAM_MAP(laser110_mem, 0)
    MDRV_CPU_IO_MAP(vtech1_io, 0)
    MDRV_CPU_VBLANK_INT(vtech1_interrupt,1)
	MDRV_SCREEN_REFRESH_RATE(M6847_PAL_FRAMES_PER_SECOND)
    MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START(laser110)

    /* video hardware */
	MDRV_VIDEO_START(vtech1)
	MDRV_VIDEO_UPDATE(m6847)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_MAXSIZE(320, 25+192+26)
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

    MDRV_PALETTE_INIT(color)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(laser210)
    MDRV_IMPORT_FROM(laser110)
    MDRV_CPU_MODIFY("main")
    MDRV_CPU_PROGRAM_MAP(laser210_mem, 0)

    MDRV_MACHINE_START(laser210)
    MDRV_PALETTE_INIT(color)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(laser310)
    MDRV_IMPORT_FROM( laser110 )
    MDRV_CPU_REPLACE( "main", Z80, LASER310_MAIN_OSCILLATOR/5)  /* 3.54690 Mhz */
    MDRV_CPU_PROGRAM_MAP(laser310_mem, 0)

    MDRV_MACHINE_START(laser310)
    MDRV_PALETTE_INIT(color)
MACHINE_DRIVER_END


/******************************************************************************
 ROM Definitions
******************************************************************************/

ROM_START(laser110)
    ROM_REGION(0x6800, REGION_CPU1, 0)
    ROM_LOAD("vtechv12.lo", 0x0000, 0x2000, CRC(99412d43) SHA1(6aed8872a0818be8e1b08ecdfd92acbe57a3c96d))
    ROM_LOAD("vtechv12.hi", 0x2000, 0x2000, CRC(e4c24e8b) SHA1(9d8fb3d24f3d4175b485cf081a2d5b98158ab2fb))
    ROM_CART_LOAD(0, "rom\0", 0x04000, 0x27ff, ROM_NOMIRROR | ROM_OPTIONAL )
ROM_END

#define rom_las110de    rom_laser110
#define rom_laser200    rom_laser110
#define rom_vz200de     rom_laser110
#define rom_fellow      rom_laser110
#define rom_tx8000      rom_laser110

ROM_START(laser210)
    ROM_REGION(0x6800, REGION_CPU1, 0)
    ROM_LOAD("vtechv20.lo", 0x0000, 0x2000, CRC(cc854fe9) SHA1(6e66a309b8e6dc4f5b0b44e1ba5f680467353d66))
    ROM_LOAD("vtechv20.hi", 0x2000, 0x2000, CRC(7060f91a) SHA1(8f3c8f24f97ebb98f3c88d4e4ba1f91ffd563440))
    ROM_CART_LOAD(0, "rom\0", 0x04000, 0x27ff, ROM_NOMIRROR | ROM_OPTIONAL )
ROM_END

#define rom_las210de    rom_laser210
#define rom_vz200       rom_laser210

ROM_START(laser310)
    ROM_REGION(0x6800, REGION_CPU1, 0)
    ROM_LOAD("vtechv20.rom", 0x0000, 0x4000, CRC(613de12c) SHA1(f216c266bc09b0dbdbad720796e5ea9bc7d91e53))
    ROM_CART_LOAD(0, "rom\0", 0x04000, 0x27ff, ROM_NOMIRROR | ROM_OPTIONAL )
ROM_END

#define rom_vz300       rom_laser310

ROM_START(las31021)
    ROM_REGION(0x6800, REGION_CPU1, 0)
    ROM_LOAD("vtechv21.rom", 0x0000, 0x4000, CRC(f7df980f) SHA1(5ba14a7a2eedca331b033901080fa5d205e245ea))
    ROM_CART_LOAD(0, "rom\0", 0x04000, 0x27ff, ROM_NOMIRROR | ROM_OPTIONAL )
ROM_END

#define rom_vz300_21    rom_laser310


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

/* There were two memory expansions available for the Laser/VZ computers:

   - a 16kb expansion without banking
   - a 64kb expansion where the first bank is fixed and the other 3 are
     banked in as needed

   Both are externally connected devices and were not fully compatible
   between Laser 110/210/310 computers, though they could be relativly
   easily modified. */

SYSTEM_CONFIG_START(vtech1)
    CONFIG_DEVICE(vtech1_printer_getinfo)
    CONFIG_DEVICE(cartslot_device_getinfo)
    CONFIG_DEVICE(vtech1_cassette_getinfo)
    CONFIG_DEVICE(vtech1_snapshot_getinfo)
    CONFIG_DEVICE(vtech1_floppy_getinfo)
	CONFIG_DEVICE(z80bin_quickload_getinfo)
	CONFIG_RAM_DEFAULT (66 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(laser110)
	CONFIG_IMPORT_FROM (vtech1)
	CONFIG_RAM         ( 2 * 1024)
	CONFIG_RAM         (18 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(laser210)
	CONFIG_IMPORT_FROM (vtech1)
	CONFIG_RAM         ( 6 * 1024)
	CONFIG_RAM         (22 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(laser310)
	CONFIG_IMPORT_FROM (vtech1)
	CONFIG_RAM         (16 * 1024)
	CONFIG_RAM         (32 * 1024)
SYSTEM_CONFIG_END


/******************************************************************************
 Drivers
******************************************************************************/

/*   YEAR  NAME        PARENT    COMPAT  MACHINE   INPUT   INIT  CONFIG    COMPANY                   FULLNAME */
COMP(1983, laser110,        0,        0, laser110, vtech1, NULL, laser110, "Video Technology",       "Laser 110"                     , 0)
COMP(1983, las110de, laser110,        0, laser110, vtech1, NULL, laser110, "Sanyo",                  "Laser 110 (Germany)"           , 0)

COMP(1983, laser200,        0,        0, laser200, vtech1, NULL, laser110, "Video Technology",       "Laser 200"                     , 0)
COMP(1983, vz200de,  laser200,        0, laser200, vtech1, NULL, laser110, "Video Technology",       "VZ-200 (Germany & Netherlands)", 0)
COMP(1983, fellow,   laser200,        0, laser200, vtech1, NULL, laser110, "Salora",                 "Fellow (Finland)"              , 0)
COMP(1983, tx8000,   laser200,        0, laser200, vtech1, NULL, laser110, "Texet",                  "TX-8000 (UK)"                  , 0)

COMP(1984, laser210,        0,        0, laser210, vtech1, NULL, laser210, "Video Technology",       "Laser 210"                     , 0)
COMP(1984, vz200,    laser210,        0, laser210, vtech1, NULL, laser210, "Dick Smith Electronics", "VZ-200 (Oceania)"              , 0)
COMP(1984, las210de, laser210,        0, laser210, vtech1, NULL, laser210, "Sanyo",                  "Laser 210 (Germany)"           , 0)

COMP(1984, laser310,        0,        0, laser310, vtech1, NULL, laser310, "Video Technology",       "Laser 310"                     , 0)
COMP(1984, las31021, laser310,        0, laser310, vtech1, NULL, laser310, "Video Technology",       "Laser 310 (BASIC V2.1)"        , 0)
COMP(1984, vz300,    laser310,        0, laser310, vtech1, NULL, laser310, "Dick Smith Electronics", "VZ-300 (Oceania)"              , 0)
COMP(1984, vz300_21, laser310,        0, laser310, vtech1, NULL, laser310, "Dick Smith Electronics", "VZ-300 (Oceania, BASIC V2.1)"  , 0)
