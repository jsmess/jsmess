/*
	
	Tape support for RK format
		
*/
#include "driver.h"
#include "formats/rk_cas.h"


#define RK_WAV_FREQUENCY	44100
#define WAVE_HIGH		32767
#define WAVE_LOW		-32768

#define RK_HEADER_LEN	256
#define RKU_SIZE 20
#define RK8_SIZE 60

static int  	data_size;

static INT16 *rk_emit_level(INT16 *p, int count, int level)
{
	int i;

	for (i=0; i<count; i++)
	{
		*(p++) = level;
	}
	return p;
}

static INT16* rk_output_bit(INT16 *p, UINT8 b,int bitsize)
{
	if (b)
	{
		p = rk_emit_level (p, bitsize, WAVE_HIGH);
		p = rk_emit_level (p, bitsize, WAVE_LOW);								
	}	
	else
	{
		p = rk_emit_level (p, bitsize, WAVE_LOW);	
		p = rk_emit_level (p, bitsize, WAVE_HIGH);				
			
	}
    return p;
}

static INT16* rk_output_byte(INT16 *p, UINT8 byte,int bitsize)
{
	int i;
	for (i=7; i>=0; i--)
		p = rk_output_bit(p,(byte>>i) & 0x01, bitsize);

	return p;
}


static int rku_cas_to_wav_size( const UINT8 *casdata, int caslen ) {	
	data_size = caslen;
	return  (RK_HEADER_LEN  * 8 * 2 +  8*2 + caslen * 8 * 2) * RKU_SIZE;	
}

static int rk8_cas_to_wav_size( const UINT8 *casdata, int caslen ) {	
	data_size = caslen;
	return  (RK_HEADER_LEN  * 8 * 2 +  8*2 + caslen * 8 * 2) * RK8_SIZE;	
}

static int rku_cas_fill_wave( INT16 *buffer, int length, UINT8 *bytes ) {	
	int i;
	INT16 * p = buffer;
	
	for (i=0; i<RK_HEADER_LEN; i++) {
		p = rk_output_byte (p, 0x00, RKU_SIZE );	
	}

	p = rk_output_byte (p, 0xE6, RKU_SIZE);	
	
	for (i=0; i<data_size; i++)
		p = rk_output_byte (p, bytes[i], RKU_SIZE);

	return p - buffer;
}

static int rk8_cas_fill_wave( INT16 *buffer, int length, UINT8 *bytes ) {	
	int i;
	INT16 * p = buffer;
	
	for (i=0; i<RK_HEADER_LEN; i++) {
		p = rk_output_byte (p, 0x00, RK8_SIZE );	
	}

	p = rk_output_byte (p, 0xE6, RK8_SIZE);	
	
	for (i=0; i<data_size; i++)
		p = rk_output_byte (p, bytes[i], RK8_SIZE);

	return p - buffer;
}



static const struct CassetteLegacyWaveFiller rku_legacy_fill_wave = {
	rku_cas_fill_wave,			/* fill_wave */
	-1,					/* chunk_size */
	0,					/* chunk_samples */
	rku_cas_to_wav_size,			/* chunk_sample_calc */
	RK_WAV_FREQUENCY,			/* sample_frequency */
	0,					/* header_samples */
	0					/* trailer_samples */
};



static casserr_t rku_cassette_identify( cassette_image *cassette, struct CassetteOptions *opts ) {
	return cassette_legacy_identify( cassette, opts, &rku_legacy_fill_wave );
}



static casserr_t rku_cassette_load( cassette_image *cassette ) {
	return cassette_legacy_construct( cassette, &rku_legacy_fill_wave );
}

static const struct CassetteLegacyWaveFiller rk8_legacy_fill_wave = {
	rk8_cas_fill_wave,			/* fill_wave */
	-1,					/* chunk_size */
	0,					/* chunk_samples */
	rk8_cas_to_wav_size,			/* chunk_sample_calc */
	RK_WAV_FREQUENCY,			/* sample_frequency */
	0,					/* header_samples */
	0					/* trailer_samples */
};


static casserr_t rk8_cassette_identify( cassette_image *cassette, struct CassetteOptions *opts ) {
	return cassette_legacy_identify( cassette, opts, &rk8_legacy_fill_wave );
}



static casserr_t rk8_cassette_load( cassette_image *cassette ) {
	return cassette_legacy_construct( cassette, &rk8_legacy_fill_wave );
}



static const struct CassetteFormat rku_cassette_format = {
	"rku",
	rku_cassette_identify,
	rku_cassette_load,
	NULL
};

static const struct CassetteFormat rk8_cassette_format = {
	"rk8",
	rk8_cassette_identify,
	rk8_cassette_load,
	NULL
};


CASSETTE_FORMATLIST_START(rku_cassette_formats)
	CASSETTE_FORMAT(rku_cassette_format)
CASSETTE_FORMATLIST_END

CASSETTE_FORMATLIST_START(rk8_cassette_formats)
	CASSETTE_FORMAT(rk8_cassette_format)
CASSETTE_FORMATLIST_END

