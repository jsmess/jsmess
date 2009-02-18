/***************************************************************************
 PeT mess@utanet.at march 2002
***************************************************************************/

#include <assert.h>
#include <math.h>
#include "osd_cpu.h"
#include "driver.h"
#include "streams.h"
#include "mame.h"
#include "timer.h"
#include "cpu/upd7810/upd7810.h"

#include "includes/gmaster.h"

static /*bool*/int level;
static sound_stream *mixer_channel;



int gmaster_io_callback(const device_config *device, int ioline, int state)
{
    switch (ioline)
	{
		case UPD7810_TO:
			stream_update(mixer_channel);
			level = state;
			break;
		default:
			logerror("io changed %d %.2x\n",ioline, state);
			break;
    }
    return 0;
}


/************************************/
/* Sound handler update             */
/************************************/
static STREAM_UPDATE( gmaster_update )
{
	int i;
	stream_sample_t *buffer = outputs[0];

	for (i = 0; i < samples; i++, buffer++)
	{
		*buffer = level?0x4000:0;
	}
}

/************************************/
/* Sound handler start              */
/************************************/

static DEVICE_START( gmaster_sound )
{
	mixer_channel = stream_create(device, 0, 1, device->machine->sample_rate, 0, gmaster_update);
}


DEVICE_GET_INFO( gmaster_sound )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(gmaster_sound);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "GameMaster Sound");				break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
	}
}
