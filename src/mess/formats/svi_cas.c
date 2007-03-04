#include <string.h>
#include "svi_cas.h"

#define CAS_PERIOD_0		(40)
#define CAS_PERIOD_1		(18)
#define CAS_HEADER_PERIODS (1600)
#define CAS_EMPTY_SAMPLES (11025)
#define ALLOCATE_BLOCK	  (1024*8)

static const UINT8 CasHeader[17] =
{
	0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
	0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x7f
};

#define SMPLO	(INT16)0x8001
#define SMPHI	(INT16)0x7fff

static int svi_cas_to_wav (const UINT8 *casdata, int caslen, INT16 **wavdata, int *wavlen)
{
	int cas_pos, samples_size, bit, samples_pos, size, n, i;
	INT16 *samples, *nsamples;

	if (caslen < 17) return 1;
	if (memcmp (casdata, CasHeader, sizeof (CasHeader) ) ) return 1;

	cas_pos = 17;
	samples_size = ALLOCATE_BLOCK * 2;
	samples = (INT16*) malloc (samples_size);
	if (!samples)
		return 2;

	samples_pos = 0;

	while (cas_pos < caslen)
	{
		/* check memory for entire header (silence + header itself) */
		size = CAS_PERIOD_0 * (CAS_HEADER_PERIODS + 1) +
			   CAS_PERIOD_1 * (CAS_HEADER_PERIODS + 7) +
				CAS_EMPTY_SAMPLES;

		if ( (samples_pos + size) >= samples_size)
		{
			samples_size += size;
			nsamples = (INT16*) realloc (samples, samples_size * 2);
			if (!nsamples)
				{
				free (samples);
				return 2;
				}
			else samples = nsamples;
		}

		/* write CAS_EMPTY_PERIODS of silence */
		n = CAS_EMPTY_SAMPLES; while (n--) samples[samples_pos++] = SMPHI;

		/* write CAS_HEADER_PERIODS of header */
		for (i=0;i<CAS_HEADER_PERIODS;i++)
		{
			/* write a "0" */
			n = !(i % 4) ? 21 : 18;
			while (n--) samples[samples_pos++] = SMPHI;
			n = 19; while (n--) samples[samples_pos++] = SMPLO;
			/* write a "1" */
			n = 9; while (n--) samples[samples_pos++] = SMPHI;
			n = 9; while (n--) samples[samples_pos++] = SMPLO;
		}

		/* write 0x7f */
		/* write a "0" */
		n = 21; while (n--) samples[samples_pos++] = SMPHI;
		n = 19; while (n--) samples[samples_pos++] = SMPLO;

		for (i=0;i<7;i++)
		{
			/* write a "1" */
			n = 9; while (n--) samples[samples_pos++] = SMPHI;
			n = 9; while (n--) samples[samples_pos++] = SMPLO;
		}
		while (cas_pos < caslen)
		{
			/* check if we've hit a new header (or end of block) */
			if ( (cas_pos + 17) < caslen)
			{
				if (!memcmp (casdata + cas_pos, CasHeader, 17) )
				{
					cas_pos += 17;
					break; /* falls back to loop above; plays header again */
				}
			}

			/* check if we've got enough memory for the next byte */
			size = CAS_PERIOD_0 * 9;
			if ( (samples_pos + size) >= samples_size)
			{
				samples_size += ALLOCATE_BLOCK;
				nsamples = (INT16*) realloc (samples, samples_size * 2);
				if (!nsamples)
				{
					free (samples);
					return 2;
				}
				else samples = nsamples;
			}

			for (i=-1;i<8;i++)
			{
				if (i < 0) bit = 0;
				else bit = (casdata[cas_pos] & (0x80 >> i) );

				/* write this one bit */
				if (bit)
				{
					/* write a "1" */
					n = 9; while (n--) samples[samples_pos++] = SMPHI;
					n = 9; while (n--) samples[samples_pos++] = SMPLO;
				}
				else
				{
					/* write a "0" */
					n = (i < 0) ? 21 : 18;
					while (n--) samples[samples_pos++] = SMPHI;
					n = 19; while (n--) samples[samples_pos++] = SMPLO;
				}
			}

			cas_pos++;
		}
	}

	*wavdata = samples;
	*wavlen = samples_pos;

	return 0;
}



static int svi_cas_fill_wave(INT16 *buffer, int length, UINT8 *bytes)
{
	INT16 *wavdata;
	int wavlen;

	if (svi_cas_to_wav(bytes, length, &wavdata, &wavlen))
		return -1;

	memcpy(buffer, wavdata, wavlen * 2);
	free(wavdata);
	return 0;
}


static int svi_cas_chunk_sample_calc(const UINT8 *bytes, int length)
{
	INT16 *wavdata;
	int wavlen;

	if (svi_cas_to_wav(bytes, length, &wavdata, &wavlen))
		return -1;

	free(wavdata);
	return wavlen;
}



static struct CassetteLegacyWaveFiller svi_legacy_fill_wave =
{
	svi_cas_fill_wave,						/* fill_wave */
	-1,										/* chunk_size */
	0,										/* chunk_samples */
	svi_cas_chunk_sample_calc,				/* chunk_sample_calc */
	22050,									/* sample_frequency */
	-1,										/* header_samples */
	0										/* trailer_samples */
};



static casserr_t svi_cas_identify(cassette_image *cassette, struct CassetteOptions *opts)
{
	return cassette_legacy_identify(cassette, opts, &svi_legacy_fill_wave);
}



static casserr_t svi_cas_load(cassette_image *cassette)
{
	return cassette_legacy_construct(cassette, &svi_legacy_fill_wave);
}



struct CassetteFormat svi_cas_format =
{
	"cas\0",
	svi_cas_identify,
	svi_cas_load,
	NULL
};



CASSETTE_FORMATLIST_START(svi_cassette_formats)
	CASSETTE_FORMAT(svi_cas_format)
CASSETTE_FORMATLIST_END
