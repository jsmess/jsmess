/***************************************************************************

    infomess.c

    MESS-specific augmentation to info.c

***************************************************************************/

#include <ctype.h>

#include "emu.h"
#include "sound/samples.h"
#include "info.h"
#include "hash.h"
#include "xmlfile.h"
#include "infomess.h"
#include "device.h"
#include "devices/messram.h"

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

	for (dev = config->devicelist.first(); dev != NULL; dev = dev->next)
	{
		if (is_image_device(dev))
		{
			info = image_device_getinfo(config, dev);

			/* print out device type */
			fprintf(out, "\t\t<device type=\"%s\"", xml_normalize_string(device_typename(info.type)));

			/* does this device have a tag? */
			if (dev->tag())
				fprintf(out, " tag=\"%s\"", xml_normalize_string(dev->tag()));

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
}

/* device iteration helpers */
#define ram_first(config)				(config)->devicelist.first(MESSRAM)
#define ram_next(previous)				((previous)->typenext())

/*-------------------------------------------------
    print_game_ramoptions - prints out all RAM
    options for this system
-------------------------------------------------*/
void print_game_ramoptions(FILE *out, const game_driver *game, const machine_config *config)
{
	const device_config *device;

	for (device = ram_first(config); device != NULL; device = ram_next(device))
	{
		ram_config *ram = (ram_config *)device->inline_config;
		fprintf(out, "\t\t<ramoption default=\"1\">%u</ramoption>\n",  messram_parse_string(ram->default_size));
		if (ram->extra_options != NULL)
		{
			const char *s;

			astring buffer;
			astring_cpyc(&buffer, ram->extra_options);
			astring_replacechr(&buffer, ',', 0);

			s = astring_c(&buffer);

			/* try to parse each option */
			while(*s != '\0')
			{
				fprintf(out, "\t\t<ramoption>%u</ramoption>\n",  messram_parse_string(s));
				s += strlen(s) + 1;
			}
		}
	}
}
