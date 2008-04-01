/****************************************************************************

	cheatms.c

	MESS sepcific cheat code

****************************************************************************/


#include "mame.h"
#include "mess.h"
#include "image.h"
#include "cheatms.h"
#include "deprecat.h"


/***************************************************************************
    GLOBALS
***************************************************************************/

static UINT32				* deviceCRCList = NULL;
static INT32				deviceCRCListLength = 0;
UINT32				thisGameCRC = 0;



/***************************************************************************
    CODE
***************************************************************************/

static void BuildCRCTable(void)
{
	int	listIdx;
	const device_config *img;

	free(deviceCRCList);

	// allocate list with single member (0x00000000)
	deviceCRCList = calloc(1, sizeof(UINT32));
	deviceCRCListLength = 1;

	for (img = image_device_first(Machine->config); img != NULL; img = image_device_next(img))
	{
		if (image_exists(img))
		{
			UINT32	crc = image_crc(img);
			int		isUnique = 1;

			for(listIdx = 0; listIdx < deviceCRCListLength; listIdx++)
			{
				if(deviceCRCList[listIdx] == crc)
				{
					isUnique = 0;

					break;
				}
			}

			if(isUnique)
			{
				if(!thisGameCRC)
					thisGameCRC = crc;

				deviceCRCList = realloc(deviceCRCList, (deviceCRCListLength + 1) * sizeof(UINT32));

				deviceCRCList[deviceCRCListLength] = crc;
				deviceCRCListLength++;
			}
		}
	}
}


void InitMessCheats(void)
{
	deviceCRCList =			NULL;
	deviceCRCListLength =	0;
	thisGameCRC =			0;

	BuildCRCTable();
}



void StopMessCheats(void)
{
	free(deviceCRCList);
	deviceCRCList = NULL;

	deviceCRCListLength = 0;
	thisGameCRC = 0;
}



int MatchesCRCTable(UINT32 crc)
{
	int	i;

	for(i = 0; i < deviceCRCListLength; i++)
		if(deviceCRCList[i] == crc)
			return 1;

	return 0;
}
