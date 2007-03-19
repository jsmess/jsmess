
#include "driver.h"
#include "video/generic.h"

WRITE8_HANDLER ( vdc_w );
 READ8_HANDLER ( vdc_r );
 READ8_HANDLER ( vce_r );
WRITE8_HANDLER ( vce_w );

/* Screen timing stuff */

#define VDC_LPF         262     /* number of lines in a single frame */

/* Bits in the VDC status register */

#define VDC_BSY         0x40    /* Set when the VDC accesses VRAM */
#define VDC_VD          0x20    /* Set when in the vertical blanking period */
#define VDC_DV          0x10    /* Set when a VRAM > VRAM DMA transfer is done */
#define VDC_DS          0x08    /* Set when a VRAM > SATB DMA transfer is done */
#define VDC_RR          0x04    /* Set when the current scanline equals the RCR register */
#define VDC_OR          0x02    /* Set when there are more than 16 sprites on a line */
#define VDC_CR          0x01    /* Set when sprite #0 overlaps with another sprite */

/* Bits in the CR register */

#define CR_BB           0x80    /* Background blanking */
#define CR_SB           0x40    /* Object blanking */
#define CR_VR           0x08    /* Interrupt on vertical blank enable */
#define CR_RC           0x04    /* Interrupt on line compare enable */
#define CR_OV           0x02    /* Interrupt on sprite overflow enable */
#define CR_CC           0x01    /* Interrupt on sprite #0 collision enable */

/* Bits in the DCR regsiter */

#define DCR_DSR         0x10    /* VRAM > SATB auto-transfer enable */
#define DCR_DID         0x08    /* Destination diretion */
#define DCR_SID         0x04    /* Source direction */
#define DCR_DVC         0x02    /* VRAM > VRAM EOT interrupt enable */
#define DCR_DSC         0x01    /* VRAM > SATB EOT interrupt enable */

/* just to keep things simple... */
enum vdc_regs {MAWR = 0, MARR, VxR, reg3, reg4, CR, RCR, BXR, BYR, MWR, HSR, HDR, VPR, VDW, VCR, DCR, SOUR, DESR, LENR, DVSSR };

/* todo: replace this with the PAIR structure from 'osd_cpu.h' */
typedef union
{
#ifdef LSB_FIRST
  struct { unsigned char l,h; } b;
#else
  struct { unsigned char h,l; } b;
#endif
  unsigned short int w;
}pair;

void pce_refresh_line(int bitmap_line, int line);
void vdc_write(int offset, int data);
int vdc_read(int offset);
void draw_black_line(int line);
void draw_overscan_line(int line);
void vdc_delayed_irq(int unused);

/* VDC segments */
#define STATE_TOPBLANK		0
#define STATE_TOPFILL		1
#define STATE_ACTIVE		2
#define STATE_BOTTOMFILL	3

/* define VCE frame specs, so some day  the emulator can behave right */
#define FIRST_VISIBLE 14
#define SYNC_AREA	   3
#define BLANK_AREA     4
#define LAST_VISIBLE  (VDC_LPF-SYNC_AREA-BLANK_AREA)

/* the VDC context */

typedef struct
{
    int dvssr_write;            /* Set when the DVSSR register has been written to */
    int physical_width;         /* Width of the display */
    int physical_height;        /* Height of the display */
    pair vce_address;           /* Current address in the palette */
    pair vce_data[512];         /* Palette data */
    UINT16 sprite_ram[64*4];    /* Sprite RAM */
    int curline;                /* the current scanline we're on */
    UINT8 *vram;
    UINT8   inc;
    UINT8 vdc_register;
    UINT8 vdc_latch;
    pair vdc_data[32];
    int status;
    mame_bitmap *bmp;
    int current_segment;
    int current_segment_line;
    int current_bitmap_line;
    int y_scroll;
    int top_blanking;
    int top_overscan;
    int active_lines;
    int bottomfill;
}VDC;


/* from vidhrdw\vdc.c */

extern VDC vdc;
extern VIDEO_START( pce );
extern VIDEO_UPDATE( pce );
extern WRITE8_HANDLER ( vdc_w );
extern  READ8_HANDLER ( vdc_r );
extern WRITE8_HANDLER ( vce_w );
extern  READ8_HANDLER ( vce_r );
