/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "includes/samcoupe.h"
#include "machine/msm6242.h"


#define LMPR_RAM0    0x20	/* If bit set ram is paged into bank 0, else its rom0 */
#define LMPR_ROM1    0x40	/* If bit set rom1 is paged into bank 3, else its ram */
#define HMPR_MCNTRL  0x80	/* If set external RAM is enabled */


static void samcoupe_update_bank(const address_space *space, int bank, UINT8 *memory, int is_readonly)
{
	read8_space_func rh = SMH_NOP;
	write8_space_func wh = SMH_NOP;

	if (memory)
	{
		memory_set_bankptr(space->machine, bank, memory);
		rh = (read8_space_func) (STATIC_BANK1 + (FPTR)bank - 1);
		wh = is_readonly ? SMH_UNMAP : (write8_space_func) (STATIC_BANK1 + (FPTR)bank - 1);
	}

	memory_install_read8_handler(space, ((bank-1) * 0x4000), ((bank-1) * 0x4000) + 0x3FFF, 0, 0, rh);
	memory_install_write8_handler(space, ((bank-1) * 0x4000), ((bank-1) * 0x4000) + 0x3FFF, 0, 0, wh);
}


static void samcoupe_install_ext_mem(const address_space *space)
{
	coupe_asic *asic = space->machine->driver_data;
	UINT8 *mem;

	/* bank 3 */
	if (asic->lext >> 6 < mess_ram_size >> 20)
		mem = &mess_ram[(mess_ram_size & 0xfffff) + (asic->lext >> 6) * 0x100000 + (asic->lext & 0x3f) * 0x4000];
	else
		mem = NULL;

	samcoupe_update_bank(space, 3, mem, FALSE);

	/* bank 4 */
	if (asic->hext >> 6 < mess_ram_size >> 20)
		mem = &mess_ram[(mess_ram_size & 0xfffff) + (asic->hext >> 6) * 0x100000 + (asic->hext & 0x3f) * 0x4000];
	else
		mem = NULL;

	samcoupe_update_bank(space, 4, mem, FALSE);
}


void samcoupe_update_memory(const address_space *space)
{
	coupe_asic *asic = space->machine->driver_data;
	const int PAGE_MASK = ((mess_ram_size & 0xfffff) / 0x4000) - 1;
	UINT8 *rom = memory_region(space->machine, "maincpu");
	UINT8 *memory;
	int is_readonly;

	/* BANK1 */
    if (asic->lmpr & LMPR_RAM0)   /* Is ram paged in at bank 1 */
	{
		if ((asic->lmpr & 0x1F) <= PAGE_MASK)
			memory = &mess_ram[(asic->lmpr & PAGE_MASK) * 0x4000];
		else
			memory = NULL;	/* Attempt to page in non existant ram region */
		is_readonly = FALSE;
	}
	else
	{
		memory = rom;	/* Rom0 paged in */
		is_readonly = TRUE;
	}
	samcoupe_update_bank(space, 1, memory, is_readonly);


	/* BANK2 */
	if (((asic->lmpr + 1) & 0x1f) <= PAGE_MASK)
		memory = &mess_ram[((asic->lmpr + 1) & PAGE_MASK) * 0x4000];
	else
		memory = NULL;	/* Attempt to page in non existant ram region */
	samcoupe_update_bank(space, 2, memory, FALSE);

	/* only update bank 3 and 4 when external memory is not enabled */
	if (asic->hmpr & HMPR_MCNTRL)
	{
		samcoupe_install_ext_mem(space);
	}
	else
	{
		/* BANK3 */
		if ((asic->hmpr & 0x1F) <= PAGE_MASK )
			memory = &mess_ram[(asic->hmpr & PAGE_MASK)*0x4000];
		else
			memory = NULL;	/* Attempt to page in non existant ram region */
		samcoupe_update_bank(space, 3, memory, FALSE);


		/* BANK4 */
		if (asic->lmpr & LMPR_ROM1)	/* Is Rom1 paged in at bank 4 */
		{
			memory = rom + 0x4000;
			is_readonly = TRUE;
		}
		else
		{
			if (((asic->hmpr + 1) & 0x1f) <= PAGE_MASK)
				memory = &mess_ram[((asic->hmpr + 1) & PAGE_MASK) * 0x4000];
			else
				memory = NULL;	/* Attempt to page in non existant ram region */
			is_readonly = FALSE;
		}
		samcoupe_update_bank(space, 4, memory, FALSE);
	}

	/* video memory location */
	if (asic->vmpr & 0x40)	/* if bit set in 2 bank screen mode */
		videoram = &mess_ram[((asic->vmpr & 0x1e) & PAGE_MASK) * 0x4000];
	else
		videoram = &mess_ram[((asic->vmpr & 0x1f) & PAGE_MASK) * 0x4000];
}


WRITE8_HANDLER( samcoupe_ext_mem_w )
{
	const address_space *space_program = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	coupe_asic *asic = space->machine->driver_data;

	if (offset & 1)
		asic->hext = data;
	else
		asic->lext = data;

	/* external RAM enabled? */
	if (asic->hmpr & HMPR_MCNTRL)
	{
		samcoupe_install_ext_mem(space_program);
	}
}


static READ8_DEVICE_HANDLER( samcoupe_rtc_r )
{
	return msm6242_r(device, offset >> 12);
}


static WRITE8_DEVICE_HANDLER( samcoupe_rtc_w )
{
	msm6242_w(device, offset >> 12, data);
}


MACHINE_RESET( samcoupe )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	const address_space *spaceio = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_IO);
	coupe_asic *asic = machine->driver_data;

	asic->lmpr = 0x0f;      /* ROM0 paged in, ROM1 paged out RAM Banks */
	asic->hmpr = 0x01;
	asic->vmpr = 0x81;
	asic->line_int = 0xff;  /* line interrupts disabled */
	asic->status = 0x1f;    /* no interrupts active */

	if (input_port_read(machine, "config") & 0x01)
	{
		/* install RTC */
		const device_config *rtc = devtag_get_device(machine, "rtc");
		memory_install_readwrite8_device_handler(spaceio, rtc, 0xef, 0xef, 0xffff, 0xff00, samcoupe_rtc_r, samcoupe_rtc_w);
	}
	else
	{
		/* no RTC support */
		memory_install_readwrite8_handler(spaceio, 0xef, 0xef, 0xffff, 0xff00, SMH_UNMAP, SMH_UNMAP);
	}

	samcoupe_update_memory(space);
}
