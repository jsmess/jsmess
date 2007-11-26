#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "formats/svi_cas.h"
#include "imgtoolx.h"
#include "utils.h"

/*
	SVI module

	SVI style .cas for the svi-318 and svi-328. Converts them to .wav files. 
	Uses the mess/formats/svi_cas.[ch] files, for the actual conversion.
*/

typedef struct {
	imgtool_image			base;
	char			*file_name;
	imgtool_stream 			*file_handle;
	int 			size;
	unsigned char	*data;
	INT16			*wavdata;
	int 			count;
	} CAS_IMAGE;

typedef struct
	{
	imgtool_directory 	base;
	CAS_IMAGE	*image;
	int			index;
	} CAS_ITERATOR;

static int svi_cas_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg);
static void svi_cas_image_exit(imgtool_image *img);
//static void svi_cas_image_info(imgtool_image *img, char *string, const int len);
static int svi_cas_image_beginenum(imgtool_image *img, imgtool_directory **outenum);
static int svi_cas_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent);
static void svi_cas_image_closeenum(imgtool_directory *enumeration);
static int svi_cas_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf);

IMAGEMODULE(
	svi_cas,
	"SVI-318/SVI-328 .cas file",				/* human readable name */
	"cas",								/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,
	svi_cas_image_init,				/* init function */
	svi_cas_image_exit,				/* exit function */
	NULL,								/* info function */
	svi_cas_image_beginenum,			/* begin enumeration */
	svi_cas_image_nextenum,			/* enumerate next */
	svi_cas_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	svi_cas_image_readfile,			/* read file */
	NULL,/* write file */
	NULL,/* delete file */
	NULL,/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,					/* file options */
	NULL					/* create options */
)

static int svi_cas_image_init(const imgtool_module *mod, imgtool_stream *f, imgtool_image **outimg)
	{
	CAS_IMAGE *image;
	int len, i;
	char *pbase;
	char default_name[] = "svitape";

	image = (CAS_IMAGE*)malloc (sizeof (CAS_IMAGE) );
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	*outimg = (imgtool_image*)image;

	memset(image, 0, sizeof(CAS_IMAGE));
	image->base.module = mod;
	image->size=stream_size(f);
	image->file_handle=f;

    if (image->size < 17)
		{
		free (image);
		return IMGTOOLERR_CORRUPTIMAGE;
		}

	image->data = (unsigned char *) malloc(image->size);
	if ( (!image->data)
		 ||(stream_read(f, image->data, image->size)!=image->size) )
		{
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
		}

  	i = svi_cas_to_wav (image->data, image->size, &image->wavdata, &image->count);
  	if (i)
		{
		free(image->data);
		free(image);
		*outimg=NULL;
		return (i == 2) ? IMGTOOLERR_OUTOFMEMORY : IMGTOOLERR_CORRUPTIMAGE;
		}

    pbase = NULL;
    if (pbase) len = strlen (pbase);
    else len = strlen (default_name);

    image->file_name = malloc (len + 5);
	if (!image->file_name)
		{
		free(image->data);
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_CORRUPTIMAGE;
		}

    if (!pbase)
		strcpy (image->file_name, default_name);
    else
		{
		strcpy (image->file_name, pbase);
        if (len > 4)
			if (!strncmpi (".cas", image->file_name + len - 4, 4) ) len -= 4;

		strcpy (image->file_name + len, ".wav");
		}

	return 0;
	}

static void svi_cas_image_exit(imgtool_image *img)
	{
	CAS_IMAGE *image=(CAS_IMAGE*)img;
	stream_close(image->file_handle);
	free(image->file_name);
	free(image->data);
	free(image->wavdata);
	free(image);
	}

static int svi_cas_image_beginenum(imgtool_image *img, imgtool_directory **outenum)
	{
	CAS_IMAGE *image=(CAS_IMAGE*)img;
	CAS_ITERATOR *iter;

	iter=*(CAS_ITERATOR**)outenum = (CAS_ITERATOR*) malloc(sizeof(CAS_ITERATOR));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;

	iter->image=image;
	iter->index = 0;
	return 0;
	}

static int svi_cas_image_nextenum(imgtool_directory *enumeration, imgtool_dirent *ent)
	{
	CAS_ITERATOR *iter=(CAS_ITERATOR*)enumeration;

	ent->eof=iter->index;
	if (!ent->eof)
		{
		strcpy (ent->fname, iter->image->file_name);
		ent->corrupt=0;
		ent->filesize = iter->image->count * 2 + 0x2c;
		iter->index++;
		}

	return 0;
	}

static void svi_cas_image_closeenum(imgtool_directory *enumeration)
	{
	free(enumeration);
	}

static int svi_cas_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf)
	{
	CAS_IMAGE *image=(CAS_IMAGE*)img;
	INT16 *wavdata;
	UINT16	temp16;
	UINT32	temp32;
	int wavlen, offset;

	if (mame_stricmp (fname, image->file_name) )
		return IMGTOOLERR_MODULENOTFOUND;

	wavdata = image->wavdata;
	wavlen = image->count;
    /* write the core header for a WAVE file */
	offset = stream_write(destf, "RIFF", 4);
    if( offset < 4 )
		return IMGTOOLERR_WRITEERROR;

	temp32 = LITTLE_ENDIANIZE_INT32(image->count * 2 + 0x24);
	offset += stream_write(destf, &temp32, 4);
    if( offset < 8 )
		return IMGTOOLERR_WRITEERROR;

	/* read the RIFF file type and make sure it's a WAVE file */
	offset += stream_write(destf, "WAVEfmt ", 8);
	if( offset < 16 )
		return IMGTOOLERR_WRITEERROR;

    /* size of the following 'fmt ' fields */
    offset += stream_write(destf, "\x10\x00\x00\x00", 4);
	if( offset < 20 )
		return IMGTOOLERR_WRITEERROR;

	/* format: PCM */
	temp16 = LITTLE_ENDIANIZE_INT16 (1);
	offset += stream_write(destf, &temp16, 2);
	if( offset < 22 )
		return IMGTOOLERR_WRITEERROR;

	/* channels: 1 (mono) */
	temp16 = LITTLE_ENDIANIZE_INT16 (1);
	offset += stream_write(destf, &temp16, 2);
	if( offset < 24 )
		return IMGTOOLERR_WRITEERROR;

	/* sample rate */
	temp32 = LITTLE_ENDIANIZE_INT32(22050);
	offset += stream_write(destf, &temp32, 4);
	if( offset < 24 )
		return IMGTOOLERR_WRITEERROR;

	/* byte rate */
	temp32 = LITTLE_ENDIANIZE_INT32(22050 * 2);
	offset += stream_write(destf, &temp32, 4);
	if( offset < 28 )
		return IMGTOOLERR_WRITEERROR;

	/* block align (size of one `sample') */
	temp16 = LITTLE_ENDIANIZE_INT16 (2);
	offset += stream_write(destf, &temp16, 2);
	if( offset < 30 )
		return IMGTOOLERR_WRITEERROR;

	/* block align */
	temp16 = LITTLE_ENDIANIZE_INT16 (16);
	offset += stream_write(destf, &temp16, 2);
	if( offset < 32 )
		return IMGTOOLERR_WRITEERROR;

	/* 'data' tag */
	offset += stream_write(destf, "data", 4);
	if( offset < 36 )
		return IMGTOOLERR_WRITEERROR;

	/* data size */
	temp32 = LITTLE_ENDIANIZE_INT32(image->count * 2);
	offset += stream_write(destf, &temp32, 4);
	if( offset < 40 )
		return IMGTOOLERR_WRITEERROR;

	if (stream_write(destf, wavdata, (image->count*2))!=(image->count*2))
		return IMGTOOLERR_WRITEERROR;

	return 0;
	}

