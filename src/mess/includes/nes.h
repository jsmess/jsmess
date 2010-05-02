/*****************************************************************************
 *
 * includes/nes.h
 *
 * Nintendo Entertainment System (Famicom)
 *
 ****************************************************************************/

#ifndef NES_H_
#define NES_H_


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define NTSC_CLOCK           N2A03_DEFAULTCLOCK     /* 1.789772 MHz */
#define PAL_CLOCK	           (26601712.0/16)        /* 1.662607 MHz */

#define NES_BATTERY_SIZE 0x2000


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct nes_input
{
	UINT32 shift;
	UINT32 i0, i1, i2;
};

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


class nes_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, nes_state(machine)); }

	nes_state(running_machine &machine) { }

	/* input_related - this part has to be cleaned up (e.g. in_2 and in_3 are not really necessary here...) */
	nes_input in_0, in_1, in_2, in_3;

	chr_bank      chr_map[8];  //quick banking structure, because some of this changes multiple times per scanline!
	name_table    nt_page[4];  //quick banking structure for a maximum of 4K of RAM/ROM/ExRAM

	int MMC5_floodtile;
	int MMC5_floodattr;
	UINT8 mmc5_vram[0x400];
	int mmc5_vram_control;
	UINT8 *extended_ntram;

	/* video-related */
	int nes_vram_sprite[8]; /* Used only by mmc5 for now */
	int last_frame_flip;
	double scanlines_per_frame;

	/* misc */
	write8_space_func   mmc_write_low;
	read8_space_func    mmc_read_low;
	write8_space_func   mmc_write_mid;
	write8_space_func   mmc_write;
	emu_timer	        *irq_timer;

	/* devices */
	running_device      *maincpu;
	running_device      *ppu;
	running_device      *sound;
	running_device      *cart;

	/* misc region to be allocated at init */
	// variables which don't change at run-time
	UINT8      *rom;
	UINT8      *vrom;
	UINT8      *vram;
	UINT8      *wram;
	UINT8      *ciram; //PPU nametable RAM - external to PPU!
	// Variables which can change
	UINT8      mid_ram_enable;

	/* SRAM-related (we have two elements due to the init order, but it would be better to verify they both are still needed) */
	UINT8      *battery_ram;
	UINT8      battery_data[NES_BATTERY_SIZE];

	/***** Mapper-related variables *****/
	
	// common ones
	int        IRQ_enable, IRQ_enable_latch;
	UINT16     IRQ_count, IRQ_count_latch;
	UINT8      IRQ_toggle;
	UINT8      IRQ_reset;
	UINT8      IRQ_status;
	UINT8      IRQ_mode;
	int        mult1, mult2;
	
	UINT8 mmc_chr_source;			// This is set at init to CHRROM or CHRRAM. a few mappers can swap between
									// the two (this is done in the specific handlers).
	
	UINT8 mmc_cmd1, mmc_cmd2;		// These represent registers where the mapper writes important values
	UINT8 mmc_count;				// This is used as counter in mappers like 1 and 45
	
	int mmc_prg_base, mmc_prg_mask;	// MMC3 based multigame carts select a block of banks by using these (and then act like normal MMC3),
	int mmc_chr_base, mmc_chr_mask;	// while MMC3 and clones (mapper 118 & 119) simply set them as 0 and 0xff resp.
	
	UINT8 prg_bank[4];				// Many mappers writes only some bits of the selected bank (for both PRG and CHR),
	UINT8 vrom_bank[16];			// hence these are handy to latch bank values.

	UINT8 extra_bank[16];			// some MMC3 clone have 2 series of PRG/CHR banks...
									// we collect them all here: first 4 elements PRG banks, then 6/8 CHR banks
	
	/***** NES-cart related *****/

	/* load-time cart variables which remain constant */
	UINT8 trainer;
	UINT8 battery;
	UINT16 prg_chunks;	// a recently dumped multigame cart has 256 chunks of both PRG & CHR!
	UINT16 chr_chunks;
	
	UINT8 format;	// 1 = iNES, 2 = UNIF
	
	/* system variables which don't change at run-time */
	UINT16 mapper;		// for iNES
	const char *board;	// for UNIF
	UINT8 four_screen_vram;
	UINT8 hard_mirroring;
	UINT8 slow_banking;
	UINT8 crc_hack;	// this is needed to detect different boards sharing the same Mappers (shame on .nes format)
	
	
	/***** FDS-floppy related *****/

	UINT8   *fds_data;
	UINT8   fds_sides;
	UINT8   *fds_ram;
	
	/* Variables which can change */
	UINT8   fds_motor_on;
	UINT8   fds_door_closed;
	UINT8   fds_current_side;
	UINT32  fds_head_position;
	UINT8   fds_status0;
	UINT8   fds_read_mode;
	UINT8   fds_write_reg;

	/* these are used in the mapper 20 handlers */
	int     fds_last_side;
	int     fds_count;
};


/*----------- defined in machine/nes.c -----------*/

WRITE8_HANDLER ( nes_IN0_w );
WRITE8_HANDLER ( nes_IN1_w );

/* protos */

DEVICE_IMAGE_LOAD(nes_cart);
DEVICE_START(nes_disk);
DEVICE_IMAGE_LOAD(nes_disk);
DEVICE_IMAGE_UNLOAD(nes_disk);

MACHINE_START( nes );
MACHINE_RESET( nes );

DRIVER_INIT( famicom );

READ8_HANDLER( nes_IN0_r );
READ8_HANDLER( nes_IN1_r );

int nes_ppu_vidaccess( running_device *device, int address, int data );

void nes_partialhash(char *dest, const unsigned char *data, unsigned long length, unsigned int functions);

/*----------- defined in video/nes.c -----------*/

extern int nes_vram_sprite[8];

PALETTE_INIT( nes );
VIDEO_START( nes_ntsc );
VIDEO_START( nes_pal );
VIDEO_UPDATE( nes );


#endif /* NES_H_ */
