#ifndef _GBA_H_
#define _GBA_H_

#define DISPSTAT_VBL			0x0001
#define DISPSTAT_HBL			0x0002
#define DISPSTAT_VCNT			0x0004
#define DISPSTAT_VBL_IRQ_EN		0x0008
#define DISPSTAT_HBL_IRQ_EN		0x0010
#define DISPSTAT_VCNT_IRQ_EN		0x0020
#define DISPSTAT_VCNT_VALUE		0xff00

#define INT_VBL				0x0001
#define INT_HBL				0x0002
#define INT_VCNT			0x0004
#define INT_TM0_OVERFLOW		0x0008
#define INT_TM1_OVERFLOW		0x0010
#define INT_TM2_OVERFLOW		0x0020
#define INT_TM3_OVERFLOW		0x0040
#define INT_SIO				0x0080
#define INT_DMA0			0x0100
#define INT_DMA1			0x0200
#define INT_DMA2			0x0400
#define INT_DMA3			0x0800
#define INT_KEYPAD			0x1000
#define INT_GAMEPAK			0x2000

#define DISPCNT_MODE			0x0007
#define DISPCNT_FRAMESEL		0x0010
#define DISPCNT_HBL_FREE		0x0020

#define DISPCNT_VRAM_MAP		0x0040
#define DISPCNT_VRAM_MAP_2D		0x0000
#define DISPCNT_VRAM_MAP_1D		0x0040

#define DISPCNT_BLANK			0x0080
#define DISPCNT_BG0_EN			0x0100
#define DISPCNT_BG1_EN			0x0200
#define DISPCNT_BG2_EN			0x0400
#define DISPCNT_BG3_EN			0x0800
#define DISPCNT_OBJ_EN			0x1000
#define DISPCNT_WIN0_EN			0x2000
#define DISPCNT_WIN1_EN			0x4000
#define DISPCNT_OBJWIN_EN		0x8000

#define OBJ_Y_COORD			0x00ff
#define OBJ_ROZMODE			0x0300
#define OBJ_ROZMODE_NONE	0x0000
#define OBJ_ROZMODE_ROZ		0x0100
#define OBJ_ROZMODE_DISABLE	0x0200
#define OBJ_ROZMODE_DBLROZ	0x0300

#define OBJ_MODE			0x0c00
#define OBJ_MODE_NORMAL			0x0000
#define OBJ_MODE_ALPHA			0x0400
#define OBJ_MODE_WINDOW			0x0800
#define OBJ_MODE_UNDEFINED		0x0c00

#define OBJ_MOSAIC			0x1000

#define OBJ_PALMODE			0x2000
#define OBJ_PALMODE_16			0x0000
#define OBJ_PALMODE_256			0x2000

#define OBJ_SHAPE			0xc000
#define OBJ_SHAPE_SQR			0x0000
#define OBJ_SHAPE_HORIZ			0x4000
#define OBJ_SHAPE_VERT			0x8000

#define OBJ_X_COORD			0x01ff
#define OBJ_SCALE_PARAM			0x3e00
#define OBJ_SCALE_PARAM_SHIFT		9
#define OBJ_HFLIP			0x1000
#define OBJ_VFLIP			0x2000
#define OBJ_SIZE			0xc000
#define OBJ_SIZE_8			0x0000
#define OBJ_SIZE_16			0x4000
#define OBJ_SIZE_32			0x8000
#define OBJ_SIZE_64			0xc000

#define OBJ_TILENUM 			0x03ff
#define OBJ_PRIORITY			0x0c00
#define OBJ_PRIORITY_SHIFT		10
#define OBJ_PALNUM			0xf000
#define OBJ_PALNUM_SHIFT		12

#define BGCNT_SCREENSIZE		0xc000
#define BGCNT_SCREENSIZE_SHIFT	14
#define BGCNT_PALETTESET_WRAP	0x2000
#define BGCNT_SCREENBASE		0x1f00
#define BGCNT_SCREENBASE_SHIFT	8
#define BGCNT_PALETTE256		0x0080
#define BGCNT_MOSAIC			0x0040
#define BGCNT_CHARBASE			0x003c
#define BGCNT_CHARBASE_SHIFT	2
#define BGCNT_PRIORITY			0x0003

#define BLDCNT_BG0TP1			0x0001
#define BLDCNT_BG1TP1			0x0002
#define BLDCNT_BG2TP1			0x0004
#define BLDCNT_BG3TP1			0x0008
#define BLDCNT_OBJTP1			0x0010
#define BLDCNT_BDTP1			0x0020
#define BLDCNT_SFX				0x00c0
#define BLDCNT_SFX_NONE			0x0000
#define BLDCNT_SFX_ALPHA		0x0040
#define BLDCNT_SFX_LIGHTEN		0x0080
#define BLDCNT_SFX_DARKEN		0x00c0
#define BLDCNT_BG0TP2			0x0100
#define BLDCNT_BG1TP2			0x0200
#define BLDCNT_BG2TP2			0x0400
#define BLDCNT_BG3TP2			0x0800
#define BLDCNT_OBJTP2			0x1000
#define BLDCNT_BDTP2			0x2000
#define BLDCNT_TP2_SHIFT		8

#define TILEOBJ_TILE			0x03ff
#define TILEOBJ_HFLIP			0x0400
#define TILEOBJ_VFLIP			0x0800
#define TILEOBJ_PALETTE			0xf000

enum
{
	EEP_IDLE,
	EEP_COMMAND,
	EEP_ADDR,
	EEP_AFTERADDR,
	EEP_READ,
	EEP_WRITE,
	EEP_AFTERWRITE,
	EEP_READFIRST
};

/* driver state */
class gba_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, gba_state(machine)); }

	gba_state(running_machine &machine)
		: driver_data_t(machine) { }

	UINT32 DISPSTAT;
	UINT32 BG2X, BG2Y, BG3X, BG3Y;
	UINT16 DISPCNT,	GRNSWAP;
	UINT16 BG0CNT, BG1CNT, BG2CNT, BG3CNT;
	UINT16 BG0HOFS, BG0VOFS, BG1HOFS, BG1VOFS, BG2HOFS, BG2VOFS, BG3HOFS, BG3VOFS;
	UINT16 BG2PA, BG2PB, BG2PC, BG2PD, BG3PA, BG3PB, BG3PC, BG3PD;
	UINT16 WIN0H, WIN1H, WIN0V, WIN1V, WININ, WINOUT;
	UINT16 MOSAIC;
	UINT16 BLDCNT;
	UINT16 BLDALPHA;
	UINT16 BLDY;
	UINT8  SOUNDCNT_X;
	UINT16 SOUNDCNT_H;
	UINT16 SOUNDBIAS;
	UINT16 SIOMULTI0, SIOMULTI1, SIOMULTI2, SIOMULTI3;
	UINT16 SIOCNT, SIODATA8;
	UINT16 KEYCNT;
	UINT16 RCNT;
	UINT16 JOYCNT;
	UINT32 JOY_RECV, JOY_TRANS;
	UINT16 JOYSTAT;
	UINT16 IR, IE, IF, IME;
	UINT16 WAITCNT;
	UINT8  POSTFLG;
	UINT8  HALTCNT;

	UINT8  windowOn;
	UINT8  fxOn;
	UINT8  gfxBG2Changed;
	UINT8  gfxBG3Changed;
	INT32  gfxBG2X;
	INT32  gfxBG2Y;
	INT32  gfxBG3X;
	INT32  gfxBG3Y;

	UINT32 *gba_pram;
	UINT32 *gba_vram;
	UINT32 *gba_oam;

	UINT32 dma_regs[16];
	UINT32 dma_src[4], dma_dst[4], dma_cnt[4], dma_srcadd[4], dma_dstadd[4];
	UINT32 timer_regs[4];
	UINT16 timer_reload[4];

	UINT32 gba_sram[0x10000/4];
	UINT8 gba_eeprom[0x2000];
	UINT32 gba_flash[0x20000/4];
	UINT32 flash_size;
	UINT32 flash_mask;
	int eeprom_state, eeprom_command, eeprom_count, eeprom_addr, eeprom_bits;
	UINT8 eep_data;

	/* nvram-specific for MESS */
	UINT8 *nvptr;
	UINT32 nvsize;
	running_device *nvimage;

	emu_timer *dma_timer[4], *tmr_timer[4], *irq_timer;
	emu_timer *scan_timer, *hbl_timer;

	double timer_hz[4];

	int fifo_a_ptr, fifo_b_ptr, fifo_a_in, fifo_b_in;
	UINT8 fifo_a[20], fifo_b[20];
};

/*----------- defined in video/gba.c -----------*/

void gba_draw_scanline(running_machine *machine, int y);
void gba_video_start(running_machine *machine);

#endif
