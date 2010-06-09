/*****************************************************************************************

    NES MMC Emulation - UNIF boards

    Very preliminary support for UNIF boards

    TODO:
    - add more info
    - test more images
    - properly support mirroring, WRAM, etc.

****************************************************************************************/

typedef struct __unif
{
	const char *board; /* UNIF board */
	
	int nvwram;
	int wram;
	int chrram;
	int nt;
	int board_idx;
} unif;

/*************************************************************

    Board BMC-64IN1NOREPEAT

    Games: 64-in-1 Y2K

    In MESS: Supported

*************************************************************/

static void bmc_64in1nr_set_prg( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 helper1 = (state->bmc_64in1nr_reg[1] & 0x1f);
	UINT8 helper2 = (helper1 << 1) | ((state->bmc_64in1nr_reg[1] & 0x40) >> 6);

	if (state->bmc_64in1nr_reg[0] & 0x80)
	{
		if (state->bmc_64in1nr_reg[1] & 0x80)
			prg32(machine, helper1);
		else
		{
			prg16_89ab(machine, helper2);
			prg16_cdef(machine, helper2);
		}
	}
	else
		prg16_cdef(machine, helper2);
}

static WRITE8_HANDLER( bmc_64in1nr_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("bmc_64in1nr_l_w offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	switch (offset)
	{
	case 0x1000:
	case 0x1001:
	case 0x1002:
	case 0x1003:
		state->bmc_64in1nr_reg[offset & 0x03] = data;
		bmc_64in1nr_set_prg(space->machine);
		chr8(space->machine, ((state->bmc_64in1nr_reg[0] >> 1) & 0x03) | (state->bmc_64in1nr_reg[2] << 2), CHRROM);
		break;
	}
	if (offset == 0x1000)	/* reg[0] also sets mirroring */
		set_nt_mirroring(space->machine, BIT(data, 5) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
}

static WRITE8_HANDLER( bmc_64in1nr_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("bmc_64in1nr_w offset: %04x, data: %02x\n", offset, data));

	state->bmc_64in1nr_reg[3] = data;	// reg[3] is currently unused?!?
}

/*************************************************************

    Board BMC-190IN1

    Games: 190-in-1

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( bmc_190in1_w )
{
	LOG_MMC(("bmc_190in1_w offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
	data >>= 2;
	prg16_89ab(space->machine, data);
	prg16_cdef(space->machine, data);
	chr8(space->machine, data, CHRROM);
}

/*************************************************************

    Board BMC-A65AS

    Games: 3-in-1 (N068)

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( bmc_a65as_w )
{
	UINT8 helper = (data & 0x30) >> 1;
	LOG_MMC(("bmc_a65as_w offset: %04x, data: %02x\n", offset, data));

	if (data & 0x80)
		set_nt_mirroring(space->machine, BIT(data, 5) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
	else
		set_nt_mirroring(space->machine, BIT(data, 3) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	if (data & 0x40)
		prg32(space->machine, data >> 1);
	else
	{
		prg16_89ab(space->machine, helper | (data & 0x07));
		prg16_cdef(space->machine, helper | 0x07);
	}
}

/*************************************************************

    Board BMC-GS2004

    Games: Tetris Family 6-in-1

    In MESS: Preliminary Support. It also misses WRAM handling
       (we need reads from 0x6000-0x7fff)

*************************************************************/

static WRITE8_HANDLER( bmc_gs2004_w )
{
	LOG_MMC(("bmc_gs2004_w offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, data);
}

/*************************************************************

    Board BMC-GS2013

    Games: Tetris Family 12-in-1

    In MESS: Preliminary Support. It also misses WRAM handling
       (we need reads from 0x6000-0x7fff)

*************************************************************/

static WRITE8_HANDLER( bmc_gs2013_w )
{
	LOG_MMC(("bmc_gs2013_w offset: %04x, data: %02x\n", offset, data));

	if (data & 0x08)
		prg32(space->machine, data & 0x09);
	else
		prg32(space->machine, data & 0x07);
}

/*************************************************************

    Board BMC-SUPER24IN1SC03

    Games: Super 24-in-1

    In MESS: Supported

*************************************************************/

static void bmc_s24in1sc03_set_prg( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	static const UINT8 masks[8] = {0x3f, 0x1f, 0x0f, 0x01, 0x03, 0x00, 0x00, 0x00};
	UINT8 prg_flip = (state->mmc_cmd1 & 0x40) ? 2 : 0;
	int prg_base = state->bmc_s24in1sc03_reg[1] << 1;
	int prg_mask = masks[state->bmc_s24in1sc03_reg[0] & 0x07];

	prg8_89(machine, prg_base | (state->mmc_prg_bank[0 ^ prg_flip] & prg_mask));
	prg8_ab(machine, prg_base | (state->mmc_prg_bank[1] & prg_mask));
	prg8_cd(machine, prg_base | (state->mmc_prg_bank[2 ^ prg_flip] & prg_mask));
	prg8_ef(machine, prg_base | (state->mmc_prg_bank[3] & prg_mask));
}

static void bmc_s24in1sc03_set_chr( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 chr = (state->bmc_s24in1sc03_reg[0] & 0x20) ? CHRRAM : CHRROM;
	int chr_base = (state->bmc_s24in1sc03_reg[2] << 3) & 0xf00;

	chr1_x(machine, chr_page ^ 0, chr_base | (state->mmc_vrom_bank[0] & ~0x01), chr);
	chr1_x(machine, chr_page ^ 1, chr_base | (state->mmc_vrom_bank[0] |  0x01), chr);
	chr1_x(machine, chr_page ^ 2, chr_base | (state->mmc_vrom_bank[1] & ~0x01), chr);
	chr1_x(machine, chr_page ^ 3, chr_base | (state->mmc_vrom_bank[1] |  0x01), chr);
	chr1_x(machine, chr_page ^ 4, chr_base | state->mmc_vrom_bank[2], chr);
	chr1_x(machine, chr_page ^ 5, chr_base | state->mmc_vrom_bank[3], chr);
	chr1_x(machine, chr_page ^ 6, chr_base | state->mmc_vrom_bank[4], chr);
	chr1_x(machine, chr_page ^ 7, chr_base | state->mmc_vrom_bank[5], chr);
}

static WRITE8_HANDLER( bmc_s24in1sc03_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("bmc_s24in1sc03_l_w offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	if (offset == 0x1ff0)
	{
		state->bmc_s24in1sc03_reg[0] = data;
		bmc_s24in1sc03_set_prg(space->machine);
		bmc_s24in1sc03_set_chr(space->machine);
	}

	if (offset == 0x1ff1)
	{
		state->bmc_s24in1sc03_reg[1] = data;
		bmc_s24in1sc03_set_prg(space->machine);
	}

	if (offset == 0x1ff2)
	{
		state->bmc_s24in1sc03_reg[2] = data;
		bmc_s24in1sc03_set_chr(space->machine);
	}
}

static WRITE8_HANDLER( bmc_s24in1sc03_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 cmd, bmc_s24in1sc03_helper;
	LOG_MMC(("bmc_s24in1sc03_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
		case 0x0000:
			bmc_s24in1sc03_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (bmc_s24in1sc03_helper & 0x40)
				bmc_s24in1sc03_set_prg(space->machine);

			/* Has CHR Mode changed? */
			if (bmc_s24in1sc03_helper & 0x80)
				bmc_s24in1sc03_set_chr(space->machine);
			break;

		case 0x0001:
			cmd = state->mmc_cmd1 & 0x07;
			switch (cmd)
			{
				case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
				case 2: case 3: case 4: case 5:
					state->mmc_vrom_bank[cmd] = data;
					bmc_s24in1sc03_set_chr(space->machine);
					break;
				case 6:
				case 7:
					state->mmc_prg_bank[cmd - 6] = data;
					bmc_s24in1sc03_set_prg(space->machine);
					break;
			}
			break;

		default:
			mapper4_w(space, offset, data);
			break;
	}
}

/*************************************************************

    Board BMC-T-262

    Games: 4-in-1 (D-010), 8-in-1 (A-020)

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( bmc_t262_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 mmc_helper;
	LOG_MMC(("bmc_t262_w offset: %04x, data: %02x\n", offset, data));

	if (state->mmc_cmd2 || offset == 0)
	{
		state->mmc_cmd1 = (state->mmc_cmd1 & 0x38) | (data & 0x07);
		prg16_89ab(space->machine, state->mmc_cmd1);
	}
	else
	{
		state->mmc_cmd2 = 1;
		set_nt_mirroring(space->machine, BIT(data, 1) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		mmc_helper = ((offset >> 3) & 0x20) | ((offset >> 2) & 0x18);
		state->mmc_cmd1 = mmc_helper | (state->mmc_cmd1 & 0x07);
		prg16_89ab(space->machine, state->mmc_cmd1);
		prg16_cdef(space->machine, mmc_helper | 0x07);
	}
}

/*************************************************************

    Board BMC-WS

    Games: Super 40-in-1

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( bmc_ws_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 mmc_helper;
	LOG_MMC(("bmc_ws_m_w offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x1000)
	{
		switch (offset & 0x01)
		{
		case 0:
			if (!state->mmc_cmd1)
			{
				state->mmc_cmd1 = data & 0x20;
				set_nt_mirroring(space->machine, BIT(data, 4) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
				mmc_helper = (~data & 0x08) >> 3;
				prg16_89ab(space->machine, data & ~mmc_helper);
				prg16_cdef(space->machine, data |  mmc_helper);
			}
			break;
		case 1:
			if (!state->mmc_cmd1)
			{
				chr8(space->machine, data, CHRROM);
			}
			break;
		}
	}
}

/*************************************************************

    Board DREAMTECH01

    Games: Korean Igo

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( dreamtech_l_w )
{
	LOG_MMC(("dreamtech_l_w offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	if (offset == 0x1020)	/* 0x5020 */
		prg16_89ab(space->machine, data);
}

/*************************************************************

    Board UNL-8237

    Games: Pochahontas 2

    MMC3 clone

    In MESS: Not working

*************************************************************/

static void unl_8237_set_prg( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 prg_flip = (state->mmc_cmd1 & 0x40) ? 2 : 0;

	if (!(state->unl_8237_reg[0] & 0x80))
	{
		prg8_89(machine, state->mmc_prg_bank[0 ^ prg_flip]);
		prg8_ab(machine, state->mmc_prg_bank[1]);
		prg8_cd(machine, state->mmc_prg_bank[2 ^ prg_flip]);
		prg8_ef(machine, state->mmc_prg_bank[3]);
	}
}

static void unl_8237_set_chr( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 bank[8];
	int i;

	for(i = 0; i < 6; i++)
		bank[i] = state->mmc_vrom_bank[i] | ((state->unl_8237_reg[1] << 6) & 0x100);

	chr1_x(machine, chr_page ^ 0, (bank[0] & ~0x01), CHRROM);
	chr1_x(machine, chr_page ^ 1, (bank[0] |  0x01), CHRROM);
	chr1_x(machine, chr_page ^ 2, (bank[1] & ~0x01), CHRROM);
	chr1_x(machine, chr_page ^ 3, (bank[1] |  0x01), CHRROM);
	chr1_x(machine, chr_page ^ 4, bank[2], CHRROM);
	chr1_x(machine, chr_page ^ 5, bank[3], CHRROM);
	chr1_x(machine, chr_page ^ 6, bank[4], CHRROM);
	chr1_x(machine, chr_page ^ 7, bank[5], CHRROM);
}

static WRITE8_HANDLER( unl_8237_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("unl_8237_l_w offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	if (offset == 0x1000)
	{
		state->unl_8237_reg[0] = data;
		if (state->unl_8237_reg[0] & 0x80)
		{
			if (state->unl_8237_reg[0] & 0x20)
				prg32(space->machine, (state->unl_8237_reg[0] & 0x0f) >> 1);
			else
			{
				prg16_89ab(space->machine, state->unl_8237_reg[0] & 0x1f);
				prg16_cdef(space->machine, state->unl_8237_reg[0] & 0x1f);
			}
		}
		else
			unl_8237_set_prg(space->machine);
	}

	if (offset == 0x1001)
	{
		state->unl_8237_reg[1] = data;
		unl_8237_set_chr(space->machine);
	}
}

static WRITE8_HANDLER( unl_8237_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 unl_8237_helper, cmd;
	static const UINT8 conv_table[8] = {0,2,6,1,7,3,4,5};
	LOG_MMC(("unl_8237_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
		case 0x2000:
		case 0x3000:
			state->unl_8237_reg[2] = 1;
			data = (data & 0xc0) | conv_table[data & 0x07];
			unl_8237_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (unl_8237_helper & 0x40)
				unl_8237_set_prg(space->machine);

			/* Has CHR Mode changed? */
			if (unl_8237_helper & 0x80)
				unl_8237_set_chr(space->machine);
			break;

		case 0x4000:
		case 0x5000:
			if (state->unl_8237_reg[2])
			{
				state->unl_8237_reg[2] = 0;
				cmd = state->mmc_cmd1 & 0x07;
				switch (cmd)
				{
					case 0: case 1:
					case 2: case 3: case 4: case 5:
						state->mmc_vrom_bank[cmd] = data;
						unl_8237_set_chr(space->machine);
						break;
					case 6:
					case 7:
						state->mmc_prg_bank[cmd - 6] = data;
						unl_8237_set_prg(space->machine);
						break;
				}
			}
			break;

		case 0x0000:
		case 0x1000:
			set_nt_mirroring(space->machine, (data | (data >> 7)) & 0x01 ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;

		case 0x6000:
			break;

		case 0x7000:
			mapper4_w(space, 0x6001, data);
			mapper4_w(space, 0x4000, data);
			mapper4_w(space, 0x4001, data);
			break;
	}
}

/*************************************************************

    Board UNL-AX5705

    Games: Super Mario Bros. Pocker Mali (Crayon Shin-chan pirate hack)

    In MESS: Supported

*************************************************************/

static void unl_ax5705_set_prg( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	prg8_89(machine, state->mmc_prg_bank[0]);
	prg8_ab(machine, state->mmc_prg_bank[1]);
}

static WRITE8_HANDLER( unl_ax5705_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 bank;
	LOG_MMC(("unl_ax5705_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x700f)
	{
	case 0x0000:
		state->mmc_prg_bank[0] = (data & 0x05) | ((data & 0x08) >> 2) | ((data & 0x02) << 2);
		unl_ax5705_set_prg(space->machine);
		break;
	case 0x0008:
		set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 0x2000:
		state->mmc_prg_bank[1] = (data & 0x05) | ((data & 0x08) >> 2) | ((data & 0x02) << 2);
		unl_ax5705_set_prg(space->machine);
		break;
	/* CHR banks 0, 1, 4, 5 */
	case 0x2008:
	case 0x200a:
	case 0x4008:
	case 0x400a:
		bank = ((offset & 0x4000) ? 4 : 0) + ((offset & 0x0002) ? 1 : 0);
		state->mmc_vrom_bank[bank] = (state->mmc_vrom_bank[bank] & 0xf0) | (data & 0x0f);
		chr1_x(space->machine, bank, state->mmc_vrom_bank[bank], CHRROM);
		break;
	case 0x2009:
	case 0x200b:
	case 0x4009:
	case 0x400b:
		bank = ((offset & 0x4000) ? 4 : 0) + ((offset & 0x0002) ? 1 : 0);
		state->mmc_vrom_bank[bank] = (state->mmc_vrom_bank[bank] & 0x0f) | ((data & 0x04) << 3) | ((data & 0x02) << 5) | ((data & 0x09) << 4);
		chr1_x(space->machine, bank, state->mmc_vrom_bank[bank], CHRROM);
		break;
	/* CHR banks 2, 3, 6, 7 */
	case 0x4000:
	case 0x4002:
	case 0x6000:
	case 0x6002:
		bank = 2 + ((offset & 0x2000) ? 4 : 0) + ((offset & 0x0002) ? 1 : 0);
		state->mmc_vrom_bank[bank] = (state->mmc_vrom_bank[bank] & 0xf0) | (data & 0x0f);
		chr1_x(space->machine, bank, state->mmc_vrom_bank[bank], CHRROM);
		break;
	case 0x4001:
	case 0x4003:
	case 0x6001:
	case 0x6003:
		bank = 2 + ((offset & 0x2000) ? 4 : 0) + ((offset & 0x0002) ? 1 : 0);
		state->mmc_vrom_bank[bank] = (state->mmc_vrom_bank[bank] & 0x0f) | ((data & 0x04) << 3) | ((data & 0x02) << 5) | ((data & 0x09) << 4);
		chr1_x(space->machine, bank, state->mmc_vrom_bank[bank], CHRROM);
		break;
	}
}

/*************************************************************

    Board UNL-CC-21

    Games: Mi Hun Che

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( unl_cc21_w )
{
	LOG_MMC(("unl_cc21_w offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring(space->machine, BIT(data, 1) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
	chr8(space->machine, (offset & 0x01), CHRROM);
}

/*************************************************************

    Board UNL-KOF97

    Games: King of Fighters 97 (Rex Soft)

    MMC3 clone

    In MESS: Not working

*************************************************************/

static UINT8 unl_kof97_unscramble( UINT8 data )
{
	return ((data >> 1) & 0x01) | ((data >> 4) & 0x02) | ((data << 2) & 0x04) | ((data >> 0) & 0xd8) | ((data << 3) & 0x20);
}

static WRITE8_HANDLER( unl_kof97_w )
{
	LOG_MMC(("unl_kof97_w offset: %04x, data: %02x\n", offset, data));

	/* Addresses 0x9000, 0xa000, 0xd000 & 0xf000 behaves differently than MMC3 */
	if (offset == 0x1000)
	{
		data = unl_kof97_unscramble(data);
		mapper4_w(space, 0x0001, data);
	}
	else if (offset == 0x2000)
	{
		data = unl_kof97_unscramble(data);
		mapper4_w(space, 0x0000, data);
	}
	else if (offset == 0x5000)
	{
		data = unl_kof97_unscramble(data);
		mapper4_w(space, 0x4001, data);
	}
	else if (offset == 0x7000)
	{
		data = unl_kof97_unscramble(data);
		mapper4_w(space, 0x6001, data);
	}
	else		/* Other addresses behaves like MMC3, up to unscrambling data */
	{
		switch (offset & 0x6001)
		{
		case 0x0000:
		case 0x0001:
		case 0x4000:
		case 0x4001:
		case 0x6000:
		case 0x6001:
		case 0x2000:	/* are these ever called?!? */
		case 0x2001:
			data = unl_kof97_unscramble(data);
			mapper4_w(space, offset, data);
			break;
		}
	}
}

/*************************************************************

    Board UNL-T-230

    Games: Dragon Ball Z IV (Unl)

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( unl_t230_w )
{
	LOG_MMC(("unl_t230_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7007)
	{
		case 0x0000:
			break;
		case 0x2000:
			data = (data << 1) & 0x1f;
			prg8_89(space->machine, data);
			prg8_ab(space->machine, data | 0x01);
			break;

		default:
			konami_vrc2b_w(space, offset, data);
			break;
	}
}


/*************************************************************

    Constants

*************************************************************/

/* CHRRAM sizes */
enum
{
	CHRRAM_0 = 0,
	CHRRAM_1,
	CHRRAM_2,
	CHRRAM_4,
	CHRRAM_6,
	CHRRAM_8,
	CHRRAM_16,
	CHRRAM_32
};

/* NT mirroring */
enum
{
	NT_X = 0,
	NT_HORZ,	// not hardwired, horizontal at start
	NT_VERT,	// not hardwired, vertical at start
	NT_Y,
	NT_1SCREEN,
	NT_4SCR_2K,
	NT_4SCR_4K
};

/* Boards */
enum
{
	STD_NROM = 0,
	STD_AXROM, STD_BXROM, STD_CNROM, STD_CPROM, 
	STD_DXROM, STD_EXROM, STD_FXROM, STD_GXROM, 
	STD_HKROM, STD_JXROM, STD_MXROM, STD_NXROM, 
	STD_PXROM, STD_SXROM, STD_TXROM, STD_TXSROM, 
	STD_TQROM, STD_UN1ROM, STD_UXROM,
	HVC_FAMBASIC, NES_QJ, PAL_ZZ,
	STD_SXROM_A, STD_SOROM,
	/* Discrete components boards (by various manufacturer */
	DIS_74X161X138, 
	DIS_74X139X74, DIS_74X377, DIS_74X161X161X32,
	/* Active Enterprises */
	ACTENT_ACT52,
	/* AGCI */
	AGCI_50282,
	/* AVE */
	AVE_NINA01, AVE_NINA06,
	/* Bandai */
	BANDAI_JUMP2, BANDAI_PT554,
	BANDAI_DATACH, BANDAI_KARAOKE, BANDAI_OEKAKIDS, 
	BANDAI_FCG, BANDAI_LZ93EX,
	/* Caltron */
	CALTRON_6IN1,
	/* Camerica */
	CAMERICA_ALGN, CAMERICA_ALGQ, CAMERICA_9097,
	CAMERICA_GOLDENFIVE, GG_NROM,
	/* Dreamtech */
	DREAMTECH_BOARD,
	/* Irem */
	IREM_G101, IREM_HOLYDIV, IREM_H3001, IREM_LROG017,
	/* Jaleco */
	JALECO_SS88006, JALECO_JF11, JALECO_JF13,
	JALECO_JF16, JALECO_JF17, JALECO_JF19,
	/* Konami */
	KONAMI_VRC1, KONAMI_VRC2, KONAMI_VRC3,
	KONAMI_VRC4, KONAMI_VRC6, KONAMI_VRC7,
	/* Namcot */
	NAMCOT_163,
	NAMCOT_3425, NAMCOT_34X3, NAMCOT_3446,
	/* NTDEC */
	NTDEC_ASDER, NTDEC_FIGHTINGHERO,
	/* Rex Soft */
	REXSOFT_SL1632, REXSOFT_DBZ5,
	/* Sachen */
	SACHEN_8259A, SACHEN_8259B, SACHEN_8259C,
	SACHEN_8259D, SACHEN_SA0036, SACHEN_SA0037,
	SACHEN_SA72007, SACHEN_SA72008, SACHEN_TCA01, 
	SACHEN_TCU01, SACHEN_TCU02, SACHEN_74LS374,
	/* Sunsoft */
	SUNSOFT_1, SUNSOFT_2, SUNSOFT_3, 
	SUNSOFT_4, SUNSOFT_5B, SUNSOFT_FME7,
	/* Taito */
	TAITO_TC0190FMC, TAITO_TC0190FMCP, TAITO_X1005, TAITO_X1017,
	/* Tengen */
	TENGEN_800008, TENGEN_800032, TENGEN_800037,
	/* TXC */
	TXC_22211A, TXC_MXMDHTWO, TXC_TW,
	/* Multigame Carts */
	BMC_64IN1NR, BMC_190IN1, BMC_A65AS, BMC_GS2004, BMC_GS2013,
	BMC_HIK8IN1, BMC_NOVELDIAMOND, BMC_S24IN1SC03, BMC_T262,
	BMC_WS, BMC_SUPERBIG_7IN1, BMC_SUPERHIK_4IN1, BMC_BALLGAMES_11IN1,
	BMC_MARIOPARTY_7IN1, BMC_SUPER_700IN1, BMC_FAMILY_4646B, 
	BMC_36IN1, BMC_21IN1, BMC_150IN1, BMC_35IN1, BMC_64IN1,
	BMC_15IN1, BMC_SUPERHIK_300IN1, BMC_9999999IN1, BMC_SUPERGUN_20IN1,
	BMC_GOLDENCARD_6IN1, BMC_72IN1, BMC_SUPER_42IN1, BMC_76IN1,
	BMC_1200IN1, BMC_31IN1, BMC_22GAMES, BMC_20IN1, BMC_110IN1, BMC_GKA, BMC_GKB,
	/* Unlicensed */
	UNL_8237, UNL_CC21, UNL_AX5705, UNL_KOF97,
	UNL_N625092, UNL_SC127, UNL_SMB2J, UNL_T230,
	UNL_UXROM, UNL_MK2, UNL_XZY, UNL_KOF96,
	UNL_SUPERFIGHTER3, 
	/* Bootleg boards */
	BTL_SMB2A, BTL_MARIOBABY, BTL_AISENSHINICOL,
	BTL_SMB2B, BTL_SMB3, BTL_SUPERBROS11, BTL_DRAGONNINJA,
	/* Misc: these are needed to convert mappers to boards, I will sort them later */
	OPENCORP_DAOU306, HES_BOARD, RUMBLESTATION_BOARD, SUPERGAME_BOOGERMAN,
	MAGICSERIES_MAGICDRAGON, KASING_BOARD, FUTUREMEDIA_BOARD, SOMERITEAM_SL12,
	HENGEDIANZI_BOARD, SUBOR_TYPE0, SUBOR_TYPE1, KAISER_KS7058, CONY_BOARD,
	CNE_DECATHLON, CNE_PSB, CNE_SHLZ, RCM_GS2015, RCM_TETRISFAMILY,
	WAIXING_TYPE_A, WAIXING_TYPE_B, WAIXING_TYPE_C, WAIXING_TYPE_D,
	WAIXING_TYPE_E, WAIXING_TYPE_F, WAIXING_TYPE_G, WAIXING_TYPE_H,
	WAIXING_SGZLZ, WAIXING_SGZ, WAIXING_ZS, WAIXING_SECURITY,
	WAIXING_FFV, WAIXING_PS2, KAY_PANDAPRINCE, SUPERGAME_LIONKING,
	HOSENKAN_BOARD, NITRA_TDA, GOUDER_37017, 
	/* Unsupported (for place-holder boards, with no working emulation) */
	UNSUPPORTED_BOARD
};

/*************************************************************

    unif_list

    Supported UNIF boards and corresponding handlers

*************************************************************/

static const unif unif_list[] =
{
/*       UNIF                       LOW_W, LOW_R, MED_W, HIGH_W, PPU_latch, scanline CB, hblank CB     PRG  CHR  NVW  WRAM  CRAM       NMT    IDX*/
	{ "DREAMTECH01",                0,    0, CHRRAM_8,  NT_X,  DREAMTECH_BOARD},		//UNIF only!
	{ "NES-ANROM",                  0,    0, CHRRAM_8,  NT_Y,  STD_AXROM},
	{ "NES-AOROM",                  0,    0, CHRRAM_8,  NT_Y,  STD_AXROM},
	{ "NES-CNROM",                  0,    0, CHRRAM_0,  NT_X,  STD_CNROM},
	{ "NES-NROM",                   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NES-NROM-128",               0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NES-NROM-256",               0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NES-NTBROM",                 8,    0, CHRRAM_0,  NT_VERT,  STD_NXROM},
	{ "NES-SLROM",                  0,    0, CHRRAM_0,  NT_HORZ,  STD_SXROM},
	{ "NES-TBROM",                  0,    0, CHRRAM_0,  NT_X,  STD_TXROM},
	{ "NES-TFROM",                  0,    0, CHRRAM_0,  NT_X,  STD_TXROM},
	{ "NES-TKROM",                  8,    0, CHRRAM_0,  NT_X,  STD_TXROM},
	{ "NES-TLROM",                  0,    0, CHRRAM_0,  NT_X,  STD_TXROM},
	{ "NES-UOROM",                  0,    0, CHRRAM_8,  NT_X,  STD_UXROM},
	{ "UNL-22211",                  0,    0, CHRRAM_0,  NT_X,  TXC_22211A},
	// mapper 172 & 173 are variant of this one... no UNIF?
	{ "UNL-KOF97",                  0,    0, CHRRAM_0,  NT_X,  UNL_KOF97},
	{ "UNL-SA-NROM",                0,    0, CHRRAM_0,  NT_X,  SACHEN_TCA01},
	{ "UNL-VRC7",                   0,    0, CHRRAM_0,  NT_VERT,  KONAMI_VRC7},
	{ "UNL-T-230",                  0,    0, CHRRAM_8,  NT_VERT,  UNL_T230},
	{ "UNL-CC-21",                  0,    0, CHRRAM_0,  NT_Y,  UNL_CC21},
	{ "UNL-AX5705",                 0,    0, CHRRAM_0,  NT_X,  UNL_AX5705},
	{ "UNL-SMB2J",                  8,    8, CHRRAM_0,  NT_X,  UNL_SMB2J},
	{ "UNL-8237",                   0,    0, CHRRAM_0,  NT_X,  UNL_8237},
	{ "UNL-SL1632",                 0,    0, CHRRAM_0,  NT_VERT,  REXSOFT_SL1632},
	{ "UNL-SACHEN-74LS374N",        0,    0, CHRRAM_0,  NT_X,  SACHEN_74LS374},
	// mapper 243 variant exists! how to distinguish?!?  mapper243_l_w, NULL, NULL, NULL, NULL, NULL, NULL (also uses NT_VERT!)
	{ "UNL-TC-U01-1.5M",            0,    0, CHRRAM_0,  NT_X,  SACHEN_TCU01},
	{ "UNL-SACHEN-8259C",           0,    0, CHRRAM_0,  NT_X,  SACHEN_8259C},
	{ "UNL-SA-016-1M",              0,    0, CHRRAM_0,  NT_X,  AVE_NINA06},	// actually this is Mapper 146, but works like 79!
	{ "UNL-SACHEN-8259D",           0,    0, CHRRAM_0,  NT_X,  SACHEN_8259D},
	{ "UNL-SA-72007",               0,    0, CHRRAM_0,  NT_X,  SACHEN_SA72007},
	{ "UNL-SA-72008",               0,    0, CHRRAM_0,  NT_X,  SACHEN_SA72008},
	{ "UNL-SA-0037",                0,    0, CHRRAM_0,  NT_X,  SACHEN_SA0037},
	{ "UNL-SA-0036",                0,    0, CHRRAM_0,  NT_X,  SACHEN_SA0036},
	{ "UNL-SACHEN-8259A",           0,    0, CHRRAM_0,  NT_X,  SACHEN_8259A},
	{ "UNL-SACHEN-8259B",           0,    0, CHRRAM_0,  NT_X,  SACHEN_8259B},
	{ "BMC-190IN1",                 0,    0, CHRRAM_0,  NT_VERT,  BMC_190IN1},
	{ "BMC-64IN1NOREPEAT",          0,    0, CHRRAM_0,  NT_VERT,  BMC_64IN1NR},		//UNIF only!
	{ "BMC-A65AS",                  0,    0, CHRRAM_8,  NT_VERT,  BMC_A65AS},		//UNIF only!
	{ "BMC-GS-2004",                0,    0, CHRRAM_8,  NT_X,  BMC_GS2004},		//UNIF only!
	{ "BMC-GS-2013",                0,    0, CHRRAM_8,  NT_X,  BMC_GS2013},		//UNIF only!
	{ "BMC-NOVELDIAMOND9999999IN1", 0,    0, CHRRAM_0,  NT_X,  BMC_NOVELDIAMOND},
	{ "BMC-SUPER24IN1SC03",         8,    0, CHRRAM_8,  NT_X,  BMC_S24IN1SC03},
	{ "BMC-SUPERHIK8IN1",           8,    0, CHRRAM_0,  NT_X,  BMC_HIK8IN1},
	{ "BMC-T-262",                  0,    0, CHRRAM_8,  NT_VERT,  BMC_T262},		//UNIF only!
	{ "BMC-WS",                     0,    0, CHRRAM_0,  NT_VERT,  BMC_WS},		//UNIF only!	
	{ "BMC-N625092",                0,    0, CHRRAM_0,  NT_VERT,  UNL_N625092},
	// below are boards which are not yet supported, but are used by some UNIF files. they are here as a reminder to what is missing to be added
	{ "UNL-TEK90",                  0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},	// related to JY Company? (i.e. mappers 90, 209, 211?)
	{ "UNL-KS7017",                 0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},
	{ "UNL-KS7032",                 0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD}, //  mapper 142
	{ "UNL-DANCE",                  0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},
	{ "UNL-603-5052",               0,    0, 0,         0,     UNSUPPORTED_BOARD}, // mapper 238?
	{ "UNL-EDU2000",               32,    0, CHRRAM_8,  NT_Y,  UNSUPPORTED_BOARD /*UNL_EDU2K*/},
	{ "UNL-H2288",                  0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},	// mapper 123
	{ "UNL-SHERO",                  0,    0, CHRRAM_8,  NT_4SCR_2K, UNSUPPORTED_BOARD /*SACHEN_SHERO*/},
	{ "UNL-TF1201",                 0,    0, CHRRAM_0,  NT_VERT, UNSUPPORTED_BOARD /*UNL_TF1201*/},
	{ "UNL-DRIPGAME",               0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD}, // [by Quietust - we need more info]}
	{ "BTL-MARIO1-MALEE2",          0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},	// mapper 55?
	{ "BMC-FK23C",                  0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD /*BMC_FK23C*/},
	{ "BMC-FK23CA",                 0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD /*BMC_FK23C*/},	// diff reg init
	{ "BMC-GHOSTBUSTERS63IN1",      0,    0, CHRRAM_8,  NT_HORZ, UNSUPPORTED_BOARD /*BMC_G63IN1*/},
	{ "BMC-BS-5",                   0,    0, CHRRAM_0,  NT_VERT, UNSUPPORTED_BOARD /*BENSHENG_BS5*/},
	{ "BMC-810544-C-A1",            0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},
	{ "BMC-411120-C",               0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},
	{ "BMC-8157",                   0,    0, CHRRAM_8,  NT_VERT, UNSUPPORTED_BOARD /*BMC_8157*/},
	{ "BMC-42IN1RESETSWITCH",       0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},	// mapper 60?
	{ "BMC-830118C",                0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},
	{ "BMC-D1038",                  0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD}, // mapper 60?
	{ "BMC-12-IN-1",                0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},
	{ "BMC-70IN1",                  0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},	// mapper 236?
	{ "BMC-70IN1B",                 0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},	// mapper 236?
	{ "BMC-SUPERVISION16IN1",       0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD},	// mapper 53
};

const unif *nes_unif_lookup( const char *board )
{
	int i;
	for (i = 0; i < ARRAY_LENGTH(unif_list); i++)
	{
		if (!mame_stricmp(unif_list[i].board, board))
			return &unif_list[i];
	}
	return NULL;
}

// WIP code
static int unif_initialize( running_machine *machine, int idx )
{
	nes_state *state = (nes_state *)machine->driver_data;
	int err = 0;
	
	switch (idx)
	{
		case BMC_64IN1NR:
			state->bmc_64in1nr_reg[0] = 0x80;
			state->bmc_64in1nr_reg[1] = 0x43;
			state->bmc_64in1nr_reg[2] = state->bmc_64in1nr_reg[3] = 0;
			bmc_64in1nr_set_prg(machine);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			chr8(machine, 0, CHRROM);
			break;
		case BMC_190IN1:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
		case BMC_A65AS:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 7);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
		case BMC_GS2004:
		case BMC_GS2013:
			prg32(machine, 0xff);
			chr8(machine, 0, CHRRAM);
			break;
		case BMC_S24IN1SC03:
			state->bmc_s24in1sc03_reg[0] = 0x24;
			state->bmc_s24in1sc03_reg[1] = 0x9f;
			state->bmc_s24in1sc03_reg[2] = 0;
			state->mmc_prg_bank[0] = 0xfe;
			state->mmc_prg_bank[1] = 0xff;
			state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			bmc_s24in1sc03_set_prg(machine);
			bmc_s24in1sc03_set_chr(machine);
			break;
		case BMC_T262:
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 7);
			break;
		case BMC_WS:
			state->mmc_cmd1 = 0;
			prg32(machine, 0);
			break;
		case DREAMTECH_BOARD:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 8);
			chr8(machine, 0, CHRRAM);
			break;
		case UNL_8237:
			state->unl_8237_reg[0] = state->unl_8237_reg[1] = state->unl_8237_reg[2] = 0;
			state->mmc_prg_bank[0] = 0x00;
			state->mmc_prg_bank[1] = 0x01;
			state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[3] = 0xff;
			state->mmc_cmd1 = 0;
			state->mmc_cmd2 = 0x80;
			unl_8237_set_prg(machine);
			unl_8237_set_chr(machine);
			break;
		case UNL_AX5705:
			state->mmc_prg_bank[0] = 0;
			state->mmc_prg_bank[1] = 1;
			prg8_89(machine, state->mmc_prg_bank[0]);
			prg8_ab(machine, state->mmc_prg_bank[1]);
			prg8_cd(machine, 0xfe);
			prg8_ef(machine, 0xff);
			break;
		case UNL_CC21:
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
		case UNL_KOF97:
			mapper_initialize(machine, 4);
			break;
		case UNL_T230:
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
			/* For the Boards corresponding to a Mapper, fall back to the mapper init */
		case STD_NROM:
		case HVC_FAMBASIC:
		case GG_NROM:
			mapper_initialize(machine, 0);
			break;
		case STD_SXROM:
		case STD_SOROM:
		case STD_SXROM_A:
			mapper_initialize(machine, 1);
			break;
		case STD_UXROM:
			mapper_initialize(machine, 2);
			break;
		case STD_CNROM:
		case BANDAI_PT554:
		case TENGEN_800008:
			mapper_initialize(machine, 3);
			break;
		case STD_HKROM:
		case STD_TXROM:
			mapper_initialize(machine, 4);
			break;
		case STD_EXROM:
			mapper_initialize(machine, 5);
			break;
		case STD_AXROM:
			mapper_initialize(machine, 7);
			break;
		case STD_PXROM:
			mapper_initialize(machine, 9);
			break;
		case STD_FXROM:
			mapper_initialize(machine, 10);
			break;
		case DIS_74X377:
			mapper_initialize(machine, 11);
			break;
		case REXSOFT_DBZ5:
			mapper_initialize(machine, 12);
			break;
		case REXSOFT_SL1632:
			mapper_initialize(machine, 14);
			break;
		case WAIXING_PS2:
			mapper_initialize(machine, 15);
			break;
		case BANDAI_LZ93EX:
			mapper_initialize(machine, 16);
			break;
		case JALECO_SS88006:
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
			mapper_initialize(machine, 32);
			break;
		case TAITO_TC0190FMC:
			mapper_initialize(machine, 33);
			break;
		case STD_BXROM:
			mapper_initialize(machine, 34);
			break;
		case UNL_SC127:
			mapper_initialize(machine, 35);
			break;
		case PAL_ZZ:
			mapper_initialize(machine, 37);
			break;
		case DIS_74X161X138:
			mapper_initialize(machine, 38);
			break;
		case BTL_SMB2A:
			mapper_initialize(machine, 40);
			break;
		case CALTRON_6IN1:
			mapper_initialize(machine, 41);
			break;
		case BTL_MARIOBABY:
		case BTL_AISENSHINICOL:
			mapper_initialize(machine, 42);
			break;
		case UNL_SMB2J:
			mapper_initialize(machine, 43);
			break;
		case BMC_SUPERBIG_7IN1:
			mapper_initialize(machine, 44);
			break;
		case RUMBLESTATION_BOARD:
			mapper_initialize(machine, 46);
			break;
		case NES_QJ:
			mapper_initialize(machine, 47);
			break;
		case TAITO_TC0190FMCP:
			mapper_initialize(machine, 48);
			break;
		case BMC_SUPERHIK_4IN1:
			mapper_initialize(machine, 49);
			break;
		case BTL_SMB2B:
			mapper_initialize(machine, 50);
			break;
		case BMC_BALLGAMES_11IN1:
			mapper_initialize(machine, 51);
			break;
		case BMC_MARIOPARTY_7IN1:
			mapper_initialize(machine, 52);
			break;
		case BMC_NOVELDIAMOND:
			mapper_initialize(machine, 54);
			break;
		case BMC_GKA:
			mapper_initialize(machine, 57);
			break;
		case BMC_GKB:
			mapper_initialize(machine, 58);
			break;
		case RCM_TETRISFAMILY:
			mapper_initialize(machine, 61);
			break;
		case BMC_SUPER_700IN1:
			mapper_initialize(machine, 62);
			break;
		case TENGEN_800032:
			mapper_initialize(machine, 64);
			break;
		case IREM_H3001:
			mapper_initialize(machine, 65);
			break;
		case STD_GXROM:
		case STD_MXROM:
			mapper_initialize(machine, 66);
			break;
		case SUNSOFT_3:
			mapper_initialize(machine, 67);
			break;
		case STD_NXROM:
			mapper_initialize(machine, 68);
			break;
		case STD_JXROM:
		case SUNSOFT_5B:
		case SUNSOFT_FME7:
			mapper_initialize(machine, 69);
			break;
		case DIS_74X161X161X32:
			mapper_initialize(machine, 70);
			break;
		case CAMERICA_ALGN:
			mapper_initialize(machine, 71);
			break;
		case CAMERICA_9097:
			state->crc_hack = 0xf0;
			mapper_initialize(machine, 71);
			break;
		case JALECO_JF17:
			mapper_initialize(machine, 72);
			break;
		case KONAMI_VRC3:
			mapper_initialize(machine, 73);
			break;
		case WAIXING_TYPE_A:
			mapper_initialize(machine, 74);
			break;
		case KONAMI_VRC1:
			mapper_initialize(machine, 75);
			break;
		case NAMCOT_3446:
			mapper_initialize(machine, 76);
			break;
		case IREM_LROG017:
			mapper_initialize(machine, 77);
			break;
		case IREM_HOLYDIV:
		case JALECO_JF16:
			mapper_initialize(machine, 78);
			break;
		case AVE_NINA06:
			mapper_initialize(machine, 79);
			break;
		case TAITO_X1005:
			mapper_initialize(machine, 80);
			break;
		case TAITO_X1017:
			mapper_initialize(machine, 82);
			break;
		case CONY_BOARD:
			mapper_initialize(machine, 83);
			break;
		case KONAMI_VRC7:
			mapper_initialize(machine, 85);
			break;
		case JALECO_JF13:
			mapper_initialize(machine, 86);
			break;
		case DIS_74X139X74:
			mapper_initialize(machine, 87);
			break;
		case NAMCOT_34X3:
			mapper_initialize(machine, 88);
			break;
		case SUNSOFT_2:
			mapper_initialize(machine, 89);
			break;
		case UNL_MK2:
			mapper_initialize(machine, 91);
			break;
		case JALECO_JF19:
			mapper_initialize(machine, 92);
			break;
		case STD_UN1ROM:
			mapper_initialize(machine, 94);
			break;
		case NAMCOT_3425:
			mapper_initialize(machine, 95);
			break;
		case BANDAI_OEKAKIDS:
			mapper_initialize(machine, 96);
			break;
		case CAMERICA_GOLDENFIVE:
			mapper_initialize(machine, 104);
			break;
		case BTL_SMB3:
			mapper_initialize(machine, 106);
			break;
		case MAGICSERIES_MAGICDRAGON:
			mapper_initialize(machine, 107);
			break;
		case NTDEC_ASDER:
			mapper_initialize(machine, 112);
			break;
		case HES_BOARD:
			mapper_initialize(machine, 113);
			break;
		case SUPERGAME_LIONKING:
			mapper_initialize(machine, 114);
			break;
		case KASING_BOARD:
			mapper_initialize(machine, 115);
			break;
		case SOMERITEAM_SL12:
			mapper_initialize(machine, 116);
			break;
		case FUTUREMEDIA_BOARD:
			mapper_initialize(machine, 117);
			break;
		case STD_TXSROM:
			mapper_initialize(machine, 118);
			break;
		case STD_TQROM:
			mapper_initialize(machine, 119);
			break;
		case KAY_PANDAPRINCE:
			mapper_initialize(machine, 121);
			break;
		case TXC_22211A:
			mapper_initialize(machine, 132);
			break;
		case SACHEN_SA72008:
			mapper_initialize(machine, 133);
			break;
		case BMC_FAMILY_4646B:
			mapper_initialize(machine, 134);
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
		case BANDAI_FCG:
		case BANDAI_JUMP2:
			mapper_initialize(machine, 153);
			break;
		case OPENCORP_DAOU306:
			mapper_initialize(machine, 156);
			break;
		case BANDAI_DATACH:
			mapper_initialize(machine, 157);
			break;
		case TENGEN_800037:
			mapper_initialize(machine, 158);
			break;
		case WAIXING_FFV:
			mapper_initialize(machine, 164);
			break;
		case SUBOR_TYPE1:
			mapper_initialize(machine, 166);
			break;
		case SUBOR_TYPE0:
			mapper_initialize(machine, 167);
			break;
		case KAISER_KS7058:
			mapper_initialize(machine, 171);
			break;
		case UNL_XZY:
			mapper_initialize(machine, 176);
			break;
		case HENGEDIANZI_BOARD:
			mapper_initialize(machine, 177);
			break;
		case WAIXING_SGZLZ:
			mapper_initialize(machine, 178);
			break;
		case HOSENKAN_BOARD:
			mapper_initialize(machine, 182);
			break;
		case SUNSOFT_1:
			mapper_initialize(machine, 184);
			break;
		case UNL_KOF96:
			mapper_initialize(machine, 187);
			break;
		case BANDAI_KARAOKE:
			mapper_initialize(machine, 188);
			break;
		case TXC_TW:
			mapper_initialize(machine, 189);
			break;
		case WAIXING_TYPE_B:
			mapper_initialize(machine, 191);
			break;
		case WAIXING_TYPE_C:
			mapper_initialize(machine, 192);
			break;
		case NTDEC_FIGHTINGHERO:
			mapper_initialize(machine, 193);
			break;
		case WAIXING_TYPE_D:
			mapper_initialize(machine, 194);
			break;
		case WAIXING_TYPE_E:
			mapper_initialize(machine, 195);
			break;
		case BTL_SUPERBROS11:
			mapper_initialize(machine, 196);
			break;
		case UNL_SUPERFIGHTER3:
			mapper_initialize(machine, 197);
			break;
		case WAIXING_TYPE_F:
			mapper_initialize(machine, 198);
			break;
		case WAIXING_TYPE_G:
			mapper_initialize(machine, 199);
			break;
		case BMC_36IN1:
			mapper_initialize(machine, 200);
			break;
		case BMC_21IN1:
			mapper_initialize(machine, 201);
			break;
		case BMC_150IN1:
			mapper_initialize(machine, 202);
			break;
		case BMC_35IN1:
			mapper_initialize(machine, 203);
			break;
		case BMC_64IN1:
			mapper_initialize(machine, 204);
			break;
		case BMC_15IN1:
			mapper_initialize(machine, 205);
			break;
		case STD_DXROM:
			mapper_initialize(machine, 206);
			break;
		case GOUDER_37017:
			mapper_initialize(machine, 208);
			break;
		case BMC_SUPERHIK_300IN1:
			mapper_initialize(machine, 212);
			break;
		case BMC_9999999IN1:
			mapper_initialize(machine, 213);
			break;
		case BMC_SUPERGUN_20IN1:
			mapper_initialize(machine, 214);
			break;
		case SUPERGAME_BOOGERMAN:
			mapper_initialize(machine, 215);
			break;
		case RCM_GS2015:
			mapper_initialize(machine, 216);
			break;
		case BMC_GOLDENCARD_6IN1:
			mapper_initialize(machine, 217);
			break;
		case UNL_N625092:
			mapper_initialize(machine, 221);
			break;
		case BTL_DRAGONNINJA:
			mapper_initialize(machine, 222);
			break;
		case BMC_72IN1:
			mapper_initialize(machine, 225);
			break;
		case BMC_76IN1:
		case BMC_SUPER_42IN1:
			mapper_initialize(machine, 226);
			break;
		case BMC_1200IN1:
			mapper_initialize(machine, 227);
			break;
		case ACTENT_ACT52:
			mapper_initialize(machine, 228);
			break;
		case BMC_31IN1:
			mapper_initialize(machine, 229);
			break;
		case BMC_22GAMES:
			mapper_initialize(machine, 230);
			break;
		case BMC_20IN1:
			mapper_initialize(machine, 231);
			break;
		case CAMERICA_ALGQ:
			mapper_initialize(machine, 232);
			break;
		case CNE_SHLZ:
			mapper_initialize(machine, 240);
			break;
		case TXC_MXMDHTWO:
			mapper_initialize(machine, 241);
			break;
		case WAIXING_ZS:
			mapper_initialize(machine, 242);
			break;
		case CNE_DECATHLON:
			mapper_initialize(machine, 244);
			break;
		case WAIXING_TYPE_H:
			mapper_initialize(machine, 245);
			break;
		case CNE_PSB:
			mapper_initialize(machine, 246);
			break;
		case WAIXING_SECURITY:
			mapper_initialize(machine, 249);
			break;
		case NITRA_TDA:
			mapper_initialize(machine, 250);
			break;
		case WAIXING_SGZ:
			mapper_initialize(machine, 252);
			break;
		case BMC_110IN1:
			mapper_initialize(machine, 255);
			break;
			
		case UNSUPPORTED_BOARD:
		default:
			/* Mapper not supported */
			err = 2;
			break;
	}
	
	return err;
}

/*************************************************************
 
 unif_mapr_setup

 setup the board specific variables (wram, nvwram, pcb_id etc.)
 for a given board (after reading the MAPR chunk of the UNIF file)

 *************************************************************/

void unif_mapr_setup( running_machine *machine, const char *board )
{
	nes_state *state = (nes_state *)machine->driver_data;
	const unif *unif_board = nes_unif_lookup(board);
	
	logerror("%s\n", board);
	
	if (unif_board == NULL)
		fatalerror("Unknown UNIF board %s.", board);
	
	state->mapper = 0;	// this allows us to set up memory handlers without duplicating code (for the moment)
	state->pcb_id = unif_board->board_idx;
	state->battery = unif_board->nvwram;	// we should implement WRAM banks based on the size of this...
	state->battery_size = NES_BATTERY_SIZE; // FIXME: we should allow for smaller battery!
	state->prg_ram = unif_board->wram;	// we should implement WRAM banks based on the size of this...
	// state->hard_mirroring = unif_board->nt;
	// state->four_screen_vram = ;
}
