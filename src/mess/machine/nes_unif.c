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
	NT_HORZ,
	NT_VERT,
	NT_Y,
	NT_1SCREEN,
	NT_4SCR_2K,
	NT_4SCR_4K
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
	{ "UNL-603-5052",               0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD}, // mapper 238?
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
	{ "BMC-NTD-03",                 0,    0, CHRRAM_0,  NT_X,  UNSUPPORTED_BOARD}
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
	int err = 0, i;
	
	switch (idx)
	{
		case STD_NROM:	// mapper 0
		case HVC_FAMBASIC:
		case GG_NROM:
			prg32(machine, 0);
			break;
		case STD_UXROM:	// mapper 2
		case STD_UN1ROM:	// mapper 94
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case UXROM_CC:	// mapper 180
			prg32(machine, 0);
			break;
		case STD_CNROM:	// mapper 3, 185
		case BANDAI_PT554:
		case TENGEN_800008:
			prg32(machine, 0);
			break;
		case STD_CPROM:	// mapper 13
			chr4_0(machine, 0, CHRRAM);
			chr4_4(machine, 0, CHRRAM);
			prg32(machine, 0);
			break;
		case STD_AXROM:	// mapper 7
			set_nt_mirroring(machine, PPU_MIRROR_LOW);
			prg32(machine, 0);
			break;
		case STD_BXROM:	// mapper 34
			prg32(machine, 0);
			break;
		case STD_GXROM:	// mapper 66
		case STD_MXROM:
			prg32(machine, 0);
			break;
		case STD_SXROM:	// mapper 1, 155
		case STD_SOROM:
		case STD_SXROM_A:
		case STD_SOROM_A:
			state->mmc_latch1 = 0;
			state->mmc_count = 0;
			state->mmc_reg[0] = 0x0f;
			state->mmc_reg[1] = state->mmc_reg[2] = state->mmc_reg[3] = 0;
			state->mmc1_reg_write_enable = 1;
			set_nt_mirroring(machine, PPU_MIRROR_HORZ);
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc1_set_chr(machine);
			mmc1_set_prg(machine);
			wram_bank(machine, 0, (idx == STD_SOROM) ? NES_WRAM : NES_BATTERY);
			break;
		case STD_PXROM:	// mapper 9
			state->MMC2_regs[0] = state->MMC2_regs[2] = 0;
			state->MMC2_regs[1] = state->MMC2_regs[3] = 0;
			state->mmc_latch1 = state->mmc_latch2 = 0xfe;
			prg8_89(machine, 0);
			prg8_ab(machine, (state->prg_chunks << 1) - 3);
			prg8_cd(machine, (state->prg_chunks << 1) - 2);
			prg8_ef(machine, (state->prg_chunks << 1) - 1);
			break;			
		case STD_FXROM: // mapper 10
			state->MMC2_regs[0] = state->MMC2_regs[2] = 0;
			state->MMC2_regs[1] = state->MMC2_regs[3] = 0;
			state->mmc_latch1 = state->mmc_latch2 = 0xfe;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case STD_TXROM:	// mapper 4
		case STD_TVROM:
		case STD_TKROM:
		case REXSOFT_DBZ5:	// mapper 12
		case WAIXING_TYPE_A:	// mapper 74
		case STD_TXSROM:	// mapper 118
		case STD_TQROM:	// mapper 119
		case WAIXING_TYPE_B:	// mapper 191
		case WAIXING_TYPE_C:	// mapper 192
		case WAIXING_TYPE_D:	// mapper 194
		case WAIXING_TYPE_E:	// mapper 195
		case WAIXING_TYPE_H:	// mapper 245
		case BTL_SUPERBROS11:	// mapper 196
		case NITRA_TDA:	// mapper 250
			if (state->four_screen_vram)	// only TXROM and DXROM have 4-screen mirroring
			{
				set_nt_page(machine, 0, CART_NTRAM, 0, 1);
				set_nt_page(machine, 1, CART_NTRAM, 1, 1);
				set_nt_page(machine, 2, CART_NTRAM, 2, 1);
				set_nt_page(machine, 3, CART_NTRAM, 3, 1);
			}
			state->mmc3_alt_irq = 0;	// in fact, later MMC3 boards seem to use MMC6-type IRQ... more investigations are in progress at NESDev...
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;	// prg_bank[2] & prg_bank[3] remain always the same in most MMC3 variants
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;	// but some pirate clone mappers change them after writing certain registers
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;	// these could be init'ed as xxx_chunks-1 and they would work the same
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case STD_HKROM:	// MMC6 (basically the same as TxROM, but alt IRQ behaviour)
			state->mmc3_alt_irq = 0;
			state->mmc6_reg = 0xf0;
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;	// prg_bank[2] & prg_bank[3] remain always the same in most MMC3 variants
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;	// but some pirate clone mappers change them after writing certain registers
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;	// these could be init'ed as xxx_chunks-1 and they would work the same
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;			
		case PAL_ZZ:	// mapper 37
			state->mmc3_alt_irq = 0;
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = 0x07;
			state->mmc_chr_mask = 0x7f;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case NES_QJ:	// mapper 47
			state->mmc3_alt_irq = 0;
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = 0x0f;
			state->mmc_chr_mask = 0x7f;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
		case STD_EXROM:	// mapper 5
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
		case STD_NXROM:		// mapper 68
		case SUNSOFT_DCS:		// mapper 68
			state->mmc_reg[0] = state->mmc_latch1 = state->mmc_latch2 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case STD_JXROM:		// mapper 69
		case SUNSOFT_5B:
		case SUNSOFT_FME7:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case NAMCOT_34X3:	// mapper 88
		case STD_DXROM:	// mapper 206
		case STD_DRROM:
			if (state->four_screen_vram)	// only TXROM and DXROM have 4-screen mirroring
			{
				set_nt_page(machine, 0, CART_NTRAM, 0, 1);
				set_nt_page(machine, 1, CART_NTRAM, 1, 1);
				set_nt_page(machine, 2, CART_NTRAM, 2, 1);
				set_nt_page(machine, 3, CART_NTRAM, 3, 1);
			}
		case NAMCOT_3453:	// mapper 154
			prg16_89ab(machine, state->prg_chunks - 2);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case NAMCOT_3446:	// mapper 76
			prg8_89(machine, 0);
			prg8_ab(machine, 1);
			prg16_cdef(machine, state->prg_chunks - 1);
			chr2_0(machine, 0, CHRROM);
			chr2_2(machine, 1, CHRROM);
			chr2_4(machine, 2, CHRROM);
			chr2_6(machine, 3, CHRROM);
			state->mmc_latch1 = 0;
			break;
		case NAMCOT_3425:	// mapper 95
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case DIS_74X377:	// mapper 11
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;	// NINA-07 has no CHRROM
			prg32(machine, 0);
			break;
		case DIS_74X139X74:	// mapper 87
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case DIS_74X161X138:	// mapper 38
			prg32(machine, 0);
			break;
		case DIS_74X161X161X32:	// mapper 152 & 70
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			if (state->chr_chunks)
				chr8(machine, 0, CHRROM);
			break;
		case BANDAI_LZ93:	// mapper 16, 157
		case BANDAI_LZ93EX:	// same + EEPROM
		case BANDAI_DATACH:
			state->mmc_latch1 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			break;
		case BANDAI_JUMP2:	// mapper 153
			for (i = 0; i < 8; i++)
				state->mmc_reg[i] = 0;
			state->mmc_latch1 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			fjump2_set_prg(machine);
			break;
		case BANDAI_FCG:
			state->mmc_latch1 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			break;
		case BANDAI_OEKAKIDS:	// mapper 96
			prg32(machine, 0);
			break;
		case BANDAI_KARAOKE:	// mapper 188
			prg16_89ab(machine, 0);
			prg16_cdef(machine, (state->prg_chunks - 1) ^ 0x08);
			break;
		case IREM_LROG017:	// mapper 77
			prg32(machine, 0);
			chr2_2(machine, 0, CHRROM);
			chr2_4(machine, 1, CHRROM);
			chr2_6(machine, 2, CHRROM);
			break;
		case IREM_HOLYDIV:	// mapper 78
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case IREM_TAM_S1:	// mapper 97
			prg16_89ab(machine, state->prg_chunks - 1);
			prg16_cdef(machine, 0);
			break;
		case IREM_G101:
			state->mmc_latch1 = 0;
			prg16_89ab(machine, 0);
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case IREM_H3001:	// mapper 65
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case JALECO_SS88006:	// mapper 18
			state->mmc_latch1 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			break;
		case JALECO_JF11:	// mapper 140
		case JALECO_JF13:	// mapper 86
		case JALECO_JF19:	// mapper 92
			prg32(machine, 0);
			break;
		case JALECO_JF16:	// mapper 78
		case JALECO_JF17:	// mapper 72
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case KONAMI_VRC1:	// mapper 75
		case KONAMI_VRC2:
		case KONAMI_VRC3:	// mapper 73
		case KONAMI_VRC4:
		case KONAMI_VRC6:
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case KONAMI_VRC7:	// mapper 85
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg8_89(machine, 0);
			prg8_ab(machine, 0);
			prg8_cd(machine, 0);
			prg8_ef(machine, 0xff);
			break;
		case NAMCOT_163:	// mapper 19
			state->mmc_latch1 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
		case SUNSOFT_1:	// mapper 184
		case SUNSOFT_2:	// mapper 89 & 93
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			if (!state->hard_mirroring)
				set_nt_mirroring(machine, PPU_MIRROR_LOW);
			break;
		case SUNSOFT_3:	// mapper 67
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
		case TAITO_TC0190FMC:	// mapper 33
		case TAITO_TC0190FMCP:	// mapper 48
		case TAITO_X1_005:	// mapper 80
		case TAITO_X1_005_A:	// mapper 207
		case TAITO_X1_017:	// mapper 82
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
// mapper 14
		case REXSOFT_SL1632:
			state->mmc_extra_bank[2] = 0xfe;
			state->mmc_extra_bank[3] = 0xff;
			state->mmc_extra_bank[0] = state->mmc_extra_bank[1] = state->mmc_extra_bank[4] = state->mmc_extra_bank[5] = state->mmc_extra_bank[6] = 0;
			state->mmc_extra_bank[7] = state->mmc_extra_bank[8] = state->mmc_extra_bank[9] = state->mmc_extra_bank[0xa] = state->mmc_extra_bank[0xb] = 0;
			state->map14_reg[0] = state->map14_reg[1] = 0;
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 15
		case WAIXING_PS2:
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			prg32(machine, 0);
			break;

// mapper 35
		case UNL_SC127:
			prg32(machine, 0xff);
			break;

// mapper 36
		case TXC_STRIKEWOLF:
			break;
// mapper 40
		case BTL_SMB2A:
			prg8_67(machine, 0xfe);
			prg8_89(machine, 0xfc);
			prg8_ab(machine, 0xfd);
			prg8_cd(machine, 0xfe);
			prg8_ef(machine, 0xff);
			break;
// mapper 41
		case CALTRON_6IN1:
			state->mmc_latch1 = 0;
			prg32(machine, 0);
			break;
// mapper 42
		case BTL_MARIOBABY:
		case BTL_AISENSHINICOL:
			prg32(machine, state->prg_chunks - 1);
			break;
// mapper 43
		case UNL_SMB2J:
			prg32(machine, 0);
			if (state->battery)
				memset(state->battery_ram, 0x2000, 0xff);
			else if (state->prg_ram)
				memset(state->wram, 0x2000, 0xff);
			break;
// mapper 44
		case BMC_SUPERBIG_7IN1:
// mapper 49
		case BMC_SUPERHIK_4IN1:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = 0x0f;
			state->mmc_chr_mask = 0x7f;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 45
		case BMC_HIK8IN1:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			state->mapper45_reg[0] = state->mapper45_reg[1] = state->mapper45_reg[2] = state->mapper45_reg[3] = 0;
			state->mmc_prg_base = 0;
			state->mmc_prg_mask = 0x3f;
			state->mmc_chr_base = 0;
			state->mmc_chr_mask = 0xff;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 46
		case RUMBLESTATION_BOARD:
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
		case BTL_SMB2B:
			mapper_initialize(machine, 50);
			break;
// mapper 51
		case BMC_BALLGAMES_11IN1:
			state->mapper51_reg[0] = 0x01;
			state->mapper51_reg[1] = 0x00;
			bmc_ball11_set_banks(machine);
			break;
// mapper 52
		case BMC_MARIOPARTY_7IN1:
			state->mmc_prg_bank[0] = 0xfe;
			state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = 0xff;
			state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->map52_reg_written = 0;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			state->mmc_prg_base = 0;
			state->mmc_prg_mask = 0x1f;
			state->mmc_chr_base = 0;
			state->mmc_chr_mask = 0xff;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 54
		case BMC_NOVELDIAMOND:
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
// mapper 57
		case BMC_GKA:
			state->mmc_latch1 = 0x00;
			state->mmc_latch2 = 0x00;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
// mapper 58
		case BMC_GKB:
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg32(machine, 0);
			chr8(machine, 0, state->mmc_chr_source);
			break;
// mapper 61
		case RCM_TETRISFAMILY:
			prg32(machine, 0);
			break;
// mapper 62
		case BMC_SUPER_700IN1:
			prg32(machine, 0);
			break;
// mapper 64
		case TENGEN_800032:
// mapper 158
		case TENGEN_800037:
			state->mmc_latch1 = 0;
			prg16_89ab(machine, state->prg_chunks - 1);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
// mapper 71
		case CAMERICA_BF9097:
			set_nt_mirroring(machine, PPU_MIRROR_HORZ);
		case CAMERICA_BF9093:
			prg32(machine, 0xff);
			break;

// mapper 79 (& 146)
		case AVE_NINA06:
			set_nt_mirroring(machine, PPU_MIRROR_HORZ);
			chr8(machine, 0, CHRROM);
			prg32(machine, 0);
			break;

// mapper 83
		case CONY_BOARD:
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			state->mapper83_reg[9] = 0x0f;
			prg8_cd(machine, 0x1e);
			prg8_ef(machine, 0x1f);
			break;

// mapper 91
		case UNL_MK2:
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			prg16_89ab(machine, state->prg_chunks - 1);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;

// mapper 104
		case CAMERICA_GOLDENFIVE:
			prg16_89ab(machine, 0x00);
			prg16_cdef(machine, 0x0f);
			break;
// mapper 106
		case BTL_SMB3:
			prg8_89(machine, (state->prg_chunks << 1) - 1);
			prg8_ab(machine, 0);
			prg8_cd(machine, 0);
			prg8_ef(machine, (state->prg_chunks << 1) - 1);
			break;
// mapper 107
		case MAGICSERIES_MD:
			prg32(machine, 0);
			break;
// mapper 112
		case NTDEC_ASDER:
			state->mmc_latch1 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
// mapper 113
		case HES_BOARD:
		case HES6IN1_BOARD:
			prg32(machine, 0);
			break;
// mapper 114
		case SUPERGAME_LIONKING:
			state->map114_reg = state->map114_reg_enabled = 0;
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 115
		case KASING_BOARD:
			state->mapper115_reg[0] = 0;
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 116
//		case SOMERITEAM_SL12:
//			mapper_initialize(machine, 116);
//			break;
// mapper 117
		case FUTUREMEDIA_BOARD:
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
// mapper 121
		case KAY_PANDAPRINCE:
#if 0
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mapper121_reg[0] = state->mapper121_reg[1] = state->mapper121_reg[2] = 0;
			state->mapper121_reg[3] = 0;
			state->mapper121_reg[4] = state->mapper121_reg[5] = state->mapper121_reg[6] = 0;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			kay_pp_update_reg(machine);
			kay_pp_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			kay_pp_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
#endif
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mapper121_reg[0] = state->mapper121_reg[1] = state->mapper121_reg[2] = 0;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 132
		case TXC_22211A:
// mapper 172
		case TXC_22211B:
// mapper 173
		case TXC_22211C:
			state->txc_reg[0] = state->txc_reg[1] = state->txc_reg[2] = state->txc_reg[3] = 0;
			prg32(machine, 0);
			break;
// mapper 133
		case SACHEN_SA72008:
// mapper 145
		case SACHEN_SA72007:
// mapper 147
		case SACHEN_TCU01:
// mapper 148
		case SACHEN_SA0037:
// mapper 149
		case SACHEN_SA0036:
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
// mapper 134
		case BMC_FAMILY_4646B:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = 0x1f;
			state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 136
		case SACHEN_TCU02:
			state->mmc_latch1 = 0;
			prg32(machine, 0);
			break;
// mapper 137
		case SACHEN_8259D:
			state->mmc_latch1 = 0;
			prg32(machine, 0);
			chr8(machine, state->chr_chunks - 1, CHRROM);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
// mapper 138
		case SACHEN_8259B:
// mapper 139
		case SACHEN_8259C:
// mapper 141
		case SACHEN_8259A:
			state->mmc_latch1 = 0;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg32(machine, 0);
			chr8(machine, 0, state->mmc_chr_source);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
// mapper 143
		case SACHEN_TCA01:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 1);
			chr8(machine, 0, CHRROM);
			break;
// mapper 144
		case AGCI_50282:
			prg32(machine, 0);
			break;
// mapper 150
		case SACHEN_74LS374:
			state->mmc_latch1 = 0;
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
// mapper 156
		case OPENCORP_DAOU306:
			prg16_89ab(machine, state->prg_chunks - 2);
			prg16_cdef(machine, state->prg_chunks - 1);
			set_nt_mirroring(machine, PPU_MIRROR_LOW);
			break;
// mapper 164
		case WAIXING_FFV:
			prg32(machine, 0xff);
			state->mid_ram_enable = 1;
			break;
// mapper 166
		case SUBOR_TYPE1:
			state->subor_reg[0] = state->subor_reg[1] = state->subor_reg[2] = state->subor_reg[3] = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0x07);
			break;
// mapper 167
		case SUBOR_TYPE0:
			state->subor_reg[0] = state->subor_reg[1] = state->subor_reg[2] = state->subor_reg[3] = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0x20);
			break;
// mapper 171
		case KAISER_KS7058:
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
// mapper 176
		case UNL_XZY:
			prg32(machine, (state->prg_chunks - 1) >> 1);
			break;
// mapper 177
		case HENGEDIANZI_BOARD:
			prg32(machine, 0);
			break;
// mapper 178
		case WAIXING_SGZLZ:
			state->mmc_latch1 = 0;
			prg32(machine, 0);
			break;
// mapper 179
		case HENGEDIANZI_XJZB:
			prg32(machine, 0);
			break;
// mapper 182
		case HOSENKAN_BOARD:
			prg32(machine, (state->prg_chunks - 1) >> 1);
			break;

// mapper 187
		case UNL_KOF96:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mapper187_reg[0] = state->mapper187_reg[1] = state->mapper187_reg[2] = state->mapper187_reg[3] = 0;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			kof96_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			kof96_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 189
		case TXC_TW:
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_chr_base = 0;
			state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg32(machine, 0);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 193
		case NTDEC_FIGHTINGHERO:
			prg32(machine, (state->prg_chunks - 1) >> 1);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
// mapper 197
		case UNL_SUPERFIGHTER3:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			unl_sf3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 198
		case WAIXING_TYPE_F:
			state->mmc_prg_bank[0] = 0x00;
			state->mmc_prg_bank[1] = 0x01;
			state->mmc_prg_bank[2] = 0x4e;
			state->mmc_prg_bank[3] = 0x4f;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			waixing_f_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 199
		case WAIXING_TYPE_G:
			state->mmc_prg_bank[0] = 0x00;
			state->mmc_prg_bank[1] = 0x01;
			state->mmc_prg_bank[2] = 0x3e;
			state->mmc_prg_bank[3] = 0x3f;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			waixing_g_set_chr(machine, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 200
		case BMC_36IN1:
			prg16_89ab(machine, state->prg_chunks - 1);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
// mapper 201
		case BMC_21IN1:
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
// mapper 202
		case BMC_150IN1:
// mapper 203
		case BMC_35IN1:
// mapper 204
		case BMC_64IN1:
// mapper 214
		case BMC_SUPERGUN_20IN1:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
// mapper 205
		case BMC_15IN1:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = 0x10;
			state->mmc_prg_mask = 0x1f;
			state->mmc_chr_base = 0;
			state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;

// mapper 208
		case GOUDER_37017:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->map208_reg[0] = state->map208_reg[1] = state->map208_reg[2] = state->map208_reg[3] = state->map208_reg[4] = 0;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			mmc3_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			mmc3_set_chr(machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 212
		case BMC_SUPERHIK_300IN1:
			chr8(machine, 0xff, CHRROM);
			prg32(machine, 0xff);
			break;
// mapper 213
		case BMC_9999999IN1:
			prg32(machine, 0);
			chr8(machine, 0, CHRROM);
			break;
// mapper 215
		case SUPERGAME_BOOGERMAN:
			state->mmc_prg_bank[0] = 0x00;
			state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = 0x01;
			state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
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
// mapper 216
		case RCM_GS2015:
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg32(machine, 0);
			chr8(machine, 0, state->mmc_chr_source);
			break;
// mapper 217
		case BMC_GOLDENCARD_6IN1:
			state->map217_reg[0] = 0x00;
			state->map217_reg[1] = 0xff;
			state->map217_reg[2] = 0x03;
			state->map217_reg[3] = 0;
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			bmc_gc6in1_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			bmc_gc6in1_set_chr(machine, state->mmc_chr_source);
			break;
// mapper 221
		case UNL_N625092:
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0);
			break;
// mapper 222
		case BTL_DRAGONNINJA:
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
// mapper 225
		case BMC_72IN1:
			prg32(machine, 0);
			break;
// mapper 226
		case BMC_76IN1:
		case BMC_SUPER_42IN1:
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0;
			prg32(machine, 0);
			break;
// mapper 227
		case BMC_1200IN1:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 0);
			break;
// mapper 228
		case ACTENT_ACT52:
			chr8(machine, 0, CHRROM);
			prg32(machine, 0);
			break;
// mapper 229
		case BMC_31IN1:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 1);
			chr8(machine, 0, CHRROM);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
// mapper 230
		case BMC_22GAMES:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 7);
			break;
// mapper 231
		case BMC_20IN1:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
// mapper 232
		case CAMERICA_BF9096:
			state->mmc_latch1 = 0x18;
			state->mmc_latch2 = 0x00;
			bf9096_set_prg(machine);
			break;
// mapper 240
		case CNE_SHLZ:
			prg32(machine, 0);
			break;
// mapper 241
		case TXC_MXMDHTWO:
			prg32(machine, 0);
			break;
// mapper 242
		case WAIXING_ZS:
		case WAIXING_DQ8:
			prg32(machine, 0);
			break;
// mapper 243
		case SACHEN_74LS374_A:
			state->mmc_vrom_bank[0] = 3;
			state->mmc_latch1 = 0;
			chr8(machine, 3, CHRROM);
			prg32(machine, 0);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;
// mapper 244
		case CNE_DECATHLON:
			prg32(machine, 0);
			break;
// mapper 246
		case CNE_FSB:
			prg32(machine, 0xff);
			break;
// mapper 249
		case WAIXING_SECURITY:
			state->mmc_prg_bank[0] = state->mmc_prg_bank[2] = 0xfe;
			state->mmc_prg_bank[1] = state->mmc_prg_bank[3] = 0xff;
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			state->map249_reg = 0;
			state->mmc_prg_base = state->mmc_chr_base = 0;
			state->mmc_prg_mask = state->mmc_chr_mask = 0xff;
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			waixing_sec_set_prg(machine, state->mmc_prg_base, state->mmc_prg_mask);
			waixing_sec_set_chr(machine, state->mmc_chr_base, state->mmc_chr_mask);
			break;
// mapper 252
		case WAIXING_SGZ:
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
			break;
// mapper 255
		case BMC_110IN1:
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 1);
			chr8(machine, 0, CHRROM);
			set_nt_mirroring(machine, PPU_MIRROR_VERT);
			break;

// UNIF only			
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
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
			bmc_s24in1sc03_set_prg(machine);
			bmc_s24in1sc03_set_chr(machine);
			break;
		case BMC_T262:
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0;
			prg16_89ab(machine, 0);
			prg16_cdef(machine, 7);
			break;
		case BMC_WS:
			state->mmc_latch1 = 0;
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
			state->mmc_latch1 = 0;
			state->mmc_latch2 = 0x80;
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
		case UNL_RACERMATE:
			state->mmc_chr_source = state->chr_chunks ? CHRROM : CHRRAM;
			chr4_0(machine, 0, state->mmc_chr_source);
			chr4_4(machine, 0, state->mmc_chr_source);
			prg16_89ab(machine, 0);
			prg16_cdef(machine, state->prg_chunks - 1);
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
	state->battery = unif_board->nvwram;	// we should implement battery banks based on the size of this...
	state->battery_size = NES_BATTERY_SIZE; // FIXME: we should allow for smaller battery!
	state->prg_ram = unif_board->wram;	// we should implement WRAM banks based on the size of this...
	state->four_screen_vram = (unif_board->nt == NT_4SCR_2K || unif_board->nt == NT_4SCR_4K) ? 1 : 0;

	if (unif_board->nt == NT_VERT)
		state->hard_mirroring = PPU_MIRROR_VERT;
	else if (unif_board->nt == NT_HORZ)
		state->hard_mirroring = PPU_MIRROR_HORZ;

	if (unif_board->chrram <= CHRRAM_8)
		state->vram_chunks = 1;
	else if (unif_board->chrram == CHRRAM_16)
		state->vram_chunks = 2;
	else if (unif_board->chrram == CHRRAM_32)
		state->vram_chunks = 4;
}
