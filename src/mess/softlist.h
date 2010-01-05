/*********************************************************************

    softlist.h

    Software entry and software list information.

*********************************************************************/

#ifndef __SOFTLIST_H_
#define __SOFTLIST_H_

#include "romload.h"

/* ----- software typedef ----- */

typedef struct _software_entry software_entry;
struct _software_entry
{
	const char *	name;				/* short name of the software */
	const char *	parent;				/* parent */
	const char *	fullname;			/* full name of the software */
	const char *	release_date;		/* release date */
	const char *	manufacturer;		/* manufacturer */
	const char *	region;				/* region (World, USA, Japan, etc) */
	UINT64			userflags;			/* freely usable flags, can be used to store things like board type or pcb features */
	UINT32			flags;				/* general flags known to the framework */
	const rom_entry	*rom_info;			/* rom information for the software */
};


typedef struct _software_list software_list;
struct _software_list
{
	const char * name;
	const char * description;
	const software_entry *	entries;
};


/* ----- software macros ----- */

#define SOFTWARE_LIST_NAME(name)					software_list_##name
#define SOFTWARE_NAME(name)							software_##name
#define SOFTWARE_LIST(name,desc)					const software_list SOFTWARE_LIST_NAME(name) = { #name, desc, software_##name };
#define SOFTWARE_LIST_START(name)	\
	static const software_entry SOFTWARE_NAME(name)[] = {

#define SOFTWARE_LIST_END							{ NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL } };

#define SOFTWARE_ROM_NAME(name)						software_rom_##name
#define SOFTWARE_START(name)						static const rom_entry SOFTWARE_ROM_NAME(name)[] = {
#define SOFTWARE_END								ROM_END


#define SOFTWARE(name,parent,year,manufacturer,fullname,region,userflags,flags) \
    { #name, #parent, fullname, #year, manufacturer, region, userflags, flags, SOFTWARE_ROM_NAME(name) },


/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

extern const software_list * const software_lists[];


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

const software_list* software_list_get_by_name(const char *name);
const software_entry* software_get_by_name(const software_list* list, const char *name);

#endif
