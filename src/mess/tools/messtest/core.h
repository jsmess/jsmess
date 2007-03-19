/*********************************************************************

	core.h

	Automated testing for MAME/MESS

*********************************************************************/

#ifndef CORE_H
#define CORE_H

#include "mame.h"
#include "timer.h"
#include "xmlfile.h"
#include "pile.h"


/***************************************************************************

	Type definitions

***************************************************************************/

typedef enum
{
	DATA_NONE,
	DATA_TEXT,
	DATA_BINARY
} tagdatatype_t;

typedef enum
{
	MSG_INFO,
	MSG_FAILURE,
	MSG_PREFAILURE
} messtest_messagetype_t;

struct messtest_tagdispatch
{
	const char *tag;
	tagdatatype_t datatype;
	void (*start_handler)(const char **attributes);
	void (*end_handler)(const void *ptr, size_t len);
	const struct messtest_tagdispatch *subdispatch;
};

struct messtest_options
{
	const char *script_filename;
	unsigned int preserve_directory : 1;
	unsigned int dump_screenshots : 1;
};



/***************************************************************************

	Prototypes

***************************************************************************/

/* executing the tests */
int messtest(const struct messtest_options *opts, int *test_count, int *failure_count);

/* utility functions to aid in parsing */
int memory_region_from_string(const char *region_name);
const char *memory_region_to_string(int region);
const char *find_attribute(const char **attributes, const char *seek_attribute);
mame_time parse_time(const char *s);
offs_t parse_offset(const char *s);

/* reporting */
void error_report(const char *message);
void error_reportf(const char *fmt, ...);
void error_missingattribute(const char *attribute);
void error_outofmemory(void);
void error_invalidmemregion(const char *s);
void error_baddevicetype(const char *s);

void report_message(messtest_messagetype_t msgtype, const char *fmt, ...);
void report_testcase_begin(const char *testcase_name);
void report_testcase_ran(int failure);

void messtest_get_data(xml_data_node *node, mess_pile *pile);

#endif /* CORE_H */
