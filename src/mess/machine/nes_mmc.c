/*****************************************************************************************

    NES MMC Emulation


    MESS source file to handle various Multi-Memory Controllers (aka Mappers) used by NES carts.

    Many information about the mappers below come from the wonderful doc written by Disch.
    Current info (when used) are based on v0.6.1 of his docs.
    You can find the latest version of the doc at http://www.romhacking.net/docs/362/


    Missing components:
    * Currently, we are not emulating 4-screen mirroring (only 3 games use it)
    * We need a better way to implement CPU cycle based IRQ. Currently we update their counter once every scanline
      but, with this approach, every time 114 clocks have passed and for certain games it is too much!
    * We need to emulate additional sound hardware in mappers 019, 024, 026, 069, 085 (and possibly other ones)
    * We need to emulate EEPROM in mapper 016 and 159
    * Barcode World (mapper 069) and Datach games (mapper 157) need Barcode Reader emulation
      Notice that for mapper 157, the barcode reader enters in the irq, so glitches may be related to this!
    * Karaoke Studio (mapper 188) misses input devices

    Known issues on specific mappers:

    * 000 F1 Race requires more precise PPU timing. It currently has plenty of 1-line glitches.
    * 001 AD&D Hillsfar and Bill & Ted are broken. We need to ignore the case of writes happening on 2
          consecutive CPU cycles because these games use dirty tricks to reset counters (see note in mapper1_w)
    * 001 Yoshi flashes in-game. Zombie Hunter regressed. Rocket Ranger and Back to the Future have heavily corrupted graphics
    * 001, 155 We don't handle (yet) WRAM enable/disable bit
    * 002, 003, 094, 097, 152 Bus conflict?
    * 004 Mendel Palace has never worked properly
    * 005 has issues (see e.g. Just Breed or Metal Slader Glory), RAM banking needs hardware flags to determine size
    * 007 Marble Madness has small graphics corruptions
    * 014 in-game graphics is glitched
    * 015 Shanghai Tycoon has corrupted graphics
    * 025 TMNT & TMNT2 Jpn do not work
    * 033 has still some graphics problem (e.g. missing text in Akira)
    * 034 Impossible Mission II does not show graphics
    * 038 seems to miss inputs. separate reads?
    * 039 Study n Game 32 in 1 misses keyboard and starts differently than in NEStopia
    * 042 Ai Senshi Nicol has broken graphics (our Mapper 42 implementation does not handle CHR banks)
    * 051 only half of the games work
    * 064 has many IRQ problems (due to the way we implement CPU based IRQ) - see Skull & Crossbones.
          Klax has problems as well (even if it uses scanline based IRQ, according to Disch's docs).
    * 067 some 1-line glitches that cannot be solved without a better implementation for cycle-based IRQs
    * 071 Micro Machines has various small graphics problems. Fire Hawk is flashing all the times.
    * 072, 086, 092 lack samples support (maybe others as well)
    * 073 16 bit IRQ mode is not implemented
    * 077 Requires 4-screen mirroring. Currently, it is very glitchy
    * 083 has serious glitches
    * 088 Quinty has never worked properly
    * 096 is preliminary (no correct chr switch in connection to PPU)
    * 104 Not all the games work
    * 107 Are the scrolling glitches (check status bar) correct? NEStopia behaves similarly
    * 112 Master Shooter is not working and misses some graphics
    * 117 In-game glitches
    * 119 Pin Bot has glitches when the ball is in the upper half of the screen
    * 133 Qi Wang starts with corrupted graphics (ingame seems better)
    * 143 are Dancing Block borders (in the intro) correct?
    * 153 Famicom Jump II variant is not working, for some reason
    * 158 In addition to IRQ problems (same as 64), mirroring was just a guess (wrong?). info needed!
    * 164 preliminary - no sprites?
    * 176 has some graphics problem
    * 178 Fan Kong Jin Ying is not working (but not even in NEStopia)
    * 180 Crazy Climber controller?
    * 185 we emulate it like plain Mapper 3, but it's not: Mighty BombJack and Seicross don't work
    * 187, 198, 208, 215 have some PRG banking issues - preliminary!
    * 188 needs mic input (reads from 0x6000-0x7fff)
    * 197 Super Fighter 3 has some glitch in-game (maybe mirroring?)
    * 222 is only preliminar (wrong IRQ, mirroring or CHR banking?)
    * 225 115-in-1 has glitches in the menu (games seem fine)
    * 228 seems wrong. e.g. Action52 starts with a game rather than in the menu
    * 229 is preliminary
    * 230 not working yet (needs a value to flip at reset)
    * 232 has graphics glitches
    * 241 Commandos is not working (but not even in NEStopia)
    * 242 In Dragon Quest VIII graphics of the main character is not drawn (it seems similar to Shanghai Tycoon [map15]
          because in place of the missing graphics we get glitches in the left border)
    * 249 only half of the games work (and Du Bao Ying Hao seems to suffer the same problem as DQ8 and Shanghai Tycoon)
    * 252 shows logo, but doesn't reach the game
    * 255 does not really select game (same in NEStopia apparently)

    A few Mappers suffer of hardware conflict: original dumpers have used the same mapper number for more than
    a kind of boards. In these cases (and only in these cases) we exploit nes.hsi to set up accordingly
    emulation. Games which requires this hack are the following:
    * 032 - Major League needs hardwired mirroring (missing windows and glitched field in top view, otherwise)
    * 034 - Impossible Mission II is not like BxROM games (it writes to 0x7ffd-0x7fff, instead of 0x8000). It is still
          unplayable though (see above)
    * 071 - Fire Hawk is different from other Camerica games (no hardwired mirroring). Without crc_hack no helicopter graphics
    * 078 - Cosmo Carrier needs a different mirroring than Holy Diver
    * 113 - HES 6-in-1 requires mirroring (check Bookyman playfield), while other games break with this (check AV Soccer)
    * 153 - Famicom Jump II uses a different board (or the same in a very different way)
    * 242 - DQ8 has no mirroring (missing graphics is due to other reasons though)

    Details to investigate:
    * 034 writes to 0x8000-0xffff should not be used for NINA-001 and BNROM, only unlicensed BxROM...
    * 144 we ignore writes to 0x8000 while NEStopia does not. is it a problem?
    * 240 we do not map writes to 0x4020-0x40ff. is this a problem?
    * some other emus uses mapper 210 for mapper 019 games without additional sound hardware and mirroring capabilities
      (in 210 mirroring is hardwired). However, simply initializing mirroring to Vertical in 019 seems to fix all glitches
      in 210 games, so there seems to be no reason to have this duplicate mapper

    Some mappers copy PRG rom in SRAM region. Below is a list of those which does it (in MESS) to further
    investigate if we are supporting them in the right way (e.g. do any of the following have conflicts with SRAM?):
    * 065 (Gimmick! breaks badly without it)
    * 040, 042, 050

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
static chr_bank chr_map[8];  //quick banking structure, because some of this changes multiple times per scanline!
static name_table nt_page[4];  //quick banking structure for a maximum of 4K of RAM/ROM/ExRAM

static int IRQ_enable, IRQ_enable_latch;
static UINT16 IRQ_count, IRQ_count_latch;
static UINT8 IRQ_toggle;
static UINT8 IRQ_reset;
static UINT8 IRQ_status;
static UINT8 IRQ_mode;

int MMC1_extended;	/* 0 = normal MMC1 cart, 1 = 512k MMC1, 2 = 1024k MMC1 */

write8_space_func mmc_write_low;
read8_space_func mmc_read_low;
write8_space_func mmc_write_mid;
read8_space_func mmc_read_mid;
write8_space_func mmc_write;

static int mult1, mult2;

/* common local variables */
static UINT8 mmc_chr_source;		// This is set at init to CHRROM or CHRRAM. a few mappers can swap between
						// the two (this is done in the specific handlers).

static UINT8 mmc_cmd1, mmc_cmd2;	// These represent registers where the mapper writes important values
static UINT8 mmc_count;			// This is used as counter in mappers like 1 and 45

static int mmc_prg_base, mmc_prg_mask;	// MMC3 based multigame carts select a block of banks by using these (and then act like normal MMC3),
static int mmc_chr_base, mmc_chr_mask;	// while MMC3 and clones (mapper 118 & 119) simply set them as 0 and 0xff resp.

static UINT8 prg_bank[4];			// Many mappers writes only some bits of the selected bank (for both PRG and CHR),
static UINT8 vrom_bank[16];			// hence these are handy to latch bank values.


// FIXME: mapper specific variables should be unified as much as possible to make easier future implementation of save states
/* Local variables (a few more are defined just before handlers that use them...) */

static UINT8 MMC1_regs[4];

static UINT8 MMC2_regs[4];	// these replace bank0/bank0_hi/bank1/bank1_hi

static UINT8* extended_ntram;


static int MMC5_rom_bank_mode;
static int MMC5_vrom_bank_mode;
static int MMC5_vram_protect;
static int MMC5_scanline;
static int MMC5_floodtile;
static int MMC5_floodattr;
UINT8 MMC5_vram[0x400];
int MMC5_vram_control;

// these might be unified in single mmc_reg[] array, together with mmc_cmd1 & mmc_cmd2
// but be careful that MMC3 clones often use mmc_cmd1/mmc_cmd2 (from base MMC3) AND additional regs below!
static UINT8 mapper45_reg[4];
static UINT8 mapper51_reg[3];
static UINT8 mapper83_reg[10];
static UINT8 mapper83_low_reg[4];
static UINT8 mapper95_reg[4];
static UINT8 mapper115_reg[4];
static UINT8 txc_reg[4]; 	// used by mappers 132, 172 & 173
static UINT8 subor_reg[4];	// used by mappers 166 & 167
static UINT8 sachen_reg[8];	// used by mappers 137, 138, 139 & 141
static UINT8 mapper12_reg;
static UINT8 map52_reg_written;
static UINT8 map114_reg, map114_reg_enabled;
static UINT8 map215_reg[4];
static UINT8 map217_reg[4];
static UINT8 map249_reg;
static UINT8 map14_reg[2];
static UINT8 mapper121_reg[3];
static UINT8 mapper187_reg[4];
static UINT8 map208_reg[5];


static UINT8 extra_bank[16];	// some MMC3 clone have 2 series of PRG/CHR banks...
					// we collect them all here: first 4 elements PRG banks, then 6/8 CHR banks

// mapper 68 needs these for NT mirroring!
static int m68_mirror;
static int m0, m1;


// FDS / mapper 20 need these for disks (to be added to fds struct?)
static int fds_last_side;
static int fds_count;

// Famicom Jump II is identified as Mapper 153, but it works in a completely different way.
// in particular, it requires these (not needed by other Mapper 153 games)
static UINT8 map153_reg[8];
static UINT8 map153_bank_latch;

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
    care separately in init_nes_core

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
#ifdef MAME_DEBUG
		if (!mapper_warning)
		{
			logerror("This game is writing to the mapper area but no mapper is set. You may get better results by switching to a valid mapper.\n");
			mapper_warning = 1;
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

/* We define an additional helper to map PRG-ROM to 0x6000-0x7000 */
// TODO: are we implementing this correctly in the mappers which uses it? check!

static void prg8_67( running_machine *machine, int bank )
{
	/* assumes that bank references an 8k chunk */
	bank &= ((nes.prg_chunks << 1) - 1);
	memory_set_bankptr(machine, 5, &nes.rom[bank * 0x2000 + 0x10000]);
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

    Support for .nes Files

*************************************************************/

/* Include emulation of iNES Mappers for .nes files */
#include "machine/nes_ines.c"

/*************************************************************

    mapper_initialize

    Initialize various constants needed by mappers, depending
    on the mapper number. As long as the code is mainly iNES
    centric, also UNIF images will call this function to
    initialize their bankswitch and registers.

*************************************************************/

static int mapper_initialize( running_machine *machine, int mmc_num )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int err = 0, i;

	switch (mmc_num)
	{
		case 0:
			err = 1; /* No mapper found */
			prg32(space->machine, 0);
			break;
		case 1:
		case 155:
			mmc_cmd1 = 0;
			mmc_count = 0;
			MMC1_regs[0] = 0x0f;
			MMC1_regs[1] = MMC1_regs[2] = MMC1_regs[3] = 0;
			set_nt_mirroring(PPU_MIRROR_HORZ);
			MMC1_set_chr(space->machine);
			MMC1_set_prg(space->machine);
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
		case 185:
			prg32(space->machine, 0);
			break;
		case 4:
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
		case 12:
		case 74:
		case 118:
		case 119:
		case 191:
		case 192:
		case 194:
		case 195:
		case 196:
		case 245:
		case 250:
			prg_bank[0] = prg_bank[2] = 0xfe;	// prg_bank[2] & prg_bank[3] remain always the same in most MMC3 variants
			prg_bank[1] = prg_bank[3] = 0xff;	// but some pirate clone mappers change them after writing certain registers
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = mmc_chr_mask = 0xff;	// these could be init'ed as xxx_chunks-1 and they would work the same
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 5:
			/* Can switch 8k prg banks, but they are saved as 16k in size */
			MMC5_rom_bank_mode = 3;
			MMC5_vrom_bank_mode = 0;
			MMC5_vram_protect = 0;
			nes.mid_ram_enable = 0;
			prg16_89ab(space->machine, nes.prg_chunks - 2);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 6:
			mmc_cmd1 = 0;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, 7);
			break;
		case 7:
			/* Bankswitches 32k at a time */
			set_nt_mirroring(PPU_MIRROR_LOW);
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 8:
		case 96:
			prg32(space->machine, 0);
			break;
		case 9:
			MMC2_regs[0] = MMC2_regs[2] = 0;
			MMC2_regs[1] = MMC2_regs[3] = 0;
			mmc_cmd1 = mmc_cmd2 = 0xfe;
			prg8_89(space->machine, 0);
			//ugly hack to deal with iNES header usage of chunk count.
			prg8_ab(space->machine, (nes.prg_chunks << 1) - 3);
			prg8_cd(space->machine, (nes.prg_chunks << 1) - 2);
			prg8_ef(space->machine, (nes.prg_chunks << 1) - 1);
			break;
		case 10:
			/* Reset VROM latches */
			MMC2_regs[0] = MMC2_regs[2] = 0;
			MMC2_regs[1] = MMC2_regs[3] = 0;
			mmc_cmd1 = mmc_cmd2 = 0xfe;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 11:
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;	// NINA-07 has no CHRROM
			prg32(space->machine, 0);
			break;
		case 14:
			extra_bank[2] = 0xfe;
			extra_bank[3] = 0xff;
			extra_bank[0] = extra_bank[1] = extra_bank[4] = extra_bank[5] = extra_bank[6] = 0;
			extra_bank[7] = extra_bank[8] = extra_bank[9] = extra_bank[0xa] = extra_bank[0xb] = 0;
			map14_reg[0] = map14_reg[1] = 0;
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 15:
			set_nt_mirroring(PPU_MIRROR_VERT);
			prg32(space->machine, 0);
			break;
		case 16:
		case 17:
		case 18:
		case 157:
		case 159:
			mmc_cmd1 = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			break;
		case 19:
			mmc_cmd1 = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 20:
			fds_last_side = 0;
			fds_count = 0;
			nes_fds.motor_on = 0;
			nes_fds.door_closed = 0;
			nes_fds.current_side = 1;
			nes_fds.head_position = 0;
			nes_fds.status0 = 0;
			nes_fds.read_mode = nes_fds.write_reg = 0;
			break;
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 33:
		case 48:
		case 73:
		case 75:
		case 80:
		case 82:
		case 85:
		case 87:
		case 117:
		case 207:
		case 222:
		case 252:
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 32:
			mmc_cmd1 = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			if (nes.crc_hack)
				set_nt_mirroring(PPU_MIRROR_HIGH);  // needed by Major League
			break;
		case 34:
			prg32(space->machine, 0);
			break;
		case 37:
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = 0x07;
			mmc_chr_mask = 0x7f;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 40:
			prg8_67(space->machine, 0xfe);
			prg8_89(space->machine, 0xfc);
			prg8_ab(space->machine, 0xfd);
			prg8_cd(space->machine, 0xfe);
			prg8_ef(space->machine, 0xff);
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
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = 0x0f;
			mmc_chr_mask = 0x7f;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 45:
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper45_reg[0] = mapper45_reg[1] = mapper45_reg[2] = mapper45_reg[3] = 0;
			mmc_prg_base = 0x30;
			mmc_prg_mask = 0x3f;
			mmc_chr_base = 0;
			mmc_chr_mask = 0x7f;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			memory_set_bankptr(space->machine, 5, nes.wram);
			break;
		case 46:
			prg32(space->machine, 0);
			chr8(space->machine, 0, CHRROM);
			break;
		case 50:
			prg8_67(space->machine, 0x0f);
			prg8_89(space->machine, 0x08);
			prg8_ab(space->machine, 0x09);
			prg8_cd(space->machine, 0);
			prg8_ef(space->machine, 0x0b);
			break;
		case 51:
			mapper51_reg[0] = 0x01;
			mapper51_reg[1] = 0x00;
			mapper51_set_banks(space->machine);
			break;
		case 52:
			prg_bank[0] = 0xfe;
			prg_bank[2] = 0xfe;
			prg_bank[1] = 0xff;
			prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			map52_reg_written = 0;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mmc_prg_base = 0;
			mmc_prg_mask = 0x1f;
			mmc_chr_base = 0;
			mmc_chr_mask = 0xff;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			memory_set_bankptr(space->machine, 5, nes.wram);
			break;
		case 57:
			mmc_cmd1 = 0x00;
			mmc_cmd2 = 0x00;
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
		case 158:
			mmc_cmd1 = 0;
			prg16_89ab(space->machine, nes.prg_chunks - 1);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 65:
		case 67:
		case 69:
		case 72:
		case 78:
		case 92:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 68:
			m68_mirror = m0 = m1 = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 70:
			prg16_89ab(space->machine, nes.prg_chunks - 2);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			break;
		case 71:
			set_nt_mirroring(PPU_MIRROR_HORZ);
			prg16_89ab(space->machine, 0);
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
		case 228:
			chr8(space->machine, 0, CHRROM);
			prg32(space->machine, 0);
			break;
		case 83:
			mapper83_reg[9] = 0x0f;
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
			prg16_89ab(space->machine, 0x00);
			prg16_cdef(space->machine, 0x0f);
			break;
		case 106:
			prg8_89(space->machine, (nes.prg_chunks << 1) - 1);
			prg8_ab(space->machine, 0);
			prg8_cd(space->machine, 0);
			prg8_ef(space->machine, (nes.prg_chunks << 1) - 1);
			break;
		case 114:
			map114_reg = map114_reg_enabled = 0;
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 115:
			mapper115_reg[0] = 0;
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 121:
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mapper121_reg[0] = mapper121_reg[1] = mapper121_reg[2] = 0;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 132:
		case 172:
		case 173:
			txc_reg[0] = txc_reg[1] = txc_reg[2] = txc_reg[3] = 0;
			prg32(space->machine, 0);
			break;
		case 134:
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = 0x1f;
			mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
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
			mmc_cmd1 = 0;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
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
		case 153:
			mmc_cmd1 = 0;
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			// Famicom Jump II needs also the following!
			for (i = 0; i < 8; i++)
				map153_reg[i] = 0;

			map153_bank_latch = 0;
			if (nes.crc_hack)
				mapper153_set_prg(space->machine);
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
			prg32(space->machine, (nes.prg_chunks - 1) >> 1);
			break;
		case 188:
			prg16_89ab(space->machine, 0);
			prg16_cdef(space->machine, (nes.prg_chunks - 1) ^ 0x08);
			break;
		case 187:
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mapper187_reg[0] = mapper187_reg[1] = mapper187_reg[2] = mapper187_reg[3] = 0;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper187_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper187_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 189:
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_chr_base = 0;
			mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			prg32(space->machine, 0);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 193:
			prg32(space->machine, (nes.prg_chunks - 1) >> 1);
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 197:
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper197_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 198:
			prg_bank[0] = 0x00;
			prg_bank[1] = 0x01;
			prg_bank[2] = 0x4e;
			prg_bank[3] = 0x4f;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper198_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 199:
			prg_bank[0] = 0x00;
			prg_bank[1] = 0x01;
			prg_bank[2] = 0x3e;
			prg_bank[3] = 0x3f;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper199_set_chr(space->machine, mmc_chr_base, mmc_chr_mask);
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
		case 205:
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = 0x10;
			mmc_prg_mask = 0x1f;
			mmc_chr_base = 0;
			mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 208:
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			map208_reg[0] = map208_reg[1] = map208_reg[2] = map208_reg[3] = map208_reg[4] = 0;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper4_set_chr(space->machine, mmc_chr_source, mmc_chr_base, mmc_chr_mask);
			break;
		case 212:
			chr8(space->machine, 0xff, CHRROM);
			prg32(space->machine, 0xff);
			break;
		case 215:
			prg_bank[0] = 0x00;
			prg_bank[2] = 0xfe;
			prg_bank[1] = 0x01;
			prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			map215_reg[0] = 0x00;
			map215_reg[1] = 0xff;
			map215_reg[2] = 0x04;
			map215_reg[3] = 0;
			mmc_prg_base = 0;
			mmc_prg_mask = 0x1f;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper215_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper215_set_chr(space->machine, mmc_chr_source);
			break;
		case 216:
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			prg32(space->machine, 0);
			chr8(space->machine, 0, mmc_chr_source);
			break;
		case 217:
			map217_reg[0] = 0x00;
			map217_reg[1] = 0xff;
			map217_reg[2] = 0x03;
			map217_reg[3] = 0;
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper217_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper217_set_chr(space->machine, mmc_chr_source);
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
			prg16_cdef(space->machine, nes.prg_chunks - 1);
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 232:
			mmc_cmd1 = 0x18;
			mmc_cmd2 = 0x00;
			mapper232_set_prg(space->machine);
			break;
		case 243:
			vrom_bank[0] = 3;
			mmc_cmd1 = 0;
			chr8(space->machine, 3, CHRROM);
			prg32(space->machine, 0);
			set_nt_mirroring(PPU_MIRROR_VERT);
			break;
		case 246:
			prg32(space->machine, 0xff);
			break;
		case 249:
			prg_bank[0] = prg_bank[2] = 0xfe;
			prg_bank[1] = prg_bank[3] = 0xff;
			mmc_cmd1 = 0;
			mmc_cmd2 = 0x80;
			map249_reg = 0;
			mmc_prg_base = mmc_chr_base = 0;
			mmc_prg_mask = mmc_chr_mask = 0xff;
			mmc_chr_source = nes.chr_chunks ? CHRROM : CHRRAM;
			mapper249_set_prg(space->machine, mmc_prg_base, mmc_prg_mask);
			mapper249_set_chr(space->machine, mmc_chr_base, mmc_chr_mask);
			break;
		default:
			/* Mapper not supported */
			err = 2;
			break;
	}

	return err;
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
	int err = 0, i;
	const mmc *mapper;

	if (nes.chr_chunks == 0)
		chr8(machine, 0,CHRRAM);
	else
		chr8(machine, 0,CHRROM);

	/* Set the mapper irq callback */
	mapper = nes_mapper_lookup(mmc_num);

	if (mapper == NULL)
		fatalerror("Unimplemented Mapper %d", mmc_num);
//      logerror("Mapper %d is not yet supported, defaulting to no mapper.\n", mmc_num);    // this one would be a better output


	ppu2c0x_set_scanline_callback(state->ppu, mapper ? mapper->mmc_scanline : NULL);
	ppu2c0x_set_hblank_callback(state->ppu, mapper ? mapper->mmc_hblank : NULL);

	if (!nes_irq_timer)
		nes_irq_timer = timer_alloc(machine, nes_irq_callback, NULL);

	mapper_warning = 0;

	MMC5_vram_control = 0;

	/* Point the WRAM/battery area to the first RAM bank */
	if (mmc_num != 20)
		memory_set_bankptr(space->machine, 5, &nes.wram[0x0000]);

	/* Here, we init a few helpers: 4 prg banks and 16 chr banks - some mappers use them */
	for (i = 0; i < 4; i++)
		prg_bank[i] = 0;
	for (i = 0; i < 16; i++)
		vrom_bank[i] = 0;
	for (i = 0; i < 16; i++)
		extra_bank[i] = 0;

	/* Finally, we init IRQ-related quantities. */
	IRQ_enable = IRQ_enable_latch = 0;
	IRQ_count = IRQ_count_latch = 0;
	IRQ_toggle = 0;

	err = mapper_initialize(machine, mmc_num);

	return err;
}

/*************************************************************

    Support for .unf Files

*************************************************************/

/* Include emulation of UNIF Boards for .unf files */
#include "machine/nes_unif.c"

// WIP code
int unif_reset( running_machine *machine, const char *board )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	nes_state *state = machine->driver_data;
	int err = 0;
	const unif *unif_board;

	if (nes.chr_chunks == 0)
		chr8(machine, 0,CHRRAM);
	else
		chr8(machine, 0,CHRROM);

	/* Set the mapper irq callback */
	unif_board = nes_unif_lookup(board);

	if (unif_board == NULL)
		fatalerror("Unimplemented UNIF Board");

	ppu2c0x_set_scanline_callback(state->ppu, unif_board ? unif_board->mmc_scanline : NULL);
	ppu2c0x_set_hblank_callback(state->ppu, unif_board ? unif_board->mmc_hblank : NULL);

	if (!nes_irq_timer)
		nes_irq_timer = timer_alloc(machine, nes_irq_callback, NULL);

	/* Point the WRAM/battery area to the first RAM bank */
	memory_set_bankptr(space->machine, 5, &nes.wram[0x0000]);

	switch (unif_board->board_idx)
	{
		case STD_NROM:
		case HVC_FAMBASIC:
		case JALECO_JF01:
		case JALECO_JF02:
		case JALECO_JF03:
		case JALECO_JF04:
			mapper_initialize(machine, 0);
			break;
		case STD_SAROM:
		case STD_SBROM:
		case STD_SCROM:
		case STD_SEROM:
		case STD_SFROM:
		case STD_SGROM:
		case STD_SHROM:
		case STD_SJROM:
		case STD_SKROM:
		case STD_SLROM:
		case STD_SNROM:
		case STD_SOROM:
		case STD_SUROM:
		case STD_SXROM:
			mapper_initialize(machine, 1);
			break;
		case STD_UNROM:
		case STD_UOROM:
		case JALECO_JF15:
		case JALECO_JF18:
		case JALECO_JF39:
			mapper_initialize(machine, 2);
			break;
		case AVE_74161:
		case BANDAI_PT554:
		case STD_CNROM:
		case NTDEC_N715062:
		case TENGEN_800008:
			mapper_initialize(machine, 3);
			break;
		case STD_HKROM:
		case STD_TEROM:
		case STD_TBROM:
		case STD_TFROM:
		case STD_TGROM:
		case STD_TKROM:
		case STD_TLROM:
		case STD_TNROM:
		case STD_TR1ROM:
		case STD_TSROM:
		case STD_TVROM:
			mapper_initialize(machine, 4);
			break;
		case STD_EKROM:
		case STD_ELROM:
		case STD_ETROM:
		case STD_EWROM:
			mapper_initialize(machine, 5);
			break;
		case STD_AMROM:
		case STD_ANROM:
		case STD_AN1ROM:
		case STD_AOROM:
			mapper_initialize(machine, 7);
			break;
		case STD_FJROM:
		case STD_FKROM:
			mapper_initialize(machine, 10);
			break;
		case AGCI_47516:
		case AVE_NINA07:
		case COLORDREAMS:
			mapper_initialize(machine, 11);
			break;
		case REXSOFT_SL1632:
			mapper_initialize(machine, 14);
			break;
		case BANDAI_LZ93D50_A:
		case BANDAI_LZ93D50_B:
			mapper_initialize(machine, 16);
			break;
		case JALECO_JF23:
		case JALECO_JF24:
		case JALECO_JF25:
		case JALECO_JF27:
		case JALECO_JF29:
		case JALECO_JF30:
		case JALECO_JF31:
		case JALECO_JF32:
		case JALECO_JF33:
		case JALECO_JF34:
		case JALECO_JF35:
		case JALECO_JF36:
		case JALECO_JF37:
		case JALECO_JF38:
		case JALECO_JF40:
		case JALECO_JF41:
			mapper_initialize(machine, 18);
			break;
		case NAMCOT_163:
			mapper_initialize(machine, 19);
			break;
		case KONAMI_VRC4:
			mapper_initialize(machine, 21);
			break;
		case KONAMI_VRC2:
			mapper_initialize(machine, 22);
			break;
		case KONAMI_VRC6:
			mapper_initialize(machine, 24);
			break;
		case IREM_G101:
		case IREM_G101A:
		case IREM_G101B:
			mapper_initialize(machine, 32);
			break;
		case TAITO_TC0190FMC:
			mapper_initialize(machine, 33);
			break;
		case STD_BNROM:
		case AVE_NINA01:
		case AVE_NINA02:
			mapper_initialize(machine, 34);
			break;
		case NES_ZZ:
			mapper_initialize(machine, 37);
			break;
		case CALTRON_6IN1:
			mapper_initialize(machine, 41);
			break;
		case UNL_SMB2J:
			mapper_initialize(machine, 43);
			break;
		case NES_QJ:
			mapper_initialize(machine, 47);
			break;
		case TAITO_TC0190FMCP:
			mapper_initialize(machine, 48);
			break;
		case TENGEN_800032:
			mapper_initialize(machine, 64);
			break;
		case STD_GNROM:
		case STD_MHROM:
			mapper_initialize(machine, 66);
			break;
		case SUNSOFT_3:
			mapper_initialize(machine, 67);
			break;
		case STD_NTBROM:
		case SUNSOFT_4:
		case TENGEN_800042:
			mapper_initialize(machine, 68);
			break;
		case STD_JLROM:
		case STD_JSROM:
		case NES_BTR:
		case SUNSOFT_5B:
		case SUNSOFT_FME7:
			mapper_initialize(machine, 69);
			break;
		case BANDAI_74161:
		case TAITO_74161:
			mapper_initialize(machine, 70);
			break;
		case CAMERICA_ALGN:
		case CAMERICA_9093:
			mapper_initialize(machine, 71);
			break;
		case CAMERICA_9097:
			nes.crc_hack = 0xf0;
			mapper_initialize(machine, 71);
			break;
		case JALECO_JF17:
		case JALECO_JF26:
		case JALECO_JF28:
			mapper_initialize(machine, 72);
			break;
		case KONAMI_VRC3:
			mapper_initialize(machine, 73);
			break;
		case KONAMI_VRC1:
		case JALECO_JF20:
		case JALECO_JF22:
			mapper_initialize(machine, 75);
			break;
		case NAMCOT_3446:
			mapper_initialize(machine, 76);
			break;
		case IREM_74161:
			mapper_initialize(machine, 77);
			break;
		case IREM_HOLYDIV:
		case JALECO_JF16:
			mapper_initialize(machine, 78);
			break;
		case AVE_NINA03:
		case AVE_NINA06:
		case AVE_MB91:
			mapper_initialize(machine, 79);
			break;
		case TAITO_X1005:
			mapper_initialize(machine, 80);
			break;
		case TAITO_X1017:
			mapper_initialize(machine, 82);
			break;
		case KONAMI_VRC7:
			mapper_initialize(machine, 85);
			break;
		case JALECO_JF13:
			mapper_initialize(machine, 86);
			break;
		case TAITO_74139:
		case KONAMI_74139:
		case JALECO_JF05:
		case JALECO_JF06:
		case JALECO_JF07:
		case JALECO_JF08:
		case JALECO_JF09:
		case JALECO_JF10:
			mapper_initialize(machine, 87);
			break;
		case NAMCOT_3443:
		case NAMCOT_3433:
			mapper_initialize(machine, 88);
			break;
		case SUNSOFT_2B:
			mapper_initialize(machine, 89);
			break;
		case JALECO_JF19:
		case JALECO_JF21:
			mapper_initialize(machine, 92);
			break;
		case STD_UN1ROM:
			mapper_initialize(machine, 94);
			break;
		case NAMCOT_3425:
			mapper_initialize(machine, 95);
			break;
		case STD_TKSROM:
		case STD_TLSROM:
			mapper_initialize(machine, 118);
			break;
		case STD_TQROM:
			mapper_initialize(machine, 119);
			break;
		case TXC_22211A:
			mapper_initialize(machine, 132);
			break;
		case SACHEN_SA72008:
			mapper_initialize(machine, 133);
			break;
		case SACHEN_TCU02:
			mapper_initialize(machine, 136);
			break;
		case SACHEN_8259D:
			mapper_initialize(machine, 137);
			break;
		case SACHEN_8259B:
			mapper_initialize(machine, 138);
			break;
		case SACHEN_8259C:
			mapper_initialize(machine, 139);
			break;
		case JALECO_JF11:
		case JALECO_JF12:
		case JALECO_JF14:
			mapper_initialize(machine, 140);
			break;
		case SACHEN_8259A:
			mapper_initialize(machine, 141);
			break;
		case SACHEN_TCA01:
			mapper_initialize(machine, 143);
			break;
		case AGCI_50282:
			mapper_initialize(machine, 144);
			break;
		case SACHEN_SA72007:
			mapper_initialize(machine, 145);
			break;
		case SACHEN_SA0161M:
			mapper_initialize(machine, 146);
			break;
		case SACHEN_TCU01:
			mapper_initialize(machine, 147);
			break;
		case SACHEN_SA0037:
			mapper_initialize(machine, 148);
			break;
		case SACHEN_SA0036:
			mapper_initialize(machine, 149);
			break;
		case SACHEN_74LS374:
			mapper_initialize(machine, 150);
			break;
		case BANDAI_FCG1:
		case BANDAI_FCG2:
		case BANDAI_JUMP2:
			mapper_initialize(machine, 153);
			break;
		case TENGEN_800037:
			mapper_initialize(machine, 158);
			break;
		case SUNSOFT_1:
			mapper_initialize(machine, 184);
			break;
		case STD_DEROM:
		case STD_DE1ROM:
		case STD_DRROM:
		case TENGEN_800002:
		case TENGEN_800004: 
		case TENGEN_800030:
			mapper_initialize(machine, 206);
			break;
		case UNL_N625092:
			mapper_initialize(machine, 221);
			break;
		case ACTENT_ACT52:
			mapper_initialize(machine, 228);
			break;
		case CAMERICA_ALGQ:
		case CAMERICA_9096:
			mapper_initialize(machine, 232);
			break;

		default:
			/* Mapper not supported */
			err = 2;
			break;
	}
	return err;
}
