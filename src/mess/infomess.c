/***************************************************************************

    infomess.c

    MESS-specific augmentation to info.c

***************************************************************************/

#include <ctype.h>

#include "driver.h"
#include "sound/samples.h"
#include "info.h"
#include "hash.h"

static int strwildcmp(const char *sp1, const char *sp2)
{
	char s1[9], s2[9];
	int i, l1, l2;
	char *p;

	strncpy(s1, sp1, 8); s1[8] = 0; if (s1[0] == 0) strcpy(s1, "*");

	strncpy(s2, sp2, 8); s2[8] = 0; if (s2[0] == 0) strcpy(s2, "*");

	p = strchr(s1, '*');
	if (p)
	{
		for (i = p - s1; i < 8; i++) s1[i] = '?';
		s1[8] = 0;
	}

	p = strchr(s2, '*');
	if (p)
	{
		for (i = p - s2; i < 8; i++) s2[i] = '?';
		s2[8] = 0;
	}

	l1 = strlen(s1);
	if (l1 < 8)
	{
		for (i = l1 + 1; i < 8; i++) s1[i] = ' ';
		s1[8] = 0;
	}

	l2 = strlen(s2);
	if (l2 < 8)
	{
		for (i = l2 + 1; i < 8; i++) s2[i] = ' ';
		s2[8] = 0;
	}

	for (i = 0; i < 8; i++)
	{
		if (s1[i] == '?' && s2[i] != '?') s1[i] = s2[i];
		if (s2[i] == '?' && s1[i] != '?') s2[i] = s1[i];
	}

	return mame_stricmp(s1, s2);
}



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

	count = ram_option_count(game);

	for (i = 0; i < count; i++)
	{
		ram = ram_option(game, i);
		fprintf(out, "\t\t<ramoption>%u</ramoption>\n", ram);
	}
}



/*************************************
 *
 *  Implementation of -listdevices
 *
 *************************************/

int frontend_listdevices(FILE *output)
{
	int i, dev, id;
	const struct IODevice *devices;
	const char *src;
	const char *driver_name;
	const char *name;
	const char *shortname;
	char paren_shortname[16];
	const char *gamename = "*";

	i = 0;

	fprintf(output, " SYSTEM      DEVICE NAME (brief)   IMAGE FILE EXTENSIONS SUPPORTED    \n");
	fprintf(output, "----------  --------------------  ------------------------------------\n");

	while (drivers[i])
	{
		if (!strwildcmp(gamename, drivers[i]->name))
		{
			begin_resource_tracking();
			devices = devices_allocate(drivers[i]);

			driver_name = drivers[i]->name;

			for (dev = 0; devices[dev].type < IO_COUNT; dev++)
			{
				src = devices[dev].file_extensions;

				for (id = 0; id < devices[dev].count; id++)
				{
					name = device_instancename(&devices[dev].devclass, id);
					shortname = device_briefinstancename(&devices[dev].devclass, id);

					sprintf(paren_shortname, "(%s)", shortname);

					fprintf(output, "%-13s%-12s%-8s   ", driver_name, name, paren_shortname);
					driver_name = " ";

					if (id == 0)
					{
						while (src && *src)
						{
							fprintf(output, ".%-5s", src);
							src += strlen(src) + 1;
						}
					}
					fprintf(output, "\n");
				}
			}
			end_resource_tracking();
		}
		i++;
	}
	return 0;
}



/*************************************
 *
 *  Implementation of -listtext
 *
 *************************************/

void print_mess_text(void)
{
}

