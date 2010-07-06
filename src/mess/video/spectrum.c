/***************************************************************************

  spectrum.c

  Functions to emulate the video hardware of the ZX Spectrum.

  Changes:

  DJR 08/02/00 - Added support for FLASH 1.
  DJR 16/05/00 - Support for TS2068/TC2048 hires and 64 column modes.
  DJR 19/05/00 - Speeded up Spectrum 128 screen refresh.
  DJR 23/05/00 - Preliminary support for border colour emulation.

***************************************************************************/

#include "emu.h"
#include "includes/spectrum.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/
VIDEO_START( spectrum )
{
	spectrum_state *state = (spectrum_state *)machine->driver_data;
	state->frame_number = 0;
	state->flash_invert = 0;

	EventList_Initialise(machine, 30000);

	state->retrace_cycles = SPEC_RETRACE_CYCLES;

	state->screen_location = state->video_ram;
}

VIDEO_START( spectrum_128 )
{
	spectrum_state *state = (spectrum_state *)machine->driver_data;
	state->frame_number = 0;
	state->flash_invert = 0;

	EventList_Initialise(machine, 30000);

	state->retrace_cycles = SPEC128_RETRACE_CYCLES;
}


/* return the color to be used inverting FLASHing colors if necessary */
INLINE unsigned char get_display_color (unsigned char color, int invert)
{
	if (invert && (color & 0x80))
		return (color & 0xc0) + ((color & 0x38) >> 3) + ((color & 0x07) << 3);
	else
		return color;
}

/* Code to change the FLASH status every 25 frames. Note this must be
   independent of frame skip etc. */
VIDEO_EOF( spectrum )
{
	spectrum_state *state = (spectrum_state *)machine->driver_data;
	EVENT_LIST_ITEM *pItem;
	int NumItems;

	state->frame_number++;
	if (state->frame_number >= 25)
	{
		state->frame_number = 0;
		state->flash_invert = !state->flash_invert;
	}

	/* Empty event buffer for undisplayed frames noting the last border
       colour (in case colours are not changed in the next frame). */
	NumItems = EventList_NumEvents();
	if (NumItems)
	{
		pItem = EventList_GetFirstItem();
		border_set_last_color ( pItem[NumItems-1].Event_Data );
		EventList_Reset();
		EventList_SetOffsetStartTime ( machine->firstcpu->attotime_to_cycles(attotime_mul(machine->primary_screen->scan_period(), machine->primary_screen->vpos())) );
		logerror ("Event log reset in callback fn.\n");
	}
}



/***************************************************************************
  Update the spectrum screen display.

  The screen consists of 312 scanlines as follows:
  64  border lines (the last 48 are actual border lines; the others may be
                    border lines or vertical retrace)
  192 screen lines
  56  border lines

  Each screen line has 48 left border pixels, 256 screen pixels and 48 right
  border pixels.

  Each scanline takes 224 T-states divided as follows:
  128 Screen (reads a screen and ATTR byte [8 pixels] every 4 T states)
  24  Right border
  48  Horizontal retrace
  24  Left border

  The 128K Spectrums have only 63 scanlines before the TV picture (311 total)
  and take 228 T-states per scanline.

***************************************************************************/

INLINE void spectrum_plot_pixel(bitmap_t *bitmap, int x, int y, UINT32 color)
{
	*BITMAP_ADDR16(bitmap, y, x) = (UINT16)color;
}

VIDEO_UPDATE( spectrum )
{
        /* for now do a full-refresh */
	spectrum_state *state = (spectrum_state *)screen->machine->driver_data;
	int x, y, b, scrx, scry;
	unsigned short ink, pap;
	unsigned char *attr, *scr;
		int full_refresh = 1;

	scr=state->screen_location;

	for (y=0; y<192; y++)
	{
		scrx=SPEC_LEFT_BORDER;
		scry=((y&7) * 8) + ((y&0x38)>>3) + (y&0xC0);
		attr=state->screen_location + ((scry>>3)*32) + 0x1800;

		for (x=0;x<32;x++)
		{
			/* Get ink and paper colour with bright */
			if (state->flash_invert && (*attr & 0x80))
			{
				ink=((*attr)>>3) & 0x0f;
				pap=((*attr) & 0x07) + (((*attr)>>3) & 0x08);
			}
			else
			{
				ink=((*attr) & 0x07) + (((*attr)>>3) & 0x08);
				pap=((*attr)>>3) & 0x0f;
			}

			for (b=0x80;b!=0;b>>=1)
			{
				if (*scr&b)
					spectrum_plot_pixel(bitmap,scrx++,SPEC_TOP_BORDER+scry,ink);
				else
					spectrum_plot_pixel(bitmap,scrx++,SPEC_TOP_BORDER+scry,pap);
			}

			scr++;
			attr++;
		}
	}

	border_draw(screen->machine, bitmap, full_refresh,
		SPEC_TOP_BORDER, SPEC_DISPLAY_YSIZE, SPEC_BOTTOM_BORDER,
		SPEC_LEFT_BORDER, SPEC_DISPLAY_XSIZE, SPEC_RIGHT_BORDER,
		SPEC_LEFT_BORDER_CYCLES, SPEC_DISPLAY_XSIZE_CYCLES,
		SPEC_RIGHT_BORDER_CYCLES, state->retrace_cycles, 200, 0xfe);
	return 0;
}


static const rgb_t spectrum_palette[16] = {
	MAKE_RGB(0x00, 0x00, 0x00),
	MAKE_RGB(0x00, 0x00, 0xbf),
	MAKE_RGB(0xbf, 0x00, 0x00),
	MAKE_RGB(0xbf, 0x00, 0xbf),
	MAKE_RGB(0x00, 0xbf, 0x00),
	MAKE_RGB(0x00, 0xbf, 0xbf),
	MAKE_RGB(0xbf, 0xbf, 0x00),
	MAKE_RGB(0xbf, 0xbf, 0xbf),
	MAKE_RGB(0x00, 0x00, 0x00),
	MAKE_RGB(0x00, 0x00, 0xff),
	MAKE_RGB(0xff, 0x00, 0x00),
	MAKE_RGB(0xff, 0x00, 0xff),
	MAKE_RGB(0x00, 0xff, 0x00),
	MAKE_RGB(0x00, 0xff, 0xff),
	MAKE_RGB(0xff, 0xff, 0x00),
	MAKE_RGB(0xff, 0xff, 0xff)
};
/* Initialise the palette */
PALETTE_INIT( spectrum )
{
	palette_set_colors(machine, 0, spectrum_palette, ARRAY_LENGTH(spectrum_palette));
}

/***************************************************************************
        Border engine:

        Functions for drawing multi-coloured screen borders using the
        Event List processing.

Changes:

28/05/2000 DJR - Initial implementation.
08/06/2000 DJR - Now only uses events with the correct ID value.
28/06/2000 DJR - draw_border now uses full_refresh flag.

***************************************************************************/

/* Last border colour output in the previous frame */
static int CurrBorderColor = 0;

static int LastDisplayedBorderColor = -1; /* Negative value indicates redraw */

/* Force the border to be redrawn on the next frame */
void border_force_redraw (void)
{
        LastDisplayedBorderColor = -1;
}

/* Set the last border colour to have been displayed. Used when loading snap
   shots and to record the last colour change in a frame that was skipped. */
void border_set_last_color (int NewColor)
{
        CurrBorderColor = NewColor;
}

void border_draw(running_machine *machine, bitmap_t *bitmap,
	int full_refresh,               /* Full refresh flag */
	int TopBorderLines,             /* Border lines before actual screen */
	int ScreenLines,                /* Screen height in pixels */
	int BottomBorderLines,          /* Border lines below screen */
	int LeftBorderPixels,           /* Border pixels to the left of each screen line */
	int ScreenPixels,               /* Width of actual screen in pixels */
	int RightBorderPixels,          /* Border pixels to the right of each screen line */
	int LeftBorderCycles,           /* Cycles taken to draw left border of each scan line */
	int ScreenCycles,               /* Cycles taken to draw screen data part of each scan line */
	int RightBorderCycles,          /* Cycles taken to draw right border of each scan line */
	int HorizontalRetraceCycles,    /* Cycles taken to return to LHS of CRT after each scan line */
	int VRetraceTime,               /* Cycles taken before start of first border line */
	int EventID)                    /* Event ID of border messages */
{
	EVENT_LIST_ITEM *pItem;
	int TotalScreenHeight = TopBorderLines+ScreenLines+BottomBorderLines;
	int TotalScreenWidth = LeftBorderPixels+ScreenPixels+RightBorderPixels;
	int DisplayCyclesPerLine = LeftBorderCycles+ScreenCycles+RightBorderCycles;
	int CyclesPerLine = DisplayCyclesPerLine+HorizontalRetraceCycles;
	int CyclesSoFar = 0;
	int NumItems, CurrItem = 0, NextItem;
	int Count, ScrX, NextScrX, ScrY;
	rectangle r;

	pItem = EventList_GetFirstItem();
	NumItems = EventList_NumEvents();

	for (Count = 0; Count < NumItems; Count++)
	{
//      logerror ("Event no %05d, ID = %04x, data = %04x, time = %ld\n", Count, pItem[Count].Event_ID, pItem[Count].Event_Data, (long) pItem[Count].Event_Time);
	}

	/* Find the first and second events with the correct ID */
	while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID))
			CurrItem++;
	NextItem = CurrItem + 1;
	while ((NextItem < NumItems) && (pItem[NextItem].Event_ID != EventID))
			NextItem++;

	/* Single border colour */
	if ((CurrItem < NumItems) && (NextItem >= NumItems))
			CurrBorderColor = pItem[CurrItem].Event_Data;

	if ((NextItem >= NumItems) && (CurrBorderColor==LastDisplayedBorderColor) && !full_refresh)
	{
			/* Do nothing if border colour has not changed */
	}
	else if (NextItem >= NumItems)
	{
			/* Single border colour - this is not strictly correct as the
                colour change may have occurred midway through the frame
                or after the last visible border line however the whole
                border would be redrawn in the correct colour during the
                next frame anyway! */
			r.min_x = 0;
			r.max_x = TotalScreenWidth-1;
			r.min_y = 0;
			r.max_y = TopBorderLines-1;
			bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);

			r.min_x = 0;
			r.max_x = LeftBorderPixels-1;
			r.min_y = TopBorderLines;
			r.max_y = TopBorderLines+ScreenLines-1;
			bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);

			r.min_x = LeftBorderPixels+ScreenPixels;
			r.max_x = TotalScreenWidth-1;
			bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);

			r.min_x = 0;
			r.max_x = TotalScreenWidth-1;
			r.min_y = TopBorderLines+ScreenLines;
			r.max_y = TotalScreenHeight-1;
			bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);

//          logerror ("Setting border colour to %d (Last = %d, Full Refresh = %d)\n", CurrBorderColor, LastDisplayedBorderColor, full_refresh);
			LastDisplayedBorderColor = CurrBorderColor;
	}
	else
	{
			/* Multiple border colours */

			/* Process entries before first displayed line */
			while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= VRetraceTime))
			{
					CurrBorderColor = pItem[CurrItem].Event_Data;
					do {
							CurrItem++;
					} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
			}

			/* Draw top border */
			CyclesSoFar = VRetraceTime;
			for (ScrY = 0; ScrY < TopBorderLines; ScrY++)
			{
					r.min_x = 0;
					r.min_y = r.max_y = ScrY;
					if ((CurrItem >= NumItems) || (pItem[CurrItem].Event_Time >= (CyclesSoFar+DisplayCyclesPerLine)))
					{
							/* Single colour on line */
							r.max_x = TotalScreenWidth-1;
							bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
					}
					else
					{
							/* Multiple colours on a line */
							ScrX = (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)TotalScreenWidth / (float)DisplayCyclesPerLine;
							r.max_x = ScrX-1;
							bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
							CurrBorderColor = pItem[CurrItem].Event_Data;
							do {
									CurrItem++;
							} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));

							while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time < (CyclesSoFar+DisplayCyclesPerLine)))
							{
									NextScrX = (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)TotalScreenWidth / (float)DisplayCyclesPerLine;
									r.min_x = ScrX;
									r.max_x = NextScrX-1;
									bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
									ScrX = NextScrX;
									CurrBorderColor = pItem[CurrItem].Event_Data;
									do {
											CurrItem++;
									} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
							}
							r.min_x = ScrX;
							r.max_x = TotalScreenWidth-1;
							bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
					}

					/* Process colour changes during horizontal retrace */
					CyclesSoFar+= CyclesPerLine;
					while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= CyclesSoFar))
					{
							CurrBorderColor = pItem[CurrItem].Event_Data;
							do {
									CurrItem++;
							} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
					}
			}

			/* Draw left and right borders next to screen lines */
			for (ScrY = TopBorderLines; ScrY < (TopBorderLines+ScreenLines); ScrY++)
			{
					/* Draw left hand border */
					r.min_x = 0;
					r.min_y = r.max_y = ScrY;

					if ((CurrItem >= NumItems) || (pItem[CurrItem].Event_Time >= (CyclesSoFar+LeftBorderCycles)))
					{
							/* Single colour */
							r.max_x = LeftBorderPixels-1;
							bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
					}
					else
					{
							/* Multiple colours */
							ScrX = (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)LeftBorderPixels / (float)LeftBorderCycles;
							r.max_x = ScrX-1;
							bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
							CurrBorderColor = pItem[CurrItem].Event_Data;
							do {
									CurrItem++;
							} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));

							while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time < (CyclesSoFar+LeftBorderCycles)))
							{
									NextScrX = (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)LeftBorderPixels / (float)LeftBorderCycles;
									r.min_x = ScrX;
									r.max_x = NextScrX-1;
									bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
									ScrX = NextScrX;
									CurrBorderColor = pItem[CurrItem].Event_Data;
									do {
											CurrItem++;
									} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
							}
							r.min_x = ScrX;
							r.max_x = LeftBorderPixels-1;
							bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
					}

					/* Process colour changes during screen draw */
					while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= (CyclesSoFar+LeftBorderCycles+ScreenCycles)))
					{
							CurrBorderColor = pItem[CurrItem].Event_Data;
							do {
									CurrItem++;
							} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
					}

					/* Draw right hand border */
					r.min_x = LeftBorderPixels+ScreenPixels;
					if ((CurrItem >= NumItems) || (pItem[CurrItem].Event_Time >= (CyclesSoFar+DisplayCyclesPerLine)))
					{
							/* Single colour */
							r.max_x = TotalScreenWidth-1;
							bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
					}
					else
					{
							/* Multiple colours */
							ScrX = LeftBorderPixels + ScreenPixels + (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)RightBorderPixels / (float)RightBorderCycles;
							r.max_x = ScrX-1;
							bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
							CurrBorderColor = pItem[CurrItem].Event_Data;
							do {
									CurrItem++;
							} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));

							while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time < (CyclesSoFar+DisplayCyclesPerLine)))
							{
									NextScrX = LeftBorderPixels + ScreenPixels + (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)RightBorderPixels / (float)RightBorderCycles;
									r.min_x = ScrX;
									r.max_x = NextScrX-1;
									bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
									ScrX = NextScrX;
									CurrBorderColor = pItem[CurrItem].Event_Data;
									do {
											CurrItem++;
									} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
							}
							r.min_x = ScrX;
							r.max_x = TotalScreenWidth-1;
							bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
					}

					/* Process colour changes during horizontal retrace */
					CyclesSoFar+= CyclesPerLine;
					while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= CyclesSoFar))
					{
							CurrBorderColor = pItem[CurrItem].Event_Data;
							do {
									CurrItem++;
							} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
					}
			}

			/* Draw bottom border */
			for (ScrY = TopBorderLines+ScreenLines; ScrY < TotalScreenHeight; ScrY++)
			{
					r.min_x = 0;
					r.min_y = r.max_y = ScrY;
					if ((CurrItem >= NumItems) || (pItem[CurrItem].Event_Time >= (CyclesSoFar+DisplayCyclesPerLine)))
					{
							/* Single colour on line */
							r.max_x = TotalScreenWidth-1;
							bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
					}
					else
					{
							/* Multiple colours on a line */
							ScrX = (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)TotalScreenWidth / (float)DisplayCyclesPerLine;
							r.max_x = ScrX-1;
							bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
							CurrBorderColor = pItem[CurrItem].Event_Data;
							do {
									CurrItem++;
							} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));

							while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time < (CyclesSoFar+DisplayCyclesPerLine)))
							{
									NextScrX = (int)(pItem[CurrItem].Event_Time - CyclesSoFar) * (float)TotalScreenWidth / (float)DisplayCyclesPerLine;
									r.min_x = ScrX;
									r.max_x = NextScrX-1;
									bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
									ScrX = NextScrX;
									CurrBorderColor = pItem[CurrItem].Event_Data;
									do {
											CurrItem++;
									} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
							}
							r.min_x = ScrX;
							r.max_x = TotalScreenWidth-1;
							bitmap_fill(bitmap, &r, machine->pens[CurrBorderColor]);
					}

					/* Process colour changes during horizontal retrace */
					CyclesSoFar+= CyclesPerLine;
					while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= CyclesSoFar))
					{
							CurrBorderColor = pItem[CurrItem].Event_Data;
							do {
									CurrItem++;
							} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
					}
			}

			/* Process colour changes after last displayed line */
			while (CurrItem < NumItems)
			{
					if (pItem[CurrItem].Event_ID == EventID)
							CurrBorderColor = pItem[CurrItem].Event_Data;
					CurrItem++;
			}

			/* Set value to ensure redraw on next frame */
			LastDisplayedBorderColor = -1;

//          logerror ("Multi coloured border drawn (last colour = %d)\n", CurrBorderColor);
	}

	/* Assume all other routines have processed their data from the list */
	EventList_Reset();
	EventList_SetOffsetStartTime ( machine->firstcpu->attotime_to_cycles(attotime_mul(machine->primary_screen->scan_period(), machine->primary_screen->vpos())));
}

/* current item */
static EVENT_LIST_ITEM *pCurrentItem;
/* number of items in buffer */
static int NumEvents = 0;

/* size of the buffer - used to prevent buffer overruns */
static int TotalEvents = 0;

/* the buffer */
static char *pEventListBuffer = NULL;

/* Cycle count at last frame draw - used for timing offset calculations */
static int LastFrameStartTime = 0;

static int CyclesPerFrame=0;

/* initialise */

/* if the CPU is the controlling factor, the size of the buffer
can be setup as:

Number_of_CPU_Cycles_In_A_Frame/Minimum_Number_Of_Cycles_Per_Instruction */
void EventList_Initialise(running_machine *machine, int NumEntries)
{
	pEventListBuffer = auto_alloc_array(machine, char, NumEntries);
	TotalEvents = NumEntries;
	CyclesPerFrame = 0;
	EventList_Reset();
}

/* reset the change list */
void    EventList_Reset(void)
{
	NumEvents = 0;
	pCurrentItem = (EVENT_LIST_ITEM *)pEventListBuffer;
}


/* add an event to the buffer */
void	EventList_AddItem(int ID, int Data, int Time)
{
        if (NumEvents < TotalEvents)
        {
                /* setup item only if there is space in the buffer */
                pCurrentItem->Event_ID = ID;
                pCurrentItem->Event_Data = Data;
                pCurrentItem->Event_Time = Time;

                pCurrentItem++;
                NumEvents++;
        }
}

/* set the start time for use with EventList_AddItemOffset usually this will
   be cpu_getcurrentcycles() at the time that the screen is being refreshed */
void    EventList_SetOffsetStartTime(int StartTime)
{
        LastFrameStartTime = StartTime;
}

/* add an event to the buffer with a time index offset from a specified time */
void    EventList_AddItemOffset(running_machine *machine, int ID, int Data, int Time)
{

        if (!CyclesPerFrame)
                CyclesPerFrame = (int)(cpu_get_clock(machine->firstcpu) / machine->primary_screen->frame_period().attoseconds);	//totalcycles();    //_(int)(cpunum_get_clock(0) / machine->config->frames_per_second);

        if (NumEvents < TotalEvents)
        {
                /* setup item only if there is space in the buffer */
                pCurrentItem->Event_ID = ID;
                pCurrentItem->Event_Data = Data;

                Time -= LastFrameStartTime;
                if ((Time < 0) || ((Time == 0) && NumEvents))
                        Time+= CyclesPerFrame;
                pCurrentItem->Event_Time = Time;

                pCurrentItem++;
                NumEvents++;
        }
}

/* get number of events */
int     EventList_NumEvents(void)
{
	return NumEvents;
}

/* get first item in buffer */
EVENT_LIST_ITEM *EventList_GetFirstItem(void)
{
	return (EVENT_LIST_ITEM *)pEventListBuffer;
}
