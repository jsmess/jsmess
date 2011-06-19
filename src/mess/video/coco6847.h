/*********************************************************************

    coco6847.h

    OBSOLETE Implementation of Motorola 6847 video hardware chip

**********************************************************************/

#ifndef __COCO6847_H__
#define __COCO6847_H__


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
	void (*horizontal_sync_callback)(running_machine &machine, int line);
	void (*field_sync_callback)(running_machine &machine,int line);
	UINT8 (*get_attributes)(running_machine &machine, UINT8 c, int scanline, int pos) ATTR_CONST;
	const UINT8 *(*get_video_ram)(running_machine &machine, int scanline);
	UINT8 (*get_char_rom)(running_machine &machine, UINT8 ch, int line);

	/* needed for the CoCo 3 */
	int (*new_frame_callback)(running_machine &machine);	/* returns whether the M6847 is in charge of this frame */
	void (*custom_prepare_scanline)(running_machine &machine, int scanline);

	const UINT32 *custom_palette;
};

/* creates a new M6847 instance */
void m6847_init(running_machine &machine, const m6847_config *cfg);

/* video update proc */
SCREEN_UPDATE(m6847);
void coco6847_video_changed(void);

/* sync */
int m6847_get_horizontal_sync(running_machine &machine);
int m6847_get_field_sync(running_machine &machine);

/* timing functions */
attotime coco6847_time_delay(running_machine &machine, m6847_timing_type timing, UINT64 target_time);

/* CoCo 3 hooks */
attotime coco6847_scanline_time(int scanline);

INPUT_PORTS_EXTERN( coco6847_artifacting );


#endif /* __COCO6847_H__ */
