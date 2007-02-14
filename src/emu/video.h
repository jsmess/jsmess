/***************************************************************************

    video.h

    Core MAME video routines.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "mamecore.h"
#include "timer.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* maximum number of screens for one game */
#define MAX_SCREENS					8



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/*-------------------------------------------------
    screen_state - current live state of a screen
-------------------------------------------------*/

typedef struct _screen_state screen_state;
struct _screen_state
{
	int				width, height;				/* total width/height (HTOTAL, VTOTAL) */
	rectangle		visarea;					/* visible area (HBLANK end/start, VBLANK end/start) */
	float			refresh;					/* refresh rate */
	double			vblank;						/* duration of a VBLANK */
	bitmap_format	format;						/* bitmap format */
};


/*-------------------------------------------------
    screen_config - configuration of a single
    screen
-------------------------------------------------*/

typedef struct _screen_config screen_config;
struct _screen_config
{
	const char *	tag;						/* nametag for the screen */
	UINT32			palette_base;				/* base palette entry for this screen */
	screen_state	defstate;					/* default state */
};


/*-------------------------------------------------
    performance_info - information about the
    current performance
-------------------------------------------------*/

/* In mamecore.h: typedef struct _performance_info performance_info; */
struct _performance_info
{
	double			game_speed_percent;			/* % of full speed */
	double			frames_per_second;			/* actual rendered fps */
	int				vector_updates_last_second; /* # of vector updates last second */
	int				partial_updates_this_frame; /* # of partial updates last frame */
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* ----- screen rendering and management ----- */

/* core initialization */
int video_init(running_machine *machine);

/* core VBLANK callback */
void video_vblank_start(void);

/* reset the partial updating for a frame; generally only called by cpuexec.c */
void video_reset_partial_updates(void);

/* are we skipping the current frame? */
int video_skip_this_frame(void);

/* update the screen, handling frame skipping and rendering */
void video_frame_update(void);

/* return current performance data */
const performance_info *mame_get_performance_info(void);


/* ----- screens ----- */

/* set the resolution of a screen */
void video_screen_configure(int scrnum, int width, int height, const rectangle *visarea, float refresh);

/* set the visible area of a screen; this is a subset of video_screen_configure */
void video_screen_set_visarea(int scrnum, int min_x, int max_x, int min_y, int max_y);

/* force a partial update of the screen up to and including the requested scanline */
void video_screen_update_partial(int scrnum, int scanline);

/* return the current vertical or horizontal position of the beam for a screen */
int video_screen_get_vpos(int scrnum);
int video_screen_get_hpos(int scrnum);

/* return the current vertical or horizontal blanking state for a screen */
int video_screen_get_vblank(int scrnum);
int video_screen_get_hblank(int scrnum);

/* return the time when the beam will reach a particular H,V position */
mame_time video_screen_get_time_until_pos(int scrnum, int vpos, int hpos);


/* ----- snapshots ----- */

/* save a snapshot of a given screen */
void video_screen_save_snapshot(mame_file *fp, int scrnum);

/* save a snapshot of all the active screens */
void video_save_active_screen_snapshots(void);


/* ----- movie recording ----- */

/* Movie recording */
int video_is_movie_active(void);
void video_movie_begin_recording(const char *name);
void video_movie_end_recording(void);


/* ----- crosshair rendering ----- */

void video_crosshair_toggle(void);

#endif	/* __VIDEO_H__ */
