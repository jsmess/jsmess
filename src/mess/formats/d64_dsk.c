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
    Code  Error  Type   1541 error description
    ----  -----  ----   ------------------------------
     01    00    N/A    No error, Sektor ok.
     02    20    Read   Header block not found
     03    21    Seek   No sync character
     04    22    Read   Data block not present
     05    23    Read   Checksum error in data block
     06    24    Write  Write verify (on format)
     07    25    Write  Write verify error
     08    26    Write  Write protect on
     09    27    Seek   Checksum error in header block
     0A    28    Write  Write error
     0B    29    Seek   Disk ID mismatch
     0F    74    Read   Disk Not Ready (no device 1)
*/

/*

    TODO:

	- D71
	- cleanup
	- dos2_encode_gcr() function
    - disk errors
	- D80/D82
	- variable gaps

*/

#include "emu.h"
#include "formats/flopimg.h"
#include "formats/d64_dsk.h"
#include "devices/flopdrv.h"

#define LOG 1

#define HALF_TRACKS		84
#define SECTOR_SIZE		256

#define OFFSET_INVALID	0xbadbad

#define D71_MAX_TRACKS      70
#define D81_MAX_TRACKS		80
#define D80_MAX_TRACKS		77
/* Still have to investigate if these are read as 154 or as 77 in the drive... */
/* For now we assume it's 154, but I'm not sure */
#define D82_MAX_TRACKS		154

#define D64_SIZE_35_TRACKS				174848
#define D64_SIZE_35_TRACKS_WITH_ERRORS	175531
#define D64_SIZE_40_TRACKS				196608
#define D64_SIZE_40_TRACKS_WITH_ERRORS	197376
#define D64_SIZE_42_TRACKS				205312
#define D64_SIZE_42_TRACKS_WITH_ERRORS	206114
#define D71_SIZE_70_TRACKS				349696
#define D71_SIZE_70_TRACKS_WITH_ERRORS	351062

static const int D64_SECTORS_PER_TRACK[] =
{
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17,
	17, 17, 17, 17, 17,
	17, 17
};

static const int D64_SPEED_ZONE[] =
{
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2, 2, 2,
	1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#if 0

/*

	CBM8050/8250 Format

	29sectors/track for track 1-39/78-116
	27sectors/track for track 40-53/117-130
	25sectors/track for track 54-64/131-141
	23sectors/track for track 65-77/142-154

	BAM on track 38 (8050 2 blocks, 8250 4 blocks, rest is free for files)
	DIR on track 39 (1 block for header, 28 blocks for dir entries)

*/

static const int D80_SECTORS_PER_TRACK[] =
{
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23
};

static const int D82_SECTORS_PER_TRACK[] =
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
#endif

struct d64dsk_tag
{
	int heads;
	int tracks;
	int track_offset[2][HALF_TRACKS];	/* offset within data for each half track */
	UINT32 speed_zone[HALF_TRACKS];		/* speed zone for each half track */
	int error[HALF_TRACKS];				/* DOS error code for each half track */

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

static floperr_t get_track_offset(floppy_image *floppy, int head, int track, UINT64 *offset)
{
	struct d64dsk_tag *tag = get_tag(floppy);
	UINT64 offs = 0;

	if ((track < 0) || (track >= tag->tracks))
		return FLOPPY_ERROR_SEEKERROR;

	offs = tag->track_offset[head][track];

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
	err = get_track_offset(floppy, head, track, &track_offset);
logerror("D64 read head %u track %.1f\n", head, get_track_index(track));
	if (err)
		return err;

	if (track_offset != OFFSET_INVALID)
	{
		int gcr_pos = 2, i, sector, d64_pos;
		int id1 = get_tag(floppy)->id1;
		int id2 = get_tag(floppy)->id2;
		int full_track = track / 2;
		int dos_track = full_track + 1;
		int sectors_per_track = D64_SECTORS_PER_TRACK[full_track];

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

static void d64_identify(floppy_image *floppy, int *heads, int *tracks, bool *has_errors)
{
	switch (floppy_image_size(floppy))
	{
	case D64_SIZE_35_TRACKS:				*heads = 1;	*tracks = 35; *has_errors = false; break;
	case D64_SIZE_35_TRACKS_WITH_ERRORS:	*heads = 1;	*tracks = 35; *has_errors = true;  break;
	case D64_SIZE_40_TRACKS:				*heads = 1; *tracks = 40; *has_errors = false; break;
	case D64_SIZE_40_TRACKS_WITH_ERRORS:	*heads = 1;	*tracks = 40; *has_errors = true;  break;
	case D64_SIZE_42_TRACKS:				*heads = 1;	*tracks = 42; *has_errors = false; break;
	case D64_SIZE_42_TRACKS_WITH_ERRORS:	*heads = 1;	*tracks = 42; *has_errors = true;  break;
	case D71_SIZE_70_TRACKS:				*heads = 2;	*tracks = 35; *has_errors = false; break;
	case D71_SIZE_70_TRACKS_WITH_ERRORS:	*heads = 2;	*tracks = 35; *has_errors = true;  break;
	}
}

FLOPPY_IDENTIFY( d64_dsk_identify )
{
	int heads = 0, tracks = 0;
	bool has_errors = false;

	*vote = 0;

	d64_identify(floppy, &heads, &tracks, &has_errors);

	if (heads == 1 && (tracks == 35 || tracks == 40 || tracks == 42))
	{
		*vote = 100;
	}

	return FLOPPY_ERROR_SUCCESS;
}

FLOPPY_IDENTIFY( d71_dsk_identify )
{
	int heads = 0, tracks = 0;
	bool has_errors = false;

	*vote = 0;

	d64_identify(floppy, &heads, &tracks, &has_errors);

	if (heads == 2 && tracks == 35)
	{
		*vote = 100;
	}

	return FLOPPY_ERROR_SUCCESS;
}

FLOPPY_CONSTRUCT( d64_dsk_construct )
{
	struct FloppyCallbacks *callbacks;
	struct d64dsk_tag *tag;
	UINT8 id[2];

	int track_offset = 0;
	int head, track;
	
	int heads = 0, dos_tracks = 0;
	bool has_errors;

	if (params)
	{
		/* create not supported */
		return FLOPPY_ERROR_UNSUPPORTED;
	}

	tag = (struct d64dsk_tag *) floppy_create_tag(floppy, sizeof(struct d64dsk_tag));

	if (!tag) return FLOPPY_ERROR_OUTOFMEMORY;

	/* identify image type */
	d64_identify(floppy, &heads, &dos_tracks, &has_errors);

	tag->heads = heads;
	tag->tracks = HALF_TRACKS;

	/* read format ID from directory */
	floppy_image_read(floppy, id, get_tag(floppy)->track_offset[0][34] + 0xa2, 2);

	get_tag(floppy)->id1 = id[0];
	get_tag(floppy)->id2 = id[1];

	if (LOG)
	{
		logerror("D64 size: %04x\n", (UINT32)floppy_image_size(floppy));
		logerror("D64 heads: %u\n", heads);
		logerror("D64 tracks: %u\n", dos_tracks);
		logerror("D64 format ID: %02x%02x\n", id[0], id[1]);
	}

	/* data offsets */
	for (head = 0; head < heads; head++)
	{
		for (track = 0; track < tag->tracks; track++)
		{
			if ((track % 2) || ((track / 2) >= dos_tracks))
			{
				/* half track or out of range */
				tag->track_offset[head][track] = OFFSET_INVALID;
			}
			else
			{
				/* full track */
				tag->track_offset[head][track] = track_offset;
				track_offset += D64_SECTORS_PER_TRACK[track / 2] * SECTOR_SIZE;
			}

			if (LOG) logerror("D64 head %u track %.1f data offset: %04x\n", head, get_track_index(track), tag->track_offset[head][track]);
		}
	}

	/* speed zones */
	for (track = 0; track < tag->tracks; track++)
	{
		tag->speed_zone[track] = D64_SPEED_ZONE[track / 2];

		if (LOG) logerror("D64 track %.1f speed zone: %u\n", get_track_index(track), tag->speed_zone[track]);
	}

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
