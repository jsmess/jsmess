/***************************************************************************

  apple2.c

  Machine file to handle emulation of the Apple II series.

  TODO:  Make a standard set of peripherals work.
  TODO:  Allow swappable peripherals in each slot.
  TODO:  Verify correctness of C08X switches.
            - need to do double-read before write-enable RAM

***************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "includes/apple2.h"
#include "machine/ap2_slot.h"
#include "machine/ay3600.h"
#include "machine/applefdc.h"
#include "devices/sonydriv.h"
#include "devices/appldriv.h"
#include "imagedev/flopdrv.h"
#include "image.h"
#include "imagedev/cassette.h"
#include "machine/ram.h"
#include "sound/speaker.h"
#include "profiler.h"

#ifdef MAME_DEBUG
#define VERBOSE 1
#else
#define VERBOSE 0
#endif /* MAME_DEBUG */

#define LOG(x)	do { if (VERBOSE) logerror x; } while (0)

#define PROFILER_C00X	PROFILER_USER2
#define PROFILER_C01X	PROFILER_USER2
#define PROFILER_C08X	PROFILER_USER2
#define PROFILER_A2INT	PROFILER_USER2

/* softswitch */

/* before the softswitch is changed, these are applied */


/* local */


static WRITE8_HANDLER ( apple2_mainram0400_w );
static WRITE8_HANDLER ( apple2_mainram2000_w );
static WRITE8_HANDLER ( apple2_auxram0400_w );
static WRITE8_HANDLER ( apple2_auxram2000_w );

static READ8_HANDLER ( apple2_c00x_r );
static READ8_HANDLER ( apple2_c01x_r );
static READ8_HANDLER ( apple2_c02x_r );
static READ8_HANDLER ( apple2_c03x_r );
static READ8_HANDLER ( apple2_c05x_r );
static READ8_HANDLER ( apple2_c06x_r );
static READ8_HANDLER ( apple2_c07x_r );

static WRITE8_HANDLER ( apple2_c00x_w );
static WRITE8_HANDLER ( apple2_c01x_w );
static WRITE8_HANDLER ( apple2_c02x_w );
static WRITE8_HANDLER ( apple2_c03x_w );
static WRITE8_HANDLER ( apple2_c05x_w );
static WRITE8_HANDLER ( apple2_c07x_w );

static READ8_HANDLER ( apple2_c1xx_r );
static WRITE8_HANDLER ( apple2_c1xx_w );
static READ8_HANDLER ( apple2_c4xx_r );
static WRITE8_HANDLER ( apple2_c4xx_w );

static READ8_HANDLER ( apple2_c800_r );
static WRITE8_HANDLER ( apple2_c800_w );
static READ8_HANDLER ( apple2_ce00_r );
static WRITE8_HANDLER ( apple2_ce00_w );

/* -----------------------------------------------------------------------
 * New Apple II memory manager
 * ----------------------------------------------------------------------- */



static READ8_HANDLER(read_floatingbus)
{
	return apple2_getfloatingbusvalue(space->machine());
}



void apple2_setup_memory(running_machine &machine, const apple2_memmap_config *config)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	state->m_mem_config = *config;
	state->m_current_meminfo = NULL;
	apple2_update_memory(machine);
}



void apple2_update_memory(running_machine &machine)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	address_space* space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	int i, bank;
	char rbank[10], wbank[10];
	int full_update = 0;
	apple2_meminfo meminfo;
	read8_space_func rh;
	write8_space_func wh;
	offs_t begin, end_r, end_w;
	UINT8 *rbase, *wbase, *rom, *slot_ram;
	UINT32 rom_length, slot_length, offset;
	bank_disposition_t bank_disposition;
	int wh_nop = 0;

	/* need to build list of current info? */
	if (!state->m_current_meminfo)
	{
		for (i = 0; state->m_mem_config.memmap[i].end; i++)
			;
		state->m_current_meminfo = auto_alloc_array(machine, apple2_meminfo, i);
		full_update = 1;
	}

	/* get critical info */
	rom = machine.region("maincpu")->base();
	rom_length = machine.region("maincpu")->bytes() & ~0xFFF;
	slot_length = machine.region("maincpu")->bytes() - rom_length;
	slot_ram = (slot_length > 0) ? &rom[rom_length] : NULL;

	/* loop through the entire memory map */
	bank = state->m_mem_config.first_bank;
	for (i = 0; state->m_mem_config.memmap[i].get_meminfo; i++)
	{
		/* retrieve information on this entry */
		memset(&meminfo, 0, sizeof(meminfo));
		state->m_mem_config.memmap[i].get_meminfo(machine, state->m_mem_config.memmap[i].begin, state->m_mem_config.memmap[i].end, &meminfo);

		bank_disposition = state->m_mem_config.memmap[i].bank_disposition;

		/* do we need to memory reading? */
		if (full_update
			|| (meminfo.read_mem != state->m_current_meminfo[i].read_mem)
			|| (meminfo.read_handler != state->m_current_meminfo[i].read_handler))
		{
			rbase = NULL;
			sprintf(rbank,"bank%d",bank);
			begin = state->m_mem_config.memmap[i].begin;
			end_r = state->m_mem_config.memmap[i].end;
			rh = NULL;

			LOG(("apple2_update_memory():  Updating RD {%06X..%06X} [#%02d] --> %08X\n",
				begin, end_r, bank, meminfo.read_mem));

			/* read handling */
			if (meminfo.read_handler)
			{
				/* handler */
				rh = meminfo.read_handler;
			}
			else if (meminfo.read_mem == APPLE2_MEM_FLOATING)
			{
				/* floating RAM */
				rh = read_floatingbus;
			}
			else if ((meminfo.read_mem & 0xC0000000) == APPLE2_MEM_AUX)
			{
				/* auxillary memory */
				assert(state->m_mem_config.auxmem);
				offset = meminfo.read_mem & APPLE2_MEM_MASK;
				rbase = &state->m_mem_config.auxmem[offset];
			}
			else if ((meminfo.read_mem & 0xC0000000) == APPLE2_MEM_SLOT)
			{
				// slots 1-2
				if ((meminfo.write_mem & APPLE2_MEM_MASK) == 0)
				{
					rh = apple2_c1xx_r;
				}
				else if ((meminfo.write_mem & APPLE2_MEM_MASK) == 0x300)
				{	// slots 4-7
					rh = apple2_c4xx_r;
				}
			}
			else if ((meminfo.read_mem & 0xC0000000) == APPLE2_MEM_ROM)
			{
				/* ROM */
				offset = meminfo.read_mem & APPLE2_MEM_MASK;
				rbase = &rom[offset % rom_length];
			}
			else
			{
				/* RAM */
				if (end_r >= ram_get_size(machine.device(RAM_TAG)))
					end_r = ram_get_size(machine.device(RAM_TAG)) - 1;
				offset = meminfo.read_mem & APPLE2_MEM_MASK;
				if (end_r >= begin)
					rbase = &ram_get_ptr(machine.device(RAM_TAG))[offset];
			}

			/* install the actual handlers */
			if (begin <= end_r) {
				if (rh) {
					space->install_legacy_read_handler(begin, end_r, FUNC(rh));
				} else {
					space->install_read_bank(begin, end_r, rbank);
				}
			}

			/* did we 'go past the end?' */
			if (end_r < state->m_mem_config.memmap[i].end)
				space->nop_read(end_r + 1, state->m_mem_config.memmap[i].end);

			/* set the memory bank */
			if (rbase)
			{
				memory_set_bankptr(machine, rbank, rbase);
			}

			/* record the current settings */
			state->m_current_meminfo[i].read_mem = meminfo.read_mem;
			state->m_current_meminfo[i].read_handler = meminfo.read_handler;
		}

		/* do we need to memory writing? */
		if (full_update
			|| (meminfo.write_mem != state->m_current_meminfo[i].write_mem)
			|| (meminfo.write_handler != state->m_current_meminfo[i].write_handler))
		{
			wbase = NULL;
			if (bank_disposition == A2MEM_MONO)
				sprintf(wbank,"bank%d",bank);
			else if (bank_disposition == A2MEM_DUAL)
				sprintf(wbank,"bank%d",bank+1);
			begin = state->m_mem_config.memmap[i].begin;
			end_w = state->m_mem_config.memmap[i].end;
			wh = NULL;

			LOG(("apple2_update_memory():  Updating WR {%06X..%06X} [#%02d] --> %08X\n",
				begin, end_w, bank, meminfo.write_mem));

			/* write handling */
			if (meminfo.write_handler)
			{
				/* handler */
				wh = meminfo.write_handler;
			}
			else if ((meminfo.write_mem & 0xC0000000) == APPLE2_MEM_AUX)
			{
				/* auxillary memory */
				assert(state->m_mem_config.auxmem);
				offset = meminfo.write_mem & APPLE2_MEM_MASK;
				wbase = &state->m_mem_config.auxmem[offset];
			}
			else if ((meminfo.write_mem & 0xC0000000) == APPLE2_MEM_SLOT)
			{
				/* slot RAM/ROM */

				// slots 1-2
				if ((meminfo.write_mem & APPLE2_MEM_MASK) == 0)
				{
					wh = apple2_c1xx_w;
				}
				else if ((meminfo.write_mem & APPLE2_MEM_MASK) == 0x300)
				{	// slots 4-7
					wh = apple2_c4xx_w;
				}
			}
			else if ((meminfo.write_mem & 0xC0000000) == APPLE2_MEM_ROM)
			{
				/* ROM */
				wh_nop = 1;
			}
			else
			{
				/* RAM */
				if (end_w >= ram_get_size(machine.device(RAM_TAG)))
					end_w = ram_get_size(machine.device(RAM_TAG)) - 1;
				offset = meminfo.write_mem & APPLE2_MEM_MASK;
				if (end_w >= begin)
					wbase = &ram_get_ptr(machine.device(RAM_TAG))[offset];
			}


			/* install the actual handlers */
			if (begin <= end_w) {
				if (wh) {
					space->install_legacy_write_handler(begin, end_w, FUNC(wh));
				} else {
					if (wh_nop) {
						space->nop_write(begin, end_w);
					} else {
						space->install_write_bank(begin, end_w, wbank);
					}
				}
			}

			/* did we 'go past the end?' */
			if (end_w < state->m_mem_config.memmap[i].end)
				space->nop_write(end_w + 1, state->m_mem_config.memmap[i].end);

			/* set the memory bank */
			if (wbase)
			{
				memory_set_bankptr(machine, wbank, wbase);
			}

			/* record the current settings */
			state->m_current_meminfo[i].write_mem = meminfo.write_mem;
			state->m_current_meminfo[i].write_handler = meminfo.write_handler;
		}
		bank += bank_disposition;
	}
}



static STATE_POSTLOAD( apple2_update_memory_postload )
{
	apple2_update_memory(machine);
}



/* -----------------------------------------------------------------------
 * Apple II memory map
 * ----------------------------------------------------------------------- */

READ8_HANDLER(apple2_c0xx_r)
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	if(!space->debugger_access())
	{
		static const read8_space_func handlers[] =
		{
			apple2_c00x_r,
			apple2_c01x_r,
			apple2_c02x_r,
			apple2_c03x_r,
			NULL,
			apple2_c05x_r,
			apple2_c06x_r,
			apple2_c07x_r
		};
		UINT8 result = 0x00;
		int slotnum;
		device_t *slotdevice;

		offset &= 0xFF;

		if (offset < 0x80)
		{
			if (state->m_a2_cnxx_slot != -1)
			{
				state->m_a2_cnxx_slot = -1;
				apple2_update_memory(space->machine());
			}

			/* normal handler */
			if (handlers[offset / 0x10])
				result = handlers[offset / 0x10](space, offset % 0x10);
		}
		else
		{
			/* slot handler; identify the slot number */
			slotnum = (offset - 0x80) / 0x10;

			/* now identify the device */
			slotdevice = apple2_slot(space->machine(), slotnum);

			/* and if we can, read from the slot */
			if (slotdevice != NULL)
				result = apple2_slot_r(slotdevice, offset % 0x10);
		}
		return result;
	}

	return 0;
}



WRITE8_HANDLER(apple2_c0xx_w)
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	static const write8_space_func handlers[] =
	{
		apple2_c00x_w,
		apple2_c01x_w,
		apple2_c02x_w,
		apple2_c03x_w,
		NULL,
		apple2_c05x_w,
		NULL,
		apple2_c07x_w
	};
	int slotnum;
	device_t *slotdevice;

	offset &= 0xFF;

	if (offset < 0x80)
	{
		if (state->m_a2_cnxx_slot != -1)
		{
			state->m_a2_cnxx_slot = -1;
			apple2_update_memory(space->machine());
		}

		/* normal handler */
		if (handlers[offset / 0x10])
			handlers[offset / 0x10](space, offset % 0x10, data);
	}
	else
	{
		/* slot handler; identify the slot number */
		slotnum = (offset - 0x80) / 0x10;

		/* now identify the device */
		slotdevice = apple2_slot(space->machine(), slotnum);

		/* and if we can, write to the slot */
		if (slotdevice != NULL)
			apple2_slot_w(slotdevice, offset % 0x10, data);
	}
}

/* returns default CnXX slotram for a slot space */
INT8 apple2_slotram_r(running_machine &machine, int slotnum, int offset)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	UINT8 *rom, *slot_ram;
	UINT32 rom_length, slot_length;

	// find slot_ram if any
	rom = machine.region("maincpu")->base();
	rom_length = machine.region("maincpu")->bytes() & ~0xFFF;
	slot_length = machine.region("maincpu")->bytes() - rom_length;
	slot_ram = (slot_length > 0) ? &rom[rom_length] : NULL;

	if (slot_ram)
	{
		if (state->m_a2_cnxx_slot == -1)
		{
			state->m_a2_cnxx_slot = slotnum;
			apple2_update_memory(machine);
		}

		return slot_ram[offset];
	}

	// else fall through to floating bus
	return apple2_getfloatingbusvalue(machine);
}

static READ8_HANDLER( apple2_c1xx_r )
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	int slotnum;
	device_t *slotdevice;

	slotnum = ((offset>>8) & 0xf) + 1;
	slotdevice = apple2_slot(space->machine(), slotnum);

	if (state->m_a2_cnxx_slot == -1)
	{
		state->m_a2_cnxx_slot = slotnum;
		apple2_update_memory(space->machine());
	}

	if (slotdevice != NULL)
	{
		return apple2_slot_ROM_r(slotdevice, offset&0xff);
	}
	else
	{
		return apple2_slotram_r(space->machine(), slotnum, offset);
	}

	// else fall through to floating bus
	return apple2_getfloatingbusvalue(space->machine());
}

static WRITE8_HANDLER ( apple2_c1xx_w )
{
	int slotnum;
	device_t *slotdevice;
	UINT8 *rom, *slot_ram;
	UINT32 rom_length, slot_length;

	// find slot_ram if any
	rom = space->machine().region("maincpu")->base();
	rom_length = space->machine().region("maincpu")->bytes() & ~0xFFF;
	slot_length = space->machine().region("maincpu")->bytes() - rom_length;
	slot_ram = (slot_length > 0) ? &rom[rom_length] : NULL;

	slotnum = ((offset>>8) & 0xf) + 1;

	slotdevice = apple2_slot(space->machine(), slotnum);

	if (slotdevice != NULL)
	{
		apple2_slot_ROM_w(slotdevice, offset&0xff, data);
	}
	else
	{
		if (slot_ram)
			slot_ram[offset] = data;
	}
}

static READ8_HANDLER( apple2_c4xx_r )
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	int slotnum;
	device_t *slotdevice;

	slotnum = ((offset>>8) & 0xf) + 4;
	slotdevice = apple2_slot(space->machine(), slotnum);

	if (state->m_a2_cnxx_slot == -1)
	{
		state->m_a2_cnxx_slot = slotnum;
		apple2_update_memory(space->machine());
	}

	// is a card installed in this slot?
	if (slotdevice != NULL)
	{
		return apple2_slot_ROM_r(slotdevice, offset&0xff);
	}
	else
	{
		return apple2_slotram_r(space->machine(), slotnum, offset);
	}

	// else fall through to floating bus
	return apple2_getfloatingbusvalue(space->machine());
}

static WRITE8_HANDLER ( apple2_c4xx_w )
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	int slotnum;
	device_t *slotdevice;
	UINT8 *rom, *slot_ram;
	UINT32 rom_length, slot_length;

	// find slot_ram if any
	rom = space->machine().region("maincpu")->base();
	rom_length = space->machine().region("maincpu")->bytes() & ~0xFFF;
	slot_length = space->machine().region("maincpu")->bytes() - rom_length;
	slot_ram = (slot_length > 0) ? &rom[rom_length] : NULL;

	slotnum = ((offset>>8) & 0xf) + 4;
	slotdevice = apple2_slot(space->machine(), slotnum);

	if (state->m_a2_cnxx_slot == -1)
	{
		state->m_a2_cnxx_slot = slotnum;
		apple2_update_memory(space->machine());
	}

	if (slotdevice != NULL)
	{
		apple2_slot_ROM_w(slotdevice, offset&0xff, data);
	}
	else
	{
		if (slot_ram)
			slot_ram[offset] = data;
	}
}

static READ8_HANDLER(apple2_cfff_r)
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	// debugger guard
	if(!space->debugger_access())
	{
		state->m_a2_cnxx_slot = -1;
		apple2_update_memory(space->machine());
	}

	return apple2_getfloatingbusvalue(space->machine());
}

static WRITE8_HANDLER(apple2_cfff_w)
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	state->m_a2_cnxx_slot = -1;
	apple2_update_memory(space->machine());
}

static READ8_HANDLER ( apple2_c800_r )
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	device_t *slotdevice;

	slotdevice = apple2_slot(space->machine(), state->m_a2_cnxx_slot);

	if (slotdevice != NULL)
	{
		return apple2_c800_slot_r(slotdevice, offset&0xfff);
	}

	return apple2_getfloatingbusvalue(space->machine());
}

static WRITE8_HANDLER ( apple2_c800_w )
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	device_t *slotdevice;

	slotdevice = apple2_slot(space->machine(), state->m_a2_cnxx_slot);

	if (slotdevice != NULL)
	{
		apple2_c800_slot_w(slotdevice, offset&0xfff, data);
	}
}

static READ8_HANDLER ( apple2_ce00_r )
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	device_t *slotdevice;

	slotdevice = apple2_slot(space->machine(), state->m_a2_cnxx_slot);

	if (slotdevice != NULL)
	{
		return apple2_c800_slot_r(slotdevice, (offset&0xfff) + 0x600);
	}

	return apple2_getfloatingbusvalue(space->machine());
}

static WRITE8_HANDLER ( apple2_ce00_w )
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	device_t *slotdevice;

	slotdevice = apple2_slot(space->machine(), state->m_a2_cnxx_slot);

	if (slotdevice != NULL)
	{
		apple2_c800_slot_w(slotdevice, (offset&0xfff)+0x600, data);
	}
}

static void apple2_mem_0000(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	meminfo->read_mem			= (state->m_flags & VAR_ALTZP)	? 0x010000 : 0x000000;
	meminfo->write_mem			= (state->m_flags & VAR_ALTZP)	? 0x010000 : 0x000000;
}

static void apple2_mem_0200(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	meminfo->read_mem			= (state->m_flags & VAR_RAMRD)	? 0x010200 : 0x000200;
	meminfo->write_mem			= (state->m_flags & VAR_RAMWRT)	? 0x010200 : 0x000200;
}

static void apple2_mem_0400(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	if (state->m_flags & VAR_80STORE)
	{
		meminfo->read_mem		= (state->m_flags & VAR_PAGE2)	? 0x010400 : 0x000400;
		meminfo->write_mem		= (state->m_flags & VAR_PAGE2)	? 0x010400 : 0x000400;
		meminfo->write_handler	= (state->m_flags & VAR_PAGE2)	? apple2_auxram0400_w : apple2_mainram0400_w;
	}
	else
	{
		meminfo->read_mem		= (state->m_flags & VAR_RAMRD)	? 0x010400 : 0x000400;
		meminfo->write_mem		= (state->m_flags & VAR_RAMWRT)	? 0x010400 : 0x000400;
		meminfo->write_handler	= (state->m_flags & VAR_RAMWRT)	? apple2_auxram0400_w : apple2_mainram0400_w;
	}
}

static void apple2_mem_0800(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	meminfo->read_mem			= (state->m_flags & VAR_RAMRD)	? 0x010800 : 0x000800;
	meminfo->write_mem			= (state->m_flags & VAR_RAMWRT)	? 0x010800 : 0x000800;
}

static void apple2_mem_2000(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	if ((state->m_flags & (VAR_80STORE|VAR_HIRES)) == (VAR_80STORE|VAR_HIRES))
	{
		meminfo->read_mem		= (state->m_flags & VAR_PAGE2)	? 0x012000 : 0x002000;
		meminfo->write_mem		= (state->m_flags & VAR_PAGE2)	? 0x012000 : 0x002000;
		meminfo->write_handler	= (state->m_flags & VAR_PAGE2)	? apple2_auxram2000_w : apple2_mainram2000_w;
	}
	else
	{
		meminfo->read_mem		= (state->m_flags & VAR_RAMRD)	? 0x012000 : 0x002000;
		meminfo->write_mem		= (state->m_flags & VAR_RAMWRT)	? 0x012000 : 0x002000;
		meminfo->write_handler	= (state->m_flags & VAR_RAMWRT)	? apple2_auxram2000_w : apple2_mainram2000_w;
	}
}

static void apple2_mem_4000(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	meminfo->read_mem			= (state->m_flags & VAR_RAMRD)	? 0x014000 : 0x004000;
	meminfo->write_mem			= (state->m_flags & VAR_RAMWRT)	? 0x014000 : 0x004000;
}

static void apple2_mem_C000(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	meminfo->read_handler = apple2_c0xx_r;
	meminfo->write_handler = apple2_c0xx_w;
}

static void apple2_mem_CFFF(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	meminfo->read_handler = apple2_cfff_r;
	meminfo->write_handler = apple2_cfff_w;
}

static void apple2_mem_Cx00(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	if (state->m_flags & VAR_INTCXROM)
	{
		meminfo->read_mem		= (begin & 0x0FFF) | (state->m_flags & VAR_ROMSWITCH ? 0x4000 : 0x0000) | APPLE2_MEM_ROM;
		meminfo->write_mem		= APPLE2_MEM_FLOATING;
	}
	else
	{
		meminfo->read_mem		= ((begin & 0x0FFF) - 0x100) | APPLE2_MEM_SLOT;
		meminfo->write_mem		= ((begin & 0x0FFF) - 0x100) | APPLE2_MEM_SLOT;
	}
}

static void apple2_mem_C300(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	if ((state->m_flags & (VAR_INTCXROM|VAR_SLOTC3ROM)) != VAR_SLOTC3ROM)
	{
		meminfo->read_mem		= (begin & 0x0FFF) | (state->m_flags & VAR_ROMSWITCH ? 0x4000 : 0x0000) | APPLE2_MEM_ROM;
		meminfo->write_mem		= APPLE2_MEM_FLOATING;
	}
	else
	{
		meminfo->read_mem		= ((begin & 0x0FFF) - 0x100) | APPLE2_MEM_SLOT;
		meminfo->write_mem		= ((begin & 0x0FFF) - 0x100) | APPLE2_MEM_SLOT;
	}
}

static void apple2_mem_C800(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	if (state->m_a2_cnxx_slot == -1)
	{
		meminfo->read_mem			= (begin & 0x0FFF) | (state->m_flags & VAR_ROMSWITCH ? 0x4000 : 0x0000) | APPLE2_MEM_ROM;
		meminfo->write_mem			= APPLE2_MEM_FLOATING;
	}
	else
	{
		meminfo->read_handler = apple2_c800_r;
		meminfo->write_handler = apple2_c800_w;
	}
}

static void apple2_mem_CE00(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	if ((state->m_flags & VAR_ROMSWITCH) && !strcmp(machine.system().name, "apple2cp"))
	{
		meminfo->read_mem		= APPLE2_MEM_AUX;
		meminfo->write_mem		= APPLE2_MEM_AUX;
	}
	else
	{
		if (state->m_a2_cnxx_slot == -1)
		{
			meminfo->read_mem		= (begin & 0x0FFF) | (state->m_flags & VAR_ROMSWITCH ? 0x4000 : 0x0000) | APPLE2_MEM_ROM;
			meminfo->write_mem		= APPLE2_MEM_FLOATING;
		}
		else
		{
			meminfo->read_handler = apple2_ce00_r;
			meminfo->write_handler = apple2_ce00_w;
		}
	}
}

static void apple2_mem_D000(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	if (state->m_flags & VAR_LCRAM)
	{
		if (state->m_flags & VAR_LCRAM2)
			meminfo->read_mem	= (state->m_flags & VAR_ALTZP)	? 0x01C000 : 0x00C000;
		else
			meminfo->read_mem	= (state->m_flags & VAR_ALTZP)	? 0x01D000 : 0x00D000;
	}
	else
	{
		meminfo->read_mem		= (state->m_flags & VAR_ROMSWITCH) ? 0x005000 : 0x001000;
		meminfo->read_mem		|= APPLE2_MEM_ROM;
	}

	if (state->m_flags & VAR_LCWRITE)
	{
		if (state->m_flags & VAR_LCRAM2)
			meminfo->write_mem	= (state->m_flags & VAR_ALTZP)	? 0x01C000 : 0x00C000;
		else
			meminfo->write_mem	= (state->m_flags & VAR_ALTZP)	? 0x01D000 : 0x00D000;
	}
	else
	{
		meminfo->write_mem = APPLE2_MEM_FLOATING;
	}
}

static void apple2_mem_E000(running_machine &machine, offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	if (state->m_flags & VAR_LCRAM)
	{
		meminfo->read_mem		= (state->m_flags & VAR_ALTZP)	? 0x01E000 : 0x00E000;
	}
	else
	{
		meminfo->read_mem		= (state->m_flags & VAR_ROMSWITCH) ? 0x006000 : 0x002000;
		meminfo->read_mem		|= APPLE2_MEM_ROM;
	}

	if (state->m_flags & VAR_LCWRITE)
	{
		meminfo->write_mem		= (state->m_flags & VAR_ALTZP)	? 0x01E000 : 0x00E000;
	}
	else
	{
		meminfo->write_mem		= APPLE2_MEM_FLOATING;
	}
}



static const apple2_memmap_entry apple2_memmap_entries[] =
{
	{ 0x0000, 0x01FF, apple2_mem_0000, A2MEM_MONO },
	{ 0x0200, 0x03FF, apple2_mem_0200, A2MEM_DUAL },
	{ 0x0400, 0x07FF, apple2_mem_0400, A2MEM_DUAL },
	{ 0x0800, 0x1FFF, apple2_mem_0800, A2MEM_DUAL },
	{ 0x2000, 0x3FFF, apple2_mem_2000, A2MEM_DUAL },
	{ 0x4000, 0xBFFF, apple2_mem_4000, A2MEM_DUAL },
	{ 0xC000, 0xC0FF, apple2_mem_C000, A2MEM_IO },
	{ 0xC100, 0xC2FF, apple2_mem_Cx00, A2MEM_MONO },
	{ 0xC300, 0xC3FF, apple2_mem_C300, A2MEM_MONO },
	{ 0xC400, 0xC7FF, apple2_mem_Cx00, A2MEM_MONO },
	{ 0xC800, 0xCDFF, apple2_mem_C800, A2MEM_MONO },
	{ 0xCE00, 0xCFFE, apple2_mem_CE00, A2MEM_MONO },
	{ 0xCFFF, 0xCFFF, apple2_mem_CFFF, A2MEM_IO },
	{ 0xD000, 0xDFFF, apple2_mem_D000, A2MEM_DUAL },
	{ 0xE000, 0xFFFF, apple2_mem_E000, A2MEM_DUAL },
	{ 0 }
};



void apple2_setvar(running_machine &machine, UINT32 val, UINT32 mask)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	LOG(("apple2_setvar(): val=0x%06x mask=0x%06x pc=0x%04x\n", val, mask,
					(unsigned int) cpu_get_reg(machine.device("maincpu"), STATE_GENPC)));

	assert((val & mask) == val);

	/* apply mask and set */
	val &= state->m_a2_mask;
	val |= state->m_a2_set;

	/* change the softswitch */
	state->m_flags &= ~mask;
	state->m_flags |= val;

	apple2_update_memory(machine);
}



/* -----------------------------------------------------------------------
 * Floating bus code
 *
 *     preliminary floating bus video scanner code - look for comments
 *     with FIX:
 * ----------------------------------------------------------------------- */

UINT8 apple2_getfloatingbusvalue(running_machine &machine)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	enum
	{
		// scanner types
		kScannerNone = 0, kScannerApple2, kScannerApple2e,

		// scanner constants
		kHBurstClock      =    53, // clock when Color Burst starts
		kHBurstClocks     =     4, // clocks per Color Burst duration
		kHClock0State     =  0x18, // H[543210] = 011000
		kHClocks          =    65, // clocks per horizontal scan (including HBL)
		kHPEClock         =    40, // clock when HPE (horizontal preset enable) goes low
		kHPresetClock     =    41, // clock when H state presets
		kHSyncClock       =    49, // clock when HSync starts
		kHSyncClocks      =     4, // clocks per HSync duration
		kNTSCScanLines    =   262, // total scan lines including VBL (NTSC)
		kNTSCVSyncLine    =   224, // line when VSync starts (NTSC)
		kPALScanLines     =   312, // total scan lines including VBL (PAL)
		kPALVSyncLine     =   264, // line when VSync starts (PAL)
		kVLine0State      = 0x100, // V[543210CBA] = 100000000
		kVPresetLine      =   256, // line when V state presets
		kVSyncLines       =     4, // lines per VSync duration
		kClocksPerVSync   = kHClocks * kNTSCScanLines // FIX: NTSC only?
	};

	// vars
	//
	int i, Hires, Mixed, Page2, _80Store, ScanLines, /* VSyncLine, ScanCycles,*/
		h_clock, h_state, h_0, h_1, h_2, h_3, h_4, h_5,
		v_line, v_state, v_A, v_B, v_C, v_0, v_1, v_2, v_3, v_4, /* v_5, */
		_hires, addend0, addend1, addend2, sum, address;

	// video scanner data
	//
	i = (machine.device<cpu_device>("maincpu"))->total_cycles() % kClocksPerVSync; // cycles into this VSync

	// machine state switches
	//
	Hires    = (state->m_flags & VAR_HIRES) ? 1 : 0;
	Mixed    = (state->m_flags & VAR_MIXED) ? 1 : 0;
	Page2    = (state->m_flags & VAR_PAGE2) ? 1 : 0;
	_80Store = (state->m_flags & VAR_80STORE) ? 1 : 0;

	// calculate video parameters according to display standard
	//
	ScanLines  = 1 ? kNTSCScanLines : kPALScanLines; // FIX: NTSC only?
	// VSyncLine  = 1 ? kNTSCVSyncLine : kPALVSyncLine; // FIX: NTSC only?
	// ScanCycles = ScanLines * kHClocks;

	// calculate horizontal scanning state
	//
	h_clock = (i + kHPEClock) % kHClocks; // which horizontal scanning clock
	h_state = kHClock0State + h_clock; // H state bits
	if (h_clock >= kHPresetClock) // check for horizontal preset
	{
		h_state -= 1; // correct for state preset (two 0 states)
	}
	h_0 = (h_state >> 0) & 1; // get horizontal state bits
	h_1 = (h_state >> 1) & 1;
	h_2 = (h_state >> 2) & 1;
	h_3 = (h_state >> 3) & 1;
	h_4 = (h_state >> 4) & 1;
	h_5 = (h_state >> 5) & 1;

	// calculate vertical scanning state
	//
	v_line  = i / kHClocks; // which vertical scanning line
	v_state = kVLine0State + v_line; // V state bits
	if ((v_line >= kVPresetLine)) // check for previous vertical state preset
	{
		v_state -= ScanLines; // compensate for preset
	}
	v_A = (v_state >> 0) & 1; // get vertical state bits
	v_B = (v_state >> 1) & 1;
	v_C = (v_state >> 2) & 1;
	v_0 = (v_state >> 3) & 1;
	v_1 = (v_state >> 4) & 1;
	v_2 = (v_state >> 5) & 1;
	v_3 = (v_state >> 6) & 1;
	v_4 = (v_state >> 7) & 1;
	//v_5 = (v_state >> 8) & 1;

	// calculate scanning memory address
	//
	_hires = Hires;
	if (Hires && Mixed && (v_4 & v_2))
	{
		_hires = 0; // (address is in text memory)
	}

	addend0 = 0x68; // 1            1            0            1
	addend1 =              (h_5 << 5) | (h_4 << 4) | (h_3 << 3);
	addend2 = (v_4 << 6) | (v_3 << 5) | (v_4 << 4) | (v_3 << 3);
	sum     = (addend0 + addend1 + addend2) & (0x0F << 3);

	address = 0;
	address |= h_0 << 0; // a0
	address |= h_1 << 1; // a1
	address |= h_2 << 2; // a2
	address |= sum;      // a3 - aa6
	address |= v_0 << 7; // a7
	address |= v_1 << 8; // a8
	address |= v_2 << 9; // a9
	address |= ((_hires) ? v_A : (1 ^ (Page2 & (1 ^ _80Store)))) << 10; // a10
	address |= ((_hires) ? v_B : (Page2 & (1 ^ _80Store))) << 11; // a11
	if (_hires) // hires?
	{
		// Y: insert hires only address bits
		//
		address |= v_C << 12; // a12
		address |= (1 ^ (Page2 & (1 ^ _80Store))) << 13; // a13
		address |= (Page2 & (1 ^ _80Store)) << 14; // a14
	}
	else
	{
		// N: text, so no higher address bits unless Apple ][, not Apple //e
		//
		if ((1) && // Apple ][? // FIX: check for Apple ][? (FB is most useful in old games)
			(kHPEClock <= h_clock) && // Y: HBL?
			(h_clock <= (kHClocks - 1)))
		{
			address |= 1 << 12; // Y: a12 (add $1000 to address!)
		}
	}

	// update VBL' state
	//
	if (v_4 & v_3) // VBL?
	{
		//CMemory::mState &= ~CMemory::kVBLBar; // Y: VBL' is false // FIX: MESS?
	}
	else
	{
		//CMemory::mState |= CMemory::kVBLBar; // N: VBL' is true // FIX: MESS?
	}

	return ram_get_ptr(machine.device(RAM_TAG))[address % ram_get_size(machine.device(RAM_TAG))]; // FIX: this seems to work, but is it right!?
}



/* -----------------------------------------------------------------------
 * Machine reset
 * ----------------------------------------------------------------------- */

static void apple2_reset(running_machine &machine)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	int need_intcxrom;

	need_intcxrom = !strcmp(machine.system().name, "apple2c")
		|| !strcmp(machine.system().name, "apple2c0")
		|| !strcmp(machine.system().name, "apple2c3")
		|| !strcmp(machine.system().name, "apple2c4")
		|| !strcmp(machine.system().name, "prav8c")
		|| !strcmp(machine.system().name, "apple2cp")
		|| !strncmp(machine.system().name, "apple2g", 7);
	apple2_setvar(machine, need_intcxrom ? VAR_INTCXROM : 0, ~0);

	// ROM 0 cannot boot unless language card bank 2 is write-enabled (but read ROM) on startup
	if (!strncmp(machine.system().name, "apple2g", 7))
	{
		apple2_setvar(machine, VAR_LCWRITE|VAR_LCRAM2, VAR_LCWRITE | VAR_LCRAM | VAR_LCRAM2);
	}

	state->m_a2_speaker_state = 0;

	state->m_joystick_x1_time = state->m_joystick_y1_time = 0;
	state->m_joystick_x2_time = state->m_joystick_y2_time = 0;
}



/* -----------------------------------------------------------------------
 * Apple II interrupt; used to force partial updates
 * ----------------------------------------------------------------------- */

INTERRUPT_GEN( apple2_interrupt )
{
	int irq_freq = 1;
	int scanline;

	g_profiler.start(PROFILER_A2INT);

	scanline = device->machine().primary_screen->vpos();

	if (scanline > 190)
	{
		irq_freq --;
		if (irq_freq < 0)
			irq_freq = 1;

		if (irq_freq)
			cputag_set_input_line(device->machine(), "maincpu", M6502_IRQ_LINE, PULSE_LINE);
	}

	device->machine().primary_screen->update_partial(scanline);

	g_profiler.stop();
}



/***************************************************************************
    apple2_mainram0400_w
    apple2_mainram2000_w
    apple2_auxram0400_w
    apple2_auxram2000_w
***************************************************************************/

static WRITE8_HANDLER ( apple2_mainram0400_w )
{
	offset += 0x400;
	ram_get_ptr(space->machine().device(RAM_TAG))[offset] = data;
}

static WRITE8_HANDLER ( apple2_mainram2000_w )
{
	offset += 0x2000;
	ram_get_ptr(space->machine().device(RAM_TAG))[offset] = data;
}

static WRITE8_HANDLER ( apple2_auxram0400_w )
{
	offset += 0x10400;
	ram_get_ptr(space->machine().device(RAM_TAG))[offset] = data;
}

static WRITE8_HANDLER ( apple2_auxram2000_w )
{
	offset += 0x12000;
	ram_get_ptr(space->machine().device(RAM_TAG))[offset] = data;
}



/***************************************************************************
  apple2_c00x_r
***************************************************************************/

READ8_HANDLER ( apple2_c00x_r )
{
	UINT8 result = 0;

	if(!space->debugger_access())
	{
		/* Read the keyboard data and strobe */
		g_profiler.start(PROFILER_C00X);
		result = AY3600_keydata_strobe_r(space->machine());
		g_profiler.stop();
	}

	return result;
}



/***************************************************************************
  apple2_c00x_w

  C000  80STOREOFF
  C001  80STOREON - use 80-column memory mapping
  C002  RAMRDOFF
  C003  RAMRDON - read from aux 48k
  C004  RAMWRTOFF
  C005  RAMWRTON - write to aux 48k
  C006  INTCXROMOFF
  C007  INTCXROMON
  C008  ALTZPOFF
  C009  ALTZPON - use aux ZP, stack and language card area
  C00A  SLOTC3ROMOFF
  C00B  SLOTC3ROMON - use external slot 3 ROM
  C00C  80COLOFF
  C00D  80COLON - use 80-column display mode
  C00E  ALTCHARSETOFF
  C00F  ALTCHARSETON - use alt character set
***************************************************************************/

WRITE8_HANDLER ( apple2_c00x_w )
{
	UINT32 mask;
	mask = 1 << (offset / 2);
	apple2_setvar(space->machine(), (offset & 1) ? mask : 0, mask);
}



/***************************************************************************
  apple2_c01x_r
***************************************************************************/

READ8_HANDLER ( apple2_c01x_r )
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	UINT8 result = apple2_getfloatingbusvalue(space->machine()) & 0x7F;

	if(!space->debugger_access())
	{
		g_profiler.start(PROFILER_C01X);

		LOG(("a2 softswitch_r: %04x\n", offset + 0xc010));
		switch (offset)
		{
			case 0x00:			result |= AY3600_anykey_clearstrobe_r(space->machine());		break;
			case 0x01:			result |= (state->m_flags & VAR_LCRAM2)		? 0x80 : 0x00;	break;
			case 0x02:			result |= (state->m_flags & VAR_LCRAM)		? 0x80 : 0x00;	break;
			case 0x03:			result |= (state->m_flags & VAR_RAMRD)		? 0x80 : 0x00;	break;
			case 0x04:			result |= (state->m_flags & VAR_RAMWRT)		? 0x80 : 0x00;	break;
			case 0x05:			result |= (state->m_flags & VAR_INTCXROM)	? 0x80 : 0x00;	break;
			case 0x06:			result |= (state->m_flags & VAR_ALTZP)		? 0x80 : 0x00;	break;
			case 0x07:			result |= (state->m_flags & VAR_SLOTC3ROM)	? 0x80 : 0x00;	break;
			case 0x08:			result |= (state->m_flags & VAR_80STORE)	? 0x80 : 0x00;	break;
			case 0x09:			result |= !space->machine().primary_screen->vblank()		? 0x80 : 0x00;	break;
			case 0x0A:			result |= (state->m_flags & VAR_TEXT)		? 0x80 : 0x00;	break;
			case 0x0B:			result |= (state->m_flags & VAR_MIXED)		? 0x80 : 0x00;	break;
			case 0x0C:			result |= (state->m_flags & VAR_PAGE2)		? 0x80 : 0x00;	break;
			case 0x0D:			result |= (state->m_flags & VAR_HIRES)		? 0x80 : 0x00;	break;
			case 0x0E:			result |= (state->m_flags & VAR_ALTCHARSET)	? 0x80 : 0x00;	break;
			case 0x0F:			result |= (state->m_flags & VAR_80COL)		? 0x80 : 0x00;	break;
		}

		g_profiler.stop();
	}

	return result;
}



/***************************************************************************
  apple2_c01x_w
***************************************************************************/

WRITE8_HANDLER( apple2_c01x_w )
{
	/* Clear the keyboard strobe - ignore the returned results */
	g_profiler.start(PROFILER_C01X);
	AY3600_anykey_clearstrobe_r(space->machine());
	g_profiler.stop();
}



/***************************************************************************
  apple2_c02x_r
***************************************************************************/

READ8_HANDLER( apple2_c02x_r )
{
	if(!space->debugger_access())
	{
		apple2_c02x_w(space, offset, 0);
	}
	return apple2_getfloatingbusvalue(space->machine());
}



/***************************************************************************
  apple2_c02x_w
***************************************************************************/

WRITE8_HANDLER( apple2_c02x_w )
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	switch(offset)
	{
		case 0x08:
			apple2_setvar(space->machine(), (state->m_flags & VAR_ROMSWITCH) ^ VAR_ROMSWITCH, VAR_ROMSWITCH);
			break;
	}
}



/***************************************************************************
  apple2_c03x_r
***************************************************************************/

READ8_HANDLER ( apple2_c03x_r )
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	if(!space->debugger_access())
	{
		if (!offset)
		{
			device_t *speaker_device = space->machine().device("a2speaker");

			if (state->m_a2_speaker_state == 1)
			{
				state->m_a2_speaker_state = 0;
			}
			else
			{
				state->m_a2_speaker_state = 1;
			}
			speaker_level_w(speaker_device, state->m_a2_speaker_state);
		}
	}
	return apple2_getfloatingbusvalue(space->machine());
}



/***************************************************************************
  apple2_c03x_w
***************************************************************************/

WRITE8_HANDLER ( apple2_c03x_w )
{
	apple2_c03x_r(space, offset);
}



/***************************************************************************
  apple2_c05x_r
***************************************************************************/

READ8_HANDLER ( apple2_c05x_r )
{
	if(!space->debugger_access())
	{
		UINT32 mask;

		/* ANx has reverse SET logic */
		if (offset >= 8)
		{
			offset ^= 1;
		}

		mask = 0x100 << (offset / 2);
		apple2_setvar(space->machine(), (offset & 1) ? mask : 0, mask);
	}
	return apple2_getfloatingbusvalue(space->machine());
}



/***************************************************************************
  apple2_c05x_w
***************************************************************************/

WRITE8_HANDLER ( apple2_c05x_w )
{
	apple2_c05x_r(space, offset);
}



/***************************************************************************
  apple2_c06x_r
***************************************************************************/

static device_t *cassette_device_image(running_machine &machine)
{
	return machine.device("cassette");
}

READ8_HANDLER ( apple2_c06x_r )
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	int result = 0;
	if(!space->debugger_access())
	{
		switch (offset & 0x0F)
		{
			case 0x00:
				/* Cassette input */
				result = cassette_input(cassette_device_image(space->machine())) > 0.0 ? 0x80 : 0;
				break;
			case 0x01:
				/* Open-Apple/Joystick button 0 */
				result = apple2_pressed_specialkey(space->machine(), SPECIALKEY_BUTTON0);
				break;
			case 0x02:
				/* Closed-Apple/Joystick button 1 */
				result = apple2_pressed_specialkey(space->machine(), SPECIALKEY_BUTTON1);
				break;
			case 0x03:
				/* Joystick button 2. Later revision motherboards connected this to SHIFT also */
				result = apple2_pressed_specialkey(space->machine(), SPECIALKEY_BUTTON2);
				break;
			case 0x04:
				/* X Joystick 1 axis */
				result = space->machine().time().as_double() < state->m_joystick_x1_time;
				break;
			case 0x05:
				/* Y Joystick 1 axis */
				result = space->machine().time().as_double() < state->m_joystick_y1_time;
				break;
			case 0x06:
				/* X Joystick 2 axis */
				result = space->machine().time().as_double() < state->m_joystick_x2_time;
				break;
			case 0x07:
				/* Y Joystick 2 axis */
				result = space->machine().time().as_double() < state->m_joystick_y2_time;
				break;
			default:
				/* c060 Empty Cassette head read
                 * and any other non joystick c06 port returns this according to applewin
                 */
				return apple2_getfloatingbusvalue(space->machine());
		}
	}
	return result ? 0x80 : 0x00;
}



/***************************************************************************
  apple2_c07x_r
***************************************************************************/

READ8_HANDLER ( apple2_c07x_r )
{
	apple2_state *state = space->machine().driver_data<apple2_state>();
	if(!space->debugger_access())
	{
		double x_calibration = attotime::from_usec(12).as_double();
		double y_calibration = attotime::from_usec(13).as_double();

		if (offset == 0)
		{
			state->m_joystick_x1_time = space->machine().time().as_double() + x_calibration * input_port_read(space->machine(), "joystick_1_x");
			state->m_joystick_y1_time = space->machine().time().as_double() + y_calibration * input_port_read(space->machine(), "joystick_1_y");
			state->m_joystick_x2_time = space->machine().time().as_double() + x_calibration * input_port_read(space->machine(), "joystick_2_x");
			state->m_joystick_y2_time = space->machine().time().as_double() + y_calibration * input_port_read(space->machine(), "joystick_2_y");
		}
	}
	return 0;
}



/***************************************************************************
  apple2_c07x_w
***************************************************************************/

WRITE8_HANDLER ( apple2_c07x_w )
{
	apple2_c07x_r(space, offset);
}



/* -----------------------------------------------------------------------
 * Floppy disk controller
 * ----------------------------------------------------------------------- */


static int apple2_fdc_has_35(running_machine &machine)
{
	return (floppy_get_count(machine) - apple525_get_count(machine)) > 0;
}

static int apple2_fdc_has_525(running_machine &machine)
{
	return apple525_get_count(machine) > 0;
}

static void apple2_fdc_set_lines(device_t *device, UINT8 lines)
{
	apple2_state *state = device->machine().driver_data<apple2_state>();
	if (state->m_fdc_diskreg & 0x40)
	{
		if (apple2_fdc_has_35(device->machine()))
		{
			/* slot 5: 3.5" disks */
			sony_set_lines(device,lines);
		}
	}
	else
	{
		if (apple2_fdc_has_525(device->machine()))
		{
			/* slot 6: 5.25" disks */
			apple525_set_lines(device,lines);
		}
	}
}



static void apple2_fdc_set_enable_lines(device_t *device,int enable_mask)
{
	apple2_state *state = device->machine().driver_data<apple2_state>();
	int slot5_enable_mask = 0;
	int slot6_enable_mask = 0;

	if (state->m_fdc_diskreg & 0x40)
		slot5_enable_mask = enable_mask;
	else
		slot6_enable_mask = enable_mask;

	if (apple2_fdc_has_35(device->machine()))
	{
		/* set the 3.5" enable lines */
		sony_set_enable_lines(device,slot5_enable_mask);
	}

	if (apple2_fdc_has_525(device->machine()))
	{
		/* set the 5.25" enable lines */
		apple525_set_enable_lines(device,slot6_enable_mask);
	}
}



static UINT8 apple2_fdc_read_data(device_t *device)
{
	apple2_state *state = device->machine().driver_data<apple2_state>();
	UINT8 result = 0x00;

	if (state->m_fdc_diskreg & 0x40)
	{
		if (apple2_fdc_has_35(device->machine()))
		{
			/* slot 5: 3.5" disks */
			result = sony_read_data(device);
		}
	}
	else
	{
		if (apple2_fdc_has_525(device->machine()))
		{
			/* slot 6: 5.25" disks */
			result = apple525_read_data(device);
		}
	}
	return result;
}



static void apple2_fdc_write_data(device_t *device, UINT8 data)
{
	apple2_state *state = device->machine().driver_data<apple2_state>();
	if (state->m_fdc_diskreg & 0x40)
	{
		if (apple2_fdc_has_35(device->machine()))
		{
			/* slot 5: 3.5" disks */
			sony_write_data(device,data);
		}
	}
	else
	{
		if (apple2_fdc_has_525(device->machine()))
		{
			/* slot 6: 5.25" disks */
			apple525_write_data(device,data);
		}
	}
}



static int apple2_fdc_read_status(device_t *device)
{
	apple2_state *state = device->machine().driver_data<apple2_state>();
	int result = 0;

	if (state->m_fdc_diskreg & 0x40)
	{
		if (apple2_fdc_has_35(device->machine()))
		{
			/* slot 5: 3.5" disks */
			result = sony_read_status(device);
		}
	}
	else
	{
		if (apple2_fdc_has_525(device->machine()))
		{
			/* slot 6: 5.25" disks */
			result = apple525_read_status(device);
		}
	}
	return result;
}



void apple2_iwm_setdiskreg(running_machine &machine, UINT8 data)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	state->m_fdc_diskreg = data & 0xC0;
	if (apple2_fdc_has_35(machine))
		sony_set_sel_line( machine.device("fdc"),state->m_fdc_diskreg & 0x80);
}



UINT8 apple2_iwm_getdiskreg(running_machine &machine)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	return state->m_fdc_diskreg;
}




const applefdc_interface apple2_fdc_interface =
{
	apple2_fdc_set_lines,			/* set_lines */
	apple2_fdc_set_enable_lines,	/* set_enable_lines */

	apple2_fdc_read_data,			/* read_data */
	apple2_fdc_write_data,			/* write_data */
	apple2_fdc_read_status			/* read_status */
};



/* -----------------------------------------------------------------------
 * Driver init
 * ----------------------------------------------------------------------- */

void apple2_init_common(running_machine &machine)
{
	apple2_state *state = machine.driver_data<apple2_state>();
	state->m_flags = 0;
	state->m_fdc_diskreg = 0;

	AY3600_init(machine);
	machine.add_notifier(MACHINE_NOTIFY_RESET, apple2_reset);

	/* state save registers */
	state->save_item(NAME(state->m_flags));
	machine.state().register_postload(apple2_update_memory_postload, NULL);

	/* --------------------------------------------- *
     * set up the softswitch mask/set                *
     * --------------------------------------------- */
	state->m_a2_mask = ~0;
	state->m_a2_set = 0;

	/* disable VAR_ROMSWITCH if the ROM is only 16k */
	if (machine.region("maincpu")->bytes() < 0x8000)
		state->m_a2_mask &= ~VAR_ROMSWITCH;

	if (ram_get_size(machine.device(RAM_TAG)) <= 64*1024)
		state->m_a2_mask &= ~(VAR_RAMRD | VAR_RAMWRT | VAR_80STORE | VAR_ALTZP | VAR_80COL);
}



MACHINE_START( apple2 )
{
	apple2_memmap_config mem_cfg;
	void *apple2cp_ce00_ram = NULL;

	/* there appears to be some hidden RAM that is swapped in on the Apple
     * IIc plus; I have not found any official documentation but the BIOS
     * clearly uses this area as writeable memory */
	if (!strcmp(machine.system().name, "apple2cp"))
		apple2cp_ce00_ram = auto_alloc_array(machine, UINT8, 0x200);

	apple2_init_common(machine);

	/* setup memory */
	memset(&mem_cfg, 0, sizeof(mem_cfg));
	mem_cfg.first_bank = 1;
	mem_cfg.memmap = apple2_memmap_entries;
	mem_cfg.auxmem = (UINT8*)apple2cp_ce00_ram;
	apple2_setup_memory(machine, &mem_cfg);

	/* perform initial reset */
	apple2_reset(machine);
}



int apple2_pressed_specialkey(running_machine &machine, UINT8 key)
{
	return (input_port_read(machine, "keyb_special") & key)
		|| (input_port_read_safe(machine, "joystick_buttons", 0x00) & key);
}
