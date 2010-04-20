/*********************************************************************

    formats/atom_tap.c

    Cassette code for Acorn Atom tap files

*********************************************************************/

#include "emu.h"
#include "formats/atom_tap.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    CassetteLegacyWaveFiller atom_legacy_fill_wave
-------------------------------------------------*/

static const struct CassetteLegacyWaveFiller atom_legacy_fill_wave = 
{
	NULL,					/* fill_wave */
	-1,						/* chunk_size */
	0,						/* chunk_samples */
	NULL,					/* chunk_sample_calc */
	22050,					/* sample_frequency */
	0,						/* header_samples */
	0						/* trailer_samples */
};

/*-------------------------------------------------
    atom_cassette_identify - identify cassette
-------------------------------------------------*/

static casserr_t atom_cassette_identify( cassette_image *cassette, struct CassetteOptions *opts )
{
	return cassette_legacy_identify( cassette, opts, &atom_legacy_fill_wave );
}

/*-------------------------------------------------
    atom_cassette_load - load cassette
-------------------------------------------------*/

static casserr_t atom_cassette_load( cassette_image *cassette )
{
	return cassette_legacy_construct( cassette, &atom_legacy_fill_wave );
}

/*-------------------------------------------------
    CassetteFormat atom_tap_cassette_format
-------------------------------------------------*/

static const struct CassetteFormat atom_tap_cassette_format = 
{
	"tap",
	atom_cassette_identify,
	atom_cassette_load,
	NULL
};

/*-------------------------------------------------
    CASSETTE_FORMATLIST( atom_cassette_formats )
-------------------------------------------------*/

CASSETTE_FORMATLIST_START( atom_cassette_formats )
	CASSETTE_FORMAT(atom_tap_cassette_format)
CASSETTE_FORMATLIST_END
