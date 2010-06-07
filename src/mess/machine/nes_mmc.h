#ifndef __MMC_H
#define __MMC_H

//extern int MMC1_extended; /* 0 = normal MMC1 cart, 1 = 512k MMC1, 2 = 1024k MMC1 */

#define MMC5_VRAM

int nes_mapper_reset(running_machine *machine);
int nes_unif_reset(running_machine *machine);
int nes_pcb_reset(running_machine *machine);

void mapper_handlers_setup(running_machine *machine);
void unif_handlers_setup(running_machine *machine);
void unif_mapr_setup(running_machine *machine, const char *board);
void pcb_handlers_setup(running_machine *machine);

WRITE8_HANDLER( nes_low_mapper_w );
WRITE8_HANDLER( nes_mid_mapper_w );
WRITE8_HANDLER( nes_mapper_w );
READ8_HANDLER( nes_low_mapper_r );
READ8_HANDLER( nes_mid_mapper_r );
READ8_HANDLER( nes_mapper_r );
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

void set_nt_mirroring(running_machine *machine, int mirroring);


#endif
