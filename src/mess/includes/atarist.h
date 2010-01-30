#ifndef __ATARI_ST__
#define __ATARI_ST__

#define SCREEN_TAG	"screen"
#define M68000_TAG	"m68000"
#define HD6301_TAG	"hd6301"
#define COP888_TAG	"u703"
#define YM3439_TAG	"ym3439"
#define MC68901_TAG	"mc68901"
#define LMC1992_TAG "lmc1992"
#define WD1772_TAG	"wd1772"
#define CENTRONICS_TAG	"centronics"

// Atari ST

#define Y1		XTAL_2_4576MHz

// STBook

#define U517	XTAL_16MHz
#define Y200	XTAL_2_4576MHz
#define Y700	XTAL_10MHz

#define ATARIST_FLOPPY_STATUS_FDC_DATA_REQUEST	0x04
#define ATARIST_FLOPPY_STATUS_SECTOR_COUNT_ZERO	0x02
#define ATARIST_FLOPPY_STATUS_DMA_ERROR			0x01

#define ATARIST_FLOPPY_MODE_WRITE				0x0100
#define ATARIST_FLOPPY_MODE_FDC_ACCESS			0x0080
#define ATARIST_FLOPPY_MODE_DMA_DISABLE			0x0040
#define ATARIST_FLOPPY_MODE_SECTOR_COUNT		0x0010
#define ATARIST_FLOPPY_MODE_HDC					0x0008
#define ATARIST_FLOPPY_MODE_ADDRESS_MASK		0x0006

#define ATARIST_FLOPPY_BYTES_PER_SECTOR			512

enum
{
	IKBD_MOUSE_PHASE_STATIC = 0,
	IKBD_MOUSE_PHASE_POSITIVE,
	IKBD_MOUSE_PHASE_NEGATIVE
};

typedef struct _atarist_state atarist_state;
struct _atarist_state
{
	/* memory state */
	UINT8 mmu;
	UINT16 megaste_cache;

	/* keyboard state */
	UINT8 ikbd_keylatch;
	UINT8 ikbd_mouse_x;
	UINT8 ikbd_mouse_y;
	UINT8 ikbd_mouse_px;
	UINT8 ikbd_mouse_py;
	UINT8 ikbd_mouse_pc;
	int ikbd_rx;
	int ikbd_tx;
	int midi_rx;
	int midi_tx;
	int acia_irq;

	/* floppy state */
	UINT32 fdc_dmabase;
	UINT16 fdc_status;
	UINT16 fdc_mode;
	UINT8 fdc_sectors;
	int fdc_dmabytes;
	int fdc_irq;

	/* shifter state */
	UINT32 shifter_base;
	UINT32 shifter_ofs;
	UINT8 shifter_sync;
	UINT8 shifter_mode;
	UINT16 shifter_palette[16];
	UINT8 shifter_lineofs;  // STe
	UINT8 shifter_pixelofs; // STe
	UINT16 shifter_rr[4];
	UINT16 shifter_ir[4];
	int shifter_bitplane;
	int shifter_shift;
	int shifter_h;
	int shifter_v;
	int shifter_de;
	int shifter_x_start;
	int shifter_x_end;
	int shifter_y_start;
	int shifter_y_end;
	int shifter_hblank_start;
	int shifter_vblank_start;

	/* blitter state */
	UINT16 blitter_halftone[16];
	INT16 blitter_src_inc_x;
	INT16 blitter_src_inc_y;
	INT16 blitter_dst_inc_x;
	INT16 blitter_dst_inc_y;
	UINT32 blitter_src;
	UINT32 blitter_dst;
	UINT16 blitter_endmask1;
	UINT16 blitter_endmask2;
	UINT16 blitter_endmask3;
	UINT16 blitter_xcount;
	UINT16 blitter_ycount;
	UINT16 blitter_xcountl;
	UINT8 blitter_hop;
	UINT8 blitter_op;
	UINT8 blitter_ctrl;
	UINT8 blitter_skew;
	UINT32 blitter_srcbuf;

	/* microwire state */
	UINT16 mw_data;
	UINT16 mw_mask;
	int mw_shift;

	/* DMA sound state */
	UINT32 dmasnd_base;
	UINT32 dmasnd_end;
	UINT32 dmasnd_cntr;
	UINT32 dmasnd_baselatch;
	UINT32 dmasnd_endlatch;
	UINT16 dmasnd_ctrl;
	UINT16 dmasnd_mode;
	UINT8 dmasnd_fifo[8];
	UINT8 dmasnd_samples;
	int dmasnd_active;

	/* devices */
	running_device *mc68901;
	running_device *lmc1992;
	running_device *wd1772;
	running_device *centronics;

	/* timers */
	emu_timer *glue_timer;
	emu_timer *shifter_timer;
	emu_timer *microwire_timer;
	emu_timer *dmasound_timer;
};

#endif
