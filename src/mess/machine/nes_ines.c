/*****************************************************************************************

    NES MMC Emulation

    Support for iNES Mappers

****************************************************************************************/

typedef struct _nes_mmc  nes_mmc;
struct _nes_mmc
{
	int    iNesMapper; /* iNES Mapper # */
	int    pcb_id;
};

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

#if 0

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

    Mapper 90

    Known Boards: Type A by J.Y. Company
    Games: Aladdin, Final Fight 3, Super Mario World, Tekken 2

    In MESS: Unsupported.

*************************************************************/

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

#endif

/*************************************************************

    mmc_list

    Supported mappers and corresponding handlers

*************************************************************/

static const nes_mmc mmc_list[] =
{
/*  INES   DESC                          LOW_W, LOW_R, MED_W, HIGH_W, PPU_latch, scanline CB, hblank CB */
	{  0, STD_NROM },
	{  1, STD_SXROM },
	{  2, STD_UXROM },
	{  3, STD_CNROM },
	{  4, STD_TXROM },
	{  5, STD_EXROM },
	{  6, FFE_MAPPER6 },
	{  7, STD_AXROM },
	{  8, FFE_MAPPER8 },
	{  9, STD_PXROM },
	{ 10, STD_FXROM },
	{ 11, DIS_74X377 },
	{ 12, REXSOFT_DBZ5 },
	{ 13, STD_CPROM },
	{ 14, REXSOFT_SL1632 },
	{ 15, WAIXING_PS2 },
	{ 16, BANDAI_LZ93EX },	// with 24c02
	{ 17, FFE_MAPPER17 },
	{ 18, JALECO_SS88006 },
	{ 19, NAMCOT_163 },
	{ 21, KONAMI_VRC4 },	// 4a
	{ 22, KONAMI_VRC2 },	// 2a
	{ 23, KONAMI_VRC2 },	// 2b
	{ 24, KONAMI_VRC6 },	// 6a
	{ 25, KONAMI_VRC4 },	// 4b
	{ 26, KONAMI_VRC6 },	// 6b
	// 27 World Hero
// 28, 29, 30, 31 Unused
	{ 32, IREM_G101 },
	{ 33, TAITO_TC0190FMC },
	{ 34, STD_BXROM },
	{ 35, UNL_SC127 },
// 35 Unused
	{ 36, TXC_STRIKEWOLF },
	{ 37, PAL_ZZ },
	{ 38, DIS_74X161X138 },
	{ 39, UNL_STUDYNGAME },
	{ 40, BTL_SMB2A },
	{ 41, CALTRON_6IN1 },
	{ 42, BTL_MARIOBABY },	// ai senshi nicol?
	{ 43, UNL_SMB2J },
	{ 44, BMC_SUPERBIG_7IN1 },
	{ 45, BMC_HIK8IN1 },
	{ 46, RUMBLESTATION_BOARD },
	{ 47, NES_QJ },
	{ 48, TAITO_TC0190FMCP },
	{ 49, BMC_SUPERHIK_4IN1 },
	{ 50, BTL_SMB2B },
	{ 51, BMC_BALLGAMES_11IN1 },
	{ 52, BMC_MARIOPARTY_7IN1 },
// 53 Supervision 16-in-1
	{ 54, BMC_NOVELDIAMOND },
// 55 Genius SMB
// 56 Kaiser KS202
	{ 57, BMC_GKA },
	{ 58, BMC_GKB },
// 59 Unused
// 60 4-in-1 Reset based
	{ 61, RCM_TETRISFAMILY },
	{ 62, BMC_SUPER_700IN1 },
// 63 CH001 X-in-1
	{ 64, TENGEN_800032 },
	{ 65, IREM_H3001 },
	{ 66, STD_GXROM },
	{ 67, SUNSOFT_3 },
	{ 68, STD_NXROM },
	{ 69, STD_JXROM },
	{ 70, DIS_74X161X161X32 },
	{ 71, CAMERICA_BF9093 },
	{ 72, JALECO_JF17 },
	{ 73, KONAMI_VRC3 },
	{ 74, WAIXING_TYPE_A },
	{ 75, KONAMI_VRC1 },
	{ 76, NAMCOT_3446 },
	{ 77, IREM_LROG017 },
	{ 78, IREM_HOLYDIV },
	{ 79, AVE_NINA06 },
	{ 80, TAITO_X1_005 },
// 81 Unused
	{ 82, TAITO_X1_017 },
	{ 83, CONY_BOARD },
//	{ 84, "Pasofami hacked images?",   NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ 85, KONAMI_VRC7 },
	{ 86, JALECO_JF13 },
	{ 87, DIS_74X139X74 },
	{ 88, NAMCOT_34X3 },
	{ 89, SUNSOFT_2 },
// 90 JY Company Type A
	{ 91, UNL_MK2 },
	{ 92, JALECO_JF19 },
	{ 93, SUNSOFT_2 },
	{ 94, STD_UN1ROM },
	{ 95, NAMCOT_3425 },
	{ 96, BANDAI_OEKAKIDS },
	{ 97, IREM_TAM_S1 },
// 98 Unused
// 99 VS. system
// 100 images hacked to work with nesticle?
// 101 Unused
// 102 Unused
// 103 Bootleg cart 2708
	{ 104, CAMERICA_GOLDENFIVE },
// 105 Nintendo World Championship
	{ 106, BTL_SMB3 },
	{ 107, MAGICSERIES_MD },
// 108 Whirlwind
// 109, 110 Unused
// 111 Ninja Ryuukenden Chinese?
	{ 112, NTDEC_ASDER },
	{ 113, HES_BOARD },
	{ 114, SUPERGAME_LIONKING },
	{ 115, KASING_BOARD },
// 116 Someri Team
	{ 117, FUTUREMEDIA_BOARD },
	{ 118, STD_TXSROM },
	{ 119, STD_TQROM },
// 120 FDS bootleg
	{ 121, KAY_PANDAPRINCE },
// 122 Unused
// 123 K H2288
// 124, 125 Unused
// 126 Powerjoy 84-in-1
// 127, 128, 129, 130, 131 Unused
	{ 132, TXC_22211B },
	{ 133, SACHEN_SA72008 },
	{ 134, BMC_FAMILY_4646B },
// 135 Unused
	{ 136, SACHEN_TCU02 },
	{ 137, SACHEN_8259D },
	{ 138, SACHEN_8259B },
	{ 139, SACHEN_8259C },
	{ 140, JALECO_JF11 },
	{ 141, SACHEN_8259A },
// 142 Kaiser KS7032
	{ 143, SACHEN_TCA01 },
	{ 144, AGCI_50282 },
	{ 145, SACHEN_SA72007 },
	{ 146, AVE_NINA06 }, // basically same as Mapper 79 (Nina006)
	{ 147, SACHEN_TCU01 },
	{ 148, SACHEN_SA0037 },
	{ 149, SACHEN_SA0036 },
	{ 150, SACHEN_74LS374 },
// 151 Konami VS. system
	{ 152, DIS_74X161X161X32 },
	{ 153, BANDAI_LZ93 },
	{ 154, NAMCOT_3453 },
	{ 155, STD_SXROM_A }, // diff compared to MMC1 concern WRAM
	{ 156, OPENCORP_DAOU306 },
	{ 157, BANDAI_DATACH },	// no Datach Reader -> we fall back to mapper 16
	{ 158, TENGEN_800037 },
	{ 159, BANDAI_LZ93EX },	// with 24c01
// 160, 161, 162 Unused
// 163 Nanjing
	{ 164, WAIXING_FFV },
// 165 Waixing SH2
	{ 166, SUBOR_TYPE1 },
	{ 167, SUBOR_TYPE0 },
// 168 Racermate Challenger II
// 169 Unused
// 170 Fujiya
	{ 171, KAISER_KS7058 },
	{ 172, TXC_22211B },
	{ 173, TXC_22211C },
// 174 Unused
// 175 Kaiser KS7022
	{ 176, UNL_XZY },
	{ 177, HENGEDIANZI_BOARD },
	{ 178, WAIXING_SGZLZ },
	{ 179, HENGEDIANZI_XJZB },
	{ 180, UXROM_CC },
// 181 Unused
	{ 182, HOSENKAN_BOARD },
// 183 FDS bootleg
	{ 184, SUNSOFT_1 },
	{ 185, STD_CNROM },
// 186 Fukutake
	{ 187, UNL_KOF96 },
	{ 188, BANDAI_KARAOKE },
	{ 189, TXC_TW },
// 190 Unused
	{ 191, WAIXING_TYPE_B },
	{ 192, WAIXING_TYPE_C },
	{ 193, NTDEC_FIGHTINGHERO },
	{ 194, WAIXING_TYPE_D },
	{ 195, WAIXING_TYPE_E },
	{ 196, BTL_SUPERBROS11 },
	{ 197, UNL_SUPERFIGHTER3 },
	{ 198, WAIXING_TYPE_F },
	{ 199, WAIXING_TYPE_G },
	{ 200, BMC_36IN1 },
	{ 201, BMC_21IN1 },
	{ 202, BMC_150IN1 },
	{ 203, BMC_35IN1 },
	{ 204, BMC_64IN1 },
	{ 205, BMC_15IN1 },
	{ 206, STD_DXROM },
	{ 207, TAITO_X1_005_A },
	{ 208, GOUDER_37017 },
// 209 JY Company Type B
// 210 Some emu uses this as Mapper 19 without some features
// 211 JY Company Type C
	{ 212, BMC_SUPERHIK_300IN1 },
	{ 213, BMC_9999999IN1 },
	{ 214, BMC_SUPERGUN_20IN1 },
	{ 215, SUPERGAME_BOOGERMAN },
	{ 216, RCM_GS2015 },
	{ 217, BMC_GOLDENCARD_6IN1 },
// 218 Unused
// 219 Bootleg a9746
// 220 Summer Carnival '92??
	{ 221, UNL_N625092 },
	{ 222, BTL_DRAGONNINJA },
// 223 Waixing Type I
// 224 Waixing Type J
	{ 225, BMC_72IN1 },
	{ 226, BMC_76IN1 },
	{ 227, BMC_1200IN1 },
	{ 228, ACTENT_ACT52 },
	{ 229, BMC_31IN1 },
	{ 230, BMC_22GAMES },
	{ 231, BMC_20IN1 },
	{ 232, CAMERICA_BF9096 },
// 233 Super 22 Games
// 234 AVE Maxi 15
// 235 Golden Game x-in-1
// 236 Game 800-in-1
// 237 Unused
// 238 Bootleg 6035052
// 239 Unused
	{ 240, CNE_SHLZ },
	{ 241, TXC_MXMDHTWO },
	{ 242, WAIXING_ZS },
	{ 243, SACHEN_74LS374_A },
	{ 244, CNE_DECATHLON },
	{ 245, WAIXING_TYPE_H },
	{ 246, CNE_FSB },
// 247, 248 Unused
	{ 249, WAIXING_SECURITY },
	{ 250, NITRA_TDA },
// 251 Shen Hua Jian Yun III??
	{ 252, WAIXING_SGZ },
// 253 Super 8-in-1 99 King Fighter??
// 254 Pikachu Y2K
	{ 255, BMC_110IN1 },
};

#if 0
// problematic
{  6, "FFE F4xxxx",                mapper6_l_w, NULL, NULL, mapper6_w, NULL, NULL, ffe_irq },
{  8, "FFE F3xxxx",                NULL, NULL, NULL, mapper8_w, NULL, NULL, NULL },
{ 17, "FFE F8xxx",                 mapper17_l_w, NULL, NULL, NULL, NULL, NULL, ffe_irq },
{ 84, "Pasofami hacked images?",   NULL, NULL, NULL, NULL, NULL, NULL, NULL },
#endif

const nes_mmc *nes_mapper_lookup( int mapper )
{
	int i;

	for (i = 0; i < ARRAY_LENGTH(mmc_list); i++)
	{
		if (mmc_list[i].iNesMapper == mapper)
			return &mmc_list[i];
	}

	return NULL;
}

int nes_get_mmc_id( running_machine *machine, int mapper )
{
	const nes_mmc *mmc = nes_mapper_lookup(mapper);
	
	if (mmc == NULL)
		fatalerror("Unimplemented Mapper %d", mapper);
	
	return mmc->pcb_id;
}

/*************************************************************

 mapper_initialize

 Initialize various constants needed by mappers, depending
 on the mapper number. As long as the code is mainly iNES
 centric, also UNIF images will call this function to
 initialize their bankswitch and registers.

 *************************************************************/
#if 0
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
			mmc1_set_chr(machine);
			mmc1_set_prg(machine);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
				fjump2_set_prg(machine);
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
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
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
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
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
#endif
