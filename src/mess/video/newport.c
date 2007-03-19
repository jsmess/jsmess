/*
	SGI "Newport" graphics board used in the Indy and some Indigo2s

	Newport is modular, consisting of the following custom chips:
	- REX3: Raster Engine, which is basically a blitter which can also draw antialiased lines.
	        REX also acts as the interface to the rest of the system - all the other chips on
		a Newport board are accessed through it.
	- RB2: Frame buffer input controller
	- RO1: Frame buffer output controller
	- XMAP9: Final display generator
	- CMAP: Palette mapper
	- VC2: Video timing controller / CRTC

	Taken from the Linux Newport driver, slave addresses for Newport devices are:
			VC2			0
			Both CMAPs	1
			CMAP 0		2
			CMAP 1		3
			Both XMAPs	4
			XMAP 0		5
			XMAP 1		6
			RAMDAC		7
			VIDEO (CC1)	8
			VIDEO (AB1)	9
*/

#include "driver.h"
#include "video/newport.h"

#define VERBOSE_LEVEL ( 0 )

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", activecpu_get_pc(), buf );
	}
}

static READ32_HANDLER( newport_cmap0_r );
static WRITE32_HANDLER( newport_cmap0_w );
static READ32_HANDLER( newport_cmap1_r );
static READ32_HANDLER( newport_xmap0_r );
static WRITE32_HANDLER( newport_xmap0_w );
static READ32_HANDLER( newport_xmap1_r );
static WRITE32_HANDLER( newport_xmap1_w );
static READ32_HANDLER( newport_vc2_r );
static WRITE32_HANDLER( newport_vc2_w );

static UINT16 nVC2_Register[0x21];
static UINT16 nVC2_RAM[0x8000];
static UINT8 nVC2_RegIdx;
static UINT16 nVC2_RegData;

#define VC2_VIDENTRY		nVC2_Register[0x00]
#define VC2_CURENTRY		nVC2_Register[0x01]
#define VC2_CURSORX			nVC2_Register[0x02]
#define VC2_CURSORY			nVC2_Register[0x03]
#define VC2_CURCURSORX		nVC2_Register[0x04]
#define VC2_DIDENTRY		nVC2_Register[0x05]
#define VC2_SCANLINELEN		nVC2_Register[0x06]
#define VC2_RAMADDR			nVC2_Register[0x07]
#define VC2_VTFRAMEPTR		nVC2_Register[0x08]
#define VC2_VTLINEPTR		nVC2_Register[0x09]
#define VC2_VTLINERUN		nVC2_Register[0x0a]
#define VC2_VLINECNT		nVC2_Register[0x0b]
#define VC2_CURTABLEPTR		nVC2_Register[0x0c]
#define VC2_WORKCURSORY		nVC2_Register[0x0d]
#define VC2_DIDFRAMEPTR		nVC2_Register[0x0e]
#define VC2_DIDLINEPTR		nVC2_Register[0x0f]
#define VC2_DISPLAYCTRL		nVC2_Register[0x10]
#define VC2_CONFIG			nVC2_Register[0x1f]

static UINT32 nXMAP0_Register[0x08];
static UINT32 nXMAP0_ModeTable[0x20];

#define XMAP0_CONFIG		nXMAP0_Register[0x00]
#define XMAP0_REVISION		nXMAP0_Register[0x01]
#define XMAP0_ENTRIES		nXMAP0_Register[0x02]
#define XMAP0_CURCMAP		nXMAP0_Register[0x03]
#define XMAP0_POPUPCMAP		nXMAP0_Register[0x04]
#define XMAP0_MODETBLIDX	nXMAP0_Register[0x07]

static UINT32 nXMAP1_Register[0x08];
static UINT32 nXMAP1_ModeTable[0x20];

#define XMAP1_CONFIG		nXMAP1_Register[0x00]
#define XMAP1_REVISION		nXMAP1_Register[0x01]
#define XMAP1_ENTRIES		nXMAP1_Register[0x02]
#define XMAP1_CURCMAP		nXMAP1_Register[0x03]
#define XMAP1_POPUPCMAP		nXMAP1_Register[0x04]
#define XMAP1_MODETBLIDX	nXMAP1_Register[0x07]

static UINT32 nREX3_DrawMode1;
static UINT32 nREX3_DrawMode0;
static UINT32 nREX3_LSMode;
static UINT32 nREX3_LSPattern;
static UINT32 nREX3_LSPatSave;
static UINT32 nREX3_ZPattern;
static UINT32 nREX3_ColorBack;
static UINT32 nREX3_ColorVRAM;
static UINT32 nREX3_AlphaRef;
//static UINT32 nREX3_Stall0;
static UINT32 nREX3_SMask0X;
static UINT32 nREX3_SMask0Y;
static UINT32 nREX3_Setup;
static UINT32 nREX3_StepZ;
static UINT32 nREX3_XStart;
static UINT32 nREX3_YStart;
static UINT32 nREX3_XEnd;
static UINT32 nREX3_YEnd;
static UINT32 nREX3_XSave;
static UINT32 nREX3_XYMove;
static UINT32 nREX3_BresD;
static UINT32 nREX3_BresS1;
static UINT32 nREX3_BresOctInc1;
static UINT32 nREX3_BresRndInc2;
static UINT32 nREX3_BresE1;
static UINT32 nREX3_BresS2;
static UINT32 nREX3_AWeight0;
static UINT32 nREX3_AWeight1;
static UINT32 nREX3_XStartF;
static UINT32 nREX3_YStartF;
static UINT32 nREX3_XEndF;
static UINT32 nREX3_YEndF;
static UINT32 nREX3_XStartI;
//static UINT32 nREX3_YEndF1;
static UINT32 nREX3_XYStartI;
static UINT32 nREX3_XYEndI;
static UINT32 nREX3_XStartEndI;
static UINT32 nREX3_ColorRed;
static UINT32 nREX3_ColorAlpha;
static UINT32 nREX3_ColorGreen;
static UINT32 nREX3_ColorBlue;
static UINT32 nREX3_SlopeRed;
static UINT32 nREX3_SlopeAlpha;
static UINT32 nREX3_SlopeGreen;
static UINT32 nREX3_SlopeBlue;
static UINT32 nREX3_WriteMask;
static UINT32 nREX3_ZeroFract;
static UINT32 nREX3_ZeroOverflow;
//static UINT32 nREX3_ColorIndex;
static UINT32 nREX3_HostDataPortMSW;
static UINT32 nREX3_HostDataPortLSW;
static UINT32 nREX3_DCBMode;
static UINT32 nREX3_DCBRegSelect;
static UINT32 nREX3_DCBSlvSelect;
static UINT32 nREX3_DCBDataMSW;
static UINT32 nREX3_DCBDataLSW;
static UINT32 nREX3_SMask1X;
static UINT32 nREX3_SMask1Y;
static UINT32 nREX3_SMask2X;
static UINT32 nREX3_SMask2Y;
static UINT32 nREX3_SMask3X;
static UINT32 nREX3_SMask3Y;
static UINT32 nREX3_SMask4X;
static UINT32 nREX3_SMask4Y;
static UINT32 nREX3_TopScanline;
static UINT32 nREX3_XYWin;
static UINT32 nREX3_ClipMode;
static UINT32 nREX3_Config;
static UINT32 nREX3_Status;
static UINT8 nREX3_XFerWidth;
/*static UINT32 nREX3_CurrentX;
static UINT32 nREX3_CurrentY;*/
static UINT32 nREX3_Kludge_SkipLine;

static UINT32 *video_base;

static UINT8 nDrawGreen;

VIDEO_START( newport )
{
	nDrawGreen = 0;
	nREX3_DrawMode0 = 0x00000000;
	nREX3_DrawMode1 = 0x3002f001;
	nREX3_DCBMode = 0x00000780;
	nREX3_Kludge_SkipLine = 0;
	video_base = auto_malloc( (1280+64) * (1024+64) * 4 );
	memset( video_base, 0x00, (1280+64) * (1024+64) * 4 );
	return 0;
}

VIDEO_UPDATE( newport )
{
	int y;

	fillbitmap( bitmap, get_black_pen(machine), cliprect );

	/* loop over rows and copy to the destination */
	for( y = cliprect->min_y; y <= cliprect->max_y; y++ )
	{
		UINT32 *src = &video_base[1344 * y];
		UINT16 *dest = BITMAP_ADDR16(bitmap, y, cliprect->min_x);
		int x;

		/* loop over columns */
		for( x = cliprect->min_x; x < cliprect->max_x; x++ )
		{
			UINT32 nPix = *src++;
			*dest++ = ( ( nPix & 0x00f80000 ) >> 9 ) | ( ( nPix & 0x0000f800 ) >> 6 ) | ( ( nPix & 0x000000f8 ) >> 3 );
		}
	}
	return 0;
}

UINT16 nCMAP0_PaletteIndex;
UINT32 nCMAP0_Palette[0x10000];

static WRITE32_HANDLER( newport_cmap0_w )
{
	switch( nREX3_DCBRegSelect )
	{
	case 0x00:
		verboselog( 2, "CMAP0 Palette Index Write: %04x\n", data & 0x0000ffff );
		nCMAP0_PaletteIndex = data & 0x0000ffff;
		break;
	case 0x02:
		verboselog( 2, "CMAP0 Palette Entry %04x Write: %08x\n", nCMAP0_PaletteIndex, ( data >> 8 ) & 0x00ffffff );
		nCMAP0_Palette[nCMAP0_PaletteIndex] = ( data >> 8 ) & 0x00ffffff;
		break;
	default:
		verboselog( 2, "Unknown CMAP0 Register %d Write: %08x\n", nREX3_DCBRegSelect, data );
		break;
	}
}

static READ32_HANDLER( newport_cmap0_r )
{
	switch( nREX3_DCBRegSelect )
	{
	case 0x04:
		verboselog( 2, "CMAP0 Status Read: %08x\n", 0x00000008 );
		return 0x00000008;
		break;
	case 0x06: /* Revision */
		verboselog( 2, "CMAP0 Revision Read: CMAP Rev 1, Board Rev 2, 8bpp\n" );
		return 0x000000a1;
		break;
	default:
		verboselog( 2, "Unknown CMAP0 Register %d Read\n", nREX3_DCBRegSelect );
		return 0x00000000;
		break;
	}
}

static READ32_HANDLER( newport_cmap1_r )
{
	switch( nREX3_DCBRegSelect )
	{
	case 0x04:
		verboselog( 2, "CMAP1 Status Read: %08x\n", 0x00000008 );
		return 0x00000008;
		break;
	case 0x06: /* Revision */
		verboselog( 2, "CMAP1 Revision Read: CMAP Rev 1, Board Rev 2, 8bpp\n" );
		return 0x000000a1;
		break;
	default:
		verboselog( 2, "Unknown CMAP0 Register %d Read\n", nREX3_DCBRegSelect );
		return 0x00000000;
		break;
	}
}

static READ32_HANDLER( newport_xmap0_r )
{
	UINT8 nModeIdx;
	switch( nREX3_DCBRegSelect )
	{
	case 0:
		verboselog( 2, "XMAP0 Config Read: %08x\n", XMAP0_CONFIG );
		return XMAP0_CONFIG;
		break;
	case 1:
		verboselog( 2, "XMAP0 Revision Read: %08x\n", 0x00 );
		return 0x00000000;
		break;
	case 2:
		verboselog( 2, "XMAP0 FIFO Availability Read: %08x\n", 0x02 );
		return 0x00000002;
		break;
	case 3:
		verboselog( 2, "XMAP0 Cursor CMAP MSB Read: %08x\n", XMAP0_CURCMAP );
		return XMAP0_CURCMAP;
		break;
	case 4:
		verboselog( 2, "XMAP0 Pop Up CMAP MSB Read: %08x\n", XMAP0_POPUPCMAP );
		return XMAP0_POPUPCMAP;
		break;
	case 5:
		nModeIdx = ( XMAP0_MODETBLIDX & 0x0000007c ) >> 2;
		switch( XMAP0_MODETBLIDX & 0x00000003 )
		{
		case 0:
			verboselog( 2, "XMAP0 Mode Register Read: %02x (Byte 0): %08x\n", nModeIdx, ( nXMAP0_ModeTable[ nModeIdx ] & 0x00ff0000 ) >> 16 );
			return ( nXMAP0_ModeTable[ nModeIdx ] & 0x00ff0000 ) >> 16;
			break;
		case 1:
			verboselog( 2, "XMAP0 Mode Register Read: %02x (Byte 1): %08x\n", nModeIdx, ( nXMAP0_ModeTable[ nModeIdx ] & 0x0000ff00 ) >>  8 );
			return ( nXMAP0_ModeTable[ nModeIdx ] & 0x0000ff00 ) >>  8;
			break;
		case 2:
			verboselog( 2, "XMAP0 Mode Register Read: %02x (Byte 2): %08x\n", nModeIdx, ( nXMAP0_ModeTable[ nModeIdx ] & 0x000000ff ) );
			return ( nXMAP0_ModeTable[ nModeIdx ] & 0x000000ff );
			break;
		}
		break;
	case 6:
		verboselog( 2, "XMAP0 Unused Read: %08x\n", 0x00000000 );
		return 0x00000000;
		break;
	case 7:
		verboselog( 2, "XMAP0 Mode Table Address Read: %08x\n", XMAP0_MODETBLIDX );
		return XMAP0_MODETBLIDX;
		break;
	}

	verboselog( 2, "XMAP0 Unknown nREX3_DCBRegSelect Value: %02x, returning 0\n", nREX3_DCBRegSelect );
	return 0x00000000;
}

static WRITE32_HANDLER( newport_xmap0_w )
{
	UINT8 n8BitVal = data & 0x000000ff;
	switch( nREX3_DCBRegSelect )
	{
	case 0:
		verboselog( 2, "XMAP0 Config Write: %02x\n", n8BitVal );
		XMAP0_CONFIG = n8BitVal;
		break;
	case 1:
		verboselog( 2, "XMAP0 Revision Write (Ignored): %02x\n", n8BitVal );
		break;
	case 2:
		verboselog( 2, "XMAP0 FIFO Availability Write (Ignored): %02x\n", n8BitVal );
		break;
	case 3:
		verboselog( 2, "XMAP0 Cursor CMAP MSB Write: %02x\n", n8BitVal );
		XMAP0_CURCMAP = n8BitVal;
		break;
	case 4:
		verboselog( 2, "XMAP0 Pop Up CMAP MSB Write: %02x\n", n8BitVal );
		XMAP0_POPUPCMAP = n8BitVal;
		break;
	case 5:
		verboselog( 2, "XMAP0 Mode Register Write: %02x = %06x\n", ( data & 0xff000000 ) >> 24, data & 0x00ffffff );
		nXMAP0_ModeTable[ ( data & 0xff000000 ) >> 24 ] = data & 0x00ffffff;
		break;
	case 6:
		verboselog( 2, "XMAP0 Unused Write (Ignored): %08x\n", data );
		break;
	case 7:
		verboselog( 2, "XMAP0 Mode Table Address Write: %02x\n", n8BitVal );
		XMAP0_MODETBLIDX = n8BitVal;
		break;
	}
}

static READ32_HANDLER( newport_xmap1_r )
{
	UINT8 nModeIdx;
	switch( nREX3_DCBRegSelect )
	{
	case 0:
		verboselog( 2, "XMAP1 Config Read: %08x\n", XMAP1_CONFIG );
		return XMAP1_CONFIG;
		break;
	case 1:
		verboselog( 2, "XMAP1 Revision Read: %08x\n", 0x00 );
		return 0x00000000;
		break;
	case 2:
		verboselog( 2, "XMAP1 FIFO Availability Read: %08x\n", 0x02 );
		return 0x00000002;
		break;
	case 3:
		verboselog( 2, "XMAP1 Cursor CMAP MSB Read: %08x\n", XMAP1_CURCMAP );
		return XMAP1_CURCMAP;
		break;
	case 4:
		verboselog( 2, "XMAP1 Pop Up CMAP MSB Read: %08x\n", XMAP1_POPUPCMAP );
		return XMAP1_POPUPCMAP;
		break;
	case 5:
		nModeIdx = ( XMAP1_MODETBLIDX & 0x0000007c ) >> 2;
		switch( XMAP1_MODETBLIDX & 0x00000003 )
		{
		case 0:
			verboselog( 2, "XMAP1 Mode Register Read: %02x (Byte 0): %08x\n", nModeIdx, ( nXMAP1_ModeTable[ nModeIdx ] & 0x00ff0000 ) >> 16 );
			return ( nXMAP1_ModeTable[ nModeIdx ] & 0x00ff0000 ) >> 16;
			break;
		case 1:
			verboselog( 2, "XMAP1 Mode Register Read: %02x (Byte 1): %08x\n", nModeIdx, ( nXMAP1_ModeTable[ nModeIdx ] & 0x0000ff00 ) >>  8 );
			return ( nXMAP1_ModeTable[ nModeIdx ] & 0x0000ff00 ) >>  8;
			break;
		case 2:
			verboselog( 2, "XMAP1 Mode Register Read: %02x (Byte 2): %08x\n", nModeIdx, ( nXMAP1_ModeTable[ nModeIdx ] & 0x000000ff ) );
			return ( nXMAP1_ModeTable[ nModeIdx ] & 0x000000ff );
			break;
		}
		break;
	case 6:
		verboselog( 2, "XMAP1 Unused Read: %08x\n", 0x00000000 );
		return 0x00000000;
		break;
	case 7:
		verboselog( 2, "XMAP1 Mode Table Address Read: %08x\n", XMAP0_MODETBLIDX );
		return XMAP1_MODETBLIDX;
		break;
	}

	verboselog( 2, "XMAP1 Unknown nREX3_DCBRegSelect Value: %02x, returning 0\n", nREX3_DCBRegSelect );
	return 0x00000000;
}

static WRITE32_HANDLER( newport_xmap1_w )
{
	UINT8 n8BitVal = data & 0x000000ff;
	switch( nREX3_DCBRegSelect )
	{
	case 0:
		verboselog( 2, "XMAP1 Config Write: %02x\n", n8BitVal );
		XMAP1_CONFIG = n8BitVal;
		break;
	case 1:
		verboselog( 2, "XMAP1 Revision Write (Ignored): %02x\n", n8BitVal );
		break;
	case 2:
		verboselog( 2, "XMAP1 FIFO Availability Write (Ignored): %02x\n", n8BitVal );
		break;
	case 3:
		verboselog( 2, "XMAP1 Cursor CMAP MSB Write: %02x\n", n8BitVal );
		XMAP1_CURCMAP = n8BitVal;
		break;
	case 4:
		verboselog( 2, "XMAP1 Pop Up CMAP MSB Write: %02x\n", n8BitVal );
		XMAP1_POPUPCMAP = n8BitVal;
		break;
	case 5:
		verboselog( 2, "XMAP1 Mode Register Write: %02x = %06x\n", ( data & 0xff000000 ) >> 24, data & 0x00ffffff );
		nXMAP1_ModeTable[ ( data & 0xff000000 ) >> 24 ] = data & 0x00ffffff;
		break;
	case 6:
		verboselog( 2, "XMAP1 Unused Write (Ignored): %08x\n", data );
		break;
	case 7:
		verboselog( 2, "XMAP1 Mode Table Address Write: %02x\n", n8BitVal );
		XMAP1_MODETBLIDX = n8BitVal;
		break;
	}
}

static READ32_HANDLER( newport_vc2_r )
{
	UINT16 ret16;
	switch( nREX3_DCBRegSelect )
	{
	case 0x01: /* Register Read */
		verboselog( 2, "VC2 Register Read: %02x, %08x\n", nVC2_RegIdx, nVC2_Register[nVC2_RegIdx] );
		return nVC2_Register[nVC2_RegIdx];
		break;
	case 0x03: /* RAM Read */
		verboselog( 2, "VC2 RAM Read: %04x = %08x\n", VC2_RAMADDR, nVC2_RAM[VC2_RAMADDR] );
		ret16 = nVC2_RAM[VC2_RAMADDR];
		VC2_RAMADDR++;
		if( VC2_RAMADDR == 0x8000 )
		{
			VC2_RAMADDR = 0x0000;
		}
		return ret16;
		break;
	default:
		verboselog( 2, "Unknown VC2 Register Read: %02x\n", nREX3_DCBRegSelect );
		return 0;
		break;
	}
	return 0;
}

static WRITE32_HANDLER( newport_vc2_w )
{
	switch( nREX3_XFerWidth )
	{
	case 0x01: /* Register Select */
		switch( nREX3_DCBRegSelect )
		{
		case 0x00:
			nVC2_RegIdx = ( data & 0x000000ff ) >> 0;
			verboselog( 2, "VC2 Register Select: %02x\n", nVC2_RegIdx );
			break;
		default:
			verboselog( 2, "Unknown VC2 Register Select: DCB Register %02x, data = 0x%08x\n", nREX3_DCBRegSelect, data );
			break;
		}
		break;
	case 0x02: /* RAM Write */
		switch( nREX3_DCBRegSelect )
		{
		case 0x03:
			verboselog( 2, "VC2 RAM Write: %04x = %08x\n", VC2_RAMADDR, data & 0x0000ffff );
			nVC2_RAM[VC2_RAMADDR] = data & 0x0000ffff;
			VC2_RAMADDR++;
			if( VC2_RAMADDR == 0x8000 )
			{
				VC2_RAMADDR = 0x0000;
			}
			break;
		default:
			verboselog( 2, "Unknown 2-byte Write: DCB Register %02x, data = 0x%08x\n", nREX3_DCBRegSelect, data );
			break;
		}
		break;
	case 0x03: /* Register Write */
	switch( nREX3_DCBRegSelect )
	{
	case 0x00:
		verboselog( 2, "VC2 Register Setup:\n" );
		nVC2_RegIdx = ( data & 0xff000000 ) >> 24;
		nVC2_RegData = ( data & 0x00ffff00 ) >> 8;
		switch( nVC2_RegIdx )
		{
		case 0x00:
			verboselog( 2, "    Video Entry Pointer:  %04x\n", nVC2_RegData );
			break;
		case 0x01:
			verboselog( 2, "    Cursor Entry Pointer: %04x\n", nVC2_RegData );
			break;
		case 0x02:
			verboselog( 2, "    Cursor X Location:    %04x\n", nVC2_RegData );
			break;
		case 0x03:
			verboselog( 2, "    Cursor Y Location:    %04x\n", nVC2_RegData );
			break;
		case 0x04:
			verboselog( 2, "    Current Cursor X:     %04x\n", nVC2_RegData );
			break;
		case 0x05:
			verboselog( 2, "    DID Entry Pointer:    %04x\n", nVC2_RegData );
			break;
		case 0x06:
			verboselog( 2, "    Scanline Length:      %04x\n", nVC2_RegData );
			break;
		case 0x07:
			verboselog( 2, "    RAM Address:          %04x\n", nVC2_RegData );
			break;
		case 0x08:
			verboselog( 2, "    VT Frame Table Ptr:   %04x\n", nVC2_RegData );
			break;
		case 0x09:
			verboselog( 2, "    VT Line Sequence Ptr: %04x\n", nVC2_RegData );
			break;
		case 0x0a:
			verboselog( 2, "    VT Lines in Run:      %04x\n", nVC2_RegData );
			break;
		case 0x0b:
			verboselog( 2, "    Vertical Line Count:  %04x\n", nVC2_RegData );
			break;
		case 0x0c:
			verboselog( 2, "    Cursor Table Ptr:     %04x\n", nVC2_RegData );
			break;
		case 0x0d:
			verboselog( 2, "    Working Cursor Y:     %04x\n", nVC2_RegData );
			break;
		case 0x0e:
			verboselog( 2, "    DID Frame Table Ptr:  %04x\n", nVC2_RegData );
			break;
		case 0x0f:
			verboselog( 2, "    DID Line Table Ptr:   %04x\n", nVC2_RegData );
			break;
		case 0x10:
			verboselog( 2, "    Display Control:      %04x\n", nVC2_RegData );
			break;
		case 0x1f:
			verboselog( 2, "    Configuration:        %04x\n", nVC2_RegData );
				nVC2_Register[0x20] = nVC2_RegData;
			break;
		default:
			verboselog( 2, "    Unknown VC2 Register: %04x\n", nVC2_RegData );
			break;
		}
			nVC2_Register[nVC2_RegIdx] = nVC2_RegData;
		break;
		default:
			verboselog( 2, "Unknown VC2 Register Write: %02x = %08x\n", nREX3_DCBRegSelect, data );
		break;
		}
		break;
	default:
		verboselog( 2, "Unknown VC2 XFer Width: Width %02x, DCB Register %02x, Value 0x%08x\n", nREX3_XFerWidth, nREX3_DCBRegSelect, data );
		break;
	}
}

READ32_HANDLER( newport_rex3_r )
{
	UINT32 nTemp;
//	if( offset >= ( 0x0800 / 4 ) )
//	{
//		verboselog( 2, "%08x:\n", 0xbf0f0000 + ( offset << 2 ) );
//	}
	switch( offset )
	{
	case 0x0000/4:
	case 0x0800/4:
		verboselog( 2, "REX3 Draw Mode 1 Read: %08x\n", nREX3_DrawMode1 );
		return nREX3_DrawMode1;
		break;
	case 0x0004/4:
	case 0x0804/4:
		verboselog( 2, "REX3 Draw Mode 0 Read: %08x\n", nREX3_DrawMode0 );
		return nREX3_DrawMode0;
		break;
	case 0x0008/4:
	case 0x0808/4:
		verboselog( 2, "REX3 Line Stipple Mode Read: %08x\n", nREX3_LSMode );
		return nREX3_LSMode;
		break;
	case 0x000c/4:
	case 0x080c/4:
		verboselog( 2, "REX3 Line Stipple Pattern Read: %08x\n", nREX3_LSPattern );
		return nREX3_LSPattern;
		break;
	case 0x0010/4:
	case 0x0810/4:
		verboselog( 2, "REX3 Line Stipple Pattern (Save) Read: %08x\n", nREX3_LSPatSave );
		return nREX3_LSPatSave;
		break;
	case 0x0014/4:
	case 0x0814/4:
		verboselog( 2, "REX3 Pattern Register Read: %08x\n", nREX3_ZPattern );
		return nREX3_ZPattern;
		break;
	case 0x0018/4:
	case 0x0818/4:
		verboselog( 2, "REX3 Opaque Pattern / Blendfunc Dest Color Read: %08x\n", nREX3_ColorBack );
		return nREX3_ColorBack;
		break;
	case 0x001c/4:
	case 0x081c/4:
		verboselog( 2, "REX3 VRAM Fastclear Color Read: %08x\n", nREX3_ColorVRAM );
		return nREX3_ColorVRAM;
		break;
	case 0x0020/4:
	case 0x0820/4:
		verboselog( 2, "REX3 AFUNCTION Reference Alpha Read: %08x\n", nREX3_AlphaRef );
		return nREX3_AlphaRef;
		break;
	case 0x0028/4:
	case 0x0828/4:
		verboselog( 2, "REX3 Screenmask 0 X Min/Max Read: %08x\n", nREX3_SMask0X );
		return nREX3_SMask0X;
		break;
	case 0x002c/4:
	case 0x082c/4:
		verboselog( 2, "REX3 Screenmask 0 Y Min/Max Read: %08x\n", nREX3_SMask0Y );
		return nREX3_SMask0Y;
		break;
	case 0x0030/4:
	case 0x0830/4:
		verboselog( 2, "REX3 Line/Span Setup Read: %08x\n", nREX3_Setup );
		return nREX3_Setup;
		break;
	case 0x0034/4:
	case 0x0834/4:
		verboselog( 2, "REX3 ZPattern Enable Read: %08x\n", nREX3_StepZ );
		return nREX3_StepZ;
		break;
	case 0x0100/4:
	case 0x0900/4:
		verboselog( 2, "REX3 X Start Read: %08x\n", nREX3_XStart );
		return nREX3_XStart;
		break;
	case 0x0104/4:
	case 0x0904/4:
		verboselog( 2, "REX3 YStart Read: %08x\n", nREX3_YStart );
		return nREX3_YStart;
		break;
	case 0x0108/4:
	case 0x0908/4:
		verboselog( 2, "REX3 XEnd Read: %08x\n", nREX3_XEnd );
		return nREX3_XEnd;
		break;
	case 0x010c/4:
	case 0x090c/4:
		verboselog( 2, "REX3 YEnd Read: %08x\n", nREX3_YEnd );
		return nREX3_YEnd;
		break;
	case 0x0110/4:
	case 0x0910/4:
		verboselog( 2, "REX3 XSave Read: %08x\n", nREX3_XSave );
		return nREX3_XSave;
		break;
	case 0x0114/4:
	case 0x0914/4:
		verboselog( 2, "REX3 XYMove Read: %08x\n", nREX3_XYMove );
		return nREX3_XYMove;
		break;
	case 0x0118/4:
	case 0x0918/4:
		verboselog( 2, "REX3 Bresenham D Read: %08x\n", nREX3_BresD );
		return nREX3_BresD;
		break;
	case 0x011c/4:
	case 0x091c/4:
		verboselog( 2, "REX3 Bresenham S1 Read: %08x\n", nREX3_BresS1 );
		return nREX3_BresS1;
		break;
	case 0x0120/4:
	case 0x0920/4:
		verboselog( 2, "REX3 Bresenham Octant & Incr1 Read: %08x\n", nREX3_BresOctInc1 );
		return nREX3_BresOctInc1;
		break;
	case 0x0124/4:
	case 0x0924/4:
		verboselog( 2, "REX3 Bresenham Octant Rounding Mode & Incr2 Read: %08x\n", nREX3_BresRndInc2 );
		return nREX3_BresRndInc2;
		break;
	case 0x0128/4:
	case 0x0928/4:
		verboselog( 2, "REX3 Bresenham E1 Read: %08x\n", nREX3_BresE1 );
		return nREX3_BresE1;
		break;
	case 0x012c/4:
	case 0x092c/4:
		verboselog( 2, "REX3 Bresenham S2 Read: %08x\n", nREX3_BresS2 );
		return nREX3_BresS2;
		break;
	case 0x0130/4:
	case 0x0930/4:
		verboselog( 2, "REX3 AA Line Weight Table 1/2 Read: %08x\n", nREX3_AWeight0 );
		return nREX3_AWeight0;
		break;
	case 0x0134/4:
	case 0x0934/4:
		verboselog( 2, "REX3 AA Line Weight Table 2/2 Read: %08x\n", nREX3_AWeight1 );
		return nREX3_AWeight1;
		break;
	case 0x0138/4:
	case 0x0938/4:
		verboselog( 2, "REX3 GL XStart Read: %08x\n", nREX3_XStartF );
		return nREX3_XStartF;
		break;
	case 0x013c/4:
	case 0x093c/4:
		verboselog( 2, "REX3 GL YStart Read: %08x\n", nREX3_YStartF );
		return nREX3_YStartF;
		break;
	case 0x0140/4:
	case 0x0940/4:
		verboselog( 2, "REX3 GL XEnd Read: %08x\n", nREX3_XEndF );
		return nREX3_XEndF;
		break;
	case 0x0144/4:
	case 0x0944/4:
		verboselog( 2, "REX3 GL YEnd Read: %08x\n", nREX3_YEndF );
		return nREX3_YEndF;
		break;
	case 0x0148/4:
	case 0x0948/4:
		verboselog( 2, "REX3 XStart (integer) Read: %08x\n", nREX3_XStartI );
		return nREX3_XStartI;
		break;
	case 0x014c/4:
	case 0x094c/4:
		verboselog( 2, "REX3 GL XEnd (copy) Read: %08x\n", nREX3_XEndF );
		return nREX3_XEndF;
		break;
	case 0x0150/4:
	case 0x0950/4:
		verboselog( 2, "REX3 XYStart (integer) Read: %08x\n", nREX3_XYStartI );
		return nREX3_XYStartI;
		break;
	case 0x0154/4:
	case 0x0954/4:
		verboselog( 2, "REX3 XYEnd (integer) Read: %08x\n", nREX3_XYEndI );
		return nREX3_XYEndI;
		break;
	case 0x0158/4:
	case 0x0958/4:
		verboselog( 2, "REX3 XStartEnd (integer) Read: %08x\n", nREX3_XStartEndI );
		return nREX3_XStartEndI;
		break;
	case 0x0200/4:
	case 0x0a00/4:
		verboselog( 2, "REX3 Red/CI Full State Read: %08x\n", nREX3_ColorRed );
		return nREX3_ColorRed;
		break;
	case 0x0204/4:
	case 0x0a04/4:
		verboselog( 2, "REX3 Alpha Full State Read: %08x\n", nREX3_ColorAlpha );
		return nREX3_ColorAlpha;
		break;
	case 0x0208/4:
	case 0x0a08/4:
		verboselog( 2, "REX3 Green Full State Read: %08x\n", nREX3_ColorGreen );
		return nREX3_ColorGreen;
		break;
	case 0x020c/4:
	case 0x0a0c/4:
		verboselog( 2, "REX3 Blue Full State Read: %08x\n", nREX3_ColorBlue );
		return nREX3_ColorBlue;
		break;
	case 0x0210/4:
	case 0x0a10/4:
		verboselog( 2, "REX3 Red/CI Slope Read: %08x\n", nREX3_SlopeRed );
		return nREX3_SlopeRed;
		break;
	case 0x0214/4:
	case 0x0a14/4:
		verboselog( 2, "REX3 Alpha Slope Read: %08x\n", nREX3_SlopeAlpha );
		return nREX3_SlopeAlpha;
		break;
	case 0x0218/4:
	case 0x0a18/4:
		verboselog( 2, "REX3 Green Slope Read: %08x\n", nREX3_SlopeGreen );
		return nREX3_SlopeGreen;
		break;
	case 0x021c/4:
	case 0x0a1c/4:
		verboselog( 2, "REX3 Blue Slope Read: %08x\n", nREX3_SlopeBlue );
		return nREX3_SlopeBlue;
		break;
	case 0x0220/4:
	case 0x0a20/4:
		verboselog( 2, "REX3 Write Mask Read: %08x\n", nREX3_WriteMask );
		return nREX3_WriteMask;
		break;
	case 0x0224/4:
	case 0x0a24/4:
		verboselog( 2, "REX3 Packed Color Fractions Read: %08x\n", nREX3_ZeroFract );
		return nREX3_ZeroFract;
		break;
	case 0x0228/4:
	case 0x0a28/4:
		verboselog( 2, "REX3 Color Index Zeros Overflow Read: %08x\n", nREX3_ZeroOverflow );
		return nREX3_ZeroOverflow;
		break;
	case 0x022c/4:
	case 0x0a2c/4:
		verboselog( 2, "REX3 Red/CI Slope (copy) Read: %08x\n", nREX3_SlopeRed );
		return nREX3_SlopeRed;
		break;
	case 0x0230/4:
	case 0x0a30/4:
		verboselog( 2, "REX3 Host Data Port MSW Read: %08x\n", nREX3_HostDataPortMSW );
		return nREX3_HostDataPortMSW;
		break;
	case 0x0234/4:
	case 0x0a34/4:
		verboselog( 2, "REX3 Host Data Port LSW Read: %08x\n", nREX3_HostDataPortLSW );
		return nREX3_HostDataPortLSW;
		break;
	case 0x0238/4:
	case 0x0a38/4:
		verboselog( 2, "REX3 Display Control Bus Mode Read: %08x\n", nREX3_DCBMode );
		return nREX3_DCBMode;
		break;
	case 0x0240/4:
	case 0x0a40/4:
		switch( nREX3_DCBSlvSelect )
		{
		case 0x00:
			return newport_vc2_r( 0, mem_mask );
			break;
		case 0x02:
			return newport_cmap0_r( 0, mem_mask );
			break;
		case 0x03:
			return newport_cmap1_r( 0, mem_mask );
			break;
		case 0x05:
			return newport_xmap0_r( 0, mem_mask );
			break;
		case 0x06:
			return newport_xmap1_r( 0, mem_mask );
			break;
		default:
			verboselog( 2, "REX3 Display Control Bus Data MSW Read: %08x\n", nREX3_DCBDataMSW );
			break;
		}
		return nREX3_DCBDataMSW;
		break;
	case 0x0244/4:
	case 0x0a44/4:
		verboselog( 2, "REX3 Display Control Bus Data LSW Read: %08x\n", nREX3_DCBDataLSW );
		return nREX3_DCBDataLSW;
		break;
	case 0x1300/4:
		verboselog( 2, "REX3 Screenmask 1 X Min/Max Read: %08x\n", nREX3_SMask1X );
		return nREX3_SMask1X;
		break;
	case 0x1304/4:
		verboselog( 2, "REX3 Screenmask 1 Y Min/Max Read: %08x\n", nREX3_SMask1Y );
		return nREX3_SMask1Y;
		break;
	case 0x1308/4:
		verboselog( 2, "REX3 Screenmask 2 X Min/Max Read: %08x\n", nREX3_SMask2X );
		return nREX3_SMask2X;
		break;
	case 0x130c/4:
		verboselog( 2, "REX3 Screenmask 2 Y Min/Max Read: %08x\n", nREX3_SMask2Y );
		return nREX3_SMask2Y;
		break;
	case 0x1310/4:
		verboselog( 2, "REX3 Screenmask 3 X Min/Max Read: %08x\n", nREX3_SMask3X );
		return nREX3_SMask3X;
		break;
	case 0x1314/4:
		verboselog( 2, "REX3 Screenmask 3 Y Min/Max Read: %08x\n", nREX3_SMask3Y );
		return nREX3_SMask3Y;
		break;
	case 0x1318/4:
		verboselog( 2, "REX3 Screenmask 4 X Min/Max Read: %08x\n", nREX3_SMask4X );
		return nREX3_SMask4X;
		break;
	case 0x131c/4:
		verboselog( 2, "REX3 Screenmask 4 Y Min/Max Read: %08x\n", nREX3_SMask4Y );
		return nREX3_SMask4Y;
		break;
	case 0x1320/4:
		verboselog( 2, "REX3 Top of Screen Scanline Read: %08x\n", nREX3_TopScanline );
		return nREX3_TopScanline;
		break;
	case 0x1324/4:
		verboselog( 2, "REX3 Clipping Mode Read: %08x\n", nREX3_XYWin );
		return nREX3_XYWin;
		break;
	case 0x1328/4:
		verboselog( 2, "REX3 Clipping Mode Read: %08x\n", nREX3_ClipMode );
		return nREX3_ClipMode;
		break;
	case 0x1330/4:
		verboselog( 2, "REX3 Config Read: %08x\n", nREX3_Config );
		return nREX3_Config;
		break;
	case 0x1338/4:
		verboselog( 2, "REX3 Status Read: %08x\n", 0x00000001 );
		nTemp = nREX3_Status;
		nREX3_Status = 0;
		return 0x00000001;
		break;
	case 0x133c/4:
		verboselog( 2, "REX3 User Status Read: %08x\n", 0x00000001 );
		return 0x00000001;
		break;
	default:
		verboselog( 2, "Unknown REX3 Read: %08x (%08x)\n", 0x1f0f0000 + ( offset << 2 ), mem_mask );
		return 0;
		break;
	}
	return 0;
}

static void DoREX3Command(void)
{
	UINT32 nCommand = ( ( nREX3_DrawMode0 & ( 1 << 15 ) ) >> 15 ) |
						( ( nREX3_DrawMode0 & ( 1 <<  5 ) ) >>  4 ) |
						( ( nREX3_DrawMode0 & ( 1 <<  9 ) ) >>  7 ) |
						( ( nREX3_DrawMode0 & ( 1 <<  8 ) ) >>  5 ) |
						( ( nREX3_DrawMode0 & 0x0000001c  ) <<  2 ) |
						( ( nREX3_DrawMode0 & 0x00000003  ) <<  7 );
	UINT16 nX, nY;
	UINT16 nStartX = ( nREX3_XYStartI >> 16 ) & 0x0000ffff;
	UINT16 nStartY = ( nREX3_XYStartI >>  0 ) & 0x0000ffff;
	UINT16 nEndX = ( nREX3_XYEndI >> 16 ) & 0x0000ffff;
	UINT16 nEndY = ( nREX3_XYEndI >>  0 ) & 0x0000ffff;
	INT16 nMoveX, nMoveY;

	switch( nCommand )
	{
	case 0x00000110:
		nX = nStartX;
		nY = nStartY;
		verboselog( 3, "Tux Logo Draw: %04x, %04x = %08x\n", nX, nY, nCMAP0_Palette[ ( nREX3_HostDataPortMSW & 0xff000000 ) >> 24 ] );
//		nREX3_Kludge_SkipLine = 1;
		nREX3_BresOctInc1 = 0;
		video_base[ nY*(1280+64) + nX ] = nCMAP0_Palette[ ( nREX3_HostDataPortMSW & 0xff000000 ) >> 24 ];
		nX++;
		if( nX > ( ( nREX3_XYEndI & 0xffff0000 ) >> 16 ) )
		{
			nY++;
			nX = nREX3_XSave;
		}
		nREX3_XYStartI = ( nX << 16 ) | nY;
		nREX3_XStartI = nX;
		nREX3_XStart = 0 | ( ( nREX3_XYStartI & 0xffff0000 ) >>  5 );
		nREX3_YStart = 0 | ( ( nREX3_XYStartI & 0x0000ffff ) << 11 );
		break;
	case 0x0000011e:
		verboselog( 3, "Block draw: %04x, %04x to %04x, %04x = %08x\n", nStartX, nStartY, nEndX, nEndY, nCMAP0_Palette[ nREX3_ZeroFract ] );
		for( nY = nStartY; nY <= nEndY; nY++ )
		{
			verboselog( 3, "Pixel: %04x, %04x = %08x\n", nStartX, nY, nCMAP0_Palette[ nREX3_ZeroFract ] );
			for( nX = nStartX; nX <= nEndX; nX++ )
			{
				video_base[ nY*(1280+64) + nX ] = nCMAP0_Palette[ nREX3_ZeroFract ];
			}
		}
		break;
	case 0x00000119:
		if( !nREX3_Kludge_SkipLine )
		{
			verboselog( 3, "Pattern Line Draw: %08x at %04x, %04x color %08x\n", nREX3_ZPattern, nREX3_XYStartI >> 16, nREX3_XYStartI & 0x0000ffff, nCMAP0_Palette[ nREX3_ZeroFract ] );
		for( nX = nStartX; nX <= nEndX && nX < ( nStartX + 32 ); nX++ )
		{
			if( nREX3_ZPattern & ( 1 << ( 31 - ( nX - nStartX ) ) ) )
			{
				video_base[ nStartY*(1280+64) + nX ] = nCMAP0_Palette[ nREX3_ZeroFract ];
			}
		}
			if( nREX3_BresOctInc1 & 0x01000000 )
		{
			nStartY--;
		}
			else
		{
			nStartY++;
		}
		nREX3_XYStartI = ( nStartX << 16 ) | nStartY;
		nREX3_YStart = 0 | ( ( nREX3_XYStartI & 0x0000ffff ) << 11 );
		}
		break;
	case 0x0000019e:
		nMoveX = (INT16)( ( nREX3_XYMove >> 16 ) & 0x0000ffff );
		nMoveY = (INT16)( nREX3_XYMove & 0x0000ffff );
		verboselog( 1, "FB to FB Copy: %04x, %04x - %04x, %04x to %04x, %04x\n", nStartX, nStartY, nEndX, nEndY, nStartX + nMoveX, nStartY + nMoveY );
		for( nY = nStartY; nY <= nEndY; nY++ )
		{
			for( nX = nStartX; nX <= nEndX; nX++ )
			{
				video_base[ (nY + nMoveY)*(1280+64) + (nX + nMoveX) ] = video_base[ nY*(1280+64) + nX ];
			}
		}
		break;
	default:
		verboselog( 1, "Unknown draw command: %08x\n", nCommand );
		break;
	}
}

WRITE32_HANDLER( newport_rex3_w )
{
	UINT32 nTemp=0;
	if( offset & 0x00000200 )
	{
		verboselog( 2, "Start Cmd\n" );
	}
	switch( offset )
	{
	case 0x0000/4:
	case 0x0800/4:
		verboselog( 2, "REX3 Draw Mode 1 Write: %08x\n", data );
		switch( data & 0x00000007 )
		{
		case 0x00:
			verboselog( 2, "    Planes Enabled:     None\n" );
			break;
		case 0x01:
			verboselog( 2, "    Planes Enabled:     R/W RGB/CI\n" );
			break;
		case 0x02:
			verboselog( 2, "    Planes Enabled:     R/W RGBA\n" );
			break;
		case 0x03:
			verboselog( 2, "    Planes Enabled:     R/W OLAY\n" );
			break;
		case 0x04:
			verboselog( 2, "    Planes Enabled:     R/W PUP\n" );
			break;
		case 0x05:
			verboselog( 2, "    Planes Enabled:     R/W CID\n" );
			break;
		default:
			verboselog( 2, "    Unknown Plane Enable Value\n" );
			break;
		}
		switch( ( data & 0x00000018 ) >> 3 )
		{
		case 0x00:
			verboselog( 2, "    Plane Draw Depth:    4 bits\n" );
			break;
		case 0x01:
			verboselog( 2, "    Plane Draw Depth:    8 bits\n" );
			break;
		case 0x02:
			verboselog( 2, "    Plane Draw Depth:   12 bits\n" );
			break;
		case 0x03:
			verboselog( 2, "    Plane Draw Depth:   32 bits\n" );
			break;
		}
		verboselog( 2, "    DBuf Source Buffer: %d\n", ( data & 0x00000020 ) >>  5 );
		verboselog( 2, "    GL Y Coordinates:   %d\n", ( data & 0x00000040 ) >>  6 );
		verboselog( 2, "    Enable Pxl Packing: %d\n", ( data & 0x00000080 ) >>  7 );
		switch( ( data & 0x00000300 ) >> 8 )
		{
		case 0x00:
			verboselog( 2, "    HOSTRW Depth:        4 bits\n" );
			break;
		case 0x01:
			verboselog( 2, "    HOSTRW Depth:        8 bits\n" );
			break;
		case 0x02:
			verboselog( 2, "    HOSTRW Depth:       12 bits\n" );
			break;
		case 0x03:
			verboselog( 2, "    HOSTRW Depth:       32 bits\n" );
			break;
		}
		verboselog( 2, "    DWord Transfers:    %d\n", ( data & 0x00000400 ) >> 10 );
		verboselog( 2, "    Swap Endianness:    %d\n", ( data & 0x00000800 ) >> 11 );
		verboselog( 2, "    Compare Src > Dest: %d\n", ( data & 0x00001000 ) >> 12 );
		verboselog( 2, "    Compare Src = Dest: %d\n", ( data & 0x00002000 ) >> 13 );
		verboselog( 2, "    Compare Src < Dest: %d\n", ( data & 0x00004000 ) >> 14 );
		verboselog( 2, "    RGB Mode Select:    %d\n", ( data & 0x00008000 ) >> 15 );
		verboselog( 2, "    Enable Dithering:   %d\n", ( data & 0x00010000 ) >> 16 );
		verboselog( 2, "    Enable Fast Clear:  %d\n", ( data & 0x00020000 ) >> 17 );
		verboselog( 2, "    Enable Blending:    %d\n", ( data & 0x00040000 ) >> 18 );
		switch( ( data & 0x00380000 ) >> 19 )
		{
		case 0x00:
			verboselog( 2, "    Src Blend Factor:   0\n" );
			break;
		case 0x01:
			verboselog( 2, "    Src Blend Factor:   1\n" );
			break;
		case 0x02:
			verboselog( 2, "    Src Blend Factor:   Normalized Dest (or COLORBACK)\n" );
			break;
		case 0x03:
			verboselog( 2, "    Src Blend Factor:   1 - Normalized Dest (or COLORBACK)\n" );
			break;
		case 0x04:
			verboselog( 2, "    Src Blend Factor:   Normalized Src\n" );
			break;
		case 0x05:
			verboselog( 2, "    Src Blend Factor:   1 - Normalized Src\n" );
			break;
		default:
			verboselog( 2, "    Unknown Src Blend Factor: %02x\n", ( data & 0x00380000 ) >> 19 );
			break;
		}
		switch( ( data & 0x01c00000 ) >> 22 )
		{
		case 0x00:
			verboselog( 2, "    Dest Blend Factor:  0\n" );
			break;
		case 0x01:
			verboselog( 2, "    Dest Blend Factor:  1\n" );
			break;
		case 0x02:
			verboselog( 2, "    Dest Blend Factor:  Normalized Dest (or COLORBACK)\n" );
			break;
		case 0x03:
			verboselog( 2, "    Dest Blend Factor:  1 - Normalized Dest (or COLORBACK)\n" );
			break;
		case 0x04:
			verboselog( 2, "    Dest Blend Factor:  Normalized Src\n" );
			break;
		case 0x05:
			verboselog( 2, "    Dest Blend Factor:  1 - Normalized Src\n" );
			break;
		default:
			verboselog( 2, "    Unknown Src Blend Factor: %02x\n", ( data & 0x00380000 ) >> 19 );
			break;
		}
		verboselog( 2, "  COLORBACK Dest Blend: %d\n", ( data & 0x02000000 ) >> 25 );
		verboselog( 2, "   Enable Pxl Prefetch: %d\n", ( data & 0x04000000 ) >> 26 );
		verboselog( 2, "    SFACTOR Src Alpha:  %d\n", ( data & 0x08000000 ) >> 27 );
		switch( ( data & 0xf0000000 ) >> 28 )
		{
		case 0x00:
			verboselog( 2, "    Logical Op. Type:   0\n" );
			break;
		case 0x01:
			verboselog( 2, "    Logical Op. Type:   Src & Dst\n" );
			break;
		case 0x02:
			verboselog( 2, "    Logical Op. Type:   Src & ~Dst\n" );
			break;
		case 0x03:
			verboselog( 2, "    Logical Op. Type:   Src\n" );
			break;
		case 0x04:
			verboselog( 2, "    Logical Op. Type:   ~Src & Dst\n" );
			break;
		case 0x05:
			verboselog( 2, "    Logical Op. Type:   Dst\n" );
			break;
		case 0x06:
			verboselog( 2, "    Logical Op. Type:   Src ^ Dst\n" );
			break;
		case 0x07:
			verboselog( 2, "    Logical Op. Type:   Src | Dst\n" );
			break;
		case 0x08:
			verboselog( 2, "    Logical Op. Type:   ~(Src | Dst)\n" );
			break;
		case 0x09:
			verboselog( 2, "    Logical Op. Type:   ~(Src ^ Dst)\n" );
			break;
		case 0x0a:
			verboselog( 2, "    Logical Op. Type:   ~Dst\n" );
			break;
		case 0x0b:
			verboselog( 2, "    Logical Op. Type:   Src | ~Dst\n" );
			break;
		case 0x0c:
			verboselog( 2, "    Logical Op. Type:   ~Src\n" );
			break;
		case 0x0d:
			verboselog( 2, "    Logical Op. Type:   ~Src | Dst\n" );
			break;
		case 0x0e:
			verboselog( 2, "    Logical Op. Type:   ~(Src & Dst)\n" );
			break;
		case 0x0f:
			verboselog( 2, "    Logical Op. Type:   1\n" );
			break;
		}
		nREX3_DrawMode1 = data;
//		if( offset >= ( 0x800 / 4 ) )
//		{
//			DoREX3Command();
//		}
		break;
	case 0x0004/4:
		verboselog( 2, "REX3 Draw Mode 0 Write: %08x\n", data );
		switch( data & 0x00000003 )
		{
		case 0x00:
			verboselog( 2, "    Primitive Function: No Op\n" );
			break;
		case 0x01:
			verboselog( 2, "    Primitive Function: Read From FB\n" );
			break;
		case 0x02:
			verboselog( 2, "    Primitive Function: Draw To FB\n" );
			break;
		case 0x03:
			verboselog( 2, "    Primitive Function: Copy FB To FB\n" );
			break;
		}
		switch( ( data & 0x0000001c ) >> 2 )
		{
		case 0x00:
			verboselog( 2, "    Addressing Mode: Span/Point\n" );
			break;
		case 0x01:
			verboselog( 2, "    Addressing Mode: Block\n" );
			break;
		case 0x02:
			verboselog( 2, "    Addressing Mode: Bresenham Line, Integer Endpoints\n" );
			break;
		case 0x03:
			verboselog( 2, "    Addressing Mode: Bresenham Line, Fractional Endpoints\n" );
			break;
		case 0x04:
			verboselog( 2, "    Addressing Mode: AA Bresenham Line\n" );
			break;
		default:
			verboselog( 2, "    Unknown Addressing Mode: %02x\n", ( data & 0x0000001c ) >> 2 );
			break;
		}
		verboselog( 2, "    Iterator Setup:     %d\n", ( data & 0x00000020 ) >>  5 );
		verboselog( 2, "    RGB/CI Draw Source: %d\n", ( data & 0x00000040 ) >>  6 );
		verboselog( 2, "     Alpha Draw Source: %d\n", ( data & 0x00000080 ) >>  7 );
		verboselog( 2, "    Stop On X:          %d\n", ( data & 0x00000100 ) >>  8 );
		verboselog( 2, "    Stop On Y:          %d\n", ( data & 0x00000200 ) >>  9 );
		verboselog( 2, "    Skip Start Point:   %d\n", ( data & 0x00000400 ) >> 10 );
		verboselog( 2, "    Skip End Point:     %d\n", ( data & 0x00000800 ) >> 11 );
		verboselog( 2, "    Enable Patterning:  %d\n", ( data & 0x00001000 ) >> 12 );
		verboselog( 2, "    Enable Stippling:   %d\n", ( data & 0x00002000 ) >> 13 );
		verboselog( 2, "    Stipple Advance:    %d\n", ( data & 0x00004000 ) >> 14 );
		verboselog( 2, "    Limit Draw To 32px: %d\n", ( data & 0x00008000 ) >> 15 );
		verboselog( 2, "     Z Opaque Stipple   %d\n", ( data & 0x00010000 ) >> 16 );
		verboselog( 2, "    LS Opaque Stipple:  %d\n", ( data & 0x00020000 ) >> 17 );
		verboselog( 2, "    Enable Lin. Shade:  %d\n", ( data & 0x00040000 ) >> 18 );
		verboselog( 2, "    Left-Right Only:    %d\n", ( data & 0x00080000 ) >> 19 );
		verboselog( 2, "    Offset by XYMove:   %d\n", ( data & 0x00100000 ) >> 20 );
		verboselog( 2, "    Enable CI Clamping: %d\n", ( data & 0x00200000 ) >> 21 );
		verboselog( 2, "    Enable End Filter:  %d\n", ( data & 0x00400000 ) >> 22 );
		verboselog( 2, "    Enable Y+2 Stride:  %d\n", ( data & 0x00800000 ) >> 23 );
		nREX3_DrawMode0 = data;
		break;
	case 0x0804/4:
		verboselog( 2, "REX3 Draw Mode 0 Write: %08x\n", data );
		nREX3_DrawMode0 = data;
		break;
	case 0x0008/4:
	case 0x0808/4:
		verboselog( 2, "REX3 Line Stipple Mode Write: %08x\n", data );
		nREX3_LSMode = data & 0x0fffffff;
		break;
	case 0x000C/4:
	case 0x080c/4:
		verboselog( 2, "REX3 Line Stipple Pattern Write: %08x\n", data );
		nREX3_LSPattern = data;
		break;
	case 0x0010/4:
	case 0x0810/4:
		verboselog( 2, "REX3 Line Stipple Pattern (Save) Write: %08x\n", data );
		nREX3_LSPatSave = data;
		break;
	case 0x0014/4:
	case 0x0814/4:
		verboselog( 2, "REX3 Pattern Register Write: %08x\n", data );
		nREX3_ZPattern = data;
		if( offset & 0x00000200 )
		{
			DoREX3Command();
		}
		break;
	case 0x0018/4:
	case 0x0818/4:
		verboselog( 2, "REX3 Opaque Pattern / Blendfunc Dest Color Write: %08x\n", data );
		nREX3_ColorBack = data;
		break;
	case 0x001c/4:
	case 0x081c/4:
		verboselog( 2, "REX3 VRAM Fastclear Color Write: %08x\n", data );
		nREX3_ColorVRAM = data;
		break;
	case 0x0020/4:
	case 0x0820/4:
		verboselog( 2, "REX3 AFUNCTION Reference Alpha Write: %08x\n", data );
		nREX3_AlphaRef = data & 0x000000ff;
		break;
	case 0x0024/4:
	case 0x0824/4:
		verboselog( 2, "REX3 Stall GFIFO Write: %08x\n", data );
		break;
	case 0x0028/4:
	case 0x0828/4:
		verboselog( 2, "REX3 Screenmask 0 X Min/Max Write: %08x\n", data );
		nREX3_SMask0X = data;
		break;
	case 0x002c/4:
	case 0x082c/4:
		verboselog( 2, "REX3 Screenmask 0 Y Min/Max Write: %08x\n", data );
		nREX3_SMask0Y = data;
		break;
	case 0x0030/4:
	case 0x0830/4:
		verboselog( 2, "REX3 Line/Span Setup Write: %08x\n", data );
		nREX3_Setup = data;
		break;
	case 0x0034/4:
	case 0x0834/4:
		verboselog( 2, "REX3 ZPattern Enable Write: %08x\n", data );
		nREX3_StepZ = data;
		break;
	case 0x0038/4:
	case 0x0838/4:
		verboselog( 2, "REX3 Update LSPATTERN/LSRCOUNT\n" );
		nREX3_LSPattern = nREX3_LSPatSave;
		break;
	case 0x003c/4:
	case 0x083c/4:
		verboselog( 2, "REX3 Update LSPATSAVE/LSRCNTSAVE\n" );
		nREX3_LSPatSave = nREX3_LSPattern;
		break;
	case 0x0100/4:
	case 0x0900/4:
		verboselog( 2, "REX3 XStart Write: %08x\n", data );
		nREX3_XStart = data & ( 0x0000fffff << 7 );
		break;
	case 0x0104/4:
	case 0x0904/4:
		verboselog( 2, "REX3 YStart Write: %08x\n", data );
		nREX3_YStart = data & ( 0x0000fffff << 7 );
		break;
	case 0x0108/4:
	case 0x0908/4:
		verboselog( 2, "REX3 XEnd Write: %08x\n", data );
		nREX3_XEnd = data & ( 0x0000fffff << 7 );
		break;
	case 0x010c/4:
	case 0x090c/4:
		verboselog( 2, "REX3 YEnd Write: %08x\n", data );
		nREX3_YEnd = data & ( 0x0000fffff << 7 );
		break;
	case 0x0110/4:
	case 0x0910/4:
		verboselog( 2, "REX3 XSave Write: %08x\n", data );
		nREX3_XSave = data & 0x0000ffff;
		nREX3_XStartI = nREX3_XSave & 0x0000ffff;
		break;
	case 0x0114/4:
	case 0x0914/4:
		verboselog( 2, "REX3 XYMove Write: %08x\n", data );
		nREX3_XYMove = data;
		if( offset & 0x00000200 )
		{
			DoREX3Command();
		}
		break;
	case 0x0118/4:
	case 0x0918/4:
		verboselog( 2, "REX3 Bresenham D Write: %08x\n", data );
		nREX3_BresD = data & 0x07ffffff;
		break;
	case 0x011c/4:
	case 0x091c/4:
		verboselog( 2, "REX3 Bresenham S1 Write: %08x\n", data );
		nREX3_BresS1 = data & 0x0001ffff;
		break;
	case 0x0120/4:
	case 0x0920/4:
		verboselog( 2, "REX3 Bresenham Octant & Incr1 Write: %08x\n", data );
		nREX3_BresOctInc1 = data & 0x070fffff;
		break;
	case 0x0124/4:
	case 0x0924/4:
		verboselog( 2, "REX3 Bresenham Octant Rounding Mode & Incr2 Write: %08x\n", data );
		nREX3_BresRndInc2 = data & 0xff1fffff;
		break;
	case 0x0128/4:
	case 0x0928/4:
		verboselog( 2, "REX3 Bresenham E1 Write: %08x\n", data );
		nREX3_BresE1 = data & 0x0000ffff;
		break;
	case 0x012c/4:
	case 0x092c/4:
		verboselog( 2, "REX3 Bresenham S2 Write: %08x\n", data );
		nREX3_BresS2 = data & 0x03ffffff;
		break;
	case 0x0130/4:
	case 0x0930/4:
		verboselog( 2, "REX3 AA Line Weight Table 1/2 Write: %08x\n", data );
		nREX3_AWeight0 = data;
		break;
	case 0x0134/4:
	case 0x0934/4:
		verboselog( 2, "REX3 AA Line Weight Table 2/2 Write: %08x\n", data );
		nREX3_AWeight1 = data;
		break;
	case 0x0138/4:
	case 0x0938/4:
		verboselog( 2, "REX3 GL XStart Write: %08x\n", data );
		nREX3_XStartF = data & ( 0x0000ffff << 7 );
		break;
	case 0x013c/4:
	case 0x093c/4:
		verboselog( 2, "REX3 GL YStart Write: %08x\n", data );
		nREX3_YStartF = data & ( 0x0000ffff << 7 );
		break;
	case 0x0140/4:
	case 0x0940/4:
		verboselog( 2, "REX3 GL XEnd Write: %08x\n", data );
		nREX3_XEndF = data & ( 0x0000ffff << 7 );
		break;
	case 0x0144/4:
	case 0x0944/4:
		verboselog( 2, "REX3 GL YEnd Write: %08x\n", data );
		nREX3_YEndF = data & ( 0x0000ffff << 7 );
		break;
	case 0x0148/4:
	case 0x0948/4:
		verboselog( 2, "REX3 XStart (integer) Write: %08x\n", data );
		nREX3_XStartI = data & 0x0000ffff;
		nREX3_XSave = nREX3_XStartI;
		nREX3_XStart = 0 | ( ( nREX3_XStartI & 0x0000ffff ) << 11 );
		break;
	case 0x014c/4:
	case 0x094c/4:
		verboselog( 2, "REX3 GL XEnd (copy) Write: %08x\n", data );
		nREX3_XEndF = data & ( 0x0000ffff << 7 );
		break;
	case 0x0150/4:
	case 0x0950/4:
		verboselog( 2, "REX3 XYStart (integer) Write: %08x\n", data );
		nREX3_XYStartI = data;
		nREX3_XStartI = ( data & 0xffff0000 ) >> 16;
		nREX3_XSave = nREX3_XStartI;
		nREX3_XStart = 0 | ( ( nREX3_XYStartI & 0xffff0000 ) >>  5 );
		nREX3_YStart = 0 | ( ( nREX3_XYStartI & 0x0000ffff ) << 11 );
		break;
	case 0x0154/4:
	case 0x0954/4:
		verboselog( 2, "REX3 XYEnd (integer) Write: %08x\n", data );
		nREX3_XYEndI = data;
		nREX3_XEnd = 0 | ( ( nREX3_XYEndI & 0xffff0000 ) >>  5 );
		nREX3_YEnd = 0 | ( ( nREX3_XYEndI & 0x0000ffff ) << 11 );
		if( offset & 0x00000200 )
		{
			DoREX3Command();
		}
		break;
	case 0x0158/4:
	case 0x0958/4:
		verboselog( 2, "REX3 XStartEnd (integer) Write: %08x\n", data );
		nREX3_XStartEndI = data;
		nREX3_XYEndI   = ( nREX3_XYEndI   & 0x0000ffff ) | ( ( nREX3_XStartEndI & 0x0000ffff ) << 16 );
		nREX3_XYStartI = ( nREX3_XYStartI & 0x0000ffff ) | ( nREX3_XStartEndI & 0xffff0000 );
		nREX3_XSave = nREX3_XStartI;
		nREX3_XStart = 0 | ( ( nREX3_XStartEndI & 0xffff0000 ) >>  5 );
		nREX3_XEnd   = 0 | ( ( nREX3_XStartEndI & 0x0000ffff ) << 11 );
		break;
	case 0x0200/4:
	case 0x0a00/4:
		verboselog( 2, "REX3 Red/CI Full State Write: %08x\n", data );
		nREX3_ColorRed = data & 0x00ffffff;
		break;
	case 0x0204/4:
	case 0x0a04/4:
		verboselog( 2, "REX3 Alpha Full State Write: %08x\n", data );
		nREX3_ColorAlpha = data & 0x000fffff;
		break;
	case 0x0208/4:
	case 0x0a08/4:
		verboselog( 2, "REX3 Green Full State Write: %08x\n", data );
		nREX3_ColorGreen = data & 0x000fffff;
		break;
	case 0x020c/4:
	case 0x0a0c/4:
		verboselog( 2, "REX3 Blue Full State Write: %08x\n", data );
		nREX3_ColorBlue = data & 0x000fffff;
		break;
	case 0x0210/4:
	case 0x0a10/4:
		verboselog( 2, "REX3 Red/CI Slope Write: %08x\n", data );
		data &= 0x807fffff;
		switch( data & 0x80000000 )
		{
		case 0x00000000:
			nTemp = data & 0x007fffff;
			break;
		case 0x80000000:
			nTemp  = 0x00800000 - ( data & 0x007fffff );
			nTemp |= 0x00800000;
			break;
		}
		nREX3_SlopeRed = nTemp;
		break;
	case 0x0214/4:
	case 0x0a14/4:
		verboselog( 2, "REX3 Alpha Slope Write: %08x\n", data );
		data &= 0x8007ffff;
		switch( data & 0x80000000 )
		{
		case 0x00000000:
			nTemp = data & 0x0007ffff;
			break;
		case 0x80000000:
			nTemp  = 0x00080000 - ( data & 0x0007ffff );
			nTemp |= 0x00080000;
			break;
		}
		nREX3_SlopeAlpha = nTemp;
		break;
	case 0x0218/4:
	case 0x0a18/4:
		verboselog( 2, "REX3 Green Slope Write: %08x\n", data );
		data &= 0x8007ffff;
		switch( data & 0x80000000 )
		{
		case 0x00000000:
			nTemp = data & 0x0007ffff;
			break;
		case 0x80000000:
			nTemp  = 0x00080000 - ( data & 0x0007ffff );
			nTemp |= 0x00080000;
			break;
		}
		nREX3_SlopeGreen = nTemp;
		break;
	case 0x021c/4:
	case 0x0a1c/4:
		verboselog( 2, "REX3 Blue Slope Write: %08x\n", data );
		data &= 0x8007ffff;
		switch( data & 0x80000000 )
		{
		case 0x00000000:
			nTemp = data & 0x0007ffff;
			break;
		case 0x80000000:
			nTemp  = 0x00080000 - ( data & 0x0007ffff );
			nTemp |= 0x00080000;
			break;
		}
		nREX3_SlopeBlue = nTemp;
		break;
	case 0x0220/4:
	case 0x0a20/4:
		verboselog( 2, "REX3 Write Mask Write: %08x\n", data );
		nREX3_WriteMask = data & 0x00ffffff;
		break;
	case 0x0224/4:
	case 0x0a24/4:
		verboselog( 2, "REX3 Packed Color Fractions Write: %08x\n", data );
		nREX3_ZeroFract = data;
		break;
	case 0x0228/4:
	case 0x0a28/4:
		verboselog( 2, "REX3 Color Index Zeros Overflow Write: %08x\n", data );
		nREX3_ZeroOverflow = data;
		break;
	case 0x022c/4:
	case 0x0a2c/4:
		verboselog( 2, "REX3 Red/CI Slope (copy) Write: %08x\n", data );
		nREX3_SlopeRed = data;
		break;
	case 0x0230/4:
	case 0x0a30/4:
		verboselog( 3, "REX3 Host Data Port MSW Write: %08x\n", data );
		nREX3_HostDataPortMSW = data;
		if( offset & 0x00000200 )
		{
			DoREX3Command();
		}
		break;
	case 0x0234/4:
	case 0x0a34/4:
		verboselog( 2, "REX3 Host Data Port LSW Write: %08x\n", data );
		nREX3_HostDataPortLSW = data;
		break;
	case 0x0238/4:
	case 0x0a38/4:
		verboselog( 2, "REX3 Display Control Bus Mode Write: %08x\n", data );
		switch( data & 0x00000003 )
		{
		case 0x00:
			verboselog( 2, "    Transfer Width:     4 bytes\n" );
			nREX3_XFerWidth = 4;
			break;
		case 0x01:
			verboselog( 2, "    Transfer Width:     1 bytes\n" );
			nREX3_XFerWidth = 1;
			break;
		case 0x02:
			verboselog( 2, "    Transfer Width:     2 bytes\n" );
			nREX3_XFerWidth = 2;
			break;
		case 0x03:
			verboselog( 2, "    Transfer Width:     3 bytes\n" );
			nREX3_XFerWidth = 3;
			break;
		}
		verboselog( 2, "    DCB Reg Select Adr: %d\n", ( data & 0x00000070 ) >> 4 );
		verboselog( 2, "     DCB Slave Address: %d\n", ( data & 0x00000780 ) >> 7 );
//		verboselog( 2, "    Use Sync XFer ACK:  %d\n", ( data & 0x00000800 ) >> 11 );
//		verboselog( 2, "    Use Async XFer ACK: %d\n", ( data & 0x00001000 ) >> 12 );
//		verboselog( 2, "   GIO CLK Cycle Width: %d\n", ( data & 0x0003e000 ) >> 13 );
//		verboselog( 2, "    GIO CLK Cycle Hold: %d\n", ( data & 0x007c0000 ) >> 18 );
//		verboselog( 2, "   GIO CLK Cycle Setup: %d\n", ( data & 0x0f800000 ) >> 23 );
//		verboselog( 2, "    Swap Byte Ordering: %d\n", ( data & 0x10000000 ) >> 28 );
		nREX3_DCBRegSelect = ( data & 0x00000070 ) >> 4;
		nREX3_DCBSlvSelect = ( data & 0x00000780 ) >> 7;
		nREX3_DCBMode = data & 0x1fffffff;
		break;
	case 0x0240/4:
	case 0x0a40/4:
		nREX3_DCBDataMSW = data;
		switch( nREX3_DCBSlvSelect )
		{
		case 0x00:
			newport_vc2_w( 0, data, mem_mask );
			break;
		case 0x01:
 			newport_cmap0_w( 0, data, mem_mask );
 			break;
		case 0x04:
			newport_xmap0_w( 0, data, mem_mask );
			newport_xmap1_w( 0, data, mem_mask );
			break;
		case 0x05:
			newport_xmap0_w( 0, data, mem_mask );
			break;
		case 0x06:
			newport_xmap1_w( 0, data, mem_mask );
			break;
		default:
			verboselog( 2, "REX3 Display Control Bus Data MSW Write: %08x\n", data );
			break;
		}
		break;
	case 0x0244/4:
	case 0x0a44/4:
		verboselog( 2, "REX3 Display Control Bus Data LSW Write: %08x\n", data );
		nREX3_DCBDataLSW = data;
		break;
	case 0x1300/4:
		verboselog( 2, "REX3 Screenmask 1 X Min/Max Write: %08x\n", data );
		nREX3_SMask1X = data;
		break;
	case 0x1304/4:
		verboselog( 2, "REX3 Screenmask 1 Y Min/Max Write: %08x\n", data );
		nREX3_SMask1Y = data;
		break;
	case 0x1308/4:
		verboselog( 2, "REX3 Screenmask 2 X Min/Max Write: %08x\n", data );
		nREX3_SMask2X = data;
		break;
	case 0x130c/4:
		verboselog( 2, "REX3 Screenmask 2 Y Min/Max Write: %08x\n", data );
		nREX3_SMask2Y = data;
		break;
	case 0x1310/4:
		verboselog( 2, "REX3 Screenmask 3 X Min/Max Write: %08x\n", data );
		nREX3_SMask3X = data;
		break;
	case 0x1314/4:
		verboselog( 2, "REX3 Screenmask 3 Y Min/Max Write: %08x\n", data );
		nREX3_SMask3Y = data;
		break;
	case 0x1318/4:
		verboselog( 2, "REX3 Screenmask 4 X Min/Max Write: %08x\n", data );
		nREX3_SMask4X = data;
		break;
	case 0x131c/4:
		verboselog( 2, "REX3 Screenmask 4 Y Min/Max Write: %08x\n", data );
		nREX3_SMask4Y = data;
		break;
	case 0x1320/4:
		verboselog( 2, "REX3 Top of Screen Scanline Write: %08x\n", data );
		nREX3_TopScanline = data & 0x000003ff;
		break;
	case 0x1324/4:
		verboselog( 2, "REX3 Clipping Mode Write: %08x\n", data );
		nREX3_XYWin = data;
		break;
	case 0x1328/4:
		verboselog( 2, "REX3 Clipping Mode Write: %08x\n", data );
		nREX3_ClipMode = data & 0x00001fff;
		break;
	case 0x132c/4:
		verboselog( 2, "Request GFIFO Stall\n" );
		break;
	case 0x1330/4:
		verboselog( 2, "REX3 Config Write: %08x\n", data );
		nREX3_Config = data & 0x001fffff;
		break;
	case 0x1340/4:
		verboselog( 2, "Reset DCB Bus and Flush BFIFO\n" );
		break;
	default:
		verboselog( 2, "Unknown REX3 Write: %08x (%08x): %08x\n", 0xbf0f0000 + ( offset << 2 ), mem_mask, data );
		break;
	}
}


