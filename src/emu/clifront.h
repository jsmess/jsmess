/***************************************************************************

    clifront.h

    Command-line interface frontend for MAME.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __CLIFRONT_H__
#define __CLIFRONT_H__


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define CLIOPTION_CREATECONFIG			"createconfig"
#define CLIOPTION_SHOWCONFIG			"showconfig"
#define CLIOPTION_SHOWUSAGE				"showusage"
#define CLIOPTION_VALIDATE				"validate"
#define CLIOPTION_HELP					"help"
#define CLIOPTION_LISTXML				"listxml"
#define CLIOPTION_LISTFULL				"listfull"
#define CLIOPTION_LISTSOURCE			"listsource"
#define CLIOPTION_LISTCLONES			"listclones"
#define CLIOPTION_LISTCRC				"listcrc"
#define CLIOPTION_LISTDEVICES			"listdevices"
#define CLIOPTION_LISTROMS				"listroms"
#define CLIOPTION_LISTSAMPLES			"listsamples"
#define CLIOPTION_VERIFYROMS			"verifyroms"
#define CLIOPTION_VERIFYSAMPLES			"verifysamples"
#define CLIOPTION_ROMIDENT				"romident"



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

int cli_execute(int argc, char **argv, const options_entry *osd_options);

/* informational functions */
int cli_info_listxml(const char *gamename);
int cli_info_listfull(const char *gamename);
int cli_info_listsource(const char *gamename);
int cli_info_listclones(const char *gamename);
int cli_info_listcrc(const char *gamename);
int cli_info_listroms(const char *gamename);
int cli_info_listsamples(const char *gamename);

#endif	/* __CLIFRONT_H__ */
