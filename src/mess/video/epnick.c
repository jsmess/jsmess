/* NICK GRAPHICS CHIP */
/* (found in Enterprise) */
/* this is a display list graphics chip, with bitmap,
character and attribute graphics modes. Each entry in the
display list defines a char line, with variable number of
scanlines. Colour modes are 2,4, 16 and 256 colour.
Nick has 256 colours, 3 bits for R and G, with 2 bits for Blue.
It's a nice and flexible graphics processor..........
 */

#include "nick.h"
#include "epnick.h"

/*************************************************************/
/* MESS stuff */
static NICK_STATE Nick;

// MESS specific
/* fetch a byte from "video ram" at Addr specified */
static char Nick_FetchByte(unsigned long Addr)
{
   return mess_ram[Addr & 0x0ffff];
}

// MESS specific
/* 8-bit pixel write! */
#define NICK_WRITE_PIXEL(ci, dest)	\
	*dest = Machine->pens[ci];	\
	dest++

/*****************************************************/


/* Enterprise has 256 colours, all may be on the screen at once!
the NICK_GET_RED8, NICK_GET_GREEN8, NICK_GET_BLUE8 macros
return a 8-bit colour value for the index specified.  */
static unsigned char nick_colour_palette[256*3];

/* the mapping doesn't change - EP uses 0..256 index's for
colour specification - don't know if I require this for the
driver or not. */
static unsigned short nick_colour_table[256]=
{
	0,1,2,3,4,5,6,7,8,
	9,10,11,12,13,14,15,16,
	17,18,19,20,21,22,23,24,
	25,26,27,28,29,30,31,32,
	33,34,35,36,37,38,39,40,
	41,42,43,44,45,46,47,48,
	49,50,51,52,53,54,55,56,
	57,58,59,60,61,62,63,64,
	65,66,67,68,69,70,71,72,
	73,74,75,76,77,78,79,80,
	81,82,83,84,85,86,87,88,
	89,90,91,92,93,94,95,96,
	97,98,99,100,101,102,103,104,
	105,106,107,108,109,110,111,
	112,113,114,115,116,117,118,
	119,120,121,122,123,124,125,
	126,127,128,129,130,131,132,
	133,134,135,136,137,138,139,
	140,141,142,143,144,145,146,
	147,148,149,150,151,152,153,
	154,155,156,157,158,159,160,
	161,162,163,164,165,166,167,
	168,169,170,171,172,173,174,
	175,176,177,178,179,180,181,
	182,183,184,185,186,187,188,
	189,190,191,192,193,194,195,
	196,197,198,199,200,201,202,
	203,204,205,206,207,208,209,
	210,211,212,213,214,215,216,
	217,218,219,220,221,222,223,
	224,225,226,227,228,229,230,
	231,232,233,234,235,236,237,
	238,239,240,241,242,243,244,
	245,246,247,248,249,250,251,
	252,253,254,255
};


/* initial the palette */
PALETTE_INIT( nick )
{
	int i;

	for (i=0; i<256; i++)
	{
		nick_colour_palette[(i*3)] = NICK_GET_RED8(i);
		nick_colour_palette[(i*3)+1] = NICK_GET_GREEN8(i);
		nick_colour_palette[(i*3)+2] = NICK_GET_BLUE8(i);
	}

	palette_set_colors(machine, 0, nick_colour_palette, sizeof(nick_colour_palette) / 3);
	memcpy(colortable, nick_colour_table, sizeof(nick_colour_table));
	//color_prom=malloc(0*sizeof(char));
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

	Nick_CalcVisibleClocks(50*16);

//	Nick.BORDER = 0;
//	Nick.FIXBIAS = 0;
}

/* write border colour */
static void	Nick_WriteBorder(int Clocks)
{
	int i;
	int ColIndex = Nick.BORDER;

	for (i=0; i<(Clocks<<4); i++)
	{
		NICK_WRITE_PIXEL(ColIndex, Nick.dest);
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

		NICK_WRITE_PIXEL(PenIndex, Nick.dest);

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

		NICK_WRITE_PIXEL(PenIndex, Nick.dest);
		NICK_WRITE_PIXEL(PenIndex, Nick.dest);

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

//			if (Nick.LPT.RM & NICK_RM_ALTIND0)
//			{
//				if (Data & 0x040)
//				{
//					PenOffset|=0x04;
//				}
//			}



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

				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);

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

				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);

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

			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);


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

//			if (Nick.LPT.RM & NICK_RM_ALTIND0)
//			{
//				if (Data & 0x040)
//				{
//					PenOffset|=0x04;
//				}
//			}



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

				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);

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

				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);
				NICK_WRITE_PIXEL(PalIndex, Nick.dest);

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

			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);

			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);
			NICK_WRITE_PIXEL(PalIndex, Nick.dest);


		}
		break;
	}
}


static void Nick_DoPixel(int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 =  Nick_FetchByte(Nick.LD1);
		Nick.LD1++;

		Buf2 = Nick_FetchByte(Nick.LD1);
		Nick.LD1++;

		Nick_WritePixels(Buf1,Buf1);

		Nick_WritePixels(Buf2,Buf1);
	}
}


static void Nick_DoLPixel(int ClocksVisible)
{
	int i;
	unsigned char Buf1;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 =  Nick_FetchByte(Nick.LD1);
		Nick.LD1++;

		Nick_WritePixelsLPIXEL(Buf1,Buf1);
	}
}

static void Nick_DoAttr(int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 = Nick_FetchByte(Nick.LD1);
		Nick.LD1++;

		Buf2 = Nick_FetchByte(Nick.LD2);
		Nick.LD2++;

		{
			unsigned char BackgroundColour = ((Buf1>>4) & 0x0f);
			unsigned char ForegroundColour = (Buf1 & 0x0f);

			Nick_WritePixels2ColourLPIXEL(BackgroundColour, ForegroundColour, Buf2);
		}
	}
}

static void Nick_DoCh256(int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 = Nick_FetchByte(Nick.LD1);
		Nick.LD1++;
		Buf2 = Nick_FetchByte(ADDR_CH256(Nick.LD2, Buf1));

		Nick_WritePixelsLPIXEL(Buf2,Buf1);
	}
}

static void Nick_DoCh128(int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 = Nick_FetchByte(Nick.LD1);
		Nick.LD1++;
		Buf2 = Nick_FetchByte(ADDR_CH128(Nick.LD2, Buf1));

		Nick_WritePixelsLPIXEL(Buf2,Buf1);
	}
}

static void Nick_DoCh64(int ClocksVisible)
{
	int i;
	unsigned char Buf1, Buf2;

	for (i=0; i<ClocksVisible; i++)
	{
		Buf1 = Nick_FetchByte(Nick.LD1);
		Nick.LD1++;
		Buf2 = Nick_FetchByte(ADDR_CH64(Nick.LD2, Buf1));

		Nick_WritePixelsLPIXEL(Buf2,Buf1);
	}
}


static void Nick_DoDisplay(LPT_ENTRY *pLPT)
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
                            Nick_DoPixel(ClocksVisible);
			}
			break;

			case NICK_ATTR_MODE:
			{
				//mame_printf_info("attr mode\r\n");
                            Nick_DoAttr(ClocksVisible);
			}
			break;

			case NICK_CH256_MODE:
			{
				//mame_printf_info("ch256 mode\r\n");
				Nick_DoCh256(ClocksVisible);
			}
			break;

			case NICK_CH128_MODE:
			{
				Nick_DoCh128(ClocksVisible);
			}
			break;

			case NICK_CH64_MODE:
			{
				//mame_printf_info("ch64 mode\r\n");
				Nick_DoCh64(ClocksVisible);
			}
			break;

			case NICK_LPIXEL_MODE:
			{
				Nick_DoLPixel(ClocksVisible);
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


static void	Nick_ReloadLPT(void)
{
	unsigned long LPT_Addr;

		/* get addr of LPT */
		LPT_Addr = ((Nick.LPL & 0x0ff)<<4) | ((Nick.LPH & 0x0f)<<(8+4));

		/* update internal LPT state */
		Nick.LPT.SC = Nick_FetchByte(LPT_Addr);
		Nick.LPT.MB = Nick_FetchByte(LPT_Addr+1);
		Nick.LPT.LM = Nick_FetchByte(LPT_Addr+2);
		Nick.LPT.RM = Nick_FetchByte(LPT_Addr+3);
		Nick.LPT.LD1L = Nick_FetchByte(LPT_Addr+4);
		Nick.LPT.LD1H = Nick_FetchByte(LPT_Addr+5);
		Nick.LPT.LD2L = Nick_FetchByte(LPT_Addr+6);
		Nick.LPT.LD2H = Nick_FetchByte(LPT_Addr+7);
		Nick.LPT.COL[0] = Nick_FetchByte(LPT_Addr+8);
		Nick.LPT.COL[1] = Nick_FetchByte(LPT_Addr+9);
		Nick.LPT.COL[2] = Nick_FetchByte(LPT_Addr+10);
		Nick.LPT.COL[3] = Nick_FetchByte(LPT_Addr+11);
		Nick.LPT.COL[4] = Nick_FetchByte(LPT_Addr+12);
		Nick.LPT.COL[5] = Nick_FetchByte(LPT_Addr+13);
		Nick.LPT.COL[6] = Nick_FetchByte(LPT_Addr+14);
		Nick.LPT.COL[7] = Nick_FetchByte(LPT_Addr+15);
}

/* call here to render a line of graphics */
static void	Nick_DoLine(void)
{
	unsigned char ScanLineCount;

	if ((Nick.LPT.MB & NICK_MB_LPT_RELOAD)!=0)
	{
		/* reload LPT */

		Nick.LPL = Nick.Reg[2];
		Nick.LPH = Nick.Reg[3];

		Nick_ReloadLPT();
	}

		/* left border */
		Nick_DoLeftMargin(&Nick.LPT);

		/* do visible part */
		Nick_DoDisplay(&Nick.LPT);

		/* right border */
		Nick_DoRightMargin(&Nick.LPT);

	// 0x0f7 is first!
	/* scan line count for this LPT */
	ScanLineCount = ((~Nick.LPT.SC)+1) & 0x0ff;

//	printf("ScanLineCount %02x\r\n",ScanLineCount);

	/* update count of scanlines done so far */
	Nick.ScanLineCount++;

	if (((unsigned char)Nick.ScanLineCount) ==
		((unsigned char)ScanLineCount))
	{
		/* done all scanlines of this Line Parameter Table, get next */


		Nick.ScanLineCount = 0;

		Nick_UpdateLPT();
		Nick_ReloadLPT();


	}
}

/* MESS specific */
int	Nick_vh_start(void)
{
  Nick_Init();
  return 0;
}

#if 0
int	Nick_reg_r(int RegIndex)
{
  /* read from a nick register - return floating bus,
   because the registers are not readable! */
  return 0x0ff;
}
#endif

void	Nick_reg_w(int RegIndex, int Data)
{
	//mame_printf_info("Nick write %02x %02x\r\n",RegIndex, Data);

  /* write to a nick register */
  Nick.Reg[RegIndex & 0x0f] = Data;

  if ((RegIndex == 0x03) || (RegIndex == 0x02))
  {
    /* write LPH */

    /* reload LPT base? */
//    if (NICK_RELOAD_LPT(Data))
    {
      /* reload LPT base pointer */
      Nick.LPL = Nick.Reg[2];
      Nick.LPH = Nick.Reg[3];

	Nick_ReloadLPT();
    }
  }

  if (RegIndex == 0x01)
  {
	Nick.BORDER = Data;
  }

  if (RegIndex == 0x00)
  {
	Nick.FIXBIAS = Data;
  }
}

void    Nick_DoScreen(mame_bitmap *bm)
{
  int line = 0;

  do
  {

    /* set write address for line */
    Nick.dest = BITMAP_ADDR16(bm, line, 0);

    /* write line */
    Nick_DoLine();

    /* next line */
    line++;
  }
  while (((Nick.LPT.MB & 0x080)==0) && (line<(35*8)));

}






















