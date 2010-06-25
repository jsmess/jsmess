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
#include "devices/messram.h"


/*************************************
 *
 *  Code used by print_mame_xml()
 *
 *************************************/

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
		ram_config *ram = (ram_config *)downcast<const legacy_device_config_base *>(device)->inline_config();
		fprintf(out, "\t\t<ramoption default=\"1\">%u</ramoption>\n",  messram_parse_string(ram->default_size));
		if (ram->extra_options != NULL)
		{
			int j;
			int size = strlen(ram->extra_options);
			char * const s = mame_strdup(ram->extra_options);
			char * const e = s + size;
			char *p = s;
			for (j=0;j<size;j++) {
				if (p[j]==',') p[j]=0;
			}
			/* try to parse each option */
			while(p <= e)
			{
				fprintf(out, "\t\t<ramoption>%u</ramoption>\n",  messram_parse_string(p));
				p += strlen(p);
				if (p == e)
					break;
				p += 1;
			}

			osd_free(s);
		}
	}
}


/*-------------------------------------------------
    print_mess_game_xml - print MESS specific game
    information.
-------------------------------------------------*/

void print_mess_game_xml(FILE *out, const game_driver *game, const machine_config *config)
{
	print_game_ramoptions( out, game, config );	
}

