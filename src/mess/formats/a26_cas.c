/*

Atari 2600 SuperCharger support

*/

#include "mame.h"
#include "driver.h"
#include "a26_cas.h"

#define A26_CAS_SIZE			8448
#define A26_WAV_FREQUENCY		44100
#define A26_WAV_FREQUENCY_PAL	43697.37
#define WAVE_LOW				0xF600
#define WAVE_HIGH				0x0500
#define WAVE_NULL				0
#define BIT_ZERO_LENGTH			3
#define BIT_ONE_LENGTH			5
#define CLEARING_TONES			432
#define CLEARING_TONE_SAMPLES	51
#define ZEROS_ONES				2755

const INT16 clearing_wave[CLEARING_TONE_SAMPLES] = {
	0x8000, 0x8F00, 0x9E00, 0xAD00, 0xBB00, 0xC800, 0xD400, 0xDF00, 0xE800, 0xF000,
	0xF600, 0xFA00, 0xFC00, 0xFD00, 0xFB00, 0xF800, 0xF300, 0xEC00, 0xE400, 0xDA00,
	0xCE00, 0xC200, 0xB400, 0xA600, 0x9700, 0x8700, 0x7800, 0x6800, 0x5900, 0x4B00,
	0x3D00, 0x3100, 0x2500, 0x1B00, 0x1300, 0x0C00, 0x0700, 0x0400, 0x0200, 0x0300,
	0x0500, 0x0900, 0x0F00, 0x1700, 0x2000, 0x2B00, 0x3700, 0x4400, 0x5200, 0x6100,
	0x7000
};

static int a26_cas_output_wave( INT16 **buffer, INT16 wave_data, int length ) {
	int i;

	if ( buffer ) {
		for ( i = 0 ; i < length; i++ ) {
			**buffer = wave_data;
			*buffer = *buffer + 1;
		}
	}
	return length;
}

static int a26_cas_output_bit( INT16 **buffer, int bit ) {
	int size = 0;

	size += a26_cas_output_wave( buffer, WAVE_LOW, bit ? BIT_ONE_LENGTH : BIT_ZERO_LENGTH );
	size += a26_cas_output_wave( buffer, WAVE_HIGH, bit ? BIT_ONE_LENGTH : BIT_ZERO_LENGTH );

	return size;
}

static int a26_cas_output_byte( INT16 **buffer, UINT8 byte ) {
	int		size = 0;
	int		i;

	for( i = 0; i < 8; i++, byte <<= 1 ) {
		size += a26_cas_output_bit( buffer, ( ( byte & 0x80 ) ? 1 : 0 ) );
	}
	return size;
}

static int a26_cas_clearing_tone( INT16 **buffer ) {
	int		size = 0;
	int 	i,j;

	for ( i = 0; i < CLEARING_TONES; i++ ) {
		for ( j = 0; j < CLEARING_TONE_SAMPLES; j++ ) {
			size += a26_cas_output_wave( buffer, clearing_wave[j], 1 );
		}
	}
	return size;
}

static int a26_cas_zeros_ones( INT16 **buffer ) {
	int		size = 0;
	int		i;

	for ( i = 0; i < ZEROS_ONES; i++ ) {
		size += a26_cas_output_bit( buffer, 0 );
		size += a26_cas_output_bit( buffer, 1 );
	}
	size += a26_cas_output_bit( buffer, 0 );
	size += a26_cas_output_bit( buffer, 0 );
	return size;
}

static int a26_cas_output_contents( INT16 **buffer, const UINT8 *bytes ) {
	int		size = 0;
	int		i, pages, j;

	/* There are 8 header bytes */
	for( i = 0; i < 8; i++ ) {
		size += a26_cas_output_byte( buffer, bytes[ 0x2000 + i ] );
	}

	pages = bytes[0x2003];

	/* Output each page prefixed with a small page header */
	for ( i = 0; i < pages; i++ ) {
		size += a26_cas_output_byte( buffer, bytes[ 0x2010 + i ] );
		size += a26_cas_output_byte( buffer, bytes[ 0x2040 + i ] );
		for ( j = 0; j < 256; j++ ) {
			size += a26_cas_output_byte( buffer, bytes[ i * 256 + j ] );
		}
	}

	return size;
}

static int a26_cas_do_work( INT16 **buffer, const UINT8 *bytes ) {
	int		size = 0;

	/* Output clearing tone */
	size += a26_cas_clearing_tone( buffer );

	/* Output header tone, alternating 1s and 0s for about a second ending with two 0s */
	size += a26_cas_zeros_ones( buffer );

	/* Output the actual contents of the tape */
	size += a26_cas_output_contents( buffer, bytes );

	/* Output footer tone, alternating 1s and 0s for about a second ending with two 0s */
	size += a26_cas_zeros_ones( buffer );

	return size;
}

static int a26_cas_fill_wave( INT16 *buffer, int length, UINT8 *bytes ) {
	INT16	*p = buffer;

	return a26_cas_do_work( &p, (const UINT8 *)bytes );
}

static int a26_cas_to_wav_size( const UINT8 *casdata, int caslen ) {
	return a26_cas_do_work( NULL, casdata );
}

static struct CassetteLegacyWaveFiller a26_legacy_fill_wave = {
	a26_cas_fill_wave,
	-1,
	0,
	a26_cas_to_wav_size,
	A26_WAV_FREQUENCY,
	0,
	0
};

static casserr_t a26_cassette_identify( cassette_image *cassette, struct CassetteOptions *opts ) {
	UINT64 size;

	size = cassette_image_size( cassette );
	if ( size == A26_CAS_SIZE ) {
		if ( !strcmp( Machine->gamedrv->name, "a2600p" ) ) {
			a26_legacy_fill_wave.sample_frequency = A26_WAV_FREQUENCY_PAL;
		}
		return cassette_legacy_identify( cassette, opts, &a26_legacy_fill_wave );
	}
	return CASSETTE_ERROR_INVALIDIMAGE;
}

static casserr_t a26_cassette_load( cassette_image *cassette ) {
	return cassette_legacy_construct( cassette, &a26_legacy_fill_wave );
}

struct CassetteFormat a26_cassette_format = {
	"a26",
	a26_cassette_identify,
	a26_cassette_load,
	NULL
};

CASSETTE_FORMATLIST_START(a26_cassette_formats)
	CASSETTE_FORMAT(a26_cassette_format)
CASSETTE_FORMATLIST_END

