/*********************************************************************

	hashfile.c

	Code for parsing hash info (*.hsi) files

*********************************************************************/

#include "hashfile.h"
#include "pool.h"
#include "expat.h"


/***************************************************************************

	Type definitions

***************************************************************************/

struct _hash_file
{
	mame_file *file;
	memory_pool pool;
	unsigned int functions[IO_COUNT];

	struct hash_info **preloaded_hashes;
	int preloaded_hash_count;

	void (*error_proc)(const char *message);
};



enum hash_parse_position
{
	POS_ROOT,
	POS_MAIN,
	POS_HASH
};



struct hash_parse_state
{
	XML_Parser parser;
	hash_file *hashfile;
	int done;
	
	int (*selector_proc)(hash_file *hashfile, void *param, const char *name, const char *hash);
	void (*use_proc)(hash_file *hashfile, void *param, struct hash_info *hi);
	void (*error_proc)(const char *message);
	void *param;

	enum hash_parse_position pos;
	char **text_dest;
	struct hash_info *hi;
};



/***************************************************************************

	Functions

***************************************************************************/

static void parse_error(struct hash_parse_state *state, const char *fmt, ...)
{
	char buf[256];
	va_list va;

	if (state->error_proc)
	{
		va_start(va, fmt);
		vsnprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, va);
		va_end(va);
		state->error_proc(buf);
	}
}



static void unknown_tag(struct hash_parse_state *state, const char *tagname)
{
	parse_error(state, "[%d:%d]: Unknown tag: %s\n",
		XML_GetCurrentLineNumber(state->parser),
		XML_GetCurrentColumnNumber(state->parser),
		tagname);
}



static void unknown_attribute(struct hash_parse_state *state, const char *attrname)
{
	parse_error(state, "[%d:%d]: Unknown attribute: %s\n",
		XML_GetCurrentLineNumber(state->parser),
		XML_GetCurrentColumnNumber(state->parser),
		attrname);
}



static void unknown_attribute_value(struct hash_parse_state *state,
	const char *attrname, const char *attrvalue)
{
	parse_error(state, "[%d:%d]: Unknown attribute value: %s\n",
		XML_GetCurrentLineNumber(state->parser),
		XML_GetCurrentColumnNumber(state->parser),
		attrvalue);
}



static void start_handler(void *data, const char *tagname, const char **attributes)
{
	struct hash_parse_state *state = (struct hash_parse_state *) data;
	const char *name;
	struct hash_info *hi;
	char **text_dest;
	char hash_string[HASH_BUF_SIZE];
	unsigned int functions, all_functions;
	iodevice_t device;
	int i;

	switch(state->pos++) {
	case POS_ROOT:
		if (!strcmp(tagname, "hashfile"))
		{
		}
		else
		{
			unknown_tag(state, tagname);
		}
		break;

	case POS_MAIN:
		if (!strcmp(tagname, "hash"))
		{
			// we are now examining a hash tag
			name = NULL;
			memset(hash_string, 0, sizeof(hash_string));
			all_functions = 0;
			device = IO_COUNT;

			while(attributes[0])
			{
				functions = 0;
				if (!strcmp(attributes[0], "name"))
				{
					/* name attribute */
					name = attributes[1];
				}
				else if (!strcmp(attributes[0], "crc32"))
				{
					/* crc32 attribute */
					functions = HASH_CRC;
				}
				else if (!strcmp(attributes[0], "md5"))
				{
					/* md5 attribute */
					functions = HASH_MD5;
				}
				else if (!strcmp(attributes[0], "sha1"))
				{
					/* sha1 attribute */
					functions = HASH_SHA1;
				}
				else if (!strcmp(attributes[0], "type"))
				{
					/* type attribute */
					i = device_typeid(attributes[1]);
					if (i < 0)
						unknown_attribute_value(state, attributes[0], attributes[1]);
					else
						device = (iodevice_t) i;
				}
				else
				{
					/* unknown attribute */
					unknown_attribute(state, attributes[0]);
				}

				if (functions)
				{
					hash_data_insert_printable_checksum(hash_string, functions, attributes[1]);
					all_functions |= functions;
				}

				attributes += 2;
			}

			if (device == IO_COUNT)
			{
				for (i = 0; i < IO_COUNT; i++)
					state->hashfile->functions[i] |= all_functions;
			}
			else
				state->hashfile->functions[device] |= all_functions;

			/* do we use this hash? */
			if (!state->selector_proc || state->selector_proc(state->hashfile, state->param, name, hash_string))
			{
				hi = pool_malloc(&state->hashfile->pool, sizeof(struct hash_info));
				if (!hi)
					return;
				memset(hi, 0, sizeof(*hi));

				hi->longname = pool_strdup(&state->hashfile->pool, name);
				if (!hi->longname)
					return;

				strcpy(hi->hash, hash_string);
				state->hi = hi;
			}
		}
		else
		{
			unknown_tag(state, tagname);
		}
		break;

	case POS_HASH:
		text_dest = NULL;

		if (!strcmp(tagname, "year"))
			text_dest = (char **) &state->hi->year;
		else if (!strcmp(tagname, "manufacturer"))
			text_dest = (char **) &state->hi->manufacturer;
		else if (!strcmp(tagname, "status"))
			text_dest = (char **) &state->hi->playable;
		else if (!strcmp(tagname, "extrainfo"))
			text_dest = (char **) &state->hi->extrainfo;
		else
			unknown_tag(state, tagname);

		if (text_dest && state->hi)
			state->text_dest = text_dest;
		break;
	}
}



static void end_handler(void *data, const char *name)
{
	struct hash_parse_state *state = (struct hash_parse_state *) data;
	state->text_dest = NULL;

	switch(--state->pos) {
	case POS_ROOT:
	case POS_HASH:
		break;

	case POS_MAIN:
		if (state->hi)
		{
			if (state->use_proc)
				state->use_proc(state->hashfile, state->param, state->hi);
			state->hi = NULL;
		}
		break;
	}
}



static void data_handler(void *data, const XML_Char *s, int len)
{
	struct hash_parse_state *state = (struct hash_parse_state *) data;
	int text_len;
	char *text;

	if (state->text_dest)
	{
		text = *state->text_dest;

		text_len = text ? strlen(text) : 0;
		text = pool_realloc(&state->hashfile->pool, text, text_len + len + 1);
		if (!text)
			return;

		memcpy(&text[text_len], s, len);
		text[text_len + len] = '\0';
		*state->text_dest = text;
	}
}



static void hashfile_parse(hash_file *hashfile,
	int (*selector_proc)(hash_file *hashfile, void *param, const char *name, const char *hash),
	void (*use_proc)(hash_file *hashfile, void *param, struct hash_info *hi),
	void (*error_proc)(const char *message),
	void *param)
{
	struct hash_parse_state state;
	char buf[1024];
	UINT32 len;

	mame_fseek(hashfile->file, 0, SEEK_SET);

	memset(&state, 0, sizeof(state));
	state.hashfile = hashfile;
	state.selector_proc = selector_proc;
	state.use_proc = use_proc;
	state.error_proc = error_proc;
	state.param = param;

	state.parser = XML_ParserCreate(NULL);
	if (!state.parser)
		goto done;

	XML_SetUserData(state.parser, &state);
	XML_SetElementHandler(state.parser, start_handler, end_handler);
	XML_SetCharacterDataHandler(state.parser, data_handler);

	while(!state.done)
	{
		len = mame_fread(hashfile->file, buf, sizeof(buf));
		state.done = mame_feof(hashfile->file);	
		if (XML_Parse(state.parser, buf, len, state.done) == XML_STATUS_ERROR)
		{
			parse_error(&state, "[%d:%d]: %s\n",
				XML_GetCurrentLineNumber(state.parser),
				XML_GetCurrentColumnNumber(state.parser),
				XML_ErrorString(XML_GetErrorCode(state.parser)));
			goto done;
		}
	}

done:
	if (state.parser)
		XML_ParserFree(state.parser);
}



/* ----------------------------------------------------------------------- */

static void preload_use_proc(hash_file *hashfile, void *param, struct hash_info *hi)
{
	struct hash_info **new_preloaded_hashes;

	new_preloaded_hashes = pool_realloc(&hashfile->pool, hashfile->preloaded_hashes,
		(hashfile->preloaded_hash_count + 1) * sizeof(struct hash_info *));
	if (!new_preloaded_hashes)
		return;

	hashfile->preloaded_hashes = new_preloaded_hashes;
	hashfile->preloaded_hashes[hashfile->preloaded_hash_count++] = hi;
}



hash_file *hashfile_open(const char *sysname, int is_preload,
	void (*error_proc)(const char *message))
{
	mame_file_error filerr;
	char *fname;
	hash_file *hashfile;
	
	hashfile = malloc(sizeof(struct _hash_file));
	if (!hashfile)
		goto error;
	memset(hashfile, 0, sizeof(*hashfile));
	pool_init(&hashfile->pool);
	hashfile->error_proc = error_proc;

	/* open a file */
	fname = assemble_2_strings(sysname, ".hsi");
	filerr = mame_fopen(SEARCHPATH_HASH, fname, OPEN_FLAG_READ, &hashfile->file);
	free(fname);

	if (filerr != FILERR_NONE)
		goto error;

	if (is_preload)
		hashfile_parse(hashfile, NULL, preload_use_proc, hashfile->error_proc, NULL);

	return hashfile;

error:
	if (hashfile)
		hashfile_close(hashfile);
	return NULL;
}



void hashfile_close(hash_file *hashfile)
{
	pool_exit(&hashfile->pool);
	if (hashfile->file)
		mame_fclose(hashfile->file);
	free(hashfile);
}



struct hashlookup_params
{
	const char *hash;
	struct hash_info *hi;
};



static int singular_selector_proc(hash_file *hashfile, void *param, const char *name, const char *hash)
{
	struct hashlookup_params *hlparams = (struct hashlookup_params *) param;
	return hash_data_is_equal(hash, hlparams->hash,
		hash_data_used_functions(hash)) == 1;
}



static void singular_use_proc(hash_file *hashfile, void *param, struct hash_info *hi)
{
	struct hashlookup_params *hlparams = (struct hashlookup_params *) param;
	hlparams->hi = hi;
}



const struct hash_info *hashfile_lookup(hash_file *hashfile, const char *hash)
{
	struct hashlookup_params param;
	int i;

	param.hash = hash;
	param.hi = NULL;

	for (i = 0; i < hashfile->preloaded_hash_count; i++)
	{
		if (singular_selector_proc(hashfile, &param, NULL, hashfile->preloaded_hashes[i]->hash))
			return hashfile->preloaded_hashes[i];
	}

	hashfile_parse(hashfile, singular_selector_proc, singular_use_proc,
		hashfile->error_proc, (void *) &param);
	return param.hi;
}



unsigned int hashfile_functions_used(hash_file *hashfile, iodevice_t devtype)
{
	assert(devtype >= 0);
	assert(devtype < IO_COUNT);
	return hashfile->functions[devtype];
}



int hashfile_verify(const char *sysname, void (*my_error_proc)(const char *message))
{
	hash_file *hashfile;
	
	hashfile = hashfile_open(sysname, FALSE, my_error_proc);
	if (!hashfile)
		return -1;

	hashfile_parse(hashfile, NULL, NULL, my_error_proc, NULL);
	hashfile_close(hashfile);
	return 0;
}

