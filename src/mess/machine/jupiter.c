/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "emu.h"
#include "includes/jupiter.h"


static TIMER_CALLBACK( jupiter_begin )
{
	jupiter_state *state = machine->driver_data<jupiter_state>();
	UINT16 loop;
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	for (loop = 0; loop < 0x6000; loop++)
		space->write_byte(loop + 0x2000, state->data[loop]);

	free(state->data);
	state->data = NULL;
}


/* Load in .ace files. These are memory images of 0x2000 to 0x7fff
   and compressed as follows:

   ED 00        : End marker
   ED 01 ED     : 0xED
   ED <cnt> <byt>   : repeat <byt> count <cnt:3-240> times
   <byt>        : <byt>
*/

DEVICE_IMAGE_LOAD( jupiter_ace )
{
	jupiter_state *state = image.device().machine->driver_data<jupiter_state>();
	unsigned char jupiter_repeat, jupiter_byte, loop;
	int done=0, jupiter_index=0;

	if ((state->data = (UINT8*)malloc(0x6000)))
	{
		logerror("Loading file %s.\r\n", image.filename());
		while (!done && (jupiter_index < 0x6001))
		{
			image.fread( &jupiter_byte, 1);
			if (jupiter_byte == 0xed)
			{
				image.fread(&jupiter_byte, 1);
				switch (jupiter_byte)
				{
				case 0x00:
					logerror("File loaded!\r\n");
					done = 1;
					break;
				case 0x01:
					image.fread(&jupiter_byte, 1);
					state->data[jupiter_index++] = jupiter_byte;
					break;
				case 0x02:
					logerror("Sequence 0xED 0x02 found in .ace file\r\n");
					break;
				default:
					image.fread(&jupiter_repeat, 1);
					for (loop = 0; loop < jupiter_byte; loop++)
						state->data[jupiter_index++] = jupiter_repeat;
					break;
				}
			}
			else
				state->data[jupiter_index++] = jupiter_byte;
		}
	}

	if (!done)
	{
		logerror("file not loaded\r\n");
		if (state->data) free (state->data);
		state->data = NULL;
		return 1;
	}

	logerror("Decoded %d bytes.\r\n", jupiter_index);

	// wait for machine to start up, then load file into ram
	timer_set(image.device().machine, ATTOTIME_IN_SEC(1), NULL, 0, jupiter_begin);

	return 0;
}

