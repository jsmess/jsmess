/**********************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrups,
  I/O ports)

**********************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/p2000t.h"
#include "sound/speaker.h"

#define P2000M_101F_CASDAT	0x01
#define P2000M_101F_CASCMD	0x02
#define P2000M_101F_CASREW	0x04
#define P2000M_101F_CASFOR	0x08
#define P2000M_101F_KEYINT	0x40
#define P2000M_101F_PRNOUT	0x80

#define	P2000M_202F_PINPUT	0x01
#define P2000M_202F_PREADY	0x02
#define	P2000M_202F_STRAPN	0x04
#define P2000M_202F_CASENB	0x08
#define P2000M_202F_CASPOS	0x10
#define P2000M_202F_CASEND	0x20
#define P2000M_202F_CASCLK	0x40
#define P2000M_202F_CASDAT	0x80

#define P2000M_303F_VIDEO	0x01

#define P2000M_707F_DISA	0x01

static struct
{
	UINT8 port_101f;
	UINT8 port_202f;
	UINT8 port_303f;
	UINT8 port_707f;
} p2000t_ports;

READ8_HANDLER (	p2000t_port_000f_r )
{
	if (p2000t_ports.port_101f & P2000M_101F_KEYINT)
	{
		return (input_port_read_indexed(machine, 0) & input_port_read_indexed(machine, 1) &
		input_port_read_indexed(machine, 2) & input_port_read_indexed(machine, 3) &
		input_port_read_indexed(machine, 4) & input_port_read_indexed(machine, 5) &
		input_port_read_indexed(machine, 6) & input_port_read_indexed(machine, 7) &
		input_port_read_indexed(machine, 8) & input_port_read_indexed(machine, 9));
	}
	else if (offset < 10)
	{
		return (input_port_read_indexed(machine, offset));
	}
	return (0xff);
}

READ8_HANDLER (	p2000t_port_202f_r ) { return (0xff); }

WRITE8_HANDLER ( p2000t_port_101f_w )
{
	p2000t_ports.port_101f = data;
}

WRITE8_HANDLER ( p2000t_port_303f_w )
{
	p2000t_ports.port_303f = data;
}

WRITE8_HANDLER ( p2000t_port_505f_w )
{
	speaker_level_w(0, data & 0x01);
}

WRITE8_HANDLER ( p2000t_port_707f_w )
{
	p2000t_ports.port_707f = data;
}

WRITE8_HANDLER ( p2000t_port_888b_w ) {}
WRITE8_HANDLER ( p2000t_port_8c90_w ) {}
WRITE8_HANDLER ( p2000t_port_9494_w ) {}

