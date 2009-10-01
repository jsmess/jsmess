/*
    header file for machine/ti99_4x.c
*/
#include "video/tms9928a.h"
#include "machine/tms9901.h"

#include "devices/ti99cart.h"
#include "devices/cartslot.h"

/* defines */

#define HAS_99CCFDC 0

/* region identifiers */
#define region_grom "cons_grom"
#define region_dsr "dsr"
#define region_hsgpl "hsgpl"
#define region_speech_rom "tms5220"

/* offsets for "maincpu" */
enum
{
	offset_sram = 0x2000,		/* scratch RAM (256 bytes) */
	offset_cart = 0x2100,		/* cartridge ROM/RAM (2*8 kbytes) */
	offset_xram = 0x6100,		/* extended RAM (32 kbytes - 512kb with myarc-like mapper, 1Mb with super AMS) */
	region_cpu1_len = 0x106100	/* total len */
};

enum
{
	offset_rom0_4p = 0x4000,
	offset_rom4_4p = 0xc000,
	offset_rom6_4p = 0x6000,
	offset_rom6b_4p= 0xe000,
	offset_sram_4p = 0x10000,		/* scratch RAM (1kbyte) */
	offset_dram_4p = 0x10400,		/* extra ram for debugger (768 bytes) */
	offset_xram_4p = 0x10700,		/* extended RAM (1Mb with super AMS compatible mapper) */
	region_cpu1_len_4p = 0x110700	/* total len */
};

enum
{
	offset_rom0_8 = 0x0000,			/*  system ROM (32kbytes, though hexbus DSR and pascal are missing) */
	offset_cart_8 = 0x8000,			/* cartridge ROM/RAM (2*8 kbytes) */
	offset_sram_8 = 0xc000,			/* scratch RAM (2kbytes) */
	offset_xram_8 = 0xc800,			/* extended RAM (64kbytes, expandable to almost 16MBytes) */
	region_cpu1_len_8 = 0x1c800		/* total len */
};

/* offsets for region_dsr */
enum
{
	offset_fdc_dsr   = 0x0000,			/* TI FDC DSR (8kbytes) */
	offset_ccfdc_dsr = offset_fdc_dsr   + 0x02000,	/* CorcComp FDC DSR (16kbytes) */
	offset_bwg_dsr   = offset_ccfdc_dsr + 0x04000,	/* BwG FDC DSR (32kbytes) */
	offset_bwg_ram   = offset_bwg_dsr   + 0x08000,	/* BwG FDC RAM (2kbytes) */
	offset_hfdc_dsr  = offset_bwg_ram   + 0x00800,	/* HFDC FDC DSR (16kbytes) */
	offset_hfdc_ram  = offset_hfdc_dsr  + 0x04000,	/* HFDC FDC RAM (32kbytes) */
	offset_rs232_dsr = offset_hfdc_ram  + 0x08000,	/* TI RS232 DSR (4kbytes) */
	offset_evpc_dsr  = offset_rs232_dsr + 0x01000,	/* EVPC DSR (64kbytes) */
	offset_ide_ram   = offset_evpc_dsr  + 0x10000,	/* IDE card RAM (32 to 2Mbytes, we settled for 512kbytes) */
	offset_ide_ram2  = offset_ide_ram   + 0x80000,	/* IDE RTC RAM (4kbytes) */
	offset_pcode_rom = offset_ide_ram2  + 0x01000,  /* P-Code Card ROM (12KiB) */
	offset_pcode_grom= offset_pcode_rom + 0x03000,  /* P-Code Card GROM (64KiB) */
	region_dsr_len   = offset_pcode_grom+ 0x10000
};

/* offsets for region_hsgpl */
enum
{
	offset_hsgpl_dsr  = 0x000000,					/* DSR (512kbytes) */
	offset_hsgpl_grom = 0x080000,					/* GROM (1Mbytes) */
	offset_hsgpl_rom6 = 0x180000,					/* ROM6 (512kbytes) */
	offset_hsgpl_gram = 0x200000,					/* GRAM (128kbytes) */
	offset_hsgpl_ram6 = 0x220000,					/* RAM6 (128kbytes, but only 64kbytes are used) */

	region_hsgpl_len  = /*0x240000*/0x230000
};

/* enum for RAM config */
typedef enum
{
	xRAM_kind_none = 0,
	xRAM_kind_TI,				/* 32kb TI and clones */
	xRAM_kind_super_AMS,		/* 1Mb super AMS */
	xRAM_kind_foundation_128k,	/* 128kb foundation */
	xRAM_kind_foundation_512k,	/* 512kb foundation */
	xRAM_kind_myarc_128k,		/* 128kb myarc clone (no ROM) */
	xRAM_kind_myarc_512k,		/* 512kb myarc clone (no ROM) */

	xRAM_kind_99_4p_1Mb,		/* ti99/4p super AMS clone */
	xRAM_kind_99_8				/* ti99/8 memory mapper */
} xRAM_kind_t;

/* enum for fdc config */
typedef enum
{
	fdc_kind_none = 0,
	fdc_kind_TI,				/* TI fdc */
	fdc_kind_BwG,				/* SNUG's BwG fdc */
	fdc_kind_hfdc				/* Myarc's HFDC (handles SD and DD floppies (I
                                    think an HD update existed) and prehistoric
                                    MFM hard disks) */
#if HAS_99CCFDC
	,
	fdc_kind_CC = 0x100			/* CorComp fdc */
#endif
} fdc_kind_t;

/* defines for input port "CFG" */
enum
{
	config_xRAM_bit		= 0,
	config_xRAM_mask	= 0x7,	/* 3 bits */
	config_speech_bit	= 3,
	config_speech_mask	= 0x1,
	config_fdc_bit		= 4,
	config_fdc_mask		= 0x103,/* 3 bits (4, 5 and 12) */
	config_rs232_bit	= 6,
	config_rs232_mask	= 0x1,
	/* next option only makes sense for ti99/4 */
	config_handsets_bit	= 7,
	config_handsets_mask= 0x1,
	config_ide_bit		= 8,
	config_ide_mask		= 0x1,
	config_hsgpl_bit	= 9,
	config_hsgpl_mask	= 0x1,
	config_mecmouse_bit	= 10,
	config_mecmouse_mask	= 0x1,
	config_usbsm_bit	= 11,
	config_usbsm_mask	= 0x1,
	config_boot_bit		= 13,
	config_boot_mask	= 0x1,
	config_cartslot_bit	= 14,
	config_cartslot_mask	= 0xf, /* need four bits: covers all current and possibly future cartslots, and one auto mode */
	config_pcode_bit	= 18,
	config_pcode_mask	= 0x1
};

enum {
	STANDARD,
	EXBAS,
	MINIMEM,
	SUPERSPACE,
	MBX
};

/* prototypes for machine code */

DRIVER_INIT( ti99_4 );
DRIVER_INIT( ti99_4a );
DRIVER_INIT( ti99_4ev );
DRIVER_INIT( ti99_8 );
DRIVER_INIT( ti99_4p );

MACHINE_START( ti99_4_60hz );
MACHINE_START( ti99_4_50hz );
MACHINE_START( ti99_4a_60hz );
MACHINE_START( ti99_4a_50hz );
MACHINE_START( ti99_4ev_60hz );
MACHINE_RESET( ti99 );

// DEVICE_START( ti99_cart );
// DEVICE_IMAGE_LOAD( ti99_cart );
// DEVICE_IMAGE_UNLOAD( ti99_cart );

VIDEO_START( ti99_4ev );
INTERRUPT_GEN( ti99_vblank_interrupt );
INTERRUPT_GEN( ti99_4ev_hblank_interrupt );

void set_hsgpl_crdena(int data);
void ti99_common_init(running_machine *machine, const TMS9928a_interface *gfxparm);
int is_99_8(void);

READ16_HANDLER ( ti99_nop_8_r );
WRITE16_HANDLER ( ti99_nop_8_w );

READ16_HANDLER ( ti99_4p_cart_r );
WRITE16_HANDLER ( ti99_4p_cart_w );

WRITE16_HANDLER( ti99_wsnd_w );

READ16_HANDLER ( ti99_rvdp_r );
WRITE16_HANDLER ( ti99_wvdp_w );
READ16_HANDLER ( ti99_rv38_r );
WRITE16_HANDLER ( ti99_wv38_w );

READ16_HANDLER ( ti99_grom_r );
WRITE16_HANDLER( ti99_grom_w );
READ16_HANDLER ( ti99_4p_grom_r );
WRITE16_HANDLER ( ti99_4p_grom_w );

extern void tms9901_set_int2(running_machine *machine, int state);

READ8_HANDLER ( ti99_8_r );
WRITE8_HANDLER ( ti99_8_w );

extern const tms9901_interface tms9901reset_param_ti99_4x;
extern const tms9901_interface tms9901reset_param_ti99_8;

