//============================================================
//
//  sound.c - SDL implementation of MAME sound routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

// standard sdl header
#include <SDL/SDL.h>
#include <unistd.h>
#include <math.h>

#ifdef SDLMAME_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// MAME headers
#include "driver.h"
#include "osdepend.h"
#include "window.h"
#include "video.h"
#include "osdsdl.h"

//============================================================
//	DEBUGGING
//============================================================

#define LOG_SOUND		0

//============================================================
//	PARAMETERS
//============================================================

// number of samples per SDL callback
#define SDL_XFER_SAMPLES		512

// maximum audio latency
#define MAX_AUDIO_LATENCY		10

//============================================================
//	LOCAL VARIABLES
//============================================================

static int	 				attenuation = 0;

static int				initialized_audio;
static int				buf_locked;

static INT8				*stream_buffer;
static volatile INT32	  		stream_playpos;

static UINT32				stream_buffer_size;
static UINT32				stream_buffer_in;
static volatile UINT32	 		stream_write_cursor;
static UINT32				nBlockAlign;

// buffer over/underflow counts
static int				buffer_underflows;
static int				buffer_overflows;

// debugging
#if LOG_SOUND
#define sound_log stderr
#endif

// sound enable
static int snd_enabled;

//============================================================
//	PROTOTYPES
//============================================================

static int			sdl_init(void);
static void			sdl_kill(void);
static int			sdl_create_buffers(void);
static void			sdl_destroy_buffers(void);
static void			sdl_cleanup_audio(running_machine *machine);


//============================================================
//	osd_start_audio_stream
//============================================================
void sdl_init_audio(running_machine *machine)
{
#if LOG_SOUND
	sound_log = fopen("sound.log", "w");
#endif

	// skip if sound disabled
	if (Machine->sample_rate != 0)
	{
		// attempt to initialize SDL
		if (sdl_init())
			return;

		add_exit_callback(machine, sdl_cleanup_audio);
		// set the startup volume
		osd_set_mastervolume(attenuation);
	}
	return;
}



//============================================================
//	osd_stop_audio_stream
//============================================================

static void sdl_cleanup_audio(running_machine *machine)
{
	// if nothing to do, don't do it
	if (Machine->sample_rate == 0)
		return;
	
	// kill the buffers and dsound
	sdl_kill();
	sdl_destroy_buffers();
	
	// print out over/underflow stats
	if (buffer_overflows || buffer_underflows)
		mame_printf_verbose("Sound buffer: overflows=%d underflows=%d\n", buffer_overflows, buffer_underflows);

#if LOG_SOUND
	if (sound_log)
		fprintf(sound_log, "Sound buffer: overflows=%d underflows=%d\n", buffer_overflows, buffer_underflows);
	fclose(sound_log);
#endif
}

//============================================================
//	lock_buffer
//============================================================
static int lock_buffer(long offset, long size, void **buffer1, long *length1, void **buffer2, long *length2)
{
	volatile long pstart, pend, lstart, lend;

	if (!buf_locked)
	{
		if (video_get_throttle())
		{
			pstart = stream_playpos;
			pend = (pstart + SDL_XFER_SAMPLES);
			lstart = offset;
			lend = lstart+size;
			while (((pstart >= lstart) && (pstart <= lend)) ||
			       ((pend >= lstart) && (pend <= lend)))
			{
/*				#ifdef SDLMAME_WIN32
				Sleep(0);
				#else
				sleep(0);
				#endif*/

				pstart = stream_playpos;
				pend = pstart + SDL_XFER_SAMPLES;
			}
		}

		SDL_LockAudio();
		buf_locked++;
	}

	// init lengths
	*length1 = *length2 = 0;

	if ((offset + size) > stream_buffer_size)
	{
		// 2-piece case
		*length1 = stream_buffer_size - offset;
		*buffer1 = &stream_buffer[offset];
		*length2 = size - *length1;
		*buffer2 = stream_buffer;
	}
	else
	{
		// normal 1-piece case
		*length1 = size;
		*buffer1 = &stream_buffer[offset];
	}

	#if LOG_SOUND
//	fprintf(sound_log, "locking %ld bytes at offset %ld (total %d, playpos %d): len1 %ld len2 %ld\n",
//		size, offset, stream_buffer_size, stream_playpos, *length1, *length2);
	#endif

	return 0;
}

//============================================================
//	unlock_buffer
//============================================================
static void unlock_buffer(void)
{
	buf_locked--;
	if (!buf_locked)
		SDL_UnlockAudio();

	#if LOG_SOUND
//	fprintf(sound_log, "unlocking\n");
	#endif
}

//============================================================
//	Apply attenuation
//============================================================

static void att_memcpy(void *dest, INT16 *data, int bytes_to_copy)
{	
	int level= (int) (pow(10.0, (float) attenuation / 20.0) * 128.0);
	INT16 *d = dest;
	int count = bytes_to_copy/2;
	while (count>0)
	{	
		*d++ = *data++ * level/128;
		count--;
	}
}

//============================================================
//	copy_sample_data
//============================================================

static void copy_sample_data(INT16 *data, int bytes_to_copy)
{
	void *buffer1, *buffer2 = (void *)NULL;
	long length1, length2;
	int cur_bytes;

	// attempt to lock the stream buffer
	if (lock_buffer(stream_buffer_in, bytes_to_copy, &buffer1, &length1, &buffer2, &length2) < 0)
	{
		buffer_underflows++;
		return;
	}

	// adjust the input pointer
	stream_buffer_in = (stream_buffer_in + bytes_to_copy) % stream_buffer_size;

	// copy the first chunk
	cur_bytes = (bytes_to_copy > length1) ? length1 : bytes_to_copy;
	att_memcpy(buffer1, data, cur_bytes);

	// adjust for the number of bytes
	bytes_to_copy -= cur_bytes;
	data = (INT16 *)((UINT8 *)data + cur_bytes);

	// copy the second chunk
	if (bytes_to_copy != 0)
	{
		cur_bytes = (bytes_to_copy > length2) ? length2 : bytes_to_copy;
		att_memcpy(buffer2, data, cur_bytes);
	}

	// unlock
	unlock_buffer();
}


//============================================================
//	osd_update_audio_stream
//============================================================

void osd_update_audio_stream(INT16 *buffer, int samples_this_frame)
{
	// if nothing to do, don't do it
	if (Machine->sample_rate != 0 && stream_buffer)
	{
		int bytes_this_frame = samples_this_frame * sizeof(INT16) * 2;
		int play_position, write_position, stream_in, orig_write;

		play_position = stream_playpos;

		write_position = orig_write = stream_playpos + ((Machine->sample_rate / 50) * sizeof(INT16) * 2);

		// normalize the write position so it is always after the play position
		if (write_position < play_position)
			write_position += stream_buffer_size;

		// normalize the stream in position so it is always after the write position
		stream_in = stream_buffer_in;
		if (stream_in < write_position)
			stream_in += stream_buffer_size;

		// now we should have, in order:
		//    <------pp---wp---si--------------->

		// if we're between play and write positions, then bump forward, but only in full chunks
		while (stream_in < write_position)
		{
//			printf("Underflow: PP=%d  WP=%d(%d)  SI=%d(%d)  BTF=%d\n", (int)play_position, (int)write_position, (int)orig_write, (int)stream_in, (int)stream_buffer_in, (int)bytes_this_frame);
			buffer_underflows++;
			stream_in += samples_this_frame;
		}

		// if we're going to overlap the play position, just skip this chunk
		if (stream_in + bytes_this_frame > play_position + stream_buffer_size)
		{
//			printf("Overflow: PP=%d  WP=%d(%d)  SI=%d(%d)  BTF=%d\n", (int)play_position, (int)write_position, (int)orig_write, (int)stream_in, (int)stream_buffer_in, (int)bytes_this_frame);
			buffer_overflows++;
			return;
		}

		// now we know where to copy; let's do it
		stream_buffer_in = stream_in % stream_buffer_size;
		copy_sample_data(buffer, bytes_this_frame);
	}
}



//============================================================
//	osd_set_mastervolume
//============================================================

void osd_set_mastervolume(int _attenuation)
{
	// clamp the attenuation to 0-32 range
	if (_attenuation > 0)
		_attenuation = 0;
	if (_attenuation < -32)
		_attenuation = -32;

	attenuation = _attenuation;
}



//============================================================
//	osd_get_mastervolume
//============================================================

int osd_get_mastervolume(void)
{
	return attenuation;
}



//============================================================
//	osd_sound_enable
//============================================================

void osd_sound_enable(int enable_it)
{
	snd_enabled = enable_it;
}

//============================================================
//	sdl_callback
//============================================================
void sdl_callback(void *userdata, Uint8 *stream, int len)
{
	int len1, len2;

	if ((stream_playpos+len) > stream_buffer_size)
	{
		len1 = stream_buffer_size - stream_playpos;
		len2 = len - len1;
	}
	else
	{
		len1 = len;
		len2 = 0;
	}

	if (snd_enabled)
	{
		memcpy(stream, stream_buffer + stream_playpos, len1);
		memset(stream_buffer + stream_playpos, 0, len1); // no longer needed
		if (len2)
		{
			memcpy(stream+len1, stream_buffer, len2);
			memset(stream_buffer, 0, len2); // no longer needed
		}

	}
	else
	{
		memset(stream, 0, len);
	}

	// move the play cursor
	stream_playpos += len;
	stream_playpos %= stream_buffer_size;

	// always keep the write cursor just ahead of the play position
	stream_write_cursor = stream_playpos + len;
	stream_write_cursor %= stream_buffer_size;

	#if LOG_SOUND
//	fprintf(sound_log, "callback: xfer len1 %d len2 %d, playpos %d, wcursor %d\n",
//		len1, len2, stream_playpos, stream_write_cursor);
	#endif
}


//============================================================
//	sdl_init
//============================================================
static int sdl_init(void)
{
	int			n_channels = 2;
	int			audio_latency;
	SDL_AudioSpec 	aspec;

	initialized_audio = 0;

	// set up the audio specs
	aspec.freq = Machine->sample_rate;
	aspec.format = AUDIO_S16SYS;	// keep endian independant 
	aspec.channels = n_channels;
	aspec.samples = SDL_XFER_SAMPLES;
	aspec.callback = sdl_callback;
	aspec.userdata = 0;

	nBlockAlign		= 16 * aspec.channels / 8;

	if (SDL_OpenAudio(&aspec, NULL) < 0)
		goto cant_start_audio;

	initialized_audio = 1;
	snd_enabled = 1;

	audio_latency = options_get_int(mame_options(), SDLOPTION_AUDIO_LATENCY);

	// pin audio latency
	if (audio_latency > MAX_AUDIO_LATENCY)
	{
		audio_latency = MAX_AUDIO_LATENCY;
	}
	else if (audio_latency < 1)
	{
		audio_latency = 1;
	}

	// compute the buffer sizes
	stream_buffer_size = Machine->sample_rate * 2 * sizeof(INT16) * audio_latency / MAX_AUDIO_LATENCY;
	stream_buffer_size = (stream_buffer_size / 1024) * 1024;
	if (stream_buffer_size < 1024)
		stream_buffer_size = 1024;

#if LOG_SOUND
	fprintf(sound_log, "stream_buffer_size = %d\n", stream_buffer_size);
#endif

	// create the buffers
	if (sdl_create_buffers())
		goto cant_create_buffers;

	// start playing
	SDL_PauseAudio(0);
	return 0;

	// error handling
cant_create_buffers:
cant_start_audio:
	return 0;
}



//============================================================
//	sdl_kill
//============================================================

static void sdl_kill(void)
{
	if (initialized_audio)
		SDL_CloseAudio();
}



//============================================================
//	dsound_create_buffers
//============================================================

static int sdl_create_buffers(void)
{
//	printf("sdl_create_buffers: creating stream buffer of %x bytes\n", stream_buffer_size);
	stream_buffer = (INT8 *)malloc(stream_buffer_size);
	memset(stream_buffer, 0, stream_buffer_size);
	stream_playpos = 0;
	stream_write_cursor = SDL_XFER_SAMPLES*2;
	buf_locked = 0;
	return 0;
}



//============================================================
//	sdl_destroy_buffers
//============================================================

static void sdl_destroy_buffers(void)
{
	// release the buffer
	if (stream_buffer)
		free(stream_buffer);
	stream_buffer = NULL;
}

