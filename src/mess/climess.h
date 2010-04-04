/***************************************************************************

    climess.h

    Command-line interface frontend for MESS.

***************************************************************************/

#pragma once

#ifndef __CLIMESS_H__
#define __CLIMESS_H__


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void mess_display_help(void);
int info_listmedia(core_options *opts, const char *gamename);
void mess_match_roms(core_options *options, const char *hash, int length, int *found);
int info_listsoftware(core_options *opts, const char *gamename);


#endif	/* __CLIMESS_H__ */
