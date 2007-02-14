/***************************************************************************
    polepos.c
    Sound handler
****************************************************************************/
#include "driver.h"
#include "streams.h"
#include "sound/filter.h"
#include "rescap.h"
#include "sound/custom.h"

static int sample_msb = 0;
static int sample_lsb = 0;
static int sample_enable = 0;

static sound_stream *stream;

#define POLEPOS_R166		1000.0
#define POLEPOS_R167		2200.0
#define POLEPOS_R168		4700.0
/* resistor values when shorted by 4066 running at 5V */
#define POLEPOS_R166_SHUNT	1.0/(1.0/POLEPOS_R166 + 1.0/250)
#define POLEPOS_R167_SHUNT	1.0/(1.0/POLEPOS_R166 + 1.0/250)
#define POLEPOS_R168_SHUNT	1.0/(1.0/POLEPOS_R166 + 1.0/250)

static double volume_table[8] =
{
	(POLEPOS_R168_SHUNT + POLEPOS_R167_SHUNT + POLEPOS_R166_SHUNT + 2200) / 10000,
	(POLEPOS_R168_SHUNT + POLEPOS_R167_SHUNT + POLEPOS_R166       + 2200) / 10000,
	(POLEPOS_R168_SHUNT + POLEPOS_R167       + POLEPOS_R166_SHUNT + 2200) / 10000,
	(POLEPOS_R168_SHUNT + POLEPOS_R167       + POLEPOS_R166       + 2200) / 10000,
	(POLEPOS_R168       + POLEPOS_R167_SHUNT + POLEPOS_R166_SHUNT + 2200) / 10000,
	(POLEPOS_R168       + POLEPOS_R167_SHUNT + POLEPOS_R166       + 2200) / 10000,
	(POLEPOS_R168       + POLEPOS_R167       + POLEPOS_R166_SHUNT + 2200) / 10000,
	(POLEPOS_R168       + POLEPOS_R167       + POLEPOS_R166       + 2200) / 10000
};

static struct filter2_context filter_engine[3];

static double r_filt_out[3] = {RES_K(4.7), RES_K(7.5), RES_K(10)};
static double r_filt_total = 1.0 / (1.0/RES_K(4.7) + 1.0/RES_K(7.5) + 1.0/RES_K(10));

/************************************/
/* Stream updater                   */
/************************************/
static void engine_sound_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	static UINT32 current_position;
	UINT32 step, clock, slot;
	UINT8 *base;
	double volume, i_total;
	stream_sample_t *buffer = outputs[0];
	int loop;

	/* if we're not enabled, just fill with 0 */
	if (!sample_enable)
	{
		memset(buffer, 0, length * sizeof(*buffer));
		return;
	}

	/* determine the effective clock rate */
	clock = (Machine->drv->cpu[0].cpu_clock / 16) * ((sample_msb + 1) * 64 + sample_lsb + 1) / (64*64);
	step = (clock << 12) / Machine->sample_rate;

	/* determine the volume */
	slot = (sample_msb >> 3) & 7;
	volume = volume_table[slot];
	base = &memory_region(REGION_SOUND2)[slot * 0x800];

	/* fill in the sample */
	while (length--)
	{
		filter_engine[0].x0 = (3.4 / 255 * base[(current_position >> 12) & 0x7ff] - 2) * volume;
		filter_engine[1].x0 = filter_engine[0].x0;
		filter_engine[2].x0 = filter_engine[0].x0;

		i_total = 0;
		for (loop = 0; loop < 3; loop++)
		{
			filter2_step(&filter_engine[loop]);
			/* The op-amp powered @ 5V will clip to 0V & 3.5V.
             * Adjusted to vRef of 2V, we will clip as follows: */
			if (filter_engine[loop].y0 > 1.5) filter_engine[loop].y0 = 1.5;
			if (filter_engine[loop].y0 < -2)  filter_engine[loop].y0 = -2;

			i_total += filter_engine[loop].y0 / r_filt_out[loop];
		}
		i_total *= r_filt_total * 32000/2;	/* now contains voltage adjusted by final gain */

		*buffer++ = (int)i_total;
		current_position += step;
	}
}

/************************************/
/* Sound handler start              */
/************************************/
void *polepos_sh_start(int clock, const struct CustomSound_interface *config)
{
	stream = stream_create(0, 1, Machine->sample_rate, NULL, engine_sound_update);
	sample_msb = sample_lsb = 0;
	sample_enable = 0;

	/* setup the filters */
	filter_opamp_m_bandpass_setup(RES_K(220), RES_K(33), RES_K(390), CAP_U(.01),  CAP_U(.01),
									&filter_engine[0]);
	filter_opamp_m_bandpass_setup(RES_K(150), RES_K(22), RES_K(330), CAP_U(.0047),  CAP_U(.0047),
									&filter_engine[1]);
	/* Filter 3 is a little different.  Because of the input capacitor, it is
     * a high pass filter. */
	filter2_setup(FILTER_HIGHPASS, 950, Q_TO_DAMP(.707), 1,
									&filter_engine[2]);

	return auto_malloc(1);
}

/************************************/
/* Sound handler reset              */
/************************************/
void polepos_sh_reset(void *token)
{
	int loop;
	for (loop = 0; loop < 3; loop++) filter2_reset(&filter_engine[loop]);
}

/************************************/
/* Write LSB of engine sound        */
/************************************/
WRITE8_HANDLER( polepos_engine_sound_lsb_w )
{
	/* Update stream first so all samples at old frequency are updated. */
	stream_update(stream);
	sample_lsb = data & 62;
    sample_enable = data & 1;
}

/************************************/
/* Write MSB of engine sound        */
/************************************/
WRITE8_HANDLER( polepos_engine_sound_msb_w )
{
	stream_update(stream);
	sample_msb = data & 63;
}
