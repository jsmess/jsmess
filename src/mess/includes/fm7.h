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
#define IRQ_FLAG_UNKNOWN  0x08
// the following are not read in port 0xfd03
#define IRQ_FLAG_MFD      0x10
#define IRQ_FLAG_TXRDY    0x20
#define IRQ_FLAG_RXRDY    0x40
#define IRQ_FLAG_SYNDET   0x80

struct fm7_video_flags
{
	UINT8 sub_busy;
	UINT8 sub_halt;
	UINT8 attn_irq;
	UINT8 vram_access;  // VRAM access flag
	UINT8 crt_enable;
	UINT16 vram_offset;
	UINT8 fm7_pal[8];
};

READ8_HANDLER( fm7_subintf_r );
WRITE8_HANDLER( fm7_subintf_w );
READ8_HANDLER( fm7_sub_busyflag_r );
WRITE8_HANDLER( fm7_sub_busyflag_w );

READ8_HANDLER( fm7_cancel_ack );
READ8_HANDLER( fm7_attn_irq_r );

READ8_HANDLER( fm7_vram_access_r );
WRITE8_HANDLER( fm7_vram_access_w );
READ8_HANDLER( fm7_vram_r );
WRITE8_HANDLER( fm7_vram_w );
READ8_HANDLER( fm7_crt_r );
WRITE8_HANDLER( fm7_crt_w );
WRITE8_HANDLER( fm7_vram_offset_w );

READ8_HANDLER( fm7_palette_r );
WRITE8_HANDLER( fm7_palette_w );

VIDEO_START( fm7 );
VIDEO_UPDATE( fm7 );
PALETTE_INIT( fm7 );

#endif /*FM7_H_*/
