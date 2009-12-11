/*

  Bytes: $0000-0007: File signature "GCR-1541"
               0008: G64 version (presently only $00 defined)
               0009: Number of tracks in image (usually $54, decimal 84)
          000A-000B: Size of each stored track in bytes (usually  7928,  or
                     $1EF8 in LO/HI format.

  Bytes: $000C-000F: Offset  to  stored  track  1.0  ($000002AC,  in  LO/HI
                     format, see below for more)
          0010-0013: Offset to stored track 1.5 ($00000000)
          0014-0017: Offset to stored track 2.0 ($000021A6)
             ...
          0154-0157: Offset to stored track 42.0 ($0004F8B6)
          0158-015B: Offset to stored track 42.5 ($00000000)

  Bytes: $015C-015F: Speed zone entry for track 1 ($03,  in  LO/HI  format,
                     see below for more)
          0160-0163: Speed zone entry for track 1.5 ($03)
             ...
          02A4-02A7: Speed zone entry for track 42 ($00)
          02A8-02AB: Speed zone entry for track 42.5 ($00)

  Bytes: $02AC-02AD: Actual size of stored track (7692 or $1E0C,  in  LO/HI
                     format)
          02AE-02AE+$1E0C: Track data

*/

#include "driver.h"
#include "formats/flopimg.h"
#include "devices/flopdrv.h"

#define G64_DSK_TAG		"g64tag"

#define HEADER_LENGTH	0x2ac		 /* standard length for 84 half tracks */
#define HALF_TRACKS		84

struct g64dsk_tag
{
	int version;
	int tracks;
	UINT16 track_size;
	UINT32 track_offset[HALF_TRACKS];		/* offset within data for each half track */
	UINT32 speed_zone_offset[HALF_TRACKS]; /* offset within data for each half track */
};
/*
static struct g64dsk_tag *get_tag(floppy_image *floppy)
{
	struct g64dsk_tag *tag;
	tag = floppy_tag(floppy, G64_DSK_TAG);
	return tag;
}
*/
FLOPPY_IDENTIFY( g64_dsk_identify )
{
	UINT8 header[8];

	floppy_image_read(floppy, header, 0, 8);

	if (!strncmp((const char *)header, "GCR-1541", 8))
	{
		*vote = 100;
	}
	else
	{
		*vote = 0;
	}

	return FLOPPY_ERROR_SUCCESS;
}

FLOPPY_CONSTRUCT( g64_dsk_construct )
{
	struct g64dsk_tag *tag;
	UINT8 header[HEADER_LENGTH];
	UINT64 pos = 0;
	int i;

	if (params)
	{
		/* create not supported */
		return FLOPPY_ERROR_UNSUPPORTED;
	}

	tag = (struct g64dsk_tag *) floppy_create_tag(floppy, G64_DSK_TAG, sizeof(struct g64dsk_tag));

	if (!tag) return FLOPPY_ERROR_OUTOFMEMORY;

	/* version */
	floppy_image_read(floppy, header, pos, 0x0c); pos += 0xc;
	tag->version = header[8];
	logerror("G64 version: %u\n", tag->version);

	/* number of half tracks */
	tag->tracks = header[9];
	logerror("G64 tracks: %u\n", tag->tracks);

	/* size of each stored half track */
	tag->track_size = (header[11] << 8) | header[10];
	logerror("G64 track size: %04x\n", tag->track_size);

	/* data offsets */
	for (i = 0; i < tag->tracks; i++)
	{
		floppy_image_read(floppy, header, pos, 4); pos += 4;
		tag->track_offset[i] = (header[3] << 24) | (header[2] << 16) | (header[1] << 8) | header[0];
		logerror("G64 track %f data offset: %04x\n", ((i * 0.5) + 1), tag->track_offset[i]);
	}

	/* speed zone offsets */
	for (i = 0; i < tag->tracks; i++)
	{
		floppy_image_read(floppy, header, pos, 4); pos += 4;
		tag->speed_zone_offset[i] = (header[3] << 24) | (header[2] << 16) | (header[1] << 8) | header[0];
		logerror("G64 track %f speed zone offset: %04x\n", ((i * 0.5) + 1), tag->speed_zone_offset[i]);
	}

	return FLOPPY_ERROR_SUCCESS;
}
