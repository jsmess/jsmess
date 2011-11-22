/*************************************************************************

    smsvdp.h

    Implementation of Sega VDP chip used in Master System and Game Gear

**************************************************************************/

#ifndef __SMSVDP_H__
#define __SMSVDP_H__

#include "devcb.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/


#define MODEL_315_5124			0x0001
#define MODEL_315_5246			0x0002
#define MODEL_315_5378			0x0004

#define SMS_X_PIXELS			342		/* 342 pixels */
#define SMS_NTSC_Y_PIXELS			262		/* 262 lines */
#define SMS_PAL_Y_PIXELS			313		/* 313 lines */
#define SMS_LBORDER_START			(1 + 2 + 14 + 8)
#define SMS_LBORDER_X_PIXELS		(0x0d)		/* 13 pixels */
#define SMS_RBORDER_X_PIXELS		(0x0f)		/* 15 pixels */
#define SMS_TBORDER_START			(3 + 13)
#define SMS_NTSC_192_TBORDER_Y_PIXELS	(0x1b)		/* 27 lines */
//#define NTSC_192_BBORDER_Y_PIXELS	(0x18)		/* 24 lines */
#define SMS_NTSC_224_TBORDER_Y_PIXELS	(0x0b)		/* 11 lines */
//#define NTSC_224_BBORDER_Y_PIXELS	(0x08)		/* 8 lines */
//#define PAL_192_TBORDER_Y_PIXELS	(0x36)		/* 54 lines */
//#define PAL_192_BBORDER_Y_PIXELS	(0x30)		/* 48 lines */
//#define PAL_224_TBORDER_Y_PIXELS	(0x26)		/* 38 lines */
//#define PAL_224_BBORDER_Y_PIXELS	(0x20)		/* 32 lines */
#define SMS_PAL_240_TBORDER_Y_PIXELS	(0x1e)		/* 30 lines */
//#define PAL_240_BBORDER_Y_PIXELS	(0x18)		/* 24 lines */


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _smsvdp_interface smsvdp_interface;
struct _smsvdp_interface
{
	UINT32             model;                /* Select model/features for the emulation */
	devcb_write_line   int_callback;         /* Interrupt callback function */
	devcb_write_line   pause_callback;       /* Pause callback function */
};

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(SMSVDP, smsvdp);

#define MCFG_SMSVDP_ADD(_tag, _interface) \
	MCFG_DEVICE_ADD(_tag, SMSVDP, 0) \
	MCFG_DEVICE_CONFIG(_interface)


/***************************************************************************
    DEVICE I/O FUNCTIONS
***************************************************************************/

/* prototypes */

UINT32 sms_vdp_update( device_t *device, bitmap_t *bitmap, const rectangle *cliprect );
READ8_DEVICE_HANDLER( sms_vdp_vcount_r );
READ8_DEVICE_HANDLER( sms_vdp_hcount_latch_r );
WRITE8_DEVICE_HANDLER( sms_vdp_hcount_latch_w );
READ8_DEVICE_HANDLER( sms_vdp_data_r );
WRITE8_DEVICE_HANDLER( sms_vdp_data_w );
READ8_DEVICE_HANDLER( sms_vdp_ctrl_r );
WRITE8_DEVICE_HANDLER( sms_vdp_ctrl_w );
void sms_vdp_set_ggsmsmode( device_t *device, int mode );
int sms_vdp_check_brightness( device_t *device, int x, int y );

#endif /* __SMSVDP_H__ */
