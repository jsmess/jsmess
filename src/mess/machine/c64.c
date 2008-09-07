/***************************************************************************
	commodore c64 home computer

    peter.trauner@jk.uni-linz.ac.at
    documentation
     www.funet.fi
***************************************************************************/

/*
  unsolved problems:
   execution of code in the io devices
    (program write some short test code into the vic sprite register)
 */

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
#include "deprecat.h"

#include "includes/cbm.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "video/vic6567.h"
#include "video/vdc8563.h"

#include "includes/c64.h"
#include "includes/c128.h"      /* we need c128_bankswitch_64 in MACHINE_START */

#include "devices/cassette.h"
#include "devices/cartslot.h"

#define VERBOSE_LEVEL 0
#define DBG_LOG(N,M,A) \
	{ \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time()), (char*) M ); \
			logerror A; \
		} \
	}

unsigned char c65_keyline = { 0xff };
UINT8 c65_6511_port=0xff;

UINT8 c128_keyline[3] = {0xff, 0xff, 0xff};


/* keyboard lines */
UINT8 c64_keyline[10] =
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* expansion port lines input */
int c64_pal = 0;
UINT8 c64_game = 1, c64_exrom = 1;

/* cpu port */
int c128_va1617;
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
static int ultimax = 0;
int c64_tape_on = 1;
static int c64_cia1_on = 1;
static int c64_io_enabled = 0;


static UINT8 serial_clock, serial_data, serial_atn;
static UINT8 vicirq = 0;

static int is_c65(running_machine *machine)
{
	return !strncmp(machine->gamedrv->name, "c65", 3);
}

static int is_c128(running_machine *machine)
{
	return !strncmp(machine->gamedrv->name, "c128", 4);
}

static void c64_nmi(running_machine *machine)
{
	static int nmilevel = 0;
	int cia1irq = cia_get_irq(1);

	if (nmilevel != (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq)	/* KEY_RESTORE */
	{
		if (is_c128(machine))
		{
			if (cpu_getactivecpu() == 0)
			{
				/* z80 */
				cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq);
			}
			else
			{
				cpunum_set_input_line(machine, 1, INPUT_LINE_NMI, (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq);
			}
		}
		
		else
		{
			cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq);
		}
		
		nmilevel = (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq;
	}
}


/*
 * cia 0
 * port a
 * 7-0 keyboard line select
 * 7,6: paddle select( 01 port a, 10 port b)
 * 4: joystick a fire button
 * 3,2: Paddles port a fire button
 * 3-0: joystick a direction
 * port b
 * 7-0: keyboard raw values
 * 4: joystick b fire button, lightpen select
 * 3,2: paddle b fire buttons (left,right)
 * 3-0: joystick b direction
 * flag cassette read input, serial request in
 * irq to irq connected
 */
static UINT8 c64_cia0_port_a_r (void)
{
	UINT8 value = 0xff;
	UINT8 cia0portb = cia_get_output_b(0);

	if (!(cia0portb & 0x80))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x80)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x80)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x80)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x80)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x80)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x80)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x80)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x80)) t &= ~0x01;
		value &= t;
	}

	if (!(cia0portb & 0x40))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x40)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x40)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x40)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x40)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x40)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x40)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x40)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x40)) t &= ~0x01;
		value &= t;
	}

	if (!(cia0portb & 0x20))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x20)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x20)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x20)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x20)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x20)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x20)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x20)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x20)) t &= ~0x01;
		value &= t;
	}

	if (!(cia0portb & 0x10))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x10)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x10)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x10)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x10)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x10)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x10)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x10)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x10)) t &= ~0x01;
		value &= t;
	}

	if (!(cia0portb & 0x08))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x08)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x08)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x08)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x08)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x08)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x08)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x08)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x08)) t &= ~0x01;
		value &= t;
	}

	if (!(cia0portb & 0x04))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x04)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x04)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x04)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x04)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x04)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x04)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x04)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x04)) t &= ~0x01;
		value &= t;
	}

	if (!(cia0portb & 0x02))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x02)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x02)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x02)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x02)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x02)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x02)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x02)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x02)) t &= ~0x01;
		value &= t;
	}

	if (!(cia0portb & 0x01))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x01)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x01)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x01)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x01)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x01)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x01)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x01)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x01)) t &= ~0x01;
		value &= t;
	}

	if ( input_port_read(Machine, "DSW0") & 0x0100 )
		value &= c64_keyline[8];
	else
		value &= c64_keyline[9];

	return value;
}

static UINT8 c64_cia0_port_b_r (void)
{
    UINT8 value = 0xff;
	UINT8 cia0porta = cia_get_output_a(0);

    if (!(cia0porta & 0x80)) value &= c64_keyline[7];
    if (!(cia0porta & 0x40)) value &= c64_keyline[6];
    if (!(cia0porta & 0x20)) value &= c64_keyline[5];
    if (!(cia0porta & 0x10)) value &= c64_keyline[4];
    if (!(cia0porta & 0x08)) value &= c64_keyline[3];
    if (!(cia0porta & 0x04)) value &= c64_keyline[2];
    if (!(cia0porta & 0x02)) value &= c64_keyline[1];
    if (!(cia0porta & 0x01)) value &= c64_keyline[0];

	if ( input_port_read(Machine, "DSW0") & 0x0100 )
		value &= c64_keyline[9];
    else 
		value &= c64_keyline[8];

    if (is_c128(Machine))
    {
		if (!vic2e_k0_r ())
			value &= c128_keyline[0];
		if (!vic2e_k1_r ())
			value &= c128_keyline[1];
		if (!vic2e_k2_r ())
			value &= c128_keyline[2];
    }
    if (is_c65(Machine))
	{
		if (!(c65_6511_port & 0x02))
			value &= c65_keyline;
    }

    return value;
}

static void c64_cia0_port_b_w (UINT8 data)
{
    vic2_lightpen_write (data & 0x10);
}

static void c64_irq (running_machine *machine, int level)
{
	static int old_level = 0;

	if (level != old_level)
	{
		DBG_LOG (3, "mos6510", ("irq %s\n", level ? "start" : "end"));
		if (is_c128(machine))
		{
			if (0 && (cpu_getactivecpu() == 0))
			{
				cpunum_set_input_line(machine, 0, 0, level);
			}
			else
			{
				cpunum_set_input_line(machine, 1, M6510_IRQ_LINE, level);
			}
		}
		else
		{
			cpunum_set_input_line(machine, 0, M6510_IRQ_LINE, level);
		}
		old_level = level;
	}
}

static void c64_cia0_interrupt (running_machine *machine, int level)
{
	c64_irq (machine, level || vicirq);
}

void c64_vic_interrupt (int level)
{
#if 1
	if (level != vicirq)
	{
		c64_irq (Machine, level || cia_get_irq(0));
		vicirq = level;
	}
#endif
}

/*
 * cia 1
 * port a
 * 7 serial bus data input
 * 6 serial bus clock input
 * 5 serial bus data output
 * 4 serial bus clock output
 * 3 serial bus atn output
 * 2 rs232 data output
 * 1-0 vic-chip system memory bank select
 *
 * port b
 * 7 user rs232 data set ready
 * 6 user rs232 clear to send
 * 5 user
 * 4 user rs232 carrier detect
 * 3 user rs232 ring indicator
 * 2 user rs232 data terminal ready
 * 1 user rs232 request to send
 * 0 user rs232 received data
 * flag restore key or rs232 received data input
 * irq to nmi connected ?
 */
static UINT8 c64_cia1_port_a_r (void)
{
	UINT8 value = 0xff;

	if (!serial_clock || !cbm_serial_clock_read ())
		value &= ~0x40;

	if (!serial_data || !cbm_serial_data_read ())
		value &= ~0x80;

	return value;
}

static void c64_cia1_port_a_w (UINT8 data)
{
	static const int helper[4] = {0xc000, 0x8000, 0x4000, 0x0000};

	cbm_serial_clock_write (serial_clock = !(data & 0x10));
	cbm_serial_data_write (serial_data = !(data & 0x20));
	cbm_serial_atn_write (serial_atn = !(data & 0x08));
	c64_vicaddr = c64_memory + helper[data & 0x03];
	if (is_c128(Machine))
	{
		c128_vicaddr = c64_memory + helper[data & 0x03] + c128_va1617;
	}
}

static void c64_cia1_interrupt (running_machine *machine, int level)
{
	c64_nmi(machine);
}

const cia6526_interface c64_cia0 =
{
	CIA6526,
	c64_cia0_interrupt,
	0.0, 60,

	{
		{ c64_cia0_port_a_r, NULL },
		{ c64_cia0_port_b_r, c64_cia0_port_b_w }
	}
};

const cia6526_interface c64_cia1 =
{
	CIA6526,
	c64_cia1_interrupt,
	0.0, 60,

	{
		{ c64_cia1_port_a_r, c64_cia1_port_a_w },
		{ 0, 0 }
	}
};


static UINT8 *c64_io_ram_w_ptr;
static UINT8 *c64_io_ram_r_ptr;

WRITE8_HANDLER( c64_write_io )
{
	c64_io_mirror[ offset ] = data;
	if (offset < 0x400) {
		vic2_port_w (machine, offset & 0x3ff, data);
	} else if (offset < 0x800) {
		sid6581_0_port_w (machine, offset & 0x3ff, data);
	} else if (offset < 0xc00)
		c64_colorram[offset & 0x3ff] = data | 0xf0;
	else if (offset < 0xd00)
		cia_0_w(machine, offset, data);
	else if (offset < 0xe00)
	{
		if (c64_cia1_on)
			cia_1_w(machine, offset, data);
		else
			DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
	}
	else if (offset < 0xf00)
	{
		/* i/o 1 */
			DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
	}
	else
	{
		/* i/o 2 */
			DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
	}
}

WRITE8_HANDLER(c64_ioarea_w)
{
	if (c64_io_enabled) 
	{
		c64_write_io(machine, offset, data);
	} 
	else 
	{
		c64_io_ram_w_ptr[offset] = data;
	}
}

READ8_HANDLER( c64_read_io )
{
	if (offset < 0x400)
		return vic2_port_r (machine, offset & 0x3ff);

	else if (offset < 0x800)
		return sid6581_0_port_r (machine, offset & 0x3ff);

	else if (offset < 0xc00)
		return c64_colorram[offset & 0x3ff];

	else if (offset == 0xc00)
		{
			cia_set_port_mask_value(0, 0, input_port_read(machine, "DSW0") & 0x0100 ? c64_keyline[8] : c64_keyline[9] );
			return cia_0_r(machine, offset);
		}

	else if (offset == 0xc01)
		{
			cia_set_port_mask_value(0, 1, input_port_read(machine, "DSW0") & 0x0100 ? c64_keyline[9] : c64_keyline[8] );
			return cia_0_r(machine, offset);
		}

	else if (offset < 0xd00)
		return cia_0_r(machine, offset);

	else if (c64_cia1_on && (offset < 0xe00))
		return cia_1_r(machine, offset);

	DBG_LOG (1, "io read", ("%.3x\n", offset));

	return 0xff;
}

READ8_HANDLER(c64_ioarea_r)
{
	return c64_io_enabled ? c64_read_io(machine, offset) : c64_io_ram_r_ptr[offset];
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

static void c64_bankswitch(running_machine *machine, int reset)
{
	static int old = -1, exrom, game;
	int loram, hiram, charen;
	int ultimax_mode = 0;
	int data = (UINT8) cpunum_get_info_int(0, CPUINFO_INT_M6510_PORT) & 0x07;

	/* If nothing has changed or reset = 0, don't do anything */
	if ((data == old) && (exrom == c64_exrom) && (game == c64_game) && !reset) 
		return;

	/* Are we in Ultimax mode? */
	if (!c64_game && c64_exrom)
		ultimax_mode = 1;

	DBG_LOG (1, "bankswitch", ("%d\n", data & 7));
	loram  = (data & 1) ? 1 : 0;
	hiram  = (data & 2) ? 1 : 0;
	charen = (data & 4) ? 1 : 0;
	logerror("Bankswitch mode || charen, ultimax\n");
	logerror("%d, %d, %d, %d  ||   %d,      %d  \n", loram, hiram, c64_game, c64_exrom, charen, ultimax_mode);

	if (ultimax_mode)
	{
			c64_io_enabled = 1;		// charen has no effect in ultimax_mode

			memory_set_bankptr (1, roml);
			memory_set_bankptr (2, c64_memory + 0x8000);
			memory_set_bankptr (3, c64_memory + 0xa000);
			memory_set_bankptr (4, romh);
			memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_NOP);
	}
	else
	{
		/* 0x8000-0x9000 */
		if (loram && hiram && !c64_exrom)
		{
			memory_set_bankptr (1, roml);
			memory_set_bankptr (2, c64_memory + 0x8000);
		}
		else
		{
			memory_set_bankptr (1, c64_memory + 0x8000);
			memory_set_bankptr (2, c64_memory + 0x8000);
		}

		/* 0xa000 */
		if (hiram && !c64_game && !c64_exrom)
			memory_set_bankptr (3, romh);

		else if (loram && hiram && c64_game)
			memory_set_bankptr (3, c64_basic);

		else
			memory_set_bankptr (3, c64_memory + 0xa000);

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
			c64_io_enabled = 0;
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
		memory_set_bankptr (4, hiram ? c64_kernal : c64_memory + 0xe000);
		memory_set_bankptr (5, c64_memory + 0xe000);
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

void c64_m6510_port_write(UINT8 direction, UINT8 data)
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
			cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), (data & 0x08) ? -(0x5a9e >> 1) : +(0x5a9e >> 1));
		}

		if (direction & 0x20)
		{
			if(!(data & 0x20))
			{
				cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0), CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
				timer_adjust_periodic(datasette_timer, attotime_zero, 0, ATTOTIME_IN_HZ(44100));
			}
			else
			{
				cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0), CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
				timer_reset(datasette_timer, attotime_never);
			}
		}
	}

	if (is_c65(Machine))
	{
		// NPW 8-Feb-2004 - Don't know why I have to do this
		//c65_bankswitch(Machine);
	}

	else if (!ultimax)
		c64_bankswitch(Machine, 0);

	c64_memory[0x000] = program_read_byte( 0 );
	c64_memory[0x001] = program_read_byte( 1 );
}

UINT8 c64_m6510_port_read(UINT8 direction)
{
	running_machine *machine = Machine;
	UINT8 data = c64_port_data;

	if (c64_tape_on)
	{
		if ((cassette_get_state(image_from_devtype_and_index(IO_CASSETTE, 0)) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
			data &= ~0x10;
		else
			data |=  0x10;
	}

	if (is_c65(machine)) 
	{
		if (input_port_read(machine, "SPECIAL") & 0x20)		/* Check Caps Lock */
			data &= ~0x40;

		else 
			data |=  0x40;
	}

	return data;
}


int c64_paddle_read (int which)
{
	int pot1 = 0xff, pot2 = 0xff, pot3 = 0xff, pot4 = 0xff, temp;
	UINT8 cia0porta = cia_get_output_a(0);
	int controller = input_port_read(Machine, "DSW0") & 0x0e00;

	if (which)
	{
		switch (controller)
		{
			case 0x0400:
				pot4 = (input_port_read(Machine, "PADDLE3") & 0xff);
				break;

			case 0x0600:
				if (input_port_read(Machine, "JOY1") & 0x20)	/* Joy1 Button 2 */
					pot4 = 0x00;
				break;
			
			case 0x0800:
				pot4 = input_port_read(Machine, "TRACKY");
				break;

			case 0x4000:
				pot2 = (input_port_read(Machine, "PADDLE1") & 0xff);
				break;

			case 0x6000:
				if (input_port_read(Machine, "JOY0") & 0x20)	/* Joy0 Button 2 */
					pot1 = 0x00;
				break;
			
			case 0x8000:
				pot2 = input_port_read(Machine, "TRACKY");
				break;
		
			case 0xa000:
				break;
		
			default:
				logerror("Invalid Controller Setting %d\n", controller);
				break;
		}
	}
	else
	{
		switch (controller)
		{
			case 0x0400:
				pot3 = (input_port_read(Machine, "PADDLE2") & 0xff);
				break;
			
			case 0x0800:
				pot3 = input_port_read(Machine, "TRACKX");
				break;

			case 0x4000:
				pot1 = (input_port_read(Machine, "PADDLE0") & 0xff);
				break;

			case 0x8000:
				pot1 = input_port_read(Machine, "TRACKX");
				break;
				
			case 0x0600:
			case 0x6000:
			case 0xa000:
				break;
		
			default:
				logerror("Invalid Controller Setting %d\n", controller);
				break;
		}
	}

	if (input_port_read(Machine, "DSW0") & 0x0100)		/* Swap */
	{
		temp = pot1; pot1 = pot2; pot2 = pot1;
		temp = pot3; pot3 = pot4; pot4 = pot3;
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

 READ8_HANDLER(c64_colorram_read)
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
static int c64_dma_read( int offset )
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

static int c64_dma_read_ultimax( int offset )
{
	if (offset < 0x3000)
		return c64_memory[offset];

	return c64_romh[offset & 0x1fff];
}

static int c64_dma_read_color( int offset )
{
	return c64_colorram[offset & 0x3ff] & 0xf;
}

double last = 0;

TIMER_CALLBACK( c64_tape_timer )
{
	double tmp = cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0));

	if((last > +0.0) && (tmp < +0.0))
		cia_issue_index(machine, 0);

	last = tmp;
}

static void c64_common_driver_init (running_machine *machine)
{
	cia6526_interface cia_intf[2];

	/* configure the M6510 port */
	cpunum_set_info_fct(0, CPUINFO_PTR_M6510_PORTREAD, (genf *) c64_m6510_port_read);
	cpunum_set_info_fct(0, CPUINFO_PTR_M6510_PORTWRITE, (genf *) c64_m6510_port_write);

	if (!ultimax) 
	{
		UINT8 *mem = memory_region(machine, "main");
		c64_basic    = mem + 0x10000;
		c64_kernal   = mem + 0x12000;
		c64_chargen  = mem + 0x14000;
		c64_colorram = mem + 0x15000;
		c64_roml     = mem + 0x15400;
		c64_romh     = mem + 0x17400;
	}

	if (c64_tape_on)
		datasette_timer = timer_alloc(c64_tape_timer, NULL);

	/* CIA initialization */
	cia_intf[0] = c64_cia0;
	cia_intf[0].tod_clock = c64_pal ? 50 : 60;
	cia_config(machine, 0, &cia_intf[0]);
	
	if (c64_cia1_on)
	{
		cia_intf[1] = c64_cia1;
		cia_intf[1].tod_clock = c64_pal ? 50 : 60;
		cia_config(machine, 1, &cia_intf[1]);
	}
	
	
	if (ultimax)
		vic6567_init (0, c64_pal, c64_dma_read_ultimax, c64_dma_read_color, c64_vic_interrupt);
	else
		vic6567_init (0, c64_pal, c64_dma_read, c64_dma_read_color, c64_vic_interrupt);

	cia_reset();
}

DRIVER_INIT( c64 )
{
	c64_common_driver_init (machine);
}

DRIVER_INIT( c64pal )
{
	c64_pal = 1;
	c64_common_driver_init (machine);
}

DRIVER_INIT( ultimax )
{
	ultimax = 1;
    c64_cia1_on = 0;
	c64_common_driver_init (machine);
}

DRIVER_INIT( c64gs )
{
	c64_pal = 1;
	c64_tape_on = 0;
    c64_cia1_on = 1;
	c64_common_driver_init (machine);
}

DRIVER_INIT( sx64 )
{
	VC1541_CONFIG vc1541 = { 1, 8 };
	c64_tape_on = 0;
	c64_pal = 1;
	c64_common_driver_init (machine);
	vc1541_config (0, 0, &vc1541);
}

void c64_common_init_machine (running_machine *machine)
{
#ifdef VC1541
	vc1541_reset ();
#endif

	if (c64_cia1_on)
	{
		cbm_serial_reset_write (0);
		cbm_drive_0_config (SERIAL, is_c65(machine) ? 10 : 8);
		cbm_drive_1_config (SERIAL, is_c65(machine) ? 11 : 9);
		serial_clock = serial_data = serial_atn = 1;
	}

	c64_vicaddr = c64_memory;
	vicirq = 0;
}

static OPBASE_HANDLER( c64_opbase ) 
{
	if ((address & 0xf000) == 0xd000) 
	{
		if (c64_io_enabled) 
		{
			opbase->mask = 0x0fff;
			opbase->ram = c64_io_mirror;
			opbase->rom = c64_io_mirror;
			opbase->mem_min = 0x0000;
			opbase->mem_max = 0xcfff;
			c64_io_mirror[address & 0x0fff] = c64_read_io( machine, address & 0x0fff );
		} 
		else 
		{
			opbase->mask = 0x0fff;
			opbase->ram = c64_io_ram_r_ptr;
			opbase->rom = c64_io_ram_r_ptr;
			opbase->mem_min = 0x0000;
			opbase->mem_max = 0xcfff;
		}
		return ~0;
	}
	return address;
}


MACHINE_START( c64 )
{
	c64_port_data = 0x17;

	c64_io_mirror = auto_malloc( 0x1000 );
	c64_common_init_machine (machine);

	if (is_c128(machine))
		c128_bankswitch_64(machine, 1);

	if (!ultimax)
		c64_bankswitch(machine, 1);

	memory_set_opbase_handler( 0, c64_opbase );
}


INTERRUPT_GEN( c64_frame_interrupt )
{
	static int monitor = -1;
	int value, i;
	int controller1 = input_port_read(Machine, "DSW0") & 0xe000;
	int controller2 = input_port_read(Machine, "DSW0") & 0x0e00;
	static const char *c64ports[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7" };
	static const char *c128ports[] = { "KP0", "KP1", "KP2" };

	c64_nmi(machine);

	 if (is_c128(machine))
	 {
	 	if ((input_port_read(machine, "CFG") & 0x20) != monitor)
		{
			if (input_port_read(machine, "CFG") & 0x20)
			{
				vic2_set_rastering(0);
				vdc8563_set_rastering(1);
				video_screen_set_visarea(machine->primary_screen, 0, 655, 0, 215);
			}
			else
			{
				vic2_set_rastering(1);
				vdc8563_set_rastering(0);
				if (c64_pal)
					video_screen_set_visarea(machine->primary_screen, VIC6569_STARTVISIBLECOLUMNS, VIC6569_STARTVISIBLECOLUMNS + VIC6569_VISIBLECOLUMNS - 1, VIC6569_STARTVISIBLELINES, VIC6569_STARTVISIBLELINES + VIC6569_VISIBLELINES - 1);
				else
					video_screen_set_visarea(machine->primary_screen, VIC6567_STARTVISIBLECOLUMNS, VIC6567_STARTVISIBLECOLUMNS + VIC6567_VISIBLECOLUMNS - 1, VIC6567_STARTVISIBLELINES, VIC6567_STARTVISIBLELINES + VIC6567_VISIBLELINES - 1);
			}
			monitor = input_port_read(machine, "CFG") & 0x20;
		}
	}

	/* Lines 0-7 : common keyboard */
	for (i = 0; i < 8; i++)
	{
		value = 0xff;
		value &= ~input_port_read(machine, c64ports[i]);

		/* Shift Lock is mapped on Left Shift */
		if ((i == 1) && (input_port_read(machine, "SPECIAL") & 0x40))
			value &= ~0x80;			

		c64_keyline[i] = value;
	}


	value = 0xff;
	switch(controller1)
	{
		case 0x2000:
		case 0x6000:
			value &= ~(input_port_read(machine, "JOY0") & 0x1f);	/* Joy0 Directions + Button 1 */
			break;
		
		case 0x4000:
			if (input_port_read(machine, "PADDLE1") & 0x100)		/* Paddle1 Button */
				value &= ~0x08;
			if (input_port_read(machine, "PADDLE0") & 0x100)		/* Paddle0 Button */
				value &= ~0x04;
			break;

		case 0x8000:
			if (input_port_read(machine, "TRACKIPT") & 0x02)		/* Mouse Button Left */
				value &= ~0x10;
			if (input_port_read(machine, "TRACKIPT") & 0x01)		/* Mouse Button Right */
				value &= ~0x01;
			break;
			
		case 0xa000:
			break;

		default:
			logerror("Invalid Controller 1 Setting %d\n", controller1);
			break;
	}

	c64_keyline[8] = value;


	value = 0xff;
	switch(controller2)
	{
		case 0x0200:
		case 0x0600:
			value &= ~(input_port_read(machine, "JOY1") & 0x1f);	/* Joy1 Directions + Button 1 */
			break;
		
		case 0x0400:
			if (input_port_read(machine, "PADDLE3") & 0x100)		/* Paddle3 Button */
				value &= ~0x08;
			if (input_port_read(machine, "PADDLE2") & 0x100)		/* Paddle2 Button */
				value &= ~0x04;
			break;

		case 0x0800:
			if (input_port_read(machine, "TRACKIPT") & 0x02)		/* Mouse Button Left */
				value &= ~0x10;
			if (input_port_read(machine, "TRACKIPT") & 0x01)		/* Mouse Button Right */
				value &= ~0x01;
			break;

		default:
			logerror("Invalid Controller 2 Setting %d\n", controller2);
			break;
	}

	c64_keyline[9] = value;

	/* C128 only : keypad input ports */
	if (is_c128(machine)) 
	{
		for (i = 0; i < 3; i++)
		{
			value = 0xff;
			value &= ~input_port_read(machine, c128ports[i]);
			c128_keyline[i] = value;
		}
	}

	/* C65 only : function keys input ports */
	if (is_c65(machine)) 
	{
		value = 0xff;

		value &= ~input_port_read(machine, "FUNCT");
		c65_keyline = value;
	}

	vic2_frame_interrupt (machine, cpunum);

	set_led_status (1, input_port_read(machine, "SPECIAL") & 0x40 ? 1 : 0);		/* Shift Lock */
	set_led_status (0, input_port_read(machine, "DSW0") & 0x0100 ? 1 : 0);		/* Joystick Swap */ 
}


/***********************************************

	C64 Cartridges

***********************************************/

/* Info based on http://ist.uwaterloo.ca/~schepers/formats/CRT.TXT	    */
/* Please refer to the webpage for the latest version and for a very 
   complete listing of various cart types and their bankswitch tricks 	*/
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

	$0000-0040 :	Header of the whole .crt files
	$0040-EOF :		Blocks of data

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

static CBM_ROM c64_cbm_cart[0x20] = { {0} };
static INT8 cbm_c64_game;
static INT8 cbm_c64_exrom;

static DEVICE_IMAGE_UNLOAD(c64_cart)
{
	int index = image_index_in_device(image);
	c64_cbm_cart[index].size = 0;
	c64_cbm_cart[index].chip = 0;
}

static DEVICE_START(c64_cart)
{
	int index = image_index_in_device(device);
	if (index == 0)
	{
		cbm_c64_game = -1;
		cbm_c64_exrom = -1;
	}
}

/* Hardware Types for C64 carts */
enum {
	GENERIC_CRT = 0,	/* 00 - Normal cartridge					*/
	ACTION_REPLAY,		/* 01 - Action Replay						*/
	KCS_PC,				/* 02 - KCS Power Cartridge					*/
	FINAL_CART_III,		/* 03 - Final Cartridge III					*/
	SIMONS_BASIC,		/* 04 - Simons Basic						*/
	OCEAN_1,			/* 05 - Ocean type 1 (1)					*/
	EXPERT,				/* 06 - Expert Cartridge					*/
	FUN_PLAY,			/* 07 - Fun Play, Power Play				*/
	SUPER_GAMES,		/* 08 - Super Games							*/
	ATOMIC_POWER,		/* 09 - Atomic Power						*/
	EPYX_FASTLOAD,		/* 10 - Epyx Fastload						*/
	WESTERMANN,			/* 11 - Westermann Learning					*/
	REX,				/* 12 - Rex Utility							*/
	FINAL_CART_I,		/* 13 - Final Cartridge I					*/
	MAGIC_FORMEL,		/* 14 - Magic Formel						*/
	C64GS,				/* 15 - C64 Game System, System 3			*/
	WARPSPEED,			/* 16 - WarpSpeed							*/
	DINAMIC,			/* 17 - Dinamic (2)							*/
	ZAXXON,				/* 18 - Zaxxon, Super Zaxxon (SEGA)			*/
	DOMARK,				/* 19 - Magic Desk, Domark, HES Australia	*/
	SUPER_SNAP_5,		/* 20 - Super Snapshot 5					*/
	COMAL_80,			/* 21 - Comal-80							*/
	STRUCT_BASIC,		/* 22 - Structured Basic					*/
	ROSS,				/* 23 - Ross								*/
	DELA_EP64,			/* 24 - Dela EP64							*/
	DELA_EP7X8,			/* 25 - Dela EP7x8							*/
	DELA_EP256,			/* 26 - Dela EP256							*/
	REX_EP256,			/* 27 - Rex EP256							*/
	MIKRO_ASSMBLR,		/* 28 - Mikro Assembler						*/
	REAL_FC_I,			/* 29 - (3)									*/
	ACTION_REPLAY_4,	/* 30 - Action Replay 4						*/
	STARDOS,			/* 31 - StarDOS								*/
	/* 
	(1) Ocean type 1 includes Navy Seals, Robocop 2 & 3,  Shadow  of 
	the Beast, Toki, Terminator 2 and more. Both 256 and 128 Kb images.
	(2) Dinamic includes Narco Police and more.
	(3) Type 29 is reserved for the real Final Cartridge I, the one 
	above (Type 13) will become Final Cartridge II.					*/
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

static DEVICE_IMAGE_LOAD(c64_cart)
{
	int size = image_length(image), test, i;
	const char *filetype;
	int address = 0;

	filetype = image_filetype(image);

	/* We support .crt files */
	if (!mame_stricmp (filetype, "crt"))
	{
		int j;
		unsigned short cart_type;

		for (i=0; (i<sizeof(c64_cbm_cart) / sizeof(c64_cbm_cart[0])) && (c64_cbm_cart[i].size!=0); i++)
		;

		if (i >= sizeof(c64_cbm_cart) / sizeof(c64_cbm_cart[0]))
			return INIT_FAIL;

		/* Start to parse the .crt header */
		/* 0x16-0x17 is Hardware type */
		image_fseek(image, 0x16, SEEK_SET);
		image_fread(image, &cart_type, 2);
		cart_type = BIG_ENDIANIZE_INT16(cart_type);

		/* If it is unsupported cart type, warn the user */
		switch (cart_type)
		{
			case GENERIC_CRT:
				break;

			default:
				logerror("Currently unsupported cart type (Type %d)\n", cart_type);
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
		logerror("Header info: EXROM %d, GAME %d, Cart Type %d \n", cbm_c64_exrom, cbm_c64_game, cart_type);


		/* Data in a .crt image are organized in blocks called 'CHIP':
		   each 'CHIP' consists of a 0x10 header, which contains the 
		   actual size of the block, the loading address and info on 
		   the bankswitch, followed by the actual data					*/
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
			/* We currently don't use this, but it is needed to support more cart types. */
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
			c64_cbm_cart[i].size = chip_data_size;
			test = image_fread(image, c64_cbm_cart[i].chip, chip_data_size);

			if (test != chip_data_size)
				return INIT_FAIL;

			/* Advance to the next CHIP block */
			i++;
			j += chip_size;
		}
	}

	/* We also support .80 files for c64 & .e0/.f0 for max */
	else 
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
		test = image_fread(image, c64_cbm_cart[0].chip, c64_cbm_cart[0].size);

		if (test != c64_cbm_cart[0].size)
			return INIT_FAIL;
	}

	/* If we load a .crt file, use EXROM & GAME from the header! */
	if ((cbm_c64_exrom != -1) && (cbm_c64_game != -1)) 
	{
		c64_exrom = cbm_c64_exrom;
		c64_game  = cbm_c64_game;
	}

	/* Finally load the cart */
	if (ultimax) 
	{
		for (i=0; (i < sizeof(c64_cbm_cart) / sizeof(c64_cbm_cart[0])) && (c64_cbm_cart[i].size != 0); i++) 
				memcpy(c64_memory + c64_cbm_cart[i].addr, c64_cbm_cart[i].chip, c64_cbm_cart[i].size);
	}

	else
	{
		roml = c64_roml;
		romh = c64_romh;

		memset(roml, 0, 0x2000);
		memset(romh, 0, 0x2000);

		for (i=0; (i < sizeof(c64_cbm_cart) / sizeof(c64_cbm_cart[0])) && (c64_cbm_cart[i].size != 0); i++) 
		{
			if (c64_cbm_cart[i].addr < 0xc000) 
				memcpy(roml + c64_cbm_cart[i].addr - 0x8000, c64_cbm_cart[i].chip, c64_cbm_cart[i].size);

			else 
				memcpy(romh + c64_cbm_cart[i].addr - 0xe000, c64_cbm_cart[i].chip, c64_cbm_cart[i].size);
		}
	}

	return INIT_PASS;
}

void c64_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:				info->i = 2; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "crt,80"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:				info->start = DEVICE_START_NAME(c64_cart); break;
		case MESS_DEVINFO_PTR_LOAD:					info->load = DEVICE_IMAGE_LOAD_NAME(c64_cart); break;
		case MESS_DEVINFO_PTR_UNLOAD:				info->unload = DEVICE_IMAGE_UNLOAD_NAME(c64_cart); break;

		default:									cartslot_device_getinfo(devclass, state, info);
	}
}

void ultimax_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:				info->i = 1; break;
		case MESS_DEVINFO_INT_MUST_BE_LOADED:		info->i = 1; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "crt,e0,f0"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:				info->start = DEVICE_START_NAME(c64_cart); break;
		case MESS_DEVINFO_PTR_LOAD:					info->load = DEVICE_IMAGE_LOAD_NAME(c64_cart); break;
		case MESS_DEVINFO_PTR_UNLOAD:				info->unload = DEVICE_IMAGE_UNLOAD_NAME(c64_cart); break;

		default:									cartslot_device_getinfo(devclass, state, info); break;
	}
}
