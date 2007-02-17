/* m6847.h -- Implementation of Motorola 6847 video hardware chip */

#ifndef _M6847_H
#define _M6847_H

#include "driver.h"
#include "video/generic.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************* Initialization & Functionality *******************/

#define M6847_ARTIFACT_COLOR_COUNT	14

#define M6847_TOTAL_COLORS (13 + (M6847_ARTIFACT_COLOR_COUNT * 4))

#define M6847_INTERRUPTS_PER_FRAME	263

enum {
	M6847_VERSION_ORIGINAL_PAL,
	M6847_VERSION_ORIGINAL_NTSC,
	M6847_VERSION_M6847Y_PAL,
	M6847_VERSION_M6847Y_NTSC,
	M6847_VERSION_M6847T1_PAL,
	M6847_VERSION_M6847T1_NTSC
};

struct m6847_init_params
{
	int version;				/* use one of the above initialization constants */
	int artifactdipswitch;		/* dip switch that controls artifacting; -1 if NA */
	UINT8 *ram;					/* the base of RAM */
	int ramsize;				/* the size of accessible RAM */
	void (*charproc)(UINT8 c);	/* the proc that gives the host a chance to change mode bits */
	int initial_video_offset;	/* the first video offset to use */

	write8_handler hs_func;	/* horizontal sync */
	write8_handler fs_func;	/* field sync */
	double callback_delay;		/* amount of time to wait before invoking callbacks (this is a CoCo related hack */
};

/* This call fills out the params structure with defaults; this is so I can
 * change around the structure without breaking people's code */
void m6847_vh_normalparams(struct m6847_init_params *params);

int video_start_m6847(const struct m6847_init_params *params);
int m6847_is_t1(int version);
extern INTERRUPT_GEN( m6847_vh_interrupt );

#define M6847_VIDEO_TYPE	(VIDEO_TYPE_RASTER)
#define M6847_SCREEN_WIDTH	320
#define M6847_SCREEN_HEIGHT	263

extern void mdrv_m6847(machine_config *machine, int (*video_start_proc)(void), int is_pal);

#define MDRV_M6847_NTSC(video_start_proc)		mdrv_m6847(machine, (video_start_##video_start_proc), 0);
#define MDRV_M6847_PAL(video_start_proc)		mdrv_m6847(machine, (video_start_##video_start_proc), 1);

/******************* Modifiers *******************/

enum {
	M6847_BORDERCOLOR_BLACK,
	M6847_BORDERCOLOR_GREEN,
	M6847_BORDERCOLOR_WHITE,
	M6847_BORDERCOLOR_ORANGE
};

int m6847_get_bordercolor(void);

/* This allows the size of accessable RAM to be resized */
void m6847_set_ram_size(int ramsize);

/* This is the base of video memory, within the ram specified by m6847_vh_start() */
void m6847_set_video_offset(int offset);
int m6847_get_video_offset(void);

/* Touches VRAM; offset is from vram base position */
void m6847_touch_vram(int offset);

/* Changes the height of each row */
void m6847_set_row_height(int rowheight);
void m6847_set_cannonical_row_height(void);

/* Call this function at vblank; so fs and hs will be properly set */
int m6847_vblank(void);

/******************* 1-bit mode port interfaces *******************/

 READ8_HANDLER( m6847_ag_r );
 READ8_HANDLER( m6847_as_r );
 READ8_HANDLER( m6847_intext_r );
 READ8_HANDLER( m6847_inv_r );
 READ8_HANDLER( m6847_css_r );
 READ8_HANDLER( m6847_gm2_r );
 READ8_HANDLER( m6847_gm1_r );
 READ8_HANDLER( m6847_gm0_r );
 READ8_HANDLER( m6847_hs_r );
 READ8_HANDLER( m6847_fs_r );

WRITE8_HANDLER( m6847_ag_w );
WRITE8_HANDLER( m6847_as_w );
WRITE8_HANDLER( m6847_intext_w );
WRITE8_HANDLER( m6847_inv_w );
WRITE8_HANDLER( m6847_css_w );
WRITE8_HANDLER( m6847_gm2_w );
WRITE8_HANDLER( m6847_gm1_w );
WRITE8_HANDLER( m6847_gm0_w );

#ifdef __cplusplus
}
#endif

#endif /* _M6847_H */

