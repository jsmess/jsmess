/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/jupiter.h"
#include "image.h"

#define	JUPITER_NONE	0
#define	JUPITER_ACE	1
#define	JUPITER_TAP	2

static struct
{
	UINT8 hdr_type;
	UINT8 hdr_name[10];
	UINT16 hdr_len;
	UINT16 hdr_addr;
	UINT8 hdr_vars[8];
	UINT8 hdr_3c4c;
	UINT8 hdr_3c4d;
	UINT16 dat_len;
}
jupiter_tape;

static UINT8 *jupiter_data = NULL;
static int jupiter_data_type = JUPITER_NONE;

static void jupiter_machine_stop(running_machine *machine);


/* only gets called at the start of a cpu time slice */

DIRECT_UPDATE_HANDLER( jupiter_opbaseoverride )
{
	UINT16 loop,tmpword;

	if (address == 0x059d)
	{
		if (jupiter_data_type == JUPITER_ACE)
		{
			for (loop = 0; loop < 0x6000; loop++)
				memory_write_byte(space, loop + 0x2000, jupiter_data[loop]);
		}
		else if (jupiter_data_type == JUPITER_TAP)
		{

			for (loop = 0; loop < jupiter_tape.dat_len; loop++)
				memory_write_byte(space, loop + jupiter_tape.hdr_addr, jupiter_data[loop]);

			memory_write_byte(space, 0x3c27, 0x01);

			for (loop = 0; loop < 8; loop++)
				memory_write_byte(space, loop + 0x3c31, jupiter_tape.hdr_vars[loop]);
			memory_write_byte(space, 0x3c39, 0x00);
			memory_write_byte(space, 0x3c3a, 0x00);

			tmpword = memory_read_byte(space, 0x3c3b) + memory_read_byte(space, 0x3c3c) * 256 + jupiter_tape.hdr_len;

			memory_write_byte(space, 0x3c3b, tmpword & 0xff);
			memory_write_byte(space, 0x3c3c, (tmpword >> 8) & 0xff);

			memory_write_byte(space, 0x3c45, 0x0c);	/* ? */

			memory_write_byte(space, 0x3c4c, jupiter_tape.hdr_3c4c);
			memory_write_byte(space, 0x3c4d, jupiter_tape.hdr_3c4d);

			if (!memory_read_byte(space, 0x3c57) && !memory_read_byte(space, 0x3c58))
			{
				memory_write_byte(space, 0x3c57, 0x49);
				memory_write_byte(space, 0x3c58, 0x3c);
			}
		}
	}
	return ~1;
}

MACHINE_START( jupiter )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	logerror("jupiter_init\r\n");
	logerror("data: %p\n", jupiter_data);

	if (jupiter_data)
	{
		logerror("data: %p. type: %d.\n", jupiter_data,	jupiter_data_type);
		memory_set_direct_update_handler(space, jupiter_opbaseoverride);
	}

	add_exit_callback(machine, jupiter_machine_stop);
}

static void jupiter_machine_stop(running_machine *machine)
{
	if (jupiter_data)
	{
		free(jupiter_data);
		jupiter_data = NULL;
		jupiter_data_type = JUPITER_NONE;
	}
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
	unsigned char jupiter_repeat, jupiter_byte, loop;
	int done, jupiter_index;

	if (jupiter_data_type != JUPITER_NONE)
		return (0);

	done = 0;
	jupiter_index = 0;

	if ((jupiter_data = (UINT8*)malloc(0x6000)))
	{
		logerror("Loading file %s.\r\n", image_filename(image));
		while (!done && (jupiter_index < 0x6001))
		{
			image_fread(image, &jupiter_byte, 1);
			if (jupiter_byte == 0xed)
			{
				image_fread(image, &jupiter_byte, 1);
				switch (jupiter_byte)
				{
				case 0x00:
					logerror("File loaded!\r\n");
					done = 1;
					break;
				case 0x01:
					image_fread(image, &jupiter_byte, 1);
					jupiter_data[jupiter_index++] = jupiter_byte;
					break;
				case 0x02:
					logerror("Sequence 0xED 0x02 found in .ace file\r\n");
					break;
				default:
					image_fread(image, &jupiter_repeat, 1);
					for (loop = 0; loop < jupiter_byte; loop++)
						jupiter_data[jupiter_index++] = jupiter_repeat;
					break;
				}
			}
			else
				jupiter_data[jupiter_index++] = jupiter_byte;
		}
	}
	if (!done)
	{
		logerror("file not loaded\r\n");
		return (1);
	}

	logerror("Decoded %d bytes.\r\n", jupiter_index);
	jupiter_data_type = JUPITER_ACE;

	logerror("data: %p\n", jupiter_data);
	return (0);
}

DEVICE_IMAGE_LOAD( jupiter_tap )
{
	UINT8 inpbyt;
	int loop;
	UINT16 hdr_len;

	if (jupiter_data_type != JUPITER_NONE)
		return (0);

	logerror("Loading file %s.\r\n", image_filename(image));

    image_fread(image, &inpbyt, 1);
	hdr_len = inpbyt;
	image_fread(image, &inpbyt, 1);
	hdr_len += (inpbyt * 256);

	/* Read header block */

	image_fread(image, &jupiter_tape.hdr_type, 1);
	image_fread(image, jupiter_tape.hdr_name, 10);
	image_fread(image, &inpbyt, 1);
	jupiter_tape.hdr_len = inpbyt;
	image_fread(image, &inpbyt, 1);
	jupiter_tape.hdr_len += (inpbyt * 256);
	image_fread(image, &inpbyt, 1);
	jupiter_tape.hdr_addr = inpbyt;
	image_fread(image, &inpbyt, 1);
	jupiter_tape.hdr_addr += (inpbyt * 256);
	image_fread(image, &jupiter_tape.hdr_3c4c, 1);
	image_fread(image, &jupiter_tape.hdr_3c4d, 1);
	image_fread(image, jupiter_tape.hdr_vars, 8);
	if (hdr_len > 0x19)
		for (loop = 0x19; loop < hdr_len; loop++)
			image_fread(image, &inpbyt, 1);

	/* Read data block */

	image_fread(image, &inpbyt, 1);
	jupiter_tape.dat_len = inpbyt;
	image_fread(image, &inpbyt, 1);
	jupiter_tape.dat_len += (inpbyt * 256);

	if ((jupiter_data = (UINT8*)malloc(jupiter_tape.dat_len)))
	{
		image_fread(image, jupiter_data, jupiter_tape.dat_len);
		jupiter_data_type = JUPITER_TAP;
		logerror("File loaded\r\n");
	}

	if (!jupiter_data)
	{
		logerror("file not loaded\r\n");
		return (1);
	}

	return (0);

}

DEVICE_IMAGE_UNLOAD( jupiter_tap )
{
	logerror("jupiter_tap_unload\n");
	if (jupiter_data)
	{
		free(jupiter_data);
		jupiter_data = NULL;
		jupiter_data_type = JUPITER_NONE;
	}
}
