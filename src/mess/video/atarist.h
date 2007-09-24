#ifndef _VIDEO_ATARIST_H_
#define _VIDEO_ATARIST_H_

#define Y2		32084988.0
#define Y2_NTSC	32042400.0

#define ATARIST_HBSTART_PAL		512
#define ATARIST_HBEND_PAL		0
#define ATARIST_HBSTART_NTSC	508
#define ATARIST_HBEND_NTSC		0
#define ATARIST_HTOT_PAL		516
#define ATARIST_HTOT_NTSC		512

#define ATARIST_HBDEND_PAL		14
#define ATARIST_HBDSTART_PAL	94
#define ATARIST_HBDEND_NTSC		13
#define ATARIST_HBDSTART_NTSC	93

#define ATARIST_VBEND_PAL		0
#define ATARIST_VBEND_NTSC		0
#define ATARIST_VBSTART_PAL		312
#define ATARIST_VBSTART_NTSC	262
#define ATARIST_VTOT_PAL		313
#define ATARIST_VTOT_NTSC		263

#define ATARIST_VBDEND_PAL		63
#define ATARIST_VBDSTART_PAL	263
#define ATARIST_VBDEND_NTSC		34
#define ATARIST_VBDSTART_NTSC	234

#define ATARIST_BLITTER_SKEW_NFSR	0x40
#define ATARIST_BLITTER_SKEW_FXSR	0x80

/* Atari ST Shifter */

READ16_HANDLER( atarist_shifter_base_r );
READ16_HANDLER( atarist_shifter_counter_r );
READ16_HANDLER( atarist_shifter_sync_r );
READ16_HANDLER( atarist_shifter_palette_r );
READ16_HANDLER( atarist_shifter_mode_r );

WRITE16_HANDLER( atarist_shifter_base_w );
WRITE16_HANDLER( atarist_shifter_sync_w );
WRITE16_HANDLER( atarist_shifter_palette_w );
WRITE16_HANDLER( atarist_shifter_mode_w );

/* Atari STe Shifter */

READ16_HANDLER( atariste_shifter_base_low_r );
READ16_HANDLER( atariste_shifter_counter_r );
READ16_HANDLER( atariste_shifter_lineofs_r );
READ16_HANDLER( atariste_shifter_pixelofs_r );

WRITE16_HANDLER( atariste_shifter_base_low_w );
WRITE16_HANDLER( atariste_shifter_counter_w );
WRITE16_HANDLER( atariste_shifter_lineofs_w );
WRITE16_HANDLER( atariste_shifter_pixelofs_w );
WRITE16_HANDLER( atariste_shifter_palette_w );

/* Atari ST Blitter */

READ16_HANDLER( atarist_blitter_halftone_r );
READ16_HANDLER( atarist_blitter_src_inc_x_r );
READ16_HANDLER( atarist_blitter_src_inc_y_r );
READ16_HANDLER( atarist_blitter_src_r );
READ16_HANDLER( atarist_blitter_end_mask_r );
READ16_HANDLER( atarist_blitter_dst_inc_x_r );
READ16_HANDLER( atarist_blitter_dst_inc_y_r );
READ16_HANDLER( atarist_blitter_dst_r );
READ16_HANDLER( atarist_blitter_count_x_r );
READ16_HANDLER( atarist_blitter_count_y_r );
READ16_HANDLER( atarist_blitter_op_r );
READ16_HANDLER( atarist_blitter_ctrl_r );

WRITE16_HANDLER( atarist_blitter_halftone_w );
WRITE16_HANDLER( atarist_blitter_src_inc_x_w );
WRITE16_HANDLER( atarist_blitter_src_inc_y_w );
WRITE16_HANDLER( atarist_blitter_src_w );
WRITE16_HANDLER( atarist_blitter_end_mask_w );
WRITE16_HANDLER( atarist_blitter_dst_inc_x_w );
WRITE16_HANDLER( atarist_blitter_dst_inc_y_w );
WRITE16_HANDLER( atarist_blitter_dst_w );
WRITE16_HANDLER( atarist_blitter_count_x_w );
WRITE16_HANDLER( atarist_blitter_count_y_w );
WRITE16_HANDLER( atarist_blitter_op_w );
WRITE16_HANDLER( atarist_blitter_ctrl_w );

/* Video */

VIDEO_START( atarist );
VIDEO_UPDATE( atarist );

#endif
