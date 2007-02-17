typedef struct EVENT_LIST_ITEM
{
	/* driver defined ID for this write */
	int	Event_ID;
	/* driver defined data for this write */
	int	Event_Data;
	/* time at which this write occured */
	int Event_Time;
} EVENT_LIST_ITEM;

void	EventList_Initialise(int NumEntries);
void    EventList_Reset(void);
void    EventList_AddItem(int ID, int Data,int Time);
void    EventList_SetOffsetStartTime(int StartTime);
void    EventList_AddItemOffset(int ID, int Data,int Time);
int     EventList_NumEvents(void);
EVENT_LIST_ITEM *EventList_GetFirstItem(void);
