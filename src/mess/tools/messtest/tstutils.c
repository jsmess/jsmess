/*********************************************************************

    tstutils.c

    Utility code for testing

*********************************************************************/

#include <ctype.h>
#include "core.h"

const char *find_attribute(const char **attributes, const char *seek_attribute)
{
	int i;
	for (i = 0; attributes[i] && strcmp(attributes[i], seek_attribute); i += 2)
		;
	return attributes[i] ? attributes[i+1] : NULL;
}



attotime parse_time(const char *s)
{
	double d = atof(s);
	return attotime::from_double(d);
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



