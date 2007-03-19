// Any fixes for this driver should be forwarded to AGEMAME (http://www.mameworld.net/agemame/)

#ifndef INC_MMTR
#define INC_MMTR

#define MAXMECHMETERS 8

#define METERREACTTIME 30000L	  // number of cycles meter has to be active to tick


void Mechmtr_init(     int number);

int  MechMtr_GetNumberMeters(void);

void MechMtr_Setcount( int id, long count);

long MechMtr_Getcount( int id);

void MechMtr_ReactTime(int id, long cycles);

int  Mechmtr_update(   int id, long cycles, int state); // returns 1 if meter ticked

#endif
