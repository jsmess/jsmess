/*

TZX (currently spectrum only) and spectrum TAP cassette format support by Wilbert Pol

Currently supported block types:
  0x10, 0x11, 0x12, 0x13, 0x14, 0x20

Tested block types:
  0x10

TODO:
  Add support for the remaining block types:
                case 0x15:	Direct Recording
                case 0x16:	C64 ROM Type Data Block
                case 0x17:	C64 Turbo Tape Data Block
                case 0x21:      Group Start
                case 0x22:      Group End
                case 0x23:      Jump To Block
                case 0x24:      Loop Start
                case 0x25:      Loop End
                case 0x26:      Call Sequence
                case 0x27:      Return From Sequence
                case 0x28:      Select Block
                case 0x2A:      Stop Tape if in 48K Mode
                case 0x30:      Text Description
                case 0x31:      Message Block
                case 0x32:      Archive Info
                case 0x33:      Hardware Type
                case 0x34:      Emulation Info
                case 0x35:      Custom Info Block
                case 0x40:      Snapshot Block
                case 0x5A:      Merge Block


Notes:

TZX format specification lists
8064 pulses for a header block and 3220 for a data block

but the documentaiton on worldofspectrum lists
8063 pulses for a header block and 3223 for a data block

see http://www.worldofspectrum.org/faq/reference/48kreference.htm#TapeDataStructure

We are currently using the numbers from the TZX specification...

*/

#include "mame.h"
#include "tzx_cas.h"

#define TZX_WAV_FREQUENCY	44100
#define WAVE_LOW		-0x5a9e
#define WAVE_HIGH		0x5a9e
#define WAVE_NULL		0

#define SUPPORTED_VERSION_MAJOR	0x01

#define INITIAL_MAX_BLOCK_COUNT	256
#define BLOCK_COUNT_INCREMENTS	256

static const UINT8 TZX_HEADER[8] = { 'Z','X','T','a','p','e','!',0x1a };

/*
  Global variables

  Initialized by tzx_cas_get_wave_size, used (and cleaned up) by tzx_cas_fill_wave
 */

static INT16	wave_data;
static int	block_count;
static UINT8**	blocks;

static void toggle_wave_data(void) {
	if ( wave_data == WAVE_LOW ) {
		wave_data = WAVE_HIGH;
	} else {
		wave_data = WAVE_LOW;
	}
}

static void tzx_cas_get_blocks( const UINT8 *casdata, int caslen ) {
	int pos = sizeof(TZX_HEADER) + 2;
	int max_block_count = INITIAL_MAX_BLOCK_COUNT;

	blocks = auto_malloc( max_block_count * sizeof(UINT8*) );
	block_count = 0;

	while( pos < caslen ) {
		UINT32 datasize;
		UINT8 blocktype = casdata[pos];

		if ( block_count == max_block_count ) {
			void	*old_blocks = blocks;
			int	old_max_block_count = max_block_count;

			max_block_count = max_block_count + BLOCK_COUNT_INCREMENTS;
			blocks = auto_malloc( max_block_count * sizeof(UINT8*) );
			memcpy( blocks, old_blocks, old_max_block_count * sizeof(UINT8*) );
		}

		blocks[block_count] = (UINT8*)&casdata[pos];

		pos += 1;

		switch( blocktype ) {
		case 0x10:
			pos += 2;
			datasize = casdata[pos] + ( casdata[pos+1] << 8 );
			pos += 2 + datasize;
			break;
		case 0x11:
			pos += 0x0f;
			datasize = casdata[pos] + ( casdata[pos+1] << 8 ) + ( casdata[pos+2] << 16 );
			pos += 3 + datasize;
			break;
		case 0x12:
			pos += 4;
			break;
		case 0x13:
			datasize = casdata[pos];
			pos += 1 + 2 * datasize;
			break;
		case 0x14:
			pos += 7;
			datasize = casdata[pos] + ( casdata[pos+1] << 8 ) + ( casdata[pos+2] << 16 );
			pos += 3 + datasize;
			break;
		case 0x15:
			pos += 5;
			datasize = casdata[pos] + ( casdata[pos+1] << 8 ) + ( casdata[pos+2] << 16 );
			pos += 3 + datasize;
			break;
		case 0x20: case 0x23: case 0x24:
			pos += 2;
			break;
		case 0x21: case 0x30:
			datasize = casdata[pos];
			pos += 1 + datasize;
			break;
		case 0x22: case 0x25: case 0x27:
			break;
		case 0x26:
			datasize = casdata[pos] + ( casdata[pos+1] << 8 );
			pos += 2 + 2 * datasize;
			break;
		case 0x28: case 0x32:
			datasize = casdata[pos] + ( casdata[pos+1] << 8 );
			pos += 2 + datasize;
			break;
		case 0x31:
			pos += 1;
			datasize = casdata[pos];
			pos += 1 + datasize;
			break;
		case 0x33:
			datasize = casdata[pos];
			pos += 1 + 3 * datasize;
			break;
		case 0x34:
			pos += 8;
			break;
		case 0x35:
			pos += 0x10;
			datasize = casdata[pos] + ( casdata[pos+1] << 8 ) + ( casdata[pos+2] << 16 ) + ( casdata[pos+3] << 24 );
			pos += 4 + datasize;
			break;
		case 0x40:
			pos += 1;
			datasize = casdata[pos] + ( casdata[pos+1] << 8 ) + ( casdata[pos+2] << 16 );
			pos += 3 + datasize;
			break;
		case 0x5A:
			pos += 9;
			break;
		default:
			datasize = casdata[pos] + ( casdata[pos+1] << 8 ) + ( casdata[pos+2] << 16 ) + ( casdata[pos+3] << 24 );
			pos += 4 + datasize;
			break;
		}

		block_count++;
	}
}

INLINE int millisec_to_samplecount( int millisec ) {
	return (int) ( ( millisec * ( (double)TZX_WAV_FREQUENCY / 3500000 ) ) / 1000.0 );
}

INLINE int tcycles_to_samplecount( int tcycles ) {
	return (int) ( 0.5 + ( ( (double)TZX_WAV_FREQUENCY / 3500000 ) * (double)tcycles ) );
}

static void tzx_output_wave( INT16 **buffer, int length ) {
	if ( buffer == NULL ) {
		return;
	}

	for( ; length > 0; length-- ) {
		**buffer = wave_data;
		*buffer = *buffer + 1;
	}
}

static int tzx_cas_handle_block( INT16 **buffer, const UINT8 *bytes, int pause, int data_size, int pilot, int pilot_length, int sync1, int sync2, int bit0, int bit1, int bits_in_last_byte ) {
	int pilot_samples = tcycles_to_samplecount( pilot );
	int sync1_samples = tcycles_to_samplecount( sync1 );
	int sync2_samples = tcycles_to_samplecount( sync2 );
	int bit0_samples = tcycles_to_samplecount( bit0 );
	int bit1_samples = tcycles_to_samplecount( bit1 );
	int data_index;
	int size = 0;

	logerror( "tzx_cas_block_size: pliot_length = %d, pilot_samples = %d, sync1_samples = %d, sync2_samples = %d, bit0_samples = %d, bit1_samples = %d\n", pilot_length, pilot_samples, sync1_samples, sync2_samples, bit0_samples, bit1_samples );

	/* PILOT */
	for( ; pilot_length > 0; pilot_length-- ) {
		tzx_output_wave( buffer, pilot_samples );
		size += pilot_samples;
		toggle_wave_data();
	}
	/* SYNC1 */
	if ( sync1_samples > 0 ) {
		tzx_output_wave( buffer, sync1_samples );
		size += sync1_samples;
		toggle_wave_data();
	}
	/* SYNC2 */
	if ( sync2_samples > 0 ) {
		tzx_output_wave( buffer, sync2_samples );
		size += sync2_samples;
		toggle_wave_data();
	}
	/* data */
	for( data_index = 0; data_index < data_size; data_index++ ) {
		UINT8	byte = bytes[data_index];
		int	bits_to_go = ( data_index == ( data_size - 1 ) ) ? bits_in_last_byte : 8;
		for( ; bits_to_go > 0; byte <<= 1, bits_to_go-- ) {
			int	bit_samples = ( byte & 0x80 ) ? bit1_samples : bit0_samples;
			tzx_output_wave( buffer, bit_samples );
			size += bit_samples;
			toggle_wave_data();
			tzx_output_wave( buffer, bit_samples );
			size += bit_samples;
			toggle_wave_data();
		}
	}
	/* pause */
	if ( pause > 0 ) {
		int start_pause_samples = millisec_to_samplecount( 1 );
		int rest_pause_samples = millisec_to_samplecount( pause - 1 );

		tzx_output_wave( buffer, start_pause_samples );
		size += start_pause_samples;
		wave_data = WAVE_LOW;
		tzx_output_wave( buffer, rest_pause_samples );
		size += rest_pause_samples;
	}
        return size;
}

/*
  Will go through blocks and calculate number of samples needed.
  If buffer is not NULL the sample data will also be written.
 */
static int tzx_cas_do_work( INT16 **buffer ) {
	int	current_block = 0;
	int	size = 0;

	wave_data = WAVE_LOW;

	while( current_block < block_count ) {
		int	pause_time;
		int	data_size;
		int	pilot, pilot_length, sync1, sync2;
		int	bit0, bit1, bits_in_last_byte;
		UINT8	*cur_block = blocks[current_block];
		UINT8	block_type =cur_block[0];
		logerror( "tzx_cas_fill_wave: block %d, block_type %02x\n", current_block, block_type );
		switch( block_type ) {
		case 0x10:	/* Standard Speed Data Block (.TAP block) */
			pause_time = cur_block[1] + ( cur_block[2] << 8 );
			data_size = cur_block[3] + ( cur_block[4] << 8 );
			pilot_length = ( cur_block[5] == 0x00 ) ?  8064 : 3220;
			size += tzx_cas_handle_block( buffer, &cur_block[5], pause_time, data_size, 2168, pilot_length, 667, 735, 855, 1710, 8 );
			current_block++;
			break;
		case 0x11:	/* Turbo Loading Data Block */
			pilot = cur_block[1] + ( cur_block[2] << 8 );
			sync1 = cur_block[3] + ( cur_block[4] << 8 );
			sync2 = cur_block[5] + ( cur_block[6] << 8 );
			bit0 = cur_block[7] + ( cur_block[8] << 8 );
			bit1 = cur_block[9] + ( cur_block[10] << 8 );
			pilot_length = cur_block[11] + ( cur_block[12] << 8 );
			bits_in_last_byte = cur_block[13];
			pause_time = cur_block[14] + ( cur_block[15] << 8 );
			data_size = cur_block[16] + ( cur_block[17] << 8 ) + ( cur_block[18] << 16 );
			size += tzx_cas_handle_block( buffer, &cur_block[19], pause_time, data_size, pilot, pilot_length, sync1, sync2, bit0, bit1, bits_in_last_byte );
			current_block++;
			break;
		case 0x12:	/* Pure Tone */
			pilot = cur_block[1] + ( cur_block[2] << 8 );
			pilot_length = cur_block[3] + ( cur_block[4] << 8 );
			size += tzx_cas_handle_block( buffer, cur_block, 0, 0, pilot, pilot_length, 0, 0, 0, 0, 0 );
			current_block++;
			break;
		case 0x13:	/* Sequence of Pulses of Different Lengths */
			for( data_size = 0; data_size < cur_block[1]; data_size++ ) {
				pilot = cur_block[1 + 2*data_size] + ( cur_block[2 + 2*data_size] << 8 );
				size += tzx_cas_handle_block( buffer, cur_block, 0, 0, pilot, 1, 0, 0, 0, 0, 0 );
			}
			current_block++;
			break;
		case 0x14:	/* Pure Data Block */
			bit0 = cur_block[1] + ( cur_block[2] << 8 );
			bit1 = cur_block[3] + ( cur_block[4] << 8 );
			bits_in_last_byte = cur_block[5];
			pause_time = cur_block[6] + ( cur_block[7] << 8 );
			data_size = cur_block[8] + ( cur_block[9] << 8 ) + ( cur_block[10] << 16 );
			size += tzx_cas_handle_block( buffer, &cur_block[11], pause_time, data_size, 0, 0, 0, 0, bit0, bit1, bits_in_last_byte );
			current_block++;
			break;
		case 0x15:	/* Direct Recording */
		case 0x16:	/* C64 ROM Type Data Block */
		case 0x17:	/* C64 Turbo Tape Data Block */
			logerror( "Unsupported block type (%02x) encountered\n", block_type );
			current_block++;
			break;
		case 0x20:	/* Pause (Silence) or 'Stop the Tape' Command */
			pause_time = cur_block[1] + ( cur_block[2] << 8 );
			if ( pause_time == 0 ) {
				/* pause = 0 is used to let an emulator automagically stop the tape
				   in MESS we do not do that, so we insert a 5 second pause.
				 */
				pause_time = 5000;
			}
			size += tzx_cas_handle_block( buffer, cur_block, pause_time, 0, 0, 0, 0, 0, 0, 0, 0 );
			current_block++;
			break;
		case 0x21:	/* Group Start */
		case 0x22:	/* Group End */
		case 0x23:	/* Jump To Block */
		case 0x24:	/* Loop Start */
		case 0x25:	/* Loop End */
		case 0x26:	/* Call Sequence */
		case 0x27:	/* Return From Sequence */
		case 0x28:	/* Select Block */
		case 0x2A:	/* Stop Tape if in 48K Mode */
		case 0x30:	/* Text Description */
		case 0x31:	/* Message Block */
		case 0x32:	/* Archive Info */
		case 0x33:	/* Hardware Type */
		case 0x34:	/* Emulation Info */
		case 0x35:	/* Custom Info Block */
		case 0x40:	/* Snapshot Block */
		case 0x5A:	/* Merge Block */
		default:
			logerror( "Unsupported block type (%02x) encountered\n", block_type );
			current_block++;
			break;
		}
	}
	return size;
}

static int tzx_cas_to_wav_size( const UINT8 *casdata, int caslen ) {
	int size = 0;

	/* Header size plus major and minor version number */
	if ( caslen < 10 ) {
		logerror( "tzx_cas_to_wav_size: cassette image too small\n" );
		goto cleanup;
	}

	/* Check for correct header */
	if ( memcmp( casdata, TZX_HEADER, sizeof(TZX_HEADER) ) ) {
		logerror( "tzx_cas_to_wav_size: cassette image has incompatible header\n" );
		goto cleanup;
	}

	/* Check major version number in header */
	if ( casdata[0x08] > SUPPORTED_VERSION_MAJOR ) {
		logerror( "tzx_cas_to_wav_size: unsupported version\n" );
		goto cleanup;
	}

	tzx_cas_get_blocks( casdata, caslen );

	logerror( "tzx_cas_to_wav_size: %d blocks found\n", block_count );

	if ( block_count == 0 ) {
		logerror( "tzx_cas_to_wav_size: no blocks found!\n" );
		goto cleanup;
	}

	size = tzx_cas_do_work( NULL );

	return size;

cleanup:
	return -1;
}

static int tzx_cas_fill_wave( INT16 *buffer, int length, UINT8 *bytes ) {
	INT16	*p = buffer;
	int	size = 0;
	size = tzx_cas_do_work( &p );
	return size;
}

static int tap_cas_to_wav_size( const UINT8 *casdata, int caslen ) {
	int size = 0;
	const UINT8 *p = casdata;

	while( p < casdata + caslen ) {
		int data_size = p[0] + ( p[1] << 8 );
		int pilot_length = ( p[2] == 0x00 ) ? 8064 : 3220;	/* TZX specification */
//		int pilot_length = ( p[2] == 0x00 ) ? 8063 : 3223;	/* worldofspectrum */
		logerror( "tap_cas_to_wav_size: Handling TAP block containing 0x%X bytes", data_size );
		p += 2;
		size += tzx_cas_handle_block( NULL, p, 1000, data_size, 2168, pilot_length, 667, 735, 855, 1710, 8 );
		logerror( ", total size is now: %d\n", size );
		p += data_size;
	}
	return size;
}

static int tap_cas_fill_wave( INT16 *buffer, int length, UINT8 *bytes ) {
	INT16 *p = buffer;
	int size = 0;

	while( size < length ) {
		int data_size = bytes[0] + ( bytes[1] << 8 );
		int pilot_length = ( bytes[2] == 0x00 ) ? 8064 : 3220;	/* TZX specification */
//		int pilot_length = ( bytes[2] == 0x00 ) ? 8063 : 3223;	/* worldofspectrum */
		logerror( "tap_cas_fill_wave: Handling TAP block containing 0x%X bytes\n", data_size );
		bytes += 2;
		size += tzx_cas_handle_block( &p, bytes, 1000, data_size, 2168, pilot_length, 667, 735, 855, 1710, 8 );
		bytes += data_size;
	}
	return size;
}

static struct CassetteLegacyWaveFiller tzx_legacy_fill_wave = {
	tzx_cas_fill_wave,			/* fill_wave */
	-1,					/* chunk_size */
	0,					/* chunk_samples */
	tzx_cas_to_wav_size,			/* chunk_sample_calc */
	TZX_WAV_FREQUENCY,			/* sample_frequency */
	0,					/* header_samples */
	0					/* trailer_samples */
};

static struct CassetteLegacyWaveFiller tap_legacy_fill_wave = {
	tap_cas_fill_wave,			/* fill_wave */
	-1,					/* chunk_size */
	0,					/* chunk_samples */
	tap_cas_to_wav_size,			/* chunk_sample_calc */
	TZX_WAV_FREQUENCY,			/* sample_frequency */
	0,					/* header_samples */
	0					/* trailer_samples */
};

static casserr_t tzx_cassette_identify( cassette_image *cassette, struct CassetteOptions *opts ) {
	return cassette_legacy_identify( cassette, opts, &tzx_legacy_fill_wave );
}

static casserr_t tap_cassette_identify( cassette_image *cassette, struct CassetteOptions *opts ) {
	return cassette_legacy_identify( cassette, opts, &tap_legacy_fill_wave );
}

static casserr_t tzx_cassette_load( cassette_image *cassette ) {
	return cassette_legacy_construct( cassette, &tzx_legacy_fill_wave );
}

static casserr_t tap_cassette_load( cassette_image *cassette ) {
	return cassette_legacy_construct( cassette, &tap_legacy_fill_wave );
}

struct CassetteFormat tzx_cassette_format = {
	"tzx\0",
	tzx_cassette_identify,
	tzx_cassette_load,
	NULL
};

struct CassetteFormat tap_cassette_format = {
	"tap\0",
	tap_cassette_identify,
	tap_cassette_load,
	NULL
};

CASSETTE_FORMATLIST_START(tzx_cassette_formats)
	CASSETTE_FORMAT(tzx_cassette_format)
	CASSETTE_FORMAT(tap_cassette_format)
CASSETTE_FORMATLIST_END

