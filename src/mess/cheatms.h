/*********************************************************************

    cheatms.h

    MESS specific cheat code

*********************************************************************/

#pragma once

#ifndef __CHEATMS_H__
#define __CHEATMS_H__

extern UINT32 this_game_crc;

void cheat_mess_init(running_machine *machine);
void cheat_mess_exit(void);
int cheat_mess_matches_crc_table(UINT32 crc);

#endif /* __CHEATMS_H__ */
