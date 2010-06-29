/*****************************************************************************************

    NES MMC Emulation

    Support for iNES Mappers

****************************************************************************************/

typedef struct __mmc
{
	int iNesMapper; /* iNES Mapper # */

	const char *desc;     /* Mapper description */
	write8_space_func mmc_write_low; /* $4100-$5fff write routine */
	read8_space_func mmc_read_low; /* $4100-$5fff read routine */
	write8_space_func mmc_write_mid; /* $6000-$7fff write routine */
	write8_space_func mmc_write; /* $8000-$ffff write routine */
	void (*ppu_latch)(running_device *device, offs_t offset);
	ppu2c0x_scanline_cb		mmc_scanline;
	ppu2c0x_hblank_cb		mmc_hblank;
} mmc;

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
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 prg_mode, prg_offset;

	prg_mode = state->MMC1_regs[0] & 0x0c;
	/* prg_mode&0x8 determines bank size: 32k (if 0) or 16k (if 1)? when in 16k mode,
       prg_mode&0x4 determines which half of the PRG space we can swap: if it is 4,
       MMC1_regs[3] sets banks at 0x8000; if it is 0, MMC1_regs[3] sets banks at 0xc000. */

	prg_offset = state->MMC1_regs[1] & 0x10;
	/* In principle, MMC1_regs[2]&0x10 might affect "extended" banks as well, when chr_mode=1.
       However, quoting Disch's docs: When in 4k CHR mode, 0x10 in both $A000 and $C000 *must* be
       set to the same value, or else pages will constantly be swapped as graphics render!
       Hence, we use only MMC1_regs[1]&0x10 for prg_offset */

	switch (prg_mode)
	{
	case 0x00:
	case 0x04:
		prg32(machine, (prg_offset + state->MMC1_regs[3]) >> 1);
		break;
	case 0x08:
		prg16_89ab(machine, prg_offset + 0);
		prg16_cdef(machine, prg_offset + state->MMC1_regs[3]);
		break;
	case 0x0c:
		prg16_89ab(machine, prg_offset + state->MMC1_regs[3]);
		prg16_cdef(machine, prg_offset + 0x0f);
		break;
	}
}

static void MMC1_set_chr( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_mode = state->MMC1_regs[0] & 0x10;

	if (chr_mode)
	{
		chr4_0(machine, state->MMC1_regs[1] & 0x1f, state->mmc_chr_source);
		chr4_4(machine, state->MMC1_regs[2] & 0x1f, state->mmc_chr_source);
	}
	else
		chr8(machine, (state->MMC1_regs[1] & 0x1f) >> 1, state->mmc_chr_source);
}

static WRITE8_HANDLER( mapper1_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	/* Note that there is only one latch and shift counter, shared amongst the 4 regs */
	/* Space Shuttle will not work if they have independent variables. */
	LOG_MMC(("mapper1_w, offset: %04x, data: %02x\n", offset, data));


	/* here we would need to add an if(cpu_cycles_passed>1) test, and
       if requirement is not met simply return without writing anything.
       AD&D Hillsfar and Bill & Ted rely on this behavior!! */
	if (data & 0x80)
	{
		state->mmc_count = 0;
		state->mmc_cmd1 = 0;

		/* Set reg at 0x8000 to size 16k and lower half swap - needed for Robocop 3, Dynowars */
		state->MMC1_regs[0] |= 0x0c;
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
			state->MMC1_regs[0] = state->mmc_cmd1;

			switch (state->MMC1_regs[0] & 0x03)
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
			state->MMC1_regs[1] = state->mmc_cmd1;
			MMC1_set_chr(space->machine);
			MMC1_set_prg(space->machine);
			break;
		case 0x4000:
			state->MMC1_regs[2] = state->mmc_cmd1;
			MMC1_set_chr(space->machine);
			break;
		case 0x6000:
			state->MMC1_regs[3] = state->mmc_cmd1;
			MMC1_set_prg(space->machine);
			break;
		}

		state->mmc_count = 0;
	}
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

static WRITE8_HANDLER( mapper4_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;

	LOG_MMC(("mapper4_w, offset: %04x, data: %02x\n", offset, data));

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
			if (data)
				memory_install_write_bank(space, 0x6000, 0x7fff, 0, 0, "bank5");
			else
				memory_unmap_write(space, 0x6000, 0x7fff, 0, 0);
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

    Mapper 21 & 25

    Known Boards: Konami VRC4A & VRC4C (21), VRC4B & VRC4D (25)
    Games: Ganbare Goemon Gaiden 2, Wai Wai World 2 (21), Bio
          Miracle Bokutte Upa, Ganbare Goemon Gaiden, TMNT 1 & 2 Jpn (25)

    In MESS: Supported

*************************************************************/

static void vrc4a_set_prg( running_machine *machine )
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
			vrc4a_set_prg(space->machine);
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
			vrc4a_set_prg(space->machine);
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
			vrc4a_set_prg(space->machine);
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
			vrc4a_set_prg(space->machine);
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
				LOG_MMC(("konami_vrc2b_w, offset: %04x value: %02x\n", offset, data));
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

    Mapper 45

    Known Boards: Unknown Multigame Bootleg Board
    Games: Street Fighter V, various multigame carts

    In MESS: Supported. It also uses mmc3_irq.

*************************************************************/

static WRITE8_HANDLER( mapper45_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper45_m_w, offset: %04x, data: %02x\n", offset, data));

	/* This bit is the "register lock". Once register are locked, writes go to WRAM
        and there is no way to unlock them (except by resetting the machine) */
	if (state->mapper45_reg[3] & 0x40)
		state->wram[offset] = data;
	else
	{
		state->mapper45_reg[state->mmc_count] = data;
		state->mmc_count = (state->mmc_count + 1) & 0x03;

		if (!state->mmc_count)
		{
			LOG_MMC(("mapper45_m_w, command completed %02x %02x %02x %02x\n", state->mapper45_reg[3],
				state->mapper45_reg[2], state->mapper45_reg[1], state->mapper45_reg[0]));

			state->mmc_prg_base = state->mapper45_reg[1];
			state->mmc_prg_mask = 0x3f ^ (state->mapper45_reg[3] & 0x3f);
			state->mmc_chr_base = ((state->mapper45_reg[2] & 0xf0) << 4) + state->mapper45_reg[0];
			state->mmc_chr_mask = 0xff >> (~state->mapper45_reg[2] & 0x0f);
			mapper4_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
		}
	}
}

/*************************************************************

    Mapper 53

    Known Boards: Unknown Multigame Bootleg Board
    Games: Supervision 16 in 1

    In MESS: Unsupported (SRAM banks can go in mid-regions).

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

    In MESS: Partially Supported. It also uses mmc3_irq.

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
								device->machine->primary_screen->vpos(), device->machine->primary_screen->hpos()));
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
							device->machine->primary_screen->vpos(), device->machine->primary_screen->hpos()));
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
	LOG_MMC(("mapper64_w, offset: %04x, data: %02x\n", offset, data));

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
	LOG_MMC(("mapper71_w, offset: %04x, data: %02x\n", offset, data));

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

    Mapper 84

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

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

    Mapper 93

    Known Boards: Sunsoft 2A
    Games: Shanghai, Fantasy Zone

    Very similar to mapper 89, but no NT mirroring for data&8

    In MESS: Supported.

*************************************************************/

static WRITE8_HANDLER( mapper93_w )
{
	LOG_MMC(("mapper93_w, offset %04x, data: %02x\n", offset, data));

	prg16_89ab(space->machine, data >> 4);
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

    Known Boards: Unused (Urusei Yatsura had been assigned to 
    this mapper, but it's Mapper 87)

    Games: ----------
 
    In MESS: ----------
 
*************************************************************/

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

    Mapper 105

    Known Boards: Custom Board
    Games: Nintendo World Championships 1990

    In MESS: Unsupported.

*************************************************************/

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

    Mapper 116

    Known Boards: Unknown Bootleg Board (Someri?!?)
    Games: AV Mei Shao Nv Zhan Shi, Chuugoku Taitei

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 120

    Known Boards: Unknown Bootleg Board
    Games: Tobidase Daisakusen (FDS Conversion)

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

    Mapper 135

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 137

    Known Boards: Bootleg Board by Sachen (8259D)
    Games: The Great Wall

    In MESS: Supported.

*************************************************************/

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
			state->sachen_reg[state->mmc_cmd1] = data;

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
					chr1_0(space->machine, (state->sachen_reg[0] & 0x07), CHRROM);
					chr1_1(space->machine, (state->sachen_reg[1] & 0x07) | (state->sachen_reg[4] << 4 & 0x10), CHRROM);
					chr1_2(space->machine, (state->sachen_reg[2] & 0x07) | (state->sachen_reg[4] << 3 & 0x10), CHRROM);
					chr1_3(space->machine, (state->sachen_reg[3] & 0x07) | (state->sachen_reg[4] << 2 & 0x10) | (state->sachen_reg[6] << 3 & 0x08), CHRROM);
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
			state->sachen_reg[state->mmc_cmd1] = data;

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
					bank_helper1 = state->sachen_reg[7] & 0x01;
					bank_helper2 = (state->sachen_reg[4] & 0x07) << 3;
					chr2_0(space->machine, ((state->sachen_reg[bank_helper1 ? 0 : 0] & 0x07) | bank_helper2), CHRROM);
					chr2_2(space->machine, ((state->sachen_reg[bank_helper1 ? 0 : 1] & 0x07) | bank_helper2), CHRROM);
					chr2_4(space->machine, ((state->sachen_reg[bank_helper1 ? 0 : 2] & 0x07) | bank_helper2), CHRROM);
					chr2_6(space->machine, ((state->sachen_reg[bank_helper1 ? 0 : 3] & 0x07) | bank_helper2), CHRROM);
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
			state->sachen_reg[state->mmc_cmd1] = data;

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
					bank_helper1 = state->sachen_reg[7] & 0x01;
					bank_helper2 = (state->sachen_reg[4] & 0x07) << 3;
					chr2_0(space->machine, ((state->sachen_reg[bank_helper1 ? 0 : 0] & 0x07) | bank_helper2) << 2, CHRROM);
					chr2_2(space->machine, ((state->sachen_reg[bank_helper1 ? 0 : 1] & 0x07) | bank_helper2) << 2 | 0x01, CHRROM);
					chr2_4(space->machine, ((state->sachen_reg[bank_helper1 ? 0 : 2] & 0x07) | bank_helper2) << 2 | 0x02, CHRROM)	;
					chr2_6(space->machine, ((state->sachen_reg[bank_helper1 ? 0 : 3] & 0x07) | bank_helper2) << 2 | 0x03, CHRROM);
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
			state->sachen_reg[state->mmc_cmd1] = data;

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
					bank_helper1 = state->sachen_reg[7] & 0x01;
					bank_helper2 = (state->sachen_reg[4] & 0x07) << 3;
					chr2_0(space->machine, ((state->sachen_reg[bank_helper1 ? 0 : 0] & 0x07) | bank_helper2) << 1, state->mmc_chr_source);
					chr2_2(space->machine, ((state->sachen_reg[bank_helper1 ? 0 : 1] & 0x07) | bank_helper2) << 1 | 0x01, state->mmc_chr_source);
					chr2_4(space->machine, ((state->sachen_reg[bank_helper1 ? 0 : 2] & 0x07) | bank_helper2) << 1, state->mmc_chr_source);
					chr2_6(space->machine, ((state->sachen_reg[bank_helper1 ? 0 : 3] & 0x07) | bank_helper2) << 1 | 0x01, state->mmc_chr_source);
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
    Games: 

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
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 mmc_helper = 0;
	int i;

	for (i = 0; i < 8; i++)
		mmc_helper |= ((state->map153_reg[i] & 0x01) << 4);

	state->map153_bank_latch = mmc_helper | (state->map153_bank_latch & 0x0f);

	prg16_89ab(machine, state->map153_bank_latch);
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
			state->map153_reg[offset & 0x07] = data;
			mapper153_set_prg(space->machine);
			break;
		case 8:
			state->map153_bank_latch = (data & 0x0f) | (state->map153_bank_latch & 0x10);
			prg16_89ab(space->machine, state->map153_bank_latch);
			break;
		default:
			lz93d50_m_w(space, offset & 0x0f, data);
			break;
		}
	}
	else
		lz93d50_m_w(space, offset, data);
}

static WRITE8_HANDLER( mapper153_w )
{
	LOG_MMC(("mapper153_w, offset: %04x, data: %02x\n", offset, data));

	mapper153_m_w(space, offset, data);
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
	LOG_MMC(("mapper158_w, offset: %04x, data: %02x\n", offset, data));

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

    Mapper 165

    Known Boards: Bootleg Board by Waixing
    Games: Fire Emblem (C), Fire Emblem Gaiden (C)

    MMC3 clone

    In MESS: Unsupported.

*************************************************************/

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

    Mapper 181

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

*************************************************************/

/*************************************************************

    Mapper 183

    Known Boards: Unknown Bootleg Board
    Games: Shui Guan Pipe

    In MESS: Unsupported.

*************************************************************/

/*************************************************************

    Mapper 186

    Known Boards: Bootleg Board by Fukutake
    Games: Study Box

    In MESS: Unsupported. Needs reads from 0x6000-0x7fff.

*************************************************************/

/*************************************************************

    Mapper 190

    Known Boards: Undocumented / Unused ?!?
    Games: ----------

    In MESS: ----------

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
	LOG_MMC(("mapper206_w, offset: %04x, data: %02x\n", offset, data));

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

    Mapper 251

    Known Boards: Undocumented
    Games: Super 8-in-1 99 King Fighter

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

    mmc_list

    Supported mappers and corresponding handlers

*************************************************************/

static const mmc mmc_list[] =
{
/*  INES   DESC                          LOW_W, LOW_R, MED_W, HIGH_W, PPU_latch, scanline CB, hblank CB */
	{  0, "No Mapper",                 NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{  1, "MMC1",                      NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL },
	{  2, "Ux-ROM",                    NULL, NULL, NULL, uxrom_w, NULL, NULL, NULL },
	{  3, "Cx-ROM",                    NULL, NULL, NULL, cnrom_w, NULL, NULL, NULL },
	{  4, "MMC3",                      NULL, NULL, NULL, mapper4_w, NULL, NULL, mmc3_irq },
	{  5, "MMC5",                      exrom_l_w, exrom_l_r, NULL, NULL, NULL, NULL, mmc5_irq },
	{  6, "FFE F4xxxx",                mapper6_l_w, NULL, NULL, mapper6_w, NULL, NULL, ffe_irq },
	{  7, "Ax-ROM",                    NULL, NULL, NULL, axrom_w, NULL, NULL, NULL },
	{  8, "FFE F3xxxx",                NULL, NULL, NULL, mapper8_w, NULL, NULL, NULL },
	{  9, "MMC2",                      NULL, NULL, NULL, pxrom_w, mmc2_latch, NULL, NULL},
	{ 10, "MMC4",                      NULL, NULL, NULL, fxrom_w, mmc2_latch, NULL, NULL },
	{ 11, "Color Dreams",              NULL, NULL, NULL, dis_74x377_w, NULL, NULL, NULL },
	{ 12, "Rex Soft DBZ5",             rex_dbz_l_w, rex_dbz_l_r, NULL, rex_dbz_w, NULL, NULL, mmc3_irq },
	{ 13, "CP-ROM",                    NULL, NULL, NULL, cprom_w, NULL, NULL, NULL },
	{ 14, "Rex Soft SL1632",           NULL, NULL, NULL, rex_sl1632_w, NULL, NULL, mmc3_irq },
	{ 15, "100-in-1",                  NULL, NULL, NULL, waixing_ps2_w, NULL, NULL, NULL },
	{ 16, "Bandai LZ93D50 - 24C02",    NULL, NULL, lz93d50_m_w, lz93d50_w, NULL,NULL,  bandai_lz_irq },
	{ 17, "FFE F8xxx",                 mapper17_l_w, NULL, NULL, NULL, NULL, NULL, ffe_irq },
	{ 18, "Jaleco SS88006",            NULL, NULL, NULL, ss88006_w, NULL, NULL, ss88006_irq },
	{ 19, "Namcot 106 + N106",         namcot163_l_w, namcot163_l_r, NULL, namcot163_w, NULL, NULL, namcot_irq },
	{ 21, "Konami VRC 4a",             NULL, NULL, NULL, konami_vrc4a_w, NULL, NULL, konami_irq },
	{ 22, "Konami VRC 2a",             NULL, NULL, NULL, konami_vrc2a_w, NULL, NULL, NULL },
	{ 23, "Konami VRC 2b",             NULL, NULL, NULL, konami_vrc2b_w, NULL, NULL, konami_irq },
	{ 24, "Konami VRC 6a",             NULL, NULL, NULL, konami_vrc6a_w, NULL, NULL, konami_irq },
	{ 25, "Konami VRC 4b",             NULL, NULL, NULL, konami_vrc4b_w, NULL, NULL, konami_irq },
	{ 26, "Konami VRC 6b",             NULL, NULL, NULL, konami_vrc6b_w, NULL, NULL, konami_irq },
// 27 World Hero
// 28, 29, 30, 31 Unused
	{ 32, "Irem G-101",                NULL, NULL, NULL, mapper32_w, NULL, NULL, NULL },
	{ 33, "Taito TC0190FMC",           NULL, NULL, NULL, tc0190fmc_w, NULL, NULL, NULL },
	{ 34, "Nina-001",                  NULL, NULL, mapper34_m_w, mapper34_w, NULL, NULL, NULL },
	{ 35, "SC-127",                    NULL, NULL, NULL, sc127_w, NULL, NULL, sc127_irq },
// 35 Unused
	{ 36, "TXC Policeman",             NULL, NULL, NULL, txc_strikewolf_w, NULL, NULL, NULL },
	{ 37, "ZZ Board",                  NULL, NULL, zz_m_w, txrom_w, NULL, NULL, mmc3_irq },
	{ 38, "Crime Buster",              NULL, NULL, dis_74x161x138_m_w, NULL, NULL, NULL, NULL },
	{ 39, "Subor Study n Game",        NULL, NULL, NULL, mapper39_w, NULL, NULL, NULL },
	{ 40, "SMB2 JPN (bootleg)",        NULL, NULL, NULL, btl_smb2a_w, NULL, NULL, btl_smb2a_irq },
	{ 41, "Caltron 6-in-1",            NULL, NULL, caltron6in1_m_w, caltron6in1_w, NULL, NULL, NULL },
	{ 42, "Mario Baby",                NULL, NULL, NULL, btl_mariobaby_w, NULL, NULL, NULL },
	{ 43, "150-in-1",                  NULL, NULL, NULL, smb2j_w, NULL, NULL, NULL },
	{ 44, "SuperBig 7-in-1",           NULL, NULL, NULL, bmc_sbig7_w, NULL, NULL, mmc3_irq },
	{ 45, "X-in-1 MMC3",               NULL, NULL, mapper45_m_w, mapper4_w, NULL, NULL, mmc3_irq },
	{ 46, "Rumblestation",             NULL, NULL, rumblestation_m_w, rumblestation_w, NULL, NULL, NULL },
	{ 47, "QJ Board",                  NULL, NULL, qj_m_w, txrom_w, NULL, NULL, mmc3_irq },
	{ 48, "Taito TC0190FMC PAL16R4",   NULL, NULL, NULL, tc0190fmc_p16_w, NULL, NULL, mmc3_irq },
	{ 49, "Super HIK 4-in-1",          NULL, NULL, bmc_hik4in1_m_w, txrom_w, NULL, NULL, mmc3_irq },
	{ 50, "SMB2 JPN (bootleg 2)",      smb2jb_l_w, NULL, NULL, NULL, NULL, NULL, smb2jb_irq },
	{ 51, "Ballgames 11-in-1",         NULL, NULL, bmc_ball11_m_w, bmc_ball11_w, NULL, NULL, NULL },
	{ 52, "Mario 7-in-1",              NULL, NULL, bmc_mario7in1_m_w, txrom_w, NULL, NULL, mmc3_irq },
// 53 Supervision 16-in-1
	{ 54, "Novel Diamond X-in-1",      NULL, NULL, NULL, novel1_w, NULL, NULL, NULL },
// 55 Genius SMB
// 56 Kaiser KS202
	{ 57, "GKA 6-in-1",                NULL, NULL, NULL, bmc_gka_w, NULL, NULL, NULL },
	{ 58, "GKB X-in-1",                NULL, NULL, NULL, bmc_gkb_w, NULL, NULL, NULL },
// 59 Unused
// 60 4-in-1 Reset based
	{ 61, "Tetris Family 20-in-1",     NULL, NULL, NULL, rcm_tf_w, NULL, NULL, NULL },
	{ 62, "Super 700-in-1",            NULL, NULL, NULL, bmc_super700in1_w, NULL, NULL, NULL },
// 63 CH001 X-in-1
	{ 64, "Tengen 800032",             NULL, NULL, NULL, mapper64_w, NULL, NULL, mapper64_irq },
	{ 65, "Irem H3001",                NULL, NULL, NULL, h3001_w, NULL, NULL, h3001_irq },
	{ 66, "Gx-ROM",                    NULL, NULL, NULL, gxrom_w, NULL, NULL, NULL },
	{ 67, "SunSoft 3",                 NULL, NULL, NULL, sunsoft3_w, NULL, NULL, sunsoft3_irq },
	{ 68, "SunSoft 4",                 NULL, NULL, NULL, ntbrom_w, NULL, NULL, NULL },
	{ 69, "SunSoft FME",               NULL, NULL, NULL, jxrom_w, NULL, NULL, jxrom_irq },
	{ 70, "74161/32 Bandai",           NULL, NULL, NULL, mapper70_w, NULL, NULL, NULL },
	{ 71, "Camerica",                  NULL, NULL, NULL, mapper71_w, NULL, NULL, NULL },
	{ 72, "74161/32 Jaleco",           NULL, NULL, NULL, jf17_w, NULL, NULL, NULL },
	{ 73, "Konami VRC 3",              NULL, NULL, NULL, konami_vrc3_w, NULL, NULL, konami_irq },
	{ 74, "Waixing Type A",            NULL, NULL, NULL, waixing_a_w, NULL, NULL, mmc3_irq },
	{ 75, "Konami VRC 1",              NULL, NULL, NULL, konami_vrc1_w, NULL, NULL, NULL },
	{ 76, "Namco 3446",                NULL, NULL, NULL, namcot3446_w, NULL, NULL, NULL },
	{ 77, "Irem LROG017",              NULL, NULL, NULL, lrog017_w, NULL, NULL, NULL },
	{ 78, "Irem Holy Diver",           NULL, NULL, NULL, mapper78_w, NULL, NULL, NULL },
	{ 79, "Nina-03",                   nina06_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 80, "Taito X1-005 Ver. A",       NULL, NULL, mapper80_m_w, NULL, NULL, NULL, NULL },
// 81 Unused
	{ 82, "Taito X1-017",              NULL, NULL, x1017_m_w, NULL, NULL, NULL, NULL },
	{ 83, "Cony",                      cony_l_w, cony_l_r, NULL, cony_w, NULL, NULL, NULL },
	{ 84, "Pasofami hacked images?",   NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 85, "Konami VRC 7",              NULL, NULL, NULL, konami_vrc7_w, NULL, NULL, konami_irq },
	{ 86, "Jaleco JF13",               NULL, NULL, jf13_m_w, NULL, NULL, NULL, NULL },
	{ 87, "74139/74",                  NULL, NULL, dis_74x139x74_m_w, NULL, NULL, NULL, NULL },
	{ 88, "Namcot 34x3",               NULL, NULL, NULL, dxrom_w, NULL, NULL, NULL },
	{ 89, "Sunsoft 2b",                NULL, NULL, NULL, mapper89_w, NULL, NULL, NULL },
// 90 JY Company Type A
	{ 91, "HK-SF3 (bootleg)",          NULL, NULL, mk2_m_w, NULL, NULL, NULL, mmc3_irq },
	{ 92, "Jaleco JF19 / JF21",        NULL, NULL, NULL, jf19_w, NULL, NULL, NULL },
	{ 93, "Sunsoft 2A",                NULL, NULL, NULL, mapper93_w, NULL, NULL, NULL },
	{ 94, "Capcom LS161",              NULL, NULL, NULL, un1rom_w, NULL, NULL, NULL },
	{ 95, "Namcot 3425",               NULL, NULL, NULL, namcot3425_w, NULL, NULL, NULL },
	{ 96, "Bandai OekaKids",           NULL, NULL, NULL, bandai_ok_w, NULL, NULL, NULL },
	{ 97, "Irem Kaiketsu",             NULL, NULL, NULL, tam_s1_w, NULL, NULL, NULL },
// 98 Unused
// 99 VS. system
// 100 images hacked to work with nesticle?
// 101 Unused
// 102 Unused
// 103 Bootleg cart 2708
	{ 104, "Golden Five",              NULL, NULL, NULL, golden5_w, NULL, NULL, NULL },
// 105 Nintendo World Championship
	{ 106, "SMB3 (bootleg)",           NULL, NULL, NULL, btl_smb3_w, NULL, NULL, btl_smb3_irq },
	{ 107, "Magic Dragon",             NULL, NULL, NULL, magics_md_w, NULL, NULL, NULL },
// 108 Whirlwind
// 109, 110 Unused
// 111 Ninja Ryuukenden Chinese?
	{ 112, "Asder",                    NULL, NULL, NULL, ntdec_asder_w, NULL, NULL, NULL },
	{ 113, "Sachen/Hacker/Nina",       mapper113_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 114, "The Lion King",            NULL, NULL, sgame_lion_m_w, sgame_lion_w, NULL, NULL, mmc3_irq },
	{ 115, "Kasing",                   NULL, NULL, kasing_m_w, kasing_w, NULL, NULL, mmc3_irq },
// 116 Someri Team
	{ 117, "Future Media",             NULL, NULL, NULL, futuremedia_w, NULL, NULL, futuremedia_irq },
	{ 118, "TKS-ROM / TLS-ROM",        NULL, NULL, NULL, txsrom_w, NULL, NULL, mmc3_irq },
	{ 119, "TQ-ROM",                   NULL, NULL, NULL, tqrom_w, NULL, NULL, mmc3_irq },
// 120 FDS bootleg
	{ 121, "K - Panda Prince",         kay_pp_l_w, kay_pp_l_r, NULL, kay_pp_w, NULL, NULL, mmc3_irq },
// 122 Unused
// 123 K H2288
// 124, 125 Unused
// 126 Powerjoy 84-in-1
// 127, 128, 129, 130, 131 Unused
	{ 132, "TXC T22211A",              txc_22211_l_w, txc_22211_l_r, NULL, txc_22211_w, NULL, NULL, NULL },
	{ 133, "Sachen SA72008",           sa72008_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 134, "Family 4646B",             NULL, NULL, bmc_family4646_m_w, txrom_w, NULL, NULL, mmc3_irq },
// 135 Unused
	{ 136, "Sachen TCU02",             tcu02_l_w, tcu02_l_r, NULL, NULL, NULL, NULL, NULL },
	{ 137, "Sachen 8259D",             mapper137_l_w, NULL, mapper137_m_w, NULL, NULL, NULL, NULL },
	{ 138, "Sachen 8259B",             mapper138_l_w, NULL, mapper138_m_w, NULL, NULL, NULL, NULL },
	{ 139, "Sachen 8259C",             mapper139_l_w, NULL, mapper139_m_w, NULL, NULL, NULL, NULL },
	{ 140, "Jaleco JF11",              NULL, NULL, jf11_m_w, NULL, NULL, NULL, NULL },
	{ 141, "Sachen 8259A",             mapper141_l_w, NULL, mapper141_m_w, NULL, NULL, NULL, NULL },
// 142 Kaiser KS7032
	{ 143, "Sachen TCA01",             NULL, tca01_l_r, NULL, NULL, NULL, NULL, NULL },
	{ 144, "AGCI 50282",               NULL, NULL, NULL, agci_50282_w, NULL, NULL, NULL }, //Death Race only
	{ 145, "Sachen SA72007",           sa72007_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 146, "Sachen SA-016-1M",         nina06_l_w, NULL, NULL, NULL, NULL, NULL, NULL }, // basically same as Mapper 79 (Nina006)
	{ 147, "Sachen TCU01",             tcu01_l_w, NULL, tcu01_m_w, tcu01_w, NULL, NULL, NULL },
	{ 148, "Sachen SA0037",            NULL, NULL, NULL, sa0036_w, NULL, NULL, NULL },
	{ 149, "Sachen SA0036",            NULL, NULL, NULL, sa0037_w, NULL, NULL, NULL },
	{ 150, "Sachen 74LS374B",          sachen_74x374_l_w, sachen_74x374_l_r, NULL, NULL, NULL, NULL, NULL },
// 151 Konami VS. system
	{ 152, "Taito 74161/161",          NULL, NULL, NULL, mapper152_w, NULL, NULL, NULL },
	{ 153, "Bandai LZ93D50",           NULL, NULL, mapper153_m_w, mapper153_w, NULL,NULL,  bandai_lz_irq },
	{ 154, "Namcot 34xx",              NULL, NULL, NULL, namcot3453_w, NULL, NULL, NULL },
	{ 155, "SK-ROM",                   NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL }, // diff compared to MMC1 concern WRAM (unsupported for MMC1 as well, atm)
	{ 156, "Open Corp. DAOU36",        NULL, NULL, NULL, daou306_w, NULL, NULL, NULL },
	{ 157, "Bandai Datach Board",      NULL, NULL, lz93d50_m_w, lz93d50_w, NULL,NULL,  bandai_lz_irq },	// no Datach Reader -> we fall back to mapper 16
	{ 158, "Tengen T800037",           NULL, NULL, NULL, mapper158_w, NULL, NULL, mapper64_irq },
	{ 159, "Bandai LZ93D50 - 24C01",   NULL, NULL, lz93d50_m_w, lz93d50_w, NULL,NULL,  bandai_lz_irq },
// 160, 161, 162 Unused
// 163 Nanjing
	{ 164, "Final Fantasy V",          waixing_ffv_l_w, NULL, NULL, waixing_ffv_w, NULL, NULL, NULL },
// 165 Waixing SH2
	{ 166, "Subor Board Type 1",       NULL, NULL, NULL, subor1_w, NULL, NULL, NULL },
	{ 167, "Subor Board Type 2",       NULL, NULL, NULL, subor0_w, NULL, NULL, NULL },
// 168 Racermate Challenger II
// 169 Unused
// 170 Fujiya
	{ 171, "Kaiser KS7058",            NULL, NULL, NULL, ks7058_w, NULL, NULL, NULL },
	{ 172, "TXC T22211B",              txc_22211_l_w, txc_22211_l_r, NULL, txc_22211b_w, NULL, NULL, NULL },
	{ 173, "TXC T22211C",              txc_22211_l_w, txc_22211c_l_r, NULL, txc_22211_w, NULL, NULL, NULL },
// 174 Unused
// 175 Kaiser KS7022
	{ 176, "Zhi Li Xiao Zhuan Yuan",   unl_xzy_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 177, "Henggedianzi Board",       NULL, NULL, NULL, henggedianzi_w, NULL, NULL, NULL },
	{ 178, "San Guo Zhong Lie Zhuan",  waixing_sgzlz_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 179, "Xing He Zhan Shi",         heng_xjzb_l_w, NULL, NULL, heng_xjzb_w, NULL, NULL, NULL },
	{ 180, "Nihon Bussan UN-ROM",      NULL, NULL, NULL, uxrom_cc_w, NULL, NULL, NULL },
// 181 Unused
	{ 182, "Hosenkan",                 NULL, NULL, NULL, hosenkan_w, NULL, NULL, mmc3_irq },
// 183 FDS bootleg
	{ 184, "Sunsoft 1",                NULL, NULL, sunsoft1_m_w, NULL, NULL, NULL, NULL },
	{ 185, "CN-ROM",                   NULL, NULL, NULL, cnrom_w, NULL, NULL, NULL },
// 186 Fukutake
	{ 187, "King of Fighters 96",      kof96_l_w, kof96_l_r, NULL, kof96_w, NULL, NULL, mmc3_irq },
	{ 188, "Bandai Karaoke",           NULL, NULL, NULL, bandai_ks_w, NULL, NULL, NULL },
	{ 189, "TXC TW Board",             txc_tw_l_w, NULL, txc_tw_m_w, txc_tw_w, NULL, NULL, mmc3_irq },
// 190 Unused
	{ 191, "Waixing Type B",           NULL, NULL, NULL, waixing_b_w, NULL, NULL, mmc3_irq },
	{ 192, "Waixing Type C",           NULL, NULL, NULL, waixing_c_w, NULL, NULL, mmc3_irq },
	{ 193, "Fighting Hero",            NULL, NULL, ntdec_fh_m_w, NULL, NULL, NULL, NULL },
	{ 194, "Waixing Type D",           NULL, NULL, NULL, waixing_d_w, NULL, NULL, mmc3_irq },
	{ 195, "Waixing Type E",           NULL, NULL, NULL, waixing_e_w, NULL, NULL, mmc3_irq },
	{ 196, "Super Mario Bros. 11",     NULL, NULL, NULL, btl_smb11_w, NULL, NULL, mmc3_irq },
	{ 197, "Super Fighter 3",          NULL, NULL, NULL, unl_sf3_w, NULL, NULL, mmc3_irq },
	{ 198, "Waixing Type F",           NULL, NULL, NULL, waixing_f_w, NULL, NULL, mmc3_irq },
	{ 199, "Waixing Type G",           NULL, NULL, NULL, waixing_g_w, NULL, NULL, mmc3_irq },
	{ 200, "36-in-1",                  NULL, NULL, NULL, bmc_36in1_w, NULL, NULL, NULL },
	{ 201, "21-in-1",                  NULL, NULL, NULL, bmc_21in1_w, NULL, NULL, NULL },
	{ 202, "150-in-1",                 NULL, NULL, NULL, bmc_150in1_w, NULL, NULL, NULL },
	{ 203, "35-in-1",                  NULL, NULL, NULL, bmc_35in1_w, NULL, NULL, NULL },
	{ 204, "64-in-1",                  NULL, NULL, NULL, bmc_64in1_w, NULL, NULL, NULL },
	{ 205, "15-in-1",                  NULL, NULL, bmc_15in1_m_w, txrom_w, NULL, NULL, mmc3_irq },
	{ 206, "MIMIC-1",                  NULL, NULL, NULL, mapper206_w, NULL, NULL, mmc3_irq },
	{ 207, "Taito X1-005 Ver. B",      NULL, NULL, mapper207_m_w, NULL, NULL, NULL, NULL },
	{ 208, "Gouder G3717",             gouder_sf4_l_w, gouder_sf4_l_r, NULL, gouder_sf4_w, NULL, NULL, mmc3_irq },
// 209 JY Company Type B
// 210 Some emu uses this as Mapper 19 without some features
// 211 JY Company Type C
	{ 212, "Super HIK 300-in-1",       NULL, NULL, NULL, bmc_hik300_w, NULL, NULL, NULL },
	{ 213, "9999999-in-1",             NULL, NULL, NULL, novel2_w, NULL, NULL, NULL },
	{ 214, "Super Gun 20-in-1",        NULL, NULL, NULL, supergun20in1_w, NULL, NULL, NULL },
	{ 215, "Super Game Boogerman",     sgame_boog_l_w, NULL, NULL, sgame_boog_w, NULL, NULL, mmc3_irq },
	{ 216, "RCM GS2015",               NULL, NULL, NULL, gs2015_w, NULL, NULL, NULL },
	{ 217, "Golden Card 6-in-1",       bmc_gc6in1_l_w, NULL, NULL, bmc_gc6in1_w, NULL, NULL, mmc3_irq },
// 218 Unused
// 219 Bootleg a9746
// 220 Summer Carnival '92??
	{ 221, "X-in-1 (N625092)",         NULL, NULL, NULL, n625092_w, NULL, NULL, NULL },
	{ 222, "Dragonninja Bootleg",      NULL, NULL, NULL, btl_dn_w, NULL, NULL, btl_dn_irq },
// 223 Waixing Type I
// 224 Waixing Type J
	{ 225, "72-in-1 bootleg",          NULL, NULL, NULL, bmc_72in1_w, NULL, NULL, NULL },
	{ 226, "76-in-1 bootleg",          NULL, NULL, NULL, bmc_76in1_w, NULL, NULL, NULL },
	{ 227, "1200-in-1 bootleg",        NULL, NULL, NULL, bmc_1200in1_w, NULL, NULL, NULL },
	{ 228, "Action 52",                NULL, NULL, NULL, mapper228_w, NULL, NULL, NULL },
	{ 229, "31-in-1",                  NULL, NULL, NULL, bmc_31in1_w, NULL, NULL, NULL },
	{ 230, "22-in-1",                  NULL, NULL, NULL, bmc_22g_w, NULL, NULL, NULL },
	{ 231, "20-in-1",                  NULL, NULL, NULL, bmc_20in1_w, NULL, NULL, NULL },
	{ 232, "Quattro",                  NULL, NULL, bf9096_w, bf9096_w, NULL, NULL, NULL },
// 233 Super 22 Games
// 234 AVE Maxi 15
// 235 Golden Game x-in-1
// 236 Game 800-in-1
// 237 Unused
// 238 Bootleg 6035052
// 239 Unused
	{ 240, "Jing Ke Xin Zhuan",        cne_shlz_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 241, "Education 18-in-1",        NULL, txc_mxmdhtwo_l_r, NULL, txc_mxmdhtwo_w, NULL, NULL, NULL },
	{ 242, "Wai Xing Zhan Shi",        NULL, NULL, NULL, mapper242_w, NULL, NULL, NULL },
	{ 243, "Sachen 74LS374A",          sachen_74x374a_l_w, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 244, "Decathlon",                NULL, NULL, NULL, cne_decathl_w, NULL, NULL, NULL },
	{ 245, "Waixing Type H",           NULL, NULL, NULL, waixing_h_w, NULL, NULL, mmc3_irq },
	{ 246, "Fong Shen Bang",           NULL, NULL, cne_fsb_m_w, NULL, NULL, NULL, NULL },
// 247, 248 Unused
	{ 249, "Waixing Security Board",   waixing_sec_l_w, NULL, NULL, waixing_sec_w, NULL, NULL, mmc3_irq },
	{ 250, "Time Diver Avenger",       NULL, NULL, NULL, nitra_w, NULL, NULL, mmc3_irq },
// 251 Shen Hua Jian Yun III??
	{ 252, "Waixing San Guo Zhi",      NULL, NULL, NULL, waixing_sgz_w, NULL, NULL, konami_irq },
// 253 Super 8-in-1 99 King Fighter??
// 254 Pikachu Y2K
	{ 255, "110-in-1",                 NULL, NULL, NULL, bmc_110in1_w, NULL, NULL, NULL },
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

/*************************************************************

 mapper_initialize

 Initialize various constants needed by mappers, depending
 on the mapper number. As long as the code is mainly iNES
 centric, also UNIF images will call this function to
 initialize their bankswitch and registers.

 *************************************************************/

static int mapper_initialize( running_machine *machine, int mmc_num )
{
	nes_state *state = (nes_state *)machine->driver_data;
	int err = 0, i;

	switch (mmc_num)
	{
		case 0:
			err = 1; /* No mapper found */
			prg32(machine, 0);
			break;
		case 1:
		case 155:
			state->mmc_cmd1 = 0;
			state->mmc_count = 0;
			state->MMC1_regs[0] = 0x0f;
			state->MMC1_regs[1] = state->MMC1_regs[2] = state->MMC1_regs[3] = 0;
			set_nt_mirroring(machine, PPU_MIRROR_HORZ);
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			MMC1_set_chr(machine);
			MMC1_set_prg(machine);
			break;
		case 2:
			/* These games don't switch VROM, but some ROMs incorrectly have CHR chunks */
			state->chr_chunks = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 13:
			chr4_0(machine, 0, CHRRAM);
			prg32(machine, 0);
			break;
		case 3:
		case 185:
			prg32(machine, 0);
			break;
		case 4:
		case 206:
			/* Only MMC3 and clones seem to use 4-screen! */
			/* hardware uses an 8K RAM chip, but can't address the high 4K! */
			/* that much is documented for Nintendo Gauntlet boards */
			if (state->four_screen_vram)
			{
				set_nt_page(machine, 0, CART_NTRAM, 0, 1);
				set_nt_page(machine, 1, CART_NTRAM, 1, 1);
				set_nt_page(machine, 2, CART_NTRAM, 2, 1);
				set_nt_page(machine, 3, CART_NTRAM, 3, 1);
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
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;	// prg_bank[2] & prg_bank[3] remain always the same in most MMC3 variants
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;	// but some pirate clone mappers change them after writing certain registers
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;	// these could be init'ed as xxx_chunks-1 and they would work the same
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 5:
			/* Can switch 8k prg banks, but they are saved as 16k in size */
			state->MMC5_rom_bank_mode = 3;
			state->MMC5_vrom_bank_mode = 0;
			state->MMC5_vram_protect = 0;
			state->mmc5_high_chr = 0;
			state->mmc5_vram_control = 0;
			state->mmc5_split_scr = 0;
			memset(state->MMC5_vrom_bank, 0, ARRAY_LENGTH(state->MMC5_vrom_bank));
			state->mid_ram_enable = 0;
			prg16_89ab(machine, state->prg_chunks - 2);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 6:
			state->mmc_cmd1 = 0;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 7);
			break;
		case 7:
			/* Bankswitches 32k at a time */
			set_nt_mirroring(machine, PPU_MIRROR_LOW);
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 8:
		case 96:
			prg32(machine, 0);
			break;
		case 9:
			state->MMC2_regs[0] = state->MMC2_regs[2] = 0;
			state->MMC2_regs[1] = state->MMC2_regs[3] = 0;
			state->mmc_cmd1 = state->mmc_cmd2 = 0xfe;
			prg8_89(machine, 0);
			//ugly hack to deal with iNES header usage of chunk count.
			prg8_ab(machine, (state->prg_chunks << 1) - 3);
			prg8_cd(machine, (state->prg_chunks << 1) - 2);
			prg8_ef(machine, (state->prg_chunks << 1) - 1);
			break;
		case 10:
			/* Reset VROM latches */
			state->MMC2_regs[0] = state->MMC2_regs[2] = 0;
			state->MMC2_regs[1] = state->MMC2_regs[3] = 0;
			state->mmc_cmd1 = state->mmc_cmd2 = 0xfe;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 11:
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;	// NINA-07 has no CHRROM
			prg32(machine, 0);
			break;
		case 14:
			state->mmc_extra_bank[2] = 0xfe;
			state->mmc_extra_bank[3] = 0xff;
			state->mmc_extra_bank[0] = state->mmc_extra_bank[1] = state->mmc_extra_bank[4] = state->mmc_extra_bank[5] = state->mmc_extra_bank[6] = 0;
			state->mmc_extra_bank[7] = state->mmc_extra_bank[8] = state->mmc_extra_bank[9] = state->mmc_extra_bank[0xa] = state->mmc_extra_bank[0xb] = 0;
			state->map14_reg[0] = state->map14_reg[1] = 0;
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 15:
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			prg32(machine, 0);
			break;
		case 16:
		case 17:
		case 18:
		case 157:
		case 159:
			state->mmc_cmd1 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			break;
		case 19:
			state->mmc_cmd1 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
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
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 32:
			state->mmc_cmd1 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			if (state->crc_hack)
				set_nt_mirroring(machine, PPU_MIRROR_HIGH);  // needed by Major League
			break;
		case 34:
			prg32(machine, 0);
			break;
		case 35:
			prg32(machine, 0xff);
			break;
		case 37:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = 0x07;
			state->mmc_chr_mask = 0x7f;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 40:
			prg8_67(machine, 0xfe);
			prg8_89(machine, 0xfc);
			prg8_ab(machine, 0xfd);
			prg8_cd(machine, 0xfe);
			prg8_ef(machine, 0xff);
			break;
		case 41:
			state->mmc_cmd1 = 0;
			prg32(machine, 0);
			break;
		case 42:
			prg32(machine, state->prg_chunks - 1);
			break;
		case 43:
			prg32(machine, 0);
			if (state->battery)
				memset(state->battery_ram, 0x2000, 0xff);
			else if (state->prg_ram)
				memset(state->wram, 0x2000, 0xff);
			break;
		case 44:
		case 47:
		case 49:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = 0x0f;
			state->mmc_chr_mask = 0x7f;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 45:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			state->mapper45_reg[0] = state->mapper45_reg[1] = state->mapper45_reg[2] = state->mapper45_reg[3] = 0;
			state->mmc_prg_base = 0x30;
			state->mmc_prg_mask = 0x3f;
			state->mmc_chr_base = 0;
			state->mmc_chr_mask = 0x7f;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			//          memory_set_bankptr(machine, "bank5", state->wram);
			break;
		case 46:
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
		case 54:
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
		case 50:
			prg8_67(machine, 0x0f);
			prg8_89(machine, 0x08);
			prg8_ab(machine, 0x09);
			prg8_cd(machine, 0);
			prg8_ef(machine, 0x0b);
			break;
		case 51:
			state->mapper51_reg[0] = 0x01;
			state->mapper51_reg[1] = 0x00;
			bmc_ball11_set_banks(machine);
			break;
		case 52:
			state->mmc_prg_bank[0] = 0xfe;
			state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = 0xff;
			state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->map52_reg_written = 0;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			state->mmc_prg_base = 0;
			state->mmc_prg_mask = 0x1f;
			state->mmc_chr_base = 0;
			state->mmc_chr_mask = 0xff;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			//          memory_set_bankptr(machine, "bank5", state->wram);
			break;
		case 57:
			state->mmc_cmd1 = 0x00;
			state->mmc_cmd2 = 0x00;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
		case 58:
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
		case 61:
		case 62:
		case 86:
			prg32(machine, 0);
			break;
		case 64:
		case 158:
			state->mmc_cmd1 = 0;
			prg16_89ab(machine, state->prg_chunks - 1);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 65:
		case 67:
		case 69:
		case 72:
		case 78:
		case 92:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 68:
			state->m68_mirror = state->m0 = state->m1 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 70:
			prg16_89ab(machine, state->prg_chunks - 2);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 71:
			set_nt_mirroring(machine, PPU_MIRROR_HORZ);
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 76:
			prg8_89(machine, 0);
			prg8_ab(machine, 1);
			//cd is bankable, but this should init all banks just fine.
			prg16_cdef(machine, state->prg_chunks - 1);
			chr2_0(machine, 0, CHRROM);
			chr2_2(machine, 1, CHRROM);
			chr2_4(machine, 2, CHRROM);
			chr2_6(machine, 3, CHRROM);
			state->mmc_cmd1 = 0;
			break;
		case 77:
			prg32(machine, 0);
			chr2_2(machine, 0, CHRROM);
			chr2_4(machine, 1, CHRROM);
			chr2_6(machine, 2, CHRROM);
			break;
		case 79:
		case 146:
			/* Mirroring always horizontal...? */
			//          Mirroring = 1;
			set_nt_mirroring(machine, PPU_MIRROR_HORZ);
			chr8(machine, 0, CHRROM);
			prg32(machine, 0);
			break;
		case 228:
			chr8(machine, 0, CHRROM);
			prg32(machine, 0);
			break;
		case 83:
			state->mapper83_reg[9] = 0x0f;
			prg8_cd(machine, 0x1e);
			prg8_ef(machine, 0x1f);
			break;
		case 88:
			prg8_89(machine, 0xc);
			prg8_ab(machine, 0xd);
			prg8_cd(machine, 0xe);
			prg8_ef(machine, 0xf);
			break;
		case 89:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			set_nt_mirroring(machine, PPU_MIRROR_LOW);
			break;
		case 91:
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			prg16_89ab(machine, state->prg_chunks - 1);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 93:
		case 94:
		case 95:
		case 101:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 97:
			prg16_89ab(machine, state->prg_chunks - 1);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 112:
			state->mmc_cmd1 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
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
			prg32(machine, 0);
			break;
		case 104:
			prg16_89ab(machine, 0x00);
			prg16_cdef(machine, 0x0f);
			break;
		case 106:
			prg8_89(machine, (state->prg_chunks << 1) - 1);
			prg8_ab(machine, 0);
			prg8_cd(machine, 0);
			prg8_ef(machine, (state->prg_chunks << 1) - 1);
			break;
		case 114:
			state->map114_reg = state->map114_reg_enabled = 0;
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 115:
			state->mapper115_reg[0] = 0;
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 121:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mapper121_reg[0] = state->mapper121_reg[1] = state->mapper121_reg[2] = 0;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 132:
		case 172:
		case 173:
			state->txc_reg[0] = state->txc_reg[1] = state->txc_reg[2] = state->txc_reg[3] = 0;
			prg32(machine, 0);
			break;
		case 134:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = 0x1f;
			state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 166:
			state->subor_reg[0] = state->subor_reg[1] = state->subor_reg[2] = state->subor_reg[3] = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0x07);
			break;
		case 167:
			state->subor_reg[0] = state->subor_reg[1] = state->subor_reg[2] = state->subor_reg[3] = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0x20);
			break;
		case 136:
			state->mmc_cmd1 = 0;
			prg32(machine, 0);
			break;
		case 137:
			state->mmc_cmd1 = 0;
			prg32(machine, 0);
			chr8(machine, state->chr_chunks - 1, CHRROM);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
		case 138:
		case 139:
		case 141:
			state->mmc_cmd1 = 0;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg32(machine, 0);
			chr8(machine, 0, state->mmc_chr_source);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
		case 143:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 1);
			chr8(machine, 0, CHRROM);
			break;
		case 133:
		case 145:
		case 147:
		case 148:
		case 149:
		case 171:
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
		case 150:
			state->mmc_cmd1 = 0;
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
		case 152:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			chr8(machine, 0, CHRROM);
			break;
		case 153:
			state->mmc_cmd1 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			// Famicom Jump II needs also the following!
			for (i = 0; i < 8; i++)
				state->map153_reg[i] = 0;

			state->map153_bank_latch = 0;
			if (state->crc_hack)
				mapper153_set_prg(machine);
			break;
		case 154:
			prg16_89ab(machine, state->prg_chunks - 2);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 156:
			prg16_89ab(machine, state->prg_chunks - 2);
			prg16_cdef(machine, state->prg_chunks - 1);
			set_nt_mirroring(machine, PPU_MIRROR_LOW);
			break;
		case 164:
			prg32(machine, 0xff);
			state->mid_ram_enable = 1;
			break;
		case 176:
			prg32(machine, (state->prg_chunks - 1) >> 1);
			break;
		case 178:
			state->mmc_cmd1 = 0;
			prg32(machine, 0);
			break;
		case 180:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0);
			break;
		case 182:
			prg32(machine, (state->prg_chunks - 1) >> 1);
			break;
		case 188:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, (state->prg_chunks - 1) ^ 0x08);
			break;
		case 187:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mapper187_reg[0] = state->mapper187_reg[1] = state->mapper187_reg[2] = state->mapper187_reg[3] = 0;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			kof96_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			kof96_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 189:
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_chr_base = 0;
			state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg32(machine, 0);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 193:
			prg32(machine, (state->prg_chunks - 1) >> 1);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
		case 197:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			unl_sf3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 198:
			state->mmc_prg_bank[0] = 0x00;
			state->mmc_prg_bank[1] = 0x01;
			state->mmc_prg_bank[2] = 0x4e;
			state->mmc_prg_bank[3] = 0x4f;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			waixing_f_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 199:
			state->mmc_prg_bank[0] = 0x00;
			state->mmc_prg_bank[1] = 0x01;
			state->mmc_prg_bank[2] = 0x3e;
			state->mmc_prg_bank[3] = 0x3f;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			waixing_g_set_chr(machine, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 200:
			prg16_89ab(machine, state->prg_chunks - 1);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case 201:
		case 213:
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
		case 202:
		case 203:
		case 204:
		case 214:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
		case 205:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = 0x10;
			state->mmc_prg_mask = 0x1f;
			state->mmc_chr_base = 0;
			state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 208:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->map208_reg[0] = state->map208_reg[1] = state->map208_reg[2] = state->map208_reg[3] = state->map208_reg[4] = 0;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mapper4_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mapper4_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case 212:
			chr8(machine, 0xff, CHRROM);
			prg32(machine, 0xff);
			break;
		case 215:
			state->mmc_prg_bank[0] = 0x00;
			state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = 0x01;
			state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->map215_reg[0] = 0x00;
			state->map215_reg[1] = 0xff;
			state->map215_reg[2] = 0x04;
			state->map215_reg[3] = 0;
			state->mmc_prg_base = 0;
			state->mmc_prg_mask = 0x1f;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			sgame_boog_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			sgame_boog_set_chr(machine, state->mmc_chr_source);
			break;
		case 216:
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg32(machine, 0);
			chr8(machine, 0, state->mmc_chr_source);
			break;
		case 217:
			state->map217_reg[0] = 0x00;
			state->map217_reg[1] = 0xff;
			state->map217_reg[2] = 0x03;
			state->map217_reg[3] = 0;
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			bmc_gc6in1_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			bmc_gc6in1_set_chr(machine, state->mmc_chr_source);
			break;
		case 221:
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0);
			break;
		case 225:
			prg32(machine, 0);
			break;
		case 226:
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0;
			prg32(machine, 0);
			break;
		case 227:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0);
			break;
		case 229:
		case 255:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 1);
			chr8(machine, 0, CHRROM);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
		case 230:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 7);
			break;
		case 231:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
		case 232:
			state->mmc_cmd1 = 0x18;
			state->mmc_cmd2 = 0x00;
			bf9096_set_prg(machine);
			break;
		case 243:
			state->mmc_vrom_bank[0] = 3;
			state->mmc_cmd1 = 0;
			chr8(machine, 3, CHRROM);
			prg32(machine, 0);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
		case 246:
			prg32(machine, 0xff);
			break;
		case 249:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			state->map249_reg = 0;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			waixing_sec_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			waixing_sec_set_chr(machine, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		default:
			/* Mapper not supported */
			err = 2;
			break;
	}

	return err;
}

/*************************************************************

 nes_mapper_reset

 Resets the mapper bankswitch areas to their defaults.
 It returns a value "err" that indicates if it was
 successful. Possible values for err are:

 0 = success
 1 = no mapper found
 2 = mapper not supported

 *************************************************************/

int nes_mapper_reset( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	int err = 0, i;
	const mmc *mapper;

	/* Set the mapper irq callback */
	mapper = nes_mapper_lookup(state->mapper);

	if (mapper == NULL)
		fatalerror("Unimplemented Mapper %d", state->mapper);
	//      logerror("Mapper %d is not yet supported, defaulting to no mapper.\n", state->mapper);    // this one would be a better output

	ppu2c0x_set_scanline_callback(state->ppu, mapper ? mapper->mmc_scanline : NULL);
	ppu2c0x_set_hblank_callback(state->ppu, mapper ? mapper->mmc_hblank : NULL);

	if (state->chr_chunks == 0)
		chr8(machine, 0, CHRRAM);
	else
		chr8(machine, 0, CHRROM);

	state->mmc5_vram_control = 0;

	/* Here, we init a few helpers: 4 prg banks and 16 chr banks - some mappers use them */
	for (i = 0; i < 4; i++)
		state->mmc_prg_bank[i] = 0;
	for (i = 0; i < 16; i++)
		state->mmc_vrom_bank[i] = 0;
	for (i = 0; i < 16; i++)
		state->mmc_extra_bank[i] = 0;

	/* Finally, we init IRQ-related quantities. */
	state->IRQ_enable = state->IRQ_enable_latch = 0;
	state->IRQ_count = state->IRQ_count_latch = 0;
	state->IRQ_toggle = 0;

	err = mapper_initialize(machine, state->mapper);

	return err;
}

void mapper_handlers_setup( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	const mmc *mapper = nes_mapper_lookup(state->mapper);

	if (mapper)
	{
		state->mmc_write_low = mapper->mmc_write_low;
		state->mmc_write_mid = mapper->mmc_write_mid;
		state->mmc_write = mapper->mmc_write;
		state->mmc_read_low = mapper->mmc_read_low;
		state->mmc_read_mid = NULL;	// in progress
		state->mmc_read = NULL;	// in progress
		ppu_latch = mapper->ppu_latch;
	}
	else
	{
		logerror("Mapper %d is not yet supported, defaulting to no mapper.\n",state->mapper);
		state->mmc_write_low = NULL;
		state->mmc_write_mid = NULL;
		state->mmc_write = NULL;
		state->mmc_read_low = NULL;
		state->mmc_read_mid = NULL;	// in progress
		state->mmc_read = NULL;	// in progress
		ppu_latch = NULL;
	}
}
