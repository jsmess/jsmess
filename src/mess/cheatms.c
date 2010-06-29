/****************************************************************************

    cheatms.c

    MESS sepcific cheat code

****************************************************************************/


#include "emu.h"
#include "mess.h"
#include "image.h"
#include "cheatms.h"


/***************************************************************************
    GLOBALS
***************************************************************************/

static UINT32 *device_crc_list = NULL;
static INT32 device_crc_list_length = 0;
UINT32 this_game_crc = 0;



/***************************************************************************
    CODE
***************************************************************************/

static void build_crc_table(running_machine *machine)
{
	int	listIdx;
	device_image_interface *image = NULL;
	static UINT32 *tmp_list = NULL;

	free(device_crc_list);

	/* allocate list with single member (0x00000000) */
	device_crc_list = (UINT32*)malloc(sizeof(UINT32));
	memset(device_crc_list,0,sizeof(UINT32));
	device_crc_list_length = 1;

	for (bool gotone = machine->devicelist.first(image); gotone; gotone = image->next(image))
	{
		if (image->exists())
		{
			UINT32	crc = image->crc();
			int		isUnique = 1;

			for(listIdx = 0; listIdx < device_crc_list_length; listIdx++)
			{
				if(device_crc_list[listIdx] == crc)
				{
					isUnique = 0;

					break;
				}
			}

			if(isUnique)
			{
				if(!this_game_crc)
					this_game_crc = crc;

				tmp_list = (UINT32*)malloc((device_crc_list_length + 1) * sizeof(UINT32));
				memcpy(tmp_list,device_crc_list,device_crc_list_length * sizeof(UINT32));
				if (device_crc_list) free(device_crc_list);
				device_crc_list = tmp_list;

				device_crc_list[device_crc_list_length] = crc;
				device_crc_list_length++;
			}
		}
	}
}


void cheat_mess_init(running_machine *machine)
{
	device_crc_list = NULL;
	device_crc_list_length = 0;
	this_game_crc =	0;

	build_crc_table(machine);
}



void cheat_mess_exit(void)
{
	free(device_crc_list);
	device_crc_list = NULL;

	device_crc_list_length = 0;
	this_game_crc = 0;
}



int cheat_mess_matches_crc_table(UINT32 crc)
{
	int i;

	for(i = 0; i < device_crc_list_length; i++)
		if(device_crc_list[i] == crc)
			return 1;

	return 0;
}

