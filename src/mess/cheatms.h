/*********************************************************************

	cheatms.h

	MESS specific cheat code

*********************************************************************/

#ifndef CHEATMS_H
#define CHEATMS_H

extern UINT32				thisGameCRC;

void InitMessCheats(void);
void StopMessCheats(void);
int MatchesCRCTable(UINT32 crc);

#endif /* CHEATMS_H */
