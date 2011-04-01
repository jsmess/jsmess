/*
    video/dgn_beta.c

The Dragon Beta uses a 68B45 for it's display generation, this is used in the
conventional wat with a character generator ROM in the two text modes, which are
standard 40x25 and 80x25. In adition to the 6845 there is some TTL logic which
provides colour and attributes. In text modes the video ram is organised as pairs
of character and attribute, in alternate bytes.

The attributes decode as follows :-

    7-6-5-4-3-2-1-0
    f-u-F-F-F-B-B-B

    f=flash
    u=underline

    FFF = foreground colour
    BBB = bakcground colour

        000 black
        001 red
        010 green
        011 yellow
        100 blue
        101 magenta
        110 cyan
        111 white

    If flash is true, foreground and background colours will be exchanged.

It is interesting to note that the 6845 uses 16 bit wide access to the ram, in contrast
to the 8 bit accesses from the CPUs, this allows each increment of the MA lines to move
2 bytes at a time, and therefore feed both the character rom and the attribute decode
circuit simultaniously.

The RAM addresses are made up of two parts, the MA0..13 from the 6845, plus two output
lines from the 6821 PIA, I28, the lines are BP6 and PB7, with PB7 being the most
significant. This effectivly allows the 6845 access to the first 128K of memory, as there
are 16 address lines, accessing a 16 bit wide memory.

The relationship between how the cpu sees the RAM, and the way the 6845 sees it is simply
CPU addr=2x6845 addr. So for the default video address of $1F000, the CPU sees this as
being at $1F000 (after DAT trasnlation). The 6845 is programmed to start it's MA lines
counting at $3800, plus A14 and A15 being supplied by PB6 and PB7 from I28, gives an address
of $F800, which is the same as $1F000 / 2.

I am currently at this time not sure of how any of the graphics modes, work, this will need
further investigation.

However the modes supported are :-

Text Modes
        width   height  colours
        40  25  8
        80  25  8
Graphics modes
        320 256 4
        320 256 16
        640 512 4
        640 256 4**
        640 512 2

Looking at the parts of the circuit sheet that I have seen it looks like the graphics modes
are driven using a combination of the 6845 MA and RA lines to address more than 64K or memory
which is needed for some of the modes above (specifically, 640x512x4, which needs 80K).

2006-11-30, Text mode is almost completely implemented, in both 40 and 80 column modes.
I have made a start on implementing the graphics modes of the beta, drawing from technical
documents, from the project, this is still a work in progress. I have however managed to get
it to display a distorted graphical image, so I know I am going in the correct direction !

** 2006-12-05, this mode is not documented in any of the printed documentation, however it
is supported and displayed by the graphics test rom, it is basically the 640x512x4 mode with
half the number of vertical lines, and in non-interlaced mode.

It seems that the 640x512 modes operate the 6845 in interlaced mode, just how this affects
the access to the video memory is unclear to me at the moment.

*/

#include "emu.h"
#include "includes/dgn_beta.h"
#include "video/m6845.h"
#include "machine/ram.h"

#include "debug/debugcpu.h"
#include "debug/debugcon.h"

#define LOG_VIDEO

/* Names for 6845 regs, to save having to remember their offsets */
/* perhaps these should move into 6845 header ? */
typedef enum {
	H_TOTAL = 0,		// Horizontal total
	H_DISPLAYED,		// Horizontal displayed
	H_SYNC_POS,		    // Horizontal sync pos
	H_SYNC_WIDTH,		// Horizontal sync width
	V_TOTAL,		    // Vertical total
	V_TOTAL_ADJ,		// Vertical total adjust
	V_DISPLAYED,		// Vertical displayed
	V_SYNC_POS,		    // Vertical sync pos
	INTERLACE,		    // Interlace
	MAX_SCAN,		    // Maximum scan line
	CURS_START,		    // Cursor start pos
	CURS_END,		    // Cursor end pos
	START_ADDR_H,		// Start address High
	START_ADDR_L,		// Start address low
	CURS_H,			    // Cursor addr High
	CURS_L			    // CURSOR addr Low
} m6845_regs;


//static BETA_VID_MODES VIDMODE = TEXT_40x25;

/* Debugging variables */

/* Debugging commands and handlers. */
static void execute_beta_vid_log(running_machine &machine, int ref, int params, const char *param[]);
static void RegLog(running_machine &machine, int offset, int data);
static void execute_beta_vid_fill(running_machine &machine, int ref, int params, const char *param[]);
static void execute_beta_vid_box(running_machine &machine, int ref, int params, const char *param[]);
static void execute_beta_vid(running_machine &machine, int ref, int params, const char *param[]);
static void execute_beta_vid_limits(running_machine &machine, int ref, int params, const char *param[]);
static void execute_beta_vid_clkmax(running_machine &machine, int ref, int params, const char *param[]);



static void beta_Set_RA(running_machine &machine, int offset, int data);
static void beta_Set_HSync(running_machine &machine, int offset, int data);
static void beta_Set_VSync(running_machine &machine, int offset, int data);
static void beta_Set_DE(running_machine &machine, int offset, int data);

static const struct m6845_interface beta_m6845_interface = {
	0,		        // Memory Address register
	beta_Set_RA,	// Row Address register
	beta_Set_HSync,	// Horizontal status
	beta_Set_VSync,	// Vertical status
	beta_Set_DE,	// Display Enabled status
	0,		        // Cursor status
	0
};





typedef enum {
	INTERLACE_OFF=0,	/* No interlace mode, normal drawing */
	INTERLACE_ON=1,		/* Interlaced mode, use Field to determine which field to draw */
	INTERLACE_AT_VS=2	/* Turn on interlace at next VSync, then go to INTERLACE */
} INTERLACE_MODES;

#define Background	1	/* Background colour, red now so I can see where border is ! */

/* GCtrl bitmasks, infered from bits of Beta schematic */
#define GCtrlWI		0x01
#define GCtrlSWChar	0x02	/* Character set select */
#define GCtrlHiLo	0x04	/* Hi/Lo res graphics, Hi=1, Lo=0 */
#define GCtrlChrGfx	0x08	/* Character=1 / Graphics=0 */
#define GCtrlControl	0x10	/* Control bit, sets direct drive mode */
#define GCtrlFS		0x20	/* labeled F/S, not yet sure of function Fast or Slow scan ? */
#define GCtrlAddrLines	0xC0	/* Top two address lines for text mode */

#define IsTextMode	(state->m_GCtrl & GCtrlChrGfx) ? 1 : 0					// Is this text mode ?
#define IsGfx16 	((~state->m_GCtrl & GCtrlChrGfx) && (~state->m_GCtrl & GCtrlControl)) ? 1 : 0	// is this 320x256x16bpp mode
#define IsGfx2		((state->m_GCtrl & GCtrlHiLo) && (~state->m_GCtrl & GCtrlFS)) ? 1 : 0		// Is this a 2 colour mode
#define SWChar		(state->m_GCtrl & GCtrlSWChar)>>1					// Swchar bit

//static int beta_state;

/* Set video control register from I28 port B, the control register is laid out as */
/* follows :-                                                                      */
/*  bit function                               */
/*  0   WI, unknown                            */
/*  1   Character set select, drives A12 of character rom in text mode     */
/*  2   High (1) or Low(0) resolution if in graphics mode.         */
/*  3   Text (1) or Graphics(0) mode                       */
/*  4   Control bit, Selects between colour palate and drirect drive       */
/*  5   F/S bit, 1=80 bytes/line, 0=40bytes/line               */
/*  6   Effective A14, to ram, in text mode                    */
/*  7   Effective A15, to ram, in text mode                */
/* the top two address lines for the video ram, are supplied by the BB6 and PB7 on */
/* 6821-I28, this allows the 6845 to access the full 64K address range, however    */
/* since the ram data is addressed as a 16bit wide unit, this allows the 6845      */
/* access to the first 128K or ram.                                                */
void dgnbeta_vid_set_gctrl(running_machine &machine, int data)
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	state->m_GCtrl=data;
	if (state->m_LogRegWrites)
		debug_console_printf(machine, "I28-PB=$%2X, %2X-%s-%s-%s-%s-%s-%s PC=%4X\n",
				     data,
				     data & GCtrlAddrLines,
				     data & GCtrlFS 		? "FS" : "  ",
				     data & GCtrlControl	? "CT" : "  ",
				     data & GCtrlChrGfx		? "Ch" : "Gf",
				     data & GCtrlHiLo		? "Hi" : "Lo",
				     data & GCtrlSWChar		? "C0" : "C1",
				     data & GCtrlWI		? "Wi" : "  ",
				     cpu_get_pc(machine.device("maincpu")));
}

// called when the 6845 changes the character row
static void beta_Set_RA(running_machine &machine, int offset, int data)
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	state->m_beta_6845_RA=data;
}

// called when the 6845 changes the HSync
static void beta_Set_HSync(running_machine &machine, int offset, int data)
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	int Dots;	/* Pixels per 16 bits */

	state->m_beta_HSync=data;

	if(IsGfx16)
		Dots=2;
	else
		Dots=8;

	if(!state->m_beta_HSync)
	{
		int HT=m6845_get_register(H_TOTAL);			    // Get H total
		int HS=m6845_get_register(H_SYNC_POS);		    // Get Hsync pos
		int HW=m6845_get_register(H_SYNC_WIDTH)&0xF;	// Hsync width (in chars)

		state->m_beta_scr_y++;
//      state->m_beta_scr_x=0-(((HT-HS)-HW)*8);  // Number of dots after HS to wait before start of next line
		state->m_beta_scr_x=0-((HT-(HS+HW))*Dots);

//debug_console_printf(machine, "HT=%d, HS=%d, HW=%d, (HS+HW)=%d, HT-(HS+HW)=%d\n",HT,HS,HW,(HS+HW),(HT-(HS+HW)));
//debug_console_printf(machine, "Scanline=%d, row=%d\n",m6845_get_scanline_counter(),m6845_get_row_counter());
		state->m_HSyncMin=state->m_beta_scr_x;
	}
}

// called when the 6845 changes the VSync
static void beta_Set_VSync(running_machine &machine, int offset, int data)
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	state->m_beta_VSync=data;
	if (!state->m_beta_VSync)
	{
		state->m_beta_scr_y = -20;
		state->m_FlashCount++;
		if(state->m_FlashCount==10)
		{
			state->m_FlashCount=0;			// Reset counter
			state->m_FlashBit=(!state->m_FlashBit) & 0x01;	// Invert flash bit.
		}

		if (state->m_DrawInterlace==INTERLACE_AT_VS)
		{
			state->m_DrawInterlace=INTERLACE_ON;
			state->m_Field=0;
		}
		else if (state->m_DrawInterlace==INTERLACE_ON)
		{
			state->m_Field=(state->m_Field+1) & 0x01;	/* Invert field */
//          debug_console_printf(machine, "Invert field=%d\n",state->m_Field);
		}
		state->m_VSyncMin=state->m_beta_scr_y;
	}

	dgn_beta_frame_interrupt(machine, data);
}

static void beta_Set_DE(running_machine &machine, int offset, int data)
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	state->m_beta_DE = data;

	if(state->m_beta_DE)
		state->m_DEPos=state->m_beta_scr_x;
}

/* Video init */
void dgnbeta_init_video(running_machine &machine)
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	UINT8 *videoram = state->m_videoram;
	/* initialise 6845 */
	m6845_config(&beta_m6845_interface);
	m6845_set_personality(M6845_PERSONALITY_HD6845S);

	state->m_GCtrl=0;

	videoram=ram_get_ptr(machine.device(RAM_TAG));

	state->m_ClkMax=65000;
	state->m_FlashCount=0;
	state->m_FlashBit=0;
	state->m_DoubleHL=1;			/* Default to normal height */
	state->m_s_DoubleY=1;
	state->m_DrawInterlace=INTERLACE_OFF;	/* No interlace by default */

	/* setup debug commands */
	if (machine.debug_flags & DEBUG_FLAG_ENABLED)
	{
		debug_console_register_command(machine, "beta_vid_log", CMDFLAG_NONE, 0, 0, 0, execute_beta_vid_log);
		debug_console_register_command(machine, "beta_vid_fill", CMDFLAG_NONE, 0, 0, 0, execute_beta_vid_fill);
		debug_console_register_command(machine, "beta_vid_box", CMDFLAG_NONE, 0, 0, 5, execute_beta_vid_box);
		debug_console_register_command(machine, "beta_vid", CMDFLAG_NONE, 0, 0, 0, execute_beta_vid);
		debug_console_register_command(machine, "beta_vid_limits", CMDFLAG_NONE, 0, 0, 0, execute_beta_vid_limits);
		debug_console_register_command(machine, "beta_vid_clkmax", CMDFLAG_NONE, 0, 0, 1, execute_beta_vid_clkmax);
	}
	state->m_LogRegWrites=0;
	state->m_BoxColour=1;
	state->m_BoxMinX		= 100;
	state->m_BoxMinY		= 100;
	state->m_BoxMaxX		= 500;
	state->m_BoxMaxY		= 500;

	state->m_MinAddr	    = 0xFFFF;
	state->m_MaxAddr	    = 0x0000;
	state->m_MinX	        = 0xFFFF;
	state->m_MaxX	        = 0x0000;
	state->m_MinY	        = 0xFFFF;
	state->m_MaxY	        = 0x0000;
}

void dgnbeta_video_reset(running_machine &machine)
{
    logerror("dgnbeta_video_reset\n");
    m6845_reset();
}

/**************************/
/** Text screen handlers **/
/**************************/

/* Plot a pixel on beta text screen, takes care of doubling height and width where needed */
static void plot_text_pixel(dgn_beta_state *state, bitmap_t *bitmap, int x, int y,int Dot,int Colour, int CharsPerLine)
{
	int PlotX;
	int PlotY;
	int Double = (~state->m_GCtrl & GCtrlHiLo);

	/* We do this so that we can plot 40 column characters twice as wide */
	if(Double)
	{
		PlotX=(x+Dot)*2;
	}
	else
	{
		PlotX=(x+Dot);
	}

	PlotY=y*2;

	/* Error check, make sure we're drawing onm the actual bitmap ! */
	if (((PlotX+1)<bitmap->width) && ((PlotY+1)<bitmap->height))
	{
		/* As the max resolution of the beta is 640x512, we need to draw characters */
		/* 2 pixel lines high as the with a screen of 40 or 80 by 25 lines of 10 pixels high */
		/* the effective character to pixel resolution is 320x250 or 640x250 */
		/* Plot characters twice as wide in 40 col mode */
		if(Double)
		{
			*BITMAP_ADDR16(bitmap, (PlotY+0), (PlotX+0)) = Colour;
			*BITMAP_ADDR16(bitmap, (PlotY+0), (PlotX+1)) = Colour;

			*BITMAP_ADDR16(bitmap, (PlotY+1), (PlotX+0)) = Colour;
			*BITMAP_ADDR16(bitmap, (PlotY+1), (PlotX+1)) = Colour;
		}
		else
		{
			*BITMAP_ADDR16(bitmap, (PlotY+0), PlotX) = Colour;
			*BITMAP_ADDR16(bitmap, (PlotY+1), PlotX) = Colour;
		}
	}
}

static void beta_plot_char_line(running_machine &machine, int x,int y, bitmap_t *bitmap)
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	UINT8 *videoram = state->m_videoram;
	int CharsPerLine	= m6845_get_register(H_DISPLAYED);	// Get chars per line.
	unsigned char *data = machine.region("gfx1")->base();		// ptr to char rom
	int Dot;
	unsigned char data_byte;
	int char_code;
	int crtcAddr;							// Character to draw
	int Offset;
	int FgColour;							// Foreground colour
	int BgColour;							// Background colour
	int CursorOn		= m6845_cursor_enabled_r(0);
	int Invert;
	int UnderLine;							// Underline
	int FlashChar;							// Flashing char
	int ULActive;							// Underline active

	state->m_bit=bitmap;

	if (state->m_beta_DE)
	{
		/* The beta text RAM contains alternate character and attribute bytes */
		/* Top two address lines from PIA, IC28 PB6,7 */
		crtcAddr=m6845_memory_address_r(0)-1;
		Offset=(crtcAddr | ((state->m_GCtrl & GCtrlAddrLines)<<8))*2;
		if (Offset<0)
			Offset=0;

		/* Get the character to display */
		char_code	= videoram[Offset];

		/* Extract non-colour attributes, in character set 1, undeline is used */
		/* We will extract the colours below, when we have decoded inverse */
		/* to indicate a double height character */
		UnderLine=(videoram[Offset+1] & 0x40) >> 6;
		FlashChar=(videoram[Offset+1] & 0x80) >> 7;

		// underline is active for character set 0, on character row 9
		ULActive=(UnderLine && (state->m_beta_6845_RA==9) && ~SWChar);

		/* If Character set one, and undeline set, latch double height */
		state->m_s_DoubleY=(UnderLine & SWChar & state->m_DoubleHL) |
		        (SWChar & ~state->m_s_DoubleY & state->m_DoubleHL) |
			(SWChar & ~state->m_s_DoubleY & (state->m_beta_6845_RA==9));

		/* Invert forground and background if flashing char and flash acive */
		Invert=(FlashChar & state->m_FlashBit);

		/* Underline inverts flash */
		if (ULActive)
			Invert=~Invert;

		/* Cursor on also inverts */
		if (CursorOn)
			Invert=~Invert;

		/* Invert colours if invert is true */
		if(!Invert)
		{
			FgColour	= (videoram[Offset+1] & 0x38) >> 3;
			BgColour	= (videoram[Offset+1] & 0x07);
		}
		else
		{
			BgColour	= (videoram[Offset+1] & 0x38) >> 3;
			FgColour	= (videoram[Offset+1] & 0x07);
		}

		/* The beta Character ROM has characters of 8x10, each aligned to a 16 byte boundry */
		data_byte = data[(char_code*16) + state->m_beta_6845_RA];

		for (Dot=0; Dot<8; Dot++)
		{
			if (data_byte & 0x080)
				plot_text_pixel(state, bitmap, x, y, Dot, FgColour, CharsPerLine);
			else
				plot_text_pixel(state, bitmap, x, y, Dot, BgColour, CharsPerLine);

			data_byte = data_byte<<1;
		}
	}
	else
	{
		for (Dot=0; Dot<8; Dot++)
			plot_text_pixel(state, bitmap, x, y, Dot, Background, CharsPerLine);

	}

}

/******************************/
/** Graphics screen handlers **/
/******************************/

/* Plot a pixel on the graphics screen, similar to character plotter above */
/* May merge at some point in the future, if they turn out to be sufficiently similar ! */
static void plot_gfx_pixel(dgn_beta_state *state, bitmap_t *bitmap, int x, int y, int Dot, int Colour)
{
	int	DoubleX		= (~state->m_GCtrl & GCtrlHiLo) ? 1 : 0;
	int	DoubleY		= (~m6845_get_register(INTERLACE) & 0x03) ? 1 : 0;
	int	PlotX;
	int	PlotY;

	if (DoubleX)
		PlotX=(x+Dot)*2;
	else
		PlotX=x+Dot;

	if(DoubleY)
		PlotY=(y)*2;
	else
		PlotY=y;

	if(state->m_DrawInterlace==INTERLACE_ON)
	{
		PlotY=(y*2);//+state->m_Field;
		DoubleY=0;
//      debug_console_printf(machine, "Field=%d\n",state->m_Field);
	}

	/* Error check, make sure we're drawing on the actual bitmap ! */
	if (((PlotX+1)<bitmap->width) && ((PlotY+1)<bitmap->height))
	{
		/* As the max resolution of the beta is 640x512, we need to draw pixels */
		/* twice as high and twice as wide when plotting in 320x modes */
		if(DoubleX)
		{
			*BITMAP_ADDR16(bitmap, (PlotY+0), (PlotX+0)) = Colour;
			*BITMAP_ADDR16(bitmap, (PlotY+0), (PlotX+1)) = Colour;
		}

		/* Plot pixels twice as high if in x256 modes */
		if (DoubleY)
		{
			*BITMAP_ADDR16(bitmap, (PlotY+1), (PlotX+0)) = Colour;
			*BITMAP_ADDR16(bitmap, (PlotY+1), (PlotX+1)) = Colour;
		}

		if (~(DoubleX | DoubleY))
		{
			*BITMAP_ADDR16(bitmap, PlotY, PlotX) = Colour;
		}
	}
}

/* Get and plot a graphics bixel block */

static void beta_plot_gfx_line(running_machine &machine,int x,int y, bitmap_t *bitmap)
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	UINT8 *videoram = state->m_videoram;
	int crtcAddr;
	int Addr;
	int Red;
	int Green;
	int Blue;
	int Intense;
	int Colour;
	int Dot;
	int Hi;
	int Lo;
	int Word;
	int Dots;	/* pixesls per byte */

	/* We have to do this here so that it is correct for border and drawing area ! */
	if (IsGfx16)
		Dots=4;
	else if (IsGfx2)
		Dots=16;
	else
		Dots=8;

	if (state->m_beta_DE)
	{
		/* Calculate address of graphics pixels */
		crtcAddr=(m6845_memory_address_r(0) & 0x1FFF);
		Addr=((crtcAddr<<3) | (state->m_beta_6845_RA & 0x07))*2;

		if(Addr<state->m_MinAddr)
			state->m_MinAddr=Addr;

		if(Addr>state->m_MaxAddr)
			state->m_MaxAddr=Addr;

		if(x>state->m_MaxX) state->m_MaxX=x;
		if(x<state->m_MinX) state->m_MinX=x;
		if(y>state->m_MaxY) state->m_MaxY=y;
		if(y<state->m_MinY) state->m_MinY=y;

		Lo	= videoram[Addr];
		Hi	= videoram[Addr+1];
		Word	= (Hi<<8) | Lo;

		/* If contol is low then we are plotting 4 bit per pixel, 16 colour mode */
		/* This directly drives the colour output lines, from the pixel value */
		/* If Control is high, then we lookup the colour from the LS670 4x4 bit */
		/* palate register */
		if (IsGfx16)
		{
			Intense	=(Lo & 0x0F);
			Red	=(Lo & 0xF0)>>4;
			Green	=(Hi & 0x0F);
			Blue	=(Hi & 0xF0)>>4;
			Colour=((Intense&0x08) | (Red&0x08)>>1) | ((Green&0x08)>>2) | ((Blue&0x08)>>3);

			for (Dot=0;Dot<Dots;Dot++)
			{
				plot_gfx_pixel(state,bitmap,x,y,Dot,Colour);

				Intense	=Intense<<1;
				Red	=Red<<1;
				Green	=Green<<1;
				Blue	=Blue<<1;
			}
		}
		else if (IsGfx2)
		{
			for (Dot=0;Dot<Dots;Dot=Dot+1)
			{
				Colour=state->m_ColourRAM[((Word&0x8000)>>15)];
				plot_gfx_pixel(state,bitmap,x,y,Dot,Colour);

//              Colour=state->m_ColourRAM[((Word&0x0080)>>7)];
//              plot_gfx_pixel(state,bitmap,x,y,Dot+1,Colour);

				Hi=Word&0x8000;
				Word=((Word<<1)&0xFFFE) | (Hi>>15);
			}
		}
		else
		{
			for (Dot=0;Dot<Dots;Dot++)
			{
				Colour=state->m_ColourRAM[((Word&0x8000)>>14) | ((Word&0x80)>>7)];
				plot_gfx_pixel(state,bitmap,x,y,Dot,Colour);

				Hi=Word&0x8000;
				Word=((Word<<1)&0xFFFE) | (Hi>>15);
			}
		}

	}
	else
	{
		for (Dot=0;Dot<Dots;Dot++)
		{
			plot_gfx_pixel(state,bitmap,x,y,Dot,Background);
		}
	}
}

/* Update video screen, calls either text or graphics update routine as needed */
SCREEN_UPDATE( dgnbeta )
{
	dgn_beta_state *state = screen->machine().driver_data<dgn_beta_state>();
	long c=0; // this is used to time out the screen redraw, in the case that the 6845 is in some way out state.

	state->m_bit=bitmap;

	c=0;

	// loop until the end of the Vertical Sync pulse
	while((state->m_beta_VSync)&&(c<state->m_ClkMax))
	{
		// Clock the 6845
		m6845_clock(screen->machine());
		c++;
	}

	// loop until the Vertical Sync pulse goes high
	// or until a timeout (this catches the 6845 with silly register values that would not give a VSYNC signal)
	while((!state->m_beta_VSync)&&(c<state->m_ClkMax))
	{
		while ((state->m_beta_HSync)&&(c<state->m_ClkMax))
		{
			m6845_clock(screen->machine());
			c++;
		}

		while ((!state->m_beta_HSync)&&(c<state->m_ClkMax))
		{
			// check that we are on the emulated screen area.
			if ((state->m_beta_scr_x>=0) && (state->m_beta_scr_x<699) && (state->m_beta_scr_y>=0) && (state->m_beta_scr_y<549))
			{
				if(IsTextMode)
					beta_plot_char_line(screen->machine(), state->m_beta_scr_x, state->m_beta_scr_y, bitmap);
				else
					beta_plot_gfx_line(screen->machine(), state->m_beta_scr_x, state->m_beta_scr_y, bitmap);
			}

			/* In direct drive mode we have 4bpp, so 4 pixels per word, so increment x by 4 */
			/* In lookup mode we have 2bpp, so 8 pixels per word, so increment by 8 */
			/* In text mode characters are 8 pixels wide, so increment by 8 */
			if (IsGfx16)
				state->m_beta_scr_x+=4;
			else if (IsGfx2)
				state->m_beta_scr_x+=16;
			else
				state->m_beta_scr_x+=8;

			// Clock the 6845
			m6845_clock(screen->machine());
			c++;
		}
	}
	return 0;
}

/* Read and write handlers for CPU interface to 6845 */
READ8_HANDLER(dgnbeta_6845_r)
{
	return m6845_register_r(offset);
}

WRITE8_HANDLER(dgnbeta_6845_w)
{
	dgn_beta_state *state = space->machine().driver_data<dgn_beta_state>();
	if(offset&0x1)
	{
		m6845_register_w(offset,data);

		if(state->m_VidAddr==INTERLACE)
		{
			state->m_DrawInterlace=(data & 0x03) ? INTERLACE_AT_VS : INTERLACE_OFF;
		}
	}
	else
	{
		m6845_address_w(offset,data);
		state->m_VidAddr=data;				        /* Record reg being written to */
	}
	if (state->m_LogRegWrites)
		RegLog(space->machine(), offset,data);
}

/* Write handler for colour, pallate ram */
WRITE8_HANDLER(dgnbeta_colour_ram_w)
{
	dgn_beta_state *state = space->machine().driver_data<dgn_beta_state>();
	state->m_ColourRAM[offset]=data&0x0f;			/* Colour ram 4 bit and write only to CPU */
}

/*************************************
 *
 *  Debugging
 *
 *************************************/

static void execute_beta_vid_log(running_machine &machine, int ref, int params, const char *param[])
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	state->m_LogRegWrites=!state->m_LogRegWrites;

	debug_console_printf(machine, "6845 register write info set : %d\n",state->m_LogRegWrites);
}


static void RegLog(running_machine &machine, int offset, int data)
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	char	RegName[16];

	switch (state->m_VidAddr)
	{
		case H_TOTAL		: sprintf(RegName,"H Total      "); break;
		case H_DISPLAYED	: sprintf(RegName,"H Displayed  "); break;
		case H_SYNC_POS		: sprintf(RegName,"H Sync Pos   "); break;
		case H_SYNC_WIDTH	: sprintf(RegName,"H Sync Width "); break;
		case V_TOTAL		: sprintf(RegName,"V Total      "); break;
		case V_TOTAL_ADJ	: sprintf(RegName,"V Total Adj  "); break;
		case V_DISPLAYED	: sprintf(RegName,"V Displayed  "); break;
		case V_SYNC_POS		: sprintf(RegName,"V Sync Pos   "); break;
		case INTERLACE		: sprintf(RegName,"Interlace    "); break;
		case MAX_SCAN		: sprintf(RegName,"Max Scan Line"); break;
		case CURS_START		: sprintf(RegName,"Cusror Start "); break;
		case CURS_END		: sprintf(RegName,"Cusrsor End  "); break;
		case START_ADDR_H	: sprintf(RegName,"Start Addr H "); break;
		case START_ADDR_L	: sprintf(RegName,"Start Addr L "); break;
		case CURS_H		    : sprintf(RegName,"Cursor H     "); break;
		case CURS_L		    : sprintf(RegName,"Cursor L     "); break;
	}

	if(offset&0x1)
		debug_console_printf(machine, "6845 write Reg %s Addr=%3d Data=%3d ($%02X) \n",RegName,state->m_VidAddr,data,data);
}

static void execute_beta_vid_fill(running_machine &machine, int ref, int params, const char *param[])
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	int	x;

	for(x=1;x<899;x++)
	{
		plot_box(state->m_bit, x, 1, 1, 698, x & 0x07);
	}
	state->m_NoScreen=1;
}

static void execute_beta_vid_box(running_machine &machine, int ref, int params, const char *param[])
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	int	x,y;

	if(params>0)	sscanf(param[0],"%d",&state->m_BoxMinX);
	if(params>1)	sscanf(param[1],"%d",&state->m_BoxMinY);
	if(params>2)	sscanf(param[2],"%d",&state->m_BoxMaxX);
	if(params>3)	sscanf(param[3],"%d",&state->m_BoxMaxY);
	if(params>4)	sscanf(param[4],"%d",&state->m_BoxColour);

	for(x=state->m_BoxMinX;x<state->m_BoxMaxX;x++)
	{
		*BITMAP_ADDR16(state->m_bit, state->m_BoxMinY, x) = state->m_BoxColour;
		*BITMAP_ADDR16(state->m_bit, state->m_BoxMaxY, x) = state->m_BoxColour;
	}

	for(y=state->m_BoxMinY;y<state->m_BoxMaxY;y++)
	{
		*BITMAP_ADDR16(state->m_bit, y, state->m_BoxMinX) = state->m_BoxColour;
		*BITMAP_ADDR16(state->m_bit, y, state->m_BoxMaxX) = state->m_BoxColour;
	}
	debug_console_printf(machine, "ScreenBox()\n");
}


static void execute_beta_vid(running_machine &machine, int ref, int params, const char *param[])
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	state->m_NoScreen=!state->m_NoScreen;
}

static void execute_beta_vid_limits(running_machine &machine, int ref, int params, const char *param[])
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	debug_console_printf(machine, "Min X     =$%4X, Max X     =$%4X\n",state->m_MinX,state->m_MaxX);
	debug_console_printf(machine, "Min Y     =$%4X, Max Y     =$%4X\n",state->m_MinY,state->m_MaxY);
	debug_console_printf(machine, "MinVidAddr=$%5X, MaxVidAddr=$%5X\n",state->m_MinAddr,state->m_MaxAddr);
	debug_console_printf(machine, "HsyncMin  =%d, VSyncMin=%d\n",state->m_HSyncMin, state->m_VSyncMin);
	debug_console_printf(machine, "Interlace =%d\n",state->m_DrawInterlace);
	debug_console_printf(machine, "DEPos=%d\n",state->m_DEPos);
	if (IsGfx16)
		debug_console_printf(machine, "Gfx16\n");
	else if (IsGfx2)
		debug_console_printf(machine, "Gfx2\n");
	else
		debug_console_printf(machine, "Gfx4/Text\n");
}

static void execute_beta_vid_clkmax(running_machine &machine, int ref, int params, const char *param[])
{
	dgn_beta_state *state = machine.driver_data<dgn_beta_state>();
	if(params>0)	sscanf(param[0],"%d",&state->m_ClkMax);
}
