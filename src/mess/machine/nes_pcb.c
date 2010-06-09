/*****************************************************************************************

    NES Cart PCBs Emulation

****************************************************************************************/

typedef struct _nes_pcb  nes_pcb;
struct _nes_pcb
{
	const char              *pcb_name;
	int                     pcb_id;
};


// Here, we take the feature attribute from .xml (i.e. the PCB name) and we assign a unique ID to it
static const nes_pcb pcb_list[] =
{
	/* Nintendo HROM, NROM, RROM, SROM & STROM */
	{ "HVC-HROM",         STD_NROM },
	{ "HVC-NROM",         STD_NROM },
	{ "HVC-NROM-128",     STD_NROM },
	{ "HVC-NROM-256",     STD_NROM },
	{ "HVC-RROM",         STD_NROM },
	{ "HVC-RROM-128",     STD_NROM },
	{ "HVC-SROM",         STD_NROM },
	{ "HVC-STROM",        STD_NROM },
	{ "NES-HROM",         STD_NROM },
	{ "NES-NROM",         STD_NROM },
	{ "NES-NROM-128",     STD_NROM },
	{ "NES-NROM-256",     STD_NROM },
	{ "NES-RROM",         STD_NROM },
	{ "NES-RROM-128",     STD_NROM },
	{ "NES-SROM",         STD_NROM },
	{ "NES-STROM",        STD_NROM },
	/* "No mapper" boards by other manufacturer */
	{ "IREM-NROM-128",    STD_NROM },
	{ "IREM-NROM-256",    STD_NROM },
	{ "BANDAI-NROM-128",  STD_NROM },
	{ "BANDAI-NROM-256",  STD_NROM },
	{ "KONAMI-NROM-128",  STD_NROM },
	{ "SETA-NROM-128",    STD_NROM },
	{ "SUNSOFT-NROM-256", STD_NROM },
	{ "TAITO-NROM-128",   STD_NROM },
	{ "TAITO-NROM-256",   STD_NROM },
	{ "NAMCOT-3301",      STD_NROM },
	{ "NAMCOT-3302",      STD_NROM },
	{ "NAMCOT-3303",      STD_NROM },
	{ "NAMCOT-3305",      STD_NROM },
	{ "NAMCOT-3311",      STD_NROM },
	{ "NAMCOT-3411",      STD_NROM },
	{ "JALECO-JF-01",     STD_NROM },
	{ "JALECO-JF-02",     STD_NROM },
	{ "JALECO-JF-03",     STD_NROM },
	{ "JALECO-JF-04",     STD_NROM },
	{ "TENGEN-800003",    STD_NROM },
	/* Nintendo Family BASIC pcb (NROM + 2K or 4K WRAM) */
	{ "HVC-FAMILYBASIC",  HVC_FAMBASIC },
	/* Game Genie */
	{ "CAMERICA-GAMEGENIE", GG_NROM },
//
	/* Nintendo UNROM/UOROM */
	{ "HVC-UNROM",        STD_UXROM },
	{ "HVC-UOROM",        STD_UXROM },
	{ "NES-UNROM",        STD_UXROM },
	{ "NES-UOROM",        STD_UXROM },
	/* UxROM boards by other manufacturer */
	{ "IREM-UNROM",       STD_UXROM },
	{ "KONAMI-UNROM",     STD_UXROM },
	{ "TAITO-UNROM",      STD_UXROM },
	{ "JALECO-JF-15",     STD_UXROM },
	{ "JALECO-JF-18",     STD_UXROM },
	{ "JALECO-JF-39",     STD_UXROM },
	{ "UNL-UXROM",        STD_UXROM }, // e.g. mapper 2 mod (see Korean Igo)
//
	/* Nintendo CNROM */
	{ "HVC-CNROM",        STD_CNROM },
	{ "NES-CNROM",        STD_CNROM },
	/* CxROM boards by other manufacturer */
	{ "BANDAI-CNROM",     STD_CNROM },
	{ "KONAMI-CNROM",     STD_CNROM },
	{ "TAITO-CNROM",      STD_CNROM },
	{ "AVE-74*161",       STD_CNROM },
	{ "NTDEC-N715062",    STD_CNROM },
	{ "SACHEN-CNROM",     STD_CNROM },
	/* Bandai Aerobics Studio (CNROM boards + special audio chip) */
	{ "BANDAI-PT-554",    BANDAI_PT554 },
	/* FIXME: Is this the same as mapper 3? */
	{ "TENGEN-800008",    TENGEN_800008 },
//
	/* Nintendo AxROM */
	{ "HVC-AMROM",        STD_AXROM },
	{ "HVC-AN1ROM",       STD_AXROM },
	{ "HVC-ANROM",        STD_AXROM },
	{ "HVC-AOROM",        STD_AXROM },
	{ "NES-AMROM",        STD_AXROM },
	{ "NES-AN1ROM",       STD_AXROM },
	{ "NES-ANROM",        STD_AXROM },
	{ "NES-AOROM",        STD_AXROM },
	/* AxROM boards by other manufacturer */
	{ "ACCLAIM-AOROM",    STD_AXROM },
//
	/* Nintendo PxROM */
	{ "HVC-PEEOROM",      STD_PXROM },
	{ "HVC-PNROM",        STD_PXROM },
	{ "NES-PEEOROM",      STD_PXROM },
	{ "NES-PNROM",        STD_PXROM },
//
	/* Nintendo FxROM */
	{ "HVC-FJROM",        STD_FXROM },
	{ "HVC-FKROM",        STD_FXROM },
	{ "NES-FJROM",        STD_FXROM },
	{ "NES-FKROM",        STD_FXROM },
//
	/* Nintendo BXROM */
	{ "HVC-BNROM",        STD_BXROM },
	{ "NES-BNROM",        STD_BXROM },
	/* BxROM boards by other manufacturer */
	{ "IREM-BNROM",       STD_BXROM },
//
	/* Nintendo CPROM */
	{ "HVC-CPROM",        STD_CPROM },
	{ "NES-CPROM",        STD_CPROM },
//
	/* Nintendo GNROM & MHROM */
	{ "HVC-GNROM",        STD_GXROM },
	{ "NES-GNROM",        STD_GXROM },
	{ "HVC-MHROM",        STD_MXROM },
	{ "NES-MHROM",        STD_MXROM },
	{ "PAL-MH",           STD_MXROM },
	/* GxROM boards by other manufacturer */
	{ "BANDAI-GNROM",     STD_GXROM },
//
	/* Nintendo NxROM */
	{ "HVC-NTBROM",       STD_NXROM },
	{ "NES-NTBROM",       STD_NXROM },
	/* NxROM boards by other manufacturer (this board was mainly used by Sunsoft?) */
	{ "SUNSOFT-4",        STD_NXROM },
	{ "TENGEN-800042",    STD_NXROM },
//
	/* Nintendo JxROM */
	{ "HVC-JLROM",        STD_JXROM },
	{ "HVC-JSROM",        STD_JXROM },
	{ "NES-JLROM",        STD_JXROM },
	{ "NES-JSROM",        STD_JXROM },
	{ "NES-BTR",          STD_JXROM },
	/* JxROM boards by other manufacturer (this board was mainly used by Sunsoft?) */
	{ "SUNSOFT-5B",       STD_JXROM },
	{ "SUNSOFT-FME-7",    STD_JXROM },
//
	/* Nintendo UN1sROM */
	{ "HVC-UN1ROM",       STD_UN1ROM },
	{ "NES-UN1ROM",       STD_UN1ROM },
//
	/* Nintendo SxROM */
	{ "HVC-SAROM",        STD_SXROM },
	{ "HVC-SBROM",        STD_SXROM },
	{ "HVC-SC1ROM",       STD_SXROM },
	{ "HVC-SCROM",        STD_SXROM },
	{ "HVC-SEROM",        STD_SXROM },
	{ "HVC-SF1ROM",       STD_SXROM },
	{ "HVC-SFROM",        STD_SXROM },
	{ "HVC-SGROM",        STD_SXROM },
	{ "HVC-SH1ROM",       STD_SXROM },
	{ "HVC-SHROM",        STD_SXROM },
	{ "HVC-SJROM",        STD_SXROM },
	{ "HVC-SKROM",        STD_SXROM },
	{ "HVC-SL1ROM",       STD_SXROM },
	{ "HVC-SL2ROM",       STD_SXROM },
	{ "HVC-SL3ROM",       STD_SXROM },
	{ "HVC-SLROM",        STD_SXROM },
	{ "HVC-SLRROM",       STD_SXROM },
	{ "HVC-SNROM",        STD_SXROM },
	{ "HVC-SUROM",        STD_SXROM },
	{ "HVC-SXROM",        STD_SXROM },
	{ "NES-SAROM",        STD_SXROM },
	{ "NES-SBROM",        STD_SXROM },
	{ "NES-SC1ROM",       STD_SXROM },
	{ "NES-SCROM",        STD_SXROM },
	{ "NES-SEROM",        STD_SXROM },
	{ "NES-SF1ROM",       STD_SXROM },
	{ "NES-SFROM",        STD_SXROM },
	{ "NES-SGROM",        STD_SXROM },
	{ "NES-SH1ROM",       STD_SXROM },
	{ "NES-SHROM",        STD_SXROM },
	{ "NES-SJROM",        STD_SXROM },
	{ "NES-SKROM",        STD_SXROM },
	{ "NES-SL1ROM",       STD_SXROM },
	{ "NES-SL2ROM",       STD_SXROM },
	{ "NES-SL3ROM",       STD_SXROM },
	{ "NES-SLROM",        STD_SXROM },
	{ "NES-SLRROM",       STD_SXROM },
	{ "NES-SNROM",        STD_SXROM },
	{ "NES-SUROM",        STD_SXROM },
	{ "NES-SXROM",        STD_SXROM },
	{ "NES-WH",           STD_SXROM },
	{ "NES-SOROM",        STD_SOROM },
	{ "HVC-SOROM",        STD_SOROM },
	/* SxROM boards by other manufacturer */
	{ "KONAMI-SLROM",     STD_SXROM },
	{ "VIRGIN-SNROM",     STD_SXROM },
	/* FIXME: Made up board for 2 MMC1A games (i.e. no WRAM disable bit) */
	{ "HVC-SJROM-A",      STD_SXROM_A },
	{ "HVC-SKROM-A",      STD_SXROM_A },
//
	/* Nintendo TxROM */
	{ "HVC-TBROM",        STD_TXROM },
	{ "HVC-TEROM",        STD_TXROM },
	{ "HVC-TFROM",        STD_TXROM },
	{ "HVC-TGROM",        STD_TXROM },
	{ "HVC-TKROM",        STD_TXROM },
	{ "HVC-TL1ROM",       STD_TXROM },
	{ "HVC-TL2ROM",       STD_TXROM },
	{ "HVC-TLROM",        STD_TXROM },
	{ "HVC-TNROM",        STD_TXROM },
	{ "HVC-TR1ROM",       STD_TXROM },
	{ "HVC-TSROM",        STD_TXROM },
	{ "HVC-TVROM",        STD_TXROM },
	{ "NES-B4",           STD_TXROM },
	{ "NES-TBROM",        STD_TXROM },
	{ "NES-TEROM",        STD_TXROM },
	{ "NES-TFROM",        STD_TXROM },
	{ "NES-TGROM",        STD_TXROM },
	{ "NES-TKROM",        STD_TXROM },
	{ "NES-TL1ROM",       STD_TXROM },
	{ "NES-TL2ROM",       STD_TXROM },
	{ "NES-TLROM",        STD_TXROM },
	{ "NES-TNROM",        STD_TXROM },
	{ "NES-TR1ROM",       STD_TXROM },
	{ "NES-TSROM",        STD_TXROM },
	{ "NES-TVROM",        STD_TXROM },
	/* TxROM boards by other manufacturer */
	{ "ACCLAIM-MC-ACC",   STD_TXROM },
	{ "ACCLAIM-TLROM",    STD_TXROM },
	{ "KONAMI-TLROM",     STD_TXROM },
//
	/* Nintendo DxROM */
	{ "HVC-DE1ROM",       STD_DXROM },
	{ "HVC-DEROM",        STD_DXROM },
	{ "HVC-DRROM",        STD_DXROM },
	{ "NES-DE1ROM",       STD_DXROM },
	{ "NES-DEROM",        STD_DXROM },
	{ "NES-DRROM",        STD_DXROM },
	/* DxROM boards by other manufacturer */
	{ "NAMCOT-3401",      STD_DXROM },
	{ "NAMCOT-3405",      STD_DXROM },
	{ "NAMCOT-3406",      STD_DXROM },
	{ "NAMCOT-3407",      STD_DXROM },
	{ "NAMCOT-3413",      STD_DXROM },
	{ "NAMCOT-3414",      STD_DXROM },
	{ "NAMCOT-3415",      STD_DXROM },
	{ "NAMCOT-3416",      STD_DXROM },
	{ "NAMCOT-3417",      STD_DXROM },
	{ "NAMCOT-3451",      STD_DXROM },
	{ "TENGEN-800002",    STD_DXROM },
	{ "TENGEN-800004",    STD_DXROM },
	{ "TENGEN-800030",    STD_DXROM },
//
	/* Nintendo HKROM */
	{ "HVC-HKROM",        STD_HKROM },
	{ "NES-HKROM",        STD_HKROM },
//
	/* Nintendo BNROM */
	{ "HVC-TQROM",        STD_TQROM },
	{ "NES-TQROM",        STD_TQROM },
//
	/* Nintendo TxSROM */
	{ "HVC-TKSROM",       STD_TXSROM },
	{ "HVC-TLSROM",       STD_TXSROM },
	{ "NES-TKSROM",       STD_TXSROM },
	{ "NES-TLSROM",       STD_TXSROM },
//
	/* Nintendo ExROM */
	{ "HVC-EKROM",        STD_EXROM },
	{ "HVC-ELROM",        STD_EXROM },
	{ "HVC-ETROM",        STD_EXROM },
	{ "HVC-EWROM",        STD_EXROM },
	{ "NES-EKROM",        STD_EXROM },
	{ "NES-ELROM",        STD_EXROM },
	{ "NES-ETROM",        STD_EXROM },
	{ "NES-EWROM",        STD_EXROM },
//
	/* Nintendo Custom boards */
	{ "PAL-ZZ",           PAL_ZZ },
	{ "NES-QJ",           NES_QJ },
	{ "NES-EVENT",        UNSUPPORTED_BOARD },
//
	/* Discrete board IC_74x139x74 */
	{ "JALECO-JF-05",     DIS_74X139X74 },
	{ "JALECO-JF-06",     DIS_74X139X74 },
	{ "JALECO-JF-07",     DIS_74X139X74 },
	{ "JALECO-JF-08",     DIS_74X139X74 },
	{ "JALECO-JF-09",     DIS_74X139X74 },
	{ "JALECO-JF-10",     DIS_74X139X74 },
	{ "KONAMI-74*139/74", DIS_74X139X74 },
	{ "TAITO-74*139/74",  DIS_74X139X74 },
	/* Discrete board IC_74x377 */
	{ "AGCI-47516",       DIS_74X377 },
	{ "AVE-NINA-07",      DIS_74X377 },
	{ "COLORDREAMS-74*377", DIS_74X377 },
	/* Discrete board IC_74x161x161x32 */
	{ "BANDAI-74*161/161/32", DIS_74X161X161X32 },
	{ "TAITO-74*161/161/32", DIS_74X161X161X32 },
	/* Discrete board IC_74x161x138 */
	{ "BIT-CORP-74*161/138", DIS_74X161X138 },
//
	{ "BANDAI-LZ93D50+24C01", BANDAI_LZ93EX },
	{ "BANDAI-LZ93D50+24C02", BANDAI_LZ93EX },
	{ "BANDAI-FCG-1",     BANDAI_FCG },
	{ "BANDAI-FCG-2",     BANDAI_FCG },
	{ "BANDAI-JUMP2",     BANDAI_JUMP2 },	
	{ "BANDAI-DATACH",    BANDAI_DATACH },
	{ "BANDAI-KARAOKE",   BANDAI_KARAOKE },
	{ "BANDAI-OEKAKIDS",  BANDAI_OEKAKIDS },
//
	{ "IREM-G101",        IREM_G101 },
	{ "IREM-G101-A",      IREM_G101 },
	{ "IREM-G101-B",      IREM_G101 },
	{ "IREM-74*161/161/21/138", IREM_LROG017 },
	{ "IREM-H-3001",      IREM_H3001 },
	{ "IREM-HOLYDIVER",   IREM_HOLYDIV },
//
	{ "JALECO-JF-23",     JALECO_SS88006 },
	{ "JALECO-JF-24",     JALECO_SS88006 },
	{ "JALECO-JF-25",     JALECO_SS88006 },
	{ "JALECO-JF-27",     JALECO_SS88006 },
	{ "JALECO-JF-29",     JALECO_SS88006 },
	{ "JALECO-JF-30",     JALECO_SS88006 },
	{ "JALECO-JF-31",     JALECO_SS88006 },
	{ "JALECO-JF-32",     JALECO_SS88006 },
	{ "JALECO-JF-33",     JALECO_SS88006 },
	{ "JALECO-JF-34",     JALECO_SS88006 },
	{ "JALECO-JF-35",     JALECO_SS88006 },
	{ "JALECO-JF-36",     JALECO_SS88006 },
	{ "JALECO-JF-37",     JALECO_SS88006 },
	{ "JALECO-JF-38",     JALECO_SS88006 },
	{ "JALECO-JF-40",     JALECO_SS88006 },
	{ "JALECO-JF-41",     JALECO_SS88006 },
	{ "JALECO-JF-11",     JALECO_JF11 },
	{ "JALECO-JF-12",     JALECO_JF11 },
	{ "JALECO-JF-14",     JALECO_JF11 },
	{ "JALECO-JF-13",     JALECO_JF13 },
	{ "JALECO-JF-16",     JALECO_JF16 },
	{ "JALECO-JF-17",     JALECO_JF17 },
	{ "JALECO-JF-26",     JALECO_JF17 },
	{ "JALECO-JF-28",     JALECO_JF17 },
	{ "JALECO-JF-19",     JALECO_JF19 },
	{ "JALECO-JF-21",     JALECO_JF19 },
//
	{ "KONAMI-VRC-1",     KONAMI_VRC1 },
	{ "JALECO-JF-20",     KONAMI_VRC1 },
	{ "JALECO-JF-22",     KONAMI_VRC1 },
	{ "KONAMI-VRC-2",     KONAMI_VRC2 },
	{ "KONAMI-VRC-3",     KONAMI_VRC3 },
	{ "KONAMI-VRC-4",     KONAMI_VRC4 },
	{ "KONAMI-VRC-6",     KONAMI_VRC6 },
	{ "KONAMI-VRC-7",     KONAMI_VRC7 },
	{ "UNL-VRC7",         KONAMI_VRC7 },
//
	{ "NAMCOT-163",       NAMCOT_163 },
	{ "NAMCOT-3425",      NAMCOT_3425 },
	{ "NAMCOT-3433",      NAMCOT_34X3 },
	{ "NAMCOT-3443",      NAMCOT_34X3 },
	{ "NAMCOT-3446",      NAMCOT_3446 },
//
	{ "SUNSOFT-1",        SUNSOFT_1 },
	{ "SUNSOFT-2",        SUNSOFT_2 },
	{ "SUNSOFT-3",        SUNSOFT_3 },
//
	{ "TAITO-TC0190FMC",  TAITO_TC0190FMC },
	{ "TAITO-TC0190FMC+PAL16R4", TAITO_TC0190FMCP },
	{ "TAITO-X1-005",     TAITO_X1005 },
	{ "TAITO-X1-017",     TAITO_X1017 },
//
	{ "AGCI-50282",       AGCI_50282 },
	{ "AVE-NINA-01",      AVE_NINA01 },
	{ "AVE-NINA-02",      AVE_NINA01 },
	{ "AVE-NINA-03",      AVE_NINA06 },
	{ "AVE-NINA-06",      AVE_NINA06 },
	{ "AVE-MB-91",        AVE_NINA06 },
	{ "UNL-SA-016-1M",    AVE_NINA06 },
	{ "CAMERICA-ALGN",    CAMERICA_ALGN },
	{ "CAMERICA-BF9093",  CAMERICA_ALGN },
	{ "CAMERICA-BF9097",  CAMERICA_9097 },
	{ "CAMERICA-ALGQ",    CAMERICA_ALGQ },
	{ "CAMERICA-BF9096",  CAMERICA_ALGQ },
	{ "CAMERICA-GOLDENFIVE", CAMERICA_GOLDENFIVE },
	{ "CNE-DECATHLON",    CNE_DECATHLON },
	{ "CNE-PSB",          CNE_PSB },
	{ "CNE-SHLZ",         CNE_SHLZ },
	{ "JYCOMPANY-A",      UNSUPPORTED_BOARD },	// mapper 90
	{ "JYCOMPANY-B",      UNSUPPORTED_BOARD },	// mapper 209
	{ "JYCOMPANY-C",      UNSUPPORTED_BOARD },	// mapper 211
	{ "NTDEC-112",        NTDEC_ASDER },
	{ "NTDEC-193",        NTDEC_FIGHTINGHERO },
	{ "UNL-TEK90",        UNSUPPORTED_BOARD },	// related to JY Company? (i.e. mappers 90, 209, 211?)
	{ "UNL-SA-002",       SACHEN_TCU02 },
	{ "UNL-SA-0036",      SACHEN_SA0036 },
	{ "UNL-SA-0037",      SACHEN_SA0037 },
	{ "UNL-SA-72007",     SACHEN_SA72007 },
	{ "UNL-SA-72008",     SACHEN_SA72008 },
	{ "UNL-SA-NROM",      SACHEN_TCA01 },
	{ "UNL-SACHEN-TCA01", SACHEN_TCA01 },
	{ "SACHEN-8259A",     SACHEN_8259A },
	{ "UNL-SACHEN-8259A", SACHEN_8259A },
	{ "SACHEN-8259B",     SACHEN_8259B },
	{ "UNL-SACHEN-8259B", SACHEN_8259B },
	{ "SACHEN-8259C",     SACHEN_8259C },
	{ "UNL-SACHEN-8259C", SACHEN_8259C },
	{ "SACHEN-8259D",     SACHEN_8259D },
	{ "UNL-SACHEN-8259D", SACHEN_8259D },
	{ "UNL-SACHEN-74LS374N", SACHEN_74LS374 },
	{ "UNL-TC-U01-1.5M",  SACHEN_TCU01 },
	{ "TENGEN-800032",    TENGEN_800032 },
	{ "TENGEN-800037",    TENGEN_800037 },
	{ "WAIXING-A",        WAIXING_TYPE_A },
	{ "WAIXING-B",        WAIXING_TYPE_B },
	{ "WAIXING-C",        WAIXING_TYPE_C },
	{ "WAIXING-D",        WAIXING_TYPE_D },
	{ "WAIXING-E",        WAIXING_TYPE_E },
	{ "WAIXING-F",        WAIXING_TYPE_F },
	{ "WAIXING-G",        WAIXING_TYPE_G },
	{ "WAIXING-H",        WAIXING_TYPE_H },
	{ "WAIXING-SGZLZ",    WAIXING_SGZLZ },
	{ "WAIXING-SEC",      WAIXING_SECURITY },
	{ "WAIXING-SGZ",      WAIXING_SGZ },
	{ "WAIXING-PS2",      WAIXING_PS2 },
	{ "WAIXING-FFV",      WAIXING_FFV },
	{ "WAIXING-ZS",       WAIXING_ZS },
	{ "WAIXING-SH2",      UNSUPPORTED_BOARD },
//
	{ "TXC-TW",           TXC_TW },
	{ "TXC-MXMDHTWO",     TXC_MXMDHTWO },
	{ "UNL-22211",        TXC_22211A },
	{ "UNL-REXSOFT-DBZ5", REXSOFT_DBZ5 },
	{ "UNL-SL1632",       REXSOFT_SL1632 },
	{ "SUBOR-BOARD-0",    SUBOR_TYPE0 },
	{ "SUBOR-BOARD-1",    SUBOR_TYPE1 },
	{ "SOMERITEAM-SL-12", UNSUPPORTED_BOARD }, // mapper 116
	{ "UNL-CONY",         CONY_BOARD },
	{ "UNL-GOUDER",       GOUDER_37017 },
	{ "UNL-NITRA",        NITRA_TDA },
	{ "UNL-HOSENKAN",     HOSENKAN_BOARD },
	{ "UNL-SUPERGAME",    SUPERGAME_LIONKING },
	{ "UNL-PANDAPRINCE",  KAY_PANDAPRINCE },
	{ "DREAMTECH01",      DREAMTECH_BOARD },
	{ "DAOU-306",         OPENCORP_DAOU306 },
	{ "HES",              HES_BOARD },
	{ "SUPERGAME-BOOGERMAN", SUPERGAME_BOOGERMAN },
	{ "FUTUREMEDIA",      FUTUREMEDIA_BOARD },
	{ "MAGICSERIES",      MAGICSERIES_MAGICDRAGON },
	{ "KASING",           KASING_BOARD },
	{ "HENGGEDIANZI",     HENGEDIANZI_BOARD },
	{ "KAISER-KS7058",    KAISER_KS7058 },
	{ "KAISER-KS202",     UNSUPPORTED_BOARD },// mapper 56
	{ "KAISER-KS7022",    UNSUPPORTED_BOARD },// mapper 175
	{ "UNL-KS7017",       UNSUPPORTED_BOARD },
	{ "UNL-KS7032",       UNSUPPORTED_BOARD }, //  mapper 142
	{ "RCM-GS2015",       RCM_GS2015 },
	{ "RCM-TETRISFAMILY", RCM_TETRISFAMILY },
	{ "UNL-NINJARYU",     UNSUPPORTED_BOARD },// mapper 111
	{ "UNL-NANJING",      UNSUPPORTED_BOARD },// mapper 163
	{ "FUKUTAKE",         UNSUPPORTED_BOARD },
	{ "WHIRLWIND-2706",   UNSUPPORTED_BOARD },
	{ "UNL-H2288",        UNSUPPORTED_BOARD },	// mapper 123
	{ "UNL-DANCE",        UNSUPPORTED_BOARD },
	{ "UNL-EDU2000",      UNSUPPORTED_BOARD /*UNL_EDU2K*/ },
	{ "UNL-SHERO",        UNSUPPORTED_BOARD /*SACHEN_SHERO*/ },
	{ "UNL-TF1201",       UNSUPPORTED_BOARD /*UNL_TF1201*/ },
	{ "RUMBLESTATION",    RUMBLESTATION_BOARD },	// mapper 46
	{ "UNL-WORLDHERO",    UNSUPPORTED_BOARD },// mapper 27
	{ "UNL-A9746",        UNSUPPORTED_BOARD },// mapper 219
	{ "UNL-603-5052",     UNSUPPORTED_BOARD },// mapper 238?
	{ "UNL-SHJY3",        UNSUPPORTED_BOARD },// mapper 253
	{ "UNL-N625092",      UNL_N625092 },
	{ "BMC-N625092",      UNL_N625092 },
	{ "UNL-SC-127",       UNL_SC127 },
	{ "UNL-SMB2J",        UNL_SMB2J },
	{ "UNL-MK2",          UNL_MK2 },
	{ "UNL-XZY",          UNL_XZY },
	{ "UNL-KOF96",        UNL_KOF96 },
	{ "UNL-SUPERFIGHTER3", UNL_SUPERFIGHTER3 },
	{ "UNL-8237",         UNL_8237 },
	{ "UNL-AX5705",       UNL_AX5705 },
	{ "UNL-CC-21",        UNL_CC21 },
	{ "UNL-KOF97",        UNL_KOF97 },
	{ "UNL-T-230",        UNL_T230 },
//
	{ "BTL-SMB2A",         BTL_SMB2A },
	{ "BTL-MARIOBABY",     BTL_MARIOBABY },
	{ "BTL-AISENSHINICOL", BTL_AISENSHINICOL },
	{ "BTL-SMB2B",         BTL_SMB2B },
	{ "BTL-SMB3",          BTL_SMB3 },
	{ "BTL-SUPERBROS11",   BTL_SUPERBROS11 },
	{ "BTL-DRAGONNINJA",   BTL_DRAGONNINJA },
	{ "BTL-MARIO1-MALEE2", UNSUPPORTED_BOARD },	// mapper 55?
	{ "BTL-2708",          UNSUPPORTED_BOARD },// mapper 103
	{ "BTL-TOBIDASEDAISAKUSEN", UNSUPPORTED_BOARD },// mapper 120
	{ "BTL-SHUIGUANPIPE",  UNSUPPORTED_BOARD },// mapper 183
	{ "BTL-PIKACHUY2K",    UNSUPPORTED_BOARD },// mapper 254
//	
	{ "BMC-190IN1",          BMC_190IN1 },
	{ "BMC-64IN1NOREPEAT",   BMC_64IN1NR },
	{ "BMC-A65AS",           BMC_A65AS },
	{ "BMC-GS-2004",         BMC_GS2004 },
	{ "BMC-GS-2013",         BMC_GS2013 },
	{ "BMC-NOVELDIAMOND9999999IN1", BMC_NOVELDIAMOND },
	{ "BMC-SUPER24IN1SC03",  BMC_S24IN1SC03 },
	{ "BMC-SUPERHIK8IN1",    BMC_HIK8IN1 },
	{ "BMC-T-262",           BMC_T262 },
	{ "BMC-WS",              BMC_WS },
	{ "MLT-ACTION52",        ACTENT_ACT52 },
	{ "MLT-CALTRON6IN1",     CALTRON_6IN1 },
	{ "MLT-MAXI15",          UNSUPPORTED_BOARD}, //  mapper 234
	{ "BMC-SUPERBIG-7IN1",   BMC_SUPERBIG_7IN1 },
	{ "BMC-SUPERHIK-4IN1",   BMC_SUPERHIK_4IN1 },
	{ "BMC-BALLGAMES-11IN1", BMC_BALLGAMES_11IN1 },
	{ "BMC-MARIOPARTY-7IN1", BMC_MARIOPARTY_7IN1 },
	{ "BMC-GKA",             BMC_GKA },
	{ "BMC-GKB",             BMC_GKB },
	{ "BMC-SUPER700IN1",     BMC_SUPER_700IN1 },
	{ "BMC-FAMILY-4646B",    BMC_FAMILY_4646B },
	{ "BMC-36IN1",           BMC_36IN1 },
	{ "BMC-21IN1",           BMC_21IN1 },
	{ "BMC-150IN1",          BMC_150IN1 },
	{ "BMC-35IN1",           BMC_35IN1 },
	{ "BMC-64IN1",           BMC_64IN1 },
	{ "BMC-15IN1",           BMC_15IN1 },
	{ "BMC-SUPERHIK-300IN1", BMC_SUPERHIK_300IN1 },
	{ "BMC-9999999IN1",      BMC_9999999IN1 }, // mapper 213... same as BMC-NOVELDIAMOND9999999IN1 ??
	{ "BMC-SUPERGUN-20IN1",  BMC_SUPERGUN_20IN1 },
	{ "BMC-GOLDENCARD-6IN1", BMC_GOLDENCARD_6IN1 },
	{ "BMC-72IN1",           BMC_72IN1 },
	{ "BMC-76IN1",           BMC_76IN1 },
	{ "BMC-SUPER42IN1",      BMC_SUPER_42IN1 },
	{ "BMC-1200IN1",         BMC_1200IN1 },
	{ "BMC-31IN1",           BMC_31IN1 },
	{ "BMC-22GAMES",         BMC_22GAMES },
	{ "BMC-20IN1",           BMC_20IN1 },
	{ "BMC-110IN1",          BMC_110IN1 },
	{ "BMC-810544-C-A1",     UNSUPPORTED_BOARD },
	{ "BMC-411120-C",        UNSUPPORTED_BOARD },
	{ "BMC-830118C",         UNSUPPORTED_BOARD },
	{ "BMC-12-IN-1",         UNSUPPORTED_BOARD },
	{ "BMC-8157",            UNSUPPORTED_BOARD /*BMC_8157*/ },
	{ "BMC-BS-5",            UNSUPPORTED_BOARD /*BENSHENG_BS5*/ },
	{ "BMC-FK23C",           UNSUPPORTED_BOARD /*BMC_FK23C*/ },
	{ "BMC-FK23CA",          UNSUPPORTED_BOARD /*BMC_FK23C*/ },	// diff reg init
	{ "BMC-GHOSTBUSTERS63IN1", UNSUPPORTED_BOARD /*BMC_G63IN1*/ },
	{ "BMC-SUPERVISION16IN1",  UNSUPPORTED_BOARD },	// mapper 53
	{ "BMC-RESETBASED-4IN1",   UNSUPPORTED_BOARD },// mapper 60 with 64k prg and 32k chr
	{ "BMC-VT5201",          UNSUPPORTED_BOARD },// mapper 60 otherwise
	{ "BMC-42IN1RESETSWITCH",  UNSUPPORTED_BOARD },	// mapper 60?
	{ "BMC-D1038",           UNSUPPORTED_BOARD }, // mapper 60?
	{ "BMC-SUPER22GAMES",    UNSUPPORTED_BOARD },// mapper 233
	{ "BMC-GOLDENGAME-150IN1",      UNSUPPORTED_BOARD },// mapper 235 with 2M PRG
	{ "BMC-GOLDENGAME-260IN1",      UNSUPPORTED_BOARD },// mapper 235 with 4M PRG
	{ "BMC-70IN1",           UNSUPPORTED_BOARD },	// mapper 236?
	{ "BMC-70IN1B",          UNSUPPORTED_BOARD },	// mapper 236?
	{ "BMC-SUPERHIK-KOF",    UNSUPPORTED_BOARD },// mapper 251
// are there dumps of games with these boards?
	{ "WAIXING-I",        UNSUPPORTED_BOARD }, // [supported by NEStopia - we need more info]
	{ "WAIXING-J",        UNSUPPORTED_BOARD }, // [supported by NEStopia - we need more info]
	{ "BMC-13IN1JY110",  UNSUPPORTED_BOARD }, //  [mentioned in FCEUMM source - we need more info]
	{ "BMC-GK-192",      UNSUPPORTED_BOARD }, //  [mentioned in FCEUMM source - we need more info]
	{ "KONAMI-QTAI",     UNSUPPORTED_BOARD }, //  [mentioned in FCEUMM source - we need more info]
	{ "UNL-8157",        UNSUPPORTED_BOARD }, //  [mentioned in FCEUMM source - we need more info]
	{ "UNL-3D-BLOCK",    UNSUPPORTED_BOARD }, //  [mentioned in FCEUMM source - we need more info]
	{ "UNL-C-N22M",      UNSUPPORTED_BOARD }, //  [mentioned in FCEUMM source - we need more info]
	{ "UNL-KS7017",      UNSUPPORTED_BOARD }, //  [mentioned in FCEUMM source - we need more info]
	{ "UNL-PEC-586",     UNSUPPORTED_BOARD }, //  [mentioned in FCEUMM source - we need more info]
	{ "UNL-SA-009",      UNSUPPORTED_BOARD }, //  [mentioned in FCEUMM source - we need more info]
	{0}
};

const nes_pcb *nes_pcb_lookup( const char *board )
{
	int i;
	for (i = 0; i < ARRAY_LENGTH(pcb_list); i++)
	{
		if (!mame_stricmp(pcb_list[i].pcb_name, board))
			return &pcb_list[i];
	}
	return NULL;
}

/************************************************

   PCB Emulation (to be moved to MAME later)

   In the end, iNES, UNIF and xml should be 
   simply handled through a look-up table 
   which associates the mapper/board/feature
   to the correct pcb_id and then the core
   function pcb_handlers_setup should be called
   to set-up the expected handlers and callbacks
 
   Similarly, PC-10, VSNES and NES-based 
   multigame boards, should simply call 
   pcb_handlers_setup with the proper pcb_id 
   at the beginning, rather than use ad hoc
   implementations.
 
************************************************/


/*************************************************************
 
 UxROM board emulation
 
 writes to 0x8000-0xffff change PRG 16K lower banks
 
 missing BC?

 iNES: mapper 2

 *************************************************************/

static WRITE8_HANDLER( uxrom_w )
{
	LOG_MMC(("uxrom_w, offset: %04x, data: %02x\n", offset, data));
	
	prg16_89ab(space->machine, data);
}

/*************************************************************
 
 UN1ROM board emulation
 
 writes to 0x8000-0xffff change PRG 16K lower banks 
 
 missing BC?

 iNES: mapper 94
 
 *************************************************************/

static WRITE8_HANDLER( un1rom_w )
{
	LOG_MMC(("un1rom_w, offset: %04x, data: %02x\n", offset, data));
	
	prg16_89ab(space->machine, data >> 2);
}

/*************************************************************
 
 CNROM board emulation
 
 writes to 0x8000-0xffff change CHR 8K banks
 
 missing BC?

 iNES: mapper 3
 
 Notice that BANDAI_PT554 board (Aerobics Studio) uses very 
 similar hardware but with an additional sound chip which 
 gets writes to 0x6000 (currently unemulated in MESS)

 *************************************************************/

static WRITE8_HANDLER( cnrom_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("cnrom_w, offset: %04x, data: %02x\n", offset, data));

	if (state->ce_mask)
	{
		chr8(space->machine, data & ~state->ce_mask, CHRROM);

		if ((data & state->ce_mask) != state->ce_state)
			state->chr_open_bus = 0;
		else
			state->chr_open_bus = 1;
	}
	else
		chr8(space->machine, data, CHRROM);
}

/*************************************************************
 
 CPROM board emulation
 
 writes to 0x8000-0xffff change CHR 4K lower banks
 
 iNES: mapper 13
 
 *************************************************************/

static WRITE8_HANDLER( cprom_w )
{
	LOG_MMC(("cprom_w, offset: %04x, data: %02x\n", offset, data));
	chr4_4(space->machine, data, CHRRAM);
}

/*************************************************************
 
 AxROM board emulation
 
 writes to 0x8000-0xffff change PRG banks + sets mirroring
 
 missing BC for AMROM?
 
 iNES: mapper 7
 
 *************************************************************/

static WRITE8_HANDLER( axrom_w )
{
	LOG_MMC(("axrom_w, offset: %04x, data: %02x\n", offset, data));
	
	set_nt_mirroring(space->machine, BIT(data, 4) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
	prg32(space->machine, data);
}

/*************************************************************
 
 BxROM board emulation
 
 writes to 0x8000-0xffff change PRG banks
 
 missing BC?
 
 iNES: mapper 34
 
 *************************************************************/

static WRITE8_HANDLER( bxrom_w )
{
	/* This portion of the mapper is nearly identical to Mapper 7, except no one-screen mirroring */
	/* Deadly Towers is really a BxROM game - the demo screens look wrong using mapper 7. */
	LOG_MMC(("bxrom_w, offset: %04x, data: %02x\n", offset, data));
	
	prg32(space->machine, data);
}

/*************************************************************
 
 GxROM/MxROM board emulation
 
 writes to 0x8000-0xffff change PRG and CHR banks
 
 missing BC?
 
 iNES: mapper 66
 
 *************************************************************/

static WRITE8_HANDLER( gxrom_w )
{
	LOG_MMC(("gxrom_w, offset %04x, data: %02x\n", offset, data));
	
	prg32(space->machine, (data & 0xf0) >> 4);
	chr8(space->machine, data & 0x0f, CHRROM);
}

/*************************************************************
 
 SxROM (MMC1 based) board emulation
 
 iNES: mapper 1 (and 155 for the MMC1A variant which does not 
 have WRAM disable bit)
 
 *************************************************************/

static void mmc1_set_wram( const address_space *space, int board )
{
	running_machine *machine = space->machine;
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 bank = BIT(state->mmc_reg[0], 4) ? BIT(state->mmc_reg[1], 4) : BIT(state->mmc_reg[0], 3);

	switch (board)
	{
		case STD_SOROM:		// there are 2 WRAM banks only and battery is bank 2 for the cart (hence, we invert bank, because we have battery first)
			if (!BIT(state->mmc_reg[3], 4))
				memory_install_readwrite_bank(space, 0x6000, 0x7fff, 0, 0, "bank5");
			else
				memory_unmap_readwrite(space, 0x6000, 0x7fff, 0, 0);
			wram_bank(machine, 0, bank ? NES_BATTERY : NES_WRAM);
			break;
		case STD_SXROM:		// here also reads are disabled!
			if (!BIT(state->mmc_reg[3], 4))
				memory_install_readwrite_bank(space, 0x6000, 0x7fff, 0, 0, "bank5");
			else
				memory_unmap_readwrite(space, 0x6000, 0x7fff, 0, 0);
		case STD_SXROM_A:	// ignore WRAM enable bit
			if (state->battery)
				wram_bank(machine, ((state->mmc_reg[1] & 3) >> 2), NES_BATTERY);
			break;
	}
}

static void mmc1_set_prg( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 prg_mode, prg_offset;
	
	prg_mode = state->mmc_reg[0] & 0x0c;
	/* prg_mode&0x8 determines bank size: 32k (if 0) or 16k (if 1)? when in 16k mode,
	 prg_mode&0x4 determines which half of the PRG space we can swap: if it is 4,
	 mmc_reg[3] sets banks at 0x8000; if it is 0, mmc_reg[3] sets banks at 0xc000. */
	
	prg_offset = state->mmc_reg[1] & 0x10;
	/* In principle, mmc_reg[2]&0x10 might affect "extended" banks as well, when chr_mode=1.
	 However, quoting Disch's docs: When in 4k CHR mode, 0x10 in both $A000 and $C000 *must* be
	 set to the same value, or else pages will constantly be swapped as graphics render!
	 Hence, we use only mmc_reg[1]&0x10 for prg_offset */
	
	switch (prg_mode)
	{
		case 0x00:
		case 0x04:
			prg32(machine, prg_offset + state->mmc_reg[3]);
			break;
		case 0x08:
			prg16_89ab(machine, prg_offset + 0);
			prg16_cdef(machine, prg_offset + state->mmc_reg[3]);
			break;
		case 0x0c:
			prg16_89ab(machine, prg_offset + state->mmc_reg[3]);
			prg16_cdef(machine, prg_offset + 0x0f);
			break;
	}
}

static void mmc1_set_prg_wram( const address_space *space, int board )
{
	mmc1_set_prg(space->machine);
	mmc1_set_wram(space, board);
}

static void mmc1_set_chr( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_mode = state->mmc_reg[0] & 0x10;
	
	if (chr_mode)
	{
		chr4_0(machine, state->mmc_reg[1] & 0x1f, state->mmc_chr_source);
		chr4_4(machine, state->mmc_reg[2] & 0x1f, state->mmc_chr_source);
	}
	else
		chr8(machine, (state->mmc_reg[1] & 0x1f) >> 1, state->mmc_chr_source);
}

static void common_sxrom_write_handler( const address_space *space, offs_t offset, UINT8 data, int board )
{
	running_machine *machine = space->machine;
	nes_state *state = (nes_state *)machine->driver_data;
	/* Note that there is only one latch and shift counter, shared amongst the 4 regs */
	/* Space Shuttle will not work if they have independent variables. */	
	
	/* here we would need to add an if(cpu_cycles_passed>1) test, and
	 if requirement is not met simply return without writing anything.
	 AD&D Hillsfar and Bill & Ted rely on this behavior!! */
	if (data & 0x80)
	{
		state->mmc_count = 0;
		state->mmc_latch1 = 0;
		
		/* Set reg at 0x8000 to size 16k and lower half swap - needed for Robocop 3, Dynowars */
		state->mmc_reg[0] |= 0x0c;
		mmc1_set_prg_wram(space, board);
		return;
	}
	
	if (state->mmc_count < 5)
	{
		if (state->mmc_count == 0) state->mmc_latch1 = 0;
		state->mmc_latch1 >>= 1;
		state->mmc_latch1 |= (data & 0x01) ? 0x10 : 0x00;
		state->mmc_count++;
	}
	
	if (state->mmc_count == 5)
	{
		switch (offset & 0x6000)	/* Which reg shall we write to? */
		{
			case 0x0000:
				state->mmc_reg[0] = state->mmc_latch1;
				
				switch (state->mmc_reg[0] & 0x03)
				{
				case 0: set_nt_mirroring(machine, PPU_MIRROR_LOW); break;
				case 1: set_nt_mirroring(machine, PPU_MIRROR_HIGH); break;
				case 2: set_nt_mirroring(machine, PPU_MIRROR_VERT); break;
				case 3: set_nt_mirroring(machine, PPU_MIRROR_HORZ); break;
				}
				mmc1_set_chr(machine);
				mmc1_set_prg_wram(space, board);
				break;
			case 0x2000:
				state->mmc_reg[1] = state->mmc_latch1;
				mmc1_set_chr(machine);
				mmc1_set_prg_wram(space, board);
				break;
			case 0x4000:
				state->mmc_reg[2] = state->mmc_latch1;
				mmc1_set_chr(machine);
				break;
			case 0x6000:
				state->mmc_reg[3] = state->mmc_latch1;
				mmc1_set_prg_wram(space, board);
				break;
		}
		
		state->mmc_count = 0;
	}
}

static WRITE8_HANDLER( sxrom_w )
{
	LOG_MMC(("sxrom_w, offset: %04x, data: %02x\n", offset, data));
	common_sxrom_write_handler(space, offset, data, STD_SXROM);
}

static WRITE8_HANDLER( sxrom_a_w )
{
	LOG_MMC(("sxrom_a_w, offset: %04x, data: %02x\n", offset, data));
	common_sxrom_write_handler(space, offset, data, STD_SXROM_A);
}

static WRITE8_HANDLER( sorom_w )
{
	LOG_MMC(("sorom_w, offset: %04x, data: %02x\n", offset, data));
	common_sxrom_write_handler(space, offset, data, STD_SOROM);
}

/*************************************************************
 
 PxROM (MMC2 based) board emulation
 
 iNES: mapper 9
 
 *************************************************************/

static void mmc2_latch( running_device *device, offs_t offset )
{
	nes_state *state = (nes_state *)device->machine->driver_data;
	if ((offset & 0x3ff0) == 0x0fd0)
	{
		LOG_MMC(("mmc2 vrom latch switch (bank 0 low): %02x\n", state->MMC2_regs[0]));
		state->mmc_latch1 = 0xfd;
		chr4_0(device->machine, state->MMC2_regs[0], CHRROM);
	}
	else if ((offset & 0x3ff0) == 0x0fe0)
	{
		LOG_MMC(("mmc2 vrom latch switch (bank 0 high): %02x\n", state->MMC2_regs[1]));
		state->mmc_latch1 = 0xfe;
		chr4_0(device->machine, state->MMC2_regs[1], CHRROM);
	}
	else if ((offset & 0x3ff0) == 0x1fd0)
	{
		LOG_MMC(("mmc2 vrom latch switch (bank 1 low): %02x\n", state->MMC2_regs[2]));
		state->mmc_latch2 = 0xfd;
		chr4_4(device->machine, state->MMC2_regs[2], CHRROM);
	}
	else if ((offset & 0x3ff0) == 0x1fe0)
	{
		LOG_MMC(("mmc2 vrom latch switch (bank 0 high): %02x\n", state->MMC2_regs[3]));
		state->mmc_latch2 = 0xfe;
		chr4_4(device->machine, state->MMC2_regs[3], CHRROM);
	}
}

static WRITE8_HANDLER( pxrom_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("pxrom_w, offset: %04x, data: %02x\n", offset, data));
	switch (offset & 0x7000)
	{
		case 0x2000:
			prg8_89(space->machine, data);
			break;
		case 0x3000:
			state->MMC2_regs[0] = data;
			if (state->mmc_latch1 == 0xfd)
				chr4_0(space->machine, state->MMC2_regs[0], CHRROM);
			break;
		case 0x4000:
			state->MMC2_regs[1] = data;
			if (state->mmc_latch1 == 0xfe)
				chr4_0(space->machine, state->MMC2_regs[1], CHRROM);
			break;
		case 0x5000:
			state->MMC2_regs[2] = data;
			if (state->mmc_latch2 == 0xfd)
				chr4_4(space->machine, state->MMC2_regs[2], CHRROM);
			break;
		case 0x6000:
			state->MMC2_regs[3] = data;
			if (state->mmc_latch2 == 0xfe)
				chr4_4(space->machine, state->MMC2_regs[3], CHRROM);
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
 
 FxROM (MMC4 based) board emulation
 
 iNES: mapper 10
 
 *************************************************************/

static WRITE8_HANDLER( fxrom_w )
{
	LOG_MMC(("fxrom_w, offset: %04x, data: %02x\n", offset, data));
	switch (offset & 0x7000)
	{
		case 0x2000:
			prg16_89ab(space->machine, data);
			break;
		default:
			pxrom_w(space, offset, data);
			break;
	}
}

/*************************************************************
 
 TxROM (MMC3 based) board emulation
 
 iNES: mapper 4
 
 *************************************************************/

static void mmc3_set_prg( running_machine *machine, int prg_base, int prg_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 prg_flip = (state->mmc_latch1 & 0x40) ? 2 : 0;
	
	prg8_89(machine, prg_base | (state->mmc_prg_bank[0 ^ prg_flip] & prg_mask));
	prg8_ab(machine, prg_base | (state->mmc_prg_bank[1] & prg_mask));
	prg8_cd(machine, prg_base | (state->mmc_prg_bank[2 ^ prg_flip] & prg_mask));
	prg8_ef(machine, prg_base | (state->mmc_prg_bank[3] & prg_mask));
}

static void mmc3_set_chr( running_machine *machine, UINT8 chr, int chr_base, int chr_mask )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_latch1 & 0x80) >> 5;
	
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
static void mmc3_irq( running_device *device, int scanline, int vblank, int blanked )
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

static WRITE8_HANDLER( txrom_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	
	LOG_MMC(("mapper4_w, offset: %04x, data: %02x\n", offset, data));
	
	switch (offset & 0x6001)
	{
		case 0x0000:
			MMC3_helper = state->mmc_latch1 ^ data;
			state->mmc_latch1 = data;
			
			/* Has PRG Mode changed? */
			if (MMC3_helper & 0x40)
				mmc3_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			
			/* Has CHR Mode changed? */
			if (MMC3_helper & 0x80)
				mmc3_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			break;
			
		case 0x0001:
			cmd = state->mmc_latch1 & 0x07;
			switch (cmd)
			{
			case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
			case 2: case 3: case 4: case 5:
				state->mmc_vrom_bank[cmd] = data;
				mmc3_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
				break;
			case 6:
			case 7:
				state->mmc_prg_bank[cmd - 6] = data;
				mmc3_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
				break;
			}
			break;
			
		case 0x2000:
			set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;
			
		case 0x2001: /* extra RAM enable/disable */
			state->mmc_latch2 = data;	/* This actually is made of two parts: data&0x80 = WRAM enabled and data&0x40 = WRAM readonly!  */
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
 
 HKROM (MMC6 based) board emulation
 
 iNES: mapper 4
 
 *************************************************************/

static WRITE8_HANDLER( hkrom_w )
{
	LOG_MMC(("hkrom_w, offset: %04x, data: %02x\n", offset, data));
	/* what are the differences MMC6 vs. MMC3? */
	txrom_w(space, offset, data);
}

/*************************************************************
 
 DxROM (MMC3 based) board emulation
 
 iNES: mapper 206
 
 *************************************************************/

static WRITE8_HANDLER( dxrom_w )
{
	LOG_MMC(("mapper206_w, offset: %04x, data: %02x\n", offset, data));

	if ((offset & 0x6001) == 0x2000)
		return;
	
	txrom_w(space, offset, data);
}


/*************************************************************
 
 TxSROM (MMC3 based) board emulation
 
 iNES: mapper 118
 
 *************************************************************/

static void txsrom_set_mirror( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	if (state->mmc_latch1 & 0x80)
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

static WRITE8_HANDLER( txsrom_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper118_w, offset: %04x, data: %02x\n", offset, data));
	
	switch (offset & 0x6001)
	{
		case 0x0000:
			MMC3_helper = state->mmc_latch1 ^ data;
			state->mmc_latch1 = data;
			
			/* Has PRG Mode changed? */
			if (MMC3_helper & 0x40)
				mmc3_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			
			/* Has CHR Mode changed? */
			if (MMC3_helper & 0x80)
			{
				txsrom_set_mirror(space->machine);
				mmc3_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
			}
			break;
			
		case 0x0001:
			cmd = state->mmc_latch1 & 0x07;
			switch (cmd)
		{
			case 0: case 1:
			case 2: case 3: case 4: case 5:
				state->mmc_vrom_bank[cmd] = data;
				txsrom_set_mirror(space->machine);
				mmc3_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
				break;
			case 6:
			case 7:
				state->mmc_prg_bank[cmd - 6] = data;
				mmc3_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
				break;
		}
			break;
			
		case 0x2000:
			break;
			
		default:
			txrom_w(space, offset, data);
			break;
	}
}

/*************************************************************
 
 TQROM (MMC3 based) board emulation
 
 iNES: mapper 119
 
 *************************************************************/

static void tqrom_set_chr( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_latch1 & 0x80) >> 5;
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

static WRITE8_HANDLER( tqrom_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 MMC3_helper, cmd;
	LOG_MMC(("mapper119_w, offset: %04x, data: %02x\n", offset, data));
	
	switch (offset & 0x6001)
	{
		case 0x0000:
			MMC3_helper = state->mmc_latch1 ^ data;
			state->mmc_latch1 = data;
			
			/* Has PRG Mode changed? */
			if (MMC3_helper & 0x40)
				mmc3_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
			
			/* Has CHR Mode changed? */
			if (MMC3_helper & 0x80)
				tqrom_set_chr(space->machine);
			break;
		case 0x0001: /* $8001 */
			cmd = state->mmc_latch1 & 0x07;
			switch (cmd)
			{
			case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
			case 2: case 3: case 4: case 5:
				state->mmc_vrom_bank[cmd] = data;
				tqrom_set_chr(space->machine);
				break;
			case 6:
			case 7:
				state->mmc_prg_bank[cmd - 6] = data;
				mmc3_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
				break;
			}
			break;
			
		default:
			txrom_w(space, offset, data);
			break;
	}
}

/*************************************************************
 
 PAL-ZZ board (MMC3 variant for European 3-in-1 Nintendo cart
 Super Mario Bros. + Tetris + Nintendo World Cup)
 
 iNES: mapper 37
 
 *************************************************************/

static WRITE8_HANDLER( zz_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 map37_helper;
	LOG_MMC(("mapper37_m_w, offset: %04x, data: %02x\n", offset, data));
	
	map37_helper = (data & 0x06) >> 1;
	
	state->mmc_prg_base = map37_helper << 3;
	state->mmc_prg_mask = (map37_helper == 2) ? 0x0f : 0x07;
	state->mmc_chr_base = map37_helper << 6;
	state->mmc_chr_mask = 0x7f;
	mmc3_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
	mmc3_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
}

/*************************************************************
 
 NES-QJ board (MMC3 variant for US 2-in-1 Nintendo cart
 Super Spike V'Ball + Nintendo World Cup)
 
 iNES: mapper 47
 
 *************************************************************/

static WRITE8_HANDLER( qj_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper47_m_w, offset: %04x, data: %02x\n", offset, data));
	
	state->mmc_prg_base = (data & 0x01) << 4;
	state->mmc_prg_mask = 0x0f;
	state->mmc_chr_base = (data & 0x01) << 7;
	state->mmc_chr_mask = 0x7f;
	mmc3_set_prg(space->machine, state->mmc_prg_base, state->mmc_prg_mask);
	mmc3_set_chr(space->machine, state->mmc_chr_source, state->mmc_chr_base, state->mmc_chr_mask);
}

/*************************************************************
 
 ExROM (MMC5 based) board emulation
 
 iNES: mapper 5
 
 MESS status: Mostly Unsupported
 
 *************************************************************/

static void mmc5_irq( running_device *device, int scanline, int vblank, int blanked )
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

static void mmc5_ppu_mirror( running_machine *machine, int page, int src )
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

static READ8_HANDLER( exrom_l_r )
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


static WRITE8_HANDLER( exrom_l_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	
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
		if (state->MMC5_vram_protect == 0x03)
			state->mmc5_vram[offset - 0x1b00] = data;
		return;
	}
	
	switch (offset)
	{
		case 0x1000: /* $5100 */
			state->MMC5_rom_bank_mode = data & 0x03;
			LOG_MMC(("MMC5 rom bank mode: %02x\n", data));
			break;
			
		case 0x1001: /* $5101 */
			state->MMC5_vrom_bank_mode = data & 0x03;
			LOG_MMC(("MMC5 vrom bank mode: %02x\n", data));
			break;
			
		case 0x1002: /* $5102 */
			if (data == 0x02)
				state->MMC5_vram_protect |= 1;
			else
				state->MMC5_vram_protect = 0;
			LOG_MMC(("MMC5 vram protect 1: %02x\n", data));
			break;
		case 0x1003: /* 5103 */
			if (data == 0x01)
				state->MMC5_vram_protect |= 2;
			else
				state->MMC5_vram_protect = 0;
			LOG_MMC(("MMC5 vram protect 2: %02x\n", data));
			break;
			
		case 0x1004: /* $5104 - Extra VRAM (EXRAM) control */
			state->mmc5_vram_control = data & 0x03;
			LOG_MMC(("MMC5 exram control: %02x\n", data));
			break;
			
		case 0x1005: /* $5105 */
			mmc5_ppu_mirror(space->machine, 0, data & 0x03);
			mmc5_ppu_mirror(space->machine, 1, (data & 0x0c) >> 2);
			mmc5_ppu_mirror(space->machine, 2, (data & 0x30) >> 4);
			mmc5_ppu_mirror(space->machine, 3, (data & 0xc0) >> 6);
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
			LOG_MMC(("MMC5 $5114 bank select: %02x (mode: %d)\n", data, state->MMC5_rom_bank_mode));
			switch (state->MMC5_rom_bank_mode)
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
					state->prg_bank[0] = state->prg_chunks + (data & 0x07);
					memory_set_bank(space->machine, "bank1", state->prg_bank[0]);
				}
				break;
			}
			break;
		case 0x1015: /* $5115 */
			LOG_MMC(("MMC5 $5115 bank select: %02x (mode: %d)\n", data, state->MMC5_rom_bank_mode));
			switch (state->MMC5_rom_bank_mode)
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
					state->prg_bank[1] = state->prg_chunks + (data & 0x07);
					memory_set_bank(space->machine, "bank2", state->prg_bank[1]);
				}
				break;
			}
			break;
		case 0x1016: /* $5116 */
			LOG_MMC(("MMC5 $5116 bank select: %02x (mode: %d)\n", data, state->MMC5_rom_bank_mode));
			switch (state->MMC5_rom_bank_mode)
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
					state->prg_bank[2] = state->prg_chunks + (data & 0x07);
					memory_set_bank(space->machine, "bank3", state->prg_bank[2]);
				}
				break;
			}
			break;
		case 0x1017: /* $5117 */
			LOG_MMC(("MMC5 $5117 bank select: %02x (mode: %d)\n", data, state->MMC5_rom_bank_mode));
			switch (state->MMC5_rom_bank_mode)
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
			LOG_MMC(("MMC5 $5120 vrom select: %02x (mode: %d)\n", data, state->MMC5_vrom_bank_mode));
			switch (state->MMC5_vrom_bank_mode)
			{
			case 0x03:
				/* 1k switch */
				state->MMC5_vrom_bank[0] = data | (state->mmc5_high_chr << 8);
				//                  mapper5_sync_vrom(0);
				chr1_0(space->machine, state->MMC5_vrom_bank[0], CHRROM);
				//                  state->nes_vram_sprite[0] = state->MMC5_vrom_bank[0] * 64;
				//                  vrom_next[0] = 4;
				//                  vrom_page_a = 1;
				//                  vrom_page_b = 0;
				break;
			}
			break;
		case 0x1021: /* $5121 */
			LOG_MMC(("MMC5 $5121 vrom select: %02x (mode: %d)\n", data, state->MMC5_vrom_bank_mode));
			switch (state->MMC5_vrom_bank_mode)
			{
			case 0x02:
				/* 2k switch */
				chr2_0(space->machine, data | (state->mmc5_high_chr << 8), CHRROM);
				break;
			case 0x03:
				/* 1k switch */
				state->MMC5_vrom_bank[1] = data | (state->mmc5_high_chr << 8);
				//                  mapper5_sync_vrom(0);
				chr1_1(space->machine, state->MMC5_vrom_bank[1], CHRROM);
				//                  state->nes_vram_sprite[1] = state->MMC5_vrom_bank[0] * 64;
				//                  vrom_next[1] = 5;
				//                  vrom_page_a = 1;
				//                  vrom_page_b = 0;
				break;
			}
			break;
		case 0x1022: /* $5122 */
			LOG_MMC(("MMC5 $5122 vrom select: %02x (mode: %d)\n", data, state->MMC5_vrom_bank_mode));
			switch (state->MMC5_vrom_bank_mode)
			{
			case 0x03:
				/* 1k switch */
				state->MMC5_vrom_bank[2] = data | (state->mmc5_high_chr << 8);
				//                  mapper5_sync_vrom(0);
				chr1_2(space->machine, state->MMC5_vrom_bank[2], CHRROM);
				//                  state->nes_vram_sprite[2] = state->MMC5_vrom_bank[0] * 64;
				//                  vrom_next[2] = 6;
				//                  vrom_page_a = 1;
				//                  vrom_page_b = 0;
				break;
			}
			break;
		case 0x1023: /* $5123 */
			LOG_MMC(("MMC5 $5123 vrom select: %02x (mode: %d)\n", data, state->MMC5_vrom_bank_mode));
			switch (state->MMC5_vrom_bank_mode)
			{
			case 0x01:
				chr4_0(space->machine, data, CHRROM);
				break;
			case 0x02:
				/* 2k switch */
				chr2_2(space->machine, data | (state->mmc5_high_chr << 8), CHRROM);
				break;
			case 0x03:
				/* 1k switch */
				state->MMC5_vrom_bank[3] = data | (state->mmc5_high_chr << 8);
				//                  mapper5_sync_vrom(0);
				chr1_3(space->machine, state->MMC5_vrom_bank[3], CHRROM);
				//                  state->nes_vram_sprite[3] = state->MMC5_vrom_bank[0] * 64;
				//                  vrom_next[3] = 7;
				//                  vrom_page_a = 1;
				//                  vrom_page_b = 0;
				break;
			}
			break;
		case 0x1024: /* $5124 */
			LOG_MMC(("MMC5 $5124 vrom select: %02x (mode: %d)\n", data, state->MMC5_vrom_bank_mode));
			switch (state->MMC5_vrom_bank_mode)
			{
			case 0x03:
				/* 1k switch */
				state->MMC5_vrom_bank[4] = data | (state->mmc5_high_chr << 8);
				//                  mapper5_sync_vrom(0);
				chr1_4(space->machine, state->MMC5_vrom_bank[4], CHRROM);
				//                  state->nes_vram_sprite[4] = state->MMC5_vrom_bank[0] * 64;
				//                  vrom_next[0] = 0;
				//                  vrom_page_a = 0;
				//                  vrom_page_b = 0;
				break;
			}
			break;
		case 0x1025: /* $5125 */
			LOG_MMC(("MMC5 $5125 vrom select: %02x (mode: %d)\n", data, state->MMC5_vrom_bank_mode));
			switch (state->MMC5_vrom_bank_mode)
			{
			case 0x02:
				/* 2k switch */
				chr2_4(space->machine, data | (state->mmc5_high_chr << 8), CHRROM);
				break;
			case 0x03:
				/* 1k switch */
				state->MMC5_vrom_bank[5] = data | (state->mmc5_high_chr << 8);
				//                  mapper5_sync_vrom(0);
				chr1_5(space->machine, state->MMC5_vrom_bank[5], CHRROM);
				//                  state->nes_vram_sprite[5] = state->MMC5_vrom_bank[0] * 64;
				//                  vrom_next[1] = 1;
				//                  vrom_page_a = 0;
				//                  vrom_page_b = 0;
				break;
			}
			break;
		case 0x1026: /* $5126 */
			LOG_MMC(("MMC5 $5126 vrom select: %02x (mode: %d)\n", data, state->MMC5_vrom_bank_mode));
			switch (state->MMC5_vrom_bank_mode)
			{
			case 0x03:
				/* 1k switch */
				state->MMC5_vrom_bank[6] = data | (state->mmc5_high_chr << 8);
				//                  mapper5_sync_vrom(0);
				chr1_6(space->machine, state->MMC5_vrom_bank[6], CHRROM);
				//                  state->nes_vram_sprite[6] = state->MMC5_vrom_bank[0] * 64;
				//                  vrom_next[2] = 2;
				//                  vrom_page_a = 0;
				//                  vrom_page_b = 0;
				break;
			}
			break;
		case 0x1027: /* $5127 */
			LOG_MMC(("MMC5 $5127 vrom select: %02x (mode: %d)\n", data, state->MMC5_vrom_bank_mode));
			switch (state->MMC5_vrom_bank_mode)
			{
			case 0x00:
				/* 8k switch */
				chr8(space->machine, data, CHRROM);
				break;
			case 0x01:
				/* 4k switch */
				chr4_4(space->machine, data, CHRROM);
				break;
			case 0x02:
				/* 2k switch */
				chr2_6(space->machine, data | (state->mmc5_high_chr << 8), CHRROM);
				break;
			case 0x03:
				/* 1k switch */
				state->MMC5_vrom_bank[7] = data | (state->mmc5_high_chr << 8);
				//                  mapper5_sync_vrom(0);
				chr1_7(space->machine, state->MMC5_vrom_bank[7], CHRROM);
				//                  state->nes_vram_sprite[7] = state->MMC5_vrom_bank[0] * 64;
				//                  vrom_next[3] = 3;
				//                  vrom_page_a = 0;
				//                  vrom_page_b = 0;
				break;
			}
			break;
		case 0x1028: /* $5128 */
			LOG_MMC(("MMC5 $5128 vrom select: %02x (mode: %d)\n", data, state->MMC5_vrom_bank_mode));
			switch (state->MMC5_vrom_bank_mode)
			{
			case 0x03:
				/* 1k switch */
				state->MMC5_vrom_bank[8] = data | (state->mmc5_high_chr << 8);
				//                  nes_vram[vrom_next[0]] = data * 64;
				//                  nes_vram[0 + (vrom_page_a*4)] = data * 64;
				//                  nes_vram[0] = data * 64;
				chr1_4(space->machine, state->MMC5_vrom_bank[8], CHRROM);
				//                  mapper5_sync_vrom(1);
				if (!state->vrom_page_b)
				{
					state->vrom_page_a ^= 0x01;
					state->vrom_page_b = 1;
				}
				break;
			}
			break;
		case 0x1029: /* $5129 */
			LOG_MMC(("MMC5 $5129 vrom select: %02x (mode: %d)\n", data, state->MMC5_vrom_bank_mode));
			switch (state->MMC5_vrom_bank_mode)
			{
			case 0x02:
				/* 2k switch */
				chr2_0(space->machine, data | (state->mmc5_high_chr << 8), CHRROM);
				chr2_4(space->machine, data | (state->mmc5_high_chr << 8), CHRROM);
				break;
			case 0x03:
				/* 1k switch */
				state->MMC5_vrom_bank[9] = data | (state->mmc5_high_chr << 8);
				//                  nes_vram[vrom_next[1]] = data * 64;
				//                  nes_vram[1 + (vrom_page_a*4)] = data * 64;
				//                  nes_vram[1] = data * 64;
				chr1_5(space->machine, state->MMC5_vrom_bank[9], CHRROM);
				//                  mapper5_sync_vrom(1);
				if (!state->vrom_page_b)
				{
					state->vrom_page_a ^= 0x01;
					state->vrom_page_b = 1;
				}
				break;
			}
			break;
		case 0x102a: /* $512a */
			LOG_MMC(("MMC5 $512a vrom select: %02x (mode: %d)\n", data, state->MMC5_vrom_bank_mode));
			switch (state->MMC5_vrom_bank_mode)
			{
			case 0x03:
				/* 1k switch */
				state->MMC5_vrom_bank[10] = data | (state->mmc5_high_chr << 8);
				//                  nes_vram[vrom_next[2]] = data * 64;
				//                  nes_vram[2 + (vrom_page_a*4)] = data * 64;
				//                  nes_vram[2] = data * 64;
				chr1_6(space->machine, state->MMC5_vrom_bank[10], CHRROM);
				//                  mapper5_sync_vrom(1);
				if (!state->vrom_page_b)
				{
					state->vrom_page_a ^= 0x01;
					state->vrom_page_b = 1;
				}
				break;
			}
			break;
		case 0x102b: /* $512b */
			LOG_MMC(("MMC5 $512b vrom select: %02x (mode: %d)\n", data, state->MMC5_vrom_bank_mode));
			switch (state->MMC5_vrom_bank_mode)
			{
			case 0x00:
				/* 8k switch */
				/* switches in first half of an 8K bank!) */
				chr4_0(space->machine, data << 1, CHRROM);
				chr4_4(space->machine, data << 1, CHRROM);
				break;
			case 0x01:
				/* 4k switch */
				chr4_0(space->machine, data, CHRROM);
				chr4_4(space->machine, data, CHRROM);
				break;
			case 0x02:
				/* 2k switch */
				chr2_2(space->machine, data | (state->mmc5_high_chr << 8), CHRROM);
				chr2_6(space->machine, data | (state->mmc5_high_chr << 8), CHRROM);
				break;
			case 0x03:
				/* 1k switch */
				state->MMC5_vrom_bank[11] = data | (state->mmc5_high_chr << 8);
				//                  nes_vram[vrom_next[3]] = data * 64;
				//                  nes_vram[3 + (vrom_page_a*4)] = data * 64;
				//                  nes_vram[3] = data * 64;
				chr1_7(space->machine, state->MMC5_vrom_bank[11], CHRROM);
				//                  mapper5_sync_vrom(1);
				if (!state->vrom_page_b)
				{
					state->vrom_page_a ^= 0x01;
					state->vrom_page_b = 1;
				}
				break;
			}
			break;
			
		case 0x1030: /* $5130 */
			state->mmc5_high_chr = data & 0x03;
			if (state->mmc5_vram_control == 1)
			{
				// in this case state->mmc5_high_chr selects which 256KB of CHR ROM 
				// is to be used for all background tiles on the screen.
			}
			break;
			
			
		case 0x1100: /* $5200 */
			state->mmc5_split_scr = data;
			// in EX2 and EX3 modes, no split screen
			if (state->mmc5_vram_control & 0x02)
				state->mmc5_split_scr &= 0x7f;
			break;
			
		case 0x1103: /* $5203 */
			state->IRQ_count = data;
			state->MMC5_scanline = data;
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

/*************************************************************
 
 NTBROM board emulation
 
 if PRG > 128k -> SUNSOFT_DCS (additional handlers at 0x6000
 0x8000-0xbfff and 0xf000-0xffff), otherwise SUNSOFT_4
 
 iNES: mapper 68
 
 *************************************************************/

static void ntbrom_mirror( running_machine *machine, int mirror, int mirr0, int mirr1 )
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

static WRITE8_HANDLER( ntbrom_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	
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
			state->m0 = data & 0x7f;
			ntbrom_mirror(space->machine, state->m68_mirror, state->m0, state->m1);
			break;
		case 0x5000:
			state->m1 = data & 0x7f;
			ntbrom_mirror(space->machine, state->m68_mirror, state->m0, state->m1);
			break;
		case 0x6000:
			state->m68_mirror = data & 0x13;
			ntbrom_mirror(space->machine, state->m68_mirror, state->m0, state->m1);
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
 
 JxROM board emulation
 
 Sunsoft 5b / FME7 (5b = FME7 + sound chip)
 
 iNES: mapper 69
 
 *************************************************************/

/* Here, IRQ counter decrements every CPU cycle. Since we update it every scanline,
 we need to decrement it by 114 (Each scanline consists of 341 dots and, on NTSC,
 there are 3 dots to every 1 CPU cycle, hence 114 is the number of cycles per scanline ) */
static void jxrom_irq( running_device *device, int scanline, int vblank, int blanked )
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

static WRITE8_HANDLER( jxrom_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	LOG_MMC(("mapper69_w, offset %04x, data: %02x\n", offset, data));
	
	switch (offset & 0x6000)
	{
		case 0x0000:
			state->mmc_latch1 = data & 0x0f;
			break;
			
		case 0x2000:
			switch (state->mmc_latch1)
		{
			case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
				chr1_x(space->machine, state->mmc_latch1, data, CHRROM);
				break;
				
				/* TODO: deal properly with bankswitching/write-protecting the mid-mapper area */
			case 8:
				if (!(data & 0x40))
				{
					// is PRG ROM
					memory_unmap_write(space, 0x6000, 0x7fff, 0, 0);
					prg8_67(space->machine, data & 0x3f);
				}
				else if (data & 0x80)
				{
					// is PRG RAM
					memory_install_write_bank(space, 0x6000, 0x7fff, 0, 0, "bank5");
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
 
 NINA-001 board by AVE emulation
 
 iNES: mapper 34
 
 *************************************************************/

static WRITE8_HANDLER( nina01_m_w )
{
	LOG_MMC(("mapper34_m_w, offset: %04x, data: %02x\n", offset, data));
	
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


typedef void (*nes_ppu_latch)(running_device *device, offs_t offset);

struct nes_memory_accessor
{
	write8_space_func write;
	read8_space_func  read;
};

typedef struct _nes_pcb_intf  nes_pcb_intf;
struct _nes_pcb_intf
{
	int                     mmc_pcb;
	nes_memory_accessor     mmc_l;  /* $4100-$5fff read/write routines */
	nes_memory_accessor     mmc_m;  /* $6000-$7fff read/write routines */
	nes_memory_accessor     mmc_h;  /* $8000-$ffff read/write routines */
	nes_ppu_latch           mmc_ppu_latch;
	ppu2c0x_scanline_cb		mmc_scanline;
	ppu2c0x_hblank_cb		mmc_hblank;
};

#define NES_NOACCESS \
{NULL, NULL}

#define NES_READONLY(a) \
{NULL, a}

#define NES_WRITEONLY(a) \
{a, NULL}

static const nes_pcb_intf nes_intf_list[] =
{
	{ STD_NROM,             NES_NOACCESS, NES_NOACCESS, NES_NOACCESS,                         NULL, NULL, NULL },
	{ HVC_FAMBASIC,         NES_NOACCESS, NES_NOACCESS, NES_NOACCESS,                         NULL, NULL, NULL },
	{ GG_NROM,              NES_NOACCESS, NES_NOACCESS, NES_NOACCESS,                         NULL, NULL, NULL },
	{ STD_UXROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(uxrom_w),               NULL, NULL, NULL },
	{ STD_UN1ROM,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(un1rom_w),              NULL, NULL, NULL },
	{ STD_CPROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(cprom_w),               NULL, NULL, NULL },
	{ STD_CNROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(cnrom_w),               NULL, NULL, NULL },
	{ BANDAI_PT554,         NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(cnrom_w),               NULL, NULL, NULL },	
	{ STD_AXROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(axrom_w),               NULL, NULL, NULL },
	{ STD_PXROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(pxrom_w),               mmc2_latch, NULL, NULL },
	{ STD_FXROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(fxrom_w),               mmc2_latch, NULL, NULL },
	{ STD_BXROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(bxrom_w),               NULL, NULL, NULL },
	{ STD_GXROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(gxrom_w),               NULL, NULL, NULL },
	{ STD_MXROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(gxrom_w),               NULL, NULL, NULL },
	{ STD_NXROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(ntbrom_w),              NULL, NULL, NULL },
	{ STD_JXROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(jxrom_w),               NULL, NULL, jxrom_irq },
	{ STD_SXROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(sxrom_w),               NULL, NULL, NULL },
	{ STD_SXROM_A,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(sxrom_a_w),             NULL, NULL, NULL },
	{ STD_SOROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(sorom_w),               NULL, NULL, NULL },
	{ STD_TXROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(txrom_w),               NULL, NULL, mmc3_irq },
	{ STD_HKROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(hkrom_w),               NULL, NULL, mmc3_irq },
	{ STD_TQROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(tqrom_w),               NULL, NULL, mmc3_irq },
	{ STD_TXSROM,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(txsrom_w),              NULL, NULL, mmc3_irq },
	{ STD_DXROM,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(dxrom_w),               NULL, NULL, mmc3_irq },
	{ STD_EXROM,            {exrom_l_w, exrom_l_r}, NES_NOACCESS, NES_NOACCESS,               NULL, NULL, mmc5_irq },
	{ NES_QJ,               NES_NOACCESS, NES_WRITEONLY(qj_m_w), NES_WRITEONLY(txrom_w),      NULL, NULL, mmc3_irq },
	{ PAL_ZZ,               NES_NOACCESS, NES_WRITEONLY(zz_m_w), NES_WRITEONLY(txrom_w),      NULL, NULL, mmc3_irq },
	//
	{ DIS_74X139X74,        NES_NOACCESS, NES_WRITEONLY(mapper87_m_w), NES_NOACCESS,          NULL, NULL, NULL },
	{ DIS_74X377,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper11_w),            NULL, NULL, NULL },
	{ DIS_74X161X161X32,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper70_w),            NULL, NULL, NULL },
	{ DIS_74X161X138,       NES_NOACCESS, NES_WRITEONLY(mapper38_m_w), NES_NOACCESS,          NULL, NULL, NULL },
	{ AGCI_50282,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper144_w),           NULL, NULL, NULL },
	{ AVE_NINA01,           NES_NOACCESS, NES_WRITEONLY(nina01_m_w), NES_NOACCESS,            NULL, NULL, NULL },
	{ AVE_NINA06,           NES_WRITEONLY(mapper79_l_w), NES_NOACCESS, NES_NOACCESS,          NULL, NULL, NULL },
	{ BANDAI_LZ93EX,        NES_NOACCESS, NES_WRITEONLY(mapper16_m_w), NES_WRITEONLY(mapper16_w),   NULL, NULL, bandai_irq },
	{ BANDAI_FCG,           NES_NOACCESS, NES_WRITEONLY(mapper153_m_w), NES_WRITEONLY(mapper153_w),   NULL, NULL, bandai_irq },
	{ BANDAI_JUMP2,         NES_NOACCESS, NES_WRITEONLY(mapper153_m_w), NES_WRITEONLY(mapper153_w),   NULL, NULL, bandai_irq },
	{ BANDAI_DATACH,        NES_NOACCESS, NES_WRITEONLY(mapper16_m_w), NES_WRITEONLY(mapper16_w),   NULL, NULL, bandai_irq },
	{ BANDAI_KARAOKE,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper188_w),           NULL, NULL, NULL },
	{ BANDAI_OEKAKIDS,      NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper96_w),            NULL, NULL, NULL },
	{ CAMERICA_ALGN,        NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper71_w),            NULL, NULL, NULL },
	{ CAMERICA_9097,        NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper71_w),            NULL, NULL, NULL },
	{ CAMERICA_ALGQ,        NES_NOACCESS, NES_WRITEONLY(mapper232_w), NES_WRITEONLY(mapper232_w),      NULL, NULL, NULL },
	{ CAMERICA_GOLDENFIVE,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper104_w),           NULL, NULL, NULL },
	{ CNE_DECATHLON,        NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper244_w),           NULL, NULL, NULL },
	{ CNE_PSB,              NES_NOACCESS, NES_WRITEONLY(mapper246_m_w), NES_NOACCESS,         NULL, NULL, NULL },
	{ CNE_SHLZ,             NES_WRITEONLY(mapper240_l_w), NES_NOACCESS, NES_NOACCESS,         NULL, NULL, NULL },
	{ CONY_BOARD,           {mapper83_l_w, mapper83_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper83_w),      NULL, NULL, NULL },
	{ DREAMTECH_BOARD,      NES_WRITEONLY(dreamtech_l_w), NES_NOACCESS, NES_NOACCESS,         NULL, NULL, NULL },
	{ FUTUREMEDIA_BOARD,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper117_w),           NULL, NULL, mapper117_irq },
	{ GOUDER_37017,         {mapper208_l_w, mapper208_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper208_w),      NULL, NULL, mmc3_irq },
	{ HENGEDIANZI_BOARD,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper177_w),           NULL, NULL, NULL },
	{ HES_BOARD,            NES_WRITEONLY(mapper113_l_w), NES_NOACCESS, NES_NOACCESS,         NULL, NULL, NULL },
	{ HOSENKAN_BOARD,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper182_w),           NULL, NULL, mmc3_irq },
	{ IREM_G101,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper32_w),            NULL, NULL, NULL },
	{ IREM_LROG017,         NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper77_w),            NULL, NULL, NULL },
	{ IREM_H3001,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper65_w),            NULL, NULL, irem_irq },
	{ IREM_HOLYDIV,         NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper78_w),            NULL, NULL, NULL },
	{ JALECO_SS88006,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper18_w),            NULL, NULL, jaleco_irq },
	{ JALECO_JF11,          NES_NOACCESS, NES_WRITEONLY(mapper_140_m_w), NES_NOACCESS,        NULL, NULL, NULL },
	{ JALECO_JF13,          NES_NOACCESS, NES_WRITEONLY(mapper86_m_w), NES_NOACCESS,          NULL, NULL, NULL },
	{ JALECO_JF16,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper78_w),            NULL, NULL, NULL },
	{ JALECO_JF17,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper72_w),            NULL, NULL, NULL },
	{ JALECO_JF19,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper92_w),            NULL, NULL, NULL },
	{ KAISER_KS7058,        NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper171_w),           NULL, NULL, NULL },
	{ KAY_PANDAPRINCE,      {mapper121_l_w, mapper121_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper121_w), NULL, NULL, mmc3_irq },
	{ KASING_BOARD,         NES_NOACCESS, NES_WRITEONLY(mapper115_m_w), NES_WRITEONLY(mapper115_w),      NULL, NULL, mmc3_irq },
	{ KONAMI_VRC1,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper75_w),            NULL, NULL, NULL },
	{ KONAMI_VRC2,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(konami_vrc2a_w),        NULL, NULL, NULL },
	{ KONAMI_VRC3,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper73_w),            NULL, NULL, konami_irq },
	{ KONAMI_VRC4,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(konami_vrc4a_w),        NULL, NULL, konami_irq },
	{ KONAMI_VRC6,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(konami_vrc6a_w),        NULL, NULL, konami_irq },
	{ KONAMI_VRC7,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(konami_vrc7_w),         NULL, NULL, konami_irq },
	{ MAGICSERIES_MAGICDRAGON,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper107_w),     NULL, NULL, NULL },
	{ NAMCOT_163,           {mapper19_l_w, mapper19_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper19_w), NULL, NULL, namcot_irq },
	{ NAMCOT_3425,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper95_w),            NULL, NULL, NULL },
	{ NAMCOT_34X3,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper88_w),            NULL, NULL, NULL },
	{ NAMCOT_3446,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper76_w),            NULL, NULL, NULL },
	{ NITRA_TDA,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper250_w),           NULL, NULL, mmc3_irq },
	{ NTDEC_ASDER,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper112_w),           NULL, NULL, NULL },
	{ NTDEC_FIGHTINGHERO,   NES_NOACCESS, NES_WRITEONLY(mapper193_m_w), NES_NOACCESS,         NULL, NULL, NULL },
	{ OPENCORP_DAOU306,     NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper156_w),           NULL, NULL, NULL },
	{ RCM_GS2015,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper216_w),           NULL, NULL, NULL },
	{ RCM_TETRISFAMILY,     NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper61_w),            NULL, NULL, NULL },
	{ REXSOFT_DBZ5,         {mapper12_l_w, mapper12_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper12_w),      NULL, NULL, mmc3_irq },
	{ REXSOFT_SL1632,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper14_w),            NULL, NULL, mmc3_irq },
	{ RUMBLESTATION_BOARD,  NES_NOACCESS, NES_WRITEONLY(mapper46_m_w), NES_WRITEONLY(mapper46_w),      NULL, NULL, NULL },
	{ SACHEN_74LS374,       {mapper150_l_w, mapper150_l_r}, NES_WRITEONLY(mapper150_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_8259A,         NES_WRITEONLY(mapper141_l_w), NES_WRITEONLY(mapper141_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_8259B,         NES_WRITEONLY(mapper138_l_w), NES_WRITEONLY(mapper138_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_8259C,         NES_WRITEONLY(mapper139_l_w), NES_WRITEONLY(mapper139_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_8259D,         NES_WRITEONLY(mapper137_l_w), NES_WRITEONLY(mapper137_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_SA0036,        NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper149_w),           NULL, NULL, NULL },
	{ SACHEN_SA0037,        NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper148_w),           NULL, NULL, NULL },
	{ SACHEN_SA72007,       NES_WRITEONLY(mapper145_l_w), NES_NOACCESS, NES_NOACCESS,         NULL, NULL, NULL },
	{ SACHEN_SA72008,       NES_WRITEONLY(mapper133_l_w), NES_WRITEONLY(mapper133_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_TCA01,         NES_READONLY(mapper143_l_r), NES_NOACCESS, NES_NOACCESS,          NULL, NULL, NULL },
	{ SACHEN_TCU01,         NES_WRITEONLY(mapper147_l_w), NES_WRITEONLY(mapper147_m_w), NES_WRITEONLY(mapper147_w),      NULL, NULL, NULL },
	{ SACHEN_TCU02,         {mapper136_l_w, mapper136_l_r}, NES_NOACCESS, NES_NOACCESS,       NULL, NULL, NULL },
	{ SUBOR_TYPE0,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper167_w),           NULL, NULL, NULL },
	{ SUBOR_TYPE1,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper166_w),           NULL, NULL, NULL },
	{ SUNSOFT_1,            NES_NOACCESS, NES_WRITEONLY(mapper184_m_w), NES_NOACCESS,         NULL, NULL, NULL },
	{ SUNSOFT_2,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper89_w),            NULL, NULL, NULL },
	{ SUNSOFT_3,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper67_w),            NULL, NULL, mapper67_irq },
	{ SUPERGAME_BOOGERMAN,  NES_WRITEONLY(mapper215_l_w), NES_NOACCESS, NES_WRITEONLY(mapper215_w),      NULL, NULL, mmc3_irq },
	{ SUPERGAME_LIONKING,   NES_NOACCESS, NES_WRITEONLY(mapper114_m_w), NES_WRITEONLY(mapper114_w),      NULL, NULL, mmc3_irq },
	{ TAITO_TC0190FMC,      NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper33_w),            NULL, NULL, NULL },
	{ TAITO_TC0190FMCP,     NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper48_w),            NULL, NULL, mmc3_irq },
	{ TAITO_X1005,          NES_NOACCESS, NES_WRITEONLY(mapper80_m_w), NES_NOACCESS,          NULL, NULL, NULL },
	{ TAITO_X1017,          NES_NOACCESS, NES_WRITEONLY(mapper82_m_w), NES_NOACCESS,          NULL, NULL, NULL },
	{ TENGEN_800032,        NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper64_w),            NULL, NULL, mapper64_irq },
	{ TENGEN_800037,        NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper158_w),           NULL, NULL, mapper64_irq },
	{ TXC_22211A,           {mapper132_l_w, mapper132_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper132_w),      NULL, NULL, NULL }, 
	{ TXC_TW,               NES_WRITEONLY(mapper189_l_w), NES_WRITEONLY(mapper189_m_w), NES_WRITEONLY(mapper189_w),      NULL, NULL, mmc3_irq },
	{ TXC_MXMDHTWO,         NES_READONLY(mapper241_l_r), NES_NOACCESS, NES_WRITEONLY(mapper241_w),      NULL, NULL, NULL },
	{ UNL_8237,             NES_WRITEONLY(unl_8237_l_w), NES_NOACCESS, NES_WRITEONLY(unl_8237_w),      NULL, NULL, mmc3_irq },
	{ UNL_AX5705,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(unl_ax5705_w),          NULL, NULL, NULL },
	{ UNL_CC21,             NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(unl_cc21_w),            NULL, NULL, NULL },
	{ UNL_KOF96,            {mapper187_l_w, mapper187_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper187_w),      NULL, NULL, mmc3_irq },
 	{ UNL_KOF97,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(unl_kof97_w),           NULL, NULL, mmc3_irq },
	{ UNL_MK2,              NES_NOACCESS, NES_WRITEONLY(mapper91_m_w), NES_NOACCESS,          NULL, NULL, mmc3_irq },
	{ UNL_N625092,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper221_w),           NULL, NULL, NULL },
	{ UNL_SC127,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper35_w),            NULL, NULL, mapper35_irq },
	{ UNL_SMB2J,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper43_w),            NULL, NULL, NULL },
	{ UNL_SUPERFIGHTER3,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper197_w),           NULL, NULL, mmc3_irq },
 	{ UNL_T230,             NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(unl_t230_w),            NULL, NULL, konami_irq },
	{ UNL_XZY,              NES_WRITEONLY(mapper176_l_w), NES_NOACCESS, NES_NOACCESS,         NULL, NULL, NULL },
	{ WAIXING_TYPE_A,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper74_w),            NULL, NULL, mmc3_irq },
	{ WAIXING_TYPE_B,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper191_w),           NULL, NULL, mmc3_irq },
	{ WAIXING_TYPE_C,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper192_w),           NULL, NULL, mmc3_irq },
	{ WAIXING_TYPE_D,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper194_w),           NULL, NULL, mmc3_irq },
	{ WAIXING_TYPE_E,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper195_w),           NULL, NULL, mmc3_irq },
	{ WAIXING_TYPE_F,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper198_w),           NULL, NULL, mmc3_irq },
	{ WAIXING_TYPE_G,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper199_w),           NULL, NULL, mmc3_irq },
	{ WAIXING_TYPE_H,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper245_w),           NULL, NULL, mmc3_irq },
	{ WAIXING_SGZ,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper252_w),           NULL, NULL, konami_irq },
	{ WAIXING_SGZLZ,        NES_WRITEONLY(mapper178_l_w), NES_NOACCESS, NES_NOACCESS,         NULL, NULL, NULL },
	{ WAIXING_SECURITY,     NES_WRITEONLY(mapper249_l_w), NES_NOACCESS, NES_WRITEONLY(mapper249_w),      NULL, NULL, mmc3_irq },
	{ WAIXING_FFV,          NES_WRITEONLY(mapper164_l_w), NES_NOACCESS, NES_WRITEONLY(mapper164_w),      NULL, NULL, NULL },
	{ WAIXING_PS2,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper15_w),            NULL, NULL, NULL },
	{ WAIXING_ZS,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper242_w),           NULL, NULL, NULL },
	{ BTL_AISENSHINICOL,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper42_w),            NULL, NULL, NULL },
	{ BTL_DRAGONNINJA,      NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper222_w),           NULL, NULL, mapper222_irq },
	{ BTL_MARIOBABY,        NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper42_w),            NULL, NULL, NULL },
	{ BTL_SMB2A,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper40_w),            NULL, NULL, mapper40_irq },
	{ BTL_SMB2B,            NES_WRITEONLY(mapper50_l_w), NES_NOACCESS, NES_NOACCESS,          NULL, NULL, mapper50_irq },
	{ BTL_SMB3,             NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper106_w),           NULL, NULL, mapper106_irq },
	{ BTL_SUPERBROS11,      NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper196_w),           NULL, NULL, mmc3_irq },
	{ ACTENT_ACT52,         NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper228_w),           NULL, NULL, NULL },
	{ CALTRON_6IN1,         NES_NOACCESS, NES_WRITEONLY(mapper41_m_w), NES_WRITEONLY(mapper41_w),      NULL, NULL, NULL },
	{ BMC_190IN1,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(bmc_190in1_w),          NULL, NULL, NULL },
	{ BMC_A65AS,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(bmc_a65as_w),           NULL, NULL, NULL },
	{ BMC_GS2004,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(bmc_gs2004_w),          NULL, NULL, NULL },
	{ BMC_GS2013,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(bmc_gs2013_w),          NULL, NULL, NULL },
	{ BMC_NOVELDIAMOND,     NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper54_w),            NULL, NULL, NULL },
	{ BMC_T262,             NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(bmc_t262_w),            NULL, NULL, NULL },
	{ BMC_WS,               NES_NOACCESS, NES_WRITEONLY(bmc_ws_m_w), NES_NOACCESS,            NULL, NULL, NULL },
	{ BMC_GKA,              NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper57_w),            NULL, NULL, NULL },
	{ BMC_GKB,              NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper58_w),            NULL, NULL, NULL },
	{ BMC_SUPER_700IN1,     NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper62_w),            NULL, NULL, NULL },
	{ BMC_36IN1,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper200_w),           NULL, NULL, NULL },
	{ BMC_21IN1,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper201_w),           NULL, NULL, NULL },
	{ BMC_150IN1,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper202_w),           NULL, NULL, NULL },
	{ BMC_35IN1,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper203_w),           NULL, NULL, NULL },
	{ BMC_64IN1,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper204_w),           NULL, NULL, NULL },
	{ BMC_SUPERHIK_300IN1,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper212_w),           NULL, NULL, NULL },
	{ BMC_9999999IN1,       NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper213_w),           NULL, NULL, NULL },
	{ BMC_SUPERGUN_20IN1,   NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper214_w),           NULL, NULL, NULL },
	{ BMC_72IN1,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper225_w),           NULL, NULL, NULL },
	{ BMC_76IN1,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper226_w),           NULL, NULL, NULL },
	{ BMC_SUPER_42IN1,      NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper226_w),           NULL, NULL, NULL },
	{ BMC_1200IN1,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper227_w),           NULL, NULL, NULL },
	{ BMC_31IN1,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper229_w),           NULL, NULL, NULL },
	{ BMC_22GAMES,          NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper230_w),           NULL, NULL, NULL },
	{ BMC_20IN1,            NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper231_w),           NULL, NULL, NULL },
	{ BMC_110IN1,           NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper255_w),           NULL, NULL, NULL },
	{ BMC_64IN1NR,          NES_WRITEONLY(bmc_64in1nr_l_w), NES_NOACCESS, NES_WRITEONLY(bmc_64in1nr_w),      NULL, NULL, NULL },
	{ BMC_S24IN1SC03,       NES_WRITEONLY(bmc_s24in1sc03_l_w), NES_NOACCESS, NES_WRITEONLY(bmc_s24in1sc03_w),      NULL, NULL, mmc3_irq },
	{ BMC_HIK8IN1,          NES_NOACCESS, NES_WRITEONLY(mapper45_m_w), NES_WRITEONLY(txrom_w),      NULL, NULL, mmc3_irq },
	{ BMC_SUPERHIK_4IN1,    NES_NOACCESS, NES_WRITEONLY(mapper49_m_w), NES_WRITEONLY(txrom_w),      NULL, NULL, mmc3_irq },
	{ BMC_SUPERBIG_7IN1,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper44_w),            NULL, NULL, mmc3_irq },
	{ BMC_MARIOPARTY_7IN1,  NES_NOACCESS, NES_WRITEONLY(mapper52_m_w), NES_WRITEONLY(txrom_w),      NULL, NULL, mmc3_irq },
	{ BMC_FAMILY_4646B,     NES_NOACCESS, NES_WRITEONLY(mapper134_m_w), NES_WRITEONLY(txrom_w),      NULL, NULL, mmc3_irq },
	{ BMC_15IN1,            NES_NOACCESS, NES_WRITEONLY(mapper205_m_w), NES_WRITEONLY(txrom_w),      NULL, NULL, mmc3_irq },
	{ BMC_BALLGAMES_11IN1,  NES_NOACCESS, NES_WRITEONLY(mapper51_m_w), NES_WRITEONLY(mapper51_w),      NULL, NULL, NULL },
	{ BMC_GOLDENCARD_6IN1,  NES_WRITEONLY(mapper217_l_w), NES_NOACCESS, NES_WRITEONLY(mapper217_w),      NULL, NULL, mmc3_irq },
	{ UNSUPPORTED_BOARD,    NES_NOACCESS, NES_NOACCESS, NES_NOACCESS,                         NULL, NULL, NULL },
};


const nes_pcb_intf *nes_pcb_intf_lookup( int pcb_id )
{
	int i;
	for (i = 0; i < ARRAY_LENGTH(nes_intf_list); i++)
	{
		if (nes_intf_list[i].mmc_pcb == pcb_id)
			return &nes_intf_list[i];
	}
	return NULL;
}

int nes_pcb_reset( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	int err = 0;
	const nes_pcb_intf *intf = nes_pcb_intf_lookup(state->pcb_id);
	
	if (intf == NULL)
		fatalerror("Missing PCB interface\n");
	
	if (state->chr_chunks == 0)
		chr8(machine, 0, CHRRAM);
	else
		chr8(machine, 0, CHRROM);
	
	/* Set the mapper irq callback */
	ppu2c0x_set_scanline_callback(state->ppu, intf ? intf->mmc_scanline : NULL);
	ppu2c0x_set_hblank_callback(state->ppu, intf ? intf->mmc_hblank : NULL);
	
	err = unif_initialize(machine, state->pcb_id);
	
	return err;
}

int nes_get_pcb_id( running_machine *machine, const char *feature )
{
	const nes_pcb *pcb = nes_pcb_lookup(feature);

	if (pcb == NULL)
		fatalerror("Unimplemented PCB\n");
	
	return pcb->pcb_id;
}

void pcb_handlers_setup( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	const nes_pcb_intf *intf = nes_pcb_intf_lookup(state->pcb_id);
	
	if (intf == NULL)
		fatalerror("Missing PCB interface\n");
	
	if (intf)
	{
		state->mmc_write_low = intf->mmc_l.write;
		state->mmc_write_mid = intf->mmc_m.write;
		state->mmc_write = intf->mmc_h.write;
		state->mmc_read_low = intf->mmc_l.read;
		state->mmc_read_mid = NULL;	// in progress
		state->mmc_read = NULL;	// in progress
		ppu_latch = intf->mmc_ppu_latch;
	}
	else
	{
		logerror("PCB %d is not yet supported, defaulting to no mapper.\n", state->pcb_id);
		state->mmc_write_low = NULL;
		state->mmc_write_mid = NULL;
		state->mmc_write = NULL;
		state->mmc_read_low = NULL;
		state->mmc_read_mid = NULL;	// in progress
		state->mmc_read = NULL;	// in progress
		ppu_latch = NULL;
	}
}

