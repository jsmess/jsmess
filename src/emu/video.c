/***************************************************************************

    video.c

    Core MAME video routines.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "profiler.h"
#include "png.h"
#include "debugger.h"
#include "video/vector.h"
#include "render.h"
#include "rendutil.h"
#include "ui.h"

#include "snap.lh"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define LOG_PARTIAL_UPDATES(x)		/* logerror x */

#define FRAMES_PER_FPS_UPDATE		12



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _internal_screen_info internal_screen_info;
struct _internal_screen_info
{
	render_texture *	texture;
	int					format;
	int					changed;
	render_bounds		bounds;
	mame_bitmap *		bitmap[2];
	int					curbitmap;
	int					last_partial_scanline;
	subseconds_t		scantime;
	subseconds_t		pixeltime;
	mame_time 			vblank_time;
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* handy globals for other parts of the system */
int vector_updates = 0;

/* main bitmap to render to */
static int skipping_this_frame;
static internal_screen_info scrinfo[MAX_SCREENS];

/* speed computation */
static osd_ticks_t last_fps_time;
static int frames_since_last_fps;
static int rendered_frames_since_last_fps;
static int vfcount;
static performance_info performance;

/* snapshot stuff */
static render_target *snap_target;
static mame_bitmap *snap_bitmap;
static mame_file *movie_file;
static int movie_frame;

/* crosshair bits */
static mame_bitmap *crosshair_bitmap[MAX_PLAYERS];
static render_texture *crosshair_texture[MAX_PLAYERS];
static UINT8 crosshair_animate;
static UINT8 crosshair_visible;
static UINT8 crosshair_needed;

/* misc other statics */
static UINT32 leds_status;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void video_exit(running_machine *machine);
static void allocate_graphics(const gfx_decode *gfxdecodeinfo);
static void decode_graphics(const gfx_decode *gfxdecodeinfo);
static void init_buffered_spriteram(void);
static void recompute_fps(int skipped_it);
static void movie_record_frame(int scrnum);
static void crosshair_init(void);
static void crosshair_render(void);
static void crosshair_free(void);
static void rgb888_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    video_init - start up the video system
-------------------------------------------------*/

int video_init(running_machine *machine)
{
	int scrnum;

	add_exit_callback(machine, video_exit);

	/* reset globals */
	memset(scrinfo, 0, sizeof(scrinfo));

	/* configure all of the screens */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (machine->drv->screen[scrnum].tag != NULL)
		{
			internal_screen_info *info = &scrinfo[scrnum];
			screen_state *state = &machine->screen[scrnum];

			/* configure the screen with the default parameters */
			video_screen_configure(scrnum, state->width, state->height, &state->visarea, state->refresh);

			/* reset VBLANK timing */
			info->vblank_time = sub_mame_times(time_zero, double_to_mame_time(Machine->screen[0].vblank));

			/* register for save states */
			state_save_register_item("video", scrnum, info->vblank_time.seconds);
			state_save_register_item("video", scrnum, info->vblank_time.subseconds);
		}

	/* create spriteram buffers if necessary */
	if (machine->drv->video_attributes & VIDEO_BUFFERS_SPRITERAM)
		init_buffered_spriteram();

	/* convert the gfx ROMs into character sets. This is done BEFORE calling the driver's */
	/* palette_init() routine because it might need to check the machine->gfx[] data */
	if (machine->drv->gfxdecodeinfo != NULL)
		allocate_graphics(machine->drv->gfxdecodeinfo);

	/* configure the palette */
	palette_config(machine);

	/* actually decode the graphics */
	if (machine->drv->gfxdecodeinfo != NULL)
		decode_graphics(machine->drv->gfxdecodeinfo);

	/* reset performance data */
	last_fps_time = osd_ticks();
	rendered_frames_since_last_fps = frames_since_last_fps = 0;
	performance.game_speed_percent = 100;
	performance.frames_per_second = machine->screen[0].refresh;
	performance.vector_updates_last_second = 0;

	/* reset video statics and get out of here */
	pdrawgfx_shadow_lowpri = 0;
	leds_status = 0;

	/* initialize tilemaps */
	if (tilemap_init(machine) != 0)
		fatalerror("tilemap_init failed");

	/* create a render target for snapshots */
	if (Machine->drv->screen[0].tag != NULL)
	{
		snap_bitmap = NULL;
		snap_target = render_target_alloc(layout_snap, RENDER_CREATE_SINGLE_FILE | RENDER_CREATE_HIDDEN);
		assert(snap_target != NULL);
		if (snap_target == NULL)
			return 1;
		render_target_set_layer_config(snap_target, 0);
	}

	/* create crosshairs */
	crosshair_init();

	return 0;
}


/*-------------------------------------------------
    video_exit - close down the video system
-------------------------------------------------*/

static void video_exit(running_machine *machine)
{
	int scrnum;
	int i;

	/* free crosshairs */
	crosshair_free();

	/* stop recording any movie */
	video_movie_end_recording();

	/* free all the graphics elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
	{
		freegfx(machine->gfx[i]);
		machine->gfx[i] = 0;
	}

	/* free all the textures and bitmaps */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
	{
		internal_screen_info *info = &scrinfo[scrnum];
		if (info->texture != NULL)
			render_texture_free(info->texture);
		if (info->bitmap[0] != NULL)
			bitmap_free(info->bitmap[0]);
		if (info->bitmap[1] != NULL)
			bitmap_free(info->bitmap[1]);
	}

	/* free the snapshot target */
	if (snap_target != NULL)
		render_target_free(snap_target);
	if (snap_bitmap != NULL)
		bitmap_free(snap_bitmap);
}


/*-------------------------------------------------
    video_vblank_start - called at the start of
    VBLANK, which is driven by the CPU scheduler
-------------------------------------------------*/

void video_vblank_start(void)
{
	mame_time curtime = mame_timer_get_time();
	int scrnum;

	/* kludge: we get called at time 0 to reset, but at that point,
       the time of last VBLANK is actually -vblank_duration */
	if (curtime.seconds == 0 && curtime.subseconds == 0)
		curtime = sub_mame_times(time_zero, double_to_mame_time(Machine->screen[0].vblank));

	/* reset VBLANK timers for each screen -- fix me */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		scrinfo[scrnum].vblank_time = curtime;
}


/*-------------------------------------------------
    allocate_graphics - allocate memory for the
    graphics
-------------------------------------------------*/

static void allocate_graphics(const gfx_decode *gfxdecodeinfo)
{
	int i;

	/* loop over all elements */
	for (i = 0; i < MAX_GFX_ELEMENTS && gfxdecodeinfo[i].memory_region != -1; i++)
	{
		int region_length = 8 * memory_region_length(gfxdecodeinfo[i].memory_region);
		UINT32 extxoffs[MAX_ABS_GFX_SIZE], extyoffs[MAX_ABS_GFX_SIZE];
		gfx_layout glcopy;
		int j;

		/* make a copy of the layout */
		glcopy = *gfxdecodeinfo[i].gfxlayout;
		if (glcopy.extxoffs)
		{
			memcpy(extxoffs, glcopy.extxoffs, glcopy.width * sizeof(extxoffs[0]));
			glcopy.extxoffs = extxoffs;
		}
		if (glcopy.extyoffs)
		{
			memcpy(extyoffs, glcopy.extyoffs, glcopy.height * sizeof(extyoffs[0]));
			glcopy.extyoffs = extyoffs;
		}

		/* if the character count is a region fraction, compute the effective total */
		if (IS_FRAC(glcopy.total))
			glcopy.total = region_length / glcopy.charincrement * FRAC_NUM(glcopy.total) / FRAC_DEN(glcopy.total);

		/* for non-raw graphics, decode the X and Y offsets */
		if (glcopy.planeoffset[0] != GFX_RAW)
		{
			UINT32 *xoffset = glcopy.extxoffs ? extxoffs : glcopy.xoffset;
			UINT32 *yoffset = glcopy.extyoffs ? extyoffs : glcopy.yoffset;

			/* loop over all the planes, converting fractions */
			for (j = 0; j < glcopy.planes; j++)
			{
				UINT32 value = glcopy.planeoffset[j];
				if (IS_FRAC(value))
					glcopy.planeoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
			}

			/* loop over all the X/Y offsets, converting fractions */
			for (j = 0; j < glcopy.width; j++)
			{
				UINT32 value = xoffset[j];
				if (IS_FRAC(value))
					xoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
			}

			for (j = 0; j < glcopy.height; j++)
			{
				UINT32 value = yoffset[j];
				if (IS_FRAC(value))
					yoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
			}
		}

		/* otherwise, just use yoffset[0] as the line modulo */
		else
		{
			int base = gfxdecodeinfo[i].start;
			int end = region_length/8;
			while (glcopy.total > 0)
			{
				int elementbase = base + (glcopy.total - 1) * glcopy.charincrement / 8;
				int lastpixelbase = elementbase + glcopy.height * glcopy.yoffset[0] / 8 - 1;
				if (lastpixelbase < end)
					break;
				glcopy.total--;
			}
		}

		/* allocate the graphics */
		Machine->gfx[i] = allocgfx(&glcopy);

		/* if we have a remapped colortable, point our local colortable to it */
		if (Machine->remapped_colortable)
			Machine->gfx[i]->colortable = &Machine->remapped_colortable[gfxdecodeinfo[i].color_codes_start];
		Machine->gfx[i]->total_colors = gfxdecodeinfo[i].total_color_codes;
	}
}


/*-------------------------------------------------
    decode_graphics - decode the graphics
-------------------------------------------------*/

static void decode_graphics(const gfx_decode *gfxdecodeinfo)
{
	int totalgfx = 0, curgfx = 0;
	char buffer[200];
	int i;

	/* count total graphics elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
		if (Machine->gfx[i])
			totalgfx += Machine->gfx[i]->total_elements;

	/* loop over all elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
		if (Machine->gfx[i])
		{
			/* if we have a valid region, decode it now */
			if (gfxdecodeinfo[i].memory_region > REGION_INVALID)
			{
				UINT8 *region_base = memory_region(gfxdecodeinfo[i].memory_region);
				gfx_element *gfx = Machine->gfx[i];
				int j;

				/* now decode the actual graphics */
				for (j = 0; j < gfx->total_elements; j += 1024)
				{
					int num_to_decode = (j + 1024 < gfx->total_elements) ? 1024 : (gfx->total_elements - j);
					decodegfx(gfx, region_base + gfxdecodeinfo[i].start, j, num_to_decode);
					curgfx += num_to_decode;

					/* display some startup text */
					sprintf(buffer, "Decoding (%d%%)", curgfx * 100 / totalgfx);
					ui_set_startup_text(buffer, FALSE);
				}
			}

			/* otherwise, clear the target region */
			else
				memset(Machine->gfx[i]->gfxdata, 0, Machine->gfx[i]->char_modulo * Machine->gfx[i]->total_elements);
		}
}


/*-------------------------------------------------
    init_buffered_spriteram - initialize the
    double-buffered spriteram
-------------------------------------------------*/

static void init_buffered_spriteram(void)
{
	assert_always(spriteram_size != 0, "Video buffers spriteram but spriteram_size is 0");

	/* allocate memory for the back buffer */
	buffered_spriteram = auto_malloc(spriteram_size);

	/* register for saving it */
	state_save_register_global_pointer(buffered_spriteram, spriteram_size);

	/* do the same for the secon back buffer, if present */
	if (spriteram_2_size)
	{
		/* allocate memory */
		buffered_spriteram_2 = auto_malloc(spriteram_2_size);

		/* register for saving it */
		state_save_register_global_pointer(buffered_spriteram_2, spriteram_2_size);
	}

	/* make 16-bit and 32-bit pointer variants */
	buffered_spriteram16 = (UINT16 *)buffered_spriteram;
	buffered_spriteram32 = (UINT32 *)buffered_spriteram;
	buffered_spriteram16_2 = (UINT16 *)buffered_spriteram_2;
	buffered_spriteram32_2 = (UINT32 *)buffered_spriteram_2;
}



/***************************************************************************
    SCREEN RENDERING
***************************************************************************/

/*-------------------------------------------------
    video_screen_configure - configure the parameters
    of a screen
-------------------------------------------------*/

void video_screen_configure(int scrnum, int width, int height, const rectangle *visarea, float refresh)
{
	const screen_config *config = &Machine->drv->screen[scrnum];
	screen_state *state = &Machine->screen[scrnum];
	internal_screen_info *info = &scrinfo[scrnum];
	mame_time timeval;

	/* reallocate bitmap if necessary */
	if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
	{
		int curwidth = 0, curheight = 0;

		/* reality checks */
		if (visarea->min_x < 0 || visarea->min_y < 0 || visarea->max_x >= width || visarea->max_y >= height)
			fatalerror("video_screen_configure(): visible area must be contained within the width/height!");

		/* extract the current width/height from the bitmap */
		if (info->bitmap[0] != NULL)
		{
			curwidth = info->bitmap[0]->width;
			curheight = info->bitmap[0]->height;
		}

		/* if we're too small to contain this width/height, reallocate our bitmaps and textures */
		if (width > curwidth || height > curheight)
		{
			bitmap_format screen_format = state->format;

			/* free what we have currently */
			if (info->texture != NULL)
				render_texture_free(info->texture);
			if (info->bitmap[0] != NULL)
				bitmap_free(info->bitmap[0]);
			if (info->bitmap[1] != NULL)
				bitmap_free(info->bitmap[1]);

			/* compute new width/height */
			curwidth = MAX(width, curwidth);
			curheight = MAX(height, curheight);

			/* choose the texture format */
			/* convert the screen format to a texture format */
			switch (screen_format)
			{
				case BITMAP_FORMAT_INDEXED16:	info->format = TEXFORMAT_PALETTE16;		break;
				case BITMAP_FORMAT_RGB15:		info->format = TEXFORMAT_RGB15;			break;
				case BITMAP_FORMAT_RGB32:		info->format = TEXFORMAT_RGB32;			break;
				default:						fatalerror("Invalid bitmap format!");	break;
			}

			/* allocate new stuff */
			info->bitmap[0] = bitmap_alloc(curwidth, curheight, screen_format);
			info->bitmap[1] = bitmap_alloc(curwidth, curheight, screen_format);
			info->texture = render_texture_alloc(info->bitmap[0], visarea, config->palette_base, info->format, NULL, NULL);
		}
	}

	/* now fill in the new parameters */
	state->width = width;
	state->height = height;
	state->visarea = *visarea;
	state->refresh = refresh;

	/* compute timing parameters */
	timeval = double_to_mame_time(TIME_IN_HZ(refresh) / (double)height);
	assert(timeval.seconds == 0);
	info->scantime = timeval.subseconds;
	info->pixeltime = timeval.subseconds / width;

	/* recompute scanline timing */
	cpu_compute_scanline_timing();
}


/*-------------------------------------------------
    video_screen_set_visarea - just set the visible area
    of a screen
-------------------------------------------------*/

void video_screen_set_visarea(int scrnum, int min_x, int max_x, int min_y, int max_y)
{
	screen_state *state = &Machine->screen[scrnum];
	rectangle visarea;

	visarea.min_x = min_x;
	visarea.max_x = max_x;
	visarea.min_y = min_y;
	visarea.max_y = max_y;

	video_screen_configure(scrnum, state->width, state->height, &visarea, state->refresh);
}


/*-------------------------------------------------
    video_screen_update_partial - perform a partial
    update from the last scanline up to and
    including the specified scanline
-------------------------------------------------*/

void video_screen_update_partial(int scrnum, int scanline)
{
	internal_screen_info *screen = &scrinfo[scrnum];
	rectangle clip = Machine->screen[scrnum].visarea;

	LOG_PARTIAL_UPDATES(("Partial: video_screen_update_partial(%d,%d): ", scrnum, scanline));

	/* these two checks only apply if we're allowed to skip frames */
	if (!(Machine->drv->video_attributes & VIDEO_ALWAYS_UPDATE))
	{
		/* if skipping this frame, bail */
		if (video_skip_this_frame())
		{
			LOG_PARTIAL_UPDATES(("skipped due to frameskipping\n"));
			return;
		}

		/* skip if this screen is not visible anywhere */
		if (!(render_get_live_screens_mask() & (1 << scrnum)))
		{
			LOG_PARTIAL_UPDATES(("skipped because screen not live\n"));
			return;
		}
	}

	/* skip if less than the lowest so far */
	if (scanline < screen->last_partial_scanline)
	{
		LOG_PARTIAL_UPDATES(("skipped because less than previous\n"));
		return;
	}

	/* set the start/end scanlines */
	if (screen->last_partial_scanline > clip.min_y)
		clip.min_y = screen->last_partial_scanline;
	if (scanline < clip.max_y)
		clip.max_y = scanline;

	/* render if necessary */
	if (clip.min_y <= clip.max_y)
	{
		UINT32 flags;

		profiler_mark(PROFILER_VIDEO);
		LOG_PARTIAL_UPDATES(("updating %d-%d\n", clip.min_y, clip.max_y));
		flags = (*Machine->drv->video_update)(Machine, scrnum, screen->bitmap[screen->curbitmap], &clip);
		performance.partial_updates_this_frame++;
		profiler_mark(PROFILER_END);

		/* if we modified the bitmap, we have to commit */
		screen->changed |= (~flags & UPDATE_HAS_NOT_CHANGED);
	}

	/* remember where we left off */
	screen->last_partial_scanline = scanline + 1;
}


/*-------------------------------------------------
    video_screen_get_vpos - returns the current
    vertical position of the beam for a given
    screen
-------------------------------------------------*/

int video_screen_get_vpos(int scrnum)
{
	mame_time delta = sub_mame_times(mame_timer_get_time(), scrinfo[scrnum].vblank_time);
	int vpos;

	assert(delta.seconds == 0);
	vpos = delta.subseconds / scrinfo[scrnum].scantime;
	return (Machine->screen[scrnum].visarea.max_y + 1 + vpos) % Machine->screen[scrnum].height;
}


/*-------------------------------------------------
    video_screen_get_hpos - returns the current
    horizontal position of the beam for a given
    screen
-------------------------------------------------*/

int video_screen_get_hpos(int scrnum)
{
	mame_time delta = sub_mame_times(mame_timer_get_time(), scrinfo[scrnum].vblank_time);
	int vpos, hpos;

	assert(delta.seconds == 0);
	vpos = delta.subseconds / scrinfo[scrnum].scantime;
	hpos = (delta.subseconds - (vpos * scrinfo[scrnum].scantime)) / scrinfo[scrnum].pixeltime;
	return hpos;
}


/*-------------------------------------------------
    video_screen_get_vblank - returns the VBLANK
    state of a given screen
-------------------------------------------------*/

int video_screen_get_vblank(int scrnum)
{
	int vpos = video_screen_get_vpos(scrnum);
	return (vpos < Machine->screen[scrnum].visarea.min_y || vpos > Machine->screen[scrnum].visarea.max_y);
}


/*-------------------------------------------------
    video_screen_get_hblank - returns the HBLANK
    state of a given screen
-------------------------------------------------*/

int video_screen_get_hblank(int scrnum)
{
	int hpos = video_screen_get_hpos(scrnum);
	return (hpos < Machine->screen[scrnum].visarea.min_x || hpos > Machine->screen[scrnum].visarea.max_x);
}


/*-------------------------------------------------
    video_screen_get_time_until_pos - returns the
    amount of time remaining until the beam is
    at the given hpos,vpos
-------------------------------------------------*/

mame_time video_screen_get_time_until_pos(int scrnum, int vpos, int hpos)
{
	mame_time curdelta = sub_mame_times(mame_timer_get_time(), scrinfo[scrnum].vblank_time);
	subseconds_t targetdelta;

	assert(curdelta.seconds == 0);

	/* since we measure time relative to VBLANK, compute the scanline offset from VBLANK */
	vpos += Machine->screen[scrnum].height - (Machine->screen[scrnum].visarea.max_y + 1);
	vpos %= Machine->screen[scrnum].height;

	/* compute the delta for the given X,Y position */
	targetdelta = (subseconds_t)vpos * scrinfo[scrnum].scantime + (subseconds_t)hpos * scrinfo[scrnum].pixeltime;

	/* if we're past that time, head to the next frame */
	if (targetdelta <= curdelta.subseconds)
		targetdelta += DOUBLE_TO_SUBSECONDS(TIME_IN_HZ(Machine->screen[scrnum].refresh));

	/* return the difference */
	return make_mame_time(0, targetdelta - curdelta.subseconds);
}


/*-------------------------------------------------
    video_reset_partial_updates - reset partial updates
    at the start of each frame
-------------------------------------------------*/

void video_reset_partial_updates(void)
{
	int scrnum;

	/* reset partial updates */
	LOG_PARTIAL_UPDATES(("Partial: reset to 0\n"));
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		scrinfo[scrnum].last_partial_scanline = 0;
	performance.partial_updates_this_frame = 0;
}


/*-------------------------------------------------
    recompute_fps - recompute the frame rate
-------------------------------------------------*/

static void recompute_fps(int skipped_it)
{
	/* increment the frame counters */
	frames_since_last_fps++;
	if (!skipped_it)
		rendered_frames_since_last_fps++;

	/* if we didn't skip this frame, we may be able to compute a new FPS */
	if (!skipped_it && frames_since_last_fps >= FRAMES_PER_FPS_UPDATE)
	{
		osd_ticks_t cps = osd_ticks_per_second();
		osd_ticks_t curr = osd_ticks();
		double seconds_elapsed = (double)(curr - last_fps_time) * (1.0 / (double)cps);
		double frames_per_sec = (double)frames_since_last_fps / seconds_elapsed;

		/* compute the performance data */
		performance.game_speed_percent = 100.0 * frames_per_sec / Machine->screen[0].refresh;
		performance.frames_per_second = (double)rendered_frames_since_last_fps / seconds_elapsed;

		/* reset the info */
		last_fps_time = curr;
		frames_since_last_fps = 0;
		rendered_frames_since_last_fps = 0;
	}

	/* for vector games, compute the vector update count once/second */
	vfcount++;
	if (vfcount >= (int)Machine->screen[0].refresh)
	{
		performance.vector_updates_last_second = vector_updates;
		vector_updates = 0;

		vfcount -= (int)Machine->screen[0].refresh;
	}
}


/*-------------------------------------------------
    video_skip_this_frame - accessor to determine if this
    frame is being skipped
-------------------------------------------------*/

int video_skip_this_frame(void)
{
	return skipping_this_frame;
}


/*-------------------------------------------------
    video_frame_update - handle frameskipping and
    UI, plus updating the screen during normal
    operations
-------------------------------------------------*/

void video_frame_update(void)
{
	int skipped_it = video_skip_this_frame();
	int paused = mame_is_paused(Machine);
	int phase = mame_get_phase(Machine);
	int livemask;
	int scrnum;

	/* only render sound and video if we're in the running phase */
	if (phase == MAME_PHASE_RUNNING)
	{
		/* update sound */
		sound_frame_update();

		/* finish updating the screens */
		for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
			if (Machine->drv->screen[scrnum].tag != NULL)
				video_screen_update_partial(scrnum, Machine->screen[scrnum].visarea.max_y);

		/* now add the quads for all the screens */
		livemask = render_get_live_screens_mask();
		for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
			if (livemask & (1 << scrnum))
			{
				internal_screen_info *screen = &scrinfo[scrnum];

				/* only update if empty and not a vector game; otherwise assume the driver did it directly */
				if (render_container_is_empty(render_container_get_screen(scrnum)) && !(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
				{
					mame_bitmap *bitmap = screen->bitmap[screen->curbitmap];
					if (!skipping_this_frame && screen->changed)
					{
						rectangle fixedvis = Machine->screen[scrnum].visarea;
						fixedvis.max_x++;
						fixedvis.max_y++;
						render_texture_set_bitmap(screen->texture, bitmap, &fixedvis, Machine->drv->screen[scrnum].palette_base, screen->format);
						screen->curbitmap = 1 - screen->curbitmap;
					}
					render_screen_add_quad(scrnum, 0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(0xff,0xff,0xff,0xff), screen->texture, PRIMFLAG_BLENDMODE(BLENDMODE_NONE) | PRIMFLAG_SCREENTEX(1));
				}
			}

		/* update our movie recording state */
		if (!paused)
			movie_record_frame(0);

		/* reset the screen changed flags */
		for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
			scrinfo[scrnum].changed = 0;
	}

	/* draw any crosshairs */
	crosshair_render();

	/* draw the user interface */
	ui_update_and_render();

	/* call the OSD to update */
	skipping_this_frame = osd_update(mame_timer_get_time());

	/* empty the containers */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (Machine->drv->screen[scrnum].tag != NULL)
			render_container_empty(render_container_get_screen(scrnum));

	/* update FPS */
	recompute_fps(skipped_it);

	/* call the end-of-frame callback */
	if (phase == MAME_PHASE_RUNNING)
	{
		/* reset partial updates if we're paused or if the debugger is active */
		if (paused || mame_debug_is_active())
			video_reset_partial_updates();

		/* otherwise, call the video EOF callback */
		else if (Machine->drv->video_eof != NULL)
		{
			profiler_mark(PROFILER_VIDEO);
			(*Machine->drv->video_eof)(Machine);
			profiler_mark(PROFILER_END);
		}
	}
}


/*-------------------------------------------------
    mame_get_performance_info - return performance
    info
-------------------------------------------------*/

const performance_info *mame_get_performance_info(void)
{
	return &performance;
}



/***************************************************************************
    SCREEN SNAPSHOTS
***************************************************************************/

/*-------------------------------------------------
    get_snapshot_bitmap - return a pointer to the
    bitmap containing the screenshot for the
    given screen number
-------------------------------------------------*/

static mame_bitmap *get_snapshot_bitmap(int scrnum)
{
	const render_primitive_list *primlist;
	INT32 width, height;

	assert(scrnum >= 0 && scrnum < MAX_SCREENS);

	/* if no screens, do nothing */
	if (snap_target == NULL)
		return NULL;

	/* select the appropriate view in our dummy target */
	render_target_set_view(snap_target, scrnum);

	/* get the minimum width/height and set it on the target */
	render_target_get_minimum_size(snap_target, &width, &height);
	render_target_set_bounds(snap_target, width, height, 0);

	/* if we don't have a bitmap, or if it's not the right size, allocate a new one */
	if (snap_bitmap == NULL || width != snap_bitmap->width || height != snap_bitmap->height)
	{
		if (snap_bitmap != NULL)
			bitmap_free(snap_bitmap);
		snap_bitmap = bitmap_alloc(width, height, BITMAP_FORMAT_RGB32);
		assert(snap_bitmap != NULL);
	}

	/* render the screen there */
	primlist = render_target_get_primitives(snap_target);
	osd_lock_acquire(primlist->lock);
	rgb888_draw_primitives(primlist->head, snap_bitmap->base, width, height, snap_bitmap->rowpixels);
	osd_lock_release(primlist->lock);

	/* now do the actual work */
	return snap_bitmap;
}


/*-------------------------------------------------
    video_screen_save_snapshot - save a snapshot
    to  the given file handle
-------------------------------------------------*/

void video_screen_save_snapshot(mame_file *fp, int scrnum)
{
	png_info pnginfo = { 0 };
	mame_bitmap *bitmap;
	png_error error;
	char text[256];

	/* generate the bitmap to pass in */
	bitmap = get_snapshot_bitmap(scrnum);
	if (bitmap == NULL)
		return;

	/* add two text entries describing the image */
	sprintf(text, APPNAME " %s", build_version);
	png_add_text(&pnginfo, "Software", text);
	sprintf(text, "%s %s", Machine->gamedrv->manufacturer, Machine->gamedrv->description);
	png_add_text(&pnginfo, "System", text);

	/* now do the actual work */
	error = png_write_bitmap(mame_core_file(fp), &pnginfo, snap_bitmap, Machine->drv->total_colors, palette_get_adjusted_colors(Machine));

	/* free any data allocated */
	png_free(&pnginfo);
}


/*-------------------------------------------------
    mame_fopen_next - open the next non-existing
    file of type filetype according to our
    numbering scheme
-------------------------------------------------*/

static mame_file_error mame_fopen_next(const char *pathoption, const char *extension, mame_file **file)
{
	mame_file_error filerr;
	char *fname;
	int seq;

	/* allocate temp space for the name */
	fname = malloc_or_die(strlen(Machine->basename) + 1 + 10 + strlen(extension) + 1);

	/* try until we succeed */
	for (seq = 0; ; seq++)
	{
		sprintf(fname, "%s" PATH_SEPARATOR "%04d.%s", Machine->basename, seq, extension);
		filerr = mame_fopen(pathoption, fname, OPEN_FLAG_READ, file);
		if (filerr != FILERR_NONE)
			break;
		mame_fclose(*file);
	}

	/* create the final file */
    filerr = mame_fopen(pathoption, fname, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, file);

    /* free the name and get out */
    free(fname);
    return filerr;
}


/*-------------------------------------------------
    video_save_active_screen_snapshots - save a
    snapshot of all active screens
-------------------------------------------------*/

void video_save_active_screen_snapshots(void)
{
	UINT32 screenmask = render_get_live_screens_mask();
	mame_file *fp;
	int scrnum;

	/* write one snapshot per visible screen */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (screenmask & (1 << scrnum))
		{
			mame_file_error filerr = mame_fopen_next(SEARCHPATH_SCREENSHOT, "png", &fp);
			if (filerr == FILERR_NONE)
			{
				video_screen_save_snapshot(fp, scrnum);
				mame_fclose(fp);
			}
		}
}



/***************************************************************************
    MNG MOVIE RECORDING
***************************************************************************/

/*-------------------------------------------------
    video_is_movie_active - return true if a movie
    is currently being recorded
-------------------------------------------------*/

int video_is_movie_active(void)
{
	return (movie_file != NULL);
}



/*-------------------------------------------------
    video_movie_begin_recording - begin recording
    of a MNG movie
-------------------------------------------------*/

void video_movie_begin_recording(const char *name)
{
	mame_file_error filerr;

	/* close any existing movie file */
	if (movie_file != NULL)
		video_movie_end_recording();

	/* create a new movie file and start recording */
	if (name != NULL)
		filerr = mame_fopen(SEARCHPATH_MOVIE, name, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &movie_file);
	else
		filerr = mame_fopen_next(SEARCHPATH_MOVIE, "mng", &movie_file);

	movie_frame = 0;
}


/*-------------------------------------------------
    video_movie_end_recording - stop recording of
    a MNG movie
-------------------------------------------------*/

void video_movie_end_recording(void)
{
	/* close the file if it exists */
	if (movie_file != NULL)
	{
		mng_capture_stop(mame_core_file(movie_file));
		mame_fclose(movie_file);
		movie_file = NULL;
		movie_frame = 0;
	}
}


/*-------------------------------------------------
    movie_record_frame - record a frame of a
    movie
-------------------------------------------------*/

static void movie_record_frame(int scrnum)
{
	/* only record if we have a file */
	if (movie_file != NULL)
	{
		png_info info = { 0 };
		mame_bitmap *bitmap;
		png_error error;

		profiler_mark(PROFILER_MOVIE_REC);

		/* get the bitmap */
		bitmap = get_snapshot_bitmap(scrnum);
		if (bitmap == NULL)
			return;

		/* track frames */
		if (movie_frame++ == 0)
		{
			char text[256];

			/* set up the text fields in the movie info */
			sprintf(text, APPNAME " %s", build_version);
			png_add_text(&info, "Software", text);
			sprintf(text, "%s %s", Machine->gamedrv->manufacturer, Machine->gamedrv->description);
			png_add_text(&info, "System", text);

			/* start the capture */
			error = mng_capture_start(mame_core_file(movie_file), bitmap, Machine->screen[0].refresh);
			if (error != PNGERR_NONE)
			{
				png_free(&info);
				video_movie_end_recording();
				return;
			}
		}

		/* write the next frame */
		error = mng_capture_frame(mame_core_file(movie_file), &info, bitmap, Machine->drv->total_colors, palette_get_adjusted_colors(Machine));
		png_free(&info);
		if (error != PNGERR_NONE)
		{
			video_movie_end_recording();
			return;
		}

		profiler_mark(PROFILER_END);
	}
}



/***************************************************************************
    CROSSHAIR RENDERING
***************************************************************************/

/* decripton of the bitmap data */
#define CROSSHAIR_RAW_SIZE		100
#define CROSSHAIR_RAW_ROWBYTES	((CROSSHAIR_RAW_SIZE + 7) / 8)

/* raw bitmap */
static const UINT8 crosshair_raw_top[] =
{
	0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,
	0x00,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,
	0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xF0,0x00,
	0x01,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xF8,0x00,
	0x03,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xFC,0x00,
	0x07,0xFE,0x00,0x00,0x00,0x0F,0xFE,0x00,0x00,0x00,0x07,0xFE,0x00,
	0x0F,0xFF,0x00,0x00,0x01,0xFF,0xFF,0xF0,0x00,0x00,0x0F,0xFF,0x00,
	0x1F,0xFF,0x80,0x00,0x1F,0xFF,0xFF,0xFF,0x00,0x00,0x1F,0xFF,0x80,
	0x3F,0xFF,0x80,0x00,0xFF,0xFF,0xFF,0xFF,0xE0,0x00,0x1F,0xFF,0xC0,
	0x7F,0xFF,0xC0,0x03,0xFF,0xFF,0xFF,0xFF,0xF8,0x00,0x3F,0xFF,0xE0,
	0xFF,0xFF,0xE0,0x07,0xFF,0xFF,0xFF,0xFF,0xFC,0x00,0x7F,0xFF,0xF0,
	0x7F,0xFF,0xF0,0x1F,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0xFF,0xFF,0xE0,
	0x3F,0xFF,0xF8,0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xC1,0xFF,0xFF,0xC0,
	0x0F,0xFF,0xF8,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xE1,0xFF,0xFF,0x00,
	0x07,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFB,0xFF,0xFE,0x00,
	0x03,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFC,0x00,
	0x01,0xFF,0xFF,0xFF,0xFF,0xF0,0x01,0xFF,0xFF,0xFF,0xFF,0xF8,0x00,
	0x00,0x7F,0xFF,0xFF,0xFF,0x00,0x00,0x1F,0xFF,0xFF,0xFF,0xE0,0x00,
	0x00,0x3F,0xFF,0xFF,0xF8,0x00,0x00,0x03,0xFF,0xFF,0xFF,0xC0,0x00,
	0x00,0x1F,0xFF,0xFF,0xE0,0x00,0x00,0x00,0xFF,0xFF,0xFF,0x80,0x00,
	0x00,0x0F,0xFF,0xFF,0x80,0x00,0x00,0x00,0x3F,0xFF,0xFF,0x00,0x00,
	0x00,0x03,0xFF,0xFE,0x00,0x00,0x00,0x00,0x0F,0xFF,0xFC,0x00,0x00,
	0x00,0x01,0xFF,0xFC,0x00,0x00,0x00,0x00,0x07,0xFF,0xF8,0x00,0x00,
	0x00,0x03,0xFF,0xF8,0x00,0x00,0x00,0x00,0x01,0xFF,0xF8,0x00,0x00,
	0x00,0x07,0xFF,0xFC,0x00,0x00,0x00,0x00,0x03,0xFF,0xFC,0x00,0x00,
	0x00,0x0F,0xFF,0xFE,0x00,0x00,0x00,0x00,0x07,0xFF,0xFE,0x00,0x00,
	0x00,0x0F,0xFF,0xFF,0x00,0x00,0x00,0x00,0x0F,0xFF,0xFE,0x00,0x00,
	0x00,0x1F,0xFF,0xFF,0x80,0x00,0x00,0x00,0x1F,0xFF,0xFF,0x00,0x00,
	0x00,0x1F,0xFF,0xFF,0x80,0x00,0x00,0x00,0x1F,0xFF,0xFF,0x00,0x00,
	0x00,0x3F,0xFE,0xFF,0xC0,0x00,0x00,0x00,0x3F,0xFF,0xFF,0x80,0x00,
	0x00,0x7F,0xFC,0x7F,0xE0,0x00,0x00,0x00,0x7F,0xE7,0xFF,0xC0,0x00,
	0x00,0x7F,0xF8,0x3F,0xF0,0x00,0x00,0x00,0xFF,0xC3,0xFF,0xC0,0x00,
	0x00,0xFF,0xF8,0x1F,0xF8,0x00,0x00,0x01,0xFF,0x83,0xFF,0xE0,0x00,
	0x00,0xFF,0xF0,0x07,0xF8,0x00,0x00,0x01,0xFE,0x01,0xFF,0xE0,0x00,
	0x00,0xFF,0xF0,0x03,0xFC,0x00,0x00,0x03,0xFC,0x01,0xFF,0xE0,0x00,
	0x01,0xFF,0xE0,0x01,0xFE,0x00,0x00,0x07,0xF8,0x00,0xFF,0xF0,0x00,
	0x01,0xFF,0xE0,0x00,0xFF,0x00,0x00,0x0F,0xF0,0x00,0xFF,0xF0,0x00,
	0x01,0xFF,0xC0,0x00,0x3F,0x80,0x00,0x1F,0xC0,0x00,0x7F,0xF0,0x00,
	0x01,0xFF,0xC0,0x00,0x1F,0x80,0x00,0x1F,0x80,0x00,0x7F,0xF0,0x00,
	0x03,0xFF,0xC0,0x00,0x0F,0xC0,0x00,0x3F,0x00,0x00,0x7F,0xF8,0x00,
	0x03,0xFF,0x80,0x00,0x07,0xE0,0x00,0x7E,0x00,0x00,0x3F,0xF8,0x00,
	0x03,0xFF,0x80,0x00,0x01,0xF0,0x00,0xF8,0x00,0x00,0x3F,0xF8,0x00,
	0x03,0xFF,0x80,0x00,0x00,0xF8,0x01,0xF0,0x00,0x00,0x3F,0xF8,0x00,
	0x03,0xFF,0x80,0x00,0x00,0x78,0x01,0xE0,0x00,0x00,0x3F,0xF8,0x00,
	0x07,0xFF,0x00,0x00,0x00,0x3C,0x03,0xC0,0x00,0x00,0x3F,0xFC,0x00,
	0x07,0xFF,0x00,0x00,0x00,0x0E,0x07,0x00,0x00,0x00,0x1F,0xFC,0x00,
	0x07,0xFF,0x00,0x00,0x00,0x07,0x0E,0x00,0x00,0x00,0x1F,0xFC,0x00,
	0x07,0xFF,0x00,0x00,0x00,0x03,0x9C,0x00,0x00,0x00,0x1F,0xFC,0x00,
	0x07,0xFF,0x00,0x00,0x00,0x01,0x98,0x00,0x00,0x00,0x1F,0xFC,0x00,
	0x07,0xFF,0x00,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x1F,0xFC,0x00
};

/* per-player colors */
static const rgb_t crosshair_colors[] =
{
	MAKE_RGB(0x40,0x40,0xff),
	MAKE_RGB(0xff,0x40,0x40),
	MAKE_RGB(0x40,0xff,0x40),
	MAKE_RGB(0xff,0xff,0x40),
	MAKE_RGB(0xff,0x40,0xff),
	MAKE_RGB(0x40,0xff,0xff),
	MAKE_RGB(0xff,0xff,0xff)
};


/*-------------------------------------------------
    crosshair_init - initialize the crosshair
    bitmaps and such
-------------------------------------------------*/

static void crosshair_init(void)
{
	input_port_entry *ipt;
	int player;

	/* determine who needs crosshairs */
	crosshair_needed = 0x00;
	for (ipt = Machine->input_ports; ipt->type != IPT_END; ipt++)
		if (ipt->analog.crossaxis != CROSSHAIR_AXIS_NONE)
			crosshair_needed |= 1 << ipt->player;

	/* all visible by default */
	crosshair_visible = crosshair_needed;

	/* loop over each player and load or create a bitmap */
	for (player = 0; player < MAX_PLAYERS; player++)
		if (crosshair_needed & (1 << player))
		{
			char filename[20];

			/* first try to load a bitmap for the crosshair */
			sprintf(filename, "cross%d.png", player);
			crosshair_bitmap[player] = render_load_png(NULL, filename, NULL, NULL);

			/* if that didn't work, make one up */
			if (crosshair_bitmap[player] == NULL)
			{
				rgb_t color = crosshair_colors[player];
				int x, y;

				/* allocate a blank bitmap to start with */
				crosshair_bitmap[player] = bitmap_alloc(CROSSHAIR_RAW_SIZE, CROSSHAIR_RAW_SIZE, BITMAP_FORMAT_ARGB32);
				fillbitmap(crosshair_bitmap[player], MAKE_ARGB(0x00,0xff,0xff,0xff), NULL);

				/* extract the raw source data to it */
				for (y = 0; y < CROSSHAIR_RAW_SIZE / 2; y++)
				{
					/* assume it is mirrored vertically */
					UINT32 *dest0 = BITMAP_ADDR32(crosshair_bitmap[player], y, 0);
					UINT32 *dest1 = BITMAP_ADDR32(crosshair_bitmap[player], CROSSHAIR_RAW_SIZE - 1 - y, 0);

					/* extract to two rows simultaneously */
					for (x = 0; x < CROSSHAIR_RAW_SIZE; x++)
						if ((crosshair_raw_top[y * CROSSHAIR_RAW_ROWBYTES + x / 8] << (x % 8)) & 0x80)
							dest0[x] = dest1[x] = MAKE_ARGB(0xff,0x00,0x00,0x00) | color;
				}
			}

			/* create a texture to reference the bitmap */
			crosshair_texture[player] = render_texture_alloc(crosshair_bitmap[player], NULL, 0, TEXFORMAT_ARGB32, render_texture_hq_scale, NULL);
		}
}


/*-------------------------------------------------
    video_crosshair_toggle - toggle crosshair
    visibility
-------------------------------------------------*/

void video_crosshair_toggle(void)
{
	int player;

	/* if we're all visible, turn all off */
	if (crosshair_visible == crosshair_needed)
		crosshair_visible = 0;

	/* otherwise, turn on the first bit that isn't currently on and stop there */
	else
		for (player = 0; player < MAX_PLAYERS; player++)
			if ((crosshair_needed & (1 << player)) && !(crosshair_visible & (1 << player)))
			{
				crosshair_visible |= 1 << player;
				break;
			}
}


/*-------------------------------------------------
    crosshair_render - render the crosshairs
-------------------------------------------------*/

static void crosshair_render(void)
{
	float x[MAX_PLAYERS], y[MAX_PLAYERS];
	input_port_entry *ipt;
	int portnum = -1;
	int player;
	UINT8 tscale;

	/* skip if not needed */
	if (crosshair_visible == 0)
		return;

	/* animate via crosshair_animate */
	crosshair_animate += 0x04;

	/* compute a color scaling factor from the current animation value */
	if (crosshair_animate < 0x80)
		tscale = 0xa0 + (0x60 * (crosshair_animate & 0x7f) / 0x80);
	else
		tscale = 0xa0 + (0x60 * (~crosshair_animate & 0x7f) / 0x80);

	/* read all the lightgun values */
	for (ipt = Machine->input_ports; ipt->type != IPT_END; ipt++)
	{
		/* keep track of the port number */
		if (ipt->type == IPT_PORT)
			portnum++;

		/* compute the values */
		if (ipt->analog.crossaxis != CROSSHAIR_AXIS_NONE)
		{
			float value = (float)(get_crosshair_pos(portnum, ipt->player, ipt->analog.crossaxis) - ipt->analog.min) / (float)(ipt->analog.max - ipt->analog.min);
			if (ipt->analog.crossscale < 0)
				value = -(1.0 - value) * ipt->analog.crossscale;
			else
				value *= ipt->analog.crossscale;
			value += ipt->analog.crossoffset;

			/* switch off the axis */
			switch (ipt->analog.crossaxis)
			{
				case CROSSHAIR_AXIS_X:
					x[ipt->player] = value;
					if (ipt->analog.crossaltaxis != 0)
						y[ipt->player] = ipt->analog.crossaltaxis;
					break;

				case CROSSHAIR_AXIS_Y:
					y[ipt->player] = value;
					if (ipt->analog.crossaltaxis != 0)
						x[ipt->player] = ipt->analog.crossaltaxis;
					break;
			}
		}
	}

	/* draw all crosshairs */
	for (player = 0; player < MAX_PLAYERS; player++)
		if (crosshair_visible & (1 << player))
		{
			/* add a quad assuming a 4:3 screen (this is not perfect) */
			render_screen_add_quad(0,
						x[player] - 0.03f, y[player] - 0.04f,
						x[player] + 0.03f, y[player] + 0.04f,
						MAKE_ARGB(0xc0, tscale, tscale, tscale),
						crosshair_texture[player], PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
		}
}


/*-------------------------------------------------
    crosshair_free - free memory allocated for
    the crosshairs
-------------------------------------------------*/

static void crosshair_free(void)
{
	int player;

	/* free bitmaps and textures for each player */
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		if (crosshair_texture[player] != NULL)
			render_texture_free(crosshair_texture[player]);
		crosshair_texture[player] = NULL;

		if (crosshair_bitmap[player] != NULL)
			bitmap_free(crosshair_bitmap[player]);
		crosshair_bitmap[player] = NULL;
	}
}



/***************************************************************************
    SOFTWARE RENDERING
***************************************************************************/

#define FUNC_PREFIX(x)		rgb888_##x
#define PIXEL_TYPE			UINT32
#define SRCSHIFT_R			0
#define SRCSHIFT_G			0
#define SRCSHIFT_B			0
#define DSTSHIFT_R			16
#define DSTSHIFT_G			8
#define DSTSHIFT_B			0

#include "rendersw.c"
