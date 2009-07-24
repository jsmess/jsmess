#ifndef __MMC_H
#define __MMC_H

typedef struct __mmc
{
	int iNesMapper; /* iNES Mapper # */
	const char *desc;     /* Mapper description */
	write8_space_func mmc_write_low; /* $4100-$5fff write routine */
	read8_space_func mmc_read_low; /* $4100-$5fff read routine */
	write8_space_func mmc_write_mid; /* $6000-$7fff write routine */
	write8_space_func mmc_write; /* $8000-$ffff write routine */
	void (*ppu_latch)(const device_config *device, offs_t offset);
	ppu2c0x_scanline_cb		mmc_scanline;
	ppu2c0x_hblank_cb		mmc_hblank;
} mmc;

const mmc *nes_mapper_lookup(int mapper);

extern int MMC1_extended; /* 0 = normal MMC1 cart, 1 = 512k MMC1, 2 = 1024k MMC1 */

#define MMC5_VRAM

extern UINT8 MMC5_vram[0x400];
extern int MMC5_vram_control;

extern write8_space_func mmc_write_low;
extern read8_space_func mmc_read_low;
extern write8_space_func mmc_write_mid;
extern read8_space_func mmc_read_mid;
extern write8_space_func mmc_write;

int mapper_reset (running_machine *machine, int mapperNum);

WRITE8_HANDLER( nes_low_mapper_w );
READ8_HANDLER ( nes_low_mapper_r );
READ8_HANDLER ( nes_mid_mapper_r );
WRITE8_HANDLER ( nes_mid_mapper_w );
WRITE8_HANDLER ( nes_mapper_w );
WRITE8_HANDLER ( nes_chr_w );
READ8_HANDLER ( nes_chr_r );
WRITE8_HANDLER ( nes_nt_w );
READ8_HANDLER ( nes_nt_r );

 READ8_HANDLER ( fds_r );
WRITE8_HANDLER ( fds_w );

#endif
