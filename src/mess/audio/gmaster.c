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
//#include "sound/mixer.h"
#include "cpu/upd7810/upd7810.h"

#include "includes/gmaster.h"

static /*bool*/int level;
static sound_stream *mixer_channel;

int gmaster_io_callback(int ioline, int state)
{
    switch (ioline) {
    case UPD7810_TO:
	stream_update(mixer_channel);
	level=state;
	break;
    default:
	logerror("io changed %d %.2x\n",ioline, state);
    }
    return 0;
}


/************************************/
/* Sound handler update             */
/************************************/
static void gmaster_update (void* param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
    int i;
    stream_sample_t *buffer=outputs[0];
    
    for (i = 0; i < samples; i++, buffer++)
    {
	*buffer = level?0x4000:0;
    }
}

/************************************/
/* Sound handler start              */
/************************************/

CUSTOM_START( gmaster_custom_start )
{
  mixer_channel = stream_create(device, 0, 1, device->machine->sample_rate, 0, gmaster_update);

  return (void*)~0;
}
