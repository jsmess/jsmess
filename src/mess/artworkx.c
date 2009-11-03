/*********************************************************************

	artworkx.c

	MESS specific artwork code

*********************************************************************/

#include <ctype.h>

#include "mame.h"
#include "driver.h"
#include "artworkx.h"
#include "image.h"
#include "png.h"


/***************************************************************************

	Local variables

***************************************************************************/

static char *override_artfile;



/**************************************************************************/

void artwork_use_device_art(const device_config *img, const char *defaultartfile)
{
	const char *strs[3];
	int len, pos, i;

	/* This function builds the override_artfile string.  This string is a
	 * list of NUL terminated strings.  These strings become the basename for
	 * the .art file that we use.
	 *
	 * We use the following strings:
	 * 1.  The basename
	 * 2.  The goodname
	 * 3.  The file specified by defaultartfile
	 */

	strs[0] = image_basename_noext(img);
	strs[1] = strs[0] ? image_longname(img) : NULL;
	strs[2] = defaultartfile;

	/* concatenate the strings; first calculate length of the string */
	len = 1;
	for (i = 0; i < ARRAY_LENGTH(strs); i++)
	{
		if (strs[i])
			len += strlen(strs[i]) + 1;
	}

	override_artfile = auto_alloc_array(img->machine, char, len);
	override_artfile[len - 1] = '\0';

	/* now actually concatenate the strings */
	pos = 0;
	for (i = 0; i < ARRAY_LENGTH(strs); i++)
	{
		if (strs[i])
		{
			strcpy(&override_artfile[pos], strs[i]);
			pos += strlen(strs[i]) + 1;
		}
	}

	/* small transformations */
	for (i = 0; i < len; i++)
	{
		if (override_artfile[i] == ':')
			override_artfile[i] = '-';
	}
}



/********************************************************************/

int artwork_get_inputscreen_customizations(png_info *png, artwork_cust_type cust_type,
	const char *section,
	struct inputform_customization *customizations,
	int customizations_length)
{
	file_error filerr;
	mame_file *file;
	char buffer[1000];
	char current_section[64];
	char ipt_name[64];
	char *p;
	int x1, y1, x2, y2;
	const char *png_filename;
	const char *ini_filename;
	int enabled = TRUE;
	int item_count = 0;

	static const char *const cust_files[] =
	{
		"ctrlr.png",		"ctrlr.ini",
		"keyboard.png",		"keyboard.ini"
		"misc.png",			"misc.ini"
	};

	if ((cust_type >= 0) && (cust_type < (ARRAY_LENGTH(cust_files) / 2)))
	{
		png_filename = cust_files[cust_type * 2 + 0];
		ini_filename = cust_files[cust_type * 2 + 1];
	}
	else
	{
		png_filename = NULL;
		ini_filename = NULL;
	}

	/* subtract one from the customizations length; so we can place IPT_END */
	customizations_length--;

	/* open the INI file, if available */
	if (ini_filename)
	{
		filerr = mame_fopen(SEARCHPATH_ARTWORK, ini_filename, OPEN_FLAG_READ, &file);
		if (filerr == FILERR_NONE)
		{
			/* loop until we run out of lines */
			while (customizations_length && mame_fgets(buffer, sizeof(buffer), file))
			{
				/* strip off any comments */
				p = strstr(buffer, "//");
				if (p)
					*p = 0;

				/* section header? */
				if (buffer[0] == '[')
				{
					strncpyz(current_section, &buffer[1],
						ARRAY_LENGTH(current_section));
					p = strchr(current_section, ']');
					if (!p)
						continue;
					*p = '\0';
					if (section)
						enabled = !mame_stricmp(current_section, section);
					continue;
				}

				if (!enabled || sscanf(buffer, "%64s (%d,%d)-(%d,%d)", ipt_name, &x1, &y1, &x2, &y2) != 5)
					continue;

#if 0
				/* temporarily disabled */
				for (pik = input_keywords; pik->name[0]; pik++)
				{
					pik_name = pik->name;
					if ((pik_name[0] == 'P') && (pik_name[1] == '1') && (pik_name[2] == '_'))
						pik_name += 3;

					if (!strcmp(ipt_name, pik_name))
					{
						if ((x1 > 0) && (y1 > 0) && (x2 > x1) && (y2 > y1))
						{
							customizations->ipt = pik->val;
							customizations->x = x1;
							customizations->y = y1;
							customizations->width = x2 - x1;
							customizations->height = y2 - y1;
							customizations++;
							customizations_length--;
							item_count++;
						}
						break;
					}
				}
#endif
			}
			mame_fclose(file);
		}
	}

	/* terminate list */
	customizations->ipt = IPT_END;
	customizations->x = -1;
	customizations->y = -1;
	customizations->width = -1;
	customizations->height = -1;

	/* open the PNG, if available */
	memset(png, 0, sizeof(*png));
	if (png_filename && item_count > 0)
	{
		filerr = mame_fopen(SEARCHPATH_ARTWORK, ini_filename, OPEN_FLAG_READ, &file);
		if (filerr == FILERR_NONE)
		{
			png_read_file(mame_core_file(file), png);
			mame_fclose(file);
		}
	}
	return item_count;
}
