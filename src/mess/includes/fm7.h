/*
 * 
 *  FM-7 header file
 * 
 */

#ifndef FM7_H_
#define FM7_H_

// Interrupt flags
#define IRQ_FLAG_KEY      0x01
#define IRQ_FLAG_PRINTER  0x02
#define IRQ_FLAG_TIMER    0x04
#define IRQ_FLAG_OTHER    0x08
// the following are not read in port 0xfd03
#define IRQ_FLAG_MFD      0x10
#define IRQ_FLAG_TXRDY    0x20
#define IRQ_FLAG_RXRDY    0x40
#define IRQ_FLAG_SYNDET   0x80

// system types
#define SYS_FM7        1
#define SYS_FM77AV     2
#define SYS_FM77AV40EX 3

// keyboard scancode formats
#define KEY_MODE_FM7   0 // FM-7 ASCII type code
#define KEY_MODE_FM16B 1 // FM-16B (FM-77AV and later only)
#define KEY_MODE_SCAN  2 // Scancode Make/Break (PC-like)

struct fm7_video_flags
{
	UINT8 sub_busy;
	UINT8 sub_halt;
	UINT8 sub_reset;  // high if reset caused by subrom change
	UINT8 attn_irq;
	UINT8 vram_access;  // VRAM access flag
	UINT8 crt_enable;
	UINT16 vram_offset;
	UINT8 fm7_pal[8];
	UINT16 fm77av_pal_selected;
	UINT8 fm77av_pal_r[4096];
	UINT8 fm77av_pal_g[4096];
	UINT8 fm77av_pal_b[4096];
	UINT8 subrom;  // currently active sub CPU ROM (AV only)
	UINT8 cgrom;  // currently active CGROM (AV only)
	UINT8 modestatus;
	UINT8 multi_page;
	UINT8 fine_offset;
	UINT8 nmi_mask;
};

void fm7_mmr_refresh(const address_space*);

READ8_HANDLER( fm7_subintf_r );
WRITE8_HANDLER( fm7_subintf_w );
READ8_HANDLER( fm7_sub_busyflag_r );
WRITE8_HANDLER( fm7_sub_busyflag_w );
READ8_HANDLER( fm77av_sub_modestatus_r );
WRITE8_HANDLER( fm77av_sub_modestatus_w );
WRITE8_HANDLER( fm77av_sub_bank_w );

READ8_HANDLER( fm7_cancel_ack );
READ8_HANDLER( fm7_attn_irq_r );

READ8_HANDLER( fm7_vram_access_r );
WRITE8_HANDLER( fm7_vram_access_w );
READ8_HANDLER( fm7_vram_r );
WRITE8_HANDLER( fm7_vram_w );
READ8_HANDLER( fm7_crt_r );
WRITE8_HANDLER( fm7_crt_w );
WRITE8_HANDLER( fm7_vram_offset_w );
READ8_HANDLER( fm77av_video_flags_r );
WRITE8_HANDLER( fm77av_video_flags_w );

WRITE8_HANDLER( fm77av_analog_palette_w );
WRITE8_HANDLER( fm7_multipage_w );
READ8_HANDLER( fm7_palette_r );
WRITE8_HANDLER( fm7_palette_w );

READ8_HANDLER( fm7_vram0_r );
READ8_HANDLER( fm7_vram1_r );
READ8_HANDLER( fm7_vram2_r );
READ8_HANDLER( fm7_vram3_r );
READ8_HANDLER( fm7_vram4_r );
READ8_HANDLER( fm7_vram5_r );
READ8_HANDLER( fm7_vram6_r );
READ8_HANDLER( fm7_vram7_r );
READ8_HANDLER( fm7_vram8_r );
READ8_HANDLER( fm7_vram9_r );
READ8_HANDLER( fm7_vramA_r );
READ8_HANDLER( fm7_vramB_r );
WRITE8_HANDLER( fm7_vram0_w );
WRITE8_HANDLER( fm7_vram1_w );
WRITE8_HANDLER( fm7_vram2_w );
WRITE8_HANDLER( fm7_vram3_w );
WRITE8_HANDLER( fm7_vram4_w );
WRITE8_HANDLER( fm7_vram5_w );
WRITE8_HANDLER( fm7_vram6_w );
WRITE8_HANDLER( fm7_vram7_w );
WRITE8_HANDLER( fm7_vram8_w );
WRITE8_HANDLER( fm7_vram9_w );
WRITE8_HANDLER( fm7_vramA_w );
WRITE8_HANDLER( fm7_vramB_w );


VIDEO_START( fm7 );
VIDEO_UPDATE( fm7 );
PALETTE_INIT( fm7 );

#endif /*FM7_H_*/
