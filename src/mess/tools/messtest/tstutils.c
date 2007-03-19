/*********************************************************************

	tstutils.c

	Utility code for testing

*********************************************************************/

#include <ctype.h>
#include "core.h"

static const struct
{
	const char *name;
	int region;
} region_map[] =
{
	{ "RAM",	0 },
	{ "CPU1",	REGION_CPU1 },
	{ "CPU2",	REGION_CPU2 },
	{ "CPU3",	REGION_CPU3 },
	{ "CPU4",	REGION_CPU4 },
	{ "CPU5",	REGION_CPU5 },
	{ "CPU6",	REGION_CPU6 },
	{ "CPU7",	REGION_CPU7 },
	{ "CPU8",	REGION_CPU8 },
	{ "GFX1",	REGION_GFX1 },
	{ "GFX2",	REGION_GFX2 },
	{ "GFX3",	REGION_GFX3 },
	{ "GFX4",	REGION_GFX4 },
	{ "GFX5",	REGION_GFX5 },
	{ "GFX6",	REGION_GFX6 },
	{ "GFX7",	REGION_GFX7 },
	{ "GFX8",	REGION_GFX8 },
	{ "PROMS",	REGION_PROMS },
	{ "SOUND1",	REGION_SOUND1 },
	{ "SOUND2",	REGION_SOUND2 },
	{ "SOUND3",	REGION_SOUND3 },
	{ "SOUND4",	REGION_SOUND4 },
	{ "SOUND5",	REGION_SOUND5 },
	{ "SOUND6",	REGION_SOUND6 },
	{ "SOUND7",	REGION_SOUND7 },
	{ "SOUND8",	REGION_SOUND8 },
	{ "USER1",	REGION_USER1 },
	{ "USER2",	REGION_USER2 },
	{ "USER3",	REGION_USER3 },
	{ "USER4",	REGION_USER4 },
	{ "USER5",	REGION_USER5 },
	{ "USER6",	REGION_USER6 },
	{ "USER7",	REGION_USER7 },
	{ "USER8",	REGION_USER8 },
	{ "DISKS",	REGION_DISKS },
	{ NULL,		REGION_INVALID }
};

/* ----------------------------------------------------------------------- */

int memory_region_from_string(const char *region_name)
{
	int i;
	for (i = 0; region_map[i].name && mame_stricmp(region_map[i].name, region_name); i++)
		;
	return region_map[i].region;
}



const char *memory_region_to_string(int region)
{
	int i;
	for (i = 0; region_map[i].name && (region_map[i].region != region); i++)
		;
	return region_map[i].name;
}



const char *find_attribute(const char **attributes, const char *seek_attribute)
{
	int i;
	for (i = 0; attributes[i] && strcmp(attributes[i], seek_attribute); i += 2)
		;
	return attributes[i] ? attributes[i+1] : NULL;
}



mame_time parse_time(const char *s)
{
	double d = atof(s);
	return double_to_mame_time(d);
}



offs_t parse_offset(const char *s)
{
	offs_t result = 0;
	if ((s[0] == '0') && (tolower(s[1]) == 'x'))
		sscanf(&s[2], "%x", &result);
	else
		result = atoi(s);
	return result;
}



