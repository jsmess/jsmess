/*

 Commodore GCR format 

Original    Encoded
4 bits      5 bits

0000    ->  01010 = 0x0a
0001    ->  01011 = 0x0b
0010    ->  10010 = 0x12
0011    ->  10011 = 0x13
0100    ->  01110 = 0x0e
0101    ->  01111 = 0x0f
0110    ->  10110 = 0x16
0111    ->  10111 = 0x17
1000    ->  01001 = 0x09
1001    ->  11001 = 0x19
1010    ->  11010 = 0x1a
1011    ->  11011 = 0x1b
1100    ->  01101 = 0x0d
1101    ->  11101 = 0x1d
1110    ->  11110 = 0x1e
1111    ->  10101 = 0x15

We use the encoded values in bytes because we use them to encode
groups of 4 bytes into groups of 5 bytes, below.

*/

/*

    TODO:

	- cleanup
    - disk errors
	- D71
	- D80/D82
	- variable gaps

*/

#include "emu.h"
#include "formats/flopimg.h"
#include "formats/d64_dsk.h"
#include "devices/flopdrv.h"

#define LOG 1

#define HEADER_LENGTH	0x2ac		 /* standard length for 84 half tracks */
#define HALF_TRACKS		84
#define OFFSET_INVALID	0xbadbad

#define D64_MAX_TRACKS         35 * 2
#define D64_40T_MAX_TRACKS     40 * 2
#define D71_MAX_TRACKS         70
#define D81_MAX_TRACKS 80
#define D80_MAX_TRACKS 77
/* Still have to investigate if these are read as 154 or as 77 in the drive... */
/* For now we assume it's 154, but I'm not sure */
#define D82_MAX_TRACKS 154

static const int d64_sectors_per_track[] =
{
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17,
	17, 17, 17, 17, 17,		/* only for 40 tracks d64 */
	17, 17					/* in principle the drive could read 42 tracks */
};

#if 0
static const int d71_sectors_per_track[] =
{
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17,
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17
};

/* Each track in a d81 file has 40 sectors, so no need of a sectors_per_track table */

static const int d80_sectors_per_track[] =
{
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23
};

static const int d82_sectors_per_track[] =
{
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23
};


static const int max_tracks_per_format[] =
{
	D64_MAX_TRACKS,
	D64_MAX_TRACKS,
	D64_40T_MAX_TRACKS,
	D64_40T_MAX_TRACKS,
	D64_MAX_TRACKS,
	D71_MAX_TRACKS,
	D71_MAX_TRACKS,
	D81_MAX_TRACKS,
	D80_MAX_TRACKS,
	D82_MAX_TRACKS,
	0
};

#endif

struct d64dsk_tag
{
	int heads;
	int tracks;
	int track_offset[HALF_TRACKS];			/* offset within data for each half track */
	UINT32 speed_zone_offset[HALF_TRACKS];	/* speed zone for each half track */
	int error[HALF_TRACKS];					/* DOS error code for each half track */

	int id1, id2;
};

INLINE float get_track_index(int track)
{
	return ((float)track / 2) + 1;
}

static struct d64dsk_tag *get_tag(floppy_image *floppy)
{
	struct d64dsk_tag *tag;
	tag = (d64dsk_tag *)floppy_tag(floppy);
	return tag;
}

static int d64_get_heads_per_disk(floppy_image *floppy)
{
	return get_tag(floppy)->heads;
}

static int d64_get_tracks_per_disk(floppy_image *floppy)
{
	return get_tag(floppy)->tracks;
}

static floperr_t get_track_offset(floppy_image *floppy, int track, UINT64 *offset)
{
	struct d64dsk_tag *tag = get_tag(floppy);
	UINT64 offs = 0;

	if ((track < 0) || (track >= tag->tracks))
		return FLOPPY_ERROR_SEEKERROR;

	offs = tag->track_offset[track];

	if (offset)
		*offset = offs;

	return FLOPPY_ERROR_SUCCESS;
}

static const int bin_2_gcr[] =
{
	0x0a, 0x0b, 0x12, 0x13, 0x0e, 0x0f, 0x16, 0x17,
	0x09, 0x19, 0x1a, 0x1b, 0x0d, 0x1d, 0x1e, 0x15
};

/* This could be of use if we ever implement saving in .d64 format, to convert back GCR -> d64 */
/*
static const int gcr_2_bin[] =
{
    -1, -1,   -1,   -1,
    -1, -1,   -1,   -1,
    -1, 0x08, 0x00, 0x01,
    -1, 0x0c, 0x04, 0x05,
    -1, -1,   0x02, 0x03,
    -1, 0x0f, 0x06, 0x07,
    -1, 0x09, 0x0a, 0x0b,
    -1, 0x0d, 0x0e, -1
};
*/

/* gcr_double_2_gcr takes 4 bytes (a, b, c, d) and shuffles their nibbles to obtain 5 bytes in dest */
/* The result is basically res = (enc(a) << 15) | (enc(b) << 10) | (enc(c) << 5) | enc(d)
 * with res being 5 bytes long and enc(x) being the GCR encode of x.
 * In fact, we store the result as five separate bytes in the dest argument
 */
static void gcr_double_2_gcr(UINT8 a, UINT8 b, UINT8 c, UINT8 d, UINT8 *dest)
{
	UINT8 gcr[8];

	/* Encode each nibble to 5 bits */
	gcr[0] = bin_2_gcr[a >> 4];
	gcr[1] = bin_2_gcr[a & 0x0f];
	gcr[2] = bin_2_gcr[b >> 4];
	gcr[3] = bin_2_gcr[b & 0x0f];
	gcr[4] = bin_2_gcr[c >> 4];
	gcr[5] = bin_2_gcr[c & 0x0f];
	gcr[6] = bin_2_gcr[d >> 4];
	gcr[7] = bin_2_gcr[d & 0x0f];

	/* Re-order the encoded data to only keep the 5 lower bits of each byte */
	dest[0] = (gcr[0] << 3) | (gcr[1] >> 2);
	dest[1] = (gcr[1] << 6) | (gcr[2] << 1) | (gcr[3] >> 4);
	dest[2] = (gcr[3] << 4) | (gcr[4] >> 1);
	dest[3] = (gcr[4] << 7) | (gcr[5] << 2) | (gcr[6] >> 3);
	dest[4] = (gcr[6] << 5) | gcr[7];
}

static floperr_t d64_read_track(floppy_image *floppy, int head, int track, UINT64 offset, void *buffer, size_t buflen)
{
	floperr_t err;
	UINT64 track_offset;

	/* get track offset */
	err = get_track_offset(floppy, track, &track_offset);

	if (err)
		return err;

	if (track_offset != OFFSET_INVALID)
	{
		int gcr_pos = 2, i, sector, d64_pos;
		int id1 = get_tag(floppy)->id1;
		int id2 = get_tag(floppy)->id2;
		int full_track = track / 2;
		int dos_track = full_track + 1;
		int sectors_per_track = d64_sectors_per_track[full_track];

		UINT16 d64_track_size = sectors_per_track * 256;
		UINT16 gcr_track_size = 2 + (sectors_per_track * 368);

		UINT8 d64_track_data[d64_track_size];
		UINT8 gcr_track_data[gcr_track_size];

		UINT8 sector_checksum;

		if (buflen < gcr_track_size) fatalerror("D64 track buffer too small: %u!\n", (UINT32)buflen);

		/* set GCR track size to buffer */
		gcr_track_data[0] = gcr_track_size & 0xff;
		gcr_track_data[1] = gcr_track_size >> 8;

		floppy_image_read(floppy, d64_track_data, track_offset, d64_track_size);

		for (sector = 0; sector < sectors_per_track; sector++)
		{
			// here we convert the sector data to gcr directly!
			// IMPORTANT: we shall implement errors in reading sectors!
			// these can modify e.g. header info $01 & $05

			// first we set the position at which sector data starts in the image
			d64_pos = sector * 256;

			/*
                1. Header sync       FF FF FF FF FF (40 'on' bits, not GCR encoded)
                2. Header info       52 54 B5 29 4B 7A 5E 95 55 55 (10 GCR bytes)
                3. Header gap        55 55 55 55 55 55 55 55 55 (9 bytes, never read)
                4. Data sync         FF FF FF FF FF (40 'on' bits, not GCR encoded)
                5. Data block        55...4A (325 GCR bytes)
                6. Inter-sector gap  55 55 55 55...55 55 (4 to 19 bytes, never read)
            */

			/* Header sync */
			for (i = 0; i < 5; i++)
				gcr_track_data[gcr_pos + i] = 0xff;
			gcr_pos += 5;

			/* Header info */
			/* These are 8 bytes unencoded, which become 10 bytes encoded */
			// $00 - header block ID ($08)                      // this byte can be modified by error code 20 -> 0xff
			// $01 - header block checksum (EOR of $02-$05)     // this byte can be modified by error code 27 -> ^ 0xff
			// $02 - Sector# of data block
			// $03 - Track# of data block
			gcr_double_2_gcr(0x08, sector ^ dos_track ^ id2 ^ id1, sector, dos_track, gcr_track_data + gcr_pos);
			gcr_pos += 5;

			// $04 - Format ID byte #2
			// $05 - Format ID byte #1
			// $06 - $0F ("off" byte)
			// $07 - $0F ("off" byte)
			gcr_double_2_gcr(id2, id1, 0x0f, 0x0f, gcr_track_data + gcr_pos);
			gcr_pos += 5;

			/* Header gap */
			for (i = 0; i < 9; i++)
				gcr_track_data[gcr_pos + i] = 0x55;
			gcr_pos += 9;

			/* Data sync */
			for (i = 0; i < 5; i++)
				gcr_track_data[gcr_pos + i] = 0xff;
			gcr_pos += 5;

			/* Data block */
			// we first need to calculate the checksum of the 256 bytes of the sector
			sector_checksum = d64_track_data[d64_pos];
			for (i = 1; i < 256; i++)
				sector_checksum ^= d64_track_data[d64_pos + i];

			/*
                $00      - data block ID ($07)
                $01-100  - 256 bytes sector data
                $101     - data block checksum (EOR of $01-100)
                $102-103 - $00 ("off" bytes, to make the sector size a multiple of 5)
            */
			gcr_double_2_gcr(0x07, d64_track_data[d64_pos], d64_track_data[d64_pos + 1], d64_track_data[d64_pos + 2], gcr_track_data + gcr_pos);
			gcr_pos += 5;

			for (i = 1; i < 64; i++)
			{
				gcr_double_2_gcr(d64_track_data[d64_pos + 4 * i - 1], d64_track_data[d64_pos + 4 * i],
									d64_track_data[d64_pos + 4 * i + 1], d64_track_data[d64_pos + 4 * i + 2], gcr_track_data + gcr_pos);
				gcr_pos += 5;
			}

			gcr_double_2_gcr(d64_track_data[d64_pos + 255], sector_checksum, 0x00, 0x00, gcr_track_data + gcr_pos);
			gcr_pos += 5;

			/* Inter-sector gap */
			// "In tests that the author conducted on a real 1541 disk, gap sizes of 8  to  19 bytes were seen."
			// Here we put 14 as an average...
			for (i = 0; i < 14; i++)
				gcr_track_data[gcr_pos + i] = 0x55;
			gcr_pos += 14;
		}

		/* copy GCR track data to buffer */
		memcpy(buffer, gcr_track_data, gcr_track_size);
	}
	else	/* half tracks */
	{
		/* set track length to 0 */
		memset(buffer, 0, buflen);
	}

	return FLOPPY_ERROR_SUCCESS;
}

static floperr_t d64_write_track(floppy_image *floppy, int head, int track, UINT64 offset, const void *buffer, size_t buflen)
{
	return FLOPPY_ERROR_UNSUPPORTED;
}

FLOPPY_IDENTIFY( d64_dsk_identify )
{
	UINT64 size = floppy_image_size(floppy);

	if ((size == 0x2ab00) || (size == 0x2ab00 + 0x2ab) || (size == 0x30000) || (size == 0x30000 + 0x300))
	{
		*vote = 100;
	}
	else
	{
		*vote = 0;
	}

	return FLOPPY_ERROR_SUCCESS;
}

FLOPPY_CONSTRUCT( d64_dsk_construct )
{
	struct FloppyCallbacks *callbacks;
	struct d64dsk_tag *tag;
	UINT8 id[2];
	UINT64 size = floppy_image_size(floppy);	// needed to set track #
	int track_offset = 0;
	int track_count = 0;
	int i;

	if (params)
	{
		/* create not supported */
		return FLOPPY_ERROR_UNSUPPORTED;
	}

	tag = (struct d64dsk_tag *) floppy_create_tag(floppy, sizeof(struct d64dsk_tag));

	if (!tag) return FLOPPY_ERROR_OUTOFMEMORY;

	/* number of half tracks */
	if ((size == 0x2ab00) || (size == 0x2ab00 + 0x2ab))
		track_count = D64_MAX_TRACKS;

	if ((size == 0x30000) || (size == 0x30000 + 0x300))
		track_count = D64_40T_MAX_TRACKS;

	if (LOG) logerror("D64 tracks: %u\n", track_count / 2);

	tag->heads = 1;
	tag->tracks = 84;

	/* data offsets */
	for (i = 0; i < tag->tracks; i++)
	{
		if ((i % 2) || (i >= track_count))
		{
			/* half track */
			tag->track_offset[i] = OFFSET_INVALID;
		}
		else
		{
			/* full track */
			tag->track_offset[i] = track_offset;
			track_offset += d64_sectors_per_track[i / 2] * 0x100;
		}

		if (LOG) logerror("D64 track %.1f data offset: %04x\n", get_track_index(i), tag->track_offset[i]);
	}

	/* speed zone offsets */
	for (i = 0; i < tag->tracks; i++)
	{
		if (i < 0x11)
			tag->speed_zone_offset[2 * i] = 3;
		else if (i < 0x18)
			tag->speed_zone_offset[2 * i] = 2;
		else if (i < 0x1e)
			tag->speed_zone_offset[2 * i] = 1;
		else
			tag->speed_zone_offset[2 * i] = 0;

		tag->speed_zone_offset[2 * i + 1] = 0;

		if (LOG) logerror("D64 track %.1f speed zone: %u\n", get_track_index(i), tag->speed_zone_offset[i]);
	}

	floppy_image_read(floppy, id, get_tag(floppy)->track_offset[34] + 0xa2, 2);

	get_tag(floppy)->id1 = id[0];
	get_tag(floppy)->id2 = id[1];

	/* set callbacks */
	callbacks = floppy_callbacks(floppy);

	callbacks->read_track = d64_read_track;
	callbacks->write_track = d64_write_track;
	callbacks->get_heads_per_disk = d64_get_heads_per_disk;
	callbacks->get_tracks_per_disk = d64_get_tracks_per_disk;

	return FLOPPY_ERROR_SUCCESS;
}

/*
id1, id2 are the same for extended d64 (i.e. with error tables), for d67 and for d71

for d81 they are at track 40 bytes 0x17 & 0x18
for d80 & d82 they are at track 39 bytes 0x18 & 0x19
*/
