/***************************************************************************

  apple2.c

  Machine file to handle emulation of the Apple II series.

  TODO:  Make a standard set of peripherals work.
  TODO:  Allow swappable peripherals in each slot.
  TODO:  Verify correctness of C08X switches.
			- need to do double-read before write-enable RAM

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "video/generic.h"
#include "cpu/m6502/m6502.h"
#include "includes/apple2.h"
#include "machine/ay3600.h"
#include "machine/applefdc.h"
#include "devices/sonydriv.h"
#include "devices/appldriv.h"
#include "devices/flopdrv.h"
#include "sound/dac.h"
#include "sound/ay8910.h"
#include "profiler.h"

#ifdef MAME_DEBUG
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif /* MAME_DEBUG */

#define PROFILER_C00X	PROFILER_USER2
#define PROFILER_C01X	PROFILER_USER2
#define PROFILER_C08X	PROFILER_USER2
#define PROFILER_A2INT	PROFILER_USER2

/* softswitch */
UINT32 a2;

/* before the softswitch is changed, these are applied */
static UINT32 a2_mask;
static UINT32 a2_set;


/* local */
static apple2_config *a2_config;
static void **a2_slot_tokens;
static int a2_speaker_state;

static double joystick_x1_time;
static double joystick_y1_time;
static double joystick_x2_time;
static double joystick_y2_time;

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



/* -----------------------------------------------------------------------
 * New Apple II memory manager
 * ----------------------------------------------------------------------- */

static apple2_memmap_config apple2_mem_config;
static apple2_meminfo *apple2_current_meminfo;


static READ8_HANDLER(read_floatingbus)
{
	return apple2_getfloatingbusvalue();
}



void apple2_setup_memory(const apple2_memmap_config *config)
{
	apple2_mem_config = *config;
	apple2_current_meminfo = NULL;
	apple2_update_memory();
}



void apple2_update_memory(void)
{
	int i, bank, rbank, wbank;
	int full_update = 0;
	apple2_meminfo meminfo;
	read8_handler rh;
	write8_handler wh;
	offs_t begin, end_r, end_w;
	UINT8 *rbase, *wbase, *rom, *slot_ram;
	UINT32 rom_length, slot_length, offset;
	bank_disposition_t bank_disposition;

	/* need to build list of current info? */
	if (!apple2_current_meminfo)
	{
		for (i = 0; apple2_mem_config.memmap[i].end; i++)
			;
		apple2_current_meminfo = auto_malloc(i * sizeof(*apple2_current_meminfo));
		full_update = 1;
	}

	/* get critical info */
	rom = memory_region(REGION_CPU1);
	rom_length = memory_region_length(REGION_CPU1) & ~0xFFF;
	slot_length = memory_region_length(REGION_CPU1) - rom_length;
	slot_ram = (slot_length > 0) ? &rom[rom_length] : NULL;

	/* loop through the entire memory map */
	bank = apple2_mem_config.first_bank;
	for (i = 0; apple2_mem_config.memmap[i].get_meminfo; i++)
	{
		/* retrieve information on this entry */
		memset(&meminfo, 0, sizeof(meminfo));
		apple2_mem_config.memmap[i].get_meminfo(apple2_mem_config.memmap[i].begin, apple2_mem_config.memmap[i].end, &meminfo);

		bank_disposition = apple2_mem_config.memmap[i].bank_disposition;

		/* do we need to memory reading? */
		if (full_update
			|| (meminfo.read_mem != apple2_current_meminfo[i].read_mem)
			|| (meminfo.read_handler != apple2_current_meminfo[i].read_handler))
		{
			rbase = NULL;
			rbank = (bank_disposition != A2MEM_IO) ? bank : 0;
			begin = apple2_mem_config.memmap[i].begin;
			end_r = apple2_mem_config.memmap[i].end;
			rh = (read8_handler) (STATIC_BANK1 + rbank - 1);

			LOG(("apple2_update_memory():  Updating RD {%06X..%06X} [#%02d] --> %08X\n",
				begin, end_r, rbank, meminfo.read_mem));

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
				assert(apple2_mem_config.auxmem);
				offset = meminfo.read_mem & APPLE2_MEM_MASK;
				rbase = &apple2_mem_config.auxmem[offset];
			}
			else if ((meminfo.read_mem & 0xC0000000) == APPLE2_MEM_SLOT)
			{
				/* slot RAM */
				if (slot_ram)
					rbase = &slot_ram[meminfo.read_mem & APPLE2_MEM_MASK];
				else
					rh = read_floatingbus;
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
				if (end_r >= mess_ram_size)
					end_r = mess_ram_size - 1;
				offset = meminfo.read_mem & APPLE2_MEM_MASK;
				if (end_r >= begin)
					rbase = &mess_ram[offset];
			}

			/* install the actual handlers */
			if (begin <= end_r)
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end_r, 0, 0, rh);

			/* did we 'go past the end?' */
			if (end_r < apple2_mem_config.memmap[i].end)
				memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, end_r + 1, apple2_mem_config.memmap[i].end, 0, 0, MRA8_NOP);

			/* set the memory bank */
			if (rbase)
			{
				memory_set_bankptr(rbank, rbase);
			}

			/* record the current settings */
			apple2_current_meminfo[i].read_mem = meminfo.read_mem;
			apple2_current_meminfo[i].read_handler = meminfo.read_handler;
		}

		/* do we need to memory writing? */
		if (full_update
			|| (meminfo.write_mem != apple2_current_meminfo[i].write_mem)
			|| (meminfo.write_handler != apple2_current_meminfo[i].write_handler))
		{
			wbase = NULL;
			if (bank_disposition == A2MEM_MONO)
				wbank = bank + 0;
			else if (bank_disposition == A2MEM_DUAL)
				wbank = bank + 1;
			else
				wbank = 0;
			begin = apple2_mem_config.memmap[i].begin;
			end_w = apple2_mem_config.memmap[i].end;
			wh = (write8_handler) (STATIC_BANK1 + wbank - 1);

			LOG(("apple2_update_memory():  Updating WR {%06X..%06X} [#%02d] --> %08X\n",
				begin, end_w, wbank, meminfo.write_mem));

			/* write handling */
			if (meminfo.write_handler)
			{
				/* handler */
				wh = meminfo.write_handler;
			}
			else if ((meminfo.write_mem & 0xC0000000) == APPLE2_MEM_AUX)
			{
				/* auxillary memory */
				assert(apple2_mem_config.auxmem);
				offset = meminfo.write_mem & APPLE2_MEM_MASK;
				wbase = &apple2_mem_config.auxmem[offset];
			}
			else if ((meminfo.write_mem & 0xC0000000) == APPLE2_MEM_SLOT)
			{
				/* slot RAM */
				if (slot_ram)
					wbase = &slot_ram[meminfo.write_mem & APPLE2_MEM_MASK];
				else
					wh = MWA8_NOP;
			}
			else if ((meminfo.write_mem & 0xC0000000) == APPLE2_MEM_ROM)
			{
				/* ROM */
				wh = MWA8_NOP;
			}
			else
			{
				/* RAM */
				if (end_w >= mess_ram_size)
					end_w = mess_ram_size - 1;
				offset = meminfo.write_mem & APPLE2_MEM_MASK;
				if (end_w >= begin)
					wbase = &mess_ram[offset];
			}


			/* install the actual handlers */
			if (begin <= end_w)
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end_w, 0, 0, wh);

			/* did we 'go past the end?' */
			if (end_w < apple2_mem_config.memmap[i].end)
				memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, end_w + 1, apple2_mem_config.memmap[i].end, 0, 0, MWA8_NOP);

			/* set the memory bank */
			if (wbase)
			{
				memory_set_bankptr(wbank, wbase);
			}

			/* record the current settings */
			apple2_current_meminfo[i].write_mem = meminfo.write_mem;
			apple2_current_meminfo[i].write_handler = meminfo.write_handler;
		}
		bank += bank_disposition;
	}
}



/* -----------------------------------------------------------------------
 * Apple II memory map
 * ----------------------------------------------------------------------- */

READ8_HANDLER(apple2_c0xx_r)
{
	static const read8_handler handlers[] =
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
	int slot;

	offset &= 0xFF;

	if (offset < 0x80)
	{
		if (handlers[offset / 0x10])
			result = handlers[offset / 0x10](offset % 0x10);
	}
	else
	{
		slot = (offset - 0x80) / 0x10;
		if (a2_config->slots[slot] && a2_config->slots[slot]->read)
			result = a2_config->slots[slot]->read(a2_slot_tokens[slot], offset % 0x10);
	}
	return result;
}



WRITE8_HANDLER(apple2_c0xx_w)
{
	static const write8_handler handlers[] =
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
	int slot;
	
	offset &= 0xFF;
	
	if (offset < 0x80)
	{
		if (handlers[offset / 0x10])
			handlers[offset / 0x10](offset % 0x10, data);
	}
	else
	{
		slot = (offset - 0x80) / 0x10;
		if (a2_config->slots[slot] && a2_config->slots[slot]->write)
			a2_config->slots[slot]->write(a2_slot_tokens[slot], offset % 0x10, data);
	}
}



static void apple2_mem_0000(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	meminfo->read_mem			= (a2 & VAR_ALTZP)	? 0x010000 : 0x000000;
	meminfo->write_mem			= (a2 & VAR_ALTZP)	? 0x010000 : 0x000000;
}

static void apple2_mem_0200(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	meminfo->read_mem			= (a2 & VAR_RAMRD)	? 0x010200 : 0x000200;
	meminfo->write_mem			= (a2 & VAR_RAMWRT)	? 0x010200 : 0x000200;
}

static void apple2_mem_0400(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	if (a2 & VAR_80STORE)
	{
		meminfo->read_mem		= (a2 & VAR_PAGE2)	? 0x010400 : 0x000400;
		meminfo->write_mem		= (a2 & VAR_PAGE2)	? 0x010400 : 0x000400;
		meminfo->write_handler	= (a2 & VAR_PAGE2)	? apple2_auxram0400_w : apple2_mainram0400_w;
	}
	else
	{
		meminfo->read_mem		= (a2 & VAR_RAMRD)	? 0x010400 : 0x000400;
		meminfo->write_mem		= (a2 & VAR_RAMWRT)	? 0x010400 : 0x000400;
		meminfo->write_handler	= (a2 & VAR_RAMWRT)	? apple2_auxram0400_w : apple2_mainram0400_w;
	}
}

static void apple2_mem_0800(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	meminfo->read_mem			= (a2 & VAR_RAMRD)	? 0x010800 : 0x000800;
	meminfo->write_mem			= (a2 & VAR_RAMWRT)	? 0x010800 : 0x000800;
}

static void apple2_mem_2000(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	if ((a2 & (VAR_80STORE|VAR_HIRES)) == (VAR_80STORE|VAR_HIRES))
	{
		meminfo->read_mem		= (a2 & VAR_PAGE2)	? 0x012000 : 0x002000;
		meminfo->write_mem		= (a2 & VAR_PAGE2)	? 0x012000 : 0x002000;
		meminfo->write_handler	= (a2 & VAR_PAGE2)	? apple2_auxram2000_w : apple2_mainram2000_w;
	}
	else
	{
		meminfo->read_mem		= (a2 & VAR_RAMRD)	? 0x012000 : 0x002000;
		meminfo->write_mem		= (a2 & VAR_RAMWRT)	? 0x012000 : 0x002000;
		meminfo->write_handler	= (a2 & VAR_RAMWRT)	? apple2_auxram2000_w : apple2_mainram2000_w;
	}
}

static void apple2_mem_4000(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	meminfo->read_mem			= (a2 & VAR_RAMRD)	? 0x014000 : 0x004000;
	meminfo->write_mem			= (a2 & VAR_RAMWRT)	? 0x014000 : 0x004000;
}

static void apple2_mem_C000(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	meminfo->read_handler = apple2_c0xx_r;
	meminfo->write_handler = apple2_c0xx_w;
}

static void apple2_mem_Cx00(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	if (a2 & VAR_INTCXROM)
	{
		meminfo->read_mem		= (begin & 0x0FFF) | (a2 & VAR_ROMSWITCH ? 0x4000 : 0x0000) | APPLE2_MEM_ROM;
		meminfo->write_mem		= APPLE2_MEM_FLOATING;
	}
	else
	{
		meminfo->read_mem		= ((begin & 0x0FFF) - 0x100) | APPLE2_MEM_SLOT;
		meminfo->write_mem		= ((begin & 0x0FFF) - 0x100) | APPLE2_MEM_SLOT;
	}
}

static void apple2_mem_C300(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	if ((a2 & (VAR_INTCXROM|VAR_SLOTC3ROM)) != VAR_SLOTC3ROM)
	{
		meminfo->read_mem		= (begin & 0x0FFF) | (a2 & VAR_ROMSWITCH ? 0x4000 : 0x0000) | APPLE2_MEM_ROM;
		meminfo->write_mem		= APPLE2_MEM_FLOATING;
	}
	else
	{
		meminfo->read_mem		= ((begin & 0x0FFF) - 0x100) | APPLE2_MEM_SLOT;
		meminfo->write_mem		= ((begin & 0x0FFF) - 0x100) | APPLE2_MEM_SLOT;
	}
}

static void apple2_mem_C800(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	meminfo->read_mem			= (begin & 0x0FFF) | (a2 & VAR_ROMSWITCH ? 0x4000 : 0x0000) | APPLE2_MEM_ROM;
	meminfo->write_mem			= APPLE2_MEM_FLOATING;
}

static void apple2_mem_CE00(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	if ((a2 & VAR_ROMSWITCH) && !strcmp(Machine->gamedrv->name, "apple2cp"))
	{
		meminfo->read_mem		= APPLE2_MEM_AUX;
		meminfo->write_mem		= APPLE2_MEM_AUX;
	}
	else
	{
		meminfo->read_mem		= (begin & 0x0FFF) | (a2 & VAR_ROMSWITCH ? 0x4000 : 0x0000) | APPLE2_MEM_ROM;
		meminfo->write_mem		= APPLE2_MEM_FLOATING;
	}
}

static void apple2_mem_D000(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	if (a2 & VAR_LCRAM)
	{
		if (a2 & VAR_LCRAM2)
			meminfo->read_mem	= (a2 & VAR_ALTZP)	? 0x01C000 : 0x00C000;
		else
			meminfo->read_mem	= (a2 & VAR_ALTZP)	? 0x01D000 : 0x00D000;
	}
	else
	{
		meminfo->read_mem		= (a2 & VAR_ROMSWITCH) ? 0x005000 : 0x001000;
		meminfo->read_mem		|= APPLE2_MEM_ROM;
	}
	
	if (a2 & VAR_LCWRITE)
	{
		if (a2 & VAR_LCRAM2)
			meminfo->write_mem	= (a2 & VAR_ALTZP)	? 0x01C000 : 0x00C000;
		else
			meminfo->write_mem	= (a2 & VAR_ALTZP)	? 0x01D000 : 0x00D000;
	}
	else
	{
		meminfo->write_mem = APPLE2_MEM_FLOATING;
	}
}

static void apple2_mem_E000(offs_t begin, offs_t end, apple2_meminfo *meminfo)
{
	if (a2 & VAR_LCRAM)
	{
		meminfo->read_mem		= (a2 & VAR_ALTZP)	? 0x01E000 : 0x00E000;
	}
	else
	{
		meminfo->read_mem		= (a2 & VAR_ROMSWITCH) ? 0x006000 : 0x002000;
		meminfo->read_mem		|= APPLE2_MEM_ROM;
	}

	if (a2 & VAR_LCWRITE)
	{
		meminfo->write_mem		= (a2 & VAR_ALTZP)	? 0x01E000 : 0x00E000;
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
	{ 0xCE00, 0xCFFF, apple2_mem_CE00, A2MEM_MONO },
	{ 0xD000, 0xDFFF, apple2_mem_D000, A2MEM_DUAL },
	{ 0xE000, 0xFFFF, apple2_mem_E000, A2MEM_DUAL },
	{ 0 }
};



void apple2_setvar(UINT32 val, UINT32 mask)
{
	LOG(("apple2_setvar(): val=0x%06x mask=0x%06x pc=0x%04x\n", val, mask, (unsigned int) cpunum_get_reg(0, REG_PC)));

	assert((val & mask) == val);

	/* apply mask and set */
	val &= a2_mask;
	val |= a2_set;

	/* change the softswitch */
	a2 &= ~mask;
	a2 |= val;

	apple2_update_memory();
}



/* -----------------------------------------------------------------------
 * Floating bus code
 * 
 *     preliminary floating bus video scanner code - look for comments
 *     with FIX:
 * ----------------------------------------------------------------------- */

UINT8 apple2_getfloatingbusvalue(void)
{
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
	int i, Hires, Mixed, Page2, _80Store, ScanLines, VSyncLine, ScanCycles,
		h_clock, h_state, h_0, h_1, h_2, h_3, h_4, h_5,
		v_line, v_state, v_A, v_B, v_C, v_0, v_1, v_2, v_3, v_4, v_5,
		_hires, addend0, addend1, addend2, sum, address;

	// video scanner data
	//
	i = activecpu_gettotalcycles() % kClocksPerVSync; // cycles into this VSync

	// machine state switches
	//
	Hires    = (a2 & VAR_HIRES) ? 1 : 0;
	Mixed    = (a2 & VAR_MIXED) ? 1 : 0;
	Page2    = (a2 & VAR_PAGE2) ? 1 : 0;
	_80Store = (a2 & VAR_80STORE) ? 1 : 0;

	// calculate video parameters according to display standard
	//
	ScanLines  = 1 ? kNTSCScanLines : kPALScanLines; // FIX: NTSC only?
	VSyncLine  = 1 ? kNTSCVSyncLine : kPALVSyncLine; // FIX: NTSC only?
	ScanCycles = ScanLines * kHClocks;

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
	v_5 = (v_state >> 8) & 1;

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

	return mess_ram[address % mess_ram_size]; // FIX: this seems to work, but is it right!?
}



/* -----------------------------------------------------------------------
 * Machine reset
 * ----------------------------------------------------------------------- */

static void apple2_reset(running_machine *machine)
{
	int need_intcxrom, i;

	need_intcxrom = !strcmp(Machine->gamedrv->name, "apple2c")
		|| !strcmp(Machine->gamedrv->name, "apple2c0")
		|| !strcmp(Machine->gamedrv->name, "apple2c3")
		|| !strcmp(Machine->gamedrv->name, "apple2cp");
	apple2_setvar(need_intcxrom ? VAR_INTCXROM : 0, ~0);

	a2_speaker_state = 0;

	/* reset slots */
	for (i = 0; i < APPLE2_SLOT_COUNT; i++)
	{
		if (a2_config->slots[i])
		{
			if (a2_config->slots[i]->reset)
				a2_config->slots[i]->reset(a2_slot_tokens[i]);
		}
	}

	joystick_x1_time = joystick_y1_time = 0;
	joystick_x2_time = joystick_y2_time = 0;
}



/* -----------------------------------------------------------------------
 * Apple II interrupt; used to force partial updates
 * ----------------------------------------------------------------------- */

void apple2_interrupt(void)
{
	int irq_freq = 1;
	int scanline;

	profiler_mark(PROFILER_A2INT);

	scanline = cpu_getscanline();

	if (scanline > 190)
	{
		irq_freq --;
		if (irq_freq < 0)
			irq_freq = 1;

		if (irq_freq)
			cpunum_set_input_line(0, M6502_IRQ_LINE, PULSE_LINE);
	}

	video_screen_update_partial(0, scanline);

	profiler_mark(PROFILER_END);
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
	mess_ram[offset] = data;
	apple2_video_touch(offset);
}

static WRITE8_HANDLER ( apple2_mainram2000_w )
{
	offset += 0x2000;
	mess_ram[offset] = data;
	apple2_video_touch(offset);
}

static WRITE8_HANDLER ( apple2_auxram0400_w )
{
	offset += 0x10400;
	mess_ram[offset] = data;
	apple2_video_touch(offset);
}

static WRITE8_HANDLER ( apple2_auxram2000_w )
{
	offset += 0x12000;
	mess_ram[offset] = data;
	apple2_video_touch(offset);
}



/***************************************************************************
  apple2_c00x_r
***************************************************************************/

READ8_HANDLER ( apple2_c00x_r )
{
	UINT8 result;

	/* Read the keyboard data and strobe */
	profiler_mark(PROFILER_C00X);
	result = AY3600_keydata_strobe_r();
	profiler_mark(PROFILER_END);

	return result;
}



/***************************************************************************
  apple2_c00x_w

  C000	80STOREOFF
  C001	80STOREON - use 80-column memory mapping
  C002	RAMRDOFF
  C003	RAMRDON - read from aux 48k
  C004	RAMWRTOFF
  C005	RAMWRTON - write to aux 48k
  C006	INTCXROMOFF
  C007	INTCXROMON
  C008	ALTZPOFF
  C009	ALTZPON - use aux ZP, stack and language card area
  C00A	SLOTC3ROMOFF
  C00B	SLOTC3ROMON - use external slot 3 ROM
  C00C	80COLOFF
  C00D	80COLON - use 80-column display mode
  C00E	ALTCHARSETOFF
  C00F	ALTCHARSETON - use alt character set
***************************************************************************/

WRITE8_HANDLER ( apple2_c00x_w )
{
	UINT32 mask;
	mask = 1 << (offset / 2);
	apple2_setvar((offset & 1) ? mask : 0, mask);
}



/***************************************************************************
  apple2_c01x_r
***************************************************************************/

READ8_HANDLER ( apple2_c01x_r )
{
	UINT8 result = apple2_getfloatingbusvalue() & 0x7F;

	profiler_mark(PROFILER_C01X);

	LOG(("a2 softswitch_r: %04x\n", offset + 0xc010));
	switch (offset)
	{
		case 0x00:			result |= AY3600_anykey_clearstrobe_r();		break;
		case 0x01:			result |= (a2 & VAR_LCRAM2)		? 0x80 : 0x00;	break;
		case 0x02:			result |= (a2 & VAR_LCRAM)		? 0x80 : 0x00;	break;
		case 0x03:			result |= (a2 & VAR_RAMRD)		? 0x80 : 0x00;	break;
		case 0x04:			result |= (a2 & VAR_RAMWRT)		? 0x80 : 0x00;	break;
		case 0x05:			result |= (a2 & VAR_INTCXROM)	? 0x80 : 0x00;	break;
		case 0x06:			result |= (a2 & VAR_ALTZP)		? 0x80 : 0x00;	break;
		case 0x07:			result |= (a2 & VAR_SLOTC3ROM)	? 0x80 : 0x00;	break;
		case 0x08:			result |= (a2 & VAR_80STORE)	? 0x80 : 0x00;	break;
		case 0x09:			result |= !cpu_getvblank()		? 0x80 : 0x00;	break;
		case 0x0A:			result |= (a2 & VAR_TEXT)		? 0x80 : 0x00;	break;
		case 0x0B:			result |= (a2 & VAR_MIXED)		? 0x80 : 0x00;	break;
		case 0x0C:			result |= (a2 & VAR_PAGE2)		? 0x80 : 0x00;	break;
		case 0x0D:			result |= (a2 & VAR_HIRES)		? 0x80 : 0x00;	break;
		case 0x0E:			result |= (a2 & VAR_ALTCHARSET)	? 0x80 : 0x00;	break;
		case 0x0F:			result |= (a2 & VAR_80COL)		? 0x80 : 0x00;	break;
	}

	profiler_mark(PROFILER_END);
	return result;
}



/***************************************************************************
  apple2_c01x_w
***************************************************************************/

WRITE8_HANDLER( apple2_c01x_w )
{
	/* Clear the keyboard strobe - ignore the returned results */
	profiler_mark(PROFILER_C01X);
	AY3600_anykey_clearstrobe_r();
	profiler_mark(PROFILER_END);
}



/***************************************************************************
  apple2_c02x_r
***************************************************************************/

READ8_HANDLER( apple2_c02x_r )
{
	apple2_c02x_w(offset, 0);
	return apple2_getfloatingbusvalue();
}



/***************************************************************************
  apple2_c02x_w
***************************************************************************/

WRITE8_HANDLER( apple2_c02x_w )
{
	switch(offset)
	{
		case 0x08:
			apple2_setvar((a2 & VAR_ROMSWITCH) ^ VAR_ROMSWITCH, VAR_ROMSWITCH);
			break;
	}
}



/***************************************************************************
  apple2_c03x_r
***************************************************************************/

READ8_HANDLER ( apple2_c03x_r )
{
	if (!offset)
	{
		if (a2_speaker_state == 0xFF)
			a2_speaker_state = 0;
		else
			a2_speaker_state = 0xFF;
		DAC_data_w(0, a2_speaker_state);
	}
	return apple2_getfloatingbusvalue();
}



/***************************************************************************
  apple2_c03x_w
***************************************************************************/

WRITE8_HANDLER ( apple2_c03x_w )
{
	apple2_c03x_r(offset);
}



/***************************************************************************
  apple2_c05x_r
***************************************************************************/

READ8_HANDLER ( apple2_c05x_r )
{
	UINT32 mask;

	/* ANx has reverse SET logic */
	if (offset >= 8)
		offset ^= 1;

	mask = 0x100 << (offset / 2);
	apple2_setvar((offset & 1) ? mask : 0, mask);
	return apple2_getfloatingbusvalue();
}



/***************************************************************************
  apple2_c05x_w
***************************************************************************/

WRITE8_HANDLER ( apple2_c05x_w )
{
	apple2_c05x_r(offset);
}



/***************************************************************************
  apple2_c06x_r
***************************************************************************/

READ8_HANDLER ( apple2_c06x_r )
{
	int result = 0;
	switch (offset & 0x0F)
	{
		case 0x01:
			/* Open-Apple/Joystick button 0 */
			result = pressed_specialkey(SPECIALKEY_BUTTON0);
			break;
		case 0x02:
			/* Closed-Apple/Joystick button 1 */
			result = pressed_specialkey(SPECIALKEY_BUTTON1);
			break;
		case 0x03:
			/* Joystick button 2. Later revision motherboards connected this to SHIFT also */
			result = pressed_specialkey(SPECIALKEY_BUTTON2);
			break;
		case 0x04:
			/* X Joystick 1 axis */
			result = timer_get_time() < joystick_x1_time;
			break;
		case 0x05:
			/* Y Joystick 1 axis */
			result = timer_get_time() < joystick_y1_time;
			break;
		case 0x06:
			/* X Joystick 2 axis */
			result = timer_get_time() < joystick_x2_time;
			break;
		case 0x07:
			/* Y Joystick 2 axis */
			result = timer_get_time() < joystick_y2_time;
			break;
		default:
			/* c060 Empty Cassette head read 
			 * and any other non joystick c06 port returns this according to applewin
			 */
			return apple2_getfloatingbusvalue();
			break;
	}
	return result ? 0x80 : 0x00;
}



/***************************************************************************
  apple2_c07x_r
***************************************************************************/

READ8_HANDLER ( apple2_c07x_r )
{
	double x_calibration = TIME_IN_USEC(12.0);
	double y_calibration = TIME_IN_USEC(13.0);

	if (offset == 0)
	{
		joystick_x1_time = timer_get_time() + x_calibration * readinputportbytag("joystick_1_x");
		joystick_y1_time = timer_get_time() + y_calibration * readinputportbytag("joystick_1_y");
		joystick_x2_time = timer_get_time() + x_calibration * readinputportbytag("joystick_2_x");
		joystick_y2_time = timer_get_time() + y_calibration * readinputportbytag("joystick_2_y");
	}
	return 0;
}



/***************************************************************************
  apple2_c07x_w
***************************************************************************/

WRITE8_HANDLER ( apple2_c07x_w )
{
	apple2_c07x_r(offset);
}



/***************************************************************************
  apple2_c08x_r
***************************************************************************/

READ8_HANDLER ( apple2_c08x_r )
{
	UINT32 val, mask;

	profiler_mark(PROFILER_C08X);
	LOG(("language card bankswitch read, offset: $c08%0x\n", offset));

	mask = VAR_LCWRITE | VAR_LCRAM | VAR_LCRAM2;
	val = 0;

	if (offset & 0x01)
		val |= VAR_LCWRITE;

	switch(offset & 0x03)
	{
		case 0x03:
		case 0x00:
			val |= VAR_LCRAM;
			break;
	}

	if ((offset & 0x08) == 0)
		val |= VAR_LCRAM2;

	apple2_setvar(val, mask);

	profiler_mark(PROFILER_END);
	return 0;
}



/***************************************************************************
  apple2_c08x_w
***************************************************************************/

WRITE8_HANDLER ( apple2_c08x_w )
{
	apple2_c08x_r(offset);
}



/* -----------------------------------------------------------------------
 * Language Card
 * ----------------------------------------------------------------------- */

static UINT8 apple2_langcard_read(void *token, offs_t offset)
{
	return apple2_c08x_r(offset);

}



static void apple2_langcard_write(void *token, offs_t offset, UINT8 data)
{
	apple2_c08x_w(offset, data);
}



const apple2_slotdevice apple2_slot_langcard =
{
	"langcard",
	"Language Card",
	NULL,
	NULL,
	apple2_langcard_read,
	apple2_langcard_write
};



/* -----------------------------------------------------------------------
 * Mockingboard
 * ----------------------------------------------------------------------- */

static void apple2_mockingboard_reset(void *token)
{
	/* TODO: fix this */
	/* What follows is pure filth. It abuses the core like an angry pimp on a bad hair day. */

	/* Since we know that the Mockingboard has no code ROM, we'll copy into the slot ROM space
	   an image of the onboard ROM so that when an IRQ bankswitches to the onboard ROM, we read
	   the proper stuff. Without this, it will choke and try to use the memory handler above, and
	   fail miserably. That should really be fixed. I beg you -- if you are reading this comment,
	   fix this :) */
//	memcpy (apple2_slotrom(slot), &apple_rom[0x0000 + (slot * 0x100)], 0x100);
}



static UINT8 apple2_mockingboard_read(void *token, offs_t offset)
{
	static int flip1 = 0, flip2 = 0;

	switch (offset)
	{
		/* This is used to ID the board */
		case 0x04:
			flip1 ^= 0x08;
			return flip1;
			break;
		case 0x84:
			flip2 ^= 0x08;
			return flip2;
			break;
		default:
			LOG(("mockingboard_r unmapped, offset: %02x, pc: %04x\n", offset, (unsigned) cpunum_get_reg(0, REG_PC)));
			break;
	}
	return 0x00;
}



static void apple2_mockingboard_write(void *token, offs_t offset, UINT8 data)
{
	static int latch0, latch1;

	LOG(("mockingboard_w, $%02x:%02x\n", offset, data));

	/* There is a 6522 in here which interfaces to the 8910s */
	switch (offset)
	{
		case 0x00: /* ORB1 */
			switch (data)
			{
				case 0x00: /* reset */
					sndti_reset(SOUND_AY8910, 0);
					break;
				case 0x04: /* make inactive */
					break;
				case 0x06: /* write data */
					AY8910_write_port_0_w (0, latch0);
					break;
				case 0x07: /* set register */
					AY8910_control_port_0_w (0, latch0);
					break;
			}
			break;

		case 0x01: /* ORA1 */
			latch0 = data;
			break;

		case 0x02: /* DDRB1 */
		case 0x03: /* DDRA1 */
			break;

		case 0x80: /* ORB2 */
			switch (data)
			{
				case 0x00: /* reset */
					sndti_reset(SOUND_AY8910, 1);
					break;
				case 0x04: /* make inactive */
					break;
				case 0x06: /* write data */
					AY8910_write_port_1_w (0, latch1);
					break;
				case 0x07: /* set register */
					AY8910_control_port_1_w (0, latch1);
					break;
			}
			break;

		case 0x81: /* ORA2 */
			latch1 = data;
			break;

		case 0x82: /* DDRB2 */
		case 0x83: /* DDRA2 */
			break;
	}
}



const apple2_slotdevice apple2_slot_mockingboard =
{
	"mockingboard",
	"Mockingboard",
	NULL,
	apple2_mockingboard_reset,
	apple2_mockingboard_read,
	apple2_mockingboard_write
};



/* -----------------------------------------------------------------------
 * Floppy disk controller
 * ----------------------------------------------------------------------- */

static int apple2_fdc_has_35;
static int apple2_fdc_has_525;
static int apple2_fdc_diskreg;

static void apple2_fdc_set_lines(UINT8 lines)
{
	if (apple2_fdc_diskreg & 0x40)
	{
		if (apple2_fdc_has_35)
		{
			/* slot 5: 3.5" disks */
			sony_set_lines(lines);
		}
	}
	else
	{
		if (apple2_fdc_has_525)
		{
			/* slot 6: 5.25" disks */
			apple525_set_lines(lines);
		}
	}
}



static void apple2_fdc_set_enable_lines(int enable_mask)
{
	int slot5_enable_mask = 0;
	int slot6_enable_mask = 0;

	if (apple2_fdc_diskreg & 0x40)
		slot5_enable_mask = enable_mask;
	else
		slot6_enable_mask = enable_mask;

	if (apple2_fdc_has_35)
	{
		/* set the 3.5" enable lines */
		sony_set_enable_lines(slot5_enable_mask);
	}

	if (apple2_fdc_has_525)
	{
		/* set the 5.25" enable lines */
		apple525_set_enable_lines(slot6_enable_mask);
	}
}



static UINT8 apple2_fdc_read_data(void)
{
	UINT8 result = 0x00;

	if (apple2_fdc_diskreg & 0x40)
	{
		if (apple2_fdc_has_35)
		{
			/* slot 5: 3.5" disks */
			result = sony_read_data();
		}
	}
	else
	{
		if (apple2_fdc_has_525)
		{
			/* slot 6: 5.25" disks */
			result = apple525_read_data();
		}
	}
	return result;
}



static void apple2_fdc_write_data(UINT8 data)
{
	if (apple2_fdc_diskreg & 0x40)
	{
		if (apple2_fdc_has_35)
		{
			/* slot 5: 3.5" disks */
			sony_write_data(data);
		}
	}
	else
	{
		if (apple2_fdc_has_525)
		{
			/* slot 6: 5.25" disks */
			apple525_write_data(data);
		}
	}
}



static int apple2_fdc_read_status(void)
{
	int result = 0;

	if (apple2_fdc_diskreg & 0x40)
	{
		if (apple2_fdc_has_35)
		{
			/* slot 5: 3.5" disks */
			result = sony_read_status();
		}
	}
	else
	{
		if (apple2_fdc_has_525)
		{
			/* slot 6: 5.25" disks */
			result = apple525_read_status();
		}
	}
	return result;
}



void apple2_iwm_setdiskreg(UINT8 data)
{
	apple2_fdc_diskreg = data & 0xC0;
	if (apple2_fdc_has_35)
		sony_set_sel_line(apple2_fdc_diskreg & 0x80);
}



UINT8 apple2_iwm_getdiskreg(void)
{
	return apple2_fdc_diskreg;
}



static void *apple2_fdc_init(int slot, applefdc_t fdc_type)
{
	const struct IODevice *dev;
	struct applefdc_interface intf;

	apple2_fdc_has_35 = FALSE;
	apple2_fdc_has_525 = FALSE;
	apple2_fdc_diskreg = 0x00;

	/* setup interface */
	memset(&intf, 0, sizeof(intf));
	intf.type = fdc_type;
	intf.set_lines = apple2_fdc_set_lines;
	intf.set_enable_lines = apple2_fdc_set_enable_lines;
	intf.read_status = apple2_fdc_read_status;
	intf.read_data = apple2_fdc_read_data;
	intf.write_data = apple2_fdc_write_data;

	for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
	{
		if (!strcmp(dev->tag, "sonydriv"))
			apple2_fdc_has_35 = !dev->not_working;
		else if (!strcmp(dev->tag, "apple525driv"))
			apple2_fdc_has_525 = !dev->not_working;
	}

	applefdc_init(&intf);
	return (void *) ~0;
}



static void *apple2_fdc_apple2_init(int slot)
{
	return apple2_fdc_init(slot, APPLEFDC_APPLE2);
}



static void *apple2_fdc_iwm_init(int slot)
{
	return apple2_fdc_init(slot, APPLEFDC_IWM);
}



static UINT8 apple2_fdc_read(void *token, offs_t offset)
{
	return applefdc_r(offset);
}



static void apple2_fdc_write(void *token, offs_t offset, UINT8 data)
{
	applefdc_w(offset, data);
}



const apple2_slotdevice apple2_slot_floppy525 =
{
	"floppy525",
	"5.25\" Floppy Drive",
	apple2_fdc_apple2_init,
	NULL,
	apple2_fdc_read,
	apple2_fdc_write
};



const apple2_slotdevice apple2_slot_iwm =
{
	"iwm",
	"IWM",
	apple2_fdc_iwm_init,
	NULL,
	apple2_fdc_read,
	apple2_fdc_write
};



/* -----------------------------------------------------------------------
 * Driver init
 * ----------------------------------------------------------------------- */

void apple2_init_common(running_machine *machine, const apple2_config *config)
{
	int i;
	void *token;

	a2 = 0;

	AY3600_init();
	add_reset_callback(machine, apple2_reset);

	/* copy configuration */
	a2_config = auto_malloc(sizeof(*config));
	memcpy(a2_config, config, sizeof(*config));

	/* state save registers */
	state_save_register_global(a2);
	state_save_register_func_postload(apple2_update_memory);

	/* apple2 behaves much better when the default memory is zero */
	memset(mess_ram, 0, mess_ram_size);

	/* initialize slots */
	a2_slot_tokens = auto_malloc(sizeof(*a2_slot_tokens) * APPLE2_SLOT_COUNT);
	memset(a2_slot_tokens, 0, sizeof(*a2_slot_tokens) * APPLE2_SLOT_COUNT);
	for (i = 0; i < APPLE2_SLOT_COUNT; i++)
	{
		if (a2_config->slots[i])
		{
			if (a2_config->slots[i]->init)
				token = a2_config->slots[i]->init(i);
			else
				token = (void *) ~0;
				
			a2_slot_tokens[i] = token;
		}
	}
	
	/* --------------------------------------------- *
	 * set up the softswitch mask/set                *
	 * --------------------------------------------- */
	a2_mask = ~0;
	a2_set = 0;
	
	/* disable VAR_ROMSWITCH if the ROM is only 16k */
	if (memory_region_length(REGION_CPU1) < 0x8000)
		a2_mask &= ~VAR_ROMSWITCH;

	if (mess_ram_size <= 64*1024)
		a2_mask &= ~(VAR_RAMRD | VAR_RAMWRT | VAR_80STORE | VAR_ALTZP | VAR_80COL);
}



MACHINE_START( apple2 )
{
	apple2_memmap_config mem_cfg;
	apple2_config a2_cfg;
	void *apple2cp_ce00_ram = NULL;
	
	memset(&a2_cfg, 0, sizeof(a2_cfg));

	/* specify slots */
	if (!strcmp(Machine->gamedrv->name, "apple2c0") ||
		!strcmp(Machine->gamedrv->name, "apple2c3") ||
		!strcmp(Machine->gamedrv->name, "apple2cp"))
	{
		a2_cfg.slots[0] = &apple2_slot_langcard;
		a2_cfg.slots[4] = &apple2_slot_mockingboard;
		a2_cfg.slots[6] = &apple2_slot_iwm;
	}
	else
	{
		a2_cfg.slots[0] = &apple2_slot_langcard;
		a2_cfg.slots[4] = &apple2_slot_mockingboard;
		a2_cfg.slots[6] = &apple2_slot_floppy525;
	}

	/* there appears to be some hidden RAM that is swapped in on the Apple
	 * IIc plus; I have not found any official documentation but the BIOS
	 * clearly uses this area as writeable memory */
	if (!strcmp(Machine->gamedrv->name, "apple2cp"))
		apple2cp_ce00_ram = auto_malloc(0x200);

	apple2_init_common(machine, &a2_cfg);

	/* setup memory */
	memset(&mem_cfg, 0, sizeof(mem_cfg));
	mem_cfg.first_bank = 1;
	mem_cfg.memmap = apple2_memmap_entries;
	mem_cfg.auxmem = apple2cp_ce00_ram;
	apple2_setup_memory(&mem_cfg);

	/* perform initial reset */
	apple2_reset(machine);
	return 0;
}

