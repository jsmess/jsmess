/**********************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrups,
  I/O ports)

**********************************************************************/

#include <stdio.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/aquarius.h"
#include "sound/speaker.h"

static	int	aquarius_ramsize = 0;

MACHINE_RESET( aquarius )
{
	logerror("aquarius_init\r\n");
	if (readinputport(9) != aquarius_ramsize)
	{
		aquarius_ramsize = readinputport(9);
		switch (aquarius_ramsize)
		{
			case 02:
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, 0, MWA8_RAM);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, 0, MRA8_RAM);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MWA8_RAM);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MRA8_RAM);
				break;
			case 01:
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, 0, MWA8_NOP);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, 0, MRA8_NOP);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MWA8_RAM);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MRA8_RAM);
				break;
			case 00:
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, 0, MWA8_NOP);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, 0, MRA8_NOP);
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MWA8_NOP);
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MRA8_NOP);
				break;
		}
	}
}

 READ8_HANDLER ( aquarius_port_ff_r )
{

	int loop, loop2, bc, rpl;

	bc = activecpu_get_reg(Z80_BC) >> 8;
	rpl = 0xff;

	for (loop = 0x80, loop2 = 7; loop; loop >>= 1, loop2--)
	{
		if (!(bc & loop))
		{
			rpl &= readinputport (loop2);
		}
	}

	return (rpl);
}

 READ8_HANDLER ( aquarius_port_fe_r )
{
	return (0xff);
}

WRITE8_HANDLER ( aquarius_port_fc_w )
{
	speaker_level_w(0, data & 0x01);
}

WRITE8_HANDLER ( aquarius_port_fe_w )
{
}

WRITE8_HANDLER ( aquarius_port_ff_w )
{
}

