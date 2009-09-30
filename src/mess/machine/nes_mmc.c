/*****************************************************************************************

    NES MMC Emulation


    MESS source file to handle various Multi-Memory Controllers (aka Mappers) used by NES carts.

    Many information about the mappers below come from the wonderful doc written by Disch.
    Current info (when used) are based on v0.6.1 of his docs.
    You can find the latest version of the doc at http://www.romhacking.net/docs/362/


    Known issues on specific mappers:

    * 001 AD&D Hillsfar seems to be broken.
    * 002, 003 Bus conflict?
    * 005 has some issues (e.g. Just Breed is not working), RAM banking needs hardware flags to determine size
    * 015 Shanghai Tycoon has corrupted graphics
    * 016 some graphics problem (e.g. Rokudenashi Blues misses some element of graphics)
    * 018 Pizza Pop and Plasma Ball shows graphics issues for this mapper
    * 024, 026, 085 Registers of the additional sound hardware are not emulated
    * 032 Major League is not properly working: check missing windows or field graphics in top view screens
          (it requires one hack to support both boards using this mapper)
    * 033 has still some graphics problem (e.g. missing text in Akira)
    * 034 Impossible Mission 2 is not working
    * 038 seems to miss inputs. separate reads?
    * 039 Study n Game 32 in 1 misses keyboard and starts differently than in NEStopia
    * 040 is not dealing correctly 0x6000-0x7fff (this got broken long ago)
    * 042 Ai Senshi Nicol has broken graphics (our Mapper 42 implementation does not handle CHR banks)
    * 045 is not fully working
    * 047 selection menu is totally glitched (unique regression left from 0.130u2 changes)
    * 051 only half of the games work
    * 064 has some IRQ problems - see Skull & Crossbones. Klax has broken graphics.
    * 065 Spartan X 2 has various glitches
    * 067 display issues, but vrom fixed
    * 071 Fire Hawk is not properly working (it requires one hack to support both boards using this mapper)
    * 072, 086, 092 lack samples support (maybe others as well)
    * 074 is only preliminar (no correct CHR swap)
    * 077 Requires 4-screen mirroring. Currently, it is very glitchy
    * 078 Cosmo Carrier is not working (it requires one hack to support both boards using this mapper).
          also, we lack NT mirroring
    * 082 has chr-rom banking problems. Also mapper is in the middle of sram, which is unemulated.
    * 083 has serious glitches
    * 091 lacks IRQ needed by MK2
    * 094 Bus conflict?
    * 096 is preliminary (no correct chr switch in connection to NT)
    * 097 Bus conflict?
    * 104 Not all the games work
    * 107 Are the scrolling glitches (check status bar) correct? NEStopia behaves similarly
    * 112 Master Shooter is not working and misses some graphics
    * 113 is not following docs (missing NT mirroring and high CHR bit)
    * 115 (formerly 248) is preliminary
    * 118 mirroring is a guess, chr-rom banking is likely different
    * 143 are dancing block borders (in the intro) correct?
    * 146 Pyramid 2 does not work
    * 150 some problems (Magic Cubes, Mahjong Academy, others?), maybe mirroring related
    * 152 Bus conflict?
    * 164 preliminary - no sprites?
    * 176 has some graphics problem
    * 178 Fan Kong Jin Ying is not working (but not even in NEStopia)
    * 180 Crazy Climber controller?
    * 188 needs mic input (reads from 0x6000-0x7fff)
    * 225 115-in-1 has glitches in the menu (games seem fine)
    * 228 seems wrong. e.g. Action52 starts with a game rather than in the menu
    * 229 is preliminary
    * 230 not working yet (needs a value to flip at reset)
    * 231 not working yet
    * 232 has graphics glitches
    * 241 Commandos is not working (but not even in NEStopia)
    * 242 DQ7 is not working (it requires one hack to support both boards using this mapper)
    * 243 has glitches in Sachen logo
    * 255 does not really select game (same in NEStopia apparently)

    Details to investigate:
    * 034 writes to 0x8000-0xffff should not be used for NINA-001 and BNROM, only unlicensed BxROM...
    * 144 we ignore writes to 0x8000 while NEStopia does not. is it a problem?
    * 240 we do not map writes to 0x4020-0x40ff. is this a problem?

    Remember that the MMC # does not equal the mapper #. In particular, Mapper 4 is
    in fact MMC3, Mapper 9 is MMC2 and Mapper 10 is MMC4. Makes perfect sense, right?

    TODO:
    - add more info
    - complete the removal of global variable
    - add missing mappers

****************************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "video/ppu2c0x.h"
#include "includes/nes.h"
#include "nes_mmc.h"
#include "sound/nes_apu.h"

#ifdef MAME_DEBUG
#define VERBOSE 1
#else
#define VERBOSE 0
#endif

#define LOG_MMC(x) do { if (VERBOSE) logerror x; } while (0)
#define LOG_FDS(x) do { if (VERBOSE) logerror x; } while (0)

/*PPU fast banking constants and structures */

#define CHRROM 0
#define CHRRAM 1

typedef struct
{
	int source;	//defines source of base pointer
	int origin; //defines offset of 0x400 byte segment at base pointer
	UINT8* access;	//source translated + origin -> valid pointer!
} chr_bank;

/*PPU nametable fast banking constants and structures */

#define CIRAM 0
#define ROM 1
#define EXRAM 2
#define MMC5FILL 3
#define CART_NTRAM 4

typedef struct
{
	int source;		/* defines source of base pointer */
	int origin;		/* defines offset of 0x400 byte segment at base pointer */
	int writable;	/* ExRAM, at least, can be write-protected AND used as nametable */
	UINT8* access;	/* direct access when possible */
} name_table;

/* Global variables */
static int prg_mask;

static chr_bank chr_map[8];  //quick banking structure, because some of this changes multiple times per scanline!
static name_table nt_page[4];  //quick banking structure for a maximum of 4K of RAM/ROM/ExRAM

static int IRQ_enable, IRQ_enable_latch;
static UINT16 IRQ_count, IRQ_count_latch, IRQ_reload;
static UINT8 IRQ_status;
static UINT8 IRQ_mode;

int MMC1_extended;	/* 0 = normal MMC1 cart, 1 = 512k MMC1, 2 = 1024k MMC1 */

write8_space_func mmc_write_low;
read8_space_func mmc_read_low;
write8_space_func mmc_write_mid;
read8_space_func mmc_read_mid;
write8_space_func mmc_write;

static int vrom_bank[16];
static int mult1, mult2;

/* common local variables */
static UINT8 mmc_chr_source;				// this is set at init to CHRROM or CHRRAM. a few mappers can swap between
								// the two (this is done in the specific handlers)
static UINT8 mmc_cmd1, mmc_cmd2;			// these represent registers where the mapper writes important values
static UINT8 mmc_bank_latch1, mmc_bank_latch2;	// some mappers change banks by writing subsets of these bits

// FIXME: mapper specific variables should be unified as much as possible to make easier future implementation of save states
/* Local variables (a few more are defined just before handlers that use them...) */
static int MMC1_Size_16k;
static int MMC1_High;
static int MMC1_reg;
static int MMC1_reg_count;
static int MMC1_Switch_Low, MMC1_SizeVrom_4k;
static int MMC1_bank1, MMC1_bank2, MMC1_bank3, MMC1_bank4;
static int MMC1_extended_bank;
static int MMC1_extended_base;
static int MMC1_extended_swap;

static int MMC2_bank0, MMC2_bank0_hi, MMC2_bank0_latch, MMC2_bank1, MMC2_bank1_hi, MMC2_bank1_latch;

static int MMC3_cmd;
static int MMC3_prg0, MMC3_prg1;
static int MMC3_chr[6];
static int MMC3_prg_base, MMC3_prg_mask;
static int MMC3_chr_base, MMC3_chr_mask;
UINT8* extended_ntram;

static int MMC5_rom_bank_mode;
static int MMC5_vrom_bank_mode;
static int MMC5_vram_protect;
static int MMC5_scanline;
static int MMC5_floodtile;
static int MMC5_floodattr;
UINT8 MMC5_vram[0x400];
int MMC5_vram_control;

static int mapper45_data[4], mapper45_cmd;

static int mapper64_data[0x10], mapper64_cmd;

static int mapper83_data[10];
static int mapper83_low_data[4];

static int mapper_warning;

static emu_timer	*nes_irq_timer;


/*************************************************************

    Base emulation (see drivers/nes.c):
    memory 0x0000-0x1fff RAM
           0x2000-0x3fff PPU
           0x4000-0x4017 APU
           0x4018-0x5fff Expansion Area
           0x6000-0x7fff SRAM
           0x8000-0xffff PRG ROM

    nes_chr_r/w take care of RAM, nes_nt_r/w take care of PPU.
    for mapper specific emulation, we generically setup the
    following handlers:
    - nes_low_mapper_r/w take care of accesses to 0x4100-0x5fff
    - nes_mid_mapper_r/w take care of accesses to 0x6000-0x7fff
    - nes_mapper_w takes care of writes to 0x8000-0xffff
    some mappers may access 0x4018-0x4100: this must be taken
    care separately in init_nes_core (see e.g. mapper 40)

*************************************************************/

static TIMER_CALLBACK( nes_irq_callback )
{
	cputag_set_input_line(machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
	timer_adjust_oneshot(nes_irq_timer, attotime_never, 0);
}

WRITE8_HANDLER( nes_chr_w )
{
	int bank = offset >> 10;
	if(chr_map[bank].source == CHRRAM)
	{
		chr_map[bank].access[offset & 0x3ff] = data;
	}
}

READ8_HANDLER( nes_chr_r )
{
	int bank = offset >> 10;
	return chr_map[bank].access[offset & 0x3ff];
}

WRITE8_HANDLER( nes_nt_w )
{
	int page = ((offset & 0xc00) >> 10);

	if (nt_page[page].writable == 0)
		return;

	nt_page[page].access[offset & 0x3ff] = data;
}

READ8_HANDLER( nes_nt_r )
{
	int page = ((offset & 0xc00) >> 10);
	if (nt_page[page].source == MMC5FILL)
	{
		if ((offset & 0x3ff) >= 0x3c0)
			return MMC5_floodattr;

		return MMC5_floodtile;
	}
	return nt_page[page].access[offset & 0x3ff];
}

WRITE8_HANDLER( nes_low_mapper_w )
{
	if (mmc_write_low)
		(*mmc_write_low)(space, offset, data);
	else
	{
		logerror("Unimplemented LOW mapper write, offset: %04x, data: %02x\n", offset, data);
#ifdef MAME_DEBUG
		if (!mapper_warning)
		{
			logerror("This game is writing to the low mapper area but no mapper is set. You may get better results by switching to a valid mapper.\n");
			mapper_warning = 1;
		}
#endif
	}
}

READ8_HANDLER( nes_low_mapper_r )
{
	if (mmc_read_low)
		return (*mmc_read_low)(space, offset);
	else
		logerror("low mapper area read, addr: %04x\n", offset + 0x4100);

	return 0;
}

WRITE8_HANDLER( nes_mid_mapper_w )
{
	if (mmc_write_mid)
		(*mmc_write_mid)(space, offset, data);
	else if (nes.mid_ram_enable)
		nes_battery_ram[offset] = data;
	else
	{
		logerror("Unimplemented MID mapper write, offset: %04x, data: %02x\n", offset, data);
#ifdef MAME_DEBUG
		if (!mapper_warning)
		{
			logerror("This game is writing to the MID mapper area but no mapper is set. You may get better results by switching to a valid mapper or changing the battery flag for this ROM.\n");
			mapper_warning = 1;
		}
#endif
	}
}

READ8_HANDLER( nes_mid_mapper_r )
{
	if ((nes.mid_ram_enable) || (nes.mapper == 5))
		return nes_battery_ram[offset];
	else
		return 0;
}

WRITE8_HANDLER( nes_mapper_w )
{
	if (mmc_write)
		(*mmc_write)(space, offset, data);
	else
	{
		logerror("Unimplemented mapper write, offset: %04x, data: %02x\n", offset, data);
#if 1
		if (!mapper_warning)
		{
			logerror("This game is writing to the mapper area but no mapper is set. You may get better results by switching to a valid mapper.\n");
			mapper_warning = 1;
		}

		switch (offset)
		{
			/* Hacked mapper for the 24-in-1 NES_ROM. */
			/* It's really 35-in-1, and it's mostly different versions of Battle City. Unfortunately, the vrom dumps are bad */
			case 0x7fde:
				data &= (nes.prg_chunks - 1);
				memory_set_bankptr(space->machine, 3, &nes.rom[data * 0x4000 + 0x10000]);
				memory_set_bankptr(space->machine, 4, &nes.rom[data * 0x4000 + 0x12000]);
				break;
		}
#endif
	}
}


/*************************************************************

    Helpers to handle MMC

*************************************************************/

/* PRG ROM in 8K, 16K or 32K blocks */

static void prg32( running_machine *machine, int bank )
{
	/* assumes that bank references a 32k chunk */
	bank &= ((nes.prg_chunks >> 1) - 1);
	if (nes.slow_banking)
		memcpy(&nes.rom[0x8000], &nes.rom[bank * 0x8000 + 0x10000], 0x8000);
	else
	{
		memory_set_bankptr(machine, 1, &nes.rom[bank * 0x8000 + 0x10000]);
		memory_set_bankptr(machine, 2, &nes.rom[bank * 0x8000 + 0x12000]);
		memory_set_bankptr(machine, 3, &nes.rom[bank * 0x8000 + 0x14000]);
		memory_set_bankptr(machine, 4, &nes.rom[bank * 0x8000 + 0x16000]);
	}
}

static void prg16_89ab( running_machine *machine, int bank )
{
	/* assumes that bank references a 16k chunk */
	bank &= (nes.prg_chunks - 1);
	if (nes.slow_banking)
		memcpy(&nes.rom[0x8000], &nes.rom[bank * 0x4000 + 0x10000], 0x4000);
	else
	{
		memory_set_bankptr(machine, 1, &nes.rom[bank * 0x4000 + 0x10000]);
		memory_set_bankptr(machine, 2, &nes.rom[bank * 0x4000 + 0x12000]);
	}
}

static void prg16_cdef( running_machine *machine, int bank )
{
	/* assumes that bank references a 16k chunk */
	bank &= (nes.prg_chunks - 1);
	if (nes.slow_banking)
		memcpy(&nes.rom[0xc000], &nes.rom[bank * 0x4000 + 0x10000], 0x4000);
	else
	{
		memory_set_bankptr(machine, 3, &nes.rom[bank * 0x4000 + 0x10000]);
		memory_set_bankptr(machine, 4, &nes.rom[bank * 0x4000 + 0x12000]);
	}
}

static void prg8_89( running_machine *machine, int bank )
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	if (nes.slow_banking)
		memcpy(&nes.rom[0x8000], &nes.rom[bank * 0x2000 + 0x10000], 0x2000);
	else
		memory_set_bankptr(machine, 1, &nes.rom[bank * 0x2000 + 0x10000]);
}

static void prg8_ab( running_machine *machine, int bank )
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	if (nes.slow_banking)
		memcpy(&nes.rom[0xa000], &nes.rom[bank * 0x2000 + 0x10000], 0x2000);
	else
		memory_set_bankptr(machine, 2, &nes.rom[bank * 0x2000 + 0x10000]);
}

static void prg8_cd( running_machine *machine, int bank )
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	if (nes.slow_banking)
		memcpy(&nes.rom[0xc000], &nes.rom[bank * 0x2000 + 0x10000], 0x2000);
	else
		memory_set_bankptr(machine, 3, &nes.rom[bank * 0x2000 + 0x10000]);
}

static void prg8_ef( running_machine *machine, int bank )
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	if (nes.slow_banking)
		memcpy(&nes.rom[0xe000], &nes.rom[bank * 0x2000 + 0x10000], 0x2000);
	else
		memory_set_bankptr(machine, 4, &nes.rom[bank * 0x2000 + 0x10000]);
}

/* CHR ROM in 1K, 2K, 4K or 8K blocks */

static void chr8( running_machine *machine, int bank, int source )
{
	int i;

	bank &= (nes.chr_chunks - 1);
	for (i = 0; i < 8; i++)
	{
		chr_map[i].source = source;
		chr_map[i].origin = (bank * 0x2000) + (i * 0x400); // for save state uses!

		if (source == CHRRAM)
			chr_map[i].access = &nes.vram[chr_map[i].origin];
		else
			chr_map[i].access = &nes.vrom[chr_map[i].origin];
	}
}

static void chr4_x( running_machine *machine, int start, int bank, int source )
{
	int i;

	bank &= ((nes.chr_chunks << 1) - 1);
	for (i = 0; i < 4; i++)
	{
		chr_map[i + start].source = source;
		chr_map[i + start].origin = (bank * 0x1000) + (i * 0x400); // for save state uses!

		if (source == CHRRAM)
			chr_map[i+start].access = &nes.vram[chr_map[i + start].origin];
		else
			chr_map[i+start].access = &nes.vrom[chr_map[i + start].origin];
	}
}

static void chr4_0( running_machine *machine, int bank, int source )
{
	chr4_x(machine, 0, bank, source);
}

static void chr4_4( running_machine *machine, int bank, int source )
{
	chr4_x(machine, 4, bank, source);
}

static void chr2_x( running_machine *machine, int start, int bank, int source )
{
	int i;

	bank &= ((nes.chr_chunks << 2) - 1);
	for (i = 0; i < 2; i++)
	{
		chr_map[i + start].source = source;
		chr_map[i + start].origin = (bank * 0x800) + (i * 0x400);

		if (source == CHRRAM)
			chr_map[i + start].access = &nes.vram[chr_map[i + start].origin];
		else
			chr_map[i + start].access = &nes.vrom[chr_map[i + start].origin];
	}
}

static void chr2_0( running_machine *machine, int bank, int source )
{
	chr2_x(machine, 0, bank, source);
}

static void chr2_2( running_machine *machine, int bank, int source )
{
	chr2_x(machine, 2, bank, source);
}

static void chr2_4( running_machine *machine, int bank, int source )
{
	chr2_x(machine, 4, bank, source);
}

static void chr2_6( running_machine *machine, int bank, int source )
{
	chr2_x(machine, 6, bank, source);
}

static void chr1_x( running_machine *machine, int start, int bank, int source )
{
	chr_map[start].source = source;
	bank &= ((nes.chr_chunks << 3) - 1);
	chr_map[start].origin = bank * 0x400;

	if (source == CHRRAM)
		chr_map[start].access = &nes.vram[chr_map[start].origin];
	else
		chr_map[start].access = &nes.vrom[chr_map[start].origin];
}

static void chr1_0 (running_machine *machine, int bank, int source)
{
	chr1_x(machine, 0, bank, source);
}

static void chr1_1( running_machine *machine, int bank, int source )
{
	chr1_x(machine, 1, bank, source);
}

static void chr1_2( running_machine *machine, int bank, int source )
{
	chr1_x(machine, 2, bank, source);
}

static void chr1_3( running_machine *machine, int bank, int source )
{
	chr1_x(machine, 3, bank, source);
}

static void chr1_4( running_machine *machine, int bank, int source )
{
	chr1_x(machine, 4, bank, source);
}

static void chr1_5( running_machine *machine, int bank, int source )
{
	chr1_x(machine, 5, bank, source);
}

static void chr1_6( running_machine *machine, int bank, int source )
{
	chr1_x(machine, 6, bank, source);
}

static void chr1_7( running_machine *machine, int bank, int source )
{
	chr1_x(machine, 7, bank, source);
}


/* NameTable paging and mirroring */

static void set_nt_page( int page, int source, int bank, int writable )
{
	UINT8* base_ptr;

	switch (source)
	{
		case CART_NTRAM:
			base_ptr = extended_ntram;
			break;
		case MMC5FILL:
			base_ptr = NULL;
			break;
		case ROM:
			base_ptr = nes.vrom;
			break;
		case EXRAM:
			base_ptr = MMC5_vram;
			break;
		case CIRAM:
		default:
			base_ptr = nes.ciram;
			break;
	}

	page &= 3; /* mask down to the 4 logical pages */
	nt_page[page].source = source;
	if (NULL != base_ptr)
	{
		nt_page[page].origin = bank * 0x400;
		nt_page[page].access = base_ptr + nt_page[page].origin;
	}
	nt_page[page].writable = writable;
}

void set_nt_mirroring( int mirroring )
{
	/* setup our videomem handlers based on mirroring */
	switch (mirroring)
	{
		case PPU_MIRROR_VERT:
			set_nt_page(0, CIRAM, 0, 1);
			set_nt_page(1, CIRAM, 1, 1);
			set_nt_page(2, CIRAM, 0, 1);
			set_nt_page(3, CIRAM, 1, 1);
			break;

		case PPU_MIRROR_HORZ:
			set_nt_page(0, CIRAM, 0, 1);
			set_nt_page(1, CIRAM, 0, 1);
			set_nt_page(2, CIRAM, 1, 1);
			set_nt_page(3, CIRAM, 1, 1);
			break;

		case PPU_MIRROR_HIGH:
			set_nt_page(0, CIRAM, 1, 1);
			set_nt_page(1, CIRAM, 1, 1);
			set_nt_page(2, CIRAM, 1, 1);
			set_nt_page(3, CIRAM, 1, 1);
			break;

		case PPU_MIRROR_LOW:
			set_nt_page(0, CIRAM, 0, 1);
			set_nt_page(1, CIRAM, 0, 1);
			set_nt_page(2, CIRAM, 0, 1);
			set_nt_page(3, CIRAM, 0, 1);
			break;

		case PPU_MIRROR_NONE:
		case PPU_MIRROR_4SCREEN:
		default:
			/* external RAM needs to be used somehow. */
			/* but as a default, we'll arbitrarily set vertical so as not to crash*/
			/* mappers should use set_nt_page and assign which pages are which */

			logerror("Mapper set 4-screen mirroring without supplying external nametable memory!\n");

			set_nt_page(0, CIRAM, 0, 1);
			set_nt_page(1, CIRAM, 0, 1);
			set_nt_page(2, CIRAM, 1, 1);
			set_nt_page(3, CIRAM, 1, 1);
			break;
	}
}

/*  Other custom mirroring helpers are defined below: Waixing games use waixing_set_mirror (which swaps
    MIRROR_HIGH and MIRROR_LOW compared to the above) and Sachen games use sachen_set_mirror (which has
    a slightly different MIRROR_HIGH, with page 0 set to 0) */


/*************************************************************

    Mapper 0

    32K PRG ROM + 8K or less CHR ROM
    No need of additional MMC

    Known Boards: NROM, FB02??, FB04??, Jaleco JF01, JF02, JF03, JF04
    Games: Mario Bros., Super Mario Bros., Tennis and most of
          the first generation games

    In MESS: Supported, no need of specific handlers or IRQ

*************************************************************/

/*************************************************************

    Mapper 1

    Known Boards: MMC1 based Boards, i.e. SAROM, SBROM, SCROM,
          SEROM, SFROM, SGROM, SHROM, SJROM, SKROM, SLROM, SNROM,
          SOROM, SUROM, SXROM, WH??
    Games: Faxanadu, Final Fantasy I & II, Kid Icarus, Legend
          of Zelda, Metroid

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( mapper1_w )
{
	int reg;

	/* Find which register we are dealing with */
	/* Note that there is only one latch and shift counter, shared amongst the 4 regs */
	/* Space Shuttle will not work if they have independent variables. */
	reg = (offset >> 13);

	if (data & 0x80)
	{
		MMC1_reg_count = 0;
		MMC1_reg = 0;

		/* Set these to their defaults - needed for Robocop 3, Dynowars */
		MMC1_Size_16k = 1;
		MMC1_Switch_Low = 1;
		/* TODO: should we switch banks at this time also? */
		LOG_MMC(("=== MMC1 regs reset to default\n"));
		return;
	}

	if (MMC1_reg_count < 5)
	{
		if (MMC1_reg_count == 0) MMC1_reg = 0;
		MMC1_reg >>= 1;
		MMC1_reg |= (data & 0x01) ? 0x10 : 0x00;
		MMC1_reg_count ++;
	}

	if (MMC1_reg_count == 5)
	{
//      LOG_MMC(("   MMC1 reg#%02x val:%02x\n", offset, MMC1_reg));
		switch (reg)
		{
			case 0:
				MMC1_Switch_Low = MMC1_reg & 0x04;
				MMC1_Size_16k =   MMC1_reg & 0x08;

				switch (MMC1_reg & 0x03)
				{
					case 0: set_nt_mirroring(PPU_MIRROR_LOW); break;
					case 1: set_nt_mirroring(PPU_MIRROR_HIGH); break;
					case 2: set_nt_mirroring(PPU_MIRROR_VERT); break;
					case 3: set_nt_mirroring(PPU_MIRROR_HORZ); break;
				}

				MMC1_SizeVrom_4k = (MMC1_reg & 0x10);
				LOG_MMC(("   MMC1 reg #1 val:%02x\n", MMC1_reg));
					LOG_MMC(("\t\tBank Size: "));
					if (MMC1_Size_16k)
						LOG_MMC(("16k\n"));
					else LOG_MMC(("32k\n"));

					LOG_MMC(("\t\tBank Select: "));
					if (MMC1_Switch_Low)
						LOG_MMC(("$8000\n"));
					else LOG_MMC(("$C000\n"));

					LOG_MMC(("\t\tVROM Bankswitch Size Select: "));
					if (MMC1_SizeVrom_4k)
						LOG_MMC(("4k\n"));
					else LOG_MMC(("8k\n"));

					LOG_MMC(("\t\tMirroring: %d\n", MMC1_reg & 0x03));
				break;

			case 1:
				MMC1_extended_bank = (MMC1_extended_bank & ~0x01) | ((MMC1_reg & 0x10) >> 4);
				if (MMC1_extended == 2)
				{
					/* MMC1_SizeVrom_4k determines if we use the special 256k bank register */
				 	if (!MMC1_SizeVrom_4k)
				 	{
						/* Pick 1st or 4th 256k bank */
						MMC1_extended_base = 0xc0000 * (MMC1_extended_bank & 0x01) + 0x10000;
						memory_set_bankptr(space->machine, 1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
						memory_set_bankptr(space->machine, 2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
						memory_set_bankptr(space->machine, 3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
						memory_set_bankptr(space->machine, 4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
						LOG_MMC(("MMC1_extended 1024k bank (no reg) select: %02x\n", MMC1_extended_bank));
					}
					else
					{
						/* Set 256k bank based on the 256k bank select register */
						if (MMC1_extended_swap)
						{
							MMC1_extended_base = 0x40000 * MMC1_extended_bank + 0x10000;
							memory_set_bankptr(space->machine, 1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
							memory_set_bankptr(space->machine, 2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
							memory_set_bankptr(space->machine, 3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
							memory_set_bankptr(space->machine, 4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
							LOG_MMC(("MMC1_extended 1024k bank (reg 1) select: %02x\n", MMC1_extended_bank));
							MMC1_extended_swap = 0;
						}
						else MMC1_extended_swap = 1;
					}
				}
				else if (MMC1_extended == 1 && nes.chr_chunks == 0)
				{
					/* Pick 1st or 2nd 256k bank */
					MMC1_extended_base = 0x40000 * (MMC1_extended_bank & 0x01) + 0x10000;
					memory_set_bankptr(space->machine, 1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
					memory_set_bankptr(space->machine, 2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
					memory_set_bankptr(space->machine, 3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
					memory_set_bankptr(space->machine, 4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
					LOG_MMC(("MMC1_extended 512k bank select: %02x\n", MMC1_extended_bank & 0x01));
				}
				else if (nes.chr_chunks > 0)
				{
//                  LOG_MMC(("MMC1_SizeVrom_4k: %02x bank:%02x\n", MMC1_SizeVrom_4k, MMC1_reg));

					if (!MMC1_SizeVrom_4k)
					{
						/* we take care of the actual bank size when we divide bank by 2 in the chr8 call */
						//int bank = MMC1_reg & (nes.chr_chunks - 1);   // this breaks Pinball Quest
						int bank = MMC1_reg & ((nes.chr_chunks << 1) - 1);
						chr8(space->machine, bank >> 1, CHRROM);
						LOG_MMC(("MMC1 8k VROM switch: %02x\n", MMC1_reg));
					}
					else
					{
						int bank = MMC1_reg & ((nes.chr_chunks << 1) - 1);
						chr4_0(space->machine, bank, CHRROM);
						LOG_MMC(("MMC1 4k VROM switch (low): %02x\n", MMC1_reg));
					}
				}
				break;
			case 2:
//              LOG_MMC(("MMC1_Reg_2: %02x\n",MMC1_Reg_2));
				MMC1_extended_bank = (MMC1_extended_bank & ~0x02) | ((MMC1_reg & 0x10) >> 3);
				if (MMC1_extended == 2 && MMC1_SizeVrom_4k)
				{
					if (MMC1_extended_swap)
					{
						/* Set 256k bank based on the 256k bank select register */
						MMC1_extended_base = 0x40000 * MMC1_extended_bank + 0x10000;
						memory_set_bankptr(space->machine, 1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
						memory_set_bankptr(space->machine, 2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
						memory_set_bankptr(space->machine, 3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
						memory_set_bankptr(space->machine, 4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
						LOG_MMC(("MMC1_extended 1024k bank (reg 2) select: %02x\n", MMC1_extended_bank));
						MMC1_extended_swap = 0;
					}
					else
						MMC1_extended_swap = 1;
				}
				if (MMC1_SizeVrom_4k)
				{
					int bank = MMC1_reg & ((nes.chr_chunks << 1) - 1);
					chr4_4(space->machine, bank, CHRROM);
					LOG_MMC(("MMC1 4k VROM switch (high): %02x\n", MMC1_reg));
				}
				break;
			case 3:
				/* Switching 1 32k bank of PRG ROM */
				MMC1_reg &= 0x0f;
				if (!MMC1_Size_16k)
				{
					int bank = MMC1_reg & (nes.prg_chunks - 1);

					MMC1_bank1 = bank * 0x4000;
					MMC1_bank2 = bank * 0x4000 + 0x2000;
					memory_set_bankptr(space->machine, 1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
					memory_set_bankptr(space->machine, 2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
					if (!MMC1_extended)
					{
						MMC1_bank3 = bank * 0x4000 + 0x4000;
						MMC1_bank4 = bank * 0x4000 + 0x6000;
						memory_set_bankptr(space->machine, 3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
						memory_set_bankptr(space->machine, 4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
					}
					LOG_MMC(("MMC1 32k bank select: %02x\n", MMC1_reg));
				}
				else
				/* Switching one 16k bank */
				{
					if (MMC1_Switch_Low)
					{
						int bank = MMC1_reg & (nes.prg_chunks - 1);

						MMC1_bank1 = bank * 0x4000;
						MMC1_bank2 = bank * 0x4000 + 0x2000;

						memory_set_bankptr(space->machine, 1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
						memory_set_bankptr(space->machine, 2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
						if (!MMC1_extended)
						{
							MMC1_bank3 = MMC1_High;
							MMC1_bank4 = MMC1_High + 0x2000;
							memory_set_bankptr(space->machine, 3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
							memory_set_bankptr(space->machine, 4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
						}
						LOG_MMC(("MMC1 16k-low bank select: %02x\n", MMC1_reg));
					}
					else
					{
						int bank = MMC1_reg & (nes.prg_chunks - 1);

						if (!MMC1_extended)
						{

							MMC1_bank1 = 0;
							MMC1_bank2 = 0x2000;
							MMC1_bank3 = bank * 0x4000;
							MMC1_bank4 = bank * 0x4000 + 0x2000;

							memory_set_bankptr(space->machine, 1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
							memory_set_bankptr(space->machine, 2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
							memory_set_bankptr(space->machine, 3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
							memory_set_bankptr(space->machine, 4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
						}
						LOG_MMC(("MMC1 16k-high bank select: %02x\n", MMC1_reg));
					}
				}

				LOG_MMC(("-- page1: %06x\n", MMC1_bank1));
				LOG_MMC(("-- page2: %06x\n", MMC1_bank2));
				LOG_MMC(("-- page3: %06x\n", MMC1_bank3));
				LOG_MMC(("-- page4: %06x\n", MMC1_bank4));
				break;
		}
		MMC1_reg_count = 0;
	}
}

/*************************************************************

    Mapper 2

    Known Boards: UNROM, UOROM, UXROM, Jaleco JF15, JF18 & JF39
    Games: Castlevania, Dragon Quest II, Duck Tales, MegaMan, Metal Gear

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( mapper2_w )
{
	prg16_89ab(space->machine, data);
}

/*************************************************************

    Mapper 3

    Known Boards: CNROM, CXROM, X79B?, Bandai Aerobics Board,
          Tengen 800008, Bootleg Board by NTDEC (N715062)
    Games: Adventure Island, Flipull, Friday 13th, GeGeGe no
          Kitarou, Ghostbusters, Gradius, Hokuto no Ken, Milon's
          Secret Castle

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( mapper3_w )
{
	chr8(space->machine, data, CHRROM);
}

/*************************************************************

    Mapper 4

    Known Boards: MMC3 based Boards, i.e. HKROM, TEROM, TBROM,
          TFROM, TGROM, TKROM, TLROM, TNROM, TR1ROM, TSROM,
          TVROM, TRXROM
    Games: Final Fantasy III, MegaMan 3,4,5 & 6, Startropics,
          Super Mario Bros. 2 & 3,

    In MESS: Supported

*************************************************************/

static void mapper4_set_prg( running_machine *machine )
{
	int prg0_bank = MMC3_prg_base | (MMC3_prg0 & MMC3_prg_mask);
	int prg1_bank = MMC3_prg_base | (MMC3_prg1 & MMC3_prg_mask);
	int last_bank = MMC3_prg_base | MMC3_prg_mask;

	if (MMC3_cmd & 0x40)
	{
		memory_set_bankptr(machine, 1, &nes.rom[(last_bank - 1) * 0x2000 + 0x10000]);
		memory_set_bankptr(machine, 3, &nes.rom[0x2000 * (prg0_bank) + 0x10000]);
	}
	else
	{
		memory_set_bankptr(machine, 1, &nes.rom[0x2000 * (prg0_bank) + 0x10000]);
		memory_set_bankptr(machine, 3, &nes.rom[(last_bank - 1) * 0x2000 + 0x10000]);
	}
	memory_set_bankptr(machine, 2, &nes.rom[0x2000 * (prg1_bank) + 0x10000]);
	memory_set_bankptr(machine, 4, &nes.rom[(last_bank) * 0x2000 + 0x10000]);
}

static void mapper4_set_chr( running_machine *machine, UINT8 chr )
{
	UINT8 chr_page = (MMC3_cmd & 0x80) >> 5;

	chr2_x(machine, chr_page ^ 0, MMC3_chr_base | ((MMC3_chr[0] >> 1) & MMC3_chr_mask), chr);
	chr2_x(machine, chr_page ^ 2, MMC3_chr_base | ((MMC3_chr[1] >> 1) & MMC3_chr_mask), chr);
	chr1_x(machine, chr_page ^ 4, MMC3_chr_base | (MMC3_chr[2]& MMC3_chr_mask), chr);
	chr1_x(machine, chr_page ^ 5, MMC3_chr_base | (MMC3_chr[3]& MMC3_chr_mask), chr);
	chr1_x(machine, chr_page ^ 6, MMC3_chr_base | (MMC3_chr[4]& MMC3_chr_mask), chr);
	chr1_x(machine, chr_page ^ 7, MMC3_chr_base | (MMC3_chr[5]& MMC3_chr_mask), chr);
}

static void mapper4_irq( const device_config *device, int scanline, int vblank, int blanked )
{
	if ((scanline < PPU_BOTTOM_VISIBLE_SCANLINE) /*|| (scanline == ppu_scanlines_per_frame-1)*/)
	{
		int priorCount = IRQ_count;
		if ((IRQ_count == 0) || IRQ_reload)
		{
			IRQ_count = IRQ_count_latch;
			IRQ_reload = 0;
		}
		else
			IRQ_count --;

		if (IRQ_enable && !blanked && (IRQ_count == 0) && priorCount)
		{
			LOG_MMC(("irq fired, scanline: %d (MAME %d, beam pos: %d)\n", scanline,
					video_screen_get_vpos(device->machine->primary_screen), video_screen_get_hpos(device->machine->primary_screen)));
			cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
//          timer_adjust_oneshot(nes_irq_timer, cputag_clocks_to_attotime(device->machine, "maincpu", 4), 0);
		}
	}
}

static WRITE8_HANDLER( mapper4_w )
{
	static UINT8 last_bank = 0xff;

//  LOG_MMC(("mapper4_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline));

	//only bits 14,13, and 0 matter for offset!
	switch (offset & 0x6001)
	{
		case 0x0000: /* $8000 */
			MMC3_cmd = data;

			/* Toggle between switching $8000 and $c000 */
			if (last_bank != (data & 0xc0))
			{
				/* Reset the banks */
				mapper4_set_prg(space->machine);
				mapper4_set_chr(space->machine, mmc_chr_source);
				LOG_MMC(("     MMC3 reset banks\n"));
			}
			last_bank = data & 0xc0;
			break;

		case 0x0001: /* $8001 */
		{
			UINT8 cmd = MMC3_cmd & 0x07;
			switch (cmd)
			{
				case 0: case 1:
					data &= 0xfe;
					MMC3_chr[cmd] = data;
					mapper4_set_chr(space->machine, mmc_chr_source);
					LOG_MMC(("     MMC3 set vram %d: %d\n", cmd, data));
					break;

				case 2: case 3: case 4: case 5:
					MMC3_chr[cmd] = data;
					mapper4_set_chr(space->machine, mmc_chr_source);
					LOG_MMC(("     MMC3 set vram %d: %d\n", cmd, data));
					break;

				case 6:
					MMC3_prg0 = data;
					mapper4_set_prg(space->machine);
					break;

				case 7:
					MMC3_prg1 = data;
					mapper4_set_prg(space->machine);
					break;
			}
			break;
		}
		case 0x2000: /* $a000 */
			if (data & 0x40)
				set_nt_mirroring(PPU_MIRROR_HIGH);
			else
			{
				if (data & 0x01)
					set_nt_mirroring(PPU_MIRROR_HORZ);
				else
					set_nt_mirroring(PPU_MIRROR_VERT);
			}
			break;

		case 0x2001: /* $a001 - extra RAM enable/disable */
			nes.mid_ram_enable = data;
			LOG_MMC(("     MMC3 mid_ram enable: %02x\n", 0));
			break;

		case 0x4000: /* $c000 - IRQ scanline counter */
			IRQ_count_latch = data;
//          LOG_MMC(("     MMC3 set irq count latch: %02x (scanline %d)\n", data, ppu2c0x_get_current_scanline(0)));
			break;

		case 0x4001: /* $c001 - IRQ scanline latch */
			IRQ_reload = 1;
//          LOG_MMC(("     MMC3 set irq reload (scanline %d)\n", ppu2c0x_get_current_scanline(0)));
			break;

		case 0x6000: /* $e000 - Disable IRQs */
			IRQ_enable = 0;
			LOG_MMC(("     MMC3 disable irqs\n"));
			break;

		case 0x6001: /* $e001 - Enable IRQs */
			IRQ_enable = 1;
			LOG_MMC(("     MMC3 enable irqs\n"));
			break;

		default:
			logerror("mapper4_w uncaught: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

/*************************************************************

    Mapper 5

    Known Boards:  MMC3 based Boards, i.e. ELROM, EKROM, ETROM,
          EWROM and EXROM variants
    Games: Castlevania III, Just Breed, many Koei titles

    In MESS: Partially supported

*************************************************************/

static void mapper5_irq( const device_config *device, int scanline, int vblank, int blanked )
{
#if 1
	if (scanline == 0)
		IRQ_status |= 0x40;
	else if (scanline > PPU_BOTTOM_VISIBLE_SCANLINE)
		IRQ_status &= ~0x40;
#endif

	if (scanline == IRQ_count)
	{
		if (IRQ_enable)
			cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);

		IRQ_status = 0xff;
	}
}

static void ppu_mirror_MMC5( int page, int src )
{
	switch (src)
	{
	case 0:	/* CIRAM0 */
		set_nt_page(page, CIRAM, 0, 1);
		break;
	case 1:	/* CIRAM1 */
		set_nt_page(page, CIRAM, 1, 1);
		break;
	case 2:	/* ExRAM */
		set_nt_page(page, EXRAM, 0, 1);	// actually only works during rendering.
		break;
	case 3: /* Fill Registers */
		set_nt_page(page, MMC5FILL, 0, 0);
		break;
	default:
		fatalerror("This should never happen");
		break;
	}
}

static READ8_HANDLER( mapper5_l_r )
{
	int retVal;

	/* $5c00 - $5fff: extended videoram attributes */
	if ((offset >= 0x1b00) && (offset <= 0x1eff))
	{
		return MMC5_vram[offset - 0x1b00];
	}

	switch (offset)
	{
		case 0x1104: /* $5204 */
#if 0
			if (current_scanline == MMC5_scanline)
				return 0x80;
			else
				return 0x00;
#else
			retVal = IRQ_status;
			IRQ_status &= ~0x80;
			return retVal;
#endif

		case 0x1105: /* $5205 */
			return (mult1 * mult2) & 0xff;
		case 0x1106: /* $5206 */
			return ((mult1 * mult2) & 0xff00) >> 8;

		default:
			logerror("** MMC5 uncaught read, offset: %04x\n", offset + 0x4100);
			return 0x00;
	}
}

//static void mapper5_sync_vrom (int mode)
//{
//  int i;
//
//  for (i = 0; i < 8; i ++)
//      nes_vram[i] = vrom_bank[0 + (mode * 8)] * 64;
//}

static WRITE8_HANDLER( mapper5_l_w )
{
	nes_state *state = space->machine->driver_data;

//  static int vrom_next[4];
	static int vrom_page_a;
	static int vrom_page_b;

//  LOG_MMC(("Mapper 5 write, offset: %04x, data: %02x\n", offset + 0x4100, data));
	/* Send $5000-$5015 to the sound chip */
	if ((offset >= 0xf00) && (offset <= 0xf15))
	{
		nes_psg_w (state->sound, offset & 0x1f, data);
		return;
	}

	/* $5c00 - $5fff: extended videoram attributes */
	if ((offset >= 0x1b00) && (offset <= 0x1eff))
	{
		if (MMC5_vram_protect == 0x03)
			MMC5_vram[offset - 0x1b00] = data;
		return;
	}

	switch (offset)
	{
		case 0x1000: /* $5100 */
			MMC5_rom_bank_mode = data & 0x03;
			LOG_MMC(("MMC5 rom bank mode: %02x\n", data));
			break;

		case 0x1001: /* $5101 */
			MMC5_vrom_bank_mode = data & 0x03;
			LOG_MMC(("MMC5 vrom bank mode: %02x\n", data));
			break;

		case 0x1002: /* $5102 */
			if (data == 0x02)
				MMC5_vram_protect |= 1;
			else
				MMC5_vram_protect = 0;
			LOG_MMC(("MMC5 vram protect 1: %02x\n", data));
			break;
		case 0x1003: /* 5103 */
			if (data == 0x01)
				MMC5_vram_protect |= 2;
			else
				MMC5_vram_protect = 0;
			LOG_MMC(("MMC5 vram protect 2: %02x\n", data));
			break;

		case 0x1004: /* $5104 - Extra VRAM (EXRAM) control */
			MMC5_vram_control = data;
			LOG_MMC(("MMC5 exram control: %02x\n", data));
			break;

		case 0x1005: /* $5105 */
			ppu_mirror_MMC5(0, data & 0x03);
			ppu_mirror_MMC5(1, (data & 0x0c) >> 2);
			ppu_mirror_MMC5(2, (data & 0x30) >> 4);
			ppu_mirror_MMC5(3, (data & 0xc0) >> 6);
			break;

		/* tile data for MMC5 flood-fill NT mode */
		case 0x1006:
			MMC5_floodtile=data;
			break;

		/* attr data for MMC5 flood-fill NT mode */
		case 0x1007:
			switch (data & 3)
			{
			default:
			case 0: MMC5_floodattr = 0x00; break;
			case 1: MMC5_floodattr = 0x55; break;
			case 2: MMC5_floodattr = 0xaa; break;
			case 3: MMC5_floodattr = 0xff; break;
			}
			break;

		case 0x1013: /* $5113 */
			LOG_MMC(("MMC5 mid RAM bank select: %02x\n", data & 0x07));
			memory_set_bankptr(space->machine, 5, &nes.wram[data * 0x2000]);
			/* The & 4 is a hack that'll tide us over for now */
			nes_battery_ram = &nes.wram[(data & 4) * 0x2000];
			break;

		case 0x1014: /* $5114 */
			LOG_MMC(("MMC5 $5114 bank select: %02x (mode: %d)\n", data, MMC5_rom_bank_mode));
			switch (MMC5_rom_bank_mode)
			{
				case 0x03:
					/* 8k switch */
					if (data & 0x80)
					{
						/* ROM */
						LOG_MMC(("\tROM bank select (8k, $8000): %02x\n", data));
						data &= ((nes.prg_chunks << 1) - 1);
						memory_set_bankptr(space->machine, 1, &nes.rom[data * 0x2000 + 0x10000]);
					}
					else
					{
						/* RAM */
						LOG_MMC(("\tRAM bank select (8k, $8000): %02x\n", data & 0x07));
						/* The & 4 is a hack that'll tide us over for now */
						memory_set_bankptr(space->machine, 1, &nes.wram[(data & 4) * 0x2000]);
					}
					break;
			}
			break;
		case 0x1015: /* $5115 */
			LOG_MMC(("MMC5 $5115 bank select: %02x (mode: %d)\n", data, MMC5_rom_bank_mode));
			switch (MMC5_rom_bank_mode)
			{
				case 0x01:
				case 0x02:
					if (data & 0x80)
					{
						/* 16k switch - ROM only */
						prg16_89ab(space->machine, (data & 0x7f) >> 1);
					}
					else
					{
						/* RAM */
						LOG_MMC(("\tRAM bank select (16k, $8000): %02x\n", data & 0x07));
						/* The & 4 is a hack that'll tide us over for now */
						memory_set_bankptr(space->machine, 1, &nes.wram[((data & 4) >> 1) * 0x4000]);
						memory_set_bankptr(space->machine, 2, &nes.wram[((data & 4) >> 1) * 0x4000 + 0x2000]);
					}
					break;
				case 0x03:
					/* 8k switch */
					if (data & 0x80)
					{
						/* ROM */
						data &= ((nes.prg_chunks << 1) - 1);
						memory_set_bankptr(space->machine, 2, &nes.rom[data * 0x2000 + 0x10000]);
					}
					else
					{
						/* RAM */
						LOG_MMC(("\tRAM bank select (8k, $a000): %02x\n", data & 0x07));
						/* The & 4 is a hack that'll tide us over for now */
						memory_set_bankptr(space->machine, 2, &nes.wram[(data & 4) * 0x2000]);
					}
					break;
			}
			break;
		case 0x1016: /* $5116 */
			LOG_MMC(("MMC5 $5116 bank select: %02x (mode: %d)\n", data, MMC5_rom_bank_mode));
			switch (MMC5_rom_bank_mode)
			{
				case 0x02:
				case 0x03:
					/* 8k switch */
					if (data & 0x80)
					{
						/* ROM */
						data &= ((nes.prg_chunks << 1) - 1);
						memory_set_bankptr(space->machine, 3, &nes.rom[data * 0x2000 + 0x10000]);
					}
					else
					{
						/* RAM */
						LOG_MMC(("\tRAM bank select (8k, $c000): %02x\n", data & 0x07));
						/* The & 4 is a hack that'll tide us over for now */
						memory_set_bankptr(space->machine, 3, &nes.wram[(data & 4)* 0x2000]);
					}
					break;
			}
			break;
		case 0x1017: /* $5117 */
			LOG_MMC(("MMC5 $5117 bank select: %02x (mode: %d)\n", data, MMC5_rom_bank_mode));
			switch (MMC5_rom_bank_mode)
			{
				case 0x00:
					/* 32k switch - ROM only */
					prg32(space->machine, data >> 2);
					break;
				case 0x01:
					/* 16k switch - ROM only */
					prg16_cdef(space->machine, data >> 1);
					break;
				case 0x02:
				case 0x03:
					/* 8k switch */
					data &= ((nes.prg_chunks << 1) - 1);
					memory_set_bankptr(space->machine, 4, &nes.rom[data * 0x2000 + 0x10000]);
					break;
			}
			break;
		case 0x1020: /* $5120 */
			LOG_MMC(("MMC5 $5120 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[0] = data;
//                  mapper5_sync_vrom(0);
					chr1_0(space->machine, vrom_bank[0], CHRROM);
//                  ppu2c0x_set_videorom_bank(state->ppu, 0, 1, vrom_bank[0], 64);
//                  nes_vram_sprite[0] = vrom_bank[0] * 64;
//                  vrom_next[0] = 4;
//                  vrom_page_a = 1;
//                  vrom_page_b = 0;
					break;
			}
			break;
		case 0x1021: /* $5121 */
			LOG_MMC(("MMC5 $5121 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x02:
					/* 2k switch */
					chr2_0 (space->machine, data, CHRROM);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[1] = data;
//                  mapper5_sync_vrom(0);
					chr1_1(space->machine, vrom_bank[1], CHRROM);
					//ppu2c0x_set_videorom_bank(state->ppu, 1, 1, vrom_bank[1], 64);
//                  nes_vram_sprite[1] = vrom_bank[0] * 64;
//                  vrom_next[1] = 5;
//                  vrom_page_a = 1;
//                  vrom_page_b = 0;
					break;
			}
			break;
		case 0x1022: /* $5122 */
			LOG_MMC(("MMC5 $5122 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[2] = data;
//                  mapper5_sync_vrom(0);
					chr1_2(space->machine, vrom_bank[2], CHRROM);
//                  ppu2c0x_set_videorom_bank(state->ppu, 2, 1, vrom_bank[2], 64);
//                  nes_vram_sprite[2] = vrom_bank[0] * 64;
//                  vrom_next[2] = 6;
//                  vrom_page_a = 1;
//                  vrom_page_b = 0;
					break;
			}
			break;
		case 0x1023: /* $5123 */
			LOG_MMC(("MMC5 $5123 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x01:
					chr4_0 (space->machine, data, CHRROM);
					break;
				case 0x02:
					/* 2k switch */
					chr2_2 (space->machine, data, CHRROM);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[3] = data;
//                  mapper5_sync_vrom(0);
					chr1_3(space->machine, vrom_bank[3], CHRROM);
//                  ppu2c0x_set_videorom_bank(state->ppu, 3, 1, vrom_bank[3], 64);
//                  nes_vram_sprite[3] = vrom_bank[0] * 64;
//                  vrom_next[3] = 7;
//                  vrom_page_a = 1;
//                  vrom_page_b = 0;
					break;
			}
			break;
		case 0x1024: /* $5124 */
			LOG_MMC(("MMC5 $5124 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[4] = data;
//                  mapper5_sync_vrom(0);
					chr1_4(space->machine, vrom_bank[4], CHRROM);
//                  ppu2c0x_set_videorom_bank(state->ppu, 4, 1, vrom_bank[4], 64);
//                  nes_vram_sprite[4] = vrom_bank[0] * 64;
//                  vrom_next[0] = 0;
//                  vrom_page_a = 0;
//                  vrom_page_b = 0;
					break;
			}
			break;
		case 0x1025: /* $5125 */
			LOG_MMC(("MMC5 $5125 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x02:
					/* 2k switch */
					chr2_4 (space->machine, data, CHRROM);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[5] = data;
//                  mapper5_sync_vrom(0);
					chr1_5(space->machine, vrom_bank[5], CHRROM);
//                  ppu2c0x_set_videorom_bank(state->ppu, 5, 1, vrom_bank[5], 64);
//                  nes_vram_sprite[5] = vrom_bank[0] * 64;
//                  vrom_next[1] = 1;
//                  vrom_page_a = 0;
//                  vrom_page_b = 0;
					break;
			}
			break;
		case 0x1026: /* $5126 */
			LOG_MMC(("MMC5 $5126 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[6] = data;
//                  mapper5_sync_vrom(0);
					chr1_6(space->machine, vrom_bank[6], CHRROM);
//                  ppu2c0x_set_videorom_bank(state->ppu, 6, 1, vrom_bank[6], 64);
//                  nes_vram_sprite[6] = vrom_bank[0] * 64;
//                  vrom_next[2] = 2;
//                  vrom_page_a = 0;
//                  vrom_page_b = 0;
					break;
			}
			break;
		case 0x1027: /* $5127 */
			LOG_MMC(("MMC5 $5127 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x00:
					/* 8k switch */
					chr8(space->machine, data, CHRROM);
					break;
				case 0x01:
					/* 4k switch */
					chr4_4 (space->machine, data, CHRROM);
					break;
				case 0x02:
					/* 2k switch */
					chr2_6 (space->machine, data, CHRROM);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[7] = data;
//                  mapper5_sync_vrom(0);
					chr1_7(space->machine, vrom_bank[7], CHRROM);
//                  ppu2c0x_set_videorom_bank(state->ppu, 7, 1, vrom_bank[7], 64);
//                  nes_vram_sprite[7] = vrom_bank[0] * 64;
//                  vrom_next[3] = 3;
//                  vrom_page_a = 0;
//                  vrom_page_b = 0;
					break;
			}
			break;
		case 0x1028: /* $5128 */
			LOG_MMC(("MMC5 $5128 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[8] = vrom_bank[12] = data;
//                  nes_vram[vrom_next[0]] = data * 64;
//                  nes_vram[0 + (vrom_page_a*4)] = data * 64;
//                  nes_vram[0] = data * 64;
					chr1_4(space->machine, data, CHRROM);
//                  ppu2c0x_set_videorom_bank(state->ppu, 4, 1, data, 64);
//                  mapper5_sync_vrom(1);
					if (!vrom_page_b)
					{
						vrom_page_a ^= 0x01;
						vrom_page_b = 1;
					}
					break;
			}
			break;
		case 0x1029: /* $5129 */
			LOG_MMC(("MMC5 $5129 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x02:
					/* 2k switch */
					chr2_0 (space->machine, data, CHRROM);
					chr2_4 (space->machine, data, CHRROM);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[9] = vrom_bank[13] = data;
//                  nes_vram[vrom_next[1]] = data * 64;
//                  nes_vram[1 + (vrom_page_a*4)] = data * 64;
//                  nes_vram[1] = data * 64;
					chr1_5(space->machine, data, CHRROM);
//                  ppu2c0x_set_videorom_bank(state->ppu, 5, 1, data, 64);
//                  mapper5_sync_vrom(1);
					if (!vrom_page_b)
					{
						vrom_page_a ^= 0x01;
						vrom_page_b = 1;
					}
					break;
			}
			break;
		case 0x102a: /* $512a */
			LOG_MMC(("MMC5 $512a vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					vrom_bank[10] = vrom_bank[14] = data;
//                  nes_vram[vrom_next[2]] = data * 64;
//                  nes_vram[2 + (vrom_page_a*4)] = data * 64;
//                  nes_vram[2] = data * 64;
					chr1_6(space->machine, data, CHRROM);
//                  ppu2c0x_set_videorom_bank(state->ppu, 6, 1, data, 64);
//                  mapper5_sync_vrom(1);
					if (!vrom_page_b)
					{
						vrom_page_a ^= 0x01;
						vrom_page_b = 1;
					}
					break;
			}
			break;
		case 0x102b: /* $512b */
			LOG_MMC(("MMC5 $512b vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x00:
					/* 8k switch */
					/* switches in first half of an 8K bank!) */
					chr4_0 (space->machine, data << 1, CHRROM);
					chr4_4 (space->machine, data << 1, CHRROM);
					break;
				case 0x01:
					/* 4k switch */
					chr4_0 (space->machine, data, CHRROM);
					chr4_4 (space->machine, data, CHRROM);
					break;
				case 0x02:
					/* 2k switch */
					chr2_2 (space->machine, data, CHRROM);
					chr2_6 (space->machine, data, CHRROM);
					break;
				case 0x03:
					/* 1k switch */
					vrom_bank[11] = vrom_bank[15] = data;
//                  nes_vram[vrom_next[3]] = data * 64;
//                  nes_vram[3 + (vrom_page_a*4)] = data * 64;
//                  nes_vram[3] = data * 64;
					chr1_7(space->machine, data, CHRROM);
//                  ppu2c0x_set_videorom_bank(state->ppu, 7, 1, data, 64);
//                  mapper5_sync_vrom(1);
					if (!vrom_page_b)
					{
						vrom_page_a ^= 0x01;
						vrom_page_b = 1;
					}
					break;
			}
			break;

		case 0x1103: /* $5203 */
			IRQ_count = data;
			MMC5_scanline = data;
			LOG_MMC(("MMC5 irq scanline: %d\n", IRQ_count));
			break;
		case 0x1104: /* $5204 */
			IRQ_enable = data & 0x80;
			LOG_MMC(("MMC5 irq enable: %02x\n", data));
			break;
		case 0x1105: /* $5205 */
			mult1 = data;
			break;
		case 0x1106: /* $5206 */
			mult2 = data;
			break;

		default:
			logerror("** MMC5 uncaught write, offset: %04x, data: %02x\n", offset + 0x4100, data);
			break;
	}
}

static WRITE8_HANDLER( mapper5_w )
{
	logerror("MMC5 uncaught high mapper w, %04x: %02x\n", offset, data);
}

/*************************************************************

    Mapper 6

    Known Boards: FFE4 Copier Board
    Games: Hacked versions of games

    In MESS: Unsupported, low priority

*************************************************************/

/*************************************************************

    Mapper 7

    Known Boards: AMROM, ANROM, AN1ROM, AOROM and unlicensed
          AxROM
    Games: Arch Rivals, Battletoads, Cabal, Commando, Solstice

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( mapper7_w )
{
	LOG_MMC(("mapper7_w, offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring((data & 0x10) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
	prg32(space->machine, data);
}

/*************************************************************

    Mapper 8

    Known Boards: FFE3 Copier Board
    Games: Hacked versions of games

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( mapper8_w )
{
	LOG_MMC(("mapper8_w, offset: %04x, data: %02x\n", offset, data));

	/* Switch 8k VROM bank */
	chr8(space->machine, data & 0x07, CHRROM);

	/* Switch 16k PRG bank */
	data = (data >> 3) & (nes.prg_chunks - 1);
	memory_set_bankptr(space->machine, 1, &nes.rom[data * 0x4000 + 0x10000]);
	memory_set_bankptr(space->machine, 2, &nes.rom[data * 0x4000 + 0x12000]);
}


/*************************************************************

    Mapper 9

    Known Boards: MMC2 based Boards, i.e. PNROM & PEEOROM
    Games: Punch Out!!, Mike Tyson's Punch Out!!

    In MESS: Supported

*************************************************************/

static void mapper9_latch (const device_config *device, offs_t offset)
{
	if ((offset & 0x3ff0) == 0x0fd0)
	{
		LOG_MMC(("mapper9 vrom latch switch (bank 0 low): %02x\n", MMC2_bank0));
		MMC2_bank0_latch = 0xfd;
		chr4_0(device->machine, MMC2_bank0, CHRROM);
	}
	else if ((offset & 0x3ff0) == 0x0fe0)
	{
		LOG_MMC(("mapper9 vrom latch switch (bank 0 high): %02x\n", MMC2_bank0_hi));
		MMC2_bank0_latch = 0xfe;
		chr4_0(device->machine, MMC2_bank0_hi, CHRROM);
	}
	else if ((offset & 0x3ff0) == 0x1fd0)
	{
		LOG_MMC(("mapper9 vrom latch switch (bank 1 low): %02x\n", MMC2_bank1));
		MMC2_bank1_latch = 0xfd;
		chr4_4(device->machine, MMC2_bank1, CHRROM);
	}
	else if ((offset & 0x3ff0) == 0x1fe0)
	{
		LOG_MMC(("mapper9 vrom latch switch (bank 0 high): %02x\n", MMC2_bank1_hi));
		MMC2_bank1_latch = 0xfe;
		chr4_4(device->machine, MMC2_bank1_hi, CHRROM);
	}
}

static WRITE8_HANDLER( mapper9_w )
{
	switch (offset & 0x7000)
	{
		case 0x2000:
			/* Switch the first 8k prg bank */
			prg8_89(space->machine, data);
			break;
		case 0x3000:
			MMC2_bank0 = data;
			if (MMC2_bank0_latch == 0xfd)
				chr4_0(space->machine, MMC2_bank0, CHRROM);
			LOG_MMC(("MMC2 VROM switch #1 (low): %02x\n", MMC2_bank0));
			break;
		case 0x4000:
			MMC2_bank0_hi = data;
			if (MMC2_bank0_latch == 0xfe)
				chr4_0(space->machine, MMC2_bank0_hi, CHRROM);
			LOG_MMC(("MMC2 VROM switch #1 (high): %02x\n", MMC2_bank0_hi));
			break;
		case 0x5000:
			MMC2_bank1 = data;
			if (MMC2_bank1_latch == 0xfd)
				chr4_4(space->machine, MMC2_bank1, CHRROM);
			LOG_MMC(("MMC2 VROM switch #2 (low): %02x\n", data));
			break;
		case 0x6000:
			MMC2_bank1_hi = data;
			if (MMC2_bank1_latch == 0xfe)
				chr4_4(space->machine, MMC2_bank1_hi, CHRROM);
			LOG_MMC(("MMC2 VROM switch #2 (high): %02x\n", data));
			break;
		case 0x7000:
			set_nt_mirroring((data & 0x01) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;
		default:
			LOG_MMC(("MMC2 uncaught w: %04x:%02x\n", offset, data));
			break;
	}
}


/*************************************************************

    Mapper 10

    Known Boards: MMC4 based Boards, i.e. FJROM & FKROM
    Games: Famicom Wars, Fire Emblem, Fire Emblem Gaiden

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( mapper10_w )
{
	LOG_MMC(("mapper10_w, offset: %04x, data: %02x\n", offset, data));
	switch (offset & 0x7000)
	{
		case 0x2000:
			/* Switch the first 16k prg bank */
			prg16_89ab(space->machine, data);
			break;
		default:
			mapper9_w(space, offset, data);
			break;
	}
}

/*************************************************************

    Mapper 11

    Known Boards: Discrete Logic Board, NINA07
    Games: many Color Dreams and Wisdom Tree titles

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( mapper11_w )
{
	LOG_MMC(("mapper11_w, offset: %04x, data: %02x\n", offset, data));

	/* Switch 8k VROM bank */
	chr8(space->machine, data >> 4, CHRROM);

	/* Switch 32k prg bank */
	prg32(space->machine, data & 0x0f);
}

/*************************************************************

    Mapper 12

    Known Boards: Bootleg Board by Rex Soft
    Games: Dragon Ball Z 5, Dragon Ball Z Super

    MMC3 clone

    In MESS: Unsupported

*************************************************************/

/*************************************************************

    Mapper 13

    Known Boards: CPROM
    Games: Videomation

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( mapper13_w )
{
	LOG_MMC(("mapper13_w, offset: %04x, data: %02x\n", offset, data));
	chr4_4(space->machine, data, CHRRAM);
}

/*************************************************************

    Mapper 14

    Known Boards: Bootleg Board by Rex Soft (SL1632)
    Games: [no games in nes.hsi]

    MMC3 clone

    In MESS: Unsupported

*************************************************************/

/*************************************************************

    Mapper 15

    Known Boards: Bootleg Board by Waixing (and probably others)
    Games: Bao Xiao Tien Guo, Bio Hazard, Pokemon Gold, Subor (R)

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( mapper15_w )
{
	UINT8 map15_flip = (data & 0x80) >> 7;
	UINT8 map15_helper = (data & 0x7f) << 1;

	LOG_MMC(("mapper15_w, offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring((data & 0x40) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	switch (offset & 0x0fff)
	{
		case 0x000:
			prg8_89(space->machine, (map15_helper + 0) ^ map15_flip);
			prg8_ab(space->machine, (map15_helper + 1) ^ map15_flip);
			prg8_cd(space->machine, (map15_helper + 2) ^ map15_flip);
			prg8_ef(space->machine, (map15_helper + 3) ^ map15_flip);
			break;
		case 0x001:
			map15_helper |= map15_flip;
			prg8_89(space->machine, map15_helper);
			prg8_ab(space->machine, map15_helper + 1);
			prg8_cd(space->machine, map15_helper + 1);
			prg8_ef(space->machine, map15_helper);
			break;
		case 0x002:
			map15_helper |= map15_flip;
			prg8_89(space->machine, map15_helper);
			prg8_ab(space->machine, map15_helper);
			prg8_cd(space->machine, map15_helper);
			prg8_ef(space->machine, map15_helper);
	        	break;
		case 0x003:
			map15_helper |= map15_flip;
			prg8_89(space->machine, map15_helper);
			prg8_ab(space->machine, map15_helper + 1);
			prg8_cd(space->machine, map15_helper);
			prg8_ef(space->machine, map15_helper);
			break;
	}
}

/*************************************************************

    Mapper 16

    Known Boards: Bandai LZ93D50 24C02
    Games: Crayon Shin-Chan - Ora to Poi Poi, Dragon Ball Z Gaiden,
          Dragon Ball Z II & III, Rokudenashi Blues, SD Gundam
          Gaiden - KGM2

    In MESS: Supported

*************************************************************/

static void bandai_irq( const device_config *device, int scanline, int vblank, int blanked )
{
	/* 114 is the number of cycles per scanline */
	/* TODO: change to reflect the actual number of cycles spent */

	if (IRQ_enable)
	{
		if (IRQ_count <= 114)
		{
			cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
		}
		IRQ_count -= 114;
	}
}

static WRITE8_HANDLER( mapper16_m_w )
{
	LOG_MMC(("mapper16_m_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x000f)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			chr1_x(space->machine, offset & 0x07, data, CHRROM);
			break;
		case 8:
			/* Switch 16k bank at $8000 */
			prg16_89ab(space->machine, data);
			break;
		case 9:
			switch (data & 0x03)
			{
				case 0:
					set_nt_mirroring(PPU_MIRROR_HORZ);
					break;
				case 1:
					set_nt_mirroring(PPU_MIRROR_VERT);
					break;
				case 2:
					set_nt_mirroring(PPU_MIRROR_LOW);
					break;
				case 3:
					set_nt_mirroring(PPU_MIRROR_HIGH);
					break;
			}
			break;
		case 0x0a:
			IRQ_enable = data & 0x01;
			break;
		case 0x0b:
			IRQ_count &= 0xff00;
			IRQ_count |= data;
			break;
		case 0x0c:
			IRQ_count &= 0x00ff;
			IRQ_count |= (data << 8);
			break;
		default:
			logerror("** uncaught mapper 16 write, offset: %04x, data: %02x\n", offset, data);
			break;
	}
}

static WRITE8_HANDLER( mapper16_w )
{
	LOG_MMC(("mapper16_w, offset: %04x, data: %02x\n", offset, data));

	mapper16_m_w(space, offset, data);
}

/*************************************************************

    Mapper 17

    Known Boards: FFE8 Copier Board
    Games: Hacked versions of games

    In MESS: Supported. it also uses mapper4_irq.

*************************************************************/

static WRITE8_HANDLER( mapper17_l_w )
{
	LOG_MMC(("mapper17_l_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
		/* $42fe - mirroring */
		case 0x1fe:
			if (data & 0x10)
				set_nt_mirroring(PPU_MIRROR_LOW);
			else
				set_nt_mirroring(PPU_MIRROR_HIGH);
			break;
		/* $42ff - mirroring */
		case 0x1ff:
			if (data & 0x10)
				set_nt_mirroring(PPU_MIRROR_HORZ);
			else
				set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		/* $4501 - $4503 */
//      case 0x401:
//      case 0x402:
//      case 0x403:
			/* IRQ control */
//          break;
		/* $4504 - $4507 : 8k PRG-Rom switch */
		case 0x404:
		case 0x405:
		case 0x406:
		case 0x407:
			data &= ((nes.prg_chunks << 1) - 1);
//          LOG_MMC(("Mapper 17 bank switch, bank: %02x, data: %02x\n", offset & 0x03, data));
			memory_set_bankptr(space->machine, (offset & 0x03) + 1, &nes.rom[0x2000 * (data) + 0x10000]);
			break;
		/* $4510 - $4517 : 1k CHR-Rom switch */
		case 0x410:
		case 0x411:
		case 0x412:
		case 0x413:
		case 0x414:
		case 0x415:
		case 0x416:
		case 0x417:
			chr1_x(space->machine, offset&7, data&((nes.chr_chunks << 3) - 1), CHRROM);
//          ppu2c0x_set_videorom_bank(state->ppu, offset & 0x07, 1, data, 64);
			break;
		default:
			logerror("** uncaught mapper 17 write, offset: %04x, data: %02x\n", offset, data);
			break;
	}
}

/*************************************************************

    Mapper 18

    Known Boards: Jaleco JF23, JF24, JF25, JF27, JF29, JF30, JF31,
          JF32, JF33, JF34, JF35, JF36, JF37, JF38, JF40, JF41, SS88006
    Games: Lord of King, Magic John, Moe Pro '90, Ninja Jajamaru,
          Pizza Pop, Plasma Ball

    In MESS: Supported

*************************************************************/

static void jaleco_irq( const device_config *device, int scanline, int vblank, int blanked )
{
	if (scanline <= PPU_BOTTOM_VISIBLE_SCANLINE)
	{
		/* Increment & check the IRQ scanline counter */
		if (IRQ_enable)
		{
			IRQ_count -= 0x100;

			LOG_MMC(("scanline: %d, irq count: %04x\n", scanline, IRQ_count));
			if (IRQ_mode & 0x08)
			{
				if ((IRQ_count & 0x0f) == 0x00)
					/* rollover every 0x10 */
					cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
			}
			else if (IRQ_mode & 0x04)
			{
				if ((IRQ_count & 0x0ff) == 0x00)
					/* rollover every 0x100 */
					cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
			}
			else if (IRQ_mode & 0x02)
			{
				if ((IRQ_count & 0x0fff) == 0x000)
					/* rollover every 0x1000 */
					cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
			}
			else if (IRQ_count == 0)
				/* rollover at 0x10000 */
				cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
		}
	}
	else
	{
		IRQ_count = IRQ_count_latch;
	}
}


static WRITE8_HANDLER( mapper18_w )
{
//  static int irq;
	static int bank_8000 = 0;
	static int bank_a000 = 0;
	static int bank_c000 = 0;

	LOG_MMC(("mapper18_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7003)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 - low 4 bits */
			bank_8000 = (bank_8000 & 0xf0) | (data & 0x0f);
			bank_8000 &= prg_mask;
			memory_set_bankptr(space->machine, 1, &nes.rom[0x2000 * (bank_8000) + 0x10000]);
			break;
		case 0x0001:
			/* Switch 8k bank at $8000 - high 4 bits */
			bank_8000 = (bank_8000 & 0x0f) | (data << 4);
			bank_8000 &= prg_mask;
			memory_set_bankptr(space->machine, 1, &nes.rom[0x2000 * (bank_8000) + 0x10000]);
			break;
		case 0x0002:
			/* Switch 8k bank at $a000 - low 4 bits */
			bank_a000 = (bank_a000 & 0xf0) | (data & 0x0f);
			bank_a000 &= prg_mask;
			memory_set_bankptr(space->machine, 2, &nes.rom[0x2000 * (bank_a000) + 0x10000]);
			break;
		case 0x0003:
			/* Switch 8k bank at $a000 - high 4 bits */
			bank_a000 = (bank_a000 & 0x0f) | (data << 4);
			bank_a000 &= prg_mask;
			memory_set_bankptr(space->machine, 2, &nes.rom[0x2000 * (bank_a000) + 0x10000]);
			break;
		case 0x1000:
			/* Switch 8k bank at $c000 - low 4 bits */
			bank_c000 = (bank_c000 & 0xf0) | (data & 0x0f);
			bank_c000 &= prg_mask;
			memory_set_bankptr(space->machine, 3, &nes.rom[0x2000 * (bank_c000) + 0x10000]);
			break;
		case 0x1001:
			/* Switch 8k bank at $c000 - high 4 bits */
			bank_c000 = (bank_c000 & 0x0f) | (data << 4);
			bank_c000 &= prg_mask;
			memory_set_bankptr(space->machine, 3, &nes.rom[0x2000 * (bank_c000) + 0x10000]);
			break;

		/* $9002, 3 (1002, 3) uncaught = Jaleco Baseball writes 0 */
		/* believe it's related to battery-backed ram enable/disable */

		case 0x2000:
			/* Switch 1k vrom at $0000 - low 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0xf0) | (data & 0x0f);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			chr1_0(space->machine, vrom_bank[0], CHRROM);
			break;
		case 0x2001:
			/* Switch 1k vrom at $0000 - high 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0x0f) | (data << 4);
			vrom_bank[0] &= ((nes.chr_chunks << 3) - 1);
			chr1_0(space->machine, vrom_bank[0], CHRROM);
			break;
		case 0x2002:
			/* Switch 1k vrom at $0400 - low 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0xf0) | (data & 0x0f);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			chr1_1(space->machine, vrom_bank[1], CHRROM);
			break;
		case 0x2003:
			/* Switch 1k vrom at $0400 - high 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0x0f) | (data << 4);
			vrom_bank[1] &= ((nes.chr_chunks << 3) - 1);
			chr1_1(space->machine, vrom_bank[1], CHRROM);
			break;
		case 0x3000:
			/* Switch 1k vrom at $0800 - low 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0xf0) | (data & 0x0f);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			chr1_2(space->machine, vrom_bank[2], CHRROM);
			break;
		case 0x3001:
			/* Switch 1k vrom at $0800 - high 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0x0f) | (data << 4);
			vrom_bank[2] &= ((nes.chr_chunks << 3) - 1);
			chr1_2(space->machine, vrom_bank[2], CHRROM);
			break;
		case 0x3002:
			/* Switch 1k vrom at $0c00 - low 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0xf0) | (data & 0x0f);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			chr1_3(space->machine, vrom_bank[3], CHRROM);
			break;
		case 0x3003:
			/* Switch 1k vrom at $0c00 - high 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0x0f) | (data << 4);
			vrom_bank[3] &= ((nes.chr_chunks << 3) - 1);
			chr1_3(space->machine, vrom_bank[3], CHRROM);
			break;
		case 0x4000:
			/* Switch 1k vrom at $1000 - low 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0xf0) | (data & 0x0f);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			chr1_4(space->machine, vrom_bank[4], CHRROM);
			break;
		case 0x4001:
			/* Switch 1k vrom at $1000 - high 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0x0f) | (data << 4);
			vrom_bank[4] &= ((nes.chr_chunks << 3) - 1);
			chr1_4(space->machine, vrom_bank[4], CHRROM);
			break;
		case 0x4002:
			/* Switch 1k vrom at $1400 - low 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0xf0) | (data & 0x0f);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			chr1_5(space->machine, vrom_bank[5], CHRROM);
			break;
		case 0x4003:
			/* Switch 1k vrom at $1400 - high 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0x0f) | (data << 4);
			vrom_bank[5] &= ((nes.chr_chunks << 3) - 1);
			chr1_5(space->machine, vrom_bank[5], CHRROM);
			break;
		case 0x5000:
			/* Switch 1k vrom at $1800 - low 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0xf0) | (data & 0x0f);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			chr1_6(space->machine, vrom_bank[6], CHRROM);
			break;
		case 0x5001:
			/* Switch 1k vrom at $1800 - high 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0x0f) | (data << 4);
			vrom_bank[6] &= ((nes.chr_chunks << 3) - 1);
			chr1_6(space->machine, vrom_bank[6], CHRROM);
			break;
		case 0x5002:
			/* Switch 1k vrom at $1c00 - low 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0xf0) | (data & 0x0f);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			chr1_7(space->machine, vrom_bank[7], CHRROM);
			break;
		case 0x5003:
			/* Switch 1k vrom at $1c00 - high 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0x0f) | (data << 4);
			vrom_bank[7] &= ((nes.chr_chunks << 3) - 1);
			chr1_7(space->machine, vrom_bank[7], CHRROM);
			break;

/* LBO - these are unverified */

		case 0x6000: /* IRQ scanline counter - low byte, low nibble */
			IRQ_count_latch = (IRQ_count_latch & 0xfff0) | (data & 0x0f);
			LOG_MMC(("     Mapper 18 copy/set irq latch (l-l): %02x\n", data));
			break;
		case 0x6001: /* IRQ scanline counter - low byte, high nibble */
			IRQ_count_latch = (IRQ_count_latch & 0xff0f) | ((data & 0x0f) << 4);
			LOG_MMC(("     Mapper 18 copy/set irq latch (l-h): %02x\n", data));
			break;
		case 0x6002:
			IRQ_count_latch = (IRQ_count_latch & 0xf0ff) | ((data & 0x0f) << 9);
			LOG_MMC(("     Mapper 18 copy/set irq latch (h-l): %02x\n", data));
			break;
		case 0x6003:
			IRQ_count_latch = (IRQ_count_latch & 0x0fff) | ((data & 0x0f) << 13);
			LOG_MMC(("     Mapper 18 copy/set irq latch (h-h): %02x\n", data));
			break;

/* LBO - these 2 are likely wrong */

		case 0x7000: /* IRQ Control 0 */
			if (data & 0x01)
			{
//              IRQ_enable = 1;
				IRQ_count = IRQ_count_latch;
			}
			LOG_MMC(("     Mapper 18 IRQ Control 0: %02x\n", data));
			break;
		case 0x7001: /* IRQ Control 1 */
			IRQ_enable = data & 0x01;
			IRQ_mode = data & 0x0e;
			LOG_MMC(("     Mapper 18 IRQ Control 1: %02x\n", data));
			break;

		case 0x7002: /* Misc */
			switch (data & 0x03)
			{
				case 0: set_nt_mirroring(PPU_MIRROR_LOW); break;
				case 1: set_nt_mirroring(PPU_MIRROR_HIGH); break;
				case 2: set_nt_mirroring(PPU_MIRROR_VERT); break;
				case 3: set_nt_mirroring(PPU_MIRROR_HORZ); break;
			}
			break;

/* $f003 uncaught, writes 0(start) & various(ingame) */
//      case 0x7003:
//          IRQ_count = data;
//          break;

		default:
			logerror("Mapper 18 uncaught addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

/*************************************************************

    Mapper 19

    Known Boards: Namcot 163 & 163S
    Games: Battle Fleet, Family Circuit '91, Famista '90, '91,
          '92 & '94, Megami Tensei II, Top Striker, Wagyan Land 2 & 3

    In MESS: Supported

*************************************************************/

static void namcot_irq( const device_config *device, int scanline, int vblank, int blanked )
{
	IRQ_count ++;
	/* Increment & check the IRQ scanline counter */
	if (IRQ_enable && (IRQ_count == 0x7fff))
	{
		cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
	}
}

static WRITE8_HANDLER( mapper19_l_w )
{
	LOG_MMC(("mapper19_l_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x1800)
	{
		case 0x1000:
			/* low byte of IRQ */
			IRQ_count = (IRQ_count & 0x7f00) | data;
			break;
		case 0x1800:
			/* high byte of IRQ, IRQ enable in high bit */
			IRQ_count = (IRQ_count & 0xff) | ((data & 0x7f) << 8);
			IRQ_enable = data & 0x80;
			break;
	}
}

static WRITE8_HANDLER( mapper19_w )
{
	LOG_MMC(("mapper19_w, offset: %04x, data: %02x\n", offset, data));
	switch (offset & 0x7800)
	{
		case 0x0000:
		case 0x0800:
		case 0x1000:
		case 0x1800:
		case 0x2000:
		case 0x2800:
		case 0x3000:
		case 0x3800:
			chr1_x(space->machine, offset / 0x800, data, CHRROM);
			break;
		case 0x4000:
			set_nt_page(0, CIRAM, data & 0x01, 1);
			break;
		case 0x4800:
			set_nt_page(1, CIRAM, data & 0x01, 1);
			break;
		case 0x5000:
			set_nt_page(2, CIRAM, data & 0x01, 1);
			break;
		case 0x5800:
			set_nt_page(3, CIRAM, data & 0x01, 1);
			break;
		case 0x6000:
			prg8_89(space->machine, data);
			break;
		case 0x6800:
			prg8_ab(space->machine, data);
			break;
		case 0x7000:
			prg8_cd(space->machine, data);
			break;
	}
}

/*************************************************************

    Mapper 20

    Known Boards: Reserved for FDS
    Games: any FDS disk

    In MESS: Supported

*************************************************************/

static void fds_irq( const device_config *device, int scanline, int vblank, int blanked )
{
	if (IRQ_enable_latch)
		cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);

	/* Increment & check the IRQ scanline counter */
	if (IRQ_enable)
	{
		if (IRQ_count <= 114)
		{
			cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
			IRQ_enable = 0;
			nes_fds.status0 |= 0x01;
		}
		else
			IRQ_count -= 114;
	}
}

READ8_HANDLER( fds_r )
{
	int ret = 0x00;
	static int last_side = 0;
	static int count = 0;

	LOG_MMC(("fds_r, offset: %04x\n", offset));

	switch (offset)
	{
		case 0x00: /* $4030 - disk status 0 */
			ret = nes_fds.status0;
			/* clear the disk IRQ detect flag */
			nes_fds.status0 &= ~0x01;
			break;
		case 0x01: /* $4031 - data latch */
			/* don't read data if disk is unloaded */
			if (nes_fds.data == NULL)
				ret = 0;
			else if (nes_fds.current_side)
				ret = nes_fds.data[(nes_fds.current_side-1) * 65500 + nes_fds.head_position++];
			else
				ret = 0;
			break;
		case 0x02: /* $4032 - disk status 1 */
			/* return "no disk" status if disk is unloaded */
			if (nes_fds.data == NULL)
				ret = 1;
			else if (last_side != nes_fds.current_side)
			{
				/* If we've switched disks, report "no disk" for a few reads */
				ret = 1;
				count ++;
				if (count == 50)
				{
					last_side = nes_fds.current_side;
					count = 0;
				}
			}
			else
				ret = (nes_fds.current_side == 0); /* 0 if a disk is inserted */
			break;
		case 0x03: /* $4033 */
			ret = 0x80;
			break;
		default:
			ret = 0x00;
			break;
	}

	LOG_FDS(("fds_r, address: %04x, data: %02x\n", offset + 0x4030, ret));

	return ret;
}

WRITE8_HANDLER( fds_w )
{
	LOG_MMC(("fds_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
		case 0x00: /* $4020 */
			IRQ_count_latch &= ~0xff;
			IRQ_count_latch |= data;
			break;
		case 0x01: /* $4021 */
			IRQ_count_latch &= ~0xff00;
			IRQ_count_latch |= (data << 8);
			break;
		case 0x02: /* $4022 */
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data;
			break;
		case 0x03: /* $4023 */
			// d0 = sound io (1 = enable)
			// d1 = disk io (1 = enable)
			break;
		case 0x04: /* $4024 */
			/* write data out to disk */
			break;
		case 0x05: /* $4025 */
			nes_fds.motor_on = data & 0x01;
			if (data & 0x02) nes_fds.head_position = 0;
			nes_fds.read_mode = data & 0x04;
			if (data & 0x08)
				set_nt_mirroring(PPU_MIRROR_HORZ);
			else
				set_nt_mirroring(PPU_MIRROR_VERT);
			if ((!(data & 0x40)) && (nes_fds.write_reg & 0x40))
				nes_fds.head_position -= 2; // ???
			IRQ_enable_latch = data & 0x80;

			nes_fds.write_reg = data;
			break;
	}

	LOG_FDS(("fds_w, address: %04x, data: %02x\n", offset + 0x4020, data));
}

/*************************************************************

    Mapper 21 & 25

    Known Boards: Konami VRC4A & VRC4C (21), VRC4B & VRC4D (25)
    Games: Ganbare Goemon Gaiden 2, Wai Wai World 2 (21), Bio
          Miracle Bokutte Upa, Ganbare Goemon Gaiden, TMNT 1 & 2 Jpn (25)

    In MESS: Supported

*************************************************************/

static void konami_irq( const device_config *device, int scanline, int vblank, int blanked )
{
	/* Increment & check the IRQ scanline counter */
	if (IRQ_enable && (++IRQ_count == 0x100))
	{
		IRQ_count = IRQ_count_latch;
		IRQ_enable = IRQ_enable_latch;
		cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
	}
}

static WRITE8_HANDLER( konami_vrc4_w )
{
	LOG_MMC(("konami_vrc4_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7007)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			prg8_89(space->machine, data);
			break;
		case 0x1000:
			switch (data & 0x03)
			{
				case 0x00: set_nt_mirroring(PPU_MIRROR_VERT); break;
				case 0x01: set_nt_mirroring(PPU_MIRROR_HORZ); break;
				case 0x02: set_nt_mirroring(PPU_MIRROR_LOW); break;
				case 0x03: set_nt_mirroring(PPU_MIRROR_HIGH); break;
			}
			break;

		/* $1001 is uncaught */

		case 0x2000:
			/* Switch 8k bank at $a000 */
			prg8_ab(space->machine, data);
			break;
		case 0x3000:
			/* Switch 1k vrom at $0000 - low 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0xf0) | (data & 0x0f);
			chr1_0(space->machine, vrom_bank[0], CHRROM);
			break;
		case 0x3002:
			/* Switch 1k vrom at $0000 - high 4 bits */
			vrom_bank[0] = (vrom_bank[0] & 0x0f) | (data << 4);
			chr1_0(space->machine, vrom_bank[0], CHRROM);
			break;
		case 0x3001:
		case 0x3004:
			/* Switch 1k vrom at $0400 - low 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0xf0) | (data & 0x0f);
			chr1_1(space->machine, vrom_bank[1], CHRROM);
			break;
		case 0x3003:
		case 0x3006:
			/* Switch 1k vrom at $0400 - high 4 bits */
			vrom_bank[1] = (vrom_bank[1] & 0x0f) | (data << 4);
			chr1_1(space->machine, vrom_bank[1], CHRROM);
			break;
		case 0x4000:
			/* Switch 1k vrom at $0800 - low 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0xf0) | (data & 0x0f);
			chr1_2(space->machine, vrom_bank[2], CHRROM);
			break;
		case 0x4002:
			/* Switch 1k vrom at $0800 - high 4 bits */
			vrom_bank[2] = (vrom_bank[2] & 0x0f) | (data << 4);
			chr1_2(space->machine, vrom_bank[2], CHRROM);
			break;
		case 0x4001:
		case 0x4004:
			/* Switch 1k vrom at $0c00 - low 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0xf0) | (data & 0x0f);
			chr1_3(space->machine, vrom_bank[3], CHRROM);
			break;
		case 0x4003:
		case 0x4006:
			/* Switch 1k vrom at $0c00 - high 4 bits */
			vrom_bank[3] = (vrom_bank[3] & 0x0f) | (data << 4);
			chr1_3(space->machine, vrom_bank[3], CHRROM);
			break;
		case 0x5000:
			/* Switch 1k vrom at $1000 - low 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0xf0) | (data & 0x0f);
			chr1_4(space->machine, vrom_bank[4], CHRROM);
			break;
		case 0x5002:
			/* Switch 1k vrom at $1000 - high 4 bits */
			vrom_bank[4] = (vrom_bank[4] & 0x0f) | (data << 4);
			chr1_4(space->machine, vrom_bank[4], CHRROM);
			break;
		case 0x5001:
		case 0x5004:
			/* Switch 1k vrom at $1400 - low 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0xf0) | (data & 0x0f);
			chr1_5(space->machine, vrom_bank[5], CHRROM);
			break;
		case 0x5003:
		case 0x5006:
			/* Switch 1k vrom at $1400 - high 4 bits */
			vrom_bank[5] = (vrom_bank[5] & 0x0f) | (data << 4);
			chr1_5(space->machine, vrom_bank[5], CHRROM);
			break;
		case 0x6000:
			/* Switch 1k vrom at $1800 - low 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0xf0) | (data & 0x0f);
			chr1_6(space->machine, vrom_bank[6], CHRROM);
			break;
		case 0x6002:
			/* Switch 1k vrom at $1800 - high 4 bits */
			vrom_bank[6] = (vrom_bank[6] & 0x0f) | (data << 4);
			chr1_6(space->machine, vrom_bank[6], CHRROM);
			break;
		case 0x6001:
		case 0x6004:
			/* Switch 1k vrom at $1c00 - low 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0xf0) | (data & 0x0f);
			chr1_7(space->machine, vrom_bank[7], CHRROM);
			break;
		case 0x6003:
		case 0x6006:
			/* Switch 1k vrom at $1c00 - high 4 bits */
			vrom_bank[7] = (vrom_bank[7] & 0x0f) | (data << 4);
			chr1_7(space->machine, vrom_bank[7], CHRROM);
			break;
		case 0x7000:
			/* Set low 4 bits of latch */
			IRQ_count_latch &= ~0x0f;
			IRQ_count_latch |= data & 0x0f;
//          LOG_MMC(("konami_vrc4 irq_latch low: %02x\n", IRQ_count_latch));
			break;
		case 0x7002:
		case 0x7040:
			/* Set high 4 bits of latch */
			IRQ_count_latch &= ~0xf0;
			IRQ_count_latch |= (data << 4) & 0xf0;
//          LOG_MMC(("konami_vrc4 irq_latch high: %02x\n", IRQ_count_latch));
			break;
		case 0x7004:
		case 0x7001:
		case 0x7080:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
//          LOG_MMC(("konami_vrc4 irq_count set: %02x\n", IRQ_count));
//          LOG_MMC(("konami_vrc4 enable: %02x\n", IRQ_enable));
//          LOG_MMC(("konami_vrc4 enable latch: %02x\n", IRQ_enable_latch));
			break;
		case 0x7006:
		case 0x7003:
		case 0x70c0:
			IRQ_enable = IRQ_enable_latch;
//          LOG_MMC(("konami_vrc4 enable copy: %02x\n", IRQ_enable));
			break;
		default:
			LOG_MMC(("konami_vrc4_w uncaught offset: %04x value: %02x\n", offset, data));
			break;
	}
}

/*************************************************************

    Mapper 22

    Known Boards: Konami VRC2A
    Games: Ganbare Pennant Race, Twin Bee 3

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( konami_vrc2a_w )
{
	UINT8 bank;
	LOG_MMC(("konami_vrc2a_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
	case 0x0000:
		prg8_89(space->machine, data);
		break;
	case 0x1000:
		switch (data & 0x03)
		{
			case 0x00: set_nt_mirroring(PPU_MIRROR_VERT); break;
			case 0x01: set_nt_mirroring(PPU_MIRROR_HORZ); break;
			case 0x02: set_nt_mirroring(PPU_MIRROR_LOW); break;
			case 0x03: set_nt_mirroring(PPU_MIRROR_HIGH); break;
		}
		break;
	case 0x2000:
		/* Switch 8k bank at $a000 */
		prg8_ab(space->machine, data);
		break;
	case 0x3000:
	case 0x4000:
	case 0x5000:
	case 0x6000:
		/* switch 1k vrom at $0000-$1c00 */
		bank = ((offset & 0x7000) - 0x3000) / 0x0800 + (offset & 0x0001);
		/* Notice that we store the banks as in other VRC, but then we take vrom_bank>>1 because it's VRC2A!*/
		if (offset & 0x0002)
			vrom_bank[bank] = (vrom_bank[bank] & 0x0f) | (data << 4);
		else
			vrom_bank[bank] = (vrom_bank[bank] & 0xf0) | (data & 0x0f);

		chr1_x(space->machine, bank, vrom_bank[bank] >> 1, CHRROM);
		break;

	default:
		LOG_MMC(("konami_vrc2a_w uncaught offset: %04x value: %02x\n", offset, data));
		break;
	}
}

/*************************************************************

    Mapper 23

    Known Boards: Konami VRC2B & VRC4E
    Games: Getsufuu Maden, Parodius Da!

    In MESS: Supported. It also uses konami_irq.

*************************************************************/

static WRITE8_HANDLER( konami_vrc2b_w )
{
	UINT16 select;
	UINT8 bank;

	LOG_MMC(("konami_vrc2b_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x3000)
	{
		switch (offset & 0x3000)
		{
			case 0:
				/* Switch 8k bank at $8000 */
				prg8_89(space->machine, data);
				break;
			case 0x1000:
				switch (data & 0x03)
				{
					case 0x00: set_nt_mirroring(PPU_MIRROR_VERT); break;
					case 0x01: set_nt_mirroring(PPU_MIRROR_HORZ); break;
					case 0x02: set_nt_mirroring(PPU_MIRROR_LOW); break;
					case 0x03: set_nt_mirroring(PPU_MIRROR_HIGH); break;
				}
				break;

			case 0x2000:
				/* Switch 8k bank at $a000 */
				prg8_ab(space->machine, data);
				break;
			default:
				LOG_MMC(("konami_vrc2b_w offset: %04x value: %02x\n", offset, data));
				break;
		}
		return;
	}

	/* The low 2 select bits vary from cart to cart */
	select = (offset & 0x7000) | (offset & 0x03) | ((offset & 0x0c) >> 2);

	switch (select)
	{
	case 0x3000:
	case 0x3001:
	case 0x3002:
	case 0x3003:
	case 0x4000:
	case 0x4001:
	case 0x4002:
	case 0x4003:
	case 0x5000:
	case 0x5001:
	case 0x5002:
	case 0x5003:
	case 0x6000:
	case 0x6001:
	case 0x6002:
	case 0x6003:
		/* switch 1k vrom at $0000-$1c00 */
		bank = ((select & 0x7000) - 0x3000) / 0x0800 + ((select & 0x0002) >> 1);
		if (select & 0x0001)
			vrom_bank[bank] = (vrom_bank[bank] & 0x0f) | (data << 4);
		else
			vrom_bank[bank] = (vrom_bank[bank] & 0xf0) | (data & 0x0f);

		chr1_x(space->machine, bank, vrom_bank[bank], CHRROM);
		break;
	case 0x7000:
		/* Set low 4 bits of latch */
		IRQ_count_latch &= ~0x0f;
		IRQ_count_latch |= data & 0x0f;
		break;
	case 0x7001:
		/* Set high 4 bits of latch */
		IRQ_count_latch &= ~0xf0;
		IRQ_count_latch |= (data << 4) & 0xf0;
		break;
	case 0x7002:
		IRQ_count = IRQ_count_latch;
		IRQ_enable = data & 0x02;
		IRQ_enable_latch = data & 0x01;
		break;

	default:
		logerror("konami_vrc2b_w uncaught offset: %04x value: %02x\n", offset, data);
		break;
	}
}

/*************************************************************

    Mapper 24

    Known Boards: Konami VRC6
    Games: Akumajou Densetsu

    In MESS: Supported. It also uses konami_irq.

*************************************************************/

static WRITE8_HANDLER( konami_vrc6a_w )
{
	UINT8 bank;
	LOG_MMC(("konami_vrc6a_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7003)
	{
		case 0x0000:
		case 0x0001:
		case 0x0002:
		case 0x0003:
			/* Switch 16k bank at $8000 */
			prg16_89ab(space->machine, data);
			break;

/* $1000-$1002 = sound regs */
/* $2000-$2002 = sound regs */
/* $3000-$3002 = sound regs */

		case 0x3003:
			switch (data & 0x0c)
			{
				case 0x00: set_nt_mirroring(PPU_MIRROR_VERT); break;
				case 0x04: set_nt_mirroring(PPU_MIRROR_HORZ); break;
				case 0x08: set_nt_mirroring(PPU_MIRROR_LOW); break;
				case 0x0c: set_nt_mirroring(PPU_MIRROR_HIGH); break;
			}
			break;
		case 0x4000:
		case 0x4001:
		case 0x4002:
		case 0x4003:
			/* Switch 8k bank at $c000 */
			prg8_cd(space->machine, data);
			break;
		case 0x5000:
		case 0x5001:
		case 0x5002:
		case 0x5003:
		case 0x6000:
		case 0x6001:
		case 0x6002:
		case 0x6003:
			/* switch 1k vrom at $0000-$1c00 */
			bank = ((offset & 0x7000) - 0x5000) / 0x0400 + (offset & 0x0003);
			chr1_x(space->machine, bank, data, CHRROM);
			break;

		case 0x7000:
			IRQ_count_latch = data;
			break;
		case 0x7001:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;
		case 0x7002:
			IRQ_enable = IRQ_enable_latch;
			break;

		default:
			logerror("konami_vrc6_w uncaught addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

/*************************************************************

    Mapper 26

    Known Boards: Konami VRC6 variant
    Games: Esper Dream 2, Mouryou Senki Madara

    In MESS: Supported. It also uses konami_irq.

*************************************************************/

static WRITE8_HANDLER( konami_vrc6b_w )
{
	UINT8 bank;

	LOG_MMC(("konami_vrc6b_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7003)
	{
		case 0: case 1: case 2: case 3:
			/* Switch 16k bank at $8000 */
			prg16_89ab(space->machine, data);
			break;

/* $1000-$1002 = sound regs */
/* $2000-$2002 = sound regs */
/* $3000-$3002 = sound regs */

		case 0x3003:
			switch (data & 0x0c)
			{
				case 0x00: set_nt_mirroring(PPU_MIRROR_VERT); break;
				case 0x04: set_nt_mirroring(PPU_MIRROR_HORZ); break;
				case 0x08: set_nt_mirroring(PPU_MIRROR_LOW); break;
				case 0x0c: set_nt_mirroring(PPU_MIRROR_HIGH); break;
			}
			break;
		case 0x4000: case 0x4001: case 0x4002: case 0x4003:
			/* Switch 8k bank at $c000 */
			prg8_cd(space->machine, data);
			break;
		case 0x5000:
		case 0x5001:
		case 0x5002:
		case 0x5003:
		case 0x6000:
		case 0x6001:
		case 0x6002:
		case 0x6003:
			/* switch 1k vrom at $0000-$1c00 */
			bank = ((offset & 0x7000) - 0x5000) / 0x0400 + ((offset & 0x0001) << 1) + ((offset & 0x0002) >> 1);
			chr1_x(space->machine, bank, data, CHRROM);
			break;
		case 0x7000:
			IRQ_count_latch = data;
			break;
		case 0x7001:
			IRQ_enable = IRQ_enable_latch;
			break;
		case 0x7002:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;

		default:
			logerror("konami_vrc6_w uncaught addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

/*************************************************************

    Mapper 27

    Known Boards: Unknown Bootleg Board
    Games: World Hero

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 28

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 29

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 30

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 31

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 32

    Known Boards: Irem G101A & G101B
    Games: Ai Sensei no Oshiete, Image Fight, Kaiketsu
          Yanchamaru 2, Maikyuu Shima, Paaman, Paaman 2

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper32_w )
{
	LOG_MMC(("mapper32_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
		case 0x0000:
			mmc_cmd1 ? prg8_cd(space->machine, data) : prg8_89(space->machine, data);
			break;
		case 0x1000:
			mmc_cmd1 = data & 0x02;
			set_nt_mirroring((data & 0x01) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;
		case 0x2000:
			/* Switch 8k bank at $A000 */
			prg8_ab(space->machine, data);
			break;
		case 0x3000:
			/* Switch 1k VROM at $1000 */
			chr1_x(space->machine, offset & 0x07, data, CHRROM);
			break;
		default:
			logerror("Uncaught mapper 32 write, addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

/*************************************************************

    Mapper 33

    Known Boards: Taito TC0190FMC
    Games: Akira, Bakushou!! Jinsei Gekijou, Don Doko Don,
          Insector X, Operation Wolf, Power Blazer, Takeshi no
          Sengoku Fuuunji

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper33_w )
{
	LOG_MMC(("mapper33_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7003)
	{
		case 0x0000:
			set_nt_mirroring((data & 0x40) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			prg8_89(space->machine, data);
			break;
		case 0x0001:
			prg8_ab(space->machine, data);
			break;
		case 0x0002:
			chr2_0(space->machine, data, CHRROM);
			break;
		case 0x0003:
			chr2_2(space->machine, data, CHRROM);
			break;
		case 0x2000:
			chr1_4(space->machine, data, CHRROM);
			break;
		case 0x2001:
			chr1_5(space->machine, data, CHRROM);
			break;
		case 0x2002:
			chr1_6(space->machine, data, CHRROM);
			break;
		case 0x2003:
			chr1_7(space->machine, data, CHRROM);
			break;
	}
}

/*************************************************************

    Mapper 34

    Known Boards: BNROM, Unlicensed BxROM, NINA001, NINA002
    Games: Deadly Tower, Impossible Mission II, Titanic 1912,
          Dance Xtreme

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper34_m_w )
{
	LOG_MMC(("mapper34_m_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
		case 0x1ffd:
			/* Switch 32k prg banks */
			prg32(space->machine, data);
			break;
		case 0x1ffe:
			/* Switch 4k VNES_ROM at 0x0000 */
			chr4_0(space->machine, data, CHRROM);
			break;
		case 0x1fff:
			/* Switch 4k VNES_ROM at 0x1000 */
			chr4_4(space->machine, data, CHRROM);
			break;
	}
}

static WRITE8_HANDLER( mapper34_w )
{
	/* This portion of the mapper is nearly identical to Mapper 7, except no one-screen mirroring */
	/* Deadly Towers is really a Mapper 34 game - the demo screens look wrong using mapper 7. */
	LOG_MMC(("mapper34_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, data);
}

/*************************************************************

    Mapper 35

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 36

    Known Boards: Bootleg Board by TXC
    Games: Strike Wolf (also Policeman?? according to Nestopia)

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper36_w )
{
	LOG_MMC(("mapper36_w, offset: %04x, data: %02x\n", offset, data));

	if ((offset >= 0x400) && (offset < 0x7fff))
	{
		prg32(space->machine, data >> 4);
		chr8(space->machine, data & 0x0f, CHRROM);
	}
}

/*************************************************************

    Mapper 37

    Known Boards: Custom ZZ??
    Games: Super Mario Bros. + Tetris + Nintendo World Cup (E)

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 38

    Known Boards: Discrete Logic Board
    Games: Crime Busters

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper38_m_w )
{
	LOG_MMC(("mapper38_m_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, data);
	chr8(space->machine, data >> 4, CHRROM);
}

/*************************************************************

    Mapper 39

    Known Boards: Bootleg Board by Subor
    Games: Study n Game 32 in 1

    In MESS: Partially Supported.

*************************************************************/

static WRITE8_HANDLER( mapper39_w )
{
	LOG_MMC(("mapper39_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, data);
}

/*************************************************************

    Mapper 40

    Known Boards: Unknown Bootleg Board
    Games: Super Mario Bros. 2 Lost Levels Pirate

    In MESS: Supported.

*************************************************************/

static void mapper40_irq( const device_config *device, int scanline, int vblank, int blanked )
{
	/* Decrement & check the IRQ scanline counter */
	if (IRQ_enable)
	{
		if (--IRQ_count == 0)
		{
			cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
		}
	}
}

static WRITE8_HANDLER( mapper40_w )
{
	LOG_MMC(("mapper40_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6000)
	{
		case 0x0000:
			IRQ_enable = 0;
			IRQ_count = 37; /* Hardcoded scanline scroll */
			break;
		case 0x2000:
			IRQ_enable = 1;
			break;
		case 0x6000:
			/* Game runs code between banks, use slow but sure method */
			prg8_cd(space->machine, data);
			break;
	}
}

/*************************************************************

    Mapper 41

    Known Boards: Caltron Board
    Games: 6 in 1 by Caltron

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper41_m_w )
{
	LOG_MMC(("mapper41_m_w, offset: %04x, data: %02x\n", offset, data));

	mmc_cmd1 = offset & 0xff;
	set_nt_mirroring((offset & 0x20) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
	prg32(space->machine, offset & 0x07);
}

static WRITE8_HANDLER( mapper41_w )
{
	LOG_MMC(("mapper41_w, offset: %04x, data: %02x\n", offset, data));

	if (mmc_cmd1 & 0x04)
		chr8(space->machine, ((mmc_cmd1 & 0x18) >> 1) | (data & 0x03), CHRROM);
}

/*************************************************************

    Mapper 42

    Known Boards: Unknown Bootleg Board
    Games: Mario Baby, Ai Senshi Nicol

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper42_w )
{
	LOG_MMC(("mapper42_w, offset: %04x, data: %02x\n", offset, data));

	if (offset >= 0x7000)
	{
		switch(offset & 0x03)
		{
		case 0x00:
			memory_set_bankptr(space->machine, 5, &nes.rom[(data & ((nes.prg_chunks << 1) - 1)) * 0x2000 + 0x10000]);
			break;
		case 0x01:
			set_nt_mirroring((data & 0x08) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;
		case 0x02:
			/* Check if IRQ is being enabled */
			if (!IRQ_enable && (data & 0x02))
			{
				IRQ_enable = 1;
				timer_adjust_oneshot(nes_irq_timer, cputag_clocks_to_attotime(space->machine, "maincpu", 24576), 0);
			}
			if (!(data & 0x02))
			{
				IRQ_enable = 0;
				timer_adjust_oneshot(nes_irq_timer, attotime_never, 0);
			}
			break;
		}
	}
}

/*************************************************************

    Mapper 43

    Known Boards: Unknown Bootleg Board
    Games: Super Mario Bros. 2 Pirate (LF36)

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper43_w )
{
	int bank = (((offset >> 8) & 0x03) * 0x20) + (offset & 0x1f);

	LOG_MMC(("mapper43_w, offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring((offset& 0x2000) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	if (offset & 0x0800)
	{
		if (offset & 0x1000)
		{
			if (bank * 2 >= nes.prg_chunks)
			{
				memory_set_bankptr(space->machine, 3, nes.wram);
				memory_set_bankptr(space->machine, 4, nes.wram);
			}
			else
			{
				LOG_MMC(("mapper43_w, selecting upper 16KB bank of #%02x\n", bank));
				prg16_cdef(space->machine, 2 * bank + 1);
			}
		}
		else
		{
			if (bank * 2 >= nes.prg_chunks)
			{
				memory_set_bankptr(space->machine, 1, nes.wram);
				memory_set_bankptr(space->machine, 2, nes.wram);
			}
			else
			{
				LOG_MMC(("mapper43_w, selecting lower 16KB bank of #%02x\n", bank));
				prg16_89ab(space->machine, 2 * bank);
			}
		}
	}
	else
	{
		if (bank * 2 >= nes.prg_chunks)
		{
			memory_set_bankptr(space->machine, 1, nes.wram);
			memory_set_bankptr(space->machine, 2, nes.wram);
			memory_set_bankptr(space->machine, 3, nes.wram);
			memory_set_bankptr(space->machine, 4, nes.wram);
		}
		else
		{
			LOG_MMC(("mapper43_w, selecting 32KB bank #%02x\n", bank));
			prg32(space->machine, bank);
		}
	}
}

/*************************************************************

    Mapper 44

    Known Boards: Unknown Multigame Bootleg Board
    Games: Kunio 8 in 1, Super Big 7 in 1

    In MESS: Supported. It also uses mapper4_irq.

*************************************************************/

static WRITE8_HANDLER( mapper44_w )
{
//  LOG_MMC(("mapper44_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline));

	//only bits 14,13, and 0 matter for offset!
	switch (offset & 0x6001)
	{
	case 0x2001: /* $a001 - Select 128K ROM/VROM base (0..5) or last 256K ROM/VRAM base (6) */
		{
			UINT8 page = (data & 0x07);
			if (page > 6)
				page = 6;
			MMC3_prg_base = page * 16;
			MMC3_prg_mask = (page > 5) ? 0x1f : 0x0f;
			MMC3_chr_base = page * 128;
			MMC3_chr_mask = (page > 5) ? 0xff : 0x7f;
		}
		mapper4_set_prg(space->machine);
		mapper4_set_chr(space->machine, mmc_chr_source);
		break;

	default:
		mapper4_w(space, offset, data);
		break;
	}
}

/*************************************************************

    Mapper 45

    Known Boards: Unknown Multigame Bootleg Board
    Games: Street Fighter V, various multigame carts

    In MESS: Supported. It also uses mapper4_irq.

*************************************************************/

static WRITE8_HANDLER( mapper45_m_w )
{
	LOG_MMC(("mapper45_m_w, offset: %04x, data: %02x\n", offset, data));

	if (offset == 0)
	{
		mapper45_data[mapper45_cmd] = data;
		mapper45_cmd = (mapper45_cmd + 1) & 0x03;

		if (!mapper45_cmd)
		{
			LOG_MMC(("mapper45_m_w, command completed %02x %02x %02x %02x\n", mapper45_data[3],
				mapper45_data[2], mapper45_data[1], mapper45_data[0]));

			MMC3_prg_base = mapper45_data[1];
			MMC3_prg_mask = 0x3f ^ (mapper45_data[3] & 0x3f);
			MMC3_chr_base = ((mapper45_data[2] & 0xf0) << 4) + mapper45_data[0];

			if (mapper45_data[2] & 0x08)
				MMC3_chr_mask = (1 << ((mapper45_data[2] & 0x07) + 1)) - 1;
			else
				MMC3_chr_mask = 0;

			mapper4_set_prg(space->machine);
			mapper4_set_chr(space->machine, mmc_chr_source);
		}
	}
	if (mapper45_data[3] & 0x40)
		nes.wram[offset] = data;
}

/*************************************************************

    Mapper 46

    Known Boards: Rumblestation Board
    Games: Rumblestation 15 in 1

    In MESS: Partially Supported.

*************************************************************/

static WRITE8_HANDLER( mapper46_m_w )
{
	LOG_MMC(("mapper46_m_w, offset: %04x, data: %02x\n", offset, data));

	mmc_bank_latch1 = (mmc_bank_latch1 & 0x01) | ((data & 0x0f) << 1);
	mmc_bank_latch2 = (mmc_bank_latch2 & 0x07) | ((data & 0xf0) >> 1);
	prg32(space->machine, mmc_bank_latch1);
	chr8(space->machine, mmc_bank_latch2, CHRROM);
}

static WRITE8_HANDLER( mapper46_w )
{
	LOG_MMC(("mapper46_w, offset: %04x, data: %02x\n", offset, data));

	mmc_bank_latch1 = (mmc_bank_latch1 & ~0x01) | (data & 0x01);
	mmc_bank_latch2 = (mmc_bank_latch2 & ~0x07) | ((data & 0x70) >> 4);
	prg32(space->machine, mmc_bank_latch1);
	chr8(space->machine, mmc_bank_latch2, CHRROM);
}

/*************************************************************

    Mapper 47

    Known Boards: Custom QJ??
    Games: Super Spike V'Ball + Nintendo World Cup

    In MESS: Supported. It also uses mapper4_irq.

*************************************************************/

static WRITE8_HANDLER( mapper47_m_w )
{
	LOG_MMC(("mapper47_m_w, offset: %04x, data: %02x\n", offset, data));

	if (offset == 0)
	{
		MMC3_prg_base = (data & 0x01) << 4;
		MMC3_prg_mask = 0x0f;
		MMC3_chr_base = (data & 0x01) << 7;
		MMC3_chr_mask = 0x7f;
		mapper4_set_prg(space->machine);
		mapper4_set_chr(space->machine, mmc_chr_source);
	}
}

/*************************************************************

    Mapper 48

    Known Boards: Taito TC0190FMC PAL16R4
    Games: Bakushou!! Jinsei Gekijou 3, Bubble Bobble 2,
          Captain Saver, Don Doko Don 2, Flintstones, Jetsons

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 49

    Known Boards: Unknown Multigame Bootleg Board
    Games: Super HIK 4 in 1

    In MESS: Supported. It also uses mapper4_irq.

*************************************************************/

static WRITE8_HANDLER( mapper49_m_w )
{
	LOG_MMC(("mapper49_m_w, offset: %04x, data: %02x\n", offset, data));

	if ((offset & 0x1800) == 0x0800 && (offset & 0xff) == data)
	{
		MMC3_prg_base = (data & 0xc0) >> 2;
		MMC3_prg_mask = 0x0f;
		MMC3_chr_base = (data & 0xc0) << 1;
		MMC3_chr_mask = 0x7f;
		mapper4_set_prg(space->machine);
		mapper4_set_chr(space->machine, mmc_chr_source);
	}
}

/*************************************************************

    Mapper 50

    Known Boards: Unknown Bootleg Board
    Games: Super Mario Bros. Pirate Alt. Levels

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 51

    Known Boards: Unknown Multigame Bootleg Board
    Games: 11 in 1 Ball Games

    In MESS: Supported.

*************************************************************/

static void mapper51_set_banks( running_machine *machine )
{
	set_nt_mirroring((MMC1_bank1 == 3) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	if (MMC1_bank1 & 0x01)
	{
		prg32(machine, MMC1_bank2);
	}
	else
	{
		prg16_cdef(machine, (MMC1_bank2 * 2) + 1);
		prg16_89ab(machine, MMC1_bank3 * 2);
	}
}

static WRITE8_HANDLER( mapper51_m_w )
{
	LOG_MMC(("mapper51_m_w, offset: %04x, data: %02x\n", offset, data));

	MMC1_bank1 = ((data >> 1) & 0x01) | ((data >> 3) & 0x02);
	mapper51_set_banks(space->machine);
}

static WRITE8_HANDLER( mapper51_w )
{
	LOG_MMC(("mapper51_w, offset: %04x, data: %02x\n", offset, data));

	if (offset & 0x4000)
	{
		MMC1_bank3 = data;
	}
	else
	{
		MMC1_bank2 = data;
	}
	mapper51_set_banks(space->machine);
}

/*************************************************************

    Mapper 52

    Known Boards: Unknown Multigame Bootleg Board
    Games: Mario 7 in 1

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 53

    Known Boards: Unknown Multigame Bootleg Board
    Games: Supervision 16 in 1

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 54

    Known Boards: Unknown Multigame Bootleg Board
    Games: [no games in nes.hsi]

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 55

    Known Boards: Unknown Bootleg Board
    Games: [no games in nes.hsi]

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 56

    Known Boards: Bootleg Board by Kaiser (KS202)
    Games: Super Mario Bros. 3 Pirate

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 57

    Known Boards: Unknown Multigame Bootleg Board
    Games: 6 in 1, 54 in 1, 106 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper57_w )
{
	LOG_MMC(("mapper57_w, offset: %04x, data: %02x\n", offset, data));

	if (offset & 0x0800)
	{
		mmc_cmd2 = data;
	}
	else
	{
		mmc_cmd1 = data;
	}

	if (mmc_cmd2 & 0x80)
	{
		prg32(space->machine, 2 | (mmc_cmd2 >> 6));
	}
	else
	{
		prg16_89ab(space->machine, (mmc_cmd2 >> 5) & 0x03);
		prg16_cdef(space->machine, (mmc_cmd2 >> 5) & 0x03);
	}

	set_nt_mirroring((mmc_cmd2 & 0x08) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	chr8(space->machine, (mmc_cmd1 & 0x03) | (mmc_cmd2 & 0x07) | ((mmc_cmd2 & 0x10) >> 1), CHRROM);
}

/*************************************************************

    Mapper 58

    Known Boards: Unknown Multigame Bootleg Board
    Games: 68 in 1, 73 in 1, 98 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper58_w )
{
	UINT8 bank = (offset & 0x40) ? 0 : 1;
	LOG_MMC(("mapper58_w, offset: %04x, data: %02x\n", offset, data));

	prg16_89ab(space->machine, offset & ~bank);
	prg16_cdef(space->machine, offset | bank);
	chr8(space->machine, offset >> 3, CHRROM);
	set_nt_mirroring((data & 0x80) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
}

/*************************************************************

    Mapper 59

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 60

    Known Boards: Unknown Multigame Bootleg Board
    Games: 4 in 1, 35 in 1

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 61

    Known Boards: Bootleg Board by RCM
    Games: Tetris Family 9 in 1, 20 in 1

    Simple Mapper: prg/chr/nt are swapped depending on the offset
    of writes in 0x8000-0xffff. offset&0x80 set NT mirroring,
    when (offset&0x30) is 0,3 prg32 is set; when it is 1,2
    two 16k prg banks are set. See below for the values used in
    these banks.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper61_w )
{
	LOG_MMC(("mapper61_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x30)
	{
	case 0x00:
	case 0x30:
		prg32(space->machine, offset & 0x0f);
		break;
	case 0x10:
	case 0x20:
		prg16_89ab(space->machine, ((offset & 0x0f) << 1) | ((offset & 0x20) >> 4));
		prg16_cdef(space->machine, ((offset & 0x0f) << 1) | ((offset & 0x20) >> 4));
		break;
	}
	set_nt_mirroring((offset & 0x80) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
}

/*************************************************************

    Mapper 62

    Known Boards: Unknown Multigame Bootleg Board
    Games: Super 700 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper62_w )
{
	LOG_MMC(("mapper62_w, offset :%04x, data: %02x\n", offset, data));

	chr8(space->machine, ((offset & 0x1f) << 2) | (data & 0x03), CHRROM);

	if (offset & 0x20)
	{
		prg16_89ab(space->machine, (offset & 0x40) | ((offset >> 8) & 0x3f));
		prg16_cdef(space->machine, (offset & 0x40) | ((offset >> 8) & 0x3f));
	}
	else
	{
		prg32(space->machine, ((offset & 0x40) | ((offset >> 8) & 0x3f)) >> 1);
	}

	set_nt_mirroring((offset & 0x80) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
}

/*************************************************************

    Mapper 63

    Known Boards: Unknown Multigame Bootleg Board
    Games: [no games in nes.hsi]

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 64

    Known Boards: Tengen 800032
    Games: Klax, Road Runner, Rolling Thunder, Shinobi, Skulls
          & Croosbones, Xybots

    In MESS: Supported. It also uses mapper4_irq.

*************************************************************/

static void mapper64_set_banks( running_machine *machine )
{
	if (mapper64_cmd & 0x20)
	{
		chr1_0(machine, mapper64_data[0], CHRROM);
		chr1_2(machine, mapper64_data[1], CHRROM);
		chr1_1(machine, mapper64_data[8], CHRROM);
		chr1_3(machine, mapper64_data[9], CHRROM);
	}
	else
	{
		chr1_0(machine, mapper64_data[0] & 0xfe, CHRROM);
		chr1_1(machine, (mapper64_data[0] & 0xfe) | 1, CHRROM);
		chr1_2(machine, mapper64_data[1] & 0xfe, CHRROM);
		chr1_3(machine, (mapper64_data[1] & 0xfe) | 1, CHRROM);
	}

	chr1_4(machine, mapper64_data[2], CHRROM);
	chr1_5(machine, mapper64_data[3], CHRROM);
	chr1_6(machine, mapper64_data[4], CHRROM);
	chr1_7(machine, mapper64_data[5], CHRROM);

	prg8_89(machine, mapper64_data[6]);
	prg8_ab(machine, mapper64_data[7]);
	prg8_cd(machine, mapper64_data[10]);
}

static WRITE8_HANDLER( mapper64_m_w )
{
	LOG_MMC(("mapper64_m_w, offset: %04x, data: %02x\n", offset, data));
}

static WRITE8_HANDLER( mapper64_w )
{
/* TODO: something in the IRQ handling hoses Skull & Crossbones */

//  LOG_MMC(("mapper64_w offset: %04x, data: %02x, scanline: %d\n", offset, data, current_scanline));

	switch (offset & 0x7001)
	{
		case 0x0000:
			mapper64_cmd = data;
			break;
		case 0x0001:
			if ((mapper64_cmd & 0x0f) < 10)
				mapper64_data[mapper64_cmd & 0x0f] = data;
			if ((mapper64_cmd & 0x0f) == 0x0f)
				mapper64_data[10] = data;

			mapper64_set_banks(space->machine);
			break;
		case 0x2000:
			/* Not sure if the one-screen mirroring applies to this mapper */
			if (data & 0x40)
				set_nt_mirroring(PPU_MIRROR_HIGH);
			else
			{
				if (data & 0x01)
					set_nt_mirroring(PPU_MIRROR_HORZ);
				else
					set_nt_mirroring(PPU_MIRROR_VERT);
			}
			break;
		case 0x4000: /* $c000 - IRQ scanline counter */
			IRQ_count_latch = data;
			if (0)
				IRQ_count = IRQ_count_latch;

			LOG_MMC(("     MMC3 copy/set irq latch: %02x\n", data));
			break;

		case 0x4001: /* $c001 - IRQ scanline latch */
			IRQ_count = IRQ_count_latch;
			IRQ_mode = data & 0x01;
			LOG_MMC(("     MMC3 set latch: %02x\n", data));
			break;

		case 0x6000: /* $e000 - Disable IRQs */
			IRQ_enable = 0;
			if (0)
				IRQ_count = IRQ_count_latch;

			LOG_MMC(("     MMC3 disable irqs: %02x\n", data));
			break;

		case 0x6001: /* $e001 - Enable IRQs */
			IRQ_enable = 1;
			if (0)
				IRQ_count = IRQ_count_latch;

			LOG_MMC(("     MMC3 enable irqs: %02x\n", data));
			break;

		default:
			LOG_MMC(("Mapper 64 write. addr: %04x value: %02x\n", offset + 0x8000, data));
			break;
	}
}

/*************************************************************

    Mapper 65

    Known Boards: Irem H3001
    Games: Daiku no Gen San 2 - Akage no Dan no Gyakushuu,
          Kaiketsu Yanchamaru 3, Spartan X 2

    In MESS: Supported.

*************************************************************/

static void irem_irq( const device_config *device, int scanline, int vblank, int blanked )
{
	/* Increment & check the IRQ scanline counter */
	if (IRQ_enable)
	{
		if (--IRQ_count == 0)
			cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
	}
}

static WRITE8_HANDLER( mapper65_w )
{
	LOG_MMC(("mapper65_w, offset %04x, data: %02x\n", offset, data));

	switch (offset & 0x7007)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			data &= prg_mask;
			memory_set_bankptr(space->machine, 1, &nes.rom[0x2000 * (data) + 0x10000]);
			LOG_MMC(("     Mapper 65 switch ($8000) value: %02x\n", data));
			break;
		case 0x1001:
			if (data & 0x80)
				set_nt_mirroring(PPU_MIRROR_HORZ);
			else
				set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 0x1005:
			IRQ_count = data << 1;
			break;
		case 0x1006:
			IRQ_enable = IRQ_count;
			break;

		case 0x2000:
			/* Switch 8k bank at $a000 */
			data &= prg_mask;
			memory_set_bankptr(space->machine, 2, &nes.rom[0x2000 * (data) + 0x10000]);
			LOG_MMC(("     Mapper 65 switch ($a000) value: %02x\n", data));
			break;

		case 0x3000:
		case 0x3001:
		case 0x3002:
		case 0x3003:
		case 0x3004:
		case 0x3005:
		case 0x3006:
		case 0x3007:
			/* Switch 1k VROM at $0000-$1c00 */
			chr1_x(space->machine, offset &7, data, CHRROM);
			//ppu2c0x_set_videorom_bank(state->ppu, offset & 7, 1, data, 64);
			break;

		case 0x4000:
			/* Switch 8k bank at $c000 */
			data &= prg_mask;
			memory_set_bankptr(space->machine, 3, &nes.rom[0x2000 * (data) + 0x10000]);
			LOG_MMC(("     Mapper 65 switch ($c000) value: %02x\n", data));
			break;

		default:
			LOG_MMC(("Mapper 65 addr: %04x value: %02x\n", offset + 0x8000, data));
			break;
	}
}

/*************************************************************

    Mapper 66

    Known Boards: GNROM, GXROM, MHROM
    Games: Mobile Suit Z Gundam, Paris-Dakar Rally Special,
          Takahashi Meijin no Bugutte Honey

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper66_w )
{
	LOG_MMC(("mapper66_w, offset %04x, data: %02x\n", offset, data));

	prg32(space->machine, (data & 0x30) >> 4);
	chr8(space->machine, data & 0x03, CHRROM);
}

/*************************************************************

    Mapper 67

    Known Boards: Sunsoft 3
    Games: Fantasy Zone 2, Mito Koumon

    In MESS: Supported. It also uses mapper4_irq.

*************************************************************/

static WRITE8_HANDLER( mapper67_w )
{
	LOG_MMC(("mapper67_w, offset %04x, data: %02x\n", offset, data));
	switch (offset & 0x7801)
	{
//      case 0x0000: /* IRQ acknowledge */
//          IRQ_enable = 0;
//          break;
		case 0x0800:
			chr2_0(space->machine, data, CHRROM);
			break;
		case 0x1800:
			chr2_2(space->machine, data, CHRROM);
			break;
		case 0x2800:
			chr2_4(space->machine, data, CHRROM);
			break;
		case 0x3800:
			chr2_6(space->machine, data, CHRROM);
			break;
		case 0x4800:
//          nes_vram[5] = data * 64;
//          break;
		case 0x4801:
			/* IRQ count? */
			IRQ_count = IRQ_count_latch;
			IRQ_count_latch = data;
			break;
		case 0x5800:
//          chr4_0(space->machine, data);
//          break;
		case 0x5801:
			IRQ_enable = data;
			break;
		case 0x6800:
		case 0x6801:
			switch (data & 3)
			{
				case 0x00: set_nt_mirroring(PPU_MIRROR_VERT); break;
				case 0x01: set_nt_mirroring(PPU_MIRROR_HORZ); break;
				case 0x02: set_nt_mirroring(PPU_MIRROR_LOW); break;
				case 0x03: set_nt_mirroring(PPU_MIRROR_HIGH); break;
			}
			break;
		case 0x7800:
		case 0x7801:
			prg16_89ab(space->machine, data);
			break;
		default:
			LOG_MMC(("mapper67_w uncaught offset: %04x, data: %02x\n", offset, data));
			break;

	}
}

/*************************************************************

    Mapper 68

    Known Boards: Sunsoft 4, Sunsoft with DCS, NTBROM, Tengen 800042
    Games: After Burner, After Burner II, Maharaja

    In MESS: Supported.

*************************************************************/

static void mapper68_mirror( running_machine *machine, int m68_mirror, int m0, int m1 )
{
	/* The 0x20000 (128k) offset is a magic number */
	#define M68_OFFSET 0x20000

	switch (m68_mirror)
	{
		case 0x00:
			set_nt_mirroring(PPU_MIRROR_HORZ);
			break;
		case 0x01:
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 0x02:
			set_nt_mirroring(PPU_MIRROR_LOW);
			break;
		case 0x03:
			set_nt_mirroring(PPU_MIRROR_HIGH);
			break;
		case 0x10:
			set_nt_page(0, ROM, m0 | 0x80, 0);	//(m0 << 10) + M68_OFFSET);
			set_nt_page(1, ROM, m1 | 0x80, 0);	//(m1 << 10) + M68_OFFSET);
			set_nt_page(2, ROM, m0 | 0x80, 0);	//(m0 << 10) + M68_OFFSET);
			set_nt_page(3, ROM, m1 | 0x80, 0);	//(m1 << 10) + M68_OFFSET);
			break;
		case 0x11:
			set_nt_page(0, ROM, m0 | 0x80, 0);	//(m0 << 10) + M68_OFFSET);
			set_nt_page(1, ROM, m0 | 0x80, 0);	//(m0 << 10) + M68_OFFSET);
			set_nt_page(2, ROM, m1 | 0x80, 0);	//(m1 << 10) + M68_OFFSET);
			set_nt_page(3, ROM, m1 | 0x80, 0);	//(m1 << 10) + M68_OFFSET);
			break;
		case 0x12:
			set_nt_page(0, ROM, m0 | 0x80, 0);	//(m0 << 10) + M68_OFFSET);
			set_nt_page(1, ROM, m0 | 0x80, 0);	//(m0 << 10) + M68_OFFSET);
			set_nt_page(2, ROM, m0 | 0x80, 0);	//(m0 << 10) + M68_OFFSET);
			set_nt_page(3, ROM, m0 | 0x80, 0);	//(m0 << 10) + M68_OFFSET);
			break;
		case 0x13:
			set_nt_page(0, ROM, m1 | 0x80, 0);	//(m1 << 10) + M68_OFFSET);
			set_nt_page(1, ROM, m1 | 0x80, 0);	//(m1 << 10) + M68_OFFSET);
			set_nt_page(2, ROM, m1 | 0x80, 0);	//(m1 << 10) + M68_OFFSET);
			set_nt_page(3, ROM, m1 | 0x80, 0);	//(m1 << 10) + M68_OFFSET);
			break;
	}
}

static WRITE8_HANDLER( mapper68_w )
{
	static int m68_mirror;
	static int m0, m1;

	LOG_MMC(("mapper68_w, offset %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
		case 0x0000:
			/* Switch 2k VROM at $0000 */
			chr2_0(space->machine, data, CHRROM);
			break;
		case 0x1000:
			/* Switch 2k VROM at $0800 */
			chr2_2(space->machine, data, CHRROM);
			break;
		case 0x2000:
			/* Switch 2k VROM at $1000 */
			chr2_4(space->machine, data, CHRROM);
			break;
		case 0x3000:
			/* Switch 2k VROM at $1800 */
			chr2_6(space->machine, data, CHRROM);
			break;
		case 0x4000:
			m0 = data & 0x7f;
			mapper68_mirror(space->machine, m68_mirror, m0, m1);
			break;
		case 0x5000:
			m1 = data & 0x7f;
			mapper68_mirror(space->machine, m68_mirror, m0, m1);
			break;
		case 0x6000:
			m68_mirror = data & 0x13;
			mapper68_mirror(space->machine, m68_mirror, m0, m1);
			break;
		case 0x7000:
			prg16_89ab(space->machine, data);
			break;
		default:
			LOG_MMC(("mapper68_w uncaught offset: %04x, data: %02x\n", offset, data));
			break;
	}
}

/*************************************************************

    Mapper 69

    Known Boards: JLROM, JSROM, Sunsoft 5B, Sunsoft FME7 and
         Custom
    Games: Barcode World, Batman - Return of the Joker, Gimmick!,
          Splatter House - Wanpaku Graffiti

    In MESS: Supported.

*************************************************************/

/* should this be used for Mapper 67 as well?!? */
static void sunsoft_irq( const device_config *device, int scanline, int vblank, int blanked )
{
	/* 114 is the number of cycles per scanline */
	/* TODO: change to reflect the actual number of cycles spent */

	if (IRQ_enable)
	{
		if (IRQ_count <= 114)
		{
			cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
		}
		IRQ_count -= 114;
	}
}

static WRITE8_HANDLER( mapper69_w )
{
	static int cmd = 0;
	LOG_MMC(("mapper69_w, offset %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
		case 0x0000:
			cmd = data & 0x0f;
			break;

		case 0x2000:
			switch (cmd)
			{
				case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
					chr1_x(space->machine, cmd, data, CHRROM);
					//ppu2c0x_set_videorom_bank(state->ppu, cmd, 1, data, 64);
					break;

				/* TODO: deal with bankswitching/write-protecting the mid-mapper area */
				case 8:
#if 0
					if (!(data & 0x40))
					{
						memory_set_bankptr(space->machine, 5, &nes.rom[(data & 0x3f) * 0x2000 + 0x10000]);
					}
					else
#endif
						LOG_MMC(("mapper69_w, cmd 8, data: %02x\n", data));
					break;

				case 9:
					prg8_89(space->machine, data);
					break;
				case 10:
					prg8_ab(space->machine, data);
					break;
				case 11:
					prg8_cd(space->machine, data);
					break;
				case 12:
					switch (data & 0x03)
					{
						case 0x00: set_nt_mirroring(PPU_MIRROR_VERT); break;
						case 0x01: set_nt_mirroring(PPU_MIRROR_HORZ); break;
						case 0x02: set_nt_mirroring(PPU_MIRROR_LOW); break;
						case 0x03: set_nt_mirroring(PPU_MIRROR_HIGH); break;
					}
					break;
				case 13:
					IRQ_enable = data;
					break;
				case 14:
					IRQ_count = (IRQ_count & 0xff00) | data;
					break;
				case 15:
					IRQ_count = (IRQ_count & 0x00ff) | (data << 8);
					break;
			}
			break;

		default:
			logerror("mapper69_w uncaught %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

/*************************************************************

    Mapper 70

    Known Boards: Discrete Logic Board
    Games: Space Shadow, Family Trainer Manhattan Police,
          Kamen Rider Club

    Same board as mapper 152, but no NT mirroring

    In MESS: Supported.

*************************************************************/


static WRITE8_HANDLER( mapper70_w )
{
	LOG_MMC(("mapper70_w, offset: %04x, data: %02x\n", offset, data));

	// we lack bus emulation
	prg16_89ab(space->machine, (data >> 4) & 0x07);
	chr8(space->machine, data, CHRROM);
}

/*************************************************************

    Mapper 71

    Known Boards: Camerica Boards (BF9093, BF9097, BF909X, ALGNV11)
    Games: Linus Spacehead's Cosmic Crusade, Micro Machines,
          Mig-29, Stunt Kids

    We currently do not emulate NT mirroring for BF9097 board
    (missing in BF9093). As a result Fire Hawk is broken.

    In MESS: Partially Supported.

*************************************************************/

static WRITE8_HANDLER( mapper71_m_w )
{
	LOG_MMC(("mapper71_m_w offset: %04x, data: %02x\n", offset, data));

	prg16_89ab(space->machine, data);
}

static WRITE8_HANDLER( mapper71_w )
{
	LOG_MMC(("mapper71_w offset: %04x, data: %02x\n", offset, data));

	if (offset >= 0x4000)
		prg16_89ab(space->machine, data);
}

/*************************************************************

    Mapper 72

    Known Boards: Jaleco JF17, JF26 and JF28
    Games: Moero!! Juudou Warriors, Moero!! Pro Tennis, Pinball
          Quest Jpn

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper72_w )
{
	LOG_MMC(("mapper72_w, offset %04x, data: %02x\n", offset, data));

	if (data & 0x80)
		prg16_89ab(space->machine, data & 0x0f);
	if (data & 0x40)
		chr8(space->machine, data & 0x0f, CHRROM);

}

/*************************************************************

    Mapper 73

    Known Boards: Konami VRC3
    Games: Salamander

    In MESS: Supported. It also uses konami_irq.

*************************************************************/

static WRITE8_HANDLER( mapper73_w )
{
	switch (offset & 0x7000)
	{
		case 0x0000:
		case 0x1000:
			/* dunno which address controls these */
			IRQ_count_latch = data;
			IRQ_enable_latch = data;
			break;
		case 0x2000:
			IRQ_enable = data;
			break;
		case 0x3000:
			IRQ_count &= ~0x0f;
			IRQ_count |= data & 0x0f;
			break;
		case 0x4000:
			IRQ_count &= ~0xf0;
			IRQ_count |= (data & 0x0f) << 4;
			break;
		case 0x7000:
			prg16_89ab(space->machine, data);
			break;
		default:
			logerror("mapper73_w uncaught, offset %04x, data: %02x\n", offset, data);
			break;
	}
}


/*************************************************************

    Mapper 74

    Known Boards: Type A by Waixing
    Games: Columbus - Ougon no Yoake (C), Ji Jia Zhan Shi,
          Jia A Fung Yun, Wei Luo Chuan Qi

    In MESS: Partially Supported, but we ignore accesses to 0x5000 / 0x6000 atm
    (is this the reason for missing graphics?)

*************************************************************/

/* MIRROR_LOW and MIRROR_HIGH are swapped! */
static void waixing_set_mirror( UINT8 nt )
{
	switch (nt)
	{
	case 0:
	case 1:
		set_nt_mirroring(nt ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 2:
		set_nt_mirroring(PPU_MIRROR_LOW);
		break;
	case 3:
		set_nt_mirroring(PPU_MIRROR_HIGH);
		break;
	default:
		LOG_MMC(("Mapper set NT to invalid value %02x", nt));
		break;
	}
}

static WRITE8_HANDLER( mapper74_w )
{
	LOG_MMC(("mapper74_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x2000:
		waixing_set_mirror(data);	//maybe data & 0x03?
		break;
	case 0x2001:
		break;
	default:
		mapper4_w(space, offset, data);
		break;
	}
}

/*************************************************************

    Mapper 75

    Known Boards: Komani VRC1 and Jaleco JF20, JF22
    Games: Exciting Boxing, Ganbare Goemon!, Tetsuwan Atom

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper75_w )
{
	LOG_MMC(("mapper75_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
		case 0x0000:
			prg8_89(space->machine, data);
			break;
		case 0x1000:
			set_nt_mirroring((data & 0x01) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			mmc_bank_latch1 = (mmc_bank_latch1 & 0x0f) | ((data & 0x02) << 3);
			mmc_bank_latch2 = (mmc_bank_latch2 & 0x0f) | ((data & 0x04) << 2);
			chr4_0(space->machine, mmc_bank_latch1, CHRROM);
			chr4_4(space->machine, mmc_bank_latch2, CHRROM);
			break;
		case 0x2000:
			prg8_ab(space->machine, data);
			break;
		case 0x4000:
			prg8_cd(space->machine, data);
			break;
		case 0x6000:
			mmc_bank_latch1 = (mmc_bank_latch1 & 0x10) | (data & 0x0f);
			chr4_0(space->machine, mmc_bank_latch1, CHRROM);
			break;
		case 0x7000:
			mmc_bank_latch2 = (mmc_bank_latch2 & 0x10) | (data & 0x0f);
			chr4_4(space->machine, mmc_bank_latch2, CHRROM);
			break;
	}
}

/*************************************************************

    Mapper 76

    Known Boards: Namcot 3446
    Games: Digital Devil Monogatari - Megami Tensei

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper76_w )
{
	LOG_MMC(("mapper76_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7001)
	{
	case 0:
		mmc_cmd1 = data;
		break;
	case 1:
		{
			switch (mmc_cmd1 & 0x07)
			{
			case 2: chr2_0(space->machine, data, CHRROM); break;
			case 3: chr2_2(space->machine, data, CHRROM); break;
			case 4: chr2_4(space->machine, data, CHRROM); break;
			case 5: chr2_6(space->machine, data, CHRROM); break;
			case 6:
				(mmc_cmd1 & 0x40) ? prg8_cd(space->machine, data) : prg8_89(space->machine, data);
				break;
			case 7: prg8_ab(space->machine, data); break;
			default: logerror("mapper76 unsupported command: %02x, data: %02x\n", mmc_cmd1, data); break;
			}
		}
	case 0x2000:
		set_nt_mirroring((data & 1) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	default:
		logerror("mapper76 unmapped write, offset: %04x, data: %02x\n", offset, data);
		break;
	}
}

/*************************************************************

    Mapper 77

    Known Boards: Irem LROG017
    Games: Napoleon Senki

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper77_w )
{
	LOG_MMC(("mapper77_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, data & 0x0f);
	chr2_0(space->machine, (data >> 4), CHRROM);
}

/*************************************************************

    Mapper 78

    Known Boards: Jaleco JF16 and Irem
    Games: Holy Diver, Portopia Renzoku Satsujin Jiken,
          Uchuusen - Cosmo Carrier

    In MESS: Partially Supported.

*************************************************************/

static WRITE8_HANDLER( mapper78_w )
{
	LOG_MMC(("mapper78_w, offset: %04x, data: %02x\n", offset, data));
	/* Switch 8k VROM bank */
	chr8(space->machine, (data & 0xf0) >> 4, CHRROM);

	/* Switch 16k ROM bank */
	prg16_89ab(space->machine, data & 0x0f);
}

/*************************************************************

    Mapper 79

    Known Boards: NINA03, NINA06 by AVE
    Games: Krazy Kreatures, Poke Block, Puzzle, Pyramid,
          Solitaire, Ultimate League Soccer

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper79_l_w )
{
	LOG_MMC(("mapper79_l_w, offset: %04x, data: %02x\n", offset, data));

	if (!(offset & 0x0100))
	{
		prg32(space->machine, data >> 3);
		chr8(space->machine, data, CHRROM);
	}
}

/*************************************************************

    Mapper 80

    Known Boards: Taito X1-005 Ver. A
    Games: Bakushou!! Jinsei Gekijou 2, Kyonshiizu 2, Minelvaton
          Saga, Taito Grand Prix

    Registers are at 0x7ef0-0x7eff. first six ones choose chr
    (2x2k + 4x1k) banks, the seventh sets NT mirroring, the
    remaining ones chose prg banks.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper80_m_w )
{
	LOG_MMC(("mapper80_m_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
		case 0x1ef0:
			/* Switch 2k VROM at $0000 */
			chr2_0(space->machine, (data & 0x7f) >> 1, CHRROM);
			break;
		case 0x1ef1:
			/* Switch 2k VROM at $0000 */
			chr2_2(space->machine, (data & 0x7f) >> 1, CHRROM);
			break;
		case 0x1ef2:
			/* Switch 1k VROM at $1000 */
			chr1_4(space->machine, data, CHRROM);
			break;
		case 0x1ef3:
			/* Switch 1k VROM at $1400 */
			chr1_5(space->machine, data, CHRROM);
			break;
		case 0x1ef4:
			/* Switch 1k VROM at $1800 */
			chr1_6(space->machine, data, CHRROM);
			break;
		case 0x1ef5:
			/* Switch 1k VROM at $1c00 */
			chr1_7(space->machine, data, CHRROM);
			break;
		case 0x1ef6:
			set_nt_mirroring((data & 0x01) ? PPU_MIRROR_VERT : PPU_MIRROR_HORZ);
			break;
		case 0x1efa:
		case 0x1efb:
			/* Switch 8k ROM at $8000 */
			prg8_89(space->machine, data);
			break;
		case 0x1efc:
		case 0x1efd:
			/* Switch 8k ROM at $a000 */
			prg8_ab(space->machine, data);
			break;
		case 0x1efe:
		case 0x1eff:
			/* Switch 8k ROM at $c000 */
			prg8_cd(space->machine, data);
			break;
		default:
			logerror("mapper80_m_w uncaught addr: %04x, value: %02x\n", offset + 0x6000, data);
			break;
	}
}

/*************************************************************

    Mapper 81

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 82

    Known Boards: Taito X1017
    Games: Kyuukyoku Harikiri Koushien, Kyuukyoku Harikiri
          Stadium, SD Keiji - Blader

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper82_m_w )
{
	static int vrom_switch;

	/* This mapper has problems */

	LOG_MMC(("mapper82_m_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
		case 0x1ef0:
			/* Switch 2k VROM at $0000 or $1000 */
			if (vrom_switch)
				chr2_4 (space->machine, data, CHRROM);
			else
				chr2_0 (space->machine, data, CHRROM);
			break;
		case 0x1ef1:
			/* Switch 2k VROM at $0800 or $1800 */
			if (vrom_switch)
				chr2_6 (space->machine, data, CHRROM);
			else
				chr2_2 (space->machine, data, CHRROM);
			break;
		case 0x1ef2:
			/* Switch 1k VROM at $1000 */
			chr1_x(space->machine, 4 ^ vrom_switch, data, CHRROM);
			//ppu2c0x_set_videorom_bank(state->ppu, 4 ^ vrom_switch, 1, data, 64);
			break;
		case 0x1ef3:
			/* Switch 1k VROM at $1400 */
			chr1_x(space->machine, 5 ^ vrom_switch, data, CHRROM);
//          ppu2c0x_set_videorom_bank(state->ppu, 5 ^ vrom_switch, 1, data, 64);
			break;
		case 0x1ef4:
			/* Switch 1k VROM at $1800 */
			chr1_x(space->machine, 6 ^ vrom_switch, data, CHRROM);
//          ppu2c0x_set_videorom_bank(state->ppu, 6 ^ vrom_switch, 1, data, 64);
			break;
		case 0x1ef5:
			/* Switch 1k VROM at $1c00 */
			chr1_x(space->machine, 7 ^ vrom_switch, data, CHRROM);
//          ppu2c0x_set_videorom_bank(state->ppu, 7 ^ vrom_switch, 1, data, 64);
			break;
		case 0x1ef6:
			(data&0x1)?set_nt_mirroring(PPU_MIRROR_VERT):set_nt_mirroring(PPU_MIRROR_HORZ);
			//doc says 1= swapped. Causes in-game issues, but mostly fixes title screen
			vrom_switch = ((data & 0x02) << 1);
			break;

		case 0x1efa:
			/* Switch 8k ROM at $8000 */
			prg8_89(space->machine, data >> 2);
			break;
		case 0x1efb:
			/* Switch 8k ROM at $a000 */
			prg8_ab(space->machine, data >> 2);
			break;
		case 0x1efc:
			/* Switch 8k ROM at $c000 */
			prg8_cd(space->machine, data >> 2);
			break;
		default:
			logerror("mapper82_m_w uncaught addr: %04x, value: %02x\n", offset + 0x6000, data);
			break;
	}
}

/*************************************************************

    Mapper 83

    Known Boards: Unknown Multigame Bootleg Board
    Games: Dragon Ball Party, Fatal Fury 2, Street Blaster II
          Pro, World Heroes 2

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper83_l_w )
{
	mapper83_low_data[offset & 0x03] = data;
}

static READ8_HANDLER( mapper83_l_r )
{
	return mapper83_low_data[offset & 0x03];
}

static void mapper83_set_prg( running_machine *machine )
{
	prg16_89ab(machine, mapper83_data[8] & 0x3f);
	prg16_cdef(machine, (mapper83_data[8] & 0x30) | 0x0f);
}

static void mapper83_set_chr( running_machine *machine )
{
	chr1_0(machine, mapper83_data[0] | ((mapper83_data[8] & 0x30) << 4), CHRROM);
	chr1_1(machine, mapper83_data[1] | ((mapper83_data[8] & 0x30) << 4), CHRROM);
	chr1_2(machine, mapper83_data[2] | ((mapper83_data[8] & 0x30) << 4), CHRROM);
	chr1_3(machine, mapper83_data[3] | ((mapper83_data[8] & 0x30) << 4), CHRROM);
	chr1_4(machine, mapper83_data[4] | ((mapper83_data[8] & 0x30) << 4), CHRROM);
	chr1_5(machine, mapper83_data[5] | ((mapper83_data[8] & 0x30) << 4), CHRROM);
	chr1_6(machine, mapper83_data[6] | ((mapper83_data[8] & 0x30) << 4), CHRROM);
	chr1_7(machine, mapper83_data[7] | ((mapper83_data[8] & 0x30) << 4), CHRROM);
}

static WRITE8_HANDLER( mapper83_w )
{
	LOG_MMC(("mapper83_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
	case 0x0000:
	case 0x3000:
	case 0x30ff:
	case 0x31ff:
		mapper83_data[8] = data;
		mapper83_set_prg(space->machine);
		mapper83_set_chr(space->machine);
		break;
	case 0x0100:
		switch (data & 0x03)
		{
		case 0:
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 1:
			set_nt_mirroring(PPU_MIRROR_HORZ);
			break;
		case 2:
			set_nt_mirroring(PPU_MIRROR_LOW);
			break;
		case 3:
			set_nt_mirroring(PPU_MIRROR_HIGH);
			break;
		}
		break;
	case 0x0200:
		IRQ_count = (IRQ_count & 0xff00) | data;
		break;
	case 0x0201:
		IRQ_enable = 1;
		IRQ_count = (data << 8) | (IRQ_count & 0xff);
		break;
	case 0x0300:
		prg8_89(space->machine, data);
		break;
	case 0x0301:
		prg8_ab(space->machine, data);
		break;
	case 0x0302:
		prg8_cd(space->machine, data);
		break;
	case 0x0310:
	case 0x0311:
	case 0x0312:
	case 0x0313:
	case 0x0314:
	case 0x0315:
	case 0x0316:
	case 0x0317:
		mapper83_data[offset - 0x0310] = data;
		mapper83_set_chr(space->machine);
		break;
	case 0x0318:
		mapper83_data[9] = data;
		mapper83_set_prg(space->machine);
		break;
	}
}

/*************************************************************

    Mapper 84

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 85

    Known Boards: Konami VRC7
    Games: Lagrange Point, Tiny Toon Adventures 2

    In MESS: Supported. It also uses konami_irq.

*************************************************************/

static WRITE8_HANDLER( konami_vrc7_w )
{
	LOG_MMC(("konami_vrc7_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7018)
	{
		case 0x0000:
			/* Switch 8k bank at $8000 */
			prg8_89(space->machine, data);
			break;
		case 0x0008:
		case 0x0010:
		case 0x0018:
			/* Switch 8k bank at $a000 */
			prg8_ab(space->machine, data);
			break;

		case 0x1000:
			/* Switch 8k bank at $c000 */
			prg8_cd(space->machine, data);
			break;

/* TODO: there are sound regs in here */

		case 0x2000:
			/* Switch 1k VROM at $0000 */
			chr1_0(space->machine, data, mmc_chr_source);
			break;
		case 0x2008: case 0x2010: case 0x2018:
			/* Switch 1k VROM at $0400 */
			chr1_1(space->machine, data, mmc_chr_source);
			break;
		case 0x3000:
			/* Switch 1k VROM at $0800 */
			chr1_2(space->machine, data, mmc_chr_source);
			break;
		case 0x3008: case 0x3010: case 0x3018:
			/* Switch 1k VROM at $0c00 */
			chr1_3(space->machine, data, mmc_chr_source);
			break;
		case 0x4000:
			/* Switch 1k VROM at $1000 */
			chr1_4(space->machine, data, mmc_chr_source);
			break;
		case 0x4008: case 0x4010: case 0x4018:
			/* Switch 1k VROM at $1400 */
			chr1_5(space->machine, data, mmc_chr_source);
			break;
		case 0x5000:
			/* Switch 1k VROM at $1800 */
			chr1_6(space->machine, data, mmc_chr_source);
			break;
		case 0x5008: case 0x5010: case 0x5018:
			/* Switch 1k VROM at $1c00 */
			chr1_7(space->machine, data, mmc_chr_source);
			break;

		case 0x6000:
			switch (data & 0x03)
			{
				case 0x00: set_nt_mirroring(PPU_MIRROR_VERT); break;
				case 0x01: set_nt_mirroring(PPU_MIRROR_HORZ); break;
				case 0x02: set_nt_mirroring(PPU_MIRROR_LOW); break;
				case 0x03: set_nt_mirroring(PPU_MIRROR_HIGH); break;
			}
			break;
		case 0x6008: case 0x6010: case 0x6018:
			IRQ_count_latch = data;
			break;
		case 0x7000:
			IRQ_count = IRQ_count_latch;
			IRQ_enable = data & 0x02;
			IRQ_enable_latch = data & 0x01;
			break;
		case 0x7008: case 0x7010: case 0x7018:
			IRQ_enable = IRQ_enable_latch;
			break;

		default:
			logerror("konami_vrc7_w uncaught addr: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

/*************************************************************

    Mapper 86

    Known Boards: Jaleco JF13
    Games: Moero Pro Yakyuu

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper86_m_w )
{
	LOG_MMC(("mapper86_m_w, offset: %04x, data: %02x\n", offset, data));

	if (offset == 0)
	{
		prg32(space->machine, (data >> 4) & 0x03);
		chr8(space->machine, ((data >> 4) & 0x04) | (data & 0x03), CHRROM);
	}
}

/*************************************************************

    Mapper 87

    Known Boards: Jaleco JF05, JF06, JF07, JF08, JF09, JF10 and
                 Discrete Logic Board
    Games: The Goonies, Hyper Olympics, Jajamaru no Daibouken,
          Legend of Kage, Twin Bee

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper87_m_w )
{
	LOG_MMC(("mapper87_m_w, offset: %04x, data: %02x\n", offset, data));

	chr8(space->machine, ((data & 0x02) >> 1) | ((data & 0x01) << 1), CHRROM);
}

/*************************************************************

    Mapper 88

    Known Boards: Namcot 3433 & 3443
    Games: Dragon Spirit - Aratanaru Densetsu, Namcot Mahjong, Quinty

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper88_w )
{
	LOG_MMC(("mapper88_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 1)
	{
	case 0:
		{
			mmc_cmd1 = data & 7;
			break;
		}
	case 1:
		{
			switch (mmc_cmd1)
			{
				case 0: chr2_0(space->machine, data >> 1, CHRROM); break;
				case 1: chr2_2(space->machine, data >> 1, CHRROM); break;
				case 2: chr1_4(space->machine, data | 0x40, CHRROM); break;
				case 3: chr1_5(space->machine, data | 0x40, CHRROM); break;
				case 4: chr1_6(space->machine, data | 0x40, CHRROM); break;
				case 5: chr1_7(space->machine, data | 0x40, CHRROM); break;
				case 6: prg8_89(space->machine,data); break;
				case 7: prg8_ab(space->machine,data); break;
			}
			break;
		}
	}
}

/*************************************************************

    Mapper 89

    Known Boards: Sunsoft 2B
    Games: Tenka no Goikenban - Mito Koumon

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper89_w )
{
	UINT8 map89_helper;
	LOG_MMC(("mapper89_m_w, offset: %04x, data: %02x\n", offset, data));

	map89_helper = (data & 0x07) | ((data & 0x80) ? 0x08 : 0x00);

	prg16_89ab(space->machine, (data >> 4) & 0x07);
	chr8(space->machine, map89_helper, CHRROM);
	set_nt_mirroring((data & 0x08) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
}

/*************************************************************

    Mapper 90

    Known Boards: Type A by J.Y. Company
    Games: Aladdin, Final Fight 3, Super Mario World, Tekken 2

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 91

    Known Boards: Unknown Bootleg Board
    Games: Mortal Kombat II, Street Fighter III, Super Mario
          Kart Rider

    In MESS: Partially supported.

*************************************************************/

static WRITE8_HANDLER( mapper91_m_w )
{
	LOG_MMC(("mapper91_m_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
		case 0x0000:
			switch (offset & 0x03)
			{
				case 0x00: chr2_0(space->machine, data, CHRROM); break;
				case 0x01: chr2_2(space->machine, data, CHRROM); break;
				case 0x02: chr2_4(space->machine, data, CHRROM); break;
				case 0x03: chr2_6(space->machine, data, CHRROM); break;
			}
			break;
		case 0x1000:
			switch (offset & 0x01)
			{
				case 0x00: prg8_89(space->machine, data); break;
				case 0x01: prg8_ab(space->machine, data); break;
			}
			break;
		default:
			logerror("mapper91_m_w uncaught addr: %04x value: %02x\n", offset + 0x6000, data);
			break;
	}
}

/*************************************************************

    Mapper 92

    Known Boards: Jaleco JF19 & JF21
    Games: Moero Pro Soccer, Moero Pro Yakyuu '88

    In MESS: Supported (no samples).

*************************************************************/

static WRITE8_HANDLER( mapper92_w )
{
	LOG_MMC(("mapper92_w, offset %04x, data: %02x\n", offset, data));

	if(data & 0x80)
		prg16_cdef(space->machine, data & 0x0f);
	if(data & 0x40)
		chr8(space->machine, data & 0x0f, CHRROM);
}

/*************************************************************

    Mapper 93

    Known Boards: Sunsoft 2A
    Games: Shanghai, Fantasy Zone

    Very similar to mapper 89, but no NT mirroring for data&8
    and only CHRRAM present

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper93_w )
{
	LOG_MMC(("mapper93_w %04x:%02x\n", offset, data));

	prg16_89ab(space->machine, data >> 4);
}

/*************************************************************

    Mapper 94

    Known Boards: UN1ROM
    Games: Senjou no Okami

    Writes to 0x8000-0xffff set 16k prg bank at 0x8000. The
    bank at 0xc000 is always set to the last one.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper94_w )
{
	LOG_MMC(("mapper94_w %04x:%02x\n", offset, data));

	prg16_89ab(space->machine, data >> 2);
}

/*************************************************************

    Mapper 95

    Known Boards: Namcot 3425
    Games: Dragon Buster

    Quite similar to other Namco boards, but with a peculiar
    NT mirroring.

    In MESS: Supported.

*************************************************************/

UINT8 map95_reg[4];

static WRITE8_HANDLER( mapper95_w )
{
	LOG_MMC(("mapper95_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 1)
	{
	case 0:
		{
			mmc_cmd1 = data;
			break;
		}
	case 1:
		{
			switch (mmc_cmd1 & 0x07)
			{
				case 0:
				case 1:
					chr2_x(space->machine, mmc_cmd1 ? 2 : 0, (data >> 1) & 0x1f, CHRROM);
					break;
				case 2:
				case 3:
				case 4:
				case 5:
					chr1_x(space->machine, 2 + mmc_cmd1, data & 0x1f, CHRROM);
					map95_reg[mmc_cmd1 - 2] = data & 0x20;
					if (!(mmc_cmd1 & 0x80))
					{
						set_nt_page(0, CIRAM, map95_reg[0] ? 1 : 0, 1);
						set_nt_page(1, CIRAM, map95_reg[1] ? 1 : 0, 1);
						set_nt_page(2, CIRAM, map95_reg[2] ? 1 : 0, 1);
						set_nt_page(3, CIRAM, map95_reg[3] ? 1 : 0, 1);
					}
					else
						set_nt_mirroring(PPU_MIRROR_HORZ);
					break;
				case 6:
					prg8_89(space->machine,data & 0x1f);
					break;
				case 7:
					prg8_ab(space->machine,data & 0x1f);
					break;
			}
			break;
		}
	}
}

/*************************************************************

    Mapper 96

    Known Boards: Bandai Oeka Kids Board
    Games: Oeka Kids - Anpanman no Hiragana Daisuki, Oeka
          Kids - Anpanman to Oekaki Shiyou!!

    In MESS: Preliminary Support.

*************************************************************/

static WRITE8_HANDLER( mapper96_w )
{
	LOG_MMC(("mapper96_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, data);
}

/*************************************************************

    Mapper 97

    Known Boards: Irem Board
    Games: Kaiketsu Yanchamaru

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper97_w )
{
	LOG_MMC(("mapper97_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x4000)
	{
		set_nt_mirroring((data & 0x80) ? PPU_MIRROR_VERT : PPU_MIRROR_HORZ);
		prg16_cdef(space->machine, data & 0x0f);
	}
}

/*************************************************************

    Mapper 98

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 99

    Known Boards: VS. System
    Games: most Vs. games

    In MESS: Unsupported, low priority (you should use MAME)

*************************************************************/

/*************************************************************

    Mapper 100

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 101

    Known Boards: ???
    Games: Urusei Yatsura Alt

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper101_m_w )
{
	LOG_MMC(("mapper101_m_w, offset: %04x, data: %02x\n", offset, data));

	chr8(space->machine, data, CHRROM);
}

static WRITE8_HANDLER( mapper101_w )
{
	LOG_MMC(("mapper101_w, offset: %04x, data: %02x\n", offset, data));

	/* ??? */
}

/*************************************************************

    Mapper 102

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 103

    Known Boards: Unknown Bootleg Board (2708)
    Games: Doki Doki Panic (FDS Conversion)

    In MESS: Unsupported. Need reads from 0x6000

*************************************************************/

/*************************************************************

    Mapper 104

    Known Boards: Camerica Golden Five
    Games: Pegasus 5 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper104_w )
{
	LOG_MMC(("mapper104_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x4000)
	{
		if (data & 0x08)
		{
			mmc_bank_latch1 = ((data & 0x07) << 4) | (mmc_bank_latch1 & 0x0f);
			prg16_89ab(space->machine, mmc_bank_latch1);
			prg16_cdef(space->machine, ((data & 0x07) << 4) | 0x0f);
		}

	}
	else
	{
			mmc_bank_latch1 = (mmc_bank_latch1 & 0x70) | (data & 0x0f);
			prg16_89ab(space->machine, mmc_bank_latch1);
	}
}

/*************************************************************

    Mapper 105

    Known Boards: Custom Board
    Games: Nintendo World Championships 1990

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 106

    Known Boards: Unknown Bootleg Board
    Games: Super Mario Bros. 3 Pirate

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 107

    Known Boards: Bootleg Board by Magic Series
    Games: Magic Dragon

    Very simple mapper: writes to 0x8000-0xffff set prg32 and chr8
    banks

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper107_w )
{
	LOG_MMC(("mapper107_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, data >> 1);
	chr8(space->machine, data, CHRROM);
}

/*************************************************************

    Mapper 108

    Known Boards: Bootleg Board by Whirlwind
    Games: Meikyuu Jiin Dababa (FDS Conversion)

    Simple mapper: reads in 0x6000-0x7fff return wram, writes to
          0x8fff set prg8_89 to the bank corresponding to data

    In MESS: Unsupported. Needs to read from 0x6000.

*************************************************************/

/*************************************************************

    Mapper 109

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 110

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 111

    Known Boards: ???
    Games: Ninja Ryukenden (C)

    No info avaliable

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 112

    Known Boards: Bootleg Board NTDEC ASDER
    Games: Cobra Mission, Fighting Hero III, Huang Di, Master
          Shooter

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper112_w )
{
	LOG_MMC(("mapper112_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
	case 0x0000:
		mmc_cmd1 = data & 0x07;
		break;
	case 0x2000:
		switch (mmc_cmd1)
		{
		case 0:
			prg8_89(space->machine, data);
			break;
		case 1:
			prg8_ab(space->machine, data);
			break;
		case 2:
			data &= 0xfe;
			chr1_0(space->machine, data, CHRROM);
			chr1_1(space->machine, data + 1, CHRROM);
			break;
		case 3:
			data &= 0xfe;
			chr1_2(space->machine, data, CHRROM);
			chr1_3(space->machine, data + 1, CHRROM);
			break;
		case 4:
			chr1_4(space->machine, data, CHRROM);
			break;
		case 5:
			chr1_5(space->machine, data, CHRROM);
			break;
		case 6:
			chr1_6(space->machine, data, CHRROM);
			break;
		case 7:
			chr1_7(space->machine, data, CHRROM);
			break;
		}
		break;
	case 0x6000:
		set_nt_mirroring((data & 0x01) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	}
}

/*************************************************************

    Mapper 113

    Known Boards: Bootleg Board by HES (also used by others)
    Games: AV Hanafuda Club, AV Soccer, Papillon, Sidewinder,
          Total Funpack

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper113_l_w )
{
	LOG_MMC(("mapper113_w, offset: %04x, data: %02x\n", offset, data));

	if (!(offset & 0x100))
	{
		prg32(space->machine, data >> 3);
		chr8(space->machine, data & 0x07, CHRROM);
	}
}

/*************************************************************

    Mapper 114

    Known Boards: Unknown Bootleg Board
    Games: The Lion King

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 115 (formerly 248)

    Known Boards: Unknown Bootleg Board by Kasing
    Games: AV Jiu Ji Mahjong, Bao Qing Tian, Thunderbolt 2,
          Shisen Mahjong 2

    MMC3 clone

    In MESS: Partally Supported

*************************************************************/

static void mapper115_set_prg( running_machine *machine )
{
	if (MMC1_bank4 & 0x80)
	{
		prg16_89ab(machine, MMC1_bank4 & 0x0f);
	}
	else
	{
		prg8_89(machine, MMC1_bank2 & 0x1f);
		prg8_ab(machine, MMC1_bank3 & 0x1f);
	}
}

static WRITE8_HANDLER( mapper115_m_w )
{
	LOG_MMC(("mapper115_m_w, offset: %04x, data: %02x\n", offset, data));

	MMC1_bank4 = data;
	mapper115_set_prg(space->machine);
}

static WRITE8_HANDLER( mapper115_w )
{
	LOG_MMC(("mapper115_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7001)
	{
	case 0x0000:
		MMC1_bank1 = data;
		break;
	case 0x0001:
		switch (MMC1_bank1 & 0x07)
		{
		case 0:
			chr2_0(space->machine, data >> 1, CHRROM);
			break;
		case 1:
			chr2_2(space->machine, data >> 1, CHRROM);
			break;
		case 2:
			chr1_4(space->machine, data, CHRROM);
			break;
		case 3:
			chr1_5(space->machine, data, CHRROM);
			break;
		case 4:
			chr1_6(space->machine, data, CHRROM);
			break;
		case 5:
			chr1_7(space->machine, data, CHRROM);
			break;
		case 6:
			MMC1_bank2 = data;
			mapper115_set_prg(space->machine);
			break;
		case 7:
			MMC1_bank3 = data;
			mapper115_set_prg(space->machine);
			break;
		}
		break;
	case 0x2000:
		set_nt_mirroring((data & 0x01) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 0x4000:
		IRQ_count_latch = data;
		break;
	case 0x4001:
		IRQ_count = IRQ_count_latch;
		break;
	case 0x6000:
		IRQ_enable = 0;
		break;
	case 0x6001:
		IRQ_enable = 1;
		break;
	}
}

static void mapper115_irq( const device_config *device, int scanline, int vblank, int blanked )
{
	if ((scanline < PPU_BOTTOM_VISIBLE_SCANLINE) /*|| (scanline == ppu_scanlines_per_frame-1)*/)
	{
		int priorCount = IRQ_count;
		if ((IRQ_count == 0) || IRQ_reload)
		{
			IRQ_count = IRQ_count_latch;
			IRQ_reload = 0;
		}
		else
			IRQ_count --;

		if (IRQ_enable && !blanked && (IRQ_count == 0) && priorCount)
		{
			LOG_MMC(("irq fired, scanline: %d (MAME %d, beam pos: %d)\n", scanline,
					video_screen_get_vpos(device->machine->primary_screen), video_screen_get_hpos(device->machine->primary_screen)));
			cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
		}
	}
}

/*************************************************************

    Mapper 116

    Known Boards: Unknown Bootleg Board (Someri?!?)
    Games: AV Mei Shao Nv Zhan Shi, Chuugoku Taitei

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 117

    Known Boards: Bootleg Board by Future Media
    Games: Crayon Shin-chan (C), San Guo Zhi 4 - Chi Bi Feng Yun

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 118

    Known Boards: TKSROM, TLSROM
    Games: Armadillo, Play Action Football, Pro Hockey, RPG
          Jinsei Game, Y's 3

    In MESS: Supported. It also uses mapper4_irq.

*************************************************************/

static WRITE8_HANDLER( mapper118_w )
{
	static UINT8 last_bank = 0xff;
	LOG_MMC(("mapper118_w, offset: %04x, data: %02x\n", offset, data));

	//uses just bits 14, 13, and 0
	switch (offset & 0x6001)
	{
		case 0x0000: /* $8000 */
			MMC3_cmd = data;

			/* Toggle between switching $8000 and $c000 */
			if (last_bank != (data & 0xc0))
			{
				/* Reset the banks */
				mapper4_set_prg(space->machine);
				mapper4_set_chr(space->machine, mmc_chr_source);
			}
			last_bank = data & 0xc0;
			break;

		case 0x0001: /* $8001 */
		{
			UINT8 cmd = MMC3_cmd & 0x07;
			switch (cmd)
			{
				case 0: case 1:
					data &= 0xfe;
					MMC3_chr[cmd] = data;
					mapper4_set_chr(space->machine, mmc_chr_source);
					break;

				case 2: case 3: case 4: case 5:
					MMC3_chr[cmd] = data;
					mapper4_set_chr(space->machine, mmc_chr_source);
					break;

				case 6:
					MMC3_prg0 = data;
					mapper4_set_prg(space->machine);
					break;

				case 7:
					MMC3_prg1 = data;
					mapper4_set_prg(space->machine);
					break;
			}
			break;
		}
		case 0x2000: /* $a000 */
			LOG_MMC(("     mapper 118 mirroring: %02x\n", data));
			switch (data & 0x02)
			{
				case 0x00: set_nt_mirroring(PPU_MIRROR_LOW); break;
				case 0x01: set_nt_mirroring(PPU_MIRROR_LOW); break;
			}
			break;

		case 0x2001: /* $a001 - extra RAM enable/disable */
			nes.mid_ram_enable = data;
			LOG_MMC(("     MMC3 mid_ram enable: %02x\n", data));
			break;

		case 0x4000: /* $c000 - IRQ scanline counter */
			IRQ_count = data;
			LOG_MMC(("     MMC3 set irq count: %02x\n", data));
			break;

		case 0x4001: /* $c001 - IRQ scanline latch */
			IRQ_count_latch = data;
			LOG_MMC(("     MMC3 set irq count latch: %02x\n", data));
			break;

		case 0x6000: /* $e000 - Disable IRQs */
			IRQ_enable = 0;
			IRQ_count = IRQ_count_latch; /* TODO: verify this */
			LOG_MMC(("     MMC3 disable irqs: %02x\n", data));
			break;

		case 0x6001: /* $e001 - Enable IRQs */
			IRQ_enable = 1;
			LOG_MMC(("     MMC3 enable irqs: %02x\n", data));
			break;

		default:
			logerror("mapper4_w uncaught: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

/*************************************************************

    Mapper 119

    Known Boards: TQROM
    Games: Pin Bot, High Speed

    In MESS: Supported. It also uses mapper4_irq.

*************************************************************/

static void mapper119_set_chr( running_machine *machine )
{
	UINT8 chr_page = (MMC3_cmd & 0x80) >> 5;

	if (MMC3_chr[0] & 0x40)
		chr2_x(machine, chr_page ^ 0, (MMC3_chr[0] >> 1) & 0x03, CHRRAM);
	else
		chr2_x(machine, chr_page ^ 0, ((MMC3_chr[0] & 0x3f) >> 1), CHRROM);

	if (MMC3_chr[1] & 0x40)
		chr2_x(machine, chr_page ^ 2, (MMC3_chr[1] >> 1) & 0x03, CHRRAM);
	else
		chr2_x(machine, chr_page ^ 2, ((MMC3_chr[1] & 0x3f) >> 1), CHRROM);

	if (MMC3_chr[2] & 0x40)
		chr1_x(machine, chr_page ^ 4, MMC3_chr[2] & 0x07, CHRRAM);
	else
		chr1_x(machine, chr_page ^ 4, MMC3_chr[2] & 0x3f, CHRROM);

	if (MMC3_chr[3] & 0x40)
		chr1_x(machine, chr_page ^ 5, MMC3_chr[3] & 0x07, CHRRAM);
	else
		chr1_x(machine, chr_page ^ 5, MMC3_chr[3] & 0x3f, CHRROM);

	if (MMC3_chr[4] & 0x40)
		chr1_x(machine, chr_page ^ 6, MMC3_chr[4] & 0x07, CHRRAM);
	else
		chr1_x(machine, chr_page ^ 6, MMC3_chr[4] & 0x3f, CHRROM);

	if (MMC3_chr[5] & 0x40)
		chr1_x(machine, chr_page ^ 7, MMC3_chr[5] & 0x07, CHRRAM);
	else
		chr1_x(machine, chr_page ^ 7, MMC3_chr[5] & 0x3f, CHRROM);
}

static WRITE8_HANDLER( mapper119_w )
{
	static UINT8 last_bank = 0xff;
	LOG_MMC(("mapper119_w, offset: %04x, data: %02x\n", offset, data));

	//only bits 14,13, and 0 matter for offset!
	switch (offset & 0x6001)
	{
		case 0x0000: /* $8000 */
			MMC3_cmd = data;

			/* Toggle between switching $8000 and $c000 */
			if (last_bank != (data & 0xc0))
			{
				/* Reset the banks */
				mapper4_set_prg(space->machine);
				mapper119_set_chr(space->machine);
				LOG_MMC(("     MMC3 reset banks\n"));
			}
			last_bank = data & 0xc0;
			break;

		case 0x0001: /* $8001 */
		{
			UINT8 cmd = MMC3_cmd & 0x07;
			switch (cmd)
			{
				case 0: case 1:
					MMC3_chr[cmd] = data;
					mapper119_set_chr(space->machine);
					LOG_MMC(("     MMC3 set vram %d: %d\n", cmd, data));
					break;

				case 2: case 3: case 4: case 5:
					MMC3_chr[cmd] = data;
					mapper119_set_chr(space->machine);
					LOG_MMC(("     MMC3 set vram %d: %d\n", cmd, data));
					break;

				case 6:
					MMC3_prg0 = data;
					mapper4_set_prg(space->machine);
					break;

				case 7:
					MMC3_prg1 = data;
					mapper4_set_prg(space->machine);
					break;
			}
			break;
		}
		case 0x2000: /* $a000 */
			if (data & 0x40)
				set_nt_mirroring(PPU_MIRROR_HIGH);
			else
			{
				if (data & 0x01)
					set_nt_mirroring(PPU_MIRROR_HORZ);
				else
					set_nt_mirroring(PPU_MIRROR_VERT);
			}
			break;

		case 0x2001: /* $a001 - extra RAM enable/disable */
			nes.mid_ram_enable = data;
			LOG_MMC(("     MMC3 mid_ram enable: %02x\n", 0));
			break;

		case 0x4000: /* $c000 - IRQ scanline counter */
			IRQ_count_latch = data;
//          LOG_MMC(("     MMC3 set irq count latch: %02x (scanline %d)\n", data, ppu2c0x_get_current_scanline(0)));
			break;

		case 0x4001: /* $c001 - IRQ scanline latch */
			IRQ_reload = 1;
//          LOG_MMC(("     MMC3 set irq reload (scanline %d)\n", ppu2c0x_get_current_scanline(0)));
			break;

		case 0x6000: /* $e000 - Disable IRQs */
			IRQ_enable = 0;
			LOG_MMC(("     MMC3 disable irqs\n"));
			break;

		case 0x6001: /* $e001 - Enable IRQs */
			IRQ_enable = 1;
			LOG_MMC(("     MMC3 enable irqs\n"));
			break;

		default:
			logerror("mapper119_w uncaught: %04x value: %02x\n", offset + 0x8000, data);
			break;
	}
}

/*************************************************************

    Mapper 120

    Known Boards: Unknown Bootleg Board
    Games: Tobidase Daisakusen (FDS Conversion)

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 121

    Known Boards: Bootleg Board
    Games: The Panda Prince

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 122

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 123

    Known Boards: Bootleg Board ???
    Games: [no games in nes.hsi]

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 124

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 125

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 126

    Known Boards: Unknown Multigame Bootleg Board
    Games: [no games in nes.hsi]

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 127

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 128

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 129

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 130

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 131

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 132

    Known Boards: Bootleg Board by TXC (T22211A)
    Games: Creatom

    Info from NEStopia: this mapper features write to four
    registers (0x4100-0x4103). The third one is used to select
    PRG and CHR banks.

    In MESS: Supported.

*************************************************************/

UINT8 txc_reg[4]; // also used by mappers 172 & 173

static WRITE8_HANDLER( mapper132_l_w )
{
	LOG_MMC(("mapper132_l_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 4)
		txc_reg[offset & 0x03] = data;
}

static READ8_HANDLER( mapper132_l_r )
{
	LOG_MMC(("mapper132_l_r, offset: %04x\n", offset));

	if (offset == 0x0000)
		return (txc_reg[1] ^ txc_reg[2]) | 0x40;

	else
		return 0x00;
}

static WRITE8_HANDLER( mapper132_w )
{
	LOG_MMC(("mapper132_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, txc_reg[2] >> 2);
	chr8(space->machine, txc_reg[2], CHRROM);
}

/*************************************************************

    Mapper 133

    Known Boards: Bootleg Board by Sachen (SA72008)
    Games: Jovial Race, Qi Wang

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper133_l_w )
{
	LOG_MMC(("mapper133_l_w %04x:%02x\n", offset, data));

	prg32(space->machine, data >> 2);
	chr8(space->machine, data, CHRROM);
}

/*************************************************************

    Mapper 134

    Known Boards: Unknown Multigame Bootleg Board (4646B)
    Games: 2 in 1 - Family Camp & Aladdin 4

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 135

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 136

    Known Boards: Bootleg Board by Sachen (TCU02)
    Games: Mei Loi Siu Ji

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper136_l_w )
{
	LOG_MMC(("mapper136_l_w, offset: %04x, data: %02x\n", offset, data));

	if ((offset & 0x103) == 0x002)
	{
		mmc_cmd1 = (data & 0x30) | ((data + 3) & 0x0f);
		chr8(space->machine, mmc_cmd1, CHRROM);
	}
}

static READ8_HANDLER( mapper136_l_r )
{
	LOG_MMC(("mapper136_l_r, offset: %04x\n", offset));

	if ((offset & 0x103) == 0x000)
		return mmc_cmd1 | 0x40;
	else
		return 0x00;
}

/*************************************************************

    Mapper 137

    Known Boards: Bootleg Board by Sachen (8259D)
    Games: The Great Wall

    In MESS: Supported.

*************************************************************/

UINT8 sachen_regs[8]; // used by mappers 137, 138, 139, 141

static void sachen_set_mirror( UINT8 nt ) // used by mappers 137, 138, 139, 141
{
	switch (nt)
	{
	case 0:
	case 1:
		set_nt_mirroring(nt ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 2:
		set_nt_page(0, CIRAM, 0, 1);
		set_nt_page(1, CIRAM, 1, 1);
		set_nt_page(2, CIRAM, 1, 1);
		set_nt_page(3, CIRAM, 1, 1);
		break;
	case 3:
		set_nt_mirroring(PPU_MIRROR_LOW);
		break;
	default:
		LOG_MMC(("Mapper set NT to invalid value %02x", nt));
		break;
	}
}

static WRITE8_HANDLER( mapper137_l_w )
{
	LOG_MMC(("mapper137_l_w, offset: %04x, data: %02x\n", offset, data));

	/* write happens only if we are at 0x4100 + k * 0x200,
    but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
	{
		if (!(offset & 0x01))
			mmc_cmd1 = data & 0x07;
		else
		{
			sachen_regs[mmc_cmd1] = data;

			switch (mmc_cmd1)
			{
			case 0x05:
				prg32(space->machine, data);
				break;
			case 0x07:
				sachen_set_mirror((data & 0x01) ? 0 : (data >> 1) & 0x03);
				break;
			default:
				if (mmc_chr_source == CHRROM)
				{
					chr1_0(space->machine, (sachen_regs[0] & 0x07), CHRROM);
					chr1_1(space->machine, (sachen_regs[1] & 0x07) | (sachen_regs[4] << 4 & 0x10), CHRROM);
					chr1_2(space->machine, (sachen_regs[2] & 0x07) | (sachen_regs[4] << 3 & 0x10), CHRROM);
					chr1_3(space->machine, (sachen_regs[3] & 0x07) | (sachen_regs[4] << 2 & 0x10) | (sachen_regs[6] << 3 & 0x08), CHRROM);
				}
				break;
			}
		}
	}
}

static WRITE8_HANDLER( mapper137_m_w )
{
	LOG_MMC(("mapper137_m_w, offset: %04x, data: %02x\n", offset, data));

	mapper137_l_w(space, (offset + 0x100) & 0x01, data);
}

/*************************************************************

    Mapper 138

    Known Boards: Bootleg Board by Sachen (8259B)
    Games: Silver Eagle

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper138_l_w )
{
	UINT8 bank_helper1 = 0, bank_helper2 = 0;
	LOG_MMC(("mapper138_l_w, offset: %04x, data: %02x\n", offset, data));

	/* write happens only if we are at 0x4100 + k * 0x200,
    but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
	{
		if (!(offset & 0x01))
			mmc_cmd1 = data & 0x07;
		else
		{
			sachen_regs[mmc_cmd1] = data;

			switch (mmc_cmd1)
			{
			case 0x05:
				prg32(space->machine, data);
				break;
			case 0x07:
				sachen_set_mirror((data & 0x01) ? 0 : (data >> 1) & 0x03);
				break;
			default:
				if (mmc_chr_source == CHRROM)
				{
					bank_helper1 = sachen_regs[7] & 0x01;
					bank_helper2 = (sachen_regs[4] & 0x07) << 3;
					chr2_0(space->machine, ((sachen_regs[bank_helper1 ? 0 : 0] & 0x07) | bank_helper2), CHRROM);
					chr2_2(space->machine, ((sachen_regs[bank_helper1 ? 0 : 1] & 0x07) | bank_helper2), CHRROM);
					chr2_4(space->machine, ((sachen_regs[bank_helper1 ? 0 : 2] & 0x07) | bank_helper2), CHRROM);
					chr2_6(space->machine, ((sachen_regs[bank_helper1 ? 0 : 3] & 0x07) | bank_helper2), CHRROM);
				}
				break;
			}
		}
	}
}

static WRITE8_HANDLER( mapper138_m_w )
{
	LOG_MMC(("mapper138_m_w, offset: %04x, data: %02x\n", offset, data));

	mapper138_l_w(space, offset & 0x01, data);
}

/*************************************************************

    Mapper 139

    Known Boards: Bootleg Board by Sachen (8259C)
    Games: Final Combat, Hell Fighter, Mahjong Companion,

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper139_l_w )
{
	UINT8 bank_helper1 = 0, bank_helper2 = 0;
	LOG_MMC(("mapper139_l_w, offset: %04x, data: %02x\n", offset, data));

	/* write happens only if we are at 0x4100 + k * 0x200,
    but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
	{
		if (!(offset & 0x01))
			mmc_cmd1 = data & 0x07;
		else
		{
			sachen_regs[mmc_cmd1] = data;

			switch (mmc_cmd1)
			{
			case 0x05:
				prg32(space->machine, data);
				break;
			case 0x07:
				sachen_set_mirror((data & 0x01) ? 0 : (data >> 1) & 0x03);
				break;
			default:
				if (mmc_chr_source == CHRROM)
				{
					bank_helper1 = sachen_regs[7] & 0x01;
					bank_helper2 = (sachen_regs[4] & 0x07) << 3;
					chr2_0(space->machine, ((sachen_regs[bank_helper1 ? 0 : 0] & 0x07) | bank_helper2) << 2, CHRROM);
					chr2_2(space->machine, ((sachen_regs[bank_helper1 ? 0 : 1] & 0x07) | bank_helper2) << 2 | 0x01, CHRROM);
					chr2_4(space->machine, ((sachen_regs[bank_helper1 ? 0 : 2] & 0x07) | bank_helper2) << 2 | 0x02, CHRROM)	;
					chr2_6(space->machine, ((sachen_regs[bank_helper1 ? 0 : 3] & 0x07) | bank_helper2) << 2 | 0x03, CHRROM);
				}
				break;
			}
		}
	}
}

static WRITE8_HANDLER( mapper139_m_w )
{
	LOG_MMC(("mapper139_m_w, offset: %04x, data: %02x\n", offset, data));

	mapper139_l_w(space, (offset + 0x100) & 0x01, data);
}

/*************************************************************

    Mapper 140

    Known Boards: Jaleco JF11, JF12, JF14
    Games: Bio Senshi Dan, Mississippi Satsujin Jiken

    Writes to 0x6000-0x7fff set both prg32 and chr8 banks

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper_140_m_w )
{
       chr8(space->machine, data & 0x0f, CHRROM);
       prg32(space->machine, (data >> 4) & 0x03);
}

/*************************************************************

    Mapper 141

    Known Boards: Bootleg Board by Sachen (8259A)
    Games: Po Po Team, Poker Mahjong, Q Boy, Rockball,
          Super Cartridge (several versions), Super Pang (1 & 2)

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper141_l_w )
{
	UINT8 bank_helper1 = 0, bank_helper2 = 0;
	LOG_MMC(("mapper141_l_w, offset: %04x, data: %02x\n", offset, data));

	/* write happens only if we are at 0x4100 + k * 0x200,
    but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
	{
		if (!(offset & 0x01))
			mmc_cmd1 = data & 0x07;
		else
		{
			sachen_regs[mmc_cmd1] = data;

			switch (mmc_cmd1)
			{
			case 0x05:
				prg32(space->machine, data);
				break;
			case 0x07:
				sachen_set_mirror((data & 0x01) ? 0 : (data >> 1) & 0x03);
				break;
			default:
				if (mmc_chr_source == CHRROM)
				{
					bank_helper1 = sachen_regs[7] & 0x01;
					bank_helper2 = (sachen_regs[4] & 0x07) << 3;
					chr2_0(space->machine, ((sachen_regs[bank_helper1 ? 0 : 0] & 0x07) | bank_helper2) << 1, mmc_chr_source);
					chr2_2(space->machine, ((sachen_regs[bank_helper1 ? 0 : 1] & 0x07) | bank_helper2) << 1 | 0x01, mmc_chr_source);
					chr2_4(space->machine, ((sachen_regs[bank_helper1 ? 0 : 2] & 0x07) | bank_helper2) << 1, mmc_chr_source);
					chr2_6(space->machine, ((sachen_regs[bank_helper1 ? 0 : 3] & 0x07) | bank_helper2) << 1 | 0x01, mmc_chr_source);
				}
				break;
			}
		}
	}
}

static WRITE8_HANDLER( mapper141_m_w )
{
	LOG_MMC(("mapper141_m_w, offset: %04x, data: %02x\n", offset, data));

	mapper141_l_w(space, (offset + 0x100) & 0x01, data);
}

/*************************************************************

    Mapper 142

    Known Boards: Bootleg Board by Kaiser (KS7032)
    Games: Super Mario Bros. 2 (by Kaiser), Pipe 5 (by Sachen?)

    In MESS: Unsupported. Requires reads from 0x6000-0x7fff.

*************************************************************/

/*************************************************************

    Mapper 143

    Known Boards: Bootleg Board by Sachen (TCA01)
    Games: Dancing Blocks, Magic Mathematic

    In MESS: Supported.

*************************************************************/

static READ8_HANDLER( mapper143_l_r )
{
	LOG_MMC(("mapper143_l_r, offset: %04x\n", offset));

	/* the address is read only if we are at 0x4100 + k * 0x200,
    but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
		return (~offset & 0x3f) | 0x40;
	else
		return 0x00;
}

/*************************************************************

    Mapper 144

    Known Boards: Bootleg Board by AGCI (50282)
    Games: Death Race

    Just like mapper 11, except bit 0 taken from ROM and 0x8000
    ignored?

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper144_w )
{
	LOG_MMC(("mapper144_w, offset: %04x, data: %02x\n", offset, data));

	if ((offset & 0x7fff) == 0)
		return;

	data = data |(memory_read_byte(space, offset) & 1);

	/* Switch 8k VROM bank */
	chr8(space->machine, data >> 4, CHRROM);

	/* Switch 32k prg bank */
	prg32(space->machine, data & 0x07);
}

/*************************************************************

    Mapper 145

    Known Boards: Bootleg Board by Sachen (SA72007)
    Games: Sidewinder

    Simple mapper: writes to 0x4100 set chr8 banks.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper145_l_w )
{
	LOG_MMC(("mapper145_l_w %04x:%02x\n", offset, data));

	/* only if we are at 0x4100 + k * 0x200, but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
		chr8(space->machine, data >> 7, CHRROM);
}

/*************************************************************

    Mapper 146

    Known Boards: Bootleg Board by Sachen (SA0161M)
    Games: Galactic Crusader, Lucky 777, Metal Fighter,
          Millionaire, Pyramid II

    This is basically the same as Nina-006 (Mapper 79)

    In MESS: Supported. It shares the handler with mapper 79

*************************************************************/

/*************************************************************

    Mapper 147

    Known Boards: Bootleg Board by Sachen (TCU01)
    Games: Challenge of the Dragon, Chinese Kungfu

    Simple mapper: writes to 0x4102 set pgr32 and chr8 banks.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper147_l_w )
{
	LOG_MMC(("mapper147_l_w, offset: %04x, data: %02x\n", offset, data));

	if ((offset & 0x103) == 0x002)
	{
		prg32(space->machine, ((data >> 6) & 0x02) | ((data >> 2) & 0x01));
		chr8(space->machine, data >> 3, CHRROM);
	}
}

static WRITE8_HANDLER( mapper147_m_w )
{
	LOG_MMC(("mapper147_m_w, offset: %04x, data: %02x\n", offset, data));

	mapper147_l_w(space, offset & 0x01, data);
}

static WRITE8_HANDLER( mapper147_w )
{
	LOG_MMC(("mapper147_w, offset: %04x, data: %02x\n", offset, data));

	mapper147_l_w(space, (offset + 0x100) & 0x01, data);
}

/*************************************************************

    Mapper 148

    Known Boards: Bootleg Board by Sachen (SA0037)
    Games: Mahjong World, Shisen Mahjong

    Very simple mapper: writes to 0x8000-0xffff set pgr32 and chr8
        banks

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper148_w )
{
	LOG_MMC(("mapper148_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, data >> 3 & 0x01);
	chr8(space->machine, data & 0x07, CHRROM);
}

/*************************************************************

    Mapper 149

    Known Boards: Bootleg Board by Sachen (SA0036)
    Games: Taiwan Mahjong 16

    Very simple mapper: writes to 0x8000-0xffff set chr8 banks

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper149_w )
{
	LOG_MMC(("mapper149_w, offset: %04x, data: %02x\n", offset, data));

	chr8(space->machine, data >> 7, CHRROM);
}

/*************************************************************

    Mapper 150

    Known Boards: Bootleg Board by Sachen (74SL374B)
    Games: Chess Academy, Chinese Checkers Jpn, Mahjong Academy,
          Olympic IQ, Poker II, Tasac

    Still investigating this mapper...

    In MESS: Preliminary Support.

*************************************************************/

static WRITE8_HANDLER( mapper150_l_w )
{
	LOG_MMC(("mapper150_l_w, offset: %04x, data: %02x\n", offset, data));

	/* write happens only if we are at 0x4100 + k * 0x200,
    but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
	{
		if (!(offset & 0x01))
			mmc_cmd1 = data & 0x07;
		else
		{
			switch (mmc_cmd1)
			{
			case 0x02:
				prg32(space->machine, data & 0x01);
				chr8(space->machine, (mmc_bank_latch1 & ~0x08) | ((data << 3) & 0x08), CHRROM);
				break;
			case 0x04:
				chr8(space->machine, (mmc_bank_latch1 & ~0x04) | ((data << 2) & 0x04), CHRROM);
				break;
			case 0x05:
				prg32(space->machine, data & 0x07);
				break;
			case 0x06:
				chr8(space->machine, (mmc_bank_latch1 & ~0x03) | (data << 0 & 0x03), CHRROM);
				break;
			case 0x07:
				sachen_set_mirror((data >> 1) & 0x03);
				break;
			default:
				break;
			}
		}
	}
}

static READ8_HANDLER( mapper150_l_r )
{
	LOG_MMC(("mapper150_l_r, offset: %04x", offset));

	/* read  happens only if we are at 0x4100 + k * 0x200,
    but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
		return (~mmc_cmd1 & 0x3f) /* ^ dips*/;	// we would need to check the Dips here
	else
		return 0;
}

static WRITE8_HANDLER( mapper150_m_w )
{
	LOG_MMC(("mapper150_m_w, offset: %04x, data: %02x\n", offset, data));

	mapper150_l_w(space, (offset + 0x100) & 0x01, data);
}

/*************************************************************

    Mapper 151

    Known Boards: Konami VS System
    Games: Goonies VS, VS TKO Boxing

    In MESS: Unsupported, low priority (you should use MAME)

*************************************************************/

/*************************************************************

    Mapper 152

    Known Boards: Discrete Logic Mapper
    Games: Arkanoid 2, Gegege no Kitarou 2, Pocket Zaurus

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper152_w )
{
	LOG_MMC(("mapper152_w, offset: %04x, data: %02x\n", offset, data));

	// we lack bus emulation
	set_nt_mirroring((data & 0x80) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
	prg16_89ab(space->machine, (data >> 4) & 0x07);
	chr8(space->machine, data, CHRROM);
}

/*************************************************************

    Mapper 153

    Known Boards: Bandai Board
    Games: Dragon Ball, Dragon Ball 3, Famicom Jump, Famicom
          Jump II

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 154

    Known Boards: Namcot 34xx
    Games: Devil Man

    Acts like mapper 88, but writes to 0x8000 set NT mirroring

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper154_w )
{
	LOG_MMC(("mapper154_w, offset: %04x, data: %02x\n", offset, data));
	switch (offset & 1)
	{
	case 0:
		{
			set_nt_mirroring((data & 0x40) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
			mmc_cmd1 = data & 7;
			break;
		}
	case 1:
		{
			switch (mmc_cmd1)
			{
				case 0: chr2_0(space->machine, data >> 1, CHRROM); break;
				case 1: chr2_2(space->machine, data >> 1, CHRROM); break;
				case 2: chr1_4(space->machine, data | 0x40, CHRROM); break;
				case 3: chr1_5(space->machine, data | 0x40, CHRROM); break;
				case 4: chr1_6(space->machine, data | 0x40, CHRROM); break;
				case 5: chr1_7(space->machine, data | 0x40, CHRROM); break;
				case 6: prg8_89(space->machine, data); break;
				case 7: prg8_ab(space->machine, data); break;
			}
			break;
		}
	}
}

/*************************************************************

    Mapper 155

    Known Boards: ???
    Games: Tatakae!! Rahmen Man - Sakuretsu Choujin 102 Gei

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 156

    Known Boards: Bootleg Board by Open Corp (DAOU306)
    Games: Metal Force (K)?

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper156_w )
{
	LOG_MMC(("mapper156_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
	case 0x4000:
		chr1_0(space->machine, data, CHRROM);
		break;
	case 0x4001:
		chr1_1(space->machine, data, CHRROM);
		break;
	case 0x4002:
		chr1_2(space->machine, data, CHRROM);
		break;
	case 0x4003:
		chr1_3(space->machine, data, CHRROM);
		break;
	case 0x4008:
		chr1_4(space->machine, data, CHRROM);
		break;
	case 0x4009:
		chr1_5(space->machine, data, CHRROM);
		break;
	case 0x400a:
		chr1_6(space->machine, data, CHRROM);
		break;
	case 0x400b:
		chr1_7(space->machine, data, CHRROM);
		break;
	case 0x4010:
		prg16_89ab(space->machine, data);
		break;
	}
}

/*************************************************************

    Mapper 157

    Known Boards: Bandai Datach Board
    Games: Datach Games

    In MESS: Unsupported. Needs reads from 0x6000-0x7fff for the
      Datach barcode reader

*************************************************************/

/*************************************************************

    Mapper 158

    Known Boards: Tengen 800037
    Games: Alien Syndrome

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 159

    Known Boards: Bandai Board
    Games: Dragon Ball Z, Magical Taruruuto-kun, SD Gundam Gaiden

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 160

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 161

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 162

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 163

    Known Boards: Bootleg Board by Nanjing
    Games: Diablo, Final Fantasy VII, Harvest Moon, He Xin Wei
          Ji, Yu-Gi-Oh, Zelda - Shen Qi De Mao Zi and other

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 164

    Known Boards: Bootleg Board by Waixing
    Games: Darkseed, Digital Dragon, Final Fantasy V, Pocket
          Monster Red

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper164_l_w )
{
	LOG_MMC(("mapper164_l_w, offset: %04x, data: %02x\n", offset, data));
	offset += 0x4100; /* the checks work better on addresses */

	if (0x5000 == (offset & 0x7300))
		prg32(space->machine, data);
}

static WRITE8_HANDLER( mapper164_w )
{
	LOG_MMC(("mapper164_w, offset: %04x, data: %02x\n", offset, data));
	if ((offset & 0x7300) == 0x5000)
		prg32(space->machine, data);
}

/*************************************************************

    Mapper 165

    Known Boards: Bootleg Board by Waixing
    Games: Fire Emblem (C), Fire Emblem Gaiden (C)

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 166

    Known Boards: Bootleg Board by Subor (Type 1)
    Games: Subor v1.0 (R)

    In MESS: Supported.

*************************************************************/

UINT8 subor_reg[4];	// used also by 167 below

static WRITE8_HANDLER( mapper166_w )
{
	UINT8 map166_helper1, map166_helper2;
	LOG_MMC(("mapper166_w, offset: %04x, data: %02x\n", offset, data));

	subor_reg[(offset >> 13) & 0x03] = data;
	map166_helper1 = ((subor_reg[0] ^ subor_reg[1]) << 1) & 0x20;
	map166_helper2 = ((subor_reg[2] ^ subor_reg[3]) << 0) & 0x1f;

	if (subor_reg[1] & 0x08)
	{
		map166_helper1 += map166_helper2 & 0xfe;
		map166_helper2 = map166_helper1;
		map166_helper2 += 1;
	}
	else if (subor_reg[1] & 0x04)
	{
		map166_helper2 += map166_helper1;
		map166_helper1 = 0x1f;
	}
	else
	{
		map166_helper1 += map166_helper2;
		map166_helper2 = 0x07;
	}

	prg16_89ab(space->machine, map166_helper1);
	prg16_cdef(space->machine, map166_helper2);
}

/*************************************************************

    Mapper 167

    Known Boards: Bootleg Board by Subor (Type 0)
    Games: Subor v4.0 (C), English Word Blaster

    Basically the same as 166, except for the initial prg
    bank and a couple of bits in the choice of prg banks

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper167_w )
{
	UINT8 map167_helper1, map167_helper2;
	LOG_MMC(("mapper167_w, offset: %04x, data: %02x\n", offset, data));

	subor_reg[(offset >> 13) & 0x03] = data;
	map167_helper1 = ((subor_reg[0] ^ subor_reg[1]) << 1) & 0x20;
	map167_helper2 = ((subor_reg[2] ^ subor_reg[3]) << 0) & 0x1f;

	if (subor_reg[1] & 0x08)
	{
		map167_helper1 += map167_helper2 & 0xfe;
		map167_helper2 = map167_helper1;
		map167_helper1 += 1;
	}
	else if (subor_reg[1] & 0x04)
	{
		map167_helper2 += map167_helper1;
		map167_helper1 = 0x1f;
	}
	else
	{
		map167_helper1 += map167_helper2;
		map167_helper2 = 0x20;
	}

	prg16_89ab(space->machine, map167_helper1);
	prg16_cdef(space->machine, map167_helper2);
}

/*************************************************************

    Mapper 168

    Known Boards: ???
    Games: Racermate Challenger II

    No info at all are available.

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 169

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 170

    Known Boards: Bootleg Board by Fujiya
    Games: [no games in nes.hsi]

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 171

    Known Boards: Bootleg Board by Kaiser (KS7058)
    Games: Tui Do Woo Ma Jeung

    Writes to 0xf000-0xffff set 4k chr banks. Namely, if
    offset&0x80 is 0 the lower 4k are set, if it is 1 the
    upper 4k are set.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper171_w )
{
	LOG_MMC(("mapper171_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7080)
	{
	case 0x7000:
		chr4_0(space->machine, data, CHRROM);
		break;
	case 0x7080:
		chr4_4(space->machine, data, CHRROM);
		break;
	}
}

/*************************************************************

    Mapper 172

    Known Boards: Bootleg Board by TXC (T22211B)
    Games: 1991 Du Ma Racing

    This mapper is basically the same as 132. Only difference is
    in the way CHR banks are selected (see below)

    In MESS: Supported. It shares low handlers with board T22211A
      (mapper 132)

*************************************************************/

static WRITE8_HANDLER( mapper172_w )
{
	LOG_MMC(("mapper172_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, txc_reg[2] >> 2);
	chr8(space->machine, (((data ^ txc_reg[2]) >> 3) & 0x02) | (((data ^ txc_reg[2]) >> 5) & 0x01), CHRROM);
}

/*************************************************************

    Mapper 173

    Known Boards: Bootleg Board by TXC (T22211C)
    Games: Mahjong Block, Xiao Ma Li

    This mapper is basically the same as 132 too. Only difference is
    in 0x4100 reads which expect also bit 0 to be set

    In MESS: Supported. It shares write handlers with board T22211A
      (mapper 132)

*************************************************************/

static READ8_HANDLER( mapper173_l_r )
{
	LOG_MMC(("mapper173_l_r, offset: %04x\n", offset));

	if (offset == 0x0000)
		return (txc_reg[1] ^ txc_reg[2]) | 0x41;
	else
		return 0x00;
}

/*************************************************************

    Mapper 174

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 175

    Known Boards: Bootleg Board by Kaiser (KS7022)
    Games: 15 in 1

    In MESS: Unsupported. Requires reads from 0xfffc (according to
      NEStopia).

*************************************************************/

/*************************************************************

    Mapper 176

    Known Boards: Unknown Bootleg Board
    Games: Shu Qi Yu - Zhi Li Xiao Zhuan Yuan

    Very simple mapper: writes to 0x5ff1 set prg32 (to data>>1),
    while writes to 0x5ff2 set chr8

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper176_l_w )
{
	LOG_MMC(("mapper176_l_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
	case 0x1ef1:	/* 0x5ff1 */
		prg32(space->machine, data >> 1);
		break;
	case 0x1ef2:	/* 0x5ff2 */
		chr8(space->machine, data, CHRROM);
		break;
	}
}

/*************************************************************

    Mapper 177

    Known Boards: Bootleg Board by Henggedianzi
    Games: Mei Guo Fu Hao, Shang Gu Shen Jian , Wang Zi Fu
          Chou Ji, Xing He Zhan Shi

    Writes to 0x8000-0xffff set prg32. Moreover, data&0x20 sets
    NT mirroring.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper177_w )
{
	LOG_MMC(("mapper177_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, data);
	set_nt_mirroring(((data & 0x20) >> 5) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
}

/*************************************************************

    Mapper 178

    Known Boards: Bootleg Board by Waixing
    Games: Fan Kong Jing Ying, San Guo Zhong Lie Zhuan, Xing
          Ji Zheng Ba

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper178_l_w )
{
	LOG_MMC(("mapper178_l_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
	case 0x700:
		set_nt_mirroring((data & 0x01) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 0x701:
		mmc_cmd1 = (mmc_cmd1 & 0x0c) | ((data >> 1) & 0x03);
		prg32(space->machine, mmc_cmd1);
		break;
	case 0x702:
		mmc_cmd1 = (mmc_cmd1 & 0x03) | ((data << 2) & 0x0c);
		break;
	}
}

/*************************************************************

    Mapper 179

    Known Boards: Bootleg Board by Henggedianzi
    Games: Xing He Zhan Shi

    Writes to 0x5000-0x5fff set prg32 banks, writes to 0x8000-
    0xffff set NT mirroring

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper179_l_w )
{
	LOG_MMC(("mapper179_l_w, offset: %04x, data: %02x\n", offset, data));

	if ((offset - 0x100) & 0x1000)
		prg32(space->machine, data >> 1);
}

static WRITE8_HANDLER( mapper179_w )
{
	LOG_MMC(("mapper179_w, offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring((data & 0x01) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
}

/*************************************************************

    Mapper 180

    Known Boards: Nihon Bussan UNROM
    Games: Crazy Climber Jpn

    Very simple mapper: prg16_89ab is always set to bank 0,
    while prg16_cdef is set by writes to 0x8000-0xffff. The game
    uses a custim controller.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper180_w )
{
	LOG_MMC(("mapper180_w, offset: %04x, data: %02x\n", offset, data));

	prg16_cdef(space->machine, data & 0x07);
}

/*************************************************************

    Mapper 181

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 182

    Known Boards: Bootleg Board by Hosenkan
    Games: Pocahontas, Super Donkey Kong

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper182_w )
{
	LOG_MMC(("mapper182_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7003)
	{
	case 0x0001:
		set_nt_mirroring((data & 0x01) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 0x2000:
		mmc_bank_latch1 = data;
		break;
	case 0x4000:
		switch (mmc_bank_latch1)
		{
		case 0:
			chr2_0(space->machine, data >> 1, CHRROM);
			break;
		case 1:
			chr1_5(space->machine, data, CHRROM);
			break;
		case 2:
			chr2_2(space->machine, data >> 1, CHRROM);
			break;
		case 3:
			chr1_7(space->machine, data, CHRROM);
			break;
		case 4:
			prg8_89(space->machine, data);
			break;
		case 5:
			prg8_ab(space->machine, data);
			break;
		case 6:
			chr1_4(space->machine, data, CHRROM);
			break;
		case 7:
			chr1_6(space->machine, data, CHRROM);
			break;
		}
		break;
	case 0x6003:
		IRQ_count = data;
		IRQ_enable = 1;
		break;
	}
}

static void mapper182_irq( const device_config *device, int scanline, int vblank, int blanked )
{
	if ((scanline < PPU_BOTTOM_VISIBLE_SCANLINE) /*|| (scanline == ppu_scanlines_per_frame-1)*/)
	{
		int priorCount = IRQ_count;
		if ((IRQ_count == 0) || IRQ_reload)
		{
			IRQ_count = IRQ_count_latch;
			IRQ_reload = 0;
		}
		else
			IRQ_count --;

		if (IRQ_enable && !blanked && (IRQ_count == 0) && priorCount)
		{
			LOG_MMC(("irq fired, scanline: %d (MAME %d, beam pos: %d)\n", scanline,
					video_screen_get_vpos(device->machine->primary_screen), video_screen_get_hpos(device->machine->primary_screen)));
			cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, HOLD_LINE);
		}
	}
}

/*************************************************************

    Mapper 183

    Known Boards: Unknown Bootleg Board
    Games: Shui Guan Pipe

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 184

    Known Boards: Sunsoft 1
    Games: Atlantis no Nazo, Kanshakudama Nage Kantarou no
          Toukaidou Gojuusan Tsugi, Wing of Madoola

    Very simple mapper: writes to 0x6000-0x7fff set chr banks
    (lower bits set chr4_0, higher ones set chr4_4)

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper184_m_w )
{
	LOG_MMC(("mapper184_m_w, offset: %04x, data: %02x\n", offset, data));

	chr4_0(space->machine, data & 0x0f, CHRROM);
	chr4_4(space->machine, data >> 4, CHRROM);
}

/*************************************************************

    Mapper 185

    Known Boards: CNROM
    Games: B-Wings, Mighty Bomb Jack, Seicross, Spy vs. Spy

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 186

    Known Boards: Bootleg Board by Fukutake
    Games: Study Box

    In MESS: Unsupported. Needs reads from 0x6000-0x7fff.

*************************************************************/

/*************************************************************

    Mapper 187

    Known Boards: Unknown Bootleg Board
    Games: The King of Fighters 96, Sonic 3D Blast 6, Street
          Fighter Zero 2

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 188

    Known Boards: Bandai Karaoke Board
    Games: Karaoke Studio

    In MESS: Partially Supported (missing mic support)

*************************************************************/

static WRITE8_HANDLER( mapper188_w )
{
	LOG_MMC(("mapper188_w, offset: %04x, data: %02x\n", offset, data));

	prg16_89ab(space->machine, data ^ 0x08);
}

/*************************************************************

    Mapper 189

    Known Boards: Bootleg Board by TXC
    Games: Master Fighter II, Master Fighter 3, Thunder Warrior

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 190

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 191

    Known Boards: Type B by Waixing
    Games: Sugoro Quest (C)

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 192

    Known Boards: Type C by Waixing
    Games: Ying Lie Qun Xia Zhuan, Young Chivalry

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 193

    Known Boards: Bootleg Board by NTDEC
    Games: Fighting Hero

    Very simple mapper: writes to 0x6000-0x7fff swap PRG and
    CHR banks.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper193_m_w )
{
	LOG_MMC(("mapper193_m_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x03)
	{
	case 0:
		chr4_0(space->machine, data >> 2, CHRROM);
		break;
	case 1:
		chr2_4(space->machine, data >> 1, CHRROM);
		break;
	case 2:
		chr2_6(space->machine, data >> 1 , CHRROM);
		break;
	case 3:
		prg8_89(space->machine, data);
		break;
	}
}

/*************************************************************

    Mapper 194

    Known Boards: Type D by Waixing
    Games: Super Robot Taisen (C)

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 195

    Known Boards: Type E by Waixing
    Games: Captain Tsubasa Vol. II (C), Chaos World, God
          Slayer (C), Zu Qiu Xiao Jiang

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 196

    Known Boards: Unknown Bootleg Board
    Games: Super Mario Bros. 11, Super Mario Bros. 17

    This acts basically like a MMC3 with different use of write
    address.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper196_w )
{
	LOG_MMC(("mapper196_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6004)
	{
	case 0x0000:
		mapper4_w(space, 0x0000, data);
		break;
	case 0x0004:
		mapper4_w(space, 0x0001, data);
		break;
	case 0x2000:
		set_nt_mirroring(data & 0x01 ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);	// diff with MMC3: not check for data & 0x40
		break;
	case 0x2004:
		mapper4_w(space, 0x2001, data);
		break;
	case 0x4000:
		mapper4_w(space, 0x4000, data);
		break;
	case 0x4004:
		mapper4_w(space, 0x4001, data);
		break;
	case 0x6000:
		mapper4_w(space, 0x6000, data);
		break;
	case 0x6004:
		mapper4_w(space, 0x6001, data);
		break;
	default:
		break;
	}
}

/*************************************************************

    Mapper 197

    Known Boards: Unknown Bootleg Board
    Games: Super Fighter III

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 198

    Known Boards: Type F by Waixing
    Games: Tenchi wo Kurau II (C)

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 199

    Known Boards: Type G by Waixing
    Games: San Guo Zhi 2, Dragon Ball Z Gaiden (C), Dragon
          Ball Z II (C)

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 200

    Known Boards: Unknown Bootleg Multigame Board
    Games: 36 in 1, 1200 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper200_w )
{
	LOG_MMC(("mapper200_w, offset: %04x, data: %02x\n", offset, data));

	prg16_89ab(space->machine, offset & 0x07);
	prg16_cdef(space->machine, offset & 0x07);
	chr8(space->machine, offset & 0x07, CHRROM);

	set_nt_mirroring((offset & 0x08) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
}

/*************************************************************

    Mapper 201

    Known Boards: Unknown Bootleg Multigame Board
    Games: 8 in 1, 21 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper201_w )
{
	LOG_MMC(("mapper201_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, offset & 0x03);
	chr8(space->machine, offset & 0x03, CHRROM);
}

/*************************************************************

    Mapper 202

    Known Boards: Unknown Bootleg Multigame Board
    Games: 150 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper202_w )
{
	int bank = (offset >> 1) & 0x07;

	LOG_MMC(("mapper202_w, offset: %04x, data: %02x\n", offset, data));

	prg16_89ab(space->machine, bank);
	prg16_cdef(space->machine, bank + (((bank & 0x06) == 0x06) ? 1 : 0));
	chr8(space->machine, bank, CHRROM);

	set_nt_mirroring((offset & 0x01) ? PPU_MIRROR_HORZ: PPU_MIRROR_VERT);
}

/*************************************************************

    Mapper 203

    Known Boards: Unknown Bootleg Multigame Board
    Games: 35 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper203_w )
{
	LOG_MMC(("mapper203_w, offset: %04x, data: %02x\n", offset, data));

	prg16_89ab(space->machine, (data >> 2) & 0x03);
	prg16_cdef(space->machine, (data >> 2) & 0x03);
	chr8(space->machine, data & 0x03, CHRROM);
}

/*************************************************************

    Mapper 204

    Known Boards: Unknown Bootleg Multigame Board
    Games: 64 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper204_w )
{
	int bank = (offset >> 1) & (offset >> 2) & 0x01;

	LOG_MMC(("mapper204_w, offset: %04x, data: %02x\n", offset, data));

	prg16_89ab(space->machine, offset & ~bank);
	prg16_cdef(space->machine, offset | bank);
	chr8(space->machine, offset & ~bank, CHRROM);

	set_nt_mirroring((offset & 0x10) ? PPU_MIRROR_HORZ: PPU_MIRROR_VERT);
}

/*************************************************************

    Mapper 205

    Known Boards: Unknown Bootleg Multigame Board
    Games: 3 in 1, 15 in 1

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 206

    Known Boards: Namcot 34x3 boards, DEROM, DE1ROM, DRROM and
          Tengen 800002, 800004, 800030 boards
    Games: Babel no Tou, Dragon Buster II, Family Circuit,
          Family Tennis, Fantasy Zone, Gauntlet, Karnov,
          Mappy Land, Pac-Mania, R.B.I. Baseball, Super Sprint

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper206_w )
{
	if ((offset & 0x6001) == 0x2000)
		return;

	mapper4_w(space, offset, data);
}

/*************************************************************

    Mapper 207

    Known Boards: Taito X1-005 Ver. B
    Games: Fudou Myouou Den

    Very similar to mapper 80, but with a very particular NT
    mirroring procedure (see below)

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper207_m_w )
{
	LOG_MMC(("mapper207_m_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
		case 0x1ef0:
			set_nt_page(0, CIRAM, (data & 0x80) ? 1 : 0, 1);
			set_nt_page(1, CIRAM, (data & 0x80) ? 1 : 0, 1);
			/* Switch 2k VROM at $0000 */
			chr2_0(space->machine, (data & 0x7f) >> 1, CHRROM);
			break;
		case 0x1ef1:
			set_nt_page(2, CIRAM, (data & 0x80) ? 1 : 0, 1);
			set_nt_page(3, CIRAM, (data & 0x80) ? 1 : 0, 1);
			/* Switch 2k VROM at $0000 */
			chr2_2(space->machine, (data & 0x7f) >> 1, CHRROM);
			break;
		case 0x1ef2:
			/* Switch 1k VROM at $1000 */
			chr1_4(space->machine, data, CHRROM);
			break;
		case 0x1ef3:
			/* Switch 1k VROM at $1400 */
			chr1_5(space->machine, data, CHRROM);
			break;
		case 0x1ef4:
			/* Switch 1k VROM at $1800 */
			chr1_6(space->machine, data, CHRROM);
			break;
		case 0x1ef5:
			/* Switch 1k VROM at $1c00 */
			chr1_7(space->machine, data, CHRROM);
			break;
		case 0x1efa:
		case 0x1efb:
			/* Switch 8k ROM at $8000 */
			prg8_89(space->machine, data);
			break;
		case 0x1efc:
		case 0x1efd:
			/* Switch 8k ROM at $a000 */
			prg8_ab(space->machine, data);
			break;
		case 0x1efe:
		case 0x1eff:
			/* Switch 8k ROM at $c000 */
			prg8_cd(space->machine, data);
			break;
		default:
			logerror("mapper207_m_w uncaught addr: %04x, value: %02x\n", offset + 0x6000, data);
			break;
	}
}

/*************************************************************

    Mapper 208

    Known Boards: 37017 (?) by Gouder
    Games: Street Fighter IV

    MMC3 clone. It also requires reads from 0x5000-0x7fff.

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 209

    Known Boards: Type B by J.Y. Company
    Games: Power Rangers 3, Power Rangers 4, Shin Samurai
          Spirits 2

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 210

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 211

    Known Boards: Type C by J.Y. Company
    Games: Tiny Toon Adventures 6, Donkey Kong Country 4

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 212

    Known Boards: Unknown Bootleg Multigame Board
    Games: 100000 in 1, Super HIK 300 in 1, 1997 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper212_w )
{
	LOG_MMC(("mapper212_w, offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring((offset & 0x08) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
	chr8(space->machine, offset, CHRROM);

	if (offset < 0x4000)
	{
		prg16_89ab(space->machine, offset);
		prg16_cdef(space->machine, offset);
	}
	else
		prg32(space->machine, offset >> 1);
}

/*************************************************************

    Mapper 213

    Known Boards: Unknown Bootleg Multigame Board
    Games: 9999999 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper213_w )
{
	LOG_MMC(("mapper213_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, offset >> 1);
	chr8(space->machine, offset >> 3, CHRROM);
}

/*************************************************************

    Mapper 214

    Known Boards: Unknown Bootleg Multigame Board
    Games: Super Gun 20 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper214_w )
{
	LOG_MMC(("mapper214_w, offset: %04x, data: %02x\n", offset, data));

	prg16_89ab(space->machine, offset >> 2);
	prg16_cdef(space->machine, offset >> 2);
	chr8(space->machine, offset, CHRROM);
}

/*************************************************************

    Mapper 215

    Known Boards: Unknown Bootleg Board by Super Game
    Games: Boogerman, Mortal Kombat III

    MMC3 clone. Also, it probably needs a hack to support both
    variants (Boogerman & MK3).

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 216

    Known Boards: Unknown Bootleg Board by RCM
    Games: Bonza, Magic Jewelry 2

    Very simple mapper: writes to 0x8000-0xffff sets prg32
    to offset and chr8 to offset>>1 (when chrrom is present)

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper216_w )
{
	LOG_MMC(("mapper216_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, offset);
	chr8(space->machine, offset >> 1, mmc_chr_source);
}

/*************************************************************

    Mapper 217

    Known Boards: Unknown Bootleg Multigame Board
    Games: Golden Card 6 in 1

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 218

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 219

    Known Boards: A9746??
    Games: [no games in nes.hsi]

    MMC3 clone according to NEStopia

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 220

    Known Boards: ???
    Games: Summer Carnival '92 - Recca US ?? (MMC3 for nestopia)

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 221

    Known Boards: Unknown Bootleg Board, N625092
    Games: 400 in 1, 700 in 1, 1000 in 1

    In MESS: Supported.

*************************************************************/

static void mapper221_set_prg( running_machine *machine, UINT8 reg1, UINT8 reg2 )
{
	UINT8 map221_helper1, map221_helper2;

	map221_helper1 = !(reg1 & 0x01) ? reg2 :
				(reg1 & 0x80) ? reg2 : (reg2 & 0x06) | 0x00;
	map221_helper2 = !(reg1 & 0x01) ? reg2 :
				(reg1 & 0x80) ? 0x07 : (reg2 & 0x06) | 0x01;

	prg16_89ab(machine, map221_helper1 | ((reg1 & 0x70) >> 1));
	prg16_cdef(machine, map221_helper2 | ((reg1 & 0x70) >> 1));
}

static WRITE8_HANDLER( mapper221_w )
{
	LOG_MMC(("mapper221_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x4000)
	{
		set_nt_mirroring(offset & 0x01 ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		offset = (offset >> 1) & 0xff;

		if (mmc_cmd1 != offset)
		{
			mmc_cmd1 = offset;
			mapper221_set_prg(space->machine, mmc_cmd1, mmc_cmd2);
		}
	}
	else
	{
		offset &= 0x07;

		if (mmc_cmd2 != offset)
		{
			mmc_cmd2 = offset;
			mapper221_set_prg(space->machine, mmc_cmd1, mmc_cmd2);
		}
	}
}

/*************************************************************

    Mapper 222

    Known Boards: Unknown Bootleg Board
    Games: Dragon Ninja (Bootleg), Super Mario Bros. 8

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 223

    Known Boards: Type I by Waixing
    Games: [no games in nes.hsi] Tang Mu Li Xian Ji?

    MMC3 clone according to NEStopia

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 224

    Known Boards: Type J by Waixing
    Games: [no games in nes.hsi] Ying Xiong Chuan Qi?

    MMC3 clone according to NEStopia

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 225

    Known Boards: Unknown Bootleg Multigame Board
    Games: 72 in 1, 115 in 1 and other multigame carts

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper225_w )
{
	int hi_bank;
	int size_16;
	int bank;

	LOG_MMC(("mapper225_w, offset: %04x, data: %02x\n", offset, data));

	chr8(space->machine, offset, CHRROM);
	set_nt_mirroring((offset & 0x2000) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	hi_bank = offset & 0x40;
	size_16 = offset & 0x1000;
	bank = (offset & 0xf80) >> 7;
	if (size_16)
	{
		bank <<= 1;
		if (hi_bank)
			bank ++;

		prg16_89ab(space->machine, bank);
		prg16_cdef(space->machine, bank);
	}
	else
		prg32(space->machine, bank);
}

/*************************************************************

    Mapper 226

    Known Boards: Unknown Bootleg Multigame Board
    Games: 76 in 1, Super 42 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper226_w )
{
	int hi_bank;
	int size_16;
	int bank;

	LOG_MMC(("mapper226_w, offset: %04x, data: %02x\n", offset, data));

	if (offset & 0x01)
	{
		mmc_cmd2 = data;
	}
	else
	{
		mmc_cmd1 = data;
	}

	set_nt_mirroring((mmc_cmd1 & 0x40) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	hi_bank = mmc_cmd1 & 0x01;
	size_16 = mmc_cmd1 & 0x20;
	bank = ((mmc_cmd1 & 0x1e) >> 1) | ((mmc_cmd1 & 0x80) >> 3) | ((mmc_cmd2 & 0x01) << 5);

	if (size_16)
	{
		bank <<= 1;
		if (hi_bank)
			bank ++;

		prg16_89ab(space->machine, bank);
		prg16_cdef(space->machine, bank);
	}
	else
		prg32(space->machine, bank);
}

/*************************************************************

    Mapper 227

    Known Boards: Unknown Bootleg Multigame Board
    Games: 1200 in 1, 295 in 1, 76 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper227_w )
{
	int hi_bank;
	int size_32;
	int bank;

	LOG_MMC(("mapper227_w, offset: %04x, data: %02x\n", offset, data));

	hi_bank = offset & 0x04;
	size_32 = offset & 0x01;
	bank = ((offset & 0x78) >> 3) | ((offset & 0x0100) >> 4);
	if (!size_32)
	{
		bank <<= 1;
		if (hi_bank)
			bank ++;

		prg16_89ab(space->machine, bank);
		prg16_cdef(space->machine, bank);
	}
	else
		prg32(space->machine, bank);

	if (!(offset & 0x80))
	{
		if (offset & 0x200)
			prg16_cdef(space->machine, ((bank << 1) & 0x38) + 7);
		else
			prg16_cdef(space->machine, ((bank << 1) & 0x38));
	}

	set_nt_mirroring((offset & 0x02) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
}

/*************************************************************

    Mapper 228

    Known Boards: Bootleg Board by Active Enterprise
    Games: Action 52, Cheetah Men II

    In MESS: Only Partially Supported.

*************************************************************/

static WRITE8_HANDLER( mapper228_w )
{
	/* The address lines double as data */
	/* --mPPppppPP-cccc */
	int bank;
	int chr;

	LOG_MMC(("mapper228_w, offset: %04x, data: %02x\n", offset, data));

	/* Determine low 4 bits of program bank */
	bank = (offset & 0x780) >> 7;

#if 1
	/* Determine high 2 bits of program bank */
	switch (offset & 0x1800)
	{
		case 0x0800:
			bank |= 0x10;
			break;
		case 0x1800:
			bank |= 0x20;
			break;
	}
#else
	bank |= (offset & 0x1800) >> 7;
#endif

	/* see if the bank value is 16k or 32k */
	if (offset & 0x20)
	{
		/* 16k bank value, adjust */
		bank <<= 1;
		if (offset & 0x40)
			bank ++;

		prg16_89ab(space->machine, bank);
		prg16_cdef(space->machine, bank);
	}
	else
	{
		prg32(space->machine, bank);
	}

	if (offset & 0x2000)
		set_nt_mirroring(PPU_MIRROR_HORZ);
	else
		set_nt_mirroring(PPU_MIRROR_VERT);

	/* Now handle vrom banking */
	chr = (data & 0x03) + ((offset & 0x0f) << 2);
	chr8(space->machine, chr, CHRROM);
}

/*************************************************************

    Mapper 229

    Known Boards: Unknown Bootleg Multigame Board
    Games: 31 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper229_w )
{
	LOG_MMC(("mapper229_w, offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring((offset & 0x20) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
	chr8(space->machine, offset, CHRROM);

	if ((offset & 0x1e) == 0)
	{
		prg16_89ab(space->machine, 0);
		prg16_89ab(space->machine, 1);
	}
	else
	{
		prg16_89ab(space->machine, offset & 0x1f);
		prg16_89ab(space->machine, offset & 0x1f);
	}
}

/*************************************************************

    Mapper 230

    Known Boards: Unknown Bootleg Multigame Board
    Games: 22 in 1

    In MESS: Supported. Needs a reset to work (not possible yet)

*************************************************************/

static WRITE8_HANDLER( mapper230_w )
{
	LOG_MMC(("mapper230_w, offset: %04x, data: %02x\n", offset, data));

	if (1)	// this should flip at reset
	{
		prg16_89ab(space->machine, data & 0x07);
	}
	else
	{
		if (data & 0x20)
		{
			prg16_89ab(space->machine, (data & 0x1f) + 8);
			prg16_cdef(space->machine, (data & 0x1f) + 8);
		}
		else
		{
			prg16_89ab(space->machine, (data & 0x1f) + 8);
			prg16_cdef(space->machine, (data & 0x1f) + 9);
		}
		set_nt_mirroring((data & 0x40) ? PPU_MIRROR_VERT : PPU_MIRROR_HORZ);
	}
}

/*************************************************************

    Mapper 231

    Known Boards: Unknown Bootleg Multigame Board
    Games: 20 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper231_w )
{
	LOG_MMC(("mapper231_w, offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring((offset & 0x80) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	prg16_89ab(space->machine, (offset & 0x1e));
	prg16_cdef(space->machine, (offset & 0x1e) + (data & 0x20) ? 1 : 0);
}

/*************************************************************

    Mapper 232

    Known Boards: Camerica BF9096 & ALGQV11 Boards
    Games: Quattro Adventure, Quattro Arcade, Quattro Sports

    Writes to 0x8000-0x9fff set prg block to (data&0x18)>>1,
    writes to 0xa000-0xbfff set prg page to data&3. selected
    prg are: prg16_89ab = block|page, prg_cdef = 3|page.
    For more info on the hardware to bypass the NES lockout, see
    Kevtris' Camerica Mappers documentation.

    In MESS: Supported.

*************************************************************/

static void mapper232_set_prg( running_machine *machine )
{
	prg16_89ab(machine, (mmc_cmd2 & 0x03) | ((mmc_cmd1 & 0x18) >> 1));
	prg16_cdef(machine, 0x03 | ((mmc_cmd1 & 0x18) >> 1));
}

static WRITE8_HANDLER( mapper232_w )
{
	LOG_MMC(("mapper232_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x2000)
		mmc_cmd1 = data;
	else
		mmc_cmd2 = data;

	mapper232_set_prg(space->machine);
}

/*************************************************************

    Mapper 233

    Known Boards: Unknown Bootleg Multigame Board
    Games: Super 22 in 1

    In MESS: Unsupported. Banking depends on dipswitch, so we
      need to implement these...

*************************************************************/

/*************************************************************

    Mapper 234

    Known Boards: Bootleg Board by AVE (D1012)
    Games: Maxi 15 Eur

    In MESS: Unsupported. Needs to read from 0xff80-0xffff.

*************************************************************/

/*************************************************************

    Mapper 235

    Known Boards: Unknown Bootleg Board by Golden Game
    Games: 260 in 1, 150 in 1

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 236

    Known Boards: Unknown Bootleg Multigame Board
    Games: a 800 in 1 multigame cart present in NEStopia db.

    No info available.

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 237

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 238

    Known Boards: Unknown Bootleg Board, 6035052
    Games: Contra Fighter

    MMC3 simple clone.

    In MESS: Unsupported. Needs to read from 0x6000-0x7fff (for
          protection). It also needs to read in 0x4020-0x40ff.

*************************************************************/

/*************************************************************

    Mapper 239

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 240

    Known Boards: Bootleg Board by C&E
    Games: Jing Ke Xin Zhuan, Sheng Huo Lie Zhuan

    Simple Mapper: writes to 0x4020-0x5fff sets prg32 to
         data>>4 and chr8 to data&f. We currently do not map
         writes to 0x4020-0x40ff (to do: verify if this produces
         issues)

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper240_l_w )
{
	LOG_MMC(("mapper240_l_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, data >> 4);
	chr8(space->machine, data & 0x0f, CHRROM);
}

/*************************************************************

    Mapper 241

    Known Boards: Bootleg Board by TXC
    Games: Commandos, Journey to the West, Ma Bu Mi Zhen &
           Qu Wei Cheng Yu Wu, Si Lu Chuan Qi

    Simple Mapper: writes to 0x8000-0xffff sets the prg32 bank.
    Not sure if returning 0x50 for reads in 0x4100-0x5000 is correct.

    In MESS: Supported.

*************************************************************/

static READ8_HANDLER( mapper241_l_r )
{
	return 0x50;
}

static WRITE8_HANDLER( mapper241_w )
{
	LOG_MMC(("mapper241_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, data);
}

/*************************************************************

    Mapper 242

    Known Boards: Bootleg Board by Waixing
    Games: Wai Xing Zhan Shi, Dragon Quest VIII

    Simple mapper: writes to 0x8000-0xffff sets prg32 banks to
        (offset>>3)&f. written data&3 sets the mirroring (with
        switched high/low compared to the standard one). However,
        a second variant DQ7 and requires no NT mirroring.
        Currently, we do not support this variant.

    In MESS: Partial Support.

*************************************************************/

static WRITE8_HANDLER( mapper242_w )
{
	LOG_MMC(("mapper242_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, (offset >> 3) & 0x0f);

	waixing_set_mirror(data & 0x03);
}

/*************************************************************

    Mapper 243

    Known Boards: Bootleg Board by Sachen (74SL374A)
    Games: Poker III

    Conflicting info about this. Still investigating. The board is similar
    to the one of mapper 150.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper243_l_w )
{
	LOG_MMC(("mapper243_l_w, offset: %04x, data: %02x\n", offset, data));

	/* write happens only if we are at 0x4100 + k * 0x200,
    but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
	{
		if (!(offset & 0x01))
			mmc_cmd1 = data;
		else
		{
			switch (mmc_cmd1 & 0x07)
			{
			case 0x00:
				prg32(space->machine, 0);
				chr8(space->machine, 3, CHRROM);
				break;
			case 0x02:
				chr8(space->machine, (mmc_bank_latch1 & ~0x08) | (data << 3 & 0x08), CHRROM);	// Nestopia
				break;
			case 0x04:
				chr8(space->machine, (mmc_bank_latch1 & ~0x01) | (data << 0 & 0x01), CHRROM);	// Nestopia
				break;
			case 0x05:
				prg32(space->machine, data & 0x01);
				break;
			case 0x06:
				chr8(space->machine, (mmc_bank_latch1 & ~0x06) | (data << 1 & 0x06), CHRROM);	// Nestopia
				break;
			case 0x07:
				set_nt_mirroring(data & 0x01 ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
				break;
			default:
				break;
			}
		}
	}
}

/*************************************************************

    Mapper 244

    Known Boards: Unknown Bootleg Board by C&E
    Games: Decathlon

    Pretty simple mapper: writes to 0x8065-0x80a4 set prg32 to
    data & 3; writes to 0x80a5-0x80e4 set chr8 to data & 7

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper244_w )
{
	LOG_MMC(("mapper244_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x0065)
		return;
	if (offset < 0x00a5)
	{
		prg32(space->machine, (offset - 0x0065) & 0x03);
		return;
	}
	if (offset < 0x00e5)
	{
		chr8(space->machine, (offset - 0x00a5) & 0x07, CHRROM);
	}
}

/*************************************************************

    Mapper 245

    Known Boards: Type H by Waixing
    Games: Ying Xiong Yuan Yi Jing Chuan Qi

    MMC3 clone. More info to come.

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 246

    Known Boards: Unknown Bootleg Board by C&E
    Games: Fong Shen Bang - Zhu Lu Zhi Zhan

    Simple mapper: writes to 0x6000-0x67ff set PRG and CHR banks.
    Namely, 0x6000->0x6003 select resp. prg8_89, prg8_ab, prg8_cd
    and prg8_ef. 0x6004->0x6007 select resp. crh2_0, chr2_2,
    chr2_4 and chr2_6. In 0x6800-0x7fff lies WRAM.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper246_m_w )
{
	LOG_MMC(("mapper246_m_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x0800)
	{
		switch (offset & 0x0007)
		{
		case 0x0000:
			prg8_89(space->machine, data);
			break;
		case 0x0001:
			prg8_ab(space->machine, data);
			break;
		case 0x0002:
			prg8_cd(space->machine, data);
			break;
		case 0x0003:
			prg8_ef(space->machine, data);
			break;
		case 0x0004:
			chr2_0(space->machine, data, CHRROM);
			break;
		case 0x0005:
			chr2_2(space->machine, data, CHRROM);
			break;
		case 0x0006:
			chr2_4(space->machine, data, CHRROM);
			break;
		case 0x0007:
			chr2_6(space->machine, data, CHRROM);
			break;
		}
	}
	else
		nes.wram[offset] = data;
}

/*************************************************************

    Mapper 247

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 248

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 249

    Known Boards: Waixing Security 0 & 1
    Games: Duo Bao Xiao Ying Hao - Guang Ming yu An Hei Chuan Shuo,
           Myth Struggle, San Shi Liu Ji, Shui Hu Zhuan

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 250

    Known Boards: Unknown Bootleg Board by Nitra
    Games: Time Diver Avenger

    This acts basically like a MMC3 with different use of write
    address. According to NEStopia, there is also a slightly
    different way to set NT mirroring (see below).

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper250_w )
{
	LOG_MMC(("mapper250_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x2000:
		set_nt_mirroring(data & 0x01 ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);	// diff with MMC3: not check for data & 0x40
		break;
	default:
		mapper4_w(space, (offset & 0x6000) | ((offset & 0x400) >> 10), offset & 0xff);
		break;
	}
}

/*************************************************************

    Mapper 251

    Known Boards: Undocumented
    Games: Super 8-in-1 99 King Fighter

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 252

    Known Boards: Unknown Bootleg Board by Waixing
    Games: San Guo Zhi

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 253

    Known Boards: Undocumented
    Games: Shen Hua Jian Yun III ?

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 254

    Known Boards: Unknown Bootleg Board
    Games: Pikachu Y2K

    MMC3 simple clone.

    In MESS: Unsupported. Needs to read from 0x6000-0x7fff (for
          protection).

*************************************************************/

/*************************************************************

    Mapper 255

    Known Boards: Unknown Bootleg Board
    Games: 110 in 1

    In MESS: Unsupported.

*************************************************************/

static WRITE8_HANDLER( mapper255_w )
{
	UINT8 map255_helper1 = (offset >> 12) ? 0 : 1;
	UINT8 map255_helper2 = ((offset >> 8) & 0x40) | ((offset >> 6) & 0x3f);

	LOG_MMC(("mapper255_w, offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring((offset & 0x2000) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
	prg16_89ab(space->machine, map255_helper1 & ~map255_helper2);
	prg16_cdef(space->machine, map255_helper1 | map255_helper2);
	chr8(space->machine, ((offset >> 8) & 0x40) | (offset & 0x3f), CHRROM);
}



/*************************************************************

    mmc_list

    Supported mappers and corresponding handlers

*************************************************************/

static const mmc mmc_list[] =
{
/*  INES   DESC                     LOW_W, LOW_R, MED_W, HIGH_W, PPU_latch, scanline CB, hblank CB */
	{  0, "No Mapper",            NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{  1, "MMC1",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL },
	{  2, "74161/32 ROM sw",      NULL, NULL, NULL, mapper2_w, NULL, NULL, NULL },
	{  3, "74161/32 VROM sw",     NULL, NULL, NULL, mapper3_w, NULL, NULL, NULL },
	{  4, "MMC3",                 NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq },
	{  5, "MMC5",                 mapper5_l_w, mapper5_l_r, NULL, mapper5_w, NULL, NULL, mapper5_irq },
	{  7, "ROM Switch",           NULL, NULL, NULL, mapper7_w, NULL, NULL, NULL },
	{  8, "FFE F3xxxx",           NULL, NULL, NULL, mapper8_w, NULL, NULL, NULL },
	{  9, "MMC2",                 NULL, NULL, NULL, mapper9_w, mapper9_latch, NULL, NULL},
	{ 10, "MMC4",                 NULL, NULL, NULL, mapper10_w, mapper9_latch, NULL, NULL },
	{ 11, "Color Dreams Mapper",  NULL, NULL, NULL, mapper11_w, NULL, NULL, NULL },
	{ 13, "CP-ROM",               NULL, NULL,	NULL, mapper13_w, NULL, NULL, NULL }, //needs CHR-RAM hooked up
	{ 15, "100-in-1",             NULL, NULL, NULL, mapper15_w, NULL, NULL, NULL },
	{ 16, "Bandai",               NULL, NULL, mapper16_m_w, mapper16_w, NULL,NULL,  bandai_irq },
	{ 17, "FFE F8xxx",            mapper17_l_w, NULL, NULL, NULL, NULL, NULL, mapper4_irq },
	{ 18, "Jaleco",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq },
	{ 19, "Namco 106",            mapper19_l_w, NULL, NULL, mapper19_w, NULL, NULL, namcot_irq },
	{ 20, "Famicom Disk System",  NULL, NULL, NULL, NULL, NULL, NULL, fds_irq },
	{ 21, "Konami VRC 4",         NULL, NULL, NULL, konami_vrc4_w, NULL, NULL, konami_irq },
	{ 22, "Konami VRC 2a",        NULL, NULL, NULL, konami_vrc2a_w, NULL, NULL, NULL },
	{ 23, "Konami VRC 2b",        NULL, NULL, NULL, konami_vrc2b_w, NULL, NULL, konami_irq },
	{ 24, "Konami VRC 6a",        NULL, NULL, NULL, konami_vrc6a_w, NULL, NULL, konami_irq },
	{ 25, "Konami VRC 4",         NULL, NULL, NULL, konami_vrc4_w, NULL, NULL, konami_irq },
	{ 26, "Konami VRC 6b",        NULL, NULL, NULL, konami_vrc6b_w, NULL, NULL, konami_irq },
	{ 32, "Irem G-101",           NULL, NULL, NULL, mapper32_w, NULL, NULL, NULL },
	{ 33, "Taito TC0190",         NULL, NULL, NULL, mapper33_w, NULL, NULL, NULL },
	{ 34, "Nina-1",               NULL, NULL, mapper34_m_w, mapper34_w, NULL, NULL, NULL },
	{ 36, "TXC Policeman",        NULL, NULL, NULL, mapper36_w, NULL, NULL, NULL },
	{ 38, "Crime Buster",         NULL, NULL, mapper38_m_w, NULL, NULL, NULL, NULL },
	{ 39, "Subor Study n Game",   NULL, NULL, NULL, mapper39_w, NULL, NULL, NULL },
	{ 40, "SMB2j (bootleg)",      NULL, NULL, NULL, mapper40_w, NULL, NULL, mapper40_irq },
	{ 41, "Caltron 6-in-1",       NULL, NULL, mapper41_m_w, mapper41_w, NULL, NULL, NULL },
	{ 42, "Mario Baby",           NULL, NULL, NULL, mapper42_w, NULL, NULL, NULL },
	{ 43, "150-in-1",             NULL, NULL, NULL, mapper43_w, NULL, NULL, NULL },
	{ 44, "7-in-1 MMC3",          NULL, NULL, NULL, mapper44_w, NULL, NULL, mapper4_irq },
	{ 45, "X-in-1 MMC3",          NULL, NULL, mapper45_m_w, mapper4_w, NULL, NULL, mapper4_irq },
	{ 46, "15-in-1 Color Dreams", NULL, NULL, mapper46_m_w, mapper46_w, NULL, NULL, NULL },
	{ 47, "2-in-1 MMC3",          NULL, NULL, mapper47_m_w, mapper4_w, NULL, NULL, mapper4_irq },
	{ 49, "4-in-1 MMC3",          NULL, NULL, mapper49_m_w, mapper4_w, NULL, NULL, mapper4_irq },
	{ 51, "11-in-1",              NULL, NULL, mapper51_m_w, mapper51_w, NULL, NULL, NULL },
	{ 57, "6-in-1",               NULL, NULL, NULL, mapper57_w, NULL, NULL, NULL },
	{ 58, "X-in-1",               NULL, NULL, NULL, mapper58_w, NULL, NULL, NULL },
	{ 61, "20-in-1",              NULL, NULL, NULL, mapper61_w, NULL, NULL, NULL },
	{ 62, "X-in-1",               NULL, NULL, NULL, mapper62_w, NULL, NULL, NULL },
	{ 64, "Tengen",               NULL, NULL, mapper64_m_w, mapper64_w, NULL, NULL, mapper4_irq },
	{ 65, "Irem H3001",           NULL, NULL, NULL, mapper65_w, NULL, NULL, irem_irq },
	{ 66, "74161/32 Jaleco",      NULL, NULL, NULL, mapper66_w, NULL, NULL, NULL },
	{ 67, "SunSoft 3",            NULL, NULL, NULL, mapper67_w, NULL, NULL, mapper4_irq },
	{ 68, "SunSoft 4",            NULL, NULL, NULL, mapper68_w, NULL, NULL, NULL },
	{ 69, "SunSoft 5",            NULL, NULL, NULL, mapper69_w, NULL, NULL, sunsoft_irq },
	{ 70, "74161/32 Bandai",      NULL, NULL, NULL, mapper70_w, NULL, NULL, NULL },
	{ 71, "Camerica",             NULL, NULL, mapper71_m_w, mapper71_w, NULL, NULL, NULL },
	{ 72, "74161/32 Jaleco",      NULL, NULL, NULL, mapper72_w, NULL, NULL, NULL },
	{ 73, "Konami VRC 3",         NULL, NULL, NULL, mapper73_w, NULL, NULL, konami_irq },
	{ 74, "Waixing Type A",       NULL, NULL, NULL, mapper74_w, NULL, NULL, mapper4_irq },
	{ 75, "Konami VRC 1",         NULL, NULL, NULL, mapper75_w, NULL, NULL, NULL },
	{ 76, "Namco 109",            NULL, NULL, NULL, mapper76_w, NULL, NULL, NULL },
	{ 77, "74161/32 ?",           NULL, NULL, NULL, mapper77_w, NULL, NULL, NULL },
	{ 78, "74161/32 Irem",        NULL, NULL, NULL, mapper78_w, NULL, NULL, NULL },
	{ 79, "Nina-3 (AVE)",         mapper79_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 80, "Taito X1-005 Ver. A",  NULL, NULL, mapper80_m_w, NULL, NULL, NULL, NULL },
	{ 82, "Taito C075",           NULL, NULL, mapper82_m_w, NULL, NULL, NULL, NULL },
	{ 83, "Cony",                 mapper83_l_w, mapper83_l_r, NULL, mapper83_w, NULL, NULL, NULL },
	{ 84, "Pasofami",             NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 85, "Konami VRC 7",         NULL, NULL, NULL, konami_vrc7_w, NULL, NULL, konami_irq },
	{ 86, "Jaleco Early Mapper 2",NULL, NULL, mapper86_m_w, NULL, NULL, NULL, NULL },
	{ 87, "74161/32 VROM sw-a",   NULL, NULL, mapper87_m_w, NULL, NULL, NULL, NULL },
	{ 88, "Namco 118",            NULL, NULL, NULL, mapper88_w, NULL, NULL, NULL },
	{ 89, "Sunsoft Early",        NULL, NULL, NULL, mapper89_w, NULL, NULL, NULL },
// 90 - pirate mapper
	{ 91, "HK-SF3 (bootleg)",     NULL, NULL, mapper91_m_w, NULL, NULL, NULL, NULL },
	{ 92, "Jaleco Early Mapper 1",NULL, NULL, NULL, mapper92_w, NULL, NULL, NULL },
	{ 93, "Sunsoft LS161",        NULL, NULL, NULL, mapper93_w, NULL, NULL, NULL },
	{ 94, "Capcom LS161",         NULL, NULL, NULL, mapper94_w, NULL, NULL, NULL },
	{ 95, "Namco ??",             NULL, NULL, NULL, mapper95_w, NULL, NULL, NULL },
	{ 96, "74161/32",             NULL, NULL, NULL, mapper96_w, NULL, NULL, NULL },
	{ 97, "Irem - PRG HI",        NULL, NULL, NULL, mapper97_w, NULL, NULL, NULL },
// 99 - vs. system
// 100 - images hacked to work with nesticle
	{ 101, "?? LS161",            NULL, NULL, mapper101_m_w, mapper101_w, NULL, NULL, NULL },
	{ 104, "Golden Five",         NULL, NULL, NULL, mapper104_w, NULL, NULL, NULL },
	{ 107, "Magic Dragon",        NULL, NULL, NULL, mapper107_w, NULL, NULL, NULL },
	{ 112, "Asper",               NULL, NULL, NULL, mapper112_w, NULL, NULL, NULL },
	{ 113, "Sachen/Hacker/Nina",  mapper113_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 115, "Bao Qing Tian",       NULL, NULL, mapper115_m_w, mapper115_w, NULL, NULL, mapper115_irq },
	{ 118, "MMC3?",               NULL, NULL, NULL, mapper118_w, NULL, NULL, mapper4_irq },
	{ 119, "MMC3 - Pinbot",       NULL, NULL, NULL, mapper119_w, NULL, NULL, mapper4_irq },
	{ 132, "TXC T22211A",         mapper132_l_w, mapper132_l_r, NULL, mapper132_w, NULL, NULL, NULL },
	{ 133, "Sachen SA72008",      mapper133_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 136, "Sachen TCU01",        mapper136_l_w, mapper136_l_r, NULL, NULL, NULL, NULL, NULL },
	{ 137, "Sachen 8259D",        mapper137_l_w, NULL, mapper137_m_w, NULL, NULL, NULL, NULL },
	{ 138, "Sachen 8259B",        mapper138_l_w, NULL, mapper138_m_w, NULL, NULL, NULL, NULL },
	{ 139, "Sachen 8259C",        mapper139_l_w, NULL, mapper139_m_w, NULL, NULL, NULL, NULL },
	{ 140, "Jaleco",              NULL, NULL, mapper_140_m_w, NULL, NULL, NULL, NULL },
	{ 141, "Sachen 8259A",        mapper141_l_w, NULL, mapper141_m_w, NULL, NULL, NULL, NULL },
	{ 143, "Sachen TCA01",        NULL, mapper143_l_r, NULL, NULL, NULL, NULL, NULL },
	{ 144, "AGCI 50282",          NULL, NULL, NULL, mapper144_w, NULL, NULL, NULL }, //Death Race only
	{ 145, "Sachen SA72007",      mapper145_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 146, "Sachen SA0161M",      mapper79_l_w, NULL, NULL, NULL, NULL, NULL, NULL }, // basically same as Mapper 79 (Nina006)
	{ 147, "Sachen TCU02",        mapper147_l_w, NULL, mapper147_m_w, mapper147_w, NULL, NULL, NULL },
	{ 148, "Sachen SA0037",       NULL, NULL, NULL, mapper148_w, NULL, NULL, NULL },
	{ 149, "Sachen SA0036",       NULL, NULL, NULL, mapper149_w, NULL, NULL, NULL },
	{ 150, "Sachen 74LS374B",     mapper150_l_w, mapper150_l_r, mapper150_m_w, NULL, NULL, NULL, NULL },
	{ 152, "Taito Discrete",      NULL, NULL, NULL, mapper152_w, NULL, NULL, NULL },
	{ 154, "Devil Man",           NULL, NULL, NULL, mapper154_w, NULL, NULL, NULL },
	{ 156, "Open Corp. DAOU36",   NULL, NULL, NULL, mapper156_w, NULL, NULL, NULL },
	{ 164, "Final Fantasy V",     mapper164_l_w, NULL, NULL, mapper164_w, NULL, NULL, NULL },
	{ 166, "Subor Board Type 1",  NULL, NULL, NULL, mapper166_w, NULL, NULL, NULL },
	{ 167, "Subor Board Type 2",  NULL, NULL, NULL, mapper167_w, NULL, NULL, NULL },
	{ 171, "Kaiser KS7058",       NULL, NULL, NULL, mapper171_w, NULL, NULL, NULL },
	{ 172, "TXC T22211B",         mapper132_l_w, mapper132_l_r, NULL, mapper172_w, NULL, NULL, NULL },
	{ 173, "TXC T22211C",         mapper132_l_w, mapper173_l_r, NULL, mapper132_w, NULL, NULL, NULL },
	{ 176, "Zhi Li Xiao Zhuan Yuan",mapper176_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 177, "Henggedianzi Board",  NULL, NULL, NULL, mapper177_w, NULL, NULL, NULL },
	{ 178, "San Guo Zhong Lie Zhuan",mapper178_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 179, "Xing He Zhan Shi",    mapper179_l_w, NULL, NULL, mapper179_w, NULL, NULL, NULL },
	{ 180, "Nihon Bussan - PRG HI",NULL, NULL, NULL, mapper180_w, NULL, NULL, NULL },
	{ 182, "Super games",         NULL, NULL, NULL, mapper182_w, NULL, NULL, mapper182_irq },
	{ 184, "Sunsoft VROM/4K",     NULL, NULL, mapper184_m_w, NULL, NULL, NULL, NULL },
	{ 188, "UNROM reversed",      NULL, NULL, NULL, mapper188_w, NULL, NULL, NULL },
	{ 193, "Fighting Hero",       NULL, NULL, mapper193_m_w, NULL, NULL, NULL, NULL },
	{ 196, "Super Mario Bros. 11",NULL, NULL, NULL, mapper196_w, NULL, NULL, mapper4_irq },
	{ 200, "X-in-1",              NULL, NULL, NULL, mapper200_w, NULL, NULL, NULL },
	{ 201, "X-in-1",              NULL, NULL, NULL, mapper201_w, NULL, NULL, NULL },
	{ 202, "150-in-1",            NULL, NULL, NULL, mapper202_w, NULL, NULL, NULL },
	{ 203, "35-in-1",             NULL, NULL, NULL, mapper203_w, NULL, NULL, NULL },
	{ 204, "64-in-1",             NULL, NULL, NULL, mapper204_w, NULL, NULL, NULL },
	{ 206, "MIMIC-1",             NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq },
	{ 207, "Taito X1-005 Ver. B", NULL, NULL, mapper207_m_w, NULL, NULL, NULL, NULL },
	{ 212, "Super HIK 300-in-1",  NULL, NULL, NULL, mapper212_w, NULL, NULL, NULL },
	{ 213, "9999999-in-1",        NULL, NULL, NULL, mapper213_w, NULL, NULL, NULL },
	{ 214, "Super Gun 20-in-1",   NULL, NULL, NULL, mapper214_w, NULL, NULL, NULL },
	{ 216, "RCM GS2015",          NULL, NULL, NULL, mapper216_w, NULL, NULL, NULL },
	{ 221, "X-in-1 (N625092)",    NULL, NULL, NULL, mapper221_w, NULL, NULL, NULL },
	{ 225, "72-in-1 bootleg",     NULL, NULL, NULL, mapper225_w, NULL, NULL, NULL },
	{ 226, "76-in-1 bootleg",     NULL, NULL, NULL, mapper226_w, NULL, NULL, NULL },
	{ 227, "1200-in-1 bootleg",   NULL, NULL, NULL, mapper227_w, NULL, NULL, NULL },
	{ 228, "Action 52",           NULL, NULL, NULL, mapper228_w, NULL, NULL, NULL },
	{ 229, "31-in-1",             NULL, NULL, NULL, mapper229_w, NULL, NULL, NULL },
	{ 230, "22-in-1",             NULL, NULL, NULL, mapper230_w, NULL, NULL, NULL },
	{ 231, "Nina-7 (AVE)",        NULL, NULL, NULL, mapper231_w, NULL, NULL, NULL },
	{ 232, "Quattro",             NULL, NULL, mapper232_w, mapper232_w, NULL, NULL, NULL },
// 234 - maxi-15
	{ 240, "Jing Ke Xin Zhuan",   mapper240_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 241, "Education 18-in-1",   NULL, mapper241_l_r, NULL, mapper241_w, NULL, NULL, NULL },
	{ 242, "Wai Xing Zhan Shi",   NULL, NULL, NULL, mapper242_w, NULL, NULL, NULL },
	{ 243, "Sachen 74LS374A",     mapper243_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 244, "Decathlon",           NULL, NULL, NULL, mapper244_w, NULL, NULL, NULL },
	{ 246, "Fong Shen Bang",      NULL, NULL, mapper246_m_w, NULL, NULL, NULL, NULL },
	{ 250, "Time Diver Avenger",  NULL, NULL, NULL, mapper250_w, NULL, NULL, mapper4_irq },
	{ 255, "X-in-1",              NULL, NULL, NULL, mapper255_w, NULL, NULL, NULL },
};


const mmc *nes_mapper_lookup( int mapper )
{
	int i;
	for (i = 0; i < sizeof(mmc_list) / sizeof(mmc_list[0]); i++)
	{
		if (mmc_list[i].iNesMapper == nes.mapper)
			return &mmc_list[i];
	}
	return NULL;
}

/*************************************************************

    mapper_reset

    Resets the mapper bankswitch areas to their defaults.
    It returns a value "err" that indicates if it was
    successful. Possible values for err are:

    0 = success
    1 = no mapper found
    2 = mapper not supported

*************************************************************/

int mapper_reset( running_machine *machine, int mmc_num )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	nes_state *state = machine->driver_data;
	int err = 0;
	const mmc *mapper;

	if (nes.chr_chunks == 0)
		chr8(machine, 0,CHRRAM);
	else
		chr8(machine, 0,CHRROM);

	/* Set the mapper irq callback */
	mapper = nes_mapper_lookup(mmc_num);

	if (mapper == NULL)
		fatalerror("Unimplemented Mapper");

	ppu2c0x_set_scanline_callback(state->ppu, mapper ? mapper->mmc_scanline : NULL);
	ppu2c0x_set_hblank_callback(state->ppu, mapper ? mapper->mmc_hblank : NULL);

	if (!nes_irq_timer)
		nes_irq_timer = timer_alloc(machine, nes_irq_callback, NULL);

	mapper_warning = 0;
	/* 8k mask */
	prg_mask = ((nes.prg_chunks << 1) - 1);

	MMC5_vram_control = 0;

	/* Point the WRAM/battery area to the first RAM bank */
	if (mmc_num != 20 && mmc_num != 40)
		memory_set_bankptr(space->machine, 5, &nes.wram[0x0000]);

	switch (mmc_num)
	{
		case 0:
			err = 1; /* No mapper found */
			prg32(space->machine, 0);
			break;
		case 1:
			/* Reset the latch */
			MMC1_reg = 0;
			MMC1_reg_count = 0;

			MMC1_Size_16k = 1;
			MMC1_Switch_Low = 1;
			MMC1_SizeVrom_4k = 0;
			MMC1_extended_bank = 0;
			MMC1_extended_swap = 0;
			MMC1_extended_base = 0x10000;
			MMC1_extended = ((nes.prg_chunks << 4) + nes.chr_chunks * 8) >> 9;

			if (!MMC1_extended)
				/* Set it to the end of the prg rom */
				MMC1_High = (nes.prg_chunks - 1) * 0x4000;
			else
				/* Set it to the end of the first 256k bank */
				MMC1_High = 15 * 0x4000;

			MMC1_bank1 = 0;
			MMC1_bank2 = 0x2000;
			MMC1_bank3 = MMC1_High;
			MMC1_bank4 = MMC1_High + 0x2000;

			memory_set_bankptr(space->machine, 1, &nes.rom[MMC1_extended_base + MMC1_bank1]);
			memory_set_bankptr(space->machine, 2, &nes.rom[MMC1_extended_base + MMC1_bank2]);
			memory_set_bankptr(space->machine, 3, &nes.rom[MMC1_extended_base + MMC1_bank3]);
			memory_set_bankptr(space->machine, 4, &nes.rom[MMC1_extended_base + MMC1_bank4]);
			logerror("-- page1: %06x\n", MMC1_bank1);
			logerror("-- page2: %06x\n", MMC1_bank2);
			logerror("-- page3: %06x\n", MMC1_bank3);
			logerror("-- page4: %06x\n", MMC1_bank4);
			break;
		case 2:
			/* These games don't switch VROM, but some ROMs incorrectly have CHR chunks */
			nes.chr_chunks = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 13:
			chr4_0(space->machine, 0, CHRRAM);
			prg32(space->machine, 0);
			break;
		case 3:
			/* Doesn't bank-switch */
			prg32(space->machine, 0);
			break;
		case 4:
		case 74:
		case 206:
			/* Only MMC3 and clones seem to use 4-screen! */

			/* hardware uses an 8K RAM chip, but can't address the high 4K! */
			/* that much is documented for Nintendo Gauntlet boards */
			if(nes.four_screen_vram)
			{
				extended_ntram=auto_alloc_array(space->machine, UINT8, 0x2000);
				set_nt_page(0, CART_NTRAM, 0, 1);
				set_nt_page(1, CART_NTRAM, 1, 1);
				set_nt_page(2, CART_NTRAM, 2, 1);
				set_nt_page(3, CART_NTRAM, 3, 1);
			}
		case 118:
		case 119:
		case 196:
		case 250:
			/* Can switch 8k prg banks */
			IRQ_enable = 0;
			IRQ_count = IRQ_count_latch = 0;
			IRQ_reload = 0;
			MMC3_prg0 = 0xfe;
			MMC3_prg1 = 0xff;
			MMC3_cmd = 0;
			MMC3_prg_base = 0;
			MMC3_prg_mask = (nes.prg_chunks << 1) - 1;
			MMC3_chr_base = 0;
			MMC3_chr_mask = (nes.chr_chunks << 3) - 1;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine);
			mapper4_set_chr(space->machine, mmc_chr_source);
			break;
		case 5:
			/* Can switch 8k prg banks, but they are saved as 16k in size */
			MMC5_rom_bank_mode = 3;
			MMC5_vrom_bank_mode = 0;
			MMC5_vram_protect = 0;
			IRQ_enable = 0;
			IRQ_count = 0;
			nes.mid_ram_enable = 0;
			prg16_89ab(space->machine, nes.prg_chunks - 2);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 7:
			/* Bankswitches 32k at a time */
			set_nt_mirroring(PPU_MIRROR_LOW);
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 96:
		case 8:
			/* Switches 16k banks at $8000, 1st 2 16k banks loaded on reset */
			prg32(space->machine, 0);
			break;
		case 9:
			/* Can switch 8k prg banks */
			/* Note that the iNES header defines the number of banks as 8k in size, rather than 16k */
			/* Reset VROM latches */
			MMC2_bank0 = MMC2_bank1 = 0;
			MMC2_bank0_hi = MMC2_bank1_hi = 0;
			MMC2_bank0_latch = MMC2_bank1_latch = 0xfe;
			prg8_89(space->machine, 0);
			//ugly hack to deal with iNES header usage of chunk count.
			prg8_ab(space->machine, (nes.prg_chunks << 1) - 3);
			prg8_cd(space->machine, (nes.prg_chunks << 1) - 2);
			prg8_ef(space->machine, (nes.prg_chunks << 1) - 1);
			break;
		case 10:
			/* Reset VROM latches */
			MMC2_bank0 = MMC2_bank1 = 0;
			MMC2_bank0_hi = MMC2_bank1_hi = 0;
			MMC2_bank0_latch = MMC2_bank1_latch = 0xfe;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 11:	/* Switches 32k banks, 1st 32k bank loaded on reset (?) May be more like mapper 7... */
			prg32(space->machine, 0);
			break;
		case 15:	/* Can switch 8k prg banks */
			set_nt_mirroring(PPU_MIRROR_VERT);
			prg32(space->machine, 0);
			break;
		case 16:
		case 17:
		case 18:
		case 19:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 20:
			IRQ_enable = IRQ_enable_latch = 0;
			IRQ_count = IRQ_count_latch = 0;
			nes_fds.motor_on = 0;
			nes_fds.door_closed = 0;
			nes_fds.current_side = 1;
			nes_fds.head_position = 0;
			nes_fds.status0 = 0;
			nes_fds.read_mode = nes_fds.write_reg = 0;
			break;
		case 21:
		case 25:
			IRQ_enable = IRQ_enable_latch = 0;
			IRQ_count = IRQ_count_latch = 0;
			/* fall through */
		case 22:
		case 23:
		case 33:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 32:
			mmc_cmd1 = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
//          set_nt_mirroring(PPU_MIRROR_HIGH);  // this would be needed for Major League
			break;
		case 24:
		case 26:
		case 73:
		case 75:
			IRQ_enable = IRQ_enable_latch = 0;
			IRQ_count = IRQ_count_latch = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 34:
			/* Can switch 32k prg banks */
			prg32(space->machine, 0);
			break;
		case 40:
			IRQ_enable = 0;
			IRQ_count = 0;
			/* Who's your daddy? */
			memcpy(&nes.rom[0x6000], &nes.rom[((nes.prg_chunks << 1) - 2) * 0x2000 + 0x10000], 0x2000);
			//ugly hack to deal with iNES header usage of chunk count.
			prg8_89(space->machine, (nes.prg_chunks << 1) - 4);
			prg8_ab(space->machine, (nes.prg_chunks << 1) - 3);
			prg8_cd(space->machine, 0);
			prg8_ef(space->machine, (nes.prg_chunks << 1) - 1);
			break;
		case 41:
			mmc_cmd1 = 0;
			prg32(space->machine, 0);
			break;
		case 42:
			prg32(space->machine, nes.prg_chunks - 1);
			break;
		case 43:
			prg32(space->machine, 0);
			memset(nes.wram, 0x2000, 0xff);
			break;
		case 44:
		case 47:
		case 49:
			IRQ_enable = 0;
			IRQ_count = IRQ_count_latch = 0;
			IRQ_reload = 0;
			MMC3_prg0 = 0xfe;
			MMC3_prg1 = 0xff;
			MMC3_cmd = 0;
			MMC3_prg_base = 0;
			MMC3_prg_mask = 0x0f;
			MMC3_chr_base = 0;
			MMC3_chr_mask = 0x7f;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine);
			mapper4_set_chr(space->machine, mmc_chr_source);
			break;
		case 45:
			IRQ_enable = 0;
			IRQ_count = IRQ_count_latch = 0;
			IRQ_reload = 0;
			MMC3_prg0 = 0xfe;
			MMC3_prg1 = 0xff;
			MMC3_cmd = 0;
			MMC3_prg_base = 0x30;
			MMC3_prg_mask = 0x0f;
			MMC3_chr_base = 0;
			MMC3_chr_mask = 0x7f;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper45_cmd = 0;
			mapper45_data[0] = mapper45_data[1] = mapper45_data[2] = mapper45_data[3] = 0;
			mapper4_set_prg(space->machine);
			mapper4_set_chr(space->machine, mmc_chr_source);
			memory_set_bankptr(space->machine, 5, nes.wram);
			break;
		case 46:
			mmc_bank_latch1 = 0;
			mmc_bank_latch2 = 0;
			prg32(space->machine, 0);
			chr8(space->machine, 0, CHRROM);
			break;
		case 51:
			MMC1_bank1 = 0x01;
			MMC1_bank2 = 0x00;
			MMC1_bank3 = 0x00;
			mapper51_set_banks(space->machine);
			break;
		case 57:
			MMC1_bank1 = 0x00;
			MMC1_bank2 = 0x00;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, 0);
			chr8(space->machine, 0, CHRROM);
			break;
		case 58:
			prg32(space->machine, 0);
			chr8(space->machine, 0, CHRROM);
			break;
		case 61:
		case 62:
		case 86:
			prg32(space->machine, 0);
			break;
		case 64:
			/* Can switch 3 8k prg banks */
			prg16_89ab(space->machine, nes.prg_chunks - 1);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 65:
			IRQ_enable = 0;
			IRQ_count = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 67:
		case 68:
		case 69:
		case 71:
		case 72:
		case 78:
		case 92:
			IRQ_enable = IRQ_enable_latch = 0;
			IRQ_count = IRQ_count_latch = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 70:
			prg16_89ab(space->machine, nes.prg_chunks - 2);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 76:
			prg8_89(space->machine, 0);
			prg8_ab(space->machine, 1);
			//cd is bankable, but this should init all banks just fine.
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			chr2_0(space->machine, 0, CHRROM);
			chr2_2(space->machine, 1, CHRROM);
			chr2_4(space->machine, 2, CHRROM);
			chr2_6(space->machine, 3, CHRROM);
			mmc_cmd1 = 0;
			break;
		case 77:
			prg32(space->machine, 0);
			chr2_2(space->machine, 0, CHRROM);
			chr2_4(space->machine, 1, CHRROM);
			chr2_6(space->machine, 2, CHRROM);
			break;
		case 79:
		case 146:
			/* Mirroring always horizontal...? */
//          Mirroring = 1;
			set_nt_mirroring(PPU_MIRROR_HORZ);
			chr8(space->machine, 0, CHRROM);
			prg32(space->machine, 0);
			break;
		case 80:
		case 207:
		case 82:
		case 85:
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
//      case 70:
		case 87:
		case 228:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 83:
			mapper83_data[9] = 0x0f;
			prg8_cd(space->machine, 0x1e);
			prg8_ef(space->machine, 0x1f);
			break;
		case 88:
			prg8_89(space->machine, 0xc);
			prg8_ab(space->machine, 0xd);
			prg8_cd(space->machine, 0xe);
			prg8_ef(space->machine, 0xf);
			break;
		case 89:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			set_nt_mirroring(PPU_MIRROR_LOW);
			break;
		case 91:
			set_nt_mirroring(PPU_MIRROR_VERT);
			prg16_89ab(space->machine, nes.prg_chunks - 1);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 93:
		case 94:
		case 95:
		case 101:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 97:
			prg16_89ab(space->machine, nes.prg_chunks - 1);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 112:
			mmc_cmd1 = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 36:
		case 38:
		case 39:
		case 66:
		case 107:
		case 113:
		case 177:
		case 179:
		case 184:
		case 140:
		case 144:
		case 240:
		case 241:
		case 242:
		case 244:
			prg32(space->machine, 0);
			break;
		case 104:
			mmc_bank_latch1 = 0;
			prg16_89ab(space->machine, 0x00);
			prg16_cdef(space->machine, 0x0f);
			break;
		case 132:
		case 172:
		case 173:
			txc_reg[0] = txc_reg[1] = txc_reg[2] = txc_reg[3] = 0;
			prg32(space->machine, 0);
			break;
		case 166:
			subor_reg[0] = subor_reg[1] = subor_reg[2] = subor_reg[3] = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, 0x07);
			break;
		case 167:
			subor_reg[0] = subor_reg[1] = subor_reg[2] = subor_reg[3] = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, 0x20);
			break;
		case 136:
			mmc_cmd1 = 0;
			prg32(space->machine, 0);
			break;
		case 137:
			mmc_cmd1 = 0;
			prg32(space->machine, 0);
			chr8(space->machine, nes.chr_chunks - 1, CHRROM);
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 138:
		case 139:
		case 141:
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mmc_cmd1 = 0;
			prg32(space->machine, 0);
			chr8(space->machine, 0, mmc_chr_source);
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 143:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, 1);
			chr8(space->machine, 0, CHRROM);
			break;
		case 133:
		case 145:
		case 147:
		case 148:
		case 149:
		case 171:
			prg32(space->machine, 0);
			chr8(space->machine, 0, CHRROM);
			break;
		case 150:
			mmc_bank_latch1 = 0;
			mmc_cmd1 = 0;
			prg32(space->machine, 0);
			chr8(space->machine, 0, CHRROM);
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 152:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			chr8(space->machine, 0, CHRROM);
			break;
		case 154:
			prg16_89ab(space->machine, nes.prg_chunks - 2);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 156:
			prg16_89ab(space->machine, nes.prg_chunks - 2);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			set_nt_mirroring(PPU_MIRROR_LOW);
			break;
		case 164:
			prg32(space->machine, 0xff);
			nes.mid_ram_enable = 1;
			break;
		case 176:
			prg32(space->machine, (nes.prg_chunks - 1) >> 1);
			break;
		case 178:
			mmc_cmd1 = 0;
			prg32(space->machine, 0);
			break;
		case 180:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, 0);
			break;
		case 182:
			IRQ_enable = 0;
			IRQ_count = 0;
			prg32(space->machine, (nes.prg_chunks - 1) >> 1);
			break;
		case 188:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, (nes.prg_chunks - 1) ^ 0x08);
			break;
		case 193:
			prg32(space->machine, (nes.prg_chunks - 1) >> 1);
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 200:
			prg16_89ab(space->machine, nes.prg_chunks - 1);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 201:
		case 213:
			prg32(space->machine, 0);
			chr8(space->machine, 0, CHRROM);
			break;
		case 202:
		case 203:
		case 204:
		case 214:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, 0);
			chr8(space->machine, 0, CHRROM);
			break;
		case 212:
			chr8(space->machine, 0xff, CHRROM);
			prg32(space->machine, 0xff);
			break;
		case 216:
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			prg32(space->machine, 0);
			chr8(space->machine, 0, mmc_chr_source);
			break;
		case 221:
			mmc_cmd1 = 0;
			mmc_cmd2 = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, 0);
			break;
		case 225:
			prg32(space->machine, 0);
			break;
		case 226:
			mmc_cmd1 = 0;
			mmc_cmd2 = 0;
			prg32(space->machine, 0);
			break;
		case 227:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, 0);
			break;
		case 229:
		case 255:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, 1);
			chr8(space->machine, 0, CHRROM);
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 230:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, 7);
			break;
		case 231:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, 0);
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 232:
			mmc_cmd1 = 0x18;
			mmc_cmd2 = 0x00;
			mapper232_set_prg(space->machine);
			break;
		case 243:
			mmc_bank_latch1 = 3;
			mmc_cmd1 = 0;
			chr8(space->machine, 3, CHRROM);
			prg32(space->machine, 0);
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 246:
			prg32(space->machine, 0xff);
			break;
		case 248:
			MMC1_bank1 = MMC1_bank2 = MMC1_bank3 = MMC1_bank4 = 0;
			prg32(space->machine, 0xff);
			break;
		default:
			/* Mapper not supported */
			err = 2;
			break;
	}
	return err;
}

/* VERY preliminary UNIF code - to be split afterwords! */

/*************************************************************

    unif_list

    Supported UNIF boards and corresponding handlers

*************************************************************/

static const unif unif_list[] =
{
/*       UNIF                       LOW_W, LOW_R, MED_W, HIGH_W, PPU_latch, scanline CB, hblank CB */
	{ "UNL-Sachen-8259A",         mapper141_l_w, NULL, mapper141_m_w, NULL, NULL, NULL, NULL },
};

const unif *nes_unif_lookup( const char *board )
{
	int i;
	for (i = 0; i < sizeof(unif_list) / sizeof(unif_list[0]); i++)
	{
		if (unif_list[i].board == board)
			return &unif_list[i];
	}
	return NULL;
}
