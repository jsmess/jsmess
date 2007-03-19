/**********************************************************************

  mess/machine/nascom1.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrups,
  I/O ports)

**********************************************************************/

#include <stdio.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/nascom1.h"
#include "image.h"

static	int		nascom1_tape_size = 0;
static	UINT8	*nascom1_tape_image = NULL;
static	int		nascom1_tape_index = 0;

static	int	nascom1_ramsize = 3;

static	struct
{
	UINT8	stat_flags;
	UINT8	stat_count;
} nascom1_portstat;

#define NASCOM1_KEY_RESET	0x02
#define NASCOM1_KEY_INCR	0x01
#define NASCOM1_CAS_ENABLE	0x10

MACHINE_RESET( nascom1 )
{
	logerror("nascom1_init\r\n");
	if (readinputport(9) != nascom1_ramsize)
	{
		nascom1_ramsize = readinputport(9);
		switch (nascom1_ramsize)
		{
			case 03:
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xafff, 0, 0, MWA8_RAM);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xafff, 0, 0, MRA8_RAM);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x8fff, 0, 0, MWA8_RAM);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x8fff, 0, 0, MRA8_RAM);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1400, 0x4fff, 0, 0, MWA8_RAM);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1400, 0x4fff, 0, 0, MRA8_RAM);
				break;
			case 02:
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xafff, 0, 0, MWA8_NOP);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xafff, 0, 0, MRA8_NOP);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x8fff, 0, 0, MWA8_RAM);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x8fff, 0, 0, MRA8_RAM);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1400, 0x4fff, 0, 0, MWA8_RAM);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1400, 0x4fff, 0, 0, MRA8_RAM);
				break;
			case 01:
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xafff, 0, 0, MWA8_NOP);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xafff, 0, 0, MRA8_NOP);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x8fff, 0, 0, MWA8_NOP);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x8fff, 0, 0, MRA8_NOP);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1400, 0x4fff, 0, 0, MWA8_RAM);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1400, 0x4fff, 0, 0, MRA8_RAM);
				break;
			case 00:
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xafff, 0, 0, MWA8_NOP);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xafff, 0, 0, MRA8_NOP);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x8fff, 0, 0, MWA8_NOP);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x8fff, 0, 0, MRA8_NOP);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1400, 0x4fff, 0, 0, MWA8_NOP);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1400, 0x4fff, 0, 0, MRA8_NOP);
				break;
		}
	}
}

 READ8_HANDLER ( nascom1_port_00_r )
{
	if (nascom1_portstat.stat_count < 9)
		return (readinputport (nascom1_portstat.stat_count) | ~0x7f);

	return (0xff);
}

 READ8_HANDLER ( nascom1_port_01_r )
{
	if (nascom1_portstat.stat_flags & NASCOM1_CAS_ENABLE)
		return (nascom1_read_cassette());

	return (0);
}

 READ8_HANDLER ( nascom1_port_02_r )
{
	if (nascom1_portstat.stat_flags & NASCOM1_CAS_ENABLE) return (0x80);

	return (0x00);
}

WRITE8_HANDLER (	nascom1_port_00_w )
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

int	nascom1_init_cartridge(int id, mame_file *file)
{
	int		done;
	char	fileaddr[5];
	/* int	filebyt1, filebyt2, filebyt3, filebyt4;
	int	filebyt5, filebyt6, filebyt7, filebyt8;
	int	addr; */

	return (1);

	if (file)
	{
		done = 0;
		fileaddr[4] = 0;
		while (!done)
		{
			mame_fread(file, (void *)fileaddr, 4);
			logerror ("%4.4s\n", fileaddr);
			if (fileaddr[0] == '.')
			{
				done = 1;
			}
			else
			{
				/* vsscanf (fileaddr, "%4X", &addr); */
			    /* logerror ("%04X: %02X %02X %02X %02X %02X %02X %02X %02X\n",
							  addr, filebyt1, filebyt2, filebyt3, filebyt4,
									filebyt5, filebyt6, filebyt7, filebyt8); */

			}
		}
	}
	else
	{
		return (1);
	}

	return (0);
}
