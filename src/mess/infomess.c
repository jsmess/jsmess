/***************************************************************************

    infomess.c

    MESS-specific augmentation to info.c

***************************************************************************/

#include <ctype.h>

#include "driver.h"
#include "sound/samples.h"
#include "info.h"
#include "hash.h"

/*************************************
 *
 *  Code used by print_mame_xml()
 *
 *************************************/

/* Print a free format string */
static const char *normalize_string(const char* s)
{
	static char buffer[1024];
	char *d = &buffer[0];

	if (s)
	{
		while (*s)
		{
			switch (*s)
			{
				case '\"' : d += sprintf(d, "&quot;"); break;
				case '&'  : d += sprintf(d, "&amp;"); break;
				case '<'  : d += sprintf(d, "&lt;"); break;
				case '>'  : d += sprintf(d, "&gt;"); break;
				default:
					if (*s>=' ' && *s<='~')
						*d++ = *s;
					else
						d += sprintf(d, "&#%d;", (unsigned)(unsigned char)*s);
			}
			++s;
		}
	}
	*d++ = 0;
	return buffer;
}



void print_game_device(FILE* out, const game_driver* game)
{
	const struct IODevice* dev;
	const char *name;
	const char *shortname;
	int id;

	begin_resource_tracking();

	dev = devices_allocate(game);
	if (dev)
	{
		while(dev->type < IO_COUNT)
		{
			/* print out device type */
			fprintf(out, "\t\t<device type=\"%s\"", normalize_string(device_typename(dev->type)));

			/* does this device have a tag? */
			if (dev->tag)
				fprintf(out, " tag=\"%s\"", normalize_string(dev->tag));

			/* is this device mandatory? */
			if (dev->must_be_loaded)
				fprintf(out, " mandatory=\"1\"");

			/* close the XML tag */
			fprintf(out, ">\n");

			for (id = 0; id < dev->count; id++)
			{
				name = device_instancename(&dev->devclass, id);
				shortname = device_briefinstancename(&dev->devclass, id);

				fprintf(out, "\t\t\t<instance");
				fprintf(out, " name=\"%s\"", normalize_string(name));
				fprintf(out, " briefname=\"%s\"", normalize_string(shortname));
				fprintf(out, "/>\n");
			}

			if (dev->file_extensions)
			{
				const char* ext = dev->file_extensions;
				while (*ext)
				{
					fprintf(out, "\t\t\t<extension");
					fprintf(out, " name=\"%s\"", normalize_string(ext));
					fprintf(out, "/>\n");
					ext += strlen(ext) + 1;
				}
			}

			fprintf(out, "\t\t</device>\n");

			dev++;
		}
	}
	end_resource_tracking();
}



void print_game_ramoptions(FILE* out, const game_driver* game)
{
	int i, count;
	UINT32 ram;
	UINT32 default_ram;
	
	count = ram_option_count(game);
	default_ram = ram_default(game);

	for (i = 0; i < count; i++)
	{
		ram = ram_option(game, i);
		if (ram == default_ram)
			fprintf(out, "\t\t<ramoption default=\"1\">%u</ramoption>\n", ram);
		else
			fprintf(out, "\t\t<ramoption>%u</ramoption>\n", ram);
	}
}



/*************************************
 *
 *  Implementation of -listtext
 *
 *************************************/

void print_mess_text(void)
{
}

