#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "imgtoolx.h"
#include "formats/cococas.h"
#include "utils.h"

static int cococas_initalt(imgtool_stream *instream, imgtool_stream **outstream, int *basepos,
	int *length, int *channels, int *frequency, int *resolution);
static int cococas_nextfile(imgtool_image *img, imgtool_dirent *ent);
static int cococas_readfile(imgtool_image *img, imgtool_stream *destf);

static UINT8 blockheader[] = { 0x55, 0x3C };

WAVEMODULE(
	cococas,
	"Tandy CoCo Cassette",
	"wav",
	"\r",
	IMGMODULE_FLAG_FILENAMES_PREFERUCASE,
	1200, 1600, 2400,						/* 0=1200 Hz, 1=2400 Hz, threshold=1600 Hz */
	WAVEIMAGE_LSB_FIRST,
	blockheader, sizeof(blockheader) / sizeof(blockheader[0]),
	cococas_initalt,						/* alternate initializer */
	cococas_nextfile,						/* enumerate next */
	cococas_readfile						/* read file */
)

typedef struct {
	UINT8 type;
	UINT8 length;
	UINT8 data[255];
} casblock;

enum {
	COCOCAS_FILETYPE_BASIC = 0,
	COCOCAS_FILETYPE_DATA = 1,
	COCOCAS_FILETYPE_BIN = 2
};

enum {
	COCOCAS_BLOCKTYPE_FILENAME = 0,
	COCOCAS_BLOCKTYPE_DATA = 1,
	COCOCAS_BLOCKTYPE_EOF = 0xff
};

static int readblock(imgtool_image *img, casblock *blk)
{
	int err;
	int i;
	UINT8 sum = 0;
	UINT8 actualsum;

	/* Move forward until we hit header */
	err = imgwave_forward(img);
	if (err)
		return err;

	/* Read in the block type and length */
	err = imgwave_read(img, &blk->type, 2);
	if (err)
		return err;
	sum += blk->type;
	sum += blk->length;

	/* Read in the block data */
	err = imgwave_read(img, blk->data, blk->length);
	if (err)
		return err;

	/* Calculate checksum */
	for (i = 0; i < blk->length; i++)
		sum += blk->data[i];

	/* Read the checksum */
	err = imgwave_read(img, &actualsum, 1);
	if (err)
		return err;

	if (sum != actualsum)
		return IMGTOOLERR_CORRUPTIMAGE;

	return 0;
}

static int cococas_nextfile(imgtool_image *img, imgtool_dirent *ent)
{
	int err, filesize;
	casblock blk;
	char fname[9];

	/* Read directory block */
	err = readblock(img, &blk);
	if (err == IMGTOOLERR_INPUTPASTEND) {
		ent->eof = 1;
		return 0;
	}
	if (err)
		return err;
	
	/* If block is not a filename block, fail */
	if (blk.type != COCOCAS_BLOCKTYPE_FILENAME)
		return IMGTOOLERR_CORRUPTIMAGE;

	fname[8] = '\0';
	memcpy(fname, blk.data, 8);
	rtrim(fname);

	if (strlen(fname) >= ent->fname_len)
		return IMGTOOLERR_BUFFERTOOSMALL;
	strcpy(ent->fname, fname);

	filesize = 0;

	do {
		err = readblock(img, &blk);
		if (err)
			return err;

		if (blk.type == COCOCAS_BLOCKTYPE_DATA) {
			filesize += blk.length;
		}
	}
	while(blk.type == COCOCAS_BLOCKTYPE_DATA);

	if (blk.type != COCOCAS_BLOCKTYPE_EOF)
		return IMGTOOLERR_CORRUPTIMAGE;
	
	ent->filesize = filesize;

	return 0;
}

static int cococas_readfile(imgtool_image *img, imgtool_stream *destf)
{
	int err;
	casblock blk;

	/* Read directory block */
	err = readblock(img, &blk);
	if (err)
		return err;
	
	/* If block is not a filename block, fail */
	if (blk.type != COCOCAS_BLOCKTYPE_FILENAME)
		return IMGTOOLERR_CORRUPTIMAGE;

	do {
		err = readblock(img, &blk);
		if (err)
			return err;

		if (blk.type == COCOCAS_BLOCKTYPE_DATA)
			stream_write(destf, blk.data, blk.length);
	}
	while(blk.type == COCOCAS_BLOCKTYPE_DATA);

	if (blk.type != COCOCAS_BLOCKTYPE_EOF)
		return IMGTOOLERR_CORRUPTIMAGE;
	
	return 0;
}

static int cococas_initalt(imgtool_stream *instream, imgtool_stream **outstream, int *basepos,
	int *length, int *channels, int *frequency, int *resolution)
{
	int caslength;
	int numsamples;
	int samplepos;
	UINT8 *bytes = NULL;
	INT16 *buffer = NULL;
	INT16 *newbuffer;

	caslength = stream_size(instream);
	bytes = malloc(caslength);
	if (!bytes)
		goto outofmemory;
	stream_read(instream, bytes, caslength);

	numsamples = COCO_WAVESAMPLES_HEADER + COCO_WAVESAMPLES_TRAILER + (caslength * 8*8);

	buffer = malloc(numsamples * sizeof(INT16));
	if (!buffer)
		goto outofmemory;

	coco_wave_size = caslength;

	samplepos = 0;
	samplepos += coco_cassette_fill_wave(&buffer[samplepos], COCO_WAVESAMPLES_HEADER, CODE_HEADER);
	samplepos += coco_cassette_fill_wave(&buffer[samplepos], caslength, bytes);
	samplepos += coco_cassette_fill_wave(&buffer[samplepos], COCO_WAVESAMPLES_TRAILER, CODE_TRAILER);

	free(bytes);
	bytes = NULL;

	newbuffer = realloc(buffer, samplepos * sizeof(INT16));
	if (!newbuffer)
		goto outofmemory;
	buffer = newbuffer;

	*outstream = stream_open_mem(buffer, samplepos * sizeof(INT16));
	if (!(*outstream))
		goto outofmemory;

	*basepos = 0;
	*length = samplepos * 2;
	*channels = 1;
	*frequency = 4800;
	*resolution = 16;
	return 0;

outofmemory:
	if (bytes)
		free(bytes);
	if (buffer)
		free(buffer);
	return IMGTOOLERR_OUTOFMEMORY;
}



