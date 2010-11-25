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

static void jupiter_machine_stop(running_machine &machine);


/* only gets called at the start of a cpu time slice */

DIRECT_UPDATE_HANDLER( jupiter_opbaseoverride )
{
	jupiter_state *state = machine->driver_data<jupiter_state>();
	UINT16 loop,tmpword;
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if (address == 0x059d)
	{
		if (state->data_type == JUPITER_ACE)
		{
			for (loop = 0; loop < 0x6000; loop++)
				space->write_byte(loop + 0x2000, state->data[loop]);
		}
		else if (state->data_type == JUPITER_TAP)
		{

			for (loop = 0; loop < state->tape.dat_len; loop++)
				space->write_byte(loop + state->tape.hdr_addr, state->data[loop]);

			space->write_byte(0x3c27, 0x01);

			for (loop = 0; loop < 8; loop++)
				space->write_byte(loop + 0x3c31, state->tape.hdr_vars[loop]);
			space->write_byte(0x3c39, 0x00);
			space->write_byte(0x3c3a, 0x00);

			tmpword = space->read_byte(0x3c3b) + space->read_byte(0x3c3c) * 256 + state->tape.hdr_len;

			space->write_byte(0x3c3b, tmpword & 0xff);
			space->write_byte(0x3c3c, (tmpword >> 8) & 0xff);

			space->write_byte(0x3c45, 0x0c);	/* ? */

			space->write_byte(0x3c4c, state->tape.hdr_3c4c);
			space->write_byte(0x3c4d, state->tape.hdr_3c4d);

			if (!space->read_byte(0x3c57) && !space->read_byte(0x3c58))
			{
				space->write_byte(0x3c57, 0x49);
				space->write_byte(0x3c58, 0x3c);
			}
		}
	}
	return ~1;
}

MACHINE_START( jupiter )
{
	jupiter_state *state = machine->driver_data<jupiter_state>();
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	state->data_type = JUPITER_NONE;
	logerror("jupiter_init\r\n");
	logerror("data: %p\n", state->data);

	if (state->data)
	{
		logerror("data: %p. type: %d.\n", state->data,	state->data_type);
		space->set_direct_update_handler(direct_update_delegate_create_static(jupiter_opbaseoverride, *machine));

	}

	machine->add_notifier(MACHINE_NOTIFY_EXIT, jupiter_machine_stop);
}

static void jupiter_machine_stop(running_machine &machine)
{
	jupiter_state *state = machine.driver_data<jupiter_state>();
	if (state->data)
	{
		free(state->data);
		state->data = NULL;
		state->data_type = JUPITER_NONE;
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
	jupiter_state *state = image.device().machine->driver_data<jupiter_state>();
	unsigned char jupiter_repeat, jupiter_byte, loop;
	int done, jupiter_index;

	if (state->data_type != JUPITER_NONE)
		return (0);

	done = 0;
	jupiter_index = 0;

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
		return (1);
	}

	logerror("Decoded %d bytes.\r\n", jupiter_index);
	state->data_type = JUPITER_ACE;

	logerror("data: %p\n", state->data);
	return (0);
}

DEVICE_IMAGE_LOAD( jupiter_tap )
{
	jupiter_state *state = image.device().machine->driver_data<jupiter_state>();
	UINT8 inpbyt;
	int loop;
	UINT16 hdr_len;

	if (state->data_type != JUPITER_NONE)
		return (0);

	logerror("Loading file %s.\r\n", image.filename());

    image.fread(&inpbyt, 1);
	hdr_len = inpbyt;
	image.fread(&inpbyt, 1);
	hdr_len += (inpbyt * 256);

	/* Read header block */

	image.fread(&state->tape.hdr_type, 1);
	image.fread(state->tape.hdr_name, 10);
	image.fread(&inpbyt, 1);
	state->tape.hdr_len = inpbyt;
	image.fread(&inpbyt, 1);
	state->tape.hdr_len += (inpbyt * 256);
	image.fread(&inpbyt, 1);
	state->tape.hdr_addr = inpbyt;
	image.fread(&inpbyt, 1);
	state->tape.hdr_addr += (inpbyt * 256);
	image.fread(&state->tape.hdr_3c4c, 1);
	image.fread(&state->tape.hdr_3c4d, 1);
	image.fread(state->tape.hdr_vars, 8);
	if (hdr_len > 0x19)
		for (loop = 0x19; loop < hdr_len; loop++)
			image.fread(&inpbyt, 1);

	/* Read data block */

	image.fread(&inpbyt, 1);
	state->tape.dat_len = inpbyt;
	image.fread(&inpbyt, 1);
	state->tape.dat_len += (inpbyt * 256);

	if ((state->data = (UINT8*)malloc(state->tape.dat_len)))
	{
		image.fread(state->data, state->tape.dat_len);
		state->data_type = JUPITER_TAP;
		logerror("File loaded\r\n");
	}

	if (!state->data)
	{
		logerror("file not loaded\r\n");
		return (1);
	}

	return (0);

}

DEVICE_IMAGE_UNLOAD( jupiter_tap )
{
	jupiter_state *state = image.device().machine->driver_data<jupiter_state>();
	logerror("jupiter_tap_unload\n");
	if (state->data)
	{
		free(state->data);
		state->data = NULL;
		state->data_type = JUPITER_NONE;
	}
}
