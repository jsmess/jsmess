#include "oric_tap.h"
#include "driver.h"

/* this code based heavily on tap2wav by Fabrice Frances */
#define ORIC_SYNC_BYTE	0x016

/* frequency of wave */
/* tapes use 1200hz and 2400hz samples */
#define ORIC_WAV_FREQUENCY	4800

/* 13 bits define a byte on the cassette */
/* 1 start bit, 8 data bits, 1 parity bit and 3 stop bits */
#define ORIC_BYTE_TO_BITS_ON_CASSETTE 13

#define ORIC_WAVESAMPLES_HEADER  3000
#define ORIC_WAVESAMPLES_TRAILER 1000

enum
{
	ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE,
	ORIC_CASSETTE_GOT_SYNC_BYTE,
	ORIC_CASSETTE_READ_HEADER,
	ORIC_CASSETTE_READ_FILENAME,
	ORIC_CASSETTE_WRITE_DATA
};

#define WAVEENTRY_LOW  -32768
#define WAVEENTRY_HIGH  32767
#define WAVEENTRY_NULL  0

/* to write a bit to the tape, the rom routines output either 4 periods at 1200 Hz for a 0 or 8 periods at 2400 Hz for a 1  */
/* 4800 is twice 2400hz */

/* 8 periods at 2400hz */
/* hi,lo, hi,lo, hi,lo, hi,lo */

static INT16 wave_state = WAVEENTRY_HIGH;
static INT16 xor_wave_state = WAVEENTRY_HIGH^WAVEENTRY_LOW;

static INT16 *oric_emit_level(INT16 *p, int count)
{	
	int i;

	for (i=0; i<count; i++)
	{
		*(p++) = wave_state;
	}
	wave_state = wave_state^xor_wave_state;
	return p;
}

/* 4 periods at 1200hz */ 
static INT16* oric_output_bit(INT16 *p, UINT8 b)
{
	p = oric_emit_level(p,1);

	if (b)
	{
		p = oric_emit_level(p,1);
	}
	else
	{
		p = oric_emit_level(p,2);
	}

    return p;
}

static int oric_get_bit_size_in_samples(UINT8 b)
{
	int count;

	count = 1;

	if (b)
	{
		count++;
	}
	else
	{
		count+=2;
	}

	return count;
}


/*	each byte on cassette is stored as: 
	
	start bit		0 * 1
	data bits		8 * x (x is 0 or 1, and depends on data-bit value)
	parity bit		1 * x (x is 0 or 1, and depends on the parity of the data bits)
	stop bits		1 * 3

	if data has even parity, parity bit will be 1.
	if data has odd parity, parity bit will be 0.
*/

/*
	512 * data byte 0x016		-> leader
	1   * data byte 0x024		-> sync byte
	9   * data byte 			-> header
	delay (of last pulse written)
	x   * data byte			-> length


	header structure:
	3	* ?			->		???
	1	* ?			->		???
	1	* x			->		end address high byte
	1	* x			->		end address low byte
	1	* x			->		start address high byte
	1	* x			->		start address low byte
	1	* ?			->		???
*/

static int oric_calculate_byte_size_in_samples(UINT8 byte)
{
	int count;
	int i;
	UINT8 parity;
	UINT8 data;

	count = 0;


	/* start bit */
	count+=oric_get_bit_size_in_samples(0);

	/* set initial parity */	
	parity = 1;
	
	/* data bits, written bit 0, bit 1...bit 7 */
	data = byte;
	for (i=0; i<8; i++)
	{
		UINT8 data_bit;

		data_bit = data & 0x01;

		parity = parity + data_bit;

		count+=oric_get_bit_size_in_samples(data_bit);
		data = data>>1;
	}

	/* parity */
	count+=oric_get_bit_size_in_samples((parity & 0x01));

	/* stop bits */
	count+=oric_get_bit_size_in_samples(1);
	count+=oric_get_bit_size_in_samples(1);
	count+=oric_get_bit_size_in_samples(1);
	count+=oric_get_bit_size_in_samples(1);

	return count;
}


static INT16 *oric_output_byte(INT16 *p, UINT8 byte)
{
	int i;
	UINT8 parity;
	UINT8 data;

	/* start bit */
	p = oric_output_bit(p, 0);
	
	/* set initial parity */	
	parity = 1;
	
	/* data bits, written bit 0, bit 1...bit 7 */
	data = byte;
	for (i=0; i<8; i++)
	{
		UINT8 data_bit;

		data_bit = data & 0x01;

		parity = parity + data_bit;

		p = oric_output_bit(p, data_bit);
		data = data>>1;
	}

	/* parity */
	p = oric_output_bit(p, parity & 0x01);

	/* stop bits */
	p = oric_output_bit(p, 1);
	p = oric_output_bit(p, 1);
	p = oric_output_bit(p, 1);
	p = oric_output_bit(p, 1);

	return p;
}

static INT16 *oric_fill_pause(INT16 *p, int sample_count)
{
	int i;

	for (i=0; i<sample_count; i++)
	{
		*(p++) = WAVEENTRY_NULL;
	}

	return p;
}

static int oric_seconds_to_samples(float seconds)
{
	return (int)((float)seconds*(float)ORIC_WAV_FREQUENCY);
}

#define ORIC_LEADER_LENGTH 512

static int oric_cassette_state;
static int oric_data_count;
static int oric_data_length;
static int oric_tap_size;

/* length is length of .tap file! */
static int oric_cassette_calculate_size_in_samples(const UINT8 *bytes, int length)
{
	unsigned char header[9];
	int count;

	const UINT8 *data_ptr;
	int i;
	UINT8 data;

	oric_tap_size = length;

	oric_cassette_state = ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE;
	count = 0;
	data_ptr = bytes;

	while (data_ptr<(bytes+length))
	{
		data = data_ptr[0];
		data_ptr++;

		switch (oric_cassette_state)
		{
			case ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE:
			{
				if (data==ORIC_SYNC_BYTE)
				{
					logerror("found sync byte!\n");
					/* found first sync byte */
					oric_cassette_state = ORIC_CASSETTE_GOT_SYNC_BYTE;
				}
			}
			break;

			case ORIC_CASSETTE_GOT_SYNC_BYTE:
			{
				if (data!=ORIC_SYNC_BYTE)
				{
					/* 0.25 second pause */
					count += oric_seconds_to_samples(0.25);

					logerror("found end of sync bytes!\n");
				
					/* oric writes approx 512 bytes */
					/* found end of sync bytes */
					for (i=0; i<ORIC_LEADER_LENGTH; i++)
					{
						count+=oric_calculate_byte_size_in_samples(0x016);
					}

					if (data==0x024)
					{
						//logerror("reading header!\n");
						count+=oric_calculate_byte_size_in_samples(0x024);
		
						oric_cassette_state = ORIC_CASSETTE_READ_HEADER;
						oric_data_count = 0;
						oric_data_length = 9;
					}
				}
			}
			break;

			case ORIC_CASSETTE_READ_HEADER:
			{
				header[oric_data_count] = data;
				count+=oric_calculate_byte_size_in_samples(data);
		
				oric_data_count++;

				if (oric_data_count==oric_data_length)
				{
					//logerror("finished reading header!\n");
					oric_cassette_state = ORIC_CASSETTE_READ_FILENAME;
				}
			}
			break;

			case ORIC_CASSETTE_READ_FILENAME:
			{
				count+=oric_calculate_byte_size_in_samples(data);

				/* got end of filename? */
				if (data==0)
				{
					UINT16 end, start;
					logerror("got end of filename\n");

					/* 100 1 bits to seperate header from data */
                                        for (i=0; i<100; i++)
                                        {
                                            count+=oric_get_bit_size_in_samples(1);
                                        }

					oric_cassette_state = ORIC_CASSETTE_WRITE_DATA;
					oric_data_count = 0;
					
					end = (((header[4] & 0x0ff)<<8) | (header[5] & 0x0ff));
					start = (((header[6] & 0x0ff)<<8) | (header[7] & 0x0ff));
//#ifdef ORIC_WAV_DEBUG		
					logerror("start (from header): %02x\n",start);
					logerror("end (from header): %02x\n",end);
//#endif
                                        oric_data_length =end - start + 1;
				}
			}
			break;

			case ORIC_CASSETTE_WRITE_DATA:
			{
				count+=oric_calculate_byte_size_in_samples(data);
				oric_data_count++;

				if (oric_data_count==oric_data_length)
				{
					logerror("finished writing data!\n");
					oric_cassette_state = ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE;
				}
			}
			break;

		}
	}

	return count;
}

/* length is length of sample buffer to fill! */
static int oric_cassette_fill_wave(INT16 *buffer, int length, UINT8 *bytes)
{
	unsigned char header[9];
	UINT8 *data_ptr;
	INT16 *p;
	int i;
	UINT8 data;

	p = buffer;


	/* header and trailer act as pauses */
	/* the trailer is required so that the via sees the last bit of the last 
		byte */
    if (bytes == CODE_HEADER) {
        for (i = 0; i < ORIC_WAVESAMPLES_HEADER; i++)
            *(p++) = WAVEENTRY_NULL;
    }
    else if (bytes == CODE_TRAILER) {
        for (i = 0; i < ORIC_WAVESAMPLES_TRAILER; i++)
            *(p++) = WAVEENTRY_NULL;
    }
	else
{
	/* the length is the number of samples left in the buffer and NOT the number of bytes for the input file */
	length = length - ORIC_WAVESAMPLES_TRAILER;

	oric_cassette_state = ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE;
	data_ptr = bytes;

	while ((data_ptr<(bytes+oric_tap_size)) && (p<(buffer+length)) )
	{
		data = data_ptr[0];
		data_ptr++;

		switch (oric_cassette_state)
		{
			case ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE:
			{
				wave_state = WAVEENTRY_HIGH;

				if (data==ORIC_SYNC_BYTE)
				{
					logerror("found sync byte!\n");
					/* found first sync byte */
					oric_cassette_state = ORIC_CASSETTE_GOT_SYNC_BYTE;
				}
			}
			break;

			case ORIC_CASSETTE_GOT_SYNC_BYTE:
			{
				if (data!=ORIC_SYNC_BYTE)
				{
					/* 0.25 second pause */
					p = oric_fill_pause(p, oric_seconds_to_samples(0.25));

					logerror("found end of sync bytes!\n");
					/* found end of sync bytes */
					for (i=0; i<ORIC_LEADER_LENGTH; i++)
					{
						p = oric_output_byte(p,0x016);
					}

					if (data==0x024)
					{
						//logerror("reading header!\n");
						p = oric_output_byte(p,data);
						oric_cassette_state = ORIC_CASSETTE_READ_HEADER;
						oric_data_count = 0;
						oric_data_length = 9;
					}
				}
			}
			break;

			case ORIC_CASSETTE_READ_HEADER:
			{
				header[oric_data_count] = data;
				p = oric_output_byte(p, data);
				oric_data_count++;

				if (oric_data_count==oric_data_length)
				{
					//logerror("finished reading header!\n");
					oric_cassette_state = ORIC_CASSETTE_READ_FILENAME;
				}
			}
			break;

			case ORIC_CASSETTE_READ_FILENAME:
			{
				p = oric_output_byte(p, data);

				/* got end of filename? */
				if (data==0)
				{
					UINT16 end, start;
					logerror("got end of filename\n");

					/* oric includes a small delay, but I don't see
					it being 1 bits */
					for (i=0; i<100; i++)
					{
                                            p = oric_output_bit(p,1);
                                        }
					
					oric_cassette_state = ORIC_CASSETTE_WRITE_DATA;
					oric_data_count = 0;
					
					end = (((header[4] & 0x0ff)<<8) | (header[5] & 0x0ff));
					start = (((header[6] & 0x0ff)<<8) | (header[7] & 0x0ff));
//#ifdef ORIC_WAV_DEBUG		
					logerror("start (from header): %02x\n",start);
					logerror("end (from header): %02x\n",end);
//#endif
                                        oric_data_length = end - start + 1;
				}
			}
			break;

			case ORIC_CASSETTE_WRITE_DATA:
			{
				p = oric_output_byte(p, data);
				oric_data_count++;

				if (oric_data_count==oric_data_length)
				{
					logerror("finished writing data!\n");
					oric_cassette_state = ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE;
				}
			}
			break;

		}
	}
}
	return p - buffer;
}



static struct CassetteLegacyWaveFiller oric_legacy_fill_wave =
{
	oric_cassette_fill_wave,					/* fill_wave */
	-1,											/* chunk_size */
	0,											/* chunk_samples */
	oric_cassette_calculate_size_in_samples,	/* chunk_sample_calc */
	ORIC_WAV_FREQUENCY,							/* sample_frequency */
	ORIC_WAVESAMPLES_HEADER,					/* header_samples */
	ORIC_WAVESAMPLES_TRAILER					/* trailer_samples */
};



static casserr_t oric_tap_identify(cassette_image *cassette, struct CassetteOptions *opts)
{
	return cassette_legacy_identify(cassette, opts, &oric_legacy_fill_wave);
}



static casserr_t oric_tap_load(cassette_image *cassette)
{
	return cassette_legacy_construct(cassette, &oric_legacy_fill_wave);
}



struct CassetteFormat oric_tap_format =
{
	"tap\0",
	oric_tap_identify,
	oric_tap_load,
	NULL
};



CASSETTE_FORMATLIST_START(oric_cassette_formats)
	CASSETTE_FORMAT(oric_tap_format)
CASSETTE_FORMATLIST_END
