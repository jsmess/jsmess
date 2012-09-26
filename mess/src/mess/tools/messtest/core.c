/*********************************************************************

    core.c

    MESS testing code

*********************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <expat.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif /* WIN32 */
#include "emu.h"
#include "core.h"
#include "testmess.h"
#include "testimgt.h"
#include "testzpth.h"
#include "osdepend.h"
#include "pool.h"
#include "pile.h"
#include "unzip.h"

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


int hexdigit(char c)
{
	int result = 0;
	if (isdigit(c))
		result = c - '0';
	else if (isxdigit(c))
		result = toupper(c) - 'A' + 10;
	return result;
}

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
	vsnprintf(buf, ARRAY_LENGTH(buf), fmt, va);
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
	size_t multiple = 0;

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
					multiple = -1;
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
					if (multiple == -1)
						multiple = 0;
					else
						multiple *= 10;
					multiple += c - '0';
				}
				else if ((c != '\0') && isspace(c) && (multiple == -1))
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



#define STANDARD_CODE_STRING(x)	{ x, #x },

static input_code messtest_token_to_code(const char *token)
{
	static const struct
	{
		int				code;
		const char *	codename;
	} standard_code_strings[] =
	{
		STANDARD_CODE_STRING(KEYCODE_A)
		STANDARD_CODE_STRING(KEYCODE_B)
		STANDARD_CODE_STRING(KEYCODE_C)
		STANDARD_CODE_STRING(KEYCODE_D)
		STANDARD_CODE_STRING(KEYCODE_E)
		STANDARD_CODE_STRING(KEYCODE_F)
		STANDARD_CODE_STRING(KEYCODE_G)
		STANDARD_CODE_STRING(KEYCODE_H)
		STANDARD_CODE_STRING(KEYCODE_I)
		STANDARD_CODE_STRING(KEYCODE_J)
		STANDARD_CODE_STRING(KEYCODE_K)
		STANDARD_CODE_STRING(KEYCODE_L)
		STANDARD_CODE_STRING(KEYCODE_M)
		STANDARD_CODE_STRING(KEYCODE_N)
		STANDARD_CODE_STRING(KEYCODE_O)
		STANDARD_CODE_STRING(KEYCODE_P)
		STANDARD_CODE_STRING(KEYCODE_Q)
		STANDARD_CODE_STRING(KEYCODE_R)
		STANDARD_CODE_STRING(KEYCODE_S)
		STANDARD_CODE_STRING(KEYCODE_T)
		STANDARD_CODE_STRING(KEYCODE_U)
		STANDARD_CODE_STRING(KEYCODE_V)
		STANDARD_CODE_STRING(KEYCODE_W)
		STANDARD_CODE_STRING(KEYCODE_X)
		STANDARD_CODE_STRING(KEYCODE_Y)
		STANDARD_CODE_STRING(KEYCODE_Z)
		STANDARD_CODE_STRING(KEYCODE_0)
		STANDARD_CODE_STRING(KEYCODE_1)
		STANDARD_CODE_STRING(KEYCODE_2)
		STANDARD_CODE_STRING(KEYCODE_3)
		STANDARD_CODE_STRING(KEYCODE_4)
		STANDARD_CODE_STRING(KEYCODE_5)
		STANDARD_CODE_STRING(KEYCODE_6)
		STANDARD_CODE_STRING(KEYCODE_7)
		STANDARD_CODE_STRING(KEYCODE_8)
		STANDARD_CODE_STRING(KEYCODE_9)
		STANDARD_CODE_STRING(KEYCODE_F1)
		STANDARD_CODE_STRING(KEYCODE_F2)
		STANDARD_CODE_STRING(KEYCODE_F3)
		STANDARD_CODE_STRING(KEYCODE_F4)
		STANDARD_CODE_STRING(KEYCODE_F5)
		STANDARD_CODE_STRING(KEYCODE_F6)
		STANDARD_CODE_STRING(KEYCODE_F7)
		STANDARD_CODE_STRING(KEYCODE_F8)
		STANDARD_CODE_STRING(KEYCODE_F9)
		STANDARD_CODE_STRING(KEYCODE_F10)
		STANDARD_CODE_STRING(KEYCODE_F11)
		STANDARD_CODE_STRING(KEYCODE_F12)
		STANDARD_CODE_STRING(KEYCODE_F13)
		STANDARD_CODE_STRING(KEYCODE_F14)
		STANDARD_CODE_STRING(KEYCODE_F15)
		STANDARD_CODE_STRING(KEYCODE_ESC)
		STANDARD_CODE_STRING(KEYCODE_TILDE)
		STANDARD_CODE_STRING(KEYCODE_MINUS)
		STANDARD_CODE_STRING(KEYCODE_EQUALS)
		STANDARD_CODE_STRING(KEYCODE_BACKSPACE)
		STANDARD_CODE_STRING(KEYCODE_TAB)
		STANDARD_CODE_STRING(KEYCODE_OPENBRACE)
		STANDARD_CODE_STRING(KEYCODE_CLOSEBRACE)
		STANDARD_CODE_STRING(KEYCODE_ENTER)
		STANDARD_CODE_STRING(KEYCODE_COLON)
		STANDARD_CODE_STRING(KEYCODE_QUOTE)
		STANDARD_CODE_STRING(KEYCODE_BACKSLASH)
		STANDARD_CODE_STRING(KEYCODE_BACKSLASH2)
		STANDARD_CODE_STRING(KEYCODE_COMMA)
		STANDARD_CODE_STRING(KEYCODE_STOP)
		STANDARD_CODE_STRING(KEYCODE_SLASH)
		STANDARD_CODE_STRING(KEYCODE_SPACE)
		STANDARD_CODE_STRING(KEYCODE_INSERT)
		STANDARD_CODE_STRING(KEYCODE_DEL)
		STANDARD_CODE_STRING(KEYCODE_HOME)
		STANDARD_CODE_STRING(KEYCODE_END)
		STANDARD_CODE_STRING(KEYCODE_PGUP)
		STANDARD_CODE_STRING(KEYCODE_PGDN)
		STANDARD_CODE_STRING(KEYCODE_LEFT)
		STANDARD_CODE_STRING(KEYCODE_RIGHT)
		STANDARD_CODE_STRING(KEYCODE_UP)
		STANDARD_CODE_STRING(KEYCODE_DOWN)
		STANDARD_CODE_STRING(KEYCODE_0_PAD)
		STANDARD_CODE_STRING(KEYCODE_1_PAD)
		STANDARD_CODE_STRING(KEYCODE_2_PAD)
		STANDARD_CODE_STRING(KEYCODE_3_PAD)
		STANDARD_CODE_STRING(KEYCODE_4_PAD)
		STANDARD_CODE_STRING(KEYCODE_5_PAD)
		STANDARD_CODE_STRING(KEYCODE_6_PAD)
		STANDARD_CODE_STRING(KEYCODE_7_PAD)
		STANDARD_CODE_STRING(KEYCODE_8_PAD)
		STANDARD_CODE_STRING(KEYCODE_9_PAD)
		STANDARD_CODE_STRING(KEYCODE_SLASH_PAD)
		STANDARD_CODE_STRING(KEYCODE_ASTERISK)
		STANDARD_CODE_STRING(KEYCODE_MINUS_PAD)
		STANDARD_CODE_STRING(KEYCODE_PLUS_PAD)
		STANDARD_CODE_STRING(KEYCODE_DEL_PAD)
		STANDARD_CODE_STRING(KEYCODE_ENTER_PAD)
		STANDARD_CODE_STRING(KEYCODE_PRTSCR)
		STANDARD_CODE_STRING(KEYCODE_PAUSE)
		STANDARD_CODE_STRING(KEYCODE_LSHIFT)
		STANDARD_CODE_STRING(KEYCODE_RSHIFT)
		STANDARD_CODE_STRING(KEYCODE_LCONTROL)
		STANDARD_CODE_STRING(KEYCODE_RCONTROL)
		STANDARD_CODE_STRING(KEYCODE_LALT)
		STANDARD_CODE_STRING(KEYCODE_RALT)
		STANDARD_CODE_STRING(KEYCODE_SCRLOCK)
		STANDARD_CODE_STRING(KEYCODE_NUMLOCK)
		STANDARD_CODE_STRING(KEYCODE_CAPSLOCK)
		STANDARD_CODE_STRING(KEYCODE_LWIN)
		STANDARD_CODE_STRING(KEYCODE_RWIN)
		STANDARD_CODE_STRING(KEYCODE_MENU)
	};

	char buf[128];
	int i;

	snprintf(buf, ARRAY_LENGTH(buf), "KEYCODE_%s", token);

	for (i = 0; i < ARRAY_LENGTH(standard_code_strings); i++)
	{
		if (!mame_stricmp(buf, standard_code_strings[i].codename))
			return standard_code_strings[i].code;
	}

	return INPUT_CODE_INVALID;
}



/* this external entity handler allows us to do things like this:
 *
 *  <!DOCTYPE tests
 *  [
 *      <!ENTITY mamekey_esc SYSTEM "http://www.mess.org/messtest/">
 *  ]>
 */
static int external_entity_handler(XML_Parser parser,
	const XML_Char *context,
	const XML_Char *base,
	const XML_Char *systemId,
	const XML_Char *publicId)
{
	XML_Parser extparser = NULL;
	int rc = 0;
	char buf[256];
	static const char mamekey_prefix[] = "mamekey_";
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
		c = messtest_token_to_code(context);

		if (c != INPUT_CODE_INVALID)
		{
			snprintf(buf, ARRAY_LENGTH(buf), "<%s%s>&#%d;</%s%s>",
				mamekey_prefix, context,
				UCHAR_MAMEKEY_BEGIN + c,
				mamekey_prefix, context);

			if (XML_Parse(extparser, buf, strlen(buf), 0) == XML_STATUS_ERROR)
				goto done;
		}
	}
	else if (!strcmp(context, "eoln"))
	{
		snprintf(buf, ARRAY_LENGTH(buf), "<eoln>%s</eoln>", EOLN);

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


#ifdef UNUSED_FUNCTION
static size_t parse_read(void *param, void *buffer, size_t length)
{
	return fread(buffer, 1, length, (FILE *) param);
}



static int parse_eof(void *param)
{
	return feof((FILE *) param);
}
#endif


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
		else if (!strcmp(child_node->name, "zippathtest"))
		{
			/* a zippath test */
			node_testzippath(child_node);

			(*test_count)++;
			if (is_failure)
				(*failure_count)++;
		}
	}
}



int messtest(const struct messtest_options *opts, int *test_count, int *failure_count)
{
	//char saved_directory[1024];
	file_error filerr;
	core_file *file;
	int result = -1;
//  char *script_directory;
	xml_parse_options parse_options;
	xml_parse_error parse_error;
	xml_data_node *root_node = NULL;
	xml_data_node *tests_node;

	*test_count = 0;
	*failure_count = 0;

	/* open the script file */
	filerr = core_fopen(opts->script_filename, OPEN_FLAG_READ, &file);
	if (filerr != FILERR_NONE)
	{
		fprintf(stderr, "%s: Cannot open file\n", opts->script_filename);
		goto done;
	}

	/* save the current working directory, and change to the test directory */
	//saved_directory[0] = '\0';
/*  if (!opts->preserve_directory)
    {
        script_directory = osd_dirname(opts->script_filename);
        if (script_directory)
        {
            osd_getcurdir(saved_directory, ARRAY_LENGTH(saved_directory));
            osd_setcurdir(script_directory);
            free(script_directory);
        }
    }*/

	/* set up parse options */
	memset(&parse_options, 0, sizeof(parse_options));
	parse_options.init_parser = parse_init;
	parse_options.flags = XML_PARSE_FLAG_WHITESPACE_SIGNIFICANT;
	parse_options.error = &parse_error;

	/* do the parse */
	root_node = xml_file_read(file, &parse_options);
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
//  if (saved_directory[0])
//      osd_setcurdir(saved_directory);
	if (file != NULL)
		core_fclose(file);
	if (root_node != NULL)
		xml_file_free(root_node);
	zip_file_cache_clear();
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
	vsnprintf(buf, ARRAY_LENGTH(buf), fmt, va);
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

#if 0
	/* did we abort? */
	if ((msgtype == MSG_FAILURE) && (state != STATE_ABORTED))
	{
		state = STATE_ABORTED;
		final_time = machine.time();
		if (final_time > 0.0)
			dump_screenshot();
	}
#endif
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
