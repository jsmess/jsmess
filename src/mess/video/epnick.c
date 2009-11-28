/*****************************************************************************
 *
 * video/epnick.c
 *
 * Nick Graphics Chip - found in Enterprise
 *
 * this is a display list graphics chip, with bitmap,
 * character and attribute graphics modes. Each entry in the
 * display list defines a char line, with variable number of
 * scanlines. Colour modes are 2,4, 16 and 256 colour.
 * Nick has 256 colours, 3 bits for R and G, with 2 bits for Blue.
 * It's a nice and flexible graphics processor..........
 *
 ****************************************************************************/

#include "driver.h"
#include "epnick.h"
#include "devices/messram.h"

/* given a colour index in range 0..255 gives the Red component */
#define NICK_GET_RED8(x) \
	((		\
	  ( ( (x & (1<<0)) >>0) <<2) | \
	  ( ( (x & (1<<3)) >>3) <<1) | \
	  ( ( (x & (1<<6)) >>6) <<0) \
	)<<5)

/* given a colour index in range 0..255 gives the Red component */
#define NICK_GET_GREEN8(x) \
	((	\
	  ( ( (x & (1<<1)) >>1) <<2) | \
	  ( ( (x & (1<<4)) >>4) <<1) | \
	  ( ( (x & (1<<7)) >>7) <<0) \
	)<<5)

/* given a colour index in range 0..255 gives the Red component */
#define NICK_GET_BLUE8(x) \
	(( \
	  ( ( (x & (1<<2)) >>2) <<1) | \
	  ( ( (x & (1<<5)) >>5) <<0) \
	)<<6)


/* Nick executes a Display list, in the form of a list of Line Parameter
Tables, this is the form of the data */
typedef struct LPT_ENTRY
{
	unsigned char SC;		/* scanlines in this modeline (two's complement) */
	unsigned char MB;		/* the MODEBYTE (defines video display mode) */
	unsigned char LM;		/* left margin etc */
	unsigned char RM;		/* right margin etc */
	unsigned char LD1L;	/* (a7..a0) of line data pointer LD1 */
	unsigned char LD1H;	/* (a8..a15) of line data pointer LD1 */
	unsigned char LD2L;	/* (a7..a0) of line data pointer LD2 */
	unsigned char LD2H;	/* (a8..a15) of line data pointer LD2 */
	unsigned char COL[8];	/* COL0..COL7 */
} LPT_ENTRY;

typedef struct NICK_STATE
{
	/* horizontal position */
	unsigned char HorizontalClockCount;
	/* current scanline within LPT */
	unsigned char ScanLineCount;

	unsigned char FIXBIAS;
	unsigned char BORDER;
	unsigned char LPL;
	unsigned char LPH;

	unsigned long LD1;
	unsigned long LD2;

	LPT_ENTRY	LPT;

	UINT16 *dest;
	int dest_pos;
	int dest_max_pos;

	unsigned char Reg[16];
} NICK_STATE;

/* colour mode types */
#define	NICK_2_COLOUR_MODE	0
#define	NICK_4_COLOUR_MODE	1
#define	NICK_16_COLOUR_MODE	2
#define	NICK_256_COLOUR_MODE	3

/* Display mode types */
#define	NICK_VSYNC_MODE	0
#define	NICK_PIXEL_MODE	1
#define	NICK_ATTR_MODE	2
#define	NICK_CH256_MODE	3
#define	NICK_CH128_MODE	4
#define	NICK_CH64_MODE	5
#define	NICK_UNUSED_MODE	6
#define	NICK_LPIXEL_MODE	7

/* MODEBYTE defines */
#define	NICK_MB_VIRQ			(1<<7)
#define	NICK_MB_VRES			(1<<4)
#define	NICK_MB_LPT_RELOAD		(1<<0)

/* Left margin defines */
#define	NICK_LM_MSBALT			(1<<7)
#define	NICK_LM_LSBALT			(1<<6)

/* Right margin defines */
#define	NICK_RM_ALTIND1			(1<<7)
#define	NICK_RM_ALTIND0			(1<<6)

/* useful macros */
#define	NICK_GET_LEFT_MARGIN(x)		(x & 0x03f)
#define	NICK_GET_RIGHT_MARGIN(x)	(x & 0x03f)
#define	NICK_GET_DISPLAY_MODE(x)	((x>>1) & 0x07)
#define	NICK_GET_COLOUR_MODE(x)		((x>>5) & 0x03)

#define NICK_RELOAD_LPT(x)			(x & 0x080)
#define NICK_CLOCK_LPT(x)			(x & 0x040)

/* Macros to generate memory address is CHx modes */
/* x = LD2, y = buf1 */
#define ADDR_CH256(x,y)		(((x & 0x0ff)<<8) | (y & 0x0ff))
#define ADDR_CH128(x,y)		(((x & 0x01ff)<<7) | (y & 0x07f))
#define ADDR_CH64(x,y)		(((x & 0x03ff)<<6) | (y & 0x03f))


/*************************************************************/
/* MESS stuff */
static NICK_STATE Nick;

// MESS specific
/* fetch a byte from "video ram" at Addr specified */
static char Nick_FetchByte(running_machine *machine,unsigned long Addr)
{
   return messram_get_ptr(devtag_get_device(machine, "messram"))[Addr & 0x0ffff];
}

// MESS specific
/* 8-bit pixel write! */
static void nick_write_pixel(NICK_STATE *nick, int ci)
{
	if (nick->dest_pos < nick->dest_max_pos)
	{
		nick->dest[nick->dest_pos++] = ci;
	}
}

/*****************************************************/


/* Enterprise has 256 colours, all may be on the screen at once!
the NICK_GET_RED8, NICK_GET_GREEN8, NICK_GET_BLUE8 macros
return a 8-bit colour value for the index specified.  */

/* initial the palette */
PALETTE_INIT( epnick )
{
	int i;

	for (i=0; i<256; i++)
	{
		palette_set_color_rgb( machine, i, NICK_GET_RED8(i), NICK_GET_GREEN8(i), NICK_GET_BLUE8(i) );
	}
}

/* first clock visible on left hand side */
static unsigned char Nick_FirstVisibleClock;
/* first clock visible on right hand side */
static unsigned char Nick_LastVisibleClock;

/* No of highest resolution pixels per "clock" */
#define NICK_PIXELS_PER_CLOCK	16

/* "clocks" per line */
#define NICK_TOTAL_CLOCKS_PER_LINE	64

/* we align based on the clocks */
static void Nick_CalcVisibleClocks(int Width)
{
	/* number of clocks we can see */
	int NoOfVisibleClocks = Width/NICK_PIXELS_PER_CLOCK;

	Nick_FirstVisibleClock =
		(NICK_TOTAL_CLOCKS_PER_LINE - NoOfVisibleClocks)>>1;

	Nick_LastVisibleClock = Nick_FirstVisibleClock + NoOfVisibleClocks;
}


/* given a bit pattern, this will get the pen index */
static unsigned int	Nick_PenIndexLookup_4Colour[256];
/* given a bit pattern, this will get the pen index */
static unsigned int	Nick_PenIndexLookup_16Colour[256];

static void	Nick_Init(void)
{
	int i;

	memset(&Nick, 0, sizeof(NICK_STATE));

	for (i=0; i<256; i++)
	{
		int PenIndex;

		PenIndex = (
                                (((i & 0x080)>>7)<<0) |
                                (((i & 0x08)>>3)<<1)
				);

                Nick_PenIndexLookup_4Colour[i] = PenIndex;

		PenIndex = (
                                ((((i & 0x080)>>7))<<0) |
                                ((((i & 0x08)>>3))<<1)  |
                                ((((i & 0x020)>>5))<<2) |
                                ((((i & 0x02)>>1))<<3)
				);

		Nick_PenIndexLookup_16Colour[i] = PenIndex;
	}

	Nick_CalcVisibleClocks(ENTERPRISE_SCREEN_WIDTH);

//  Nick.BORDER = 0;
//  Nick.FIXBIAS = 0;
}

/* write border colour */
static void	Nick_WriteBorder(int Clocks)
{
	int i;
	int ColIndex = Nick.BORDER;

	for (i=0; i<(Clocks<<4); i++)
	{
		nick_write_pixel(&Nick, ColIndex);
	}
}


static void Nick_DoLeftMargin(LPT_ENTRY *pLPT)
{
		unsigned char LeftMargin;

		LeftMargin = NICK_GET_LEFT_MARGIN(pLPT->LM);

		if (LeftMargin>Nick_FirstVisibleClock)
		{
			unsigned char LeftMarginVisible;

			/* some of the left margin is visible */
			LeftMarginVisible = LeftMargin-Nick_FirstVisibleClock;

			/* render the border */
			Nick_WriteBorder(LeftMarginVisible);
		}
}

static void Nick_DoRightMargin(LPT_ENTRY *pLPT)
{
		unsigned char RightMargin;

		RightMargin = NICK_GET_RIGHT_MARGIN(pLPT->RM);

		if (RightMargin<Nick_LastVisibleClock)
		{
			unsigned char RightMarginVisible;

			/* some of the right margin is visible */
			RightMarginVisible = Nick_LastVisibleClock - RightMargin;

			/* render the border */
			Nick_WriteBorder(RightMarginVisible);
		}
}

static int Nick_GetColourIndex(int PenIndex)
{
	if (PenIndex & 0x08)
	{
		return ((Nick.FIXBIAS & 0x01f)<<3) | (PenIndex & 0x07);
	}
	else
	{
		return Nick.LPT.COL[PenIndex];
	}
}

static void Nick_WritePixels2Colour(unsigned char Pen0, unsigned char Pen1, unsigned char DataByte)
{
	int i;
	int ColIndex[2];
	int PenIndex;
	unsigned char Data;

	Data = DataByte;

	ColIndex[0] = Nick_GetColourIndex(Pen0);
	ColIndex[1] = Nick_GetColourIndex(Pen1);

	for (i=0; i<8; i++)
	{
		PenIndex = ColIndex[(Data>>7) & 0x01];

		nick_write_pixel(&Nick, PenIndex);

		Data = Data<<1;
	}
}

static void Nick_WritePixels2ColourLPIXEL(unsigned char Pen0, unsigned char Pen1, unsigned char DataByte)
{
	int i;
	int ColIndex[2];
	int PenIndex;
	unsigned char Data;

	Data = DataByte;

	ColIndex[0] = Nick_GetColourIndex(Pen0);
	ColIndex[1] = Nick_GetColourIndex(Pen1);

	for (i=0; i<8; i++)
	{
		PenIndex = ColIndex[(Data>>7) & 0x01];

		nick_write_pixel(&Nick, PenIndex);
		nick_write_pixel(&Nick, PenIndex);

		Data = Data<<1;
	}
}


static void Nick_WritePixels(unsigned char DataByte, unsigned char CharIndex)
{
	int i;

	/* pen index colour 2-C (0,1), 4-C (0..3) 16-C (0..16) */
	int PenIndex;
	/* Col index = EP colour value */
	int PalIndex;
	unsigned char	ColourMode = NICK_GET_COLOUR_MODE(Nick.LPT.MB);
	unsigned char Data = DataByte;

	switch (ColourMode)
	{
		case NICK_2_COLOUR_MODE:
		{
			int PenOffset = 0;

			/* do before displaying byte */

			/* left margin attributes */
			if (Nick.LPT.LM & NICK_LM_MSBALT)
			{
				if (Data & 0x080)
				{
					PenOffset |= 2;
				}

				Data &=~0x080;
			}

			if (Nick.LPT.LM & NICK_LM_LSBALT)
			{
				if (Data & 0x001)
				{
					PenOffset |= 4;
				}

				Data &=~0x01;
			}

			if (Nick.LPT.RM & NICK_RM_ALTIND1)
			{
				if (CharIndex & 0x080)
				{
					PenOffset|=0x02;
				}
			}

//          if (Nick.LPT.RM & NICK_RM_ALTIND0)
//          {
//              if (Data & 0x040)
//              {
//                  PenOffset|=0x04;
//              }
//          }



			Nick_WritePixels2Colour(PenOffset,
				(PenOffset|0x01), Data);
		}
		break;

		case NICK_4_COLOUR_MODE:
		{
			//mame_printf_info("4 colour\r\n");

			/* left margin attributes */
			if (Nick.LPT.LM & NICK_LM_MSBALT)
			{
				Data &= ~0x080;
			}

			if (Nick.LPT.LM & NICK_LM_LSBALT)
			{
				Data &= ~0x01;
			}


			for (i=0; i<4; i++)
			{
				PenIndex = Nick_PenIndexLookup_4Colour[Data];
				PalIndex = Nick.LPT.COL[PenIndex & 0x03];

				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);

				Data = Data<<1;
 			}
		}
		break;

		case NICK_16_COLOUR_MODE:
		{
			//mame_printf_info("16 colour\r\n");

			/* left margin attributes */
			if (Nick.LPT.LM & NICK_LM_MSBALT)
			{
				Data &= ~0x080;
			}

			if (Nick.LPT.LM & NICK_LM_LSBALT)
			{
				Data &= ~0x01;
			}


			for (i=0; i<2; i++)
			{
				PenIndex = Nick_PenIndexLookup_16Colour[Data];

				PalIndex = Nick_GetColourIndex(PenIndex);

				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);

				Data = Data<<1;
			}
		}
		break;

		case NICK_256_COLOUR_MODE:
		{
			/* left margin attributes */
			if (Nick.LPT.LM & NICK_LM_MSBALT)
			{
				Data &= ~0x080;
			}

			if (Nick.LPT.LM & NICK_LM_LSBALT)
			{
				Data &= ~0x01;
			}


			PalIndex = Data;

			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);


		}
		break;
	}
}

static void Nick_WritePixelsLPIXEL(unsigned char DataByte, unsigned char CharIndex)
{
	int i;

	/* pen index colour 2-C (0,1), 4-C (0..3) 16-C (0..16) */
	int PenIndex;
	/* Col index = EP colour value */
	int PalIndex;
	unsigned char	ColourMode = NICK_GET_COLOUR_MODE(Nick.LPT.MB);
	unsigned char Data = DataByte;

	switch (ColourMode)
	{
		case NICK_2_COLOUR_MODE:
		{
			int PenOffset = 0;

			/* do before displaying byte */

			/* left margin attributes */
			if (Nick.LPT.LM & NICK_LM_MSBALT)
			{
				if (Data & 0x080)
				{
					PenOffset |= 2;
				}

				Data &=~0x080;
			}

			if (Nick.LPT.LM & NICK_LM_LSBALT)
			{
				if (Data & 0x001)
				{
					PenOffset |= 4;
				}

				Data &=~0x01;
			}

			if (Nick.LPT.RM & NICK_RM_ALTIND1)
			{
				if (CharIndex & 0x080)
				{
					PenOffset|=0x02;
				}
			}

//          if (Nick.LPT.RM & NICK_RM_ALTIND0)
//          {
//              if (Data & 0x040)
//              {
//                  PenOffset|=0x04;
//              }
//          }



			Nick_WritePixels2ColourLPIXEL(PenOffset,(PenOffset|0x01), Data);
		}
		break;

		case NICK_4_COLOUR_MODE:
		{
			//mame_printf_info("4 colour\r\n");

			/* left margin attributes */
			if (Nick.LPT.LM & NICK_LM_MSBALT)
			{
				Data &= ~0x080;
			}

			if (Nick.LPT.LM & NICK_LM_LSBALT)
			{
				Data &= ~0x01;
			}


			for (i=0; i<4; i++)
			{
				PenIndex = Nick_PenIndexLookup_4Colour[Data];
				PalIndex = Nick.LPT.COL[PenIndex & 0x03];

				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);

				Data = Data<<1;
 			}
		}
		break;

		case NICK_16_COLOUR_MODE:
		{
			//mame_printf_info("16 colour\r\n");

			/* left margin attributes */
			if (Nick.LPT.LM & NICK_LM_MSBALT)
			{
				Data &= ~0x080;
			}

			if (Nick.LPT.LM & NICK_LM_LSBALT)
			{
				Data &= ~0x01;
			}


			for (i=0; i<2; i++)
			{
				PenIndex = Nick_PenIndexLookup_16Colour[Data];

				PalIndex = Nick_GetColourIndex(PenIndex);

				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);
				nick_write_pixel(&Nick, PalIndex);

				Data = Data<<1;
			}
		}
		break;

		case NICK_256_COLOUR_MODE:
		{
			/* left margin attributes */
			if (Nick.LPT.LM & NICK_LM_MSBALT)
			{
				Data &= ~0x080;
			}

			if (Nick.LPT.LM & NICK_LM_LSBALT)
			{
				Data &= ~0x01;
			}


			PalIndex = Data;

			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);

			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);
			nick_write_pixel(&Nick, PalIndex);


		}
		break;
	}
}


static void Nick_DoPixel(running_machine *machine,int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 =  Nick_FetchByte(machine,Nick.LD1);
		Nick.LD1++;

		Buf2 = Nick_FetchByte(machine,Nick.LD1);
		Nick.LD1++;

		Nick_WritePixels(Buf1,Buf1);

		Nick_WritePixels(Buf2,Buf1);
	}
}


static void Nick_DoLPixel(running_machine *machine,int ClocksVisible)
{
	int i;
	unsigned char Buf1;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 =  Nick_FetchByte(machine,Nick.LD1);
		Nick.LD1++;

		Nick_WritePixelsLPIXEL(Buf1,Buf1);
	}
}

static void Nick_DoAttr(running_machine *machine,int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 = Nick_FetchByte(machine,Nick.LD1);
		Nick.LD1++;

		Buf2 = Nick_FetchByte(machine,Nick.LD2);
		Nick.LD2++;

		{
			unsigned char BackgroundColour = ((Buf1>>4) & 0x0f);
			unsigned char ForegroundColour = (Buf1 & 0x0f);

			Nick_WritePixels2ColourLPIXEL(BackgroundColour, ForegroundColour, Buf2);
		}
	}
}

static void Nick_DoCh256(running_machine *machine,int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 = Nick_FetchByte(machine,Nick.LD1);
		Nick.LD1++;
		Buf2 = Nick_FetchByte(machine,ADDR_CH256(Nick.LD2, Buf1));

		Nick_WritePixelsLPIXEL(Buf2,Buf1);
	}
}

static void Nick_DoCh128(running_machine *machine,int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 = Nick_FetchByte(machine,Nick.LD1);
		Nick.LD1++;
		Buf2 = Nick_FetchByte(machine,ADDR_CH128(Nick.LD2, Buf1));

		Nick_WritePixelsLPIXEL(Buf2,Buf1);
	}
}

static void Nick_DoCh64(running_machine *machine,int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 = Nick_FetchByte(machine,Nick.LD1);
		Nick.LD1++;
		Buf2 = Nick_FetchByte(machine,ADDR_CH64(Nick.LD2, Buf1));

		Nick_WritePixelsLPIXEL(Buf2,Buf1);
	}
}


static void Nick_DoDisplay(running_machine *machine,LPT_ENTRY *pLPT)
{
	unsigned char ClocksVisible;
	unsigned char RightMargin, LeftMargin;

	LeftMargin = NICK_GET_LEFT_MARGIN(pLPT->LM);
	RightMargin = NICK_GET_RIGHT_MARGIN(pLPT->RM);

	ClocksVisible = RightMargin - LeftMargin;

	if (ClocksVisible!=0)
	{
		unsigned char DisplayMode;

		/* get display mode */
		DisplayMode = NICK_GET_DISPLAY_MODE(pLPT->MB);

		if ((Nick.ScanLineCount == 0))	// ||
			//((pLPT->MB & NICK_MB_VRES)==0))
		{
			/* doing first line */
			/* reload LD1, and LD2 (if necessary) regardless of display mode */
			Nick.LD1 = 	(pLPT->LD1L & 0x0ff) |
					((pLPT->LD1H & 0x0ff)<<8);

			if ((DisplayMode != NICK_LPIXEL_MODE) && (DisplayMode != NICK_PIXEL_MODE))
			{
				/* lpixel and pixel modes don't use LD2 */
				Nick.LD2 = (pLPT->LD2L & 0x0ff) |
					((pLPT->LD2H & 0x0ff)<<8);
			}
		}
		else
		{
			/* not first line */

			switch (DisplayMode)
			{
				case NICK_ATTR_MODE:
				{
					/* reload LD1 */
					Nick.LD1 = (pLPT->LD1L & 0x0ff) |
					((pLPT->LD1H & 0x0ff)<<8);
				}
				break;

				case NICK_CH256_MODE:
				case NICK_CH128_MODE:
				case NICK_CH64_MODE:
				{
					/* reload LD1 */
					Nick.LD1 = (pLPT->LD1L & 0x0ff) |
						((pLPT->LD1H & 0x0ff)<<8);
					Nick.LD2++;
				}
				break;

				default:
					break;
			}
		}

		switch (DisplayMode)
		{
			case NICK_PIXEL_MODE:
			{
                            Nick_DoPixel(machine,ClocksVisible);
			}
			break;

			case NICK_ATTR_MODE:
			{
				//mame_printf_info("attr mode\r\n");
                            Nick_DoAttr( machine,ClocksVisible);
			}
			break;

			case NICK_CH256_MODE:
			{
				//mame_printf_info("ch256 mode\r\n");
				Nick_DoCh256(machine,ClocksVisible);
			}
			break;

			case NICK_CH128_MODE:
			{
				Nick_DoCh128(machine,ClocksVisible);
			}
			break;

			case NICK_CH64_MODE:
			{
				//mame_printf_info("ch64 mode\r\n");
				Nick_DoCh64(machine,ClocksVisible);
			}
			break;

			case NICK_LPIXEL_MODE:
			{
				Nick_DoLPixel(machine,ClocksVisible);
			}
			break;

			default:
				break;
		}
	}
}

static void	Nick_UpdateLPT(void)
{
	unsigned long CurLPT;

	CurLPT = (Nick.LPL & 0x0ff) | ((Nick.LPH & 0x0f)<<8);
	CurLPT++;
	Nick.LPL = CurLPT & 0x0ff;
	Nick.LPH = (Nick.LPH & 0x0f0) | ((CurLPT>>8) & 0x0f);
}


static void	Nick_ReloadLPT(running_machine *machine)
{
	unsigned long LPT_Addr;

		/* get addr of LPT */
		LPT_Addr = ((Nick.LPL & 0x0ff)<<4) | ((Nick.LPH & 0x0f)<<(8+4));

		/* update internal LPT state */
		Nick.LPT.SC = Nick_FetchByte(machine,LPT_Addr);
		Nick.LPT.MB = Nick_FetchByte(machine,LPT_Addr+1);
		Nick.LPT.LM = Nick_FetchByte(machine,LPT_Addr+2);
		Nick.LPT.RM = Nick_FetchByte(machine,LPT_Addr+3);
		Nick.LPT.LD1L = Nick_FetchByte(machine,LPT_Addr+4);
		Nick.LPT.LD1H = Nick_FetchByte(machine,LPT_Addr+5);
		Nick.LPT.LD2L = Nick_FetchByte(machine,LPT_Addr+6);
		Nick.LPT.LD2H = Nick_FetchByte(machine,LPT_Addr+7);
		Nick.LPT.COL[0] = Nick_FetchByte(machine,LPT_Addr+8);
		Nick.LPT.COL[1] = Nick_FetchByte(machine,LPT_Addr+9);
		Nick.LPT.COL[2] = Nick_FetchByte(machine,LPT_Addr+10);
		Nick.LPT.COL[3] = Nick_FetchByte(machine,LPT_Addr+11);
		Nick.LPT.COL[4] = Nick_FetchByte(machine,LPT_Addr+12);
		Nick.LPT.COL[5] = Nick_FetchByte(machine,LPT_Addr+13);
		Nick.LPT.COL[6] = Nick_FetchByte(machine,LPT_Addr+14);
		Nick.LPT.COL[7] = Nick_FetchByte(machine,LPT_Addr+15);
}

/* call here to render a line of graphics */
static void	Nick_DoLine(running_machine *machine)
{
	unsigned char ScanLineCount;

	if ((Nick.LPT.MB & NICK_MB_LPT_RELOAD)!=0)
	{
		/* reload LPT */

		Nick.LPL = Nick.Reg[2];
		Nick.LPH = Nick.Reg[3];

		Nick_ReloadLPT(machine);
	}

		/* left border */
		Nick_DoLeftMargin(&Nick.LPT);

		/* do visible part */
		Nick_DoDisplay(machine,&Nick.LPT);

		/* right border */
		Nick_DoRightMargin(&Nick.LPT);

	// 0x0f7 is first!
	/* scan line count for this LPT */
	ScanLineCount = ((~Nick.LPT.SC)+1) & 0x0ff;

//  printf("ScanLineCount %02x\r\n",ScanLineCount);

	/* update count of scanlines done so far */
	Nick.ScanLineCount++;

	if (((unsigned char)Nick.ScanLineCount) ==
		((unsigned char)ScanLineCount))
	{
		/* done all scanlines of this Line Parameter Table, get next */


		Nick.ScanLineCount = 0;

		Nick_UpdateLPT();
		Nick_ReloadLPT(machine);


	}
}

/* MESS specific */
static int	Nick_vh_start(void)
{
  Nick_Init();
  return 0;
}

#ifdef UNUSED_FUNCTION
READ8_HANDLER( nick_reg_r )
{
  /* read from a nick register - return floating bus,
   because the registers are not readable! */
  return 0x0ff;
}
#endif

WRITE8_HANDLER( epnick_reg_w )
{
	//mame_printf_info("Nick write %02x %02x\r\n",offset, data);

  /* write to a nick register */
  Nick.Reg[offset & 0x0f] = data;

  if ((offset == 0x03) || (offset == 0x02))
  {
    /* write LPH */

    /* reload LPT base? */
//    if (NICK_RELOAD_LPT(data))
    {
      /* reload LPT base pointer */
      Nick.LPL = Nick.Reg[2];
      Nick.LPH = Nick.Reg[3];

	Nick_ReloadLPT(space->machine);
    }
  }

  if (offset == 0x01)
  {
	Nick.BORDER = data;
  }

  if (offset == 0x00)
  {
	Nick.FIXBIAS = data;
  }
}

static void    Nick_DoScreen(running_machine *machine,bitmap_t *bm)
{
  int line = 0;

  do
  {

    /* set write address for line */
    Nick.dest = BITMAP_ADDR16(bm, line, 0);
	Nick.dest_pos = 0;
	Nick.dest_max_pos = bm->width;

    /* write line */
    Nick_DoLine(machine);

    /* next line */
    line++;
  }
  while (((Nick.LPT.MB & 0x080)==0) && (line<ENTERPRISE_SCREEN_HEIGHT));

}


VIDEO_START( epnick )
{
	Nick_vh_start();
	VIDEO_START_CALL(generic_bitmapped);
}


VIDEO_UPDATE( epnick )
{
	Nick_DoScreen(screen->machine,tmpbitmap);
	return VIDEO_UPDATE_CALL(generic_bitmapped);
}
