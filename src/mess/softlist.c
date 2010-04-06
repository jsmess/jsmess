/***************************************************************************

    softlist.c

    Software list construction helpers.


***************************************************************************/

#include "emu.h"
#include "pool.h"
#include "expat.h"
#include "emuopts.h"
#include "softlist.h"


enum softlist_parse_position
{
	POS_ROOT,
	POS_MAIN,
	POS_SOFT,
	POS_PART,
	POS_DATA
};


typedef struct _parse_state
{
	XML_Parser	parser;
	int			done;

	void (*error_proc)(const char *message);
	void *param;

	enum softlist_parse_position pos;
	char **text_dest;
} parse_state;


struct software_info_list
{
	software_info				info;
	struct software_info_list	*next;
};


struct _software_list
{
	mame_file	*file;
	object_pool	*pool;
	parse_state	state;
	struct software_info_list	*software_info_list;
	struct software_info_list	*current_software_info;
	software_info	*softinfo;
	const char *look_for;
	int part_entries;
	int current_part_entry;
	int rom_entries;
	int current_rom_entry;
	void (*error_proc)(const char *message);
};


/***************************************************************************
    EXPAT INTERFACES
***************************************************************************/

/*-------------------------------------------------
    expat_malloc/expat_realloc/expat_free -
    wrappers for memory allocation functions so
    that they pass through out memory tracking
    systems
-------------------------------------------------*/

static void *expat_malloc(size_t size)
{
	return global_alloc_array_clear(UINT8,size);
}

static void *expat_realloc(void *ptr, size_t size)
{
	if (ptr) global_free(ptr);
	return global_alloc_array_clear(UINT8,size);
}

static void expat_free(void *ptr)
{
	global_free(ptr);
}


/*-------------------------------------------------
    parse_error
-------------------------------------------------*/

INLINE void ATTR_PRINTF(2,3) parse_error(parse_state *state, const char *fmt, ...)
{
	char buf[256];
	va_list va;

	if (state->error_proc)
	{
		va_start(va, fmt);
		vsnprintf(buf, ARRAY_LENGTH(buf), fmt, va);
		va_end(va);
		(*state->error_proc)(buf);
	}
}


/*-------------------------------------------------
    unknown_tag
-------------------------------------------------*/

INLINE void unknown_tag(parse_state *state, const char *tagname)
{
	parse_error(state, "[%lu:%lu]: Unknown tag: %s\n",
		XML_GetCurrentLineNumber(state->parser),
		XML_GetCurrentColumnNumber(state->parser),
		tagname);
}



/*-------------------------------------------------
    unknown_attribute
-------------------------------------------------*/

INLINE void unknown_attribute(parse_state *state, const char *attrname)
{
	parse_error(state, "[%lu:%lu]: Unknown attribute: %s\n",
		XML_GetCurrentLineNumber(state->parser),
		XML_GetCurrentColumnNumber(state->parser),
		attrname);
}



/*-------------------------------------------------
    unknown_attribute_value
-------------------------------------------------*/

INLINE void unknown_attribute_value(parse_state *state,
	const char *attrname, const char *attrvalue)
{
	parse_error(state, "[%lu:%lu]: Unknown attribute value: %s\n",
		XML_GetCurrentLineNumber(state->parser),
		XML_GetCurrentColumnNumber(state->parser),
		attrvalue);
}


/*-------------------------------------------------
	software_name_split
	helper; splits a software_list:software:part
    string into seperate software_list, software,
    and part strings.

    str1:str2:str3  => swlist_name - str1, swname - str2, swpart - str3
	str1:str2		=> swlist_name - NULL, swname - str1, swpart - str2
    str1			=> swlist_name - NULL, swname - str1, swpart - NULL

    swlist_namem, swnane and swpart will be global_alloc'ed
    from the global pool. So they should be global_free'ed
    when they are not used anymore.
-------------------------------------------------*/
static void software_name_split(const char *swlist_swname, char **swlist_name, char **swname, char **swpart )
{
	const char *split_1st_loc = strchr( swlist_swname, ':' );
	const char *split_2nd_loc = ( split_1st_loc ) ? strchr( split_1st_loc + 1, ':' ) : NULL;

	*swlist_name = NULL;
	*swname = NULL;
	*swpart = NULL;

	if ( split_1st_loc )
	{
		if ( split_2nd_loc )
		{
			int size = split_1st_loc - swlist_swname;
			*swlist_name = global_alloc_array_clear(char,size+1);
			memcpy( *swlist_name, swlist_swname, size );

			size = split_2nd_loc - ( split_1st_loc + 1 );
			*swname = global_alloc_array_clear(char,size+1);
			memcpy( *swname, split_1st_loc + 1, size );

			size = strlen( swlist_swname ) - ( split_2nd_loc + 1 - swlist_swname );
			*swpart = global_alloc_array_clear(char,size+1);
			memcpy( *swpart, split_2nd_loc + 1, size );
		}
		else
		{
			int size = split_1st_loc - swlist_swname;
			*swname = global_alloc_array_clear(char,size+1);
			memcpy( *swname, swlist_swname, size );

			size = strlen( swlist_swname ) - ( split_1st_loc + 1 - swlist_swname );
			*swpart = global_alloc_array_clear(char,size+1);
			memcpy( *swpart, split_1st_loc + 1, size );
		}
	}
	else
	{
		int size = strlen( swlist_swname );
		*swname = global_alloc_array_clear(char,size+1);
		memcpy( *swname, swlist_swname, size );
	}
}


/*-------------------------------------------------
    add_rom_entry
-------------------------------------------------*/

static void add_rom_entry(software_list *swlist, const char *name, const char *hashdata, UINT32 offset, UINT32 length, UINT32 flags)
{
	software_part *part = &swlist->softinfo->partdata[swlist->current_part_entry-1];
	struct rom_entry *entry = &part->romdata[swlist->current_rom_entry];

	entry->_name = name;
	entry->_hashdata = hashdata;
	entry->_offset = offset;
	entry->_length = length;
	entry->_flags = flags;

	swlist->current_rom_entry += 1;

	if ( swlist->current_rom_entry >= swlist->rom_entries )
	{
		struct rom_entry *new_entries;

		swlist->rom_entries += 10;
		new_entries = (struct rom_entry *)pool_realloc_lib(swlist->pool, part->romdata, swlist->rom_entries * sizeof(struct rom_entry) );

		if ( new_entries )
		{
			part->romdata = new_entries;
		}
		else
		{
			/* Allocation error */
			swlist->current_rom_entry -= 1;
		}
	}
}


/*-------------------------------------------------
    add_software_part
-------------------------------------------------*/

static void add_software_part(software_list *swlist, const char *name, const char *interface, const char *feature)
{
	software_part *part = &swlist->softinfo->partdata[swlist->current_part_entry];

	part->name = name;
	part->interface_ = interface;
	part->feature = feature;
	part->romdata = NULL;

	swlist->current_part_entry += 1;

	if ( swlist->current_part_entry >= swlist->part_entries )
	{
		software_part *new_parts;

		swlist->part_entries += 2;
		new_parts = (software_part *)pool_realloc_lib(swlist->pool, swlist->softinfo->partdata, swlist->part_entries * sizeof(software_part) );

		if ( new_parts )
		{
			swlist->softinfo->partdata = new_parts;
		}
		else
		{
			/* Allocation error */
			swlist->current_part_entry -= 1;
		}
	}
}


/*-------------------------------------------------
    start_handler
-------------------------------------------------*/

static void start_handler(void *data, const char *tagname, const char **attributes)
{
	software_list *swlist = (software_list *) data;
	char **text_dest;

	switch(swlist->state.pos)
	{
		case POS_ROOT:
			if (!strcmp(tagname, "softwarelist"))
			{
				for( ; attributes[0]; attributes += 2 )
				{
					if ( ! strcmp(attributes[0], "name" ) )
					{
					}
					if ( ! strcmp(attributes[0], "description" ) )
					{
					}
				}
			}
			else
			{
				unknown_tag(&swlist->state, tagname);
			}
			break;

		case POS_MAIN:
			if ( !strcmp( tagname, "software" ) )
			{
				const char *name = NULL;
				const char *supported = NULL;

				for ( ; attributes[0]; attributes += 2 )
				{
					if ( !strcmp( attributes[0], "name" ) )
					{
						name = attributes[1];
					}
					if ( !strcmp( attributes[0], "supported" ) )
					{
						supported = attributes[1];
					}
				}

				if ( name && !mame_strwildcmp( swlist->look_for, name ) )
				{
					struct software_info_list *elem = (struct software_info_list *)pool_malloc_lib(swlist->pool,sizeof(struct software_info_list));

					if ( !elem )
						return;

					/* Clear element and add element to list */
					memset(elem,0,sizeof(struct software_info_list));

					/* Allocate space to hold the shortname and copy the short name */
					elem->info.shortname = (const char*)pool_malloc_lib(swlist->pool, ( strlen( name ) + 1 ) * sizeof(char) );

					if ( ! elem->info.shortname )
						return;

					strcpy( (char *)elem->info.shortname, name );

					/* Allocate initial space to hold part information */
					swlist->part_entries = 2;
					swlist->current_part_entry = 0;
					elem->info.partdata = (software_part *)pool_malloc_lib(swlist->pool, swlist->part_entries * sizeof(software_part) );
					if ( !elem->info.partdata )
						return;

					/* Handle the supported flag */
					elem->info.supported = SOFTWARE_SUPPORTED_YES;
					if ( supported && ! strcmp( supported, "partial" ) )
						elem->info.supported = SOFTWARE_SUPPORTED_PARTIAL;
					if ( supported && ! strcmp( supported, "no" ) )
						elem->info.supported = SOFTWARE_SUPPORTED_NO;

					/* Add the entry to the end of the list */
					if ( swlist->software_info_list == NULL )
					{
						swlist->software_info_list = elem;
						swlist->current_software_info = elem;
					}
					else
					{
						swlist->current_software_info->next = elem;
						swlist->current_software_info = elem;
					}

					/* Quick lookup for setting software information */
					swlist->softinfo = &swlist->current_software_info->info;
				}
				else
				{
					swlist->softinfo = NULL;
				}
			}
			else
			{
				unknown_tag(&swlist->state, tagname);
			}
			break;

		case POS_SOFT:
			text_dest = NULL;

			if (!strcmp(tagname, "description"))
				text_dest = (char **) &swlist->softinfo->longname;
			else if (!strcmp(tagname, "year"))
				text_dest = (char **) &swlist->softinfo->year;
			else if (!strcmp(tagname, "publisher"))
				text_dest = (char **) &swlist->softinfo->publisher;
			else if ( !strcmp(tagname, "part" ) )
			{
				const char *str_name = NULL;
				const char *str_interface = NULL;
				const char *str_feature = NULL;

				for ( ; attributes[0]; attributes += 2 )
				{
					if ( !strcmp( attributes[0], "name" ) )
						str_name = attributes[1];

					if ( !strcmp( attributes[0], "interface" ) )
						str_interface = attributes[1];

					if ( !strcmp( attributes[0], "feature" ) )
						str_feature = attributes[1];
				}

				if ( str_name && str_interface )
				{
					if ( swlist->softinfo )
					{
						char *name = (char *)pool_malloc_lib(swlist->pool, ( strlen( str_name ) + 1 ) * sizeof(char) );
						char *interface = (char *)pool_malloc_lib(swlist->pool, ( strlen( str_interface ) + 1 ) * sizeof(char) );
						char *feature = NULL;

						if ( !name || !interface )
							return;

						strcpy( name, str_name );
						strcpy( interface, str_interface );

						if ( str_feature )
						{
							feature = (char *)pool_malloc_lib(swlist->pool, ( strlen( str_feature ) + 1 ) * sizeof(char) );
							if ( !feature )
								return;
							strcpy( feature, str_feature );
						}

						add_software_part( swlist, name, interface, feature );

						/* Allocate initial space to hold the rom information */
						swlist->rom_entries = 3;
						swlist->current_rom_entry = 0;
						swlist->softinfo->partdata[swlist->current_part_entry-1].romdata = (struct rom_entry *)pool_malloc_lib(swlist->pool, swlist->rom_entries * sizeof(struct rom_entry));
						if ( ! swlist->softinfo->partdata[swlist->current_part_entry-1].romdata )
							return;
					}
				}
				else
				{
					/* Incomplete/incorrect part definition */
				}
			}
			else
				unknown_tag(&swlist->state, tagname);

			if (text_dest && swlist->softinfo)
				swlist->state.text_dest = text_dest;
			break;

		case POS_PART:
			if (!strcmp(tagname, "dataarea"))
			{
				const char *str_name = NULL;
				const char *str_size = NULL;

				for ( ; attributes[0]; attributes += 2 )
				{
					if ( !strcmp( attributes[0], "name" ) )
						str_name = attributes[1];

					if ( !strcmp( attributes[0], "size") )
						str_size = attributes[1];
				}
				if ( str_name && str_size )
				{
					if ( swlist->softinfo )
					{
						UINT32 length = strtol( str_size, NULL, 10 );
						char *s = (char *)pool_malloc_lib(swlist->pool, ( strlen( str_name ) + 1 ) * sizeof(char) );

						if ( !s )
							return;

						strcpy( s, str_name );

						/* ROM_REGION( length, "name", flags ) */
						add_rom_entry( swlist, s, NULL, 0, length, ROMENTRYTYPE_REGION );
					}
				}
				else
				{
					/* Missing dataarea name or size */
				}
			}
			else
				unknown_tag( &swlist->state, tagname );
			break;

		case POS_DATA:
			if (!strcmp(tagname, "rom"))
			{
				const char *str_name = NULL;
				const char *str_size = NULL;
				const char *str_crc = NULL;
				const char *str_sha1 = NULL;
				const char *str_offset = NULL;
				const char *str_status = NULL;
				const char *str_loadflag = NULL;

				for ( ; attributes[0]; attributes += 2 )
				{
					if ( !strcmp( attributes[0], "name" ) )
						str_name = attributes[1];
					if ( !strcmp( attributes[0], "size" ) )
						str_size = attributes[1];
					if ( !strcmp( attributes[0], "crc" ) )
						str_crc = attributes[1];
					if ( !strcmp( attributes[0], "sha1" ) )
						str_sha1 = attributes[1];
					if ( !strcmp( attributes[0], "offset" ) )
						str_offset = attributes[1];
					if ( !strcmp( attributes[0], "status" ) )
						str_status = attributes[1];
					if ( !strcmp( attributes[0], "loadflag" ) )
						str_loadflag = attributes[1];
				}
				if ( str_name && str_size && str_crc && str_sha1 && str_offset )
				{
					if ( swlist->softinfo )
					{
						UINT32 length = strtol( str_size, NULL, 10 );
						UINT32 offset = strtol( str_offset, NULL, 16 );
						char *s_name = (char *)pool_malloc_lib(swlist->pool, ( strlen( str_name ) + 1 ) * sizeof(char) );
						char *hashdata = (char *)pool_malloc_lib( swlist->pool, sizeof(char) * ( strlen(str_crc) + strlen(str_sha1) + 7 ) );
						int baddump = ( str_status && !strcmp(str_status,"baddump") ) ? 1 : 0;
						int nodump = ( str_status && !strcmp(str_status,"nodump" ) ) ? 1 : 0;

						if ( !s_name || !hashdata )
							return;

						strcpy( s_name, str_name );
						sprintf( hashdata, "c:%s#s:%s#%s", str_crc, str_sha1, ( nodump ? NO_DUMP : ( baddump ? BAD_DUMP : 0 ) ) );

						/* ROM_LOAD( name, offset, length, hash ) */
						add_rom_entry( swlist, s_name, hashdata, offset, length, ROMENTRYTYPE_ROM );
					}
				}
				else
				{
					/* Missing name, size, crc, sha1, or offset */
				}
			}
			else
				unknown_tag(&swlist->state, tagname);
			break;
	}
	swlist->state.pos = (softlist_parse_position) (swlist->state.pos + 1);
}

/*-------------------------------------------------
    end_handler
-------------------------------------------------*/

static void end_handler(void *data, const char *name)
{
	software_list *swlist = (software_list *) data;
	swlist->state.text_dest = NULL;

	swlist->state.pos = (softlist_parse_position) (swlist->state.pos - 1);
	switch(swlist->state.pos)
	{
		case POS_ROOT:
			break;

		case POS_SOFT:
			if ( ! strcmp( name, "part" ) && swlist->softinfo )
			{
				/* ROM_END */
				add_rom_entry( swlist, NULL, NULL, 0, 0, ROMENTRYTYPE_END );
			}
			break;

		case POS_MAIN:
			if ( swlist->softinfo )
			{
				add_software_part( swlist, NULL, NULL, NULL );
			}
			break;

		case POS_PART:
			break;

		case POS_DATA:
			break;
	}
}


/*-------------------------------------------------
    data_handler
-------------------------------------------------*/

static void data_handler(void *data, const XML_Char *s, int len)
{
	software_list *swlist = (software_list *) data;
	int text_len;
	char *text;

	if (swlist->state.text_dest)
	{
		text = *swlist->state.text_dest;

		text_len = text ? strlen(text) : 0;
		text = (char*)pool_realloc_lib(swlist->pool, text, text_len + len + 1);
		if (!text)
			return;

		memcpy(&text[text_len], s, len);
		text[text_len + len] = '\0';
		*swlist->state.text_dest = text;
	}
}


/*-------------------------------------------------
    software_list_parse
-------------------------------------------------*/

static void software_list_parse(software_list *swlist,
	void (*error_proc)(const char *message),
	void *param)
{
	char buf[1024];
	UINT32 len;
	XML_Memory_Handling_Suite memcallbacks;

	mame_fseek(swlist->file, 0, SEEK_SET);

	memset(&swlist->state, 0, sizeof(swlist->state));
	swlist->state.error_proc = error_proc;
	swlist->state.param = param;

	/* create the XML parser */
	memcallbacks.malloc_fcn = expat_malloc;
	memcallbacks.realloc_fcn = expat_realloc;
	memcallbacks.free_fcn = expat_free;
	swlist->state.parser = XML_ParserCreate_MM(NULL, &memcallbacks, NULL);
	if (!swlist->state.parser)
		goto done;

	XML_SetUserData(swlist->state.parser, swlist);
	XML_SetElementHandler(swlist->state.parser, start_handler, end_handler);
	XML_SetCharacterDataHandler(swlist->state.parser, data_handler);

	while(!swlist->state.done)
	{
		len = mame_fread(swlist->file, buf, sizeof(buf));
		swlist->state.done = mame_feof(swlist->file);
		if (XML_Parse(swlist->state.parser, buf, len, swlist->state.done) == XML_STATUS_ERROR)
		{
			parse_error(&swlist->state, "[%lu:%lu]: %s\n",
				XML_GetCurrentLineNumber(swlist->state.parser),
				XML_GetCurrentColumnNumber(swlist->state.parser),
				XML_ErrorString(XML_GetErrorCode(swlist->state.parser)));
			goto done;
		}
	}

done:
	if (swlist->state.parser)
		XML_ParserFree(swlist->state.parser);
	swlist->state.parser = NULL;
	swlist->current_software_info = swlist->software_info_list;
}


/*-------------------------------------------------
    software_list_open
-------------------------------------------------*/

software_list *software_list_open(core_options *options, const char *listname, int is_preload,
	void (*error_proc)(const char *message))
{
	file_error filerr;
	astring *fname;
	software_list *swlist = NULL;
	object_pool *pool = NULL;

	/* create a pool for this software list file */
	pool = pool_alloc_lib(error_proc);
	if (!pool)
		goto error;

	/* allocate space for this software list file */
	swlist = (software_list *) pool_malloc_lib(pool, sizeof(*swlist));
	if (!swlist)
		goto error;

	/* set up the software_list structure */
	memset(swlist, 0, sizeof(*swlist));
	swlist->pool = pool;
	swlist->error_proc = error_proc;

	/* open a file */
	fname = astring_assemble_2(astring_alloc(), listname, ".xml");
	filerr = mame_fopen_options(options, SEARCHPATH_HASH, astring_c(fname), OPEN_FLAG_READ, &swlist->file);
	astring_free(fname);

	if (filerr != FILERR_NONE)
		goto error;

//	if (is_preload)
//		software_list_parse(swlist, swlist->error_proc, NULL);

	return swlist;

error:
	if (swlist != NULL)
		software_list_close(swlist);
	return NULL;
}


/*-------------------------------------------------
    software_list_close
-------------------------------------------------*/

void software_list_close(software_list *swlist)
{
	if (swlist->file)
		mame_fclose(swlist->file);
	pool_free_lib(swlist->pool);
}


/*-------------------------------------------------
    software_list_find
-------------------------------------------------*/

software_info *software_list_find(software_list *swlist, const char *software)
{
	swlist->look_for = software;
	software_list_parse( swlist, swlist->error_proc, NULL );

	return swlist->current_software_info ? &swlist->current_software_info->info : NULL;
}


/*-------------------------------------------------
    software_list_first
-------------------------------------------------*/

software_info *software_list_first(software_list *swlist)
{
	return software_list_find(swlist, "*");
}


/*-------------------------------------------------
    software_list_next
-------------------------------------------------*/

software_info *software_list_next(software_list *swlist)
{
	if ( swlist->current_software_info )
	{
		swlist->current_software_info = swlist->current_software_info->next;
	}
	return swlist->current_software_info ? &swlist->current_software_info->info : NULL;
}


/*-------------------------------------------------
    software_find_part
-------------------------------------------------*/

software_part *software_find_part(software_info *sw, const char *partname, const char *interface)
{
	software_part *part = sw ? sw->partdata : NULL;

	 /* If neither partname nor interface supplied, then we just return the first entry */
	if ( partname || interface )
	{
		while( part && part->name )
		{
			if ( partname )
			{
				if ( !strcmp(partname, part->name ) )
				{
					if ( interface )
					{
						if ( !strcmp(interface, part->interface_) )
						{
							break;
						}
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				/* No specific partname supplied, find the first match based on interface */
				if ( interface )
				{
					if ( !strcmp(interface, part->interface_) )
					{
						break;
					}
				}
			}
			part++;
		}
	}

	if ( ! part->name )
		part = NULL;

	return part;
}


/*-------------------------------------------------
    software_part_next
-------------------------------------------------*/

software_part *software_part_next(software_part *part)
{
	if ( part && part->name )
	{
		part++;
	}

	if ( ! part->name )
		part = NULL;

	return part;
}


/*-------------------------------------------------
    load_software_part

    Load a software part for a device. The part to
    load is determined by the "path", software lists
    configured for a driver, and the interface
    supported by the device.

    returns true if the software could be loaded,
    false otherwise. If the software could be loaded
    sw_info and sw_part are also set.
-------------------------------------------------*/

bool load_software_part(running_device *device, const char *path, software_info **sw_info, software_part **sw_part, char **full_sw_name)
{
	char *swlist_name, *swname, *swpart;
	const char *interface;
	bool result = false;
	software_list *software_list_ptr = NULL;
	software_info *software_info_ptr = NULL;
	software_part *software_part_ptr = NULL;

	*sw_info = NULL;
	*sw_part = NULL;

	/* Split full software name into software list name and short software name */
	software_name_split( path, &swlist_name, &swname, &swpart );

	interface = device->get_config_string(DEVINFO_STR_INTERFACE);

	if ( swlist_name )
	{
		/* Try to open the software list xml file explicitly named by the user */
		software_list_ptr = software_list_open( mame_options(), swlist_name, FALSE, NULL );

		if ( software_list_ptr )
		{
			software_info_ptr = software_list_find( software_list_ptr, swname );

			if ( software_info_ptr )
			{
				software_part_ptr = software_find_part( software_info_ptr, swpart, interface );
			}
		}
	}
	else
	{
		/* Loop through all the software lists named in the driver */
		running_device *swlists = devtag_get_device( device->machine, __SOFTWARE_LIST_TAG );

		if ( swlists )
		{
			UINT32 i = DEVINFO_STR_SWLIST_0;

			while ( ! software_part_ptr && i <= DEVINFO_STR_SWLIST_MAX )
			{
				swlist_name = (char *)swlists->baseconfig().get_config_string( i );

				if ( swlist_name && *swlist_name )
				{
					if ( software_list_ptr )
					{
						software_list_close( software_list_ptr );
					}

					software_list_ptr = software_list_open( mame_options(), swlist_name, FALSE, NULL );

					if ( software_list_ptr )
					{
						software_info_ptr = software_list_find( software_list_ptr, swname );

						if ( software_info_ptr )
						{
							software_part_ptr = software_find_part( software_info_ptr, swpart, interface );
						}
					}
				}

				i++;
			}
		}

		/* If not found try to load the software list using the driver name */
		if ( ! software_part_ptr )
		{
			swlist_name = (char *)device->machine->gamedrv->name;

			if ( software_list_ptr )
			{
				software_list_close( software_list_ptr );
			}

			software_list_ptr = software_list_open( mame_options(), swlist_name, FALSE, NULL );

			if ( software_list_ptr )
			{
				software_info_ptr = software_list_find( software_list_ptr, swname );

				if ( software_info_ptr )
				{   
					software_part_ptr = software_find_part( software_info_ptr, swpart, interface );
				}
			}
		}

		/* If not found try to load the software list using the software name as software */
		/* list name and software part name as software name. */
		if ( ! software_part_ptr )
		{
			swlist_name = swname;
			swname = swpart;
			swpart = NULL;

			if ( software_list_ptr )
			{
				software_list_close( software_list_ptr );
			}

			software_list_ptr = software_list_open( mame_options(), swlist_name, FALSE, NULL );

			if ( software_list_ptr )
			{
				software_info_ptr = software_list_find( software_list_ptr, swname );

				if ( software_info_ptr )
				{
					software_part_ptr = software_find_part( software_info_ptr, swpart, interface );
				}

				if ( ! software_part_ptr )
				{
					software_list_close( software_list_ptr );
					software_list_ptr = NULL;
				}
			}
		}
	}

	if ( software_part_ptr )
	{
		/* Load the software part */
		load_software_part_region( device, (char *)swlist_name, (char *)software_info_ptr->shortname, software_part_ptr->romdata );

		/* Create a copy of the software and part information */
		*sw_info = auto_alloc_clear( device->machine, software_info );
		(*sw_info)->shortname = auto_strdup( device->machine, software_info_ptr->shortname );
		(*sw_info)->longname = auto_strdup( device->machine, software_info_ptr->longname );
		if ( software_info_ptr->year )
			(*sw_info)->year = auto_strdup( device->machine, software_info_ptr->year );
		if ( software_info_ptr->publisher )
			(*sw_info)->publisher = auto_strdup( device->machine, software_info_ptr->publisher );

		*sw_part = auto_alloc_clear( device->machine, software_part );
		(*sw_part)->name = auto_strdup( device->machine, software_part_ptr->name );
		if ( software_part_ptr->interface_ )
			(*sw_part)->interface_ = auto_strdup( device->machine, software_part_ptr->interface_ );
		if ( software_part_ptr->feature )
			(*sw_part)->feature = auto_strdup( device->machine, software_part_ptr->feature );

		/* Tell the world which part we actually loaded */
		*full_sw_name = auto_alloc_array( device->machine, char, strlen(swlist_name) + strlen(software_info_ptr->shortname) + strlen(software_part_ptr->name) + 3 );
		sprintf( *full_sw_name, "%s:%s:%s", swlist_name, software_info_ptr->shortname, software_part_ptr->name );

		result = true;
	}

	/* Close the software list if it's still open */
	if ( software_list_ptr )
	{
		software_list_close( software_list_ptr );
		software_info_ptr = NULL;
		software_list_ptr = NULL;
	}
	global_free( swlist_name );
	global_free( swname );
	global_free( swpart );

	return result;
}


/***************************************************************************
    DEVICE INTERFACE
***************************************************************************/


static DEVICE_START( software_list )
{
}


DEVICE_GET_INFO( software_list )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = 1;										break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(software_list_config);				break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( software_list );	break;
		case DEVINFO_FCT_STOP:							/* Nothing */										break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Software lists");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Software lists");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");				break;
	}

	if ( state >= DEVINFO_STR_SWLIST_0 && state <= DEVINFO_STR_SWLIST_MAX )
	{
		software_list_config *config = (software_list_config *)device->inline_config;

		if ( config->list_name[ state - DEVINFO_STR_SWLIST_0 ] )
			strcpy(info->s, config->list_name[ state - DEVINFO_STR_SWLIST_0 ]);
	}
}

