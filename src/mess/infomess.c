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
#include "softlist.h"


/*************************************
 *
 *  Code used by print_mame_xml()
 *
 *************************************/

/*-------------------------------------------------
    print_game_device - prints out all info on
    MESS-specific devices
-------------------------------------------------*/

static void print_game_device(FILE *out, const game_driver *game, const machine_config *config)
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

			if (info.interface_name[0] )
				fprintf(out, " interface=\"%s\"", xml_normalize_string(info.interface_name));

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
static void print_game_ramoptions(FILE *out, const game_driver *game, const machine_config *config)
{
	const device_config *device;

	for (device = ram_first(config); device != NULL; device = ram_next(device))
	{
		ram_config *ram = (ram_config *)device->inline_config;
		fprintf(out, "\t\t<ramoption default=\"1\">%u</ramoption>\n",  messram_parse_string(ram->default_size));
		if (ram->extra_options != NULL)
		{
			int j;
			int size = strlen(ram->extra_options);
			char *s = mame_strdup(ram->extra_options);
			for (j=0;j<size;j++) {
				if (s[j]==',') s[j]=0;
			}
			/* try to parse each option */
			while(*s != '\0')
			{
				fprintf(out, "\t\t<ramoption>%u</ramoption>\n",  messram_parse_string(s));
				s += strlen(s) + 1;
			}
		}
	}
}


/*-------------------------------------------------
    print_game_software_list - print the information
    for all known software lists for this system
-------------------------------------------------*/

static void print_game_software_list(FILE *out, const game_driver *game, const machine_config *config)
{
	for (const device_config *dev = config->devicelist.first(); dev != NULL; dev = dev->next)
	{
		if ( ! strcmp( dev->tag(), __SOFTWARE_LIST_TAG ) )
		{
			software_list_config *swlist = (software_list_config *)dev->inline_config;

			for ( int i = 0; i < DEVINFO_STR_SWLIST_MAX - DEVINFO_STR_SWLIST_0; i++ )
			{
				if ( swlist->list_name[i] )
				{
					fprintf(out, "\t\t<softwarelist name=\"%s\" />\n", swlist->list_name[i] );
				}
			}
		}
	}
}


/*-------------------------------------------------
    print_mess_game_xml - print MESS specific game
    information.
-------------------------------------------------*/

void print_mess_game_xml(FILE *out, const game_driver *game, const machine_config *config)
{
	print_game_device( out, game, config );
	print_game_ramoptions( out, game, config );
	print_game_software_list( out, game, config );
}

