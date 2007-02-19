//============================================================
//
//  sound.c - Win32 implementation of MAME sound routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

// undef WINNT for dsound.h to prevent duplicate definition
#undef WINNT
//#define LPCWAVEFORMATEX _LPCWAVEFORMATEX
#include <dsound.h>
//#undef LPWAVEFORMATEX

// MAME headers
#include "driver.h"
#include "osdepend.h"
#include "sound/wavwrite.h"

// MAMEOS headers
#include "winmain.h"
#include "window.h"
#include "video.h"


//============================================================
//  DEBUGGING
//============================================================

#define LOG_SOUND				0
#define DISPLAY_UNDEROVERFLOW	0



//============================================================
//  PARAMETERS
//============================================================

#define INGORE_UNDERFLOW_FRAMES	100

// the local buffer is what the stream buffer feeds from
// note that this needs to be large enough to buffer at frameskip 11
// for 30fps games like Tapper; we will scale the value down based
// on the actual framerate of the game
#define MAX_BUFFER_SIZE			(128 * 1024)

// this is the maximum number of extra samples we will ask for
// per frame (I know this looks like a lot, but most of the
// time it will generally be nowhere close to this)
#define MAX_SAMPLE_ADJUST		16



//============================================================
//  GLOBAL VARIABLES
//============================================================

// global parameters
int							attenuation = 0;

int							audio_latency;
char *						wavwrite;


//============================================================
//  LOCAL VARIABLES
//============================================================

// DirectSound objects
static LPDIRECTSOUND		dsound;
static DSCAPS				dsound_caps;

// sound buffers
static LPDIRECTSOUNDBUFFER	primary_buffer;
static LPDIRECTSOUNDBUFFER	stream_buffer;
static UINT32				stream_buffer_size;
static UINT32				stream_buffer_in;

// descriptors and formats
static DSBUFFERDESC			primary_desc;
static DSBUFFERDESC			stream_desc;
static WAVEFORMATEX			primary_format;
static WAVEFORMATEX			stream_format;

// buffer over/underflow counts
static int					total_frames;
static int					buffer_underflows;
static int					buffer_overflows;

// global sample tracking
static double				samples_per_frame;
static double				samples_left_over;
static UINT32				samples_this_frame;

// sample rate adjustments
static int					current_adjustment = 0;
static int					lower_thresh;
static int					upper_thresh;

// enabled state
static int					is_enabled = 1;

// debugging
#if LOG_SOUND
static FILE *				sound_log;
#endif

static void *				wavptr;


//============================================================
//  PROTOTYPES
//============================================================

static int			dsound_init(void);
static void			dsound_kill(void);
static int			dsound_create_buffers(void);
static void			dsound_destroy_buffers(void);



//============================================================
//  bytes_in_stream_buffer
//============================================================

INLINE int bytes_in_stream_buffer(void)
{
	DWORD play_position, write_position;
	HRESULT result;

	result = IDirectSoundBuffer_GetCurrentPosition(stream_buffer, &play_position, &write_position);
	if (stream_buffer_in > play_position)
		return stream_buffer_in - play_position;
	else
		return stream_buffer_size + stream_buffer_in - play_position;
}



//============================================================
//  osd_start_audio_stream
//============================================================

int osd_start_audio_stream(int stereo)
{
#if LOG_SOUND
	sound_log = fopen("sound.log", "w");
#endif

	// attempt to initialize directsound
	if (dsound_init())
		return 1;

	// set the startup volume
	osd_set_mastervolume(attenuation);

	// determine the number of samples per frame
	samples_per_frame = (double)Machine->sample_rate / (double)Machine->screen[0].refresh;

	// compute how many samples to generate the first frame
	samples_left_over = samples_per_frame;
	samples_this_frame = (UINT32)samples_left_over;
	samples_left_over -= (double)samples_this_frame;

	// create wav file
	if( wavwrite != NULL )
	{
		wavptr = wav_open( wavwrite, Machine->sample_rate, 2);
	}
	else
	{
		wavptr = NULL;
	}

	// return the samples to play the first frame
	return samples_this_frame;
}



//============================================================
//  osd_stop_audio_stream
//============================================================

void osd_stop_audio_stream(void)
{
	if( wavptr != NULL )
	{
		wav_close( wavptr );
		wavptr = NULL;
	}

	// kill the buffers and dsound
	dsound_destroy_buffers();
	dsound_kill();

	// print out over/underflow stats
	if (buffer_overflows || buffer_underflows)
		verbose_printf("Sound: buffer overflows=%d underflows=%d\n", buffer_overflows, buffer_underflows);

#if LOG_SOUND
	if (sound_log)
		fprintf(sound_log, "Sound buffer: overflows=%d underflows=%d\n", buffer_overflows, buffer_underflows);
	fclose(sound_log);
#endif
}



//============================================================
//  update_sample_adjustment
//============================================================

static void update_sample_adjustment(int buffered)
{
	static int consecutive_lows = 0;
	static int consecutive_mids = 0;
	static int consecutive_highs = 0;

	// if we're not throttled don't bother
	if (!video_config.throttle)
	{
		consecutive_lows = 0;
		consecutive_mids = 0;
		consecutive_highs = 0;
		current_adjustment = 0;
		return;
	}

#if LOG_SOUND
	fprintf(sound_log, "update_sample_adjustment: %d\n", buffered);
#endif

	// do we have too few samples in the buffer?
	if (buffered < lower_thresh)
	{
		// keep track of how many consecutive times we get this condition
		consecutive_lows++;
		consecutive_mids = 0;
		consecutive_highs = 0;

		// adjust so that we generate more samples per frame to compensate
		current_adjustment = (consecutive_lows < MAX_SAMPLE_ADJUST) ? consecutive_lows : MAX_SAMPLE_ADJUST;

#if LOG_SOUND
		fprintf(sound_log, "  (too low - adjusting to %d)\n", current_adjustment);
#endif
	}

	// do we have too many samples in the buffer?
	else if (buffered > upper_thresh)
	{
		// keep track of how many consecutive times we get this condition
		consecutive_lows = 0;
		consecutive_mids = 0;
		consecutive_highs++;

		// adjust so that we generate more samples per frame to compensate
		current_adjustment = (consecutive_highs < MAX_SAMPLE_ADJUST) ? -consecutive_highs : -MAX_SAMPLE_ADJUST;

#if LOG_SOUND
		fprintf(sound_log, "  (too high - adjusting to %d)\n", current_adjustment);
#endif
	}

	// otherwise, we're in the sweet spot
	else
	{
		// keep track of how many consecutive times we get this condition
		consecutive_lows = 0;
		consecutive_mids++;
		consecutive_highs = 0;

		// after 10 or so of these, revert back to no adjustment
		if (consecutive_mids > 10 && current_adjustment != 0)
		{
			current_adjustment = 0;

#if LOG_SOUND
			fprintf(sound_log, "  (%d consecutive_mids - adjusting to %d)\n", consecutive_mids, current_adjustment);
#endif
		}
	}
}



//============================================================
//  copy_sample_data
//============================================================

static void copy_sample_data(INT16 *data, int bytes_to_copy)
{
	void *buffer1, *buffer2;
	DWORD length1, length2;
	HRESULT result;
	int cur_bytes;

	// attempt to lock the stream buffer
	result = IDirectSoundBuffer_Lock(stream_buffer, stream_buffer_in, bytes_to_copy, &buffer1, &length1, &buffer2, &length2, 0);
	if (result != DS_OK)
	{
		buffer_underflows++;
		return;
	}

	// adjust the input pointer
	stream_buffer_in = (stream_buffer_in + bytes_to_copy) % stream_buffer_size;

	// copy the first chunk
	cur_bytes = (bytes_to_copy > length1) ? length1 : bytes_to_copy;
	memcpy(buffer1, data, cur_bytes);

	// adjust for the number of bytes
	bytes_to_copy -= cur_bytes;
	data = (INT16 *)((UINT8 *)data + cur_bytes);

	// copy the second chunk
	if (bytes_to_copy != 0)
	{
		cur_bytes = (bytes_to_copy > length2) ? length2 : bytes_to_copy;
		memcpy(buffer2, data, cur_bytes);
	}

	// unlock
	result = IDirectSoundBuffer_Unlock(stream_buffer, buffer1, length1, buffer2, length2);
}



//============================================================
//  osd_update_audio_stream
//============================================================

int osd_update_audio_stream(INT16 *buffer)
{
	// if nothing to do, don't do it
	if (stream_buffer)
	{
		int original_bytes = bytes_in_stream_buffer();
		int input_bytes = samples_this_frame * stream_format.nBlockAlign;
		int final_bytes;

		// update the sample adjustment
		update_sample_adjustment(original_bytes);

		// copy data into the sound buffer
		copy_sample_data(buffer, input_bytes);

		// check for overflows
		final_bytes = bytes_in_stream_buffer();
		if (final_bytes < original_bytes)
			buffer_overflows++;
	}

	if( wavptr != NULL )
		wav_add_data_16( wavptr, buffer, samples_this_frame * 2 );

	// reset underflow/overflow tracking
	if (++total_frames == INGORE_UNDERFLOW_FRAMES)
		buffer_overflows = buffer_underflows = 0;

	// update underflow/overflow logging
#if (DISPLAY_UNDEROVERFLOW || LOG_SOUND)
{
	static int prev_overflows, prev_underflows;
	if (total_frames > INGORE_UNDERFLOW_FRAMES && (buffer_overflows != prev_overflows || buffer_underflows != prev_underflows))
	{
		prev_overflows = buffer_overflows;
		prev_underflows = buffer_underflows;
#if DISPLAY_UNDEROVERFLOW
		popmessage("overflows=%d underflows=%d", buffer_overflows, buffer_underflows);
#endif
#if LOG_SOUND
		fprintf(sound_log, "************************ overflows=%d underflows=%d\n", buffer_overflows, buffer_underflows);
#endif
	}
}
#endif

	// determine the number of samples per frame
	samples_per_frame = (double)Machine->sample_rate / (double)Machine->screen[0].refresh;

	// compute how many samples to generate next frame
	samples_left_over += samples_per_frame;
	samples_this_frame = (UINT32)samples_left_over;
	samples_left_over -= (double)samples_this_frame;

	samples_this_frame += current_adjustment;

	// return the samples to play this next frame
	return samples_this_frame;
}



//============================================================
//  osd_set_mastervolume
//============================================================

void osd_set_mastervolume(int _attenuation)
{
	// clamp the attenuation to 0-32 range
	if (_attenuation > 0)
		_attenuation = 0;
	if (_attenuation < -32)
		_attenuation = -32;
	attenuation = _attenuation;

	// set the master volume
	if (stream_buffer && is_enabled)
		IDirectSoundBuffer_SetVolume(stream_buffer, attenuation * 100);
}



//============================================================
//  osd_get_mastervolume
//============================================================

int osd_get_mastervolume(void)
{
	return attenuation;
}



//============================================================
//  osd_sound_enable
//============================================================

void osd_sound_enable(int enable_it)
{
	if (stream_buffer)
	{
		if (enable_it)
			IDirectSoundBuffer_SetVolume(stream_buffer, attenuation * 100);
		else
			IDirectSoundBuffer_SetVolume(stream_buffer, DSBVOLUME_MIN);

		is_enabled = enable_it;
	}
}



//============================================================
//  dsound_init
//============================================================

static int dsound_init(void)
{
	HRESULT result;

	// now attempt to create it
	result = DirectSoundCreate(NULL, &dsound, NULL);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error creating DirectSound: %08x\n", (UINT32)result);
		goto cant_create_dsound;
	}

	// get the capabilities
	dsound_caps.dwSize = sizeof(dsound_caps);
	result = IDirectSound_GetCaps(dsound, &dsound_caps);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error getting DirectSound capabilities: %08x\n", (UINT32)result);
		goto cant_get_caps;
	}

	// set the cooperative level
	result = IDirectSound_SetCooperativeLevel(dsound, win_window_list->hwnd, DSSCL_PRIORITY);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error setting cooperative level: %08x\n", (UINT32)result);
		goto cant_set_coop_level;
	}

	// make a format description for what we want
	stream_format.wBitsPerSample	= 16;
	stream_format.wFormatTag		= WAVE_FORMAT_PCM;
	stream_format.nChannels			= 2;
	stream_format.nSamplesPerSec	= Machine->sample_rate;
	stream_format.nBlockAlign		= stream_format.wBitsPerSample * stream_format.nChannels / 8;
	stream_format.nAvgBytesPerSec	= stream_format.nSamplesPerSec * stream_format.nBlockAlign;

	// compute the buffer sizes
	stream_buffer_size = ((UINT64)MAX_BUFFER_SIZE * (UINT64)stream_format.nSamplesPerSec) / 44100;
	stream_buffer_size = (stream_buffer_size * stream_format.nBlockAlign) / 4;
	stream_buffer_size = (stream_buffer_size * 30) / Machine->screen[0].refresh;
	stream_buffer_size = (stream_buffer_size / 1024) * 1024;

	// compute the upper/lower thresholds
	lower_thresh = audio_latency * stream_buffer_size / 5;
	upper_thresh = (audio_latency + 1) * stream_buffer_size / 5;
#if LOG_SOUND
	fprintf(sound_log, "stream_buffer_size = %d (max %d)\n", stream_buffer_size, MAX_BUFFER_SIZE);
	fprintf(sound_log, "lower_thresh = %d\n", lower_thresh);
	fprintf(sound_log, "upper_thresh = %d\n", upper_thresh);
#endif

	// create the buffers
	if (dsound_create_buffers())
		goto cant_create_buffers;

	// start playing
	is_enabled = 1;

	result = IDirectSoundBuffer_Play(stream_buffer, 0, 0, DSBPLAY_LOOPING);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error playing: %08x\n", (UINT32)result);
		goto cant_play;
	}
	return 0;

	// error handling
cant_play:
	dsound_destroy_buffers();
cant_create_buffers:
cant_set_coop_level:
cant_get_caps:
	IDirectSound_Release(dsound);
cant_create_dsound:
	dsound = NULL;
	return 0;
}



//============================================================
//  dsound_kill
//============================================================

static void dsound_kill(void)
{
	// release the object
	if (dsound)
		IDirectSound_Release(dsound);
	dsound = NULL;
}



//============================================================
//  dsound_create_buffers
//============================================================

static int dsound_create_buffers(void)
{
	HRESULT result;
	void *buffer;
	DWORD locked;

	// create a buffer desc for the primary buffer
	primary_desc.dwSize				= sizeof(primary_desc);
	primary_desc.dwFlags			= DSBCAPS_PRIMARYBUFFER |
									  DSBCAPS_GETCURRENTPOSITION2;
	primary_desc.lpwfxFormat		= NULL;

	// create the primary buffer
	result = IDirectSound_CreateSoundBuffer(dsound, &primary_desc, &primary_buffer, NULL);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error creating primary buffer: %08x\n", (UINT32)result);
		goto cant_create_primary;
	}

	// attempt to set the primary format
	result = IDirectSoundBuffer_SetFormat(primary_buffer, &stream_format);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error setting primary format: %08x\n", (UINT32)result);
		goto cant_set_primary_format;
	}

	// get the primary format
	result = IDirectSoundBuffer_GetFormat(primary_buffer, &primary_format, sizeof(primary_format), NULL);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error getting primary format: %08x\n", (UINT32)result);
		goto cant_get_primary_format;
	}
	verbose_printf("DirectSound: Primary buffer: %d Hz, %d bits, %d channels\n",
				(int)primary_format.nSamplesPerSec, (int)primary_format.wBitsPerSample, (int)primary_format.nChannels);

	// create a buffer desc for the stream buffer
	stream_desc.dwSize				= sizeof(stream_desc);
	stream_desc.dwFlags				= DSBCAPS_CTRLVOLUME |
									  DSBCAPS_GLOBALFOCUS |
									  DSBCAPS_GETCURRENTPOSITION2;
	stream_desc.dwBufferBytes 		= stream_buffer_size;
	stream_desc.lpwfxFormat			= &stream_format;

	// create the stream buffer
	result = IDirectSound_CreateSoundBuffer(dsound, &stream_desc, &stream_buffer, NULL);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error creating DirectSound buffer: %08x\n", (UINT32)result);
		goto cant_create_buffer;
	}

	// lock the buffer
	result = IDirectSoundBuffer_Lock(stream_buffer, 0, stream_buffer_size, &buffer, &locked, NULL, NULL, 0);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error locking stream buffer: %08x\n", (UINT32)result);
		goto cant_lock_buffer;
	}

	// clear the buffer and unlock it
	memset(buffer, 0, locked);
	IDirectSoundBuffer_Unlock(stream_buffer, buffer, locked, NULL, 0);
	return 0;

	// error handling
cant_lock_buffer:
	IDirectSoundBuffer_Release(stream_buffer);
cant_create_buffer:
	stream_buffer = NULL;
cant_get_primary_format:
cant_set_primary_format:
	IDirectSoundBuffer_Release(primary_buffer);
cant_create_primary:
	primary_buffer = NULL;
	return 1;
}



//============================================================
//  dsound_destroy_buffers
//============================================================

static void dsound_destroy_buffers(void)
{
	// stop any playback
	if (stream_buffer)
		IDirectSoundBuffer_Stop(stream_buffer);

	// release the buffer
	if (stream_buffer)
		IDirectSoundBuffer_Release(stream_buffer);
	stream_buffer = NULL;
}
