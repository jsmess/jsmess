// Any fixes for this driver should be forwarded to AGEMAME HQ (http://www.mameworld.net/agemame/)

#ifndef INC_LAMPS
#define INC_LAMPS

#define MAX_LAMPS 512


void Lamps_init(int number);

int  Lamps_GetNumberLamps(void);

void Lamps_SetBrightness(int start_id, int count, UINT8 *bright );

int  Lamps_GetBrightness(int id);

#endif
