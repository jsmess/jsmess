/***************************************************************************
TRS80 memory map

0000-2fff ROM				  R   D0-D7
3000-37ff ROM on Model III		  R   D0-D7
		  unused on Model I
37de      UART status			  R/W D0-D7
37df      UART data			  R/W D0-D7
37e0      interrupt latch address (lnw80 = for the realtime clock)
37e1      select disk drive 0		  W
37e2      cassette drive latch address	  W
37e3      select disk drive 1		  W
37e4      select which cassette unit      W   D0-D1 (D0 selects unit 1, D1 selects unit 2)
37e5      select disk drive 2		  W
37e7      select disk drive 3		  W
37e0-37e3 floppy motor			  W   D0-D3
		  or floppy head select   W   D3
37e8      send a byte to printer          W   D0-D7
37e8      read printer status             R   D7
37ec-37ef FDC WD179x			  R/W D0-D7
37ec	  command			  W   D0-D7
37ec	  status			  R   D0-D7
37ed	  track 			  R/W D0-D7
37ee	  sector			  R/W D0-D7
37ef	  data				  R/W D0-D7
3800-38ff keyboard matrix		  R   D0-D7
3900-3bff unused - kbd mirrored
3c00-3fff video RAM 			  R/W D0-D5,D7 (or D0-D7)
4000-ffff RAM

Interrupts:
IRQ mode 1
NMI

I/O ports
FF:
- bits 0 and 1 are for writing a cassette
- bit 2 must be high to turn the cassette player on, enables cassette data paths on a system-80
- bit 3 switches the display between 64 or 32 characters per line
- bit 7 is for reading from a cassette
FE:
- bit 0 is for selecting inverse video of the whole screen on a lnw80
- bit 2 enables colour on a lnw80
- bit 3 is for selecting roms (low) or 16k hires area (high) on a lnw80
- bit 4 selects internal cassette player (low) or external unit (high) on a system-80
FD:
- bit 7 for reading the printer status on a system-80
- all bits for writing to a printer on a system-80 
F9:
- UART data (write) status (read) on a system-80
F8:
- UART data (read) status (write) on a system-80
EB:
- UART data (read and write) on a Model III/4
EA:
- UART status (read and write) on a Model III/4
E9:
- UART Configuration jumpers (read) on a Model III/4
E8:
- UART Modem Status register (read) on a Model III/4
- UART Master Reset (write) on a Model III/4
***************************************************************************

Not dumped:
 TRS80 Japanese bios
 TRS80 Katakana Character Generator
 TRS80 Model III and 4 Character Generators

Not emulated:
 TRS80 Japanese kana/ascii switch and alternate keyboard
 TRS80 Model III and 4 hardware above what is in a Model I.
 LNW80 1.77 / 4.0 mhz switch
 LNW80 Colour board
 LNW80 Hires graphics
 LNW80 24x80 screen
 LNW80 Character generator hasn't been decoded, that's why the screen is corrupt

***************************************************************************/

#include "includes/trs80.h"
#include "devices/basicdsk.h"
#include "sound/speaker.h"

#define FW	TRS80_FONT_W
#define FH	TRS80_FONT_H

READ8_HANDLER (trs80_wd179x_r)
{
	if (readinputport(0) & 0x80)
	{
		return wd179x_status_r(offset);
	}
	else
	{
		return 0xff;
	}
}

static ADDRESS_MAP_START( mem_level1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x3800, 0x38ff) AM_READ(trs80_keyboard_r)
	AM_RANGE(0x3c00, 0x3fff) AM_READWRITE(MRA8_RAM, trs80_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0x7fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_level1, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xfe, 0xfe) AM_READ(trs80_port_xx_r)
	AM_RANGE(0xff, 0xff) AM_READWRITE(trs80_port_ff_r, trs80_port_ff_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mem_model1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x3000, 0x37df) AM_NOP
	AM_RANGE(0x37e0, 0x37e3) AM_READWRITE(trs80_irq_status_r, trs80_motor_w)
	AM_RANGE(0x37e4, 0x37e7) AM_NOP
	AM_RANGE(0x37e8, 0x37eb) AM_READWRITE(trs80_printer_r, trs80_printer_w)
	AM_RANGE(0x37ec, 0x37ec) AM_READWRITE(trs80_wd179x_r, wd179x_command_w)
	AM_RANGE(0x37ed, 0x37ed) AM_READWRITE(wd179x_track_r, wd179x_track_w)
	AM_RANGE(0x37ee, 0x37ee) AM_READWRITE(wd179x_sector_r, wd179x_sector_w)
	AM_RANGE(0x37ef, 0x37ef) AM_READWRITE(wd179x_data_r, wd179x_data_w)
	AM_RANGE(0x37f0, 0x37ff) AM_NOP
	AM_RANGE(0x3800, 0x38ff) AM_READ(trs80_keyboard_r)
	AM_RANGE(0x3900, 0x3bff) AM_NOP
	AM_RANGE(0x3c00, 0x3fff) AM_READWRITE(MRA8_RAM, trs80_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_model1, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xfe, 0xfe) AM_READ(trs80_port_xx_r)
	AM_RANGE(0xff, 0xff) AM_READWRITE(trs80_port_ff_r, trs80_port_ff_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mem_model3, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x37ff) AM_ROM
	AM_RANGE(0x3800, 0x38ff) AM_READ(trs80_keyboard_r)
	AM_RANGE(0x3c00, 0x3fff) AM_READWRITE(MRA8_RAM, trs80_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_model3, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xe0, 0xe3) AM_READWRITE(trs80_irq_status_r, trs80_irq_mask_w)
	AM_RANGE(0xe4, 0xe4)	AM_WRITE(trs80_motor_w)
	AM_RANGE(0xf0, 0xf0) AM_READWRITE(trs80_wd179x_r, wd179x_command_w)
	AM_RANGE(0xf1, 0xf1) AM_READWRITE(wd179x_track_r, wd179x_track_w)
	AM_RANGE(0xf2, 0xf2) AM_READWRITE(wd179x_sector_r, wd179x_sector_w)
	AM_RANGE(0xf3, 0xf3) AM_READWRITE(wd179x_data_r, wd179x_data_w)
	AM_RANGE(0xff, 0xff) AM_READWRITE(trs80_port_ff_r, trs80_port_ff_w)
ADDRESS_MAP_END



/**************************************************************************
   w/o SHIFT							 with SHIFT
   +-------------------------------+	 +-------------------------------+
   | 0	 1	 2	 3	 4	 5	 6	 7 |	 | 0   1   2   3   4   5   6   7 |
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
|0 | @ | A | B | C | D | E | F | G |  |0 | ` | a | b | c | d | e | f | g |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|1 | H | I | J | K | L | M | N | O |  |1 | h | i | j | k | l | m | n | o |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|2 | P | Q | R | S | T | U | V | W |  |2 | p | q | r | s | t | u | v | w |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|3 | X | Y | Z | [ | \ | ] | ^ | _ |  |3 | x | y | z | { | | | } | ~ |	 |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |  |4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|5 | 8 | 9 | : | ; | , | - | . | / |  |5 | 8 | 9 | * | + | < | = | > | ? |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|  |6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|  |7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
NB: row 7 contains some originally unused bits
	only the shift bit was there in the TRS80
***************************************************************************/

INPUT_PORTS_START( trs80 )
	PORT_START /* IN0 */
	PORT_CONFNAME(	  0x80, 0x80,	"Floppy Disc Drives")
	PORT_CONFSETTING(	0x00, DEF_STR( Off ) )
	PORT_CONFSETTING(	0x80, DEF_STR( On ) )
	PORT_CONFNAME(	  0x40, 0x40,	"Video RAM") PORT_CODE(KEYCODE_F1)
	PORT_CONFSETTING(	0x40, "7 bit" )
	PORT_CONFSETTING(	0x00, "8 bit" )
	PORT_CONFNAME(	  0x20, 0x00,	"Virtual Tape") PORT_CODE(KEYCODE_F2)
	PORT_CONFSETTING(	0x00, DEF_STR( Off ) )
	PORT_CONFSETTING(	0x20, DEF_STR( On ) )
	PORT_BIT(	  0x08, 0x00, IPT_KEYBOARD) PORT_NAME("NMI") PORT_CODE(KEYCODE_F4)
	PORT_BIT(	  0x04, 0x00, IPT_KEYBOARD) PORT_NAME("Tape start") PORT_CODE(KEYCODE_F5)
	PORT_BIT(	  0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Tape stop") PORT_CODE(KEYCODE_F6)
	PORT_BIT(	  0x01, 0x00, IPT_KEYBOARD) PORT_NAME("Tape rewind") PORT_CODE(KEYCODE_F7)

	PORT_START /* KEY ROW 0 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("0.0: @") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("0.1: A  a") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("0.2: B  b") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("0.3: C  c") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("0.4: D  d") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("0.5: E  e") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("0.6: F  f") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("0.7: G  g") PORT_CODE(KEYCODE_G)

	PORT_START /* KEY ROW 1 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("1.0: H  h") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("1.1: I  i") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("1.2: J  j") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("1.3: K  k") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("1.4: L  l") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("1.5: M  m") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("1.6: N  n") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("1.7: O  o") PORT_CODE(KEYCODE_O)

	PORT_START /* KEY ROW 2 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("2.0: P  p") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("2.1: Q  q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("2.2: R  r") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("2.3: S  s") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("2.4: T  t") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("2.5: U  u") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("2.6: V  v") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("2.7: W  w") PORT_CODE(KEYCODE_W)

	PORT_START /* KEY ROW 3 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("3.0: X  x") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("3.1: Y  y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("3.2: Z  z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("3.3: [  {") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("3.4: \\  |") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("3.5: ]  }") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("3.6: ^  ~") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("3.7: _") PORT_CODE(KEYCODE_EQUALS)

	PORT_START /* KEY ROW 4 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("4.0: 0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("4.1: 1  !") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("4.2: 2  \"") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("4.3: 3  #") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("4.4: 4  $") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("4.5: 5  %") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("4.6: 6  &") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("4.7: 7  '") PORT_CODE(KEYCODE_7)

	PORT_START /* KEY ROW 5 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("5.0: 8  (") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("5.1: 9  )") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("5.2: :  *") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("5.3: ;  +") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("5.4: ,  <") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("5.5: -  =") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("5.6: .  >") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("5.7: /  ?") PORT_CODE(KEYCODE_SLASH)

	PORT_START /* KEY ROW 6 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("6.0: ENTER") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("6.1: CLEAR") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("6.2: BREAK") PORT_CODE(KEYCODE_END)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("6.3: UP") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("6.4: DOWN") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("6.5: LEFT") PORT_CODE(KEYCODE_LEFT)
	/* backspace should do the same as cursor left */
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("6.5: (BSP)") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("6.6: RIGHT") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("6.7: SPACE") PORT_CODE(KEYCODE_SPACE)

	PORT_START /* KEY ROW 7 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("7.0: SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("7.1: (ALT)") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("7.2: (PGUP)") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("7.3: (PGDN)") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("7.4: (INS)") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("7.5: (DEL)") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("7.6: (CTRL)") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("7.7: (ALTGR)") PORT_CODE(KEYCODE_RALT)

INPUT_PORTS_END

static gfx_layout trs80_charlayout_normal_width =
{
	FW,FH,			/* 6 x 12 characters */
	256,			/* 256 characters */
	1,				/* 1 bits per pixel */
	{ 0 },			/* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5 },
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8,
	   6*8, 7*8, 8*8, 9*8,10*8,11*8 },
	8*FH		   /* every char takes FH bytes */
};

static gfx_layout trs80_charlayout_double_width =
{
	FW*2,FH,	   /* FW*2 x FH*3 characters */
	256,		   /* 256 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	/* x offsets double width: use each bit twice */
	{ 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 },
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8,
	   6*8, 7*8, 8*8, 9*8,10*8,11*8 },
	8*FH		   /* every char takes FH bytes */
};

static gfx_decode trs80_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &trs80_charlayout_normal_width, 0, 4 },
	{ REGION_GFX1, 0, &trs80_charlayout_double_width, 0, 4 },
	{ -1 } /* end of array */
};

static unsigned char trs80_palette[] =
{
   0x00,0x00,0x00,
   0xff,0xff,0xff
};

static unsigned short trs80_colortable[] =
{
	0,1 	/* white on black */
};



/* Initialise the palette */
static PALETTE_INIT( trs80 )
{
	palette_set_colors(machine, 0, trs80_palette, sizeof(trs80_palette)/3);
	memcpy(colortable,trs80_colortable,sizeof(trs80_colortable));
}

static INT16 speaker_levels[3] = {0.0*32767,0.46*32767,0.85*32767};

static struct Speaker_interface speaker_interface =
{
	3,				/* optional: number of different levels */
	speaker_levels	/* optional: level lookup table */
};


static MACHINE_DRIVER_START( level1 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 1796000)        /* 1.796 Mhz */
	MDRV_CPU_PROGRAM_MAP(mem_level1, 0)
	MDRV_CPU_IO_MAP(io_level1, 0)
	MDRV_CPU_VBLANK_INT(trs80_frame_interrupt, 1)
	MDRV_CPU_PERIODIC_INT(trs80_timer_interrupt, TIME_IN_HZ(40))
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( trs80 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*FW, 16*FH)
	MDRV_SCREEN_VISIBLE_AREA(0*FW,64*FW-1,0*FH,16*FH-1)
	MDRV_GFXDECODE( trs80_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof(trs80_palette)/sizeof(trs80_palette[0])/3)
	MDRV_COLORTABLE_LENGTH(sizeof(trs80_colortable)/sizeof(trs80_colortable[0]))
	MDRV_PALETTE_INIT( trs80 )

	MDRV_VIDEO_START( trs80 )
	MDRV_VIDEO_UPDATE( trs80 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_CONFIG(speaker_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( model1 )
	MDRV_IMPORT_FROM( level1 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( mem_model1, 0 )
	MDRV_CPU_IO_MAP( io_model1, 0 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( model3 )
	MDRV_IMPORT_FROM( level1 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( mem_model3, 0 )
	MDRV_CPU_IO_MAP( io_model3, 0 )
	MDRV_CPU_VBLANK_INT(trs80_frame_interrupt, 2)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(trs80)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("level1.rom",  0x0000, 0x1000, CRC(70d06dff) SHA1(20d75478fbf42214381e05b14f57072f3970f765))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END


ROM_START(trs80l2)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("trs80.z33",   0x0000, 0x1000, CRC(83dbbbe2) SHA1(b013edb75b934693128baf105f75f47a359c47ae))
	ROM_LOAD("trs80.z34",   0x1000, 0x1000, CRC(05818718) SHA1(43c538ca77623af6417474ca5b95fb94205500c1))
	ROM_LOAD("trs80.zl2",   0x2000, 0x1000, CRC(306e5d66) SHA1(1e1abcfb5b02d4567cf6a81ffc35318723442369))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END


ROM_START(trs80l2a)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("trs80alt.z33",0x0000, 0x1000, CRC(be46faf5) SHA1(0e63fc11e207bfd5288118be5d263e7428cc128b))
	ROM_LOAD("trs80alt.z34",0x1000, 0x1000, CRC(6c791c2d) SHA1(2a38e0a248f6619d38f1a108eea7b95761cf2aee))
	ROM_LOAD("trs80alt.zl2",0x2000, 0x1000, CRC(55b3ad13) SHA1(6279f6a68f927ea8628458b278616736f0b3c339))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END


ROM_START(sys80)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("sys80rom.1",  0x0000, 0x1000, CRC(8f5214de) SHA1(d8c052be5a2d0ec74433043684791d0554bf203b))
	ROM_LOAD("sys80rom.2",  0x1000, 0x1000, CRC(46e88fbf) SHA1(a3ca32757f269e09316e1e91ba1502774e2f5155))
	ROM_LOAD("trs80.zl2",  0x2000, 0x1000, CRC(306e5d66) SHA1(1e1abcfb5b02d4567cf6a81ffc35318723442369))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END

ROM_START(lnw80)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("lnw_a.bin",  0x0000, 0x0800, CRC(e09f7e91) SHA1(cd28e72efcfebde6cf1c7dbec4a4880a69e683da))
	ROM_LOAD("lnw_a1.bin", 0x0800, 0x0800, CRC(ac297d99) SHA1(ccf31d3f9d02c3b68a0ee3be4984424df0e83ab0))
	ROM_LOAD("lnw_b.bin",  0x1000, 0x0800, CRC(c4303568) SHA1(13e3d81c6f0de0e93956fa58c465b5368ea51682))
	ROM_LOAD("lnw_b1.bin", 0x1800, 0x0800, CRC(3a5ea239) SHA1(8c489670977892d7f2bfb098f5df0b4dfa8fbba6))
	ROM_LOAD("lnw_c.bin",  0x2000, 0x0800, CRC(2ba025d7) SHA1(232efbe23c3f5c2c6655466ebc0a51cf3697be9b))
	ROM_LOAD("lnw_c1.bin", 0x2800, 0x0800, CRC(ed547445) SHA1(20102de89a3ee4a65366bc2d62be94da984a156b))

	ROM_REGION(0x01000, REGION_GFX1,0)
	ROM_LOAD("lnw_chr.bin",0x0800, 0x0800, CRC(c89b27df) SHA1(be2a009a07e4378d070002a558705e9a0de59389))
ROM_END

ROM_START(trs80m3)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("trs80m3.rom", 0x0000, 0x3800, CRC(bddbf843))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END

ROM_START(trs80m4)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("trs80m4.rom", 0x0000, 0x3800, CRC(1a92d54d) SHA1(752555fdd0ff23abc9f35c6e03d9d9b4c0e9677b))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	/* this rom unlikely to be the correct one, but it will do for now */
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END

static void trs80_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 0; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_trs80_cas; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_trs80_cas; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "cas"); break;
	}
}

static void trs80_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "cmd"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_trs80_cmd; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_QUICKLOAD_DELAY:				info->d = 0.5; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(trs80)
	CONFIG_DEVICE(trs80_quickload_getinfo)
	CONFIG_DEVICE(trs80_cassette_getinfo)
SYSTEM_CONFIG_END


static void trs8012_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_trs80_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(trs8012)
	CONFIG_IMPORT_FROM(trs80)
	CONFIG_DEVICE(trs8012_floppy_getinfo)
SYSTEM_CONFIG_END


/*    YEAR  NAME      PARENT	 COMPAT	        MACHINE   INPUT	 INIT  CONFIG	COMPANY	 FULLNAME */
COMP( 1977, trs80,    0,	 0,		level1,   trs80, trs80,    trs80,	"Tandy Radio Shack",  "TRS-80 Model I (Level I Basic)" , 0)
COMP( 1978, trs80l2,  trs80,	 0,		model1,   trs80, trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model I (Radio Shack Level II Basic)" , 0)
COMP( 1978, trs80l2a, trs80,	 0,		model1,   trs80, trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model I (R/S L2 Basic)" , 0)
COMP( 1980, sys80,    trs80,	 0,		model1,   trs80, trs80,    trs8012,	"EACA Computers Ltd.","System-80" , 0)
COMP( 1981, lnw80,    trs80,	 0,		model1,   trs80, trs80,    trs8012,	"LNW Research","LNW-80", GAME_NOT_WORKING )
COMP( 1980, trs80m3,  trs80,	 0,		model3,   trs80, trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model III", GAME_NOT_WORKING )
COMP( 1980, trs80m4,  trs80,	 0,		model3,   trs80, trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model 4", GAME_NOT_WORKING )
								 
