#ifndef __MMC_H
#define __MMC_H

typedef struct __mmc_intf
{
	write8_space_func mmc_write_low; /* $4100-$5fff write routine */
	read8_space_func mmc_read_low; /* $4100-$5fff read routine */
	write8_space_func mmc_write_mid; /* $6000-$7fff write routine */
	write8_space_func mmc_write; /* $8000-$ffff write routine */
} mmc_intf;

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

const mmc *nes_mapper_lookup(int mapper);

typedef struct __unif
{
	const char *board; /* UNIF board */

	write8_space_func mmc_write_low; /* $4100-$5fff write routine */
	read8_space_func mmc_read_low; /* $4100-$5fff read routine */
	write8_space_func mmc_write_mid; /* $6000-$7fff write routine */
	write8_space_func mmc_write; /* $8000-$ffff write routine */
	void (*ppu_latch)(running_device *device, offs_t offset);
	ppu2c0x_scanline_cb		mmc_scanline;
	ppu2c0x_hblank_cb		mmc_hblank;

	int prgrom;
	int chrrom;
	int nvwram;
	int wram;
	int chrram;
	int nt;
	int board_idx;
} unif;

const unif *nes_unif_lookup(const char *board);

//extern int MMC1_extended; /* 0 = normal MMC1 cart, 1 = 512k MMC1, 2 = 1024k MMC1 */

#define MMC5_VRAM

extern UINT8 nes_mmc5_vram[0x400];
extern int nes_mmc5_vram_control;

void nes_mapper_init( const mmc_intf *intf );
int nes_mapper_reset( running_machine *machine, int mapperNum );
int nes_unif_reset( running_machine *machine, const char *board );

WRITE8_HANDLER( nes_low_mapper_w );
READ8_HANDLER( nes_low_mapper_r );
READ8_HANDLER( nes_mid_mapper_r );
WRITE8_HANDLER( nes_mid_mapper_w );
WRITE8_HANDLER( nes_mapper_w );
WRITE8_HANDLER( nes_chr_w );
READ8_HANDLER( nes_chr_r );
WRITE8_HANDLER( nes_nt_w );
READ8_HANDLER( nes_nt_r );

READ8_HANDLER( nes_fds_r );
WRITE8_HANDLER( nes_fds_w );
WRITE8_HANDLER( nes_mapper50_add_w );

//TEMPORARY PPU STUFF

/* mirroring types */
#define PPU_MIRROR_NONE		0
#define PPU_MIRROR_VERT		1
#define PPU_MIRROR_HORZ		2
#define PPU_MIRROR_HIGH		3
#define PPU_MIRROR_LOW		4
#define PPU_MIRROR_4SCREEN	5	// Same effect as NONE, but signals that we should never mirror

void set_nt_mirroring(int mirroring);


#endif
