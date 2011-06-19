/*********************************************************************

    testzpth.c

    Code for testing zip paths

*********************************************************************/

#include "testzpth.h"
#include "zippath.h"


/*-------------------------------------------------
    dir_entry_type_string - gets a string
    representation of a osd_dir_entry_type
-------------------------------------------------*/

static const char *dir_entry_type_string(osd_dir_entry_type type)
{
	const char *s;
	switch(type)
	{
		case ENTTYPE_NONE:
			s = "none";
			break;
		case ENTTYPE_FILE:
			s = "file";
			break;
		case ENTTYPE_DIR:
			s = "directory";
			break;
		case ENTTYPE_OTHER:
			s = "other";
			break;
		default:
			s = "???";
			break;
	}
	return s;
}



/*-------------------------------------------------
    node_testzippath - test a zip path
-------------------------------------------------*/

void node_testzippath(xml_data_node *node)
{
	xml_attribute_node *attr_node;
	xml_data_node *first_child_node;
	xml_data_node *child_node;
	const char *path;
	const char *plus;
	astring *apath = NULL;
	zippath_directory *directory = NULL;
	const osd_directory_entry *dirent;
	const char *type_string;
	file_error err;
	UINT64 size;
	mess_pile pile;
	core_file *file = NULL;

	pile_init(&pile);

	/* name the test case */
	report_testcase_begin("zippath");

	/* retrieve path */
	attr_node = xml_get_attribute(node, "path");
	path = (attr_node != NULL) ? attr_node->value : "";

	/* retrieve 'plus' - for testing zippath_combine */
	attr_node = xml_get_attribute(node, "plus");
	if (attr_node != NULL)
	{
		plus = attr_node->value;
		apath = zippath_combine(astring_alloc(), path, plus);
		report_message(MSG_INFO, "Testing ZIP Path '%s' + '%s' ==> '%s'", path, plus, astring_c(apath));
	}
	else
	{
		apath = astring_cpyc(astring_alloc(), path);
		report_message(MSG_INFO, "Testing ZIP Path '%s'", astring_c(apath));
	}

	/* try doing a file compare */
	messtest_get_data(node, &pile);
	if (pile_size(&pile) > 0)
	{
		err = zippath_fopen(astring_c(apath), OPEN_FLAG_READ, &file, NULL);
		if (err != FILERR_NONE)
		{
			report_message(MSG_FAILURE, "Error %d opening file", (int) err);
			goto done;
		}

		if (pile_size(&pile) != core_fsize(file))
		{
			report_message(MSG_FAILURE, "Expected file to be of length %d, instead got %d", (int) pile_size(&pile), (int) core_fsize(file));
			goto done;
		}

		if (memcmp(pile_getptr(&pile), core_fbuffer(file), pile_size(&pile)))
		{
			report_message(MSG_FAILURE, "File sizes match, but contents do not");
			goto done;
		}
	}

	/* try doing a directory listing */
	first_child_node = xml_get_sibling(node->child, "entry");
	if (first_child_node != NULL)
	{
		err = zippath_opendir(astring_c(apath), &directory);
		if (err != FILERR_NONE)
		{
			report_message(MSG_FAILURE, "Error %d opening directory", (int) err);
			goto done;
		}

		/* read each directory entry */
		while((dirent = zippath_readdir(directory)) != NULL)
		{
			/* find it in the list */
			for (child_node = first_child_node; child_node != NULL; child_node = xml_get_sibling(child_node->next, "entry"))
			{
				attr_node = xml_get_attribute(child_node, "name");
				if ((attr_node != NULL) && !strcmp(attr_node->value, dirent->name))
					break;
			}

			/* did we find the node? */
			if (child_node != NULL)
			{
				/* check dirent type */
				attr_node = xml_get_attribute(child_node, "type");
				if (attr_node != NULL)
				{
					type_string = dir_entry_type_string(dirent->type);
					if (mame_stricmp(attr_node->value, type_string))
						report_message(MSG_FAILURE, "Expected '%s' to be '%s', but instead got '%s'", dirent->name, attr_node->value, type_string);
				}

				/* check size */
				attr_node = xml_get_attribute(child_node, "size");
				if (attr_node != NULL)
				{
					size = atoi(attr_node->value);
					if (size != dirent->size)
						report_message(MSG_FAILURE, "Expected '%s' to be of size '%ld', but instead got '%ld'", dirent->name, (long) size, (long) dirent->size);
				}
			}
			else
			{
				report_message(MSG_FAILURE, "Unexpected directory entry '%s'", dirent->name);
			}
		}
	}

done:
	pile_delete(&pile);
	if (apath != NULL)
		astring_free(apath);
	if (file != NULL)
		core_fclose(file);
	if (directory != NULL)
		zippath_closedir(directory);
}
