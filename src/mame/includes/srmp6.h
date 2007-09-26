


/* vidhrdw/srmp6.c */

extern UINT16 *sprite_ram;	// Sprite RAM 0x400000-0x47dfff
extern UINT16 *dec_regs;	//Graphic decode register

extern UINT16 *video_regs;
extern gfx_layout srmp6_layout_8x8;

READ16_HANDLER( video_regs_r );
WRITE16_HANDLER( video_regs_w );

WRITE16_HANDLER( gfx_decode_w );

READ16_HANDLER( srmp6_sprite_r );
WRITE16_HANDLER( srmp6_sprite_w );

VIDEO_START(srmp6);
VIDEO_UPDATE(srmp6);
