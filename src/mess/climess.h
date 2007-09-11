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
int info_listdevices(core_options *opts, const char *gamename);


#endif	/* __CLIMESS_H__ */
