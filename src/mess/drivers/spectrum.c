/***************************************************************************

	NOTE: ****** Specbusy: press N, R, or E to boot *************


        Spectrum/Inves/TK90X etc. memory map:

    CPU:
        0000-3fff ROM
        4000-ffff RAM

        Spectrum 128/+2/+2a/+3 memory map:

        CPU:
                0000-3fff Banked ROM/RAM (banked rom only on 128/+2)
                4000-7fff Banked RAM
                8000-bfff Banked RAM
                c000-ffff Banked RAM

        TS2068 memory map: (Can't have both EXROM and DOCK active)
        The 8K EXROM can be loaded into multiple pages.

    CPU:
                0000-1fff     ROM / EXROM / DOCK (Cartridge)
                2000-3fff     ROM / EXROM / DOCK
                4000-5fff \
                6000-7fff  \
                8000-9fff  |- RAM / EXROM / DOCK
                a000-bfff  |
                c000-dfff  /
                e000-ffff /


Interrupts:

Changes:

29/1/2000   KT -    Implemented initial +3 emulation.
30/1/2000   KT -    Improved input port decoding for reading and therefore
            correct keyboard handling for Spectrum and +3.
31/1/2000   KT -    Implemented buzzer sound for Spectrum and +3.
            Implementation copied from Paul Daniel's Jupiter driver.
            Fixed screen display problems with dirty chars.
            Added support to load .Z80 snapshots. 48k support so far.
13/2/2000   KT -    Added Interface II, Kempston, Fuller and Mikrogen
            joystick support.
17/2/2000   DJR -   Added full key descriptions and Spectrum+ keys.
            Fixed Spectrum +3 keyboard problems.
17/2/2000   KT -    Added tape loading from WAV/Changed from DAC to generic
            speaker code.
18/2/2000   KT -    Added tape saving to WAV.
27/2/2000   KT -    Took DJR's changes and added my changes.
27/2/2000   KT -    Added disk image support to Spectrum +3 driver.
27/2/2000   KT -    Added joystick I/O code to the Spectrum +3 I/O handler.
14/3/2000   DJR -   Tape handling dipswitch.
26/3/2000   DJR -   Snapshot files are now classifed as snapshots not
            cartridges.
04/4/2000   DJR -   Spectrum 128 / +2 Support.
13/4/2000   DJR -   +4 Support (unofficial 48K hack).
13/4/2000   DJR -   +2a Support (rom also used in +3 models).
13/4/2000   DJR -   TK90X, TK95 and Inves support (48K clones).
21/4/2000   DJR -   TS2068 and TC2048 support (TC2048 Supports extra video
            modes but doesn't have bank switching or sound chip).
09/5/2000   DJR -   Spectrum +2 (France, Spain), +3 (Spain).
17/5/2000   DJR -   Dipswitch to enable/disable disk drives on +3 and clones.
27/6/2000   DJR -   Changed 128K/+3 port decoding (sound now works in Zub 128K).
06/8/2000   DJR -   Fixed +3 Floppy support
10/2/2001   KT  -   Re-arranged code and split into each model emulated.
            Code is split into 48k, 128k, +3, tc2048 and ts2048
            segments. 128k uses some of the functions in 48k, +3
            uses some functions in 128, and tc2048/ts2048 use some
            of the functions in 48k. The code has been arranged so
            these functions come in some kind of "override" order,
            read functions changed to use  READ8_HANDLER and write
            functions changed to use WRITE8_HANDLER.
            Added Scorpion256 preliminary.
18/6/2001   DJR -   Added support for Interface 2 cartridges.
xx/xx/2001  KS -    TS-2068 sound fixed.
            Added support for DOCK cartridges for TS-2068.
            Added Spectrum 48k Psycho modified rom driver.
            Added UK-2086 driver.
23/12/2001  KS -    48k machines are now able to run code in screen memory.
                Programs which keep their code in screen memory
                like monitors, tape copiers, decrunchers, etc.
                works now.
                Fixed problem with interrupt vector set to 0xffff (much
            more 128k games works now).
                A useful used trick on the Spectrum is to set
                interrupt vector to 0xffff (using the table
                which contain 0xff's) and put a byte 0x18 hex,
                the opcode for JR, at this address. The first
                byte of the ROM is a 0xf3 (DI), so the JR will
                jump to 0xfff4, where a long JP to the actual
                interrupt routine is put. Due to unideal
                bankswitching in MAME this JP were to 0001 what
                causes Spectrum to reset. Fixing this problem
                made much more software runing (i.e. Paperboy).
            Corrected frames per second value for 48k and 128k
            Sincalir machines.
                There are 50.08 frames per second for Spectrum
                48k what gives 69888 cycles for each frame and
                50.021 for Spectrum 128/+2/+2A/+3 what gives
                70908 cycles for each frame.
            Remaped some Spectrum+ keys.
                Presing F3 to reset was seting 0xf7 on keyboard
                input port. Problem occured for snapshots of
                some programms where it was readed as pressing
                key 4 (which is exit in Tapecopy by R. Dannhoefer
                for example).
            Added support to load .SP snapshots.
            Added .BLK tape images support.
                .BLK files are identical to .TAP ones, extension
                is an only difference.
08/03/2002  KS -    #FF port emulation added.
                Arkanoid works now, but is not playable due to
                completly messed timings.

Initialisation values used when determining which model is being emulated:
 48K        Spectrum doesn't use either port.
 128K/+2    Bank switches with port 7ffd only.
 +3/+2a     Bank switches with both ports.

Notes:
 1. No contented memory.
 2. No hi-res colour effects (need contended memory first for accurate timing).
 3. Multiface 1 and Interface 1 not supported.
 4. Horace and the Spiders cartridge doesn't run properly.
 5. Tape images not supported:
    .TZX, .SPC, .ITM, .PAN, .TAP(Warajevo), .VOC, .ZXS.
 6. Snapshot images not supported:
    .ACH, .PRG, .RAW, .SEM, .SIT, .SNX, .ZX, .ZXS, .ZX82.
 7. 128K emulation is not perfect - the 128K machines crash and hang while
    running quite a lot of games.
 8. Disk errors occur on some +3 games.
 9. Video hardware of all machines is timed incorrectly.
10. EXROM and HOME cartridges are not emulated.
11. The TK90X and TK95 roms output 0 to port #df on start up.
12. The purpose of this port is unknown (probably display mode as TS2068) and
    thus is not emulated.

Very detailed infos about the ZX Spectrum +3e can be found at

http://www.z88forever.org.uk/zxplus3e/

*******************************************************************************/

#include "driver.h"
#include "includes/spectrum.h"
#include "eventlst.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "sound/speaker.h"
#include "formats/tzx_cas.h"



/****************************************************************************************************/
/* Spectrum 48k functions */

/*
 bit 7-5: not used
 bit 4: Ear output/Speaker
 bit 3: MIC/Tape Output
 bit 2-0: border colour
*/

int PreviousFE = 0;

WRITE8_HANDLER(spectrum_port_fe_w)
{
	unsigned char Changed;

	Changed = PreviousFE^data;

	/* border colour changed? */
	if ((Changed & 0x07)!=0)
	{
		/* yes - send event */
		EventList_AddItemOffset(machine, 0x0fe, data & 0x07, ATTOTIME_TO_CYCLES(0,attotime_mul(video_screen_get_scan_period(machine->primary_screen), video_screen_get_vpos(machine->primary_screen))));
	}

	if ((Changed & (1<<4))!=0)
	{
		/* DAC output state */
		speaker_level_w(0,(data>>4) & 0x01);
	}

	if ((Changed & (1<<3))!=0)
	{
		/* write cassette data */
		cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), (data & (1<<3)) ? -1.0 : +1.0);
	}

	PreviousFE = data;
}

static ADDRESS_MAP_START (spectrum_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x57ff) AM_RAM AM_BASE(&spectrum_characterram )
	AM_RANGE(0x5800, 0x5aff) AM_RAM AM_BASE(&spectrum_colorram )
	AM_RANGE(0x5b00, 0xffff) AM_RAM
ADDRESS_MAP_END

/* KT: more accurate keyboard reading */
/* DJR: Spectrum+ keys added */
READ8_HANDLER(spectrum_port_fe_r)
{
   int lines = offset>>8;
   int data = 0xff;

   int cs_extra1 = input_port_read(machine, "PLUS0")  & 0x1f;
   int cs_extra2 = input_port_read(machine, "PLUS1")  & 0x1f;
   int cs_extra3 = input_port_read(machine, "PLUS2") & 0x1f;
   int ss_extra1 = input_port_read(machine, "PLUS3") & 0x1f;
   int ss_extra2 = input_port_read(machine, "PLUS4") & 0x1f;

   /* Caps - V */
   if ((lines & 1)==0)
   {
		data &= input_port_read(machine, "LINE0");
		/* CAPS for extra keys */
		if (cs_extra1 != 0x1f || cs_extra2 != 0x1f || cs_extra3 != 0x1f)
			data &= ~0x01;
   }

   /* A - G */
   if ((lines & 2)==0)
		data &= input_port_read(machine, "LINE1");

   /* Q - T */
   if ((lines & 4)==0)
		data &= input_port_read(machine, "LINE2");

   /* 1 - 5 */
   if ((lines & 8)==0)
		data &= input_port_read(machine, "LINE3") & cs_extra1;

   /* 6 - 0 */
   if ((lines & 16)==0)
		data &= input_port_read(machine, "LINE4") & cs_extra2;

   /* Y - P */
   if ((lines & 32)==0)
		data &= input_port_read(machine, "LINE5") & ss_extra1;

   /* H - Enter */
   if ((lines & 64)==0)
		data &= input_port_read(machine, "LINE6");

	/* B - Space */
	if ((lines & 128)==0)
	{
		data &= input_port_read(machine, "LINE7") & cs_extra3 & ss_extra2;
		/* SYMBOL SHIFT for extra keys */
		if (ss_extra1 != 0x1f || ss_extra2 != 0x1f)
			data &= ~0x02;
	}

	data |= (0xe0); /* Set bits 5-7 - as reset above */

	/* cassette input from wav */
	if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.0038 )
	{
		data &= ~0x40;
	}

	/* Issue 2 Spectrums default to having bits 5, 6 & 7 set.
    Issue 3 Spectrums default to having bits 5 & 7 set and bit 6 reset. */
	if (input_port_read(machine, "CONFIG") & 0x80)
		data ^= (0x40);
	return data;
}

/* kempston joystick interface */
READ8_HANDLER(spectrum_port_1f_r)
{
	return input_port_read(machine, "KEMPSTON") & 0x1f;
}

/* fuller joystick interface */
READ8_HANDLER(spectrum_port_7f_r)
{
	return input_port_read(machine, "FULLER") | (0xff^0x8f);
}

/* mikrogen joystick interface */
READ8_HANDLER(spectrum_port_df_r)
{
	return input_port_read(machine, "MIKROGEN") | (0xff^0x1f);
}

static  READ8_HANDLER ( spectrum_port_ula_r )
{
	return video_screen_get_vpos(machine->primary_screen)<193 ? spectrum_colorram[(video_screen_get_vpos(machine->primary_screen)&0xf8)<<2]:0xff;
}

/* ports are not decoded full.
The function decodes the ports appropriately */
static ADDRESS_MAP_START (spectrum_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x00, 0x00) AM_READWRITE(spectrum_port_fe_r,spectrum_port_fe_w) AM_MIRROR(0xfffe) AM_MASK(0xffff) 
	AM_RANGE(0x1f, 0x1f) AM_READ(spectrum_port_1f_r) AM_MIRROR(0xff00)
	AM_RANGE(0x7f, 0x7f) AM_READ(spectrum_port_7f_r) AM_MIRROR(0xff00)
	AM_RANGE(0xdf, 0xdf) AM_READ(spectrum_port_df_r) AM_MIRROR(0xff00)
	AM_RANGE(0x01, 0x01) AM_READ(spectrum_port_ula_r) AM_MIRROR(0xfffe)
ADDRESS_MAP_END

/****************************************************************************************************/
static GFXDECODE_START( spectrum )
	GFXDECODE_ENTRY( 0, 0x0, spectrum_charlayout, 0, 0x80 )
	GFXDECODE_ENTRY( 0, 0x0, spectrum_charlayout, 0, 0x80 )
	GFXDECODE_ENTRY( 0, 0x0, spectrum_charlayout, 0, 0x80 )
GFXDECODE_END

INPUT_PORTS_START( spectrum )
	PORT_START_TAG("LINE0") /* [0] 0xFEFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z  COPY    :      LN       BEEP") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X  CLEAR   Pound  EXP      INK") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C  CONT    ?      LPRINT   PAPER") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V  CLS     /      LLIST    FLASH") PORT_CODE(KEYCODE_V)

	PORT_START_TAG("LINE1") /* [1] 0xFDFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A  NEW     STOP   READ     ~") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S  SAVE    NOT    RESTORE  |") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D  DIM     STEP   DATA     \\") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F  FOR     TO     SGN      {") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G  GOTO    THEN   ABS      }") PORT_CODE(KEYCODE_G)

	PORT_START_TAG("LINE2") /* [2] 0xFBFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q  PLOT    <=     SIN      ASN") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W  DRAW    <>     COS      ACS") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E  REM     >=     TAN      ATN") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R  RUN     <      INT      VERIFY") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T  RAND    >      RND      MERGE") PORT_CODE(KEYCODE_T)

	/* interface II uses this port for joystick */
	PORT_START_TAG("LINE3") /* [3] 0xF7FE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1          !      BLUE     DEF FN") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2          @      RED      FN") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3          #      MAGENTA  LINE") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4          $      GREEN    OPEN#") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5          %      CYAN     CLOSE#") PORT_CODE(KEYCODE_5)

	/* protek clashes with interface II! uses 5 = left, 6 = down, 7 = up, 8 = right, 0 = fire */
	PORT_START_TAG("LINE4") /* [4] 0xEFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0          _      BLACK    FORMAT") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9          )               POINT") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8          (               CAT") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7          '      WHITE    ERASE") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6          &      YELLOW   MOVE") PORT_CODE(KEYCODE_6)

	PORT_START_TAG("LINE5") /* [5] 0xDFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P  PRINT   \"      TAB      (c)") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O  POKE    ;      PEEK     OUT") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I  INPUT   AT     CODE     IN") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U  IF      OR     CHR$     ]") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y  RETURN  AND    STR$     [") PORT_CODE(KEYCODE_Y)

	PORT_START_TAG("LINE6") /* [6] 0xBFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L  LET     =      USR      ATTR") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K  LIST    +      LEN      SCREEN$") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J  LOAD    -      VAL      VAL$") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H  GOSUB   ^      SQR      CIRCLE") PORT_CODE(KEYCODE_H)

	PORT_START_TAG("LINE7") /* [7] 0x7FFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SYMBOL SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M  PAUSE   .      PI       INVERSE") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N  NEXT    ,      INKEY$   OVER") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B  BORDER  *      BIN      BRIGHT") PORT_CODE(KEYCODE_B)

	PORT_START_TAG("PLUS0") /* [8] Spectrum+ Keys (set CAPS + 1-5) */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EDIT          (CAPS + 1)") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS LOCK     (CAPS + 2)") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TRUE VID      (CAPS + 3)") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INV VID       (CAPS + 4)") PORT_CODE(KEYCODE_END)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor left   (CAPS + 5)") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("PLUS1") /* [9] Spectrum+ Keys (set CAPS + 6-0) */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL           (CAPS + 0)") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GRAPH         (CAPS + 9)") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor right  (CAPS + 8)") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor up     (CAPS + 7)") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor down   (CAPS + 6)") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("PLUS2") /* [10] Spectrum+ Keys (set CAPS + SPACE and CAPS + SYMBOL */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BREAK") PORT_CODE(KEYCODE_PAUSE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXT MODE") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("PLUS3") /* [11] Spectrum+ Keys (set SYMBOL SHIFT + O/P */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\"") PORT_CODE(KEYCODE_F4)
	/*        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\"", KEYCODE_QUOTE,  CODE_NONE ) */
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("PLUS4") /* [12] Spectrum+ Keys (set SYMBOL SHIFT + N/M */
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0xf3, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("KEMPSTON") /* [13] Kempston joystick interface */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK RIGHT") PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK LEFT") PORT_CODE(JOYCODE_X_LEFT_SWITCH)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK DOWN") PORT_CODE(JOYCODE_Y_DOWN_SWITCH)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK UP") PORT_CODE(JOYCODE_Y_UP_SWITCH)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK FIRE") PORT_CODE(JOYCODE_BUTTON1)

	PORT_START_TAG("FULLER") /* [14] Fuller joystick interface */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK UP") PORT_CODE(JOYCODE_Y_UP_SWITCH)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK DOWN") PORT_CODE(JOYCODE_Y_DOWN_SWITCH)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK LEFT") PORT_CODE(JOYCODE_X_LEFT_SWITCH)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK RIGHT") PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK FIRE") PORT_CODE(JOYCODE_BUTTON1)

	PORT_START_TAG("MIKROGEN") /* [15] Mikrogen joystick interface */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK UP") PORT_CODE(JOYCODE_Y_UP_SWITCH)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK DOWN") PORT_CODE(JOYCODE_Y_DOWN_SWITCH)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK RIGHT") PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK LEFT") PORT_CODE(JOYCODE_X_LEFT_SWITCH)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK FIRE") PORT_CODE(JOYCODE_BUTTON1)

	PORT_START_TAG("NMI") 
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NMI") PORT_CODE(KEYCODE_F12)

	PORT_START_TAG("CONFIG") /* [16] */
	PORT_DIPNAME(0x80, 0x00, "Hardware Version")
	PORT_DIPSETTING(0x00, "Issue 2" )
	PORT_DIPSETTING(0x80, "Issue 3" )
	PORT_DIPNAME(0x40, 0x00, "End of .TAP action")
	PORT_DIPSETTING(0x00, "Disable .TAP support" )
	PORT_DIPSETTING(0x40, "Rewind tape to start (to reload earlier levels)" )
	PORT_DIPNAME(0x20, 0x00, "+3/+2a etc. Disk Drive")
	PORT_DIPSETTING(0x00, "Enabled" )
	PORT_DIPSETTING(0x20, "Disabled" )
	PORT_BIT(0x1f, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


static INTERRUPT_GEN( spec_interrupt )
{
	cpunum_set_input_line(machine, 0, 0, HOLD_LINE);
}

MACHINE_DRIVER_START( spectrum )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3500000)        /* 3.5 Mhz */
	MDRV_CPU_PROGRAM_MAP(spectrum_mem, 0)
	MDRV_CPU_IO_MAP(spectrum_io, 0)
	MDRV_CPU_VBLANK_INT("main", spec_interrupt)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( spectrum )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50.08)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(SPEC_SCREEN_WIDTH, SPEC_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1)
	MDRV_GFXDECODE( spectrum )
	MDRV_PALETTE_LENGTH(256 + 16)
	MDRV_PALETTE_INIT( spectrum )

	MDRV_VIDEO_START( spectrum )
	MDRV_VIDEO_UPDATE( spectrum )
	MDRV_VIDEO_EOF( spectrum )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("wave", WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD_TAG("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* devices */
	MDRV_SNAPSHOT_ADD(spectrum, "sna,z80,sp", 0)
	MDRV_QUICKLOAD_ADD(spectrum, "scr", 0)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(spectrum)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_SYSTEM_BIOS(0, "spectrum", "ZX Spectrum")
	ROMX_LOAD("spectrum.rom", 0x0000, 0x4000, CRC(ddee531f) SHA1(5ea7c2b824672e914525d1d5c419d71b84a426a2), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "specbusy", "BusySoft Upgrade v1.18")
	ROMX_LOAD("48-busy.rom", 0x0000, 0x4000, CRC(1511cddb) SHA1(ab3c36daad4325c1d3b907b6dc9a14af483d14ec), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "specpsch", "Maly's Psycho Upgrade")
	ROMX_LOAD("48-psych.rom", 0x0000, 0x4000, CRC(cd60b589) SHA1(0853e25857d51dd41b20a6dbc8e80f028c5befaa), ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "specgrot", "De Groot's Upgrade")
	ROMX_LOAD("48-groot.rom", 0x0000, 0x4000, CRC(abf18c45) SHA1(51165cde68e218512d3145467074bc7e786bf307), ROM_BIOS(4))
	ROM_SYSTEM_BIOS(4, "specimc", "Collier's Upgrade")
	ROMX_LOAD("48-imc.rom", 0x0000, 0x4000, CRC(d1be99ee) SHA1(dee814271c4d51de257d88128acdb324fb1d1d0d), ROM_BIOS(5))
	ROM_SYSTEM_BIOS(5, "speclec", "LEC Upgrade")
	ROMX_LOAD("80-lec.rom", 0x0000, 0x4000, CRC(5b5c92b1) SHA1(bb7a77d66e95d2e28ebb610e543c065e0d428619), ROM_BIOS(6))
	ROM_SYSTEM_BIOS(6, "specpls4", "ZX Spectrum +4")
	ROMX_LOAD("plus4.rom",0x0000,0x4000, CRC(7e0f47cb) SHA1(c103e89ef58e6ade0c01cea0247b332623bd9a30), ROM_BIOS(7))
	ROM_CART_LOAD(0, "rom", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(tk90x)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("tk90x.rom",0x0000,0x4000, CRC(3e785f6f) SHA1(9a943a008be13194fb006bddffa7d22d2277813f))
	ROM_CART_LOAD(0, "rom", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(tk95)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("tk95.rom",0x0000,0x4000, CRC(17368e07) SHA1(94edc401d43b0e9a9cdc1d35de4b6462dc414ab3))
	ROM_CART_LOAD(0, "rom", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(inves)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("inves.rom",0x0000,0x4000, CRC(8ff7a4d1) SHA1(d020440638aff4d39467128413ef795677be9c23))
	ROM_CART_LOAD(0, "rom", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

static void spectrum_common_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:				info->i = 1; break;
		case MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE:	info->i = CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_ENABLED; break;
		case MESS_DEVINFO_PTR_CASSETTE_FORMATS:		info->p = (void *)tzx_cassette_formats; break;
		default:					cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(spectrum_common)
	CONFIG_DEVICE(spectrum_common_cassette_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(spectrum)
	CONFIG_IMPORT_FROM(spectrum_common)
	CONFIG_DEVICE(cartslot_device_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT       INIT    CONFIG      COMPANY     FULLNAME */
COMP( 1982, spectrum, 0,        0,		spectrum,		spectrum,	0,		spectrum,	"Sinclair Research",	"ZX Spectrum" , 0)
COMP( 1986, inves,    spectrum, 0,		spectrum,		spectrum,	0,		spectrum,	"Investronica",	"Inves Spectrum 48K+" , 0)
COMP( 1985, tk90x,    spectrum, 0,		spectrum,		spectrum,	0,		spectrum,	"Micro Digital",	"TK-90x Color Computer" , 0)
COMP( 1986, tk95,     spectrum, 0,		spectrum,		spectrum,	0,		spectrum,	"Micro Digital",	"TK-95 Color Computer" , 0)
