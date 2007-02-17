/*********************************************************************

	core.c

	MESS testing code

*********************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <expat.h>

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#include "strconv.h"
#endif /* WIN32 */

#include "core.h"
#include "testmess.h"
#include "testimgt.h"
#include "osdepend.h"
#include "pool.h"
#include "pile.h"
#include "inputx.h"

#define EOLN (CRLF == 1 ? "\r" : (CRLF == 2 ? "\n" : (CRLF == 3 ? "\r\n" : NULL)))

/* ----------------------------------------------------------------------- */

typedef enum
{
	BLOBSTATE_INITIAL,
	BLOBSTATE_AFTER_0,
	BLOBSTATE_AFTER_STAR,
	BLOBSTATE_HEX,
	BLOBSTATE_SINGLEQUOTES,
	BLOBSTATE_DOUBLEQUOTES
} blobparse_state_t;

static const char *current_testcase_name;
static int is_failure;



void error_report(const char *message)
{
	fprintf(stderr, "%s: %s\n",
		current_testcase_name,
		message);
	is_failure = 1;
}



void error_reportf(const char *fmt, ...)
{
	char buf[1024];
	va_list va;

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, va);
	va_end(va);

	error_report(buf);
}



void error_missingattribute(const char *attribute)
{
	error_reportf("Missing attribute '%s'\n", attribute);
}



void error_outofmemory(void)
{
	error_reportf("Out of memory\n");
}



void error_invalidmemregion(const char *s)
{
	error_reportf("Invalid memory region '%s'\n", s);
}



void error_baddevicetype(const char *s)
{
	error_reportf("Bad device type '%s'\n", s);
}



void messtest_get_data(xml_data_node *node, mess_pile *pile)
{
	void *ptr;
	int i = 0;
	size_t j, size;
	int found;
	char c;
	char quote_char;
	blobparse_state_t blobstate = BLOBSTATE_INITIAL;
	const char *s;
	size_t len;
	int multiple = 0;

	s = node->value ? node->value : "";
	len = strlen(s);

	while(i < len)
	{
		/* read next character */
		c = s[i++];

		switch(blobstate)
		{
			case BLOBSTATE_INITIAL:
				if ((c == '\0') || isspace(c))
				{
					/* ignore EOF and whitespace */
				}
				else if (c == '0')
				{
					blobstate = BLOBSTATE_AFTER_0;
				}
				else if (c == '*')
				{
					blobstate = BLOBSTATE_AFTER_STAR;
					multiple = (size_t) -1;
				}
				else if (c == '\'')
				{
					blobstate = BLOBSTATE_SINGLEQUOTES;
				}
				else if (c == '\"')
				{
					blobstate = BLOBSTATE_DOUBLEQUOTES;
				}
				else
					goto parseerror;
				break;

			case BLOBSTATE_AFTER_0:
				if (tolower(c) == 'x')
				{
					blobstate = BLOBSTATE_HEX;
				}
				else
					goto parseerror;
				break;

			case BLOBSTATE_AFTER_STAR:
				if (isdigit(c))
				{
					/* add this digit to the multiple */
					if (multiple == (size_t) -1)
						multiple = 0;
					else
						multiple *= 10;
					multiple += c - '0';
				}
				else if ((c != '\0') && isspace(c) && (multiple == (size_t) -1))
				{
					/* ignore whitespace */
				}
				else
				{
					/* do the multiplication */
					size = pile_size(pile);
					ptr = pile_detach(pile);

					for (j = 0; j < multiple; j++)
						pile_write(pile, ptr, size);

					free(ptr);
					blobstate = BLOBSTATE_INITIAL;
				}
				break;

			case BLOBSTATE_HEX:
				if ((c == '\0') || isspace(c))
				{
					blobstate = BLOBSTATE_INITIAL;
				}
				else
				{
					found = FALSE;
					i--;
					while(((i + 2) <= len) && isxdigit(s[i]) && isxdigit(s[i+1]))
					{
						c = (hexdigit(s[i]) << 4) | hexdigit(s[i+1]);
						pile_putc(pile, c);
						i += 2;
						found = TRUE;
					}
					if (!found)
						goto parseerror;
				}
				break;

			case BLOBSTATE_SINGLEQUOTES:
			case BLOBSTATE_DOUBLEQUOTES:
				quote_char = blobstate == BLOBSTATE_SINGLEQUOTES ? '\'' : '\"';
				if (c == quote_char)
				{
					blobstate = BLOBSTATE_INITIAL;
				}
				else if (c != '\0')
				{
					pile_putc(pile, c);
				}
				else
					goto parseerror;
				break;
		}
	}
	return;

parseerror:
	error_report("Parse Error");
	return;
}



/* this external entity handler allows us to do things like this:
 *
 *	<!DOCTYPE tests
 *	[
 *		<!ENTITY mamekey_esc SYSTEM "http://www.mess.org/messtest/">
 *	]>
 */
static int external_entity_handler(XML_Parser parser,
	const XML_Char *context,
	const XML_Char *base,
	const XML_Char *systemId,
	const XML_Char *publicId)
{
	XML_Parser extparser = NULL;
	int rc = 0, i;
	char buf[256];
	static const char *mamekey_prefix = "mamekey_";
	input_code c;

	buf[0] = '\0';

	/* only supportr our own schema */
	if (strcmp(systemId, "http://www.mess.org/messtest/"))
		goto done;

	extparser = XML_ExternalEntityParserCreate(parser, context, "us-ascii");
	if (!extparser)
		goto done;

	/* does this use the 'mamekey' prefix? */
	if ((strlen(context) > strlen(mamekey_prefix)) && !memcmp(context,
		mamekey_prefix, strlen(mamekey_prefix)))
	{
		context += strlen(mamekey_prefix);
		c = 0;

		/* this is interim until we can come up with a real solution */
		snprintf(buf, sizeof(buf) / sizeof(buf[0]), "KEYCODE_%s", context);
		for (i = 0; buf[i]; i++)
			buf[i] = toupper(buf[i]);

		code_init(NULL);
		c = token_to_code(buf);

		if (c != CODE_NONE)
		{
			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "<%s%s>&#%d;</%s%s>",
				mamekey_prefix, context,
				UCHAR_MAMEKEY_BEGIN + c,
				mamekey_prefix, context);

			if (XML_Parse(extparser, buf, strlen(buf), 0) == XML_STATUS_ERROR)
				goto done;
		}
	}
	else if (!strcmp(context, "eoln"))
	{
		snprintf(buf, sizeof(buf) / sizeof(buf[0]), "<eoln>%s</eoln>", EOLN);

		if (XML_Parse(extparser, buf, strlen(buf), 0) == XML_STATUS_ERROR)
			goto done;
	}

	if (XML_Parse(extparser, NULL, 0, 1) == XML_STATUS_ERROR)
		goto done;

	rc = 1;
done:
	if (extparser)
		XML_ParserFree(extparser);
	return rc;
}



static void parse_init(XML_Parser parser)
{
	XML_SetExternalEntityRefHandler(parser, external_entity_handler);
}



static size_t parse_read(void *param, void *buffer, size_t length)
{
	return fread(buffer, 1, length, (FILE *) param);
}



static int parse_eof(void *param)
{
	return feof((FILE *) param);
}



static void node_tests(xml_data_node *tests_node, int *test_count, int *failure_count)
{
	xml_data_node *child_node;

	for (child_node = tests_node->child; child_node; child_node = child_node->next)
	{
		if (!strcmp(child_node->name, "tests"))
		{
			node_tests(child_node, test_count, failure_count);
		}
		else if (!strcmp(child_node->name, "test"))
		{
			/* a MESS test */
			node_testmess(child_node);

			(*test_count)++;
			if (is_failure)
				(*failure_count)++;
		}
		else if (!strcmp(child_node->name, "imgtooltest"))
		{
			/* an Imgtool test */
			node_testimgtool(child_node);

			(*test_count)++;
			if (is_failure)
				(*failure_count)++;
		}
	}
}



int messtest(const struct messtest_options *opts, int *test_count, int *failure_count)
{
	char saved_directory[1024];
	FILE *file;
	int result = -1;
	char *script_directory;
	xml_parse_options parse_options;
	xml_parse_error parse_error;
	xml_data_node *root_node;
	xml_data_node *tests_node;
	mess_pile pile;
	const char *xml;
	size_t sz;
	char buf[256];

	*test_count = 0;
	*failure_count = 0;

	pile_init(&pile);

	/* open the script file */
	file = fopen(opts->script_filename, "r");
	if (!file)
	{
		fprintf(stderr, "%s: Cannot open file\n", opts->script_filename);
		goto done;
	}

	/* read the file */
	while(!feof(file))
	{
		sz = fread(buf, 1, sizeof(buf), file);
		pile_write(&pile, buf, sz);
	}
	pile_writebyte(&pile, '\0', 1);
	xml = (const char *) pile_getptr(&pile);

	/* save the current working directory, and change to the test directory */
	saved_directory[0] = '\0';
	if (!opts->preserve_directory)
	{
		script_directory = osd_dirname(opts->script_filename);
		if (script_directory)
		{
			osd_getcurdir(saved_directory, sizeof(saved_directory) / sizeof(saved_directory[0]));
			osd_setcurdir(script_directory);
			free(script_directory);
		}
	}

	/* set up parse options */
	memset(&parse_options, 0, sizeof(parse_options));
	parse_options.init_parser = parse_init;
	parse_options.flags = XML_PARSE_FLAG_WHITESPACE_SIGNIFICANT;
	parse_options.error = &parse_error;

	/* do the parse */
	root_node = xml_string_read(xml, &parse_options);
	if (!root_node)
	{
		fprintf(stderr, "%s:%d:%d: %s\n",
			opts->script_filename,
			parse_error.error_line,
			parse_error.error_column,
			parse_error.error_message);
		goto done;
	}

	/* find the tests node */
	tests_node = xml_get_sibling(root_node->child, "tests");
	if (!tests_node)
		goto done;

	node_tests(tests_node, test_count, failure_count);
	result = 0;

done:
	/* restore the directory */
	if (saved_directory[0])
		osd_setcurdir(saved_directory);
	pile_delete(&pile);
	return result;
}



void report_message(messtest_messagetype_t msgtype, const char *fmt, ...)
{
	char buf[1024];
	va_list va;
	const char *prefix1;
	const char *prefix2;
	int column_filename = 15;
	int column_tag = 3;
	int column_message = 80 - column_filename - column_tag;
	int last_space, base, i;

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, va);
	va_end(va);

	prefix1 = current_testcase_name;
	prefix2 = msgtype ? "***" : "...";

	base = 0;
	last_space = -1;
	i = 0;
	do
	{
		if ((buf[i] == '\0') || (i - base > column_message))
		{
			if (buf[i] && (last_space > 0))
				buf[last_space] = '\0';
			else
				last_space = i;
			printf("%-*s %-*s %s\n",
				column_filename, prefix1,
				column_tag, prefix2,
				&buf[base]);

			base = last_space + 1;
			last_space = -1;
			prefix1 = "";
			prefix2 = "";
		}
		else if (isspace(buf[i]))
		{
			last_space = i;
		}
	}
	while(buf[i++]);

	/* did we abort? */
/*	if ((msgtype == MSG_FAILURE) && (state != STATE_ABORTED))
	{
		state = STATE_ABORTED;
		final_time = timer_get_time();
		if (final_time > 0.0)
			dump_screenshot(); 
	}
*/
}



void report_testcase_begin(const char *testcase_name)
{
	current_testcase_name = testcase_name;
	is_failure = FALSE;
}



void report_testcase_ran(int failure)
{
	is_failure = failure;
}
