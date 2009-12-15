/***************************************************************************
    commodore c64 home computer

    peter.trauner@jk.uni-linz.ac.at
    documentation
     www.funet.fi
***************************************************************************/

/*
    2008-09-06: Tape status for C64 & C128 [FP & RZ]
    - tape loading works
    - tap files are supported
    - tape writing works
*/

#include "driver.h"

#include "cpu/m6502/m6502.h"
#include "cpu/z80/z80.h"
#include "sound/sid6581.h"
#include "machine/6526cia.h"
#include "machine/cbmserial.h"
#include "video/vic6567.h"
#include "video/vdc8563.h"

#include "includes/cbm.h"
#include "includes/cbmdrive.h"
#include "includes/vc1541.h"

#include "includes/c64.h"

#include "devices/cassette.h"
#include "devices/cartslot.h"

#define VERBOSE_LEVEL 0
#define DBG_LOG( MACHINE, N, M, A ) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(MACHINE)), (char*) M ); \
			logerror A; \
		} \
	} while (0)

#define log_cart 0

/* expansion port lines input */
int c64_pal;
UINT8 c64_game = 1, c64_exrom = 1;

/* cpu port */
UINT8 *c64_vicaddr, *c128_vicaddr;
UINT8 *c64_memory;
UINT8 *c64_colorram;
UINT8 *c64_basic;
UINT8 *c64_kernal;
UINT8 *c64_chargen;
UINT8 *c64_roml = 0;
UINT8 *c64_romh = 0;
static UINT8 *c64_io_mirror = NULL;

static UINT8 c64_port_data;

static UINT8 *roml = 0, *romh = 0;
static int ultimax;
int c64_tape_on;
static int c64_cia1_on;
static int c64_io_enabled;
static int is_sx64;				// temporary workaround until we implement full vc1541 emulation for every c64 set
static UINT8 c64_cart_n_banks = 0;

static UINT8 serial_clock, serial_data, serial_atn;
static UINT8 vicirq;

static void c64_nmi( running_machine *machine )
{
	static int nmilevel = 0;
	const device_config *cia_1 = devtag_get_device(machine, "cia_1");
	int cia1irq = cia_get_irq(cia_1);

	if (nmilevel != (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq)	/* KEY_RESTORE */
	{
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq);

		nmilevel = (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq;
	}
}


/***********************************************

    CIA Interfaces

***********************************************/

/*
 *  CIA 0 - Port A keyboard line select
 *  CIA 0 - Port B keyboard line read
 *
 *  flag cassette read input, serial request in
 *  irq to irq connected
 *
 *  see machine/cbm.c
 */

static READ8_DEVICE_HANDLER( c64_cia0_port_a_r )
{
	UINT8 cia0portb = cia_get_output_b(devtag_get_device(device->machine, "cia_0"));

	return cbm_common_cia0_port_a_r(device, cia0portb);
}

static READ8_DEVICE_HANDLER( c64_cia0_port_b_r )
{
	UINT8 cia0porta = cia_get_output_a(devtag_get_device(device->machine, "cia_0"));

	return cbm_common_cia0_port_b_r(device, cia0porta);
}

static WRITE8_DEVICE_HANDLER( c64_cia0_port_b_w )
{
    vic2_lightpen_write(data & 0x10);
}

static void c64_irq( running_machine *machine, int level )
{
	static int old_level = 0;

	if (level != old_level)
	{
		DBG_LOG(machine, 3, "mos6510", ("irq %s\n", level ? "start" : "end"));
		cputag_set_input_line(machine, "maincpu", M6510_IRQ_LINE, level);
		old_level = level;
	}
}

static void c64_cia0_interrupt( const device_config *device, int level )
{
	c64_irq (device->machine, level || vicirq);
}

void c64_vic_interrupt( running_machine *machine, int level )
{
	const device_config *cia_0 = devtag_get_device(machine, "cia_0");
#if 1
	if (level != vicirq)
	{
		c64_irq (machine, level || cia_get_irq(cia_0));
		vicirq = level;
	}
#endif
}

const cia6526_interface c64_ntsc_cia0 =
{
	DEVCB_LINE(c64_cia0_interrupt),
	DEVCB_NULL,	/* pc_func */
	10, /* 1/10 second */

	{
		{ DEVCB_HANDLER(c64_cia0_port_a_r), DEVCB_NULL },
		{ DEVCB_HANDLER(c64_cia0_port_b_r), DEVCB_HANDLER(c64_cia0_port_b_w) }
	}
};

const cia6526_interface c64_pal_cia0 =
{
	DEVCB_LINE(c64_cia0_interrupt),
	DEVCB_NULL,	/* pc_func */
	10, /* 1/10 second */

	{
		{ DEVCB_HANDLER(c64_cia0_port_a_r), DEVCB_NULL },
		{ DEVCB_HANDLER(c64_cia0_port_b_r), DEVCB_HANDLER(c64_cia0_port_b_w) }
	}
};


/*
 * CIA 1 - Port A
 * bit 7 serial bus data input
 * bit 6 serial bus clock input
 * bit 5 serial bus data output
 * bit 4 serial bus clock output
 * bit 3 serial bus atn output
 * bit 2 rs232 data output
 * bits 1-0 vic-chip system memory bank select
 *
 * CIA 1 - Port B
 * bit 7 user rs232 data set ready
 * bit 6 user rs232 clear to send
 * bit 5 user
 * bit 4 user rs232 carrier detect
 * bit 3 user rs232 ring indicator
 * bit 2 user rs232 data terminal ready
 * bit 1 user rs232 request to send
 * bit 0 user rs232 received data
 *
 * flag restore key or rs232 received data input
 * irq to nmi connected ?
 */
static READ8_DEVICE_HANDLER( c64_cia1_port_a_r )
{
	UINT8 value = 0xff;
	const device_config *serbus = devtag_get_device(device->machine, "serial_bus");

	if (!cbmserial_clk_r(serbus))
		value &= ~0x40;

	if (!cbmserial_data_r(serbus))
		value &= ~0x80;

	return value;
}

static WRITE8_DEVICE_HANDLER( c64_cia1_port_a_w )
{
	static const int helper[4] = {0xc000, 0x8000, 0x4000, 0x0000};
	const device_config *serbus = devtag_get_device(device->machine, "serial_bus");

	cbmserial_clk_w(serbus, device, !(data & 0x10));
	cbmserial_data_w(serbus, device, !(data & 0x20));
	cbmserial_atn_w(serbus, device, !(data & 0x08));
	c64_vicaddr = c64_memory + helper[data & 0x03];
}

static void c64_cia1_interrupt( const device_config *device, int level )
{
	c64_nmi(device->machine);
}

const cia6526_interface c64_ntsc_cia1 =
{
	DEVCB_LINE(c64_cia1_interrupt),
	DEVCB_NULL,	/* pc_func */
	10, /* 1/10 second */

	{
		{ DEVCB_HANDLER(c64_cia1_port_a_r), DEVCB_HANDLER(c64_cia1_port_a_w) },
		{ DEVCB_NULL, DEVCB_NULL }
	}
};

const cia6526_interface c64_pal_cia1 =
{
	DEVCB_LINE(c64_cia1_interrupt),
	DEVCB_NULL,	/* pc_func */
	10, /* 1/10 second */

	{
		{ DEVCB_HANDLER(c64_cia1_port_a_r), DEVCB_HANDLER(c64_cia1_port_a_w) },
		{ DEVCB_NULL, DEVCB_NULL }
	}
};

/***********************************************

    Memory Handlers

***********************************************/

static UINT8 *c64_io_ram_w_ptr;
static UINT8 *c64_io_ram_r_ptr;

WRITE8_HANDLER( c64_write_io )
{
	const device_config *cia_0 = devtag_get_device(space->machine, "cia_0");
	const device_config *cia_1 = devtag_get_device(space->machine, "cia_1");
	const device_config *sid = devtag_get_device(space->machine, "sid6581");

	c64_io_mirror[offset] = data;
	if (offset < 0x400)
		vic2_port_w(space, offset & 0x3ff, data);
	else if (offset < 0x800)
		sid6581_w(sid, offset & 0x3ff, data);
	else if (offset < 0xc00)
		c64_colorram[offset & 0x3ff] = data | 0xf0;
	else if (offset < 0xd00)
		cia_w(cia_0, offset, data);
	else if (offset < 0xe00)
	{
		if (c64_cia1_on)
			cia_w(cia_1, offset, data);
		else
			DBG_LOG(space->machine, 1, "io write", ("%.3x %.2x\n", offset, data));
	}
	else if (offset < 0xf00)
		DBG_LOG(space->machine, 1, "io write", ("%.3x %.2x\n", offset, data)); 		/* i/o 1 */
	else
		DBG_LOG(space->machine, 1, "io write", ("%.3x %.2x\n", offset, data));		/* i/o 2 */
}

WRITE8_HANDLER( c64_ioarea_w )
{
	if (c64_io_enabled)
		c64_write_io(space, offset, data);
	else
		c64_io_ram_w_ptr[offset] = data;
}

READ8_HANDLER( c64_read_io )
{
	const device_config *cia_0 = devtag_get_device(space->machine, "cia_0");
	const device_config *cia_1 = devtag_get_device(space->machine, "cia_1");
	const device_config *sid = devtag_get_device(space->machine, "sid6581");

	if (offset < 0x400)
		return vic2_port_r(space, offset & 0x3ff);

	else if (offset < 0x800)
		return sid6581_r(sid, offset & 0x3ff);

	else if (offset < 0xc00)
		return c64_colorram[offset & 0x3ff];

	else if (offset == 0xc00)
		{
			cia_set_port_mask_value(cia_0, 0, input_port_read(space->machine, "CTRLSEL") & 0x80 ? c64_keyline[8] : c64_keyline[9] );
			return cia_r(cia_0, offset);
		}

	else if (offset == 0xc01)
		{
			cia_set_port_mask_value(cia_0, 1, input_port_read(space->machine, "CTRLSEL") & 0x80 ? c64_keyline[9] : c64_keyline[8] );
			return cia_r(cia_0, offset);
		}

	else if (offset < 0xd00)
		return cia_r(cia_0, offset);

	else if (c64_cia1_on && (offset < 0xe00))
		return cia_r(cia_1, offset);

	DBG_LOG(space->machine, 1, "io read", ("%.3x\n", offset));

	return 0xff;
}

READ8_HANDLER( c64_ioarea_r )
{
	return c64_io_enabled ? c64_read_io(space, offset) : c64_io_ram_r_ptr[offset];
}


/* Info from http://unusedino.de/ec64/technical/misc/c64/64doc.html */
/*

  The leftmost column of the table contains addresses in hexadecimal
notation. The columns aside it introduce all possible memory
configurations. The default mode is on the left, and the absolutely
most rarely used Ultimax game console configuration is on the right.
(Has anybody ever seen any Ultimax games?) Each memory configuration
column has one or more four-digit binary numbers as a title. The bits,
from left to right, represent the state of the -LORAM, -HIRAM, -GAME
and -EXROM lines, respectively. The bits whose state does not matter
are marked with "x". For instance, when the Ultimax video game
configuration is active (the -GAME line is shorted to ground), the
-LORAM and -HIRAM lines have no effect.

      default                      001x                       Ultimax
       1111   101x   1000   011x   00x0   1110   0100   1100   xx01
10000
----------------------------------------------------------------------
 F000
       Kernal RAM    RAM    Kernal RAM    Kernal Kernal Kernal ROMH(*
 E000
----------------------------------------------------------------------
 D000  IO/C   IO/C   IO/RAM IO/C   RAM    IO/C   IO/C   IO/C   I/O
----------------------------------------------------------------------
 C000  RAM    RAM    RAM    RAM    RAM    RAM    RAM    RAM     -
----------------------------------------------------------------------
 B000
       BASIC  RAM    RAM    RAM    RAM    BASIC  ROMH   ROMH    -
 A000
----------------------------------------------------------------------
 9000
       RAM    RAM    RAM    RAM    RAM    ROML   RAM    ROML   ROML(*
 8000
----------------------------------------------------------------------
 7000

 6000
       RAM    RAM    RAM    RAM    RAM    RAM    RAM    RAM     -
 5000

 4000
----------------------------------------------------------------------
 3000

 2000  RAM    RAM    RAM    RAM    RAM    RAM    RAM    RAM     -

 1000
----------------------------------------------------------------------
 0000  RAM    RAM    RAM    RAM    RAM    RAM    RAM    RAM    RAM
----------------------------------------------------------------------

   *) Internal memory does not respond to write accesses to these
       areas.


    Legend: Kernal      E000-FFFF       Kernal ROM.

            IO/C        D000-DFFF       I/O address space or Character
                                        generator ROM, selected by
                                        -CHAREN. If the CHAREN bit is
                                        clear, the character generator
                                        ROM will be selected. If it is
                                        set, the I/O chips are
                                        accessible.

            IO/RAM      D000-DFFF       I/O address space or RAM,
                                        selected by -CHAREN. If the
                                        CHAREN bit is clear, the
                                        character generator ROM will
                                        be selected. If it is set, the
                                        internal RAM is accessible.

            I/O         D000-DFFF       I/O address space.
                                        The -CHAREN line has no effect.

            BASIC       A000-BFFF       BASIC ROM.

            ROMH        A000-BFFF or    External ROM with the -ROMH line
                        E000-FFFF       connected to its -CS line.

            ROML        8000-9FFF       External ROM with the -ROML line
                                        connected to its -CS line.

            RAM         various ranges  Commodore 64's internal RAM.

            -           1000-7FFF and   Open address space.
                        A000-CFFF       The Commodore 64's memory chips
                                        do not detect any memory accesses
                                        to this area except the VIC-II's
                                        DMA and memory refreshes.

    NOTE:   Whenever the processor tries to write to any ROM area
            (Kernal, BASIC, CHAROM, ROML, ROMH), the data will get
            "through the ROM" to the C64's internal RAM.

            For this reason, you can easily copy data from ROM to RAM,
            without any bank switching. But implementing external
            memory expansions without DMA is very hard, as you have to
            use a 256 byte window on the I/O1 or I/O2 area, like
            GEORAM, or the Ultimax memory configuration, if you do not
            want the data to be written both to internal and external
            RAM.

            However, this is not true for the Ultimax video game
            configuration. In that mode, the internal RAM ignores all
            memory accesses outside the area $0000-$0FFF, unless they
            are performed by the VIC, and you can write to external
            memory at $1000-$CFFF and $E000-$FFFF, if any, without
            changing the contents of the internal RAM.

*/

static void c64_bankswitch( running_machine *machine, int reset )
{
	static int old = -1, exrom, game;
	int loram, hiram, charen;
	int ultimax_mode = 0;
	int data = (UINT8) devtag_get_info_int(machine, "maincpu", CPUINFO_INT_M6510_PORT) & 0x07;

	/* If nothing has changed or reset = 0, don't do anything */
	if ((data == old) && (exrom == c64_exrom) && (game == c64_game) && !reset)
		return;

	/* Are we in Ultimax mode? */
	if (!c64_game && c64_exrom)
		ultimax_mode = 1;

	DBG_LOG(machine, 1, "bankswitch", ("%d\n", data & 7));
	loram  = (data & 1) ? 1 : 0;
	hiram  = (data & 2) ? 1 : 0;
	charen = (data & 4) ? 1 : 0;
	logerror("Bankswitch mode || charen, ultimax\n");
	logerror("%d, %d, %d, %d  ||   %d,      %d  \n", loram, hiram, c64_game, c64_exrom, charen, ultimax_mode);

	if (ultimax_mode)
	{
			c64_io_enabled = 1;		// charen has no effect in ultimax_mode

			memory_set_bankptr(machine, "bank1", roml);
			memory_set_bankptr(machine, "bank2", c64_memory + 0x8000);
			memory_set_bankptr(machine, "bank3", c64_memory + 0xa000);
			memory_set_bankptr(machine, "bank4", romh);
			memory_nop_write(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xe000, 0xffff, 0, 0);
	}
	else
	{
		/* 0x8000-0x9000 */
		if (loram && hiram && !c64_exrom)
		{
			memory_set_bankptr(machine, "bank1", roml);
			memory_set_bankptr(machine, "bank2", c64_memory + 0x8000);
		}
		else
		{
			memory_set_bankptr(machine, "bank1", c64_memory + 0x8000);
			memory_set_bankptr(machine, "bank2", c64_memory + 0x8000);
		}

		/* 0xa000 */
		if (hiram && !c64_game && !c64_exrom)
			memory_set_bankptr(machine, "bank3", romh);

		else if (loram && hiram && c64_game)
			memory_set_bankptr(machine, "bank3", c64_basic);

		else
			memory_set_bankptr(machine, "bank3", c64_memory + 0xa000);

		/* 0xd000 */
		// RAM
		if (!loram && !hiram && (c64_game || !c64_exrom))
		{
			c64_io_enabled = 0;
			c64_io_ram_r_ptr = c64_memory + 0xd000;
			c64_io_ram_w_ptr = c64_memory + 0xd000;
		}
		// IO/RAM
		else if (loram && !hiram && !c64_game)	// remember we cannot be in ultimax_mode, no need of !c64_exrom
		{
			c64_io_enabled = 1;
			c64_io_ram_r_ptr = (!charen) ? c64_chargen : c64_memory + 0xd000;
			c64_io_ram_w_ptr = c64_memory + 0xd000;
		}
		// IO/C
		else
		{
			c64_io_enabled = charen ? 1 : 0;

			if (!charen)
			{
			c64_io_ram_r_ptr = c64_chargen;
			c64_io_ram_w_ptr = c64_memory + 0xd000;
			}
		}

		/* 0xe000-0xf000 */
		memory_set_bankptr(machine, "bank4", hiram ? c64_kernal : c64_memory + 0xe000);
		memory_set_bankptr(machine, "bank5", c64_memory + 0xe000);
	}

	/* make sure the opbase function gets called each time */
	/* NPW 15-May-2008 - Another hack in the C64 drivers broken! */
	/* opbase->mem_max = 0xcfff; */

	game = c64_game;
	exrom = c64_exrom;
	old = data;
}

/**
  ddr bit 1 port line is output
  port bit 1 port line is high

  p0 output loram
  p1 output hiram
  p2 output charen
  p3 output cassette data
  p4 input cassette switch
  p5 output cassette motor
  p6,7 not available on M6510
 */

static emu_timer *datasette_timer;

void c64_m6510_port_write( const device_config *device, UINT8 direction, UINT8 data )
{
	/* if line is marked as input then keep current value */
	data = (c64_port_data & ~direction) | (data & direction);

	/* resistors make P0,P1,P2 go high when respective line is changed to input */
	if (!(direction & 0x04))
		data |= 0x04;

	if (!(direction & 0x02))
		data |= 0x02;

	if (!(direction & 0x01))
		data |= 0x01;

	c64_port_data = data;

	if (c64_tape_on)
	{
		if (direction & 0x08)
		{
			cassette_output(devtag_get_device(device->machine, "cassette"), (data & 0x08) ? -(0x5a9e >> 1) : +(0x5a9e >> 1));
		}

		if (direction & 0x20)
		{
			if(!(data & 0x20))
			{
				cassette_change_state(devtag_get_device(device->machine, "cassette"), CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
				timer_adjust_periodic(datasette_timer, attotime_zero, 0, ATTOTIME_IN_HZ(44100));
			}
			else
			{
				cassette_change_state(devtag_get_device(device->machine, "cassette"), CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
				timer_reset(datasette_timer, attotime_never);
			}
		}
	}

	if (!ultimax)
		c64_bankswitch(device->machine, 0);

	c64_memory[0x000] = memory_read_byte(cpu_get_address_space(device, ADDRESS_SPACE_PROGRAM), 0);
	c64_memory[0x001] = memory_read_byte(cpu_get_address_space(device, ADDRESS_SPACE_PROGRAM), 1);
}

UINT8 c64_m6510_port_read( const device_config *device, UINT8 direction )
{
	UINT8 data = c64_port_data;

	if (c64_tape_on)
	{
		if ((cassette_get_state(devtag_get_device(device->machine, "cassette")) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
			data &= ~0x10;
		else
			data |=  0x10;
	}

	return data;
}


int c64_paddle_read( const device_config *device, int which )
{
	running_machine *machine = device->machine;
	int pot1 = 0xff, pot2 = 0xff, pot3 = 0xff, pot4 = 0xff, temp;
	UINT8 cia0porta = cia_get_output_a(devtag_get_device(machine, "cia_0"));
	int controller1 = input_port_read(machine, "CTRLSEL") & 0x07;
	int controller2 = input_port_read(machine, "CTRLSEL") & 0x70;

	/* Notice that only a single input is defined for Mouse & Lightpen in both ports */
	switch (controller1)
	{
		case 0x01:
			if (which)
				pot2 = input_port_read(machine, "PADDLE2");
			else
				pot1 = input_port_read(machine, "PADDLE1");
			break;

		case 0x02:
			if (which)
				pot2 = input_port_read(machine, "TRACKY");
			else
				pot1 = input_port_read(machine, "TRACKX");
			break;

		case 0x03:
			if (which && (input_port_read(machine, "JOY1_2B") & 0x20))	/* Joy1 Button 2 */
				pot1 = 0x00;
			break;

		case 0x04:
			if (which)
				pot2 = input_port_read(machine, "LIGHTY");
			else
				pot1 = input_port_read(machine, "LIGHTX");
			break;

		case 0x00:
		case 0x07:
			break;

		default:
			logerror("Invalid Controller Setting %d\n", controller1);
			break;
	}

	switch (controller2)
	{
		case 0x10:
			if (which)
				pot4 = input_port_read(machine, "PADDLE4");
			else
				pot3 = input_port_read(machine, "PADDLE3");
			break;

		case 0x20:
			if (which)
				pot4 = input_port_read(machine, "TRACKY");
			else
				pot3 = input_port_read(machine, "TRACKX");
			break;

		case 0x30:
			if (which && (input_port_read(machine, "JOY2_2B") & 0x20))	/* Joy2 Button 2 */
				pot4 = 0x00;
			break;

		case 0x40:
			if (which)
				pot4 = input_port_read(machine, "LIGHTY");
			else
				pot3 = input_port_read(machine, "LIGHTX");
			break;

		case 0x00:
		case 0x70:
			break;

		default:
			logerror("Invalid Controller Setting %d\n", controller1);
			break;
	}

	if (input_port_read(machine, "CTRLSEL") & 0x80)		/* Swap */
	{
		temp = pot1; pot1 = pot2; pot2 = temp;
		temp = pot3; pot3 = pot4; pot4 = temp;
	}

	switch (cia0porta & 0xc0)
	{
	case 0x40:
		return which ? pot2 : pot1;

	case 0x80:
		return which ? pot4 : pot3;

	default:
		return 0;
	}
}

READ8_HANDLER( c64_colorram_read )
{
	return c64_colorram[offset & 0x3ff];
}

WRITE8_HANDLER( c64_colorram_write )
{
	c64_colorram[offset & 0x3ff] = data | 0xf0;
}

/*
 * only 14 address lines
 * a15 and a14 portlines
 * 0x1000-0x1fff, 0x9000-0x9fff char rom
 */
static int c64_dma_read( running_machine *machine, int offset )
{
	if (!c64_game && c64_exrom)
	{
		if (offset < 0x3000)
			return c64_memory[offset];

		return c64_romh[offset & 0x1fff];
	}

	if (((c64_vicaddr - c64_memory + offset) & 0x7000) == 0x1000)
		return c64_chargen[offset & 0xfff];

	return c64_vicaddr[offset];
}

static int c64_dma_read_ultimax( running_machine *machine, int offset )
{
	if (offset < 0x3000)
		return c64_memory[offset];

	return c64_romh[offset & 0x1fff];
}

static int c64_dma_read_color( running_machine *machine, int offset )
{
	return c64_colorram[offset & 0x3ff] & 0xf;
}

static double last = 0;

TIMER_CALLBACK( c64_tape_timer )
{
	double tmp = cassette_input(devtag_get_device(machine, "cassette"));
	const device_config *cia_0 = devtag_get_device(machine, "cia_0");

	if((last > +0.0) && (tmp < +0.0))
		cia_issue_index(cia_0);

	last = tmp;
}

static void c64_common_driver_init( running_machine *machine )
{
	if (!ultimax)
	{
		UINT8 *mem = memory_region(machine, "maincpu");
		c64_basic    = mem + 0x10000;
		c64_kernal   = mem + 0x12000;
		c64_chargen  = mem + 0x14000;
		c64_colorram = mem + 0x15000;
		c64_roml     = mem + 0x15400;
		c64_romh     = mem + 0x17400;
	}

	if (c64_tape_on)
		datasette_timer = timer_alloc(machine, c64_tape_timer, NULL);

	if (ultimax)
		vic6567_init(0, c64_pal, c64_dma_read_ultimax, c64_dma_read_color, c64_vic_interrupt);
	else
		vic6567_init(0, c64_pal, c64_dma_read, c64_dma_read_color, c64_vic_interrupt);

	// "cyberload" tape loader check the e000-ffff ram; the init ram need to return different value
	{
		int i;
		for (i = 0; i < 0x2000; i += 0x40)
			memset(c64_memory + (0xe000 + i), ((i & 0x40) >> 6) * 0xff, 0x40);
	}
}

DRIVER_INIT( c64 )
{
	ultimax = 0;
	is_sx64 = 0;
	c64_pal = 0;
	c64_cia1_on = 1;
	c64_tape_on = 1;
	c64_common_driver_init(machine);
}

DRIVER_INIT( c64pal )
{
	ultimax = 0;
	is_sx64 = 0;
	c64_pal = 1;
	c64_cia1_on = 1;
	c64_tape_on = 1;
	c64_common_driver_init(machine);
}

DRIVER_INIT( ultimax )
{
	ultimax = 1;
	is_sx64 = 0;
	c64_pal = 0;
	c64_cia1_on = 0;
	c64_tape_on = 1;
	c64_common_driver_init(machine);
}

DRIVER_INIT( c64gs )
{
	ultimax = 0;
	is_sx64 = 0;
	c64_pal = 1;
	c64_cia1_on = 1;
	c64_tape_on = 0;
	c64_common_driver_init(machine);
}

DRIVER_INIT( sx64 )
{
	ultimax = 0;
	is_sx64 = 1;
	c64_pal = 1;
	c64_cia1_on = 1;
	c64_tape_on = 0;
	c64_common_driver_init(machine);
	cbm_drive_config(machine, type_1541, 0, 0, "cpu_vc1540", 8);
}

MACHINE_START( c64 )
{
	c64_port_data = 0x17;

	c64_io_mirror = auto_alloc_array(machine, UINT8, 0x1000);
	c64_io_enabled = 0;

	if (is_sx64)
	{
		cbm_drive_reset(machine);
	}
	else if (c64_cia1_on)
	{
		cbm_drive_0_config(SERIAL, 8);
		cbm_drive_1_config(SERIAL, 9);
		serial_clock = serial_data = serial_atn = 1;
	}

	c64_vicaddr = c64_memory;
	vicirq = 0;

	if (!ultimax)
		c64_bankswitch(machine, 1);
}

INTERRUPT_GEN( c64_frame_interrupt )
{
	c64_nmi(device->machine);
	cbm_common_interrupt(device);
}


/***********************************************

    C64 Cartridges

***********************************************/

/* Info based on http://ist.uwaterloo.ca/~schepers/formats/CRT.TXT      */
/* Please refer to the webpage for the latest version and for a very
   complete listing of various cart types and their bankswitch tricks   */
/*
  Cartridge files were introduced in the CCS64  emulator,  written  by  Per
Hakan Sundell, and use the ".CRT" file extension. This format  was  created
to handle the various ROM cartridges that exist, such as Action Replay, the
Power cartridge, and the Final Cartridge.

  Normal game cartridges can load  into  several  different  memory  ranges
($8000-9FFF,  $A000-BFFF  or  $E000-FFFF).  Newer   utility   and   freezer
cartridges were less intrusive, hiding themselves until  called  upon,  and
still others used bank-switching techniques to allow much larger ROM's than
normal. Because of these "stealthing" and bank-switching methods, a special
cartridge format  was  necessary,  to  let  the  emulator  know  where  the
cartridge should reside, the control line  states  to  enable  it  and  any
special hardware features it uses.

(...)

[ A .CRT file consists of

    $0000-0040 :    Header of the whole .crt files
    $0040-EOF :     Blocks of data

  Each block of data, called 'CHIP', can be of variable size. The first
0x10 bytes of each CHIP block is the block header, and it contains various
informations on the block itself, as its size (both with and without the
header), the loading address and an index to identify which memory bank
the data must be loaded to.  FP ]

.CRT header description
-----------------------

 Bytes: $0000-000F - 16-byte cartridge signature  "C64  CARTRIDGE"  (padded
                     with space characters)
         0010-0013 - File header length  ($00000040,  in  high/low  format,
                     calculated from offset $0000). The default  (also  the
                     minimum) value is $40.  Some  cartridges  exist  which
                     show a value of $00000020 which is wrong.
         0014-0015 - Cartridge version (high/low, presently 01.00)
         0016-0017 - Cartridge hardware type ($0000, high/low)
              0018 - Cartridge port EXROM line status
                      0 - inactive
                      1 - active
              0019 - Cartridge port GAME line status
                      0 - inactive
                      1 - active
         001A-001F - Reserved for future use
         0020-003F - 32-byte cartridge  name  "CCSMON"  (uppercase,  padded
                     with null characters)
         0040-xxxx - Cartridge contents (called CHIP PACKETS, as there  can
                     be more than one  per  CRT  file).  See  below  for  a
                     breakdown of the CHIP format.

CHIP content description
------------------------

[ Addresses shifted back to $0000.  FP ]

 Bytes: $0000-0003 - Contained ROM signature "CHIP" (note there can be more
                     than one image in a .CRT file)
         0004-0007 - Total packet length (ROM  image  size  and
                     header combined) (high/low format)
         0008-0009 - Chip type
                      0 - ROM
                      1 - RAM, no ROM data
                      2 - Flash ROM
         000A-000B - Bank number
         000C-000D - Starting load address (high/low format)
         000E-000F - ROM image size in bytes  (high/low  format,  typically
                     $2000 or $4000)
         0010-xxxx - ROM data


*/


#define C64_MAX_ROMBANK 64 //?

static CBM_ROM c64_cbm_cart[C64_MAX_ROMBANK] = { {0} };
static INT8 cbm_c64_game;
static INT8 cbm_c64_exrom;

static DEVICE_IMAGE_UNLOAD( c64_cart )
{
	int i;

	for (i = 0; i < C64_MAX_ROMBANK; i++)
	{
		c64_cbm_cart[i].size = 0;
		c64_cbm_cart[i].addr = 0;
		c64_cbm_cart[i].chip = 0;
	}
}


static DEVICE_START( c64_cart )
{
	int index = 0;

	if (strcmp(device->tag, "cart1") == 0)
		index = 0;

	if (strcmp(device->tag, "cart2") == 0)
		index = 1;


	/* In the first slot we can load a .crt file. In this case we want
        to use game & exrom values from the header, not the default ones. */
	if (index == 0)
	{
		cbm_c64_game = -1;
		cbm_c64_exrom = -1;
	}
}

/* Hardware Types for C64 carts */
enum {
	GENERIC_CRT = 0,		/* 00 - Normal cartridge                    */
	ACTION_REPLAY,		/* 01 - Action Replay                       */
	KCS_PC,			/* 02 - KCS Power Cartridge                 */
	FINAL_CART_III,		/* 03 - Final Cartridge III                 */
	SIMONS_BASIC,		/* 04 - Simons Basic                        */
	OCEAN_1,			/* 05 - Ocean type 1 (1)                    */
	EXPERT,			/* 06 - Expert Cartridge                    */
	FUN_PLAY,			/* 07 - Fun Play, Power Play                    */
	SUPER_GAMES,		/* 08 - Super Games                     */
	ATOMIC_POWER,		/* 09 - Atomic Power                        */
	EPYX_FASTLOAD,		/* 10 - Epyx Fastload                       */
	WESTERMANN,			/* 11 - Westermann Learning                 */
	REX,				/* 12 - Rex Utility                     */
	FINAL_CART_I,		/* 13 - Final Cartridge I                   */
	MAGIC_FORMEL,		/* 14 - Magic Formel                        */
	C64GS,			/* 15 - C64 Game System, System 3               */
	WARPSPEED,			/* 16 - WarpSpeed                           */
	DINAMIC,			/* 17 - Dinamic (2)                     */
	ZAXXON,			/* 18 - Zaxxon, Super Zaxxon (SEGA)             */
	DOMARK,			/* 19 - Magic Desk, Domark, HES Australia           */
	SUPER_SNAP_5,		/* 20 - Super Snapshot 5                    */
	COMAL_80,			/* 21 - Comal-80                            */
	STRUCT_BASIC,		/* 22 - Structured Basic                    */
	ROSS,				/* 23 - Ross                            */
	DELA_EP64,			/* 24 - Dela EP64                           */
	DELA_EP7X8,			/* 25 - Dela EP7x8                      */
	DELA_EP256,			/* 26 - Dela EP256                      */
	REX_EP256,			/* 27 - Rex EP256                           */
	MIKRO_ASSMBLR,		/* 28 - Mikro Assembler                     */
	REAL_FC_I,			/* 29 - (3)                             */
	ACTION_REPLAY_4,		/* 30 - Action Replay 4                     */
	STARDOS,			/* 31 - StarDOS                         */
	/*
    (1) Ocean type 1 includes Navy Seals, Robocop 2 & 3,  Shadow  of
    the Beast, Toki, Terminator 2 and more. Both 256 and 128 Kb images.
    (2) Dinamic includes Narco Police and more.
    (3) Type 29 is reserved for the real Final Cartridge I, the one
    above (Type 13) will become Final Cartridge II.                 */
	/****************************************
    Vice also defines the following types:
    #define CARTRIDGE_ACTION_REPLAY3    -29
    #define CARTRIDGE_IEEE488           -11
    #define CARTRIDGE_IDE64             -7
    #define CARTRIDGE_RETRO_REPLAY      -5
    #define CARTRIDGE_SUPER_SNAPSHOT    -4

    Can we support these as well?
    *****************************************/
};

static UINT8 c64_mapper = GENERIC_CRT;

static DEVICE_IMAGE_LOAD( c64_cart )
{
	int size = image_length(image), test, i = 0, n_banks;
	const char *filetype;
	int address = 0, new_start = 0;
	// int lbank_end_addr = 0, hbank_end_addr = 0;
	UINT8 *cart = memory_region(image->machine, "cart");

	filetype = image_filetype(image);

	/* We support .crt files */
	if (!mame_stricmp(filetype, "crt"))
	{
		int j;
		unsigned short c64_cart_type;

		for (i = 0; (i < ARRAY_LENGTH(c64_cbm_cart)) && (c64_cbm_cart[i].size != 0); i++)
		;

		if (i >= ARRAY_LENGTH(c64_cbm_cart))
			return INIT_FAIL;

		/* Start to parse the .crt header */
		/* 0x16-0x17 is Hardware type */
		image_fseek(image, 0x16, SEEK_SET);
		image_fread(image, &c64_cart_type, 2);
		c64_cart_type = BIG_ENDIANIZE_INT16(c64_cart_type);
		c64_mapper = c64_cart_type;

		/* If it is unsupported cart type, warn the user */
		switch (c64_cart_type)
		{
			case ACTION_REPLAY:	/* Type #  1 */
			case KCS_PC:		/* Type #  2 */
			case FINAL_CART_III:	/* Type #  3 */
			case SIMONS_BASIC:	/* Type #  4 */
			case OCEAN_1:		/* Type #  5 */
			case EXPERT:		/* Type #  6 */
			case FUN_PLAY:		/* Type #  7 */
			case SUPER_GAMES:		/* Type #  8 */
			case ATOMIC_POWER:	/* Type #  9 */
			case EPYX_FASTLOAD:	/* Type # 10 */
			case WESTERMANN:		/* Type # 11 */
			case REX:			/* Type # 12 */
			case FINAL_CART_I:	/* Type # 13 */
			case MAGIC_FORMEL:	/* Type # 14 */
			case C64GS:			/* Type # 15 */
			case DINAMIC:		/* Type # 17 */
			case ZAXXON:		/* Type # 18 */
			case DOMARK:		/* Type # 19 */
			case SUPER_SNAP_5:	/* Type # 20 */
			case COMAL_80:		/* Type # 21 */
			case GENERIC_CRT:		/* Type #  0 */
				printf("Currently supported cart type (Type %d)\n", c64_cart_type);
				break;

			default:
				printf("Currently unsupported cart type (Type %d)\n", c64_cart_type);
				break;
		}

		/* 0x18 is EXROM */
		image_fseek(image, 0x18, SEEK_SET);
		image_fread(image, &cbm_c64_exrom, 1);

		/* 0x19 is GAME */
		image_fread(image, &cbm_c64_game, 1);

		/* We can pass to the data: it starts from 0x40 */
		image_fseek(image, 0x40, SEEK_SET);
		j = 0x40;

		logerror("Loading cart %s size:%.4x\n", image_filename(image), size);
		logerror("Header info: EXROM %d, GAME %d, Cart Type %d \n", cbm_c64_exrom, cbm_c64_game, c64_cart_type);


		/* Data in a .crt image are organized in blocks called 'CHIP':
           each 'CHIP' consists of a 0x10 header, which contains the
           actual size of the block, the loading address and info on
           the bankswitch, followed by the actual data                  */
		while (j < size)
		{
			unsigned short chip_size, chip_bank_index, chip_data_size;
			unsigned char buffer[10];

			/* Start to parse the CHIP header */
			/* First 4 bytes are the string 'CHIP' */
			image_fread(image, buffer, 6);

			/* 0x06-0x07 is the size of the CHIP block (header + data) */
			image_fread(image, &chip_size, 2);
			chip_size = BIG_ENDIANIZE_INT16(chip_size);

			/* 0x08-0x09 chip type (ROM, RAM + no ROM, Flash ROM) */
			image_fread(image, buffer + 6, 2);

			/* 0x0a-0x0b is the bank number of the CHIP block */
			image_fread(image, &chip_bank_index, 2);
			chip_bank_index = BIG_ENDIANIZE_INT16(chip_bank_index);

			/* 0x0c-0x0d is the loading address of the CHIP block */
			image_fread(image, &address, 2);
			address = BIG_ENDIANIZE_INT16(address);

			/* 0x0e-0x0f is the data size of the CHIP block (without header) */
			image_fread(image, &chip_data_size, 2);
			chip_data_size = BIG_ENDIANIZE_INT16(chip_data_size);

			/* Print out the CHIP header! */
			logerror("%.4s %.2x %.2x %.4x %.2x %.2x %.4x %.4x:%.4x\n",
				buffer, buffer[4], buffer[5], chip_size,
				buffer[6], buffer[7], chip_bank_index,
				address, chip_data_size);
			logerror("Loading CHIP data at %.4x size:%.4x\n", address, chip_data_size);

			/* Does CHIP contain any data? */
			c64_cbm_cart[i].chip = (UINT8*) image_malloc(image, chip_data_size);
			if (!c64_cbm_cart[i].chip)
				return INIT_FAIL;

			/* Store data, address & size of the CHIP block */
			c64_cbm_cart[i].addr = address;
			c64_cbm_cart[i].index = chip_bank_index;
			c64_cbm_cart[i].size = chip_data_size;
			c64_cbm_cart[i].start = new_start;

			test = image_fread(image, c64_cbm_cart[i].chip, chip_data_size);

			memcpy(&cart[new_start], c64_cbm_cart[i].chip, c64_cbm_cart[i].size);
			new_start += c64_cbm_cart[i].size;

			if (test != chip_data_size)
				return INIT_FAIL;

			/* Advance to the next CHIP block */
			i++;
			j += chip_size;
		}
	}
	else /* We also support .80 files for c64 & .e0/.f0 for max */
	{
		/* Assign loading address according to extension */
		if (!mame_stricmp (filetype, "80"))
			address = 0x8000;

		if (!mame_stricmp (filetype, "e0"))
			address = 0xe000;

		if (!mame_stricmp (filetype, "f0"))
			address = 0xf000;

		logerror("loading %s rom at %.4x size:%.4x\n", image_filename(image), address, size);

		/* Does cart contain any data? */
		c64_cbm_cart[0].chip = (UINT8*) image_malloc(image, size);
		if (!c64_cbm_cart[0].chip)
			return INIT_FAIL;

		/* Store data, address & size */
		c64_cbm_cart[0].addr = address;
		c64_cbm_cart[0].size = size;
		c64_cbm_cart[0].start = new_start;

		test = image_fread(image, c64_cbm_cart[0].chip, size);

		memcpy(&cart[new_start], c64_cbm_cart[0].chip, c64_cbm_cart[0].size);
		new_start += c64_cbm_cart[0].size;

		if (test != c64_cbm_cart[0].size)
			return INIT_FAIL;
	}

	n_banks = i;

	/* If we load a .crt file, use EXROM & GAME from the header! */
	if ((cbm_c64_exrom != -1) && (cbm_c64_game != -1))
	{
		c64_exrom = cbm_c64_exrom;
		c64_game  = cbm_c64_game;
	}

	/* Finally load the cart */
	roml = c64_roml;
	romh = c64_romh;

	memset(roml, 0, 0x2000);
	memset(romh, 0, 0x2000);

	switch (c64_mapper)
	{
	case ZAXXON:
		memcpy(romh, cart + 0x1000, 0x2000);
		break;
	default:
		if (!c64_game && c64_exrom && (n_banks == 1))
			memcpy(romh, cart, 0x2000);
		else
		{
			memcpy(roml, cart + 0 * 0x2000, 0x2000);
			memcpy(romh, cart + 1 * 0x2000, 0x2000);
		}
	}

	c64_cart_n_banks = n_banks; // this is needed so that we only set mappers if a cart is present!

	return INIT_PASS;
}


static WRITE8_HANDLER( fc3_bank_w )
{
	// Type # 3
	// working:
	// not working:

	UINT8 bank = data & 0x3f;
	UINT8 *cart = memory_region(space->machine, "cart");

	if (data & 0x40)
	{
		if (bank > 3)
			logerror("Warning: This cart type should have at most 4 banks and the cart looked for bank %d... Something strange is going on!\n", bank);
		else
		{
			memcpy(roml, cart + bank * 0x4000, 0x2000);
			memcpy(romh, cart + bank * 0x4000 + 0x2000, 0x2000);
/*
            if (log_cart)
            {
                logerror("bank %d of size %d successfully loaded at %d!\n", bank, c64_cbm_cart[bank].size, c64_cbm_cart[bank].addr);
                if (c64_cbm_cart[bank].index != bank)
                    logerror("Warning: According to the CHIP info this should be bank %d, but we are loading it as bank %d!\n", c64_cbm_cart[bank].index, bank);
            }
*/
		}
	}
}

static WRITE8_HANDLER( ocean1_bank_w )
{
	// Type # 5
	// working: Double Dragon, Ghostbusters, Terminator 2
	// not working: Pang, Robocop 2, Toki

	UINT8 bank = data & 0x3f;
	UINT8 *cart = memory_region(space->machine, "cart");

	if (c64_cart_n_banks != 64)								// all carts except Terminator II
	{
		if (bank < 16)
			memcpy(roml, cart + bank * 0x2000, 0x2000);
		else
			memcpy(romh, cart + (bank - 16) * 0x2000, 0x2000);
	}
	else  											// Terminator II
	{
		memcpy(roml, cart + bank * 0x2000, 0x2000);
	}
/*
    if (log_cart)
    {
        logerror("bank %d of size %d successfully loaded at %d!\n", bank, c64_cbm_cart[bank].size, c64_cbm_cart[bank].addr);
        if (c64_cbm_cart[bank].index != bank)
            logerror("Warning: According to the CHIP info this should be bank %d, but we are loading it as bank %d!\n", c64_cbm_cart[bank].index, bank);
    }
*/
}

static WRITE8_HANDLER( funplay_bank_w )
{
	// Type # 7
	// working:
	// not working:

	UINT8 bank = data & 0x39, real_bank = 0;
	UINT8 *cart = memory_region(space->machine, "cart");

	/* This should be written after the bankswitch has happened. We log it to see if it is really working */
	if (data == 0x86)
		logerror("Reserved value written\n");
	else
	{
		/* bank number is not the value written, but c64_cbm_cart[bank].index IS the value written! */
		real_bank = ((bank & 0x01) << 3) + ((bank & 0x38) >> 3);

		memcpy(roml, cart + real_bank * 0x2000, 0x2000);
/*
        if (log_cart)
        {
            logerror("bank %d of size %d successfully loaded at %d!\n", bank, c64_cbm_cart[bank].size, c64_cbm_cart[bank].addr);
            if (c64_cbm_cart[bank].index != bank)
                logerror("Warning: According to the CHIP info this should be bank %d, but we are loading it as bank %d!\n", c64_cbm_cart[bank].index, bank);
        }
*/
	}
}

static WRITE8_HANDLER( supergames_bank_w )
{
	// Type # 8
	// working:
	// not working:

	UINT8 bank = data & 0x03, bit2 = data & 0x04;
	UINT8 *cart = memory_region(space->machine, "cart");

	if (data & 0x04)
	{
		c64_game = 0;
		c64_exrom = 0;
	}
	else
	{
		c64_game = 0;
		c64_exrom = 0;
	}

	if (data == 0xc)
	{
		c64_game = 1;
		c64_exrom = 1;
	}

	if (bit2)
	{
		memcpy(roml, cart + bank * 0x4000, 0x2000);
		memcpy(romh, cart + bank * 0x4000 + 0x2000, 0x2000);
	}
	else
	{
		memcpy(roml, cart + bank * 0x4000, 0x2000);
		memcpy(romh, cart + bank * 0x4000 + 0x2000, 0x2000);
	}
}

static WRITE8_HANDLER( c64gs_bank_w )
{
	// Type # 15
	// working:
	// not working: The Last Ninja Remix

	UINT8 bank = offset & 0xff;
	UINT8 *cart = memory_region(space->machine, "cart");

	if (bank > 0x3f)
		logerror("Warning: This cart type should have at most 64 banks and the cart looked for bank %d... Something strange is going on!\n", bank);

	memcpy(roml, cart + bank * 0x2000, 0x2000);
/*
    if (log_cart)
    {
        logerror("bank %d of size %d successfully loaded at %d!\n", bank, c64_cbm_cart[bank].size, c64_cbm_cart[bank].addr);
        if (c64_cbm_cart[bank].index != bank)
            logerror("Warning: According to the CHIP info this should be bank %d, but we are loading it as bank %d!\n", c64_cbm_cart[bank].index, bank);
    }
*/
}

static READ8_HANDLER( dinamic_bank_r )
{
	// Type # 17
	// working: Satan
	// not working:

	UINT8 bank = offset & 0xff;
	UINT8 *cart = memory_region(space->machine, "cart");

	if (bank > 0xf)
		logerror("Warning: This cart type should have 16 banks and the cart looked for bank %d... Something strange is going on!\n", bank);

	memcpy(roml, cart + bank * 0x2000, 0x2000);
/*
    if (log_cart)
    {
        logerror("bank %d of size %d successfully loaded at %d!\n", bank, c64_cbm_cart[bank].size, c64_cbm_cart[bank].addr);
        if (c64_cbm_cart[bank].index != bank)
            logerror("Warning: According to the CHIP info this should be bank %d, but we are loading it as bank %d!\n", c64_cbm_cart[bank].index, bank);
    }
*/
	return 0;
}

static READ8_HANDLER( zaxxon_bank_r )
{
	// Type # 18
	// working:
	// not working:

	UINT8 bank;
	UINT8 *cart = memory_region(space->machine, "cart");

	if (offset < 0x1000)
		bank = 0;
	else
		bank = 1;

	// c64_game = 0;
	// c64_exrom = 0;

	memcpy(romh, cart + bank * 0x2000 + 0x1000, 0x2000);

	return cart[offset & 0x0fff];
}

static WRITE8_HANDLER( domark_bank_w )
{
	// Type # 19
	// working:
	// not working:

	UINT8 bank = data & 0x7f;
	UINT8 *cart = memory_region(space->machine, "cart");

	if (data & 0x80)
	{
		c64_game = 1;
		c64_exrom = 1;
	}
	else
	{
		c64_game = 1;
		c64_exrom = 0;
		memcpy(roml, cart + bank * 0x2000, 0x2000);
	}
}

static WRITE8_HANDLER( comal80_bank_w )
{
	// Type # 21
	// working: Comal 80
	// not working:

	UINT8 bank = data & 0x83;
	UINT8 *cart = memory_region(space->machine, "cart");

	/* only valid values 0x80, 0x81, 0x82, 0x83 */
	if (!(bank & 0x80))
		logerror("Warning: we are writing an invalid bank value %d\n", bank);
	else
	{
		bank &= 0x03;

		memcpy(roml, cart + bank * 0x4000, 0x4000);
/*
        if (log_cart)
        {
            logerror("bank %d of size %d successfully loaded at %d!\n", bank, c64_cbm_cart[bank].size, c64_cbm_cart[bank].addr);
            if (c64_cbm_cart[bank].index != bank)
                logerror("Warning: According to the CHIP info this should be bank %d, but we are loading it as bank %d!\n", c64_cbm_cart[bank].index, bank);
        }
*/
	}
}

static void setup_c64_custom_mappers(running_machine *machine)
{
	const address_space *space = cputag_get_address_space( machine, "maincpu", ADDRESS_SPACE_PROGRAM );

	switch (c64_mapper)
	{
		case ACTION_REPLAY:	/* Type #  1 not working */
			break;
		case KCS_PC:		/* Type #  2 not working */
			break;
		case FINAL_CART_III:    /* Type #  3 not working - 4 16k banks, loaded at 0x8000, banks chosen by writing to 0xdfff */
			memory_install_write8_handler( space, 0xdfff, 0xdfff, 0, 0, fc3_bank_w );
			break;
		case SIMONS_BASIC:	/* Type #  4 not working */
			break;
		case OCEAN_1:           /* Type #  5 - up to 64 8k banks, loaded at 0x8000 or 0xa000, banks chosen by writing to 0xde00 */
			memory_install_write8_handler( space, 0xde00, 0xde00, 0, 0, ocean1_bank_w );
			break;
		case EXPERT:		/* Type #  6 not working */
			break;
		case FUN_PLAY:          /* Type #  7 - 16 8k banks, loaded at 0x8000, banks chosen by writing to 0xde00 */
			memory_install_write8_handler( space, 0xde00, 0xde00, 0, 0, funplay_bank_w );
			break;
		case SUPER_GAMES:		/* Type #  8 not working */
			memory_install_write8_handler( space, 0xdf00, 0xdf00, 0, 0, supergames_bank_w );
			break;
		case ATOMIC_POWER:	/* Type #  9 not working */
			break;
		case EPYX_FASTLOAD:	/* Type # 10 not working */
			break;
		case WESTERMANN:		/* Type # 11 not working */
			break;
		case REX:			/* Type # 12 working */
			break;
		case FINAL_CART_I:	/* Type # 13 not working */
			break;
		case MAGIC_FORMEL:	/* Type # 14 not working */
			break;
		case C64GS:             /* Type # 15 - up to 64 8k banks, loaded at 0x8000, banks chosen by writing to 0xde00 + bank */
			memory_install_write8_handler( space, 0xde00, 0xdeff, 0, 0, c64gs_bank_w );
			break;
		case DINAMIC:           /* Type # 17 - 16 8k banks, loaded at 0x8000, banks chosen by reading to 0xde00 + bank */
			memory_install_read8_handler( space, 0xde00, 0xdeff, 0, 0, dinamic_bank_r );
			break;
		case ZAXXON:		/* Type # 18 */
			memory_install_read8_handler( space, 0x8000, 0x9fff, 0, 0, zaxxon_bank_r );
			break;
		case DOMARK:		/* Type # 19 */
			memory_install_write8_handler( space, 0xde00, 0xde00, 0, 0, domark_bank_w );
			break;
		case SUPER_SNAP_5:	/* Type # 20 not working */
			break;
		case COMAL_80:          /* Type # 21 - 4 16k banks, loaded at 0x8000, banks chosen by writing to 0xde00 */
			memory_install_write8_handler( space, 0xde00, 0xde00, 0, 0, comal80_bank_w );
			break;
		case GENERIC_CRT:       /* Type #  0 - single bank, no bankswitch, loaded at start with correct size and place */
		default:
			break;
	}
}


MACHINE_RESET( c64 )
{
	if (c64_cart_n_banks)
		setup_c64_custom_mappers(machine);
}


MACHINE_DRIVER_START( c64_cartslot )
	MDRV_CARTSLOT_ADD("cart1")
	MDRV_CARTSLOT_EXTENSION_LIST("crt,80")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_START(c64_cart)
	MDRV_CARTSLOT_LOAD(c64_cart)
	MDRV_CARTSLOT_UNLOAD(c64_cart)

	MDRV_CARTSLOT_ADD("cart2")
	MDRV_CARTSLOT_EXTENSION_LIST("crt,80")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_START(c64_cart)
	MDRV_CARTSLOT_LOAD(c64_cart)
	MDRV_CARTSLOT_UNLOAD(c64_cart)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( ultimax_cartslot )
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("crt,e0,f0")
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_START(c64_cart)
	MDRV_CARTSLOT_LOAD(c64_cart)
	MDRV_CARTSLOT_UNLOAD(c64_cart)
MACHINE_DRIVER_END
