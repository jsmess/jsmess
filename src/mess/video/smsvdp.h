#ifndef _VIDEO_SMSVDP_H_
#define _VIDEO_SMSVDP_H_

#define MODEL_315_5124			0x0001
#define MODEL_315_5246			0x0002
#define MODEL_315_5378			0x0004

#define SMS_X_PIXELS			342		/* 342 pixels */
#define NTSC_Y_PIXELS			262		/* 262 lines */
#define PAL_Y_PIXELS			313		/* 313 lines */
#define LBORDER_START			( 1 + 2 + 14 + 8 )
#define LBORDER_X_PIXELS		(0x0D)		/* 13 pixels */
#define RBORDER_X_PIXELS		(0x0F)		/* 15 pixels */
#define TBORDER_START			( 3 + 13 )
#define NTSC_192_TBORDER_Y_PIXELS	(0x1B)		/* 27 lines */
#define NTSC_192_BBORDER_Y_PIXELS	(0x18)		/* 24 lines */
#define NTSC_224_TBORDER_Y_PIXELS	(0x0B)		/* 11 lines */
#define NTSC_224_BBORDER_Y_PIXELS	(0x08)		/* 8 lines */
#define PAL_192_TBORDER_Y_PIXELS	(0x36)		/* 54 lines */
#define PAL_192_BBORDER_Y_PIXELS	(0x30)		/* 48 lines */
#define PAL_224_TBORDER_Y_PIXELS	(0x26)		/* 38 lines */
#define PAL_224_BBORDER_Y_PIXELS	(0x20)		/* 32 lines */
#define PAL_240_TBORDER_Y_PIXELS	(0x1E)		/* 30 lines */
#define PAL_240_BBORDER_Y_PIXELS	(0x18)		/* 24 lines */


typedef struct smsvdp_configuration {
	UINT32	model;			/* Select model/features for the emulation */
	void	(*int_callback)(running_machine*,int);	/* interupt callback function */
} smsvdp_configuration;


/* prototypes */

int smsvdp_video_init( running_machine *machine, const smsvdp_configuration *config );
VIDEO_UPDATE(sms);
 READ8_HANDLER(sms_vdp_vcount_r);
 READ8_HANDLER(sms_vdp_hcount_r);
 READ8_HANDLER(sms_vdp_data_r);
WRITE8_HANDLER(sms_vdp_data_w);
 READ8_HANDLER(sms_vdp_ctrl_r);
WRITE8_HANDLER(sms_vdp_ctrl_w);
void sms_set_ggsmsmode(int mode);

#endif /* _VIDEO_SMSVDP_H_ */
