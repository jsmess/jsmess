/*********************************************************************

    testimgt.c

    Imgtool testing code

*********************************************************************/

#include "testimgt.h"
#include "imageutl.h"
#include "../imgtool/imgtool.h"
#include "../imgtool/modules.h"



/***************************************************************************
    PARAMETERS
***************************************************************************/

#define VERBOSE_FILECHAIN	0



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _expected_dirent expected_dirent;
struct _expected_dirent
{
	const char *filename;
	int size;
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static const char *tempfile_name(void)
{
	static char buffer[256];
//  osd_get_temp_filename(buffer, ARRAY_LENGTH(buffer), NULL);
	return buffer;
}



static void report_imgtoolerr(imgtoolerr_t err)
{
	const char *msg;
	msg = imgtool_error(err);
	report_message(MSG_FAILURE, "%s", msg);
}



struct imgtooltest_state
{
	imgtool_image *image;
	imgtool_partition *partition;
	UINT64 recorded_freespace;
	int failed;
};



static void node_createimage(struct imgtooltest_state *state, xml_data_node *node)
{
	imgtoolerr_t err;
	xml_data_node *child_node;
	xml_attribute_node *attr_node;
	option_resolution *opts = NULL;
	const imgtool_module *module;
	const char *driver;
	const char *param_name;
	const char *param_value;

	attr_node = xml_get_attribute(node, "driver");
	if (!attr_node)
	{
		error_missingattribute("driver");
		return;
	}
	driver = attr_node->value;

	/* does image creation support options? */
	module = imgtool_find_module(attr_node->value);
	if (module && module->createimage_optguide && module->createimage_optspec)
		opts = option_resolution_create(module->createimage_optguide, module->createimage_optspec);

	report_message(MSG_INFO, "Creating image (module '%s')", driver);

	for (child_node = xml_get_sibling(node->child, "param"); child_node; child_node = xml_get_sibling(child_node->next, "param"))
	{
		if (!opts)
		{
			report_message(MSG_FAILURE, "Cannot specify creation options with this module");
			return;
		}

		attr_node = xml_get_attribute(child_node, "name");
		if (!attr_node)
		{
			error_missingattribute("name");
			return;
		}
		param_name = attr_node->value;

		attr_node = xml_get_attribute(child_node, "value");
		if (!attr_node)
		{
			error_missingattribute("value");
			return;
		}
		param_value = attr_node->value;

		option_resolution_add_param(opts, param_name, param_value);
	}

	err = imgtool_image_create_byname(driver, tempfile_name(), opts, &state->m_image);
	if (opts)
	{
		option_resolution_close(opts);
		opts = NULL;
	}
	if (err)
	{
		state->m_failed = 1;
		report_imgtoolerr(err);
		return;
	}

	err = imgtool_partition_open(state->m_image, 0, &state->m_partition);
	if (err)
	{
		state->m_failed = 1;
		report_imgtoolerr(err);
		return;
	}
}



static void get_file_params(xml_data_node *node, const char **filename, const char **fork,
	filter_getinfoproc *filter)
{
	xml_attribute_node *attr_node;

	*filename = NULL;
	*fork = NULL;
	*filter = NULL;

	attr_node = xml_get_attribute(node, "name");
	if (!attr_node)
	{
		error_missingattribute("name");
		return;
	}
	*filename = attr_node->value;

	attr_node = xml_get_attribute(node, "fork");
	if (attr_node)
		*fork = attr_node->value;

	attr_node = xml_get_attribute(node, "filter");
	if (attr_node)
	{
		*filter = filter_lookup(attr_node->value);

		if (!*filter)
			report_message(MSG_FAILURE, "Cannot find filter '%s'", attr_node->value);
	}
}



static void node_putfile(struct imgtooltest_state *state, xml_data_node *node)
{
	imgtoolerr_t err;
	const char *filename;
	const char *fork;
	filter_getinfoproc filter;
	imgtool_stream *stream = NULL;
	mess_pile pile;

	if (state->m_partition == NULL)
	{
		state->m_failed = 1;
		report_message(MSG_FAILURE, "Partition not loaded");
		goto done;
	}

	get_file_params(node, &filename, &fork, &filter);
	if (filename == NULL)
		goto done;

	pile_init(&pile);
	messtest_get_data(node, &pile);

	stream = stream_open_mem(NULL, 0);
	if (stream == NULL)
	{
		state->m_failed = 1;
		error_outofmemory();
		goto done;
	}

	stream_write(stream, pile_getptr(&pile), pile_size(&pile));
	stream_seek(stream, 0, SEEK_SET);
	pile_delete(&pile);

	err = imgtool_partition_write_file(state->m_partition, filename, fork, stream, NULL, filter);
	if (err)
	{
		state->m_failed = 1;
		report_imgtoolerr(err);
		goto done;
	}

	if (VERBOSE_FILECHAIN)
	{
		char buf[1024];
		err = imgtool_partition_get_chain_string(state->m_partition, filename, buf, ARRAY_LENGTH(buf));
		if (err == IMGTOOLERR_SUCCESS)
			report_message(MSG_INFO, "Filechain '%s': %s", filename, buf);
	}

done:
	if (stream != NULL)
		stream_close(stream);
}



static void node_checkfile(struct imgtooltest_state *state, xml_data_node *node)
{
	imgtoolerr_t err;
	const char *filename;
	const char *fork;
	filter_getinfoproc filter;
	imgtool_stream *stream = NULL;
	UINT64 stream_sz;
	const void *stream_ptr;
	mess_pile pile;

	if (!state->m_partition)
	{
		state->m_failed = 1;
		report_message(MSG_FAILURE, "Partition not loaded");
		goto done;
	}

	get_file_params(node, &filename, &fork, &filter);
	if (!filename)
		goto done;

	stream = stream_open_mem(NULL, 0);
	if (!stream)
	{
		state->m_failed = 1;
		error_outofmemory();
		goto done;
	}

	err = imgtool_partition_read_file(state->m_partition, filename, fork, stream, filter);
	if (err)
	{
		state->m_failed = 1;
		report_imgtoolerr(err);
		goto done;
	}

	pile_init(&pile);
	messtest_get_data(node, &pile);

	stream_ptr = stream_getptr(stream);
	stream_sz = stream_size(stream);

	if ((pile_size(&pile) != stream_sz) || (memcmp(stream_ptr, pile_getptr(&pile), pile_size(&pile))))
	{
		report_message(MSG_FAILURE, "Failed file verification");
		goto done;
	}

	pile_delete(&pile);

done:
	if (stream != NULL)
		stream_close(stream);
}



static void append_to_list(char *buf, size_t buflen, const char *entry)
{
	size_t pos;
	pos = strlen(buf);
	snprintf(buf + pos, buflen - pos - 1, (pos > 0) ? ", %s" : "%s", entry);
}



static void node_checkdirectory(struct imgtooltest_state *state, xml_data_node *node)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	imgtool_directory *imageenum;
	imgtool_dirent ent;
	char expected_listing[1024];
	char actual_listing[1024];
	int i/*, actual_count*/;
	int mismatch;
	xml_attribute_node *attr_node;
	xml_data_node *child_node;
	const char *filename;
	expected_dirent *entry;
	expected_dirent entries[256];
	int entry_count;

	if (!state->m_partition)
	{
		state->m_failed = 1;
		report_message(MSG_FAILURE, "Partition not loaded");
		return;
	}

	attr_node = xml_get_attribute(node, "path");
	filename = attr_node ? attr_node->value : "";

	memset(&entries, 0, sizeof(entries));
	entry_count = 0;

	for (child_node = xml_get_sibling(node->child, "entry"); child_node; child_node = xml_get_sibling(child_node->next, "entry"))
	{
		if (entry_count >= ARRAY_LENGTH(entries))
		{
			report_message(MSG_FAILURE, "Too many directory entries");
			return;
		}

		entry = &entries[entry_count++];

		attr_node = xml_get_attribute(child_node, "name");
		entry->filename = attr_node ? attr_node->value : NULL;

		attr_node = xml_get_attribute(child_node, "size");
		entry->size = attr_node ? atoi(attr_node->value) : -1;
	}

	/* build expected listing string */
	expected_listing[0] = '\0';
	for (i = 0; i < entry_count; i++)
	{
		append_to_list(expected_listing,
			ARRAY_LENGTH(expected_listing),
			entries[i].filename);
	}

	/* now enumerate though listing */
	//actual_count = 0;
	actual_listing[0] = '\0';
	mismatch = FALSE;

	memset(&ent, 0, sizeof(ent));

	err = imgtool_directory_open(state->m_partition, filename, &imageenum);
	if (err)
		goto done;

	i = 0;
	do
	{
		err = imgtool_directory_get_next(imageenum, &ent);
		if (err)
			goto done;

		if (!ent.eof)
		{
			append_to_list(actual_listing,
				ARRAY_LENGTH(actual_listing),
				ent.filename);

			if (i < entry_count && (strcmp(ent.filename, entries[i].filename)))
				mismatch = TRUE;
			i++;
		}
	}
	while(!ent.eof);

	if (i != entry_count)
		mismatch = TRUE;

	if (mismatch)
	{
		state->m_failed = 1;
		report_message(MSG_FAILURE, "File listing mismatch: {%s} expected {%s}",
			actual_listing, expected_listing);
		goto done;
	}

done:
	if (imageenum)
		imgtool_directory_close(imageenum);
	if (err)
	{
		state->m_failed = 1;
		report_imgtoolerr(err);
	}
}



static void node_deletefile(struct imgtooltest_state *state, xml_data_node *node)
{
	imgtoolerr_t err;
	xml_attribute_node *attr_node;

	if (!state->m_partition)
	{
		state->m_failed = 1;
		report_message(MSG_FAILURE, "Partition not loaded");
		return;
	}

	attr_node = xml_get_attribute(node, "name");
	if (!attr_node)
	{
		state->m_failed = 1;
		error_missingattribute("name");
		return;
	}

	err = imgtool_partition_delete_file(state->m_partition, attr_node->value);
	if (err)
	{
		state->m_failed = 1;
		report_imgtoolerr(err);
		return;
	}
}



static void node_createdirectory(struct imgtooltest_state *state, xml_data_node *node)
{
	imgtoolerr_t err;
	xml_attribute_node *attr_node;

	if (!state->m_partition)
	{
		state->m_failed = 1;
		report_message(MSG_FAILURE, "Partition not loaded");
		return;
	}

	attr_node = xml_get_attribute(node, "path");
	if (!attr_node)
	{
		state->m_failed = 1;
		error_missingattribute("path");
		return;
	}

	err = imgtool_partition_create_directory(state->m_partition, attr_node->value);
	if (err)
	{
		state->m_failed = 1;
		report_imgtoolerr(err);
		return;
	}
}



static void node_deletedirectory(struct imgtooltest_state *state, xml_data_node *node)
{
	imgtoolerr_t err;
	xml_attribute_node *attr_node;

	if (!state->m_partition)
	{
		state->m_failed = 1;
		report_message(MSG_FAILURE, "Partition not loaded");
		return;
	}

	attr_node = xml_get_attribute(node, "path");
	if (!attr_node)
	{
		state->m_failed = 1;
		error_missingattribute("path");
		return;
	}

	err = imgtool_partition_delete_directory(state->m_partition, attr_node->value);
	if (err)
	{
		state->m_failed = 1;
		report_imgtoolerr(err);
		return;
	}
}



static void node_recordfreespace(struct imgtooltest_state *state, xml_data_node *node)
{
	imgtoolerr_t err;

	if (!state->m_partition)
	{
		state->m_failed = 1;
		report_message(MSG_FAILURE, "Partition not loaded");
		return;
	}

	err = imgtool_partition_get_free_space(state->m_partition, &state->m_recorded_freespace);
	if (err)
	{
		state->m_failed = 1;
		report_imgtoolerr(err);
		return;
	}
}



static void node_checkfreespace(struct imgtooltest_state *state, xml_data_node *node)
{
	imgtoolerr_t err;
	UINT64 current_freespace;
	INT64 leaked_space;
	const char *verb;

	if (!state->m_partition)
	{
		state->m_failed = 1;
		report_message(MSG_FAILURE, "Partition not loaded");
		return;
	}

	err = imgtool_partition_get_free_space(state->m_partition, &current_freespace);
	if (err)
	{
		state->m_failed = 1;
		report_imgtoolerr(err);
		return;
	}

	if (state->m_recorded_freespace != current_freespace)
	{
		leaked_space = state->m_recorded_freespace - current_freespace;
		if (leaked_space > 0)
		{
			verb = "Leaked";
		}
		else
		{
			leaked_space = -leaked_space;
			verb = "Reverse leaked";
		}

		state->m_failed = 1;
		report_message(MSG_FAILURE, "%s %u bytes of space", verb, (unsigned int) leaked_space);
	}
}



static UINT32 identify_attribute(struct imgtooltest_state *state, xml_data_node *node, imgtool_attribute *value)
{
	xml_attribute_node *attr_node;
	UINT32 attribute;

	attr_node = xml_get_attribute(node, "name");
	if (!attr_node)
	{
		state->m_failed = 1;
		error_missingattribute("name");
		return 0;
	}

	if (!strcmp(attr_node->value, "mac_file_type"))
		attribute = IMGTOOLATTR_INT_MAC_TYPE;
	else if (!strcmp(attr_node->value, "mac_file_creator"))
		attribute = IMGTOOLATTR_INT_MAC_CREATOR;
	else
	{
		error_missingattribute("name");
		return 0;
	}

	attr_node = xml_get_attribute(node, "value");
	if (!attr_node)
	{
		state->m_failed = 1;
		error_missingattribute("value");
		return 0;
	}

	switch(attribute)
	{
		case IMGTOOLATTR_INT_MAC_TYPE:
		case IMGTOOLATTR_INT_MAC_CREATOR:
			if (strlen(attr_node->value) != 4)
			{
				state->m_failed = 1;
				error_missingattribute("value");
				return 0;
			}
			value->i = pick_integer_be(attr_node->value, 0, 4);
			break;
	}
	return attribute;
}



static void node_setattr(struct imgtooltest_state *state, xml_data_node *node)
{
	imgtoolerr_t err;
	xml_attribute_node *attr_node;
	UINT32 attribute;
	imgtool_attribute value;

	attr_node = xml_get_attribute(node, "path");
	if (!attr_node)
	{
		state->m_failed = 1;
		error_missingattribute("path");
		return;
	}

	attribute = identify_attribute(state, node, &value);
	if (!attribute)
		return;

	err = imgtool_partition_put_file_attribute(state->m_partition, attr_node->value, attribute, value);
	if (err)
	{
		state->m_failed = 1;
		report_imgtoolerr(err);
		return;
	}
}



static void node_checkattr(struct imgtooltest_state *state, xml_data_node *node)
{
	imgtoolerr_t err;
	xml_attribute_node *attr_node;
	UINT32 attribute;
	imgtool_attribute value;
	imgtool_attribute expected_value;

	memset(&value, 0, sizeof(value));
	memset(&expected_value, 0, sizeof(expected_value));

	attr_node = xml_get_attribute(node, "path");
	if (!attr_node)
	{
		state->m_failed = 1;
		error_missingattribute("path");
		return;
	}

	attribute = identify_attribute(state, node, &expected_value);
	if (!attribute)
		return;

	err = imgtool_partition_get_file_attribute(state->m_partition, attr_node->value, attribute, &value);
	if (err)
	{
		state->m_failed = 1;
		report_imgtoolerr(err);
		return;
	}

	if (memcmp(&value, &expected_value, sizeof(value)))
	{
		state->m_failed = 1;
		report_message(MSG_FAILURE, "Comparison failed");
		return;
	}
}



static void messtest_warn(const char *message)
{
	report_message(MSG_FAILURE, "%s", message);
}



void node_testimgtool(xml_data_node *node)
{
	xml_data_node *child_node;
	xml_attribute_node *attr_node;
	struct imgtooltest_state state;

	imgtool_init(FALSE, messtest_warn);

	attr_node = xml_get_attribute(node, "name");
	report_testcase_begin(attr_node ? attr_node->value : NULL);

	memset(&state, 0, sizeof(state));

	for (child_node = node->child; child_node; child_node = child_node->next)
	{
		if (!strcmp(child_node->name, "createimage"))
			node_createimage(&state, child_node);
		else if (!strcmp(child_node->name, "checkfile"))
			node_checkfile(&state, child_node);
		else if (!strcmp(child_node->name, "checkdirectory"))
			node_checkdirectory(&state, child_node);
		else if (!strcmp(child_node->name, "putfile"))
			node_putfile(&state, child_node);
		else if (!strcmp(child_node->name, "deletefile"))
			node_deletefile(&state, child_node);
		else if (!strcmp(child_node->name, "createdirectory"))
			node_createdirectory(&state, child_node);
		else if (!strcmp(child_node->name, "deletedirectory"))
			node_deletedirectory(&state, child_node);
		else if (!strcmp(child_node->name, "recordfreespace"))
			node_recordfreespace(&state, child_node);
		else if (!strcmp(child_node->name, "checkfreespace"))
			node_checkfreespace(&state, child_node);
		else if (!strcmp(child_node->name, "setattr"))
			node_setattr(&state, child_node);
		else if (!strcmp(child_node->name, "checkattr"))
			node_checkattr(&state, child_node);
	}

	report_testcase_ran(state.failed);

	/* close out any existing partition */
	if (state.partition)
	{
		imgtool_partition_close(state.partition);
		state.partition = NULL;
	}

	/* close out any existing image */
	if (state.image)
	{
		imgtool_image_close(state.image);
		state.image = NULL;
	}

	imgtool_exit();
}
