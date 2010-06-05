/*****************************************************************************************

    NES MMC Emulation

    Support for iNES Mappers

****************************************************************************************/

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

static void MMC1_set_prg( running_machine *machine )
{
	UINT8 prg_mode, prg_offset;

	prg_mode = MMC1_regs[0] & 0x0c;
	/* prg_mode&0x8 determines bank size: 32k (if 0) or 16k (if 1)? when in 16k mode,
       prg_mode&0x4 determines which half of the PRG space we can swap: if it is 4,
       MMC1_regs[3] sets banks at 0x8000; if it is 0, MMC1_regs[3] sets banks at 0xc000. */

	prg_offset = MMC1_regs[1] & 0x10;
	/* In principle, MMC1_regs[2]&0x10 might affect "extended" banks as well, when chr_mode=1.
       However, quoting Disch's docs: When in 4k CHR mode, 0x10 in both $A000 and $C000 *must* be
       set to the same value, or else pages will constantly be swapped as graphics render!
       Hence, we use only MMC1_regs[1]&0x10 for prg_offset */

	switch (prg_mode)
	{
	case 0x00:
	case 0x04:
		prg32(machine, prg_offset + MMC1_regs[3]);
		break;
	case 0x08:
		prg16_89ab(machine, prg_offset + 0);
		prg16_cdef(machine, prg_offset + MMC1_regs[3]);
		break;
	case 0x0c:
		prg16_89ab(machine, prg_offset + MMC1_regs[3]);
		prg16_cdef(machine, prg_offset + 0x0f);
		break;
	}
}

static void MMC1_set_chr( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_mode = MMC1_regs[0] & 0x10;

	if (chr_mode)
	{
		chr4_0(machine, MMC1_regs[1] & 0x1f, state->mmc_chr_source);
		chr4_4(machine, MMC1_regs[2] & 0x1f, state->mmc_chr_source);
	}
	else
		chr8(machine, (MMC1_regs[1] & 0x1f) >> 1, state->mmc_chr_source);
}

static WRITE8_HANDLER( mapper1_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	/* Note that there is only one latch and shift counter, shared amongst the 4 regs */
	/* Space Shuttle will not work if they have independent variables. */
	LOG_MMC(("mapper1_w offset: %04x, data: %02x\n", offset, data));


	/* here we would need to add an if(cpu_cycles_passed>1) test, and
       if requirement is not met simply return without writing anything.
       AD&D Hillsfar and Bill & Ted rely on this behavior!! */
	if (data & 0x80)
	{
		state->mmc_count = 0;
		state->mmc_cmd1 = 0;

		/* Set reg at 0x8000 to size 16k and lower half swap - needed for Robocop 3, Dynowars */
		MMC1_regs[0] |= 0x0c;
		MMC1_set_prg(space->machine);

		LOG_MMC(("=== MMC1 regs reset to default\n"));
		return;
	}

	if (state->mmc_count < 5)
	{
		if (state->mmc_count == 0) state->mmc_cmd1 = 0;
		state->mmc_cmd1 >>= 1;
		state->mmc_cmd1 |= (data & 0x01) ? 0x10 : 0x00;
		state->mmc_count++;
	}

	if (state->mmc_count == 5)
	{
		switch (offset & 0x6000)	/* Which reg shall we write to? */
		{
		case 0x0000:
			MMC1_regs[0] = state->mmc_cmd1;

			switch (MMC1_regs[0] & 0x03)
			{
				case 0: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
				case 1: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
				case 2: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
				case 3: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
			}
			MMC1_set_chr(space->machine);
			MMC1_set_prg(space->machine);
			break;
		case 0x2000:
			MMC1_regs[1] = state->mmc_cmd1;
			MMC1_set_chr(space->machine);
			MMC1_set_prg(space->machine);
			break;
		case 0x4000:
			MMC1_regs[2] = state->mmc_cmd1;
			MMC1_set_chr(space->machine);
			break;
		case 0x6000:
			MMC1_regs[3] = state->mmc_cmd1;
			MMC1_set_prg(space->machine);
			break;
		}

		state->mmc_count = 0;
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
	LOG_MMC(("mapper2_w offset: %04x, data: %02x\n", offset, data));

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
	LOG_MMC(("mapper3_w offset: %04x, data: %02x\n", offset, data));

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

/* Many multigame carts based on MMC3 works as follows: they select a
  block of banks by writing in the 0x6000-0x7fff range and then they
  operate like a common MMC3 board on that specific block. To emulate this
  we use the concept of bases and masks. Original MMC3 boards simply
  set base = 0 and mask = 0xff at start and never change them */
/* Notice that, atm, there is no way to directly handle MMC3 clone mappers
  which use custom routines to set PRG/CHR. For these mappers we have to
  re-define the 0x8000-0xffff handler at least for offsets 0 & 1 (i.e. the
  offsets where are used set_prg/set_chr) by using the mapper-specific routines,
  and then we can fall back to mapper4_w for the remaining offsets */

static void mapper4_set_prg( running_machine *machine, int prg_base, int prg_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 prg_flip = (state->mmc_cmd1 & 0x40) ? 2 : 0;

	prg8_89(machine, prg_base | (state->mmc_prg_bank[0 ^ prg_flip] & prg_mask));
	prg8_ab(machine, prg_base | (state->mmc_prg_bank[1] & prg_mask));
	prg8_cd(machine, prg_base | (state->mmc_prg_bank[2 ^ prg_flip] & prg_mask));
	prg8_ef(machine, prg_base | (state->mmc_prg_bank[3] & prg_mask));
}

static void mapper4_set_chr( running_machine *machine, UINT8 chr, int chr_base, int chr_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;

	chr1_x(machine, chr_page ^ 0, chr_base | ((state->mmc_vrom_bank[0] & ~0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 1, chr_base | ((state->mmc_vrom_bank[0] |  0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 2, chr_base | ((state->mmc_vrom_bank[1] & ~0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 3, chr_base | ((state->mmc_vrom_bank[1] |  0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 4, chr_base | (state->mmc_vrom_bank[2] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 5, chr_base | (state->mmc_vrom_bank[3] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 6, chr_base | (state->mmc_vrom_bank[4] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 7, chr_base | (state->mmc_vrom_bank[5] & chr_mask), chr);
}

/* Here, IRQ counter decrements every scanline. */
static void mapper4_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;

	if (scanline < PPU_BOTTOM_VISIBLE_SCANLINE)
	{
		int priorCount = state->IRQ_count;
		if ((state->IRQ_count == 0))
		{
			state->IRQ_count = state->IRQ_count_latch;
		}
		else
			state->IRQ_count--;

		if (state->IRQ_enable && !blanked && (state->IRQ_count == 0) && priorCount)
		{
			LOG_MMC(("irq fired, scanline: %d (MAME %d, beam pos: %d)\n", scanline,
					video_screen_get_vpos(device->machine->primary_screen), video_screen_get_hpos(device->machine->primary_screen)));
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
		}
	}
}

static WRITE8_HANDLER( mapper4_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;

	LOG_MMC(("mapper4_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
		case 0x0000:
			MMC3_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (MMC3_helper & 0x40)
				mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

			/* Has CHR Mode changed? */
			if (MMC3_helper & 0x80)
				mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;

		case 0x0001:
			cmd = state->mmc_cmd1 & 0x07;
			switch (cmd)
			{
				case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
				case 2: case 3: case 4: case 5:
					state->mmc_vrom_bank[cmd] = data;
					mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
					break;
				case 6:
				case 7:
					state->mmc_prg_bank[cmd - 6] = data;
					mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
					break;
			}
			break;

		case 0x2000:
			set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;

		case 0x2001: /* extra RAM enable/disable */
			state->mmc_cmd2 = data;	/* This actually is made of two parts: data&0x80 = WRAM enabled and data&0x40 = WRAM readonly!  */
						/* We save this twice because we will need state->mmc_cmd2 in some clone mapper */
			state->mid_ram_enable = data;
			break;

		case 0x4000:
			state->IRQ_count_latch = data;
			break;

		case 0x4001: /* some sources state that here we must have state->IRQ_count = state->IRQ_count_latch */
			state->IRQ_count = 0;
			break;

		case 0x6000:
			state->IRQ_enable = 0;
			break;

		case 0x6001:
			state->IRQ_enable = 1;
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

static void mapper5_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;

#if 1
	if (scanline == 0)
		state->IRQ_status |= 0x40;
	else if (scanline > PPU_BOTTOM_VISIBLE_SCANLINE)
		state->IRQ_status &= ~0x40;
#endif

	if (scanline == state->IRQ_count)
	{
		if (state->IRQ_enable)
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);

		state->IRQ_status = 0xff;
	}
}

static void ppu_mirror_MMC5( running_machine *machine, int page, int src )
{
	switch (src)
	{
	case 0:	/* CIRAM0 */
		set_nt_page(machine, page, CIRAM, 0, 1);
		break;
	case 1:	/* CIRAM1 */
		set_nt_page(machine, page, CIRAM, 1, 1);
		break;
	case 2:	/* ExRAM */
		set_nt_page(machine, page, EXRAM, 0, 1);	// actually only works during rendering.
		break;
	case 3: /* Fill Registers */
		set_nt_page(machine, page, MMC5FILL, 0, 0);
		break;
	default:
		fatalerror("This should never happen");
		break;
	}
}

static READ8_HANDLER( mapper5_l_r )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	int retVal;

	/* $5c00 - $5fff: extended videoram attributes */
	if ((offset >= 0x1b00) && (offset <= 0x1eff))
	{
		return state->mmc5_vram[offset - 0x1b00];
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
			retVal = state->IRQ_status;
			state->IRQ_status &= ~0x80;
			return retVal;
#endif

		case 0x1105: /* $5205 */
			return (state->mult1 * state->mult2) & 0xff;
		case 0x1106: /* $5206 */
			return ((state->mult1 * state->mult2) & 0xff00) >> 8;

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
//      nes_vram[i] = state->mmc_vrom_bank[0 + (mode * 8)] * 64;
//}

static WRITE8_HANDLER( mapper5_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
//  static int vrom_next[4];
	static int vrom_page_a;
	static int vrom_page_b;

//  LOG_MMC(("Mapper 5 write, offset: %04x, data: %02x\n", offset + 0x4100, data));
	/* Send $5000-$5015 to the sound chip */
	if ((offset >= 0xf00) && (offset <= 0xf15))
	{
		nes_psg_w(state->sound, offset & 0x1f, data);
		return;
	}

	/* $5c00 - $5fff: extended videoram attributes */
	if ((offset >= 0x1b00) && (offset <= 0x1eff))
	{
		if (MMC5_vram_protect == 0x03)
			state->mmc5_vram[offset - 0x1b00] = data;
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
			state->mmc5_vram_control = data;
			LOG_MMC(("MMC5 exram control: %02x\n", data));
			break;

		case 0x1005: /* $5105 */
			ppu_mirror_MMC5(space->machine, 0, data & 0x03);
			ppu_mirror_MMC5(space->machine, 1, (data & 0x0c) >> 2);
			ppu_mirror_MMC5(space->machine, 2, (data & 0x30) >> 4);
			ppu_mirror_MMC5(space->machine, 3, (data & 0xc0) >> 6);
			break;

		/* tile data for MMC5 flood-fill NT mode */
		case 0x1006:
			state->MMC5_floodtile = data;
			break;

		/* attr data for MMC5 flood-fill NT mode */
		case 0x1007:
			switch (data & 3)
			{
			default:
			case 0: state->MMC5_floodattr = 0x00; break;
			case 1: state->MMC5_floodattr = 0x55; break;
			case 2: state->MMC5_floodattr = 0xaa; break;
			case 3: state->MMC5_floodattr = 0xff; break;
			}
			break;

		case 0x1013: /* $5113 */
			LOG_MMC(("MMC5 mid RAM bank select: %02x\n", data & 0x07));
			state->prg_bank[4] = state->prgram_bank5_start + (data & 0x07);
			memory_set_bank(space->machine, "bank5", state->prg_bank[4]);
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
						LOG_MMC(("\tROM bank select (8k, $8000): %02x\n", data & 0x7f));
						prg8_89(space->machine, data & 0x7f);
					}
					else
					{
						/* RAM */
						LOG_MMC(("\tRAM bank select (8k, $8000): %02x\n", data & 0x07));
						state->prg_bank[0] = state->prg_chunks + data & 0x07;
						memory_set_bank(space->machine, "bank1", state->prg_bank[0]);
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
						LOG_MMC(("\tROM bank select (16k, $8000): %02x\n", (data & 0x7f) >> 1));
						prg16_89ab(space->machine, (data & 0x7f) >> 1);
					}
					else
					{
						/* RAM */
						LOG_MMC(("\tRAM bank select (16k, $8000): %02x\n", (data & 0x07) >> 1));
						state->prg_bank[0] = state->prg_chunks + (data & 0x06);
						state->prg_bank[1] = state->prg_chunks + (data & 0x06) + 1;
						memory_set_bank(space->machine, "bank1", state->prg_bank[0]);
						memory_set_bank(space->machine, "bank2", state->prg_bank[1]);
					}
					break;
				case 0x03:
					/* 8k switch */
					if (data & 0x80)
					{
						/* ROM */
						LOG_MMC(("\tROM bank select (8k, $a000): %02x\n", data & 0x7f));
						prg8_ab(space->machine, data & 0x7f);
					}
					else
					{
						/* RAM */
						LOG_MMC(("\tRAM bank select (8k, $a000): %02x\n", data & 0x07));
						state->prg_bank[1] = state->prg_chunks + data & 0x07;
						memory_set_bank(space->machine, "bank2", state->prg_bank[1]);
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
						LOG_MMC(("\tROM bank select (8k, $c000): %02x\n", data & 0x7f));
						prg8_cd(space->machine, data & 0x7f);
					}
					else
					{
						/* RAM */
						LOG_MMC(("\tRAM bank select (8k, $c000): %02x\n", data & 0x07));
						state->prg_bank[2] = state->prg_chunks + data & 0x07;
						memory_set_bank(space->machine, "bank3", state->prg_bank[2]);
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
					LOG_MMC(("\tROM bank select (32k, $8000): %02x\n", (data & 0x7f) >> 2));
					prg32(space->machine, data >> 2);
					break;
				case 0x01:
					/* 16k switch - ROM only */
					LOG_MMC(("\tROM bank select (16k, $c000): %02x\n", (data & 0x7f) >> 1));
					prg16_cdef(space->machine, data >> 1);
					break;
				case 0x02:
				case 0x03:
					/* 8k switch */
					LOG_MMC(("\tROM bank select (8k, $e000): %02x\n", data & 0x7f));
					prg8_ef(space->machine, data & 0x7f);
					break;
			}
			break;
		case 0x1020: /* $5120 */
			LOG_MMC(("MMC5 $5120 vrom select: %02x (mode: %d)\n", data, MMC5_vrom_bank_mode));
			switch (MMC5_vrom_bank_mode)
			{
				case 0x03:
					/* 1k switch */
					state->mmc_vrom_bank[0] = data;
//                  mapper5_sync_vrom(0);
					chr1_0(space->machine, state->mmc_vrom_bank[0], CHRROM);
//                  state->nes_vram_sprite[0] = state->mmc_vrom_bank[0] * 64;
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
					state->mmc_vrom_bank[1] = data;
//                  mapper5_sync_vrom(0);
					chr1_1(space->machine, state->mmc_vrom_bank[1], CHRROM);
//                  state->nes_vram_sprite[1] = state->mmc_vrom_bank[0] * 64;
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
					state->mmc_vrom_bank[2] = data;
//                  mapper5_sync_vrom(0);
					chr1_2(space->machine, state->mmc_vrom_bank[2], CHRROM);
//                  state->nes_vram_sprite[2] = state->mmc_vrom_bank[0] * 64;
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
					state->mmc_vrom_bank[3] = data;
//                  mapper5_sync_vrom(0);
					chr1_3(space->machine, state->mmc_vrom_bank[3], CHRROM);
//                  state->nes_vram_sprite[3] = state->mmc_vrom_bank[0] * 64;
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
					state->mmc_vrom_bank[4] = data;
//                  mapper5_sync_vrom(0);
					chr1_4(space->machine, state->mmc_vrom_bank[4], CHRROM);
//                  state->nes_vram_sprite[4] = state->mmc_vrom_bank[0] * 64;
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
					state->mmc_vrom_bank[5] = data;
//                  mapper5_sync_vrom(0);
					chr1_5(space->machine, state->mmc_vrom_bank[5], CHRROM);
//                  state->nes_vram_sprite[5] = state->mmc_vrom_bank[0] * 64;
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
					state->mmc_vrom_bank[6] = data;
//                  mapper5_sync_vrom(0);
					chr1_6(space->machine, state->mmc_vrom_bank[6], CHRROM);
//                  state->nes_vram_sprite[6] = state->mmc_vrom_bank[0] * 64;
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
					state->mmc_vrom_bank[7] = data;
//                  mapper5_sync_vrom(0);
					chr1_7(space->machine, state->mmc_vrom_bank[7], CHRROM);
//                  state->nes_vram_sprite[7] = state->mmc_vrom_bank[0] * 64;
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
					state->mmc_vrom_bank[8] = state->mmc_vrom_bank[12] = data;
//                  nes_vram[vrom_next[0]] = data * 64;
//                  nes_vram[0 + (vrom_page_a*4)] = data * 64;
//                  nes_vram[0] = data * 64;
					chr1_4(space->machine, data, CHRROM);
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
					state->mmc_vrom_bank[9] = state->mmc_vrom_bank[13] = data;
//                  nes_vram[vrom_next[1]] = data * 64;
//                  nes_vram[1 + (vrom_page_a*4)] = data * 64;
//                  nes_vram[1] = data * 64;
					chr1_5(space->machine, data, CHRROM);
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
					state->mmc_vrom_bank[10] = state->mmc_vrom_bank[14] = data;
//                  nes_vram[vrom_next[2]] = data * 64;
//                  nes_vram[2 + (vrom_page_a*4)] = data * 64;
//                  nes_vram[2] = data * 64;
					chr1_6(space->machine, data, CHRROM);
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
					state->mmc_vrom_bank[11] = state->mmc_vrom_bank[15] = data;
//                  nes_vram[vrom_next[3]] = data * 64;
//                  nes_vram[3 + (vrom_page_a*4)] = data * 64;
//                  nes_vram[3] = data * 64;
					chr1_7(space->machine, data, CHRROM);
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
			state->IRQ_count = data;
			MMC5_scanline = data;
			LOG_MMC(("MMC5 irq scanline: %d\n", state->IRQ_count));
			break;
		case 0x1104: /* $5204 */
			state->IRQ_enable = data & 0x80;
			LOG_MMC(("MMC5 irq enable: %02x\n", data));
			break;
		case 0x1105: /* $5205 */
			state->mult1 = data;
			break;
		case 0x1106: /* $5206 */
			state->mult2 = data;
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

    In MESS: Supported? Not sure if we could also have ExRAM or not...
       However, priority is pretty low for this mapper.

*************************************************************/

/* Here, IRQ counter decrements every CPU cycle. Since we update it every scanline,
we need to decrement it by 114 (Each scanline consists of 341 dots and, on NTSC,
there are 3 dots to every 1 CPU cycle, hence 114 is the number of cycles per scanline ) */
static void ffe_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;

	/* 114 is the number of cycles per scanline */
	/* TODO: change to reflect the actual number of cycles spent */
	if (state->IRQ_enable)
	{
		if ((0xffff - state->IRQ_count) < 114)
		{
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
			state->IRQ_count = 0xffff;
			state->IRQ_enable = 0;
		}
		state->IRQ_count -= 114;
	}
}

static WRITE8_HANDLER( mapper6_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper6_l_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
		case 0x1fe:
			state->mmc_cmd1 = data & 0x80;
			set_nt_mirroring(space->machine, BIT(data, 4) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
			break;
		case 0x1ff:
			set_nt_mirroring(space->machine, BIT(data, 4) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;

		case 0x401:
			state->IRQ_enable = data & 0x01;
			break;
		case 0x402:
			state->IRQ_count = (state->IRQ_count & 0xff00) | data;
			break;
		case 0x403:
			state->IRQ_enable = 1;
			state->IRQ_count = (state->IRQ_count & 0x00ff) | (data << 8);
			break;
	}
}

static WRITE8_HANDLER( mapper6_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper6_w, offset: %04x, data: %02x\n", offset, data));

	if (!state->mmc_cmd1)	// when in "FFE mode" we are forced to use CHRRAM/EXRAM bank?
	{
		prg16_89ab(space->machine, data >> 2);
		// chr8(space->machine, data & 0x03, ???);
		// due to lack of info on the exact behavior, we simply act as if mmc_cmd=1
		if (state->mmc_chr_source == CHRROM)
			chr8(space->machine, data & 0x03, CHRROM);
	}
	else if (state->mmc_chr_source == CHRROM)			// otherwise, we can use CHRROM (when present)
		chr8(space->machine, data, CHRROM);
}

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

	set_nt_mirroring(space->machine, BIT(data, 4) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
	prg32(space->machine, data);
}

/*************************************************************

    Mapper 8

    Known Boards: FFE3 Copier Board
    Games: Hacked versions of games

    In MESS: Supported? (I have no games to test this)

*************************************************************/

static WRITE8_HANDLER( mapper8_w )
{
	LOG_MMC(("mapper8_w, offset: %04x, data: %02x\n", offset, data));

	chr8(space->machine, data & 0x07, CHRROM);
	prg16_89ab(space->machine, data >> 3);
}

/*************************************************************

    Mapper 9

    Known Boards: MMC2 based Boards, i.e. PNROM & PEEOROM
    Games: Punch Out!!, Mike Tyson's Punch Out!!

    In MESS: Supported

*************************************************************/

static void mapper9_latch (running_device *device, offs_t offset)
{
	nes_state *state = (nes_state *)device->machine->driver_data;
	if ((offset & 0x3ff0) == 0x0fd0)
	{
		LOG_MMC(("mapper9 vrom latch switch (bank 0 low): %02x\n", MMC2_regs[0]));
		state->mmc_cmd1 = 0xfd;
		chr4_0(device->machine, MMC2_regs[0], CHRROM);
	}
	else if ((offset & 0x3ff0) == 0x0fe0)
	{
		LOG_MMC(("mapper9 vrom latch switch (bank 0 high): %02x\n", MMC2_regs[1]));
		state->mmc_cmd1 = 0xfe;
		chr4_0(device->machine, MMC2_regs[1], CHRROM);
	}
	else if ((offset & 0x3ff0) == 0x1fd0)
	{
		LOG_MMC(("mapper9 vrom latch switch (bank 1 low): %02x\n", MMC2_regs[2]));
		state->mmc_cmd2 = 0xfd;
		chr4_4(device->machine, MMC2_regs[2], CHRROM);
	}
	else if ((offset & 0x3ff0) == 0x1fe0)
	{
		LOG_MMC(("mapper9 vrom latch switch (bank 0 high): %02x\n", MMC2_regs[3]));
		state->mmc_cmd2 = 0xfe;
		chr4_4(device->machine, MMC2_regs[3], CHRROM);
	}
}

static WRITE8_HANDLER( mapper9_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper9_w, offset: %04x, data: %02x\n", offset, data));
	switch (offset & 0x7000)
	{
		case 0x2000:
			prg8_89(space->machine, data);
			break;
		case 0x3000:
			MMC2_regs[0] = data;
			if (state->mmc_cmd1 == 0xfd)
				chr4_0(space->machine, MMC2_regs[0], CHRROM);
			break;
		case 0x4000:
			MMC2_regs[1] = data;
			if (state->mmc_cmd1 == 0xfe)
				chr4_0(space->machine, MMC2_regs[1], CHRROM);
			break;
		case 0x5000:
			MMC2_regs[2] = data;
			if (state->mmc_cmd2 == 0xfd)
				chr4_4(space->machine, MMC2_regs[2], CHRROM);
			break;
		case 0x6000:
			MMC2_regs[3] = data;
			if (state->mmc_cmd2 == 0xfe)
				chr4_4(space->machine, MMC2_regs[3], CHRROM);
			break;
		case 0x7000:
			set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper11_w, offset: %04x, data: %02x\n", offset, data));

	chr8(space->machine, data >> 4, state->mmc_chr_source);
	prg32(space->machine, data & 0x0f);
}

/*************************************************************

    Mapper 12

    Known Boards: Bootleg Board by Rex Soft
    Games: Dragon Ball Z 5, Dragon Ball Z Super

    MMC3 clone

    In MESS: Partially Supported

*************************************************************/

static WRITE8_HANDLER( mapper12_l_w )
{
	LOG_MMC(("mapper12_l_w, offset: %04x, data: %02x\n", offset, data));

	mapper12_reg = data;
}

/* we would need to use this read handler in 0x6000-0x7fff as well */
static READ8_HANDLER( mapper12_l_r )
{
	LOG_MMC(("mapper12_l_r, offset: %04x\n", offset));
	return 0x01;
}

static void mapper12_set_chr( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;

	chr1_x(machine, chr_page ^ 0, ((mapper12_reg & 0x01) << 8) | (state->mmc_vrom_bank[0] & ~0x01), CHRROM);
	chr1_x(machine, chr_page ^ 1, ((mapper12_reg & 0x01) << 8) | (state->mmc_vrom_bank[0] |  0x01), CHRROM);
	chr1_x(machine, chr_page ^ 2, ((mapper12_reg & 0x01) << 8) | (state->mmc_vrom_bank[1] & ~0x01), CHRROM);
	chr1_x(machine, chr_page ^ 3, ((mapper12_reg & 0x01) << 8) | (state->mmc_vrom_bank[1] |  0x01), CHRROM);
	chr1_x(machine, chr_page ^ 4, ((mapper12_reg & 0x10) << 4) | state->mmc_vrom_bank[2], CHRROM);
	chr1_x(machine, chr_page ^ 5, ((mapper12_reg & 0x10) << 4) | state->mmc_vrom_bank[3], CHRROM);
	chr1_x(machine, chr_page ^ 6, ((mapper12_reg & 0x10) << 4) | state->mmc_vrom_bank[4], CHRROM);
	chr1_x(machine, chr_page ^ 7, ((mapper12_reg & 0x10) << 4) | state->mmc_vrom_bank[5], CHRROM);
}

static WRITE8_HANDLER( mapper12_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper12_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0000:
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has PRG Mode changed? */
		if (MMC3_helper & 0x40)
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper12_set_chr(space->machine);
		break;

	case 0x0001:
		cmd = state->mmc_cmd1 & 0x07;
		switch (cmd)
		{
		case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
		case 2: case 3: case 4: case 5:
			state->mmc_vrom_bank[cmd] = data;
			mapper12_set_chr(space->machine);
			break;
		case 6:
		case 7:
			state->mmc_prg_bank[cmd - 6] = data;
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			break;
		}
		break;

	default:
		mapper4_w(space, offset, data);
		break;
	}
}

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
    Games: Samurai Spirits

    MMC3 clone

    In MESS: Supported

*************************************************************/

static void mapper14_set_prg( running_machine *machine, int prg_base, int prg_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;

	if (map14_reg[0] & 0x02)
	{
		mapper4_set_prg(machine, prg_base, prg_mask);
	}
	else
	{
		prg8_89(machine, state->mmc_extra_bank[0]);
		prg8_ab(machine, state->mmc_extra_bank[1]);
		prg8_cd(machine, state->mmc_extra_bank[2]);
		prg8_ef(machine, state->mmc_extra_bank[3]);
	}
}

static void mapper14_set_chr( running_machine *machine, UINT8 chr, int chr_base, int chr_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	static const UINT8 conv_table[8] = {5, 5, 5, 5, 3, 3, 1, 1};
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 bank[8];
	UINT8 chr_base2[8];
	int i;

	if (map14_reg[0] & 0x02)
	{
		for(i = 0; i < 8; i++)
		{
			bank[i] = state->mmc_vrom_bank[i];
			chr_base2[i] = chr_base | ((map14_reg[0] << conv_table[i]) & 0x100);
		}
	}
	else
	{
		for(i = 0; i < 8; i++)
		{
			bank[i] = state->mmc_extra_bank[i + 4];	// first 4 state->mmc_extra_banks are PRG
			chr_base2[i] = chr_base;
		}
	}

	chr1_x(machine, chr_page ^ 0, chr_base2[0] | (bank[0] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 1, chr_base2[1] | (bank[1] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 2, chr_base2[2] | (bank[2] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 3, chr_base2[3] | (bank[3] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 4, chr_base2[4] | (bank[4] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 5, chr_base2[5] | (bank[5] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 6, chr_base2[6] | (bank[6] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 7, chr_base2[7] | (bank[7] & chr_mask), chr);
}

static WRITE8_HANDLER( mapper14_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 map14_helper1, map14_helper2, MMC3_helper, cmd;
	LOG_MMC(("mapper14_w, offset: %04x, data: %02x\n", offset, data));

	if (offset == 0x2131)
	{
		map14_reg[0] = data;
		mapper14_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
		mapper14_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);

		if (!(map14_reg[0] & 0x02))
			set_nt_mirroring(space->machine, BIT(map14_reg[1], 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
	}

	if (map14_reg[0] & 0x02)
	{
		switch (offset & 0x6001)
		{
		case 0x0000:
			MMC3_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (MMC3_helper & 0x40)
				mapper14_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

			/* Has CHR Mode changed? */
			if (MMC3_helper & 0x80)
				mapper14_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;

		case 0x0001:
			cmd = state->mmc_cmd1 & 0x07;
			switch (cmd)
			{
				case 0: case 1:	// these have to be changed due to the different way mapper14_set_chr works (it handles 1k banks)!
					state->mmc_vrom_bank[2 * cmd] = data;
					state->mmc_vrom_bank[2 * cmd + 1] = data;
					mapper14_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
					break;
				case 2: case 3: case 4: case 5:
					state->mmc_vrom_bank[cmd + 2] = data;
					mapper14_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
					break;
				case 6:
				case 7:
					state->mmc_prg_bank[cmd - 6] = data;
					mapper14_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
					break;
			}
			break;

		case 0x2000:
			set_nt_mirroring(space->machine, BIT(map14_reg[1], 0) ? PPU_MIRROR_VERT : PPU_MIRROR_HORZ);
			break;

		default:
			mapper4_w(space, offset, data);
			break;
		}
	}
	else if (offset >= 0x3000 && offset <= 0x6003 )
	{
		map14_helper1 = (offset & 0x01) << 2;
		offset = ((offset & 0x02) | (offset >> 10)) >> 1;
		map14_helper2 = ((offset + 2) & 0x07) + 4; // '+4' because first 4 state->mmc_extra_banks are for PRG!
		state->mmc_extra_bank[map14_helper2] = (state->mmc_extra_bank[map14_helper2] & (0xf0 >> map14_helper1)) | ((data & 0x0f) << map14_helper1);
		mapper14_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
	}
	else
	{
		switch (offset & 0x7003)
		{
		case 0x0000:
		case 0x2000:
			state->mmc_extra_bank[offset >> 13] = data;
			mapper14_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			break;

		case 0x1000:
			map14_reg[1] = data;
			set_nt_mirroring(space->machine, BIT(map14_reg[1], 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;
		}
	}
}

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

	set_nt_mirroring(space->machine, BIT(data, 6) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

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
			prg8_ef(space->machine, map15_helper + 1);
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
			prg8_ef(space->machine, map15_helper + 1);
			break;
	}
}

/*************************************************************

    Mapper 16

    Known Boards: Bandai LZ93D50 24C02
    Games: Crayon Shin-Chan - Ora to Poi Poi, Dragon Ball Z Gaiden,
          Dragon Ball Z II & III, Rokudenashi Blues, SD Gundam
          Gaiden - KGM2

    At the moment, we don't support EEPROM I/O

    In MESS: Supported

*************************************************************/

/* Here, IRQ counter decrements every CPU cycle. Since we update it every scanline,
we need to decrement it by 114 (Each scanline consists of 341 dots and, on NTSC,
there are 3 dots to every 1 CPU cycle, hence 114 is the number of cycles per scanline ) */
static void bandai_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;

	/* 114 is the number of cycles per scanline */
	/* TODO: change to reflect the actual number of cycles spent */
	if (state->IRQ_enable)
	{
		if (state->IRQ_count <= 114)
		{
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
			state->IRQ_count = (0xffff - 114 + state->IRQ_count);	// wrap around the 16 bits counter
		}
		state->IRQ_count -= 114;
	}
}

static WRITE8_HANDLER( mapper16_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper16_m_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x000f)
	{
		case 0: case 1:
		case 2: case 3:
		case 4: case 5:
		case 6: case 7:
			chr1_x(space->machine, offset & 0x07, data, state->mmc_chr_source);
			break;
		case 8:
			prg16_89ab(space->machine, data);
			break;
		case 9:
			switch (data & 0x03)
			{
				case 0: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
				case 1: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
				case 2: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
				case 3: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
			}
			break;
		case 0x0a:
			state->IRQ_enable = data & 0x01;
			break;
		case 0x0b:
			state->IRQ_count = (state->IRQ_count & 0xff00) | data;
			break;
		case 0x0c:
			state->IRQ_count = (state->IRQ_count & 0x00ff) | (data << 8);
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

    In MESS: Supported?. IRQ support is just a guess (used to
      use MMC3 IRQ but it was wrong and it never enabled it)

*************************************************************/

static WRITE8_HANDLER( mapper17_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper17_l_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
		case 0x1fe:
			set_nt_mirroring(space->machine, BIT(data, 4) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
			break;
		case 0x1ff:
			set_nt_mirroring(space->machine, BIT(data, 4) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;

		case 0x401:
			state->IRQ_enable = data & 0x01;
			break;
		case 0x402:
			state->IRQ_count = (state->IRQ_count & 0xff00) | data;
			break;
		case 0x403:
			state->IRQ_enable = 1;
			state->IRQ_count = (state->IRQ_count & 0x00ff) | (data << 8);
			break;

		case 0x404:
			prg8_89(space->machine, data);
			break;
		case 0x405:
			prg8_ab(space->machine, data);
			break;
		case 0x406:
			prg8_cd(space->machine, data);
			break;
		case 0x407:
			prg8_ef(space->machine, data);
			break;

		case 0x410:
		case 0x411:
		case 0x412:
		case 0x413:
		case 0x414:
		case 0x415:
		case 0x416:
		case 0x417:
			chr1_x(space->machine, offset & 7, data, CHRROM);
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
/* Here, IRQ counter decrements every CPU cycle. Since we update it every scanline,
we need to decrement it by 114 (Each scanline consists of 341 dots and, on NTSC,
there are 3 dots to every 1 CPU cycle, hence 114 is the number of cycles per scanline ) */
static void jaleco_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;

	/* Increment & check the IRQ scanline counter */
	if (state->IRQ_enable)
	{
		LOG_MMC(("scanline: %d, irq count: %04x\n", scanline, state->IRQ_count));
		if (state->IRQ_mode & 0x08)
		{
			if ((state->IRQ_count & 0x000f) < 114)	// always true, but we only update the IRQ once per scanlines so we cannot be more precise :(
			{
				cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
				state->IRQ_count = (state->IRQ_count & ~0x000f) | (0x0f - (114 & 0x0f) + (state->IRQ_count & 0x000f)); // sort of wrap around the counter
			}
			// decrements should not affect upper bits, so we don't do anything here (114 > 0x0f)
		}
		else if (state->IRQ_mode & 0x04)
		{
			if ((state->IRQ_count & 0x00ff) < 114)
			{
				cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
				state->IRQ_count = (state->IRQ_count & ~0x00ff) | (0xff - 114 + (state->IRQ_count & 0x00ff));	// wrap around the 8 bits counter
			}
			else
				state->IRQ_count -= 114;
		}
		else if (state->IRQ_mode & 0x02)
		{
			if ((state->IRQ_count & 0x0fff)  < 114)
			{
				cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
				state->IRQ_count = (state->IRQ_count & ~0x0fff) | (0xfff - 114 + (state->IRQ_count & 0x0fff));	// wrap around the 12 bits counter
			}
			else
				state->IRQ_count -= 114;
		}
		else if (state->IRQ_count < 114)
		{
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
			state->IRQ_count = (0xffff - 114 + state->IRQ_count);	// wrap around the 16 bits counter
		}
		else
			state->IRQ_count -= 114;
	}
}

static WRITE8_HANDLER( mapper18_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 bank;
	LOG_MMC(("mapper18_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7003)
	{
		case 0x0000:
			state->mmc_prg_bank[0] = (state->mmc_prg_bank[0] & 0xf0) | (data & 0x0f);
			prg8_89(space->machine, state->mmc_prg_bank[0]);
			break;
		case 0x0001:
			state->mmc_prg_bank[0] = (state->mmc_prg_bank[0] & 0x0f) | (data << 4);
			prg8_89(space->machine, state->mmc_prg_bank[0]);
			break;
		case 0x0002:
			state->mmc_prg_bank[1] = (state->mmc_prg_bank[1] & 0xf0) | (data & 0x0f);
			prg8_ab(space->machine, state->mmc_prg_bank[1]);
			break;
		case 0x0003:
			state->mmc_prg_bank[1] = (state->mmc_prg_bank[1] & 0x0f) | (data << 4);
			prg8_ab(space->machine, state->mmc_prg_bank[1]);
			break;
		case 0x1000:
			state->mmc_prg_bank[2] = (state->mmc_prg_bank[2] & 0xf0) | (data & 0x0f);
			prg8_cd(space->machine, state->mmc_prg_bank[2]);
			break;
		case 0x1001:
			state->mmc_prg_bank[2] = (state->mmc_prg_bank[2] & 0x0f) | (data << 4);
			prg8_cd(space->machine, state->mmc_prg_bank[2]);
			break;

		/* $9002, 3 (1002, 3) uncaught = Jaleco Baseball writes 0 */
		/* believe it's related to battery-backed ram enable/disable */

		case 0x2000: case 0x2001: case 0x2002: case 0x2003:
		case 0x3000: case 0x3001: case 0x3002: case 0x3003:
		case 0x4000: case 0x4001: case 0x4002: case 0x4003:
		case 0x5000: case 0x5001: case 0x5002: case 0x5003:
			bank = ((offset & 0x7000) - 0x2000) / 0x0800 + ((offset & 0x0002) >> 1);
			if (offset & 0x0001)
				state->mmc_vrom_bank[bank] = (state->mmc_vrom_bank[bank] & 0x0f) | ((data & 0x0f)<< 4);
			else
				state->mmc_vrom_bank[bank] = (state->mmc_vrom_bank[bank] & 0xf0) | (data & 0x0f);

			chr1_x(space->machine, bank, state->mmc_vrom_bank[bank], CHRROM);
			break;

		case 0x6000:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0xfff0) | (data & 0x0f);
			break;
		case 0x6001:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0xff0f) | ((data & 0x0f) << 4);
			break;
		case 0x6002:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0xf0ff) | ((data & 0x0f) << 8);
			break;
		case 0x6003:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0x0fff) | ((data & 0x0f) << 12);
			break;

		case 0x7000:
			state->IRQ_count = state->IRQ_count_latch;
			break;
		case 0x7001:
			state->IRQ_enable = data & 0x01;
			state->IRQ_mode = data & 0x0e;
			break;

		case 0x7002:
			switch (data & 0x03)
			{
				case 0: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
				case 1: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
				case 2: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
				case 3: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
			}
			break;

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

/* Here, IRQ counter decrements every CPU cycle. Since we update it every scanline,
we need to decrement it by 114 (Each scanline consists of 341 dots and, on NTSC,
there are 3 dots to every 1 CPU cycle, hence 114 is the number of cycles per scanline ) */
static void namcot_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;

	if (state->IRQ_enable)
	{
		if (state->IRQ_count >= (0x7fff - 114))
		{
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
			state->IRQ_count = 0;
		}
		else
			state->IRQ_count += 114;
	}
}

static WRITE8_HANDLER( mapper19_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper19_l_w, offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	switch (offset & 0x1800)
	{
		case 0x0800:
			LOG_MMC(("Mapper 19 sound port write, data: %02x\n", data));
			break;
		case 0x1000: /* low byte of IRQ */
			state->IRQ_count = (state->IRQ_count & 0x7f00) | data;
			break;
		case 0x1800: /* high byte of IRQ, IRQ enable in high bit */
			state->IRQ_count = (state->IRQ_count & 0xff) | ((data & 0x7f) << 8);
			state->IRQ_enable = data & 0x80;
			break;
	}
}

static READ8_HANDLER( mapper19_l_r )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper19_l_r, offset: %04x\n", offset));
	offset += 0x100;

	switch (offset & 0x1800)
	{
		case 0x1000:
			return state->IRQ_count & 0xff;
		case 0x1800:
			return (state->IRQ_count >> 8) & 0xff;
		default:
			return 0x00;
	}
}

static void mapper19_set_mirror( running_machine *machine, UINT8 page, UINT8 data )
{
	if (!(data < 0xe0))
		set_nt_page(machine, page, CIRAM, data & 0x01, 1);
	else
		set_nt_page(machine, page, ROM, data, 0);
}

static WRITE8_HANDLER( mapper19_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper19_w, offset: %04x, data: %02x\n", offset, data));
	switch (offset & 0x7800)
	{
		case 0x0000: case 0x0800:
		case 0x1000: case 0x1800:
		case 0x2000: case 0x2800:
		case 0x3000: case 0x3800:
			chr1_x(space->machine, offset / 0x800, data, CHRROM);
			break;
		case 0x4000:
			mapper19_set_mirror(space->machine, 0, data);
			break;
		case 0x4800:
			mapper19_set_mirror(space->machine, 1, data);
			break;
		case 0x5000:
			mapper19_set_mirror(space->machine, 2, data);
			break;
		case 0x5800:
			mapper19_set_mirror(space->machine, 3, data);
			break;
		case 0x6000:
			prg8_89(space->machine, data & 0x3f);
			break;
		case 0x6800:
			state->mmc_cmd1 = data & 0xc0;		// this should enable High CHRRAM, but we still have to properly implement it!
			prg8_ab(space->machine, data & 0x3f);
			break;
		case 0x7000:
			prg8_cd(space->machine, data & 0x3f);
			break;
		case 0x7800:
			LOG_MMC(("Mapper 19 sound address write, data: %02x\n", data));
			break;
	}
}

/*************************************************************

    Mapper 20

    Known Boards: Reserved for FDS
    Games: any FDS disk

    In MESS: Supported

*************************************************************/

static void fds_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;

	if (state->IRQ_enable_latch)
		cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);

	if (state->IRQ_enable)
	{
		if (state->IRQ_count <= 114)
		{
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
			state->IRQ_enable = 0;
			state->fds_status0 |= 0x01;
		}
		else
			state->IRQ_count -= 114;
	}
}

READ8_HANDLER( nes_fds_r )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 ret = 0x00;
	LOG_MMC(("fds_r, offset: %04x\n", offset));

	switch (offset)
	{
		case 0x00: /* $4030 - disk status 0 */
			ret = state->fds_status0;
			/* clear the disk IRQ detect flag */
			state->fds_status0 &= ~0x01;
			break;
		case 0x01: /* $4031 - data latch */
			/* don't read data if disk is unloaded */
			if (state->fds_data == NULL)
				ret = 0;
			else if (state->fds_current_side)
				ret = state->fds_data[(state->fds_current_side - 1) * 65500 + state->fds_head_position++];
			else
				ret = 0;
			break;
		case 0x02: /* $4032 - disk status 1 */
			/* return "no disk" status if disk is unloaded */
			if (state->fds_data == NULL)
				ret = 1;
			else if (state->fds_last_side != state->fds_current_side)
			{
				/* If we've switched disks, report "no disk" for a few reads */
				ret = 1;
				state->fds_count++;
				if (state->fds_count == 50)
				{
					state->fds_last_side = state->fds_current_side;
					state->fds_count = 0;
				}
			}
			else
				ret = (state->fds_current_side == 0); /* 0 if a disk is inserted */
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

WRITE8_HANDLER( nes_fds_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("fds_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
		case 0x00:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0xff00) | data;
			break;
		case 0x01:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0x00ff) | (data << 8);
			break;
		case 0x02:
			state->IRQ_count = state->IRQ_count_latch;
			state->IRQ_enable = data;
			break;
		case 0x03:
			// d0 = sound io (1 = enable)
			// d1 = disk io (1 = enable)
			break;
		case 0x04:
			/* write data out to disk */
			break;
		case 0x05:
			state->fds_motor_on = BIT(data, 0);

			if (BIT(data, 1))
				state->fds_head_position = 0;

			state->fds_read_mode = BIT(data, 2);
			set_nt_mirroring(space->machine, BIT(data, 3) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

			if ((!(data & 0x40)) && (state->fds_write_reg & 0x40))
				state->fds_head_position -= 2; // ???

			state->IRQ_enable_latch = BIT(data, 7);
			state->fds_write_reg = data;
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

static void vrc4_set_prg( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	if (state->mmc_cmd1 & 0x02)
	{
		prg8_89(machine, 0xfe);
		prg8_cd(machine, state->mmc_prg_bank[0]);
	}
	else
	{
		prg8_89(machine, state->mmc_prg_bank[0]);
		prg8_cd(machine, 0xfe);
	}
}

static void konami_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;
	/* Increment & check the IRQ scanline counter */
	if (state->IRQ_enable && (++state->IRQ_count == 0x100))
	{
		state->IRQ_count = state->IRQ_count_latch;
		state->IRQ_enable = state->IRQ_enable_latch;
		cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
	}
}

static WRITE8_HANDLER( konami_vrc4a_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("konami_vrc4a_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7fff)
	{
		case 0x0000:
		case 0x0002:
		case 0x0004:
		case 0x0006:
		case 0x0040:
		case 0x0080:
		case 0x00c0:
			state->mmc_prg_bank[0] = data;
			vrc4_set_prg(space->machine);
			break;

		case 0x1000:
		case 0x1002:
		case 0x1040:
			switch (data & 0x03)
			{
				case 0x00: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
				case 0x01: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
				case 0x02: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
				case 0x03: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
			}
			break;

		case 0x1004:
		case 0x1006:
		case 0x1080:
		case 0x10c0:
			state->mmc_cmd1 = data & 0x02;
			vrc4_set_prg(space->machine);
			break;

		case 0x2000:
		case 0x2002:
		case 0x2004:
		case 0x2006:
		case 0x2040:
		case 0x2080:
		case 0x20c0:
			prg8_ab(space->machine, data);
			break;

		case 0x3000:
			state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & 0xf0) | (data & 0x0f);
			chr1_0(space->machine, state->mmc_vrom_bank[0], CHRROM);
			break;
		case 0x3002:
		case 0x3040:
			state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & 0x0f) | (data << 4);
			chr1_0(space->machine, state->mmc_vrom_bank[0], CHRROM);
			break;
		case 0x3004:
		case 0x3080:
			state->mmc_vrom_bank[1] = (state->mmc_vrom_bank[1] & 0xf0) | (data & 0x0f);
			chr1_1(space->machine, state->mmc_vrom_bank[1], CHRROM);
			break;
		case 0x3006:
		case 0x30c0:
			state->mmc_vrom_bank[1] = (state->mmc_vrom_bank[1] & 0x0f) | (data << 4);
			chr1_1(space->machine, state->mmc_vrom_bank[1], CHRROM);
			break;
		case 0x4000:
			state->mmc_vrom_bank[2] = (state->mmc_vrom_bank[2] & 0xf0) | (data & 0x0f);
			chr1_2(space->machine, state->mmc_vrom_bank[2], CHRROM);
			break;
		case 0x4002:
		case 0x4040:
			state->mmc_vrom_bank[2] = (state->mmc_vrom_bank[2] & 0x0f) | (data << 4);
			chr1_2(space->machine, state->mmc_vrom_bank[2], CHRROM);
			break;
		case 0x4004:
		case 0x4080:
			state->mmc_vrom_bank[3] = (state->mmc_vrom_bank[3] & 0xf0) | (data & 0x0f);
			chr1_3(space->machine, state->mmc_vrom_bank[3], CHRROM);
			break;
		case 0x4006:
		case 0x40c0:
			state->mmc_vrom_bank[3] = (state->mmc_vrom_bank[3] & 0x0f) | (data << 4);
			chr1_3(space->machine, state->mmc_vrom_bank[3], CHRROM);
			break;
		case 0x5000:
			state->mmc_vrom_bank[4] = (state->mmc_vrom_bank[4] & 0xf0) | (data & 0x0f);
			chr1_4(space->machine, state->mmc_vrom_bank[4], CHRROM);
			break;
		case 0x5002:
		case 0x5040:
			state->mmc_vrom_bank[4] = (state->mmc_vrom_bank[4] & 0x0f) | (data << 4);
			chr1_4(space->machine, state->mmc_vrom_bank[4], CHRROM);
			break;
		case 0x5004:
		case 0x5080:
			state->mmc_vrom_bank[5] = (state->mmc_vrom_bank[5] & 0xf0) | (data & 0x0f);
			chr1_5(space->machine, state->mmc_vrom_bank[5], CHRROM);
			break;
		case 0x5006:
		case 0x50c0:
			state->mmc_vrom_bank[5] = (state->mmc_vrom_bank[5] & 0x0f) | (data << 4);
			chr1_5(space->machine, state->mmc_vrom_bank[5], CHRROM);
			break;
		case 0x6000:
			state->mmc_vrom_bank[6] = (state->mmc_vrom_bank[6] & 0xf0) | (data & 0x0f);
			chr1_6(space->machine, state->mmc_vrom_bank[6], CHRROM);
			break;
		case 0x6002:
		case 0x6040:
			state->mmc_vrom_bank[6] = (state->mmc_vrom_bank[6] & 0x0f) | (data << 4);
			chr1_6(space->machine, state->mmc_vrom_bank[6], CHRROM);
			break;
		case 0x6004:
		case 0x6080:
			state->mmc_vrom_bank[7] = (state->mmc_vrom_bank[7] & 0xf0) | (data & 0x0f);
			chr1_7(space->machine, state->mmc_vrom_bank[7], CHRROM);
			break;
		case 0x6006:
		case 0x60c0:
			state->mmc_vrom_bank[7] = (state->mmc_vrom_bank[7] & 0x0f) | (data << 4);
			chr1_7(space->machine, state->mmc_vrom_bank[7], CHRROM);
			break;

		case 0x7000:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0xf0) | (data & 0x0f);
			break;
		case 0x7002:
		case 0x7040:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0x0f) | ((data & 0x0f) << 4 );
			break;
		case 0x7004:
		case 0x7080:
			state->IRQ_mode = data & 0x04;	// currently not implemented: 0 = prescaler mode / 1 = CPU mode
			state->IRQ_enable = data & 0x02;
			state->IRQ_enable_latch = data & 0x01;
			if (data & 0x02)
				state->IRQ_count = state->IRQ_count_latch;
			break;
		case 0x7006:
		case 0x70c0:
			state->IRQ_enable = state->IRQ_enable_latch;
			break;
		default:
			LOG_MMC(("konami_vrc4a_w uncaught offset: %04x value: %02x\n", offset, data));
			break;
	}
}

static WRITE8_HANDLER( konami_vrc4b_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("konami_vrc4b_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7fff)
	{
		case 0x0000:
		case 0x0001:
		case 0x0002:
		case 0x0003:
		case 0x0004:
		case 0x0008:
		case 0x000c:
			state->mmc_prg_bank[0] = data;
			vrc4_set_prg(space->machine);
			break;

		case 0x1000:
		case 0x1002:
		case 0x1008:
			switch (data & 0x03)
			{
				case 0x00: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
				case 0x01: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
				case 0x02: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
				case 0x03: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
			}
			break;

		case 0x1001:
		case 0x1003:
		case 0x1004:
		case 0x100c:
			state->mmc_cmd1 = data & 0x02;
			vrc4_set_prg(space->machine);
			break;

		case 0x2000:
		case 0x2001:
		case 0x2002:
		case 0x2003:
		case 0x2004:
		case 0x2008:
		case 0x200c:
			prg8_ab(space->machine, data);
			break;

		case 0x3000:
			state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & 0xf0) | (data & 0x0f);
			chr1_0(space->machine, state->mmc_vrom_bank[0], CHRROM);
			break;
		case 0x3002:
		case 0x3008:
			state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & 0x0f) | (data << 4);
			chr1_0(space->machine, state->mmc_vrom_bank[0], CHRROM);
			break;
		case 0x3001:
		case 0x3004:
			state->mmc_vrom_bank[1] = (state->mmc_vrom_bank[1] & 0xf0) | (data & 0x0f);
			chr1_1(space->machine, state->mmc_vrom_bank[1], CHRROM);
			break;
		case 0x3003:
		case 0x300c:
			state->mmc_vrom_bank[1] = (state->mmc_vrom_bank[1] & 0x0f) | (data << 4);
			chr1_1(space->machine, state->mmc_vrom_bank[1], CHRROM);
			break;
		case 0x4000:
			state->mmc_vrom_bank[2] = (state->mmc_vrom_bank[2] & 0xf0) | (data & 0x0f);
			chr1_2(space->machine, state->mmc_vrom_bank[2], CHRROM);
			break;
		case 0x4002:
		case 0x4008:
			state->mmc_vrom_bank[2] = (state->mmc_vrom_bank[2] & 0x0f) | (data << 4);
			chr1_2(space->machine, state->mmc_vrom_bank[2], CHRROM);
			break;
		case 0x4001:
		case 0x4004:
			state->mmc_vrom_bank[3] = (state->mmc_vrom_bank[3] & 0xf0) | (data & 0x0f);
			chr1_3(space->machine, state->mmc_vrom_bank[3], CHRROM);
			break;
		case 0x4003:
		case 0x400c:
			state->mmc_vrom_bank[3] = (state->mmc_vrom_bank[3] & 0x0f) | (data << 4);
			chr1_3(space->machine, state->mmc_vrom_bank[3], CHRROM);
			break;
		case 0x5000:
			state->mmc_vrom_bank[4] = (state->mmc_vrom_bank[4] & 0xf0) | (data & 0x0f);
			chr1_4(space->machine, state->mmc_vrom_bank[4], CHRROM);
			break;
		case 0x5002:
		case 0x5008:
			state->mmc_vrom_bank[4] = (state->mmc_vrom_bank[4] & 0x0f) | (data << 4);
			chr1_4(space->machine, state->mmc_vrom_bank[4], CHRROM);
			break;
		case 0x5001:
		case 0x5004:
			state->mmc_vrom_bank[5] = (state->mmc_vrom_bank[5] & 0xf0) | (data & 0x0f);
			chr1_5(space->machine, state->mmc_vrom_bank[5], CHRROM);
			break;
		case 0x5003:
		case 0x500c:
			state->mmc_vrom_bank[5] = (state->mmc_vrom_bank[5] & 0x0f) | (data << 4);
			chr1_5(space->machine, state->mmc_vrom_bank[5], CHRROM);
			break;
		case 0x6000:
			state->mmc_vrom_bank[6] = (state->mmc_vrom_bank[6] & 0xf0) | (data & 0x0f);
			chr1_6(space->machine, state->mmc_vrom_bank[6], CHRROM);
			break;
		case 0x6002:
		case 0x6008:
			state->mmc_vrom_bank[6] = (state->mmc_vrom_bank[6] & 0x0f) | (data << 4);
			chr1_6(space->machine, state->mmc_vrom_bank[6], CHRROM);
			break;
		case 0x6001:
		case 0x6004:
			state->mmc_vrom_bank[7] = (state->mmc_vrom_bank[7] & 0xf0) | (data & 0x0f);
			chr1_7(space->machine, state->mmc_vrom_bank[7], CHRROM);
			break;
		case 0x6003:
		case 0x600c:
			state->mmc_vrom_bank[7] = (state->mmc_vrom_bank[7] & 0x0f) | (data << 4);
			chr1_7(space->machine, state->mmc_vrom_bank[7], CHRROM);
			break;

		case 0x7000:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0xf0) | (data & 0x0f);
			break;
		case 0x7002:
		case 0x7008:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0x0f) | ((data & 0x0f) << 4 );
			break;
		case 0x7001:
		case 0x7004:
			state->IRQ_mode = data & 0x04;	// currently not implemented: 0 = prescaler mode / 1 = CPU mode
			state->IRQ_enable = data & 0x02;
			state->IRQ_enable_latch = data & 0x01;
			if (data & 0x02)
				state->IRQ_count = state->IRQ_count_latch;
			break;
		case 0x7003:
		case 0x700c:
			state->IRQ_enable = state->IRQ_enable_latch;
			break;
		default:
			LOG_MMC(("konami_vrc4b_w uncaught offset: %04x value: %02x\n", offset, data));
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
	nes_state *state = (nes_state *)space->machine->driver_data;
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
			case 0x00: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
			case 0x01: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
			case 0x02: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
			case 0x03: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
		}
		break;
	case 0x2000:
		prg8_ab(space->machine, data);
		break;
	case 0x3000:
	case 0x4000:
	case 0x5000:
	case 0x6000:
		bank = ((offset & 0x7000) - 0x3000) / 0x0800 + (offset & 0x0001);
		/* Notice that we store the banks as in other VRC, but then we take state->mmc_vrom_bank>>1 because it's VRC2A!*/
		if (offset & 0x0002)
			state->mmc_vrom_bank[bank] = (state->mmc_vrom_bank[bank] & 0x0f) | (data << 4);
		else
			state->mmc_vrom_bank[bank] = (state->mmc_vrom_bank[bank] & 0xf0) | (data & 0x0f);

		chr1_x(space->machine, bank, state->mmc_vrom_bank[bank] >> 1, CHRROM);
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT16 select;
	UINT8 bank;

	LOG_MMC(("konami_vrc2b_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x3000)
	{
		switch (offset & 0x3000)
		{
			case 0:
				prg8_89(space->machine, data);
				break;
			case 0x1000:
				switch (data & 0x03)
				{
					case 0x00: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
					case 0x01: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
					case 0x02: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
					case 0x03: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
				}
				break;
			case 0x2000:
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
		bank = ((select & 0x7000) - 0x3000) / 0x0800 + ((select & 0x0002) >> 1);
		if (select & 0x0001)
			state->mmc_vrom_bank[bank] = (state->mmc_vrom_bank[bank] & 0x0f) | (data << 4);
		else
			state->mmc_vrom_bank[bank] = (state->mmc_vrom_bank[bank] & 0xf0) | (data & 0x0f);

		chr1_x(space->machine, bank, state->mmc_vrom_bank[bank], state->mmc_chr_source);
		break;
	case 0x7000:
		state->IRQ_count_latch &= ~0x0f;
		state->IRQ_count_latch |= data & 0x0f;
		break;
	case 0x7001:
		state->IRQ_count_latch &= ~0xf0;
		state->IRQ_count_latch |= (data << 4) & 0xf0;
		break;
	case 0x7002:
		state->IRQ_mode = data & 0x04;	// currently not implemented: 0 = prescaler mode / 1 = CPU mode
		state->IRQ_enable = data & 0x02;
		state->IRQ_enable_latch = data & 0x01;
		if (data & 0x02)
			state->IRQ_count = state->IRQ_count_latch;
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 bank;
	LOG_MMC(("konami_vrc6a_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7003)
	{
		case 0x0000:
		case 0x0001:
		case 0x0002:
		case 0x0003:
			prg16_89ab(space->machine, data);
			break;

/* $1000-$1002 = sound regs */
/* $2000-$2002 = sound regs */
/* $3000-$3002 = sound regs */

		case 0x3003:
			switch (data & 0x0c)
			{
				case 0x00: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
				case 0x04: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
				case 0x08: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
				case 0x0c: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
			}
			break;
		case 0x4000:
		case 0x4001:
		case 0x4002:
		case 0x4003:
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
			bank = ((offset & 0x7000) - 0x5000) / 0x0400 + (offset & 0x0003);
			chr1_x(space->machine, bank, data, CHRROM);
			break;

		case 0x7000:
			state->IRQ_count_latch = data;
			break;
		case 0x7001:
			state->IRQ_mode = data & 0x04;	// currently not implemented: 0 = prescaler mode / 1 = CPU mode
			state->IRQ_enable = data & 0x02;
			state->IRQ_enable_latch = data & 0x01;
			if (data & 0x02)
				state->IRQ_count = state->IRQ_count_latch;
			break;
		case 0x7002:
			state->IRQ_enable = state->IRQ_enable_latch;
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 bank;

	LOG_MMC(("konami_vrc6b_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7003)
	{
		case 0: case 1: case 2: case 3:
			prg16_89ab(space->machine, data);
			break;

/* $1000-$1002 = sound regs */
/* $2000-$2002 = sound regs */
/* $3000-$3002 = sound regs */

		case 0x3003:
			switch (data & 0x0c)
			{
				case 0x00: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
				case 0x04: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
				case 0x08: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
				case 0x0c: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
			}
			break;
		case 0x4000: case 0x4001: case 0x4002: case 0x4003:
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
			bank = ((offset & 0x7000) - 0x5000) / 0x0400 + ((offset & 0x0001) << 1) + ((offset & 0x0002) >> 1);
			chr1_x(space->machine, bank, data, CHRROM);
			break;
		case 0x7000:
			state->IRQ_count_latch = data;
			break;
		case 0x7001:
			state->IRQ_enable = state->IRQ_enable_latch;
			break;
		case 0x7002:
			state->IRQ_mode = data & 0x04;	// currently not implemented: 0 = prescaler mode / 1 = CPU mode
			state->IRQ_enable = data & 0x02;
			state->IRQ_enable_latch = data & 0x01;
			if (data & 0x02)
				state->IRQ_count = state->IRQ_count_latch;
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

    A crc check is required to support Major League (which uses
    a slightly different board)

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper32_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper32_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
		case 0x0000:
			state->mmc_cmd1 ? prg8_cd(space->machine, data) : prg8_89(space->machine, data);
			break;
		case 0x1000:
			state->mmc_cmd1 = data & 0x02;
			if (!state->crc_hack)	// Major League has hardwired mirroring (it would have required a separate mapper)
				set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;
		case 0x2000:
			prg8_ab(space->machine, data);
			break;
		case 0x3000:
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

	switch (offset & 0x6003)
	{
		case 0x0000:
			set_nt_mirroring(space->machine, BIT(data, 6) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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

    There actually exist TWO different kind of boards using
    this mapper. NINA-001 is not compatible with the other
    kind of board, so Impossible Mission II requires a crc check
    to be supported.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper34_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper34_m_w, offset: %04x, data: %02x\n", offset, data));

	if (state->crc_hack)	// is it AVE Nina-01?
	{
		switch (offset)
		{
		case 0x1ffd:
			prg32(space->machine, data);
			break;
		case 0x1ffe:
			chr4_0(space->machine, data, CHRROM);
			break;
		case 0x1fff:
			chr4_4(space->machine, data, CHRROM);
			break;
		}
	}
}

static WRITE8_HANDLER( mapper34_w )
{
	/* This portion of the mapper is nearly identical to Mapper 7, except no one-screen mirroring */
	/* Deadly Towers is really a Mapper 34 game - the demo screens look wrong using mapper 7. */
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper34_w, offset: %04x, data: %02x\n", offset, data));

	if (!state->crc_hack)	// is it plain BxROM?
		prg32(space->machine, data);
}

/*************************************************************

    Mapper 35

    Known Boards: SC-127 Board
    Games: Wario World II (Kirby Hack)

    In MESS: Supported

*************************************************************/

static void mapper35_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;

	if (scanline < PPU_BOTTOM_VISIBLE_SCANLINE && state->IRQ_enable)
	{
		state->IRQ_count--;

		if (!blanked && (state->IRQ_count == 0))
		{
			LOG_MMC(("irq fired, scanline: %d (MAME %d, beam pos: %d)\n", scanline,
					video_screen_get_vpos(device->machine->primary_screen), video_screen_get_hpos(device->machine->primary_screen)));
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
			state->IRQ_enable = 0;
		}
	}
}

static WRITE8_HANDLER( mapper35_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper35_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
	case 0x0000:
		prg8_89(space->machine, data);
		break;
	case 0x0001:
		prg8_ab(space->machine, data);
		break;
	case 0x0002:
//      state->mmc_prg_bank[offset & 0x02] = data;
		prg8_cd(space->machine, data);
		break;
	case 0x1000:
	case 0x1001:
	case 0x1002:
	case 0x1003:
	case 0x1004:
	case 0x1005:
	case 0x1006:
	case 0x1007:
//      state->mmc_vrom_bank[offset & 0x07] = data;
		chr1_x(space->machine, offset & 0x07, data, CHRROM);
		break;
	case 0x4002:
		state->IRQ_enable = 0;
		break;
	case 0x4003:
		state->IRQ_enable = 1;
		break;
	case 0x4005:
		state->IRQ_count = data;
		break;
	case 0x5001:
		set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	}
}

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

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper37_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 map37_helper;
	LOG_MMC(("mapper37_m_w, offset: %04x, data: %02x\n", offset, data));

	map37_helper = (data & 0x06) >> 1;

	state->mmc_prg_base = map37_helper << 3;
	state->mmc_prg_mask = (map37_helper == 2) ? 0x0f : 0x07;
	state->mmc_chr_base = map37_helper << 6;
	state->mmc_chr_mask = 0x7f;
	mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
	mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
}

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
    Games: Super Mario Bros. 2 Pirate (Jpn version of SMB2)

    In MESS: Supported.

*************************************************************/

static void mapper40_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;

	if (state->IRQ_enable)
	{
		if ((0xfff - state->IRQ_count) <= 114)
		{
			state->IRQ_count = (state->IRQ_count + 1) & 0xfff;
			state->IRQ_enable = 0;
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
		}
		else
			state->IRQ_count += 114;
	}
}

static WRITE8_HANDLER( mapper40_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper40_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6000)
	{
		case 0x0000:
			state->IRQ_enable = 0;
			state->IRQ_count = 0;
			break;
		case 0x2000:
			state->IRQ_enable = 1;
			break;
		case 0x6000:
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper41_m_w, offset: %04x, data: %02x\n", offset, data));

	state->mmc_cmd1 = offset & 0xff;
	set_nt_mirroring(space->machine, BIT(data, 5) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
	prg32(space->machine, offset & 0x07);
}

static WRITE8_HANDLER( mapper41_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper41_w, offset: %04x, data: %02x\n", offset, data));

	if (state->mmc_cmd1 & 0x04)
		chr8(space->machine, ((state->mmc_cmd1 & 0x18) >> 1) | (data & 0x03), CHRROM);
}

/*************************************************************

    Mapper 42

    Known Boards: Unknown Bootleg Board
    Games: Mario Baby, Ai Senshi Nicol

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper42_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper42_w, offset: %04x, data: %02x\n", offset, data));

	if (offset >= 0x7000)
	{
		switch(offset & 0x03)
		{
		case 0x00:
			prg8_67(space->machine, data);
			break;
		case 0x01:
			set_nt_mirroring(space->machine, BIT(data, 3) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;
		case 0x02:
			/* Check if IRQ is being enabled */
			if (!state->IRQ_enable && (data & 0x02))
			{
				state->IRQ_enable = 1;
				timer_adjust_oneshot(state->irq_timer, cpu_clocks_to_attotime(state->maincpu, 24576), 0);
			}
			if (!(data & 0x02))
			{
				state->IRQ_enable = 0;
				timer_adjust_oneshot(state->irq_timer, attotime_never, 0);
			}
			break;
		}
	}
}

/*************************************************************

    Mapper 43

    Known Boards: Unknown Bootleg Board
    Games: Super Mario Bros. 2 Pirate (LF36)

    In MESS: Supported? The only image I found is not working
       (not even in NEStopia).

*************************************************************/

static WRITE8_HANDLER( mapper43_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	int bank = (((offset >> 8) & 0x03) * 0x20) + (offset & 0x1f);

	LOG_MMC(("mapper43_w, offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring(space->machine, (offset & 0x2000) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	if (offset & 0x0800)
	{
		if (offset & 0x1000)
		{
			if (bank * 2 >= state->prg_chunks)
			{
				memory_set_bankptr(space->machine, "bank3", state->wram);
				memory_set_bankptr(space->machine, "bank4", state->wram);
			}
			else
			{
				LOG_MMC(("mapper43_w, selecting upper 16KB bank of #%02x\n", bank));
				prg16_cdef(space->machine, 2 * bank + 1);
			}
		}
		else
		{
			if (bank * 2 >= state->prg_chunks)
			{
				memory_set_bankptr(space->machine, "bank1", state->wram);
				memory_set_bankptr(space->machine, "bank2", state->wram);
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
		if (bank * 2 >= state->prg_chunks)
		{
			memory_set_bankptr(space->machine, "bank1", state->wram);
			memory_set_bankptr(space->machine, "bank2", state->wram);
			memory_set_bankptr(space->machine, "bank3", state->wram);
			memory_set_bankptr(space->machine, "bank4", state->wram);
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 page;
	LOG_MMC(("mapper44_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x2001: /* $a001 - Select 128K ROM/VROM base (0..5) or last 256K ROM/VRAM base (6) */
		page = (data & 0x07);
		if (page > 6)
			page = 6;

		state->mmc_prg_base = page << 4;
		state->mmc_prg_mask = (page > 5) ? 0x1f : 0x0f;
		state->mmc_chr_base = page << 7;
		state->mmc_chr_mask = (page > 5) ? 0xff : 0x7f;
		mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
		mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper45_m_w, offset: %04x, data: %02x\n", offset, data));

	/* This bit is the "register lock". Once register are locked, writes go to WRAM
        and there is no way to unlock them (except by resetting the machine) */
	if (mapper45_reg[3] & 0x40)
		state->wram[offset] = data;
	else
	{
		mapper45_reg[state->mmc_count] = data;
		state->mmc_count = (state->mmc_count + 1) & 0x03;

		if (!state->mmc_count)
		{
			LOG_MMC(("mapper45_m_w, command completed %02x %02x %02x %02x\n", mapper45_reg[3],
				mapper45_reg[2], mapper45_reg[1], mapper45_reg[0]));

			state->mmc_prg_base = mapper45_reg[1];
			state->mmc_prg_mask = 0x3f ^ (mapper45_reg[3] & 0x3f);
			state->mmc_chr_base = ((mapper45_reg[2] & 0xf0) << 4) + mapper45_reg[0];
			state->mmc_chr_mask = 0xff >> (~mapper45_reg[2] & 0x0f);
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
		}
	}
}

/*************************************************************

    Mapper 46

    Known Boards: Rumblestation Board
    Games: Rumblestation 15 in 1

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper46_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper46_m_w, offset: %04x, data: %02x\n", offset, data));

	state->mmc_prg_bank[0] = (state->mmc_prg_bank[0] & 0x01) | ((data & 0x0f) << 1);
	state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & 0x07) | ((data & 0xf0) >> 1);
	prg32(space->machine, state->mmc_prg_bank[0]);
	chr8(space->machine, state->mmc_vrom_bank[0], CHRROM);
}

static WRITE8_HANDLER( mapper46_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper46_w, offset: %04x, data: %02x\n", offset, data));

	state->mmc_prg_bank[0] = (state->mmc_prg_bank[0] & ~0x01) | (data & 0x01);
	state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & ~0x07) | ((data & 0x70) >> 4);
	prg32(space->machine, state->mmc_prg_bank[0]);
	chr8(space->machine, state->mmc_vrom_bank[0], CHRROM);
}

/*************************************************************

    Mapper 47

    Known Boards: Custom QJ??
    Games: Super Spike V'Ball + Nintendo World Cup

    In MESS: Supported. It also uses mapper4_irq.

*************************************************************/

static WRITE8_HANDLER( mapper47_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper47_m_w, offset: %04x, data: %02x\n", offset, data));

	state->mmc_prg_base = (data & 0x01) << 4;
	state->mmc_prg_mask = 0x0f;
	state->mmc_chr_base = (data & 0x01) << 7;
	state->mmc_chr_mask = 0x7f;
	mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
	mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
}

/*************************************************************

    Mapper 48

    Known Boards: Taito TC0190FMC PAL16R4
    Games: Bakushou!! Jinsei Gekijou 3, Bubble Bobble 2,
          Captain Saver, Don Doko Don 2, Flintstones, Jetsons

    This is basically Mapper 33 + IRQ. Notably, IRQ works the
    same as MMC3 irq, BUT latch values are "inverted" (XOR'ed
    with 0xff) and there is a little delay (not implemented yet)
    We simply use MMC3 IRQ and XOR the value written in the
    register 0xc000 below

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper48_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper48_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6003)
	{
		case 0x0000:
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
		case 0x4000:
			state->IRQ_count_latch = (0x100 - data) & 0xff;
			break;
		case 0x4001:
			state->IRQ_count = state->IRQ_count_latch;
			break;
		case 0x4002:
			state->IRQ_enable = 1;
			break;
		case 0x4003:
			state->IRQ_enable = 0;
			break;
		case 0x6000:
			set_nt_mirroring(space->machine, BIT(data, 6) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;
	}
}

/*************************************************************

    Mapper 49

    Known Boards: Unknown Multigame Bootleg Board
    Games: Super HIK 4 in 1

    In MESS: Supported. It also uses mapper4_irq.

*************************************************************/

static WRITE8_HANDLER( mapper49_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper49_m_w, offset: %04x, data: %02x\n", offset, data));

	/* mid writes only work when WRAM is enabled. not sure if I should
       change the condition to state->mmc_cmd2==0x80 (i.e. what is the effect of
       the read-only bit?) */
	if (state->mmc_cmd2 & 0x80)
	{
		if (data & 0x01)	/* if this is 0, then we have 32k PRG blocks */
		{
			state->mmc_prg_base = (data & 0xc0) >> 2;
			state->mmc_prg_mask = 0x0f;
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
		}
		else
			prg32(space->machine, (data & 0x30) >> 4);

		state->mmc_chr_base = (data & 0xc0) << 1;
		state->mmc_chr_mask = 0x7f;
		mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
	}
}

/*************************************************************

    Mapper 50

    Known Boards: Unknown Bootleg Board
    Games: Super Mario Bros. 2 Pirate (Jpn version of SMB2)

    This was marked as Alt. Levels. is it true?

    In MESS: Supported.

*************************************************************/

static void mapper50_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;
	if (state->IRQ_enable)
	{
		if (state->IRQ_count < 0x1000)
		{
			if ((0x1000 - state->IRQ_count) <= 114)
				cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
			else
				state->IRQ_count += 114;
		}
		else
			state->IRQ_count += 114;

		state->IRQ_count &= 0xffff;	// according to docs is 16bit counter -> it wraps only after 0xffff
	}
}

static WRITE8_HANDLER( mapper50_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 prg;
	LOG_MMC(("mapper50_l_w, offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	switch (offset & 0x160)
	{
	case 0x020:
		prg = (data & 0x08) | ((data & 0x06) >> 1) | ((data & 0x01) << 2);
		prg8_cd(space->machine, prg);
		break;
	case 0x120:
		state->IRQ_enable = data & 0x01;
		break;
	}
}

/* This goes to 0x4020-0x403f */
WRITE8_HANDLER( nes_mapper50_add_w )
{
	UINT8 prg;
	LOG_MMC(("nes_mapper50_add_w, offset: %04x, data: %02x\n", offset, data));

	prg = (data & 0x08) | ((data & 0x06) >> 1) | ((data & 0x01) << 2);
	prg8_cd(space->machine, prg);
}

/*************************************************************

    Mapper 51

    Known Boards: Unknown Multigame Bootleg Board
    Games: 11 in 1 Ball Games

    In MESS: Partially Supported.

*************************************************************/

static void mapper51_set_banks( running_machine *machine )
{
	set_nt_mirroring(machine, (mapper51_reg[0] == 3) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	if (mapper51_reg[0] & 0x01)
	{
		prg32(machine, mapper51_reg[1]);
	}
	else
	{
		prg16_89ab(machine, (mapper51_reg[1] << 1) | (mapper51_reg[0] >> 1));
		prg16_cdef(machine, (mapper51_reg[1] << 1) | 0x07);
	}
}

static WRITE8_HANDLER( mapper51_m_w )
{
	LOG_MMC(("mapper51_m_w, offset: %04x, data: %02x\n", offset, data));

	mapper51_reg[0] = ((data >> 1) & 0x01) | ((data >> 3) & 0x02);
	mapper51_set_banks(space->machine);
}

static WRITE8_HANDLER( mapper51_w )
{
	LOG_MMC(("mapper51_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6000)
	{
	case 0x4000:	// here we also update reg[0] upper bit
		mapper51_reg[0] = (mapper51_reg[0] & 0x01) | ((data >> 3) & 0x02);
	case 0x0000:
	case 0x2000:
	case 0x6000:
		mapper51_reg[1] = data & 0x0f;
		mapper51_set_banks(space->machine);
		break;
	}
}

/*************************************************************

    Mapper 52

    Known Boards: Unknown Multigame Bootleg Board
    Games: Mario 7 in 1

    MMC3 clone

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper52_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 map52_helper1, map52_helper2;
	LOG_MMC(("mapper52_m_w, offset: %04x, data: %02x\n", offset, data));

	/* mid writes only work when WRAM is enabled. not sure if I should
       change the condition to state->mmc_cmd2==0x80 (i.e. what is the effect of
       the read-only bit?) and it only can happen once! */
	if ((state->mmc_cmd2 & 0x80) && !map52_reg_written)
	{
		map52_helper1 = (data & 0x08);
		map52_helper2 = (data & 0x40);

		state->mmc_prg_base = map52_helper1 ? ((data & 0x07) << 4) : ((data & 0x06) << 4);
		state->mmc_prg_mask = map52_helper1 ? 0x0f : 0x1f;
		state->mmc_chr_base = ((data & 0x20) << 4) | ((data & 0x04) << 6) | (map52_helper2 ? ((data & 0x10) << 3) : 0);
		state->mmc_chr_mask = map52_helper2 ? 0x7f : 0xff;
		mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
		mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);

		map52_reg_written = 1;
	}
	else
		state->wram[offset] = data;
}

/*************************************************************

    Mapper 53

    Known Boards: Unknown Multigame Bootleg Board
    Games: Supervision 16 in 1

    In MESS: Unsupported (SRAM banks can go in mid-regions).

*************************************************************/

/*************************************************************

    Mapper 54

    Known Boards: Unknown Multigame Bootleg Board
    Games: I only found 'Novel Diamond 999999-in-1.unf' using
        this mapper (hence the code is used for BMC_NOVELDIAMOND
        board). The code is included here in case a mapper 54
        dump arises.

    In MESS: Partial Support.

*************************************************************/

static WRITE8_HANDLER( mapper54_w )
{
	LOG_MMC(("mapper54_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, offset & 0x03);
	chr8(space->machine, offset & 0x07, CHRROM);
}

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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper57_w, offset: %04x, data: %02x\n", offset, data));

	if (offset & 0x0800)
		state->mmc_cmd2 = data;
	else
		state->mmc_cmd1 = data;

	if (state->mmc_cmd2 & 0x80)
		prg32(space->machine, 2 | (state->mmc_cmd2 >> 6));
	else
	{
		prg16_89ab(space->machine, (state->mmc_cmd2 >> 5) & 0x03);
		prg16_cdef(space->machine, (state->mmc_cmd2 >> 5) & 0x03);
	}

	set_nt_mirroring(space->machine, (state->mmc_cmd2 & 0x08) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	chr8(space->machine, (state->mmc_cmd1 & 0x03) | (state->mmc_cmd2 & 0x07) | ((state->mmc_cmd2 & 0x10) >> 1), CHRROM);
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
	set_nt_mirroring(space->machine, BIT(data, 7) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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

    In MESS: Unsupported (these are reset-based and have dips).

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
	set_nt_mirroring(space->machine, BIT(data, 7) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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

	set_nt_mirroring(space->machine, BIT(data, 7) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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

    In MESS: Partially Supported. It also uses mapper4_irq.

*************************************************************/

static void mapper64_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;
	if (!state->IRQ_mode)	// we are in scanline mode!
	{
		if (scanline < PPU_BOTTOM_VISIBLE_SCANLINE)
		{
			if (!state->IRQ_reset)
			{
				if (!state->IRQ_count)
					state->IRQ_count = state->IRQ_count_latch;
				else
				{
					state->IRQ_count--;
					if (state->IRQ_enable && !blanked && !state->IRQ_count)
					{
						LOG_MMC(("irq fired, scanline: %d (MAME %d, beam pos: %d)\n", scanline,
								video_screen_get_vpos(device->machine->primary_screen), video_screen_get_hpos(device->machine->primary_screen)));
						cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
					}
				}
			}
			else
			{
				state->IRQ_reset = 0;
				state->IRQ_count = state->IRQ_count_latch + 1;
			}
		}
	}
	/* otherwise, we are in CPU cycles mode --> decrement count of 114 every scanline
     --> in the meanwhile anything can have happened to IRQ_reset and we would not know
     --> Skulls and Crossbones does not show anything!! */
	else
	{
//      if (!state->IRQ_reset)
		{
			if (state->IRQ_count <= 114)
				state->IRQ_count = state->IRQ_count_latch;
			else
			{
				state->IRQ_count -= 114;
				if (state->IRQ_enable && !blanked && (state->IRQ_count <= 114))
				{
					LOG_MMC(("irq fired, scanline: %d (MAME %d, beam pos: %d)\n", scanline,
							video_screen_get_vpos(device->machine->primary_screen), video_screen_get_hpos(device->machine->primary_screen)));
					cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
				}
			}
		}
//      else
//      {
//          state->IRQ_reset = 0;
//          state->IRQ_count = state->IRQ_count_latch + 1;
//      }
	}
}

static void mapper64_set_prg( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 prg_mode = state->mmc_cmd1 & 0x40;

	prg8_89(machine, state->mmc_prg_bank[prg_mode ? 2: 0]);
	prg8_ab(machine, state->mmc_prg_bank[prg_mode ? 0: 1]);
	prg8_cd(machine, state->mmc_prg_bank[prg_mode ? 1: 2]);
}

static void mapper64_set_chr( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;

	if (state->mmc_cmd1 & 0x20)
	{
		chr1_x(machine, 0 ^ chr_page, state->mmc_vrom_bank[0], CHRROM);
		chr1_x(machine, 1 ^ chr_page, state->mmc_vrom_bank[8], CHRROM);
		chr1_x(machine, 2 ^ chr_page, state->mmc_vrom_bank[1], CHRROM);
		chr1_x(machine, 3 ^ chr_page, state->mmc_vrom_bank[9], CHRROM);
	}
	else
	{
		chr1_x(machine, 0 ^ chr_page, state->mmc_vrom_bank[0] & ~0x01, CHRROM);
		chr1_x(machine, 1 ^ chr_page, state->mmc_vrom_bank[0] |  0x01, CHRROM);
		chr1_x(machine, 2 ^ chr_page, state->mmc_vrom_bank[1] & ~0x01, CHRROM);
		chr1_x(machine, 3 ^ chr_page, state->mmc_vrom_bank[1] |  0x01, CHRROM);
	}

	chr1_x(machine, 4 ^ chr_page, state->mmc_vrom_bank[2], CHRROM);
	chr1_x(machine, 5 ^ chr_page, state->mmc_vrom_bank[3], CHRROM);
	chr1_x(machine, 6 ^ chr_page, state->mmc_vrom_bank[4], CHRROM);
	chr1_x(machine, 7 ^ chr_page, state->mmc_vrom_bank[5], CHRROM);
}

static WRITE8_HANDLER( mapper64_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 map64_helper, cmd;
	LOG_MMC(("mapper64_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
		case 0x0000:
			map64_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (map64_helper & 0x40)
				mapper64_set_prg(space->machine);

			/* Has CHR Mode changed? */
			if (map64_helper & 0xa0)
				mapper64_set_chr(space->machine);
			break;

		case 0x0001:
			cmd = state->mmc_cmd1 & 0x0f;
			switch (cmd)
			{
				case 0: case 1:
				case 2: case 3:
				case 4: case 5:
					state->mmc_vrom_bank[cmd] = data;
					mapper64_set_chr(space->machine);
					break;
				case 6: case 7:
					state->mmc_prg_bank[cmd - 6] = data;
					mapper64_set_prg(space->machine);
					break;
				case 8: case 9:
					state->mmc_vrom_bank[cmd - 2] = data;
					mapper64_set_chr(space->machine);
					break;
				case 0x0f:
					state->mmc_prg_bank[2] = data;
					mapper64_set_prg(space->machine);
					break;
			}
			break;

		case 0x2000:
			set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;

		case 0x4000:
			state->IRQ_count_latch = data;
			break;

		case 0x4001: /* $c001 - IRQ scanline latch */
			state->IRQ_mode = data & 0x01;
			state->IRQ_reset = 1;
			break;

		case 0x6000:
			state->IRQ_enable = 0;
			break;

		case 0x6001:
			state->IRQ_enable = 1;
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

/* Here, IRQ counter decrements every CPU cycle. Since we update it every scanline,
we need to decrement it by 114 (Each scanline consists of 341 dots and, on NTSC,
there are 3 dots to every 1 CPU cycle, hence 114 is the number of cycles per scanline ) */
static void irem_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;
	if (state->IRQ_enable)
	{
		state->IRQ_count -= 114;

		if (state->IRQ_count <= 114)
		{
			state->IRQ_enable = 0;
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
		}
	}
}

static WRITE8_HANDLER( mapper65_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper65_w, offset %04x, data: %02x\n", offset, data));

	switch (offset & 0x7fff)
	{
		case 0x0000:
			prg8_89(space->machine, data);
			break;

		case 0x1001:
			set_nt_mirroring(space->machine, BIT(data, 7) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;

		case 0x1003:
			state->IRQ_enable = data & 0x80;
			break;

		case 0x1004:
			state->IRQ_count = state->IRQ_count_latch;
			break;

		case 0x1005:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0x00ff) | (data << 8);
			break;

		case 0x1006:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0xff00) | data;
			break;

		case 0x2000:
			prg8_ab(space->machine, data);
			break;

		case 0x3000:
		case 0x3001:
		case 0x3002:
		case 0x3003:
		case 0x3004:
		case 0x3005:
		case 0x3006:
		case 0x3007:
			chr1_x(space->machine, offset & 0x07, data, CHRROM);
			break;

		case 0x4000:
			prg8_cd(space->machine, data);
			break;

		default:
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

    In MESS: Supported.

*************************************************************/

/* Here, IRQ counter decrements every CPU cycle. Since we update it every scanline,
we need to decrement it by 114 (Each scanline consists of 341 dots and, on NTSC,
there are 3 dots to every 1 CPU cycle, hence 114 is the number of cycles per scanline ) */
static void mapper67_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;

	/* TODO: change to reflect the actual number of cycles spent: both using 114 or cycling 114,114,113
    produces a 1-line glitch in Fantasy Zone 2: it really requires the counter to be updated each CPU cycle! */
	if (state->IRQ_enable)
	{
		if (state->IRQ_count <= 114)
		{
			state->IRQ_enable = 0;
			state->IRQ_count = 0xffff;
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
		}
		else
			state->IRQ_count -= 114;
	}
}

static WRITE8_HANDLER( mapper67_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper67_w, offset %04x, data: %02x\n", offset, data));

	switch (offset & 0x7800)
	{
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
	case 0x4000:
	case 0x4800:
		state->IRQ_toggle ^= 1;
		if (state->IRQ_toggle)
			state->IRQ_count = (state->IRQ_count & 0x00ff) | (data << 8);
		else
			state->IRQ_count = (state->IRQ_count & 0xff00) | data;
		break;
	case 0x5800:
		state->IRQ_enable = data & 0x10;
		state->IRQ_toggle = 0;
		break;
	case 0x6800:
		switch (data & 3)
		{
			case 0x00: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
			case 0x01: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
			case 0x02: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
			case 0x03: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
		}
		break;
	case 0x7800:
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

static void mapper68_mirror( running_machine *machine, int mirror, int mirr0, int mirr1 )
{
	switch (mirror)
	{
		case 0x00:
			set_nt_mirroring(machine, PPU_MIRROR_HORZ);
			break;
		case 0x01:
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
		case 0x02:
			set_nt_mirroring(machine, PPU_MIRROR_LOW);
			break;
		case 0x03:
			set_nt_mirroring(machine, PPU_MIRROR_HIGH);
			break;
		case 0x10:
			set_nt_page(machine, 0, ROM, mirr0 | 0x80, 0);
			set_nt_page(machine, 1, ROM, mirr1 | 0x80, 0);
			set_nt_page(machine, 2, ROM, mirr0 | 0x80, 0);
			set_nt_page(machine, 3, ROM, mirr1 | 0x80, 0);
			break;
		case 0x11:
			set_nt_page(machine, 0, ROM, mirr0 | 0x80, 0);
			set_nt_page(machine, 1, ROM, mirr0 | 0x80, 0);
			set_nt_page(machine, 2, ROM, mirr1 | 0x80, 0);
			set_nt_page(machine, 3, ROM, mirr1 | 0x80, 0);
			break;
		case 0x12:
			set_nt_page(machine, 0, ROM, mirr0 | 0x80, 0);
			set_nt_page(machine, 1, ROM, mirr0 | 0x80, 0);
			set_nt_page(machine, 2, ROM, mirr0 | 0x80, 0);
			set_nt_page(machine, 3, ROM, mirr0 | 0x80, 0);
			break;
		case 0x13:
			set_nt_page(machine, 0, ROM, mirr1 | 0x80, 0);
			set_nt_page(machine, 1, ROM, mirr1 | 0x80, 0);
			set_nt_page(machine, 2, ROM, mirr1 | 0x80, 0);
			set_nt_page(machine, 3, ROM, mirr1 | 0x80, 0);
			break;
	}
}

static WRITE8_HANDLER( mapper68_w )
{
	LOG_MMC(("mapper68_w, offset %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
		case 0x0000:
			chr2_0(space->machine, data, CHRROM);
			break;
		case 0x1000:
			chr2_2(space->machine, data, CHRROM);
			break;
		case 0x2000:
			chr2_4(space->machine, data, CHRROM);
			break;
		case 0x3000:
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
    Games: Barcode World, Batman - Return of the Joker, Gimmick!

    In MESS: Supported.

*************************************************************/

/* Here, IRQ counter decrements every CPU cycle. Since we update it every scanline,
we need to decrement it by 114 (Each scanline consists of 341 dots and, on NTSC,
there are 3 dots to every 1 CPU cycle, hence 114 is the number of cycles per scanline ) */
static void mapper69_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;

	/* TODO: change to reflect the actual number of cycles spent */
	if ((state->IRQ_enable & 0x80) && (state->IRQ_enable & 0x01))
	{
		if (state->IRQ_count <= 114)
		{
			state->IRQ_count = 0xffff;
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
		}
		else
			state->IRQ_count -= 114;
	}
	else if (state->IRQ_enable & 0x01)	// if enable bit is not set, only decrement the counter!
	{
		if (state->IRQ_count <= 114)
			state->IRQ_count = 0xffff;
		else
			state->IRQ_count -= 114;
	}
}

static WRITE8_HANDLER( mapper69_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper69_w, offset %04x, data: %02x\n", offset, data));

	switch (offset & 0x6000)
	{
		case 0x0000:
			state->mmc_cmd1 = data & 0x0f;
			break;

		case 0x2000:
			switch (state->mmc_cmd1)
			{
				case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
					chr1_x(space->machine, state->mmc_cmd1, data, CHRROM);
					break;

				/* TODO: deal properly with bankswitching/write-protecting the mid-mapper area */
				case 8:
					if (!(data & 0x40))
					{
						state->mid_ram_enable = 0;	// is PRG ROM
						prg8_67(space->machine, data & 0x3f);
					}
					else if (data & 0x80)
					{
						state->mid_ram_enable = 1;	// is PRG RAM
						state->prg_bank[4] = state->prg_chunks + (data & 0x3f);
						memory_set_bank(space->machine, "bank5", state->prg_bank[4]);
					}
					break;

				case 9:
					prg8_89(space->machine, data);
					break;
				case 0x0a:
					prg8_ab(space->machine, data);
					break;
				case 0x0b:
					prg8_cd(space->machine, data);
					break;
				case 0x0c:
					switch (data & 0x03)
					{
						case 0x00: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
						case 0x01: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
						case 0x02: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
						case 0x03: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
					}
					break;
				case 0x0d:
					state->IRQ_enable = data;
					break;
				case 0x0e:
					state->IRQ_count = (state->IRQ_count & 0xff00) | data;
					break;
				case 0x0f:
					state->IRQ_count = (state->IRQ_count & 0x00ff) | (data << 8);
					break;
			}
			break;

		/* Here we would have sound command for Sunsoft 5b variant */
//      case 0x4000:
//      case 0x6000:

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

    To emulate NT mirroring for BF9097 board (missing in BF9093)
    we use crc_hack, however Fire Hawk is broken (but without
    mirroring there would be no helicopter graphics).

    In MESS: Partially Supported.

*************************************************************/

static WRITE8_HANDLER( mapper71_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper71_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
	case 0x0000:
	case 0x1000:
		if (state->crc_hack)
			set_nt_mirroring(space->machine, BIT(data, 4) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
		break;
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
		prg16_89ab(space->machine, data);
		break;
	}
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
	nes_state *state = (nes_state *)space->machine->driver_data;

	switch (offset & 0x7000)
	{
		case 0x0000:
		case 0x1000:
			/* dunno which address controls these */
			state->IRQ_count_latch = data;
			state->IRQ_enable_latch = data;
			break;
		case 0x2000:
			state->IRQ_enable = data;
			break;
		case 0x3000:
			state->IRQ_count &= ~0x0f;
			state->IRQ_count |= data & 0x0f;
			break;
		case 0x4000:
			state->IRQ_count &= ~0xf0;
			state->IRQ_count |= (data & 0x0f) << 4;
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

    This mapper is quite similar to MMC3, but with two differences:
    mirroring is not the same, and when VROM banks 8,9 are accessed
    they point to CHRRAM and not CHRROM.

    In MESS: Supported

*************************************************************/

/* MIRROR_LOW and MIRROR_HIGH are swapped! */
static void waixing_set_mirror( running_machine *machine, UINT8 nt )
{
	switch (nt)
	{
	case 0:
	case 1:
		set_nt_mirroring(machine, nt ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 2:
		set_nt_mirroring(machine, PPU_MIRROR_LOW);
		break;
	case 3:
		set_nt_mirroring(machine, PPU_MIRROR_HIGH);
		break;
	default:
		LOG_MMC(("Mapper set NT to invalid value %02x", nt));
		break;
	}
}

static void mapper74_set_chr( running_machine *machine, int chr_base, int chr_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 chr_src[6];
	int i;

	for (i = 0; i < 6; i++)
		chr_src[i] = ((state->mmc_vrom_bank[i] == 8) || (state->mmc_vrom_bank[i] == 9)) ? CHRRAM : CHRROM;

	chr1_x(machine, chr_page ^ 0, chr_base | ((state->mmc_vrom_bank[0] & ~0x01) & chr_mask), chr_src[0]);
	chr1_x(machine, chr_page ^ 1, chr_base | ((state->mmc_vrom_bank[0] |  0x01) & chr_mask), chr_src[0]);
	chr1_x(machine, chr_page ^ 2, chr_base | ((state->mmc_vrom_bank[1] & ~0x01) & chr_mask), chr_src[1]);
	chr1_x(machine, chr_page ^ 3, chr_base | ((state->mmc_vrom_bank[1] |  0x01) & chr_mask), chr_src[1]);
	chr1_x(machine, chr_page ^ 4, chr_base | (state->mmc_vrom_bank[2] & chr_mask), chr_src[2]);
	chr1_x(machine, chr_page ^ 5, chr_base | (state->mmc_vrom_bank[3] & chr_mask), chr_src[3]);
	chr1_x(machine, chr_page ^ 6, chr_base | (state->mmc_vrom_bank[4] & chr_mask), chr_src[4]);
	chr1_x(machine, chr_page ^ 7, chr_base | (state->mmc_vrom_bank[5] & chr_mask), chr_src[5]);
}

static WRITE8_HANDLER( mapper74_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper74_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0000:
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has PRG Mode changed? */
		if (MMC3_helper & 0x40)
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper74_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
		break;

	case 0x0001:
		cmd = state->mmc_cmd1 & 0x07;
		switch (cmd)
		{
		case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
		case 2: case 3: case 4: case 5:
			state->mmc_vrom_bank[cmd] = data;
			mapper74_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 6:
		case 7:
			state->mmc_prg_bank[cmd - 6] = data;
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			break;
		}
		break;

	case 0x2000:
		waixing_set_mirror(space->machine, data);	//maybe data & 0x03?
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper75_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
		case 0x0000:
			prg8_89(space->machine, data);
			break;
		case 0x1000:
			set_nt_mirroring(space->machine, (data & 0x01) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & 0x0f) | ((data & 0x02) << 3);
			state->mmc_vrom_bank[1] = (state->mmc_vrom_bank[1] & 0x0f) | ((data & 0x04) << 2);
			chr4_0(space->machine, state->mmc_vrom_bank[0], CHRROM);
			chr4_4(space->machine, state->mmc_vrom_bank[1], CHRROM);
			break;
		case 0x2000:
			prg8_ab(space->machine, data);
			break;
		case 0x4000:
			prg8_cd(space->machine, data);
			break;
		case 0x6000:
			state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & 0x10) | (data & 0x0f);
			chr4_0(space->machine, state->mmc_vrom_bank[0], CHRROM);
			break;
		case 0x7000:
			state->mmc_vrom_bank[1] = (state->mmc_vrom_bank[1] & 0x10) | (data & 0x0f);
			chr4_4(space->machine, state->mmc_vrom_bank[1], CHRROM);
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper76_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7001)
	{
	case 0:
		state->mmc_cmd1 = data;
		break;
	case 1:
		{
			switch (state->mmc_cmd1 & 0x07)
			{
			case 2: chr2_0(space->machine, data, CHRROM); break;
			case 3: chr2_2(space->machine, data, CHRROM); break;
			case 4: chr2_4(space->machine, data, CHRROM); break;
			case 5: chr2_6(space->machine, data, CHRROM); break;
			case 6:
				(state->mmc_cmd1 & 0x40) ? prg8_cd(space->machine, data) : prg8_89(space->machine, data);
				break;
			case 7: prg8_ab(space->machine, data); break;
			default: logerror("mapper76 unsupported command: %02x, data: %02x\n", state->mmc_cmd1, data); break;
			}
		}
	case 0x2000:
		set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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

    In MESS: Partially Supported (it needs 4-screen mirroring).

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

    A crc check is required to support Cosmo Carrier (which uses
    a slightly different board)

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper78_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper78_w, offset: %04x, data: %02x\n", offset, data));

	if (state->crc_hack)		// Jaleco JF16 board has different mirroring even if uses same mapper number :(
		set_nt_mirroring(space->machine, BIT(data, 3) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
	else
		set_nt_mirroring(space->machine, BIT(data, 3) ? PPU_MIRROR_VERT : PPU_MIRROR_HORZ);

	chr8(space->machine, (data & 0xf0) >> 4, CHRROM);
	prg16_89ab(space->machine, data & 0x07);
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
			chr2_0(space->machine, (data & 0x7f) >> 1, CHRROM);
			break;
		case 0x1ef1:
			chr2_2(space->machine, (data & 0x7f) >> 1, CHRROM);
			break;
		case 0x1ef2:
			chr1_4(space->machine, data, CHRROM);
			break;
		case 0x1ef3:
			chr1_5(space->machine, data, CHRROM);
			break;
		case 0x1ef4:
			chr1_6(space->machine, data, CHRROM);
			break;
		case 0x1ef5:
			chr1_7(space->machine, data, CHRROM);
			break;
		case 0x1ef6:
			set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_VERT : PPU_MIRROR_HORZ);
			break;
		case 0x1efa:
		case 0x1efb:
			prg8_89(space->machine, data);
			break;
		case 0x1efc:
		case 0x1efd:
			prg8_ab(space->machine, data);
			break;
		case 0x1efe:
		case 0x1eff:
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

    Known Boards: Taito X1-017
    Games: Kyuukyoku Harikiri Koushien, Kyuukyoku Harikiri
          Stadium, SD Keiji - Blader

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper82_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper82_m_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
		case 0x1ef0:
			if (state->mmc_cmd1)
				chr2_4(space->machine, data >> 1, CHRROM);
			else
				chr2_0(space->machine, data >> 1, CHRROM);
			break;
		case 0x1ef1:
			if (state->mmc_cmd1)
				chr2_6(space->machine, data, CHRROM);
			else
				chr2_2(space->machine, data, CHRROM);
			break;
		case 0x1ef2:
			chr1_x(space->machine, 4 ^ state->mmc_cmd1, data, CHRROM);
			break;
		case 0x1ef3:
			chr1_x(space->machine, 5 ^ state->mmc_cmd1, data, CHRROM);
			break;
		case 0x1ef4:
			chr1_x(space->machine, 6 ^ state->mmc_cmd1, data, CHRROM);
			break;
		case 0x1ef5:
			chr1_x(space->machine, 7 ^ state->mmc_cmd1, data, CHRROM);
			break;
		case 0x1ef6:
			set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_VERT : PPU_MIRROR_HORZ);
			state->mmc_cmd1 = ((data & 0x02) << 1);
			break;

		case 0x1efa:
			prg8_89(space->machine, data >> 2);
			break;
		case 0x1efb:
			prg8_ab(space->machine, data >> 2);
			break;
		case 0x1efc:
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
	mapper83_low_reg[offset & 0x03] = data;
}

static READ8_HANDLER( mapper83_l_r )
{
	return mapper83_low_reg[offset & 0x03];
}

static void mapper83_set_prg( running_machine *machine )
{
	prg16_89ab(machine, mapper83_reg[8] & 0x3f);
	prg16_cdef(machine, (mapper83_reg[8] & 0x30) | 0x0f);
}

static void mapper83_set_chr( running_machine *machine )
{
	chr1_0(machine, mapper83_reg[0] | ((mapper83_reg[8] & 0x30) << 4), CHRROM);
	chr1_1(machine, mapper83_reg[1] | ((mapper83_reg[8] & 0x30) << 4), CHRROM);
	chr1_2(machine, mapper83_reg[2] | ((mapper83_reg[8] & 0x30) << 4), CHRROM);
	chr1_3(machine, mapper83_reg[3] | ((mapper83_reg[8] & 0x30) << 4), CHRROM);
	chr1_4(machine, mapper83_reg[4] | ((mapper83_reg[8] & 0x30) << 4), CHRROM);
	chr1_5(machine, mapper83_reg[5] | ((mapper83_reg[8] & 0x30) << 4), CHRROM);
	chr1_6(machine, mapper83_reg[6] | ((mapper83_reg[8] & 0x30) << 4), CHRROM);
	chr1_7(machine, mapper83_reg[7] | ((mapper83_reg[8] & 0x30) << 4), CHRROM);
}

static WRITE8_HANDLER( mapper83_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper83_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
	case 0x0000:
	case 0x3000:
	case 0x30ff:
	case 0x31ff:
		mapper83_reg[8] = data;
		mapper83_set_prg(space->machine);
		mapper83_set_chr(space->machine);
		break;
	case 0x0100:
		switch (data & 0x03)
		{
		case 0:
			set_nt_mirroring(space->machine, PPU_MIRROR_VERT);
			break;
		case 1:
			set_nt_mirroring(space->machine, PPU_MIRROR_HORZ);
			break;
		case 2:
			set_nt_mirroring(space->machine, PPU_MIRROR_LOW);
			break;
		case 3:
			set_nt_mirroring(space->machine, PPU_MIRROR_HIGH);
			break;
		}
		break;
	case 0x0200:
		state->IRQ_count = (state->IRQ_count & 0xff00) | data;
		break;
	case 0x0201:
		state->IRQ_enable = 1;
		state->IRQ_count = (data << 8) | (state->IRQ_count & 0xff);
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
		mapper83_reg[offset - 0x0310] = data;
		mapper83_set_chr(space->machine);
		break;
	case 0x0318:
		mapper83_reg[9] = data;
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 bank;
	LOG_MMC(("konami_vrc7_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7018)
	{
		case 0x0000:
			prg8_89(space->machine, data);
			break;
		case 0x0008:
		case 0x0010:
		case 0x0018:
			prg8_ab(space->machine, data);
			break;

		case 0x1000:
			prg8_cd(space->machine, data);
			break;

/* TODO: there are sound regs in here */

		case 0x2000:
		case 0x2008:
		case 0x2010:
		case 0x2018:
		case 0x3000:
		case 0x3008:
		case 0x3010:
		case 0x3018:
		case 0x4000:
		case 0x4008:
		case 0x4010:
		case 0x4018:
		case 0x5000:
		case 0x5008:
		case 0x5010:
		case 0x5018:
			bank = ((offset & 0x7000) - 0x2000) / 0x0800 + ((offset & 0x0018) ? 1 : 0);
			chr1_x(space->machine, bank, data, state->mmc_chr_source);
			break;

		case 0x6000:
			switch (data & 0x03)
			{
				case 0x00: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
				case 0x01: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
				case 0x02: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
				case 0x03: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
			}
			break;
		case 0x6008: case 0x6010: case 0x6018:
			state->IRQ_count_latch = data;
			break;
		case 0x7000:
			state->IRQ_mode = data & 0x04;	// currently not implemented: 0 = prescaler mode / 1 = CPU mode
			state->IRQ_enable = data & 0x02;
			state->IRQ_enable_latch = data & 0x01;
			if (data & 0x02)
				state->IRQ_count = state->IRQ_count_latch;
			break;
		case 0x7008: case 0x7010: case 0x7018:
			state->IRQ_enable = state->IRQ_enable_latch;
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper88_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 1)
	{
	case 0:
		state->mmc_cmd1 = data & 7;
		break;
	case 1:
		switch (state->mmc_cmd1)
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
	set_nt_mirroring(space->machine, BIT(data, 3) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
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

    This board uses an IRQ system very similar to MMC3. We indeed
    use mapper4_irq, but there is some small glitch!

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper91_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper91_m_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x1000)
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
			switch (offset & 0x03)
			{
				case 0x00: prg8_89(space->machine, data); break;
				case 0x01: prg8_ab(space->machine, data); break;
				case 0x02: state->IRQ_enable = 0; state->IRQ_count = 0; break;
				case 0x03: state->IRQ_enable = 1; state->IRQ_count = 7; break;
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

static WRITE8_HANDLER( mapper95_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper95_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 1)
	{
	case 0:
		{
			state->mmc_cmd1 = data;
			break;
		}
	case 1:
		{
			switch (state->mmc_cmd1 & 0x07)
			{
				case 0:
				case 1:
					chr2_x(space->machine, state->mmc_cmd1 ? 2 : 0, (data >> 1) & 0x1f, CHRROM);
					break;
				case 2:
				case 3:
				case 4:
				case 5:
					chr1_x(space->machine, 2 + state->mmc_cmd1, data & 0x1f, CHRROM);
					mapper95_reg[state->mmc_cmd1 - 2] = data & 0x20;
					if (!(state->mmc_cmd1 & 0x80))
					{
						set_nt_page(space->machine, 0, CIRAM, mapper95_reg[0] ? 1 : 0, 1);
						set_nt_page(space->machine, 1, CIRAM, mapper95_reg[1] ? 1 : 0, 1);
						set_nt_page(space->machine, 2, CIRAM, mapper95_reg[2] ? 1 : 0, 1);
						set_nt_page(space->machine, 3, CIRAM, mapper95_reg[3] ? 1 : 0, 1);
					}
					else
						set_nt_mirroring(space->machine, PPU_MIRROR_HORZ);
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
		set_nt_mirroring(space->machine, BIT(data, 7) ? PPU_MIRROR_VERT : PPU_MIRROR_HORZ);
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper104_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x4000)
	{
		if (data & 0x08)
		{
			state->mmc_prg_bank[0] = ((data & 0x07) << 4) | (state->mmc_prg_bank[0] & 0x0f);
			prg16_89ab(space->machine, state->mmc_prg_bank[0]);
			prg16_cdef(space->machine, ((data & 0x07) << 4) | 0x0f);
		}

	}
	else
	{
			state->mmc_prg_bank[0] = (state->mmc_prg_bank[0] & 0x70) | (data & 0x0f);
			prg16_89ab(space->machine, state->mmc_prg_bank[0]);
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

    In MESS: Supported.

*************************************************************/

static void mapper106_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;

	if (state->IRQ_enable)
	{
		if ((0xffff - state->IRQ_count) < 114)
		{
			cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
			state->IRQ_enable = 0;
		}

		state->IRQ_count = (state->IRQ_count + 114) & 0xffff;
	}
}

static WRITE8_HANDLER( mapper106_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper106_w, offset: %04x, data: %02x\n", offset, data));
	switch (offset & 0x0f)
	{
	case 0x00:
	case 0x02:
		chr1_x(space->machine, offset & 0x07, data & 0xfe, CHRROM);
		break;
	case 0x01:
	case 0x03:
		chr1_x(space->machine, offset & 0x07, data | 0x01, CHRROM);
		break;
	case 0x04: case 0x05:
	case 0x06: case 0x07:
		chr1_x(space->machine, offset & 0x07, data, CHRROM);
		break;
	case 0x08:
		prg8_89(space->machine, data | 0x10);
		break;
	case 0x09:
		prg8_ab(space->machine, data);
		break;
	case 0x0a:
		prg8_cd(space->machine, data);
		break;
	case 0x0b:
		prg8_ef(space->machine, data | 0x10);
		break;
	case 0x0c:
		set_nt_mirroring(space->machine, BIT(data, 0) ?  PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 0x0d:
		state->IRQ_count = 0;
		state->IRQ_enable = 0;
		break;
	case 0x0e:
		state->IRQ_count = (state->IRQ_count & 0xff00) | data;
		break;
	case 0x0f:
		state->IRQ_count = (state->IRQ_count & 0x00ff) | (data << 8);
		state->IRQ_enable = 1;
		break;
	}
}

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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper112_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
	case 0x0000:
		state->mmc_cmd1 = data & 0x07;
		break;
	case 0x2000:
		switch (state->mmc_cmd1)
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
		set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper113_w, offset: %04x, data: %02x\n", offset, data));

	if (!(offset & 0x100))
	{
		prg32(space->machine, (data & 0x38) >> 3);
		chr8(space->machine, (data & 0x07) | ((data & 0x40) >> 3), CHRROM);
		if (state->crc_hack)	// this breaks AV Soccer but it is needed by HES 6-in-1!
			set_nt_mirroring(space->machine, BIT(data, 7) ? PPU_MIRROR_VERT : PPU_MIRROR_HORZ);
	}
}

/*************************************************************

    Mapper 114

    Known Boards: Unknown Bootleg Board
    Games: The Lion King

    MMC3 clone.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper114_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper114_m_w, offset: %04x, data: %02x\n", offset, data));

	map114_reg = data;

	if (map114_reg & 0x80)
	{
		prg16_89ab(space->machine, data & 0x1f);
		prg16_cdef(space->machine, data & 0x1f);
	}
	else
		mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

}

static WRITE8_HANDLER( mapper114_w )
{
	static const UINT8 conv_table[8] = {0, 3, 1, 5, 6, 7, 2, 4};
	LOG_MMC(("mapper114_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x6000)
	{
		switch (offset & 0x6000)
		{
		case 0x0000:
			set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;
		case 0x2000:
			map114_reg_enabled = 1;
			data = (data & 0xc0) | conv_table[data & 0x07];
			mapper4_w(space, 0x8000, data);
			break;
		case 0x4000:
			if (map114_reg_enabled && (map114_reg & 0x80) == 0)
			{
				map114_reg_enabled = 0;
				mapper4_w(space, 0x8001, data);
			}
			break;
		}
	}
	else
	{
		switch (offset & 0x03)
		{
		case 0x02:
			mapper4_w(space, 0xe000, data);
			break;
		case 0x03:
			mapper4_w(space, 0xe001, data);
			mapper4_w(space, 0xc000, data);
			mapper4_w(space, 0xc001, data);
			break;
		}
	}
}

/*************************************************************

    Mapper 115 (formerly 248)

    Known Boards: Unknown Bootleg Board by Kasing
    Games: AV Jiu Ji Mahjong, Bao Qing Tian, Thunderbolt 2,
          Shisen Mahjong 2

    MMC3 clone

    In MESS: Supported

*************************************************************/

static void mapper115_set_prg( running_machine *machine, int prg_base, int prg_mask  )
{
	if (mapper115_reg[0] & 0x80)
		prg32(machine, mapper115_reg[0] >> 1);
	else
		mapper4_set_prg(machine, prg_base, prg_mask);
}

static WRITE8_HANDLER( mapper115_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper115_m_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x01)
	{
	case 0x00:
		mapper115_reg[0] = data;
		mapper115_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
		break;
	case 0x01:
		state->mmc_chr_base = (data & 0x01) ? 0x100 : 0x000;
		mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
		break;
	}
}

static WRITE8_HANDLER( mapper115_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 map115_helper, cmd;
	LOG_MMC(("mapper115_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
		case 0x0000:
			map115_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (map115_helper & 0x40)
				mapper115_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

			/* Has CHR Mode changed? */
			if (map115_helper & 0x80)
				mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;

		case 0x0001:
			cmd = state->mmc_cmd1 & 0x07;
			switch (cmd)
			{
				case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
				case 2: case 3: case 4: case 5:
					state->mmc_vrom_bank[cmd] = data;
					mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
					break;
				case 6:
				case 7:
					state->mmc_prg_bank[cmd - 6] = data;
					mapper115_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
					break;
			}
			break;

		default:
			mapper4_w(space, offset, data);
			break;
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

static void mapper117_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;
//  if (scanline < PPU_BOTTOM_VISIBLE_SCANLINE)
	{
		if (state->IRQ_enable && state->IRQ_count)
		{
			state->IRQ_count--;
			if (!state->IRQ_count)
				cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
		}
	}
}

static WRITE8_HANDLER( mapper117_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper117_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
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
	case 0x2000:
	case 0x2001:
	case 0x2002:
	case 0x2003:
	case 0x2004:
	case 0x2005:
	case 0x2006:
	case 0x2007:
		chr1_x(space->machine, offset & 0x07, data, CHRROM);
		break;

	case 0x5000:
		set_nt_mirroring(space->machine, BIT(data, 0) ?  PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;

	case 0x4001:
		state->IRQ_count_latch = data;
		break;
	case 0x4002:
		// IRQ cleared
		break;
	case 0x4003:
		state->IRQ_count = state->IRQ_count_latch;
		break;
	case 0x6000:
		state->IRQ_enable = data & 0x01;
		break;
	}
}

/*************************************************************

    Mapper 118

    Known Boards: TKSROM, TLSROM
    Games: Armadillo, Play Action Football, Pro Hockey, RPG
          Jinsei Game, Y's 3

    In MESS: Supported. It also uses mapper4_irq.

*************************************************************/

static void mapper118_set_mirror( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	if (state->mmc_cmd1 & 0x80)
	{
		set_nt_page(machine, 0, CIRAM, (state->mmc_vrom_bank[2] & 0x80) >> 7, 1);
		set_nt_page(machine, 1, CIRAM, (state->mmc_vrom_bank[3] & 0x80) >> 7, 1);
		set_nt_page(machine, 2, CIRAM, (state->mmc_vrom_bank[4] & 0x80) >> 7, 1);
		set_nt_page(machine, 3, CIRAM, (state->mmc_vrom_bank[5] & 0x80) >> 7, 1);
	}
	else
	{
		set_nt_page(machine, 0, CIRAM, (state->mmc_vrom_bank[0] & 0x80) >> 7, 1);
		set_nt_page(machine, 1, CIRAM, (state->mmc_vrom_bank[0] & 0x80) >> 7, 1);
		set_nt_page(machine, 2, CIRAM, (state->mmc_vrom_bank[1] & 0x80) >> 7, 1);
		set_nt_page(machine, 3, CIRAM, (state->mmc_vrom_bank[1] & 0x80) >> 7, 1);
	}
}

static WRITE8_HANDLER( mapper118_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper118_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
		case 0x0000:
			MMC3_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (MMC3_helper & 0x40)
				mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

			/* Has CHR Mode changed? */
			if (MMC3_helper & 0x80)
			{
				mapper118_set_mirror(space->machine);
				mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			}
			break;

		case 0x0001:
			cmd = state->mmc_cmd1 & 0x07;
			switch (cmd)
			{
				case 0: case 1:
				case 2: case 3: case 4: case 5:
					state->mmc_vrom_bank[cmd] = data;
					mapper118_set_mirror(space->machine);
					mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
					break;
				case 6:
				case 7:
					state->mmc_prg_bank[cmd - 6] = data;
					mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
					break;
			}
			break;

		case 0x2000:
			break;

		default:
			mapper4_w(space, offset, data);
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
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 chr_src[6], chr_mask[6];
	int i;

	for (i = 0; i < 6; i++)
	{
		chr_src[i] = (state->mmc_vrom_bank[i] & 0x40) ? CHRRAM : CHRROM;
		chr_mask[i] =  (state->mmc_vrom_bank[i] & 0x40) ? 0x07 : 0x3f;
	}

	chr1_x(machine, chr_page ^ 0, ((state->mmc_vrom_bank[0] & ~0x01) & chr_mask[0]), chr_src[0]);
	chr1_x(machine, chr_page ^ 1, ((state->mmc_vrom_bank[0] |  0x01) & chr_mask[0]), chr_src[0]);
	chr1_x(machine, chr_page ^ 2, ((state->mmc_vrom_bank[1] & ~0x01) & chr_mask[1]), chr_src[1]);
	chr1_x(machine, chr_page ^ 3, ((state->mmc_vrom_bank[1] |  0x01) & chr_mask[1]), chr_src[1]);
	chr1_x(machine, chr_page ^ 4, (state->mmc_vrom_bank[2] & chr_mask[2]), chr_src[2]);
	chr1_x(machine, chr_page ^ 5, (state->mmc_vrom_bank[3] & chr_mask[3]), chr_src[3]);
	chr1_x(machine, chr_page ^ 6, (state->mmc_vrom_bank[4] & chr_mask[4]), chr_src[4]);
	chr1_x(machine, chr_page ^ 7, (state->mmc_vrom_bank[5] & chr_mask[5]), chr_src[5]);
}

static WRITE8_HANDLER( mapper119_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper119_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
		case 0x0000:
			MMC3_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (MMC3_helper & 0x40)
				mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

			/* Has CHR Mode changed? */
			if (MMC3_helper & 0x80)
				mapper119_set_chr(space->machine);
			break;
		case 0x0001: /* $8001 */
			cmd = state->mmc_cmd1 & 0x07;
			switch (cmd)
			{
				case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
				case 2: case 3: case 4: case 5:
					state->mmc_vrom_bank[cmd] = data;
					mapper119_set_chr(space->machine);
					break;
				case 6:
				case 7:
					state->mmc_prg_bank[cmd - 6] = data;
					mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
					break;
			}
			break;

		default:
			mapper4_w(space, offset, data);
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

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper121_l_w )
{
	LOG_MMC(("mapper12_l_w, offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	if (!(offset < 0x1000))
	{
		switch (data & 0x03)
		{
		case 0x00:
		case 0x03:
			mapper121_reg[2] = 0x00;
			break;
		case 0x01:
			mapper121_reg[2] = 0x83;
			break;
		case 0x02:
			mapper121_reg[2] = 0x42;
			break;
		}
	}
}

static READ8_HANDLER( mapper121_l_r )
{
	LOG_MMC(("mapper12_l_r, offset: %04x\n", offset));
	offset += 0x100;

	if (!(offset < 0x1000))
		return mapper121_reg[2];
	else
		return 0;
}

static void mapper121_set_prg( running_machine *machine, int prg_base, int prg_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 prg_flip = (state->mmc_cmd1 & 0x40) ? 2 : 0;
	UINT8 new_bank2, new_bank3;

	new_bank2 = mapper121_reg[0] ? mapper121_reg[0] : state->mmc_prg_bank[2 ^ prg_flip];
	new_bank3 = mapper121_reg[1] ? mapper121_reg[1] : state->mmc_prg_bank[3];

	prg8_89(machine, prg_base | (state->mmc_prg_bank[0 ^ prg_flip] & prg_mask));
	prg8_ab(machine, prg_base | (state->mmc_prg_bank[1] & prg_mask));
	prg8_cd(machine, prg_base | (new_bank2 & prg_mask));
	prg8_ef(machine, prg_base | (new_bank3 & prg_mask));
}

static WRITE8_HANDLER( mapper121_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper121_w, offset: %04x, data: %02x\n", offset, data));

	if ((offset < 0x2000) && (offset & 0x03) == 0x03)
	{
		switch (data)
		{
		case 0x20:
			mapper121_reg[1] = 0x13;
			break;
		case 0x26:
			mapper121_reg[1] = 0x08;
			break;
		case 0x28:
			mapper121_reg[0] = 0x0c;
			break;
		case 0x29:
			mapper121_reg[1] = 0x1b;
			break;
		case 0xab:
			mapper121_reg[1] = 0x07;
			break;
		case 0xec:
		case 0xef:
			mapper121_reg[1] = 0x0d;
			break;
		case 0xff:
			mapper121_reg[1] = 0x09;
			break;
		default:
			mapper121_reg[0] = 0x0;
			mapper121_reg[1] = 0x0;
			break;
		}
	}
	else
	{
		switch (offset & 0x6001)
		{
		case 0x0000:
			MMC3_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (MMC3_helper & 0x40)
				mapper121_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

			/* Has CHR Mode changed? */
			if (MMC3_helper & 0x80)
				mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;

		case 0x0001:
			cmd = state->mmc_cmd1 & 0x07;
			switch (cmd)
			{
			case 0: case 1:
			case 2: case 3: case 4: case 5:
				state->mmc_vrom_bank[cmd] = data;
				mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
				break;
			case 6:
			case 7:
				state->mmc_prg_bank[cmd - 6] = data;
				mapper121_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
				break;
			}
			break;

		default:
			mapper4_w(space, offset, data);
			break;
		}
	}
	mapper121_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
}

/*************************************************************

    Mapper 122

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 123

    Known Boards: Bootleg Board ???
    Games: [no games in state->hsi]

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
    Games: [no games in state->hsi]

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

static WRITE8_HANDLER( mapper133_m_w )
{
	LOG_MMC(("mapper133_l_w %04x:%02x\n", offset, data));

	mapper133_l_w(space, offset & 0xff, data);	// offset does not really count for this mapper
}

/*************************************************************

    Mapper 134

    Known Boards: Unknown Multigame Bootleg Board (4646B)
    Games: 2 in 1 - Family Kid & Aladdin 4

    MMC3 clone

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper134_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper134_m_w, offset: %04x, data: %02x\n", offset, data));

	if (offset == 0x01)
	{
		state->mmc_prg_base = (data & 0x02) << 4;
		state->mmc_prg_mask = 0x1f;
		state->mmc_chr_base = (data & 0x20) << 3;
		state->mmc_chr_mask = 0xff;
		mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
		mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
	}
}

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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper136_l_w, offset: %04x, data: %02x\n", offset, data));

	if ((offset & 0x103) == 0x002)
	{
		state->mmc_cmd1 = (data & 0x30) | ((data + 3) & 0x0f);
		chr8(space->machine, state->mmc_cmd1, CHRROM);
	}
}

static READ8_HANDLER( mapper136_l_r )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper136_l_r, offset: %04x\n", offset));

	if ((offset & 0x103) == 0x000)
		return state->mmc_cmd1 | 0x40;
	else
		return 0x00;
}

/*************************************************************

    Mapper 137

    Known Boards: Bootleg Board by Sachen (8259D)
    Games: The Great Wall

    In MESS: Supported.

*************************************************************/

static void sachen_set_mirror( running_machine *machine, UINT8 nt ) // used by mappers 137, 138, 139, 141
{
	switch (nt)
	{
	case 0:
	case 1:
		set_nt_mirroring(machine, nt ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 2:
		set_nt_page(machine, 0, CIRAM, 0, 1);
		set_nt_page(machine, 1, CIRAM, 1, 1);
		set_nt_page(machine, 2, CIRAM, 1, 1);
		set_nt_page(machine, 3, CIRAM, 1, 1);
		break;
	case 3:
		set_nt_mirroring(machine, PPU_MIRROR_LOW);
		break;
	default:
		LOG_MMC(("Mapper set NT to invalid value %02x", nt));
		break;
	}
}

static WRITE8_HANDLER( mapper137_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper137_l_w, offset: %04x, data: %02x\n", offset, data));

	/* write happens only if we are at 0x4100 + k * 0x200, but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
	{
		if (!(offset & 0x01))
			state->mmc_cmd1 = data & 0x07;
		else
		{
			sachen_reg[state->mmc_cmd1] = data;

			switch (state->mmc_cmd1)
			{
			case 0x05:
				prg32(space->machine, data);
				break;
			case 0x07:
				sachen_set_mirror(space->machine, BIT(data, 0) ? 0 : (data >> 1) & 0x03);
				break;
			default:
				if (state->mmc_chr_source == CHRROM)
				{
					chr1_0(space->machine, (sachen_reg[0] & 0x07), CHRROM);
					chr1_1(space->machine, (sachen_reg[1] & 0x07) | (sachen_reg[4] << 4 & 0x10), CHRROM);
					chr1_2(space->machine, (sachen_reg[2] & 0x07) | (sachen_reg[4] << 3 & 0x10), CHRROM);
					chr1_3(space->machine, (sachen_reg[3] & 0x07) | (sachen_reg[4] << 2 & 0x10) | (sachen_reg[6] << 3 & 0x08), CHRROM);
				}
				break;
			}
		}
	}
}

static WRITE8_HANDLER( mapper137_m_w )
{
	LOG_MMC(("mapper137_m_w, offset: %04x, data: %02x\n", offset, data));

	mapper137_l_w(space, (offset + 0x100) & 0xfff, data);
}

/*************************************************************

    Mapper 138

    Known Boards: Bootleg Board by Sachen (8259B)
    Games: Silver Eagle

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper138_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 bank_helper1 = 0, bank_helper2 = 0;
	LOG_MMC(("mapper138_l_w, offset: %04x, data: %02x\n", offset, data));

	/* write happens only if we are at 0x4100 + k * 0x200, but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
	{
		if (!(offset & 0x01))
			state->mmc_cmd1 = data & 0x07;
		else
		{
			sachen_reg[state->mmc_cmd1] = data;

			switch (state->mmc_cmd1)
			{
			case 0x05:
				prg32(space->machine, data);
				break;
			case 0x07:
				sachen_set_mirror(space->machine, BIT(data, 0) ? 0 : (data >> 1) & 0x03);
				break;
			default:
				if (state->mmc_chr_source == CHRROM)
				{
					bank_helper1 = sachen_reg[7] & 0x01;
					bank_helper2 = (sachen_reg[4] & 0x07) << 3;
					chr2_0(space->machine, ((sachen_reg[bank_helper1 ? 0 : 0] & 0x07) | bank_helper2), CHRROM);
					chr2_2(space->machine, ((sachen_reg[bank_helper1 ? 0 : 1] & 0x07) | bank_helper2), CHRROM);
					chr2_4(space->machine, ((sachen_reg[bank_helper1 ? 0 : 2] & 0x07) | bank_helper2), CHRROM);
					chr2_6(space->machine, ((sachen_reg[bank_helper1 ? 0 : 3] & 0x07) | bank_helper2), CHRROM);
				}
				break;
			}
		}
	}
}

static WRITE8_HANDLER( mapper138_m_w )
{
	LOG_MMC(("mapper138_m_w, offset: %04x, data: %02x\n", offset, data));

	mapper138_l_w(space, (offset + 0x100) & 0xfff, data);
}

/*************************************************************

    Mapper 139

    Known Boards: Bootleg Board by Sachen (8259C)
    Games: Final Combat, Hell Fighter, Mahjong Companion,

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper139_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 bank_helper1 = 0, bank_helper2 = 0;
	LOG_MMC(("mapper139_l_w, offset: %04x, data: %02x\n", offset, data));

	/* write happens only if we are at 0x4100 + k * 0x200, but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
	{
		if (!(offset & 0x01))
			state->mmc_cmd1 = data & 0x07;
		else
		{
			sachen_reg[state->mmc_cmd1] = data;

			switch (state->mmc_cmd1)
			{
			case 0x05:
				prg32(space->machine, data);
				break;
			case 0x07:
				sachen_set_mirror(space->machine, BIT(data, 0) ? 0 : (data >> 1) & 0x03);
				break;
			default:
				if (state->mmc_chr_source == CHRROM)
				{
					bank_helper1 = sachen_reg[7] & 0x01;
					bank_helper2 = (sachen_reg[4] & 0x07) << 3;
					chr2_0(space->machine, ((sachen_reg[bank_helper1 ? 0 : 0] & 0x07) | bank_helper2) << 2, CHRROM);
					chr2_2(space->machine, ((sachen_reg[bank_helper1 ? 0 : 1] & 0x07) | bank_helper2) << 2 | 0x01, CHRROM);
					chr2_4(space->machine, ((sachen_reg[bank_helper1 ? 0 : 2] & 0x07) | bank_helper2) << 2 | 0x02, CHRROM)	;
					chr2_6(space->machine, ((sachen_reg[bank_helper1 ? 0 : 3] & 0x07) | bank_helper2) << 2 | 0x03, CHRROM);
				}
				break;
			}
		}
	}
}

static WRITE8_HANDLER( mapper139_m_w )
{
	LOG_MMC(("mapper139_m_w, offset: %04x, data: %02x\n", offset, data));

	mapper139_l_w(space, (offset + 0x100) & 0xfff, data);
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 bank_helper1 = 0, bank_helper2 = 0;
	LOG_MMC(("mapper141_l_w, offset: %04x, data: %02x\n", offset, data));

	/* write happens only if we are at 0x4100 + k * 0x200, but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
	{
		if (!(offset & 0x01))
			state->mmc_cmd1 = data & 0x07;
		else
		{
			sachen_reg[state->mmc_cmd1] = data;

			switch (state->mmc_cmd1)
			{
			case 0x05:
				prg32(space->machine, data);
				break;
			case 0x07:
				sachen_set_mirror(space->machine, BIT(data, 0) ? 0 : (data >> 1) & 0x03);
				break;
			default:
				if (state->mmc_chr_source == CHRROM)
				{
					bank_helper1 = sachen_reg[7] & 0x01;
					bank_helper2 = (sachen_reg[4] & 0x07) << 3;
					chr2_0(space->machine, ((sachen_reg[bank_helper1 ? 0 : 0] & 0x07) | bank_helper2) << 1, state->mmc_chr_source);
					chr2_2(space->machine, ((sachen_reg[bank_helper1 ? 0 : 1] & 0x07) | bank_helper2) << 1 | 0x01, state->mmc_chr_source);
					chr2_4(space->machine, ((sachen_reg[bank_helper1 ? 0 : 2] & 0x07) | bank_helper2) << 1, state->mmc_chr_source);
					chr2_6(space->machine, ((sachen_reg[bank_helper1 ? 0 : 3] & 0x07) | bank_helper2) << 1 | 0x01, state->mmc_chr_source);
				}
				break;
			}
		}
	}
}

static WRITE8_HANDLER( mapper141_m_w )
{
	LOG_MMC(("mapper141_m_w, offset: %04x, data: %02x\n", offset, data));

	mapper141_l_w(space, (offset + 0x100) & 0xfff, data);
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

	/* the address is read only if we are at 0x4100 + k * 0x200, but 0x4100 is offset = 0 */
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

	data = data | (memory_read_byte(space, offset) & 1);

	chr8(space->machine, data >> 4, CHRROM);
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

	mapper147_l_w(space, (offset + 0x100) & 0xfff, data);
}

static WRITE8_HANDLER( mapper147_w )
{
	LOG_MMC(("mapper147_w, offset: %04x, data: %02x\n", offset, data));

	mapper147_l_w(space, (offset + 0x100) & 0xfff, data);
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

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper150_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper150_l_w, offset: %04x, data: %02x\n", offset, data));

	/* write happens only if we are at 0x4100 + k * 0x200, but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
	{
		if (!(offset & 0x01))
			state->mmc_cmd1 = data & 0x07;
		else
		{
			switch (state->mmc_cmd1)
			{
			case 0x02:
				state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & ~0x08) | ((data << 3) & 0x08);
				chr8(space->machine, state->mmc_vrom_bank[0], CHRROM);
				prg32(space->machine, data & 0x01);
				break;
			case 0x04:
				state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & ~0x04) | ((data << 2) & 0x04);
				chr8(space->machine, state->mmc_vrom_bank[0], CHRROM);
				break;
			case 0x05:
				prg32(space->machine, data & 0x07);
				break;
			case 0x06:
				state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & ~0x03) | (data << 0 & 0x03);
				chr8(space->machine, state->mmc_vrom_bank[0], CHRROM);
				break;
			case 0x07:
				sachen_set_mirror(space->machine, (data >> 1) & 0x03);
				break;
			default:
				break;
			}
		}
	}
}

static READ8_HANDLER( mapper150_l_r )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper150_l_r, offset: %04x", offset));

	/* read  happens only if we are at 0x4100 + k * 0x200, but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
		return (~state->mmc_cmd1 & 0x3f) /* ^ dips*/;	// we would need to check the Dips here
	else
		return 0;
}

static WRITE8_HANDLER( mapper150_m_w )
{
	LOG_MMC(("mapper150_m_w, offset: %04x, data: %02x\n", offset, data));

	mapper150_l_w(space, (offset + 0x100) & 0xfff, data);
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
	set_nt_mirroring(space->machine, BIT(data, 7) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
	prg16_89ab(space->machine, (data >> 4) & 0x07);
	chr8(space->machine, data, CHRROM);
}

/*************************************************************

    Mapper 153

    Known Boards: Bandai Board
    Games: Dragon Ball, Dragon Ball 3, Famicom Jump, Famicom
          Jump II

    This mapper should be exactly like Mapper 16, but with
    SRAM in place of EEPROM. Hence, writes to 0x6000-0x7fff
    go to SRAM (not sure if there exist boards with no SRAM
    and no EEPROM... investigate!)
    Famicom Jump II uses a different board (or uses the same
    board in a very different way)

    In MESS: Supported.

*************************************************************/

static void mapper153_set_prg( running_machine *machine )
{
	UINT8 mmc_helper = 0;
	int i;

	for (i = 0; i < 8; i++)
		mmc_helper |= ((map153_reg[i] & 0x01) << 4);

	map153_bank_latch = mmc_helper | (map153_bank_latch & 0x0f);

	prg16_89ab(machine, map153_bank_latch);
	prg16_cdef(machine, mmc_helper | 0x0f);
}

static WRITE8_HANDLER( mapper153_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper153_m_w, offset: %04x, data: %02x\n", offset, data));

	if (state->crc_hack)		// Famicom Jump II uses a different board
	{
		switch (offset & 0x0f)	// there is no CHRROM and prg banking works differently
		{
		case 0: case 1:
		case 2: case 3:
		case 4: case 5:
		case 6: case 7:
			map153_reg[offset & 0x07] = data;
			mapper153_set_prg(space->machine);
			break;
		case 8:
			map153_bank_latch = (data & 0x0f) | (map153_bank_latch & 0x10);
			prg16_89ab(space->machine, map153_bank_latch);
			break;
		default:
			mapper16_m_w(space, offset & 0x0f, data);
			break;
		}
	}
	else
		mapper16_m_w(space, offset, data);
}

static WRITE8_HANDLER( mapper153_w )
{
	LOG_MMC(("mapper153_w, offset: %04x, data: %02x\n", offset, data));

	mapper153_m_w(space, offset, data);
}

/*************************************************************

    Mapper 154

    Known Boards: Namcot 34xx
    Games: Devil Man

    Acts like mapper 88, but writes to 0x8000 set NT mirroring

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper154_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper154_w, offset: %04x, data: %02x\n", offset, data));
	switch (offset & 1)
	{
	case 0:
		set_nt_mirroring(space->machine, BIT(data, 6) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
		state->mmc_cmd1 = data & 7;
		break;
	case 1:
		switch (state->mmc_cmd1)
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

/*************************************************************

    Mapper 155

    Known Boards: SKROM
    Games: Tatakae!! Rahmen Man - Sakuretsu Choujin 102 Gei

    This is basically MMC1 with a different way to handle WRAM
    (this difference is not emulated yet, since we don't fully
    emulate WRAM in MMC1 either)

    In MESS: Supported (as complete as MMC1).

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

    This mapper is basically Mapper 16 + Datach reader. We
    currently do not support the Barcode reader.

    In MESS: Partially supported. Needs reads from 0x6000-0x7fff
      for the Datach barcode reader

*************************************************************/

/*************************************************************

    Mapper 158

    Known Boards: Tengen 800037
    Games: Alien Syndrome

    In MESS: Very preliminary support.

*************************************************************/

// probably wrong...
static void mapper158_set_mirror( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 nt_mode = state->mmc_cmd1 & 0x80;

	set_nt_page(machine, 0, ROM, state->mmc_vrom_bank[nt_mode ? 2 : 0], 0);
	set_nt_page(machine, 1, ROM, state->mmc_vrom_bank[nt_mode ? 3 : 0], 0);
	set_nt_page(machine, 2, ROM, state->mmc_vrom_bank[nt_mode ? 4 : 1], 0);
	set_nt_page(machine, 3, ROM, state->mmc_vrom_bank[nt_mode ? 5 : 1], 0);
}

static WRITE8_HANDLER( mapper158_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 map158_helper, cmd;
	LOG_MMC(("mapper158_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
		case 0x0000:
			map158_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (map158_helper & 0x40)
				mapper64_set_prg(space->machine);

			/* Has CHR Mode changed? */
			if (map158_helper & 0xa0)
			{
				mapper64_set_chr(space->machine);
				mapper158_set_mirror(space->machine);
			}
			break;

		case 0x0001:
			cmd = state->mmc_cmd1 & 0x0f;
			switch (cmd)
			{
				case 0: case 1:
				case 2: case 3:
				case 4: case 5:
					state->mmc_vrom_bank[cmd] = data;
					mapper64_set_chr(space->machine);
					mapper158_set_mirror(space->machine);
					break;
				case 6: case 7:
					state->mmc_prg_bank[cmd - 6] = data;
					mapper64_set_prg(space->machine);
					break;
				case 8: case 9:
					state->mmc_vrom_bank[cmd - 2] = data;
					mapper64_set_chr(space->machine);
					mapper158_set_mirror(space->machine);
					break;
				case 0x0f:
					state->mmc_prg_bank[2] = data;
					mapper64_set_prg(space->machine);
					break;
			}
			break;

		case 0x2000:
			break;

		default:
			mapper64_w(space, offset, data);
			break;
	}
}

/*************************************************************

    Mapper 159

    Known Boards: Bandai Board
    Games: Dragon Ball Z, Magical Taruruuto-kun, SD Gundam Gaiden

    This is exactly like Mapper 16, but with different EEPROM I/O.
    At the moment we don't support EEPROM in either mapper.

    In MESS: Supported.

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
	set_nt_mirroring(space->machine, BIT(data, 5) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper178_l_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset)
	{
	case 0x700:
		set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 0x701:
		state->mmc_cmd1 = (state->mmc_cmd1 & 0x0c) | ((data >> 1) & 0x03);
		prg32(space->machine, state->mmc_cmd1);
		break;
	case 0x702:
		state->mmc_cmd1 = (state->mmc_cmd1 & 0x03) | ((data << 2) & 0x0c);
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
	offset += 0x4100;

	if (offset & 0x5000)
		prg32(space->machine, data >> 1);
}

static WRITE8_HANDLER( mapper179_w )
{
	LOG_MMC(("mapper179_w, offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper182_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7003)
	{
	case 0x0001:
		set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 0x2000:
		state->mmc_cmd1 = data;
		break;
	case 0x4000:
		switch (state->mmc_cmd1)
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
		if (data)
		{
			state->IRQ_count = data;
			state->IRQ_enable = 1;
		}
		else
			state->IRQ_enable = 0;
		break;
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

    In MESS: Very Preliminar Support. We act like it is mapper 3
    but we need to emulate the read back from 0x8000-0xfff!

*************************************************************/

static WRITE8_HANDLER( mapper185_w )
{
	LOG_MMC(("mapper3_w offset: %04x, data: %02x\n", offset, data));

	chr8(space->machine, data, CHRROM);
}

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

    In MESS: Preliminary Support.

*************************************************************/

static void mapper187_set_prg( running_machine *machine, int prg_base, int prg_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 prg_flip = (state->mmc_cmd1 & 0x40) ? 2 : 0;

	if (!(mapper187_reg[0] & 0x80))
	{
		prg8_89(machine, prg_base | (state->mmc_prg_bank[0 ^ prg_flip] & prg_mask));
		prg8_ab(machine, prg_base | (state->mmc_prg_bank[1] & prg_mask));
		prg8_cd(machine, prg_base | (state->mmc_prg_bank[2 ^ prg_flip] & prg_mask));
		prg8_ef(machine, prg_base | (state->mmc_prg_bank[3] & prg_mask));
	}
}

static void mapper187_set_chr( running_machine *machine, UINT8 chr, int chr_base, int chr_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 new_base[8];
	int i;

	for (i = 0; i < 8; i++)
		new_base[i] = ((i & 0x04) == chr_page)? (chr_base | 0x100) : chr_base;

	chr1_x(machine, chr_page ^ 0, new_base[0] | ((state->mmc_vrom_bank[0] & ~0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 1, new_base[1] | ((state->mmc_vrom_bank[0] |  0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 2, new_base[2] | ((state->mmc_vrom_bank[1] & ~0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 3, new_base[3] | ((state->mmc_vrom_bank[1] |  0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 4, new_base[4] | (state->mmc_vrom_bank[2] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 5, new_base[5] | (state->mmc_vrom_bank[3] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 6, new_base[6] | (state->mmc_vrom_bank[4] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 7, new_base[7] | (state->mmc_vrom_bank[5] & chr_mask), chr);
}

static WRITE8_HANDLER( mapper187_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 new_bank;
	LOG_MMC(("mapper187_l_w, offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	if (offset == 0x1000)
	{
		switch (data & 0x03)
		{
		case 0x00:
		case 0x01:
			mapper187_reg[1] = 0x83;
			break;
		case 0x02:
			mapper187_reg[1] = 0x42;
			break;
		case 0x03:
			mapper187_reg[1] = 0x00;
			break;
		}
	}

	if (!(offset <= 0x1000))
	{
		mapper187_reg[0] = data;

		if (mapper187_reg[0] & 0x80)
		{
			new_bank = (mapper187_reg[0] & 0x1f);

			if (mapper187_reg[0] & 0x20)
				prg32(space->machine, new_bank >> 2);
			else
			{
				prg16_89ab(space->machine, new_bank);
				prg16_cdef(space->machine, new_bank);
			}
		}
		else
			mapper187_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
	}
}

static READ8_HANDLER( mapper187_l_r )
{
	LOG_MMC(("mapper187_l_r, offset: %04x\n", offset));
	offset += 0x100;

	if (!(offset < 0x1000))
		return mapper187_reg[1];
	else
		return 0;
}

static WRITE8_HANDLER( mapper187_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper187_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6003)
	{
	case 0x0000:
		mapper187_reg[2] = 1;
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has PRG Mode changed? */
		if (MMC3_helper & 0x40)
			mapper187_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper187_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
		break;

	case 0x0001:
		if (mapper187_reg[2])
		{
			cmd = state->mmc_cmd1 & 0x07;
			switch (cmd)
			{
			case 0: case 1:
			case 2: case 3: case 4: case 5:
				state->mmc_vrom_bank[cmd] = data;
				mapper187_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
				break;
			case 6:
			case 7:
				state->mmc_prg_bank[cmd - 6] = data;
				mapper187_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
				break;
			}
		}
		break;

	case 0x0002:
		break;

	case 0x0003:
		mapper187_reg[2] = 0;

		if (data == 0x28)
			prg8_cd(space->machine, 0x17);
		else if (data == 0x2a)
			prg8_ab(space->machine, 0x0f);
		break;

	default:
		mapper4_w(space, offset, data);
		break;
	}
}

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

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper189_l_w )
{
	LOG_MMC(("mapper189_l_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, (data >> 4) | data);
}

static WRITE8_HANDLER( mapper189_m_w )
{
	LOG_MMC(("mapper189_m_w, offset: %04x, data: %04x\n", offset, data));

	mapper189_l_w(space, offset & 0xff, data);	// offset does not really count for this mapper
}

/* This is like mapper4 but with no PRG bankswitch (beacuse it is handled by low writes) */
static WRITE8_HANDLER( mapper189_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper189_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0000:
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
		break;

	case 0x0001:
		cmd = state->mmc_cmd1 & 0x07;
		switch (cmd)
		{
		case 0: case 1:
		case 2: case 3: case 4: case 5:
			state->mmc_vrom_bank[cmd] = data;
			mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		}
		break;

	default:
		mapper4_w(space, offset, data);
		break;
	}
}

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

    MMC3 clone. This is a minor modification of Mapper 74,
    in the sense that it is the same board except for the
    CHRRAM pages.

    In MESS: Supported.

*************************************************************/

static void mapper191_set_chr( running_machine *machine, int chr_base, int chr_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 chr_src[6];
	int i;

	for (i = 0; i < 6; i++)
		chr_src[i] = (state->mmc_vrom_bank[i] & 0x80) ? CHRRAM : CHRROM;

	chr1_x(machine, chr_page ^ 0, chr_base | ((state->mmc_vrom_bank[0] & ~0x01) & chr_mask), chr_src[0]);
	chr1_x(machine, chr_page ^ 1, chr_base | ((state->mmc_vrom_bank[0] |  0x01) & chr_mask), chr_src[0]);
	chr1_x(machine, chr_page ^ 2, chr_base | ((state->mmc_vrom_bank[1] & ~0x01) & chr_mask), chr_src[1]);
	chr1_x(machine, chr_page ^ 3, chr_base | ((state->mmc_vrom_bank[1] |  0x01) & chr_mask), chr_src[1]);
	chr1_x(machine, chr_page ^ 4, chr_base | (state->mmc_vrom_bank[2] & chr_mask), chr_src[2]);
	chr1_x(machine, chr_page ^ 5, chr_base | (state->mmc_vrom_bank[3] & chr_mask), chr_src[3]);
	chr1_x(machine, chr_page ^ 6, chr_base | (state->mmc_vrom_bank[4] & chr_mask), chr_src[4]);
	chr1_x(machine, chr_page ^ 7, chr_base | (state->mmc_vrom_bank[5] & chr_mask), chr_src[5]);
}

static WRITE8_HANDLER( mapper191_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper191_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0000:
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has PRG Mode changed? */
		if (MMC3_helper & 0x40)
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper191_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
		break;

	case 0x0001:
		cmd = state->mmc_cmd1 & 0x07;
		switch (cmd)
		{
		case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
		case 2: case 3: case 4: case 5:
			state->mmc_vrom_bank[cmd] = data;
			mapper191_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 6:
		case 7:
			state->mmc_prg_bank[cmd - 6] = data;
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			break;
		}
		break;

	default:
		mapper74_w(space, offset, data);
		break;
	}
}

/*************************************************************

    Mapper 192

    Known Boards: Type C by Waixing
    Games: Ying Lie Qun Xia Zhuan, Young Chivalry

    MMC3 clone. This is a minor modification of Mapper 74,
    in the sense that it is the same board except for the
    CHRRAM pages.

    In MESS: Supported.

*************************************************************/

static void mapper192_set_chr( running_machine *machine, int chr_base, int chr_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 chr_src[6];
	int i;

	for (i = 0; i < 6; i++)
		chr_src[i] = ((state->mmc_vrom_bank[i] == 0x08) || (state->mmc_vrom_bank[i] == 0x09) || (state->mmc_vrom_bank[i] == 0x0a) || (state->mmc_vrom_bank[i] == 0x0b)) ? CHRRAM : CHRROM;

	chr1_x(machine, chr_page ^ 0, chr_base | ((state->mmc_vrom_bank[0] & ~0x01) & chr_mask), chr_src[0]);
	chr1_x(machine, chr_page ^ 1, chr_base | ((state->mmc_vrom_bank[0] |  0x01) & chr_mask), chr_src[0]);
	chr1_x(machine, chr_page ^ 2, chr_base | ((state->mmc_vrom_bank[1] & ~0x01) & chr_mask), chr_src[1]);
	chr1_x(machine, chr_page ^ 3, chr_base | ((state->mmc_vrom_bank[1] |  0x01) & chr_mask), chr_src[1]);
	chr1_x(machine, chr_page ^ 4, chr_base | (state->mmc_vrom_bank[2] & chr_mask), chr_src[2]);
	chr1_x(machine, chr_page ^ 5, chr_base | (state->mmc_vrom_bank[3] & chr_mask), chr_src[3]);
	chr1_x(machine, chr_page ^ 6, chr_base | (state->mmc_vrom_bank[4] & chr_mask), chr_src[4]);
	chr1_x(machine, chr_page ^ 7, chr_base | (state->mmc_vrom_bank[5] & chr_mask), chr_src[5]);
}

static WRITE8_HANDLER( mapper192_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper192_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0000:
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has PRG Mode changed? */
		if (MMC3_helper & 0x40)
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper192_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
		break;

	case 0x0001:
		cmd = state->mmc_cmd1 & 0x07;
		switch (cmd)
		{
		case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
		case 2: case 3: case 4: case 5:
			state->mmc_vrom_bank[cmd] = data;
			mapper192_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 6:
		case 7:
			state->mmc_prg_bank[cmd - 6] = data;
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			break;
		}
		break;

	default:
		mapper74_w(space, offset, data);
		break;
	}
}

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

    MMC3 clone. This is a minor modification of Mapper 74,
    in the sense that it is the same board except for the
    CHRRAM pages.

    In MESS: Supported.

*************************************************************/

static void mapper194_set_chr( running_machine *machine, int chr_base, int chr_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 chr_src[6];
	int i;

	for (i = 0; i < 6; i++)
		chr_src[i] = (state->mmc_vrom_bank[i] < 0x02) ? CHRRAM : CHRROM;

	chr1_x(machine, chr_page ^ 0, chr_base | ((state->mmc_vrom_bank[0] & ~0x01) & chr_mask), chr_src[0]);
	chr1_x(machine, chr_page ^ 1, chr_base | ((state->mmc_vrom_bank[0] |  0x01) & chr_mask), chr_src[0]);
	chr1_x(machine, chr_page ^ 2, chr_base | ((state->mmc_vrom_bank[1] & ~0x01) & chr_mask), chr_src[1]);
	chr1_x(machine, chr_page ^ 3, chr_base | ((state->mmc_vrom_bank[1] |  0x01) & chr_mask), chr_src[1]);
	chr1_x(machine, chr_page ^ 4, chr_base | (state->mmc_vrom_bank[2] & chr_mask), chr_src[2]);
	chr1_x(machine, chr_page ^ 5, chr_base | (state->mmc_vrom_bank[3] & chr_mask), chr_src[3]);
	chr1_x(machine, chr_page ^ 6, chr_base | (state->mmc_vrom_bank[4] & chr_mask), chr_src[4]);
	chr1_x(machine, chr_page ^ 7, chr_base | (state->mmc_vrom_bank[5] & chr_mask), chr_src[5]);
}

static WRITE8_HANDLER( mapper194_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper194_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0000:
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has PRG Mode changed? */
		if (MMC3_helper & 0x40)
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper194_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
		break;

	case 0x0001:
		cmd = state->mmc_cmd1 & 0x07;
		switch (cmd)
		{
		case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
		case 2: case 3: case 4: case 5:
			state->mmc_vrom_bank[cmd] = data;
			mapper194_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 6:
		case 7:
			state->mmc_prg_bank[cmd - 6] = data;
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			break;
		}
		break;

	default:
		mapper74_w(space, offset, data);
		break;
	}
}

/*************************************************************

    Mapper 195

    Known Boards: Type E by Waixing
    Games: Captain Tsubasa Vol. II (C), Chaos World, God
          Slayer (C), Zu Qiu Xiao Jiang

    MMC3 clone. This is a minor modification of Mapper 74,
    in the sense that it is the same board except for the
    CHRRAM pages.

    In MESS: Supported.

*************************************************************/

static void mapper195_set_chr( running_machine *machine, int chr_base, int chr_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 chr_src[6];
	int i;

	for (i = 0; i < 6; i++)
		chr_src[i] = (state->mmc_vrom_bank[i] < 0x04) ? CHRRAM : CHRROM;

	chr1_x(machine, chr_page ^ 0, chr_base | ((state->mmc_vrom_bank[0] & ~0x01) & chr_mask), chr_src[0]);
	chr1_x(machine, chr_page ^ 1, chr_base | ((state->mmc_vrom_bank[0] |  0x01) & chr_mask), chr_src[0]);
	chr1_x(machine, chr_page ^ 2, chr_base | ((state->mmc_vrom_bank[1] & ~0x01) & chr_mask), chr_src[1]);
	chr1_x(machine, chr_page ^ 3, chr_base | ((state->mmc_vrom_bank[1] |  0x01) & chr_mask), chr_src[1]);
	chr1_x(machine, chr_page ^ 4, chr_base | (state->mmc_vrom_bank[2] & chr_mask), chr_src[2]);
	chr1_x(machine, chr_page ^ 5, chr_base | (state->mmc_vrom_bank[3] & chr_mask), chr_src[3]);
	chr1_x(machine, chr_page ^ 6, chr_base | (state->mmc_vrom_bank[4] & chr_mask), chr_src[4]);
	chr1_x(machine, chr_page ^ 7, chr_base | (state->mmc_vrom_bank[5] & chr_mask), chr_src[5]);
}

static WRITE8_HANDLER( mapper195_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper195_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0000:
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has PRG Mode changed? */
		if (MMC3_helper & 0x40)
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper195_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
		break;

	case 0x0001:
		cmd = state->mmc_cmd1 & 0x07;
		switch (cmd)
		{
		case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
		case 2: case 3: case 4: case 5:
			state->mmc_vrom_bank[cmd] = data;
			mapper195_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 6:
		case 7:
			state->mmc_prg_bank[cmd - 6] = data;
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			break;
		}
		break;

	default:
		mapper74_w(space, offset, data);
		break;
	}
}

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

	mapper4_w(space, (offset & 0x6000) | ((offset & 0x04) >> 2), data);
}

/*************************************************************

    Mapper 197

    Known Boards: Unknown Bootleg Board
    Games: Super Fighter III

    MMC3 clone

    In MESS: Supported.

*************************************************************/

static void mapper197_set_chr( running_machine *machine, UINT8 chr_source, int chr_base, int chr_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	chr4_0(machine, chr_base | ((state->mmc_vrom_bank[0] >> 1) & chr_mask), chr_source);
	chr2_4(machine, chr_base | (state->mmc_vrom_bank[1] & chr_mask), chr_source);
	chr2_6(machine, chr_base | (state->mmc_vrom_bank[2] & chr_mask), chr_source);
}

static WRITE8_HANDLER( mapper197_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper197_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0000:
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has PRG Mode changed? */
		if (MMC3_helper & 0x40)
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper197_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
		break;

	case 0x0001:
		cmd = state->mmc_cmd1 & 0x0f;
		switch (cmd)
		{
		case 0: case 2: case 4:
			state->mmc_vrom_bank[cmd >> 1] = data;
			mapper197_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 6:
		case 7:
			state->mmc_prg_bank[cmd - 6] = data;
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			break;
		}
		break;

	default:
		mapper4_w(space, offset, data);
		break;
	}
}

/*************************************************************

    Mapper 198

    Known Boards: Type F by Waixing
    Games: Tenchi wo Kurau II (C)

    MMC3 clone.

    In MESS: Preliminary support.

*************************************************************/

static void mapper198_set_prg( running_machine *machine, int prg_base, int prg_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 prg_flip = (state->mmc_cmd1 & 0x40) ? 2 : 0;

	prg8_89(machine, prg_base | (state->mmc_prg_bank[0 ^ prg_flip] & prg_mask));
	prg8_ab(machine, prg_base | (state->mmc_prg_bank[1] & prg_mask));
	prg8_cd(machine, prg_base | (state->mmc_prg_bank[2 ^ prg_flip] & prg_mask));
	prg8_ef(machine, prg_base | (state->mmc_prg_bank[3] & prg_mask));
}

static WRITE8_HANDLER( mapper198_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper198_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0000:
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has PRG Mode changed? */
		if (MMC3_helper & 0x40)
			mapper198_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
		break;

	case 0x0001:
		cmd = state->mmc_cmd1 & 0x07;
		switch (cmd)
		{
		case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
		case 2: case 3: case 4: case 5:
			state->mmc_vrom_bank[cmd] = data;
			mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 6:
		case 7:
			state->mmc_prg_bank[cmd - 6] = data & ((data > 0x3f) ? 0x4f : 0x3f);
			mapper198_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			break;
		}
		break;

	default:
		mapper74_w(space, offset, data);
		break;
	}
}

/*************************************************************

    Mapper 199

    Known Boards: Type G by Waixing
    Games: San Guo Zhi 2, Dragon Ball Z Gaiden (C), Dragon
          Ball Z II (C)

    MMC3 clone

    In MESS: Supported.

*************************************************************/

static void mapper199_set_chr( running_machine *machine, int chr_base, int chr_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 chr_src[8];
	int i;

	for (i = 0; i < 8; i++)
		chr_src[i] = (state->mmc_vrom_bank[i] < 0x08) ? CHRRAM : CHRROM;

	chr1_x(machine, chr_page ^ 0, chr_base | (state->mmc_vrom_bank[0] & chr_mask), chr_src[0]);
	chr1_x(machine, chr_page ^ 1, chr_base | (state->mmc_vrom_bank[6] & chr_mask), chr_src[6]);
	chr1_x(machine, chr_page ^ 2, chr_base | (state->mmc_vrom_bank[1] & chr_mask), chr_src[1]);
	chr1_x(machine, chr_page ^ 3, chr_base | (state->mmc_vrom_bank[7] & chr_mask), chr_src[7]);
	chr1_x(machine, chr_page ^ 4, chr_base | (state->mmc_vrom_bank[2] & chr_mask), chr_src[2]);
	chr1_x(machine, chr_page ^ 5, chr_base | (state->mmc_vrom_bank[3] & chr_mask), chr_src[3]);
	chr1_x(machine, chr_page ^ 6, chr_base | (state->mmc_vrom_bank[4] & chr_mask), chr_src[4]);
	chr1_x(machine, chr_page ^ 7, chr_base | (state->mmc_vrom_bank[5] & chr_mask), chr_src[5]);
}

static WRITE8_HANDLER( mapper199_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper199_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0000:
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has PRG Mode changed? */
		if (MMC3_helper & 0x40)
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper199_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
		break;

	case 0x0001:
		cmd = state->mmc_cmd1 & 0x0f;
		switch (cmd)
		{
		case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
		case 2: case 3: case 4: case 5:
			state->mmc_vrom_bank[cmd] = data;
			mapper199_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 6:
		case 7:
		case 8:
		case 9:
			state->mmc_prg_bank[cmd - 6] = data;
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			break;
		case 0x0a: case 0x0b:
			state->mmc_vrom_bank[cmd - 4] = data;
			mapper199_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		}
		break;

	default:
		mapper4_w(space, offset, data);
		break;
	}
}

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

	set_nt_mirroring(space->machine, BIT(data, 3) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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

	set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ: PPU_MIRROR_VERT);
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

	set_nt_mirroring(space->machine, BIT(data, 4) ? PPU_MIRROR_HORZ: PPU_MIRROR_VERT);
}

/*************************************************************

    Mapper 205

    Known Boards: Unknown Bootleg Multigame Board
    Games: 3 in 1, 15 in 1

    MMC3 clone

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper205_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper205_m_w, offset: %04x, data: %02x\n", offset, data));

	if (offset & 0x0800)
	{
		state->mmc_prg_base = (data & 0x03) << 4;
		state->mmc_prg_mask = (data & 0x02) ? 0x0f : 0x1f;
		state->mmc_chr_base = (data & 0x03) << 7;
		state->mmc_chr_mask = (data & 0x02) ? 0x7f : 0xff;
		mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
		mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
	}
}

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
			set_nt_page(space->machine, 0, CIRAM, (data & 0x80) ? 1 : 0, 1);
			set_nt_page(space->machine, 1, CIRAM, (data & 0x80) ? 1 : 0, 1);
			/* Switch 2k VROM at $0000 */
			chr2_0(space->machine, (data & 0x7f) >> 1, CHRROM);
			break;
		case 0x1ef1:
			set_nt_page(space->machine, 2, CIRAM, (data & 0x80) ? 1 : 0, 1);
			set_nt_page(space->machine, 3, CIRAM, (data & 0x80) ? 1 : 0, 1);
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

    In MESS: Preliminary Support.

*************************************************************/

static WRITE8_HANDLER( mapper208_l_w )
{
	static const UINT8 conv_table[256] =
	{
		0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x49,0x19,0x09,0x59,0x49,0x19,0x09,
		0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x51,0x41,0x11,0x01,0x51,0x41,0x11,0x01,
		0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x49,0x19,0x09,0x59,0x49,0x19,0x09,
		0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x51,0x41,0x11,0x01,0x51,0x41,0x11,0x01,
		0x00,0x10,0x40,0x50,0x00,0x10,0x40,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x08,0x18,0x48,0x58,0x08,0x18,0x48,0x58,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x10,0x40,0x50,0x00,0x10,0x40,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x08,0x18,0x48,0x58,0x08,0x18,0x48,0x58,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x58,0x48,0x18,0x08,0x58,0x48,0x18,0x08,
		0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x50,0x40,0x10,0x00,0x50,0x40,0x10,0x00,
		0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x58,0x48,0x18,0x08,0x58,0x48,0x18,0x08,
		0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x50,0x40,0x10,0x00,0x50,0x40,0x10,0x00,
		0x01,0x11,0x41,0x51,0x01,0x11,0x41,0x51,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x09,0x19,0x49,0x59,0x09,0x19,0x49,0x59,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x01,0x11,0x41,0x51,0x01,0x11,0x41,0x51,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x09,0x19,0x49,0x59,0x09,0x19,0x49,0x59,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	};

	LOG_MMC(("mapper208_l_w, offset: %04x, data: %02x\n", offset, data));

	if (!(offset < 0x1700))
		map208_reg[offset & 0x03] = data ^ conv_table[map208_reg[4]];
	else if (!(offset < 0xf00))
		map208_reg[4] = data;
	else if (!(offset < 0x700))
		prg32(space->machine, ((data >> 3) & 0x02) | (data & 0x01));
}

static READ8_HANDLER( mapper208_l_r )
{
	LOG_MMC(("mapper208_l_r, offset: %04x\n", offset));

	if (!(offset < 0x1700))
		return map208_reg[offset & 0x03];

	return 0x00;
}

/* This is like mapper4 but with no PRG bankswitch (beacuse it is handled by low writes) */
// replace with map189_w... they are the same!!
static WRITE8_HANDLER( mapper208_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper208_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0000:
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
		break;

	case 0x0001:
		cmd = state->mmc_cmd1 & 0x07;
		switch (cmd)
		{
		case 0: case 1:
		case 2: case 3: case 4: case 5:
			state->mmc_vrom_bank[cmd] = data;
			mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		}
		break;

	default:
		mapper4_w(space, offset, data);
		break;
	}
}

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

    For some emulators, this is the same as Mapper 19 but it
    has hardwired mirroring and no additional sound hardware.
    Initializing Mirroring to Vertical in Mapper 19 seems to
    fix all glitches in 'so-called' Mapper 210 games, so there
    seems to be no reason to have this duplicate mapper (at
    least until we emulate additional sound hardware... if sound
    writes would create conflicts, we might consider to split the
    mapper in two variants)

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

	set_nt_mirroring(space->machine, BIT(data, 3) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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

    In MESS: Preliminary support.

*************************************************************/

// remove mask & base parameters!
static void mapper215_set_prg( running_machine *machine, int prg_base, int prg_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	if (map215_reg[0] & 0x80)
	{
		prg16_89ab(machine, (map215_reg[0] & 0xf0) | (map215_reg[1] & 0x10));
		prg16_cdef(machine, (map215_reg[0] & 0xf0) | (map215_reg[1] & 0x10));
	}
	else
	{
		state->mmc_prg_base = (map215_reg[1] & 0x08) ? 0x20 : (map215_reg[1] & 0x10);
		state->mmc_prg_mask = (map215_reg[1] & 0x08) ? 0x1f : 0x0f;

		mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
	}
}

static void mapper215_set_chr( running_machine *machine, UINT8 chr )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	int chr_base = (map215_reg[1] & 0x04) ? 0x100 : ((map215_reg[1] & 0x10) << 3);
	int chr_mask = (map215_reg[1] & 0x04) ? 0xff : 0x7f;

	chr1_x(machine, chr_page ^ 0, chr_base | ((state->mmc_vrom_bank[0] & ~0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 1, chr_base | ((state->mmc_vrom_bank[0] |  0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 2, chr_base | ((state->mmc_vrom_bank[1] & ~0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 3, chr_base | ((state->mmc_vrom_bank[1] |  0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 4, chr_base | (state->mmc_vrom_bank[2] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 5, chr_base | (state->mmc_vrom_bank[3] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 6, chr_base | (state->mmc_vrom_bank[4] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 7, chr_base | (state->mmc_vrom_bank[5] & chr_mask), chr);
}

static WRITE8_HANDLER( mapper215_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper215_l_w, offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	if (offset == 0x1000)
	{
		map215_reg[0] = data;
		mapper215_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
	}
	else if (offset == 0x1001)
	{
		map215_reg[1] = data;
		mapper215_set_chr(space->machine, state->mmc_chr_source);
	}
	else if (offset == 0x1007)
	{
		state->mmc_cmd1 = 0;
		map215_reg[2] = data;
		mapper215_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
		mapper215_set_chr(space->machine, state->mmc_chr_source);
	}
}

static WRITE8_HANDLER( mapper215_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	static const UINT8 conv_table[8] = {0,2,5,3,6,1,7,4};
	LOG_MMC(("mapper215_w, offset: %04x, data: %02x\n", offset, data));

	if (!map215_reg[2]) // in this case we act like MMC3, only with alt prg/chr handlers
	{
		switch (offset & 0x6001)
		{
		case 0x0000:
			MMC3_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (MMC3_helper & 0x40)
				mapper215_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

			/* Has CHR Mode changed? */
			if (MMC3_helper & 0x80)
				mapper215_set_chr(space->machine, state->mmc_chr_source);
			break;

		case 0x0001:
			cmd = state->mmc_cmd1 & 0x07;
			switch (cmd)
			{
			case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
			case 2: case 3: case 4: case 5:
				state->mmc_vrom_bank[cmd] = data;
				mapper215_set_chr(space->machine, state->mmc_chr_source);
				break;
			case 6:
			case 7:
				state->mmc_prg_bank[cmd - 6] = data;
				mapper215_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
				break;
			}
			break;

		default:
			mapper4_w(space, offset, data);
			break;
		}
	}
	else
	{
		switch (offset & 0x6001)
		{
		case 0x0000:
			break;

		case 0x0001:
			cmd = state->mmc_cmd1 & 0x07;
			if (map215_reg[3] && ((map215_reg[0] & 0x80) == 0 || cmd < 6))	// if we use the prg16 banks and cmd=6,7 DON'T enter!
			{
				map215_reg[3] = 0;
				switch (cmd)
				{
				case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
				case 2: case 3: case 4: case 5:
					state->mmc_vrom_bank[cmd] = data;
					mapper215_set_chr(space->machine, state->mmc_chr_source);
					break;
				case 6:
				case 7:
					state->mmc_prg_bank[cmd - 6] = data;
					mapper215_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
					break;
				}
			}
			break;

		case 0x2000:
			data = (data & 0xc0) | conv_table[data & 0x07];
			MMC3_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (MMC3_helper & 0x40)
				mapper215_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

			/* Has CHR Mode changed? */
			if (MMC3_helper & 0x80)
				mapper215_set_chr(space->machine, state->mmc_chr_source);

			map215_reg[3] = 1;
			break;

//      case 0x2001:
//          mapper4_w(space, 0x4001, data);
//          break;

		case 0x4000:
			set_nt_mirroring(space->machine, ((data >> 7) | data) & 0x01 ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;

		case 0x4001:
			mapper4_w(space, 0x6001, data);
			break;

//      case 0x6000:
//          mapper4_w(space, 0x6000, data);
//          break;

		case 0x6001:
			mapper4_w(space, 0x4000, data);
			mapper4_w(space, 0x4001, data);
			break;

		default:
			mapper4_w(space, offset, data);
			break;
		}
	}
}

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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper216_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, offset);
	chr8(space->machine, offset >> 1, state->mmc_chr_source);
}

/*************************************************************

    Mapper 217

    Known Boards: Unknown Bootleg Multigame Board
    Games: Golden Card 6 in 1

    MMC3 clone

    In MESS: Supported.

*************************************************************/

// remove mask & base parameters!
static void mapper217_set_prg( running_machine *machine, int prg_base, int prg_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	state->mmc_prg_base = (map217_reg[1] & 0x08) ? 0 : (map217_reg[1] & 0x10);
	state->mmc_prg_mask = (map217_reg[1] & 0x08) ? 0x1f : 0x0f;

	state->mmc_prg_base |= ((map217_reg[1] & 0x03) << 5);

	mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
}

static void mapper217_set_chr( running_machine *machine, UINT8 chr )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	int chr_base = (map217_reg[1] & 0x08) ? 0x00 : ((map217_reg[1] & 0x10) << 3);
	int chr_mask = (map217_reg[1] & 0x08) ? 0xff : 0x7f;

	chr_base |= ((map217_reg[1] & 0x03) << 8);

	chr1_x(machine, chr_page ^ 0, chr_base | ((state->mmc_vrom_bank[0] & ~0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 1, chr_base | ((state->mmc_vrom_bank[0] |  0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 2, chr_base | ((state->mmc_vrom_bank[1] & ~0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 3, chr_base | ((state->mmc_vrom_bank[1] |  0x01) & chr_mask), chr);
	chr1_x(machine, chr_page ^ 4, chr_base | (state->mmc_vrom_bank[2] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 5, chr_base | (state->mmc_vrom_bank[3] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 6, chr_base | (state->mmc_vrom_bank[4] & chr_mask), chr);
	chr1_x(machine, chr_page ^ 7, chr_base | (state->mmc_vrom_bank[5] & chr_mask), chr);
}

static WRITE8_HANDLER( mapper217_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 bank;
	LOG_MMC(("mapper217_l_w, offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	if (offset == 0x1000)
	{
		map217_reg[0] = data;
		if (data & 0x80)
		{
			bank = (data & 0x0f) | ((map217_reg[1] & 0x03) << 4);
			prg16_89ab(space->machine, bank);
			prg16_cdef(space->machine, bank);
		}
		else
			mapper217_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
	}
	else if (offset == 0x1001)
	{
		map217_reg[1] = data;
		mapper217_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
	}
	else if (offset == 0x1007)
	{
		map217_reg[2] = data;
	}
}

static WRITE8_HANDLER( mapper217_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	static const UINT8 conv_table[8] = {0, 6, 3, 7, 5, 2, 4, 1};
	LOG_MMC(("mapper217_w, offset: %04x, data: %02x\n", offset, data));

	if (!map217_reg[2]) // in this case we act like MMC3, only with alt prg/chr handlers
	{
		switch (offset & 0x6001)
		{
		case 0x0000:
			MMC3_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (MMC3_helper & 0x40)
				mapper217_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

			/* Has CHR Mode changed? */
			if (MMC3_helper & 0x80)
				mapper217_set_chr(space->machine, state->mmc_chr_source);
			break;

		case 0x0001:
			cmd = state->mmc_cmd1 & 0x07;
			switch (cmd)
			{
			case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
			case 2: case 3: case 4: case 5:
				state->mmc_vrom_bank[cmd] = data;
				mapper217_set_chr(space->machine, state->mmc_chr_source);
				break;
			case 6:
			case 7:
				state->mmc_prg_bank[cmd - 6] = data;
				mapper217_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
				break;
			}
			break;

		default:
			mapper4_w(space, offset, data);
			break;
		}
	}
	else
	{
		switch (offset & 0x6001)
		{
		case 0x0000:
			mapper4_w(space, 0xc000, data);
			break;

		case 0x0001:
			data = (data & 0xc0) | conv_table[data & 0x07];
			MMC3_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (MMC3_helper & 0x40)
				mapper217_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

			/* Has CHR Mode changed? */
			if (MMC3_helper & 0x80)
				mapper217_set_chr(space->machine, state->mmc_chr_source);

			map217_reg[3] = 1;
			break;

		case 0x2000:
			cmd = state->mmc_cmd1 & 0x07;
			if (map217_reg[3])
			{
				map217_reg[3] = 0;
				switch (cmd)
				{
				case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
				case 2: case 3: case 4: case 5:
					state->mmc_vrom_bank[cmd] = data;
					mapper217_set_chr(space->machine, state->mmc_chr_source);
					break;
				case 6:
				case 7:
					state->mmc_prg_bank[cmd - 6] = data;
					mapper217_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
					break;
				}
			}
			break;


		case 0x2001:
			set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;

		default:
			mapper4_w(space, offset, data);
			break;
		}
	}
}

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
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper221_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x4000)
	{
		set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		offset = (offset >> 1) & 0xff;

		if (state->mmc_cmd1 != offset)
		{
			state->mmc_cmd1 = offset;
			mapper221_set_prg(space->machine, state->mmc_cmd1, state->mmc_cmd2);
		}
	}
	else
	{
		offset &= 0x07;

		if (state->mmc_cmd2 != offset)
		{
			state->mmc_cmd2 = offset;
			mapper221_set_prg(space->machine, state->mmc_cmd1, state->mmc_cmd2);
		}
	}
}

/*************************************************************

    Mapper 222

    Known Boards: Unknown Bootleg Board
    Games: Dragon Ninja (Bootleg), Super Mario Bros. 8

    In MESS: Unsupported.

*************************************************************/

/* Scanline based IRQ ? */
static void mapper222_irq( running_device *device, int scanline, int vblank, int blanked )
{
	nes_state *state = (nes_state *)device->machine->driver_data;
	if (scanline < PPU_BOTTOM_VISIBLE_SCANLINE)
	{
		if (!state->IRQ_count || ++state->IRQ_count < 240)
			return;

		state->IRQ_count = 0;
		LOG_MMC(("irq fired, scanline: %d (MAME %d, beam pos: %d)\n", scanline,
				video_screen_get_vpos(device->machine->primary_screen), video_screen_get_hpos(device->machine->primary_screen)));
		cpu_set_input_line(state->maincpu, M6502_IRQ_LINE, HOLD_LINE);
	}
}

static WRITE8_HANDLER( mapper222_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 bank;
	LOG_MMC(("mapper222_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7003)
	{
	case 0x0000:
		prg8_89(space->machine, data);
		break;
	case 0x1000:
		set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 0x2000:
		prg8_ab(space->machine, data);
		break;
	case 0x3000:
	case 0x3002:
	case 0x4000:
	case 0x4002:
	case 0x5000:
	case 0x5002:
	case 0x6000:
	case 0x6002:
		bank = ((offset & 0x7000) - 0x3000) / 0x0800 + ((offset & 0x0002) >> 3);
		chr1_x(space->machine, bank, data, CHRROM);
		break;
	case 0x7000:
		state->IRQ_count = data;
		break;
	}
}

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
	set_nt_mirroring(space->machine, (offset & 0x2000) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

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
	nes_state *state = (nes_state *)space->machine->driver_data;
	int hi_bank;
	int size_16;
	int bank;

	LOG_MMC(("mapper226_w, offset: %04x, data: %02x\n", offset, data));

	if (offset & 0x01)
		state->mmc_cmd2 = data;
	else
		state->mmc_cmd1 = data;

	set_nt_mirroring(space->machine, BIT(state->mmc_cmd1, 6) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	hi_bank = state->mmc_cmd1 & 0x01;
	size_16 = state->mmc_cmd1 & 0x20;
	bank = ((state->mmc_cmd1 & 0x1e) >> 1) | ((state->mmc_cmd1 & 0x80) >> 3) | ((state->mmc_cmd2 & 0x01) << 5);

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

	set_nt_mirroring(space->machine, BIT(data, 1) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
}

/*************************************************************

    Mapper 228

    Known Boards: Bootleg Board by Active Enterprise
    Games: Action 52, Cheetah Men II

    In MESS: Partially Supported.

*************************************************************/

static WRITE8_HANDLER( mapper228_w )
{
	int pbank, pchip, cbank;
	UINT8 pmode;
	LOG_MMC(("mapper228_w, offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring(space->machine, (offset & 0x2000) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	cbank = (data & 0x03) | ((offset & 0x0f) << 2);
	chr8(space->machine, cbank, CHRROM);

	pbank = (offset & 0x7c0) >> 6;
	pchip = (offset & 0x1800) >> 11;
	pmode = offset & 0x20;

	switch (pchip)
	{
		case 0: break;			// we are already at the correct bank
		case 1: pbank |= 0x10; break;	// chip 1 starts at block 16
		case 2: break;			// chip 2 was an empty socket
		case 3: pbank |= 0x20; break;	// chip 3 starts at block 32
	}

	if (pmode)
	{
		prg16_89ab(space->machine, pbank);
		prg16_cdef(space->machine, pbank);
	}
	else
		prg32(space->machine, pbank >> 1);
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

	set_nt_mirroring(space->machine, BIT(data, 5) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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

    In MESS: Partially Supported. It would need a reset
       to work (not possible yet)

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
		set_nt_mirroring(space->machine, BIT(data, 6) ? PPU_MIRROR_VERT : PPU_MIRROR_HORZ);
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

	set_nt_mirroring(space->machine, BIT(data, 7) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	prg16_89ab(space->machine, (offset & 0x1e));
	prg16_cdef(space->machine, (offset & 0x1e) | ((offset & 0x20) ? 1 : 0));
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
	nes_state *state = (nes_state *)machine->driver_data;
	prg16_89ab(machine, (state->mmc_cmd2 & 0x03) | ((state->mmc_cmd1 & 0x18) >> 1));
	prg16_cdef(machine, 0x03 | ((state->mmc_cmd1 & 0x18) >> 1));
}

static WRITE8_HANDLER( mapper232_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper232_w, offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x2000)
		state->mmc_cmd1 = data;
	else
		state->mmc_cmd2 = data;

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
        a second variant DQ8 and requires no NT mirroring.

    A crc check is required to support Dragon Quest VIII (which
    uses a slightly different board)

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper242_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper242_w, offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, offset >> 3);

	if (!state->crc_hack)	// DQ8 board does not have this / Zan Shi does
	{
		switch (data & 0x03)
		{
			case 0: set_nt_mirroring(space->machine, PPU_MIRROR_VERT); break;
			case 1: set_nt_mirroring(space->machine, PPU_MIRROR_HORZ); break;
			case 2: set_nt_mirroring(space->machine, PPU_MIRROR_LOW); break;
			case 3: set_nt_mirroring(space->machine, PPU_MIRROR_HIGH); break;
		}
	}
}

/*************************************************************

    Mapper 243

    Known Boards: Bootleg Board by Sachen (74SL374A)
    Games: Poker III

    The board is similar to the one of mapper 150.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper243_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper243_l_w, offset: %04x, data: %02x\n", offset, data));

	/* write happens only if we are at 0x4100 + k * 0x200, but 0x4100 is offset = 0 */
	if (!(offset & 0x100))
	{
		if (!(offset & 0x01))
			state->mmc_cmd1 = data;
		else
		{
			switch (state->mmc_cmd1 & 0x07)
			{
			case 0x00:
				prg32(space->machine, 0);
				chr8(space->machine, 3, CHRROM);
				break;
			case 0x02:
				state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & ~0x08) | (data << 3 & 0x08);
				chr8(space->machine, state->mmc_vrom_bank[0], CHRROM);
				break;
			case 0x04:
				state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & ~0x01) | (data << 0 & 0x01);
				chr8(space->machine, state->mmc_vrom_bank[0], CHRROM);
				break;
			case 0x05:
				prg32(space->machine, data & 0x01);
				break;
			case 0x06:
				state->mmc_vrom_bank[0] = (state->mmc_vrom_bank[0] & ~0x06) | (data << 1 & 0x06);
				chr8(space->machine, state->mmc_vrom_bank[0], CHRROM);
				break;
			case 0x07:
				set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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
    Games: Ying Xiong Yuan Yi Jing Chuan Qi, Yong Zhe Dou E
         Long - Dragon Quest VII

    MMC3 clone. More info to come.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper245_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 cmd, map245_helper;
	LOG_MMC(("mapper245_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0001:
		cmd = state->mmc_cmd1 & 0x0f;
		switch (cmd)
		{
		case 0: 	// in this case we set prg_base in addition to state->mmc_vrom_bank!
			state->mmc_prg_base = (data << 5) & 0x40;
			state->mmc_prg_mask = 0x3f;
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
		case 1: case 2: case 3: case 4: case 5:
			state->mmc_vrom_bank[cmd] = data;
			if (state->chr_chunks > 0)
				mapper4_set_chr(space->machine, CHRROM, state->mmc_chr_base, state->mmc_chr_mask);
			else	// according to Disch's docs, state->mmc_cmd1&0x80 swaps 4k CHRRAM banks
			{
				map245_helper = /*(state->mmc_cmd1 & 0x80) ? 1 : */0;
				chr4_0(space->machine, 0 ^ map245_helper, CHRRAM);
				chr4_4(space->machine, 1 ^ map245_helper, CHRRAM);
			}
			break;
		case 6:
		case 7:
			state->mmc_prg_bank[cmd - 6] = data;
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			break;
		}
		break;

	case 0x2001:
		break;

	default:
		mapper4_w(space, offset, data);
		break;
	}
}

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
	nes_state *state = (nes_state *)space->machine->driver_data;
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
		state->wram[offset] = data;
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

    Known Boards: Waixing with Security Chip
    Games: Duo Bao Xiao Ying Hao - Guang Ming yu An Hei Chuan Shuo,
           Myth Struggle, San Shi Liu Ji, Shui Hu Zhuan

    MMC3 clone

    In MESS: Partially Supported.

*************************************************************/

static void mapper249_set_prg( running_machine *machine, int prg_base, int prg_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	int i;

	if (map249_reg)
	{
		for (i = 0; i < 4; i ++)	// should this apply only to bank[0],bank[1]?
		{
			state->mmc_prg_bank[i] = ((state->mmc_prg_bank[i] & 0x01)) | ((state->mmc_prg_bank[i] >> 3) & 0x02) |
					((state->mmc_prg_bank[i] >> 1) & 0x04) | ((state->mmc_prg_bank[i] << 2) & 0x18);
		}
	}

	mapper4_set_prg(machine, prg_base, prg_mask);
}

static void mapper249_set_chr( running_machine *machine, int chr_base, int chr_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	int i;

	if (map249_reg)
	{
		for (i = 0; i < 6; i ++)
		{
			state->mmc_vrom_bank[i] = ((state->mmc_vrom_bank[i] & 0x03)) | ((state->mmc_vrom_bank[i] >> 1) & 0x04) |
						((state->mmc_vrom_bank[i] >> 4) & 0x08) | ((state->mmc_vrom_bank[i] >> 2) & 0x10) |
						((state->mmc_vrom_bank[i] << 3) & 0x20) | ((state->mmc_vrom_bank[i] << 2) & 0xc0);
		}
	}

	chr1_x(machine, chr_page ^ 0, chr_base | ((state->mmc_vrom_bank[0] & ~0x01) & chr_mask), state->mmc_chr_source);
	chr1_x(machine, chr_page ^ 1, chr_base | ((state->mmc_vrom_bank[0] |  0x01) & chr_mask), state->mmc_chr_source);
	chr1_x(machine, chr_page ^ 2, chr_base | ((state->mmc_vrom_bank[1] & ~0x01) & chr_mask), state->mmc_chr_source);
	chr1_x(machine, chr_page ^ 3, chr_base | ((state->mmc_vrom_bank[1] |  0x01) & chr_mask), state->mmc_chr_source);
	chr1_x(machine, chr_page ^ 4, chr_base | (state->mmc_vrom_bank[2] & chr_mask), state->mmc_chr_source);
	chr1_x(machine, chr_page ^ 5, chr_base | (state->mmc_vrom_bank[3] & chr_mask), state->mmc_chr_source);
	chr1_x(machine, chr_page ^ 6, chr_base | (state->mmc_vrom_bank[4] & chr_mask), state->mmc_chr_source);
	chr1_x(machine, chr_page ^ 7, chr_base | (state->mmc_vrom_bank[5] & chr_mask), state->mmc_chr_source);
}

static WRITE8_HANDLER( mapper249_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper249_l_w, offset: %04x, data: %02x\n", offset, data));

	offset += 0x100;

	if (offset == 0x1000)
	{
		map249_reg = data & 0x02;
		mapper249_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
		mapper249_set_chr(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
	}
}

static WRITE8_HANDLER( mapper249_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper249_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
	case 0x0000:
		MMC3_helper = state->mmc_cmd1 ^ data;
		state->mmc_cmd1 = data;

		/* Has PRG Mode changed? */
		if (MMC3_helper & 0x40)
			mapper249_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);

		/* Has CHR Mode changed? */
		if (MMC3_helper & 0x80)
			mapper249_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
		break;

	case 0x0001:
		cmd = state->mmc_cmd1 & 0x07;
		switch (cmd)
		{
		case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
		case 2: case 3: case 4: case 5:
			state->mmc_vrom_bank[cmd] = data;
			mapper249_set_chr(space->machine, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 6:
		case 7:
			state->mmc_prg_bank[cmd - 6] = data;
			mapper249_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			break;
		}
		break;

	default:
		mapper4_w(space, offset, data);
		break;
	}
}

/*************************************************************

    Mapper 250

    Known Boards: Unknown Bootleg Board by Nitra
    Games: Time Diver Avenger

    This acts basically like a MMC3 with different use of write
    address.

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper250_w )
{
	LOG_MMC(("mapper250_w, offset: %04x, data: %02x\n", offset, data));

	mapper4_w(space, (offset & 0x6000) | ((offset & 0x400) >> 10), offset & 0xff);
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

    This board uses Konami IRQ

    In MESS: Unsupported.

*************************************************************/

static WRITE8_HANDLER( mapper252_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 mmc_helper, bank;
	LOG_MMC(("mapper252_w, offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
	case 0x0000:
		prg8_89(space->machine, data);
		break;
	case 0x2000:
		prg8_ab(space->machine, data);
		break;
	case 0x3000:
	case 0x4000:
	case 0x5000:
	case 0x6000:
		bank = ((offset & 0x7000) - 0x3000) / 0x0800 + ((offset & 0x0008) >> 3);
		mmc_helper = offset & 0x04;
		if (mmc_helper)
			state->mmc_vrom_bank[bank] = (state->mmc_vrom_bank[bank] & 0x0f) | ((data & 0x0f) << 4);
		else
			state->mmc_vrom_bank[bank] = (state->mmc_vrom_bank[bank] & 0xf0) | (data & 0x0f);
		chr1_x(space->machine, bank, state->mmc_vrom_bank[bank], CHRROM);
		break;
	case 0x7000:
		switch (offset & 0x0c)
		{
		case 0x00:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0xf0) | (data & 0x0f);
			break;
		case 0x04:
			state->IRQ_count_latch = (state->IRQ_count_latch & 0x0f) | ((data & 0x0f) << 4);
			break;
		case 0x08:
			state->IRQ_enable = data & 0x02;
			state->IRQ_enable_latch = data & 0x01;
			if (data & 0x02)
				state->IRQ_count = state->IRQ_count_latch;
			break;
		case 0x0c:
			state->IRQ_enable = state->IRQ_enable_latch;
			break;
		}
		break;
	}
}

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

    In MESS: Preliminary support.

*************************************************************/

static WRITE8_HANDLER( mapper255_w )
{
	UINT8 map255_helper1 = (offset >> 12) ? 0 : 1;
	UINT8 map255_helper2 = ((offset >> 8) & 0x40) | ((offset >> 6) & 0x3f);

	LOG_MMC(("mapper255_w, offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring(space->machine, (offset & 0x2000) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
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
/*  INES   DESC                          LOW_W, LOW_R, MED_W, HIGH_W, PPU_latch, scanline CB, hblank CB */
	{  0, "No Mapper",                 NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{  1, "MMC1",                      NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL },
	{  2, "Ux-ROM",                    NULL, NULL, NULL, mapper2_w, NULL, NULL, NULL },
	{  3, "Cx-ROM",                    NULL, NULL, NULL, mapper3_w, NULL, NULL, NULL },
	{  4, "MMC3",                      NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq },
	{  5, "MMC5",                      mapper5_l_w, mapper5_l_r, NULL, mapper5_w, NULL, NULL, mapper5_irq },
	{  6, "FFE F4xxxx",                mapper6_l_w, NULL, NULL, mapper6_w, NULL, NULL, ffe_irq },
	{  7, "Ax-ROM",                    NULL, NULL, NULL, mapper7_w, NULL, NULL, NULL },
	{  8, "FFE F3xxxx",                NULL, NULL, NULL, mapper8_w, NULL, NULL, NULL },
	{  9, "MMC2",                      NULL, NULL, NULL, mapper9_w, mapper9_latch, NULL, NULL},
	{ 10, "MMC4",                      NULL, NULL, NULL, mapper10_w, mapper9_latch, NULL, NULL },
	{ 11, "Color Dreams",              NULL, NULL, NULL, mapper11_w, NULL, NULL, NULL },
	{ 12, "Rex Soft DBZ5",             mapper12_l_w, mapper12_l_r, NULL, mapper12_w, NULL, NULL, mapper4_irq },
	{ 13, "CP-ROM",                    NULL, NULL,	NULL, mapper13_w, NULL, NULL, NULL },
	{ 14, "Rex Soft SL1632",           NULL, NULL, NULL, mapper14_w, NULL, NULL, mapper4_irq },
	{ 15, "100-in-1",                  NULL, NULL, NULL, mapper15_w, NULL, NULL, NULL },
	{ 16, "Bandai LZ93D50 - 24C02",    NULL, NULL, mapper16_m_w, mapper16_w, NULL,NULL,  bandai_irq },
	{ 17, "FFE F8xxx",                 mapper17_l_w, NULL, NULL, NULL, NULL, NULL, ffe_irq },
	{ 18, "Jaleco SS88006",            NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq },
	{ 19, "Namcot 106 + N106",         mapper19_l_w, mapper19_l_r, NULL, mapper19_w, NULL, NULL, namcot_irq },
	{ 20, "Famicom Disk System",       NULL, NULL, NULL, NULL, NULL, NULL, fds_irq },
	{ 21, "Konami VRC 4a",             NULL, NULL, NULL, konami_vrc4a_w, NULL, NULL, konami_irq },
	{ 22, "Konami VRC 2a",             NULL, NULL, NULL, konami_vrc2a_w, NULL, NULL, NULL },
	{ 23, "Konami VRC 2b",             NULL, NULL, NULL, konami_vrc2b_w, NULL, NULL, konami_irq },
	{ 24, "Konami VRC 6a",             NULL, NULL, NULL, konami_vrc6a_w, NULL, NULL, konami_irq },
	{ 25, "Konami VRC 4b",             NULL, NULL, NULL, konami_vrc4b_w, NULL, NULL, konami_irq },
	{ 26, "Konami VRC 6b",             NULL, NULL, NULL, konami_vrc6b_w, NULL, NULL, konami_irq },
// 27 World Hero
// 28, 29, 30, 31 Unused
	{ 32, "Irem G-101",                NULL, NULL, NULL, mapper32_w, NULL, NULL, NULL },
	{ 33, "Taito TC0190FMC",           NULL, NULL, NULL, mapper33_w, NULL, NULL, NULL },
	{ 34, "Nina-001",                  NULL, NULL, mapper34_m_w, mapper34_w, NULL, NULL, NULL },
	{ 35, "SC-127",                    NULL, NULL, NULL, mapper35_w, NULL, NULL, mapper35_irq },
// 35 Unused
	{ 36, "TXC Policeman",             NULL, NULL, NULL, mapper36_w, NULL, NULL, NULL },
	{ 37, "ZZ Board",                  NULL, NULL, mapper37_m_w, mapper4_w, NULL, NULL, mapper4_irq },
	{ 38, "Crime Buster",              NULL, NULL, mapper38_m_w, NULL, NULL, NULL, NULL },
	{ 39, "Subor Study n Game",        NULL, NULL, NULL, mapper39_w, NULL, NULL, NULL },
	{ 40, "SMB2 JPN (bootleg)",        NULL, NULL, NULL, mapper40_w, NULL, NULL, mapper40_irq },
	{ 41, "Caltron 6-in-1",            NULL, NULL, mapper41_m_w, mapper41_w, NULL, NULL, NULL },
	{ 42, "Mario Baby",                NULL, NULL, NULL, mapper42_w, NULL, NULL, NULL },
	{ 43, "150-in-1",                  NULL, NULL, NULL, mapper43_w, NULL, NULL, NULL },
	{ 44, "SuperBig 7-in-1",           NULL, NULL, NULL, mapper44_w, NULL, NULL, mapper4_irq },
	{ 45, "X-in-1 MMC3",               NULL, NULL, mapper45_m_w, mapper4_w, NULL, NULL, mapper4_irq },
	{ 46, "Rumblestation",             NULL, NULL, mapper46_m_w, mapper46_w, NULL, NULL, NULL },
	{ 47, "QJ Board",                  NULL, NULL, mapper47_m_w, mapper4_w, NULL, NULL, mapper4_irq },
	{ 48, "Taito TC0190FMC PAL16R4",   NULL, NULL, NULL, mapper48_w, NULL, NULL, mapper4_irq },
	{ 49, "Super HIK 4-in-1",          NULL, NULL, mapper49_m_w, mapper4_w, NULL, NULL, mapper4_irq },
	{ 50, "SMB2 JPN (bootleg 2)",      mapper50_l_w, NULL, NULL, NULL, NULL, NULL, mapper50_irq },
	{ 51, "Ballgames 11-in-1",         NULL, NULL, mapper51_m_w, mapper51_w, NULL, NULL, NULL },
	{ 52, "Mario 7-in-1",              NULL, NULL, mapper52_m_w, mapper4_w, NULL, NULL, mapper4_irq },
// 53 Supervision 16-in-1
	{ 54, "Novel Diamond X-in-1",      NULL, NULL, NULL, mapper54_w, NULL, NULL, NULL },
// 55 Genius SMB
// 56 Kaiser KS202
	{ 57, "GKA 6-in-1",                NULL, NULL, NULL, mapper57_w, NULL, NULL, NULL },
	{ 58, "GKB X-in-1",                NULL, NULL, NULL, mapper58_w, NULL, NULL, NULL },
// 59 Unused
// 60 4-in-1 Reset based
	{ 61, "Tetris Family 20-in-1",     NULL, NULL, NULL, mapper61_w, NULL, NULL, NULL },
	{ 62, "Super 700-in-1",            NULL, NULL, NULL, mapper62_w, NULL, NULL, NULL },
// 63 CH001 X-in-1
	{ 64, "Tengen 800032",             NULL, NULL, NULL, mapper64_w, NULL, NULL, mapper64_irq },
	{ 65, "Irem H3001",                NULL, NULL, NULL, mapper65_w, NULL, NULL, irem_irq },
	{ 66, "Gx-ROM",                    NULL, NULL, NULL, mapper66_w, NULL, NULL, NULL },
	{ 67, "SunSoft 3",                 NULL, NULL, NULL, mapper67_w, NULL, NULL, mapper67_irq },
	{ 68, "SunSoft 4",                 NULL, NULL, NULL, mapper68_w, NULL, NULL, NULL },
	{ 69, "SunSoft FME",               NULL, NULL, NULL, mapper69_w, NULL, NULL, mapper69_irq },
	{ 70, "74161/32 Bandai",           NULL, NULL, NULL, mapper70_w, NULL, NULL, NULL },
	{ 71, "Camerica",                  NULL, NULL, NULL, mapper71_w, NULL, NULL, NULL },
	{ 72, "74161/32 Jaleco",           NULL, NULL, NULL, mapper72_w, NULL, NULL, NULL },
	{ 73, "Konami VRC 3",              NULL, NULL, NULL, mapper73_w, NULL, NULL, konami_irq },
	{ 74, "Waixing Type A",            NULL, NULL, NULL, mapper74_w, NULL, NULL, mapper4_irq },
	{ 75, "Konami VRC 1",              NULL, NULL, NULL, mapper75_w, NULL, NULL, NULL },
	{ 76, "Namco 3446",                NULL, NULL, NULL, mapper76_w, NULL, NULL, NULL },
	{ 77, "Irem LROG017",              NULL, NULL, NULL, mapper77_w, NULL, NULL, NULL },
	{ 78, "Irem Holy Diver",           NULL, NULL, NULL, mapper78_w, NULL, NULL, NULL },
	{ 79, "Nina-03",                   mapper79_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 80, "Taito X1-005 Ver. A",       NULL, NULL, mapper80_m_w, NULL, NULL, NULL, NULL },
// 81 Unused
	{ 82, "Taito X1-017",              NULL, NULL, mapper82_m_w, NULL, NULL, NULL, NULL },
	{ 83, "Cony",                      mapper83_l_w, mapper83_l_r, NULL, mapper83_w, NULL, NULL, NULL },
	{ 84, "Pasofami hacked images?",   NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 85, "Konami VRC 7",              NULL, NULL, NULL, konami_vrc7_w, NULL, NULL, konami_irq },
	{ 86, "Jaleco JF13",               NULL, NULL, mapper86_m_w, NULL, NULL, NULL, NULL },
	{ 87, "74139/74",                  NULL, NULL, mapper87_m_w, NULL, NULL, NULL, NULL },
	{ 88, "Namcot 34x3",               NULL, NULL, NULL, mapper88_w, NULL, NULL, NULL },
	{ 89, "Sunsoft 2b",                NULL, NULL, NULL, mapper89_w, NULL, NULL, NULL },
// 90 JY Company Type A
	{ 91, "HK-SF3 (bootleg)",          NULL, NULL, mapper91_m_w, NULL, NULL, NULL, mapper4_irq },
	{ 92, "Jaleco JF19 / JF21",        NULL, NULL, NULL, mapper92_w, NULL, NULL, NULL },
	{ 93, "Sunsoft 2A",                NULL, NULL, NULL, mapper93_w, NULL, NULL, NULL },
	{ 94, "Capcom LS161",              NULL, NULL, NULL, mapper94_w, NULL, NULL, NULL },
	{ 95, "Namcot 3425",               NULL, NULL, NULL, mapper95_w, NULL, NULL, NULL },
	{ 96, "Bandai OekaKids",           NULL, NULL, NULL, mapper96_w, NULL, NULL, NULL },
	{ 97, "Irem Kaiketsu",             NULL, NULL, NULL, mapper97_w, NULL, NULL, NULL },
// 98 Unused
// 99 VS. system
// 100 images hacked to work with nesticle?
	{ 101, "Jaleco?? LS161",           NULL, NULL, mapper101_m_w, mapper101_w, NULL, NULL, NULL },
// 102 Unused
// 103 Bootleg cart 2708
	{ 104, "Golden Five",              NULL, NULL, NULL, mapper104_w, NULL, NULL, NULL },
// 105 Nintendo World Championship
	{ 106, "SMB3 (bootleg)",           NULL, NULL, NULL, mapper106_w, NULL, NULL, mapper106_irq },
	{ 107, "Magic Dragon",             NULL, NULL, NULL, mapper107_w, NULL, NULL, NULL },
// 108 Whirlwind
// 109, 110 Unused
// 111 Ninja Ryuukenden Chinese?
	{ 112, "Asder",                    NULL, NULL, NULL, mapper112_w, NULL, NULL, NULL },
	{ 113, "Sachen/Hacker/Nina",       mapper113_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 114, "The Lion King",            NULL, NULL, mapper114_m_w, mapper114_w, NULL, NULL, mapper4_irq },
	{ 115, "Kasing",                   NULL, NULL, mapper115_m_w, mapper115_w, NULL, NULL, mapper4_irq },
// 116 Someri Team
	{ 117, "Future Media",             NULL, NULL, NULL, mapper117_w, NULL, NULL, mapper117_irq },
	{ 118, "TKS-ROM / TLS-ROM",        NULL, NULL, NULL, mapper118_w, NULL, NULL, mapper4_irq },
	{ 119, "TQ-ROM",                   NULL, NULL, NULL, mapper119_w, NULL, NULL, mapper4_irq },
// 120 FDS bootleg
	{ 121, "K - Panda Prince",         mapper121_l_w, mapper121_l_r, NULL, mapper121_w, NULL, NULL, mapper4_irq },
// 122 Unused
// 123 K H2288
// 124, 125 Unused
// 126 Powerjoy 84-in-1
// 127, 128, 129, 130, 131 Unused
	{ 132, "TXC T22211A",              mapper132_l_w, mapper132_l_r, NULL, mapper132_w, NULL, NULL, NULL },
	{ 133, "Sachen SA72008",           mapper133_l_w, NULL, mapper133_m_w, NULL, NULL, NULL, NULL },
	{ 134, "Family 4646B",             NULL, NULL, mapper134_m_w, mapper4_w, NULL, NULL, mapper4_irq },
// 135 Unused
	{ 136, "Sachen TCU02",             mapper136_l_w, mapper136_l_r, NULL, NULL, NULL, NULL, NULL },
	{ 137, "Sachen 8259D",             mapper137_l_w, NULL, mapper137_m_w, NULL, NULL, NULL, NULL },
	{ 138, "Sachen 8259B",             mapper138_l_w, NULL, mapper138_m_w, NULL, NULL, NULL, NULL },
	{ 139, "Sachen 8259C",             mapper139_l_w, NULL, mapper139_m_w, NULL, NULL, NULL, NULL },
	{ 140, "Jaleco JF11",              NULL, NULL, mapper_140_m_w, NULL, NULL, NULL, NULL },
	{ 141, "Sachen 8259A",             mapper141_l_w, NULL, mapper141_m_w, NULL, NULL, NULL, NULL },
// 142 Kaiser KS7032
	{ 143, "Sachen TCA01",             NULL, mapper143_l_r, NULL, NULL, NULL, NULL, NULL },
	{ 144, "AGCI 50282",               NULL, NULL, NULL, mapper144_w, NULL, NULL, NULL }, //Death Race only
	{ 145, "Sachen SA72007",           mapper145_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 146, "Sachen SA-016-1M",         mapper79_l_w, NULL, NULL, NULL, NULL, NULL, NULL }, // basically same as Mapper 79 (Nina006)
	{ 147, "Sachen TCU01",             mapper147_l_w, NULL, mapper147_m_w, mapper147_w, NULL, NULL, NULL },
	{ 148, "Sachen SA0037",            NULL, NULL, NULL, mapper148_w, NULL, NULL, NULL },
	{ 149, "Sachen SA0036",            NULL, NULL, NULL, mapper149_w, NULL, NULL, NULL },
	{ 150, "Sachen 74LS374B",          mapper150_l_w, mapper150_l_r, mapper150_m_w, NULL, NULL, NULL, NULL },
// 151 Konami VS. system
	{ 152, "Taito 74161/161",          NULL, NULL, NULL, mapper152_w, NULL, NULL, NULL },
	{ 153, "Bandai LZ93D50",           NULL, NULL, mapper153_m_w, mapper153_w, NULL,NULL,  bandai_irq },
	{ 154, "Namcot 34xx",              NULL, NULL, NULL, mapper154_w, NULL, NULL, NULL },
	{ 155, "SK-ROM",                   NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL }, // diff compared to MMC1 concern WRAM (unsupported for MMC1 as well, atm)
	{ 156, "Open Corp. DAOU36",        NULL, NULL, NULL, mapper156_w, NULL, NULL, NULL },
	{ 157, "Bandai Datach Board",      NULL, NULL, mapper16_m_w, mapper16_w, NULL,NULL,  bandai_irq },	// no Datach Reader -> we fall back to mapper 16
	{ 158, "Tengen T800037",           NULL, NULL, NULL, mapper158_w, NULL, NULL, mapper64_irq },
	{ 159, "Bandai LZ93D50 - 24C01",   NULL, NULL, mapper16_m_w, mapper16_w, NULL,NULL,  bandai_irq },
// 160, 161, 162 Unused
// 163 Nanjing
	{ 164, "Final Fantasy V",          mapper164_l_w, NULL, NULL, mapper164_w, NULL, NULL, NULL },
// 165 Waixing SH2
	{ 166, "Subor Board Type 1",       NULL, NULL, NULL, mapper166_w, NULL, NULL, NULL },
	{ 167, "Subor Board Type 2",       NULL, NULL, NULL, mapper167_w, NULL, NULL, NULL },
// 168 Racermate Challenger II
// 169 Unused
// 170 Fujiya
	{ 171, "Kaiser KS7058",            NULL, NULL, NULL, mapper171_w, NULL, NULL, NULL },
	{ 172, "TXC T22211B",              mapper132_l_w, mapper132_l_r, NULL, mapper172_w, NULL, NULL, NULL },
	{ 173, "TXC T22211C",              mapper132_l_w, mapper173_l_r, NULL, mapper132_w, NULL, NULL, NULL },
// 174 Unused
// 175 Kaiser KS7022
	{ 176, "Zhi Li Xiao Zhuan Yuan",   mapper176_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 177, "Henggedianzi Board",       NULL, NULL, NULL, mapper177_w, NULL, NULL, NULL },
	{ 178, "San Guo Zhong Lie Zhuan",  mapper178_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 179, "Xing He Zhan Shi",         mapper179_l_w, NULL, NULL, mapper179_w, NULL, NULL, NULL },
	{ 180, "Nihon Bussan UN-ROM",      NULL, NULL, NULL, mapper180_w, NULL, NULL, NULL },
// 181 Unused
	{ 182, "Hosenkan",                 NULL, NULL, NULL, mapper182_w, NULL, NULL, mapper4_irq },
// 183 FDS bootleg
	{ 184, "Sunsoft 1",                NULL, NULL, mapper184_m_w, NULL, NULL, NULL, NULL },
	{ 185, "CN-ROM",                   NULL, NULL, NULL, mapper185_w, NULL, NULL, NULL },
// 186 Fukutake
	{ 187, "King of Fighters 96",      mapper187_l_w, mapper187_l_r, NULL, mapper187_w, NULL, NULL, mapper4_irq },
	{ 188, "Bandai Karaoke",           NULL, NULL, NULL, mapper188_w, NULL, NULL, NULL },
	{ 189, "TXC TW Board",             mapper189_l_w, NULL, mapper189_m_w, mapper189_w, NULL, NULL, mapper4_irq },
// 190 Unused
	{ 191, "Waixing Type B",           NULL, NULL, NULL, mapper191_w, NULL, NULL, mapper4_irq },
	{ 192, "Waixing Type C",           NULL, NULL, NULL, mapper192_w, NULL, NULL, mapper4_irq },
	{ 193, "Fighting Hero",            NULL, NULL, mapper193_m_w, NULL, NULL, NULL, NULL },
	{ 194, "Waixing Type D",           NULL, NULL, NULL, mapper194_w, NULL, NULL, mapper4_irq },
	{ 195, "Waixing Type E",           NULL, NULL, NULL, mapper195_w, NULL, NULL, mapper4_irq },
	{ 196, "Super Mario Bros. 11",     NULL, NULL, NULL, mapper196_w, NULL, NULL, mapper4_irq },
	{ 197, "Super Fighter 3",          NULL, NULL, NULL, mapper197_w, NULL, NULL, mapper4_irq },
	{ 198, "Waixing Type F",           NULL, NULL, NULL, mapper198_w, NULL, NULL, mapper4_irq },
	{ 199, "Waixing Type G",           NULL, NULL, NULL, mapper199_w, NULL, NULL, mapper4_irq },
	{ 200, "36-in-1",                  NULL, NULL, NULL, mapper200_w, NULL, NULL, NULL },
	{ 201, "21-in-1",                  NULL, NULL, NULL, mapper201_w, NULL, NULL, NULL },
	{ 202, "150-in-1",                 NULL, NULL, NULL, mapper202_w, NULL, NULL, NULL },
	{ 203, "35-in-1",                  NULL, NULL, NULL, mapper203_w, NULL, NULL, NULL },
	{ 204, "64-in-1",                  NULL, NULL, NULL, mapper204_w, NULL, NULL, NULL },
	{ 205, "15-in-1",                  NULL, NULL, mapper205_m_w, mapper4_w, NULL, NULL, mapper4_irq },
	{ 206, "MIMIC-1",                  NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq },
	{ 207, "Taito X1-005 Ver. B",      NULL, NULL, mapper207_m_w, NULL, NULL, NULL, NULL },
	{ 208, "Gouder G3717",             mapper208_l_w, mapper208_l_r, NULL, mapper208_w, NULL, NULL, mapper4_irq },
// 209 JY Company Type B
// 210 Some emu uses this as Mapper 19 without some features
// 211 JY Company Type C
	{ 212, "Super HIK 300-in-1",       NULL, NULL, NULL, mapper212_w, NULL, NULL, NULL },
	{ 213, "9999999-in-1",             NULL, NULL, NULL, mapper213_w, NULL, NULL, NULL },
	{ 214, "Super Gun 20-in-1",        NULL, NULL, NULL, mapper214_w, NULL, NULL, NULL },
	{ 215, "Super Game Boogerman",     mapper215_l_w, NULL, NULL, mapper215_w, NULL, NULL, mapper4_irq },
	{ 216, "RCM GS2015",               NULL, NULL, NULL, mapper216_w, NULL, NULL, NULL },
	{ 217, "Golden Card 6-in-1",       mapper217_l_w, NULL, NULL, mapper217_w, NULL, NULL, mapper4_irq },
// 218 Unused
// 219 Bootleg a9746
// 220 Summer Carnival '92??
	{ 221, "X-in-1 (N625092)",         NULL, NULL, NULL, mapper221_w, NULL, NULL, NULL },
	{ 222, "Dragonninja Bootleg",      NULL, NULL, NULL, mapper222_w, NULL, NULL, mapper222_irq },
// 223 Waixing Type I
// 224 Waixing Type J
	{ 225, "72-in-1 bootleg",          NULL, NULL, NULL, mapper225_w, NULL, NULL, NULL },
	{ 226, "76-in-1 bootleg",          NULL, NULL, NULL, mapper226_w, NULL, NULL, NULL },
	{ 227, "1200-in-1 bootleg",        NULL, NULL, NULL, mapper227_w, NULL, NULL, NULL },
	{ 228, "Action 52",                NULL, NULL, NULL, mapper228_w, NULL, NULL, NULL },
	{ 229, "31-in-1",                  NULL, NULL, NULL, mapper229_w, NULL, NULL, NULL },
	{ 230, "22-in-1",                  NULL, NULL, NULL, mapper230_w, NULL, NULL, NULL },
	{ 231, "20-in-1",                  NULL, NULL, NULL, mapper231_w, NULL, NULL, NULL },
	{ 232, "Quattro",                  NULL, NULL, mapper232_w, mapper232_w, NULL, NULL, NULL },
// 233 Super 22 Games
// 234 AVE Maxi 15
// 235 Golden Game x-in-1
// 236 Game 800-in-1
// 237 Unused
// 238 Bootleg 6035052
// 239 Unused
	{ 240, "Jing Ke Xin Zhuan",        mapper240_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 241, "Education 18-in-1",        NULL, mapper241_l_r, NULL, mapper241_w, NULL, NULL, NULL },
	{ 242, "Wai Xing Zhan Shi",        NULL, NULL, NULL, mapper242_w, NULL, NULL, NULL },
	{ 243, "Sachen 74LS374A",          mapper243_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 244, "Decathlon",                NULL, NULL, NULL, mapper244_w, NULL, NULL, NULL },
	{ 245, "Waixing Type H",           NULL, NULL, NULL, mapper245_w, NULL, NULL, mapper4_irq },
	{ 246, "Fong Shen Bang",           NULL, NULL, mapper246_m_w, NULL, NULL, NULL, NULL },
// 247, 248 Unused
	{ 249, "Waixing Security Board",   mapper249_l_w, NULL, NULL, mapper249_w, NULL, NULL, mapper4_irq },
	{ 250, "Time Diver Avenger",       NULL, NULL, NULL, mapper250_w, NULL, NULL, mapper4_irq },
// 251 Shen Hua Jian Yun III??
	{ 252, "Waixing San Guo Zhi",      NULL, NULL, NULL, mapper252_w, NULL, NULL, konami_irq },
// 253 Super 8-in-1 99 King Fighter??
// 254 Pikachu Y2K
	{ 255, "110-in-1",                 NULL, NULL, NULL, mapper255_w, NULL, NULL, NULL },
};


const mmc *nes_mapper_lookup( int mapper )
{
	int i;

	for (i = 0; i < ARRAY_LENGTH(mmc_list); i++)
	{
		if (mmc_list[i].iNesMapper == mapper)
			return &mmc_list[i];
	}

	return NULL;
}
