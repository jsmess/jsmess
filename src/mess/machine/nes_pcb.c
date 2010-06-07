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
	{ "HVC-SOROM",        STD_SXROM },
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
	{ "NES-SOROM",        STD_SXROM },
	{ "NES-SUROM",        STD_SXROM },
	{ "NES-SXROM",        STD_SXROM },
	{ "NES-WH",           STD_SXROM },
	/* SxROM boards by other manufacturer */
	{ "KONAMI-SLROM",     STD_SXROM },
	{ "VIRGIN-SNROM",     STD_SXROM },
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
	{ "PAL-ZZ",           NES_ZZ },
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

// Here, we take the PCB ID and we assign handlers and callbcks needed by emulation (.nes and .unf should use this as well, after a first lookup to pass from mapper/board to PCB ID)
static const nes_pcb_intf nes_intf_list[] =
{
	{ STD_NROM,     NES_NOACCESS, NES_NOACCESS, NES_NOACCESS,                  NULL, NULL, NULL },
	{ HVC_FAMBASIC, NES_NOACCESS, NES_NOACCESS, NES_NOACCESS,                  NULL, NULL, NULL },
	{ GG_NROM,      NES_NOACCESS, NES_NOACCESS, NES_NOACCESS,                  NULL, NULL, NULL },
	{ STD_UXROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper2_w),      NULL, NULL, NULL },
	{ STD_CNROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper3_w),      NULL, NULL, NULL },
	{ BANDAI_PT554, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper3_w),      NULL, NULL, NULL },	
	{ STD_AXROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper7_w),      NULL, NULL, NULL },
	{ STD_PXROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper9_w),      mapper9_latch, NULL, NULL },
	{ STD_FXROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper10_w),     mapper9_latch, NULL, NULL },
	{ STD_BXROM,    NES_NOACCESS, NES_WRITEONLY(mapper34_m_w), NES_WRITEONLY(mapper34_w),     NULL, NULL, NULL },
	{ STD_CPROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper13_w),     NULL, NULL, NULL },
	{ STD_GXROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper66_w),     NULL, NULL, NULL },
	{ STD_MXROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper66_w),     NULL, NULL, NULL },
	{ STD_NXROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper68_w),     NULL, NULL, NULL },
	{ STD_JXROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper69_w),     NULL, NULL, mapper69_irq },
	{ STD_UN1ROM,   NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper94_w),     NULL, NULL, NULL },
	{ STD_SXROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper1_w),      NULL, NULL, NULL },
	{ STD_TXROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper4_w),      NULL, NULL, mapper4_irq },
	{ STD_HKROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper4_w),      NULL, NULL, mapper4_irq },
	{ STD_TQROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper119_w),    NULL, NULL, mapper4_irq },
	{ STD_TXSROM,   NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper118_w),    NULL, NULL, mapper4_irq },
	{ STD_DXROM,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper206_w),    NULL, NULL, mapper4_irq },
	{ STD_EXROM,    {mapper5_l_w, mapper5_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper5_w),    NULL, NULL, mapper5_irq },
	{ NES_ZZ,       NES_NOACCESS, NES_WRITEONLY(mapper37_m_w), NES_WRITEONLY(mapper4_w),      NULL, NULL, mapper4_irq },
	{ NES_QJ,       NES_NOACCESS, NES_WRITEONLY(mapper47_m_w), NES_WRITEONLY(mapper4_w),      NULL, NULL, mapper4_irq },
	{ DIS_74X139X74,  NES_NOACCESS, NES_WRITEONLY(mapper87_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ DIS_74X377,   NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper11_w),     NULL, NULL, NULL },
	{ DIS_74X161X161X32, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper70_w),     NULL, NULL, NULL },
	{ DIS_74X161X138,  NES_NOACCESS, NES_WRITEONLY(mapper38_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ AGCI_50282,   NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper144_w),      NULL, NULL, NULL },
	{ AVE_NINA01,   NES_NOACCESS, NES_WRITEONLY(mapper34_m_w), NES_WRITEONLY(mapper34_w),      NULL, NULL, NULL },
	{ AVE_NINA06,   NES_WRITEONLY(mapper79_l_w), NES_NOACCESS, NES_NOACCESS,   NULL, NULL, NULL },
	{ BANDAI_LZ93EX, NES_NOACCESS, NES_WRITEONLY(mapper16_m_w), NES_WRITEONLY(mapper16_w),   NULL, NULL, bandai_irq },
	{ BANDAI_FCG,   NES_NOACCESS, NES_WRITEONLY(mapper153_m_w), NES_WRITEONLY(mapper153_w),   NULL, NULL, bandai_irq },
	{ BANDAI_JUMP2, NES_NOACCESS, NES_WRITEONLY(mapper153_m_w), NES_WRITEONLY(mapper153_w),   NULL, NULL, bandai_irq },
	{ BANDAI_DATACH, NES_NOACCESS, NES_WRITEONLY(mapper16_m_w), NES_WRITEONLY(mapper16_w),   NULL, NULL, bandai_irq },
	{ BANDAI_KARAOKE, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper188_w),     NULL, NULL, NULL },
	{ BANDAI_OEKAKIDS, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper96_w),     NULL, NULL, NULL },
	{ CAMERICA_ALGN,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper71_w),      NULL, NULL, NULL },
	{ CAMERICA_9097,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper71_w),      NULL, NULL, NULL },
	{ CAMERICA_ALGQ,    NES_NOACCESS, NES_WRITEONLY(mapper232_w), NES_WRITEONLY(mapper232_w),      NULL, NULL, NULL },
	{ CAMERICA_GOLDENFIVE, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper104_w),      NULL, NULL, NULL },
	{ CNE_DECATHLON, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper244_w),      NULL, NULL, NULL },
	{ CNE_PSB, NES_NOACCESS, NES_WRITEONLY(mapper246_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ CNE_SHLZ, NES_WRITEONLY(mapper240_l_w), NES_NOACCESS, NES_NOACCESS,      NULL, NULL, NULL },
	{ CONY_BOARD, {mapper83_l_w, mapper83_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper83_w),      NULL, NULL, NULL },
	{ DREAMTECH_BOARD, NES_WRITEONLY(dreamtech_l_w), NES_NOACCESS, NES_NOACCESS,      NULL, NULL, NULL },
	{ FUTUREMEDIA_BOARD, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper117_w),      NULL, NULL, mapper117_irq },
	{ GOUDER_37017, {mapper208_l_w, mapper208_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper208_w),      NULL, NULL, mapper4_irq },
	{ HENGEDIANZI_BOARD, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper177_w),     NULL, NULL, NULL },
	{ HES_BOARD, NES_WRITEONLY(mapper113_l_w), NES_NOACCESS, NES_NOACCESS,      NULL, NULL, NULL },
	{ HOSENKAN_BOARD, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper182_w),      NULL, NULL, mapper4_irq },
	{ IREM_G101,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper32_w),     NULL, NULL, NULL },
	{ IREM_LROG017, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper77_w),     NULL, NULL, NULL },
	{ IREM_H3001,   NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper65_w),     NULL, NULL, irem_irq },
	{ IREM_HOLYDIV, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper78_w),     NULL, NULL, NULL },
	{ JALECO_SS88006, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper18_w),      NULL, NULL, jaleco_irq },
	{ JALECO_JF11,  NES_NOACCESS, NES_WRITEONLY(mapper_140_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ JALECO_JF13,  NES_NOACCESS, NES_WRITEONLY(mapper86_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ JALECO_JF16,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper78_w),     NULL, NULL, NULL },
	{ JALECO_JF17,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper72_w),     NULL, NULL, NULL },
	{ JALECO_JF19,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper92_w),     NULL, NULL, NULL },
	{ KAISER_KS7058, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper171_w),     NULL, NULL, NULL },
	{ KAY_PANDAPRINCE, {mapper121_l_w, mapper121_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper121_w), NULL, NULL, mapper4_irq },
	{ KASING_BOARD, NES_NOACCESS, NES_WRITEONLY(mapper115_m_w), NES_WRITEONLY(mapper115_w),      NULL, NULL, mapper4_irq },
	{ KONAMI_VRC1,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper75_w),     NULL, NULL, NULL },
	{ KONAMI_VRC2,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(konami_vrc2a_w),     NULL, NULL, NULL },
	{ KONAMI_VRC3,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper73_w),     NULL, NULL, konami_irq },
	{ KONAMI_VRC4,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(konami_vrc4a_w),     NULL, NULL, konami_irq },
	{ KONAMI_VRC6,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(konami_vrc6a_w),     NULL, NULL, konami_irq },
	{ KONAMI_VRC7,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(konami_vrc7_w),     NULL, NULL, konami_irq },
	{ MAGICSERIES_MAGICDRAGON,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper107_w),      NULL, NULL, NULL },
	{ NAMCOT_163,   {mapper19_l_w, mapper19_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper19_w), NULL, NULL, namcot_irq },
	{ NAMCOT_3425,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper95_w),      NULL, NULL, NULL },
	{ NAMCOT_34X3,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper88_w),      NULL, NULL, NULL },
	{ NAMCOT_3446,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper76_w),      NULL, NULL, NULL },
	{ NITRA_TDA, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper250_w),      NULL, NULL, mapper4_irq },
	{ NTDEC_ASDER,  NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper112_w),      NULL, NULL, NULL },
	{ NTDEC_FIGHTINGHERO,  NES_NOACCESS, NES_WRITEONLY(mapper193_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ OPENCORP_DAOU306,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper156_w),      NULL, NULL, NULL },
	{ RCM_GS2015,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper216_w),      NULL, NULL, NULL },
	{ RCM_TETRISFAMILY,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper61_w),      NULL, NULL, NULL },
	{ REXSOFT_DBZ5, {mapper12_l_w, mapper12_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper12_w),      NULL, NULL, mapper4_irq },
	{ REXSOFT_SL1632, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper14_w),      NULL, NULL, mapper4_irq },
	{ RUMBLESTATION_BOARD,       NES_NOACCESS, NES_WRITEONLY(mapper46_m_w), NES_WRITEONLY(mapper46_w),      NULL, NULL, NULL },
	{ SACHEN_74LS374, {mapper150_l_w, mapper150_l_r}, NES_WRITEONLY(mapper150_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_8259A, NES_WRITEONLY(mapper141_l_w), NES_WRITEONLY(mapper141_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_8259B, NES_WRITEONLY(mapper138_l_w), NES_WRITEONLY(mapper138_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_8259C, NES_WRITEONLY(mapper139_l_w), NES_WRITEONLY(mapper139_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_8259D, NES_WRITEONLY(mapper137_l_w), NES_WRITEONLY(mapper137_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_SA0036,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper149_w),      NULL, NULL, NULL },
	{ SACHEN_SA0037,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper148_w),      NULL, NULL, NULL },
	{ SACHEN_SA72007,    NES_WRITEONLY(mapper145_l_w), NES_NOACCESS, NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_SA72008,    NES_WRITEONLY(mapper133_l_w), NES_WRITEONLY(mapper133_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_TCA01,    NES_READONLY(mapper143_l_r), NES_NOACCESS, NES_NOACCESS,      NULL, NULL, NULL },
	{ SACHEN_TCU01, NES_WRITEONLY(mapper147_l_w), NES_WRITEONLY(mapper147_m_w), NES_WRITEONLY(mapper147_w),      NULL, NULL, NULL },
	{ SACHEN_TCU02, {mapper136_l_w, mapper136_l_r}, NES_NOACCESS, NES_NOACCESS,      NULL, NULL, NULL },
	{ SUBOR_TYPE0,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper167_w),      NULL, NULL, NULL },
	{ SUBOR_TYPE1,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper166_w),      NULL, NULL, NULL },
	{ SUNSOFT_1,    NES_NOACCESS, NES_WRITEONLY(mapper184_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ SUNSOFT_2,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper89_w),      NULL, NULL, NULL },
	{ SUNSOFT_3,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper67_w),      NULL, NULL, mapper67_irq },
	{ SUPERGAME_BOOGERMAN,  NES_WRITEONLY(mapper215_l_w), NES_NOACCESS, NES_WRITEONLY(mapper215_w),      NULL, NULL, mapper4_irq },
	{ SUPERGAME_LIONKING,  NES_NOACCESS, NES_WRITEONLY(mapper114_m_w), NES_WRITEONLY(mapper114_w),      NULL, NULL, mapper4_irq },
	{ TAITO_TC0190FMC, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper33_w),      NULL, NULL, NULL },
	{ TAITO_TC0190FMCP, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper48_w),      NULL, NULL, mapper4_irq },
	{ TAITO_X1005,  NES_NOACCESS, NES_WRITEONLY(mapper80_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ TAITO_X1017,  NES_NOACCESS, NES_WRITEONLY(mapper82_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ TENGEN_800032, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper64_w),      NULL, NULL, mapper64_irq },
	{ TENGEN_800037, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper158_w),      NULL, NULL, mapper64_irq },
	{ TXC_22211A,    {mapper132_l_w, mapper132_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper132_w),      NULL, NULL, NULL }, 
	{ TXC_TW,        NES_WRITEONLY(mapper189_l_w), NES_WRITEONLY(mapper189_m_w), NES_WRITEONLY(mapper189_w),      NULL, NULL, mapper4_irq },
	{ TXC_MXMDHTWO,   NES_READONLY(mapper241_l_r), NES_NOACCESS, NES_WRITEONLY(mapper241_w),      NULL, NULL, NULL },
	{ UNL_8237, NES_WRITEONLY(unl_8237_l_w), NES_NOACCESS, NES_WRITEONLY(unl_8237_w),      NULL, NULL, mapper4_irq },
	{ UNL_AX5705,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(unl_ax5705_w),      NULL, NULL, NULL },
	{ UNL_CC21,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(unl_cc21_w),      NULL, NULL, NULL },
	{ UNL_KOF96, {mapper187_l_w, mapper187_l_r}, NES_NOACCESS, NES_WRITEONLY(mapper187_w),      NULL, NULL, mapper4_irq },
 	{ UNL_KOF97, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(unl_kof97_w),      NULL, NULL, mapper4_irq },
	{ UNL_MK2, NES_NOACCESS, NES_WRITEONLY(mapper91_m_w), NES_NOACCESS,      NULL, NULL, mapper4_irq },
	{ UNL_N625092,   NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper221_w),      NULL, NULL, NULL },
	{ UNL_SC127, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper35_w),      NULL, NULL, mapper35_irq },
	{ UNL_SMB2J,   NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper43_w),      NULL, NULL, NULL },
	{ UNL_SUPERFIGHTER3, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper197_w),      NULL, NULL, mapper4_irq },
 	{ UNL_T230, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(unl_t230_w),      NULL, NULL, konami_irq },
	{ UNL_XZY,   NES_WRITEONLY(mapper176_l_w), NES_NOACCESS, NES_NOACCESS,      NULL, NULL, NULL },
	{ WAIXING_TYPE_A, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper74_w),      NULL, NULL, mapper4_irq },
	{ WAIXING_TYPE_B, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper191_w),      NULL, NULL, mapper4_irq },
	{ WAIXING_TYPE_C, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper192_w),      NULL, NULL, mapper4_irq },
	{ WAIXING_TYPE_D, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper194_w),      NULL, NULL, mapper4_irq },
	{ WAIXING_TYPE_E, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper195_w),      NULL, NULL, mapper4_irq },
	{ WAIXING_TYPE_F, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper198_w),      NULL, NULL, mapper4_irq },
	{ WAIXING_TYPE_G, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper199_w),      NULL, NULL, mapper4_irq },
	{ WAIXING_TYPE_H, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper245_w),      NULL, NULL, mapper4_irq },
	{ WAIXING_SGZ, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper252_w),      NULL, NULL, konami_irq },
	{ WAIXING_SGZLZ, NES_WRITEONLY(mapper178_l_w), NES_NOACCESS, NES_NOACCESS,      NULL, NULL, NULL },
	{ WAIXING_SECURITY, NES_WRITEONLY(mapper249_l_w), NES_NOACCESS, NES_WRITEONLY(mapper249_w),      NULL, NULL, mapper4_irq },
	{ WAIXING_FFV, NES_WRITEONLY(mapper164_l_w), NES_NOACCESS, NES_WRITEONLY(mapper164_w),      NULL, NULL, NULL },
	{ WAIXING_PS2, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper15_w),      NULL, NULL, NULL },
	{ WAIXING_ZS, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper242_w),      NULL, NULL, NULL },
	{ BTL_AISENSHINICOL,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper42_w),      NULL, NULL, NULL },
	{ BTL_DRAGONNINJA, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper222_w),      NULL, NULL, mapper222_irq },
	{ BTL_MARIOBABY,    NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper42_w),      NULL, NULL, NULL },
	{ BTL_SMB2A, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper40_w),      NULL, NULL, mapper40_irq },
	{ BTL_SMB2B, NES_WRITEONLY(mapper50_l_w), NES_NOACCESS, NES_NOACCESS,      NULL, NULL, mapper50_irq },
	{ BTL_SMB3, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper106_w),      NULL, NULL, mapper106_irq },
	{ BTL_SUPERBROS11, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper196_w),      NULL, NULL, mapper4_irq },
	{ ACTENT_ACT52, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper228_w),      NULL, NULL, NULL },
	{ CALTRON_6IN1, NES_NOACCESS, NES_WRITEONLY(mapper41_m_w), NES_WRITEONLY(mapper41_w),      NULL, NULL, NULL },
	{ BMC_190IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(bmc_190in1_w),      NULL, NULL, NULL },
	{ BMC_A65AS, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(bmc_a65as_w),      NULL, NULL, NULL },
	{ BMC_GS2004, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(bmc_gs2004_w),      NULL, NULL, NULL },
	{ BMC_GS2013, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(bmc_gs2013_w),      NULL, NULL, NULL },
	{ BMC_NOVELDIAMOND, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper54_w),      NULL, NULL, NULL },
	{ BMC_T262, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(bmc_t262_w),      NULL, NULL, NULL },
	{ BMC_WS, NES_NOACCESS, NES_WRITEONLY(bmc_ws_m_w), NES_NOACCESS,      NULL, NULL, NULL },
	{ BMC_GKA, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper57_w),      NULL, NULL, NULL },
	{ BMC_GKB, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper58_w),      NULL, NULL, NULL },
	{ BMC_SUPER_700IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper62_w),      NULL, NULL, NULL },
	{ BMC_36IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper200_w),      NULL, NULL, NULL },
	{ BMC_21IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper201_w),      NULL, NULL, NULL },
	{ BMC_150IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper202_w),      NULL, NULL, NULL },
	{ BMC_35IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper203_w),      NULL, NULL, NULL },
	{ BMC_64IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper204_w),      NULL, NULL, NULL },
	{ BMC_SUPERHIK_300IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper212_w),      NULL, NULL, NULL },
	{ BMC_9999999IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper213_w),      NULL, NULL, NULL },
	{ BMC_SUPERGUN_20IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper214_w),      NULL, NULL, NULL },
	{ BMC_72IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper225_w),      NULL, NULL, NULL },
	{ BMC_76IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper226_w),      NULL, NULL, NULL },
	{ BMC_SUPER_42IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper226_w),      NULL, NULL, NULL },
	{ BMC_1200IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper227_w),      NULL, NULL, NULL },
	{ BMC_31IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper229_w),      NULL, NULL, NULL },
	{ BMC_22GAMES, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper230_w),      NULL, NULL, NULL },
	{ BMC_20IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper231_w),      NULL, NULL, NULL },
	{ BMC_110IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper255_w),      NULL, NULL, NULL },
	{ BMC_64IN1NR, NES_WRITEONLY(bmc_64in1nr_l_w), NES_NOACCESS, NES_WRITEONLY(bmc_64in1nr_w),      NULL, NULL, NULL },
	{ BMC_S24IN1SC03, NES_WRITEONLY(bmc_s24in1sc03_l_w), NES_NOACCESS, NES_WRITEONLY(bmc_s24in1sc03_w),      NULL, NULL, mapper4_irq },
	{ BMC_HIK8IN1, NES_NOACCESS, NES_WRITEONLY(mapper45_m_w), NES_WRITEONLY(mapper4_w),      NULL, NULL, mapper4_irq },
	{ BMC_SUPERHIK_4IN1, NES_NOACCESS, NES_WRITEONLY(mapper49_m_w), NES_WRITEONLY(mapper4_w),      NULL, NULL, mapper4_irq },
	{ BMC_SUPERBIG_7IN1, NES_NOACCESS, NES_NOACCESS, NES_WRITEONLY(mapper44_w),      NULL, NULL, mapper4_irq },
	{ BMC_MARIOPARTY_7IN1, NES_NOACCESS, NES_WRITEONLY(mapper52_m_w), NES_WRITEONLY(mapper4_w),      NULL, NULL, mapper4_irq },
	{ BMC_FAMILY_4646B, NES_NOACCESS, NES_WRITEONLY(mapper134_m_w), NES_WRITEONLY(mapper4_w),      NULL, NULL, mapper4_irq },
	{ BMC_15IN1, NES_NOACCESS, NES_WRITEONLY(mapper205_m_w), NES_WRITEONLY(mapper4_w),      NULL, NULL, mapper4_irq },
	{ BMC_BALLGAMES_11IN1, NES_NOACCESS, NES_WRITEONLY(mapper51_m_w), NES_WRITEONLY(mapper51_w),      NULL, NULL, NULL },
	{ BMC_GOLDENCARD_6IN1, NES_WRITEONLY(mapper217_l_w), NES_NOACCESS, NES_WRITEONLY(mapper217_w),      NULL, NULL, mapper4_irq },
	{ UNSUPPORTED_BOARD, NES_NOACCESS, NES_NOACCESS, NES_NOACCESS,                  NULL, NULL, NULL },
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
	const nes_pcb *pcb = nes_pcb_lookup(state->board);
	const nes_pcb_intf *intf;
	
	if (pcb == NULL)
		fatalerror("Unimplemented PCB\n");
	
	// else, the PCB is emulated
	intf = nes_pcb_intf_lookup(pcb->pcb_id);
	
	if (intf == NULL)
		fatalerror("Missing PCB interface\n");
	
	if (state->chr_chunks == 0)
		chr8(machine, 0, CHRRAM);
	else
		chr8(machine, 0, CHRROM);
	
	/* Set the mapper irq callback */
	ppu2c0x_set_scanline_callback(state->ppu, intf ? intf->mmc_scanline : NULL);
	ppu2c0x_set_hblank_callback(state->ppu, intf ? intf->mmc_hblank : NULL);
	
	err = unif_initialize(machine, pcb->pcb_id);
	
	return err;
}

void pcb_handlers_setup( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	const nes_pcb *pcb = nes_pcb_lookup(state->board);
	const nes_pcb_intf *intf;
	
	if (pcb == NULL)
		fatalerror("Unimplemented PCB: %s\n", state->board);
	
	// else, the PCB is emulated
	intf = nes_pcb_intf_lookup(pcb->pcb_id);
	
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
		logerror("PCB %s is not yet supported, defaulting to no mapper.\n", pcb->pcb_name);
		state->mmc_write_low = NULL;
		state->mmc_write_mid = NULL;
		state->mmc_write = NULL;
		state->mmc_read_low = NULL;
		state->mmc_read_mid = NULL;	// in progress
		state->mmc_read = NULL;	// in progress
		ppu_latch = NULL;
	}
}

