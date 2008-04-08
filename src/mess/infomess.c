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


/*-------------------------------------------------
    print_game_device - prints out all info on
	MESS-specific devices
-------------------------------------------------*/

void print_game_device(FILE *out, const game_driver *game, const machine_config *config)
{
	const device_config *dev;
	image_device_info info;
	const char *name;
	const char *shortname;
	const char *ext;

	for (dev = image_device_first(config); dev != NULL; dev = image_device_next(dev))
	{
		info = image_device_getinfo(config, dev);

		/* print out device type */
		fprintf(out, "\t\t<device type=\"%s\"", xml_normalize_string(device_typename(info.type)));

		/* does this device have a tag? */
		if (dev->tag)
			fprintf(out, " tag=\"%s\"", xml_normalize_string(dev->tag));

		/* is this device mandatory? */
		if (info.must_be_loaded)
			fprintf(out, " mandatory=\"1\"");

		/* close the XML tag */
		fprintf(out, ">\n");

		name = info.instance_name;
		shortname = info.brief_instance_name;

		fprintf(out, "\t\t\t<instance");
		fprintf(out, " name=\"%s\"", xml_normalize_string(name));
		fprintf(out, " briefname=\"%s\"", xml_normalize_string(shortname));
		fprintf(out, "/>\n");

		ext = info.file_extensions;
		while (*ext)
		{
			fprintf(out, "\t\t\t<extension");
			fprintf(out, " name=\"%s\"", xml_normalize_string(ext));
			fprintf(out, "/>\n");
			ext += strlen(ext) + 1;
		}

		fprintf(out, "\t\t</device>\n");
	}
}



/*-------------------------------------------------
    print_game_ramoptions - prints out all RAM
	options for this system
-------------------------------------------------*/

void print_game_ramoptions(FILE *out, const game_driver *game, const machine_config *config)
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
