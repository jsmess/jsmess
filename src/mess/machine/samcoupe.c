/***************************************************************************

    SAM Coupe Driver - Written By Lee Hammerton, Dirk Best

***************************************************************************/

#include "emu.h"
#include "includes/samcoupe.h"
#include "machine/msm6242.h"
#include "devices/messram.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define LMPR_RAM0    0x20	/* If bit set ram is paged into bank 0, else its rom0 */
#define LMPR_ROM1    0x40	/* If bit set rom1 is paged into bank 3, else its ram */
#define HMPR_MCNTRL  0x80	/* If set external RAM is enabled */


/***************************************************************************
    MEMORY BANKING
***************************************************************************/

static void samcoupe_update_bank(const address_space *space, int bank_num, UINT8 *memory, int is_readonly)
{
	char bank[10];
	sprintf(bank,"bank%d",bank_num);

	if (memory)
	{
		memory_set_bankptr(space->machine, bank, memory);
		memory_install_read_bank (space, ((bank_num-1) * 0x4000), ((bank_num-1) * 0x4000) + 0x3FFF, 0, 0, bank);
		if (is_readonly) {
			memory_unmap_write(space, ((bank_num-1) * 0x4000), ((bank_num-1) * 0x4000) + 0x3FFF, 0, 0);
		} else {
			memory_install_write_bank(space, ((bank_num-1) * 0x4000), ((bank_num-1) * 0x4000) + 0x3FFF, 0, 0, bank);
		}
	} else {
		memory_nop_readwrite(space, ((bank_num-1) * 0x4000), ((bank_num-1) * 0x4000) + 0x3FFF, 0, 0);
	}
}


static void samcoupe_install_ext_mem(const address_space *space)
{
	coupe_asic *asic = (coupe_asic *)space->machine->driver_data;
	UINT8 *mem;

	/* bank 3 */
	if (asic->lext >> 6 < messram_get_size(devtag_get_device(space->machine, "messram")) >> 20)
		mem = &messram_get_ptr(devtag_get_device(space->machine, "messram"))[(messram_get_size(devtag_get_device(space->machine, "messram")) & 0xfffff) + (asic->lext >> 6) * 0x100000 + (asic->lext & 0x3f) * 0x4000];
	else
		mem = NULL;

	samcoupe_update_bank(space, 3, mem, FALSE);

	/* bank 4 */
	if (asic->hext >> 6 < messram_get_size(devtag_get_device(space->machine, "messram")) >> 20)
		mem = &messram_get_ptr(devtag_get_device(space->machine, "messram"))[(messram_get_size(devtag_get_device(space->machine, "messram")) & 0xfffff) + (asic->hext >> 6) * 0x100000 + (asic->hext & 0x3f) * 0x4000];
	else
		mem = NULL;

	samcoupe_update_bank(space, 4, mem, FALSE);
}


void samcoupe_update_memory(const address_space *space)
{
	coupe_asic *asic = (coupe_asic *)space->machine->driver_data;
	const int PAGE_MASK = ((messram_get_size(devtag_get_device(space->machine, "messram")) & 0xfffff) / 0x4000) - 1;
	UINT8 *rom = memory_region(space->machine, "maincpu");
	UINT8 *memory;
	int is_readonly;

	/* BANK1 */
    if (asic->lmpr & LMPR_RAM0)   /* Is ram paged in at bank 1 */
	{
		if ((asic->lmpr & 0x1F) <= PAGE_MASK)
			memory = &messram_get_ptr(devtag_get_device(space->machine, "messram"))[(asic->lmpr & PAGE_MASK) * 0x4000];
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
		memory = &messram_get_ptr(devtag_get_device(space->machine, "messram"))[((asic->lmpr + 1) & PAGE_MASK) * 0x4000];
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
			memory = &messram_get_ptr(devtag_get_device(space->machine, "messram"))[(asic->hmpr & PAGE_MASK)*0x4000];
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
				memory = &messram_get_ptr(devtag_get_device(space->machine, "messram"))[((asic->hmpr + 1) & PAGE_MASK) * 0x4000];
			else
				memory = NULL;	/* Attempt to page in non existant ram region */
			is_readonly = FALSE;
		}
		samcoupe_update_bank(space, 4, memory, FALSE);
	}

	/* video memory location */
	if (asic->vmpr & 0x40)	/* if bit set in 2 bank screen mode */
		space->machine->generic.videoram.u8 = &messram_get_ptr(devtag_get_device(space->machine, "messram"))[((asic->vmpr & 0x1e) & PAGE_MASK) * 0x4000];
	else
		space->machine->generic.videoram.u8 = &messram_get_ptr(devtag_get_device(space->machine, "messram"))[((asic->vmpr & 0x1f) & PAGE_MASK) * 0x4000];
}


WRITE8_HANDLER( samcoupe_ext_mem_w )
{
	const address_space *space_program = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	coupe_asic *asic = (coupe_asic *)space->machine->driver_data;

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


/***************************************************************************
    REAL TIME CLOCK
***************************************************************************/

static READ8_DEVICE_HANDLER( samcoupe_rtc_r )
{
	return msm6242_r(device, offset >> 12);
}


static WRITE8_DEVICE_HANDLER( samcoupe_rtc_w )
{
	msm6242_w(device, offset >> 12, data);
}


/***************************************************************************
    MOUSE
***************************************************************************/

static TIMER_CALLBACK( samcoupe_mouse_reset )
{
	coupe_asic *asic = (coupe_asic *)machine->driver_data;
	asic->mouse_index = 0;
}

UINT8 samcoupe_mouse_r(running_machine *machine)
{
	coupe_asic *asic = (coupe_asic *)machine->driver_data;
	UINT8 result;

	/* on a read, reset the timer */
	timer_adjust_oneshot(asic->mouse_reset, ATTOTIME_IN_USEC(50), 0);

	/* update when we are about to read the first real values */
	if (asic->mouse_index == 2)
	{
		/* update values */
		int mouse_x = input_port_read(machine, "mouse_x");
		int mouse_y = input_port_read(machine, "mouse_y");

		int mouse_dx = asic->mouse_x - mouse_x;
		int mouse_dy = asic->mouse_y - mouse_y;

		asic->mouse_x = mouse_x;
		asic->mouse_y = mouse_y;

		/* button state */
		asic->mouse_data[2] = input_port_read(machine, "mouse_buttons");

		/* y-axis */
		asic->mouse_data[3] = (mouse_dy & 0xf00) >> 8;
		asic->mouse_data[4] = (mouse_dy & 0x0f0) >> 4;
		asic->mouse_data[5] = (mouse_dy & 0x00f) >> 0;

		/* x-axis */
		asic->mouse_data[6] = (mouse_dx & 0xf00) >> 8;
		asic->mouse_data[7] = (mouse_dx & 0x0f0) >> 4;
		asic->mouse_data[8] = (mouse_dx & 0x00f) >> 0;
	}

	/* get current value */
	result = asic->mouse_data[asic->mouse_index++];

	/* reset if we are at the end */
	if (asic->mouse_index == sizeof(asic->mouse_data))
		asic->mouse_index = 1;

	return result;
}

MACHINE_START( samcoupe )
{
	coupe_asic *asic = (coupe_asic *)machine->driver_data;
	asic->mouse_reset = timer_alloc(machine, samcoupe_mouse_reset, 0);
}

/***************************************************************************
    RESET
***************************************************************************/

MACHINE_RESET( samcoupe )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	const address_space *spaceio = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_IO);
	coupe_asic *asic = (coupe_asic *)machine->driver_data;

	/* initialize asic */
	asic->lmpr = 0x0f;      /* ROM0 paged in, ROM1 paged out RAM Banks */
	asic->hmpr = 0x01;
	asic->vmpr = 0x81;
	asic->line_int = 0xff;  /* line interrupts disabled */
	asic->status = 0x1f;    /* no interrupts active */

	/* initialize mouse */
	asic->mouse_index = 0;
	asic->mouse_data[0] = 0xff;
	asic->mouse_data[1] = 0xff;

	if (input_port_read(machine, "config") & 0x01)
	{
		/* install RTC */
		const device_config *rtc = devtag_get_device(machine, "sambus_clock");
		memory_install_readwrite8_device_handler(spaceio, rtc, 0xef, 0xef, 0xffff, 0xff00, samcoupe_rtc_r, samcoupe_rtc_w);
	}
	else
	{
		/* no RTC support */
		memory_unmap_readwrite(spaceio, 0xef, 0xef, 0xffff, 0xff00);
	}

	/* initialize memory */
	samcoupe_update_memory(space);
}
