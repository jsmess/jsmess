#include "driver.h"
#include "eventlst.h"

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
void EventList_Initialise(int NumEntries)
{
	pEventListBuffer = auto_malloc(sizeof(EVENT_LIST_ITEM)*NumEntries);
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
void    EventList_AddItemOffset(int ID, int Data, int Time)
{

        if (!CyclesPerFrame)
                CyclesPerFrame = (int)(Machine->drv->cpu[0].cpu_clock / Machine->screen[0].refresh);	//totalcycles();	//_(int)(Machine->drv->cpu[0].cpu_clock / Machine->drv->frames_per_second);

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
