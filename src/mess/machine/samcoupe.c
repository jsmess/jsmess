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

static void samcoupe_update_bank(address_space *space, int bank_num, UINT8 *memory, int is_readonly)
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


static void samcoupe_install_ext_mem(address_space *space)
{
	samcoupe_state *state = space->machine->driver_data<samcoupe_state>();
	UINT8 *mem;

	/* bank 3 */
	if (state->lext >> 6 < messram_get_size(space->machine->device("messram")) >> 20)
		mem = &messram_get_ptr(space->machine->device("messram"))[(messram_get_size(space->machine->device("messram")) & 0xfffff) + (state->lext >> 6) * 0x100000 + (state->lext & 0x3f) * 0x4000];
	else
		mem = NULL;

	samcoupe_update_bank(space, 3, mem, FALSE);

	/* bank 4 */
	if (state->hext >> 6 < messram_get_size(space->machine->device("messram")) >> 20)
		mem = &messram_get_ptr(space->machine->device("messram"))[(messram_get_size(space->machine->device("messram")) & 0xfffff) + (state->hext >> 6) * 0x100000 + (state->hext & 0x3f) * 0x4000];
	else
		mem = NULL;

	samcoupe_update_bank(space, 4, mem, FALSE);
}


void samcoupe_update_memory(address_space *space)
{
	samcoupe_state *state = space->machine->driver_data<samcoupe_state>();
	const int PAGE_MASK = ((messram_get_size(space->machine->device("messram")) & 0xfffff) / 0x4000) - 1;
	UINT8 *rom = space->machine->region("maincpu")->base();
	UINT8 *memory;
	int is_readonly;

	/* BANK1 */
    if (state->lmpr & LMPR_RAM0)   /* Is ram paged in at bank 1 */
	{
		if ((state->lmpr & 0x1F) <= PAGE_MASK)
			memory = &messram_get_ptr(space->machine->device("messram"))[(state->lmpr & PAGE_MASK) * 0x4000];
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
	if (((state->lmpr + 1) & 0x1f) <= PAGE_MASK)
		memory = &messram_get_ptr(space->machine->device("messram"))[((state->lmpr + 1) & PAGE_MASK) * 0x4000];
	else
		memory = NULL;	/* Attempt to page in non existant ram region */
	samcoupe_update_bank(space, 2, memory, FALSE);

	/* only update bank 3 and 4 when external memory is not enabled */
	if (state->hmpr & HMPR_MCNTRL)
	{
		samcoupe_install_ext_mem(space);
	}
	else
	{
		/* BANK3 */
		if ((state->hmpr & 0x1F) <= PAGE_MASK )
			memory = &messram_get_ptr(space->machine->device("messram"))[(state->hmpr & PAGE_MASK)*0x4000];
		else
			memory = NULL;	/* Attempt to page in non existant ram region */
		samcoupe_update_bank(space, 3, memory, FALSE);


		/* BANK4 */
		if (state->lmpr & LMPR_ROM1)	/* Is Rom1 paged in at bank 4 */
		{
			memory = rom + 0x4000;
			is_readonly = TRUE;
		}
		else
		{
			if (((state->hmpr + 1) & 0x1f) <= PAGE_MASK)
				memory = &messram_get_ptr(space->machine->device("messram"))[((state->hmpr + 1) & PAGE_MASK) * 0x4000];
			else
				memory = NULL;	/* Attempt to page in non existant ram region */
			is_readonly = FALSE;
		}
		samcoupe_update_bank(space, 4, memory, FALSE);
	}

	/* video memory location */
	if (state->vmpr & 0x40)	/* if bit set in 2 bank screen mode */
		state->videoram = &messram_get_ptr(space->machine->device("messram"))[((state->vmpr & 0x1e) & PAGE_MASK) * 0x4000];
	else
		state->videoram = &messram_get_ptr(space->machine->device("messram"))[((state->vmpr & 0x1f) & PAGE_MASK) * 0x4000];
}


WRITE8_HANDLER( samcoupe_ext_mem_w )
{
	samcoupe_state *state = space->machine->driver_data<samcoupe_state>();
	address_space *space_program = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if (offset & 1)
		state->hext = data;
	else
		state->lext = data;

	/* external RAM enabled? */
	if (state->hmpr & HMPR_MCNTRL)
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
	samcoupe_state *state = machine->driver_data<samcoupe_state>();
	state->mouse_index = 0;
}

UINT8 samcoupe_mouse_r(running_machine *machine)
{
	samcoupe_state *state = machine->driver_data<samcoupe_state>();
	UINT8 result;

	/* on a read, reset the timer */
	timer_adjust_oneshot(state->mouse_reset, ATTOTIME_IN_USEC(50), 0);

	/* update when we are about to read the first real values */
	if (state->mouse_index == 2)
	{
		/* update values */
		int mouse_x = input_port_read(machine, "mouse_x");
		int mouse_y = input_port_read(machine, "mouse_y");

		int mouse_dx = state->mouse_x - mouse_x;
		int mouse_dy = state->mouse_y - mouse_y;

		state->mouse_x = mouse_x;
		state->mouse_y = mouse_y;

		/* button state */
		state->mouse_data[2] = input_port_read(machine, "mouse_buttons");

		/* y-axis */
		state->mouse_data[3] = (mouse_dy & 0xf00) >> 8;
		state->mouse_data[4] = (mouse_dy & 0x0f0) >> 4;
		state->mouse_data[5] = (mouse_dy & 0x00f) >> 0;

		/* x-axis */
		state->mouse_data[6] = (mouse_dx & 0xf00) >> 8;
		state->mouse_data[7] = (mouse_dx & 0x0f0) >> 4;
		state->mouse_data[8] = (mouse_dx & 0x00f) >> 0;
	}

	/* get current value */
	result = state->mouse_data[state->mouse_index++];

	/* reset if we are at the end */
	if (state->mouse_index == sizeof(state->mouse_data))
		state->mouse_index = 1;

	return result;
}

MACHINE_START( samcoupe )
{
	samcoupe_state *state = machine->driver_data<samcoupe_state>();
	state->mouse_reset = timer_alloc(machine, samcoupe_mouse_reset, 0);

	/* schedule our video updates */
	state->video_update_timer = timer_alloc(machine, sam_video_update_callback, NULL);
	timer_adjust_oneshot(state->video_update_timer, machine->primary_screen->time_until_pos(0, 0), 0);
}

/***************************************************************************
    RESET
***************************************************************************/

MACHINE_RESET( samcoupe )
{
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	address_space *spaceio = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_IO);
	samcoupe_state *state = machine->driver_data<samcoupe_state>();

	/* initialize state */
	state->lmpr = 0x0f;      /* ROM0 paged in, ROM1 paged out RAM Banks */
	state->hmpr = 0x01;
	state->vmpr = 0x81;
	state->line_int = 0xff;  /* line interrupts disabled */
	state->status = 0x1f;    /* no interrupts active */

	/* initialize mouse */
	state->mouse_index = 0;
	state->mouse_data[0] = 0xff;
	state->mouse_data[1] = 0xff;

	if (input_port_read(machine, "config") & 0x01)
	{
		/* install RTC */
		device_t *rtc = machine->device("sambus_clock");
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
