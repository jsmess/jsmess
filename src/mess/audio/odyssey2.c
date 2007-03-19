/**************************************************************************************

  sndhrdw/odyssey2.c

  The VDC handles the audio in the Odyssey 2, so most of the code can be found
  in vidhrdw/odyssey2.c.

***************************************************************************************/
#include "driver.h"
#include "includes/odyssey2.h"

sound_stream *odyssey2_sh_channel;

struct CustomSound_interface odyssey2_sound_interface = 
{
	odyssey2_sh_start
};

void *odyssey2_sh_start(int clock, const struct CustomSound_interface *config)
{
	odyssey2_sh_channel = stream_create(0, 1, Machine->sample_rate, 0, odyssey2_sh_update );
	return (void *) ~0;
}
