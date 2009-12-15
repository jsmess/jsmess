/***************************************************************************

    Amiga cartridge emulation

TODO:
- Investigate why the AR2/3 sometimes garble the video on exit from the cart
- Add Nordic Power and similar carts
- Add HRTMon A1200 cart

***************************************************************************/


#include "driver.h"
#include "includes/amiga.h"
#include "cpu/m68000/m68000.h"
#include "machine/6526cia.h"
#include "machine/amigacrt.h"

enum
{
	/* supported cartridges */
	ACTION_REPLAY				=	0,
	ACTION_REPLAY_MKII,
	ACTION_REPLAY_MKIII
};

static int amiga_cart_type;

/***************************************************************************

  Utilities

***************************************************************************/

static int check_kickstart_12_13( running_machine *machine, const char *cart_name )
{
	UINT16 * ksmem = (UINT16 *)memory_region( machine, "user1" );

	if ( ksmem[2] == 0x00FC )
		return 1;

	logerror( "%s requires Kickstart version 1.2 or 1.3 - Cart not installed\n", cart_name );

	return 0;
}

/***************************************************************************

  Amiga Action Replay 1

  Whenever you push the button, an NMI interrupt is generated.

  The cartridge protects itself from being removed from memory by
  generating an interrupt whenever either the spurious interrupt
  vector is written at $60, or the nmi interrupt vector is written at
  $7c. If the spurious interrupt vector at $60 is written, then
  a NMI is generated, and the spurious irq vector is restored. If the
  NMI vector at $7c is written, then a spurious interrupt is generated,
  and the NMI vector is restored.

  When breakpoints are set, the target address is replaced by a trap
  instruction, and the original contents of the address are kept in
  the cartridge's RAM. A trap handler is installed pointing to $40
  in RAM, where code that clears memory address $60 followed by two
  nops are written. There seems to be a small delay between the write
  to $60 and until the actual NMI is generated, since the cart expects
  the PC to be at $46 (at the second nop).

***************************************************************************/

static int amiga_ar1_spurious;

static IRQ_CALLBACK(amiga_ar1_irqack)
{
	if ( irqline == 7 && amiga_ar1_spurious )
	{
		return M68K_INT_ACK_SPURIOUS;
	}

	return (24+irqline);
}

static TIMER_CALLBACK( amiga_ar1_delayed_nmi )
{
	(void)param;
	cputag_set_input_line(machine, "maincpu", 7, PULSE_LINE);
}

static void amiga_ar1_nmi( running_machine *machine )
{
	/* get the cart's built-in ram */
	UINT16 *ar_ram = (UINT16 *)memory_get_write_ptr(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x9fc000);

	if ( ar_ram != NULL )
	{
		int i;

		/* copy custom register values */
		for( i = 0; i < 256; i++ )
			ar_ram[0x1800+i] = CUSTOM_REG(i);

		/* trigger NMI irq */
		amiga_ar1_spurious = 0;
		timer_set(machine,  cputag_clocks_to_attotime(machine, "maincpu", 28), NULL, 0, amiga_ar1_delayed_nmi );
	}
}

static WRITE16_HANDLER( amiga_ar1_chipmem_w )
{
	int pc = cpu_get_pc(space->cpu);

	/* see if we're inside the AR1 rom */
	if ( ((pc >> 16) & 0xff ) != 0xf0 )
	{
		/* if we're not, see if either the Spurious IRQ vector
           or the NMI vector are being overwritten */
		if ( offset == (0x60/2) || offset == (0x7c/2) )
		{
			/* trigger an NMI or spurious irq */
			amiga_ar1_spurious = (offset == 0x60/2) ? 0 : 1;
			timer_set(space->machine,  cputag_clocks_to_attotime(space->machine, "maincpu", 28), NULL, 0, amiga_ar1_delayed_nmi );
		}
	}

	amiga_chip_ram_w( offset * 2, data );
}

static void amiga_ar1_check_overlay( running_machine *machine )
{
	memory_install_write16_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x000000, 0x00007f, 0, 0, amiga_ar1_chipmem_w);
}

static void amiga_ar1_init( running_machine *machine )
{
	void *ar_ram;

	/* check kickstart version */
	if ( !check_kickstart_12_13( machine, "Amiga Action Replay" ) )
	{
		amiga_cart_type = -1;
		return;
	}

	/* setup the cart ram */
	ar_ram = auto_alloc_array(machine, UINT8, 0x4000);
	memset(ar_ram, 0, 0x4000);

	/* Install ROM */
	memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf00000, 0xf7ffff, 0, 0, "bank2");
	memory_unmap_write(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xf00000, 0xf7ffff, 0, 0);

	/* Install RAM */
	memory_install_readwrite_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x9fc000, 0x9fffff, 0, 0, "bank3");

	/* Configure Banks */
	memory_set_bankptr(machine, "bank2", memory_region(machine, "user2"));
	memory_set_bankptr(machine, "bank3", ar_ram);

	amiga_ar1_spurious = 0;

	/* Install IRQ ACK callback */
	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), amiga_ar1_irqack);
}

/***************************************************************************

  Amiga Action Replay MKII/MKIII

  Whenever you push the button, an overlay of the cartridge appears
  in RAM and an NMI interrupt is generated. Only reads are overlayed.
  Writes go directly to chipmem (so that the CPU can write the stack
  information for the NMI). Once the interrupt is taken, the cart
  disables the ROM overlay and checks it's internal mode register
  to see why it was invoked.

  When a breakpoint is set the cart replaces the target instruction
  with a trap, and adds a trap handler that it locates in the $100-$120
  area. The handler simply has one instruction: TST.B $BFE001. This cia
  access is monitored, and the conditions match (PC < 120 and a previous
  command requested the monitoring of the cia) then the cart is invoked.

***************************************************************************/

static UINT16 amiga_ar23_mode;

static void amiga_ar23_freeze( running_machine *machine );

static READ16_HANDLER( amiga_ar23_cia_r )
{
	int pc = cpu_get_pc(space->cpu);

	if ( ACCESSING_BITS_0_7 && offset == 2048 && pc >= 0x40 && pc < 0x120 )
	{
		amiga_ar23_freeze(space->machine);
	}

	return amiga_cia_r( space, offset, mem_mask );
}

static WRITE16_HANDLER( amiga_ar23_mode_w )
{
	if ( data & 2 )
	{
		memory_install_read16_handler(space, 0xbfd000, 0xbfefff, 0, 0, amiga_ar23_cia_r);
	}
	else
	{
		memory_install_read16_handler(space, 0xbfd000, 0xbfefff, 0, 0, amiga_cia_r);
	}

	amiga_ar23_mode = (data&0x3);
	if ( amiga_ar23_mode == 0 )
		amiga_ar23_mode = 1;

}

static READ16_HANDLER( amiga_ar23_mode_r )
{
	UINT16 *mem = (UINT16 *)memory_region( space->machine, "user2" );

	if ( ACCESSING_BITS_0_7 )
	{
		if ( offset < 2 )
			return (mem[offset] | (amiga_ar23_mode&3));

		if ( offset == 0x03 ) /* disable cart oberlay on chip mem */
		{
			UINT32 mirror_mask = amiga_chip_ram_size;

			memory_set_bank(space->machine, "bank1", 0);

			while( (mirror_mask<<1) < 0x100000 )
			{
				mirror_mask |= ( mirror_mask << 1 );
			}

			/* overlay disabled, map RAM on 0x000000 */
			memory_install_write_bank(space, 0x000000, amiga_chip_ram_size - 1, 0, mirror_mask, "bank1");
		}
	}

	return mem[offset];
}

static WRITE16_HANDLER( amiga_ar23_chipmem_w )
{
	if ( offset == (0x08/2) )
	{
		if ( amiga_ar23_mode & 1 )
			amiga_ar23_freeze(space->machine);
	}

	amiga_chip_ram_w( offset * 2, data );
}

static void amiga_ar23_freeze( running_machine *machine )
{
	int pc = cpu_get_pc(cputag_get_cpu(machine, "maincpu"));

	/* only freeze if we're not inside the cart's ROM */
	if ( ((pc >> 16) & 0xfe ) != 0x40 )
	{
		/* get the cart's built-in ram */
		UINT16 *ar_ram = (UINT16 *)memory_get_write_ptr(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x440000);

		if ( ar_ram != NULL )
		{
			int		i;

			for( i = 0; i < 0x100; i++ )
				ar_ram[0x7800+i] = CUSTOM_REG(i);
		}

		/* overlay the cart rom's in chipram */
		memory_set_bank(machine, "bank1", 2);

		/* writes go to chipram */
		memory_install_write16_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x000000, amiga_chip_ram_size - 1, 0, 0, amiga_ar23_chipmem_w);

		/* trigger NMI irq */
		cputag_set_input_line(machine, "maincpu", 7, PULSE_LINE);
	}
}

static void amiga_ar23_nmi( running_machine *machine )
{
	amiga_ar23_mode = 0;
	amiga_ar23_freeze(machine);
}

#if 0
static WRITE16_HANDLER( amiga_ar23_custom_w )
{
	int pc = cpu_get_pc(space->cpu);

	/* see if we're inside the AR2 rom */
	if ( ((pc >> 16) & 0xfe ) != 0x40 )
	{
		/* get the cart's built-in ram */
		UINT16 *ar_ram = (UINT16 *)memory_get_write_ptr(0, ADDRESS_SPACE_PROGRAM, 0x440000);

		if ( ar_ram != NULL )
		{
			ar_ram[0x7800+offset] = data;
		}
	}

	amiga_custom_w( offset, data, mem_mask );
}

static READ16_HANDLER( amiga_ar23_custom_r )
{
	UINT16 data = amiga_custom_r( offset, mem_mask );

	int pc = cpu_get_pc(space->cpu);

	/* see if we're inside the AR2 rom */
	if ( ((pc >> 16) & 0xfe ) != 0x40 )
	{
		/* get the cart's built-in ram */
		UINT16 *ar_ram = (UINT16 *)memory_get_write_ptr(0, ADDRESS_SPACE_PROGRAM, 0x440000);

		if ( ar_ram != NULL )
		{
			ar_ram[0x7800+offset] = data;
		}
	}

	return data;
}
#endif

static void amiga_ar23_check_overlay( running_machine *machine )
{
	amiga_ar23_mode = 3;
	memory_install_write16_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x000000, 0x00000f, 0, 0, amiga_ar23_chipmem_w);
}

static void amiga_ar23_init( running_machine *machine, int ar3 )
{
	UINT32 mirror = 0x20000, size = 0x1ffff;
	void *ar_ram;

	/* check kickstart version */
	if ( !check_kickstart_12_13( machine, "Action Replay MKII or MKIII" ) )
	{
		amiga_cart_type = -1;
		return;
	}

	/* setup the cart ram */
	ar_ram = auto_alloc_array_clear(machine, UINT8,0x10000);

	if ( ar3 )
	{
		mirror = 0;
		size = 0x3ffff;
	}

	/* Install ROM */
	memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x400000, 0x400000+size, 0, mirror, "bank2");
	memory_unmap_write(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x400000, 0x400000+size, 0, mirror);

	/* Install RAM */
	memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x440000, 0x44ffff, 0, 0, "bank3");
	memory_install_write_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x440000, 0x44ffff, 0, 0, "bank3");

	/* Install Custom chip monitor */
//  memory_install_read16_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xdff000, 0xdff1ff, 0, 0, amiga_ar23_custom_r);
//  memory_install_write16_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xdff000, 0xdff1ff, 0, 0, amiga_ar23_custom_w);

	/* Install status/mode handlers */
	memory_install_read16_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x400000, 0x400007, 0, mirror, amiga_ar23_mode_r);
	memory_install_write16_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x400000, 0x400003, 0, mirror, amiga_ar23_mode_w);

	/* Configure Banks */
	memory_set_bankptr(machine, "bank2", memory_region(machine, "user2"));
	memory_set_bankptr(machine, "bank3", ar_ram);

	memory_configure_bank(machine, "bank1", 0, 2, amiga_chip_ram, 0);
	memory_configure_bank(machine, "bank1", 1, 2, memory_region(machine, "user1"), 0);
	memory_configure_bank(machine, "bank1", 2, 2, memory_region(machine, "user2"), 0);

	amiga_ar23_mode = 3;
}

/***************************************************************************

    MAME/MESS hooks

***************************************************************************/

void amiga_cart_init( running_machine *machine )
{
	/* see what is there */
	UINT16 *mem = (UINT16 *)memory_region( machine, "user2" );

	amiga_cart_type = -1;

	if ( mem != NULL )
	{
		if ( mem[0x00] == 0x1111 )
		{
			amiga_cart_type = ACTION_REPLAY;
			amiga_ar1_init(machine);
		}
		else if ( mem[0x0C] == 0x4D6B )
		{
			amiga_cart_type = ACTION_REPLAY_MKII;
			amiga_ar23_init(machine, 0);
		}
		else if ( mem[0x0C] == 0x4D4B )
		{
			amiga_cart_type = ACTION_REPLAY_MKIII;
			amiga_ar23_init(machine, 1);
		}
	}
}

void amiga_cart_check_overlay( running_machine *machine )
{
	if ( amiga_cart_type < 0 )
		return;

	switch( amiga_cart_type )
	{
		case ACTION_REPLAY:
			amiga_ar1_check_overlay(machine);
		break;

		case ACTION_REPLAY_MKII:
		case ACTION_REPLAY_MKIII:
			amiga_ar23_check_overlay(machine);
		break;
	}
}

void amiga_cart_nmi( running_machine *machine )
{
	if ( amiga_cart_type < 0 )
		return;

	switch( amiga_cart_type )
	{
		case ACTION_REPLAY:
			amiga_ar1_nmi(machine);
		break;

		case ACTION_REPLAY_MKII:
		case ACTION_REPLAY_MKIII:
			amiga_ar23_nmi(machine);
		break;
	}
}
