/*********************************************************************

	coco.h

	CoCo/Dragon code

*********************************************************************/

#ifndef DRAGON_H
#define DRAGON_H

#include "video/m6847.h"
#include "devices/snapquik.h"
#include "osdepend.h"

#define COCO_CPU_SPEED_HZ		894886	/* 0.894886 MHz */
#define COCO_FRAMES_PER_SECOND	(COCO_CPU_SPEED_HZ / 57.0 / 263)
#define COCO_CPU_SPEED			(TIME_IN_HZ(COCO_CPU_SPEED_HZ))
#define COCO_TIMER_CMPCARRIER	(COCO_CPU_SPEED * 0.25)

/* ----------------------------------------------------------------------- *
 * from vidhrdw/coco.c                                                     *
 * ----------------------------------------------------------------------- */

ATTR_CONST UINT8 coco_get_attributes(UINT8 c);

VIDEO_START( dragon );
VIDEO_START( coco );
VIDEO_START( coco2b );

/* ----------------------------------------------------------------------- *
 * from vidhrdw/coco3.c                                                    *
 * ----------------------------------------------------------------------- */

VIDEO_START( coco3 );
VIDEO_START( coco3p );
VIDEO_UPDATE( coco3 );
WRITE8_HANDLER ( coco3_palette_w );
UINT32 coco3_get_video_base(UINT8 ff9d_mask, UINT8 ff9e_mask);
void coco3_vh_blink(void);

/* ----------------------------------------------------------------------- *
 * from machine/coco.c                                                     *
 * ----------------------------------------------------------------------- */

extern UINT8 coco3_gimereg[16];

MACHINE_START( dragon32 );
MACHINE_START( dragon64 );
MACHINE_START( tanodr64 );
MACHINE_START( dgnalpha );
MACHINE_START( coco );
MACHINE_START( coco2 );
MACHINE_START( coco3 );

DEVICE_LOAD(coco_rom);
DEVICE_LOAD(coco3_rom);
DEVICE_UNLOAD(coco_rom);
DEVICE_UNLOAD(coco3_rom);

SNAPSHOT_LOAD ( coco_pak );
SNAPSHOT_LOAD ( coco3_pak );
READ8_HANDLER ( coco3_mmu_r );
WRITE8_HANDLER ( coco3_mmu_w );
READ8_HANDLER ( coco3_gime_r );
WRITE8_HANDLER ( coco3_gime_w );
READ8_HANDLER ( coco_cartridge_r);
WRITE8_HANDLER ( coco_cartridge_w );
READ8_HANDLER ( coco3_cartridge_r);
WRITE8_HANDLER ( coco3_cartridge_w );
offs_t coco3_mmu_translate(int bank, int offset);
int coco_bitbanger_init (int id);
READ8_HANDLER( coco_pia_1_r );
READ8_HANDLER( coco3_pia_1_r );
void coco3_horizontal_sync_callback(int data);
void coco3_field_sync_callback(int data);
void coco3_gime_field_sync_callback(void);

/* Compusense Dragon Plus board */
READ8_HANDLER ( plus_reg_r );
WRITE8_HANDLER ( plus_reg_w );

READ8_HANDLER( dragon_alpha_mapped_irq_r );

/* Dragon Alpha AY-8912 */
READ8_HANDLER ( dgnalpha_psg_porta_read );
WRITE8_HANDLER ( dgnalpha_psg_porta_write );

/* Dragon Alpha WD2797 FDC */
READ8_HANDLER(wd2797_r);
WRITE8_HANDLER(wd2797_w);

void coco_set_halt_line(int halt_line);

/* Returns whether a given piece of logical memory is contiguous or not */
int coco3_mmu_ismemorycontiguous(int logicaladdr, int len);

/* Reads logical memory into a buffer */
void coco3_mmu_readlogicalmemory(UINT8 *buffer, int logicaladdr, int len);

/* Translates a logical address to a physical address */
int coco3_mmu_translatelogicaladdr(int logicaladdr);

#define IO_BITBANGER IO_PRINTER
#define IO_VHD IO_HARDDISK

/* CoCo 3 video vars; controlling key aspects of the emulation */
struct coco3_video_vars
{
	UINT8 bordertop_192;
	UINT8 bordertop_199;
	UINT8 bordertop_0;
	UINT8 bordertop_225;
	unsigned int hs_gime_flip : 1;
	unsigned int fs_gime_flip : 1;
	unsigned int hs_pia_flip : 1;
	unsigned int fs_pia_flip : 1;
	UINT16 rise_scanline;
	UINT16 fall_scanline;
};

extern const struct coco3_video_vars coco3_vidvars;

#endif /* DRAGON_H */
READ8_HANDLER( dragon_alpha_mapped_irq_r );
