/***************************************************************************

  Enhanced Graphics Adapter (EGA) section

TODO - Write documentation

"Regular" register on an EGA graphics card:

    3C2 - 7 6 5 4 3 2 1 0 - Misc Output Register - Write Only
          | | | | | | | |
          | | | | | | | +-- 3Bx/3Dx I/O port select
          | | | | | | |     0 = 3Bx for CRTC I/O, 3BA for status reg 1
          | | | | | | |     1 = 3Dx for CRTC I/O, 3DA for status reg 1
          | | | | | | +---- enable ram
          | | | | | |       0 = disable ram from the processor
          | | | | | |       1 = enable ram to respond to addresses
          | | | | | |           designated by the Control Data Select
          | | | | | |           value in the Graphics Controllers.
          | | | | | +------ clock select bit 0
          | | | | +-------- clock select bit 1
          | | | |           00 = 14MHz from Processor I/O channel
          | | | |           01 = 16MHz on-bord clock
          | | | |           10 = External clock from feature connector
          | | | |           11 = reserved/unused
          | | | +---------- disable video drivers
          | | |             0 = activate internal video drivers
          | | |             1 = disable internal video drivers
          | | +------------ page bit for odd/even. Selects between 2 pages
          | |               of 64KB of memory when in odd/even mode.
          | |               0 = select low page
          | |               1 = select high page
          | +-------------- horizontal retrace polarity
          |                 0 = select positive
          |                 1 = select negative
          +---------------- vertical retrace polarity
                            0 = select positive
                            1 = select negative


    3C2 - 7 6 5 4 3 2 1 0 - Input Status Reg 0 - Read Only
          | | | | | | | |
          | | | | | | | +-- reserved/unused
          | | | | | | +---- reserved/unused
          | | | | | +------ reserved/unused
          | | | | +-------- reserved/unused
          | | | +---------- switch sense
          | | |             0 = switch is closed
          | | |             1 = allows processor to read the 4 config switches
          | | |                 on the EGA adapter. The setting of CLKSEL determines
          | | |                 switch to read.
          | | +------------ input from FEAT0 on the feature connector
          | +-------------- input from FEAT1 on the feature connector
          +---------------- CRT Interrupt
                            0 = vertical retrace if occuring
                            1 = video is being displayed


    Configuration switches
    SW1 SW2 SW3 SW4
    OFF OFF OFF ON  - EGA, Color 80x25 (5153)
                    - EGA (primary) + MDA, Color 80x25 + Monochrome
    OFF OFF ON  OFF - EGA, Monochrome (5151)
                    - EGA (primary) + CGA, Monochrome + Color 80x25
    OFF OFF ON  ON  - EGA + MDA (primary), 5154 + Enhanced Monochrome
    OFF ON  OFF ON  - EGA + CGA (primary), Monochrome + Color 80x25
    OFF ON  ON  OFF - EGA, Enhanced Color - Enhanced Mode (5154)
                    - EGA (primary) + MDA, 5154 monitor + Enhanced Monochrome
    OFF ON  ON  ON  - EGA + MDA (primary), Color 80x25 + Monochrome
    ON  OFF OFF ON  - EGA, Color 40x25 (5153)
                    - EGA (primary) + MDA, Color 40x25 + Monochrome
    ON  OFF ON  OFF - EGA (primary) + CGA, Monochrome + Color 40x25
    ON  OFF ON  ON  - EGA + MDA (primary), 5154 + Normal Monochrome
    ON  ON  OFF ON  - EGA + CGA (primary), Monochrome + Color 40x25
    ON  ON  ON  OFF - EGA, Enhanced Color - Enhanced Mode (5154)
                    - EGA (primary) + MDA, 5154 monitor + Normal Monochrome
    ON  ON  ON  ON  - EGA + MDA (primary), Color 40x25 + Monochrome


    3XA - 7 6 5 4 3 2 1 0 - Feature Control Register - Write Only
          | | | | | | | |
          | | | | | | | +-- output to FEAT0 of the feature connector
          | | | | | | +---- output to FEAT1 of the feature connector
          | | | | | +------ reserved/unused
          | | | | +-------- reserved/unused
          | | | +---------- reserved/unused
          | | +------------ reserved/unused
          | +-------------- reserved/unused
          +---------------- reserved/unused

    3XA - 7 6 5 4 3 2 1 0 - Input Status Reg 1 - Read Only
          | | | | | | | |
          | | | | | | | +-- display enable
          | | | | | | |     0 = indicates the CRT raster is in a horizontal or vertical retrace
          | | | | | | |     1 = otherwise
          | | | | | | +---- light pen strobe
          | | | | | |       0 = light pen trigger has not been set
          | | | | | |       1 = light pen trigger has been set
          | | | | | +------ light pen switch
          | | | | |         0 = switch is closed
          | | | | |         1 = switch is open
          | | | | +-------- vertical blank
          | | | |           0 = video information is being displayed
          | | | |           1 = CRT is in vertical blank
          | | | +---------- diagnostic usage, output depends on AR12 video status mux bits
          | | |             mux bits - output
          | | |             00       - blue
          | | |             01       - I blue
          | | |             10       - I red
          | | |             11       - unknown
          | | +------------ diagnostic usage, output depends on AR12 video status mux bits
          | |               mux bits - output
          | |               00       - red
          | |               01       - green
          | |               10       - I green
          | |               11       - unknown
          | +-------------- reserved/unused
          +---------------- reserved/unused



The EGA graphics card introduces a lot of new indexed registers to handle the
enhanced graphics. These new indexed registers can be divided into three
groups:
- attribute registers
- sequencer registers
- graphics controllers registers


Attribute Registers AR00 - AR13

The Attribute Registers are all accessed through I/O port 0x3C0. The first
write to I/O port 0x3C0 sets the index register. The next write to I/O port
0x3C0 actually sets the data to the indexed register.

    3C0 - 7 6 5 4 3 2 1 0 - Attribute Access Register
          | | | | | | | |
          | | | | | | | +-- index bit 0
          | | | | | | +---- index bit 1
          | | | | | +------ index bit 2
          | | | | +-------- index bit 3
          | | | +---------- index bit 4
          | | +------------ palette source
          | +-------------- reserved/unused
          +---------------- reserved/unused


    AR00-AR0F - 7 6 5 4 3 2 1 0 - Palette Register #00 - #0F
                | | | | | | | |
                | | | | | | | +-- MSB B
                | | | | | | +---- MSB G
                | | | | | +------ MSB R
                | | | | +-------- LSB B
                | | | +---------- LSB G
                | | +------------ LSB R
                | +-------------- reserved/unused
                +---------------- reserved/unused


    AR10 - 7 6 5 4 3 2 1 0 - Mode Control Register
           | | | | | | | |
           | | | | | | | +-- Text/Graphics select
           | | | | | | +---- Monochrome/Color select
           | | | | | +------ 9th dot setting
           | | | | +-------- Blink Enable
           | | | +---------- reserved/unsued
           | | +------------ 0 = line compare does not affect pixel output
           | |               1 = line compare does affect pixel output
           | +-------------- 0 = pixel changes every dot clock
           |                 1 = pixel changes every other dot clock
           +---------------- reserved/unused


    AR11 - 7 6 5 4 3 2 1 0 - Overscan Color Register
           | | | | | | | |
           | | | | | | | +-- MSB B
           | | | | | | +---- MSB G
           | | | | | +------ MSB R
           | | | | +-------- LSB B
           | | | +---------- LSB G
           | | +------------ LSB R
           | +-------------- reserved/unused
           +---------------- reserved/unused


    AR12 - 7 6 5 4 3 2 1 0 - Color Plane Enable Register
           | | | | | | | |
           | | | | | | | +-- Enable plane 0
           | | | | | | +---- Enable plane 1
           | | | | | +------ Enable plane 2
           | | | | +-------- Enable plane 3
           | | | +---------- Video Status Mux bit 0
           | | +------------ Video Status Mux bit 1
           | +-------------- reserved/unused
           +---------------- reserved/unused


    AR13 - 7 6 5 4 3 2 1 0 - Horizontal Panning Register
           | | | | | | | |
           | | | | | | | +-- Pixel left shift bit 0
           | | | | | | +---- Pixel left shift bit 1
           | | | | | +------ Pixel left shift bit 2
           | | | | +-------- Pixel left shift bit 3
           | | | +---------- reserved/unused
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


Sequencer Registers SR00 - SR04

The Sequencer Registers are accessed through an index register located at I/O
port 0x3C4, and a data register located at I/O port 0x3C5.

    3C4 - 7 6 5 4 3 2 1 0 - Sequencer Index Register - Write Only
          | | | | | | | |
          | | | | | | | +-- index bit 0
          | | | | | | +---- index bit 1
          | | | | | +------ index bit 2
          | | | | +-------- reserved/unused
          | | | +---------- reserved/unused
          | | +------------ reserved/unused
          | +-------------- reserved/unused
          +---------------- reserved/unused


    3C5 - 7 6 5 4 3 2 1 0 - Sequencer Data Register - Write Only
          | | | | | | | |
          | | | | | | | +-- data bit 0
          | | | | | | +---- data bit 1
          | | | | | +------ data bit 2
          | | | | +-------- data bit 3
          | | | +---------- data bit 4
          | | +------------ data bit 5
          | +-------------- data bit 6
          +---------------- data bit 7


    SR00 - 7 6 5 4 3 2 1 0 - Reset Control Register
           | | | | | | | |
           | | | | | | | +-- Must be 1 for normal operation
           | | | | | | +---- Must be 1 for normal operation
           | | | | | +------ reserved/unused
           | | | | +-------- reserved/unused
           | | | +---------- reserved/unused
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


    SR01 - 7 6 5 4 3 2 1 0 - Clocking Mode
           | | | | | | | |
           | | | | | | | +-- 0 = 9 dots per char, 1 = 8 dots per char
           | | | | | | +---- clock frequency, 0 = 4 out of 5 memory cycles, 1 = 2 out of 5 memory cycles
           | | | | | +------ shift load
           | | | | +-------- 0 = normal dot clock, 1 = master dot clock / 2
           | | | +---------- reserved/unused
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


    SR02 - 7 6 5 4 3 2 1 0 - Map Mask
           | | | | | | | |
           | | | | | | | +-- 1 = enable map 0 for writing
           | | | | | | +---- 1 = enable map 1 for writing
           | | | | | +------ 1 = enable map 2 for writing
           | | | | +-------- 1 = enable map 3 for writing
           | | | +---------- reserved/unused
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


    SR03 - 7 6 5 4 3 2 1 0 - Character Map Select
           | | | | | | | |
           | | | | | | | +-- character map select B bit 0
           | | | | | | +---- character map select B bit 1
           | | | | | |       Selects the map used to generate alpha characters when
           | | | | | |       attribute bit 3 is set to 0
           | | | | | |       00 = map 0 - 1st 8KB of plane 2 bank 0
           | | | | | |       01 = map 1 - 2nd 8KB of plane 2 bank 1
           | | | | | |       10 = map 2 - 3rd 8KB of plane 2 bank 2
           | | | | | |       11 = map 3 - 4th 8KB of plane 2 bank 3
           | | | | | +------ character map select A bit 0
           | | | | +-------- character map select A bit 1
           | | | |           Selects the map used to generate alpha characters when
           | | | |           attribute bit 3 is set to 1
           | | | |           00 = map 0 - 1st 8KB of plane 2 bank 0
           | | | |           01 = map 1 - 2nd 8KB of plane 2 bank 1
           | | | |           10 = map 2 - 3rd 8KB of plane 2 bank 2
           | | | |           11 = map 3 - 4th 8KB of plane 2 bank 3
           | | | +---------- reserved/unused
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


    SR04 - 7 6 5 4 3 2 1 0 - Memory Mode Register
           | | | | | | | |
           | | | | | | | +-- 0 = graphics mode, 1 = text mode
           | | | | | | +---- 0 = no memory extension, 1 = memory extension
           | | | | | +------ 0 = odd/even storage, 1 = sequential storage
           | | | | +-------- reserved/unused
           | | | +---------- reserved/unused
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


Graphics Controller Registers GR00 - GR08

The Graphics Controller Registers are accessed through an index register
located at I/O port 0x3CE, and a data register located at I/O port 0x3CF.

    GR00 - 7 6 5 4 3 2 1 0 - Set/Reset Register
           | | | | | | | |
           | | | | | | | +-- set/reset for plane 0
           | | | | | | +---- set/reset for plane 1
           | | | | | +------ set/reset for plane 2
           | | | | +-------- set/reset for plane 3
           | | | +---------- reserved/unused
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


    GR01 - 7 6 5 4 3 2 1 0 - Enable Set/Reset Register
           | | | | | | | |
           | | | | | | | +-- enable set/reset for plane 0
           | | | | | | +---- enable set/reset for plane 1
           | | | | | +------ enable set/reset for plane 2
           | | | | +-------- enable set/reset for plane 3
           | | | +---------- reserved/unused
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


    GR02 - 7 6 5 4 3 2 1 0 - Color Compare Register
           | | | | | | | |
           | | | | | | | +-- color compare 0
           | | | | | | +---- color compare 1
           | | | | | +------ color compare 2
           | | | | +-------- color compare 3
           | | | +---------- reserved/unused
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


    GR03 - 7 6 5 4 3 2 1 0 - Data Rotate Register
           | | | | | | | |
           | | | | | | | +-- number of positions to rotate bit 0
           | | | | | | +---- number of positions to rotate bit 1
           | | | | | +------ number of positions to rotate bit 2
           | | | | +-------- function select bit 0
           | | | +---------- function select bit 1
           | | |             00 = data overwrites in specified color
           | | |             01 = data ANDed with latched data
           | | |             10 = data ORed with latched data
           | | |             11 = data XORed with latched data
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


    GR04 - 7 6 5 4 3 2 1 0 - Read Map Select Register
           | | | | | | | |
           | | | | | | | +-- plane select bit 0
           | | | | | | +---- plane select bit 1
           | | | | | +------ plane select bit 2
           | | | | +-------- reserved/unused
           | | | +---------- reserved/unused
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


    GR05 - 7 6 5 4 3 2 1 0 - Mode Register
           | | | | | | | |
           | | | | | | | +-- write mode bit 0
           | | | | | | +---- write mode bit 1
           | | | | | |       00 = write 8 bits of value in set/reset register if enabled,
           | | | | | |            otherwise write rotated processor data
           | | | | | |       01 = write with contents of processor latches
           | | | | | |       10 = memory plane 0-3 filled with 8 bits of value of data bit 0-3
           | | | | | |       11 = reserved/unused
           | | | | | +------ test condition
           | | | | |         0 = normal operation
           | | | | |         1 = put outputs in high impedance state
           | | | | +-------- read mode
           | | | |           0 = read from plane selected by GR04
           | | | |           1 = do color compare
           | | | +---------- odd/even addressing mode
           | | +------------ shift register mode
           | |               0 = sequential
           | |               1 = even bits from even maps, odd bits from odd maps
           | +-------------- reserved/unused
           +---------------- reserved/unused


    GR06 - 7 6 5 4 3 2 1 0 - Miscellaneous Register
           | | | | | | | |
           | | | | | | | +-- 0 = text mode, 1 = graphics mode
           | | | | | | +---- chain odd maps to even
           | | | | | +------ memory map bit 0
           | | | | +-------- memory map bit 1
           | | | |           00 = 0xA0000, 128KB
           | | | |           01 = 0xA0000, 64KB
           | | | |           10 = 0xB0000, 32KB
           | | | |           11 = 0xB8000, 32KB
           | | | +---------- reserved/unused
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


    GR07 - 7 6 5 4 3 2 1 0 - Color Plane Ignore Register
           | | | | | | | |
           | | | | | | | +-- ignore color plane 0
           | | | | | | +---- ignore color plane 1
           | | | | | +------ ignore color plane 2
           | | | | +-------- ignore color plane 3
           | | | +---------- reserved/unused
           | | +------------ reserved/unused
           | +-------------- reserved/unused
           +---------------- reserved/unused


    GR08 - 7 6 5 4 3 2 1 0 - Bit Mask Register
           | | | | | | | |
           | | | | | | | +-- write enable bit 0
           | | | | | | +---- write enable bit 1
           | | | | | +------ write enable bit 2
           | | | | +-------- write enable bit 3
           | | | +---------- write enable bit 4
           | | +------------ write enable bit 5
           | +-------------- write enable bit 6
           +---------------- write enable bit 7


***************************************************************************/

#include "emu.h"
#include "video/pc_ega.h"
#include "video/crtc_ega.h"
#include "memconv.h"


#define VERBOSE_EGA		1


static struct
{
	device_t *crtc_ega;
	crtc_ega_update_row_func	update_row;

	/* Video memory and related variables */
	UINT8	*videoram;
	UINT8	*videoram_nothing;
	UINT8	*videoram_a0000;
	UINT8	*videoram_b0000;
	UINT8	*videoram_b8000;
	UINT8	*charA;
	UINT8	*charB;

	/* Registers */
	UINT8	misc_output;
	UINT8	feature_control;

	/* Attribute registers AR00 - AR14
    */
	struct {
		UINT8	index;
		UINT8	data[32];
		UINT8	index_write;
	} attribute;

	/* Sequencer registers SR00 - SR04
    */
	struct {
		UINT8	index;
		UINT8	data[8];
	} sequencer;

	/* Graphics controller registers GR00 - GR08
    */
	struct {
		UINT8	index;
		UINT8	data[16];
	} graphics_controller;

	UINT8	frame_cnt;
	UINT8	vsync;
	UINT8	vblank;
	UINT8	display_enable;
} ega;


/*
    Prototypes
*/
static VIDEO_START( pc_ega );
static SCREEN_UPDATE( pc_ega );
static PALETTE_INIT( pc_ega );
static CRTC_EGA_UPDATE_ROW( ega_update_row );
static CRTC_EGA_ON_DE_CHANGED( ega_de_changed );
static CRTC_EGA_ON_VSYNC_CHANGED( ega_vsync_changed );
static CRTC_EGA_ON_VBLANK_CHANGED( ega_vblank_changed );
static READ8_HANDLER( pc_ega8_3b0_r );
static WRITE8_HANDLER( pc_ega8_3b0_w );
static READ16_HANDLER( pc_ega16le_3b0_r );
static WRITE16_HANDLER( pc_ega16le_3b0_w );
static READ32_HANDLER( pc_ega32le_3b0_r );
static WRITE32_HANDLER( pc_ega32le_3b0_w );
static READ8_HANDLER( pc_ega8_3c0_r );
static WRITE8_HANDLER( pc_ega8_3c0_w );
static READ16_HANDLER( pc_ega16le_3c0_r );
static WRITE16_HANDLER( pc_ega16le_3c0_w );
static READ32_HANDLER( pc_ega32le_3c0_r );
static WRITE32_HANDLER( pc_ega32le_3c0_w );
static READ8_HANDLER( pc_ega8_3d0_r );
static WRITE8_HANDLER( pc_ega8_3d0_w );
static READ16_HANDLER( pc_ega16le_3d0_r );
static WRITE16_HANDLER( pc_ega16le_3d0_w );
static READ32_HANDLER( pc_ega32le_3d0_r );
static WRITE32_HANDLER( pc_ega32le_3d0_w );
static WRITE8_HANDLER( pc_ega_videoram_w );
static WRITE16_HANDLER( pc_ega_videoram16le_w );
static WRITE32_HANDLER( pc_ega_videoram32le_w );


static const crtc_ega_interface crtc_ega_ega_intf =
{
	EGA_SCREEN_NAME,	/* screen number */
	8,					/* numbers of pixels per video memory address */
	NULL,				/* begin_update */
	ega_update_row,		/* update_row */
	NULL,				/* end_update */
	ega_de_changed,		/* on_de_chaged */
	NULL,				/* on_hsync_changed */
	ega_vsync_changed,	/* on_vsync_changed */
	ega_vblank_changed	/* on_vblank_changed */
};


MACHINE_CONFIG_FRAGMENT( pcvideo_ega )
	MCFG_SCREEN_ADD(EGA_SCREEN_NAME, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_RAW_PARAMS(16257000,912,0,640,262,0,200)
	MCFG_SCREEN_UPDATE( pc_ega )

	MCFG_PALETTE_LENGTH( 64 )
	MCFG_PALETTE_INIT(pc_ega)

	MCFG_CRTC_EGA_ADD(EGA_CRTC_NAME, 16257000/8, crtc_ega_ega_intf)

	MCFG_VIDEO_START( pc_ega )
MACHINE_CONFIG_END


static PALETTE_INIT( pc_ega )
{
	int i;

	for ( i = 0; i < 64; i++ )
	{
		UINT8 r = ( ( i & 0x04 ) ? 0xAA : 0x00 ) + ( ( i & 0x20 ) ? 0x55 : 0x00 );
		UINT8 g = ( ( i & 0x02 ) ? 0xAA : 0x00 ) + ( ( i & 0x10 ) ? 0x55 : 0x00 );
		UINT8 b = ( ( i & 0x01 ) ? 0xAA : 0x00 ) + ( ( i & 0x08 ) ? 0x55 : 0x00 );

		palette_set_color_rgb( machine, i, r, g, b );
	}
}


static void pc_ega_install_banks( running_machine &machine )
{
	switch ( ega.graphics_controller.data[6] & 0x0c )
	{
	case 0x00:		/* 0xA0000, 128KB */
		ega.videoram_a0000 = ega.videoram + 0x10000;
		ega.videoram_b0000 = ega.videoram;
		ega.videoram_b8000 = ega.videoram + 0x8000;
		break;
	case 0x04:		/* 0xA0000, 64KB */
		ega.videoram_a0000 = ega.videoram + 0x10000;
		ega.videoram_b0000 = NULL;
		ega.videoram_b8000 = NULL;
		break;
	case 0x08:		/* 0xB0000, 32KB */
		ega.videoram_a0000 = NULL;
		ega.videoram_b0000 = ega.videoram;
		ega.videoram_b8000 = NULL;
		break;
	case 0x0c:		/* 0xB8000, 32KB */
		ega.videoram_a0000 = NULL;
		ega.videoram_b0000 = NULL;
		ega.videoram_b8000 = ega.videoram;
		break;
	}
	memory_set_bankptr(machine,  "bank11", ega.videoram_a0000 ? ega.videoram_a0000 : ega.videoram_nothing );
	memory_set_bankptr(machine,  "bank12", ega.videoram_b0000 ? ega.videoram_b0000 : ega.videoram_nothing );
	memory_set_bankptr(machine,  "bank13", ega.videoram_b8000 ? ega.videoram_b8000 : ega.videoram_nothing );
}


static VIDEO_START( pc_ega )
{
	int buswidth;
	address_space *space = machine.firstcpu->memory().space(AS_PROGRAM);
	address_space *spaceio = machine.firstcpu->memory().space(AS_IO);

	buswidth = machine.firstcpu->memory().space_config(AS_PROGRAM)->m_databus_width;
	switch(buswidth)
	{
		case 8:
			space->install_read_bank(0xa0000, 0xaffff, "bank11" );
			space->install_read_bank(0xb0000, 0xb7fff, "bank12" );
			space->install_read_bank(0xb8000, 0xbffff, "bank13" );
			space->install_legacy_write_handler(0xa0000, 0xbffff, FUNC(pc_ega_videoram_w) );
			spaceio->install_legacy_read_handler(0x3b0, 0x3bb, FUNC(pc_ega8_3b0_r) );
			spaceio->install_legacy_write_handler(0x3b0, 0x3bb, FUNC(pc_ega8_3b0_w) );
			spaceio->install_legacy_read_handler(0x3c0, 0x3cf, FUNC(pc_ega8_3c0_r) );
			spaceio->install_legacy_write_handler(0x3c0, 0x3cf, FUNC(pc_ega8_3c0_w) );
			spaceio->install_legacy_read_handler(0x3d0, 0x3db, FUNC(pc_ega8_3d0_r) );
			spaceio->install_legacy_write_handler(0x3d0, 0x3db, FUNC(pc_ega8_3d0_w) );
			break;

		case 16:
			space->install_read_bank(0xa0000, 0xaffff, "bank11" );
			space->install_read_bank(0xb0000, 0xb7fff, "bank12" );
			space->install_read_bank(0xb8000, 0xbffff, "bank13" );
			space->install_legacy_write_handler(0xa0000, 0xbffff, FUNC(pc_ega_videoram16le_w) );
			spaceio->install_legacy_read_handler(0x3b0, 0x3bb, FUNC(pc_ega16le_3b0_r) );
			spaceio->install_legacy_write_handler(0x3b0, 0x3bb, FUNC(pc_ega16le_3b0_w) );
			spaceio->install_legacy_read_handler(0x3c0, 0x3cf, FUNC(pc_ega16le_3c0_r) );
			spaceio->install_legacy_write_handler(0x3c0, 0x3cf, FUNC(pc_ega16le_3c0_w) );
			spaceio->install_legacy_read_handler(0x3d0, 0x3db, FUNC(pc_ega16le_3d0_r) );
			spaceio->install_legacy_write_handler(0x3d0, 0x3db, FUNC(pc_ega16le_3d0_w) );
			break;

		case 32:
			space->install_read_bank(0xa0000, 0xaffff, "bank11" );
			space->install_read_bank(0xb0000, 0xb7fff, "bank12" );
			space->install_read_bank(0xb8000, 0xbffff, "bank13" );
			space->install_legacy_write_handler(0xa0000, 0xbffff, FUNC(pc_ega_videoram32le_w) );
			spaceio->install_legacy_read_handler(0x3b0, 0x3bb, FUNC(pc_ega32le_3b0_r) );
			spaceio->install_legacy_write_handler(0x3b0, 0x3bb, FUNC(pc_ega32le_3b0_w) );
			spaceio->install_legacy_read_handler(0x3c0, 0x3cf, FUNC(pc_ega32le_3c0_r) );
			spaceio->install_legacy_write_handler(0x3c0, 0x3cf, FUNC(pc_ega32le_3c0_w) );
			spaceio->install_legacy_read_handler(0x3d0, 0x3db, FUNC(pc_ega32le_3d0_r) );
			spaceio->install_legacy_write_handler(0x3d0, 0x3db, FUNC(pc_ega32le_3d0_w) );
			break;

		default:
			fatalerror("EGA: Bus width %d not supported", buswidth);
			break;
	}

	memset( &ega, 0, sizeof( ega ) );

	/* Install 256KB Video ram on our EGA card */
	ega.videoram = machine.region( "gfx2" )->base();

	memset( ega.videoram + 256 * 1024, 0xFF, 64 * 1024 );

	ega.videoram_nothing = ega.videoram + ( 256 * 1024 );

	pc_ega_install_banks(machine);

	ega.crtc_ega = machine.device(EGA_CRTC_NAME);
	ega.update_row = NULL;
	ega.misc_output = 0;
	ega.attribute.index_write = 1;

	/* Set up default palette */
	ega.attribute.data[0] = 0;
	ega.attribute.data[1] = 1;
	ega.attribute.data[2] = 2;
	ega.attribute.data[3] = 3;
	ega.attribute.data[4] = 4;
	ega.attribute.data[5] = 5;
	ega.attribute.data[6] = 0x14;
	ega.attribute.data[7] = 7;
	ega.attribute.data[8] = 0x38;
	ega.attribute.data[9] = 0x39;
	ega.attribute.data[10] = 0x3A;
	ega.attribute.data[11] = 0x3B;
	ega.attribute.data[12] = 0x3C;
	ega.attribute.data[13] = 0x3D;
	ega.attribute.data[14] = 0x3E;
	ega.attribute.data[15] = 0x3F;
}


static SCREEN_UPDATE( pc_ega )
{
	crtc_ega_update( ega.crtc_ega, bitmap, cliprect);
	return 0;
}


static CRTC_EGA_UPDATE_ROW( ega_update_row )
{
	if ( ega.update_row )
	{
		ega.update_row( device, bitmap, cliprect, ma, ra, y, x_count, cursor_x, param );
	}
}


static CRTC_EGA_ON_DE_CHANGED( ega_de_changed )
{
	ega.display_enable = display_enabled ? 1 : 0;
}


static CRTC_EGA_ON_VSYNC_CHANGED( ega_vsync_changed )
{
	ega.vsync = vsync ? 8 : 0;
	if ( vsync )
	{
		ega.frame_cnt++;
	}
}


static CRTC_EGA_ON_VBLANK_CHANGED( ega_vblank_changed )
{
	ega.vblank = vblank ? 8 : 0;
}


static CRTC_EGA_UPDATE_ROW( pc_ega_graphics )
{
	UINT16	*p = BITMAP_ADDR16(bitmap, y, 0);
	int	i;

//  logerror( "pc_ega_graphics: y = %d, x_count = %d, ma = %d, ra = %d\n", y, x_count, ma, ra );

	for ( i = 0; i < x_count; i++ )
	{
		*p = 0;
	}
}


static CRTC_EGA_UPDATE_ROW( pc_ega_text )
{
	UINT16	*p = BITMAP_ADDR16(bitmap, y, 0);
	int	i;

//  logerror( "pc_ega_text: y = %d, x_count = %d, ma = %d, ra = %d\n", y, x_count, ma, ra );

	for ( i = 0; i < x_count; i++ )
	{
		UINT16	offset = ( ma + i ) << 1;
		UINT8	chr = ega.videoram[ offset ];
		UINT8	attr = ega.videoram[ offset + 1 ];
		UINT8	data = ( attr & 0x08 ) ? ega.charA[ chr * 32 + ra ] : ega.charB[ chr * 32 + ra ];
		UINT16	fg = ega.attribute.data[ attr & 0x0F ];
		UINT16	bg = ega.attribute.data[ attr >> 4 ];

		*p = ( data & 0x80 ) ? fg : bg; p++;
		*p = ( data & 0x40 ) ? fg : bg; p++;
		*p = ( data & 0x20 ) ? fg : bg; p++;
		*p = ( data & 0x10 ) ? fg : bg; p++;
		*p = ( data & 0x08 ) ? fg : bg; p++;
		*p = ( data & 0x04 ) ? fg : bg; p++;
		*p = ( data & 0x02 ) ? fg : bg; p++;
		*p = ( data & 0x01 ) ? fg : bg; p++;
	}
}


static void pc_ega_change_mode( device_t *device )
{
	int clock, pixels;

	ega.update_row = NULL;

	/* Check for graphics mode */
	if (   ( ega.attribute.data[0x10] & 0x01 ) &&
	     ! ( ega.sequencer.data[0x04] & 0x01 ) &&
	       ( ega.graphics_controller.data[0x06] & 0x01 ) )
	{
		if ( VERBOSE_EGA )
		{
			logerror("pc_ega_change_mode(): Switch to graphics mode\n");
		}

		ega.update_row = pc_ega_graphics;
	}

	/* Check for text mode */
	if ( ! ( ega.attribute.data[0x10] & 0x01 ) &&
	       ( ega.sequencer.data[0x04] & 0x01 ) &&
	     ! ( ega.graphics_controller.data[0x06] & 0x01 ) )
	{
		if ( VERBOSE_EGA )
		{
			logerror("pc_ega_chnage_mode(): Switching to text mode\n");
		}

		ega.update_row = pc_ega_text;

		/* Set character maps */
		if ( ega.sequencer.data[0x04] & 0x02 )
		{
			ega.charA = ega.videoram + 0x10000 + ( ( ega.sequencer.data[0x03] & 0x0c ) >> 1 ) * 0x2000;
			ega.charB = ega.videoram + 0x10000 + ( ega.sequencer.data[0x03] & 0x03 ) * 0x2000;
		}
		else
		{
			ega.charA = ega.videoram + 0x10000;
			ega.charB = ega.videoram + 0x10000;
		}
	}

	/* Check for changes to the crtc input clock and number of pixels per clock */
	clock = ( ( ega.misc_output & 0x0c ) ? 16257000 : XTAL_14_31818MHz );
	pixels = ( ( ega.sequencer.data[0x01] & 0x01 ) ? 8 : 9 );

	if ( ega.sequencer.data[0x01] & 0x08 )
	{
		clock >>= 1;
	}
	crtc_ega_set_clock( device, clock / pixels );
	crtc_ega_set_hpixels_per_column( device, pixels );

if ( ! ega.update_row )
	logerror("unknown video mode\n");
}


static READ8_HANDLER( pc_ega8_3X0_r )
{
	int data = 0xff;

	if ( VERBOSE_EGA )
	{
		logerror("pc_ega_3X0_r: offset = %02x\n", offset );
	}

	switch ( offset )
	{
	/* CRT Controller - address register */
	case 0: case 2: case 4: case 6:
		/* return last written mc6845 address value here? */
		break;

	/* CRT Controller - data register */
	case 1: case 3: case 5: case 7:
		data = crtc_ega_register_r( ega.crtc_ega, offset );
		break;

	/* Input Status Register 1 */
	case 10:
		data = ega.vblank | ega.display_enable;

		if ( ega.display_enable )
		{
			/* For the moment i'm putting in some bogus data */
			static int pixel_data;

			pixel_data = ( pixel_data + 1 ) & 0x03;
			data |= ( pixel_data << 4 );
		}

		/* Reset the attirubte writing flip flop to let the next write go to the index reigster */
		ega.attribute.index_write = 1;
		break;
	}

	return data;
}


static WRITE8_HANDLER( pc_ega8_3X0_w )
{
	if ( VERBOSE_EGA )
	{
		logerror("pc_ega_3X0_w: offset = %02x, data = %02x\n", offset, data );
	}

	switch ( offset )
	{
	/* CRT Controller - address register */
	case 0: case 2: case 4: case 6:
		crtc_ega_address_w( ega.crtc_ega, offset, data );
		break;

	/* CRT Controller - data register */
	case 1: case 3: case 5: case 7:
		crtc_ega_register_w( ega.crtc_ega, offset, data );
		break;

	/* Set Light Pen Flip Flop */
	case 9:
		break;

	/* Feature Control */
	case 10:
		ega.feature_control = data;
		break;

	/* Clear Light Pen Flip Flop */
	case 11:
		break;
	}
}


static READ8_HANDLER( pc_ega8_3b0_r )
{
	return ( ega.misc_output & 0x01 ) ? 0xFF : pc_ega8_3X0_r( space, offset );
}


static READ8_HANDLER( pc_ega8_3d0_r )
{
	return ( ega.misc_output & 0x01 ) ? pc_ega8_3X0_r( space, offset ) : 0xFF;
}


static WRITE8_HANDLER( pc_ega8_3b0_w )
{
	if ( ! ( ega.misc_output & 0x01 ) )
	{
		pc_ega8_3X0_w( space, offset, data );
	}
}


static WRITE8_HANDLER( pc_ega8_3d0_w )
{
	if ( ega.misc_output & 0x01 )
	{
		pc_ega8_3X0_w( space, offset, data );
	}
}


static READ8_HANDLER( pc_ega8_3c0_r )
{
	UINT8	dips = 0x00;	/* 0x01 - EGA only, 80x25 color(?) */
/*
0000 - 40x25
       CRTC_EGA config screen: HTOTAL: 0x1e0  VTOTAL: 0x105  MAX_X: 0x13f  MAX_Y: 0xc7  HSYNC: 0x188-0x1af  VSYNC: 0xe1-0xe3  Freq: 61.226464fps
0001 - no display (text at a0000) and writes to 0c00xx (?), graphics mode?
       CRTC_EGA config screen: HTOTAL: 0x3a8  VTOTAL: 0x105  MAX_X: 0x27f  MAX_Y: 0xc7  HSYNC: 0x2f0-0x2b7  VSYNC: 0xe1-0xe2  Freq: 60.684637fps
0010 - 40x25
       CRTC_EGA config screen: HTOTAL: 0x1e0  VTOTAL: 0x105  MAX_X: 0x13f  MAX_Y: 0xc7  HSYNC: 0x188-0x1af  VSYNC: 0xe1-0xe3  Freq: 61.226464fps
0011 - no display (text at a0000) and writes to 0c00xx (?), graphics mode?
       CRTC_EGA config screen: HTOTAL: 0x3a8  VTOTAL: 0x105  MAX_X: 0x27f  MAX_Y: 0xc7  HSYNC: 0x2f0-0x2b7  VSYNC: 0xe1-0xe2  Freq: 60.684637fps
0100 - 40x25
       CRTC_EGA config screen: HTOTAL: 0x1e0  VTOTAL: 0x105  MAX_X: 0x13f  MAX_Y: 0xc7  HSYNC: 0x188-0x1af  VSYNC: 0xe1-0xe3  Freq: 61.226464fps
0101 - no display
0110 - 40x25
0111 - no display
1000 - 40x25
1001 - no diplsay (text at a0000)
1010 - 40x25
1011 -
1100 - 40x25
1101 -
1110 - 40x25
1111 -
*/
	int data = 0xff;

	if ( VERBOSE_EGA )
	{
		logerror("pc_ega_3c0_r: offset = %02x\n", offset );
	}

	switch ( offset )
	{
	/* Attributes Controller */
	case 0:
		break;

	/* Feature Read */
	case 2:
		data = ( data & 0x0f );
		data |= ( ( ega.feature_control & 0x03 ) << 5 );
		data |= ( ega.vsync ? 0x00 : 0x80 );
		data |= ( ( ( dips >> ( ( ( ega.misc_output & 0xc0 ) >> 6 ) ) ) & 0x01 ) << 4 );
		break;

	/* Sequencer */
	case 4:
		break;
	case 5:
		break;

	/* Graphics Controller */
	case 14:
		break;
	case 15:
		break;
	}
	return data;
}


static WRITE8_HANDLER( pc_ega8_3c0_w )
{
	static const UINT8 ar_reg_mask[0x20] =
		{
			0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
			0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
			0x7F, 0x3F, 0x3F, 0x0F, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		};
	static const UINT8 sr_reg_mask[0x08] =
		{
			0x03, 0x0F, 0x0F, 0x0F, 0x07, 0x00, 0x00, 0x00
		};
	static const UINT8 gr_reg_mask[0x10] =
		{
			0x0F, 0x0F, 0x0F, 0x1F, 0x07, 0x3F, 0x0F, 0x0F,
			0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		};
	int	index = 0;

	if ( VERBOSE_EGA )
	{
		logerror("pc_ega_3c0_w: offset = %02x, data = %02x\n", offset, data );
	}

	switch ( offset )
	{
	/* Attributes Controller */
	case 0:
		if ( ega.attribute.index_write )
		{
			ega.attribute.index = data;
		}
		else
		{
			index = ega.attribute.index & 0x1F;

			logerror("AR%02X = 0x%02x\n", index, data );

			/* Clear unused bits */
			ega.attribute.data[ index ] = data & ar_reg_mask[ index ];

			switch ( index )
			{
			case 0x10:		/* AR10 */
				pc_ega_change_mode( ega.crtc_ega );
				break;
			}
		}
		ega.attribute.index_write ^= 0x01;
		break;

	/* Misccellaneous Output */
	case 2:
		ega.misc_output = data;
		pc_ega_change_mode( ega.crtc_ega );
		break;

	/* Sequencer */
	case 4:
		ega.sequencer.index = data;
		break;
	case 5:
		index = ega.sequencer.index & 0x07;

		logerror("SR%02X = 0x%02x\n", index & 0x07, data );

		/* Clear unused bits */
		ega.sequencer.data[ index ] = data & sr_reg_mask[ index ];

		switch ( index )
		{
		case 0x01:		/* SR01 */
		case 0x03:		/* SR03 */
		case 0x04:		/* SR04 */
			pc_ega_change_mode( ega.crtc_ega );
			break;
		}
		break;

	/* Graphics Controller */
	case 14:
		ega.graphics_controller.index = data;
		break;
	case 15:
		index = ega.graphics_controller.index & 0x0F;

		logerror("GR%02X = 0x%02x\n", index, data );

		/* Clear unused bits */
		ega.graphics_controller.data[ index ] = data & gr_reg_mask[ index ];

		switch ( index )
		{
		case 0x06:		/* GR06 */
			pc_ega_change_mode( ega.crtc_ega );
			pc_ega_install_banks(space->machine());
			break;
		}
		break;
	}
}

static READ16_HANDLER( pc_ega16le_3b0_r ) { return read16le_with_read8_handler(pc_ega8_3b0_r,space,  offset, mem_mask); }
static WRITE16_HANDLER( pc_ega16le_3b0_w ) { write16le_with_write8_handler(pc_ega8_3b0_w, space, offset, data, mem_mask); }
static READ32_HANDLER( pc_ega32le_3b0_r ) { return read32le_with_read8_handler(pc_ega8_3b0_r, space, offset, mem_mask); }
static WRITE32_HANDLER( pc_ega32le_3b0_w ) { write32le_with_write8_handler(pc_ega8_3b0_w, space, offset, data, mem_mask); }

static READ16_HANDLER( pc_ega16le_3c0_r ) { return read16le_with_read8_handler(pc_ega8_3c0_r,space,  offset, mem_mask); }
static WRITE16_HANDLER( pc_ega16le_3c0_w ) { write16le_with_write8_handler(pc_ega8_3c0_w, space, offset, data, mem_mask); }
static READ32_HANDLER( pc_ega32le_3c0_r ) { return read32le_with_read8_handler(pc_ega8_3c0_r, space, offset, mem_mask); }
static WRITE32_HANDLER( pc_ega32le_3c0_w ) { write32le_with_write8_handler(pc_ega8_3c0_w, space, offset, data, mem_mask); }

static READ16_HANDLER( pc_ega16le_3d0_r ) { return read16le_with_read8_handler(pc_ega8_3d0_r,space,  offset, mem_mask); }
static WRITE16_HANDLER( pc_ega16le_3d0_w ) { write16le_with_write8_handler(pc_ega8_3d0_w, space, offset, data, mem_mask); }
static READ32_HANDLER( pc_ega32le_3d0_r ) { return read32le_with_read8_handler(pc_ega8_3d0_r, space, offset, mem_mask); }
static WRITE32_HANDLER( pc_ega32le_3d0_w ) { write32le_with_write8_handler(pc_ega8_3d0_w, space, offset, data, mem_mask); }

static WRITE8_HANDLER( pc_ega_videoram_w )
{
	switch ( offset & 0x18000 )
	{
	case 0x00000:
	case 0x08000:
		if ( ega.videoram_a0000 )
		{
			ega.videoram_a0000[offset & 0xffff] = data;
		}
		break;
	case 0x10000:
		if ( ega.videoram_b0000 )
		{
			ega.videoram_b0000[offset & 0x7fff] = data;
		}
		break;
	case 0x18000:
		if ( ega.videoram_b8000 )
		{
			ega.videoram_b8000[offset & 0x7fff] = data;
		}
		break;
	}
}

static WRITE16_HANDLER( pc_ega_videoram16le_w ) { write16le_with_write8_handler(pc_ega_videoram_w, space, offset, data, mem_mask); }
static WRITE32_HANDLER( pc_ega_videoram32le_w ) { write32le_with_write8_handler(pc_ega_videoram_w, space, offset, data, mem_mask); }

