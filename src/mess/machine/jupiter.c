/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "emu.h"
#include "devices/snapquik.h"
#include "includes/jupiter.h"



/* Load in .ace files. These are memory images of 0x2000 to 0x7fff
   and compressed as follows:

   ED 00        : End marker
   ED 01 ED     : 0xED
   ED <cnt> <byt>   : repeat <byt> count <cnt:3-240> times
   <byt>        : <byt>
*/


/******************************************************************************
 Snapshot Handling
******************************************************************************/

SNAPSHOT_LOAD(jupiter)
{
	address_space *space = cputag_get_address_space(image.device().machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	unsigned char jupiter_repeat, jupiter_byte, loop;
	int done=0, jupiter_index=0x2000;

	if (input_port_read(space->machine, "CFG")==0)
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "At least 16KB RAM expansion required");
		image.message("At least 16KB RAM expansion required");
		return IMAGE_INIT_FAIL;
	}

	logerror("Loading file %s.\r\n", image.filename());
	while (!done && (jupiter_index < 0x8001))
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
					space->write_byte(jupiter_index++, jupiter_byte);
					break;
			case 0x02:
					logerror("Sequence 0xED 0x02 found in .ace file\r\n");
					break;
			default:
					image.fread(&jupiter_repeat, 1);
					for (loop = 0; loop < jupiter_byte; loop++)
						space->write_byte(jupiter_index++, jupiter_repeat);
					break;
			}
		}
		else
			space->write_byte(jupiter_index++, jupiter_byte);
	}

	logerror("Decoded %X bytes.\r\n", jupiter_index-0x2000);

	if (!done)
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "EOF marker not found");
		image.message("EOF marker not found");
		return IMAGE_INIT_FAIL;
	}

	return IMAGE_INIT_PASS;
}

