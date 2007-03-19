/*********************************************************************

    m6847.h

	Implementation of Motorola 6847 video hardware chip

**********************************************************************/

#ifndef __M6847_H__
#define __M6847_H__

#include "mame.h"

/* for now, the MAME core forces us to use these */
#define M6847_NTSC_FRAMES_PER_SECOND	60
#define M6847_PAL_FRAMES_PER_SECOND		50

typedef enum
{
	M6847_VERSION_ORIGINAL_NTSC,
	M6847_VERSION_ORIGINAL_PAL,
	M6847_VERSION_M6847Y_NTSC,
	M6847_VERSION_M6847Y_PAL,
	M6847_VERSION_M6847T1_NTSC,
	M6847_VERSION_M6847T1_PAL,
	M6847_VERSION_GIME_NTSC,
	M6847_VERSION_GIME_PAL
} m6847_type;

enum
{
	M6847_AG		= 0x80,
	M6847_AS		= 0x40,
	M6847_INTEXT	= 0x20,
	M6847_INV		= 0x10,
	M6847_CSS		= 0x08,
	M6847_GM2		= 0x04,
	M6847_GM1		= 0x02,
	M6847_GM0		= 0x01
};

typedef enum
{
	M6847_CLOCK,
	M6847_HSYNC
} m6847_timing_type;

typedef struct _m6847_config m6847_config;
struct _m6847_config
{
	m6847_type type;
	int cpu0_timing_factor;

	/* callbacks */
	void (*horizontal_sync_callback)(int line);
	void (*field_sync_callback)(int line);
	UINT8 (*get_attributes)(UINT8 video_byte) ATTR_CONST;
	const UINT8 *(*get_video_ram)(int scanline);

	/* needed for the CoCo 3 */
	int (*new_frame_callback)(void);	/* returns whether the M6847 is in charge of this frame */
	void (*custom_prepare_scanline)(int scanline);

	const UINT32 *custom_palette;
};

/* creates a new M6847 instance */
void m6847_init(const m6847_config *cfg);

/* video update proc */
VIDEO_UPDATE(m6847);
void m6847_video_changed(void);

/* sync */
int m6847_get_horizontal_sync(void);
int m6847_get_field_sync(void);

/* timing functions */
UINT64 m6847_time(m6847_timing_type timing);
mame_time m6847_time_until(m6847_timing_type timing, UINT64 target_time);

/* CoCo 3 hooks */
mame_time m6847_scanline_time(int scanline);

INPUT_PORTS_EXTERN( m6847_artifacting );

#endif /* __M6847_H__ */
