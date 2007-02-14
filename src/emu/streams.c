/***************************************************************************

    streams.c

    Handle general purpose audio streams

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "driver.h"
#include "streams.h"
#include <math.h>

#define VERBOSE			(0)

#if VERBOSE
#define VPRINTF(x) mame_printf_debug x
#else
#define VPRINTF(x)
#endif


#define RESAMPLE_BUFFER_SAMPLES			(128*1024)
#define RESAMPLE_TOSS_SAMPLES_THRESH	(RESAMPLE_BUFFER_SAMPLES/2)
#define RESAMPLE_KEEP_SAMPLES			256

#define OUTPUT_BUFFER_SAMPLES			(128*1024)
#define OUTPUT_TOSS_SAMPLES_THRESH		(OUTPUT_BUFFER_SAMPLES/2)
#define OUTPUT_KEEP_SAMPLES				256

#define FRAC_BITS						14
#define FRAC_ONE						(1 << FRAC_BITS)
#define FRAC_MASK						(FRAC_ONE - 1)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct stream_input
{
	sound_stream *stream;						/* pointer to the input stream */
	struct stream_output *source;				/* pointer to the sound_output for this source */
	stream_sample_t *resample;					/* buffer for resampling to the stream's sample rate */
	UINT32			source_frac;				/* fractional position within the source buffer */
	UINT32			step_frac;					/* source stepping rate */
	UINT32			resample_in_pos;			/* resample index where next sample will be written */
	UINT32			resample_out_pos;			/* resample index where next sample will be read */
	INT16			gain;						/* gain to apply to this input */
};


struct stream_output
{
	stream_sample_t *buffer;					/* output buffer */
	UINT32			cur_in_pos;					/* sample index where next sample will be written */
	UINT32			cur_out_pos;				/* sample index where next sample will be read */
	int				dependents;					/* number of dependents */
	INT16			gain;						/* gain to apply to the output */
};


struct _sound_stream
{
	/* linking information */
	struct _sound_stream *next;					/* next stream in the chain */
	void *			tag;						/* tag (used for identification) */
	int				index;						/* index for save states */

	/* general information */
	INT32			sample_rate;				/* sample rate of this stream */
	INT32			new_sample_rate;			/* newly-set sample rate for the stream */
	UINT32			samples_per_frame_frac;		/* fractional samples per frame */

	/* input information */
	int				inputs;						/* number of inputs */
	struct stream_input *input;					/* list of streams we directly depend upon */
	stream_sample_t **input_array;				/* array of inputs for passing to the callback */

	/* output information */
	int				outputs;					/* number of outputs */
	struct stream_output *output;				/* list of streams which directly depend upon us */
	stream_sample_t **output_array;				/* array of outputs for passing to the callback */

	/* callback information */
	void *			param;
	stream_callback callback;					/* callback function */
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static sound_stream *stream_head;
static void *stream_current_tag;
static int stream_index;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void stream_generate_samples(sound_stream *stream, int samples);
static void resample_input_stream(struct stream_input *input, int samples);



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*************************************
 *
 *  Start up the streams engine
 *
 *************************************/

int streams_init(void)
{
	/* reset globals */
	stream_head = NULL;
	stream_current_tag = NULL;
	stream_index = 0;

	return 0;
}



/*************************************
 *
 *  Set the current stream tag
 *
 *************************************/

void streams_set_tag(void *streamtag)
{
	stream_current_tag = streamtag;
}



/*************************************
 *
 *  Update all
 *
 *************************************/

void streams_frame_update(void)
{
	sound_stream *stream;

	VPRINTF(("streams_frame_update\n"));

	/* iterate over all the streams */
	for (stream = stream_head; stream != NULL; stream = stream->next)
	{
		int inputnum, outputnum;

		/* iterate over all the inputs */
		for (inputnum = 0; inputnum < stream->inputs; inputnum++)
		{
			struct stream_input *input = &stream->input[inputnum];

			/* if this input's resample buffer is over halfway full, throw away old samples */
			VPRINTF(("  Stream %p, input %d: resample_out_pos = %d\n", stream, inputnum, input->resample_out_pos));
			if (input->resample_out_pos >= RESAMPLE_TOSS_SAMPLES_THRESH)
			{
				int samples_to_remove = input->resample_out_pos - RESAMPLE_KEEP_SAMPLES;

				VPRINTF(("  Tossing resample buffer samples (%p, input %d) - %d samples\n", stream, inputnum, samples_to_remove));

				/* keep some of the samples and toss the rest */
				if (RESAMPLE_KEEP_SAMPLES)
					memmove(&input->resample[0], &input->resample[samples_to_remove], RESAMPLE_KEEP_SAMPLES * sizeof(*input->resample));
				input->resample_in_pos -= samples_to_remove;
				input->resample_out_pos -= samples_to_remove;
			}
		}

		/* iterate over all the outputs */
		for (outputnum = 0; outputnum < stream->outputs; outputnum++)
		{
			struct stream_output *output = &stream->output[outputnum];
			int samples_to_remove = 0;

			/* if this output is over halfway full, throw away old samples */
			VPRINTF(("  Stream %p, output %d: cur_in_pos = %d, cur_out_pos = %d\n", stream, outputnum, output->cur_in_pos, output->cur_out_pos));
			if (output->cur_in_pos >= OUTPUT_TOSS_SAMPLES_THRESH)
			{
				samples_to_remove = output->cur_in_pos - OUTPUT_KEEP_SAMPLES;

				VPRINTF(("  Tossing output buffer samples (%p, output %d) - %d samples\n", stream, outputnum, samples_to_remove));

				/* keep some of the samples and toss the rest */
				if (OUTPUT_KEEP_SAMPLES)
					memmove(&output->buffer[0], &output->buffer[samples_to_remove], OUTPUT_KEEP_SAMPLES * sizeof(*output->buffer));
				output->cur_in_pos -= samples_to_remove;
				if (output->cur_out_pos > samples_to_remove)
					output->cur_out_pos -= samples_to_remove;
				else
					output->cur_out_pos = 0;
			}

			/* now scan for any dependent inputs and fix up their source pointer */
			if (output->dependents > 0)
			{
				UINT32 min_source_frac = output->cur_in_pos << FRAC_BITS;
				sound_stream *str;

				for (str = stream_head; str != NULL; str = str->next)
					for (inputnum = 0; inputnum < str->inputs; inputnum++)
						if (str->input[inputnum].source == output)
						{
							if (samples_to_remove)
								VPRINTF(("  Adjusting source_frac of stream %p, input %d from %08x to %08x\n", str, inputnum, str->input[inputnum].source_frac, str->input[inputnum].source_frac - (samples_to_remove << FRAC_BITS)));

							/* first remove any samples */
							str->input[inputnum].source_frac -= samples_to_remove << FRAC_BITS;
							if ((INT32)str->input[inputnum].source_frac < 0)
								str->input[inputnum].source_frac = 0;

							/* if this input is further behind, note it */
							if (str->input[inputnum].source_frac < min_source_frac)
								min_source_frac = str->input[inputnum].source_frac;

							/* if the stream has changed sample rate, update things */
							if (stream->new_sample_rate != 0)
								str->input[inputnum].step_frac = ((UINT64)stream->new_sample_rate << FRAC_BITS) / str->sample_rate;
						}

				/* update the in position to the minimum source frac */
				output->cur_out_pos = min_source_frac >> FRAC_BITS;
			}
		}

		/* update the sample rate */
		if (stream->new_sample_rate)
		{
			stream->sample_rate = stream->new_sample_rate;
			stream->samples_per_frame_frac = (UINT32)((double)stream->sample_rate * (double)(1 << FRAC_BITS) / Machine->screen[0].refresh);
			stream->new_sample_rate = 0;
		}
	}
}



/*************************************
 *
 *  Create a new stream
 *
 *************************************/

static void stream_postload(void *param)
{
	sound_stream *stream = param;
	stream->new_sample_rate = stream->sample_rate;
}


sound_stream *stream_create(int inputs, int outputs, int sample_rate, void *param, stream_callback callback)
{
	int inputnum, outputnum;
	sound_stream *stream;
	char statetag[30];

	/* allocate memory */
	stream = auto_malloc(sizeof(*stream));
	memset(stream, 0, sizeof(*stream));

	VPRINTF(("stream_create(%d, %d, %d) => %p\n", inputs, outputs, sample_rate, stream));

	/* allocate space for the inputs */
	if (inputs > 0)
	{
		stream->input = auto_malloc(inputs * sizeof(*stream->input));
		memset(stream->input, 0, inputs * sizeof(*stream->input));
		stream->input_array = auto_malloc(inputs * sizeof(*stream->input_array));
		memset(stream->input_array, 0, inputs * sizeof(*stream->input_array));
	}

	/* allocate space for the outputs */
	if (outputs > 0)
	{
		stream->output = auto_malloc(outputs * sizeof(*stream->output));
		memset(stream->output, 0, outputs * sizeof(*stream->output));
		stream->output_array = auto_malloc(outputs * sizeof(*stream->output_array));
		memset(stream->output_array, 0, outputs * sizeof(*stream->output_array));
	}

	/* fill in the data */
	stream->tag         = stream_current_tag;
	stream->index		= stream_index++;
	stream->sample_rate = sample_rate;
	stream->samples_per_frame_frac = (UINT32)((double)sample_rate * (double)(1 << FRAC_BITS) / Machine->screen[0].refresh);
	stream->inputs      = inputs;
	stream->outputs     = outputs;
	stream->param       = param;
	stream->callback    = callback;

	/* create a unique tag for saving */
	sprintf(statetag, "stream.%d", stream->index);
	state_save_register_item(statetag, 0, stream->sample_rate);
	state_save_register_func_postload_ptr(stream_postload, stream);

	/* allocate resample buffers */
	for (inputnum = 0; inputnum < inputs; inputnum++)
	{
		stream->input[inputnum].resample = auto_malloc(RESAMPLE_BUFFER_SAMPLES * sizeof(*stream->input[inputnum].resample));
		stream->input[inputnum].gain = 0x100;
		state_save_register_item(statetag, inputnum, stream->input[inputnum].gain);
	}

	/* allocate output buffers */
	for (outputnum = 0; outputnum < outputs; outputnum++)
	{
		stream->output[outputnum].buffer = auto_malloc(OUTPUT_BUFFER_SAMPLES * sizeof(*stream->output[outputnum].buffer));
		stream->output[outputnum].gain = 0x100;
		state_save_register_item(statetag, outputnum, stream->output[outputnum].gain);
	}

	/* hook us in */
	if (!stream_head)
		stream_head = stream;
	else
	{
		sound_stream *temp;
		for (temp = stream_head; temp->next; temp = temp->next) ;
		temp->next = stream;
	}

	return stream;
}



/*************************************
 *
 *  Configure a stream's input
 *
 *************************************/

void stream_set_input(sound_stream *stream, int index, sound_stream *input_stream, int output_index, float gain)
{
	struct stream_input *input;

	VPRINTF(("stream_set_input(%p, %d, %p, %d, %f)\n", stream, index, input_stream, output_index, gain));

	/* make sure it's a valid input */
	if (index >= stream->inputs)
		fatalerror("Fatal error: stream_set_input attempted to configure non-existant input %d (%d max)", index, stream->inputs);

	/* make sure it's a valid output */
	if (input_stream && output_index >= input_stream->outputs)
		fatalerror("Fatal error: stream_set_input attempted to use a non-existant output %d (%d max)", output_index, input_stream->outputs);

	/* if this input is already wired, update the dependent info */
	input = &stream->input[index];
	if (input->source)
		input->source->dependents--;

	/* wire it up */
	input->stream = input_stream;
	input->source = input_stream ? &input_stream->output[output_index] : NULL;
	input->source_frac = 0;
	input->step_frac = input_stream ? (((UINT64)input_stream->sample_rate << FRAC_BITS) / stream->sample_rate) : 0;
	input->resample_in_pos = 0;
	input->resample_out_pos = 0;
	input->gain = (int)(0x100 * gain);
	VPRINTF(("  step_frac = %08X\n", input->step_frac));

	/* update the dependent info */
	if (input->source)
		input->source->dependents++;
}



/*************************************
 *
 *  Update a stream to the current
 *  time
 *
 *************************************/

void stream_update(sound_stream *stream)
{
	/* get current position based on the current time */
	UINT32 target_frac = (stream->output[0].cur_out_pos << FRAC_BITS) + sound_scalebufferpos(stream->samples_per_frame_frac);
	UINT32 target_sample = ((target_frac + FRAC_ONE - 1) >> FRAC_BITS) + 1;

	VPRINTF(("stream_update(%p)\n", stream, min_interval));
	VPRINTF(("  cur_in_pos = %d, cur_out_pos = %d, target_sample = %d\n", stream->output[0].cur_in_pos, stream->output[0].cur_out_pos, target_sample));

	/* compute how many samples we need to get to where we want to be */
	stream_generate_samples(stream, target_sample - stream->output[0].cur_in_pos);
}



/*************************************
 *
 *  Find a stream using a tag and
 *  index
 *
 *************************************/

sound_stream *stream_find_by_tag(void *streamtag, int streamindex)
{
	sound_stream *stream;

	/* scan the list looking for the nth stream that matches the tag */
	for (stream = stream_head; stream; stream = stream->next)
		if (stream->tag == streamtag && streamindex-- == 0)
			return stream;
	return NULL;
}



/*************************************
 *
 *  Return the number of inputs for
 *  a given stream
 *
 *************************************/

int stream_get_inputs(sound_stream *stream)
{
	return stream->inputs;
}



/*************************************
 *
 *  Return the number of outputs for
 *  a given stream
 *
 *************************************/

int stream_get_outputs(sound_stream *stream)
{
	return stream->outputs;
}



/*************************************
 *
 *  Set the input gain on a given
 *  stream
 *
 *************************************/

void stream_set_input_gain(sound_stream *stream, int input, float gain)
{
	stream_update(stream);
	stream->input[input].gain = (int)(0x100 * gain);
}



/*************************************
 *
 *  Set the output gain on a given
 *  stream
 *
 *************************************/

void stream_set_output_gain(sound_stream *stream, int output, float gain)
{
	stream_update(stream);
	stream->output[output].gain = (int)(0x100 * gain);
}



/*************************************
 *
 *  Set the sample rate on a given
 *  stream
 *
 *************************************/

void stream_set_sample_rate(sound_stream *stream, int sample_rate)
{
	/* we update this at the end of the current frame */
	stream->new_sample_rate = sample_rate;
}



/*************************************
 *
 *  Return a pointer to the output
 *  buffer for a stream with the
 *  requested number of samples
 *
 *************************************/

stream_sample_t *stream_consume_output(sound_stream *stream, int outputnum, int samples)
{
	struct stream_output *output = &stream->output[outputnum];
	UINT32 target_sample = output->cur_out_pos + samples;

	VPRINTF(("stream_consume_output(%p, %d, %d)\n", stream, outputnum, samples));

	/* if we don't have enough samples, fix it */
	stream_generate_samples(stream, target_sample - output->cur_in_pos);

	/* return a pointer to the buffer, and adjust the base */
	output->cur_out_pos += samples;
	return output->buffer + output->cur_out_pos - samples;
}



/*************************************
 *
 *  Generate the requested number of
 *  samples for a stream, making sure
 *  all inputs have the appropriate
 *  number of samples generated
 *
 *************************************/

static void stream_generate_samples(sound_stream *stream, int samples)
{
	int inputnum, outputnum;

	/* if we're already there, skip it */
	if (samples <= 0)
		return;

	VPRINTF(("stream_generate_samples(%p, %d)\n", stream, samples));

	/* loop over all inputs and make sure we have enough data for them */
	for (inputnum = 0; inputnum < stream->inputs; inputnum++)
	{
		struct stream_input *input = &stream->input[inputnum];
		UINT32 target_resample_out_pos;
		INT32 resample_samples_needed;

		VPRINTF(("  input %d\n", inputnum));

		/* determine the final output position where we need to be to satisfy this request */
		target_resample_out_pos = input->resample_out_pos + samples;

		/* if we don't have enough samples in the resample buffer, we need some more */
		resample_samples_needed = target_resample_out_pos - input->resample_in_pos;
		VPRINTF(("    resample_samples_needed = %d\n", resample_samples_needed));
		if (resample_samples_needed > 0)
		{
			INT32 source_samples_needed;
			UINT32 target_source_frac;

			/* determine where we will be after we process all the needed samples */
			target_source_frac = input->source_frac + resample_samples_needed * input->step_frac;

			/* if we're undersampling, we need an extra sample for linear interpolation */
			if (input->step_frac < FRAC_ONE)
				target_source_frac += FRAC_ONE;

			/* based on that, we know how many additional source samples we need to generate */
			source_samples_needed = ((target_source_frac + FRAC_ONE - 1) >> FRAC_BITS) - input->source->cur_in_pos;
			VPRINTF(("    source_samples_needed = %d\n", source_samples_needed));

			/* if we need some samples, generate them recursively */
			if (source_samples_needed > 0)
				stream_generate_samples(input->stream, source_samples_needed);

			/* now resample */
			VPRINTF(("    resample_input_stream(%d)\n", resample_samples_needed));
			resample_input_stream(input, resample_samples_needed);
			VPRINTF(("    resample_input_stream done\n"));
		}

		/* set the input pointer */
		stream->input_array[inputnum] = &input->resample[input->resample_out_pos];
		input->resample_out_pos += samples;
	}

	/* loop over all outputs and compute the output array */
	for (outputnum = 0; outputnum < stream->outputs; outputnum++)
	{
		struct stream_output *output = &stream->output[outputnum];
		stream->output_array[outputnum] = output->buffer + output->cur_in_pos;
		output->cur_in_pos += samples;
	}

	/* okay, all the inputs are up-to-date ... call the callback */
	VPRINTF(("  callback(%p, %d)\n", stream, samples));

	/* aye, a bug is lurking here; sometimes we have extreme sample counts */
	if (samples > stream->sample_rate)
	{
		logerror("samples > sample rate?! %d %d\n", samples, stream->sample_rate);
		samples = stream->sample_rate;
	}
	(*stream->callback)(stream->param, stream->input_array, stream->output_array, samples);
	VPRINTF(("  callback done\n"));
}



/*************************************
 *
 *  Resample an input stream into the
 *  correct sample rate with gain
 *  adjustment
 *
 *************************************/

static void resample_input_stream(struct stream_input *input, int samples)
{
	stream_sample_t *dest = input->resample + input->resample_in_pos;
	stream_sample_t *source = input->source->buffer;
	INT16 gain = (input->gain * input->source->gain) >> 8;
	UINT32 pos = input->source_frac;
	UINT32 step = input->step_frac;
	INT32 sample;

	VPRINTF(("    resample_input_stream -- step = %d\n", step));

	/* perfectly matching */
	if (step == FRAC_ONE)
	{
		while (samples--)
		{
			/* compute the sample */
			sample = source[pos >> FRAC_BITS];
			*dest++ = (sample * gain) >> 8;
			pos += FRAC_ONE;
		}
	}

	/* input is undersampled: use linear interpolation */
	else if (step < FRAC_ONE)
	{
		while (samples--)
		{
			/* compute the sample */
			sample  = source[(pos >> FRAC_BITS) + 0] * (FRAC_ONE - (pos & FRAC_MASK));
			sample += source[(pos >> FRAC_BITS) + 1] * (pos & FRAC_MASK);
			sample >>= FRAC_BITS;
			*dest++ = (sample * gain) >> 8;
			pos += step;
		}
	}

	/* input is oversampled: sum the energy */
	else
	{
		/* use 8 bits to allow some extra headroom */
		int smallstep = step >> (FRAC_BITS - 8);

		while (samples--)
		{
			int tpos = pos >> FRAC_BITS;
			int remainder = smallstep;
			int scale;

			/* compute the sample */
			scale = (FRAC_ONE - (pos & FRAC_MASK)) >> (FRAC_BITS - 8);
			sample = source[tpos++] * scale;
			remainder -= scale;
			while (remainder > 0x100)
			{
				sample += source[tpos++] * 0x100;
				remainder -= 0x100;
			}
			sample += source[tpos] * remainder;
			sample /= smallstep;

			*dest++ = (sample * gain) >> 8;
			pos += step;
		}
	}

	/* update the input parameters */
	input->resample_in_pos = dest - input->resample;
	input->source_frac = pos;
}
