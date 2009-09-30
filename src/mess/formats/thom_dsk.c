/*********************************************************************

    formats/thom_dsk.c

    Thomson disk images

    Based on work of  Antoine Mine'

*********************************************************************/

#include <string.h>
#include "driver.h"
#include "formats/thom_dsk.h"
#include "formats/basicdsk.h"

#define SAP_DSK_TAG	"sapdsktag"

static const int sap_magic_num = 0xB3; /* simple XOR crypt */


static const char sap_header[] =
	"SYSTEME D'ARCHIVAGE PUKALL S.A.P. "
	"(c) Alexandre PUKALL Avril 1998";


static const UINT16 sap_crc[] =
{
	0x0000, 0x1081, 0x2102, 0x3183,   0x4204, 0x5285, 0x6306, 0x7387,
	0x8408, 0x9489, 0xa50a, 0xb58b,   0xc60c, 0xd68d, 0xe70e, 0xf78f,
};

struct sap_dsk_tag
{
	int heads;
	int tracks;
	int sector_size;
};


static struct sap_dsk_tag *get_tag(floppy_image *floppy)
{
	struct sap_dsk_tag *tag;
	tag = floppy_tag(floppy, SAP_DSK_TAG);
	return tag;
}


static FLOPPY_IDENTIFY(sap_dsk_identify)
{
	char header[0x100];
	floppy_image_read(floppy, header, 1, sizeof(sap_header));
	if (!memcmp( header, sap_header, sizeof(sap_header) ) )
	{
		*vote= 100;
	} else {
		*vote = 0;
	}
	return FLOPPY_ERROR_SUCCESS;
}
static int sap_get_heads_per_disk(floppy_image *floppy)
{
	return 1;
}

static int sap_get_tracks_per_disk(floppy_image *floppy)
{
	return get_tag(floppy)->tracks;
}


static floperr_t get_offset(floppy_image *floppy, int head, int track, int sector, int sector_is_index, UINT64 *offset)
{
	UINT64 offs;
	/* translate the sector to a raw sector */
	if (!sector_is_index)
	{
		sector -= 1;
	}
	/* check to see if we are out of range */
	if ((head < 0) || (head >= 1) || (track < 0) || (track >=get_tag(floppy)->tracks)
			|| (sector < 0) || (sector >= 16))
		return FLOPPY_ERROR_SEEKERROR;

	offs = 0x42 + track * ((get_tag(floppy)->sector_size + 6)* 16) + sector * (get_tag(floppy)->sector_size + 6) + 4;
	if (offset)
		*offset = offs;
	return FLOPPY_ERROR_SUCCESS;
}



static floperr_t internal_sap_read_sector(floppy_image *floppy, int head, int track, int sector, int sector_is_index, void *buffer, size_t buflen)
{
	UINT64 offset;
	floperr_t err;
	int i;
	UINT8 *buf;
	err = get_offset(floppy, head, track, sector, sector_is_index, &offset);
	if (err)
		return err;

	floppy_image_read(floppy, buffer, offset, buflen);
	buf = (UINT8*)buffer;
	for (i=0;i<buflen;i++) {
		buf[i] ^= sap_magic_num;
	}
	return FLOPPY_ERROR_SUCCESS;
}



static floperr_t internal_sap_write_sector(floppy_image *floppy, int head, int track, int sector, int sector_is_index, const void *buffer, size_t buflen, int ddam)
{
	UINT64 offset;
	floperr_t err;
	UINT8 *buf;
	int i;

	err = get_offset(floppy, head, track, sector, sector_is_index, &offset);
	if (err)
		return err;

	buf = (UINT8*)buffer;
	for (i=0;i<buflen;i++) {
		buf[i] ^= sap_magic_num;
	}
	floppy_image_write(floppy, buffer, offset, buflen);
	return FLOPPY_ERROR_SUCCESS;
}


static floperr_t sap_read_sector(floppy_image *floppy, int head, int track, int sector, void *buffer, size_t buflen)
{
	return internal_sap_read_sector(floppy, head, track, sector, FALSE, buffer, buflen);
}

static floperr_t sap_write_sector(floppy_image *floppy, int head, int track, int sector, const void *buffer, size_t buflen, int ddam)
{
	return internal_sap_write_sector(floppy, head, track, sector, FALSE, buffer, buflen, ddam);
}

static floperr_t sap_read_indexed_sector(floppy_image *floppy, int head, int track, int sector, void *buffer, size_t buflen)
{
	return internal_sap_read_sector(floppy, head, track, sector, TRUE, buffer, buflen);
}

static floperr_t sap_write_indexed_sector(floppy_image *floppy, int head, int track, int sector, const void *buffer, size_t buflen, int ddam)
{
	return internal_sap_write_sector(floppy, head, track, sector, TRUE, buffer, buflen, ddam);
}

static floperr_t sap_get_sector_length(floppy_image *floppy, int head, int track, int sector, UINT32 *sector_length)
{
	floperr_t err;
	err = get_offset(floppy, head, track, sector, FALSE, NULL);
	if (err)
		return err;

	if (sector_length) {
		*sector_length = get_tag(floppy)->sector_size;
	}
	return FLOPPY_ERROR_SUCCESS;
}



static floperr_t sap_get_indexed_sector_info(floppy_image *floppy, int head, int track, int sector_index, int *cylinder, int *side, int *sector, UINT32 *sector_length, unsigned long *flags)
{
	floperr_t err;
	UINT8 header[4];
	UINT64 offset = 0;
	sector_index += 1;
	err = get_offset(floppy, head, track, sector_index, FALSE, &offset);

	floppy_image_read(floppy, header, offset-4, 4);
	if (cylinder)
		*cylinder = header[2];
	if (side)
		*side = head;
	if (sector)
		*sector = header[3];
	if (sector_length)
		*sector_length = get_tag(floppy)->sector_size;
	if (flags)
		/* TODO: read DAM or DDAM and determine flags */
		*flags = 0;
	return err;
}


static FLOPPY_CONSTRUCT(sap_dsk_construct)
{
	struct FloppyCallbacks *callbacks;
	struct sap_dsk_tag *tag;
	int i;
	UINT8 header[4];
	tag = (struct sap_dsk_tag *) floppy_create_tag(floppy, SAP_DSK_TAG, sizeof(struct sap_dsk_tag));
	if (!tag)
		return FLOPPY_ERROR_OUTOFMEMORY;

	floppy_image_read(floppy, header, 0, 1);
	if (header[0]==1) tag->sector_size = 256; else tag->sector_size = 128;
	for ( i = 0x42; i+4 < floppy_image_size(floppy); i += tag->sector_size + 6 ) // CRC 2 bytes + 4 bytes sector header
	{
		floppy_image_read(floppy, header, i, 4);
		tag->tracks = header[2] + 1;
	}
	callbacks = floppy_callbacks(floppy);
	callbacks->read_sector = sap_read_sector;
	callbacks->write_sector = sap_write_sector;
	callbacks->read_indexed_sector = sap_read_indexed_sector;
	callbacks->write_indexed_sector = sap_write_indexed_sector;
	callbacks->get_sector_length = sap_get_sector_length;
	callbacks->get_heads_per_disk = sap_get_heads_per_disk;
	callbacks->get_tracks_per_disk = sap_get_tracks_per_disk;
	callbacks->get_indexed_sector_info = sap_get_indexed_sector_info;

	return FLOPPY_ERROR_SUCCESS;
}

static FLOPPY_IDENTIFY(qdd_dsk_identify)
{
	*vote = (floppy_image_size(floppy) == (51200)) ? 100 : 0;
	return FLOPPY_ERROR_SUCCESS;
}

/* fixed interlacing map for QDDs */
static int thom_qdd_map[400];

static int qdd_translate_sector(floppy_image *floppy, int sector)
{
	return thom_qdd_map[sector-1];
}

static void thom_qdd_compute_map ( void )
{
	/* this map is hardcoded in the QDD BIOS */
	static const int p[6][4] =
	{
		{ 20,  2, 14,  8 }, { 21, 19, 13,  7 },
		{ 22, 18, 12,  6 }, { 23, 17, 11,  5 },
		{ 24, 16, 10,  4 }, {  1, 15,  9,  3 }
	};
	static const int q[4] = { 0, 8, 4, 12 };
	int t, s;
	for ( t = 0; t < 24; t++ )
	{
		for ( s = 0; s < 16; s++ )
		{
			thom_qdd_map[ t*16 + s ] = p[ t/4 ][ s%4 ] * 16 + (s/4) + 4*(t%4);
		}
	}
	for ( s = 0; s < 16; s++ )
	{
		thom_qdd_map[ 24*16 + s ] = q[ s%4 ] + (s/4);
	}
}

static FLOPPY_CONSTRUCT(qdd_dsk_construct)
{
	struct basicdsk_geometry geometry;

	thom_qdd_compute_map();

	memset(&geometry, 0, sizeof(geometry));
	geometry.heads = 1;
	geometry.first_sector_id = 1;
	geometry.sector_length = 128;
	geometry.tracks = 1;
	geometry.sectors = 400;
	geometry.translate_sector =  qdd_translate_sector;
	return basicdsk_construct(floppy, &geometry);
}


/* ----------------------------------------------------------------------- */

FLOPPY_OPTIONS_START(thomson)
	FLOPPY_OPTION(qdd,"qd", "Thomson QDD floppy disk image",	qdd_dsk_identify, qdd_dsk_construct,
		HEADS([1])
		TRACKS([1])
		SECTORS([400])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([0]))
	FLOPPY_OPTION(sap,"sap", "Thomson SAP floppy disk image",	sap_dsk_identify, sap_dsk_construct, NULL)
	FLOPPY_OPTION(fd, "fd", "Thomson FD (FM) 40 tracks disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([16])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(fd2, "fd", "Thomson FD (FM) 80 tracks disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(fdmfm, "fd", "Thomson FD (MFM) 40 tracks disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(fdmfm2, "fd", "Thomson FD (MFM) 80 tracks disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

