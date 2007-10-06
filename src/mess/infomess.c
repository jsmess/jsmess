/***************************************************************************

    infomess.c

    MESS-specific augmentation to info.c

***************************************************************************/

#include <ctype.h>

#include "driver.h"
#include "sound/samples.h"
#include "info.h"
#include "hash.h"
#include "xmlfile.h"

/*************************************
 *
 *  Code used by print_mame_xml()
 *
 *************************************/


void print_game_device(FILE* out, const game_driver* game)
{
	const struct IODevice* devices;
	const char *name;
	const char *shortname;
	int id, devindex;

	devices = devices_allocate(game);
	if (devices)
	{
		for (devindex = 0; devices[devindex].type < IO_COUNT; devindex++)
		{
			/* print out device type */
			fprintf(out, "\t\t<device type=\"%s\"", xml_normalize_string(device_typename(devices[devindex].type)));

			/* does this device have a tag? */
			if (devices[devindex].tag)
				fprintf(out, " tag=\"%s\"", xml_normalize_string(devices[devindex].tag));

			/* is this device mandatory? */
			if (devices[devindex].must_be_loaded)
				fprintf(out, " mandatory=\"1\"");

			/* close the XML tag */
			fprintf(out, ">\n");

			for (id = 0; id < devices[devindex].count; id++)
			{
				name = device_instancename(&devices[devindex].devclass, id);
				shortname = device_briefinstancename(&devices[devindex].devclass, id);

				fprintf(out, "\t\t\t<instance");
				fprintf(out, " name=\"%s\"", xml_normalize_string(name));
				fprintf(out, " briefname=\"%s\"", xml_normalize_string(shortname));
				fprintf(out, "/>\n");
			}

			if (devices[devindex].file_extensions)
			{
				const char* ext = devices[devindex].file_extensions;
				while (*ext)
				{
					fprintf(out, "\t\t\t<extension");
					fprintf(out, " name=\"%s\"", xml_normalize_string(ext));
					fprintf(out, "/>\n");
					ext += strlen(ext) + 1;
				}
			}

			fprintf(out, "\t\t</device>\n");
		}
		devices_free(devices);
	}
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
