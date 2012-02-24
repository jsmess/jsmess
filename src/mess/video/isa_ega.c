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
#include "video/isa_ega.h"

#define VERBOSE_EGA		1

#define EGA_SCREEN_NAME	"ega_screen"
#define EGA_CRTC_NAME	"crtc_ega_ega"

/*
    Prototypes
*/
static SCREEN_UPDATE_IND16( pc_ega );
static CRTC_EGA_UPDATE_ROW( ega_update_row );
static CRTC_EGA_ON_DE_CHANGED( ega_de_changed );
static CRTC_EGA_ON_VSYNC_CHANGED( ega_vsync_changed );
static CRTC_EGA_ON_VBLANK_CHANGED( ega_vblank_changed );

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
	MCFG_SCREEN_RAW_PARAMS(16257000,912,0,640,262,0,200)
	MCFG_SCREEN_UPDATE_STATIC( pc_ega )

	MCFG_PALETTE_LENGTH( 64 )
	MCFG_CRTC_EGA_ADD(EGA_CRTC_NAME, 16257000/8, crtc_ega_ega_intf)
MACHINE_CONFIG_END

ROM_START( ega )
	ROM_REGION(0x4000, "user1", 0)
	ROM_LOAD("6277356.u44", 0x0000, 0x4000, CRC(dc146448) SHA1(dc0794499b3e499c5777b3aa39554bbf0f2cc19b))
	ROM_REGION(0x4000, "user2", ROMREGION_ERASE00)
ROM_END

/*
0000 - MONOC PRIMARY, EGA COLOR, 40x25
0001 - MONOC PRIMARY, EGA COLOR, 80x25
0010 - MONOC PRIMARY, EGA HI RES EMULATE (SAME AS 0001)
0011 - MONOC PRIMARY, EGA HI RES ENHANCED
0100 - COLOR 40 PRIMARY, EGA MONOCHROME
0101 - COLOR 80 PRIMARY, EGA MONOCHROME

0110 - MONOC SECONDARY, EGA COLOR, 40x24
0111 - MONOC SECONDARY, EGA COLOR, 80x25
1000 - MONOC SECONDARY, EGA HI RES EMULATE (SAME AS 0111)
1001 - MONOC SECONDARY, EGA HI RES ENHANCED
1010 - COLOR 40 SECONDARY, EGA
1011 - COLOR 80 SECONDARY, EGA

1100 - RESERVED
1101 - RESERVED
1110 - RESERVED
1111 - RESERVED
*/

INPUT_PORTS_START( ega )
	PORT_START( "config" )
	PORT_CONFNAME( 0x0f, 0x0e, "dipswitches" )
	PORT_CONFSETTING( 0x00, "0000 - MDA PRIMARY, EGA COLOR, 40x25" )
	PORT_CONFSETTING( 0x08, "0001 - MDA PRIMARY, EGA COLOR, 80x25" )
	PORT_CONFSETTING( 0x04, "0010 - MDA PRIMARY, EGA HI RES EMULATE (SAME AS 0001)" )
	PORT_CONFSETTING( 0x0c, "0011 - MDA PRIMARY, EGA HI RES ENHANCED" )
	PORT_CONFSETTING( 0x02, "0100 - CGA 40 PRIMARY, EGA MONOCHROME" )
	PORT_CONFSETTING( 0x0a, "0101 - CGA 80 PRIMARY, EGA MONOCHROME" )
	PORT_CONFSETTING( 0x06, "0110 - MDA SECONDARY, EGA COLOR, 40x25" )
	PORT_CONFSETTING( 0x0e, "0111 - MDA SECONDARY, EGA COLOR, 80x25" )
	PORT_CONFSETTING( 0x01, "1000 - MDA SECONDARY, EGA HI RES EMULATE (SAME AS 0111)" )
	PORT_CONFSETTING( 0x09, "1001 - MDA SECONDARY, EGA HI RES ENHANCED" )
	PORT_CONFSETTING( 0x05, "1010 - COLOR 40 SECONDARY, EGA" )
	PORT_CONFSETTING( 0x0d, "1011 - COLOR 80 SECONDARY, EGA" )
	PORT_CONFSETTING( 0x03, "1100 - RESERVED" )
	PORT_CONFSETTING( 0x0b, "1101 - RESERVED" )
	PORT_CONFSETTING( 0x07, "1110 - RESERVED" )
	PORT_CONFSETTING( 0x0f, "1111 - RESERVED" )
INPUT_PORTS_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ISA8_EGA = &device_creator<isa8_ega_device>;


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa8_ega_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( pcvideo_ega );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *isa8_ega_device::device_rom_region() const
{
	return ROM_NAME( ega );
}

ioport_constructor isa8_ega_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( ega );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  isa8_ega_device - constructor
//-------------------------------------------------

isa8_ega_device::isa8_ega_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, ISA8_EGA, "IBM Enhanced Graphics Adapter", tag, owner, clock),
		device_isa8_card_interface(mconfig, *this)
{
	m_shortname = "ega";
}

isa8_ega_device::isa8_ega_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock),
		device_isa8_card_interface(mconfig, *this)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa8_ega_device::device_start()
{
	set_isa_device();

	for (int i = 0; i < 64; i++ )
	{
		UINT8 r = ( ( i & 0x04 ) ? 0xAA : 0x00 ) + ( ( i & 0x20 ) ? 0x55 : 0x00 );
		UINT8 g = ( ( i & 0x02 ) ? 0xAA : 0x00 ) + ( ( i & 0x10 ) ? 0x55 : 0x00 );
		UINT8 b = ( ( i & 0x01 ) ? 0xAA : 0x00 ) + ( ( i & 0x08 ) ? 0x55 : 0x00 );

		palette_set_color_rgb( machine(), i, r, g, b );
	}
	astring tempstring;
	UINT8	*dst = machine().region(subtag(tempstring, "user2" ))->base() + 0x0000;
	UINT8	*src = machine().region(subtag(tempstring, "user1" ))->base() + 0x3fff;
	int		i;

	/* Perform the EGA bios address line swaps */
	for( i = 0; i < 0x4000; i++ )
	{
		*dst++ = *src--;
	}
	/* Install 256KB Video ram on our EGA card */
	m_videoram = auto_alloc_array(machine(), UINT8, 256*1024);

	m_crtc_ega = subdevice(EGA_CRTC_NAME);

	m_isa->install_rom(this, 0xc0000, 0xc3fff, 0, 0, "ega", "user2");
	m_isa->install_device(0x3b0, 0x3bf, 0, 0, read8_delegate(FUNC(isa8_ega_device::pc_ega8_3b0_r), this), write8_delegate(FUNC(isa8_ega_device::pc_ega8_3b0_w), this));
	m_isa->install_device(0x3c0, 0x3cf, 0, 0, read8_delegate(FUNC(isa8_ega_device::pc_ega8_3c0_r), this), write8_delegate(FUNC(isa8_ega_device::pc_ega8_3c0_w), this));
	m_isa->install_device(0x3d0, 0x3df, 0, 0, read8_delegate(FUNC(isa8_ega_device::pc_ega8_3d0_r), this), write8_delegate(FUNC(isa8_ega_device::pc_ega8_3d0_w), this));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa8_ega_device::device_reset()
{
	m_feature_control = 0;

	memset(&m_attribute,0,sizeof(m_attribute));
	memset(&m_sequencer,0,sizeof(m_sequencer));
	memset(&m_graphics_controller,0,sizeof(m_graphics_controller));

	m_frame_cnt = 0;
	m_vsync = 0;
	m_vblank = 0;
	m_display_enable = 0;

	install_banks();

	m_update_row = NULL;
	m_misc_output = 0;
	m_attribute.index_write = 1;

	/* Set up default palette */
	m_attribute.data[0] = 0;
	m_attribute.data[1] = 1;
	m_attribute.data[2] = 2;
	m_attribute.data[3] = 3;
	m_attribute.data[4] = 4;
	m_attribute.data[5] = 5;
	m_attribute.data[6] = 0x14;
	m_attribute.data[7] = 7;
	m_attribute.data[8] = 0x38;
	m_attribute.data[9] = 0x39;
	m_attribute.data[10] = 0x3A;
	m_attribute.data[11] = 0x3B;
	m_attribute.data[12] = 0x3C;
	m_attribute.data[13] = 0x3D;
	m_attribute.data[14] = 0x3E;
	m_attribute.data[15] = 0x3F;

}

void isa8_ega_device::install_banks()
{
	switch ( m_graphics_controller.data[6] & 0x0c )
	{
	case 0x00:		/* 0xA0000, 128KB */
		m_videoram_a0000 = m_videoram + 0x10000;
		m_videoram_b0000 = m_videoram;
		m_videoram_b8000 = m_videoram + 0x8000;
		break;
	case 0x04:		/* 0xA0000, 64KB */
		m_videoram_a0000 = m_videoram + 0x10000;
		m_videoram_b0000 = NULL;
		m_videoram_b8000 = NULL;
		break;
	case 0x08:		/* 0xB0000, 32KB */
		m_videoram_a0000 = NULL;
		m_videoram_b0000 = m_videoram;
		m_videoram_b8000 = NULL;
		break;
	case 0x0c:		/* 0xB8000, 32KB */
		m_videoram_a0000 = NULL;
		m_videoram_b0000 = NULL;
		m_videoram_b8000 = m_videoram;
		break;
	}
	if (m_videoram_a0000 && (m_misc_output & 0x02)) {
		m_isa->install_bank(0xa0000, 0xaffff, 0, 0, "bank11", m_videoram_a0000);
	} else {
		m_isa->unmap_bank(0xa0000, 0xaffff,0,0);
	}
	if (m_videoram_b0000 && (m_misc_output & 0x02)) {
		m_isa->install_bank(0xb0000, 0xb7fff, 0, 0, "bank12", m_videoram_b0000);
	} else {
		m_isa->unmap_bank(0xb0000, 0xb7fff,0,0);
	}
	if (m_videoram_b8000 && (m_misc_output & 0x02)) {
		m_isa->install_bank(0xb8000, 0xbffff, 0, 0, "bank13", m_videoram_b8000);
	} else {
		m_isa->unmap_bank(0xb8000, 0xbffff,0,0);
	}
}

static SCREEN_UPDATE_IND16( pc_ega )
{
	isa8_ega_device *ega = dynamic_cast<isa8_ega_device*>(screen.owner());
	crtc_ega_update( ega->m_crtc_ega, bitmap, cliprect);
	return 0;
}


static CRTC_EGA_UPDATE_ROW( ega_update_row )
{
	isa8_ega_device *ega = dynamic_cast<isa8_ega_device*>(device->owner());
	if ( ega->m_update_row )
	{
		ega->m_update_row( device, bitmap, cliprect, ma, ra, y, x_count, cursor_x, param );
	}
}


static CRTC_EGA_ON_DE_CHANGED( ega_de_changed )
{
	isa8_ega_device *ega = dynamic_cast<isa8_ega_device*>(device->owner());
	ega->m_display_enable = display_enabled ? 1 : 0;
}


static CRTC_EGA_ON_VSYNC_CHANGED( ega_vsync_changed )
{
	isa8_ega_device *ega = dynamic_cast<isa8_ega_device*>(device->owner());
	ega->m_vsync = vsync ? 8 : 0;
	if ( vsync )
	{
		ega->m_frame_cnt++;
	}
}


static CRTC_EGA_ON_VBLANK_CHANGED( ega_vblank_changed )
{
	isa8_ega_device *ega = dynamic_cast<isa8_ega_device*>(device->owner());
	ega->m_vblank = vblank ? 8 : 0;
}


static CRTC_EGA_UPDATE_ROW( pc_ega_graphics )
{
	UINT16	*p = &bitmap.pix16(y);
	int	i;

//  logerror( "pc_ega_graphics: y = %d, x_count = %d, ma = %d, ra = %d\n", y, x_count, ma, ra );

	for ( i = 0; i < x_count; i++ )
	{
		*p = 0;
	}
}


static CRTC_EGA_UPDATE_ROW( pc_ega_text )
{
	isa8_ega_device *ega = dynamic_cast<isa8_ega_device*>(device->owner());
	UINT16	*p = &bitmap.pix16(y);
	int	i;

//  logerror( "pc_ega_text: y = %d, x_count = %d, ma = %d, ra = %d\n", y, x_count, ma, ra );

	for ( i = 0; i < x_count; i++ )
	{
		UINT16	offset = ( ma + i ) << 1;
		UINT8	chr = ega->m_videoram[ offset ];
		UINT8	attr = ega->m_videoram[ offset + 1 ];
		UINT8	data = ( attr & 0x08 ) ? ega->m_charA[ chr * 32 + ra ] : ega->m_charB[ chr * 32 + ra ];
		UINT16	fg = ega->m_attribute.data[ attr & 0x0F ];
		UINT16	bg = ega->m_attribute.data[ attr >> 4 ];

		if ( i == cursor_x && ega->m_frame_cnt & 0x08 )
		{
			data = 0xFF;
		}

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


void isa8_ega_device::change_mode()
{
	int clock, pixels;

	m_update_row = NULL;

	/* Check for graphics mode */
	if (   ( m_attribute.data[0x10] & 0x01 ) &&
	     ! ( m_sequencer.data[0x04] & 0x01 ) &&
	       ( m_graphics_controller.data[0x06] & 0x01 ) )
	{
		if ( VERBOSE_EGA )
		{
			logerror("change_mode(): Switch to graphics mode\n");
		}

		m_update_row = pc_ega_graphics;
	}

	/* Check for text mode */
	if ( ! ( m_attribute.data[0x10] & 0x01 ) &&
	       ( m_sequencer.data[0x04] & 0x01 ) &&
	     ! ( m_graphics_controller.data[0x06] & 0x01 ) )
	{
		if ( VERBOSE_EGA )
		{
			logerror("chnage_mode(): Switching to text mode\n");
		}

		m_update_row = pc_ega_text;

		/* Set character maps */
		if ( m_sequencer.data[0x04] & 0x02 )
		{
			m_charA = m_videoram + 0x10000 + ( ( m_sequencer.data[0x03] & 0x0c ) >> 1 ) * 0x2000;
			m_charB = m_videoram + 0x10000 + ( m_sequencer.data[0x03] & 0x03 ) * 0x2000;
		}
		else
		{
			m_charA = m_videoram + 0x10000;
			m_charB = m_videoram + 0x10000;
		}
	}

	/* Check for changes to the crtc input clock and number of pixels per clock */
	clock = ( ( m_misc_output & 0x0c ) ? 16257000 : XTAL_14_31818MHz );
	pixels = ( ( m_sequencer.data[0x01] & 0x01 ) ? 8 : 9 );

	if ( m_sequencer.data[0x01] & 0x08 )
	{
		clock >>= 1;
	}
	crtc_ega_set_clock( m_crtc_ega, clock / pixels );
	crtc_ega_set_hpixels_per_column( m_crtc_ega, pixels );

if ( ! m_update_row )
	logerror("unknown video mode\n");
}


UINT8 isa8_ega_device::pc_ega8_3X0_r(UINT8 offset)
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
		data = crtc_ega_register_r( m_crtc_ega, offset );
		break;

	/* Input Status Register 1 */
	case 10:
		data = m_vblank | m_display_enable;

		if ( m_display_enable )
		{
			/* For the moment i'm putting in some bogus data */
			static int pixel_data;

			pixel_data = ( pixel_data + 1 ) & 0x03;
			data |= ( pixel_data << 4 );
		}

		/* Reset the attirubte writing flip flop to let the next write go to the index reigster */
		m_attribute.index_write = 1;
		break;
	}

	return data;
}

void isa8_ega_device::pc_ega8_3X0_w(UINT8 offset, UINT8 data)
{
	if ( VERBOSE_EGA )
	{
		logerror("pc_ega_3X0_w: offset = %02x, data = %02x\n", offset, data );
	}

	switch ( offset )
	{
	/* CRT Controller - address register */
	case 0: case 2: case 4: case 6:
		crtc_ega_address_w( m_crtc_ega, offset, data );
		break;

	/* CRT Controller - data register */
	case 1: case 3: case 5: case 7:
		crtc_ega_register_w( m_crtc_ega, offset, data );
		break;

	/* Set Light Pen Flip Flop */
	case 9:
		break;

	/* Feature Control */
	case 10:
		m_feature_control = data;
		break;

	/* Clear Light Pen Flip Flop */
	case 11:
		break;
	}
}



READ8_MEMBER(isa8_ega_device::pc_ega8_3b0_r )
{
	return ( m_misc_output & 0x01 ) ? 0xFF : pc_ega8_3X0_r(offset);
}


READ8_MEMBER(isa8_ega_device::pc_ega8_3d0_r )
{
	return ( m_misc_output & 0x01 ) ? pc_ega8_3X0_r(offset) : 0xFF;
}


WRITE8_MEMBER(isa8_ega_device::pc_ega8_3b0_w )
{
	if ( ! ( m_misc_output & 0x01 ) )
	{
		pc_ega8_3X0_w(offset, data );
	}
}


WRITE8_MEMBER(isa8_ega_device::pc_ega8_3d0_w )
{
	if ( m_misc_output & 0x01 )
	{
		pc_ega8_3X0_w(offset, data );
	}
}


READ8_MEMBER(isa8_ega_device::pc_ega8_3c0_r )
{
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
		{
			UINT8 dips = input_port_read(*this, "config" );

			data = ( data & 0x0f );
			data |= ( ( m_feature_control & 0x03 ) << 5 );
			data |= ( m_vsync ? 0x00 : 0x80 );
			data |= ( ( ( dips >> ( ( ( m_misc_output & 0x0c ) >> 2 ) ) ) & 0x01 ) << 4 );
		}
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


WRITE8_MEMBER(isa8_ega_device::pc_ega8_3c0_w )
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
		if ( m_attribute.index_write )
		{
			m_attribute.index = data;
		}
		else
		{
			index = m_attribute.index & 0x1F;

			logerror("AR%02X = 0x%02x\n", index, data );

			/* Clear unused bits */
			m_attribute.data[ index ] = data & ar_reg_mask[ index ];

			switch ( index )
			{
			case 0x10:		/* AR10 */
				change_mode();
				break;
			}
		}
		m_attribute.index_write ^= 0x01;
		break;

	/* Misccellaneous Output */
	case 2:
		m_misc_output = data;
		install_banks();
		change_mode();
		break;

	/* Sequencer */
	case 4:
		m_sequencer.index = data;
		break;
	case 5:
		index = m_sequencer.index & 0x07;

		logerror("SR%02X = 0x%02x\n", index & 0x07, data );

		/* Clear unused bits */
		m_sequencer.data[ index ] = data & sr_reg_mask[ index ];

		switch ( index )
		{
		case 0x01:		/* SR01 */
		case 0x03:		/* SR03 */
		case 0x04:		/* SR04 */
			change_mode();
			break;
		}
		break;

	/* Graphics Controller */
	case 14:
		m_graphics_controller.index = data;
		break;
	case 15:
		index = m_graphics_controller.index & 0x0F;

		logerror("GR%02X = 0x%02x\n", index, data );

		/* Clear unused bits */
		m_graphics_controller.data[ index ] = data & gr_reg_mask[ index ];

		switch ( index )
		{
		case 0x06:		/* GR06 */
			change_mode();
			install_banks();
			break;
		}
		break;
	}
}
