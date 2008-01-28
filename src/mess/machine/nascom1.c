/**********************************************************************

  mess/machine/nascom1.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrups,
  I/O ports)

**********************************************************************/

#include <stdio.h>
#include "driver.h"
#include "mslegacy.h"
#include "image.h"
#include "includes/nascom1.h"
#include "cpu/z80/z80.h"
#include "devices/snapquik.h"



static	int		nascom1_tape_size = 0;
static	UINT8	*nascom1_tape_image = NULL;
static	int		nascom1_tape_index = 0;


static	struct
{
	UINT8	stat_flags;
	UINT8	stat_count;
} nascom1_portstat;

#define NASCOM1_KEY_RESET	0x02
#define NASCOM1_KEY_INCR	0x01
#define NASCOM1_CAS_ENABLE	0x10


DRIVER_INIT( nascom1 )
{
	switch (mess_ram_size)
	{
	case 1 * 1024:
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x1400, 0x9000, 0, 0, MRA8_NOP, MWA8_NOP);
		break;

	case 16 * 1024:
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x1400, 0x4fff, 0, 0, MRA8_BANK1, MWA8_BANK1);
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x5000, 0xafff, 0, 0, MRA8_NOP, MWA8_NOP);
		memory_set_bankptr(1, mess_ram);
		break;

	case 32 * 1024:
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x1400, 0x8fff, 0, 0, MRA8_BANK1, MWA8_BANK1);
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x9000, 0xafff, 0, 0, MRA8_NOP, MWA8_NOP);
		memory_set_bankptr(1, mess_ram);
		break;

	case 40 * 1024:
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x1400, 0xafff, 0, 0, MRA8_BANK1, MWA8_BANK1);
		memory_set_bankptr(1, mess_ram);
		break;
	}
}


READ8_HANDLER ( nascom1_port_00_r )
{
	if (nascom1_portstat.stat_count < 9)
		return (readinputport (nascom1_portstat.stat_count) | ~0x7f);

	return (0xff);
}

READ8_HANDLER( nascom1_port_01_r )
{
	if (nascom1_portstat.stat_flags & NASCOM1_CAS_ENABLE)
		return (nascom1_read_cassette());

	return 0;
}

READ8_HANDLER( nascom1_port_02_r )
{
	return nascom1_portstat.stat_flags & NASCOM1_CAS_ENABLE ? 0x80 : 0x00;
}

WRITE8_HANDLER( nascom1_port_00_w )
{
	nascom1_portstat.stat_flags &= ~NASCOM1_CAS_ENABLE;
	nascom1_portstat.stat_flags |= (data & NASCOM1_CAS_ENABLE);

	if (!(data & NASCOM1_KEY_RESET)) {
		if (nascom1_portstat.stat_flags & NASCOM1_KEY_RESET)
			nascom1_portstat.stat_count = 0;
	} else nascom1_portstat.stat_flags = NASCOM1_KEY_RESET;

	if (!(data & NASCOM1_KEY_INCR)) {
		if (nascom1_portstat.stat_flags & NASCOM1_KEY_INCR)
			nascom1_portstat.stat_count++;
	} else nascom1_portstat.stat_flags = NASCOM1_KEY_INCR;
}

WRITE8_HANDLER (	nascom1_port_01_w )
{
}

DEVICE_LOAD( nascom1_cassette )
{
	nascom1_tape_size = image_length(image);
	nascom1_tape_image = image_ptr(image);
	if (!nascom1_tape_image)
		return INIT_FAIL;

	nascom1_tape_index = 0;
	return INIT_PASS;
}

DEVICE_UNLOAD( nascom1_cassette )
{
	nascom1_tape_image = NULL;
	nascom1_tape_size = nascom1_tape_index = 0;
}

int	nascom1_read_cassette(void)
{
	if (nascom1_tape_image && (nascom1_tape_index < nascom1_tape_size))
					return (nascom1_tape_image[nascom1_tape_index++]);
	return (0);
}

/* Ascii .nas format

   <addr> <byte> x 8 ^H^H^J
   .

   Note <addr> and <byte> are in hex.
*/

SNAPSHOT_LOAD( nascom1 )
{
	UINT8 line[35];

	while (image_fread(image, &line, sizeof(line)) == sizeof(line))
	{
		int addr, b0, b1, b2, b3, b4, b5, b6, b7, dummy;

		if (sscanf((char *)line, "%x %x %x %x %x %x %x %x %x %x\010\010\n",
			&addr, &b0, &b1, &b2, &b3, &b4, &b5, &b6, &b7, &dummy) == 10)
		{
			program_write_byte_8(addr++, b0);
			program_write_byte_8(addr++, b1);
			program_write_byte_8(addr++, b2);
			program_write_byte_8(addr++, b3);
			program_write_byte_8(addr++, b4);
			program_write_byte_8(addr++, b5);
			program_write_byte_8(addr++, b6);
			program_write_byte_8(addr++, b7);
		}
	}

	return INIT_PASS;
}
